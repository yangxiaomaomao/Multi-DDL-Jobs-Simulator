import copy
from tools import get_top_n_smallest_with_indices
import numpy as np
from config import global_reward_list, node_id

class Node:
    def __init__(self, state, I, P, epochs, max_child, parent=None):
        self.state = state       # 当前节点的状态
        self.parent = parent     # 父节点
        self.children = []       # 子节点
        self.visits = 0          # 访问次数
        self.value = 0           # 节点的累积价值
        self.max_child = max_child
        
        self.legal_actions = None
        
        self.reward = -1

        self.I = I
        self.P = P
        self.epochs = epochs
        global node_id
        self.id = node_id
        node_id += 1

    def get_legal_actions(self):
        # has been computed
        if self.reward != -1:
            return self.legal_actions
        I = copy.deepcopy(self.I)
        P = copy.deepcopy(self.P)
        
        J, L, _ = np.shape(I)
        Y = np.ones((J, 1))
        Y_list = []
        for j in range(J):
            Y[j][0] = np.sum(I[j])
        
        initial_I = copy.deepcopy(I)
        initial_Y = copy.deepcopy(Y)
        
        # add one row the record the lowest prio of job in each link,
        # for when we decrease a job flow of prio 1, it will be influenced by the job flow of prio 0
        link_scale_arr = np.ones((J + 1, L))
        increase_prio_arr = np.ones((J, L))
        decrease_prio_arr = np.ones((J, L))
        reward_matrix_list = []
        # add all the epoch's reward_matrix
        final_reward_matrix = np.zeros((J, L))
        for epoch in range(self.epochs):
            # use the I from the last epoch
            newest_I = copy.deepcopy(I)
            # pre-compute the scale factor of each link
            for l in range(L):
                curr_prio = J
                while curr_prio >= 0:
                    if curr_prio == J:
                        link_scale_arr[curr_prio][l] = 1
                    else:
                        indices = np.where(P[:, l] == curr_prio)[0][0]
                        #print("indices",indices)
                        if Y[indices,0] == newest_I[indices][l][1]:
                            scale = 1
                        else:
                            scale = (Y[indices,0] / (Y[indices,0] - newest_I[indices][l][1]))
                        link_scale_arr[curr_prio][l] = link_scale_arr[curr_prio+1][l] * scale

                    curr_prio -= 1
            
            I = copy.deepcopy(initial_I)
            # First, we calculate the newest I, attention this step must be done after the above two steps
            # for it will change the value of I, which will be used in the former two steps, demanding the 
            # origin values
            for l in range(L):
                for j in range(J):
                    prio = P[j][l]
                    comm = I[j][l][1]
                    if comm == 0:
                        continue
                    I[j][l][1] = comm * link_scale_arr[prio+1][l]
            
            
            for j in range(J):
                Y[j][0] = np.sum(I[j])
            
            # if epoch > 0:
            #     print(np.sum(Y/initial_Y) / J, Y_list[-1],np.sum(Y/initial_Y) / J / Y_list[-1])
            if epoch > 0 and np.sum(Y/initial_Y) / J / Y_list[-1] > 0.97:
                break
            Y_list.append(np.sum(Y/initial_Y) / J)
            
            # we conly consider the first epoch
            if epoch != 0:
                continue
            # increase the prio, we only pay attention to the diff
            for l in range(L):
                for j in range(J):
                    prio = P[j][l]
                    if prio == J - 1:
                        increase_prio_arr[j][l] = 0
                    else:
                        increase_prio_arr[j][l] = I[j][l][1] / (link_scale_arr[prio+1][l] / link_scale_arr[prio+2][l]) - I[j][l][1]
            increase_prio_arr = increase_prio_arr / initial_Y
            # decrease the prio, we only pay attention to the diff
            for l in range(L):
                for j in range(J):
                    prio = P[j][l]
                    if prio == 0:
                        decrease_prio_arr[j][l] = 0
                    else:
                        decrease_prio_arr[j][l] =  I[j][l][1] * (link_scale_arr[prio-1][l] / link_scale_arr[prio][l]) - I[j][l][1]
            decrease_prio_arr = decrease_prio_arr / initial_Y
                
            
            reward_matrix = np.zeros((J, L))
            for l in range(L):
                for j in range(J):
                    prio = P[j][l]
                    if prio == J - 1:
                        reward_matrix[j][l] = 0
                        continue
                    peer_prio_indice = np.where(P[:, l] == prio + 1)[0][0]
                    increase_reward  = increase_prio_arr[j][l]
                    decrease_penalty = decrease_prio_arr[peer_prio_indice][l]
                    reward_matrix[j][l] = increase_reward + decrease_penalty
            # print("epoch",epoch,"reward_matrix",reward_matrix)
            reward_matrix_list.append(reward_matrix)
        
        for reward_matrix in reward_matrix_list[0:1]:
            final_reward_matrix += reward_matrix
        # print("final_reward_matrix",final_reward_matrix)
        n_smallest_values_list, n_indices_list = get_top_n_smallest_with_indices(final_reward_matrix, self.max_child)
        # print(final_reward_matrix)
        prio_to_inc_indices_list = []
        prio_to_dec_indices_list = []
        # print(n_indices_list,"pp",n_smallest_values_list)
        # filter the value
        for indices in n_indices_list[0:self.max_child]:
            # if n_smallest_values_list[i] >= 0:
            #     continue
            # print(i)
            if P[indices[0]][indices[1]] == J - 1:
                continue
            prio_to_dec_value = P[indices[0]][indices[1]] + 1
            prio_to_dec_indices = (np.where(P[:, indices[1]] == prio_to_dec_value)[0][0], indices[1])
            prio_to_inc_indices_list.append(indices)
            prio_to_dec_indices_list.append(prio_to_dec_indices)
            assert len(prio_to_inc_indices_list) == len(prio_to_dec_indices_list)
            
        res = np.sum(Y/initial_Y) / J
        
        global global_reward_list
        global_reward_list.append(res)
        self.reward = res
        self.legal_actions = list(zip(prio_to_inc_indices_list, prio_to_dec_indices_list))
        return self.legal_actions

    def take_action(self, action):
        """执行动作并返回新状态"""
        # 根据具体问题实现
        # print("action",action)
        inc_indices, dec_indices = action
        P = copy.deepcopy(self.P)
        P[inc_indices[0]][inc_indices[1]] += 1
        P[dec_indices[0]][dec_indices[1]] -= 1
        new_state = action  # 代表从上一节点是如何到本节点的
        
        # print("new_state",new_state)
        # print("P",P)
        # print("I",self.I)
        
        return Node(new_state, self.I, P, self.epochs, self.max_child, parent=self)

    def is_terminal(self):
        """判断当前状态是否为终止状态"""
        # 根据具体问题实现
        return len(self.get_legal_actions()) == 0

    def get_reward(self):
        """返回当前状态的奖励值"""
        # 根据具体问题实现
        return self.value
