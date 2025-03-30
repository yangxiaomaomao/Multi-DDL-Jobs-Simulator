#include "ddl-apps-manager.h"

#include "../helper/ddl-flow-recv-helper.h"
#include "../helper/ddl-flow-send-helper.h"
#include "ddl-JFP.h"
#include "ddl-app.h"
#include "ddl-crux.h"
#include "ddl-flow-recv.h"
#include "ddl-flow-send.h"
#include "ddl-state.h"
#include "ddl-topo.h"

#include "ns3/applications-module.h"
#include "ns3/boolean.h"
#include "ns3/ddl-app.h"
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

#include <variant>
#include <vector>

using namespace std;

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("DdlAppManager");

DdlAppManager::DdlAppManager(spineLeafTopo* topo,
                             string placeStrategy,
                             string tosStrategy,
                             uint16_t solverPort,
                             bool cruxPlus)
    : m_placeStrategy(placeStrategy),
      m_tosStrategy(tosStrategy),
      m_topo(topo),
      m_solverPort(solverPort)

{
    NS_LOG_FUNCTION(this);
    initGpuStates();
    startPythonSolver(cruxPlus);
}

DdlAppManager::~DdlAppManager()
{
    NS_LOG_FUNCTION(this);
}

// this is yinyong's code
void
eraseMapElement(std::map<uint32_t, DdlApplication*>* map, uint32_t key)
{
    auto it = (*map).find(key);
    if (it != (*map).end())
    {
        (*map).erase(key);
    }
}

void
addMapElement(std::map<uint32_t, DdlApplication*>* map, DdlApplication* value)
{
    uint32_t key = value->getJobId();
    (*map)[key] = value;
}

void
DdlAppManager::initGpuStates()
{
    NS_LOG_FUNCTION(this);
    uint32_t m_gpuNum = m_topo->getGpuNum();
    m_gpuStates.resize(m_gpuNum);
    for (uint32_t i = 0; i < m_gpuNum; ++i)
    {
        m_gpuStates[i] = GpuState::FREE;
    }
}

void
DdlAppManager::setGpuState(vector<uint32_t> gpuIndex, GpuState state)
{
    NS_LOG_FUNCTION(this);
    for (auto index : gpuIndex)
    {
        m_gpuStates[index] = state;
    }
}

vector<uint32_t>
DdlAppManager::getFreeGpuIndex()
{
    NS_LOG_FUNCTION(this);
    vector<uint32_t> freeGpuIndex;
    for (uint32_t i = 0; i < m_gpuStates.size(); i++)
    {
        if (m_gpuStates[i] == GpuState::FREE)
        {
            freeGpuIndex.push_back(i);
        }
    }
    return freeGpuIndex;
}

vector<uint32_t>
DdlAppManager::sequencePlacement(DdlApplication* job)
{
    NS_LOG_FUNCTION(this);
    uint32_t workerNum = job->getWorkerNum();
    vector<uint32_t> freeGpuIndex = getFreeGpuIndex();

    // no matter what placer, not enough gpu leads to failure
    if (freeGpuIndex.size() < workerNum)
    {
        return {};
    }
    if (workerNum == 1)
    {
        vector<uint32_t> gpuIndex(2, freeGpuIndex[0]);
        return gpuIndex;
    }
    else
    {
        vector<uint32_t> tmp(freeGpuIndex.begin(), freeGpuIndex.begin() + workerNum);
        bool test = false;
        if (test)
        {
            uint32_t jobId = job->getJobId();
            if (jobId == 1)
            {
                return {1, 4};
            }
            else if (jobId == 2)
            {
                return {0, 3, 3, 6};
            }
            else if (jobId == 3)
            {
                return {5, 7};
            }
            else if (jobId == 4)
            {
                return {2, 7};
            }
            else if (jobId == 5)
            {
                return {9, 12};
            }
            else if (jobId == 6)
            {
                return {4, 4};
            }
        }


        vector<uint32_t> gpuIndex = duplicateMiddleElements(tmp);
        for (const auto& index : gpuIndex)
        {
            cout << "GPU Index: " << index << endl;
        }
        return gpuIndex;
    }
}
// FIFO scheduler
bool DdlAppManager::jobIsFirstArrived(DdlApplication* job)
{
    if (m_pendingApps.empty())
    {
        // if a job need not to be pended, it will arrive here
        return true;
    }
    vector<uint32_t> pendingJobArriveList;
    for (auto& [jobId, job] : m_pendingApps)
    {
        pendingJobArriveList.push_back(job->getArriveTimeMilliSeconds());
    }
    sort(pendingJobArriveList.begin(), pendingJobArriveList.end());
    uint32_t jobArriveTime = job->getArriveTimeMilliSeconds();
    if (pendingJobArriveList[0] == jobArriveTime)
    {
        return true;
    }
    else
    {
        return false;
    }
}

vector<uint32_t>
DdlAppManager::consolidatePlacement(DdlApplication* job)
{
    NS_LOG_FUNCTION(this);
    uint32_t workerNum = job->getWorkerNum();
    vector<uint32_t> freeGpuIndex = getFreeGpuIndex();

    uint32_t jobId = job->getJobId();
    // to test spine=3,leaf=3 and gpuperleaf=5
    if (jobId == 1)
    {
        return {0, 5, 5, 10};
    }
    else if (jobId == 2)
    {
        return {1, 6};
    }
    else if (jobId == 3)
    {
        return {8, 11};
    }
    else if (jobId == 4)
    {
        return {2, 7};
    }
    else if (jobId == 5)
    {
        return {9, 12};
    }
    else if (jobId == 6)
    {
        return {4, 4};
    }

    // no matter what placer, not enough gpu leads to failure
    if (freeGpuIndex.size() < workerNum)
    {
        return {};
    }
    vector<uint32_t> gpuIndex(workerNum);
    gpuIndex.assign(freeGpuIndex.begin(), freeGpuIndex.begin() + workerNum);
    return gpuIndex;
}

vector<uint32_t>
DdlAppManager::loadBalancePlacement(DdlApplication* job)
{
    NS_LOG_FUNCTION(this);
    uint32_t workerNum = job->getWorkerNum();
    vector<uint32_t> freeGpuIndex = getFreeGpuIndex();

    // no matter what placer, not enough gpu leads to failure
    if (freeGpuIndex.size() < workerNum || !jobIsFirstArrived(job))
    {
        return {};
    }

    uint32_t gpusPerLeaf = m_topo->getGpuNumPerLeaf();
    std::unordered_map<int, std::vector<uint32_t>> leafToGpus;  // leafId -> GPU 编号列表
    for (uint32_t gpu : freeGpuIndex) {
        int leafId = gpu / gpusPerLeaf;
        leafToGpus[leafId].push_back(gpu);
    }
    std::vector<uint32_t> tmp;

    while (tmp.size() < workerNum) {
        // 将 leaf 按 GPU 数量排序，优先从 GPU 数最多的 leaf 中选择
        std::vector<std::pair<int, std::vector<uint32_t>>> sortedLeafs(leafToGpus.begin(), leafToGpus.end());
        std::sort(sortedLeafs.begin(), sortedLeafs.end(), [](auto &a, auto &b) {
            return a.second.size() > b.second.size();  // 按 GPU 数量降序排序
        });
        // 从 GPU 数最多的 leaf 中选择一个 GPU
        for (auto &leaf : sortedLeafs) {
            if (!leaf.second.empty()) {
                tmp.push_back(leaf.second.back());  // 选择一个 GPU
                leaf.second.pop_back();  // 从 leaf 中移除已选 GPU
                leafToGpus[leaf.first] = leaf.second;
                break;  // 每次只选一个，跳出循环
            }
        }
    }

    if(tmp.size() == 1){
        tmp = {tmp[0], tmp[0]};
    }
    // if(jobId == 6){
    //     tmp = {21,15,9,3,19,13,7,1};
    // }else if(jobId == 8){
    //     tmp = {2,20,14,8};
    // }
    vector<uint32_t> gpuIndex = duplicateMiddleElements(tmp);
    return gpuIndex;
}

void
DdlAppManager::adaptJobsFlowTos()
{
    NS_LOG_FUNCTION(this);
    if (m_tosStrategy == "crux")
    {
        adaptJobsFlowTosCrux();
    }
    else if (m_tosStrategy == "equal")
    {
        adaptJobsFlowTosEqual();
    }
    else if (m_tosStrategy == "JFP" || m_tosStrategy == "crux+")
    {
        adaptJobsFlowTosJFP();
    }
    else
    {
        NS_LOG_INFO("Not supported tos strategy: " << m_tosStrategy);
        exit(0);
    }
}

void
DdlAppManager::adaptJobsFlowTosEqual()
{
    NS_LOG_FUNCTION(this);

    // assign all the flows of all the job to the same priority
    for (auto& [jobId, job] : m_runningApps)
    {
        uint32_t flowNum = job->getFlowNum();
        cout << "Job ID: " << jobId << ", Flow Num: " << flowNum << endl;
        vector<uint32_t> flowTos(flowNum, 0);
        job->setFlowTos(flowTos);
    }
}

void
DdlAppManager::adaptJobsFlowTosCrux()
{
    NS_LOG_FUNCTION(this);
    // we should adapt the flow tos to suit the new running jobs set
    // we should first get the all running jobs set
    // 1. get the map from leaf to spine
    map<uint32_t, uint32_t> leafSpineMap = m_topo->getLeafSpineMap();
    // 2. get the all running jobs's using gpuIndex, that is m_jobGPU,
    // the job itself don't save the gpuIndex, it is saved in the manager
    // 3. get the link id from the leafSpineMap and the placement(gpuIndex)
    //  and get the job intensity
    map<uint32_t, vector<string>> jobUseLinkId;
    map<uint32_t, float> jobIntensity;

    for (auto& [jobId, job] : m_runningApps)
    {
        jobIntensity[jobId] = job->getCruxGpuIntensity();
        vector<uint32_t> useGpuIndex = m_jobGPU[jobId];
        vector<string> useLinkId;
        for (uint32_t i = 0; i < useGpuIndex.size(); i += 2) // attention here
        {
            uint32_t senderGpuId = useGpuIndex[i];
            uint32_t receiverGpuId = useGpuIndex[i + 1];
            uint32_t gpuNumPerLeaf = m_topo->getGpuNumPerLeaf();
            uint32_t senderLeafId = senderGpuId / gpuNumPerLeaf;
            uint32_t senderSpineId = leafSpineMap[senderLeafId];
            if (senderGpuId / gpuNumPerLeaf == receiverGpuId / gpuNumPerLeaf)
            {
                continue;
            }
            useLinkId.push_back("link" + to_string(senderLeafId) + to_string(senderSpineId));
        }
        jobUseLinkId[jobId] = useLinkId;
    }
    // 4. constrcut the DAG of crux through jobUseLinkId and jobIntensity
    //    and get the output cut, each group have the same tos
    vector<uint32_t> prioList = {0,1,2,3,4,5,6,7};

    DdlCrux crux(jobUseLinkId, jobIntensity, 10, prioList.size());
    crux.constructDAG();
    crux.solveDAG();
    vector<vector<uint32_t>> output = crux.getOutputCut();

    // 5. set tos
    uint32_t groupNum = output.size();

    for (uint32_t groupId = 0; groupId < groupNum; groupId++)
    {
        vector<uint32_t> group = output[groupId];
        uint32_t tos = prioList[groupId];
        
        for (auto& jobId : group)
        {
            auto job = m_runningApps[jobId];
            uint32_t flowNum = job->getFlowNum();
            vector<uint32_t> tosList(flowNum, tos);
            m_runningApps[jobId]->setFlowTos(tosList);
            cout << "Job ID: " << jobId << ", TOS List: ";
            cout << "{";
            for (const auto& tosValue : tosList)
            {
                cout << tosValue << " ";
            }
            cout << "}";
            cout << endl;
        }
    }
}

void
DdlAppManager::adaptJobsFlowTosJFP()
{
    NS_LOG_FUNCTION(this);
    // we should adapt the flow tos to suit the new running jobs set
    // we should first get the all running jobs set
    // 1. get the map from leaf to spine
    map<uint32_t, uint32_t> leafSpineMap = m_topo->getLeafSpineMap();
    // 2. get the all running jobs's using gpuIndex, that is m_jobGPU,
    // the job itself don't save the gpuIndex, it is saved in the manager
    // 3. get the link id from the leafSpineMap and the placement(gpuIndex)
    //  and get the job intensity
    map<uint32_t, vector<string>> jobUseLinkId;
    map<uint32_t, map<uint32_t, map<string, uint32_t>>> jobJFPFlowIntensity;

    for (auto& [jobId, job] : m_runningApps)
    {
        jobJFPFlowIntensity[jobId] = job->getJFPFlowFeatures();
        vector<uint32_t> useGpuIndex = m_jobGPU[jobId];
        vector<string> useLinkId;
        for (uint32_t i = 0; i < useGpuIndex.size(); i += 2) // attention here
        {
            uint32_t senderGpuId = useGpuIndex[i];
            uint32_t receiverGpuId = useGpuIndex[i + 1];
            cout << "Sender GPU ID: " << senderGpuId << ", Receiver GPU ID: " << receiverGpuId << endl;
            uint32_t gpuNumPerLeaf = m_topo->getGpuNumPerLeaf();
            uint32_t senderLeafId = senderGpuId / gpuNumPerLeaf;
            uint32_t senderSpineId = leafSpineMap[senderLeafId];
            if (senderGpuId / gpuNumPerLeaf == receiverGpuId / gpuNumPerLeaf)
            {
                // noting that the link is not the link between leaf and spine, appears once at most
                useLinkId.push_back("gpulink" + to_string(senderGpuId));
            }
            else
            {
                // noting that the link is the link between leaf and spine
                useLinkId.push_back("link" + to_string(senderLeafId) + to_string(senderSpineId));
            }
        }
        jobUseLinkId[jobId] = useLinkId;
    }
    DdlJFP jfp(jobUseLinkId, jobJFPFlowIntensity, m_solverPort);
    jfp.solveMatrix();
    map<uint32_t, map<string, uint32_t>> output = jfp.getPriorityMatrix();

    // here the JFP solver return the opposite result, 
    // the smaller the priority, the lower the priority
    // so here we reverse it
    vector<uint32_t> prioList = {7,6,5,4,3,2,1,0};

    for (auto& [jobId, job] : m_runningApps)
    {
        map<string, uint32_t> flowPriority = output[jobId];
        vector<uint32_t> tosList;
        vector<string> links = jobUseLinkId[jobId];
        // the link are complete
        cout << "Job ID: " << jobId << ", Link List: " << endl;
        for (auto& link : links)
        {
            tosList.push_back(prioList[flowPriority[link]]);
            cout << "Link: " << link << ", TOS: " << prioList[flowPriority[link]] << endl;
        }

        job->setFlowTos(tosList);
    }
    // for (auto& [jobId, job] : m_runningApps)
    // {
    //     vector<uint32_t> tosList;
    //     if (jobId == 1)
    //     {
    //         tosList = {46 << 2};
    //     }
    //     else if (jobId == 2)
    //     {
    //         tosList = {60 << 2, 60 << 2};
    //     }
    //     else if (jobId == 3)
    //     {
    //         tosList = {46 << 2};
    //     }
    //     job->setFlowTos(tosList);
    // }
}

void
DdlAppManager::addApp(DdlApplication* job)
{
    NS_LOG_FUNCTION(this);
    // the queue contains all the unarrived job
    addMapElement(&m_unArrivedApps, job);
    addMapElement(&m_allApps, job);
}

// return the GPU Node it assigns to the job
std::vector<uint32_t>
DdlAppManager::getJobPlacement(DdlApplication* job)
{
    // this function decide whether a new job can be run
    // so it is important
    NS_LOG_FUNCTION(this);
    vector<uint32_t> gpuIndex;
    uint32_t jobId = job->getJobId();

    if (m_jobGPU.find(jobId) == m_jobGPU.end())
    {
        eraseMapElement(&m_unArrivedApps, jobId);
        m_jobStatistics[jobId]["arriveTime"] = (uint32_t)Simulator::Now().GetMilliSeconds();
    }

    if (m_placeStrategy == "sequence")
    {
        gpuIndex = sequencePlacement(job);
    }
    else if (m_placeStrategy == "consolidate")
    {
        gpuIndex = consolidatePlacement(job);
    }else if (m_placeStrategy == "lb")
    {
        gpuIndex = loadBalancePlacement(job);
    }
    else
    {
        NS_LOG_INFO("Not supported place strategy: " << m_placeStrategy);
        exit(0);
    }
    //
    if (gpuIndex.size() != 0)
    {
        m_jobGPU[jobId] = gpuIndex;
        printColoredText("分配Job[" + to_string(jobId) + "]到", "yellow");
        for (const auto& index : gpuIndex)
        {
            cout << index << " ";
        }
        cout << endl;
        setGpuState(gpuIndex, GpuState::BUSY);
        // this mean a new job will be run immediately,
        // we queue it to running queue,
        // and we remove it from no matter unarrived or pending queue
        // We should adopt [ALLALLALL] the job/flow's tos to
        // suit the new running jobs set, it is also the
        // key point of my work.
        eraseMapElement(&m_pendingApps, jobId);
        addMapElement(&m_runningApps, job);
        adaptJobsFlowTos();
        m_jobStatistics[jobId]["startTime"] = (uint32_t)Simulator::Now().GetMilliSeconds();
        m_jobStatistics[jobId]["placement"] = removeDuplicates(gpuIndex);
    }
    else
    {
        // this mean the job arrive but no enough resources
        // we queue it to pending queue
        // and we remove it from unarrived queue
        // the job may not in unarrive queue
        m_jobGPU[jobId] = gpuIndex; // to deicde whether the job has arrived
        addMapElement(&m_pendingApps, job);
    }
    return gpuIndex;
}

void
DdlAppManager::runApp()
{
    NS_LOG_FUNCTION(this);
    // auto job = m_unArrivedApps[1];
    // job->checkAndStartApplication();
    for (auto& [jobId, job] : m_allApps)
    {
        job->checkAndStartApplication();
    }
}

void
DdlAppManager::stopApp(uint32_t jobId)
{
    NS_LOG_FUNCTION(this);
    // release the node resource
    vector<uint32_t> gpuUseIndex = m_jobGPU[jobId];
    for (auto nodeid : gpuUseIndex)
    {
        NS_LOG_INFO("Job[" << jobId << "]:" << Simulator::Now().As(Time::MS) << "ms. Set the node["
                           << nodeid << "] free");
    }
    m_jobGPU.erase(jobId);
    setGpuState(gpuUseIndex, GpuState::FREE);
    // add it to the finish queue
    // and remove it from running queue
    // we will use the running queue to get point,
    // so here we should first add it to finish queue then remove
    addMapElement(&m_finishedApps, m_runningApps[jobId]);
    eraseMapElement(&m_runningApps, jobId);
    m_jobStatistics[jobId]["finishTime"] = (uint32_t)Simulator::Now().GetMilliSeconds();
    m_jobStatistics[jobId]["gpuUtilization"] = m_allApps[jobId]->getGpuUtilizationSum();
    // every time a job finishs, we should adapt the flow tos again
    if ((m_tosStrategy == "crux" || m_tosStrategy == "JFP") && m_runningApps.size() > 0)
    {
        adaptJobsFlowTos();
    }
}

void
DdlAppManager::dumpJobStatistics(string filename)
{
    NS_LOG_FUNCTION(this);
    ofstream file(filename);
    if (!file.is_open())
    {
        cerr << "无法打开文件: " << filename << endl;
        return;
    }

    // 写入表头
    file << "jobId,arriveTime,startTime,finishTime,actualRunningTime,oracleRunningTime,JCT,gpuUtilization,"
            "placement\n";

    for (const auto& [jobId, jobInfo] : m_jobStatistics)
    {
        uint32_t arriveTime = get<uint32_t>(jobInfo.at("arriveTime"));
        uint32_t startTime = get<uint32_t>(jobInfo.at("startTime"));
        uint32_t finishTime = get<uint32_t>(jobInfo.at("finishTime"));
        uint32_t actualRunningTime = finishTime - startTime;
        uint32_t oracleRunningTime = m_allApps[jobId]->getOracleRunningTime();
        uint32_t JCT = finishTime - arriveTime;
        float gpuUtilization = get<float>(jobInfo.at("gpuUtilization"));

        file << jobId << "," << arriveTime << "," << startTime << "," << finishTime << ","
             << actualRunningTime << "," << oracleRunningTime << "," << JCT << "," << gpuUtilization << ",";

        // 获取 placement
        if (jobInfo.count("placement"))
        {
            const vector<uint32_t>& placement = get<vector<uint32_t>>(jobInfo.at("placement"));
            for (size_t i = 0; i < placement.size(); i++)
            {
                file << placement[i];
                if (i < placement.size() - 1)
                    file << "-";
            }
        }

        file << "\n";
    }

    file.close();
    cout << "CSV 文件 " << filename << " 写入成功！" << endl;
}

} // namespace ns3