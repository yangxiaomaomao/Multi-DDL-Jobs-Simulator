
#include <algorithm>
#include <climits>
#include <iostream>
#include <queue>
#include <random>
#include <unordered_set>
#include <vector>

using namespace std;
typedef vector<vector<int>> Matrix;
typedef vector<vector<vector<vector<int>>>> cutMatrix;

// 图的边的表示
struct Edge
{
    int u, v, weight; // 边的两个端点和边的权重
};

vector<int>
bfsTraversal(const vector<Edge>& edges)
{
    unordered_map<int, vector<int>> adjList; // 邻接表
    unordered_map<int, int> inDegree;        // 记录入度
    unordered_set<int> nodes;                // 记录所有节点（避免漏掉无入度的起点）

    // 构建邻接表和入度表
    for (const auto& edge : edges)
    {
        adjList[edge.u].push_back(edge.v);
        inDegree[edge.v]++; // 目标节点入度增加
        nodes.insert(edge.u);
        nodes.insert(edge.v);
    }

    // 找到所有入度为 0 的起点
    queue<int> q;
    for (int node : nodes)
    {
        if (inDegree[node] == 0)
        {
            q.push(node);
        }
    }

    vector<int> bfsOrder;
    while (!q.empty())
    {
        int node = q.front();
        q.pop();
        bfsOrder.push_back(node);

        // 遍历相邻节点
        for (int neighbor : adjList[node])
        {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0)
            { // 入度变 0，加入队列
                q.push(neighbor);
            }
        }
    }

    return bfsOrder;
}

vector<int>
generateRamdomVector(vector<int> vec)
{
    // 拷贝原始 vector
    vector<int> shuffledVec = vec;

    // 创建一个随机数生成器
    random_device rd; // 获取随机数种子
    mt19937 g(rd());  // 使用 Mersenne Twister 生成器

    // 使用 shuffle 函数打乱数组
    shuffle(shuffledVec.begin(), shuffledVec.end(), g);
    shuffledVec = {1, 3, 2, 4, 5};
    shuffledVec.insert(shuffledVec.begin(), 0);
    cout << "shuffle" << endl;
    for (auto val : shuffledVec)
    {
        cout << val << " ";
    }
    cout << endl;
    // 返回打乱后的 vector
    return shuffledVec;
}

Matrix
getCMatrix(vector<Edge> edges, int nodeNum, int K)
{
    Matrix S(nodeNum, vector<int>(K));
    Matrix C(nodeNum, vector<int>(K));
    for (int i = 1; i <= nodeNum; i++)
    {
        for (int k = 1; k <= K; k++)
        {
            int weight = 0;
            for (const auto& edge : edges)
            {
                if (edge.u == i && edge.v == k)
                {
                    weight = edge.weight;
                    break;
                }
            }
            S[i][k] = S[i - 1][k] + S[i][k - 1] - S[i - 1][k - 1] + weight;
        }
    }
    // 打印 S 矩阵
    // cout << "Matrix S:" << endl;
    // for (int i = 0; i < nodeNum; i++)
    // {
    //     for (int k = 0; k < K; k++)
    //     {
    //         cout << S[i][k] << " ";
    //     }
    //     cout << endl;
    // }

    // for (int i = 1; i <= nodeNum; i++)
    // {
    //     for (int j = i + 1; j <= nodeNum; j++)
    //     {
    //         C[i][j] = S[i][j] - S[i][i];
    //     }
    // }

    // // 打印 C 矩阵
    // cout << "Matrix C:" << endl;
    // for (int i = 0; i < nodeNum; i++)
    // {
    //     for (int j = 0; j < K; j++)
    //     {
    //         cout << C[i][j] << " ";
    //     }
    //     cout << endl;
    // }
    return C;
}

Matrix
computeCMatrix(const vector<Edge>& edges, const vector<int>& a)
{
    int n = a.size() - 1;
    Matrix C(n + 1, vector<int>(n + 1, 0));

    for (int j = 1; j <= n; j++)
    {
        for (int i = j + 1; i <= n; i++)
        {
            for (int m = 1; m <= j; m++)
            {
                for (int k = j + 1; k <= i; k++)
                {
                    for (const auto& edge : edges)
                    {
                        if (edge.u == a[m] && edge.v == a[k])
                        {
                            C[j][i] += edge.weight;
                        }
                    }
                }
            }
        }
    }
    // 打印 C 矩阵
    // cout << "Matrix C:" << endl;
    // for (int i = 0; i <= n; i++)
    // {
    //     for (int j = 0; j <= n; j++)
    //     {
    //         cout << C[i][j] << " ";
    //     }
    //     cout << endl;
    // }

    return C;
}

pair<Matrix, cutMatrix>
computeFMatrix(const vector<vector<int>>& C, int K, const vector<int>& a)
{
    int n = C.size() - 1;
    Matrix F(n + 1, vector<int>(K + 1, 0));
    cutMatrix cut(n + 1, vector<vector<vector<int>>>(K + 1));
    for (int i = 1; i <= n; i++)
    {
        for (int k = 1; k <= K; k++)
        {
            int max = 0;
            int maxJ = 0;
            for (int j = 1; j < i; j++)
            {
                int value = F[j][k - 1] + C[j][i];
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
            vector<int> new_nodes;
            for (int m = maxJ + 1; m <= i; m++)
            {
                new_nodes.push_back(a[m]);
            }
            cut[i][k].push_back(new_nodes);
        }
    }
    // 打印 F 矩阵
    // cout << "Matrix F:" << endl;
    // for (int i = 1; i <= n; i++)
    // {
    //     for (int k = 1; k <= K; k++)
    //     {
    //         cout << F[i][k] << " ";
    //     }
    //     cout << endl;
    // }
    // 打印 cut 矩阵
    for (int i = 1; i <= n; i++)
    {
        for (int k = 1; k <= K; k++)
        {
            cout << "cut[" << i << "][" << k << "]: ";
            for (const auto& group : cut[i][k])
            {
                cout << "{ ";
                for (int node : group)
                {
                    cout << node << " ";
                }
                cout << "} ";
            }
            cout << endl;
        }
    }
    return {F, cut};
}

int
main()
{
    // 图的边集合 (u, v, weight) 有向图
    vector<Edge> edges =
        {{2, 3, 4}, {2, 4, 1}, {3, 4, 2}, {3, 5, 7}, {4, 5, 3}, {4, 6, 6}, {5, 6, 5}};

    vector<Edge> test =
        {{1, 2, 1}, {1, 4, 1}, {2, 4, 0}, {3, 1, 3}, {1, 5, 1}, {3, 5, 3}};
    vector<int> nodeSeq = bfsTraversal(test);
    Matrix outputCut;
    int nodeNum = nodeSeq.size();
    cout << "Node Num: " << nodeNum << endl;
    int K = 2;

    int maxCut = 0;
    for (int i = 0; i < 1; i++)
    {
        vector<int> randomSeq = generateRamdomVector(nodeSeq);
        auto C = computeCMatrix(test, randomSeq);
        auto [F, cut] = computeFMatrix(C, K, randomSeq);
        // Print F matrix
        cout << "Matrix F:" << endl;
        for (int i = 1; i <= nodeNum; i++)
        {
            for (int k = 1; k <= K; k++)
            {
                cout << F[i][k] << " ";
            }
            cout << endl;
        }
        if (F[nodeNum][K] > maxCut)
        {
            maxCut = F[nodeNum][K];
            outputCut = cut[nodeNum][K];
        }
    }

    cout << "Max Cut: " << maxCut << endl;
    cout << "Output Cut:" << endl;
    for (const auto& group : outputCut)
    {
        cout << "{ ";
        for (int node : group)
        {
            cout << node << " ";
        }
        cout << "}" << endl;
    }
    return 0;
}