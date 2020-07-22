#!/bin/bash
rm -rf benchmark_AMDFFT 2> /dev/null
mkdir benchmark_AMDFFT  2> /dev/null
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
            make ocl CHUNK_SIZE=$SIZE WORK_GROUP_SIZE=1 MODE=$MODE DEVICE=$DEVICE    
            echo "Calentamiento" $SIZE $LENGTH $WORK_GROUP_SIZE $D
            ./ocl.out ipc &
            CHUNK_SIZE=$SIZE START_TIME=$(date +%s) FILE=benchmark_comms/$LENGTH.wav TIME=$LENGTH  ACTION=measure-play MEASURE_OPTIONS=cprocessing,bars TEST=benchmark_AMDFFT/prueba.txt TECH=ipc  npm start 
            cat benchmark_AMDFFT/prueba.txt

            sleep 1
            for I in {1..10}
            do 
                echo $I $SIZE $LENGTH $D
                ./ocl.out ipc  >> benchmark_AMDFFT/E$I-$SIZE-$LENGTH-$D.txt &
                CHUNK_SIZE=$SIZE START_TIME=$(date +%s) FILE=benchmark_comms/$LENGTH.wav TIME=$LENGTH  ACTION=measure-play MEASURE_OPTIONS=cprocessing,bars TEST=benchmark_AMDFFT/S$I-$SIZE-$LENGTH-$D.txt  TECH=ipc  npm start 
                sleep 1 
                #killall ocl.out 2> /dev/null
                cat benchmark_AMDFFT/S$I-$SIZE-$LENGTH-$D.txt
                cat benchmark_AMDFFT/E$I-$SIZE-$LENGTH-$D.txt
                
                echo " "
                sleep 1
            done
        
        done
    done
done

cd benchmark_AMDFFT
python3 unify_data.py
python3 AMDFFT_best.py 
python3 AMDFFT_oh.py