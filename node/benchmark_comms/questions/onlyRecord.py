import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sb
from pathlib import Path

import warnings
warnings.simplefilter("ignore")

home = str(Path.home())
os.chdir(home+"/Desktop/diegog-vol3/node/benchmark_comms/")
data = pd.read_csv("CSVs/raw_results.csv")
f = open("questions/onlyRecord.txt", "w")
data = data[["ACTION","DEVICE","CHUNK SIZE","BARS","ONLY RECORD", "TIME","TP_FRONTEND","C PROCESSING" ]]
f.write("\n################### REPRODUCCIÓN ###################"+ "\n")

data_play = data[data.ACTION == 'play']
data_play_onlyRecord = data_play[data_play["ONLY RECORD"] == True]
data_play_onlyRecord = data_play_onlyRecord[data_play_onlyRecord.BARS == False]
data_play_onlyRecord = data_play_onlyRecord.groupby(["TIME","CHUNK SIZE"]).mean()
onlyRecord = dict()
for index,row in data_play_onlyRecord.iterrows():
    onlyRecord[index] = row["TP_FRONTEND"]
data_play_all = data_play[data_play["ONLY RECORD"] == False]
data_play_all = data_play_all[data_play_all.BARS == True]
data_play_all = data_play_all[data_play_all["C PROCESSING"] == True]
data_play_all = data_play_all.groupby(["TIME","CHUNK SIZE"]).mean()
playAll = dict()

for index,row in data_play_all.iterrows():
    playAll[index] = row["TP_FRONTEND"]

for index in onlyRecord:
    solo,todo = onlyRecord[index],playAll[index]
    diff = round(solo-todo,3)
    f.write(str(index)[1:-1]+": "+ str(diff)+" kBps mejor reproduciendo solo"+ "\n")


data_record = data[data.ACTION == 'record']
for device in data_record.DEVICE.unique():
    f.write("\n################### GRABACIÓN DISPOSITIVO "+device+" ###################"+ "\n")

    data_record_onlyRecord = data_record[data_record.DEVICE == device]
    data_record_onlyRecord = data_record_onlyRecord[data_record_onlyRecord["ONLY RECORD"] == True]
    data_record_onlyRecord = data_record_onlyRecord[data_record_onlyRecord.BARS == False]
    data_record_onlyRecord = data_record_onlyRecord.groupby(["TIME","CHUNK SIZE"]).mean()
    onlyRecord = dict()
    for index,row in data_record_onlyRecord.iterrows():
        onlyRecord[index] = row["TP_FRONTEND"]
    data_record_all = data_record[data_record["ONLY RECORD"] == False]
    data_record_all = data_record_all[data_record_all.BARS == True]
    data_record_all = data_record_all[data_record_all["C PROCESSING"] == True]
    data_record_all = data_record_all.groupby(["TIME","CHUNK SIZE"]).mean()
    playAll = dict()

    for index,row in data_record_all.iterrows():
        playAll[index] = row["TP_FRONTEND"]

    for index in onlyRecord:
        solo,todo = onlyRecord[index],playAll[index]
        diff = round(solo-todo,3)
        f.write(str(index)[1:-1]+": "+ str(diff)+" kBps mejor grabando solo con microfono " +device+ "\n")

f.close()