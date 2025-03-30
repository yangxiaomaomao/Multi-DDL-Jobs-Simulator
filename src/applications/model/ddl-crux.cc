#include "ddl-crux.h"

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

DdlCrux::DdlCrux(map<uint32_t, vector<string>> jobsLinkId,
                 map<uint32_t, float> jobsIntensity,
                 uint32_t iterNum,
                 uint32_t maxPrioNum)
    : m_jobsLinkId(jobsLinkId),
      m_jobsIntensity(jobsIntensity),
      m_maxPrioNum(maxPrioNum)
{
    for (const auto& [jobId, links] : m_jobsLinkId)
    {
        m_nodeSeq.push_back(jobId);
    }
    m_nodeNum = m_nodeSeq.size();
    m_iterNum = iterNum;
    // m_maxPrioNum = 2;
    cout << "Job Link ID:" << endl;
    for (const auto& [jobId, links] : m_jobsLinkId)
    {
        cout << "Job " << jobId << ": ";
        for (const auto& link : links)
        {
            cout << link << " ";
        }
        cout << endl;
    }

    cout << "Job Intensity:" << endl;
    for (const auto& [jobId, intensity] : m_jobsIntensity)
    {
        cout << "Job " << jobId << ": " << intensity << endl;
    }
}

void
DdlCrux::constructDAG()
{
    set<pair<uint32_t, uint32_t>> addedEdges; // 记录已添加的边，防止重复

    // 构建链路到任务的映射：每个链路上有哪些任务
    map<string, set<uint32_t>> linkToJobs;
    for (const auto& [job, links] : m_jobsLinkId)
    {
        for (const string& link : links)
        {
            linkToJobs[link].insert(job);
        }
    }

    // 遍历所有链路，找出共享同一链路的任务
    for (const auto& [link, jobs] : linkToJobs)
    {
        vector<uint32_t> jobList(jobs.begin(), jobs.end());

        // 任务之间两两比较，建立边
        for (size_t i = 0; i < jobList.size(); ++i)
        {
            for (size_t j = i + 1; j < jobList.size(); ++j)
            {
                uint32_t jobA = jobList[i], jobB = jobList[j];

                // 确保方向是 GPU 密度高的指向 GPU 密度低的
                if (m_jobsIntensity[jobA] >= m_jobsIntensity[jobB])
                {
                    if (addedEdges.insert({jobA, jobB}).second)
                    {
                        m_dag.push_back({jobA, jobB, m_jobsIntensity[jobA]});
                    }
                }
                else if (m_jobsIntensity[jobA] < m_jobsIntensity[jobB])
                {
                    if (addedEdges.insert({jobB, jobA}).second)
                    {
                        m_dag.push_back({jobB, jobA, m_jobsIntensity[jobB]});
                    }
                }
            }
        }
    }
    printDAG();
}

void
DdlCrux::solveDAG()
{
    cout << "solve" << endl;
    m_maxCut = 0.0;
    for (uint32_t i = 0; i < m_iterNum; i++)
    {
        vector<uint32_t> randomSeq = generateRamdomVector();
        auto C = computeCMatrix(randomSeq);
        auto [F, cut] = computeFMatrix(C, randomSeq);
        if (F[m_nodeNum][m_maxPrioNum] > m_maxCut)
        {
            m_maxCut = F[m_nodeNum][m_maxPrioNum];
            m_outputCut = cut[m_nodeNum][m_maxPrioNum];
        }
    }
    // all the jobs are in the same group
    // there are no contention between jobs
    if (m_outputCut.size() == 0)
    {
        m_outputCut.push_back(m_nodeSeq);
    }
    // cout << "Max Cut: " << m_maxCut << endl;
    // cout << "Output Cut:" << endl;
    // for (const auto& group : m_outputCut)
    // {
    //     cout << "{ ";
    //     for (uint32_t node : group)
    //     {
    //         cout << node << " ";
    //     }
    //     cout << "} ";
    // }
    // cout << endl;
}

vector<uint32_t>
DdlCrux::generateRamdomVector()
{
    // 拷贝原始 vector
    vector<uint32_t> shuffledVec = m_nodeSeq;

    // 创建一个随机数生成器
    random_device rd; // 获取随机数种子
    mt19937 g(rd());  // 使用 Mersenne Twister 生成器

    // 使用 shuffle 函数打乱数组
    shuffle(shuffledVec.begin(), shuffledVec.end(), g);
    // shuffledVec = {1, 3, 2, 4, 5};
    // shuffledVec = {1, 2, 3, 4, 5};
    shuffledVec.insert(shuffledVec.begin(), 0);
    // 返回打乱后的 vector
    return shuffledVec;
}

MatrixFloat
DdlCrux::computeCMatrix(vector<uint32_t> randomSeq)
{
    uint32_t n = randomSeq.size() - 1;
    MatrixFloat C(n + 1, vector<float>(n + 1, 0));

    for (uint32_t j = 1; j <= n; j++)
    {
        for (uint32_t i = j + 1; i <= n; i++)
        {
            for (uint32_t m = 1; m <= j; m++)
            {
                for (uint32_t k = j + 1; k <= i; k++)
                {
                    for (const auto& edge : m_dag)
                    {
                        if (edge.u == randomSeq[m] && edge.v == randomSeq[k])
                        {
                            C[j][i] += edge.weight;
                        }
                    }
                }
            }
        }
    }
    return C;
}

pair<MatrixFloat, cutMatrix>
DdlCrux::computeFMatrix(MatrixFloat C, vector<uint32_t> randomSeq)
{
    uint32_t n = C.size() - 1;
    MatrixFloat F(n + 1, vector<float>(m_maxPrioNum + 1, 0));
    cutMatrix cut(n + 1, vector<vector<vector<uint32_t>>>(m_maxPrioNum + 1));
    for (uint32_t i = 1; i <= n; i++)
    {
        for (uint32_t k = 1; k <= m_maxPrioNum; k++)
        {
            float max = 0;
            uint32_t maxJ = 0;
            for (uint32_t j = 1; j < i; j++)
            {
                float value = F[j][k - 1] + C[j][i];
                if (k == 1)
                {
                    value = 0;
                }
                if (value > max)
                {
                    max = value;
                    maxJ = j;
                }
            }
            F[i][k] = max;

            cut[i][k] = cut[maxJ][k - 1];
            vector<uint32_t> new_nodes;
            for (uint32_t m = maxJ + 1; m <= i; m++)
            {
                new_nodes.push_back(randomSeq[m]);
            }
            cut[i][k].push_back(new_nodes);
        }
    }
    // // 打印 cut 矩阵
    // for (int i = 1; i <= n; i++)
    // {
    //     for (int k = 1; k <= m_maxPrioNum; k++)
    //     {
    //         cout << "cut[" << i << "][" << k << "]: ";
    //         for (const auto& group : cut[i][k])
    //         {
    //             cout << "{ ";
    //             for (int node : group)
    //             {
    //                 cout << node << " ";
    //             }
    //             cout << "} ";
    //         }
    //         cout << endl;
    //     }
    // }
    return {F, cut};
}

void
DdlCrux::printDAG()
{
    cout << "DAG:" << endl;
    for (const auto& edge : m_dag)
    {
        cout << edge.u << " -> " << edge.v << " : " << edge.weight << endl;
    }
}