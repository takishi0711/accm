from cProfile import label
import numpy as np
import matplotlib.pyplot as plt

filename1 = '../hard_basic_5000000_port_4_rerun.txt'
filename2 = '../hard_basic_5000000_port_5_rerun.txt'
filename3 = '../hard_basic_5000000_port_6_rerun.txt'
filename4 = '../hard_basic_5000000_port_7_rerun.txt'
filename5 = '../hard_basic_5000000_port_8_rerun.txt'
filename6 = '../hard_basic_5000000_port_9_rerun.txt'
filename7 = '../hard_basic_5000000_port_10_rerun.txt'
filename8 = '../hard_basic_5000000_port_11_rerun.txt'
filename9 = '../hard_basic_5000000_port_12_rerun.txt'
filename10 = '../hard_basic_5000000_port_13_rerun.txt'
filename11 = '../hard_basic_5000000_port_14_rerun.txt'

data1 = np.loadtxt(filename1, unpack=True)
data2 = np.loadtxt(filename2, unpack=True)
data3 = np.loadtxt(filename3, unpack=True)
data4 = np.loadtxt(filename4, unpack=True)
data5 = np.loadtxt(filename5, unpack=True)
data6 = np.loadtxt(filename6, unpack=True)
data7 = np.loadtxt(filename7, unpack=True)
data8 = np.loadtxt(filename8, unpack=True)
data9 = np.loadtxt(filename9, unpack=True)
data10 = np.loadtxt(filename10, unpack=True)
data11 = np.loadtxt(filename11, unpack=True)

fig = plt.figure()

ax1 = fig.add_subplot(111)

x = [4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14]

y1 = [np.average(data1), np.average(data2), np.average(data3), np.average(data4), np.average(data5), np.average(data6), np.average(data7), np.average(data8), np.average(data9), np.average(data10), np.average(data11)]

ax1.plot(x, y1)

# ax1.legend()

ax1.set_xlabel('port')
ax1.set_ylabel('loss (%)')

outname = '../picture/port_rerun.pdf'

fig.savefig(outname)

