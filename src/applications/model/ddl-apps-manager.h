#ifndef DDL_APPS_MANAGER_H
#define DDL_APPS_MANAGER_H
#include "../helper/ddl-flow-recv-helper.h"
#include "../helper/ddl-flow-send-helper.h"
#include "ddl-app.h"
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

#include <unistd.h>
#include <variant>
#include <vector>
using namespace std;

namespace ns3
{
class DdlApplication;

class DdlAppManager
{
  public:
    DdlAppManager(spineLeafTopo* topo,
                  string placeStrategy,
                  string tosStrategy,
                  uint16_t solverPort,
                  bool cruxPlus);
    ~DdlAppManager();

    void addApp(DdlApplication* job);
    void runApp();
    void stopApp(uint32_t jobId);

    // gpu states related functions
    void initGpuStates();
    void setGpuState(vector<uint32_t> gpuIndex, GpuState state);

    // placement strategy related functions
    vector<uint32_t> getJobPlacement(DdlApplication* job);
    bool jobIsFirstArrived(DdlApplication* job);
    void adaptJobsFlowTos();
    void adaptJobsFlowTosCrux();
    void adaptJobsFlowTosEqual();
    void adaptJobsFlowTosJFP();
    

    void startPythonSolver(bool cruxPlus)
    { 
        // start the python solver by the port
        string solverPath;
        if (cruxPlus)
        {
            solverPath = "/home/yangxiaomao/ns-3-dev/JFP/optimize/run_crux+.py ";
        }
        else
        {
            solverPath = "/home/yangxiaomao/ns-3-dev/JFP/optimize/run_JFP.py ";
        }
        string startCommand = "python3 " + solverPath + to_string(m_solverPort) + " &";
        int ret = system(startCommand.c_str());
        if (ret == -1)
        {
            perror("Solver start failed");
            exit(0);
        }
        sleep(3);
    }

    vector<uint32_t> getFreeGpuIndex();
    vector<uint32_t> consolidatePlacement(DdlApplication* job);
    vector<uint32_t> sequencePlacement(DdlApplication* job);
    vector<uint32_t> loadBalancePlacement(DdlApplication* job);

    void dumpJobStatistics(string filename);

    spineLeafTopo* getTopo()
    {
        return m_topo;
    }

  private:
    string m_placeStrategy;
    string m_tosStrategy;

    std::map<uint32_t, DdlApplication*> m_allApps;

    std::map<uint32_t, DdlApplication*> m_unArrivedApps;

    std::map<uint32_t, DdlApplication*> m_pendingApps;

    std::map<uint32_t, DdlApplication*> m_runningApps;

    std::map<uint32_t, DdlApplication*> m_finishedApps;

    spineLeafTopo* m_topo;
    std::map<uint32_t, std::vector<uint32_t>> m_jobGPU; // jobId->gpuNodeIndexVector
    std::vector<GpuState> m_gpuStates;                  // 0: free, 1: busy
    uint32_t m_gpuNum;

    map<uint32_t, map<string, variant<uint32_t, float, vector<uint32_t>>>> m_jobStatistics;

    // python solver port
    uint16_t m_solverPort;
};
} // namespace ns3
#endif