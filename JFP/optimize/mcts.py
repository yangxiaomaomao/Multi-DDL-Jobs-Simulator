import numpy as np
# from node import Node
import copy
import time
from tools import generate_random_I
import sys
import matplotlib.pyplot as plt
import seaborn as sns
from node import Node

class MCTS:
    def __init__(self, root):
        self.root = root

    def select(self, node):
        """选择最佳子节点（基于 UCT）"""
        best_child = max(
            node.children,
            key=lambda child: child.value / (child.visits + 1e-6) +
                              np.sqrt(2 * np.log(node.visits + 1) / (child.visits + 1e-6))
        )
        return best_child

    def expand(self, node:Node):
        """扩展节点"""
        actions = node.get_legal_actions()
        #print(len(actions),"llllllllllllllll",actions)
        for action in actions:
            inc_indice, dec_indice = action
            
            #print("curr state_list",[child.state for child in node.children])
            if action not in [child.state for child in node.children]:
                child = node.take_action(action)
                node.children.append(child)
        return node.children[0]

    def simulate(self, node:Node, evaluate=False):
        """模拟从当前节点到终止状态"""
        current = node
        # while not current.is_terminal():
        if evaluate:
            return current.get_legal_actions(evaluate=evaluate)
        current.get_legal_actions(evaluate=evaluate)
        
        return current.reward

    def backpropagate(self, node, reward):
        """回溯更新节点的访问次数和价值"""
        # print("back",reward,node.state)
        while node is not None:
            node.visits += 1
            node.value += reward
            node = node.parent

    def run(self, iterations, evaluate=False):
        """运行 MCTS"""
        if evaluate:
            reward = self.simulate(self.root, evaluate=evaluate)
            return reward
        for iter_cnt in range(iterations):
            # print("=" * 30, "iteration %d" % iter_cnt, "=" * 30)
            node = self.root
            # 选择阶段
            while node.children and not node.is_terminal():
                node = self.select(node)
            # 扩展阶段
            if not node.is_terminal():
                node = self.expand(node)
            # 模拟阶段
            reward = self.simulate(node)

            # 回溯阶段
            self.backpropagate(node, reward)

    
