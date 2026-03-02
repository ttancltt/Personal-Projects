#ifndef MON_OBJ_H
#define MON_OBJ_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ezv_clustering.h"
#include "ezv_topo.h"

typedef enum
{
  EZV_MONITOR_DEFAULT,
  EZV_MONITOR_TOPOLOGY,
  EZV_MONITOR_TOPOLOGY_COMPACT
} ezv_mon_mode_t;

typedef struct
{
  unsigned thr;
  unsigned gpu;
  ezv_mon_mode_t mode;
  ezv_topo_pu_block_t *bindings;
  unsigned folding_level;
  ezv_clustering_t clustering;
} mon_obj_t;

void mon_obj_init (mon_obj_t *mon, unsigned thr, unsigned gpu,
                   ezv_mon_mode_t mode, unsigned folding_level,
                   ezv_clustering_t clustering);

#ifdef __cplusplus
}
#endif

#endif
