#include "ddl-flow-recv.h"

#include "ddl-app.h"

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
NS_LOG_COMPONENT_DEFINE("DdlFlowRecvApplication");
NS_OBJECT_ENSURE_REGISTERED(DdlFlowRecvApplication);

TypeId
DdlFlowRecvApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DdlFlowRecvApplication")
            .SetParent<SinkApplication>()
            .SetGroupName("Applications")
            .AddConstructor<DdlFlowRecvApplication>()
            .AddAttribute("ddlPort",
                          "The port of the application",
                          UintegerValue(0),
                          MakeUintegerAccessor(&DdlFlowRecvApplication::m_ddlPort),
                          MakeUintegerChecker<uint16_t>(1))
            .AddAttribute("Protocol",
                          "The type of protocol to use.",
                          TypeIdValue(UdpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&DdlFlowRecvApplication::m_tid),
                          MakeTypeIdChecker())
            .AddAttribute("FlowId",
                          "The flow id of the application",
                          UintegerValue(0),
                          MakeUintegerAccessor(&DdlFlowRecvApplication::m_flowid),
                          MakeUintegerChecker<uint16_t>(0))
            .AddAttribute("JobId",
                          "The job id of the application",
                          UintegerValue(0),
                          MakeUintegerAccessor(&DdlFlowRecvApplication::m_jobid),
                          MakeUintegerChecker<uint16_t>(0))
            .AddAttribute("NodeId",
                          "The node id of the application",
                          UintegerValue(0),
                          MakeUintegerAccessor(&DdlFlowRecvApplication::m_nodeid),
                          MakeUintegerChecker<uint16_t>(0))
            .AddAttribute("ExpectedBytes",
                          "The expected number of bytes to receive",
                          UintegerValue(0),
                          MakeUintegerAccessor(&DdlFlowRecvApplication::m_expectedBytes),
                          MakeUintegerChecker<uint32_t>(0))
            .AddAttribute("IterNum",
                          "The number of iterations of the application",
                          UintegerValue(1),
                          MakeUintegerAccessor(&DdlFlowRecvApplication::m_iterNum),
                          MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("IsLastFlow",
                          "The last flow of the application",
                          BooleanValue(false),
                          MakeBooleanAccessor(&DdlFlowRecvApplication::m_isLastFlow),
                          MakeBooleanChecker());

    return tid;
} // namespace ns3

DdlFlowRecvApplication::DdlFlowRecvApplication()
    : m_socket(nullptr),
      m_receivedBytes(0),
      m_iterCnt(0)
{
    NS_LOG_FUNCTION(this);
}

DdlFlowRecvApplication::~DdlFlowRecvApplication()
{
    NS_LOG_FUNCTION(this);
}

void
DdlFlowRecvApplication::setParentDdlApp(DdlApplication* parentDdlApp)
{
    m_parentDdlApp = parentDdlApp;
}

void
DdlFlowRecvApplication::detailedLog(std::string info)
{
    NS_LOG_INFO("At time " << Simulator::Now().As(Time::MS) << " Job[" << m_jobid << "]"
                           << " Flow[" << m_flowid << "]"
                           << " At Node[" << m_nodeid << "]" << info);
}

void
DdlFlowRecvApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::UdpSocketFactory"));
        if (!m_socket)
        {
            NS_FATAL_ERROR("Failed to create socket");
        }

        m_socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_ddlPort));
        m_socket->SetRecvCallback(MakeCallback(&DdlFlowRecvApplication::HandleRead, this));
    }
    if (!m_ackSocket)
    {
        m_ackSocket =
            Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::UdpSocketFactory"));
    }
}

void
DdlFlowRecvApplication::StopApplication()
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
DdlFlowRecvApplication::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;

    while (socket->GetRxAvailable() > 0)
    {
        Address from;

        Ptr<Packet> packet = socket->RecvFrom(from);
        m_receivedBytes += packet->GetSize();
        // NS_LOG_INFO("Flow[" << m_flowid << "]" << "At time " << Simulator::Now().As(Time::S)
        //                     << " Received " << m_receivedBytes << " bytes");
        if (m_receivedBytes >= m_expectedBytes)
        {
            detailedLog(" Received " + std::to_string(m_receivedBytes) + " bytes");

            // tell the parent app that this flow has finished

            m_receivedBytes = 0;

            m_iterCnt++;
            bool notifyNextFlow = true;
            if (m_isLastFlow)
            {
                detailedLog(" Iteration " + std::to_string(m_iterCnt) + " finished===========");
                if (m_iterCnt >= m_iterNum)
                {
                    notifyNextFlow = false;
                    m_parentDdlApp->stopAllFlows(m_flowid);
                }
            }
            if (notifyNextFlow)
            {
                m_parentDdlApp->notifyFinish(m_flowid);
            }
        }
    }
}

} // namespace ns3