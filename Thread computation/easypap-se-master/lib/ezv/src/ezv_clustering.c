
#include "ezv_clustering.h"
#include "error.h"

typedef struct
{
  ezv_cluster_t *clusters;
  unsigned nb_clusters;
  unsigned *thr_cluster; // thread to cluster index
  int *reverse_mapping;  // combo to cluster index
} ezv_level_clustering_t;

struct ezv_clustering_struct
{
  unsigned nb_thr;
  ezv_topo_pu_block_t *bindings;
  ezv_level_clustering_t *clusterings;
  unsigned nb_clusterings;
};

unsigned ezv_clustering_get_nb_clusters (ezv_clustering_t clustering,
                                         unsigned level)
{
  if (level >= clustering->nb_clusterings)
    exit_with_error ("ezv_clustering_get_nb_clusters: invalid level %u\n",
                     level);

  return clustering->clusterings [level].nb_clusters;
}

void ezv_clustering_get_nth_cluster (ezv_clustering_t clustering,
                                     unsigned level, unsigned n,
                                     ezv_cluster_t *cluster)
{
  if (level >= clustering->nb_clusterings)
    exit_with_error ("ezv_clustering_get_nth_cluster: invalid level %u\n",
                     level);
  ezv_level_clustering_t *level_clustering = &clustering->clusterings [level];
  if (level_clustering->clusters == NULL)
    exit_with_error (
        "ezv_clustering_get_nth_cluster: clustering not available for "
        "level %u\n",
        level);

  if (n >= level_clustering->nb_clusters)
    exit_with_error (
        "ezv_clustering_get_nth_cluster: invalid cluster index %u\n", n);

  *cluster = level_clustering->clusters [n];
}

// return cluster idx
int ezv_clustering_get_cluster_index_from_combo (ezv_clustering_t clustering,
                                                 unsigned level,
                                                 unsigned combo_idx)
{
  if (level >= clustering->nb_clusterings)
    exit_with_error (
        "ezv_clustering_get_cluster_index_from_combo: invalid level %u\n",
        level);
  ezv_level_clustering_t *level_clustering = &clustering->clusterings [level];
  if (level_clustering->reverse_mapping == NULL)
    exit_with_error (
        "ezv_clustering_get_cluster_index_from_combo: reverse mapping "
        "not available for level %u\n",
        level);

  if (combo_idx >= ezv_topo_get_nb_combos (level))
    exit_with_error (
        "ezv_clustering_get_cluster_index_from_combo: invalid combo index %u\n",
        combo_idx);

  return level_clustering->reverse_mapping [combo_idx];
}

unsigned ezv_clustering_get_cluster_index_from_thr (ezv_clustering_t clustering,
                                                    unsigned level,
                                                    unsigned thr)
{
  if (level >= clustering->nb_clusterings)
    exit_with_error (
        "ezv_clustering_get_cluster_index_from_thr: invalid level %u\n", level);
  ezv_level_clustering_t *level_clustering = &clustering->clusterings [level];
  if (level_clustering->clusters == NULL)
    exit_with_error ("ezv_clustering_get_cluster_index_from_thr: clustering "
                     "not available for level %u\n",
                     level);

  if (thr >= clustering->nb_thr)
    exit_with_error (
        "ezv_clustering_get_cluster_index_from_thr: invalid thread %u\n", thr);
  unsigned cluster_idx = level_clustering->thr_cluster [thr];
  if (cluster_idx >= level_clustering->nb_clusters)
    exit_with_error (
        "ezv_clustering_get_cluster_index_from_thr: invalid cluster index %u\n",
        cluster_idx);

  return cluster_idx;
}

unsigned ezv_clustering_get_cluster_weight (ezv_clustering_t clustering,
                                            unsigned level,
                                            unsigned cluster_idx)
{
  if (level >= clustering->nb_clusterings)
    exit_with_error ("ezv_clustering_get_cluster_weight: invalid level %u\n",
                     level);
  ezv_level_clustering_t *level_clustering = &clustering->clusterings [level];
  if (level_clustering->clusters == NULL)
    exit_with_error (
        "ezv_clustering_get_cluster_weight: clustering not available for "
        "level %u\n",
        level);

  if (cluster_idx >= level_clustering->nb_clusters)
    exit_with_error (
        "ezv_clustering_get_cluster_weight: invalid cluster index %u\n",
        cluster_idx);

  ezv_cluster_t *cluster = &level_clustering->clusters [cluster_idx];

  // return the number of PUs covered by this cluster
  unsigned weight = 0;
  for (unsigned c = 0; c < cluster->size; c++) {
    unsigned combo_idx = cluster->first_idx + c;
    weight += ezv_topo_get_combo_weight (level, combo_idx);
  }

  return weight;
}

int ezv_clustering_binding_exists (ezv_clustering_t clustering)
{
  return clustering->bindings != NULL;
}

static int index_of_englobing_cluster (ezv_level_clustering_t *level_clustering,
                                       unsigned index)
{
  for (unsigned c = 0; c < level_clustering->nb_clusters; c++) {
    ezv_cluster_t *cluster = &level_clustering->clusters [c];
    if (index >= cluster->first_idx &&
        index < cluster->first_idx + cluster->size) {
      return c;
    }
  }
  return -1;
}

// Create one cluster per thread
static int create_fake_level0_clustering (ezv_level_clustering_t *result,
                                          ezv_clustering_t clustering)
{
  result->clusters =
      (ezv_cluster_t *)malloc (clustering->nb_thr * sizeof (ezv_cluster_t));
  if (result->clusters == NULL)
    exit_with_error ("create_fake_level0_clustering: malloc failed\n");

  result->thr_cluster =
      (unsigned *)malloc (clustering->nb_thr * sizeof (unsigned));
  if (result->thr_cluster == NULL)
    exit_with_error ("create_fake_level0_clustering: malloc failed\n");

  result->reverse_mapping = NULL;

  result->nb_clusters = clustering->nb_thr;
  for (unsigned th = 0; th < clustering->nb_thr; th++) {
    result->clusters [th].first_idx = th;
    result->clusters [th].size      = 1;
    result->clusters [th].nb_thr    = 1;
    result->thr_cluster [th]        = th;
  }

  return 0;
}

static int create_level_clustering (ezv_level_clustering_t *result,
                                    unsigned level, ezv_clustering_t clustering)
{
  unsigned size = ezv_topo_get_nb_combos (level);

  // At most we will end up with one cluster per combo
  result->clusters = (ezv_cluster_t *)malloc (size * sizeof (ezv_cluster_t));
  if (result->clusters == NULL)
    exit_with_error ("create_level_clustering: malloc failed\n");

  result->thr_cluster =
      (unsigned *)malloc (clustering->nb_thr * sizeof (unsigned));
  if (result->thr_cluster == NULL)
    exit_with_error ("create_level_clustering: malloc failed\n");

  // reverse_mapping allow to find the cluster index from a combo index
  result->reverse_mapping = (int *)malloc (size * sizeof (int));
  if (result->reverse_mapping == NULL)
    exit_with_error ("create_level_clustering: malloc failed\n");
  for (unsigned c = 0; c < size; c++)
    result->reverse_mapping [c] = -1;

  result->nb_clusters = 0;
  // For each thread, find the cluster it belongs to or add a new one
  for (unsigned th = 0; th < clustering->nb_thr; th++) {
    unsigned start_combo_idx =
        ezv_topo_get_combo_for_pu (level, clustering->bindings [th].first_pu);
    unsigned end_combo_idx = ezv_topo_get_combo_for_pu (
        level, clustering->bindings [th].first_pu +
                   clustering->bindings [th].nb_pus - 1);
    int index = index_of_englobing_cluster (result, start_combo_idx);

    if (index == -1) {
      // Create a new cluster
      index                              = result->nb_clusters;
      result->clusters [index].first_idx = start_combo_idx;
      result->clusters [index].size      = end_combo_idx - start_combo_idx + 1;
      result->clusters [index].nb_thr    = 1;
      for (unsigned c = start_combo_idx; c <= end_combo_idx; c++) {
        result->reverse_mapping [c] = index;
        // printf ("Mapping combo %u to cluster %u (at level %d)\n", c, index,
        // level);
      }

      result->nb_clusters++;
    } else {
      // First check if existing cluster marches exactly the thread's combo
      // range
      ezv_cluster_t *cluster = &result->clusters [index];
      if (cluster->first_idx != start_combo_idx ||
          cluster->size != end_combo_idx - start_combo_idx + 1) {
        // we give up
        free (result->clusters);
        free (result->thr_cluster);
        free (result->reverse_mapping);
        result->clusters        = NULL;
        result->nb_clusters     = 0;
        result->thr_cluster     = NULL;
        result->reverse_mapping = NULL;
        ezv_topo_disable_folding (level);
        return -1;
      }
      // cluster matches the thread's combo range exactly: continue
      result->clusters [index].nb_thr++;
    }

    result->thr_cluster [th] = index;
    // printf ("Thread %u -> start_combo: %u #combos: %u\n", th,
    //         result->clusters [index].first_idx, result->clusters
    //         [index].size);
  }

  return 0;
}

void destroy_level_clustering (ezv_level_clustering_t *level_clustering)
{
  if (level_clustering->clusters != NULL)
    free (level_clustering->clusters);
  if (level_clustering->thr_cluster != NULL)
    free (level_clustering->thr_cluster);
  if (level_clustering->reverse_mapping != NULL)
    free (level_clustering->reverse_mapping);
}

ezv_clustering_t ezv_clustering_create (unsigned nb_thr,
                                        ezv_topo_pu_block_t *bindings)
{
  ezv_clustering_t clustering =
      (ezv_clustering_t)malloc (sizeof (struct ezv_clustering_struct));
  if (clustering == NULL)
    exit_with_error ("ezv_clustering_create: malloc failed\n");

  clustering->nb_thr   = nb_thr;
  clustering->bindings = bindings;

  // If no binding, use only one fake level-0 clustering
  unsigned levels            = bindings ? ezv_topo_get_maxdepth () : 1;
  clustering->nb_clusterings = levels;
  clustering->clusterings    = (ezv_level_clustering_t *)malloc (
      levels * sizeof (ezv_level_clustering_t));
  if (clustering->clusterings == NULL)
    exit_with_error ("ezv_clustering_create: malloc failed\n");

  if (bindings == NULL) {
    create_fake_level0_clustering (&clustering->clusterings [0], clustering);
    for (int l = 1; l < levels; l++)
      ezv_topo_disable_folding (l);
  } else
    for (int l = 0; l < levels; l++) {
      int ret =
          create_level_clustering (&clustering->clusterings [l], l, clustering);
      if (ret != 0) {
        fprintf (stderr, "Warning: Clustering disabled for level %d.\n", l);
      } else {
        //   printf ("Level %d: %u clusters created.\n", l,
        //           clustering->clusterings [l].nb_clusters);
      }
    }

  return clustering;
}

void ezv_clustering_destroy (ezv_clustering_t clustering)
{
  if (clustering != NULL) {
    for (unsigned l = 0; l < clustering->nb_clusterings; l++) {
      if (clustering->clusterings [l].clusters != NULL)
        free (clustering->clusterings [l].clusters);
    }
    free (clustering->clusterings);
    free (clustering);
  }
}
