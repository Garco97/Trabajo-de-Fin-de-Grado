import pandas as pd
import numpy as np
import os
import sys
import matplotlib.pyplot as plt
import seaborn as sb
from collections import defaultdict
from pathlib import Path
from prettytable import PrettyTable

import warnings
warnings.simplefilter("ignore")
def main():
    global f
    home = str(Path.home())
    os.chdir(home+"/Desktop/diegog-vol3/node/benchmark_comms/")
    f = open("questions/bestPerformance.txt","w")
    data = pd.read_csv("CSVs/raw_results.csv")
    data = data[["ACTION","DEVICE","CHUNK SIZE","BARS","ONLY RECORD", "TIME","TP_FRONTEND","TP_ELECTRON","TP_NANOMSG","C PROCESSING" ]]
    check_scale(data)
    data_play = data[data.ACTION == 'play']
    data_play = data_play[data_play["ONLY RECORD"] == False]
    data_play = data_play[data_play["C PROCESSING"] == True]
    plot_play(data_play,"FRONTEND")
    plot_play(data_play,"ELECTRON")
    plot_play(data_play,"NANOMSG")
    frontend_play = diff_play(data_play,"FRONTEND")
    electron_play = diff_play(data_play,"ELECTRON")
    nanomsg_play = diff_play(data_play,"NANOMSG") 
    print_table_play(frontend_play,electron_play,nanomsg_play)
    if "record" in data.ACTION.unique():
        data_record = data[data.ACTION == 'record']
        data_record = data_record[data_record["ONLY RECORD"] == False]
        data_record = data_record[data_record["C PROCESSING"] == True]
        plot_record(data_record,"FRONTEND")
        plot_record(data_record,"ELECTRON")
        plot_record(data_record,"NANOMSG")
        frontend_record = diff_record(data_record,"FRONTEND")
        electron_record = diff_record(data_record,"ELECTRON")
        nanomsg_record = diff_record(data_record,"NANOMSG")
        print_table_record(frontend_record,electron_record, nanomsg_record)
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
    g = sb.catplot(ci="sd",x="TIME", y="TP_"+throughput,hue = "CHUNK SIZE",kind="bar",data=data)
    g.set(ylabel='Rendimiento (Kbps)')
    g.set(xlabel='Tiempo de reproducción (s)')
    g._legend.set_title("Muestras/Paquete")
    plt.ylim(minimum,maximum)
    if throughput == "FRONTEND":
        aux = "Frontend"
    elif throughput == "ELECTRON":
        aux = "Nodo de comunicaciones"
    elif throughput == "NANOMSG":
        aux = "Nodo de procesamiento"
    plt.title(aux + " reproducción" )
    #plt.savefig('questions/images/best_performance_' + throughput + '_play.png', dpi=96)
    plt.show()

def plot_record(data, throughput):
    sb.set(style="whitegrid")
    g = sb.catplot(ci="sd",x="TIME", y="TP_"+throughput,col="DEVICE",hue = "CHUNK SIZE",kind="bar",data=data)
    g.set(ylabel='Rendimiento (Kbps)')
    g.set(xlabel='Tiempo de grabación (s)')
    g._legend.set_title("Muestras/Paquete")
    if throughput == "FRONTEND":
        aux = "Frontend"
    elif throughput == "ELECTRON":
        aux = "Nodo de comunicaciones"
    elif throughput == "NANOMSG":
        aux = "Nodo de procesamiento"
    print(data.DEVICE.unique())
    g.set_titles(aux+" micrófono {col_name} ")
    plt.ylim(minimum,maximum)

    #plt.savefig('questions/images/best_performance_' + throughput + '_record.png', dpi=96)
    plt.show()

def diff_play(data,throughput):
    data_play = data.groupby(["TIME","CHUNK SIZE"]).mean()
    play = defaultdict(lambda:list())
    for index,row in data_play.iterrows():
        value = row["TP_" + throughput]
        play[index[0]].append((index[1],value))
        play[index[0]].sort(key = lambda tup: tup[1], reverse = True)
    
    return play
def diff_record(data,throughput):
    data_record = data.groupby(["TIME","CHUNK SIZE","DEVICE"]).mean()
    record = defaultdict(lambda:list())
    for index,row in data_record.iterrows():
        value = row["TP_" + throughput]
        record[(index[0],index[2])].append((index[1],value))
        record[(index[0],index[2])].sort(key = lambda tup: tup[1], reverse = True)
    
    return record


def print_table_play(frontend, electron, nanomsg):
    keys = [i for i in frontend if frontend[i]!=frontend.default_factory()]
    for key in keys:
        x = PrettyTable()
        x.field_names = ["Action","Device","Chunk Size", "Frontend", "Electron", "Nanomsg"]
        for i in range(0,len(frontend[key])):
            x.add_row(["Play","-",key, str(frontend[key][i][0]) + ":" + str(round(frontend[key][i][1] - frontend[key][0][1],3)) + " kBps",
                       str(electron[key][i][0]) + ": " + str(round(electron[key][i][1] - electron[key][0][1],3)) + " kBps",
                       str(nanomsg[key][i][0]) + ": " + str(round(nanomsg[key][i][1] - nanomsg[key][0][1],3)) + " kBps"])
        f.write(str(x) + "\n")
       
def print_table_record(frontend, electron, nanomsg):
    keys = [i for i in frontend if frontend[i]!=frontend.default_factory()]
    for key in keys:
        x = PrettyTable()
        x.field_names = ["Action","Device","Chunk Size", "Frontend", "Electron", "Nanomsg"]
        for i in range(0,len(frontend[key])):
            x.add_row(["Record",key[1],key[0], str(frontend[key][i][0]) + ": " + str(round(frontend[key][i][1] - frontend[key][0][1],3)) + " kBps",
                       str(electron[key][i][0]) + ": " + str(round(electron[key][i][1] - electron[key][0][1],3)) + " kBps",
                       str(nanomsg[key][i][0]) + ": " + str(round(nanomsg[key][i][1] - nanomsg[key][0][1],3)) + " kBps"])
        f.write(str(x) + "\n")

if __name__ == "__main__":
    main()
