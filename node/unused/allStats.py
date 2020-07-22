import csv 
import sys
import os
import numpy
from os import listdir
from os.path import isfile, join
from collections import defaultdict
os.chdir("/home/diego/Documents/Repositorios/diegog-vol3/node/benchmark/")
header = ["ACTION",
"TIME",
"CHUNK SIZE",
"TRANSPORT PROTOCOL",
"C PROCESSING",
"ONLY RECORD",
"BARS",
"FRONTEND",
"STD_FRONTEND",
"ELECTRON",
"STD_ELECTRON",
"NANOMSG",
"STD_NANOMSG",
"LAT_FRONT-ELEC",
"STD_LAT_FRONT-ELEC",
"LAT_FRONT-NANO",
"STD_LAT_FRONT-NANO",
"LAT_ELEC-NANO",
"STD_LAT_ELEC-NANO",
"TP_FRONTEND",
"TP_ELECTRON",
"TP_NANOMSG",
"SIZE",
"STD_SIZE",
"TP_WZ_FRONTEND",
"TP_WZ_ELECTRON",
"TP_WZ_NANOMSG",
"SIZE_WZ",
"STD_SIZE_WZ",
"NUMBER OF TEST"]

resultsPath = "results_benchmark/"
rawCsvPath = "CSVs/raw_results.csv"
timeFiles = [f for f in listdir(resultsPath) if isfile(join(resultsPath, f))]
timeFiles.sort();

data = []
average = defaultdict(lambda:defaultdict(lambda:0))
stdDeviation = defaultdict(lambda:defaultdict(lambda:[]))
indexes = set([])
for fileName in timeFiles:
    if "bench" in fileName:
        metadata = fileName.split('-')
        print(metadata)
        time = metadata[0]
        chunkSize = metadata[1]
        transportProtocol = metadata[2]
        action = "play" if "play" in metadata[4] else "record"
        c_processing = True if "cprocessing" in metadata[3] else False
        bars = True if "bars" in metadata[3] else False
        only_record = True if "onlyrecord" in metadata[3] else False
        with open(resultsPath + fileName, mode="r") as file:
            lines = file.readlines()
            for index, line in enumerate(lines): lines[index] = line.replace("\n","")
        index = action + str(time) + str(chunkSize) + str(transportProtocol) + str(c_processing) + str(only_record) + str(bars)
        indexes.add(index)
        average[index]["ACTION"] = action
        average[index]["TIME"] = time
        average[index]["CHUNK SIZE"] = chunkSize
        average[index]["TRANSPORT PROTOCOL"] = transportProtocol
        average[index]["C PROCESSING"] = c_processing
        average[index]["ONLY RECORD"] = only_record
        average[index]["BARS"] = bars
        frontend = (float(lines[1]) - float(lines[0])) 
        stdDeviation[index]["FRONTEND"].append(frontend)
        average[index]["FRONTEND"] += frontend  
        if not only_record:
            electron = (float(lines[4]) - float(lines[3]))
            latFrontElec = (float(lines[5]) - float(lines[2])) 
            sizeWZ = float(lines[-3])
            size = float(lines[-2])
            average[index]["ELECTRON"] += electron
            average[index]["LAT_FRONT-ELEC"] += latFrontElec      
            average[index]["SIZE"] += size
            average[index]["SIZE_WZ"] += sizeWZ        
            stdDeviation[index]["ELECTRON"].append(electron)
            stdDeviation[index]["LAT_FRONT-ELEC"].append(latFrontElec)
            stdDeviation[index]["SIZE"].append(size)
            stdDeviation[index]["SIZE_WZ"].append(sizeWZ)
        if c_processing:
            nanomsg = (float(lines[7]) - float(lines[6]))  
            latFrontNano = (float(lines[8]) - float(lines[2])) 
            latElecNano = (float(lines[8]) - float(lines[5])) 
            average[index]["NANOMSG"] += nanomsg 
            average[index]["LAT_FRONT-NANO"] += latFrontNano
            average[index]["LAT_ELEC-NANO"] += latElecNano
            stdDeviation[index]["NANOMSG"].append(nanomsg)
            stdDeviation[index]["LAT_FRONT-NANO"].append(latFrontNano)
            stdDeviation[index]["LAT_ELEC-NANO"].append(latElecNano)
        average[index]["NUMBER OF TEST"] += 1
for index in indexes: 
    c_processing = average[index]["C PROCESSING"]
    only_record = average[index]["ONLY RECORD"]
    average[index]["FRONTEND"] = round(average[index]["FRONTEND"] / average[index]["NUMBER OF TEST"],3)
    average[index]["STD_FRONTEND"] = numpy.std(stdDeviation[index]["FRONTEND"],dtype=numpy.float64)
    average[index]["SIZE"] = round(average[index]["SIZE"] / average[index]["NUMBER OF TEST"],3)
    average[index]["STD_SIZE"] = numpy.std(stdDeviation[index]["SIZE"])
    average[index]["TP_FRONTEND"] = round((average[index]["SIZE"] / average[index]["FRONTEND"])/1024.,3)    
    if not only_record:
        average[index]["ELECTRON"] = round(average[index]["ELECTRON"] / average[index]["NUMBER OF TEST"],3)
        average[index]["STD_ELECTRON"] = numpy.std(stdDeviation[index]["ELECTRON"],dtype=numpy.float64)
        average[index]["LAT_FRONT-ELEC"] = round(average[index]["LAT_FRONT-ELEC"] / average[index]["NUMBER OF TEST"],3)
        average[index]["STD_LAT_FRONT-ELEC"] = numpy.std(stdDeviation[index]["LAT_FRONT-ELEC"],dtype=numpy.float64)
        average[index]["SIZE_WZ"] = round(average[index]["SIZE_WZ"] / average[index]["NUMBER OF TEST"],3)
        average[index]["STD_SIZE_WZ"] = numpy.std(stdDeviation[index]["SIZE_WZ"])
        average[index]["TP_ELECTRON"] = round((average[index]["SIZE"] / average[index]["ELECTRON"])/1024.,3)
        average[index]["TP_WZ_FRONTEND"] = round((average[index]["SIZE_WZ"] / average[index]["FRONTEND"])/1024.,3)    
        average[index]["TP_WZ_ELECTRON"] = round((average[index]["SIZE_WZ"] / average[index]["ELECTRON"])/1024.,3)

    if c_processing:
        average[index]["NANOMSG"] = round(average[index]["NANOMSG"] / average[index]["NUMBER OF TEST"],3)
        average[index]["STD_NANOMSG"] = numpy.std(stdDeviation[index]["NANOMSG"],dtype=numpy.float64)
        average[index]["LAT_FRONT-NANO"] = round(average[index]["LAT_FRONT-NANO"] / average[index]["NUMBER OF TEST"],3)
        average[index]["STD_LAT_FRONT-NANO"] = numpy.std(stdDeviation[index]["LAT_FRONT-NANO"],dtype=numpy.float64)
        average[index]["LAT_ELEC-NANO"] = round(average[index]["LAT_ELEC-NANO"] / average[index]["NUMBER OF TEST"],3)
        average[index]["STD_LAT_ELEC-NANO"] = numpy.std(stdDeviation[index]["LAT_ELEC-NANO"],dtype=numpy.float64)
        average[index]["TP_NANOMSG"] = round((average[index]["SIZE"] / average[index]["NANOMSG"])/1024.,3)
        average[index]["TP_WZ_NANOMSG"] = round((average[index]["SIZE_WZ"] / average[index]["NANOMSG"])/1024.,3)    
        
for index in indexes:
    if os.path.isfile(rawCsvPath):
        with open(rawCsvPath, mode='a') as csv_file:
            writer = csv.DictWriter(csv_file,fieldnames=header)
            writer.writerow(average[index])
    else:
        with open(rawCsvPath, mode='w') as csv_file:
            writer = csv.DictWriter(csv_file,fieldnames=header)
            writer.writeheader()
            writer.writerow(average[index])
