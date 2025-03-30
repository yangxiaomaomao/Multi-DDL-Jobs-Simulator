#ifndef DDL_JFP_H
#define DDL_JFP_H
#include <algorithm>
#include <arpa/inet.h>
#include <climits>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <sys/socket.h>
#include <unordered_set>
#include <vector>

using json = nlohmann::json;

using namespace std;
using flowInfoType = map<string, uint32_t>;
using jobInfoType = map<uint32_t, flowInfoType>;

class DdlJFP
{
  public:
    DdlJFP(map<uint32_t, vector<string>> jobUseLinkId,
           map<uint32_t, jobInfoType> jobJFPFlowIntensity,
           uint16_t solverPort);

    void initMatrixSize();

    void constructJobsFlowsInfoMatrix();
    int solveMatrix();
    void dumpJobsFlowsInfoMatrix();
    void dumpJobsFlowsPriorityMatrix();

    json convertMatrixToJson();

    map<uint32_t, map<string, uint32_t>> getPriorityMatrix()
    {
        return m_priorityMatrix;
    }

  private:
    map<uint32_t, vector<string>> m_jobUseLinkId;
    map<uint32_t, jobInfoType> m_jobJFPFlowIntensity;

    // size is m_jobNum * m_linkNum
    map<uint32_t, map<string, flowInfoType>> m_allJobsFlowsInfoMatrix;
    // size is m_jobNum * m_linkNum
    map<uint32_t, map<string, uint32_t>> m_allJobsFlowsPriorityMatrix;

    vector<uint32_t> m_jobSet;
    uint32_t m_jobNum;
    vector<string> m_linkSet;
    uint32_t m_linkNum;

    // the port to communicate with the python solver
    uint16_t m_solverPort;
    vector<uint32_t> m_tosList;
    map<uint32_t, map<string, uint32_t>> m_priorityMatrix;
};

#endif // DDL_JFP_H