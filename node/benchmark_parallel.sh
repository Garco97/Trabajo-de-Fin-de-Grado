#!/bin/bash
for SIZE in 16384 8192
do
    make build CHUNK_SIZE=$SIZE MODE=FILTER
    for TEST in seq data_static data_dynamic data_guided task_5 task_10 task_20 simdtask_5 simdtask_10 simdtask_20 simddata_static simddata_dynamic simddata_guided 
    do
        for FILE in 5 20
        do
            ./$TEST.out ipc $SIZE > /dev/null &
            sleep 1
            CHUNK_SIZE=$SIZE START_TIME=$(date +%s) FILE=~/Desktop/diegog-vol3/node/benchmark_comms/$FILE.wav TIME=$FILE  ACTION=play  npm start 
            echo "TYPE,LENGTH,SIZE,TOTAL_TIME_BEGIN,TOTAL_TIME_END,FIRST_CHUNK_BEGIN,FIRST_CHUNK_END,LAST_CHUNK_BEGIN,LAST_CHUNK_END" >> benchmark_parallel/par-bench/$TEST-$SIZE-$FILE.csv
            for TIME in {1..10}
            do  
                echo $TEST $SIZE $FILE $TIME
                sleep 1
                echo -n "$TEST,$FILE,$SIZE," >> benchmark_parallel/par-bench/$TEST-$SIZE-$FILE.csv
                ./$TEST.out ipc $SIZE >> benchmark_parallel/par-bench/$TEST-$SIZE-$FILE.csv &
                sleep 1
                CHUNK_SIZE=$SIZE START_TIME=$(date +%s) FILE=~/Desktop/diegog-vol3/node/benchmark_comms/$FILE.wav TIME=$FILE  ACTION=play  npm start
            done
        done
    done
done
sleep 1
make clean
cd benchmark_parallel
mkdir images
python3 parallel_times.py
python3 parallel_plots.py
