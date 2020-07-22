import pandas as pd
import numpy as np
import os
import matplotlib.pyplot as plt
from pathlib import Path
home = str(Path.home())
os.chdir(home+"/Desktop/diegog-vol3/node/benchmark_comms/")
data = pd.read_csv("CSVs/raw_results.csv")
data = data.sort_values(by=["CHUNK SIZE","TIME","SIZE"])

data = data[["ACTION","CHUNK SIZE","TIME","SIZE"]]
data=data[data.ACTION != 'record']
data = data.groupby(by = ["ACTION","CHUNK SIZE","TIME"])
print("Comprueba que todos los valores de SIZE son 0, sino el benchmark estará mal")
print(data.std())

#Todos los valores de la columna SIZE tienen que ser 0, sino es que ha habido un error en el benchmark
