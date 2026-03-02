
#define _GNU_SOURCE
#include <hwloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "ezv_topo.h"

typedef struct
{
  unsigned inf, sup;
} ezv_topo_range_t;

typedef struct
{
  int adjusted_depth;
  unsigned skipped;
} ezv_topo_depthinfo_t;

typedef struct
{
  ezv_topo_pu_block_t coverage;
} ezv_topo_combo_t;

typedef struct
{
  unsigned enabled;         // is this folding enabled?
  unsigned nb_combos;       // number of PU combinations at this level
  ezv_topo_combo_t *combos; // array of PU combinations at this level
  unsigned depth;           // total depth of this folding
  unsigned *pu_to_combo;    // mapping PU logical index -> combo index
} ezv_topo_folding_t;

static hwloc_topology_t topology         = NULL;
static ezv_topo_depthinfo_t *topo_levels = NULL;
static unsigned total_depth              = 0;
static unsigned nb_pus                   = 0;
static unsigned nb_levels_skipped        = 0;
unsigned *ezv_core_kinds                 = NULL;
int nb_pu_kinds                          = 1;

static unsigned nb_foldings         = 1;
static ezv_topo_folding_t *foldings = NULL;

static unsigned topo_loaded_from_file = 0;
static char topo_filename [PATH_MAX];

unsigned ezv_topo_get_nb_pu_kinds (void)
{
  return nb_pu_kinds;
}

unsigned ezv_topo_get_pu_efficiency (unsigned pu_logical_index)
{
  if (ezv_core_kinds && pu_logical_index < nb_pus) {
    return ezv_core_kinds [pu_logical_index];
  } else {
    return 0;
  }
}

static int traverse (hwloc_obj_t obj, unsigned target_depth,
                     ezv_topo_callback_t func, ezv_topo_range_t *range,
                     void *arg)
{
  if (topo_levels [obj->depth].skipped)
    return traverse (obj->children [0], target_depth, func, range, arg);

  if (target_depth < topo_levels [obj->depth].adjusted_depth)
    // We've gone too deep
    return 1;

  if (target_depth == topo_levels [obj->depth].adjusted_depth) {
    range->inf = range->sup = obj->logical_index;
  } else {
    ezv_topo_range_t ranges [obj->arity];

    for (int i = 0; i < obj->arity; i++) {
      int r = traverse (obj->children [i], target_depth, func, ranges + i, arg);
      if (r > 0) {
        // our child is farther than target_depth, fill-in the range
        ranges [i].inf = (i > 0) ? ranges [i - 1].sup + 1 : 0;
        ranges [i].sup = ranges [i].inf;
      }
    }
    range->inf = ranges [0].inf;
    range->sup = ranges [obj->arity - 1].sup;
  }

  func (obj, topo_levels [obj->depth].adjusted_depth, range->inf, range->sup,
        arg);

  return 0;
}

void ezv_topo_for_each_obj (unsigned folding_level, ezv_topo_callback_t func,
                            void *arg)
{
  ezv_topo_range_t range = {0, 0};
  unsigned target_depth = (total_depth - nb_levels_skipped) - 1 - folding_level;

  traverse (hwloc_get_root_obj (topology), target_depth, func, &range, arg);
}

static int level_can_be_skipped (hwloc_topology_t topology, int depth)
{
  unsigned i;
  hwloc_obj_t obj;

  for (i = 0; i < hwloc_get_nbobjs_by_depth (topology, depth); i++) {
    obj = hwloc_get_obj_by_depth (topology, depth, i);
    if (obj->arity > 1)
      return 0;
  }

  return 1;
}

static void prune_levels (void)
{
  topo_levels = malloc (total_depth * sizeof (ezv_topo_depthinfo_t));
  if (!topo_levels)
    exit_with_error ("cannot alloc topo_levels array");

  for (unsigned i = 0; i < total_depth - 1; i++) {
    topo_levels [i].skipped = level_can_be_skipped (topology, i);
    if (topo_levels [i].skipped) {
      nb_levels_skipped++;
      // printf ("Objects at depth %u can be skipped…\n", i);
      topo_levels [i].adjusted_depth = -1;
    } else
      topo_levels [i].adjusted_depth = i - nb_levels_skipped;
  }
  topo_levels [total_depth - 1].skipped = 0; /* never skip the last level */
  topo_levels [total_depth - 1].adjusted_depth =
      total_depth - 1 - nb_levels_skipped;
}

static void count_combo (unsigned level, ezv_topo_range_t *range)
{
  foldings [level].nb_combos++;
}

static void add_combo (unsigned level, ezv_topo_range_t *range)
{
  ezv_topo_pu_block_t *block =
      &foldings [level].combos [foldings [level].nb_combos].coverage;

  block->first_pu = range->inf;
  block->nb_pus   = range->sup - range->inf + 1;
  foldings [level].nb_combos++;
}

static unsigned traverse_and_build (hwloc_obj_t obj,
                                    void (*func) (unsigned level,
                                                  ezv_topo_range_t *range),
                                    ezv_topo_range_t *range)
{
  if (topo_levels [obj->depth].skipped) {
    return traverse_and_build (obj->children [0], func, range);
  }

  unsigned depth = topo_levels [obj->depth].adjusted_depth;
  unsigned level = total_depth - nb_levels_skipped - depth - 1;

  if (obj->depth == total_depth - 1) {
    range->inf = range->sup = obj->logical_index;
  } else {
    ezv_topo_range_t ranges [obj->arity];
    for (int i = 0; i < obj->arity; i++) {
      unsigned d = traverse_and_build (obj->children [i], func, ranges + i);
      // Fill-in intermediate levels
      for (unsigned n = depth + 1; n < d; n++) {
        unsigned intermediate_level = total_depth - nb_levels_skipped - n - 1;
        ezv_topo_range_t intermediate_range = {ranges [i].inf, ranges [i].sup};
        func (intermediate_level, &intermediate_range);
      }
    }
    range->inf = ranges [0].inf;
    range->sup = ranges [obj->arity - 1].sup;
  }

  func (level, range);

  return depth;
}

static void build_foldings (void)
{
  nb_foldings = total_depth - nb_levels_skipped;
  foldings    = malloc (nb_foldings * sizeof (ezv_topo_folding_t));
  if (!foldings)
    exit_with_error ("cannot alloc foldings array");

  unsigned max_adjusted_depth = total_depth - nb_levels_skipped - 1;

  ezv_topo_range_t range = {0, 0};

  // Pre-allocate combos arrays
  for (unsigned folding_level = 0; folding_level < nb_foldings;
       folding_level++) {
    unsigned target_depth = max_adjusted_depth - folding_level;

    foldings [folding_level].depth     = target_depth;
    foldings [folding_level].enabled   = 1;
    foldings [folding_level].nb_combos = 0;
  }

  // We first traverse the topology to count the number of combos at each level
  traverse_and_build (hwloc_get_root_obj (topology), count_combo, &range);

  for (unsigned folding_level = 0; folding_level < nb_foldings;
       folding_level++) {
    foldings [folding_level].combos =
        malloc (foldings [folding_level].nb_combos * sizeof (ezv_topo_combo_t));
    // Reset nb_combos to use it as an index during the second traversal
    foldings [folding_level].nb_combos = 0;
  }

  // Second traversal to fill-in the combos arrays
  traverse_and_build (hwloc_get_root_obj (topology), add_combo, &range);

  for (unsigned folding_level = 0; folding_level < nb_foldings;
       folding_level++) {

    // Allouer et initialiser le mapping PU -> combo
    foldings [folding_level].pu_to_combo = malloc (nb_pus * sizeof (unsigned));

    // Remplir le mapping PU -> combo
    for (unsigned combo_idx = 0; combo_idx < foldings [folding_level].nb_combos;
         combo_idx++) {
      unsigned first_pu =
          foldings [folding_level].combos [combo_idx].coverage.first_pu;
      unsigned nb_pus_in_combo =
          foldings [folding_level].combos [combo_idx].coverage.nb_pus;
      for (unsigned pu = 0; pu < nb_pus_in_combo; pu++) {
        foldings [folding_level].pu_to_combo [first_pu + pu] = combo_idx;
      }
    }
  }
}

void print_folding (unsigned folding_level)
{
  if (folding_level >= nb_foldings)
    exit_with_error ("print_folding: invalid folding level %u", folding_level);

  printf ("Folding level %u (depth %u): %u combos, %s\n", folding_level,
          foldings [folding_level].depth, foldings [folding_level].nb_combos,
          foldings [folding_level].enabled ? "enabled" : "disabled");
  for (unsigned c = 0; c < foldings [folding_level].nb_combos; c++) {
    ezv_topo_pu_block_t *block = &foldings [folding_level].combos [c].coverage;
    if (block->nb_pus == 1)
      printf ("  Combo %2u: PU #%u\n", c, block->first_pu);
    else
      printf ("  Combo %2u: PU #%u-#%u\n", c, block->first_pu,
              block->first_pu + block->nb_pus - 1);
  }
}

void print_foldings (void)
{
  for (unsigned f = 0; f < nb_foldings; f++)
    print_folding (f);
}

static void __init (void)
{
  // Load the actual topology
  int err = hwloc_topology_load (topology);
  if (err)
    exit_with_error ("hwloc_topology_load() failed: %s", strerror (err));

  total_depth = hwloc_topology_get_depth (topology);
  nb_pus      = hwloc_get_nbobjs_by_type (topology, HWLOC_OBJ_PU);

  nb_pu_kinds = hwloc_cpukinds_get_nr (topology, 0);

  if (nb_pu_kinds > 1) {
    ezv_core_kinds        = malloc (nb_pus * sizeof (unsigned));
    hwloc_bitmap_t bitmap = hwloc_bitmap_alloc ();
    for (int kind = 0; kind < nb_pu_kinds; kind++) {
      int efficiency = 0;

      if (hwloc_cpukinds_get_info (topology, kind, bitmap, &efficiency, 0, NULL,
                                   0) != 0) {
        exit_with_error ("hwloc_cpukinds_get_info() failed for kind %d: %s",
                         kind, strerror (err));
      }

      int os_index;
      hwloc_bitmap_foreach_begin (os_index, bitmap)
      {
        hwloc_obj_t pu = hwloc_get_pu_obj_by_os_index (topology, os_index);
        ezv_core_kinds [pu->logical_index] = efficiency;
      }
      hwloc_bitmap_foreach_end ();
    }
    hwloc_bitmap_free (bitmap);
  } else
    nb_pu_kinds = 1;

  prune_levels ();

  build_foldings ();

  // print_foldings ();
}

static int init_done = 0;

void ezv_topo_init (void)
{
  if (init_done)
    exit_with_error ("ezv_topo_init: already initialized");
  init_done = 1;

  // Allocate and initialize hwloc topology object
  int err = hwloc_topology_init (&topology);
  if (err)
    exit_with_error ("hwloc_topology_init() failed: %s", strerror (err));

  __init ();
}

int ezv_topo_init_from_file (const char *filename)
{
  if (init_done)
    exit_with_error ("ezv_topo_init_from_file: already initialized");
  init_done = 1;

  // Allocate and initialize hwloc topology object
  int err = hwloc_topology_init (&topology);
  if (err)
    exit_with_error ("hwloc_topology_init() failed: %s", strerror (err));

  // Load the actual topology from file
  err = hwloc_topology_set_xml (topology, filename);
  if (err)
    return err;

  topo_loaded_from_file = 1;
  strncpy (topo_filename, filename, PATH_MAX - 1);
  topo_filename [PATH_MAX - 1] = '\0';

  __init ();

  return 0;
}

int ezv_topo_is_initialized (void)
{
  return init_done;
}

void ezv_topo_finalize (void)
{
  // Destroy topology object
  if (topology) {
    hwloc_topology_destroy (topology);
    topology = NULL;
  }

  if (topo_levels) {
    free (topo_levels);
    topo_levels = NULL;
  }

  if (ezv_core_kinds) {
    free (ezv_core_kinds);
    ezv_core_kinds = NULL;
  }

  if (foldings) {
    free (foldings);
    foldings = NULL;
  }
}

hwloc_topology_t ezv_topo_get_topology (void)
{
  return topology;
}

unsigned ezv_topo_get_depth (unsigned folding_level)
{
  if (folding_level < nb_foldings) {
    return foldings [folding_level].depth + 1;
  } else
    exit_with_error ("ezv_topo_get_depth: invalid folding level %u",
                     folding_level);
}

unsigned ezv_topo_get_maxdepth (void)
{
  if (topology == NULL)
    return 1;

  return total_depth - nb_levels_skipped;
}

unsigned ezv_topo_get_nb_pus (void)
{
  return nb_pus;
}

unsigned ezv_topo_get_nb_combos (unsigned folding_level)
{
  if (!foldings)
    exit_with_error ("ezv_topo_get_nb_combos: topology not initialized");

  if (folding_level < nb_foldings) {
    return foldings [folding_level].nb_combos;
  } else
    exit_with_error ("ezv_topo_get_nb_combos: invalid folding level %u",
                     folding_level);
}

unsigned ezv_topo_get_combo_for_pu (unsigned folding_level,
                                    unsigned pu_logical_index)
{
  if (!foldings)
    exit_with_error ("ezv_topo_get_combo_for_pu: topology not initialized");

  if (folding_level < nb_foldings) {
    if (pu_logical_index < nb_pus) {
      return foldings [folding_level].pu_to_combo [pu_logical_index];
    } else
      exit_with_error ("ezv_topo_get_combo_for_pu: invalid PU logical index %u",
                       pu_logical_index);
  } else
    exit_with_error ("ezv_topo_get_combo_for_pu: invalid folding level %u",
                     folding_level);
}

void ezv_topo_get_combo_coverage (unsigned folding_level, unsigned combo_index,
                                  ezv_topo_pu_block_t *block)
{
  if (!foldings)
    exit_with_error ("ezv_topo_get_combo_coverage: topology not initialized");

  if (folding_level < nb_foldings) {
    if (combo_index < foldings [folding_level].nb_combos) {
      *block = foldings [folding_level].combos [combo_index].coverage;
    } else
      exit_with_error ("ezv_topo_get_combo_coverage: invalid combo index %u "
                       "for folding level %u",
                       combo_index, folding_level);
  } else
    exit_with_error ("ezv_topo_get_combo_coverage: invalid folding level %u",
                     folding_level);
}

unsigned ezv_topo_get_combo_weight (unsigned folding_level,
                                    unsigned combo_index)
{
  if (!foldings)
    exit_with_error ("ezv_topo_get_combo_coverage: topology not initialized");

  if (folding_level < nb_foldings) {
    if (combo_index < foldings [folding_level].nb_combos) {
      return foldings [folding_level].combos [combo_index].coverage.nb_pus;
    } else
      exit_with_error ("ezv_topo_get_combo_coverage: invalid combo index %u "
                       "for folding level %u",
                       combo_index, folding_level);
  } else
    exit_with_error ("ezv_topo_get_combo_coverage: invalid folding level %u",
                     folding_level);
}

void ezv_topo_disable_folding (unsigned folding_level)
{
  if (!foldings)
    exit_with_error ("ezv_topo_get_combo_coverage: topology not initialized");

  if (folding_level < nb_foldings) {
    foldings [folding_level].enabled = 0;
  }
}

int ezv_topo_first_valid_folding (void)
{
  if (!foldings)
    return 0;

  for (unsigned l = 0; l < nb_foldings; l++) {
    if (foldings [l].enabled)
      return l;
  }
  return -1;
}

int ezv_topo_next_valid_folding (unsigned folding_level)
{
  if (!foldings)
    return -1;

  for (unsigned l = folding_level + 1; l < nb_foldings; l++) {
    if (foldings [l].enabled)
      return l;
  }
  return -1;
}

int ezv_topo_prev_valid_folding (unsigned folding_level)
{
  if (!foldings)
    return -1;

  if (folding_level == 0)
    return -1;

  for (int l = folding_level - 1; l >= 0; l--) {
    if (foldings [l].enabled)
      return l;
  }
  return -1;
}

hwloc_cpuset_t ezv_topo_convert_os_to_logical_cpuset (hwloc_cpuset_t os_set)
{
  if (topology == NULL)
    exit_with_error (
        "ezv_topo_convert_os_to_logical_cpuset: topology not initialized");

  hwloc_cpuset_t logical_set = hwloc_bitmap_alloc ();
  int os_index;

  hwloc_bitmap_foreach_begin (os_index, os_set)
  {
    hwloc_obj_t pu = hwloc_get_pu_obj_by_os_index (topology, os_index);
    if (pu)
      hwloc_bitmap_set (logical_set, pu->logical_index);
  }
  hwloc_bitmap_foreach_end ();

  return logical_set;
}

static int ezv_topo_cpuset_contiguous (hwloc_cpuset_t set)
{
  if (hwloc_bitmap_iszero (set))
    return 0;

  int first = hwloc_bitmap_first (set);
  int last  = hwloc_bitmap_last (set);

  return hwloc_bitmap_weight (set) == last - first + 1;
}

int ezv_topo_cpusets_contiguous_and_disjoint (hwloc_cpuset_t *sets,
                                              unsigned nsets)
{
  hwloc_cpuset_t union_set = hwloc_bitmap_alloc ();

  for (unsigned i = 0; i < nsets; i++) {
    if (!ezv_topo_cpuset_contiguous (sets [i]) ||
        hwloc_bitmap_intersects (union_set, sets [i])) {
      hwloc_bitmap_free (union_set);
      return 0;
    }
    hwloc_bitmap_or (union_set, union_set, sets [i]);
  }

  hwloc_bitmap_free (union_set);
  return 1;
}

void ezv_topo_cpuset_to_pu_block (hwloc_cpuset_t set,
                                  ezv_topo_pu_block_t *block)
{
  if (!ezv_topo_cpuset_contiguous (set))
    exit_with_error ("ezv_topo_cpuset_to_pu_block: cpuset is not contiguous");

  block->first_pu = hwloc_bitmap_first (set);
  block->nb_pus   = hwloc_bitmap_weight (set);
}

void ezv_topo_save_to_file (const char *filename)
{
  if (topology == NULL)
    exit_with_error (
        "ezv_topo_convert_os_to_logical_cpuset: topology not initialized");

  int err = hwloc_topology_export_xml (topology, filename, 0);
  if (err)
    exit_with_error ("ezv_topo: cannot save topology to file %s (%s)", filename,
                     strerror (err));
}

void ezv_topo_save_if_needed (const char *directory, char *fullname,
                              size_t size)
{
  if (topo_loaded_from_file) {
    // Use existing file: don't save, just
    strncpy (fullname, topo_filename, size - 1);
    fullname [size - 1] = '\0';
  } else {
    char machine [256];
    char filename [PATH_MAX];

    gethostname (machine, sizeof (machine));
    snprintf (filename, sizeof (filename), "%s/%s.xml", directory, machine);

    if (strlen (filename) >= size)
      exit_with_error (
          "ezv_topo_save: fullname buffer too small to host filename\n"
          "  (%s)",
          filename);

    strncpy (fullname, filename, size - 1);
    fullname [size - 1] = '\0';

    // Check if topology file exists for this machine
    if (access (filename, F_OK) == -1) {
      // file doesn't exist, so create it
      ezv_topo_save_to_file (filename);
    }
  }
}

char *ezv_topo_get_topo_filename (void)
{
  if (topo_loaded_from_file)
    return topo_filename;
  else
    return NULL;
}

int ezv_topo_get_current_cpuset (hwloc_cpuset_t cpuset)
{
  if (topology == NULL)
    return -1;

  return hwloc_get_cpubind (topology, cpuset, HWLOC_CPUBIND_THREAD);
}

int ezv_topo_set_current_cpuset (hwloc_cpuset_t cpuset)
{
  if (topology == NULL)
    return -1;

  return hwloc_set_cpubind (topology, cpuset, HWLOC_CPUBIND_THREAD);
}
