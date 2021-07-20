import matplotlib.pyplot as plt
import pandas as pd
import csv
import sys

i = int(sys.argv[1])

cc = ['k', 'b', 'g', 'r', 'm', 'y', 'w']


def press(event):
    if (event.key == 'n'):
        plot_next(1)
    elif (event.key == 'N'):
        plot_next(-1)
    elif (event.key == 'r'):
        plot_next(0)


def plot_next(delta):
    global i
    i += delta
    data = pd.read_csv('P' + str(i) + '.CSV')

    plt.clf()

    plt.subplot(411)
    plt.plot(data['ax'], label='ax')
    plt.plot(data['p_ax'], label='p_ax')
    marker = 0
    c = data['p_class'][marker]
    for x in range(0, data.shape[0]):
        if (data['p_class'][x] != c):
            plt.axvspan(marker, x, facecolor=cc[c], alpha=0.2)
            marker = x
            c = data['p_class'][x]
    plt.axvspan(marker, data.shape[0], facecolor=cc[c], alpha=0.2)

    plt.legend()
    plt.grid()
    plt.title('x')

    plt.subplot(412)
    plt.plot(data['ay'], label='ay')
    plt.plot(data['p_ay'], label='p_ay')
    marker = 0
    c = data['p_class'][marker]
    for x in range(0, data.shape[0]):
        if (data['p_class'][x] != c):
            plt.axvspan(marker, x, facecolor=cc[c], alpha=0.2)
            marker = x
            c = data['p_class'][x]
    plt.axvspan(marker, data.shape[0], facecolor=cc[c], alpha=0.2)
    plt.legend()
    plt.grid()
    plt.title('y')

    plt.subplot(413)
    plt.plot(data['az'], label='az')
    plt.plot(data['p_az'], label='p_az')
    marker = 0
    c = data['p_class'][marker]
    for x in range(0, data.shape[0]):
        if (data['p_class'][x] != c):
            plt.axvspan(marker, x, facecolor=cc[c], alpha=0.2)
            marker = x
            c = data['p_class'][x]
    plt.axvspan(marker, data.shape[0], facecolor=cc[c], alpha=0.2)
    plt.legend()
    plt.grid()
    plt.title('z')

    plt.subplot(414)
    plt.plot(data['p_class'], label='p_class')
    plt.ylim([-1, 5])
    plt.title('p_class')

    plt.suptitle(str(i) + '.CSV')
    plt.grid()
    plt.show()


fig, ax = plt.subplots()
fig.canvas.mpl_connect('key_press_event', press)

plot_next(0)
