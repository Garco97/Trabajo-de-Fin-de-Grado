// N = size of input, power of 2.

// T = N/2 = number of threads.

// I = index of current thread, in 0..T-1.

// DFT step, input is X[N] and output is Y[N].

// P is the length of input sub-sequences, 1,2,4,...,N/2.

// Cell (S,K) is stored at index S*P+K.
// a*b => a, b unchanged
#define MUL(a,b,tmp) { tmp=a; a.x=tmp.x*b.x-tmp.y*b.y; a.y=tmp.x*b.y+tmp.y*b.x; }
#define PI_ 3.14159265359f

// DFT2(a,b) => (a,b)
#define DFT2(a,b,tmp) { tmp=a-b; a+=b; b=tmp; }

// Return cos(alpha)+I*sin(alpha)
float2 exp_alpha(float alpha){
  float cs,sn;
  sn = sincos(alpha,&cs);
  return (float2)(cs,sn);
}

// pow_theta(p,q) returns exp(-Pi*P/Q)
float2 pow_theta(int p, int q){
    return exp(-PI_ * p / q);
}

__kernel void fft_radix2(__global const float2 * x,__global float2 * y,int p){

  int i = get_global_id(0); // number of threads
  int t = get_global_size(0); // current thread
  int k = i & (p-1); // index in input sequence, in 0..P-1

  // Input indices are I and I+T.
  float2 u0 = x[i];
  float2 u1 = x[i+t];

  // Twiddle input (U1 only)
  float2 twiddle = pow_theta(k,p);
  float2 tmp;
  MUL(u1,twiddle,tmp);

  // Compute DFT
  DFT2(u0,u1,tmp);

  // Output indices are J and J+P, where
  // J is I with 0 inserted at bit log2(P).
  int j = (i<<1) - k; // = ((i-k)<<1)+k
  y[j] = u0;
  y[j+p] = u1;

}