import numpy as np
from node import Node
from mcts import MCTS
from node import Node
from tree import traverse
import time
from config import global_reward_list, node_id
import matplotlib.pyplot as plt
import seaborn as sns
from tools import plot_accu_change
max_epochs = 10

def crux_plus(I:np.array):
    global max_epochs
    J, L, _ = np.shape(I)
    row_sums = np.ones((J, 1))
    for j in range(J):
        row_sums[j] = np.sum(I[j])
    
    flow_comm_ratio = np.ones((J, L))
    for j in range(J):
        for l in range(L):
            flow_comm_ratio[j][l] = I[j][l][1] / row_sums[j][0]
    # print(flow_comm_ratio)
    P = np.zeros_like(flow_comm_ratio, dtype=int)
    # 遍历每一列，对每列进行排序并赋值
    for l in range(L):
        # 对当前列按元素值排序，返回排序后的索引
        sorted_indices = np.argsort(flow_comm_ratio[:, l])[::-1]
        # 将排序后的结果映射为 0, 1, 2
        P[sorted_indices, l] = np.arange(J)
    node = Node([], I, P, max_epochs, -1, parent=None)

    node.get_legal_actions()
    # print("crux+ reward is",node.reward)
    #print(P)
    return P, node.reward

def crux(I:np.array):
    global max_epochs
    J, L, _ = np.shape(I)
    row_comm_sums = np.sum(I[:, :, 1], axis=1)
    row_sums = np.ones((J, 1))
    for j in range(J):
        row_sums[j] = np.sum(I[j])

    P = np.zeros_like(I[:,:,1], dtype=int)
    for l in range(L):
        P[np.argsort(row_comm_sums/row_sums.T[0])[::-1],l]=np.arange(J)
    
    node = Node([], I, P, max_epochs, -1, parent=None)

    node.get_legal_actions()
    print("crux reward is",node.reward)
    # print(P)
    return P, node.reward

def crux_mcts(I:np.array):
    global max_epochs
    initial_state = list()  # 根据具体问题设置初始状态
    iterations = 250
    max_child = 2
    
    J, L, _ = np.shape(I)
    # init_P = np.tile(np.arange(J), (L, 1)).T
    init_P, _ = crux_plus(I)

    root = Node(initial_state, I, init_P, max_epochs, max_child, parent=None)

    mcts = MCTS(root)
    mcts.run(iterations)
    
    min_reward_node, max_reward_node = traverse(root)
    
    # plot_accu_change(global_reward_list)
    
    for job_index,job in enumerate(I):
        for link_index,link in enumerate(job):
            if np.all(I[job_index][link_index] == 0):
                min_reward_node.P[job_index][link_index] = 0
                
    return min_reward_node.P, min_reward_node.reward
    
    
# P1 = np.array([
#     [4, 4, 4],
#     [3, 3, 3],
#     [2, 2, 2],
#     [1, 1, 1],
#     [0, 0, 0],
# ])
# P_test = np.array(
# [
#     [1,0],
#     [2,1],
#     [0,2]
# ]
# )
