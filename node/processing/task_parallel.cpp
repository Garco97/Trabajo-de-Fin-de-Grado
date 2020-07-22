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
#include "common.h"
double PI =  atan2(1, 1) * 4;

void fft(float buf[], float out[], int n, int step){
     if(step<n){ 
        fft(out,buf,n,step*2);
        fft(out+step,buf+step,n,step*2);    
        for(int i = 0; i<n; i+=2*step){
            float t = exp(PI * i/n) * out[i+step];
            buf[i/2] = out[i] + t;
            buf[(i+n)/2] = out[i] - t;
        }
    } 
}


void* analyse_threshold(float* buffer, int chunk_size, int socket,  int index_chunk){
    struct timeval time;
    char res[7];
    u_int64_t threshold_time;
    for(int i = 0; i<chunk_size/4;i++){
        if(fabsf (buffer[i]*100) > MAX_THRESHOLD){
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

void* new_analyse_threshold(float* buffer, int chunk_size, int socket,  int index_chunk){
    struct timeval time;
    char res[7];
    int counter = 5;
    for(int i = 0; i<chunk_size/4;i++){
        if(fabsf (buffer[i]*100)> MAX_THRESHOLD)
        {
            if(!counter)
            {
                counter = 5;
            }
            counter--;
        }else{
            counter = 5;
        }
        if(counter == 0) 
        {
            gcvt(i, 6, res);
            gettimeofday(&time,NULL);
            u_int64_t threshold_time = time.tv_sec * 1000000 + time.tv_usec;
            char aux[36];
            snprintf(aux, sizeof aux, "%f-%d-%d-%lu-", buffer[i]*100,i, index_chunk,threshold_time);
            int j = nn_send(socket,aux, sizeof aux, 0);
            if (j< 0)
            {
               fprintf(stderr, "%s: %s\n", "nn_connect analyser", nn_strerror(nn_errno()));
	           exit(1);
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

    for(size_t i=1;i < audio_size/4;i += 1)
        {
            out_high[i] = alpha_high*out_high[i-1] + alpha_high * (audio[i] - out_high[i-1]);
            out_low[i] = out_low[i-1] + alpha_low * (audio[i] - out_low[i-1]);
            out_original[i] = audio[i];
            
        }
}

void new_fft(float buf[], float out[], int n){
    float *aux;
    int h = n >> 1;
    int sublen = 1;
    int stride = n; 
    while (sublen < n){
        stride >>= 1;
        for(int i = 0; i < stride; i++){
            for(int k = 0; k<n; k += 2*stride){
                float omega = exp(PI * k / n);
                out[i+(k>>1)] = buf[i+k] + omega * buf[i+k+stride];
                out[i+(k>>1)+h] = buf[i+k] - omega * buf[i+k+stride];

            }
        }
        aux = buf;
        buf = out;
        out = aux;
        sublen <<=1;
    }

}