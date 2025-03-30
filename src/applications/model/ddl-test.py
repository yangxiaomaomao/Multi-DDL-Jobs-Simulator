import socket
import json
import sys
import psutil
import subprocess
import os
import numpy as np

def kill_process(port):
    output = subprocess.run(['lsof', '-ti', f':{port}'], stdout=subprocess.PIPE)
    pid = output.stdout.decode('utf-8').strip()
    print(pid, type(pid),f'kill -9 {pid}',"ssssssss")
    os.system(f'kill -9 {pid}')

def numpy_serializer(obj):
    if isinstance(obj, np.int64):
        return int(obj)  # 将 numpy.int64 转换为普通的 int
    raise TypeError("Type not serializable")

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
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)

    # 绑定 IP 和端口
    server_socket.bind(('127.0.0.1', port))
    
    # 开始监听
    server_socket.listen(100)
    print("等待客户端连接...")
    
    # 接受客户端连接
    while 1:
        client_socket, client_address = server_socket.accept()
        data = client_socket.recv(10000)
        try:
            json_data = json.loads(data.decode('utf-8'))
            matrix, job_id_list, link_id_list = get_numpy_matrix(json_data)
            response = np.array([
                [1,0],
                [2,1],
                [0,2],
            ])
            response_dict = construct_response_matrix(response, job_id_list, link_id_list)
            response_json = json.dumps(response_dict, default=numpy_serializer)
            # 发送数据
            client_socket.send(response_json.encode('utf-8'))        
        except json.JSONDecodeError as e:
            print(f"JSON 解码错误: {e}")
        client_socket.close()

if __name__ == "__main__":
    port = int(sys.argv[1])
    kill_process(port)
    start_server(port)