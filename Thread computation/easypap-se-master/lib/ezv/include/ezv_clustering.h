#ifndef EZV_CLUSTERING_H
#define EZV_CLUSTERING_H

#include "ezv_topo.h"

typedef struct
{
  unsigned first_idx;
  unsigned size;
  unsigned nb_thr;
} ezv_cluster_t;

struct ezv_clustering_struct;
typedef struct ezv_clustering_struct *ezv_clustering_t;

ezv_clustering_t ezv_clustering_create (unsigned nb_thr,
                                        ezv_topo_pu_block_t *bindings);
void ezv_clustering_destroy (ezv_clustering_t clustering);

unsigned ezv_clustering_get_nb_clusters (ezv_clustering_t clustering,
                                         unsigned level);
void ezv_clustering_get_nth_cluster (ezv_clustering_t clustering,
                                     unsigned level, unsigned n,
                                     ezv_cluster_t *cluster);
unsigned ezv_clustering_get_cluster_index_from_thr (ezv_clustering_t clustering,
                                                    unsigned level,
                                                    unsigned thr);
int ezv_clustering_get_cluster_index_from_combo (ezv_clustering_t clustering,
                                                 unsigned level,
                                                 unsigned combo_idx);
unsigned ezv_clustering_get_cluster_weight (ezv_clustering_t clustering,
                                            unsigned level,
                                            unsigned cluster_idx);
int ezv_clustering_binding_exists (ezv_clustering_t clustering);

#endif
