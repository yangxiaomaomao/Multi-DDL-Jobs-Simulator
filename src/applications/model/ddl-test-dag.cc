#include <iostream>
#include <vector>
#include <map>
#include <set>

using namespace std;

struct Edge {
    uint32_t u, v;
    float weight; // weight 现在是 float 类型
};

vector<Edge> constructDAG(map<uint32_t, vector<string>>& jobUseLinkId, map<uint32_t, float>& intensity) {
    vector<Edge> edges;
    set<pair<uint32_t, uint32_t>> addedEdges; // 记录已添加的边，防止重复

    // 构建链路到任务的映射：每个链路上有哪些任务
    map<string, set<uint32_t>> linkToJobs;
    for (const auto& [job, links] : jobUseLinkId) {
        for (const string& link : links) {
            linkToJobs[link].insert(job);
        }
    }

    // 遍历所有链路，找出共享同一链路的任务
    for (const auto& [link, jobs] : linkToJobs) {
        vector<uint32_t> jobList(jobs.begin(), jobs.end());

        // 任务之间两两比较，建立边
        for (size_t i = 0; i < jobList.size(); ++i) {
            for (size_t j = i + 1; j < jobList.size(); ++j) {
                uint32_t jobA = jobList[i], jobB = jobList[j];

                // 确保方向是 GPU 密度高的指向 GPU 密度低的
                if (intensity[jobA] > intensity[jobB]) {
                    if (addedEdges.insert({jobA, jobB}).second) {
                        edges.push_back({jobA, jobB, intensity[jobA]});
                    }
                } else if (intensity[jobA] < intensity[jobB]) {
                    if (addedEdges.insert({jobB, jobA}).second) {
                        edges.push_back({jobB, jobA, intensity[jobB]});
                    }
                }
            }
        }
    }

    return edges;
}

int main() {
    // 示例数据
    map<uint32_t, vector<string>> jobUseLinkId = {
        {0, {"link01", "link12"}},
        {1, {"link12", "link23"}},
        {2, {"link23", "link34"}},
        {3, {"link01", "link34"}},
        {4, {"link12", "link34"}}
    };
    map<uint32_t, float> intensity = {
        {0, 1.66},
        {1, 2.5},
        {2, 1.2},
        {3, 3.0},
        {4, 1.8}
    };

    vector<Edge> dag = constructDAG(jobUseLinkId, intensity);

    // 打印 DAG 结果
    for (const Edge& edge : dag) {
        cout << "Edge: " << edge.u << " -> " << edge.v << " (weight: " << edge.weight << ")" << endl;
    }

    return 0;
}
