import csv 
import sys
import os
import numpy
import re
from os import listdir
from os.path import isfile, join
from collections import defaultdict
from pathlib import Path
home = str(Path.home())
os.chdir(home+"/Desktop/diegog-vol3/node/benchmark_comms/")
header = ["ACTION",
"DEVICE",
"TIME",
"CHUNK SIZE",
"TRANSPORT PROTOCOL",
"C PROCESSING",
"ONLY RECORD",
"BARS",
"NUMBER OF TEST",
"FRONTEND",
"FRONTEND_WZ",
"ELECTRON",
"NANOMSG",
"LAT_FRONT-ELEC",
"LAT_FRONT-NANO",
"LAT_ELEC-NANO",
"TP_FRONTEND",
"TP_ELECTRON",
"TP_NANOMSG",
"SIZE",
"TP_WZ_FRONTEND",
"TP_WZ_ELECTRON",
"TP_WZ_NANOMSG",
"SIZE_WZ"]

resultsPath = "results_benchmark/"
rawCsvPath = "CSVs/raw_results.csv"
timeFiles = [f for f in listdir(resultsPath) if isfile(join(resultsPath, f))]
timeFiles.sort();

data = defaultdict(lambda:defaultdict(lambda:0))
indexes = set([])
for fileName in timeFiles:
    if "bench" in fileName:
        metadata = fileName.split('-')
        time = metadata[0]
        chunkSize = metadata[1]
        transportProtocol = metadata[2]
        c_processing = True if "cprocessing" in metadata[3] else False
        bars = True if "bars" in metadata[3] else False
        only_record = True if "onlyrecord" in metadata[3] else False
        action = "play" if "play" in metadata[4] else "record"
        aux = metadata[4].replace("bench","").replace(action,"").replace(".txt","")
        numberTest = aux
        if action is "record": 
            try: 
                numberTest = int(aux[:2])
                device = aux[2:]
            except ValueError:
                numberTest = int(aux[:1])
                device = aux[1:]

        with open(resultsPath + fileName, mode="r") as file:
            lines = file.readlines()
            for index, line in enumerate(lines): lines[index] = line.replace("\n","")
        if action is "play":
            index = action + str(time) + str(chunkSize) + str(transportProtocol) + str(c_processing) + str(only_record) + str(bars) + str(numberTest)
        else:
            index = action + str(time) + str(chunkSize) + str(transportProtocol) + str(c_processing) + str(only_record) + str(bars) + str(numberTest) + device

        indexes.add(index)
        data[index]["ACTION"] = action
        if action is "record":data[index]["DEVICE"] = device
        data[index]["TIME"] = time
        data[index]["CHUNK SIZE"] = chunkSize
        data[index]["TRANSPORT PROTOCOL"] = transportProtocol
        data[index]["C PROCESSING"] = c_processing
        data[index]["ONLY RECORD"] = only_record
        data[index]["BARS"] = bars
        data[index]["NUMBER OF TEST"] = numberTest
        frontend = (float(lines[1]) - float(lines[0])) 
        frontendWZ = (float(lines[4]) - float(lines[3]))
        data[index]["FRONTEND"] = round(frontend,3)  
        data[index]["FRONTEND_WZ"] = round(frontendWZ,3)
        if only_record:
            if action is "play":
                sizeWZ = float(lines[-2])
                data[index]["SIZE_WZ"] = sizeWZ   
                data[index]["TP_WZ_FRONTEND"] = round((sizeWZ / frontend)/1024.,3)
            for i in indexes:
                if action+str(time) in i and i is not index: 
                    size = data[i]["SIZE"]
                    break
            data[index]["SIZE"] = size   
            data[index]["TP_FRONTEND"] = round((size / frontend)/1024.,3)

        else:
            electron = (float(lines[6]) - float(lines[5]))
            latFrontElec = (float(lines[7]) - float(lines[2])) 
            size = float(lines[-3])
            data[index]["ELECTRON"] = round(electron,3)
            data[index]["LAT_FRONT-ELEC"] = round(latFrontElec,3)      
            data[index]["SIZE"] = size
            data[index]["TP_FRONTEND"] = round((size / frontend)/1024.,3)
            data[index]["TP_ELECTRON"] = round((size / electron)/1024.,3)
            if action is "play":
                sizeWZ = float(lines[-2])
                data[index]["SIZE_WZ"] = sizeWZ   
                data[index]["TP_WZ_ELECTRON"] = round((sizeWZ / electron)/1024.,3)
                data[index]["TP_WZ_FRONTEND"] = round((sizeWZ / frontendWZ)/1024.,3)
        if c_processing:
            nanomsg = (float(lines[9]) - float(lines[8]))  
            latFrontNano = (float(lines[10]) - float(lines[2])) 
            latElecNano = (float(lines[10]) - float(lines[7])) 
            data[index]["NANOMSG"] = round(nanomsg,3) 
            data[index]["LAT_FRONT-NANO"] = round(latFrontNano,3)
            data[index]["LAT_ELEC-NANO"] = round(latElecNano,3) 
            data[index]["TP_NANOMSG"] = round((size / nanomsg)/1024.,3)
            if action is "play":
                data[index]["TP_WZ_NANOMSG"] = round((sizeWZ / nanomsg)/1024.,3)
        
for index in indexes:
    if os.path.isfile(rawCsvPath):
        with open(rawCsvPath, mode='a') as csv_file:
            writer = csv.DictWriter(csv_file,fieldnames=header)
            writer.writerow(data[index])
    else:
        with open(rawCsvPath, mode='w') as csv_file:
            writer = csv.DictWriter(csv_file,fieldnames=header)
            writer.writeheader()
            writer.writerow(data[index])
