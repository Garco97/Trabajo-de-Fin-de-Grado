#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include "save_wav.cpp"
#define SAMPLE_RATE 44100.0

int size = CHUNK_SIZE*4;
char* audio = (char*) malloc(size * sizeof(char));
struct timeval start;
struct timeval end;
void fatal(const char *func)
{
	fprintf(stderr, "%s: %s\n", func, nn_strerror(nn_errno()));
	exit(1);
}

int get_audio_data(const char* file_name,int size ){
    FILE *file;
    int index = 0;
    char buffer_reader[20];
    file=fopen(file_name, "r");
    if(file == NULL ){   
        fprintf(stderr, "%s\n", "Error abriendo el archivo");
        exit(1);
    }
    while(fread(buffer_reader, 1,1,file)){
        audio[index] = *buffer_reader;
        index ++;
        if (index == size){
            size = size*2;
            audio = (char*) realloc(audio,size * sizeof(char));
        }
    }
    fclose(file);
    return index;
}
void send_audio_data(float* buffer, int socket, int n_samples, float time){
    int n_chunks = ceil(n_samples/CHUNK_SIZE);
    //n_chunks = 1;
    int res = 0;
        gettimeofday(&start, NULL);
    u_int64_t begin_time;
    for (int i = 0; i< n_chunks; i++ ){
        #ifdef REAL
        usleep(time*1000000);
        #endif
        /* 
        begin_time= (start.tv_sec * 1000000 + start.tv_usec);
        printf("Lee,%d,,%lu\n",i,begin_time/1000); */
        res = nn_send(socket,buffer + CHUNK_SIZE*i, CHUNK_SIZE*sizeof(float), 0);
        if(!res){
            printf("ERROR enviando paquete %d\n",res);
        }
        //write_txt("chunk_values.txt",CHUNK_SIZE,buffer + CHUNK_SIZE*i);
    }
    gettimeofday(&end, NULL);
}
int main(const int argc, const char **argv){    
    int index = get_audio_data(argv[1],size);
    FILE *fp;
    char str[50];
    if(argc > 2){
        strcpy(str, argv[2]);
        fp = fopen(str, "w");
    }
    int processor =  nn_socket(AF_SP, NN_PAIR);
    if (processor  < 0)
	{
		fatal("nn_socket processor");
	}

	if (nn_bind(processor, "ipc://127.0.0.1:54272") < 0)
	{
		fatal("nn_connect processor");
	}
    int seconds = 20;
	int processor_timeout = seconds * 1000;

	if (nn_setsockopt(processor, NN_SOL_SOCKET, NN_RCVTIMEO, &processor_timeout, sizeof(processor_timeout)) < 0)
	{
		fatal("nn_setsockopt processor");
	}  
    char buf[20];
     int ack = nn_send(processor,"ACK", sizeof "ACK", 0);
    ack = nn_recv(processor, buf,NN_MSG ,0);
    if(!ack){
        printf("ERROR enviando paquete %d\n",ack);
    }
    
    ack = nn_send(processor,"START", sizeof "START", 0);
    if(!ack){
        printf("ERROR enviando paquete %d\n",ack);
    }    
    float time = (CHUNK_SIZE / SAMPLE_RATE) + 0.00232189;
    ack = nn_send(processor,"0_0_0", sizeof "0_0_0", 0);
    //printf("index %d\n",index/4);
    audio = audio + 44;
    index = index/4 - 44/4;
    send_audio_data((float*) audio ,processor,index,time);
    ack = nn_send(processor,"END", sizeof "END", 0);
    if(!ack){
        printf("ERROR enviando paquete %d\n",ack);
    }
    u_int64_t begin_time = (start.tv_sec * 1000000 + start.tv_usec);
    if(argc > 2){
        fprintf(fp, "S %lu\n",begin_time);
        fclose(fp);
    }else{
        printf("S %lu\n",begin_time);
    }
    ack = nn_recv(processor, buf,NN_MSG ,0);
    if(!ack){
        printf("ERROR recibiendo final paquete %d\n",ack);
    }
    nn_close(processor);
    return 1;
}