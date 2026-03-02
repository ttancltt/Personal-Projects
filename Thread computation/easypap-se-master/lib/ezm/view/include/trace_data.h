#ifndef TRACE_DATA_IS_DEF
#define TRACE_DATA_IS_DEF

#include <stdint.h>

#include "ezm_types.h"
#include "ezv.h"
#include "ezv_clustering.h"
#include "ezv_topo.h"
#include "list.h"

typedef struct
{
  uint64_t start_time, end_time;
  unsigned x, y, w, h;
  int task_type;
  int task_id;
  unsigned iteration;
  struct list_head thr_chain;
} trace_task_t;

typedef struct
{
  uint64_t start_time, end_time;
  uint64_t correction, gap;
  struct list_head chain;
  trace_task_t **first_cpu_task;
} trace_iteration_t;

typedef struct
{
  unsigned num;
  unsigned dimensions;
  unsigned nb_thr;
  unsigned nb_gpu;
  unsigned nb_rows;
  ezv_topo_pu_block_t *thr_binding;
  ezv_clustering_t clustering;
  unsigned topo_loaded;
  unsigned folding_level;
  unsigned first_iteration;
  unsigned nb_iterations;
  char *label;
  char *mesh_file;
  ezv_palette_name_t palette;
  char **task_ids;
  unsigned task_ids_count;
  struct list_head *per_thr;
  trace_iteration_t *iteration;
} trace_t;

#define MAX_TRACES 2

extern trace_t trace [MAX_TRACES];
extern unsigned nb_traces;
extern unsigned trace_data_align_mode;

void trace_data_init (trace_t *tr, unsigned num);
void trace_data_set_nb_threads (trace_t *tr, unsigned nb_thr, unsigned nb_gpu);
void trace_data_alloc_bindings (trace_t *tr);
void trace_data_set_binding (trace_t *tr, unsigned thr, unsigned first_pu,
                             unsigned nb_pus);
void trace_data_load_topo (trace_t *tr, char *filename);
void trace_data_set_dim (trace_t *tr, unsigned dim);
void trace_data_set_first_iteration (trace_t *tr, unsigned it);
void trace_data_set_label (trace_t *tr, char *label);
void trace_data_set_meshfile (trace_t *tr, char *filename);
void trace_data_set_palette (trace_t *tr, ezv_palette_name_t palette);

void trace_data_alloc_task_ids (trace_t *tr, unsigned count);
void trace_data_add_taskid (trace_t *tr, char *id);

void trace_data_add_task (trace_t *tr, uint64_t start_time, uint64_t end_time,
                          unsigned x, unsigned y, unsigned w, unsigned h,
                          unsigned cpu, task_type_t task_type, int task_id);

void trace_data_start_iteration (trace_t *tr, uint64_t start_time);
void trace_data_end_iteration (trace_t *tr, uint64_t end_time);

void trace_data_no_more_data (trace_t *tr);
void trace_data_sync_iterations (void);

void trace_data_finalize (void);

#define for_all_tasks(tr, cpu, var)                                            \
  list_for_each_entry (trace_task_t, var, (tr)->per_thr + (cpu), thr_chain)

int trace_data_search_iteration (trace_t *tr, long t);
int trace_data_search_next_iteration (trace_t *tr, long t);
int trace_data_search_prev_iteration (trace_t *tr, long t);

#if 1
static inline uint64_t iteration_start_time (trace_t *tr, unsigned it)
{
  const trace_iteration_t *iter = &tr->iteration [it];

  if (trace_data_align_mode)
    return iter->start_time + iter->correction;
  else
    return iter->start_time;
}

static inline uint64_t iteration_end_time (trace_t *tr, unsigned it)
{
  const trace_iteration_t *iter = &tr->iteration [it];

  if (trace_data_align_mode)
    return iter->end_time + iter->correction + iter->gap;
  else
    return iter->end_time;
}

static inline uint64_t task_start_time (trace_t *tr, trace_task_t *t)
{
  if (trace_data_align_mode)
    return t->start_time +
           tr->iteration [t->iteration - tr->first_iteration].correction;
  else
    return t->start_time;
}

static inline uint64_t task_end_time (trace_t *tr, trace_task_t *t)
{
  if (trace_data_align_mode)
    return t->end_time +
           tr->iteration [t->iteration - tr->first_iteration].correction;
  else
    return t->end_time;
}

#else

#define iteration_start_time(tr, it)                                           \
  (trace_data_align_mode                                                       \
       ? (tr)->iteration [it].start_time + (tr)->iteration [it].correction     \
       : (tr)->iteration [it].start_time)

#define iteration_end_time(tr, it)                                             \
  (trace_data_align_mode                                                       \
       ? (tr)->iteration [it].end_time + (tr)->iteration [it].correction +     \
             (tr)->iteration [it].gap                                          \
       : (tr)->iteration [it].end_time)

#define task_start_time(tr, t)                                                 \
  (trace_data_align_mode                                                       \
       ? (t)->start_time +                                                     \
             (tr)->iteration [(t)->iteration - (tr)->first_iteration]          \
                 .correction                                                   \
       : (t)->start_time)

#define task_end_time(tr, t)                                                   \
  (trace_data_align_mode                                                       \
       ? (t)->end_time +                                                       \
             (tr)->iteration [(t)->iteration - (tr)->first_iteration]          \
                 .correction                                                   \
       : (t)->end_time)

#endif

#endif
