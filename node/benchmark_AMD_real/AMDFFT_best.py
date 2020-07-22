import pandas as pd 
import numpy as np
import os 
import sys
import matplotlib.pyplot as plt 
import seaborn as sb
import warnings

def check_scale(data):
    global minimum,maximum
    minimum = sys.float_info.max
    maximum = 0
    minimum = data["tiempo_total"].min() if data["tiempo_total"].min() < minimum else minimum
    maximum = data["tiempo_total"].max() if data["tiempo_total"].max() > maximum else maximum

if __name__ == "__main__":
        
    warnings.simplefilter("ignore")

    data = pd.read_csv("data.csv")

    data_aux = data
    data_aux = data_aux[data_aux.fichero == 5]
    data_aux = data_aux[data_aux.tamaño == 1024]
    check_scale(data_aux)

    sb.set(style="whitegrid")
    g = sb.catplot(ci = "sd",x = "tamaño", y = "tiempo_total",hue = "dispositivo",col = "fichero",kind="bar",data=data_aux)
    plt.ylim(minimum-0.01 , maximum + 0.01 )
    g.set_titles("Fichero {col_name} segundos" )
    sb.set(style="whitegrid")
    g._legend.set_title("Dispositivo")

    g.set(xlabel='Muestras/Paquete')
    g.set(ylabel='Tiempo de ejecución (s)')
    plt.savefig('images/AMDFFT_REAL_5_1024.png')
    

    data_aux = data
    data_aux = data_aux[data_aux.fichero == 5]
    data_aux = data_aux[data_aux.tamaño == 8192]
    check_scale(data_aux)

    sb.set(style="whitegrid")
    g = sb.catplot(ci = "sd",x = "tamaño", y = "tiempo_total",hue = "dispositivo",col = "fichero",kind="bar",data=data_aux)
    plt.ylim(minimum-0.01 , maximum + 0.01 )
    g.set_titles("Fichero {col_name} segundos" )
    sb.set(style="whitegrid")
    g._legend.set_title("Dispositivo")

    g.set(xlabel='Muestras/Paquete')
    g.set(ylabel='Tiempo de ejecución (s)')
    plt.savefig('images/AMDFFT_REAL_5_8192.png')

    data_aux = data
    data_aux = data_aux[data_aux.fichero == 20]
    data_aux = data_aux[data_aux.tamaño == 1024]
    check_scale(data_aux)

    sb.set(style="whitegrid")
    g = sb.catplot(ci = "sd",x = "tamaño", y = "tiempo_total",hue = "dispositivo",col = "fichero",kind="bar",data=data_aux)
    plt.ylim(minimum-0.01 , maximum + 0.01 )
    g.set_titles("Fichero {col_name} segundos" )
    sb.set(style="whitegrid")
    g._legend.set_title("Dispositivo")

    g.set(xlabel='Muestras/Paquete')
    g.set(ylabel='Tiempo de ejecución (s)')
    plt.savefig('images/AMDFFT_REAL_20_1024.png')

    data_aux = data
    data_aux = data_aux[data_aux.fichero == 20]
    data_aux = data_aux[data_aux.tamaño == 8192]
    check_scale(data_aux)

    sb.set(style="whitegrid")
    g = sb.catplot(ci = "sd",x = "tamaño", y = "tiempo_total",hue = "dispositivo",col = "fichero",kind="bar",data=data_aux)
    plt.ylim(minimum-0.01 , maximum + 0.01 )
    g.set_titles("Fichero {col_name} segundos" )
    sb.set(style="whitegrid")
    g._legend.set_title("Dispositivo")

    g.set(xlabel='Muestras/Paquete')
    g.set(ylabel='Tiempo de ejecución (s)')
    plt.savefig('images/AMDFFT_REAL_20_8192.png')

    plt.show()