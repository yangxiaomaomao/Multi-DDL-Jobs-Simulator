#include "ddl-topo.h"
#include "ddl-tools.h"

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/network-module.h"
#include "ns3/pfifo-fast-queue-disc.h"
#include "ns3/ping-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-helper.h"

#include <iostream>
using namespace std;

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("spineLeafTopo");

spineLeafTopo::spineLeafTopo(string topoConfigFilename)
{
    NS_LOG_FUNCTION(this);
    InitTopoConfig(topoConfigFilename);
    // create nodes and link them
    CreateNodes();
    SetupLink();
    // install device and netstack
    InstallSpineLeafLinks();
    InstallLeafGpuLinks();
    InstallInternetStack();

    // config the queue
    InitQueueDisp();
    ConfigureQueueDisp();
    // assign ip and ensure the alltoall connectivity
    AssignIpAddresses();
    ConfigureRoute();

    // captureLeafGpuPackets(0, 0);

    // ping test
    // pingAllGpuTest();
}

void
spineLeafTopo::InitTopoConfig(std::string topoConfigFilename)
{
    std::ifstream m_topoConfig(topoConfigFilename);
    if (!m_topoConfig.is_open())
    {
        std::cerr << "Failed to open file: " << topoConfigFilename << std::endl;
        exit(0);
    }

    std::string line;
    std::getline(m_topoConfig, line); // 读取并跳过表头
    while (std::getline(m_topoConfig, line))
    {
        std::stringstream ss(line);
        std::string token;
        std::getline(ss, token, ',');
        m_spineNum = std::stoi(token);
        std::getline(ss, token, ',');
        m_leafNum = std::stoi(token);
        std::getline(ss, token, ',');
        m_gpuNumPerLeaf = std::stoi(token);
        std::getline(ss, token, ',');
        m_spineLeafBandwidth = std::stof(token);
        std::getline(ss, token, ',');
        m_leafGpuBandwidth = std::stof(token);
        std::getline(ss, token, ',');
        m_loadBalanceStrategy = token;
    }
    m_topoConfig.close();
    printColoredText("************Spine-Leaf Topo Config************", "blue");
    printColoredText("Spine Num: " + to_string(m_spineNum), "green");
    printColoredText("Leaf Num: " + to_string(m_leafNum), "green");
    printColoredText("GPU Num Per Leaf: " + to_string(m_gpuNumPerLeaf), "green");
    printColoredText("Spine-Leaf Bandwidth: " + to_string(m_spineLeafBandwidth) + "MBps", "green");
    printColoredText("Leaf-GPU Bandwidth: " + to_string(m_leafGpuBandwidth) + "MBps", "green");
    printColoredText("Load Balance Strategy: " + m_loadBalanceStrategy, "green");
}

void
spineLeafTopo::CreateNodes()
{
    NS_LOG_FUNCTION(this);
    m_spineNodes.Create(m_spineNum);
    m_leafNodes.Create(m_leafNum);
    m_gpuNodes.Create(m_leafNum * m_gpuNumPerLeaf);
    for (uint32_t i = 0; i < m_leafNum * m_gpuNumPerLeaf; ++i)
    {
        m_freeNodeIndex.push_back(i);
    }
}

void
spineLeafTopo::SetupLink()
{
    NS_LOG_FUNCTION(this);
    m_spineLeafLink.SetDeviceAttribute("DataRate",
                                       StringValue(to_string(m_spineLeafBandwidth) + "MBps"));
    m_spineLeafLink.SetChannelAttribute("Delay", StringValue("0ms"));
    m_leafGpuLink.SetDeviceAttribute("DataRate",
                                     StringValue(to_string(m_leafGpuBandwidth) + "MBps"));
    m_leafGpuLink.SetChannelAttribute("Delay", StringValue("0ms"));
}

void
spineLeafTopo::InstallSpineLeafLinks()
{
    NS_LOG_FUNCTION(this);
    m_spineLeafDevices.resize(m_spineNum, std::vector<NetDeviceContainer>(m_leafNum));
    uint32_t spineIndex = 0;
    for (auto spineNode = m_spineNodes.Begin(); spineNode != m_spineNodes.End();
         ++spineNode, ++spineIndex)
    {
        {
            uint32_t leafIndex = 0;
            for (auto leafNode = m_leafNodes.Begin(); leafNode != m_leafNodes.End();
                 ++leafNode, ++leafIndex)
            {
                NetDeviceContainer spineLeafDevice = m_spineLeafLink.Install(*spineNode, *leafNode);
                m_spineLeafDevices[spineIndex][leafIndex] = spineLeafDevice;
                Ptr<NetDevice> d = spineLeafDevice.Get(0);
                Ptr<Node> n = d->GetNode();
            }
        }
    }
}

void
spineLeafTopo::InstallLeafGpuLinks()
{
    NS_LOG_FUNCTION(this);
    m_leafGpuDevices.resize(m_leafNum, std::vector<NetDeviceContainer>(m_gpuNumPerLeaf));
    uint32_t leafIndex = 0;
    for (auto leafNode = m_leafNodes.Begin(); leafNode != m_leafNodes.End();
         ++leafNode, ++leafIndex)
    {
        uint32_t gpuIndex = 0;
        for (gpuIndex = 0; gpuIndex < m_gpuNumPerLeaf; ++gpuIndex)
        {
            m_leafGpuDevices[leafIndex][gpuIndex] =
                m_leafGpuLink.Install(*leafNode,
                                      m_gpuNodes.Get(leafIndex * m_gpuNumPerLeaf + gpuIndex));
        }
    }
}

void
spineLeafTopo::InstallInternetStack()
{
    NS_LOG_FUNCTION(this);
    InternetStackHelper stack;
    stack.InstallAll();
}

void
spineLeafTopo::InitQueueDisp()
{
    NS_LOG_FUNCTION(this);
    // m_queueDisp.SetRootQueueDisc("ns3::PfifoFastQueueDisc", "MaxSize", StringValue("10000p"));
    uint16_t handle = m_queueDisp.SetRootQueueDisc("ns3::PrioQueueDisc",
                                                   "Priomap",
                                                   StringValue("0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7"));
    TrafficControlHelper::ClassIdList cid =
        m_queueDisp.AddQueueDiscClasses(handle, 8, "ns3::QueueDiscClass");
    m_queueDisp.AddChildQueueDisc(handle, cid[0], "ns3::FifoQueueDisc", "MaxSize", StringValue("100000p"));
    m_queueDisp.AddChildQueueDisc(handle, cid[1], "ns3::FifoQueueDisc", "MaxSize", StringValue("100000p"));
    m_queueDisp.AddChildQueueDisc(handle, cid[2], "ns3::FifoQueueDisc", "MaxSize", StringValue("100000p"));
    m_queueDisp.AddChildQueueDisc(handle, cid[3], "ns3::FifoQueueDisc", "MaxSize", StringValue("100000p"));
    m_queueDisp.AddChildQueueDisc(handle, cid[4], "ns3::FifoQueueDisc", "MaxSize", StringValue("100000p"));
    m_queueDisp.AddChildQueueDisc(handle, cid[5], "ns3::FifoQueueDisc", "MaxSize", StringValue("100000p"));
    m_queueDisp.AddChildQueueDisc(handle, cid[6], "ns3::FifoQueueDisc", "MaxSize", StringValue("100000p"));
    m_queueDisp.AddChildQueueDisc(handle, cid[7], "ns3::FifoQueueDisc", "MaxSize", StringValue("100000p"));
}

void
spineLeafTopo::ConfigureQueueDisp()
{
    for (uint32_t i = 0; i < m_spineNum; ++i)
    {
        for (uint32_t j = 0; j < m_leafNum; ++j)
        {
            m_queueDisp.Install(m_spineLeafDevices[i][j]);
        }
    }
    for (uint32_t i = 0; i < m_leafNum; ++i)
    {
        for (uint32_t j = 0; j < m_gpuNumPerLeaf; ++j)
        {
            m_queueDisp.Install(m_leafGpuDevices[i][j]);
        }
    }
}

void
spineLeafTopo::AssignIpAddresses()
{
    NS_LOG_FUNCTION(this);
    Ipv4AddressHelper ipHelper;
    m_spineLeafInterfaces.resize(m_spineNum, std::vector<Ipv4InterfaceContainer>(m_leafNum));
    m_leafGpuInterfaces.resize(m_leafNum, std::vector<Ipv4InterfaceContainer>(m_gpuNumPerLeaf));

    for (uint32_t i = 0; i < m_spineNum; ++i)
    {
        for (uint32_t j = 0; j < m_leafNum; ++j)
        {
            // cout << "Subnet between Spine " << i << " and Leaf " << j << ": " << endl;
            // 一定要+1!!!
            std::string subnet = "10." + std::to_string(i + 1) + "." + std::to_string(j + 1) + ".0";
            ipHelper.SetBase(subnet.c_str(), "255.255.255.0");
            m_spineLeafInterfaces[i][j] = ipHelper.Assign(m_spineLeafDevices[i][j]);
        }
    }

    for (uint32_t i = 0; i < m_leafNum; ++i)
    {
        for (uint32_t j = 0; j < m_gpuNumPerLeaf; ++j)
        {
            // cout << "Subnet between Leaf " << i << " and GPU " << j << ": " << endl;
            // 一定要加1!!!
            std::string subnet = "192.168." + std::to_string(i * m_gpuNumPerLeaf + j + 1) + ".0";
            ipHelper.SetBase(subnet.c_str(), "255.255.255.0");

            m_leafGpuInterfaces[i][j] = ipHelper.Assign(m_leafGpuDevices[i][j]);
        }
    }
}

void
spineLeafTopo::ConfigureRoute()
{
    NS_LOG_FUNCTION(this);
    Ipv4StaticRoutingHelper staticRoutingHelper;
    // direct link does not need to configure, it exists.

    // configure gpu default route, can not be omitted
    for (uint32_t i = 0; i < m_leafNum; ++i)
    {
        for (uint32_t j = 0; j < m_gpuNumPerLeaf; ++j)
        {
            // 用helper获取gpu的ipv4静态路由
            Ptr<Ipv4StaticRouting> staticRouting = staticRoutingHelper.GetStaticRouting(
                m_gpuNodes.Get(i * m_gpuNumPerLeaf + j)->GetObject<Ipv4>());
            // 获取对应leaf的ip
            Ipv4Address leafIp = m_leafGpuInterfaces[i][j].GetAddress(
                0); // the 0 means the devicescontainer,(leaf(0),gpu(1))
            // 添加默认路由, NetDevice的接口索引是从1开始的，不是从0，所以这里指唯一的接口
            staticRouting->SetDefaultRoute(leafIp, 1);
        }
    }
    srand(41);
    map<uint32_t, uint32_t> mmap;
    mmap[0] = 0;
    mmap[1] = 1;
    mmap[2] = 0;
    mmap[3] = 1;
    // configure leaf default route, here various strategies can be used
    for (uint32_t i = 0; i < m_leafNum; ++i)
    {
        Ptr<Ipv4StaticRouting> staticRouting =
            staticRoutingHelper.GetStaticRouting(m_leafNodes.Get(i)->GetObject<Ipv4>());

        // UP direction
        if (m_loadBalanceStrategy == "to0")
        {
            // here we route all the traffic from all the leaf to the spine[0]
            // of course its only the toy
            // in the m_spineLeafInterfaces[0][i], the 0 means the route from leaf to spine,
            // we will set it. Now all the traffic from the leaf will be routed to the spine[0]
            // the second 0 in GetAddress(0) means the first address in the subnet, that is the
            // first ip of the spine 0
            Ipv4Address spineIp = m_spineLeafInterfaces[0][i].GetAddress(0);
            // we set it. interface index start from 1 not 0
            staticRouting->SetDefaultRoute(spineIp, 1);
        }
        else if (m_loadBalanceStrategy == "random")
        {
            // here we route the traffic to spine selected randomly
            uint32_t spineIndex = rand() % m_spineNum;
            // uint32_t spineIndex = mmap[i];
            Ipv4Address spineIp = m_spineLeafInterfaces[spineIndex][i].GetAddress(0);
            staticRouting->SetDefaultRoute(spineIp, spineIndex + 1);
            m_leafSpineMap[i] = spineIndex;
        }
        else
        {
            cout << "Not supported load balance strategy: " << m_loadBalanceStrategy << endl;
            exit(0);
        }

        // DOWN direction
    }
    for (auto [key, value] : m_leafSpineMap)
    {   
        printColoredText("[TOPO] Leaf " + to_string(key) + " route to spine " + to_string(value), "green");
    }

    // configure spine default route, decide the leaf to route through the dst ip
    for (uint32_t i = 0; i < m_spineNum; ++i)
    {
        Ipv4StaticRoutingHelper staticRoutingHelper;
        Ptr<Ipv4StaticRouting> staticRouting =
            staticRoutingHelper.GetStaticRouting(m_spineNodes.Get(i)->GetObject<Ipv4>());
        // iter all the leaf's subnets
        for (uint32_t j = 0; j < m_leafNum; ++j)
        {
            Ipv4Address leafIp = m_spineLeafInterfaces[i][j].GetAddress(1);
            for (uint32_t k = 0; k < m_gpuNumPerLeaf; ++k)
            {
                Ipv4Address gpuIp = m_leafGpuInterfaces[j][k].GetAddress(1);
                Ipv4Mask gpuSubnetsMask = Ipv4Mask("255.255.255.0");
                Ipv4Address gpuSubnets = gpuIp.CombineMask(gpuSubnetsMask);
                staticRouting->AddNetworkRouteTo(gpuSubnets, gpuSubnetsMask, leafIp, j + 1);
                // // Add route to each GPU via the corresponding leaf node, it is the same with
                // the above Ipv4Address gpuIp = m_leafGpuInterfaces[j][k].GetAddress(1);
                // staticRouting->AddHostRouteTo(gpuIp, leafIp, j + 1);
            }
        }
    }
}

void
spineLeafTopo::captureSpineLeafPackets(uint32_t spineId, uint32_t leafId)
{
    m_spineLeafLink.EnablePcap(string("spine-leaf-" + to_string(spineId) + "-" + to_string(leafId)),
                               m_spineLeafDevices[spineId][leafId],
                               true);
}

void
spineLeafTopo::captureLeafGpuPackets(uint32_t leafId, uint32_t gpuId)
{
    PointToPointHelper pointToPoint;
    pointToPoint.EnablePcapAll(string(getenv("DDL_MAIN_DIR")) + "log/" + "leaf-gpu");
    // m_leafGpuLink.EnablePcap(string(string(getenv("DDL_MAIN_DIR")) + "leaf-gpu-" +
    // to_string(leafId) + "-" + to_string(gpuId)),
    //                          m_leafGpuDevices[leafId][gpuId], true);
}

void
spineLeafTopo::pingAllGpuTest()
{
    NS_LOG_FUNCTION(this);
    for (uint32_t i = 0; i < m_leafNum; ++i)
    {
        for (uint32_t j = 0; j < m_gpuNumPerLeaf; ++j)
        {
            for (uint32_t k = 0; k < m_leafNum; ++k)
            {
                for (uint32_t l = 0; l < m_gpuNumPerLeaf; ++l)
                {
                    pingTest(m_gpuNodes.Get(i * m_gpuNumPerLeaf + j),
                             m_gpuNodes.Get(k * m_gpuNumPerLeaf + l));
                }
            }
        }
    }
}

void
spineLeafTopo::pingTest(Ptr<Node> n1, Ptr<Node> n2)
{
    NS_LOG_FUNCTION(this);
    Ipv4Address n1Ip = n1->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    Ipv4Address n2Ip = n2->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    cout << "Ping test from " << n1Ip << " to " << n2Ip << endl;
    Ipv4Address destIp = n2Ip;
    // Ipv4Address sourceIp = n1Ip;
    PingHelper ping(destIp);
    ping.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    ping.SetAttribute("Size", UintegerValue(1024));
    ApplicationContainer apps = ping.Install(n1);
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(10.0));
}

void
spineLeafTopo::printRoutingTable(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this);
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    Ipv4StaticRoutingHelper staticRoutingHelper;
    Ptr<Ipv4StaticRouting> staticRouting = staticRoutingHelper.GetStaticRouting(ipv4);

    std::cout << "Routing table for node " << node->GetId() << ":\n";
    for (uint32_t i = 0; i < staticRouting->GetNRoutes(); ++i)
    {
        Ipv4RoutingTableEntry route = staticRouting->GetRoute(i);
        std::cout << route << std::endl;
    }
    std::cout << std::endl;
}

void
spineLeafTopo::PrintTopo()
{
    NS_LOG_FUNCTION(this);
    cout << "Spine-Leaf Topology: " << endl;
    cout << "Spine Number: " << m_spineNum << endl;
    cout << "Leaf Number: " << m_leafNum << endl;
    cout << "GPU Number Per Leaf: " << m_gpuNumPerLeaf << endl;
    cout << "Spine-Leaf Bandwidth: " << m_spineLeafBandwidth << " Gbps" << endl;
    cout << "Leaf-GPU Bandwidth: " << m_leafGpuBandwidth << " Gbps" << endl;
}

void
spineLeafTopo::PrintSpineLeafSubnets()
{
    NS_LOG_FUNCTION(this);
    cout << "Spine-Leaf Subnets: " << endl;
    for (uint32_t i = 0; i < m_spineNum; ++i)
    {
        for (uint32_t j = 0; j < m_leafNum; ++j)
        {
            cout << "Subnet between Spine " << i << " and Leaf " << j << ": ";
            cout << m_spineLeafInterfaces[i][j].GetAddress(0) << " - ";
            cout << m_spineLeafInterfaces[i][j].GetAddress(1) << endl;
        }
    }
    cout << "Leaf-GPU Subnets: " << endl;
    for (uint32_t i = 0; i < m_leafNum; ++i)
    {
        for (uint32_t j = 0; j < m_gpuNumPerLeaf; ++j)
        {
            cout << "Subnet between Leaf " << i << " and GPU " << j << ": ";
            cout << m_leafGpuInterfaces[i][j].GetAddress(0) << " - ";
            cout << m_leafGpuInterfaces[i][j].GetAddress(1) << endl;
        }
    }
}

} // namespace ns3
