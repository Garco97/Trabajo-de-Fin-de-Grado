import pandas as pd 
import numpy as np
import os 
import matplotlib.pyplot as plt 
import seaborn as sb
import warnings
warnings.simplefilter("ignore")

data = pd.read_csv("par-bench.csv")
# !TYPE
# ! LENGTH
# ! SIZE
# !TOTAL_TIME_BEGIN
# !TOTAL_TIME_END
# !FIRST_CHUNK_BEGIN
# !FIRST_CHUNK_END
# !LAST_CHUNK_BEGIN
# !LAST_CHUNK_END
data["TOTAL_TIME"] = data["TYPE"]
data["FIRST_CHUNK"] = data["TYPE"]
data["LAST_CHUNK"] = data["TYPE"]

for indice,tipo in enumerate(data.TYPE):
    data.TOTAL_TIME[indice] = round((data.TOTAL_TIME_END[indice]-data.TOTAL_TIME_BEGIN[indice])/1000.0,3)
    data.FIRST_CHUNK[indice] = round((data.FIRST_CHUNK_END[indice]-data.FIRST_CHUNK_BEGIN[indice]),3)
    data.LAST_CHUNK[indice] = round((data.LAST_CHUNK_END[indice]-data.LAST_CHUNK_BEGIN[indice]),3)
    
data.to_csv("MEANS.csv")
data = pd.read_csv("MEANS.csv")
data_aux = data.groupby(["TYPE", "LENGTH", "SIZE"])
data_mean = data_aux.mean()
data_mean.to_csv("MEANS.csv")
data_mean = pd.read_csv("MEANS.csv")

seq_values = dict()
seq_values[16384] = dict()
seq_values[8192] = dict()

for i in data_mean.iloc:
    if i.TYPE == "seq":
        seq_values[i.SIZE][i.LENGTH] = i.TOTAL_TIME
data_mean['SPEED-UP'] = data_mean['TOTAL_TIME']
for indice,tipo in enumerate(data_mean.TYPE):
    if data_mean.LENGTH[indice] == 5:
        if data_mean.SIZE[indice] == 8192:
            data_mean["SPEED-UP"][indice] = seq_values[8192][5]/data_mean["TOTAL_TIME"][indice]
        if data_mean.SIZE[indice] == 16384:
            data_mean["SPEED-UP"][indice] = seq_values[16384][5]/data_mean["TOTAL_TIME"][indice]
    if data_mean.LENGTH[indice] == 20:
        if data_mean.SIZE[indice] == 8192:
            data_mean["SPEED-UP"][indice] = seq_values[8192][20]/data_mean["TOTAL_TIME"][indice]
        if data_mean.SIZE[indice] == 16384:
            data_mean["SPEED-UP"][indice] = seq_values[16384][20]/data_mean["TOTAL_TIME"][indice]
data  = data.sort_values(['SIZE']).reset_index(drop=True)

first_sorted = data.sort_values(['FIRST_CHUNK']).reset_index(drop=True)
task_first = first_sorted
data_first = first_sorted
last_sorted = data.sort_values(['LAST_CHUNK']).reset_index(drop=True)
task_last =last_sorted
data_last = last_sorted
speed_sorted = data_mean.sort_values(['SPEED-UP']).reset_index(drop=True)
total_time_sorted = data.sort_values(['TOTAL_TIME']).reset_index(drop=True)
for t in first_sorted.TYPE.unique():
    if "data" in t: 
        task_first = task_first[task_first.TYPE != t]
        task_last = task_last[task_last.TYPE != t]
    if "task" in t:
        data_first = data_first[data_first.TYPE != t]
        data_last = data_last[data_last.TYPE != t]

for v in data.TYPE.unique():
    aux = v 
    aux = aux.replace("data","D")
    aux = aux.replace("task","T")
    aux = aux.replace("simd","SIMD_")
    aux = aux.replace("seq","S")
    aux = aux.replace("dynamic","DYN")
    aux = aux.replace("guided","GUI")
    aux = aux.replace("static","STA")
    data = data.replace(v,aux) 
    data_mean = data_mean.replace(v,aux)
    first_sorted = first_sorted.replace(v,aux) 
    task_first = task_first.replace(v,aux)
    data_first = data_first.replace(v,aux) 
    last_sorted = last_sorted.replace(v,aux)
    task_last = task_last.replace(v,aux) 
    data_last = data_last.replace(v,aux)
    speed_sorted = speed_sorted.replace(v,aux) 
    total_time_sorted = total_time_sorted.replace(v,aux)

sb.set(style="whitegrid")
g = sb.catplot(ci = "sd",x = "SIZE", y = "SPEED-UP",hue = "TYPE",col="LENGTH",kind="bar",data=speed_sorted)
plt.ylim(speed_sorted['SPEED-UP'].min()-0.01,speed_sorted['SPEED-UP'].max()+0.01)
g.set_titles("Fichero {col_name} segundos")
sb.set(style="whitegrid")
g.set(xlabel='Muestras/Paquete')
g._legend.set_title("Optimizaciones")
g.set(ylabel='Speed-Up')
plt.savefig('images/speed_up.png')

aux5 = total_time_sorted[total_time_sorted.LENGTH == 5]
g = sb.catplot(ci = "sd",x = "SIZE", y = "TOTAL_TIME",hue = "TYPE",col="LENGTH",kind="bar",data=aux5)
plt.ylim(aux5['TOTAL_TIME'].min()-1,aux5['TOTAL_TIME'].max()+1)
g.set_titles("Fichero {col_name} segundos")
sb.set(style="whitegrid")
g.set(xlabel='Muestras/Paquete')
g._legend.set_title("Optimizaciones")
g.set(ylabel='Tiempo medio del programa (s)')
plt.savefig('images/total_time_mean_5.png')
aux20 = total_time_sorted[total_time_sorted.LENGTH == 20]

g = sb.catplot(ci = "sd",x = "SIZE", y = "TOTAL_TIME",hue = "TYPE",col="LENGTH",kind="bar",data=aux20)
plt.ylim(aux20['TOTAL_TIME'].min()-1,aux20['TOTAL_TIME'].max()+1)
g.set_titles("Fichero {col_name} segundos")
sb.set(style="whitegrid")
g.set(xlabel='Muestras/Paquete')
g._legend.set_title("Optimizaciones")
g.set(ylabel='Tiempo medio del programa (s)')
plt.savefig('images/total_time_mean_20.png')

g = sb.catplot(ci = "sd",x = "SIZE", y = "FIRST_CHUNK",hue = "TYPE",col="LENGTH",kind="bar",data=first_sorted)
plt.ylim(first_sorted['FIRST_CHUNK'].min()-1,first_sorted['FIRST_CHUNK'].max()+1)
g.set_titles("Fichero {col_name} segundos")
sb.set(style="whitegrid")
g.set(xlabel='Muestras/Paquete')
g._legend.set_title("Optimizaciones")
g.set(ylabel='Latencia media primer paquete (ms)')
plt.savefig('images/first_lat.png')

g = sb.catplot(ci = "sd",x = "SIZE", y = "LAST_CHUNK",hue = "TYPE",col="LENGTH",kind="bar",data=last_sorted)
plt.ylim(last_sorted['LAST_CHUNK'].min()-1,last_sorted['LAST_CHUNK'].max()+1)
g.set_titles("Fichero {col_name} segundos")
sb.set(style="whitegrid")
g.set(xlabel='Muestras/Paquete')
g._legend.set_title("Optimizaciones")
g.set(ylabel='Latencia media último paquete (ms)')
plt.savefig('images/last_lat.png')

g = sb.catplot(ci = "sd",x = "SIZE", y = "FIRST_CHUNK",hue = "TYPE",col="LENGTH",kind="bar",data=task_first)
plt.ylim(0,task_first['FIRST_CHUNK'].max()+1)
g.set_titles("Paralelismo de tareas Fichero {col_name} segundos")
sb.set(style="whitegrid")
g.set(xlabel='Muestras/Paquete')
g._legend.set_title("Optimizaciones")
g.set(ylabel='Latencia media primer paquete (ms)')
plt.savefig('images/first_task_lat.png')

g = sb.catplot(ci = "sd",x = "SIZE", y = "LAST_CHUNK",hue = "TYPE",col="LENGTH",kind="bar",data=data_first)
plt.ylim(0,data_first['LAST_CHUNK'].max()+1)
g.set_titles("Paralelismo de datos Fichero {col_name} segundos")
sb.set(style="whitegrid")
g.set(xlabel='Muestras/Paquete')
g._legend.set_title("Optimizaciones")
g.set(ylabel='Latencia media primer paquete (ms)')
plt.savefig('images/first_data_lat.png')

g = sb.catplot(ci = "sd",x = "SIZE", y = "FIRST_CHUNK",hue = "TYPE",col="LENGTH",kind="bar",data=task_last)
plt.ylim(0,task_last['FIRST_CHUNK'].max()+1)
g.set_titles("Paralelismo de tareas Fichero {col_name} segundos")
sb.set(style="whitegrid")
g.set(xlabel='Muestras/Paquete')
g._legend.set_title("Optimizaciones")
g.set(ylabel='Latencia media último paquete (ms)')
plt.savefig('images/last_task_lat.png')

g = sb.catplot(ci = "sd",x = "SIZE", y = "LAST_CHUNK",hue = "TYPE",col="LENGTH",kind="bar",data=data_last)
plt.ylim(0,data_last['LAST_CHUNK'].max()+1)
g.set_titles("Paralelismo de datos Fichero {col_name} segundos")
sb.set(style="whitegrid")
g.set(xlabel='Muestras/Paquete')
g._legend.set_title("Optimizaciones")
g.set(ylabel='Latencia media último paquete (ms)')
plt.savefig('images/last_data_lat.png')


