# this file is different from main.py
# for it is used by the ns3 simulator
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from config import global_reward_list, node_id
from tools import kill_process, numpy_serializer
import baseline
import socket
import json
import sys
import psutil
import subprocess
import os
from pprint import pprint
def print_color(text, color):
    color_code = 32
    if color == "red":
        color_code = 31
    elif color == "green":
        color_code = 32
    elif color == "yellow":
        color_code = 33
    elif color == "blue":
        color_code = 34
    print(f"\033[{color_code}m{text}\033[0m")

def get_numpy_matrix(data):
    ret_np = []
    job_id_list = []
    link_id_list = []
    for job_id, links in data.items():
        job_links = []
        job_id_list.append(job_id)
        link_id_list = links.keys()
        for link, info in links.items():
            job_links.append([info["comp_time"], info["comm_size"]])
        ret_np.append(job_links)
    
    return np.array(ret_np, dtype=np.float64), job_id_list, link_id_list

def construct_response_matrix(prio_matrix, job_id_list, link_id_list):
    response_dict = {}
    
    for i, job_id in enumerate(job_id_list):
        response_dict[job_id] = {}
        for j, link_id in enumerate(link_id_list):
            response_dict[job_id][link_id] = prio_matrix[i][j]
    return response_dict
            
def start_server(port):
    # 创建一个 Socket 服务器
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)

    # 绑定 IP 和端口
    server_socket.bind(('127.0.0.1', port))
    
    # 开始监听
    server_socket.listen(100)
    print_color("*" * 12 + "JFP Solver" + "*" * 12, "blue")
    
    print_color("[Solver] Solver端口号为%d" % port, "green")
    print_color("[Solver] Solver程序已经启动，等待客户端连接...", "green")
    
    # 接受客户端连接
    while 1:
        client_socket, client_address = server_socket.accept()
        data = client_socket.recv(10000)
        try:
            json_data = json.loads(data.decode('utf-8'))
            matrix, job_id_list, link_id_list = get_numpy_matrix(json_data)
            print_color("[Solver] 接收matrix", "green")
            # response = np.array([
            #     [1,0],
            #     [2,1],
            #     [0,2],
            # ])
            crux_mcts_P, crux_mcts_reward = baseline.crux_mcts(matrix)
            print_color("[Solver] 返回matrix", "green")
            response_dict = construct_response_matrix(crux_mcts_P, job_id_list, link_id_list)
            response_json = json.dumps(response_dict, default=numpy_serializer)
            # 发送数据
            client_socket.send(response_json.encode('utf-8'))        
        except json.JSONDecodeError as e:
            print(f"JSON 解码错误: {e}")
        client_socket.close()

if __name__ == "__main__":
    port = int(sys.argv[1])
    # start_server(port)
    
    test = 1
    if test:
        for i in range(1000):
            if i % 50 == 0:
                print_color("test %d" % i, "yellow")
            x1 = np.random.randint(2, 6)
            x2 = np.random.randint(10, 20)
            
            x3 = np.random.randint(2, 6)
            x4 = np.random.randint(10, 20)
            
            x5 = np.random.randint(2, 6)
            x6 = np.random.randint(10, 20)
            
            x7 = np.random.randint(2, 6)
            x8 = np.random.randint(10, 20)
            
            x9 = np.random.randint(10, 20)
            x10 = np.random.randint(100, 200)
            I = np.array([
                [[x1,x2],[0,1], [x1, x2],[0, 3]],
                [[0,0],[0,0], [x7, x8],[0, 1]],
                [[0,0],[0,0], [x5, x6]  ,[0, 0]],
                # [[0,  0],[0,0], [0, 0]  ,[20, 70]],
            ])
            
            # print("JFP Solver")
            crux_mcts_P, crux_mcts_reward, mcts_jobs_res = baseline.crux_mcts(I)
            # print("crux+ Solver")
            crux_plus_P, crux_plus_reward, plus_jobs_res = baseline.crux_plus(I)
            
            plus_jobs_res_np = np.array(plus_jobs_res)
            mcts_jobs_res_np = np.array(mcts_jobs_res)
            
            improve_crux_plus = (crux_plus_reward - crux_mcts_reward) / crux_mcts_reward

            max_improve = (plus_jobs_res_np.max() - mcts_jobs_res_np.max()) / plus_jobs_res_np.max()
            # print("crux+ reward is", improve_crux)
            # print("crux reward is", crux_reward)
            if max_improve > 0.15:
                print("*" * 30)
                print("max_improve:", max_improve)
                print("improve_crux_plus:", improve_crux_plus)
                # print("plus_jobs_res", plus_jobs_res)
                # print("mcts_jobs_res", mcts_jobs_res)
                # print("enhance_list",enhance_list)
                # print("crux mcts reward is not equal to crux+ reward")
                # print("crux mcts reward is", crux_mcts_reward)
                # print("crux+ reward is", crux_plus_reward)
                # print("crux+ jobs_res", plus_jobs_res)
                # print("crux mcts jobs_res", mcts_jobs_res)
                # pprint(I)
                # print("crux+ P", crux_plus_P)
                # print("mcts P", crux_mcts_P)