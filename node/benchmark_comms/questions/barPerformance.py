from collections import defaultdict
import sys
import pandas as pd 
import numpy as np
import os 
import matplotlib.pyplot as plt 
import seaborn as sb
import warnings
from pathlib import Path
warnings.simplefilter("ignore")
def main():
    global f
    home = str(Path.home())
    os.chdir(home+"/Desktop/diegog-vol3/node/benchmark_comms/")
    f = open("questions/barPerformance.txt","w+")
    data = pd.read_csv("CSVs/raw_results.csv")
    data = data[["ACTION","DEVICE","CHUNK SIZE","BARS", "TIME","TP_FRONTEND","TP_ELECTRON","TP_NANOMSG" ]]
    check_scale(data)
    data_play = data[data.ACTION == 'play']

    plot_play(data_play,"FRONTEND")
    plot_play(data_play,"ELECTRON")
    plot_play(data_play,"NANOMSG")
    diff_play(data_play,"FRONTEND")
    diff_play(data_play,"ELECTRON")
    diff_play(data_play,"NANOMSG") 
    data_record = data[data.ACTION == 'record']
    for device in data_record["DEVICE"].unique():
        data_record_device = data_record[data_record.DEVICE == device]
        plot_record(data_record_device,"FRONTEND",device)
        plot_record(data_record_device,"ELECTRON",device)
        plot_record(data_record_device,"NANOMSG",device)
        diff_record(data_record_device,"FRONTEND")
        diff_record(data_record_device,"ELECTRON")
        diff_record(data_record_device,"NANOMSG")
    f.close()

def check_scale(data):
    global minimum,maximum
    minimum = sys.float_info.max
    maximum = 0
    for phase in ["FRONTEND", "ELECTRON", "NANOMSG"]:
        minimum = data["TP_"+phase].min() if data["TP_"+phase].min() < minimum else minimum
        maximum = data["TP_"+phase].max() if data["TP_"+phase].max() > maximum else maximum
def plot_play(data,throughput):
    sb.set(style="whitegrid")
    g = sb.catplot(ci = "sd",x = "CHUNK SIZE", y = "TP_"+throughput+"",hue = "BARS",col="TIME",kind="bar",data=data)
    g._legend.set_title("Representación")
    new_labels = ['No', 'Sí']
    for t, l in zip(g._legend.texts, new_labels): 
        t.set_text(l)
    plt.ylim(minimum,maximum)
    if throughput == "FRONTEND":
        aux = "Frontend"
    elif throughput == "ELECTRON":
        aux = "Nodo de comunicaciones"
    elif throughput == "NANOMSG":
        aux = "Nodo de procesamiento"
    g.set_titles(aux+" {col_name} s Reproducción")
    g.set(ylabel='Rendimiento (Kbps)')
    g.set(xlabel='Muestras/Paquete')

    plt.savefig('questions/images/bar_performance_'+throughput+'_play.png')

def plot_record(data,throughput,device):
    sb.set(style="whitegrid")
    g = sb.catplot(ci = "sd",x = "CHUNK SIZE", y = "TP_"+throughput,hue = "BARS",col="TIME",kind="bar",data=data)
    plt.ylim(minimum,maximum)
    g._legend.set_title("Representación")
    new_labels = ['No', 'Sí']
    for t, l in zip(g._legend.texts, new_labels): 
        t.set_text(l)
    if throughput == "FRONTEND":
        aux = "Frontend"
    elif throughput == "ELECTRON":
        aux = "Nodo de comunicaciones"
    elif throughput == "NANOMSG":
        aux = "Nodo de procesamiento"
    g.set_titles(aux+" {col_name} s dispositivo " + str(device))
    g.set(ylabel='Rendimiento (Kbps)')
    g.set(xlabel='Muestras/Paquete')

    plt.savefig('questions/images/bar_performance_'+throughput+'_record_'+device+'.png')


def diff_play(data,throughput):
    data_play = data.groupby(["TIME","BARS","CHUNK SIZE"]).mean()
    play = defaultdict(lambda:list())
    for index,row in data_play.iterrows():
        value = row["TP_" + throughput]
        play[index[0]].append((index[2],index[1],value))
        play[index[0]].sort(key = lambda tup: tup[0])

    for time in play:
        f.write("\nAudio de " + str(time) + " segundos Throughput " + throughput + "\n")
        sizes = set()
        index = 0
        for size, bars, value in play[time]:
            if size in sizes: index += 1
            else:
                index += 1
                bar = play[time][index]
                no_bar = (size,bars,value) 
                diff = round(no_bar[2] - bar[2],3) 
                f.write(str(size) + ": el wavefront añade " + str(diff) + " kBps" + "\n")
                sizes.add(size)
def diff_record(data,throughput):
    data_record = data.groupby(["TIME","BARS","CHUNK SIZE","DEVICE"]).mean()
    record = defaultdict(lambda:list())
    for index,row in data_record.iterrows():
        value = row["TP_" + throughput]
        record[(index[0],index[3])].append((index[2],index[1],value))
        record[(index[0],index[3])].sort(key = lambda tup: tup[0])
    for time,device in record:
        f.write("\nAudio de " + str(time) + " segundos microfono " + str(device) + "\n")       
        sizes = set()
        index = 0
        for size, bars, value in record[(time,device)]:
            if size in sizes: 
                index += 1
            else:
                index += 1
                bar = record[(time,device)][index]
                no_bar = (size,bars,value) 
                diff = round(no_bar[2] - bar[2],3) 
                f.write(str(size) + ": el wavefront añade " + str(diff) + " kBps" + "\n")
                sizes.add(size)

if __name__ == "__main__":
    main()