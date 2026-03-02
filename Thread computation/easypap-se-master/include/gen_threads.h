#ifndef GEN_THREADS_H
#define GEN_THREADS_H

typedef void (*trail_func_t)(void);

typedef enum {
  GEN_THREADS_SCHED_BLOCK,
  GEN_THREADS_SCHED_CYCLIC,
  GEN_THREADS_SCHED_DYN,
  GEN_THREADS_SCHED_RUNTIME
} gen_threads_schedule_t;

void gen_threads_config_2D (gen_threads_schedule_t sched, trail_func_t trail);
void gen_threads_config_3D (gen_threads_schedule_t sched, trail_func_t trail);

void gen_threads_finalize (void);

unsigned gen_threads_compute_2D (unsigned nb_iter);
unsigned gen_threads_compute_3D (unsigned nb_iter);

#endif
