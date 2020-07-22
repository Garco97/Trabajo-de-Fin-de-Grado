import pandas as pd
import numpy as np
import os
import sys
import matplotlib.pyplot as plt
import seaborn as sb
from collections import defaultdict
from pathlib import Path

import warnings
warnings.simplefilter("ignore")
def main():
    global f
    home = str(Path.home())
    os.chdir(home+"/Desktop/diegog-vol3/node/benchmark_comms/")
    f = open("questions/bestProtocol.txt","w")
    data = pd.read_csv("CSVs/raw_results.csv")
    data = data[["ACTION","DEVICE","CHUNK SIZE","TRANSPORT PROTOCOL","TP_NANOMSG", "TIME"]]
    check_scale(data)
    data = data.sort_values(by=["TRANSPORT PROTOCOL","CHUNK SIZE","TIME"])

    data_play=data[data.ACTION == 'play']
    sb.set(style="whitegrid")
    g = sb.catplot(ci="sd",x="CHUNK SIZE", y="TP_NANOMSG",hue ="TRANSPORT PROTOCOL",col="TIME",kind="bar",data=data_play)
    plt.ylim(minimum,maximum)
    g.set(ylabel='Rendimiento (Kbps)')
    g.set(xlabel='Muestras/Paquete')
    g._legend.set_title("Protocolo de transporte")
    g.set_titles("{col_name} segundos reproducción")
    plt.savefig('questions/images/best_protocol_play.png')
    diff_play(data_play)
    data_record = data[data.ACTION == 'record']
    for device in data_record["DEVICE"].unique():
        sb.set(style="whitegrid")
        data_record_device = data_record[data_record.DEVICE == device]
        g = sb.catplot(ci="sd",x="CHUNK SIZE", y="TP_NANOMSG",hue ="TRANSPORT PROTOCOL",col="TIME",kind="bar",data=data_record_device)
        g.set(ylabel='Rendimiento (Kbps)')
        g.set(xlabel='Muestras/Paquete')
        g._legend.set_title("Protocolo de transporte")
        plt.ylim(minimum,maximum)
        plt.title("¿Mayor chunk, mejor rendimiento? FRONTEND")
        g.set_titles("{col_name} segundos micrófono " + device)
        plt.savefig('questions/images/best_protocol_record_' + device + '.png')
        diff_record(data_record_device,device)

def check_scale(data):
    global minimum,maximum
    minimum = sys.float_info.max
    maximum = 0
    for phase in ["NANOMSG"]:
        minimum = data["TP_"+phase].min() if data["TP_"+phase].min() < minimum else minimum
        maximum = data["TP_"+phase].max() if data["TP_"+phase].max() > maximum else maximum
def diff_play(data):
    data_play = data.groupby(["TIME","CHUNK SIZE","TRANSPORT PROTOCOL"]).mean()
    play = defaultdict(lambda:list())
    for index,row in data_play.iterrows():
        value = row["TP_NANOMSG"]
        play[(index[0],index[1])].append((index[2],value))
        play[(index[0],index[1])].sort(key = lambda tup: tup[1], reverse = True)
    for time,size in play:
        f.write("\nAudio de " + str(time) + " segundos en reproducción\n")
        f.write("Mejor protocolo para chunks de " + str(size) + " : " + str(play[(time,size)][0][0]) + " throughput " + str(round(play[(time,size)][0][1],3)) + " kBps"+ "\n")
        for protocol, value in play[(time,size)]:
            if protocol != play[(time,size)][0][0]:f.write(str(protocol) + ":" + str(round(value - play[(time,size)][0][1],3)) + " kBps de diferencia" + "\n")
            
def diff_record(data,device):
    data_record = data.groupby(["TIME","CHUNK SIZE","TRANSPORT PROTOCOL"]).mean()
    play = defaultdict(lambda:list())
    for index,row in data_record.iterrows():
        value = row["TP_NANOMSG"]
        play[(index[0],index[1])].append((index[2],value))
        play[(index[0],index[1])].sort(key = lambda tup: tup[1], reverse = True)
    for time,size in play:
        f.write("\nAudio de " + str(time) + " segundos en grabación disp.:" + device + "\n")
        f.write("Mejor protocolo para chunks de " + str(size) + " : " + str(play[(time,size)][0][0]) + " throughput " + str(round(play[(time,size)][0][1],3)) + " kBps"+ "\n")
        for protocol, value in play[(time,size)]:
            if protocol != play[(time,size)][0][0]:f.write(str(protocol) + ":" + str(round(value - play[(time,size)][0][1],3)) + " kBps de diferencia" + "\n")
            
if __name__ == "__main__":
    main()