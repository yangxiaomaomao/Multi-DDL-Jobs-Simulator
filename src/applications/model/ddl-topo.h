#ifndef DDL_TOPO_H
#define DDL_TOPO_H
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

class spineLeafTopo
{
  public:
    spineLeafTopo(string topoConfigFile);
    void InitTopoConfig(string topoConfigFile);

    void CreateNodes();
    void SetupLink();

    void InstallSpineLeafLinks();
    void InstallLeafGpuLinks();
    void InstallInternetStack();
    void AssignIpAddresses();

    void PrintTopo();
    void PrintSpineLeafSubnets();

    void ConfigureRoute();
    void printRoutingTable(Ptr<Node> node);

    void pingAllGpuTest();
    void pingTest(Ptr<Node> n1, Ptr<Node> n2);

    void InitQueueDisp();
    void ConfigureQueueDisp();

    void captureSpineLeafPackets(uint32_t spineId, uint32_t leafId);
    void captureLeafGpuPackets(uint32_t leafId, uint32_t gpuId);

    NodeContainer getSpineNodes()
    {
        return m_spineNodes;
    }

    NodeContainer getLeafNodes()
    {
        return m_leafNodes;
    }

    NodeContainer getGpuNodes()
    {
        return m_gpuNodes;
    }

    uint32_t getGpuNum()
    {
        return m_leafNum * m_gpuNumPerLeaf;
    }

    uint32_t getSpineNum()
    {
        return m_spineNum;
    }

    uint32_t getLeafNum()
    {
        return m_leafNum;
    }

    uint32_t getBandwidth()
    {
        return m_spineLeafBandwidth;
    }

    uint32_t getGpuNumPerLeaf()
    {
        return m_gpuNumPerLeaf;
    }

    std::map<uint32_t, uint32_t> getLeafSpineMap()
    {
        return m_leafSpineMap;
    }

  private:
    uint32_t m_spineNum;
    uint32_t m_leafNum;
    uint32_t m_gpuNumPerLeaf;
    vector<uint32_t> m_freeNodeIndex;
    float m_spineLeafBandwidth;
    float m_leafGpuBandwidth;

    NodeContainer m_spineNodes;
    NodeContainer m_leafNodes;
    NodeContainer m_gpuNodes;

    PointToPointHelper m_spineLeafLink;
    PointToPointHelper m_leafGpuLink;

    std::vector<std::vector<NetDeviceContainer>> m_spineLeafDevices;
    std::vector<std::vector<NetDeviceContainer>> m_leafGpuDevices;
    std::vector<std::vector<Ipv4InterfaceContainer>> m_spineLeafInterfaces;
    std::vector<std::vector<Ipv4InterfaceContainer>> m_leafGpuInterfaces;

    map<uint32_t, uint32_t> m_leafSpineMap;

    TrafficControlHelper m_queueDisp;
    string m_loadBalanceStrategy;
};

} // namespace ns3

#endif
