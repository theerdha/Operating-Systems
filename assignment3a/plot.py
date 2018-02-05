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


f, ax1 = plt.subplots()

ax1.plot(x_axis, y_axis1, 'r^-',label='FCFS')
ax1.set_title('ATN time vs N for differnt schedulers')
#ax1.set_ylabel('FCFS ATN time')

ax1.grid()
ax1.plot(x_axis, y_axis2, 'bo-',label='SJF')
#ax2.set_ylabel('SJF ATN time')

#ax2.grid()
ax1.plot(x_axis, y_axis3, 'go-',label='RR1')
#ax3.set_ylabel('RR1 ATN time')

#ax3.grid()
ax1.plot(x_axis, y_axis4, 'y^-',label='RR2')
#ax4.set_ylabel('RR2 ATN time')

#ax4.grid()
ax1.plot(x_axis, y_axis5, 'k^-',label='RR5')
ax1.set_ylabel('ATN time')
ax1.set_xlabel('N values')
#ax5.grid()

legend = ax1.legend(loc='upper left', shadow=True, fontsize='x-large')

# Put a nicer background color on the legend.
legend.get_frame().set_facecolor('#00FFCC')


plt.show()
