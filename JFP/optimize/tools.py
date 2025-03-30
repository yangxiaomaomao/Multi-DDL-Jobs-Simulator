import numpy as np
import copy
import random
import matplotlib.pyplot as plt
import seaborn as sns
import os
import subprocess

arr = np.array([[5, 3, 8],
                [2, 9, 4],
                [7, 1, 6]])

def get_top_n_smallest_with_indices(arr, n):
    # 获取所有非零元素的索引和对应的值
    non_zero_elements = [(i, j, arr[i][j]) for i in range(arr.shape[0]) for j in range(arr.shape[1]) if arr[i][j] != 0]
    
    # 如果非零元素的数量少于n，调整n的值
    n = min(n, len(non_zero_elements))

    # 获取最小的n个非零元素
    non_zero_elements.sort(key=lambda x: x[2])  # 按值排序
    min_n_elements = non_zero_elements[:n]
    min_n_indices = [(i, j) for i, j, value in min_n_elements]

    # 创建新数组，并将最小的n个元素位置置为0
    modified_arr = arr.copy()
    for i, j in min_n_indices:
        modified_arr[i][j] = 0
    # 获取剩余非零元素
    remaining_non_zero_elements = [(i, j, modified_arr[i][j]) for i in range(modified_arr.shape[0]) for j in range(modified_arr.shape[1]) if modified_arr[i][j] != 0]

    # 随机选择剩余非零元素中的n//2个
    remaining_n = min(n // 2 + 1, len(remaining_non_zero_elements))
    random_selected_elements = random.sample(remaining_non_zero_elements, remaining_n)
    random_selected_indices = [(i, j) for i, j, value in random_selected_elements]

    # 返回所有选中的元素值和对应的索引
    selected_values = [value for i, j, value in min_n_elements] + [value for i, j, value in random_selected_elements]
    selected_indices = min_n_indices + random_selected_indices

    return selected_values, selected_indices

def generate_random_I(J, L, min_v, max_v):
    # 生成范围在[min_v, max_v]内的随机浮动值
    arr = np.random.uniform(min_v, max_v, size=(J, L, 2))

    # 确保每行不全是[0, 0]
    for i in range(J):
        while np.all(arr[i] == [0, 0]):
            arr[i] = np.random.uniform(min_v, max_v, size=(L, 2))

    # 使得每列至少有一半的[0, 0]
    for j in range(L):
        # 计算每列中[0, 0]的数量
        zero_count = np.sum(np.all(arr[:, j] == [0, 0], axis=1))

        # 如果[0, 0]数量小于一半，则添加[0, 0]元素
        while zero_count < J // 2:
            # 选择一个非[0, 0]的元素进行替换
            row_to_replace = np.random.choice(np.where(np.all(arr[:, j] != [0, 0], axis=1))[0])
            arr[row_to_replace, j] = [0, 0]
            zero_count = np.sum(np.all(arr[:, j] == [0, 0], axis=1))

    return arr

def plot_accu_change(global_reward_list):
    # 设置Seaborn主题
    sns.set_theme(style="whitegrid", palette="muted")

    sns.lineplot(data=global_reward_list)
    plt.ylabel("Evaluation Result")
    plt.xlabel("Node Counter")
    # plt.ylim(1.5,2)
    # 显示图形
    plt.tight_layout()
    plt.savefig("accu_change.pdf")
    
def kill_process(port):
    output = subprocess.run(['lsof', '-ti', f':{port}'], stdout=subprocess.PIPE)
    pid = output.stdout.decode('utf-8').strip()
    os.system(f'kill -9 {pid}')

def numpy_serializer(obj):
    if isinstance(obj, np.int64):
        return int(obj)  # 将 numpy.int64 转换为普通的 int
    raise TypeError("Type not serializable")