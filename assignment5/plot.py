import matplotlib.pyplot as plt
import csv

x = []
y = []
k = 1

with open('example.txt','r') as csvfile:
	plots = csv.reader(csvfile, delimiter=' ')
	for row in plots:
		y.append(int(row[1]))
		x.append(k)
		k = k + 1

plt.plot(x,y, label='plot')
plt.xlabel('x')
plt.ylabel('y')
plt.title('plot of the data')
plt.show()
