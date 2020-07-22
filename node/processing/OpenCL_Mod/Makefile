CC=gcc
CFLAGS=-Iinclude
DEPS = args.h bitmap.h gaussian.h
OBJ = gaussian.c args.c bitmap.c gaussian.c main.c
LIBS=-lm -lOpenCL -fopenmp

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

ocl-gaussian: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

processor:
	g++ processor.cpp ocl_processing.cpp -o processing.out -lnanomsg $(CFLAGS) $(LIBS) -O2 -D DEVICE=$(DEVICE) -D CHUNK_SIZE=$(CHUNK_SIZE) -D WORK_GROUP_SIZE=$(WORK_GROUP_SIZE) -D $(MODE) -D WINDOW_SIZE=$(WINDOW_SIZE) -D PERCENTAGE=$(PERCENTAGE) -D OCL -D MAX_BUFFERS=20 
	g++ read_wav.cpp -o generador.out -lnanomsg $(CFLAGS) $(LIBS) -O2 -D CHUNK_SIZE=$(CHUNK_SIZE)

processor_bench:
	g++ processor.cpp ocl_processing.cpp -o processing.out -lnanomsg $(CFLAGS) $(LIBS) -O2 -D DEVICE=$(DEVICE) -D CHUNK_SIZE=$(CHUNK_SIZE) -D WORK_GROUP_SIZE=$(WORK_GROUP_SIZE) -D $(MODE) -D OCL -D MAX_BUFFERS=20 
	g++ read_wav.cpp -o generador.out -lnanomsg $(CFLAGS) $(LIBS) -O2 -D CHUNK_SIZE=$(CHUNK_SIZE) 


processor_bench_real:
	g++ processor.cpp ocl_processing.cpp -o processing.out -lnanomsg $(CFLAGS) $(LIBS) -O2 -D DEVICE=$(DEVICE) -D CHUNK_SIZE=$(CHUNK_SIZE) -D WORK_GROUP_SIZE=$(WORK_GROUP_SIZE) -D $(MODE) -D OCL -D MAX_BUFFERS=20 
	g++ read_wav.cpp -o generador.out -lnanomsg $(CFLAGS) $(LIBS) -O2 -D CHUNK_SIZE=$(CHUNK_SIZE) -D REAL

