__kernel void test(__global const float *chunk, const int size, __global float *newData) 
{ 

    int i = get_global_id(0);
    // Intento de reverberación
    /*
    float decay = 0.5f;
    int delayMilliseconds = 1000; // half a second
    int delaySamples = (int)((float)delayMilliseconds * 44.1f); // assumes 44100 Hz sample rate
    newData[i + delaySamples] += chunk[i] * decay;
    */

    // Intento de low_pass
    /*
    float f_c = 800;
	float f_samp = 44100;
    float pi = 3.14159265359;
    float w_c = 2* pi * f_c;
    int middle = size / 2;
    if(chunk[i] == middle){
        newData[i] = 2*f_c;
    }else{
        newData[i + middle] = sin(w_c*chunk[i])/(pi*chunk[i]);
    }	
    */

    // Intento de bass booster 
    /*
    float selectivity, gain1, gain2, ratio, cap;
    selectivity = 70.0;
    gain2 = -0.3;
    ratio = 0.5;
    gain1 = 1.0/(selectivity + 1.0);
    cap= (chunk[i] + cap*selectivity )*gain1;
    float max = (chunk[i] + cap*ratio)*gain2 > -1.0 ? (chunk[i] + cap*ratio)*gain2 : -1.0;
    float min = max < 1.0 ? max : min;
    newData[i] = min;
    */


 }
