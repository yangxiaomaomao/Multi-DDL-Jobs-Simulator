
import numpy as np
from tools import generate_random_I

import matplotlib.pyplot as plt
import seaborn as sns
from config import global_reward_list, node_id
import baseline

I1 = np.array([
        [[1, 3], [0, 1], [0, 0]],
        [[0, 0], [1, 4], [3, 1]],
        [[0, 1], [1, 1], [0, 1]],
        [[2, 2], [7, 8], [0, 0]],
        [[4, 3], [4, 2], [1, 6]], 
    ],dtype=np.float64)
I2 = generate_random_I(3, 2, 0, 4)
I3 = np.array(
    [
        [[20,40], [0,1], [20,40]],
        [[0,0], [40,60],[0,0]],
        # [[20,0],[10,40],[0,10]],
    ],dtype=np.float64
)
I4 = np.array([
    [[1,3],[0,0]],
    [[1,8],[0,1]],
    [[0,0],[1,3]],
])

crux = 0
crux_plus = 0
crux_mcts = 1
print_P = 1
I = I4
basic_res_list = []
plus_res_list = []
for i in np.arange(0,1,1):
    # I[1][1][1] = i
    if crux:
        crux_P, crux_reward = baseline.crux(I)
        if print_P:
            print(crux_P)
    if crux_plus:
        crux_plus_P, crux_plus_reward = baseline.crux_plus(I)
        if print_P:
            print(crux_plus_P)
    if crux_mcts:
        crux_mcts_P, crux_mcts_reward = baseline.crux_mcts(I)
        if print_P:
            print(crux_mcts_P)
    # plus_res_list.append(crux_plus_reward / crux_mcts_reward)
    # basic_res_list.append(crux_reward / crux_mcts_reward)

sns.lineplot(basic_res_list, label="basic")
# plt.savefig("basic_res.pdf")

sns.lineplot(plus_res_list, label="plus")
plt.ylim(1.0,2.0)
plt.savefig("plus_res.pdf")