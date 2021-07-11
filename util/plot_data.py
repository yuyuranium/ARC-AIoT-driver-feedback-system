import matplotlib.pyplot as plt
import pandas as pd
import csv
import sys

i = int(sys.argv[1])

cc = ['k', 'g', 'b', 'r', 'm', 'y']


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
    data = pd.read_csv(str(i) + '.CSV')

    plt.clf()

    plt.subplot(311)
    plt.plot(data['ax'], label='ax')
    plt.plot(data['ay'], label='ay')
    plt.plot(data['az'], label='az')
    marker = 0
    c = data['class'][marker]
    for x in range(0, data.shape[0]):
        if (data['class'][x] != c):
            plt.axvspan(marker, x, facecolor=cc[c], alpha=0.2)
            marker = x
            c = data['class'][x]
    plt.axvspan(marker, data.shape[0], facecolor=cc[c], alpha=0.2)

    plt.legend()
    plt.grid()
    plt.title('acc')

    plt.subplot(312)
    plt.plot(data['jx'], label='jx')
    plt.plot(data['jy'], label='jy')
    plt.plot(data['jz'], label='jz')
    marker = 0
    c = data['class'][marker]
    for x in range(0, data.shape[0]):
        if (data['class'][x] != c):
            plt.axvspan(marker, x, facecolor=cc[c], alpha=0.2)
            marker = x
            c = data['class'][x]
    plt.axvspan(marker, data.shape[0], facecolor=cc[c], alpha=0.2)
    plt.legend()
    plt.grid()
    plt.title('jerk')

    plt.subplot(313)
    plt.plot(data['class'], label='class')
    plt.ylim([0, 5])
    plt.title('class')

    plt.suptitle(str(i) + '.CSV')
    plt.grid()
    plt.show()


fig, ax = plt.subplots()
fig.canvas.mpl_connect('key_press_event', press)

plot_next(0)
