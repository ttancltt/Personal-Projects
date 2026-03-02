#include <stdbool.h>
#include <unistd.h>

#include "error.h"
#include "mon_obj.h"

void mon_obj_init (mon_obj_t *mon, unsigned thr, unsigned gpu,
                   ezv_mon_mode_t mode, unsigned folding_level,
                   ezv_clustering_t clustering)
{
  mon->thr           = thr;
  mon->gpu           = gpu;
  mon->mode          = mode;
  mon->folding_level = folding_level;
  mon->clustering    = clustering;
}
