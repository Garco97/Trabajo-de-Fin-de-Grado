struct Window{
    int value_threshold = 0;
    int size = 0;
    int percentage=0;
    int margin_size = 0;
    int actual_size = 0;
    int counter = 0;
    int min_samples = 0;
    int thread = 0;
    int first_sample = 0;
    int first_chunk = 0;
    int last_sample = 0; 
    int last_chunk = 0;
    int *buffer; 
};

void* analyse_threshold(float* buffer, int chunk_size, int socket, int index_chunk);
void prepare_threshold();

void  new_analyse_threshold(float* buffer, int chunk_size, int socket, int index_chunk, Window *window);

void fft(float buf[], float out[], int n, int step);

void filter(float* audio, unsigned long audio_size, float* out_original, float* out_low, float* out_high, double time_interval, double time_constant );

void prepare_filter();
void free_resources();
void new_fft(float buf[], float out[], int n);

void switch_notifier(int socket,int on);

void prepare_windows(int value, int size, int percentage);