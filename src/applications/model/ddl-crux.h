#ifndef DDL_CRUX_H
#define DDL_CRUX_H
#include <algorithm>
#include <climits>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;
typedef vector<vector<float>> MatrixFloat;
typedef vector<vector<uint32_t>> MatrixInteger;
typedef vector<vector<vector<vector<uint32_t>>>> cutMatrix;

struct Edge
{
    uint32_t u, v;
    float weight; // weight 现在是 float 类型
};

class DdlCrux
{
  public:
    DdlCrux(map<uint32_t, vector<string>>, map<uint32_t, float>, uint32_t, uint32_t);
    void constructDAG();
    void solveDAG();

    vector<uint32_t> generateRamdomVector();
    MatrixFloat computeCMatrix(vector<uint32_t> randomSeq);
    pair<MatrixFloat, cutMatrix> computeFMatrix(MatrixFloat C, vector<uint32_t> randomSeq);

    void printDAG();

    MatrixInteger getOutputCut()
    {   
        cout << "output size" << m_outputCut.size() << endl;
        for (const auto& group : m_outputCut)
        {
            cout << "{ ";
            for (uint32_t node : group)
            {
                cout << node << " ";
            }
            cout << "} ";
        }
        cout << endl;
        return m_outputCut;
    }

  private:
    map<uint32_t, vector<string>> m_jobsLinkId;
    map<uint32_t, float> m_jobsIntensity;

    vector<uint32_t> m_nodeSeq;
    uint32_t m_nodeNum;

    vector<Edge> m_dag;
    float m_maxCut;
    MatrixInteger m_outputCut;
    uint32_t m_iterNum;
    uint32_t m_maxPrioNum;
};

#endif