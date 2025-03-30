#ifndef DDL_APP_H
#define DDL_APP_H
#include "../helper/ddl-flow-recv-helper.h"
#include "../helper/ddl-flow-send-helper.h"
#include "ddl-apps-manager.h"
#include "ddl-flow-recv.h"
#include "ddl-flow-send.h"
#include "ddl-state.h"
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

#include <cassert>
#include <variant>
#include <vector>

using namespace std;

namespace ns3
{
class DdlFlowSendApplication;
class DdlFlowRecvApplication;
class DdlAppManager;

class DdlApplication : public Application
{
  public:
    // static TypeId GetTypeId();
    DdlApplication(uint32_t jobId, DdlAppManager* appManager);
    ~DdlApplication() override;

    void generateFlow(vector<uint32_t> gpuIndex);
    void initFlowFeatures();
    void printFlowFeatures();

    uint32_t getOracleRunningTime()
    {
        return m_oracleRunningTime;
    }

    void checkAndStartApplication();

    void notifyFinish(uint32_t finishedFlowId);
    void startNextFlow(uint32_t downFlowId, uint32_t finishedFlowId);
    void stopAllFlows(uint32_t lastFlowId);
    void StartApplication() override;

    float getGpuUtilizationSum();

    void setState(JobState state)
    {
        m_state = state;
    }

    uint32_t getJobId()
    {
        return m_jobId;
    }

    float getArriveTimeMilliSeconds()
    {
        return m_arriveTimeMilliSeconds;
    }

    uint32_t getWorkerNum()
    {
        return m_workerNum;
    }

    uint32_t getFlowNum()
    {
        return m_flowNum;
    }

    float getCruxGpuIntensity()
    {
        return m_cruxGpuIntensity;
    }

    void setFlowTos(vector<uint32_t> flowTos)
    {
        if (flowTos.size() != m_flowNum)
        {
            cout << "Job[" << m_jobId << "] flowTos size is not equal to flowNum" << endl;
            cout << "flowTos size: " << flowTos.size() << ", flowNum: " << m_flowNum << endl;
            exit(0);
        }
        m_flowTos = flowTos;
    }

    vector<uint32_t> getFlowTos()
    {
        return m_flowTos;
    }

    void setGpuIndex(vector<uint32_t> gpuIndex)
    {
        m_gpuIndex = gpuIndex;
    }

    map<uint32_t, map<string, uint32_t>> getJFPFlowFeatures()
    {
        return m_JFPFlowFeatures;
    }
    

  private:
    void StopApplication() override;

    uint32_t m_jobId;
    uint32_t m_flowNum;
    uint32_t m_dp;
    uint32_t m_tp;
    uint32_t m_pp;
    uint32_t m_workerNum;

    uint32_t m_iterCnt;
    uint32_t m_iterNum;
    vector<uint32_t> m_iterTimeList;
    
    float m_arriveTimeMilliSeconds;
    uint32_t m_jobStartTime;

    // a global port for the application
    uint32_t m_port;

    std::map<uint32_t,
             std::map<std::string, std::variant<float, uint32_t, bool, std::vector<uint32_t>>>>
        m_flowFeatures;
    std::map<uint32_t, bool> m_lastFlowStates;
    // std::map<int, ApplicationContainer> m_flowSendApp;
    // std::map<int, ApplicationContainer> m_flowRecvApp;
    std::map<uint32_t, Ptr<DdlFlowSendApplication>> m_flowSendApp;
    std::map<uint32_t, Ptr<DdlFlowRecvApplication>> m_flowRecvApp;

    DdlAppManager* m_appManager;
    JobState m_state;
    vector<uint32_t> m_gpuIndex;
    vector<uint32_t> m_flowTos;

    float m_cruxGpuIntensity;
    map<uint32_t, map<string, uint32_t>> m_JFPFlowFeatures;
    uint32_t m_oracleRunningTime;
};

} // namespace ns3

#endif // DDL_APP_H