from cProfile import label
import numpy as np
import matplotlib.pyplot as plt

dataset = 'LiveJournal'
walker_num_per_vertex = 10

alpha = 0.2

f_AR_list = []
f_AR_list.append('../data/' + 'AR_' + '2000' + '_' + 'alpha' + '_' + str(alpha) + '.txt')
f_AR_list.append('../data/' + 'AR_' + '3000' + '_' + 'alpha' + '_' + str(alpha) + '.txt')
f_AR_list.append('../data/' + 'AR_' + '4000' + '_' + 'alpha' + '_' + str(alpha) + '.txt')
f_AR_list.append('../data/' + 'AR_' + '5000' + '_' + 'alpha' + '_' + str(alpha) + '.txt')
f_AR_list.append('../data/' + 'AR_' + '6000' + '_' + 'alpha' + '_' + str(alpha) + '.txt')

y_normal_AR = np.average(np.loadtxt('../data/' + 'AR_' + '0' + '_' + 'alpha' + '_' + str(alpha) + '.txt', unpack=True))
y_Kn = np.average(np.loadtxt('../data/' + 'Kn_' + 'alpha' + '_' + str(alpha) + '.txt', unpack=True))

data_AR_list = []
for i in range(5):
    data_AR_list.append(np.loadtxt(f_AR_list[i], unpack=True))
  
y_AR_list = []
for i in range(5):
    y_AR_list.append(np.average(data_AR_list[i]))


fig = plt.figure()
ax1 = fig.add_subplot(111)

x = np.array([2000, 3000, 4000, 5000, 6000])

xmin, xmax = 0, 8000

print(y_AR_list)
# ax1.bar(x, y_AR_list)

x2 = np.array([1400])
ax1.bar(x2, y_normal_AR, width=300, align="center", label='MyMethod (normal)')

ax1.bar(x, y_AR_list, width=300, align="center", label='MyMethod (reuse edges)')

# x2 = np.array([1400])
# ax1.bar(x2, y_normal_AR, width=300, align="center", label='MyMethod (normal)')

ax1.hlines(y_Kn, xmin, xmax, linestyles='dotted', colors='#2ca02c', label='KnightKing')
# ax1.hlines(y_normal_AR, xmin, xmax, linestyles='dotted', colors='#ff7f0e', label='MyMethod (normal)')


ax1.set_xlim(xmin, xmax)
ax1.set_ylim(0, 150)

ax1.set_xlabel('original edges + reuse edges (x10000)')
ax1.set_ylabel('execution_time (sec)')

ax1.legend()

outname = '../picture/AR_cache_alpha_0.2.pdf'

fig.savefig(outname)