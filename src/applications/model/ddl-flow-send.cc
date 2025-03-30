#include "ddl-flow-send.h"

#include "ddl-tools.h"

#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
using namespace std;

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DdlFlowSendApplication");
NS_OBJECT_ENSURE_REGISTERED(DdlFlowSendApplication);

TypeId
DdlFlowSendApplication::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DdlFlowSendApplication")
                            .SetParent<SourceApplication>()
                            .SetGroupName("Applications")
                            .AddConstructor<DdlFlowSendApplication>()
                            .AddAttribute("FlowId",
                                          "The flow id of the application",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&DdlFlowSendApplication::m_flowid),
                                          MakeUintegerChecker<uint16_t>(0))
                            .AddAttribute("JobId",
                                          "The job id of the application",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&DdlFlowSendApplication::m_jobid),
                                          MakeUintegerChecker<uint16_t>(0))
                            .AddAttribute("NodeId",
                                          "The node id of the application",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&DdlFlowSendApplication::m_nodeid),
                                          MakeUintegerChecker<uint16_t>(0))

                            .AddAttribute("ddlRemote",
                                          "The remote address of the application",
                                          AddressValue(),
                                          MakeAddressAccessor(&DdlFlowSendApplication::m_ddlRemote),
                                          MakeAddressChecker())
                            .AddAttribute("ddlPort",
                                          "The port of the application",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&DdlFlowSendApplication::m_ddlPort),
                                          MakeUintegerChecker<uint16_t>(1))

                            .AddAttribute("IterNum",
                                          "The number of iterations of the application",
                                          UintegerValue(1),
                                          MakeUintegerAccessor(&DdlFlowSendApplication::m_iternum),
                                          MakeUintegerChecker<uint32_t>(1))
                            .AddAttribute("CompTime",
                                          "The computation time of the application",
                                          TimeValue(MilliSeconds(1)),
                                          MakeTimeAccessor(&DdlFlowSendApplication::m_comptime),
                                          MakeTimeChecker())
                            .AddAttribute("CommSize",
                                          "The communication size of the application",
                                          UintegerValue(1),
                                          MakeUintegerAccessor(&DdlFlowSendApplication::m_commsize),
                                          MakeUintegerChecker<uint32_t>(0))
                            .AddAttribute("Protocol",
                                          "The type of protocol to use.",
                                          TypeIdValue(UdpSocketFactory::GetTypeId()),
                                          MakeTypeIdAccessor(&DdlFlowSendApplication::m_tid),
                                          MakeTypeIdChecker());
    return tid;
}

DdlFlowSendApplication::DdlFlowSendApplication()
    : m_socket(nullptr),
      m_ackSocket(nullptr),
      m_waitingAck(false),
      m_send_cnt(0)
{
    NS_LOG_FUNCTION(this);
    // m_peer = InetSocketAddress(AddressValue(m_ddlRemote), m_ddlPort);

} // namespace ns3

DdlFlowSendApplication::~DdlFlowSendApplication()
{
    NS_LOG_FUNCTION(this);
}

void
DdlFlowSendApplication::PrintInfo()
{
    std::cout << "flowid: " << m_ddlPort << std::endl;
};

void
DdlFlowSendApplication::StartApplication()
{
    m_peer = addressUtils::ConvertToSocketAddress(m_ddlRemote, m_ddlPort);
    NS_LOG_FUNCTION(this);
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::UdpSocketFactory"));
        if (!m_socket)
        {
            NS_FATAL_ERROR("Failed to create socket");
        }
        m_socket->SetIpTos(m_tos);
        // m_socket->SetPriority(5);
        m_socket->Connect(m_peer);

        if (isAllTrue(m_upstreamFinishState))
        {
            // here only schedule once at the beginning
            Simulator::Schedule(m_comptime, &DdlFlowSendApplication::SendPacket, this);
        }
    }
}

void
DdlFlowSendApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);
    if (m_socket)
    {
        m_socket->Close();
    }
    if (m_ackSocket)
    {
        m_ackSocket->Close();
    }
}

void
DdlFlowSendApplication::HandleAck(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;
    while ((packet = m_ackSocket->Recv()))
    {
        m_waitingAck = false;
        Simulator::Schedule(m_comptime, &DdlFlowSendApplication::SendPacket, this);
    }
}

void
DdlFlowSendApplication::SendFragmentPacket()
{
    uint32_t packet_size = 20000;
    uint32_t packet_sent = 0;
    while (packet_sent <= m_commsize)
    {
        // adapt the tos to the flow tos, tos is from the parent app
        // only here we assign tos
        // not the time when flow created, for the tos need to be modified
        m_socket->SetIpTos(m_parentDdlApp->getFlowTos()[m_flowid]);
        Ptr<Packet> packet = Create<Packet>(std::min(packet_size, m_commsize - packet_sent));
        m_socket->Send(packet);
        packet_sent += packet_size;
    }
}

void
DdlFlowSendApplication::detailedLog(std::string info)
{
    NS_LOG_INFO("At time " << Simulator::Now().As(Time::MS) << " Job[" << m_jobid << "]"
                           << " Flow[" << m_flowid << "]"
                           << " At Node[" << m_nodeid << "]" << info);
}

void
DdlFlowSendApplication::SendPacket()
{
    NS_LOG_FUNCTION(this);

    SendFragmentPacket();

    detailedLog(" Send packet " + std::to_string(m_commsize) + " bytes");
    m_send_cnt++;
    m_waitingAck = true; // useless now
    // when it finish the send, it stop by setting all the upstream finish to false
    setUpstreamFinishStatesAllFalse();

} // namespace ns3

void
DdlFlowSendApplication::setUpstreamFlowIds(std::vector<uint32_t> upstreamFlowIds)
{
    m_upstreamFlowIds = upstreamFlowIds;
}

void
DdlFlowSendApplication::setUpstreamFinishStatesAllFalse()
{
    for (auto flowId : m_upstreamFlowIds)
    {
        m_upstreamFinishState[flowId] = false;
    }
}

void
DdlFlowSendApplication::setUpstreamFinishStatesAllTrue()
{
    for (auto flowId : m_upstreamFlowIds)
    {
        m_upstreamFinishState[flowId] = true;
    }
}

void
DdlFlowSendApplication::setUpstreamFinishState(uint32_t flowId, bool state)
{
    m_upstreamFinishState[flowId] = state;
    if (isAllTrue(m_upstreamFinishState))
    {
        // here we should adopt the comp time to meet the true situation
        uint32_t comptimeMilliSeconds = m_comptime.GetMicroSeconds();
        float percentage = 0.0; // Adjust the percentage as needed
        if(m_jobid == 1){
            comptimeMilliSeconds *= 1;
        }
        float randomFactor = (1.0f - percentage) + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (2 * percentage)));
        Time actualCompTime = MicroSeconds((uint32_t)(comptimeMilliSeconds * randomFactor));
        Simulator::Schedule(actualCompTime, &DdlFlowSendApplication::SendPacket, this);
    }
}
} // namespace ns3