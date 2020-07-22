
//! Variable para filtrar los 0s innecesarios
extern int is_good_data;
extern int data_unnecesary;

//! Variables threshold benchmark
extern void* first_chunk_times;
extern int first_chunk_threshold;
extern void* last_chunk_times;
//! Variables OPENCL
struct Buffer {
   void* buf;
   int current_size;
};
extern bool notified;
extern bool finish; 
extern int status_ptr[];
extern int buffs_ready;
extern Buffer ptr[];
#ifdef FILTER
    extern float *audio;
    extern float *filtered_low;
    extern float *filtered_high;
    extern std::thread my_thread;
    extern std::mutex m;
    extern std::condition_variable cond_var;
#endif