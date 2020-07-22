#!/bin/bash
rm -rf benchmark_thresh 2> /dev/null
mkdir benchmark_thresh  2> /dev/null
MODE=THRESHOLD
for SIZE in 8192 16384
do
    for LENGTH in 5
    do
        for WORK_GROUP_SIZE in 1 2 4 8 16 32 64 128 256
        do
            #! 1 GPU 2 CPU
            for DEVICE in 0 1
            do
                D=CPU
                if [[ $DEVICE == 0 ]]
                then
                    D=GPU
                fi 
                make processor_bench_real CHUNK_SIZE=$SIZE WORK_GROUP_SIZE=$WORK_GROUP_SIZE MODE=$MODE DEVICE=$DEVICE
                echo "Calentamiento" $SIZE $LENGTH $WORK_GROUP_SIZE $D
                ./generador.out $LENGTH.wav > /dev/null &
                ./processing.out ipc > /dev/null
                sleep 1
                for I in {1..10}
                do 
                    echo $I $SIZE $LENGTH $WORK_GROUP_SIZE $D
                    ./generador.out $LENGTH.wav benchmark_thresh/S$I-$SIZE-$LENGTH-$WORK_GROUP_SIZE-$D.txt &
                    ./processing.out ipc >> benchmark_thresh/E$I-$SIZE-$LENGTH-$WORK_GROUP_SIZE-$D.txt 
                    killall generador.out 2> /dev/null
                    cat benchmark_thresh/S$I-$SIZE-$LENGTH-$WORK_GROUP_SIZE-$D.txt
                    cat benchmark_thresh/E$I-$SIZE-$LENGTH-$WORK_GROUP_SIZE-$D.txt
                    echo " "
                    sleep 1
                done
            done
        done
    done
done