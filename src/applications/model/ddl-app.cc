#include "ddl-app.h"

#include "../helper/ddl-flow-recv-helper.h"
#include "../helper/ddl-flow-send-helper.h"
#include "ddl-flow-recv.h"
#include "ddl-flow-send.h"
#include "ddl-state.h"
#include "ddl-tools.h"
#include "ddl-topo.h"

#include "ns3/applications-module.h"
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

#include <unordered_set>
#include <variant>
#include <vector>

using namespace std;

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("DdlApplication");
NS_OBJECT_ENSURE_REGISTERED(DdlApplication);

Address
getNodeIp(Ptr<Node> node)
{
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1, 0);
    Ipv4Address ipAddr = iaddr.GetLocal();
    return ipAddr;
}

DdlApplication::DdlApplication(uint32_t jobId, DdlAppManager* appManager)
    : m_jobId(jobId),
      m_iterCnt(0),
      m_appManager(appManager),
      m_state(JobState::UNARRIVED)
{
    NS_LOG_FUNCTION(this);
    initFlowFeatures();
}

DdlApplication::~DdlApplication()
{
    NS_LOG_FUNCTION(this);
}

void
DdlApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);
}

void
DdlApplication::checkAndStartApplication()
{
    uint32_t checkInterval = 10;
    // NS_LOG_INFO("Checking..." << Simulator::Now().GetMilliSeconds());
    // Simulator::Schedule函数只是把这个时间放入了调度序列中，并没有立即执行，而是等待模拟时钟到了之后才执行
    if (Simulator::Now().GetMilliSeconds() >= m_arriveTimeMilliSeconds)
    {
        if (m_state == JobState::UNARRIVED)
        {
            printColoredText("Job[" + to_string(m_jobId) + "] arrives at " +
                                 to_string(m_arriveTimeMilliSeconds) + "ms",
                             "green");
        }

        setState(JobState::ARRIVED);

        auto gpuIndex = m_appManager->getJobPlacement(this);
        if (gpuIndex.size() == 0)
        {
            setState(JobState::PENDING);
            Simulator::Schedule(MilliSeconds(checkInterval), &DdlApplication::checkAndStartApplication, this);
        }
        else
        {
            setState(JobState::RUNNING);
            generateFlow(gpuIndex);
        }
    }
    else
    {
        Simulator::Schedule(MilliSeconds(checkInterval), &DdlApplication::checkAndStartApplication, this);
    }
}

void
DdlApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);
}

void
DdlApplication::generateFlow(vector<uint32_t> gpuIndex)
{
    NS_LOG_FUNCTION(this);
    spineLeafTopo* topo = m_appManager->getTopo();
    NodeContainer gpuNodes = topo->getGpuNodes();
    Ptr<Node> sender;
    Ptr<Node> receiver;
    uint32_t resourceCnt = 0;
    m_jobStartTime = Simulator::Now().GetMilliSeconds();

    printColoredText("Job[" + to_string(m_jobId) + "] starts at " + to_string(m_jobStartTime) +
                         "ms",
                     "green");
    for (const auto& [flowId, value] : m_flowFeatures)
    {
        auto flowFeature = m_flowFeatures[flowId];

        uint32_t compTime = get<uint32_t>(flowFeature["comp_time"]);
        uint32_t commSize = get<uint32_t>(flowFeature["comm_size"]);

        printColoredText("Flow ID: " + to_string(flowId) + ", compTime: " + to_string(compTime) +
                             ", commSize: " + to_string(commSize),
                         "yellow");
        uint32_t from = gpuIndex[resourceCnt];
        uint32_t to = gpuIndex[resourceCnt + 1];

        resourceCnt += 2;
        vector<uint32_t> upstreamFlowIds = get<vector<uint32_t>>(flowFeature["upstream"]);
        vector<uint32_t> downstreamFlowIds = get<vector<uint32_t>>(flowFeature["downstream"]);

        bool first_flow = get<bool>(flowFeature["first_flow"]);
        bool last_flow = get<bool>(flowFeature["last_flow"]);
        uint32_t port = m_port + flowId;
        sender = gpuNodes.Get(from);
        receiver = gpuNodes.Get(to);
        DdlFlowSendHelper flowSendHelper(getNodeIp(receiver), port);
        flowSendHelper.SetAttribute("JobId", UintegerValue(m_jobId));
        flowSendHelper.SetAttribute("NodeId", UintegerValue(from));
        flowSendHelper.SetAttribute("FlowId", UintegerValue(flowId));
        flowSendHelper.SetAttribute("CompTime", TimeValue(MilliSeconds(compTime)));
        flowSendHelper.SetAttribute("CommSize", UintegerValue(commSize));
        ApplicationContainer flowSenderApp = flowSendHelper.Install(sender);
        Ptr<DdlFlowSendApplication> sendApp =
            DynamicCast<DdlFlowSendApplication>(flowSenderApp.Get(0));
        sendApp->setUpstreamFlowIds(upstreamFlowIds);
        sendApp->setParentDdlApp(this); // 核心出装
        // the first flows need not to wait other flows's finish
        if (first_flow)
        {
            sendApp->setUpstreamFinishStatesAllTrue();
        }
        else
        {
            sendApp->setUpstreamFinishStatesAllFalse();
        }
        DdlFlowRecvHelper flowRecvHelper(port);
        flowRecvHelper.SetAttribute("Protocol", TypeIdValue(UdpSocketFactory::GetTypeId()));
        flowRecvHelper.SetAttribute("JobId", UintegerValue(m_jobId));
        flowRecvHelper.SetAttribute("NodeId", UintegerValue(to));
        flowRecvHelper.SetAttribute("FlowId", UintegerValue(flowId));
        flowRecvHelper.SetAttribute("ExpectedBytes", UintegerValue(commSize));
        flowRecvHelper.SetAttribute("IterNum", UintegerValue(m_iterNum));
        flowRecvHelper.SetAttribute("IsLastFlow", BooleanValue(last_flow));
        ApplicationContainer flowReceiverApp = flowRecvHelper.Install(receiver);
        Ptr<DdlFlowRecvApplication> recvApp =
            DynamicCast<DdlFlowRecvApplication>(flowReceiverApp.Get(0));
        recvApp->setParentDdlApp(this); // 核心出装

        m_flowSendApp[flowId] = sendApp;
        m_flowRecvApp[flowId] = recvApp;

        flowSenderApp.Start(MilliSeconds(0)); // it means it start at current time!!!
        flowSenderApp.Stop(MilliSeconds(10000000));
        flowReceiverApp.Start(MilliSeconds(0));
        flowReceiverApp.Stop(MilliSeconds(10000000));
    }
}

// be called by the recv of the last flows
void
DdlApplication::stopAllFlows(uint32_t lastFlowId)
{
    m_lastFlowStates[lastFlowId] = true;
    // getGpuUtilizationSum();
    // if all the last flows have finished,
    // 1. change job state
    // 2. stop all the flows
    // 3. release the node resource of cluster

    if (isAllTrue(m_lastFlowStates))
    {
        m_iterCnt++;
        // 1. change job state
        setState(JobState::FINISH);
        // 2. stop all the flows in the job
        for (const auto& [flowId, value] : m_flowFeatures)
        {
            m_flowSendApp[flowId]->ddlStop();
            m_flowRecvApp[flowId]->ddlStop();
        }
        // 3. release the node resource
        m_appManager->stopApp(m_jobId);
    }
}

float
DdlApplication::getGpuUtilizationSum()
{
    float sum = 0;

    vector<float> gpuUtilizationList;
    uint32_t jobRunningTime = Simulator::Now().GetMilliSeconds() - m_jobStartTime;

    for (const auto& [flowId, value] : m_flowFeatures)
    {
        auto flowFeature = m_flowFeatures[flowId];
        uint32_t allCompTime = get<uint32_t>(flowFeature["comp_time"]) * m_iterNum;
        gpuUtilizationList.push_back((float)allCompTime / jobRunningTime);
    }
    // cout << "GPU Utilization: ";
    for (const auto& util : gpuUtilizationList)
    {
        // cout << util << " ";
        sum += util;
    }
    // cout << endl;
    // cout << "sum " << sum << endl;
    return sum;
}

void
DdlApplication::startNextFlow(uint32_t downFlowId, uint32_t finishedFlowId)
{
    NS_LOG_FUNCTION(this);
    m_flowSendApp[downFlowId]->setUpstreamFinishState(finishedFlowId, true);
}

void
DdlApplication::notifyFinish(uint32_t finishedFlowId)
{
    vector<uint32_t> downstreamFlowIds =
        std::get<vector<uint32_t>>(m_flowFeatures[finishedFlowId]["downstream"]);

    for (auto downFlowId : downstreamFlowIds)
    {
        startNextFlow(downFlowId, finishedFlowId);
    }
}

void
DdlApplication::printFlowFeatures()
{
    for (const auto& [flowId, features] : m_flowFeatures)
    {
        NS_LOG_INFO("Flow ID: " << flowId);
        for (const auto& [key, value] : features)
        {
            if (std::holds_alternative<float>(value))
            {
                NS_LOG_INFO("  " << key << ": " << std::get<float>(value));
            }
            else if (std::holds_alternative<uint32_t>(value))
            {
                NS_LOG_INFO("  " << key << ": " << std::get<uint32_t>(value));
            }
            else if (std::holds_alternative<bool>(value))
            {
                NS_LOG_INFO("  " << key << ": " << std::get<bool>(value));
            }
            else if (std::holds_alternative<std::vector<uint32_t>>(value))
            {
                NS_LOG_INFO("  " << key << ": [");
                for (const auto& v : std::get<std::vector<uint32_t>>(value))
                {
                    NS_LOG_INFO("    " << v);
                }
                NS_LOG_INFO("  ]");
            }
        }
    }
}

void
DdlApplication::initFlowFeatures()
{
    NS_LOG_FUNCTION(this);
    std::string prefix =
        "/home/yangxiaomao/ns-3-dev/src/applications/model/ddl-trace/job-generate/";
    m_flowFeatures =
        loadFlowFeaturesFromCSV(prefix + "ddl-job-" + std::to_string(m_jobId) + ".csv");
    // printFlowFeatures();
    // this controls whether the all the last flows' recv have recvs the last data
    for (const auto& [flowId, features] : m_flowFeatures)
    {
        if (std::get<bool>(features.at("last_flow")))
        {
            m_lastFlowStates[flowId] = false;
        }
    }

    float allCompTime = 0;
    float allCommSize = 0;
    float clusterBandwidth = m_appManager->getTopo()->getBandwidth();
    for (const auto& [flowId, value] : m_flowFeatures)
    {
        auto flowFeature = m_flowFeatures[flowId];
        float compTime = float(get<uint32_t>(flowFeature["comp_time"]));
        float commSize = float(get<uint32_t>(flowFeature["comm_size"]));
        allCompTime += compTime;
        allCommSize += commSize;
        m_JFPFlowFeatures[flowId]["comp_time"] = (uint32_t)compTime;
        m_JFPFlowFeatures[flowId]["comm_size"] =
            (uint32_t)(commSize / (m_appManager->getTopo()->getBandwidth() * 1000));
        // cout << "pppppppppppppppppppppppppppppppp" << endl;
        // cout << "comp_time: " << compTime << ", comm_size: " << commSize << endl;
    }
    uint32_t mega = 1000 * 1000;
    m_cruxGpuIntensity = allCompTime / allCommSize * mega;

    m_flowNum = m_flowFeatures.size();
    auto firstFlow = m_flowFeatures[0];
    m_workerNum = get<uint32_t>(m_flowFeatures[0]["worker_num"]);
    m_iterNum = get<uint32_t>(m_flowFeatures[0]["iter_num"]);
    m_arriveTimeMilliSeconds = get<float>(m_flowFeatures[0]["arrive_time"]);
    m_port = rand() % (9999 - 2000 + 1) + 2000; // 生成 [2000, 9999] 之间的随机数

    float iterTime;
    if (m_workerNum == 1)
    {
        iterTime = allCompTime;
    }
    else
    {
        iterTime = allCompTime + allCommSize / clusterBandwidth / 1000;
    }
    m_oracleRunningTime = (uint32_t)(iterTime * m_iterNum);
}

} // namespace ns3
