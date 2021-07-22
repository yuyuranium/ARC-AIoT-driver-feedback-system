import matplotlib.pyplot as plt
import pandas as pd
import csv
import sys

i = int(sys.argv[1])

cc = ['w', 'k', 'g', 'r', 'm', 'y', 'b', 'g', 'w']


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
    plt.plot(data['dx'], label='dx')
    marker = 0
    c = data['state'][marker]
    for x in range(0, data.shape[0]):
        if (data['state'][x] != c):
            if c > 7 and c < 15:
                plt.axvspan(marker, x, facecolor=cc[c - 7], alpha=0.2)
            elif c > 0 and c < 8:
                plt.axvspan(marker, x, facecolor=cc[c], alpha=0.1)
            marker = x
            c = data['state'][x]
    if c > 7 and c < 15:
        plt.axvspan(marker, x, facecolor=cc[c - 7], alpha=0.2)
    elif c > 0 and c < 8:
        plt.axvspan(marker, x, facecolor=cc[c], alpha=0.1)
    plt.legend()
    plt.grid()
    plt.title('x')

    plt.subplot(412)
    plt.plot(data['ay'], label='ay')
    plt.plot(data['p_ay'], label='p_ay')
    plt.plot(data['dy'], label='dy')
    marker = 0
    c = data['state'][marker]
    for x in range(0, data.shape[0]):
        if (data['state'][x] != c):
            if c > 7 and c < 15:
                plt.axvspan(marker, x, facecolor=cc[c - 7], alpha=0.2)
            elif c > 0 and c < 8:
                plt.axvspan(marker, x, facecolor=cc[c], alpha=0.1)
            marker = x
            c = data['state'][x]
    if c > 7 and c < 15:
        plt.axvspan(marker, x, facecolor=cc[c - 7], alpha=0.2)
    elif c > 0 and c < 8:
        plt.axvspan(marker, x, facecolor=cc[c], alpha=0.1)
    plt.legend()
    plt.grid()
    plt.title('y')

    plt.subplot(413)
    plt.plot(data['az'], label='az')
    plt.plot(data['p_az'], label='p_az')
    plt.plot(data['dz'], label='dz')
    marker = 0
    c = data['state'][marker]
    for x in range(0, data.shape[0]):
        if (data['state'][x] != c):
            if c > 7 and c < 15:
                plt.axvspan(marker, x, facecolor=cc[c - 7], alpha=0.2)
            elif c > 0 and c < 8:
                plt.axvspan(marker, x, facecolor=cc[c], alpha=0.1)
            marker = x
            c = data['state'][x]
    if c > 7 and c < 15:
        plt.axvspan(marker, x, facecolor=cc[c - 7], alpha=0.2)
    elif c > 0 and c < 8:
        plt.axvspan(marker, x, facecolor=cc[c], alpha=0.1)
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
