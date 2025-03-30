#include <iostream>
#include <vector>
#include <sstream>
#include <cstdio>
#include <cstdlib>

std::string VectorToJson(const std::vector<int>& vec) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i != vec.size() - 1) oss << ",";
    }
    oss << "]";
    return oss.str();
}

int CallPythonFunction(const std::vector<int>& vec) {
    std::string json_data = VectorToJson(vec);
    
    // 使用 python3 来确保使用正确的 Python 版本
    std::string command = "python ddl-test.py"; 
    
    // 打开一个管道来同时读取和写入
    FILE* pipe = popen(command.c_str(), "w+");
    if (!pipe) {
        std::cerr << "Failed to run Python script" << std::endl;
        return -1;
    }

    // 发送数据到 Python
    fprintf(pipe, "%s\n", json_data.c_str());
    fflush(pipe);  // 确保数据立即写入

    // 读取 Python 返回的数据
    int result = -1;
    fscanf(pipe, "%d", &result);  // 读取 Python 输出
    fclose(pipe);

    return result;
}

int main() {
    std::vector<int> myVector = {1, 2, 3, 4, 5};
    int result = CallPythonFunction(myVector);
    
    std::cout << "Python 计算结果: " << result << std::endl;
    return 0;
}
