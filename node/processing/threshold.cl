__kernel void threshold(__global float *chunk, __global int *salida, const int value) 
{ 
    int i = get_global_id(0);
    salida[i] = fabs(chunk[i]) *100 > value;
}

__kernel void peak(__global int *threshold_values, __global int *peaks, const int size, const int min)
{
    int i = get_global_id(0);
    int counter = 0;
    for(int j = 0; j < size; j++){
        counter += threshold_values[(i*size)+j];
        
    }
    peaks[i] = counter >= min;
}