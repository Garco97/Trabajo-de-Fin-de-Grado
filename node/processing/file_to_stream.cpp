#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CHUNK_SIZE 8192
#define SAMPLE_RATE 44100.0

int main(const int argc, const char **argv){
    FILE *file;
    int size = 40;
    int index = 0;
    char *buffer_reader;
    char* audio = (char*) malloc(size * sizeof(char));
    
    //* Abro el fichero
    file=fopen(argv[1], "r");
    if(file == NULL ){   
        fprintf(stderr, "%s\n", "Error abriendo el archivo");
        exit(1);
    }

    //* Realizo la lectura del fichero
    while(fread(buffer_reader, 1,1,file)){
        audio[index] = *buffer_reader;
        index ++;
        if (index == size){
            size = size*2;
            audio = (char*) realloc(audio,size * sizeof(char));
        }
    }
    //* Cierro el fichero
    fclose(file);

    //* Preparo variables necesarias para la lectura
    float time = CHUNK_SIZE / SAMPLE_RATE;
    float* aux = (float*) audio;
    int num_samples = index / 4;
    int num_chunks = 0;

    //* Espera para realizar la lectura cuando se desee
    printf("Enter para la lectura del fichero\n");
    char c = getchar( );
    
    //* Empieza la lectura del fichero
    for (int i = 0; i< num_samples; i++ ){
        if(!(i % CHUNK_SIZE)){
            printf("He leido el chunk %d\n",num_chunks);
            num_chunks++;
            usleep(time*1000000);
        }
    }
    //* Libero la memoria
    free(audio);
    return 1;
}