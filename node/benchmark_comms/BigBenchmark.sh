#!/bin/bash
#What do you want to measure? 'play' or 'record'?
make bench
mkdir benchmark_comms/benchmark_audios 2> /dev/null
mkdir benchmark_comms/original_audios 2> /dev/null
mkdir benchmark_comms/results_benchmark 2> /dev/null

ACTION=$1
MIC=""
if [[ $ACTION == record ]]
then 
    MIC=$2
fi
for TIME in 5 10 20
do
    ./benchmark_comms/benchmark.sh $ACTION $TIME calentamiento$ACTION$MIC
    if [[ $TIME == 5 ]]
    then
        TIMES=10
    else 
        if [[ $TIME == 10 ]]
        then
            TIMES=8
        else
            TIMES=5
        fi
    fi
    for TEST in {1..$TIMES}
    do
        ./benchmark_comms/benchmark.sh $ACTION $TIME bench$TEST$ACTION$MIC
    done
done
# Check if the benchmark finished without problems
python3 benchmark_comms/sizeVerification.py
# Gather all the data from the files in benchmark_comms/results_benchmark
rm benchmark_comms/CSVs/raw_results.csv 
python3 benchmark_comms/getRawCSV.py
python3 benchmark_comms/questions/stats.py 
