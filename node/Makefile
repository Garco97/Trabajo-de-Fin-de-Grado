
clean:
	rm seq.out file_to_stream.out data_static.out data_dynamic.out data_guided.out task_5.out task_10.out task_20.out simdtask_5.out simdtask_10.out simdtask_20.out simddata_static.out simddata_dynamic.out simddata_guided.out 

build:
	g++ processing/processor.cpp processing/sequential.cpp -o seq.out -fopenmp  -lnanomsg -lm -O2 -D SEQ -D MAX_BUFFERS=20 -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)

	g++ processing/processor.cpp processing/data_parallel.cpp -o data_static.out -fopenmp  -lnanomsg -lm -O2 -D DATA -D MAX_BUFFERS=20 -D OMP_FOR=static -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)
	g++ processing/processor.cpp processing/data_parallel.cpp -o data_dynamic.out -fopenmp  -lnanomsg -lm -O2 -D DATA -D MAX_BUFFERS=20 -D OMP_FOR=dynamic -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)
	g++ processing/processor.cpp processing/data_parallel.cpp -o data_guided.out -fopenmp  -lnanomsg -lm -O2 -D DATA -D MAX_BUFFERS=20 -D OMP_FOR=guided -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)
	
	g++ processing/processor.cpp processing/task_parallel.cpp  -o task_5.out -fopenmp  -lnanomsg -lm -O2 -D TASK -D MAX_BUFFERS=5 -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)
	g++ processing/processor.cpp processing/task_parallel.cpp  -o task_10.out -fopenmp  -lnanomsg -lm -O2 -D TASK -D MAX_BUFFERS=10 -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)
	g++ processing/processor.cpp processing/task_parallel.cpp  -o task_20.out -fopenmp  -lnanomsg -lm -O2 -D TASK -D MAX_BUFFERS=20 -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)

	g++ processing/processor.cpp processing/simd_task_parallel.cpp  -o simdtask_5.out -fopenmp  -lnanomsg -lm -O2 -D TASK_SIMD -D MAX_BUFFERS=20  -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)
	g++ processing/processor.cpp processing/simd_task_parallel.cpp  -o simdtask_10.out -fopenmp  -lnanomsg -lm -O2 -D TASK_SIMD -D MAX_BUFFERS=10  -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)
	g++ processing/processor.cpp processing/simd_task_parallel.cpp  -o simdtask_20.out -fopenmp  -lnanomsg -lm -O2 -D TASK_SIMD -D MAX_BUFFERS=5  -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)

	g++ processing/processor.cpp processing/simd_data_parallel.cpp  -o simddata_static.out -fopenmp  -lnanomsg -lm -O2 -D DATA_SIMD -D MAX_BUFFERS=20 -D OMP_FOR=static -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)
	g++ processing/processor.cpp processing/simd_data_parallel.cpp  -o simddata_dynamic.out -fopenmp  -lnanomsg -lm -O2 -D DATA_SIMD -D MAX_BUFFERS=20 -D OMP_FOR=dynamic -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)
	g++ processing/processor.cpp processing/simd_data_parallel.cpp  -o simddata_guided.out -fopenmp  -lnanomsg -lm -O2 -D DATA_SIMD -D MAX_BUFFERS=20 -D OMP_FOR=guided  -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)

seq:
	g++ processing/processor.cpp processing/sequential.cpp -o seq.out -fopenmp  -lnanomsg -lm -O2 -D SEQ -D MAX_BUFFERS=20 -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)
task:
	g++ processing/processor.cpp processing/task_parallel.cpp  -o task_20.out -fopenmp  -lnanomsg -lm -O2 -D TASK -D MAX_BUFFERS=20 -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)
data:
	g++ processing/processor.cpp processing/data_parallel.cpp -o data_guided.out -fopenmp  -lnanomsg -lm -O2 -D DATA -D MAX_BUFFERS=20 -D OMP_FOR=static -D OMP_SIZE=1 -D ${MODE} -D CHUNK_SIZE=$(CHUNK_SIZE)
ocl:
	g++ processing/processor.cpp processing/ocl_processing.cpp -o ocl.out -lnanomsg -Iinclude -lm -lOpenCL -fopenmp -O2 -D DEVICE=$(DEVICE) -D CHUNK_SIZE=$(CHUNK_SIZE) -D WORK_GROUP_SIZE=$(WORK_GROUP_SIZE) -D $(MODE) -D OCL -D MAX_BUFFERS=20 


bench: 	
	g++ processing/processorbench.cpp -lnanomsg -lm -o processorbench.out


minimum:
	g++ processing/programa_minimo.cpp -o minimo.out -fopenmp -lm -O2

minimum_debug: 
	g++ processing/programa_minimo.cpp -o minimo.out -fopenmp -lm -O2 -D DEBUG

data_reader:
	g++ processing/file_to_stream.cpp  -o file_to_stream.out    