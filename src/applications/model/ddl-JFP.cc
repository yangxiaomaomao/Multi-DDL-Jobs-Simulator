#include "ddl-JFP.h"

#include "ddl-tools.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

using namespace std;

DdlJFP::DdlJFP(map<uint32_t, vector<string>> jobUseLinkId,
               map<uint32_t, jobInfoType> jobJFPFlowIntensity,
               uint16_t solverPort)
    : m_jobUseLinkId(jobUseLinkId),
      m_jobJFPFlowIntensity(jobJFPFlowIntensity),
      m_solverPort(solverPort)
{
    initMatrixSize();
    constructJobsFlowsInfoMatrix();
    // int solveRes = solveMatrix();
    // if (solveRes == -1)
    // { 
    //     cout << "solve matrix failed" << endl;
    //     exit(0);
    // }
    // dumpJobsFlowsInfoMatrix();
}

void
DdlJFP::initMatrixSize()
{
    m_jobNum = m_jobUseLinkId.size();
    for (const auto& [jobId, links] : m_jobUseLinkId)
    {
        m_jobSet.push_back(jobId);
        for (const auto& link : links)
        {
            if (find(m_linkSet.begin(), m_linkSet.end(), link) == m_linkSet.end())
            {
                m_linkSet.push_back(link);
            }
        }
    }
    m_linkNum = m_linkSet.size();
    // init the flow matrix
    for (const auto& jobId : m_jobSet)
    {
        for (const auto& linkName : m_linkSet)
        {
            m_allJobsFlowsInfoMatrix[jobId][linkName] = {{"comp_time", 0}, {"comm_size", 0}};
        }
    }
    cout << "Link Set: ";
    for (const auto& link : m_linkSet)
    {
        cout << link << " ";
    }
    cout << endl;
}

void
DdlJFP::constructJobsFlowsInfoMatrix()
{
    for (const auto& jobId : m_jobSet)
    {
        vector<string> jobUseLinks = m_jobUseLinkId[jobId];
        jobInfoType jobFlows = m_jobJFPFlowIntensity[jobId];
        uint32_t flowNum = jobFlows.size();
        for(uint32_t flowId = 0; flowId < flowNum; flowId++)
        {
            string linkName = jobUseLinks[flowId];

            m_allJobsFlowsInfoMatrix[jobId][linkName]["comp_time"] += jobFlows[flowId]["comp_time"];
            m_allJobsFlowsInfoMatrix[jobId][linkName]["comm_size"] += jobFlows[flowId]["comm_size"];
        }
    }
    // for (const auto& jobId : m_jobSet)
    // {
    //     vector<string> jobUseLinks = m_jobUseLinkId[jobId];
    //     jobInfoType jobFlows = m_jobJFPFlowIntensity[jobId];
    //     for (uint32_t linkId = 0; linkId < m_linkNum; linkId++)
    //     {
    //         string linkName = m_linkSet[linkId];
    //         printColoredText(linkName,"blue");

    //         // cout << "Job " << jobId << " uses link " << linkName << endl;
    //         auto it = find(jobUseLinks.begin(), jobUseLinks.end(), linkName);

    //         if (it == jobUseLinks.end())
    //         { // the job does not use the link, write (0,0)
    //             m_allJobsFlowsInfoMatrix[jobId][linkName] = {{"comp_time", 0}, {"comm_size", 0}};
    //         }
    //         else
    //         { // the job is using the link, return the exact flow info
    //             uint32_t flowId = distance(jobUseLinks.begin(), it);
    //             m_allJobsFlowsInfoMatrix[jobId][linkName] = jobFlows[flowId];
    //         }
    //     }
    // }
    dumpJobsFlowsInfoMatrix();
}

int
DdlJFP::solveMatrix()
{
    // the input matrix's shape is the same as m_allJobsFlowsInfoMatrix
    // each element represents the priority of the flow of the job on the link
    // the priority is a int number, the larger the number, the higher the priority
    json testJson = convertMatrixToJson();
    string jsonStr = testJson.dump();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        cout << "socket error" << endl;
        return -1;
    }
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(m_solverPort);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
        cout << "connect error" << endl;
        return -1;
    }
    send(sock, jsonStr.c_str(), jsonStr.size(), 0);

    char buffer[1024] = {0};
    int bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read < 0)
    {
        cout << "no solver response" << endl;
    }
    buffer[bytes_read] = '\0';
    
    try
    {
        json j = json::parse(buffer);
        for (auto& [jobId, jobJson] : j.items())
        {
            uint32_t jobIdInt = stoi(jobId); // Convert jobId to uint32_t
            for (auto& [linkName, flowPrio] : jobJson.items())
            {
                m_priorityMatrix[jobIdInt][linkName] = flowPrio;
            }
        }
    }
    catch (json::parse_error& e)
    {
        cout << "parse error: " << e.what() << endl;
    }

    close(sock);
    printColoredText("优先级：", "yellow");

    dumpJobsFlowsPriorityMatrix();

    return 1;
}

json
DdlJFP::convertMatrixToJson()
{
    json j;
    for (const auto& [jobId, jobFlows] : m_allJobsFlowsInfoMatrix)
    {
        json jobJson;
        for (const auto& [linkName, flowInfo] : jobFlows)
        {
            json flowJson;
            for (const auto& [key, value] : flowInfo)
            {
                flowJson[key] = value;
            }
            jobJson[linkName] = flowJson;
        }
        j[to_string(jobId)] = jobJson;
    }
    return j;
}

void
DdlJFP::dumpJobsFlowsInfoMatrix()
{
    // 首先提取所有链路 ID（string 类型），用于作为表格的列索引
    map<string, bool> allLinks; // 使用 map 来收集所有链路 ID，避免重复
    for (const auto& job : m_allJobsFlowsInfoMatrix)
    {
        for (const auto& link : job.second)
        {
            allLinks[link.first] = true; // 记录链路 ID
        }
    }

    // 打印表头：先打印空白区域以对齐行索引（任务 ID）
    cout << left << setw(12) << "Job ID";
    for (const auto& link : allLinks)
    {
        cout << setw(20) << link.first; // 打印每个链路 ID 作为表头
    }
    cout << endl;
    cout << string(12 + 20 * allLinks.size(), '-') << endl; // 分割线

    // 遍历任务 ID（行索引）并打印对应的 comp_time 和 comm_size
    for (const auto& job : m_allJobsFlowsInfoMatrix)
    {
        uint32_t taskId = job.first; // 任务 ID
        cout << left << setw(12) << taskId;

        // 打印每个链路上的 comp_time 和 comm_size
        for (const auto& link : allLinks)
        {
            const auto& taskMap = job.second;
            if (taskMap.find(link.first) != taskMap.end())
            {
                const flowInfoType& flowInfo = taskMap.at(link.first);
                uint32_t compTime = flowInfo.count("comp_time") ? flowInfo.at("comp_time") : 0;
                uint32_t commSize = flowInfo.count("comm_size") ? flowInfo.at("comm_size") : 0;
                cout << setw(20) << ("CT: " + to_string(compTime) + ", CS: " + to_string(commSize));
            }
            else
            {
                // 如果该链路 ID 不存在，则打印空白
                cout << setw(20) << "-";
            }
        }
        cout << endl;
    }
}

void DdlJFP::dumpJobsFlowsPriorityMatrix(){
    // 提取所有链路 ID 作为列索引
    map<string, bool> allLinks;
    for (const auto& job : m_priorityMatrix) {
        for (const auto& link : job.second) {
            allLinks[link.first] = true;
        }
    }

    // 打印表头（任务 ID 及所有链路 ID）
    cout << left << setw(12) << "Job ID";
    for (const auto& link : allLinks) {
        cout << setw(12) << link.first;
    }
    cout << endl;
    cout << string(12 + 12 * allLinks.size(), '-') << endl;  // 分割线

    // 遍历每个任务及其链路优先级
    for (const auto& job : m_priorityMatrix) {
        uint32_t taskId = job.first;  // 当前任务 ID
        cout << left << setw(12) << taskId;

        for (const auto& link : allLinks) {
            uint32_t priority = job.second.at(link.first);  // 获取该任务对应链路的优先级
            cout << setw(12) << priority;
        }
        cout << endl;
    }
}