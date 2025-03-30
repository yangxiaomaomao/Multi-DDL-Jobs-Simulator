from node import Node
# 也可以不搜索树，直接将每次的搜索记录下来，记录最优的
def traverse(node:Node):
    # 初始化最小 reward 和最小 reward 的节点
    min_reward_node = node
    max_reward_node = node
    min_reward = node.reward
    max_child = node.reward
    # print("id", node.id, "child num", len(node.children), [child.id for child in node.children])
    # print("reward", node.reward)
    # print("value", node.value)
    # print("P")
    # print(node.P)
    # 遍历每个子节点，递归调用 traverse
    for child in node.children:
        
        child_min_reward_node, child_max_reward_node = traverse(child)  # 递归调用
        # child_max_reward_node = traverse(child)
        if child_min_reward_node.visits and child_min_reward_node.reward <= min_reward:
            min_reward_node = child_min_reward_node
            min_reward = child_min_reward_node.reward
        if child_max_reward_node.visits and child_max_reward_node.reward >= max_child:
            max_reward_node = child_max_reward_node
            max_child = child_max_reward_node.reward

    return min_reward_node, max_reward_node  # 返回 reward 最小的节点