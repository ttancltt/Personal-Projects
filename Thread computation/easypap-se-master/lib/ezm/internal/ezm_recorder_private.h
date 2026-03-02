#ifndef EZM_RECORDER_PRIVATE_H
#define EZM_RECORDER_PRIVATE_H

#include "ezv.h"
#include "ezv_topo.h"
#include "ezv_clustering.h"
#include "ezm.h"
#include "ezm_footprint.h"
#include "ezm_perfmeter.h"
#ifdef ENABLE_TRACE
#include "ezm_tracerec.h"
#endif

struct ezm_recorder_struct
{
  unsigned nb_thr;
  unsigned nb_gpus;
  unsigned time_needed;
  unsigned enabled;
  ezv_topo_pu_block_t *bindings;
  unsigned folding_level;
  ezv_palette_t palette;
  unsigned palette_mode;
  mon_obj_t monitor;
  ezm_perfmeter_t perfmeter;
  ezm_footprint_t footprint;
#ifdef ENABLE_TRACE
  ezm_tracerec_t tracerec;
#endif
  ezv_clustering_t clustering;
};

unsigned ezm_get_nb_cpu_colors (ezm_recorder_t rec);
uint32_t ezm_get_cpu_color (ezm_recorder_t rec, unsigned c);


#endif
