#!/bin/bash
mkdir benchmark/benchmark_audios
mkdir benchmark/CSVs
mkdir benchmark/original_audios
mkdir benchmark/results_benchmark
make bench
ACTION=play
for NTEST in {1..5}
do
    TEST=benchRaul$NTEST$ACTION
    for TIME in 5 10 20
    do 
        for shift in  6 7 8 9 10
        do
            size=$(( 16 << $shift ))
            for MEASURE_OPTIONS in bars "" cprocessing cprocessing,bars onlyrecord onlyrecord,bars
            do
                if [[ $MEASURE_OPTIONS =~ cprocessing ]]
                then
                    for TECH in "tcp" "ws" "ipc" 
                    do
                        echo measure-$ACTION $TECH $TIME $size $MEASURE_OPTIONS $TEST
                        ./processorbench $TECH $TIME $size &
                        CHUNK_SIZE=$size START_TIME=$(date +%s) FILE=~/Desktop/diegog-vol3/node/benchmark/$TIME.wav TIME=$TIME TECH=$TECH ACTION=measure-$ACTION MEASURE_OPTIONS=$MEASURE_OPTIONS TEST=$TEST npm start
                        sleep 1
                        killall processorbench  2> /dev/null             
                        if [[ $ACTION =~ play ]]
                        then
                            diff ~/Desktop/diegog-vol3/node/benchmark/benchmark_audios/$TIME-$size-$TECH-$MEASURE_OPTIONS-$TEST.wav ~/Desktop/diegog-vol3/node/benchmark/$TIME-RAW.wav   
                        fi 
                    done
                else
                    TECH=""
                    echo measure-$ACTION $TIME $size $MEASURE_OPTIONS $TEST
                    CHUNK_SIZE=$size START_TIME=$(date +%s) FILE=~/Desktop/diegog-vol3/node/benchmark/$TIME.wav TIME=$TIME ACTION=measure-$ACTION MEASURE_OPTIONS=$MEASURE_OPTIONS TEST=$TEST npm start
                    if [[ $MEASURE_OPTIONS =~ onlyrecord ]]
                    then
                        echo ""
                    else
                        if [[ $ACTION =~ play ]]
                        then
                            diff ~/Desktop/diegog-vol3/node/benchmark/benchmark_audios/$TIME-$size-$TECH-$MEASURE_OPTIONS-$TEST.wav ~/Desktop/diegog-vol3/node/benchmark/$TIME-RAW.wav   
                        fi          
                    fi 
                fi      
            done  
        done
    done
done
# Check if the benchmark finished without problems
python3 benchmark/getRawCSV.py
# Gather all the data from the files in benchmark/results_benchmark
python3 benchmark/sizeVerification.py
mkdir benchmark/questions/images
python3 benchmark/questions/stats.py