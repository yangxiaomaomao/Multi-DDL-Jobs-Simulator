#ifndef DDL_TOOLS_H
#define DDL_TOOLS_H
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <unordered_set>
#include <variant>
#include <vector>
using namespace std;
using FlowFeatureMap =
    std::map<uint32_t,
             std::map<std::string, std::variant<float, uint32_t, bool, std::vector<uint32_t>>>>;

inline std::vector<uint32_t>
parseVector(const std::string& str)
{
    std::vector<uint32_t> vec;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ':'))
    {
        vec.push_back(std::stoi(item));
    }
    return vec;
}

inline FlowFeatureMap
loadFlowFeaturesFromCSV(const std::string& filename)
{
    FlowFeatureMap m_flowFeatures;
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return m_flowFeatures;
    }

    std::string line;
    std::getline(file, line); // 读取并跳过表头

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string token;
        uint32_t flowId;

        std::map<std::string, std::variant<float, uint32_t, bool, std::vector<uint32_t>>>
            featureMap;

        // 读取 flowId
        std::getline(ss, token, ',');
        featureMap["job_id"] = (uint32_t)std::stoi(token);
        std::getline(ss, token, ',');
        featureMap["arrive_time"] = std::stof(token);
        std::getline(ss, token, ',');
        featureMap["iter_num"] = (uint32_t)std::stoi(token);
        std::getline(ss, token, ',');
        featureMap["worker_num"] = (uint32_t)std::stoi(token);

        std::getline(ss, token, ',');
        flowId = std::stoi(token);

        // 解析每个字段
        std::getline(ss, token, ',');
        featureMap["comp_time"] = (uint32_t)std::stoi(token);
        std::getline(ss, token, ',');
        featureMap["comm_size"] = (uint32_t)((float)std::stof(token) * 1000000);
        std::getline(ss, token, ',');
        featureMap["first_flow"] = (token == "true");
        std::getline(ss, token, ',');
        featureMap["last_flow"] = (token == "true");

        // 处理 vector<int>
        std::getline(ss, token, ',');
        featureMap["upstream"] = parseVector(token.substr(1, token.size() - 2)); // 去掉引号
        std::getline(ss, token, ',');
        featureMap["downstream"] = parseVector(token.substr(1, token.size() - 2)); // 去掉引号
        std::getline(ss, token, ',');

        m_flowFeatures[flowId] = featureMap;
    }

    file.close();
    return m_flowFeatures;
}

inline std::vector<uint32_t>
removeDuplicates(const std::vector<uint32_t>& input)
{
    std::unordered_set<uint32_t> seen;
    std::vector<uint32_t> result;
    for (uint32_t num : input)
    {
        if (seen.insert(num).second)
        { // 插入成功说明是新元素
            result.push_back(num);
        }
    }
    return result;
}

inline std::vector<uint32_t>
duplicateMiddleElements(const std::vector<uint32_t>& input)
{
    if (input.size() <= 2)
        return input; // 如果只有 0 或 1 或 2 个元素，直接返回

    std::vector<uint32_t> result;
    result.push_back(input.front()); // 先放入第一个元素

    // 复制中间部分
    for (size_t i = 1; i < input.size() - 1; ++i)
    {
        result.push_back(input[i]);
        result.push_back(input[i]); // 复制一份
    }

    result.push_back(input.back()); // 最后一个元素
    

    // to form a circle
    result.push_back(input.back()); // 最后一个元素
    result.push_back(input.front()); // 第一个元素


    return result;
}

inline bool
isAllTrue(std::map<uint32_t, bool>& m)
{
    for (auto it = m.begin(); it != m.end(); ++it)
    {
        if (!it->second)
        {
            return false;
        }
    }
    return true;
}


// 使用系统命令检查端口是否被占用，并获取进程 ID
// inline int getProcessIdByPort(int port) {
//     std::string command = "netstat -ano | findstr :" + std::to_string(port);  // 获取占用端口的 PID
//     std::array<char, 128> buffer;
//     std::string result;
//     std::shared_ptr<FILE> pipe(popen(command.c_str(), "r"), [](FILE* f) { if (f) pclose(f); });

//     if (!pipe) {
//         throw std::runtime_error("popen() failed!");
//     }

//     while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
//         result += buffer.data();
//     }

//     if (result.empty()) {
//         std::cerr << "No process found on port " << port << std::endl;
//         return -1; // 端口没有被占用
//     } else {
//         // 从输出中解析 PID
//         std::istringstream iss(result);
//         std::string temp;
//         for (int i = 0; i < 4; ++i) {
//             iss >> temp;  // 忽略前面的字段
//         }
//         int pid = std::stoi(temp);  // 获取 PID
//         return pid;
//     }
// }

// // 使用系统命令杀死进程
// inline void killProcessById(int pid) {
//     std::string command = "taskkill /PID " + std::to_string(pid) + " /F";  // 强制杀死进程
//     int result = system(command.c_str());
//     if (result == 0) {
//         std::cout << "Process " << pid << " killed successfully." << std::endl;
//     } else {
//         std::cerr << "Failed to kill process " << pid << std::endl;
//     }
// }

inline std::string VectorToJson(std::vector<int>& vec) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i != vec.size() - 1) oss << ",";
    }
    oss << "]";
    return oss.str();
}

inline void printColoredText(const std::string& text, const std::string& color) {
    int colorCode = 31;
    if (color == "red") {
        colorCode = 31;
    } else if (color == "green") {
        colorCode = 32;
    } else if (color == "yellow") {
        colorCode = 33;
    } else if (color == "blue") {
        colorCode = 34;
    } else if (color == "magenta") {
        colorCode = 35;
    } else if (color == "cyan") {
        colorCode = 36;
    } else if (color == "white") {
        colorCode = 37;
    }
    std::cout << "\033[1;" << colorCode << "m" << text << "\033[0m" << std::endl;
}

#endif // DDL_TOOLS_H