#!/bin/bash
REAL=$1
if [[ $REAL == 1 ]]
then
    rm -rf benchmark_AMDFFT_real 2> /dev/null
    mkdir benchmark_AMDFFT_real  2> /dev/null
else
    rm -rf benchmark_AMDFFT 2> /dev/null
    mkdir benchmark_AMDFFT  2> /dev/null
fi
MODE=FILTER
for SIZE in 1024 8192
do
    for LENGTH in 5 20
    do
        #! 1 GPU 2 CPU
        for DEVICE in 0 1
        do
            D=CPU
            if [[ $DEVICE == 0 ]]
            then
                D=GPU
            fi 
            if [[ $REAL == 1 ]]
            then
                make processor_bench_real CHUNK_SIZE=$SIZE WORK_GROUP_SIZE=1 MODE=$MODE DEVICE=$DEVICE    
            else
                make processor_bench CHUNK_SIZE=$SIZE WORK_GROUP_SIZE=1 MODE=$MODE DEVICE=$DEVICE    
            fi 
            echo "Calentamiento" $SIZE $LENGTH $WORK_GROUP_SIZE $D
            ./generador.out $LENGTH.wav  &
            ./processing.out ipc
            sleep 1
            for I in {1..10}
            do 
                echo $I $SIZE $LENGTH $D
                if [[ $REAL == 1 ]]
                then
                    ./generador.out $LENGTH.wav benchmark_AMDFFT_real/S$I-$SIZE-$LENGTH-$D.txt &
                    ./processing.out ipc >> benchmark_AMDFFT_real/E$I-$SIZE-$LENGTH-$D.txt 
                    killall generador.out 2> /dev/null
                    cat benchmark_AMDFFT_real/S$I-$SIZE-$LENGTH-$D.txt
                    cat benchmark_AMDFFT_real/E$I-$SIZE-$LENGTH-$D.txt
                else
                    ./generador.out $LENGTH.wav benchmark_AMDFFT/S$I-$SIZE-$LENGTH-$D.txt &
                    ./processing.out ipc >> benchmark_AMDFFT/E$I-$SIZE-$LENGTH-$D.txt 
                    killall generador.out 2> /dev/null
                    cat benchmark_AMDFFT/S$I-$SIZE-$LENGTH-$D.txt
                    cat benchmark_AMDFFT/E$I-$SIZE-$LENGTH-$D.txt
                fi
                
                echo " "
                sleep 1
            done
        
        done
    done
done