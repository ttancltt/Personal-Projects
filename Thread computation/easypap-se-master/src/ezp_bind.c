#include "ezp_bind.h"
#include "constants.h"
#include "debug.h"
#include "error.h"
#include "ez_pthread.h"
#include "ezv_topo.h"
#include "min_max.h"

#include <omp.h>

static unsigned easypap_nbthreads = 0;

static hwloc_topology_t topology = NULL;

#define MAX_EZ_PTHREADS 512

static enum { EZ_BIND_SPREAD, EZ_BIND_CLOSE } ez_bind_policy = EZ_BIND_SPREAD;

static enum {
  EZ_POLICY_STRICT,
  EZ_POLICY_LOOSE,
  EZ_POLICY_WILD
} ez_policy = EZ_POLICY_LOOSE; // Loose binding by default

static hwloc_cpuset_t cpu_sets [MAX_EZ_PTHREADS]     = {NULL};
static hwloc_cpuset_t logical_sets [MAX_EZ_PTHREADS] = {NULL};
static unsigned nb_pre_bindings                      = 0;

static hwloc_cpuset_t *places = NULL;
static unsigned nb_places     = 0;

#define print_bitmap(set, format, ...)                                         \
  do {                                                                         \
    char *s;                                                                   \
    hwloc_bitmap_list_asprintf (&s, set);                                      \
    PRINT_DEBUG ('t', format ": %s\n", ##__VA_ARGS__, s);                      \
    free (s);                                                                  \
  } while (0)

unsigned ezp_bind_get_nbthreads (void)
{
  if (easypap_nbthreads == 0)
    exit_with_error ("ezp_bind_get_nbthreads: nbthreads not set");

  return easypap_nbthreads;
}

void ezp_bind_set_nbthreads (unsigned nbthreads)
{
  if (easypap_nbthreads != 0)
    exit_with_error ("ezp_bind_set_nbthreads: nbthreads already set to %u",
                     easypap_nbthreads);

  if (nbthreads > MAX_EZ_PTHREADS)
    exit_with_error ("Oh oh, current implementation does not support so many "
                     "(%d) cores : please increase value of MAX_EZ_PTHREADS "
                     "(currently set to %d) in internal/constants.h",
                     nbthreads, MAX_EZ_PTHREADS);

  easypap_nbthreads = nbthreads;
}

void ezp_bind_set_nbthreads_auto (void)
{
  if (easypap_nbthreads != 0)
    return;

  unsigned nbthreads = ezv_topo_get_nb_pus ();
  char *str          = getenv ("OMP_NUM_THREADS");

  if (str != NULL)
    nbthreads = atoi (str) ?: nbthreads;

  ezp_bind_set_nbthreads (nbthreads);
}

static char *policy_to_string (int policy)
{
  switch (policy) {
  case EZ_BIND_SPREAD:
    return ez_policy == EZ_POLICY_STRICT
               ? "spread (strict)"
               : (ez_policy == EZ_POLICY_LOOSE ? "spread (loose)"
                                               : "spread (wild)");
  case EZ_BIND_CLOSE:
    return ez_policy == EZ_POLICY_STRICT ? "close (strict)" : "close (loose)";
  default:
    return "unknown";
  }
}

static void configure_places_from_env (void)
{
  nb_places = 0;

  char *env = getenv ("OMP_PLACES");
  if (env == NULL)
    env = getenv ("EZ_PLACES");

  if (env == NULL)
    return;

  // there can not be more places than nb_pus
  const unsigned nb_pus     = ezv_topo_get_nb_pus ();
  const unsigned max_places = nb_pus;

  places = calloc (max_places, sizeof (hwloc_cpuset_t));

  // Duplicate the string to allow modification
  char *env_copy = strdup (env);
  char *ptr      = env_copy;

  // Skip whitespace
  while (*ptr == ' ' || *ptr == '\t')
    ptr++;

  // Check for abstract names: threads, cores, sockets, L2, L3 (optionally with
  // (N))
  if (!strncmp (ptr, "threads", 7) || !strncmp (ptr, "cores", 5) ||
      !strncmp (ptr, "sockets", 7) || !strncmp (ptr, "L2", 2) ||
      !strncmp (ptr, "L3", 2)) {
    hwloc_obj_type_t type;
    if (!strncmp (ptr, "threads", 7))
      type = HWLOC_OBJ_PU;
    else if (!strncmp (ptr, "cores", 5))
      type = HWLOC_OBJ_CORE;
    else if (!strncmp (ptr, "sockets", 7))
      type = HWLOC_OBJ_SOCKET;
    else if (!strncmp (ptr, "L2", 2))
      type = HWLOC_OBJ_L2CACHE;
    else
      type = HWLOC_OBJ_L3CACHE;

    int requested = -1;
    // Move pointer past word
    if (!strncmp (ptr, "threads", 7))
      ptr += 7;
    else if (!strncmp (ptr, "cores", 5))
      ptr += 5;
    else if (!strncmp (ptr, "sockets", 7))
      ptr += 7;
    else if (!strncmp (ptr, "L2", 2))
      ptr += 2;
    else
      ptr += 2; // L3
    while (*ptr == ' ' || *ptr == '\t')
      ptr++;
    if (*ptr == '(') {
      ptr++; // skip '('
      char *endp;
      long val = strtol (ptr, &endp, 10);
      if (endp != ptr && val > 0)
        requested = (int)val;
      // advance past optional spaces and closing ')'
      ptr = endp;
      while (*ptr == ' ' || *ptr == '\t')
        ptr++;
      if (*ptr == ')')
        ptr++;
    }

    int nb_objs = hwloc_get_nbobjs_by_type (topology, type);
    if (nb_objs <= 0) {
      PRINT_DEBUG ('t', "PLACES '%s': no objects of requested type\n", env);
      free (env_copy);
      return;
    }

    int limit = (requested > 0) ? requested : nb_objs;
    for (int i = 0; i < limit && nb_places < max_places; i++) {
      hwloc_obj_t obj = hwloc_get_obj_by_type (topology, type, i);
      if (!obj || !obj->cpuset)
        continue;
      places [nb_places++] = hwloc_bitmap_dup (obj->cpuset);
    }

    free (env_copy);
    PRINT_DEBUG ('t', "Configured %u places from PLACES '%s' (OS indices)\n",
                 nb_places, env);
    return;
  }

  // Parse explicit place list: e.g., "{0,1,2},{3,4,5}" or "{0:4}:4:4"
  while (*ptr && nb_places < max_places) {
    // Skip whitespace and commas
    while (*ptr == ' ' || *ptr == '\t' || *ptr == ',')
      ptr++;

    if (*ptr == '\0')
      break;

    // Check for exclusion operator
    int exclude = 0;
    if (*ptr == '!') {
      exclude = 1;
      ptr++;
      // Skip whitespace after !
      while (*ptr == ' ' || *ptr == '\t')
        ptr++;
    }

    // Parse a place
    if (*ptr == '{') {
      // Explicit place definition: {res-list}
      ptr++; // skip '{'

      hwloc_cpuset_t cpuset = hwloc_bitmap_alloc ();

      // Parse resource list
      while (*ptr && *ptr != '}') {
        // Skip whitespace and commas
        while (*ptr == ' ' || *ptr == '\t' || *ptr == ',')
          ptr++;

        if (*ptr == '}')
          break;

        // Check for exclusion in resource list
        int res_exclude = 0;
        if (*ptr == '!') {
          res_exclude = 1;
          ptr++;
          while (*ptr == ' ' || *ptr == '\t')
            ptr++;
        }

        // Parse resource number or interval
        char *endptr;
        long lower = strtol (ptr, &endptr, 10);

        if (endptr == ptr) {
          // No valid number
          hwloc_bitmap_free (cpuset);
          fprintf (stderr, "Warning: Invalid PLACES format at position: %s\n",
                   ptr);
          free (env_copy);
          return;
        }

        ptr = endptr;

        // Check for interval notation: lower:length[:stride]
        long length = 1;
        long stride = 1;

        if (*ptr == ':') {
          ptr++; // skip ':'
          length = strtol (ptr, &endptr, 10);
          if (endptr == ptr || length <= 0) {
            hwloc_bitmap_free (cpuset);
            free (env_copy);
            fprintf (stderr, "Warning: Invalid interval length in PLACES\n");
            return;
          }
          ptr = endptr;

          // Check for stride
          if (*ptr == ':') {
            ptr++; // skip second ':'
            stride = strtol (ptr, &endptr, 10);
            if (endptr == ptr) {
              hwloc_bitmap_free (cpuset);
              free (env_copy);
              fprintf (stderr, "Warning: Invalid stride in PLACES\n");
              return;
            }
            ptr = endptr;
          }
        }

        // Add resources to cpuset
        for (long i = 0; i < length; i++) {
          unsigned logical_index = lower + i * stride;
          if (logical_index < nb_pus) {
            // Convert logical index to OS index
            hwloc_obj_t pu =
                hwloc_get_obj_by_type (topology, HWLOC_OBJ_PU, logical_index);
            if (pu) {
              if (res_exclude)
                hwloc_bitmap_clr (cpuset, pu->os_index);
              else
                hwloc_bitmap_set (cpuset, pu->os_index);
            }
          }
        }
      }

      if (*ptr == '}')
        ptr++; // skip '}'

      if (!exclude && !hwloc_bitmap_iszero (cpuset)) {
        places [nb_places++] = cpuset;
      } else {
        hwloc_bitmap_free (cpuset);
      }

      // Check for place interval notation: place:length[:stride]
      if (*ptr == ':') {
        ptr++; // skip ':'
        char *endptr;
        long p_length = strtol (ptr, &endptr, 10);

        if (endptr != ptr && p_length > 0) {
          ptr = endptr;

          // Replicate the place pattern
          hwloc_cpuset_t base_cpuset = places [nb_places - 1];

          // Calculate footprint of the place using logical indices
          // footprint = last_logical_PU - first_logical_PU + 1
          hwloc_cpuset_t logical_base =
              ezv_topo_convert_os_to_logical_cpuset (base_cpuset);
          int first_logical_pu = hwloc_bitmap_first (logical_base);
          int last_logical_pu  = hwloc_bitmap_last (logical_base);
          long footprint       = (first_logical_pu >= 0 && last_logical_pu >= 0)
                                     ? (last_logical_pu - first_logical_pu + 1)
                                     : 1;
          hwloc_bitmap_free (logical_base);

          long p_stride = footprint; // Default stride is the footprint

          // Check for stride
          if (*ptr == ':') {
            ptr++; // skip ':'
            p_stride = strtol (ptr, &endptr, 10);
            if (endptr != ptr)
              ptr = endptr;
          }

          for (long i = 1; i < p_length && nb_places < max_places; i++) {
            hwloc_cpuset_t new_cpuset = hwloc_bitmap_alloc ();

            // Shift all CPUs in the base cpuset by (i * p_stride)
            int os_index = -1;
            while ((os_index = hwloc_bitmap_next (base_cpuset, os_index)) !=
                   -1) {
              // Convert OS index to logical, add stride, convert back to OS
              hwloc_obj_t pu =
                  hwloc_get_pu_obj_by_os_index (topology, os_index);
              if (pu) {
                unsigned new_logical = pu->logical_index + i * p_stride;
                if (new_logical < nb_pus) {
                  hwloc_obj_t new_pu = hwloc_get_obj_by_type (
                      topology, HWLOC_OBJ_PU, new_logical);
                  if (new_pu)
                    hwloc_bitmap_set (new_cpuset, new_pu->os_index);
                }
              }
            }

            if (!hwloc_bitmap_iszero (new_cpuset))
              places [nb_places++] = new_cpuset;
            else
              hwloc_bitmap_free (new_cpuset);
          }
        }
      }
    } else {
      // Invalid format
      free (env_copy);
      fprintf (stderr, "Warning: Invalid PLACES format, expected '{'\n");
      return;
    }
  }

  free (env_copy);
}

static void config_bind_policy (void)
{
  char *str;

  str = getenv ("EZ_POLICY");
  if (str != NULL) {
    if (!strcmp (str, "strict"))
      ez_policy = EZ_POLICY_STRICT;
    else if (!strcmp (str, "loose"))
      ez_policy = EZ_POLICY_LOOSE;
    else if (!strcmp (str, "wild"))
      ez_policy = EZ_POLICY_WILD;
    else
      exit_with_error ("Unknown value '%s' for POLICY environment variable. "
                       "Supported values are 'strict', 'loose', and 'wild'.",
                       str);
  }

  str = getenv ("OMP_PROC_BIND");
  if (str == NULL)
    str = getenv ("EZ_PROC_BIND");

  if (str != NULL) {
    if (!strcmp (str, "spread"))
      ez_bind_policy = EZ_BIND_SPREAD;
    else if (!strcmp (str, "close"))
      ez_bind_policy = EZ_BIND_CLOSE;
    else
      fprintf (stderr,
               "Warning: Unknown value '%s' for PROC_BIND environment "
               "variable. Supported values are 'spread' and 'close'.\n",
               str);
  } else if (nb_places > 0) {
    // If places have been configured, default to close
    ez_bind_policy = EZ_BIND_CLOSE;
  }

  PRINT_DEBUG ('t', "Binding policy: %s\n", policy_to_string (ez_bind_policy));
}

static void show_places (void)
{
  PRINT_DEBUG ('t', "Configured places (logical indices):\n");
  if (debug_flags && debug_enabled ('t')) {
    for (unsigned i = 0; i < nb_places; i++) {
      hwloc_cpuset_t logicset =
          ezv_topo_convert_os_to_logical_cpuset (places [i]);
      print_bitmap (logicset, "  Place %u", i);
      hwloc_bitmap_free (logicset);
    }
  }
}

static void singlify (hwloc_cpuset_t os_set)
{
  // convert into logical set
  hwloc_cpuset_t logical_set = ezv_topo_convert_os_to_logical_cpuset (os_set);
  // get first logical pu
  unsigned first_logical_pu = hwloc_bitmap_first (logical_set);
  hwloc_bitmap_free (logical_set);
  // convert back to os set
  hwloc_obj_t pu =
      hwloc_get_obj_by_type (topology, HWLOC_OBJ_PU, first_logical_pu);
  // make os_set singlify
  hwloc_bitmap_zero (os_set);
  hwloc_bitmap_set (os_set, pu->os_index);
}

static void compute_cpu_sets (unsigned max)
{
  configure_places_from_env ();

  show_places ();

  config_bind_policy ();

  char *str = getenv ("OMP_NUM_THREADS");
  if (str != NULL) {
    int n = atoi (str);
    if (n < 1)
      exit_with_error ("Invalid OMP_NUM_THREADS value: %s", str);
    if (max)
      ezp_bind_set_nbthreads (MIN (n, max));
    else
      ezp_bind_set_nbthreads (n);
  } else {
    if (nb_places > 0)
      if (max)
        ezp_bind_set_nbthreads (MIN (max, nb_places));
      else
        ezp_bind_set_nbthreads (nb_places);
    else
      ezp_bind_set_nbthreads (ezv_topo_get_nb_pus ());
  }

  nb_pre_bindings = easypap_nbthreads;

  // If places have been configured from PLACES, use them to build cpu_sets
  if (nb_places > 0) {
    if (ez_bind_policy == EZ_BIND_SPREAD) {
      // Spread policy: create sparse distribution
      unsigned T = nb_pre_bindings; // Number of threads
      unsigned P = nb_places;       // Number of places

      if (T <= P) {
        // T <= P: Split P places into T subpartitions
        // Each subpartition contains floor(P/T) or ceil(P/T) consecutive places
        for (unsigned t = 0; t < T; t++) {
          unsigned subpart_start = (t * P) / T;

          if (ez_policy == EZ_POLICY_WILD) {
            // Wild: bind to the union of places in the subpartition (sparse)
            unsigned subpart_end  = ((t + 1) * P) / T;
            hwloc_cpuset_t os_set = hwloc_bitmap_alloc ();
            for (unsigned p = subpart_start; p < subpart_end; p++)
              hwloc_bitmap_or (os_set, os_set, places [p]);
            cpu_sets [t] = os_set;
          } else {
            // Loose/strict: bind to the first place of the subpartition
            hwloc_cpuset_t os_set = hwloc_bitmap_dup (places [subpart_start]);
            if (ez_policy == EZ_POLICY_STRICT)
              singlify (os_set);
            cpu_sets [t] = os_set;
          }
        }
      } else {
        // T > P: Split P places into P subpartitions (one place each)
        // Assign floor(T/P) or ceil(T/P) threads to each subpartition
        for (unsigned t = 0; t < T; t++) {
          // Determine which place this thread belongs to
          unsigned place_idx = (t * P) / T;

          hwloc_cpuset_t os_set = hwloc_bitmap_dup (places [place_idx]);
          if (ez_policy == EZ_POLICY_STRICT)
            singlify (os_set);

          cpu_sets [t] = os_set;
        }
      }
    } else {
      // Close policy (default)
      for (unsigned i = 0; i < nb_pre_bindings; i++) {
        hwloc_cpuset_t os_set = hwloc_bitmap_dup (places [i % nb_places]);
        if (ez_policy == EZ_POLICY_STRICT)
          singlify (os_set);
        cpu_sets [i] = os_set;
      }
    }

    // Convert to logical sets for reporting/queries
    for (unsigned i = 0; i < nb_pre_bindings; i++)
      logical_sets [i] = ezv_topo_convert_os_to_logical_cpuset (cpu_sets [i]);

    return;
  }

  // PLACES not set, so use hwloc_distrib for spread, and round-robin for
  // close
  if (ez_bind_policy == EZ_BIND_SPREAD) {
    hwloc_obj_t root = hwloc_get_root_obj (topology);

    // Try to spread threads on different cores/PU
    hwloc_distrib (topology, &root, 1, cpu_sets, nb_pre_bindings, INT_MAX, 0);

    for (int i = 0; i < nb_pre_bindings; i++) {
      if (ez_policy == EZ_POLICY_STRICT)
        singlify (cpu_sets [i]);
    }
  } else if (ez_bind_policy == EZ_BIND_CLOSE) {
    unsigned nb_pus = ezv_topo_get_nb_pus ();

    for (int i = 0; i < nb_pre_bindings; i++) {
      unsigned logical = i % nb_pus;
      cpu_sets [i]     = hwloc_bitmap_dup (
          hwloc_get_obj_by_depth (
              topology, hwloc_get_type_depth (topology, HWLOC_OBJ_PU), logical)
              ->cpuset);
    }
  } else
    exit_with_error ("Internal error: unknown binding policy %d",
                     ez_bind_policy);

  // Convert to logical sets
  for (int i = 0; i < nb_pre_bindings; i++)
    logical_sets [i] = ezv_topo_convert_os_to_logical_cpuset (cpu_sets [i]);
}

static void ezp_bind_get_openmp_cpusets (void)
{
  unsigned failed = 0;

#pragma omp parallel shared(logical_sets) reduction(| : failed)
  {
    const unsigned me  = omp_get_thread_num ();
    hwloc_cpuset_t tmp = hwloc_bitmap_alloc ();

    // get cpuset of thread
    if (hwloc_get_cpubind (topology, tmp, HWLOC_CPUBIND_THREAD) == 0)
      logical_sets [me] = ezv_topo_convert_os_to_logical_cpuset (tmp);
    else
      failed = 1;
    hwloc_bitmap_free (tmp);
  }

  if (!failed)
    nb_pre_bindings = ezp_bind_get_nbthreads ();
}

void ezp_bind_get_logical_cpusets (hwloc_cpuset_t **sets, unsigned *count)
{
  *sets  = logical_sets;
  *count = nb_pre_bindings;
}

void ezp_bind_self (unsigned id)
{
  if (nb_pre_bindings == 0)
    exit_with_error ("No pre-binding information available in ezp_bind_self");

  if (id >= nb_pre_bindings)
    exit_with_error ("Thread id %u exceeds number of pre-computed bindings %u "
                     "in ezp_bind_self",
                     id, nb_pre_bindings);

  hwloc_set_cpubind (topology, cpu_sets [id], HWLOC_CPUBIND_THREAD);
}

void ezp_bind_init (unsigned max, ezp_bind_mode_t mode)
{
  static int initialized = 0;

  // check if called twice
  if (initialized)
    return;
  initialized = 1;

  topology = ezv_topo_get_topology ();

  if (mode == EZP_BIND_MODE_SET)
    compute_cpu_sets (max);
  else {
    // We assume OpenMP by default in GET mode
    ezp_bind_set_nbthreads_auto ();

    if (getenv ("OMP_PLACES") || getenv ("OMP_PROC_BIND"))
      // Neither ez_pthread_init nor ez_tbb_init where called
      // So try to get OpenMP bindings if any
      ezp_bind_get_openmp_cpusets ();
  }
}

void ezp_bind_finalize (void)
{
  for (int i = 0; i < nb_pre_bindings; i++) {
    if (cpu_sets [i])
      hwloc_bitmap_free (cpu_sets [i]);
    if (logical_sets [i])
      hwloc_bitmap_free (logical_sets [i]);
  }
  nb_pre_bindings = 0;

  // Free PLACES resources if any
  if (places) {
    for (unsigned i = 0; i < nb_places; i++)
      if (places [i])
        hwloc_bitmap_free (places [i]);
    free (places);
    places    = NULL;
    nb_places = 0;
  }
}
