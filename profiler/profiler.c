#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <pthread.h>
#include <time.h>

#define MAX_FRAMES 128
#define MAX_STACKS 10000
#define SAMPLE_INTERVAL_US 1000  /* 1ms sampling interval */

typedef struct {
    void *frames[MAX_FRAMES];
    int frame_count;
    uint64_t count;
    uint64_t instructions;
    uint64_t memory_delta;
} StackSample;

typedef struct {
    StackSample samples[MAX_STACKS];
    int sample_count;
    uint64_t total_samples;
    uint64_t total_instructions;
    uint64_t total_memory;
    struct timespec start_time;
    struct timespec end_time;
} ProfilerState;

static ProfilerState state = {0};
static volatile int profiling = 0;
static pthread_t sampler_thread;

/* Get current thread CPU instructions (macOS) */
static uint64_t get_instructions(void) {
    kern_return_t ret;
    mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
    thread_basic_info_data_t info;
    
    ret = thread_info(mach_thread_self(), THREAD_BASIC_INFO,
                      (thread_info_t)&info, &count);
    if (ret != KERN_SUCCESS) return 0;
    
    /* Return CPU ticks as proxy for instructions */
    return info.cpu_usage;
}

/* Get memory usage stats */
static uint64_t get_memory_usage(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss;  /* Peak memory in bytes */
    }
    return 0;
}

/* Capture stack trace */
static int capture_stack(void **frames, int max_frames) {
    return backtrace(frames, max_frames);
}

/* Find or create stack sample entry */
static StackSample *find_or_create_sample(void **frames, int frame_count) {
    for (int i = 0; i < state.sample_count; i++) {
        if (state.samples[i].frame_count == frame_count &&
            memcmp(state.samples[i].frames, frames, 
                   frame_count * sizeof(void*)) == 0) {
            return &state.samples[i];
        }
    }
    
    if (state.sample_count >= MAX_STACKS) return NULL;
    
    StackSample *sample = &state.samples[state.sample_count++];
    memcpy(sample->frames, frames, frame_count * sizeof(void*));
    sample->frame_count = frame_count;
    sample->count = 0;
    sample->instructions = 0;
    sample->memory_delta = 0;
    
    return sample;
}

/* Sampler thread main loop */
static void *sampler_main(void *arg) {
    uint64_t last_instr = 0;
    uint64_t last_mem = 0;
    
    while (profiling) {
        void *frames[MAX_FRAMES];
        int frame_count = capture_stack(frames, MAX_FRAMES);
        
        StackSample *sample = find_or_create_sample(frames, frame_count);
        if (sample) {
            sample->count++;
            
            uint64_t instr = get_instructions();
            if (last_instr > 0) {
                sample->instructions += (instr - last_instr);
            }
            last_instr = instr;
            
            uint64_t mem = get_memory_usage();
            if (last_mem > 0) {
                sample->memory_delta += (mem - last_mem);
            }
            last_mem = mem;
            
            state.total_samples++;
        }
        
        usleep(SAMPLE_INTERVAL_US);
    }
    
    return NULL;
}

/* Initialize profiler */
int profiler_start(void) {
    if (profiling) return -1;
    
    clock_gettime(CLOCK_MONOTONIC, &state.start_time);
    profiling = 1;
    state.total_samples = 0;
    state.sample_count = 0;
    
    if (pthread_create(&sampler_thread, NULL, sampler_main, NULL) != 0) {
        profiling = 0;
        return -1;
    }
    
    return 0;
}

/* Stop profiler */
int profiler_stop(void) {
    if (!profiling) return -1;
    
    profiling = 0;
    pthread_join(sampler_thread, NULL);
    clock_gettime(CLOCK_MONOTONIC, &state.end_time);
    
    return 0;
}

/* Get elapsed time in seconds */
static double get_elapsed_time(void) {
    time_t sec = state.end_time.tv_sec - state.start_time.tv_sec;
    long nsec = state.end_time.tv_nsec - state.start_time.tv_nsec;
    
    if (nsec < 0) {
        sec--;
        nsec += 1000000000;
    }
    
    return (double)sec + (double)nsec / 1000000000.0;
}

/* Write JSON flame graph format */
int profiler_write_json(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return -1;
    
    fprintf(f, "{\n");
    fprintf(f, "  \"version\": 1,\n");
    fprintf(f, "  \"elapsed_seconds\": %.2f,\n", get_elapsed_time());
    fprintf(f, "  \"total_samples\": %llu,\n", state.total_samples);
    fprintf(f, "  \"stacks\": [\n");
    
    for (int i = 0; i < state.sample_count; i++) {
        StackSample *sample = &state.samples[i];
        
        fprintf(f, "    {\n");
        fprintf(f, "      \"count\": %llu,\n", sample->count);
        fprintf(f, "      \"instructions\": %llu,\n", sample->instructions);
        fprintf(f, "      \"memory_delta\": %llu,\n", sample->memory_delta);
        fprintf(f, "      \"frames\": [\n");
        
        for (int j = 0; j < sample->frame_count; j++) {
            Dl_info info;
            const char *symbol = "??";
            
            if (dladdr(sample->frames[j], &info) && info.dli_sname) {
                symbol = info.dli_sname;
            }
            
            fprintf(f, "        {\"addr\": \"%p\", \"symbol\": \"%s\"}",
                    sample->frames[j], symbol);
            if (j < sample->frame_count - 1) fprintf(f, ",");
            fprintf(f, "\n");
        }
        
        fprintf(f, "      ]\n");
        fprintf(f, "    }");
        if (i < state.sample_count - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    
    fclose(f);
    return 0;
}

/* Write human-readable summary */
int profiler_write_summary(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return -1;
    
    fprintf(f, "=== CPU PROFILER SUMMARY ===\n\n");
    fprintf(f, "Total Runtime: %.2f seconds\n", get_elapsed_time());
    fprintf(f, "Total Samples: %llu\n", state.total_samples);
    fprintf(f, "Unique Stack Traces: %d\n\n", state.sample_count);
    
    /* Sort by sample count */
    int indices[MAX_STACKS];
    for (int i = 0; i < state.sample_count; i++) indices[i] = i;
    
    for (int i = 0; i < state.sample_count - 1; i++) {
        for (int j = i + 1; j < state.sample_count; j++) {
            if (state.samples[indices[j]].count > state.samples[indices[i]].count) {
                int tmp = indices[i];
                indices[i] = indices[j];
                indices[j] = tmp;
            }
        }
    }
    
    fprintf(f, "HOT PATHS (Top 20):\n");
    fprintf(f, "%-8s %-10s %-15s %s\n", "Samples", "%%", "Instructions", "Function");
    fprintf(f, "--------+----------+---------------+---\n");
    
    for (int i = 0; i < state.sample_count && i < 20; i++) {
        StackSample *sample = &state.samples[indices[i]];
        double pct = (100.0 * sample->count) / state.total_samples;
        
        Dl_info info;
        const char *symbol = "??";
        
        if (sample->frame_count > 0 &&
            dladdr(sample->frames[0], &info) && info.dli_sname) {
            symbol = info.dli_sname;
        }
        
        fprintf(f, "%-8llu %-10.1f %-15llu %s\n",
                sample->count, pct, sample->instructions, symbol);
    }
    
    fprintf(f, "\n=== END SUMMARY ===\n");
    fclose(f);
    return 0;
}

/* Get raw state (for Python binding) */
ProfilerState *profiler_get_state(void) {
    return &state;
}
