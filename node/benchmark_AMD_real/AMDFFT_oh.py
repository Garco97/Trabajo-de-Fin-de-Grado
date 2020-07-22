import pandas as pd 
import numpy as np
import os 
import sys
import matplotlib.pyplot as plt 
import seaborn as sb
import warnings

def check_scale(data,oh=True):
    global minimum,maximum
    minimum = sys.float_info.max
    maximum = 0
    if oh:
        minimum = data["speed_up"].min() if data["speed_up"].min() < minimum else minimum
        maximum = data["speed_up"].max() if data["speed_up"].max() > maximum else maximum
    else:
        minimum = data["tiempo_total"].min() if data["tiempo_total"].min() < minimum else minimum
        maximum = data["tiempo_total"].max() if data["tiempo_total"].max() > maximum else maximum

def get_speedup(data):
    data_a = data.groupby(["tamaño", "dispositivo", "fichero"]).mean()
    data_a.to_csv("mean.csv")
    data_a = pd.read_csv("mean.csv").sort_values("tamaño")
    print(data_a)
    for index, row in data_a.iterrows():
        data_a["speed_up"][index] = data_a.iloc[ 0 , -2 ]/row["speed_up"]
    return data_a
if __name__ == "__main__":
        
    warnings.simplefilter("ignore")

    data = pd.read_csv("data.csv")
    
    data["speed_up"]= data["tiempo_total"]
    for fich in data.fichero.unique():
        for dispositivo in data.dispositivo.unique():
            data_aux = data[data.dispositivo == dispositivo]
            data_aux = data_aux[data_aux.fichero == fich]
            
            data_aux = get_speedup(data_aux)
            print(data_aux)
            check_scale(data_aux)
            sb.set(style="whitegrid")
            g = sb.catplot(ci = "sd",x = "dispositivo", y = "speed_up",hue = "tamaño",col = "fichero",kind="bar",data=data_aux)
            plt.ylim(minimum-0.05 , maximum + 0.05 )
            g.set_titles("Fichero {col_name} segundos" )
            sb.set(style="whitegrid")
            g.set(xlabel='Procesador')
            g.set(ylabel='Speed-Up')
            g._legend.set_title("Muestras/Paquete")
            plt.savefig('images/AMDFFT_REAL_oh_'+str(fich)+'_'+dispositivo+'.png')

            check_scale(data_aux,False)
            sb.set(style="whitegrid")
            g = sb.catplot(ci = "sd",x = "dispositivo", y = "tiempo_total",hue = "tamaño",col = "fichero",kind="bar",data=data_aux)
            plt.ylim(minimum-1 , maximum + 1 )
            g.set_titles("Fichero {col_name} segundos" )
            sb.set(style="whitegrid")
            g.set(xlabel='Procesador')
            g.set(ylabel='Tiempo de ejecución (s)')
            g._legend.set_title("Muestras/Paquete")
            plt.savefig('images/AMDFFT_REAL_te_'+str(fich)+'_'+dispositivo+'.png')

    plt.show()
    