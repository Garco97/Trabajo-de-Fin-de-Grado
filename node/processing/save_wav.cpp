#include <stdio.h>
#include <assert.h>
 
#ifndef MAKE_WAV_H
#define MAKE_WAV_H
 
void write_wav(char * filename, unsigned long num_samples, float * data, int s_rate);
void write_txt(char *filename, unsigned long num_samples, float* data);

#endif
 
void write_little_endian(unsigned int word, int num_bytes, FILE *wav_file)
{
    unsigned buf;
    while(num_bytes>0)
    {   buf = word & 0xff;
        fwrite(&buf, 1,1, wav_file);
        num_bytes--;
    word >>= 8;
    }
}
void write_txt(char *filename, unsigned long num_samples, float* data){

    FILE* wav_file;
    wav_file = fopen(filename, "w");
    for (int i=0; i< num_samples; i++)
    {   
        //fwrite(&data[i], 4,1, wav_file);
        fprintf(wav_file,"%f\n",data[i]);
    }

    fclose(wav_file);
}

 
void write_wav(char * filename, unsigned long num_samples,float * data, int s_rate)
{
    FILE* wav_file;
    unsigned int sample_rate;
    unsigned int num_channels;
    unsigned int bytes_per_sample;
    unsigned int byte_rate;
    unsigned long i;    /* counter for samples */
 
    num_channels = 1;   /* monoaural */
    bytes_per_sample = 4;
 
    sample_rate = 44100;
    byte_rate = sample_rate*num_channels*bytes_per_sample;
 
    wav_file = fopen(filename, "w");
    assert(wav_file);   /* make sure it opened */
 
    /* write RIFF header */
    fwrite("RIFF", 1, 4, wav_file);
    write_little_endian(36 + bytes_per_sample* num_samples*num_channels, 4, wav_file);
    fwrite("WAVE", 1, 4, wav_file);
 
    /* write fmt  subchunk */
    fwrite("fmt ", 1, 4, wav_file);
    write_little_endian(16, 4, wav_file);   /* SubChunk1Size is 16 */
    write_little_endian(3, 2, wav_file);    /* PCM is format 1 */
    write_little_endian(num_channels, 2, wav_file);
    write_little_endian(sample_rate, 4, wav_file);
    write_little_endian(byte_rate, 4, wav_file);
    write_little_endian(num_channels*bytes_per_sample, 2, wav_file);  /* block align */
    write_little_endian(8*bytes_per_sample, 2, wav_file);  /* bits/sample */
 
    /* write data subchunk */
    fwrite("data", 1, 4, wav_file);
    write_little_endian(bytes_per_sample* num_samples*num_channels, 4, wav_file);
    for (i=0; i< num_samples; i++)
    {   
        fwrite(&data[i], 4,1, wav_file);
    }
 
    fclose(wav_file);
}