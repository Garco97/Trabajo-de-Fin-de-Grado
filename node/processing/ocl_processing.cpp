
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <complex.h>
#include <tgmath.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "operations.h"
#include "common.h"
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
//#define DEBUG
#include <math.h>

#define MAX_SOURCE_SIZE (1048576) //1 MB
#define MAX_LOG_SIZE    (1048576) //1 MB

int counter = 0;
std::thread my_thread;
std::mutex m;
std::condition_variable cond_var;
//! Parametro a incializar 
cl_int ret; 
//!Común
char* kernelSource;
size_t kernelSrcSize;
//Get platform and device information
cl_platform_id platformID[2];//will hold the ID of the openCL available platform
cl_uint platformsN;//will hold the number of openCL available platforms on the machine
cl_device_id deviceID;//will hold the ID of the openCL device
cl_uint devicesN; //will hold the number of OpenCL devices in the system
cl_context context;
cl_command_queue cmdQueue;
cl_program program;
cl_kernel kernel;
cl_platform_id* platforms;
 cl_uint deviceCount;
cl_device_id* devices;
//! FFT
float i_real[CHUNK_SIZE*4];
float i_imag[CHUNK_SIZE*4];
int max_sample_size = 1024;
int sample_size = CHUNK_SIZE;
int n_chunks = sample_size/max_sample_size;
cl_mem entrada_reales;
cl_mem entrada_imag;
cl_command_queue cmdQueue2;
cl_command_queue cmdQueue3;
//! THRESHOLD
cl_mem entrada_chunk;
cl_mem entrada_res;
int res[CHUNK_SIZE];
//! THRESHOLD VENTANA
#ifdef WINDOW_SIZE
cl_context context_thresh;
cl_command_queue cmdQueue_thresh;
cl_program program_thresh;
cl_kernel kernel_thresh;
cl_mem entrada_values;
cl_mem entrada_peaks;
#endif

cl_event k_events[2];

size_t globalWorkItemSize;//the total size of 1 dimension of the work items.
size_t workGroupSize; //The size of one work group


struct timeval start_pass_args;
struct timeval end_pass_args;
struct timeval start_get_args;
struct timeval end_get_args;
u_int64_t time_start_pass_args;
u_int64_t time_end_pass_args;
u_int64_t time_start_get_args;
u_int64_t time_end_get_args;
bool finish;
bool notified;
#ifdef FILTER
void manage_chunks(){
    while(!finish){
        std::unique_lock<std::mutex> lock(m);
        cond_var.wait(lock, []{return notified;}); 
        for(int i=0; i<MAX_BUFFERS; i++){
            if(status_ptr[i] == 2){
                filter((float*)ptr[i].buf,CHUNK_SIZE*4,audio + ptr[i].current_size/4,filtered_low + ptr[i].current_size/4,filtered_high + ptr[i].current_size/4,0.0, 0.0);
                status_ptr[i] = 0;
                buffs_ready --;
            }
        }
    }
}

void prepare_filter(){
    globalWorkItemSize = 64;
    workGroupSize = 64;
    FILE* f;
    if((f = fopen("processing/FFT_Kernels.cl", "r")) == NULL)
    {
        fprintf(stderr, "Failed to load OpenCL kernel code.\n");
        return;
    }
    kernelSource = (char*) malloc(MAX_SOURCE_SIZE);
    kernelSrcSize = fread(kernelSource, 1, MAX_SOURCE_SIZE, f);
    fclose(f);

    clGetPlatformIDs(0, NULL, &platformsN);
    platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformsN);
    clGetPlatformIDs(platformsN, platforms, NULL);

    for (int i = 0; i < platformsN; i++) {

        // get all devices
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
        devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);
        if(i == DEVICE){
            deviceID = devices[0];
        }
    }        
    char* value;

    size_t valueSize;
    clGetDeviceInfo(deviceID, CL_DEVICE_NAME, 0, NULL, &valueSize);
    value = (char*) malloc(valueSize);
    clGetDeviceInfo(deviceID, CL_DEVICE_NAME, valueSize, value, NULL);
    //printf("%d. Device: %s\n", DEVICE, value);
    free(value);


    context = clCreateContext( NULL, 1, &deviceID, NULL, NULL, &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Could not create a valid OpenCL context\n");
        return;
    }
    cmdQueue = clCreateCommandQueue(context, deviceID, 0, &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Could not create an OpenCL Command Queue\n");
        return;
    }
    cmdQueue2 = clCreateCommandQueue(context, deviceID, 0, &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Could not create an OpenCL Command Queue\n");
        return;
    }
    cmdQueue3 = clCreateCommandQueue(context, deviceID, 0, &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Could not create an OpenCL Command Queue\n");
        return;
    }
    entrada_reales = clCreateBuffer(context,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,max_sample_size * sizeof(float),NULL,&ret );

    if(ret != CL_SUCCESS)
    {
        printf("Unable to create the GPU chunk buffer object\n");
        return;
    }
    
    entrada_imag = clCreateBuffer(context,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,max_sample_size * sizeof(float),NULL,&ret);
    if(ret != CL_SUCCESS)
    {
        printf("Unable to create the GPU entrada_imag object\n");
        return;
    } 

    //Copy the image data and the gaussian kernel to the memory buffer
    //Create a program object and associate it with the kernel's source code.
    program = clCreateProgramWithSource(context, 1,(const char **)&kernelSource, (const size_t *)&kernelSrcSize, &ret);
    free(kernelSource);
    if(ret != CL_SUCCESS)
    {
        printf("Error in creating an OpenCL program object\n");
        return;
    }
    //Build the created OpenCL program
    if((ret = clBuildProgram(program, 1, &deviceID, NULL , NULL, NULL))!= CL_SUCCESS)
    {
        printf("Failed to build the OpenCL program %d\n",ret);
        //create the log string and show it to the user. Then quit
        char* buildLog;
        buildLog =(char*) malloc(MAX_LOG_SIZE);
        if(clGetProgramBuildInfo(program,deviceID,CL_PROGRAM_BUILD_LOG,MAX_LOG_SIZE,buildLog,NULL) != CL_SUCCESS)
        {
            printf("Could not get any Build info from OpenCL\n");
            free(buildLog);
            return;
        }
        printf("**BUILD LOG**\n%s",buildLog);
        free(buildLog);
        return;
    }
    // Create the OpenCL kernel. This is basically one function of the program declared with the __kernel qualifier
    kernel = clCreateKernel(program, "kfft", &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Failed to create the OpenCL Kernel from the built program\n");
        return;
    }

    //! Inicializo el thread
    //my_thread = std::thread(manage_chunks);

}
#endif
void free_resources(){
    clFlush(cmdQueue);
    clFinish(cmdQueue);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseMemObject(entrada_imag); 
    clReleaseMemObject(entrada_reales);
    clReleaseCommandQueue(cmdQueue);
    clReleaseContext(context);
    #ifdef FILTER
    clFlush(cmdQueue2);
    clFinish(cmdQueue2);
    clFlush(cmdQueue3);
    clFinish(cmdQueue3);
    clReleaseCommandQueue(cmdQueue2);
    clReleaseCommandQueue(cmdQueue3);
    /* {
        std::unique_lock<std::mutex> lock(m);
        finish = true;
        notified = true;
        cond_var.notify_all(); 
    }
    my_thread.join(); */
    #endif
    #ifdef WINDOW_SIZE
    clFlush(cmdQueue_thresh);
    clFinish(cmdQueue_thresh);
    clReleaseKernel(kernel_thresh);
    clReleaseProgram(program_thresh);
    clReleaseMemObject(entrada_values); 
    clReleaseMemObject(entrada_peaks);
    clReleaseCommandQueue(cmdQueue_thresh);
    clReleaseContext(context_thresh);
    #endif

}

void filter(float* audio, unsigned long audio_size, float* out_original, float* out_real, float* out_imag, double time_interval, double time_constant )
{
    for(int i = 0; i<sample_size; i++){
        i_real[i] = audio[i];//real(z1);
        i_imag[i] = 0.0;//imag(z1);
    }     
    u_int64_t total_time;
    u_int64_t pass_args_time;
    u_int64_t get_args_time;
    u_int64_t kernel_time;
    for(int i = 0; i<n_chunks ; i++){
        #ifdef DEBUG_P
        gettimeofday(&start_pass_args, NULL);
        time_start_pass_args = (start_pass_args.tv_sec * 1000000 + start_pass_args.tv_usec);
        printf("Escribe, %i, %i, %lu \n",counter,i,time_start_pass_args/1000);
        #endif
        if(clEnqueueWriteBuffer(cmdQueue, entrada_reales, CL_TRUE, 0,max_sample_size*sizeof(float),i_real + i*max_sample_size, 0, NULL, NULL) != CL_SUCCESS)
        {
            printf("Error during sending the i real data to the OpenCL buffer\n");
            return;
        }
                

        if(clEnqueueWriteBuffer(cmdQueue, entrada_imag, CL_TRUE, 0,max_sample_size*sizeof(float),i_imag + i*max_sample_size, 0, NULL, NULL) != CL_SUCCESS)
        {
            printf("Error during sending the i imag data to the OpenCL buffer\n");
            return;
        }

        if(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&entrada_reales) != CL_SUCCESS)
        {
            printf("Could not set the kernel's \"entrada_reales\" argument\n");
            return;
        }
        if(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&entrada_imag) != CL_SUCCESS)
        {
            printf("Could not set the kernel's \"entrada_imag\" argument\n");
            return;
        }
        gettimeofday(&end_pass_args, NULL);
        

        ret = clEnqueueNDRangeKernel(cmdQueue2, kernel, 1, NULL, &globalWorkItemSize, &workGroupSize, 0, NULL,NULL);
        if(ret){
            printf("NDRANGE %d\n", ret);
        }

        gettimeofday(&start_get_args, NULL);
        

        ret = clEnqueueReadBuffer(cmdQueue3, entrada_reales, CL_TRUE, 0,max_sample_size*sizeof(float) , out_real+ i*max_sample_size, 0,NULL, NULL); 
        if(ret){
            printf("error reales %d\n", ret);
        }
                

        ret = clEnqueueReadBuffer(cmdQueue3, entrada_imag, CL_TRUE, 0,max_sample_size*sizeof(float) , out_imag+ i*max_sample_size,0, NULL,NULL); 
        if(ret){
            printf("error imaginario %d\n", ret);
        }  
        gettimeofday(&end_get_args, NULL);
        #ifdef DEBUG_P
        time_end_get_args = (end_get_args.tv_sec * 1000000 + end_get_args.tv_usec);
        printf("Acaba leer,%i , %i, %lu \n",counter,i,time_end_get_args/1000);
        time_end_pass_args = (end_pass_args.tv_sec * 1000000 + end_pass_args.tv_usec);
        time_start_get_args = (start_get_args.tv_sec * 1000000 + start_get_args.tv_usec);
        total_time = time_end_get_args - time_start_pass_args;
        pass_args_time = time_end_pass_args - time_start_pass_args;
        get_args_time = time_end_get_args - time_start_get_args;
        kernel_time = time_start_get_args - time_end_pass_args;
        printf("%d %d: total %lu pass %lu kernel %lu get %lu\n",counter,i,total_time, pass_args_time, kernel_time,get_args_time); 
        #endif
    }
        counter ++;
}

void prepare_threshold(){
    FILE* f;
    if((f = fopen("processing/threshold.cl", "r")) == NULL)
    {
        fprintf(stderr, "Failed to load OpenCL kernel code.\n");
        return;
    }
    kernelSource = (char*) malloc(MAX_SOURCE_SIZE);
    kernelSrcSize = fread(kernelSource, 1, MAX_SOURCE_SIZE, f);
    fclose(f);
    clGetPlatformIDs(0, NULL, &platformsN);
    platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformsN);
    clGetPlatformIDs(platformsN, platforms, NULL);

    for (int i = 0; i < platformsN; i++) {

        // get all devices
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
        devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);
        if(i == DEVICE){
            deviceID = devices[0];
        }
    }        
    char* value;

    size_t valueSize;
    clGetDeviceInfo(deviceID, CL_DEVICE_NAME, 0, NULL, &valueSize);
    value = (char*) malloc(valueSize);
    clGetDeviceInfo(deviceID, CL_DEVICE_NAME, valueSize, value, NULL);
    //printf("%d. Device: %s\n", DEVICE, value);
    free(value);
    context = clCreateContext( NULL, 1, &deviceID, NULL, NULL, &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Could not create a valid OpenCL context\n");
        return;
    }
    cmdQueue = clCreateCommandQueue(context, deviceID, 0, &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Could not create an OpenCL Command Queue\n");
        return;
    }
    cmdQueue2 = clCreateCommandQueue(context, deviceID, 0, &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Could not create an OpenCL Command Queue\n");
        return;
    }
    cmdQueue3 = clCreateCommandQueue(context, deviceID, 0, &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Could not create an OpenCL Command Queue\n");
        return;
    }
    entrada_chunk = clCreateBuffer(context,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,CHUNK_SIZE * sizeof(float),NULL,&ret );

    if(ret != CL_SUCCESS)
    {
        printf("Unable to create the GPU chunk buffer object\n");
        return;
    }
    
    entrada_res = clCreateBuffer(context,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,CHUNK_SIZE * sizeof(int),NULL,&ret);
    if(ret != CL_SUCCESS)
    {
        printf("Unable to create the GPU entrada_res object\n");
        return;
    } 

    //Copy the image data and the gaussian kernel to the memory buffer
    //Create a program object and associate it with the kernel's source code.
    program = clCreateProgramWithSource(context, 1,(const char **)&kernelSource, (const size_t *)&kernelSrcSize, &ret);
    free(kernelSource);
    if(ret != CL_SUCCESS)
    {
        printf("Error in creating an OpenCL program object\n");
        return;
    }
    //Build the created OpenCL program
    if((ret = clBuildProgram(program, 1, &deviceID, NULL , NULL, NULL))!= CL_SUCCESS)
    {
        printf("Failed to build the OpenCL program %d\n",ret);
        //create the log string and show it to the user. Then quit
        char* buildLog;
        buildLog =(char*) malloc(MAX_LOG_SIZE);
        if(clGetProgramBuildInfo(program,deviceID,CL_PROGRAM_BUILD_LOG,MAX_LOG_SIZE,buildLog,NULL) != CL_SUCCESS)
        {
            printf("Could not get any Build info from OpenCL\n");
            free(buildLog);
            return;
        }
        printf("**BUILD LOG**\n%s",buildLog);
        free(buildLog);
        return;
    }
    // Create the OpenCL kernel. This is basically one function of the program declared with the __kernel qualifier
    kernel = clCreateKernel(program, "threshold", &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Failed to create the OpenCL Kernel from the built program\n");
        return;
    }
    #ifdef WINDOW_SIZE
   
    FILE* f1;
    if((f1 = fopen("processing/threshold.cl", "r")) == NULL)
    {
        fprintf(stderr, "Failed to load OpenCL kernel code.\n");
        return;
    }
    kernelSource = (char*) malloc(MAX_SOURCE_SIZE);
    kernelSrcSize = fread(kernelSource, 1, MAX_SOURCE_SIZE, f1);
    fclose(f1);
    
    context_thresh = clCreateContext( NULL, 1, &deviceID, NULL, NULL, &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Could not create a valid OpenCL context_thresh\n");
        return;
    }
    cmdQueue_thresh = clCreateCommandQueue(context_thresh, deviceID, 0, &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Could not create an OpenCL Command Queue\n");
        return;
    }
    entrada_values = clCreateBuffer(context_thresh,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,CHUNK_SIZE * sizeof(int),NULL,&ret );

    if(ret != CL_SUCCESS)
    {
        printf("Unable to create the GPU chunk buffer object\n");
        return;
    }
    entrada_peaks = clCreateBuffer(context_thresh,CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,CHUNK_SIZE/WINDOW_SIZE * sizeof(int),NULL,&ret);
    if(ret != CL_SUCCESS)
    {
        printf("Unable to create the GPU entrada_peaks object\n");
        return;
    } 

    //Copy the image data and the gaussian kernel_thresh to the memory buffer
    //Create a program_thresh object and associate it with the kernel_thresh's source code.
    program_thresh = clCreateProgramWithSource(context_thresh, 1,(const char **)&kernelSource, (const size_t *)&kernelSrcSize, &ret);
    free(kernelSource);
    if(ret != CL_SUCCESS)
    {
        printf("Error in creating an OpenCL program_thresh object\n");
        return;
    }
    //Build the created OpenCL program_thresh
    if((ret = clBuildProgram(program_thresh, 1, &deviceID, NULL , NULL, NULL))!= CL_SUCCESS)
    {
        printf("Failed to build the OpenCL program_thresh %d\n",ret);
        //create the log string and show it to the user. Then quit
        char* buildLog;
        buildLog =(char*) malloc(MAX_LOG_SIZE);
        if(clGetProgramBuildInfo(program_thresh,deviceID,CL_PROGRAM_BUILD_LOG,MAX_LOG_SIZE,buildLog,NULL) != CL_SUCCESS)
        {
            printf("Could not get any Build info from OpenCL\n");
            free(buildLog);
            return;
        }
        printf("**BUILD LOG**\n%s",buildLog);
        free(buildLog);
        return;
    }
    // Create the OpenCL kernel_thresh. This is basically one function of the program_thresh declared with the __kernel qualifier
    kernel_thresh = clCreateKernel(program_thresh, "peak", &ret);
    if(ret != CL_SUCCESS)
    {
        printf("Failed to create the OpenCL Kernel from the built program\n");
        return;
    }
    #endif
}
void  new_analyse_threshold(float* buffer, int n_samples_chunk, int socket, int index_chunk, Window *window){
    workGroupSize = WORK_GROUP_SIZE;
    globalWorkItemSize = CHUNK_SIZE;
    #ifdef DEBUG
    printf(" %d\n",counter);
    #endif    
    if(clEnqueueWriteBuffer(cmdQueue, entrada_chunk, CL_TRUE, 0,CHUNK_SIZE*sizeof(float),buffer, 0, NULL, NULL) != CL_SUCCESS)
    {
        printf("Error during sending the chunk data to the OpenCL buffer\n");
        return;
    }
            

    if(clEnqueueWriteBuffer(cmdQueue, entrada_res, CL_TRUE, 0,CHUNK_SIZE*sizeof(int),res, 0, NULL, NULL) != CL_SUCCESS)
    {
        printf("Error during sending the res data to the OpenCL buffer\n");
        return;
    }

    if(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&entrada_chunk) != CL_SUCCESS)
    {
        printf("Could not set the kernel's \"entrada_chunk\" argument\n");
        return;
    }
    if(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&entrada_res) != CL_SUCCESS)
    {
        printf("Could not set the kernel's \"entrada_res\" argument\n");
        return;
    }
    if(clSetKernelArg(kernel, 2, sizeof(int), (void *)&window -> value_threshold) != CL_SUCCESS)
    {
        printf("Could not set the kernel's \"value\" argument\n");
        return;
    }
    
    ret = clEnqueueNDRangeKernel(cmdQueue2, kernel, 1, NULL, &globalWorkItemSize, &workGroupSize, 0, NULL,NULL);
    if(ret){
        printf("NDRANGE %d\n", ret);
    }
    ret = clEnqueueReadBuffer(cmdQueue3, entrada_res, CL_TRUE, 0,CHUNK_SIZE*sizeof(int) , res,0, NULL,NULL); 
    if(ret){
        printf("error imaginario %d\n", ret);
    }  
    #ifdef DEBUG
    for(int i=0; i<CHUNK_SIZE; i++){
        if(res[i] ){
            printf("res %d  i %d value %d\n",res[i],i, window->value_threshold);
        }
    } 
    #endif
    #ifdef WINDOW_SIZE
    workGroupSize = 1;
    globalWorkItemSize = CHUNK_SIZE/WINDOW_SIZE;
    int iter = WINDOW_SIZE;
    int minimum_number = WINDOW_SIZE*PERCENTAGE/100;
    int peaks[globalWorkItemSize];

    if(clEnqueueWriteBuffer(cmdQueue_thresh, entrada_values, CL_TRUE, 0,CHUNK_SIZE*sizeof(int),res, 0, NULL, NULL) != CL_SUCCESS)
    {
        printf("Error during sending the entrada_values to the OpenCL buffer\n");
        return;
    }

    if(clEnqueueWriteBuffer(cmdQueue_thresh, entrada_peaks, CL_TRUE, 0,globalWorkItemSize*sizeof(int),peaks, 0, NULL, NULL) != CL_SUCCESS)
    {
        printf("Error during sending the entrada_peaks to the OpenCL buffer\n");
        return;
    }

    if(clSetKernelArg(kernel_thresh, 0, sizeof(cl_mem), (void *)&entrada_values) != CL_SUCCESS)
    {
        printf("Could not set the kernel's \"entrada_values\" argument\n");
        return;
    }
    if(clSetKernelArg(kernel_thresh, 1, sizeof(cl_mem), (void *)&entrada_peaks) != CL_SUCCESS)
    {
        printf("Could not set the kernel's \"entrada_peaks\" argument\n");
        return;
    }
    if(clSetKernelArg(kernel_thresh, 2, sizeof(int), (void *)&iter) != CL_SUCCESS)
    {
        printf("Could not set the kernel's \"size\" argument\n");
        return;
    }
    if(clSetKernelArg(kernel_thresh, 3, sizeof(int), (void *)&minimum_number) != CL_SUCCESS)
    {
        printf("Could not set the kernel's \"minimum_number\" argument\n");
        return;
    }
    ret = clEnqueueNDRangeKernel(cmdQueue_thresh, kernel_thresh, 1, NULL, &globalWorkItemSize, &workGroupSize, 0, NULL,NULL);
    if(ret){
        printf("NDRANGE %d\n", ret);
    }
    ret = clEnqueueReadBuffer(cmdQueue_thresh, entrada_peaks, CL_TRUE, 0,globalWorkItemSize*sizeof(int) , peaks,0, NULL,NULL); 
    if(ret){
        printf("error leyendo picos de salida %d\n", ret);
    }  
    #ifdef DEBUG
    printf("%d\n", index_chunk);
    for(int i=0; i<globalWorkItemSize; i++){
        if(peaks[i] ){
            printf("NOTIFICACIÓN en chunk %d entre las muestras %d-%d\n",index_chunk,i*WINDOW_SIZE,(i+1)*WINDOW_SIZE-1);
        }
    }  
    #endif
    #endif
    
}

