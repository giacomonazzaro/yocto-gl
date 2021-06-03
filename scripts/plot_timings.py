#! /usr/bin/env python3 -B

import sys
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

def draw_jointgrid(file):
	df = pd.read_csv(file)
	x, y = df["triangles"], df["num_points"]
	df['bezier(s)'] = df['bezier(s)'].map(lambda a: np.log(a)) #Negative (?)


	# Setting main plot axis
	g = sns.JointGrid(marginal_ticks=False, height=8)
	g.ax_joint.set(xscale="log", yscale="log")
	g.ax_joint.set(xlabel="Triangles", ylabel="Length")
	sns.set_theme(style="ticks")


	#Main plot
	sns.scatterplot(x=x, y=y, hue=df['bezier(s)'], palette="viridis_r", legend=False, ax=g.ax_joint)
	#sns.kdeplot(x=x, y=y, cmap="viridis", thresh=0, levels=100, ax=g.ax_joint)
	#sns.histplot(x=x, y=y, bins=30, cmap="viridis", ax=g.ax_joint)


	#Top_marginal
	sns.histplot(x=x, linewidth=0, color='g', alpha=0.3, shrink=.8, kde=True, ax=g.ax_marg_x)


	#Right_marginal
	sns.histplot(y=y, linewidth=0, color='g', alpha=0.3, shrink=.8, kde=True, ax=g.ax_marg_y)


if __name__ == "__main__":
	for file in sys.argv[1:]:
		draw_jointgrid(file)
	plt.show()

