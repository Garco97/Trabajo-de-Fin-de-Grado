#include <bitmap.h>
#include <gaussian.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "save_wav.cpp"
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <math.h>

#define PI_ 3.14159265359f

#define MAX_SOURCE_SIZE (1048576) //1 MB
#define MAX_LOG_SIZE    (1048576) //1 MB


float* get_chunk(float* buffer, int chunk_size, float* chunk);
//creates a gaussian kernel
float* createGaussianKernel(uint32_t size,float sigma)
{
    float* ret;
    uint32_t x,y;
    double center = size/2;
    float sum = 0;
    //allocate and create the gaussian kernel
    ret = malloc(sizeof(float) * size * size);
    for(x = 0; x < size; x++)
    {
        for(y=0; y < size; y++)
        {
            ret[ y*size+x] = exp( (((x-center)*(x-center)+(y-center)*(y-center))/(2.0f*sigma*sigma))*-1.0f ) / (2.0f*PI_*sigma*sigma);
            sum+=ret[ y*size+x];
        }
    }
    //normalize
    for(x = 0; x < size*size;x++)
    {
        ret[x] = ret[x]/sum;
    }
    //print the kernel so the user can see it
    printf("The generated Gaussian Kernel is:\n");
    for(x = 0; x < size; x++)
    {
        for(y=0; y < size; y++)
        {
            printf("%f ",ret[ y*size+x]);
        }
        printf("\n");
    }
    printf("\n\n");
    return ret;
}

//Blurs the given image using the CPU algorithm
char pna_blur_cpu(char* imgname,uint32_t size,float sigma)
{
    uint32_t i,x,y,imgLineSize;
    int32_t center,yOff,xOff;
    float* matrix,value;
    matrix = createGaussianKernel(size,sigma);
    //read the bitmap
    ME_ImageBMP bmp;
    if(meImageBMP_Init(&bmp,imgname)==false)
    {
        printf("Image \"%s\" could not be read as a .BMP file\n",imgname);
        return false;
    }
    //find the size of one line of the image in bytes and the center of the gaussian kernel
    imgLineSize = bmp.imgWidth*3;
    center = size/2;
    //convolve all valid pixels with the gaussian kernel
    for(i = imgLineSize*(size-center)+center*3; i < bmp.imgHeight*bmp.imgWidth*3-imgLineSize*(size-center)-center*3;i++)
    {
        value = 0;
        for(y=0;y<size;y++)
        {
            yOff = imgLineSize*(y-center);
            for(x=0;x<size;x++)
            {
                xOff = 3*(x - center);
                value += matrix[y*size+x]*bmp.imgData[i+xOff+yOff];
            }
        }
        bmp.imgData[i] = value;
    }
    //free memory and save the image
    free(matrix);
    meImageBMP_Save(&bmp,"cpu_blur.bmp");
    return true;
}



int read_file(char* audio){
    FILE* file;
    int index = 0;
    char *buffer_reader;
    file = fopen("5.wav", "r");
    if(file == NULL ){   
        fprintf(stderr, "%s\n", "Error abriendo el archivo");
        exit(1);
    }

    printf("Fichero abierto \n");
    // Realizo la lectura del fichero
    int auxiliar = 0;
    int size_file = 8192;
    //* Realizo la lectura del fichero
    while(fread(buffer_reader, 1,1,file)){
        if (auxiliar >=44){
            audio[index] = *buffer_reader;
            index ++;
            if (index == size_file){
                size_file = size_file*2;
                audio = (char*) realloc(audio,size_file * sizeof(char));
            }
        }else{
            auxiliar ++;
        }
    }
    fclose(file);
    return index;
}
//Blurs the given image using the GPU
char pna_blur_gpu(char* imgname,uint32_t size,float sigma)
{
    int chunk_real_size;
    float* matrix;
    cl_int ret;//the openCL error code/s
    //get the image
    ME_ImageBMP bmp;
    meImageBMP_Init(&bmp,imgname);
    //create the gaussian kernel
    matrix = createGaussianKernel(size,sigma);
    //create the pointer that will hold the new (blurred) image data
    
    // Read in the kernel code into a c string
    FILE* f;
    char* kernelSource;
    size_t kernelSrcSize;
    if( (f = fopen("kernel.cl", "r")) == NULL)
    {
        fprintf(stderr, "Failed to load OpenCL kernel code.\n");
        return false;
    }
    kernelSource = malloc(MAX_SOURCE_SIZE);
    kernelSrcSize = fread( kernelSource, 1, MAX_SOURCE_SIZE, f);
    fclose(f);

    //Get platform and device information
    cl_platform_id platformID[2];//will hold the ID of the openCL available platform
    cl_uint platformsN;//will hold the number of openCL available platforms on the machine
    cl_device_id deviceID;//will hold the ID of the openCL device
    cl_uint devicesN; //will hold the number of OpenCL devices in the system
    if(clGetPlatformIDs(2, NULL, &platformsN) != CL_SUCCESS)
    {
        printf("Could not get num of OpenCL Platforms\n");
        return false;
    }
    printf("%d\n", platformsN);
    for (int i=0; i<1; ++i){
      if(clGetPlatformIDs(1, &platformID[i], 0) != CL_SUCCESS)
      {
          printf("Could not get the OpenCL Platform IDs %d\n", i);
          return false;
      }
      const int deviceId = 0;
      // if(clGetDeviceIDs(platformID, deviceIdCL_DEVICE_TYPE_DEFAULT, 1,&deviceID, &devicesN) != CL_SUCCESS)
      if(clGetDeviceIDs(platformID[i], CL_DEVICE_TYPE_ALL, 1, &deviceID, 0) != CL_SUCCESS)
      {
          printf("Could not get the system's OpenCL device %d for platform %d\n", deviceId, i);
          return false;
      }
    }
    printf("Empiezo la lectura del fichero\n");
    //! Lectura del fichero
    
    char* audio = (char*) malloc(8192 * sizeof(char));

    int index = read_file(audio);
    
    printf("Acabo la lectura del fichero\n");
    // Cierro el fichero
    for (int i = 0; i< index; i++){
        printf("%x %d\n",audio[i],index);
    }


    int chunk_size = 8192;
    float sample_rate = 44100;
    float time = chunk_size / sample_rate;
    int num_samples = index / 4;
    int num_chunks = ceil(num_samples/chunk_size);
    float* newData;
    // Create an OpenCL context
    cl_context context;
    cl_command_queue cmdQueue;
    float *chunk = (float*) malloc(sizeof(float) * chunk_size);

    //! Bucle donde empieza a enviarse los chunks leidos
    int indice = 0;
    for (;indice < num_chunks; indice ++){

        newData = (float*) malloc(chunk_size * sizeof(float));
        get_chunk((float*)audio + indice*chunk_size, chunk_size, chunk);
        printf("Genero contexto\n");

        context = clCreateContext( NULL, 1, &deviceID, NULL, NULL, &ret);
        if(ret != CL_SUCCESS)
        {
            printf("Could not create a valid OpenCL context\n");
            return false;
        }
        // Create a command queue
        cmdQueue = clCreateCommandQueue(context, deviceID, 0, &ret);
        if(ret != CL_SUCCESS)
        {
            printf("Could not create an OpenCL Command Queue\n");
            return false;
        }

        /// Create memory buffers on the device for the two images
        printf("Genero parametros\n");

        cl_mem gpuChunk = clCreateBuffer(context,CL_MEM_READ_ONLY,sizeof(float) * chunk_size,NULL,&ret );

        if(ret != CL_SUCCESS)
        {
            printf("Unable to create the GPU chunk buffer object\n");
            return false;
        }

        cl_mem gpuOut = clCreateBuffer(context,CL_MEM_WRITE_ONLY,sizeof(float)*chunk_size,NULL,&ret);
        if(ret != CL_SUCCESS)
        {
            printf("Unable to create the GPU image buffer object\n");
            return false;
        } 
        printf("Preparo parametros\n");

        //Copy the image data and the gaussian kernel to the memory buffer
        if(clEnqueueWriteBuffer(cmdQueue, gpuChunk, CL_TRUE, 0,sizeof(float)*chunk_size,chunk, 0, NULL, NULL) != CL_SUCCESS)
        {
            printf("Error during sending the chunk data to the OpenCL buffer\n");
            return false;
        }
        printf("creo program\n");
        //Create a program object and associate it with the kernel's source code.
        cl_program program = clCreateProgramWithSource(context, 1,(const char **)&kernelSource, (const size_t *)&kernelSrcSize, &ret);
        //free(kernelSource);
        if(ret != CL_SUCCESS)
        {
            printf("Error in creating an OpenCL program object\n");
            return false;
        }
        printf("Build program\n");
        //Build the created OpenCL program
        if((ret = clBuildProgram(program, 1, &deviceID, NULL, NULL, NULL))!= CL_SUCCESS)
        {
            printf("Failed to build the OpenCL program\n");
            //create the log string and show it to the user. Then quit
            char* buildLog;
            buildLog = malloc(MAX_LOG_SIZE);
            if(clGetProgramBuildInfo(program,deviceID,CL_PROGRAM_BUILD_LOG,MAX_LOG_SIZE,buildLog,NULL) != CL_SUCCESS)
            {
                printf("Could not get any Build info from OpenCL\n");
                free(buildLog);
                return false;
            }
            printf("**BUILD LOG**\n%s",buildLog);
            free(buildLog);
            return false;
        }
        printf("creo kernel\n");
        // Create the OpenCL kernel. This is basically one function of the program declared with the __kernel qualifier
        cl_kernel kernel = clCreateKernel(program, "test", &ret);
        if(ret != CL_SUCCESS)
        {
            printf("Failed to create the OpenCL Kernel from the built program\n");
            return false;
        }
        ///Set the arguments of the kernel
        if(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&gpuChunk) != CL_SUCCESS)
        {
            printf("Could not set the kernel's \"gpuImg\" argument\n");
            return false;
        }
        if(clSetKernelArg(kernel, 1, sizeof(int), (void *)&chunk_size) != CL_SUCCESS)
        {
            printf("Could not set the kernel's \"gpuGaussian\" argument\n");
            return false;
        }
        if(clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&gpuOut) != CL_SUCCESS)
        {
            printf("Could not set the kernel's \"gpuOut\" argument\n");
            return false;
        }


        ///enqueue the kernel into the OpenCL device for execution
        size_t globalWorkItemSize = chunk_size;//the total size of 1 dimension of the work items. Basically the whole image buffer size
        size_t workGroupSize = 1; //The size of one work group
        ret = clEnqueueNDRangeKernel(cmdQueue, kernel, 1, NULL, &globalWorkItemSize, &workGroupSize,0, NULL, NULL);


        ///Read the memory buffer of the new image on the device to the new Data local variable
        ret = clEnqueueReadBuffer(cmdQueue, gpuOut, CL_TRUE, 0,chunk_real_size, newData, 0, NULL, NULL);

        ///Clean up everything
        //free(matrix);
        clFlush(cmdQueue);
        clFinish(cmdQueue);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
    /*   clReleaseMemObject(gpuImg);
        clReleaseMemObject(gpuGaussian);
        */
        clReleaseMemObject(gpuOut); 
        clReleaseMemObject(gpuChunk);
        clReleaseCommandQueue(cmdQueue);
        clReleaseContext(context);
        usleep(time*1000000);
        ///save the new image and return success
       /*  for(int i = 0; i< chunk_size; i++){

            printf("Resultado %i: %f\n",i, newData[i]);
        } */
    }
    /* bmp.imgData = newData;
    meImageBMP_Save(&bmp,"gpu_blur.bmp"); */

    //* Preparo variables necesarias para la lectura
   
    
    //* Empieza la lectura del fichero
    //get_chunk((float*)audio, chunk_size,time);
    //write_wav("copia_5.wav", num_samples, (float*)audio, sample_rate);
    printf("He grabado el fichero\n");
    free(audio);
    return true;
}


float* get_chunk(float* buffer, int chunk_size, float* chunk){
    printf("Pillo chunk\n");
    for (int i=0 ; i< chunk_size; i++ ){
        chunk[i] = buffer[i];
        printf("%f size %d\n",buffer[i],sizeof(buffer[i]));
    }
    printf("salgo chunk\n");
    return chunk;
}