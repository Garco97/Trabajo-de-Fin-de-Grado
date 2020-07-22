import pandas as pd 
import numpy as np
import os 
import sys
import matplotlib.pyplot as plt 
import seaborn as sb
import warnings

def check_scale(data,dictio):
    global minimum,maximum
    minimum = sys.float_info.max
    maximum = 0
    for disp in data.dispositivo.unique():
        for size in data.tamaño.unique():
            for fich in data.fichero.unique():
                minimum = dictio[str(disp)+"-"+str(size)+"-"+str(fich)].speed_up.min() if dictio[str(disp)+"-"+str(size)+"-"+str(fich)].speed_up.min() < minimum else minimum
                maximum = dictio[str(disp)+"-"+str(size)+"-"+str(fich)].speed_up.max() if dictio[str(disp)+"-"+str(size)+"-"+str(fich)].speed_up.max() > maximum else maximum
if __name__ == "__main__":
        
    warnings.simplefilter("ignore")

    data = pd.read_csv("data.csv")
    data_a = data.groupby(["tamaño", "dispositivo", "fichero","work_groups","work_items"])
    data_a.mean().to_csv("mean.csv")
    data_a = pd.read_csv("mean.csv")
    data_a = data_a.groupby(["tamaño", "dispositivo", "fichero","work_groups","work_items"]).mean()
    data_a.to_csv("mean.csv")
    data_a = pd.read_csv("mean.csv")
    
    master_data = dict()
    for disp in data_a.dispositivo.unique():
        for size in data_a.tamaño.unique():
            for fich in data_a.fichero.unique():
                data_aux = data_a[data_a.dispositivo == disp]
                data_aux = data_aux[data_aux.tamaño == size]
                data_aux = data_aux[data_aux.fichero == fich]
                data_aux = data_aux.sort_values("tiempo_total")
                data_aux["speed_up"]= data_aux["tiempo_total"]
                for index, row in data_aux.iterrows():
                    data_aux["speed_up"][index] = data_aux.iloc[ -1 , -2 ]/row["speed_up"]
                print(data_aux)
                master_data[str(disp)+"-"+str(size)+"-"+str(fich)] = data_aux
    check_scale(data_a,master_data)
    for disp in data_a.dispositivo.unique():
        for size in data_a.tamaño.unique():
            for fich in data_a.fichero.unique():
                sb.set(style="whitegrid")
                g = sb.catplot(ci = "sd",x = "tamaño", y = "speed_up",hue = "work_items",col = "fichero",kind="bar",data=master_data[str(disp)+"-"+str(size)+"-"+str(fich)])
                plt.ylim(minimum-0.1, maximum + 0.1)
                g._legend.set_title("LWS")
                g.set_titles("Fichero {col_name} segundos " + disp + " GWI=" + str(size) )
                sb.set(style="whitegrid")
                g.set(xlabel='Muestras/Paquete')
                g.set(ylabel='Speed-Up')
                plt.savefig('images/cl_thresh_'+disp+"-"+str(size)+"-"+str(fich)+'.png')
    
    
    data_b = pd.read_csv("mean.csv")
    master_data = dict()
    #data_b = data_b[data_b.work_items != 1]
    #data_b = data_b[data_b.work_items != 2]
    #data_b = data_b[data_b.work_items != 4]
    for item in data_b.work_items.unique():
        for size in data_b.tamaño.unique():
            for fich in data_b.fichero.unique():
                data_aux = data_b[data_b.tamaño == size]
                data_aux = data_aux[data_aux.work_items == item]
                data_aux = data_aux[data_aux.fichero == fich]
                data_aux = data_aux.sort_values("tiempo_total")
                data_aux["speed_up"]= data_aux["tiempo_total"]
                for index, row in data_aux.iterrows():
                    #print(data_aux.iloc[ -1 , -2 ])
                    data_aux["speed_up"][index] = data_aux.iloc[ -1 , -2 ]/row["speed_up"]
                #print(data_aux)
                master_data[str(item)+"-"+str(size)+"-"+str(fich)] = data_aux
    
    
    frames = [master_data[key] for key in master_data]
    result = pd.concat(frames)
    minimum = result.speed_up.min()
    maximum = result.speed_up.max()
    
    #print(result)
    sb.set(style="whitegrid")
    g = sb.catplot(ci = "sd",x = "tamaño", y = "speed_up",hue = "dispositivo",col = "fichero",kind="bar",data=result)
    plt.ylim(minimum-0.05, maximum + 0.05)
    g.set_titles("Fichero {col_name} segundos GPU vs. CPU " )
    plt.savefig('images/cl_thresh_gpuvcpu.png') 
