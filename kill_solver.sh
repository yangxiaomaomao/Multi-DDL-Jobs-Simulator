ps -ef | grep '[p]ython3 /home/yangxiaomao/Crux+/optimize/run.py' | awk '{print $2}' | xargs kill -9