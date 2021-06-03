#!/usr/bin/env python3

import pandas as pd
import numpy as np
from collections import defaultdict 
# import plotly.express as px
import seaborn as sns
import matplotlib.pyplot as plt
import sys


def plot_bars(filename, title = 'no title'):
    # data = pd.read_csv(filename)
    data = open(filename).readlines()
    # s = [1] * len(data['strip'])

    y = defaultdict(list)
    for item in data:
        item = item.strip().split(',')
        try:
            if(int(item[1].strip()) < 1000000): continue
            # print(item)
            y[int(item[1].strip())].append(float(item[3].strip()))
        except:
            pass

    aaa = []
    for key,value in y.items():
        aaa.append(value)

    sns.boxplot(data=aaa)
    # sns.stripplot(data=aaa)

    # std = np.std(data['bezier(s)'])
    # mean = np.mean(data['bezier(s)'])
    # print(f'mean: {mean}, std: {std}')

    # # plt.scatter(data['strip'], data['bezier(s)'], s=s)
    # plt.show()

def plot_csv(filename, title = 'no title'):
    data = pd.read_csv(filename)
    plt.plot(range(0, len(data['bezier(s)'])), data['bezier(s)'])


def scatter_csv(filename, title = 'no title'):
    data = pd.read_csv(filename)
    # plt.scatter(range(0, len(data['triangles'])), data['bezier(s)']/data['num_points'], s = [1] * len(data['triangles']))
    plt.scatter(range(0, len(data['triangles'])), data['bezier(s)'], s = [1] * len(data['triangles']))


if __name__ == '__main__':
    for file in sys.argv[1:]: 
        scatter_csv(file)
    plt.show()