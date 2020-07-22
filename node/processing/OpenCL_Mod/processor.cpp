#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <sys/time.h>
#include <math.h>
#include <malloc.h>
#include <omp.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "common.h"
#include "operations.h"
#include "save_wav.cpp"
#ifdef OCL
    #ifdef __APPLE__
    #include <OpenCL/opencl.h>
    #else
    #include <CL/cl.h>
    #endif
#endif
//#define DEBUG


int INDEX_CHUNK = 0;
float resultado = 0.0;
int active;
struct timeval start;
struct timeval end;
struct timeval end_fe;
struct timeval firstChunk;
struct timeval llega_chunk;
u_int64_t llega_chunk_time;


int count = 0;
int result=0;
int first_chunk_threshold;
void* first_chunk_times;
void* last_chunk_times;
int is_good_data = 0;
int data_unnecesary = 0;
int ACTION;
struct Window wndw;
struct Window* win = &wndw;

#ifdef FILTER
int index_malloc = 2000;

float *audio = (float*) malloc(1024 * index_malloc * sizeof(float));
float *filtered_low = (float*) malloc(1024 * index_malloc * sizeof(float));
float *filtered_high = (float*) malloc(1024 * index_malloc * sizeof(float));
float *fft_out = (float*) malloc(1024*index_malloc*sizeof(float));
#endif 
size_t audio_size = 0;
int chunk_size;
//! 0 es preparado para recibir, 1 sin procesar, 2 procesando
int status_ptr[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};//
int buffs_ready = 0;
struct Buffer buf,buf2,buf3,buf4,buf5,buf6,buf7,buf8,buf9,buf10,sbuf,sbuf2,sbuf3,sbuf4,sbuf5,sbuf6,sbuf7,sbuf8,sbuf9,sbuf10;
Buffer ptr[20] = {buf,buf2, buf3, buf4, buf5, buf6, buf7, buf8, buf9, buf10,sbuf,sbuf2,sbuf3,sbuf4,sbuf5,sbuf6,sbuf7,sbuf8,sbuf9,sbuf10};


void fatal(const char *func)
{
	fprintf(stderr, "%s: %s\n", func, nn_strerror(nn_errno()));
	exit(1);
}

#ifdef FFT
float prepare_fft(float buf[],int chunk_size){
    struct timeval start_fft;
	struct timeval end_fft;
    gettimeofday(&start_fft, NULL);
    float out[chunk_size];
    for (int i = 0; i<chunk_size ;i++) out[i] = buf[i];
    fft(buf,out,chunk_size,1);
    gettimeofday(&end_fft, NULL);
	u_int64_t time = (end_fft.tv_sec * 1000000 + end_fft.tv_usec) - (start_fft.tv_sec * 1000000 + start_fft.tv_usec);
    #ifdef DEBUG
	printf("%lu \n",time);
    #endif
    return time;
}
#endif
#if defined OCL && defined FILTER
/* void notify_chunk_manager(){
    std::unique_lock<std::mutex> lock(m);
    notified = true;
    cond_var.notify_one(); 
} */
#endif

void processing(float* buf, size_t current_audio_size, int INDEX_CHUNK, int analyser_sock, Window *win){
        #ifdef THRESHOLD
            #ifdef DEBUG
            printf("Entro a threshold\n");
            #endif
            new_analyse_threshold((float*)buf, chunk_size, analyser_sock, INDEX_CHUNK,win);
            #ifdef DEBUG
            printf("Salgo de threshold\n");
            #endif
        #endif
        #ifdef FILTER
        #ifdef OCL
        filter((float*)buf, chunk_size,audio + current_audio_size/4, filtered_low + current_audio_size/4,filtered_high + current_audio_size/4, 2, 20);

        #else
        filter((float*)buf, chunk_size,audio + current_audio_size/4, filtered_low + current_audio_size/4,filtered_high + current_audio_size/4, 2, 20);
        new_fft((float*)buf,fft_out+current_audio_size/4, chunk_size/4);
        #endif
        #endif
}
void get_conf_parameters(void* conf, Window *win){
    char delim[] = "_";
    char *pointer = strtok((char*)conf, delim);
    int value= atoi(pointer);
    win -> value_threshold  = value;
	pointer = strtok(NULL, delim);
    int size = atoi(pointer);
    win -> size = size ;
	pointer = strtok(NULL, delim);
    int percentage = atoi(pointer);
    win -> percentage =percentage;
    win -> min_samples = size * win -> percentage / 100;
    win -> margin_size = size - 1;
    win -> buffer = (int*) calloc(size,sizeof(int));
    win -> counter = 0;
    win -> actual_size = 0;
    win -> first_chunk = 0;
    win -> first_sample = 0;
    win -> last_chunk = 0;
    win -> last_sample = 0;
    #if defined DATA && defined THRESHOLD
        prepare_windows(value, size, percentage);
    #endif
}
int check_save_audios(int action, int socket, void* conf){
    if(action == 11 + 1){
        #ifdef FILTER
        #ifdef OCL
        write_txt("original.txt", audio_size/4, (float*)audio);
        write_txt("real.txt", audio_size/4, filtered_low);
        write_txt("imaginario.txt", audio_size/4, filtered_high);
        #endif
        write_wav("original.wav", audio_size/4,  audio,44100);
        write_wav("high_filter.wav", audio_size/4,  filtered_high,44100);
        write_wav("low_filter.wav", audio_size/4,  filtered_low,44100);   
        #endif
        action = nn_recv(socket, &conf,NN_MSG ,0);
    }
    return action;
}

int check_action(int action, int socket, void* conf){
    #ifdef DEBUG
    printf("CA Llega la acción %s con tamaño %d\n",conf,action );
    #endif
    if(action == 5+1){
        int size_config = nn_recv(socket, &conf,NN_MSG ,0);
        if(size_config == -1){
            printf("No se han enviado los datos de configuración\n");
            return 1;
        }
        #ifdef DEBUG
        printf("Llega la configuración %s con tamaño %d\n",conf, size_config);
        #endif
        get_conf_parameters(conf,win);
        #ifdef DEBUG
        printf("THRESH %d WINDOW %d PERCEN %d SAMPLES %d\n",win -> value_threshold,win -> size,win -> percentage,win -> min_samples);
        #endif
    }
    return action;
}

int recv_name(int processor, int analyser)
{
    void* conf;
    #ifdef OCL 
    #ifdef FILTER
    prepare_filter();
    #endif 
    #ifdef THRESHOLD
    prepare_threshold();
    #endif
    #endif
    int ack = nn_recv(processor, &conf,NN_MSG ,0);
    if(ack == -1 || strcmp("ACK",(char*)conf)){
        printf("No ha habido comunicación\n");
        return 1;
    }
    #ifdef DEBUG
        printf("Conexión establecida\n");
    #endif
    int j = nn_send(processor,"ack\0", sizeof "ack\0", 0);
    ACTION = nn_recv(processor, &conf,NN_MSG ,0);
    #ifdef DEBUG
    printf("Llega la acción\n");
    #endif
    int size_config;
    ACTION = check_action(ACTION, processor, conf);
    #if defined DATA && defined THRESHOLD
    switch_notifier(analyser, 1);
    #endif
    win -> buffer = (int*) calloc(win -> size,sizeof(int));
    while(ACTION != 4+1){   
        active = 1;
        INDEX_CHUNK = 0;
        int gotTime = 0;
        int next_to_process = 0;
        int next_to_fill = 0;
        int result = 0;
        #if defined TASK || defined TASK_SIMD || defined DATA_SIMD 
        #pragma omp parallel num_threads(omp_get_max_threads()) 
        {
            #pragma omp single nowait
            {
        #endif
                #ifdef DEBUG
                printf("Empiezo a recoger\n");
                #endif
                while(active || buffs_ready)
                {
                    if(status_ptr[next_to_fill] == 0)
                    {  
                        #ifdef DEBUG
                        printf("Tengo hueco \n");
                        #endif
                        if(!gotTime)
                        {
                            gettimeofday(&start, NULL);
                        }
                        #ifdef DEBUG
                        printf("Espero a recibir mensaje \n");
                        #endif
                        result = nn_recv(processor, &ptr[next_to_fill].buf,NN_MSG ,0);
                        #ifdef DEBUG
                        printf("Recibo mensaje TAMAÑO %d \n", result);
                        #endif
                        if(result == -1)
                        {
                            active = 0;
                        #ifdef DEBUG
                        printf("Ha habido algún error en la conexión \n");
                        #endif
                        } 

                        if(result == 3+1){
                            active = 0;
                        #ifdef DEBUG
                        printf("Ya no cojo más buffs %d \n",buffs_ready);
                        #endif
                        } 
                        if(active && result == chunk_size){
                            #ifdef DEBUG_P
                            gettimeofday(&llega_chunk, NULL);
                            llega_chunk_time= (llega_chunk.tv_sec * 1000000 + llega_chunk.tv_usec);
                            printf("Llega,,,%lu\n",llega_chunk_time/1000);
                            #endif
                            #ifdef DEBUG
                            printf("Cojo chunk %d\n",result);
                            #endif
                            #ifdef FILTER
                            if(count >= index_malloc)
                            {
                                index_malloc *= 2;
                                audio = (float*) realloc(audio,chunk_size * index_malloc);
                                filtered_low = (float*) realloc(filtered_low,chunk_size * index_malloc); 
                                filtered_high = (float*) realloc(filtered_high,chunk_size * index_malloc); 
                                fft_out = (float*) realloc(fft_out,chunk_size * index_malloc);
                            }
                            #endif
                            status_ptr[next_to_fill] = 1;
                            ptr[next_to_fill].current_size = audio_size;
                            next_to_fill = (next_to_fill+1) % MAX_BUFFERS;
                            buffs_ready ++;
                            audio_size += (size_t)result;
                        }
                    }
                    if(status_ptr[next_to_process] == 1 && buffs_ready)
                    {
                        #ifdef DEBUG
                        printf("Voy a procesar chunk \n");
                        #endif
                        #if defined TASK || defined TASK_SIMD || defined DATA_SIMD
                            int index = INDEX_CHUNK;
                            status_ptr[next_to_process] = 2;

                            #pragma omp task
                            {
                            processing((float*)ptr[next_to_process].buf,ptr[next_to_process].current_size,index,analyser,win);
                            status_ptr[next_to_process] = 0;
                            buffs_ready --;
                            }
                        #else
                            
                        status_ptr[next_to_process] = 2;
                        processing((float*)ptr[next_to_process].buf,ptr[next_to_process].current_size,INDEX_CHUNK,analyser,win);
                        status_ptr[next_to_process] = 0;
                        buffs_ready --;	 
                                
                        #endif
                        #ifdef DEBUG
                            printf("Proceso chunk\n");
                        #endif
                        INDEX_CHUNK ++;
                        next_to_process = (next_to_process + 1) % MAX_BUFFERS;
                        gettimeofday(&end, NULL);
                    }
                    if(!gotTime && result == chunk_size && active)
                    {
                        gettimeofday(&firstChunk,NULL);
                        gotTime = 1; 
                        count ++;
                    }
                    if(gotTime && result== 3+1 && active)
                    {
                        gettimeofday(&end_fe, NULL);
                        count ++;
                    }
                    #ifdef DEBUG
                    printf("Acabo iteración\n");
                    #endif
                }
        #if defined TASK || defined TASK_SIMD || defined DATA_SIMD
            }
        }
        #pragma taskwait
        #endif
        #ifdef OCL
        free_resources();
        #endif
        //! Enviar el tiempo del primer chunk a Node.js
        u_int64_t time = (end_fe.tv_sec * 1000000 + end_fe.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
        u_int64_t timeFirstChunk = firstChunk.tv_sec * 1000000 + firstChunk.tv_usec;
        char msg[20];
        gcvt(timeFirstChunk, 19, msg);
        char times[20];
        gcvt(time, 19, times);
        int sz_n = 20;
        int test = nn_send(processor, msg, sz_n, 0);
        //! Grabar audio en fichero(original y filtrado)
        audio_size -= data_unnecesary;
        //printf("%lu,%lu,%s,%s\n",begin_time/1000,end_time/1000,first_chunk_times,last_chunk_times);
        #ifdef OCL
        u_int64_t begin_time = (start.tv_sec * 1000000 + start.tv_usec);
        u_int64_t end_time = (end.tv_sec * 1000000 + end.tv_usec);
        //printf("%lu\n",end_time-begin_time);
        printf("E %lu\n",end_time);
        #endif
        #if defined DATA && defined THRESHOLD
        switch_notifier(analyser, 0);
        #endif 

        /* ACTION = nn_recv(processor, &conf,NN_MSG ,0);
        if(!ACTION){
            printf("ACTION ERROR %d\n", ACTION);
        }
        ACTION = check_save_audios(ACTION,processor,conf);
        if(!ACTION){
            printf("ACTION ERROR %d\n", ACTION);
        }
        ACTION = check_action(ACTION, processor, conf);
        if(!ACTION){
            printf("ACTION ERROR %d\n", ACTION);
        } */

        data_unnecesary = 0;
        audio_size = 0;
        ACTION=5;
    }
    #ifdef DEBUG
    printf("CIERRO PROGRAMA\n");
    #endif
	return 1; 
}

int send_recv(int processor, int analyser)
{   
    int seconds = 20;
	int processor_timeout = seconds * 1000;

    int analyser_timeout = -1;
	if (nn_setsockopt(processor, NN_SOL_SOCKET, NN_RCVTIMEO, &processor_timeout, sizeof(processor_timeout)) < 0)
	{
		fatal("nn_setsockopt processor");
	}
    if (nn_setsockopt(analyser, NN_SOL_SOCKET, NN_RCVTIMEO, &analyser_timeout, sizeof(analyser_timeout)) < 0)
	{
		fatal("nn_setsockopt analyser");
	}
	recv_name(processor,analyser);
    sleep(1);
	nn_close(processor);
    nn_close(analyser);
	return 1;
}

int node0(const char *url_processor, const char * url_analyser)
{
	int const processor =  nn_socket(AF_SP, NN_PAIR);
    int const analyser = nn_socket(AF_SP, NN_PAIR);
	if (processor  < 0)
	{
		fatal("nn_socket processor");
	}
    if (analyser  < 0)
	{
		fatal("nn_socket analyser");
	}
	if (nn_connect(processor, url_processor) < 0)
	{
		fatal("nn_connect processor");
	}
    if (nn_connect(analyser, url_analyser) < 0)
	{
		fatal("nn_connect analyser");
	}
	return (send_recv(processor, analyser));
}

int main(const int argc, const char **argv)
{   char tech2[30];
	char tech[30];
	strcpy(tech, argv[1]);
	strcat(tech, "://127.0.0.1:54272");
    strcpy(tech2, argv[1]);
	strcat(tech2, "://127.0.0.1:54273");
    chunk_size = CHUNK_SIZE*4;
	return (node0(tech,tech2));
}