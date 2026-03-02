#ifndef EZV_TOPO_H
#define EZV_TOPO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hwloc.h>

#define EZV_TOPO_UNFOLDED_LEVEL 0

typedef struct
{
  unsigned first_pu;
  unsigned nb_pus;
} ezv_topo_pu_block_t;

void ezv_topo_init (void);
int ezv_topo_init_from_file (const char *filename);
void ezv_topo_finalize (void);
int ezv_topo_is_initialized (void);
char *ezv_topo_get_topo_filename(void);

hwloc_topology_t ezv_topo_get_topology (void);
unsigned ezv_topo_get_depth (unsigned folding_level);
unsigned ezv_topo_get_maxdepth (void);
unsigned ezv_topo_get_nb_pus (void);

int ezv_topo_get_current_cpuset (hwloc_cpuset_t cpuset);
int ezv_topo_set_current_cpuset (hwloc_cpuset_t cpuset);

typedef void (*ezv_topo_callback_t) (hwloc_obj_t obj, int col, int row_inf,
                                     int row_sup, void *arg);
void ezv_topo_for_each_obj (unsigned folding_level, ezv_topo_callback_t func, void *arg);

unsigned ezv_topo_get_nb_pu_kinds (void);
unsigned ezv_topo_get_pu_efficiency (unsigned pu_logical_index);

hwloc_cpuset_t ezv_topo_convert_os_to_logical_cpuset (hwloc_cpuset_t os_set);
int ezv_topo_cpusets_contiguous_and_disjoint (hwloc_cpuset_t *sets,
                                              unsigned nsets);
void ezv_topo_cpuset_to_pu_block (hwloc_cpuset_t set,
                                  ezv_topo_pu_block_t *block);
void ezv_topo_save_to_file (const char *filename);
void ezv_topo_save_if_needed (const char *directory, char *fullname, size_t size);

unsigned ezv_topo_get_nb_combos (unsigned level);
unsigned ezv_topo_get_combo_for_pu (unsigned level, unsigned pu_logical_index);
void ezv_topo_get_combo_coverage (unsigned level, unsigned combo_index,
                                  ezv_topo_pu_block_t *block);
unsigned ezv_topo_get_combo_weight (unsigned folding_level,
                                    unsigned combo_index);
void ezv_topo_disable_folding (unsigned level);

int ezv_topo_first_valid_folding (void);
int ezv_topo_next_valid_folding (unsigned level);
int ezv_topo_prev_valid_folding (unsigned level);

#ifdef __cplusplus
}
#endif

#endif
