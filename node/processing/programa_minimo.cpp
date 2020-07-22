#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <sys/time.h>
#include <math.h>
#include <malloc.h>
#include <thread>
#include <mutex>
#include <condition_variable>
int NUM_THREADS = 4;
std::mutex m;
std::condition_variable cond_var;

bool notified = false;
bool finish = false;
int const CHUNK_SIZE = 8;
int const WINDOW_SIZE = 3;
int const MARGIN_SIZE = WINDOW_SIZE-1;
int const NUM_CHUNKS = 2;
int PERCENTAGE = 75;

int CHUNK1[CHUNK_SIZE] = {0,1,0,1,0,1,0,1};//,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1};
int CHUNK2[CHUNK_SIZE] = {1,1,1,0,0,1,0,0};//,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0};
int *after_margin = (int*) calloc(MARGIN_SIZE,sizeof( int));
int *CHUNKS[NUM_CHUNKS] = {CHUNK1,CHUNK2};

struct MessageInfo{
    int thread;
    int sample;
    int send_message;
    int index_chunk;
} info1, *information1 = &info1,info2, *information2 = &info2,info3, *information3 = &info3,info4, *information4 = &info4;;

struct Window{
    int size = WINDOW_SIZE;
    int actual_size = 0;
    int *buffer = (int*) calloc(size,sizeof(int));
    int counter = 0;
    int min_samples = WINDOW_SIZE * PERCENTAGE / 100;
    int thread;
}win1, *window1 = &win1, win2, *window2 = &win2, win3, *window3 = &win3, win4, *window4 = &win4;;

Window *windows[4] = {window1, window2,window3,window4};
MessageInfo *infos[4] = {information1,information2,information3,information4};
void shift_window(Window *window){
    for(int i = 0; i<window -> actual_size-1; i++){
        window -> buffer[i] = window -> buffer[i+1];
    }
}

void reset_window(Window *window){
    for(int i = 0; i<WINDOW_SIZE; i++){
        window -> buffer[i] = 0;
    }
    window -> counter = 0;
    window -> actual_size = 0;  
}

void* notify(){
    while(!finish){
        {  
            {
                std::unique_lock<std::mutex> lock(m);
                cond_var.wait(lock, []{return notified;}); 
                #ifdef DEBUG  
                printf("\nMe han notificado: %d %d %d\n",information1 -> send_message, information2 -> send_message, information3 -> send_message);
                #endif
                for(int i = 0; i<NUM_THREADS; i++){
                    if(infos[i]->send_message){
                        infos[i]->send_message = 0;
                        //printf("NOTIFICADOR THREAD %d CHUNK %d SAMPLE %d\n",infos[i] -> thread, infos[i] ->index_chunk, infos[i] -> sample);
                    }            
                }
                notified = false;
            }
        }
    }
}

void* process_buffer(int begin, int end, int* chunk, Window *window, MessageInfo *information, int index_chunk){
    int margin_start = CHUNK_SIZE - MARGIN_SIZE;
    int margin_index = 0;
    //! Primera mitad del chunk, añadiendo a la ventana el margen anterior
    if(window->thread == 0){
        for(int i = 0; i < MARGIN_SIZE; i ++){
            window -> counter += after_margin[i] ;
            window -> buffer[i] = after_margin[i] ;
            window -> actual_size ++;
        }
    }
    //! Empiezo a almacenar en la ventana el chunk actual
    for(int i = begin; i <= end; i++){
        if(window -> actual_size == window -> size){
            window -> counter -= window -> buffer[0];
            if(window -> counter < 0){
                window -> counter = 0;
            }
            shift_window(window);
        }else{
            window -> actual_size ++;
        }
        //! Preparo el margen para el siguiente chunk
        if(window->thread == NUM_THREADS - 1){
            if(i == margin_start){
                after_margin[margin_index] = chunk[i];
                margin_start ++;
                margin_index ++;
            }
        }
        //! Almaceno la información en la ventana
        window -> buffer[window -> actual_size - 1] = chunk[i];
        window -> counter += chunk[i];

        if (window -> counter >= window -> min_samples){
            #ifdef DEBUG
            printf("BUFFER THREAD %d  CHUNK %d SAMPLE %d \n",window->thread,index_chunk,i);
            #endif
            while(information -> send_message){
                std::unique_lock<std::mutex> lock(m);
                notified = true;
                cond_var.notify_all();   
            } 
            {
                std::unique_lock<std::mutex> lock(m);
                information->sample = i;
                information ->send_message = 1;
                information ->index_chunk = index_chunk;
                notified = true;
                cond_var.notify_all();   
            }       
        }
        
    }
}
void set_windows_information(){
    for (int i = 0; i<NUM_THREADS;i++){
        windows[i] -> thread = i;
        infos[i] -> thread = i;
    }
}

void start_demo(){
    std::thread noti;
    std::thread my_threads[4];
    noti = std::thread(notify);
    set_windows_information();
    for (int i_chunk = 0; i_chunk < NUM_CHUNKS; i_chunk ++){
        for (int i = 0 ; i < NUM_THREADS; i++){
            int inicio = i*CHUNK_SIZE/NUM_THREADS;
            int fin = ((i+1)*CHUNK_SIZE/NUM_THREADS)-1;
            if(inicio) inicio = inicio - MARGIN_SIZE > 0 ? inicio - MARGIN_SIZE : 0;
            //printf("Thread %d Inicio %d Final %d\n", i, inicio, fin);
            my_threads[i] = std::thread(process_buffer,inicio,fin,CHUNKS[i_chunk],windows[i],infos[i],i_chunk);
        }
        for (int i = 0 ; i < NUM_THREADS; i++){
            my_threads[i].join();
            reset_window(windows[i]);
        }
    }
    {
        std::unique_lock<std::mutex> lock(m);
        finish = true;
        notified = true;
        cond_var.notify_all(); 
    }
    noti.join();
}
int main(const int argc, const char **argv)
{   
    start_demo();
	return 0;
}