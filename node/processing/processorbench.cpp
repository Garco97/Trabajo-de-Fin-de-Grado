#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <sys/time.h>
#include <math.h>

void fatal(const char *func)
{
	fprintf(stderr, "%s: %s\n", func, nn_strerror(nn_errno()));
	exit(1);
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

    }
    return action;
}

int recv_name(int sock)
{
	int ACTION;
	struct timeval start;
	struct timeval end;
	struct timeval firstChunk;
	int result;
	int index = 100;
	int count = 0;
	char *audio;
	audio = (char*) malloc(4096 * index * sizeof(float));
	char *buf = "BEGIN";
	char *endBuf = "END";
	int gotTime = 0;
	int active = 1;
	void* conf;
    int ack = nn_recv(sock, &conf,NN_MSG ,0);
    if(ack == -1 || strcmp("ACK",(char*)conf)){
        printf("No ha habido comunicación\n");
        return 1;
    }
    #ifdef DEBUG
        printf("Conexión establecida\n");
    #endif
    int j = nn_send(sock,"ack\0", sizeof "ack\0", 0);
    ACTION = nn_recv(sock, &conf,NN_MSG ,0);
    #ifdef DEBUG
    printf("Llega la acción\n");
    #endif
    int size_config;
    ACTION = check_action(ACTION, sock, conf);
    #if defined DATA && defined THRESHOLD
    switch_notifier(analyser, 1);
    #endif

    while(active){   

		if(gotTime == 0){
			gettimeofday(&start, NULL);
		}
		result = nn_recv(sock, &buf,NN_MSG ,0);
		if(strstr(buf,endBuf) != NULL){
			active = 0;
		}
		if(active){
			if(count >= index){
				index = index * 2;
				audio = (char*) malloc(4096 * index * sizeof(float));
			}
			strcat(audio,buf);
			//printf("%d \n",buf[0]);
		}
		if(gotTime == 0 && result > 0){
			gettimeofday(&firstChunk,NULL);
			gotTime = 1; 
			count ++;
		}
		if(gotTime == 1 && result > 3){
			gettimeofday(&end, NULL);
		}
	}
	u_int64_t time = (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
	u_int64_t timeFirstChunk = firstChunk.tv_sec * 1000000 + firstChunk.tv_usec;
	char msg[20];
	gcvt(timeFirstChunk, 19, msg);
	char times[20];
	gcvt(time, 19, times);
	// printf("%s \n", times);
	int sz_n = 20;
	nn_send(sock, msg, sz_n, 0);
	FILE *fp;
   	//fp = fopen("~/Desktop/testc.wav", "w");
	//fputs(audio,fp);
	//fclose(fp);
	return (result);
}

int send_recv(int sock)
{
	int to = 300*1000000;
	if (nn_setsockopt(sock, NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof(to)) < 0)
	{
		fatal("nn_setsockopt");
	}
	recv_name(sock);
	sleep(1);
	nn_close(sock);
}

int node0(const char *url)
{
	int sock;
	if ((sock = nn_socket(AF_SP, NN_PAIR)) < 0)
	{
		fatal("nn_socket");
	}
	if (nn_connect(sock, url) < 0)
	{
		fatal("nn_connect");
	}
	return (send_recv(sock));
}

int main(const int argc, const char **argv)
{
	char tech[30];
	strcpy(tech, argv[1]);
	strcat(tech, "://127.0.0.1:54272");
	return (node0(tech));
}