#include <stdio.h>
#include <unistd.h>
#include <nanomsg/nn.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <omp.h>
#include <condition_variable>
#include <thread>
#include <mutex>
#include "operations.h"
#include "common.h"
double PI = atan2(1, 1) * 4;
int res;


int discard_zeros(float buf[],int chunk_size ){
    int i = 0;
    for( ; i<chunk_size/4; i++){
        if(buf[i] != 0.0){
            data_unnecesary += i+1;
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
        for(int i = 0; i<n; i+=2*step){
            float t = exp(PI * i/n) * out[i+step];
            buf[i/2] = out[i] + t;
            buf[(i+n)/2] = out[i] - t;
        }
    }
}

void* analyse_threshold(float* buffer, int chunk_size, int socket,  int index_chunk){
    for(int i = 0; i<chunk_size/4;i++){
        if(fabsf (buffer[i]*100) > -1){
            if(i == ((chunk_size/4)-1)){
                char res[20];
                sprintf(res,"%f",fabsf (buffer[i]*100) );
                int size = strlen(res);
                sprintf(res,"%d",i );
                int size2 = strlen(res);
                sprintf(res,"%d",index_chunk);
                int size3 = strlen(res);
                char aux[size + 3 + size2 + size3 ];
                snprintf(aux, sizeof aux, "%f_%d_%d_", fabsf(buffer[i]*100),i, index_chunk);
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


void shift_window(Window *window){
    #pragma omp parallel num_threads(omp_get_max_threads()) 
    {
    #pragma omp for schedule(guided) 
    for(int i = 0; i<window -> actual_size-1; i++){
        window -> buffer[i] = window -> buffer[i+1];
    }
    }
}

void clean_window(Window *window){
    #pragma omp parallel num_threads(omp_get_max_threads()) 
    {
    #pragma omp for schedule(guided) 
    for(int i=0; i<window -> size; i++){
        window -> buffer[i] = 0;
    
    }
    }
}

void new_analyse_threshold(float* buffer, int chunk_size, int socket,  int index_chunk, Window *window){
    int i = 0;
    int has_passed;
    if(!is_good_data){
        i = discard_zeros(buffer, chunk_size);
    }
    i = 0;
    for( ; i<chunk_size/4;i++){
        has_passed = fabsf (buffer[i]*100) > window -> value_threshold; 
        if(window -> actual_size == window -> size){
            window -> counter -= window -> buffer[0];
            if(window -> counter < 0){
                window -> counter = 0;
            }
            shift_window(window);
            window -> first_sample ++;
            if(window -> first_sample == (chunk_size/4) -1){
                window -> first_sample = 0;
                window -> first_chunk ++;
            }
        }else{
            window -> actual_size ++;
        }
        window -> counter += has_passed;

        window -> buffer[window -> actual_size-1] = has_passed;

        window -> last_sample = i;
        window -> last_chunk = index_chunk;
        if(window -> counter == window -> min_samples){
            char sample[24], chunk[14], first_chunk[14], first_sample[14]; 
            sprintf(sample, "_%d", window -> last_sample);
            sprintf(chunk, "_%d", window -> last_chunk); 
            strcat(sample,chunk);

            sprintf(first_sample, "_%d", window -> first_sample);
            sprintf(first_chunk, "_%d_", window -> first_chunk); 
            strcat(first_sample,first_chunk);
            strcat(sample,first_sample);
            
            res = nn_send(socket,sample, sizeof(sample), 0);
            if (res< 0)
            {
               fprintf(stderr, "%s: %s\n", "nn_connect analyser", nn_strerror(nn_errno()));
	           exit(1);
            }
            //clean_window(window);
            //window -> counter = 0;
            
        }
    }
}

void filter(float* audio, unsigned long audio_size, float* out_original, float* out_low, float* out_high, double time_interval, double time_constant )
{
    int i = 0;
    if(!is_good_data){
        i = discard_zeros(audio, audio_size );
    }
    if (i == audio_size/4){
        return;
    }

    double alpha_high = time_interval/(time_interval + time_constant);
    double alpha_low = time_interval/(time_interval + time_constant);

	out_high[i] = audio[i];
    out_low[i] = alpha_low * audio[i];
    out_original[i] = audio[i];

    for( i=1;i < audio_size/4;i += 1)
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