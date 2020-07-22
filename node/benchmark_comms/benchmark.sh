#!/bin/bash
ACTION=$1
TIME=$2
TEST=$3

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
                ./processorbench.out $TECH $size &
                CHUNK_SIZE=$size START_TIME=$(date +%s) FILE=benchmark_comms/$TIME.wav TIME=$TIME TECH=$TECH ACTION=measure-$ACTION MEASURE_OPTIONS=$MEASURE_OPTIONS TEST=$TEST npm start
                sleep 1
                killall processorbench.out 2> /dev/null                    
                if [[ $ACTION =~ play ]]
                then
                    pwd
                    diff benchmark_comms/benchmark_audios/$TIME-$size-$TECH-$MEASURE_OPTIONS-$TEST.wav benchmark_comms/$TIME-RAW.wav   
                fi 
            done
        else
            TECH=""
            echo measure-$ACTION $TIME $size $MEASURE_OPTIONS $TEST
            CHUNK_SIZE=$size START_TIME=$(date +%s) FILE=benchmark_comms/$TIME.wav TIME=$TIME ACTION=measure-$ACTION MEASURE_OPTIONS=$MEASURE_OPTIONS TEST=$TEST npm start
            if [[ $MEASURE_OPTIONS =~ onlyrecord ]]
            then
                echo ""
            else
                if [[ $ACTION =~ play ]]
                then
                pwd
                    diff benchmark_comms/benchmark_audios/$TIME-$size-$TECH-$MEASURE_OPTIONS-$TEST.wav benchmark_comms/$TIME-RAW.wav   
                fi          
            fi 
        fi      
    done  
done





