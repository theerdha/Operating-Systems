import numpy as np
import matplotlib.pyplot as plt

print ("Enter the x axis points.")
x = input()
x_axis = np.array(x.split(' ')).astype(float)

print (" Enter the y axis points for FCFS.")
y = input()
y_axis1 = np.array(y.split(' ')).astype(float)

print (" Enter the y axis points for SJF.")
y = input()
y_axis2 = np.array(y.split(' ')).astype(float)

print (" Enter the y axis points for RR1.")
y = input()
y_axis3 = np.array(y.split(' ')).astype(float)

print (" Enter the y axis points for RR2.")
y = input()
y_axis4 = np.array(y.split(' ')).astype(float)

print (" Enter the y axis points for RR5.")
y = input()
y_axis5 = np.array(y.split(' ')).astype(float)


f, (ax1, ax2, ax3,ax4,ax5) = plt.subplots(5, sharex=True, sharey=True)

ax1.plot(x_axis, y_axis1, 'o-')
ax1.set_title('ATN time vs N')
ax1.set_ylabel('FCFS ATN time')

ax1.grid()
ax2.plot(x_axis, y_axis2, 'o-')
ax2.set_ylabel('SJF ATN time')

ax2.grid()
ax3.plot(x_axis, y_axis3, 'o-')
ax3.set_ylabel('RR1 ATN time')

ax3.grid()
ax4.plot(x_axis, y_axis4, 'o-')
ax4.set_ylabel('RR2 ATN time')

ax4.grid()
ax5.plot(x_axis, y_axis5, 'o-')
ax5.set_ylabel('RR3 ATN time')
ax5.set_xlabel('N values')
ax5.grid()

f.subplots_adjust(hspace=0.05)
plt.setp([a.get_xticklabels() for a in f.axes[:-1]], visible=False)

plt.show()
