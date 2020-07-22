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
    f = open("questions/initialPenalty.txt","w")
    data = pd.read_csv("CSVs/raw_results.csv")
    data = data[["ACTION","DEVICE","CHUNK SIZE", "TIME","TP_FRONTEND","C PROCESSING" ]]
    check_scale(data)
    data_play = data[data.ACTION == 'play']
    plot_play(data_play,"FRONTEND")
    frontend_play = diff_play(data_play,"FRONTEND")
    print_table_play(frontend_play)
    if "record" in data.ACTION.unique():
        data_record = data[data.ACTION == 'record']
        plot_record(data_record,"FRONTEND")
        frontend_record = diff_record(data_record,"FRONTEND")
        print_table_record(frontend_record)
    f.close()


def check_scale(data):
    global minimum,maximum
    minimum = sys.float_info.max
    maximum = 0
    for phase in ["FRONTEND"]:
        minimum = data["TP_"+phase].min() if data["TP_"+phase].min() < minimum else minimum
        maximum = data["TP_"+phase].max() if data["TP_"+phase].max() > maximum else maximum

def plot_play(data,throughput):
    sb.set(style="whitegrid")
    g = sb.catplot(ci="sd",x="CHUNK SIZE", y="TP_FRONTEND", hue = "TIME",kind="bar",data=data)
    g.set(ylabel='Rendimiento frontend (Kbps)')
    g.set(xlabel='Muestras/Paquete')
    g._legend.set_title("Tiempo (s)")
    plt.ylim(minimum, maximum)
    plt.title("Penalización al reproducir" )
    plt.savefig('questions/images/initialPenalty' + throughput + '_play.png')
    plt.show()

def plot_record(data, throughput):
    sb.set(style="whitegrid")
    g = sb.catplot(ci="sd",x="CHUNK SIZE", y="TP_FRONTEND",col="DEVICE",hue = "TIME",kind="bar",data=data)
    g.set(ylabel='Rendimiento frontend (Kbps)')
    g.set(xlabel='Muestras/Paquete')
    g._legend.set_title("Tiempo (s)")
    g.set_titles("Penalización al grabar microfono {col_name} ")
    plt.ylim(minimum, maximum)
    plt.savefig('questions/images/initialPenalty' + throughput + '_record.png')
    plt.show()

def diff_play(data,throughput):
    data_play = data.groupby(["CHUNK SIZE","TIME"]).mean()
    play = defaultdict(lambda:list())
    for index,row in data_play.iterrows():
        value = row["TP_FRONTEND"]
        play[index[0]].append((index[1],value))
        play[index[0]].sort(key = lambda tup: tup[1], reverse = True)
    return play

def diff_record(data,throughput):
    data_record = data.groupby(["CHUNK SIZE","TIME","DEVICE"]).mean()
    record = defaultdict(lambda:list())
    for index,row in data_record.iterrows():
        value = row["TP_FRONTEND"]
        record[(index[0],index[2])].append((index[1],value))
        record[(index[0],index[2])].sort(key = lambda tup: tup[1], reverse = True)
    return record

def print_table_play(frontend):
    keys = [i for i in frontend if frontend[i]!=frontend.default_factory()]
    for key in keys:
        x = PrettyTable()
        x.field_names = ["Action", "Device", "Chunk Size", "Frontend"]
        for i in range(0,len(frontend[key])):
            x.add_row(["Play","-",key, str(frontend[key][i][0]) + ": " + str(round(frontend[key][i][1] - frontend[key][0][1],3)) + "kBps (" + str(round((frontend[key][i][1] - frontend[key][0][1])/frontend[key][0][1],5)) + "%)"])
        f.write(str(x) + "\n")
       
def print_table_record(frontend):
    keys = [i for i in frontend if frontend[i]!=frontend.default_factory()]
    for key in keys:
        x = PrettyTable()
        x.field_names = ["Action", "Device", "Chunk Size", "Frontend"]
        for i in range(0,len(frontend[key])):
            x.add_row(["Record",key[1],key[0], str(frontend[key][i][0]) + ": " + str(round(frontend[key][i][1] - frontend[key][0][1],3)) + "kBps (" + str(round((frontend[key][i][1] - frontend[key][0][1])/frontend[key][0][1],5)) + "%)"])
        f.write(str(x) + "\n")

if __name__ == "__main__":
    main()
