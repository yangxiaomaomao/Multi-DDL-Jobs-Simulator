#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-header.h"
#include "ns3/network-module.h"
#include "ns3/packet.h"
#include "ns3/pfifo-fast-queue-disc.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-helper.h"

using namespace ns3;

void
BytesInQueueTrace(Ptr<OutputStreamWrapper> stream, uint32_t oldVal, uint32_t newVal)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newVal << std::endl;
}

int
main(int argc, char* argv[])
{
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    // LogComponentEnable("PfifoFastQueueDisc", LOG_LEVEL_ALL);
    // LogComponentEnable("PrioQueueDisc", LOG_LEVEL_ALL);
    std::string flowsDatarate = "2000Kbps";
    uint32_t flowsPacketsSize = 4000;
    float startTime = 0.1F; // in s
    float simDuration = 600;
    float samplingPeriod = 1;
    std::string queueDiscType = "PfifoFast";
    float stopTime = startTime + simDuration;
    // 创建三个节点
    NodeContainer n1;
    NodeContainer n2;
    NodeContainer n3;
    n1.Create(1);
    n2.Create(1);
    n3.Create(1);

    // 创建点对点链路
    PointToPointHelper accessLink;
    accessLink.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    accessLink.SetChannelAttribute("Delay", StringValue("0.1ms"));
    accessLink.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("100p"));

    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue("1000Kbps"));
    bottleneckLink.SetChannelAttribute("Delay", StringValue("5ms"));
    bottleneckLink.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("50p"));

    // 安装互联网协议栈
    InternetStackHelper stack;
    stack.InstallAll();

    // Access link traffic control configuration
    TrafficControlHelper tchPfifoFastAccess;
    tchPfifoFastAccess.SetRootQueueDisc("ns3::PfifoFastQueueDisc", "MaxSize", StringValue("1000p"));

    // Access link traffic control configuration
    // Bottleneck link traffic control configuration
    TrafficControlHelper tchBottleneck;

    if (queueDiscType == "PfifoFast")
    {
        tchBottleneck.SetRootQueueDisc(
            "ns3::PfifoFastQueueDisc",
            "MaxSize",
            QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, 1000)));
    }
    else if (queueDiscType == "prio")
    {
        uint16_t handle =
            tchBottleneck.SetRootQueueDisc("ns3::PrioQueueDisc",
                                           "Priomap",
                                           StringValue("0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1"));
        TrafficControlHelper::ClassIdList cid =
            tchBottleneck.AddQueueDiscClasses(handle, 2, "ns3::QueueDiscClass");
        tchBottleneck.AddChildQueueDisc(handle, cid[0], "ns3::FifoQueueDisc");
        tchBottleneck.AddChildQueueDisc(handle, cid[1], "ns3::RedQueueDisc");
    }
    else
    {
        NS_ABORT_MSG("--queueDiscType not valid");
    }
    // n1-n2, access link
    NetDeviceContainer devicesAccessLink = accessLink.Install(n1.Get(0), n2.Get(0));
    tchPfifoFastAccess.Install(devicesAccessLink);
    Ipv4AddressHelper address;
    address.SetBase("192.168.0.0", "255.255.255.0");
    address.NewNetwork();
    Ipv4InterfaceContainer interfacesAccess = address.Assign(devicesAccessLink);

    // n2-n3, bottleneck link
    NetDeviceContainer devicesBottleneckLink = bottleneckLink.Install(n2.Get(0), n3.Get(0));
    tchBottleneck.Install(devicesBottleneckLink);
    address.NewNetwork(); // 自动分配子网ip地址
    Ipv4InterfaceContainer interfacesBottleneck = address.Assign(devicesBottleneckLink);

    Ptr<NetDeviceQueueInterface> interface =
        devicesBottleneckLink.Get(0)->GetObject<NetDeviceQueueInterface>();
    Ptr<NetDeviceQueue> queueInterface = interface->GetTxQueue(0);

    AsciiTraceHelper ascii;
    Ptr<Queue<Packet>> queue =
        StaticCast<PointToPointNetDevice>(devicesAccessLink.Get(0))->GetQueue();
    Ptr<OutputStreamWrapper> streamBytesInQueue = ascii.CreateFileStream("test.txt");
    queue->TraceConnectWithoutContext("BytesInQueue",
                                      MakeBoundCallback(&BytesInQueueTrace, streamBytesInQueue));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t high_prio = 60 << 2;
    uint16_t low_prio = 58 << 2;
    Ipv4InterfaceContainer n1Interface;
    n1Interface.Add(interfacesAccess.Get(0));

    Ipv4InterfaceContainer n2Interface;
    n2Interface.Add(interfacesAccess.Get(1));
    n2Interface.Add(interfacesBottleneck.Get(0));

    Ipv4InterfaceContainer n3Interface;
    n3Interface.Add(interfacesBottleneck.Get(1));
    if (0)
    {
        uint16_t port1 = 9;
        PacketSinkHelper sinkHelper1("ns3::TcpSocketFactory",
                                     InetSocketAddress(Ipv4Address::GetAny(), port1));
        sinkHelper1.SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
        ApplicationContainer sinkApp1 = sinkHelper1.Install(n3);
        sinkApp1.Start(Seconds(10.0));
        sinkApp1.Stop(Seconds(1000.0));

        InetSocketAddress socketAddressUp1 = InetSocketAddress(n3Interface.GetAddress(0), port1);
        OnOffHelper onOffHelper1("ns3::TcpSocketFactory", Address());
        onOffHelper1.SetAttribute("Remote", AddressValue(socketAddressUp1));
        onOffHelper1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOffHelper1.SetAttribute("OffTime",
                                  StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        onOffHelper1.SetAttribute("PacketSize", UintegerValue(flowsPacketsSize));
        onOffHelper1.SetAttribute("DataRate", StringValue(flowsDatarate));
        onOffHelper1.SetAttribute("FlowId", UintegerValue(0));
        onOffHelper1.SetAttribute("Priority", UintegerValue(0));
        // onOffHelper1.SetAttribute("Tos", UintegerValue(46<<2));
        ApplicationContainer onOffApp1 = onOffHelper1.Install(n1);
        onOffApp1.Start(Seconds(10.0));
        onOffApp1.Stop(Seconds(1000.0));
    }
    if (1)
    {
        uint16_t port2 = 11;
        PacketSinkHelper sinkHelper2("ns3::TcpSocketFactory",
                                     InetSocketAddress(Ipv4Address::GetAny(), port2));
        sinkHelper2.SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
        ApplicationContainer sinkApp2 = sinkHelper2.Install(n3);
        sinkApp2.Start(Seconds(0.0));
        sinkApp2.Stop(Seconds(5.0));

        InetSocketAddress socketAddressUp2 = InetSocketAddress(n3Interface.GetAddress(0), port2);
        OnOffHelper onOffHelper2("ns3::TcpSocketFactory", Address());
        onOffHelper2.SetAttribute("Remote", AddressValue(socketAddressUp2));
        onOffHelper2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOffHelper2.SetAttribute("OffTime",
                                  StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        onOffHelper2.SetAttribute("PacketSize", UintegerValue(flowsPacketsSize));
        onOffHelper2.SetAttribute("DataRate", StringValue(flowsDatarate));
        onOffHelper2.SetAttribute("FlowId", UintegerValue(1));
        // onOffHelper2.SetAttribute("Priority", UintegerValue(1));
        onOffHelper2.SetAttribute("Tos", UintegerValue(60<<2));
        
        ApplicationContainer onOffApp2 = onOffHelper2.Install(n1);
        onOffApp2.Start(Seconds(0.0));
        onOffApp2.Stop(Seconds(5.0));
    }

    if (1)
    {
        // 创建Echo服务器（监听端口9）
        UdpEchoServerHelper echoServer(9);
        echoServer.SetAttribute("Tos", UintegerValue(46<<2)); // 设置TOS字段
        ApplicationContainer serverApps = echoServer.Install(n3.Get(0));
        serverApps.Start(Seconds(0.0));
        serverApps.Stop(Seconds(20.0));

        // 创建Echo客户端（发送到节点1的端口9）

        UdpEchoClientHelper echoClient(Ipv4Address("192.168.2.2"), 9); // 目标地址为节点1的IP
        echoClient.SetAttribute("MaxPackets", UintegerValue(10));       // 发送1个数据包
        echoClient.SetAttribute("Interval", TimeValue(Seconds(0.5)));  // 每隔1秒发送
        echoClient.SetAttribute("PacketSize", UintegerValue(5000));   // 数据包大小
        echoClient.SetAttribute("Tos", UintegerValue(46<<2));         // 设置TOS字段
        ApplicationContainer clientApps = echoClient.Install(n1.Get(0)); // 客户端安装在节点0

        clientApps.Start(Seconds(0.0)); // 客户端在2秒后开始
        clientApps.Stop(Seconds(20.0));

        // 创建第二个Echo服务器（监听端口10）
        // UdpEchoServerHelper echoServer2(3);
        // echoServer2.SetAttribute("Tos", UintegerValue(high_prio)); // 设置TOS字段
        // ApplicationContainer serverApps2 = echoServer2.Install(nodes.Get(2));
        // serverApps2.Start(Seconds(0.0));
        // serverApps2.Stop(Seconds(1000.0));

        // // 创建第二个Echo客户端（发送到节点1的端口10）
        // UdpEchoClientHelper echoClient2(Ipv4Address("10.1.2.2"), 10);         //
        // 目标地址为节点1的IP echoClient2.SetAttribute("MaxPackets", UintegerValue(3)); //
        // 发送1个数据包 echoClient2.SetAttribute("Interval", TimeValue(Seconds(1.0)));        //
        // 每隔1秒发送 echoClient2.SetAttribute("PacketSize", UintegerValue(500));         //
        // 数据包大小 echoClient2.SetAttribute("Tos", UintegerValue(high_prio));            //
        // 设置TOS字段 ApplicationContainer clientApps2 = echoClient2.Install(nodes.Get(0)); //
        // 客户端安装在节点0 clientApps2.Start(Seconds(0.0)); // 客户端在2秒后开始
        // clientApps2.Stop(Seconds(1000.0));
    }

    if (0)
    {
        // 创建Bulk发送器（发送到节点1的端口11）
        uint16_t port = 11;
        OnOffHelper onOffHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address("192.168.0.2"), port));
        onOffHelper.SetAttribute(
            "OnTime",
            StringValue("ns3::ConstantRandomVariable[Constant=1]")); // 持续发送
        onOffHelper.SetAttribute("OffTime",
                                 StringValue("ns3::ConstantRandomVariable[Constant=0]")); // 不停顿
        onOffHelper.SetAttribute("PacketSize", UintegerValue(1024));   // 设置每个数据包的大小
        onOffHelper.SetAttribute("DataRate", StringValue("1000kb/s")); // 设置发送速率

        ApplicationContainer sourceApps = onOffHelper.Install(n1.Get(0));
        sourceApps.Start(Seconds(0.0));
        sourceApps.Stop(Seconds(1000.0));

        // 创建接收器（监听端口11）
        PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
                                          InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApps = packetSinkHelper.Install(n2.Get(0));
        sinkApps.Start(Seconds(0.0));
        sinkApps.Stop(Seconds(1000.0));
    }
    if (0)
    {
        // 创建一个应用程序，每隔5秒发送10KB数据
        OnOffHelper onOffHelper("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address("10.1.1.2"), 12));
        onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=4]"));
        onOffHelper.SetAttribute("DataRate", DataRateValue(DataRate("10KBps")));
        onOffHelper.SetAttribute("PacketSize", UintegerValue(10240));

        ApplicationContainer onOffApp = onOffHelper.Install(n1.Get(0));
        onOffApp.Start(Seconds(0.0));
        onOffApp.Stop(Seconds(30.0));
    }
    accessLink.EnablePcapAll("test");
    Simulator::Run();
    Simulator::Destroy();
    // // 运行模拟
    
    // Simulator::Run();
    // Simulator::Destroy();

    return 0;
}
