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
#include "operations.h"
#include "common.h"
#define DEBUG_DATA
//! Variables para el new_threshold data-parallel
int const  NUM_THREADS = 1;
int const MIN_MSGS = 10;
int const MAX_SOCKETS_MSGS = 50;
std::mutex m;
std::condition_variable cond_var;
std::thread noti;
struct MessageInfo{
    int first_sample = 0;
    int first_chunk = 0;
    int last_sample = 0; 
    int last_chunk = 0;
    int counter = 0;
} ;
struct Message{
    int thread = 0;
    int counter = 0; 
    int index[MAX_SOCKETS_MSGS];
    MessageInfo *messages[MAX_SOCKETS_MSGS];
};

struct Window w1,w2,w3,w4;
struct Window *win1 = &w1;
struct Window *win2 = &w2;
struct Window *win3 = &w3;
struct Window *win4 = &w4;
struct Window *windows[4] = {win1,win2,win3,win4};

struct Message m1,m2,m3,m4;
struct Message *mes1 = &m1;
struct Message *mes2 = &m2;
struct Message *mes3 = &m3;
struct Message *mes4 = &m4;
struct Message *msgs[4] = {mes1,mes2,mes3,mes4};
std::thread my_threads[4];


bool notified = false;
bool finish = false;
int available_msgs[4];
int *after_margin;

//!------------------------------------------------
double PI =  atan2(1, 1) * 4;
int res;

int discard_zeros(float buf[],int chunk_size ){
    int i = 0;
    for( ; i<chunk_size; i++){
        if(buf[i] != 0.0){
            data_unnecesary += i +1;
            is_good_data = 1;
            return i;
        }
    }
    data_unnecesary += i;
    return i;
}

void fft(float buf[], float out[], int n, int step){
    if(step<n){ 
        fft(out,buf,n,step*2);
        fft(out+step,buf+step,n,step*2);    
        #pragma omp parallel num_threads(omp_get_max_threads())
        {
        #pragma omp for schedule(static,1024)
        for(int i = 0; i<n; i+=2*step)
        {
            float t = exp(PI * i/n) * out[i+step];
            buf[i/2] = out[i] + t;
            buf[(i+n)/2] = out[i] - t;
        }
        }
    }
}

void prepare_windows(int value, int size, int percentage){
    for(int i = 0; i<NUM_THREADS; i++){
        windows[i] -> value_threshold = value;
        windows[i] -> size = size;
        windows[i] -> percentage = percentage;
        windows[i] -> margin_size = size - 1;
        windows[i] -> min_samples = size * percentage /100;
        windows[i] -> buffer = (int*) calloc(size,sizeof(int));
        windows[i] -> thread = i;
        struct Message mes;
        msgs[i] -> thread = i;
        for(int j = 0; j<MAX_SOCKETS_MSGS;j++){
            struct MessageInfo aux;
            struct MessageInfo *a = &aux;
            
            printf("%d\n",&a);
            msgs[i] -> messages[j] = a;
        }
    }
    after_margin = (int*) calloc(size -1, sizeof(int));
}

void shift_window(Window *window){
    for(int i = 0; i<window -> actual_size-1; i++){
        window -> buffer[i] = window -> buffer[i+1];
    }  
}

void clean_window(Window *window){
    for(int i=0; i<window -> size; i++){
        window -> buffer[i] = 0;
    }
    window -> counter = 0;
    window -> actual_size = 0;  
    window -> first_sample = 0;
    window -> first_chunk = 0;
    window -> last_sample = 0;
    window -> last_chunk = 0;

}


void* notify(int socket ){
    char *message = (char*) malloc(10 * sizeof(char));
    struct timeval start;
    u_int64_t time;
    while(!finish){
        {  
            std::unique_lock<std::mutex> lock(m);
            cond_var.wait(lock, []{return notified;}); 
            #ifdef DEBUG_DATA  
            if(!finish)
            printf("Me han notificado\n");
            #endif
            
            for(int i = 0; i<NUM_THREADS; i++){
                if(available_msgs[i]){
                    #ifdef DEBUG_DATA
                    printf("Proceso mensajes en thread %d %d mensajes\n",i,msgs[i] ->counter);
                    #endif
                    int stacked_msgs = 0;
                    for (int j = 0; j<MAX_SOCKETS_MSGS; j++){
                        if(msgs[i] ->index[j]){
                            char sample[100], chunk[100], first_chunk[100], first_sample[100], counter[100]; 
                            sprintf(sample, "_%d", msgs[i] -> messages[j] -> last_sample);
                            sprintf(chunk, "_%d", msgs[i] -> messages[j] -> last_chunk); 
                            strcat(sample,chunk);
                            strcat(message,sample);

                            sprintf(first_sample, "_%d",  msgs[i] -> messages[j] -> first_sample);
                            sprintf(first_chunk, "_%d",  msgs[i] -> messages[j] -> first_chunk); 
                            strcat(message,first_sample);
                            strcat(message,first_chunk);

                            sprintf(counter, "_%d/", msgs[i] -> messages[j] -> counter);
                            strcat(message,counter);
                            msgs[i] -> index[j] = 0;
                            msgs[i] -> counter --;

                        }
                    }

                }            
            } 
            gettimeofday(&start, NULL);
            time = start.tv_sec * 1000000 + start.tv_usec;
            char msg_time[20];
            sprintf(msg_time, "_%lu_", time);
            strcat(message, msg_time);

            printf("Envio mensaje\n");
            res = nn_send(socket,message, sizeof(message), 0);

            if (res< 0)
            {
            fprintf(stderr, "%s: %s\n", "nn_connect analyser", nn_strerror(nn_errno()));
            exit(1);
            } 
            notified = false;
        }
    }
    free(message);
}

void* process_buffer(int begin, int end, float* chunk, Window *window, Message *msg, int index_chunk,int chunk_size ){
    int margin_start = chunk_size - window -> margin_size;
    int margin_index = 0;

    //! Primera mitad del chunk, añadiendo a la ventana el margen anterior
    if(window->thread == 0){
        for(int i = 0; i <  window->margin_size; i ++){
            window -> counter += after_margin[i] ;
            window -> buffer[i] = after_margin[i] ;
            window -> actual_size ++;
        }
    }
    //! Empiezo a almacenar en la ventana el chunk actual
    printf("Begin THREAD %d  counter %d first %d %d last %d %d\n",index_chunk, window->counter,  window -> first_sample,begin,window -> last_sample,end);
    for(int i = begin; i <= end; i++){
        if(window -> actual_size == window -> size){
            window -> counter -= window -> buffer[0];

            if(window -> counter < 0){
                window -> counter = 0;
            }
            shift_window(window);
            window -> first_sample ++;
            if(window -> first_sample == chunk_size ){
                window -> first_sample = 0;
                window -> first_chunk ++;
            }   
        }else{
            window -> actual_size ++;

        }
        int has_passed = fabsf(chunk[i]*100) > window -> value_threshold;
        //! Preparo el margen para el siguiente chunk
        if(window->thread == NUM_THREADS - 1){
            if(i == margin_start){
                after_margin[margin_index] = has_passed;
                margin_start ++;
                margin_index ++;
            }
        }

        //! Almaceno la información en la ventana
        window -> buffer[window -> actual_size - 1] = has_passed;
        window -> counter += has_passed;
        window -> last_sample = i;
        window -> last_chunk = index_chunk;
        //printf("asking THREAD %d  counter %d first %d %d last %d %d\n",index_chunk, window->counter,  window -> first_sample,begin,window -> last_sample,end);

        //? Empaquetar mensajes para el envi, notificar cada 4 y tener hueco de 8, por ejemplo.
        if (window -> counter >= window -> min_samples){
                std::unique_lock<std::mutex> lock(m);
            {
                if(msg -> counter < MAX_SOCKETS_MSGS){
                    int j= 0;
                    for(; j<MAX_SOCKETS_MSGS; j++){

                        if(!msg -> index[j]){
                            printf("j %d\n",j);
                        printf("looking THREAD %d %d counter %d first %d %d last %d %d\n",index_chunk,j, window->counter,  window -> first_sample,begin,window -> last_sample,end);
                            break;
                        }
                    }

                
                    if(j!=MAX_SOCKETS_MSGS){
                        printf("process1 THREAD %d %d counter %d first %d %d last %d %d\n",index_chunk,j, window->counter,  window -> first_sample,begin,window -> last_sample,end);
                        msg -> counter ++;
                        msg -> index[j] = 1;
                        available_msgs[msg -> thread] = 1;
                        msg -> messages[j] -> counter = window -> counter;
                        msg -> messages[j] -> first_sample = window -> first_sample;
                        msg -> messages[j] -> first_chunk = window -> first_chunk;
                        
                        msg -> messages[j] -> last_sample = window -> last_sample;
                        msg -> messages[j] -> last_chunk = window -> last_chunk;
                        
                          
                        printf("process2 THREAD %d %d counter %d first %d %d last %d %d\n",index_chunk,j, window->counter,  window -> first_sample,begin,window -> last_sample,end);

                    }   
                    
        
                }else{
                    //? ESPERAR ¿CÓMO MANEJO ESTO? SLEEP???                  
                } 
            }
            /* if(msg -> counter >= MIN_MSGS)
            {
                std::unique_lock<std::mutex> lock(m);
                notified = true;
                cond_var.notify_one();   
            }  */  
        
        }
    }
    printf("finish THREAD %d  counter %d first %d %d last %d %d\n",index_chunk, window->counter,  window -> first_sample,begin,window -> last_sample,end);

}

void new_analyse_threshold(float* buffer, int chunk_size, int socket,  int index_chunk, Window *w){
    //printf("Primer chunk\n");
   // int bad_samples = 0;

    chunk_size = chunk_size / 4;
    /* if(!is_good_data){
        bad_samples = discard_zeros(buffer, chunk_size);
        if(bad_samples == chunk_size) return;
    } */

    
    for (int i = 0 ; i < NUM_THREADS; i++){
        int inicio = i*chunk_size/NUM_THREADS;
        int fin = ((i+1)*chunk_size/NUM_THREADS)-1;
        if(inicio){
            inicio = inicio - windows[i]->margin_size > 0 ? inicio -  windows[i]->margin_size : 0;
        }
        printf("Thread %d Inicio %d Final %d %d \n", i, inicio, fin,chunk_size);
        windows[i] -> first_sample = inicio;
        windows[i] -> first_chunk = index_chunk;
        windows[i] -> thread = i;
        msgs[i] -> thread = i;

        my_threads[i] = std::thread(process_buffer,inicio,fin,buffer,windows[i],msgs[i],index_chunk,chunk_size);
    }
    for (int i = 0 ; i < NUM_THREADS; i++){
        my_threads[i].join();
        printf("Acabo thread\n");
        clean_window(windows[i]);
    }
}

void switch_notifier(int socket,int on){
    if(on){
        printf("Arranco\n");
        noti = std::thread(notify, socket);
    }else{
        {
        std::unique_lock<std::mutex> lock(m);
        finish = true;
        notified = true;
        cond_var.notify_all(); 
        }
        noti.join();
        printf("Apago\n");

    }
}

void* analyse_threshold(float* buffer, int chunk_size, int socket,  int index_chunk){
    struct timeval time;
    char res[7];
    u_int64_t threshold_time;

    #pragma omp parallel num_threads(omp_get_max_threads())
    {
        #pragma omp for schedule(OMP_FOR) private(threshold_time) 
        for(int i = 0; i<chunk_size/4;i++)
        {
            if(fabsf (buffer[i]*100) > 50){
                gcvt(i, 6, res);
                if(threshold_time){
                    gettimeofday(&time,NULL);
                    threshold_time = time.tv_sec * 1000000 + time.tv_usec;
                }
                if(i == (chunk_size/4)-1){
                    char aux[36];
                    snprintf(aux, sizeof aux, "%f_%d_%d_%lu_", buffer[i]*100,i, index_chunk,threshold_time);
                    int j = nn_send(socket,aux, sizeof aux, 0);
                    if (j< 0){
                        fprintf(stderr, "%s: %s\n", "nn_connect analyser", nn_strerror(nn_errno()));
                        exit(1);
                    }
                    if(first_chunk_threshold<1){
                        first_chunk_threshold = nn_recv(socket, &first_chunk_times,NN_MSG ,0);
                    }else{
                        int w = nn_recv(socket, &last_chunk_times,NN_MSG,0);
                    }
                }
            }
        }
    }
}


void filter(float* audio, unsigned long audio_size, float* out_original, float* out_low, float* out_high, double time_interval, double time_constant )
{

	double alpha_high = time_interval/(time_interval + time_constant);
    double alpha_low = time_interval/(time_interval + time_constant);
	out_high[0] = audio[0];
    out_low[0] = alpha_low * audio[0];
    out_original[0] = audio[0];

    #pragma omp parallel num_threads(omp_get_max_threads())
    {
        #pragma omp for schedule(OMP_FOR)
        for(size_t i=1;i < audio_size/4;i += 1)
        {
            out_high[i] = alpha_high*out_high[i-1] + alpha_high * (audio[i] - out_high[i-1]);
            out_low[i] = out_low[i-1] + alpha_low * (audio[i] - out_low[i-1]);
            out_original[i] = audio[i];

        }
    }
}

void new_fft(float buf[], float out[], int n){
    float *aux;
    int h = n >> 1;
    int sublen = 1;
    int stride = n; 
    int omp_for = 0;
    while (sublen < n){
        stride >>= 1;
        if(stride < 8){
            omp_for = 4;
        }else{
        omp_for = stride/8;

        }
        #pragma omp parallel num_threads(omp_get_max_threads())
        {
        #pragma omp for schedule(OMP_FOR)
        for(int i = 0; i < stride; i++){
            for(int k = 0; k<n; k += 2*stride){
                float omega = exp(PI * k / n);

                out[i+(k>>1)] = buf[i+k] + omega * buf[i+k+stride];
                out[i+(k>>1)+h] = buf[i+k] - omega * buf[i+k+stride];
            }
        }
        }
        aux = buf;
        buf = out;
        out = aux;
        sublen <<=1;
    }
}