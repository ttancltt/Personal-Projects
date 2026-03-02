
#include <hwloc.h>
#include <omp.h>
#include <stdio.h>
#include <string.h>

#include "api_funcs.h"
#include "constants.h"
#include "debug.h"
#include "error.h"
#include "ez_pthread.h"
#include "ezm.h"
#include "ezp_bind.h"
#include "ezp_ctx.h"
#include "ezv_topo.h"
#include "global.h"
#include "img_data.h"
#include "mesh_data.h"
#include "monitoring.h"
#include "private_glob.h"

ezm_recorder_t ezp_monitor = NULL;

char easypap_trace_label [MAX_LABEL] = {0};

unsigned do_trace              = 0;
unsigned trace_may_be_used     = 0;
unsigned do_gmonitor           = 0;
ezv_mon_mode_t monitoring_mode = EZV_MONITOR_DEFAULT;

// Thread binding (logical indexes)
static unsigned ezp_nb_bindings          = 0;
static ezv_topo_pu_block_t *ezp_bindings = NULL;
unsigned do_gmonitor_big_footprint = 0; // -M option: swap Image/Footprint sizes

static void set_default_trace_label (void)
{
  if (easypap_trace_label [0] == '\0') {
    char *str = getenv ("OMP_SCHEDULE");

    if (easypap_mode == EASYPAP_MODE_2D_IMAGES) {
      if (str != NULL)
        snprintf (easypap_trace_label, MAX_LABEL, "%s %s %s (%s) %d/%dx%d",
                  kernel_name, variant_name,
                  strcmp (tile_name, "none") ? tile_name : "", str, DIM, TILE_W,
                  TILE_H);
      else
        snprintf (easypap_trace_label, MAX_LABEL, "%s %s %s %d/%dx%d",
                  kernel_name, variant_name,
                  strcmp (tile_name, "none") ? tile_name : "", DIM, TILE_W,
                  TILE_H);
    } else {
      if (str != NULL)
        snprintf (easypap_trace_label, MAX_LABEL, "%s %s %s (%s) %d/%d",
                  kernel_name, variant_name,
                  strcmp (tile_name, "none") ? tile_name : "", str, NB_CELLS,
                  NB_PATCHES);
      else
        snprintf (easypap_trace_label, MAX_LABEL, "%s %s %s %d/%d", kernel_name,
                  variant_name, strcmp (tile_name, "none") ? tile_name : "",
                  NB_CELLS, NB_PATCHES);
    }
  }
}

static void ezp_show_bindings (void)
{
  if (debug_flags && debug_enabled ('t')) {
    if (ezp_nb_bindings > 0) {
      PRINT_DEBUG ('t', "Thread bindings:\n");
      for (unsigned i = 0; i < ezp_nb_bindings; i++) {
        if (ezp_bindings [i].nb_pus == 1)
          PRINT_DEBUG ('t', "  Thread %3u -> PU %u\n", i,
                       ezp_bindings [i].first_pu);
        else
          PRINT_DEBUG ('t', "  Thread %3u -> PU %u-%u\n", i,
                       ezp_bindings [i].first_pu,
                       ezp_bindings [i].first_pu + ezp_bindings [i].nb_pus - 1);
      }
    } else
      PRINT_DEBUG ('t', "No thread binding detected\n");
  }
}

static void ezp_inspect_locality (void)
{
  unsigned nb_bindings     = 0;
  hwloc_cpuset_t *bindings = NULL;

  ezp_bind_get_logical_cpusets (&bindings, &nb_bindings);

  if (nb_bindings > 0) {
    PRINT_DEBUG ('t', "Thread binding detected\n");

    if (ezv_topo_cpusets_contiguous_and_disjoint (bindings, nb_bindings)) {
      ezp_nb_bindings = nb_bindings;
      ezp_bindings    = malloc (nb_bindings * sizeof (ezv_topo_pu_block_t));
      for (unsigned i = 0; i < nb_bindings; i++)
        ezv_topo_cpuset_to_pu_block (bindings [i], &ezp_bindings [i]);

      ezp_show_bindings ();
    } else {
      PRINT_DEBUG ('t',
                   "Warning: Overlapping Thread cpusets are not supported\n");
      if (debug_flags && debug_enabled ('t'))
        for (unsigned i = 0; i < nb_bindings; i++) {
          char *s;
          // print cpusets for debug
          hwloc_bitmap_list_asprintf (&s, bindings [i]);
          PRINT_DEBUG ('t', "  Thread %3u -> cpuset %s\n", i, s);
          free (s);
        }
    }
  } else
    PRINT_DEBUG ('t', "No thread binding detected\n");
}

static void ezp_configure_monitoring_mode (void)
{
  unsigned nb_cores = ezv_topo_get_nb_pus ();
  unsigned nb_thr   = ezp_bind_get_nbthreads ();

  if (monitoring_mode == EZV_MONITOR_TOPOLOGY_COMPACT) {
    PRINT_DEBUG ('t', "Warning: EZV_MONITOR_TOPOLOGY_COMPACT mode is not yet "
                      "supported. Using EZV_MONITOR_TOPOLOGY instead.\n");
    monitoring_mode = EZV_MONITOR_TOPOLOGY;
  }

  if (monitoring_mode == EZV_MONITOR_TOPOLOGY && nb_thr > nb_cores) {
    PRINT_DEBUG ('t',
                 "Warning: Number of threads (%d) exceeds available cores (%d)."
                 " Falling back to EZV_MONITOR_DEFAULT.\n",
                 nb_thr, nb_cores);
    monitoring_mode = EZV_MONITOR_DEFAULT;
  }

  if (trace_may_be_used || monitoring_mode == EZV_MONITOR_TOPOLOGY)
    ezp_inspect_locality ();

  if (ezp_bindings == NULL) {
    if (trace_may_be_used) {
      PRINT_DEBUG ('t', "Warning: No thread binding recorded in the trace.\n");
    } else if (monitoring_mode == EZV_MONITOR_TOPOLOGY &&
               ezp_bindings == NULL) {
      PRINT_DEBUG (
          't', "Warning: EZV_MONITOR_TOPOLOGY mode requires thread bindings. "
               "Falling back to EZV_MONITOR_DEFAULT.\n");
      monitoring_mode = EZV_MONITOR_DEFAULT;
    }
  }
}

void ezp_monitoring_init (unsigned nb_thr, unsigned nb_gpus)
{
  ezm_init (do_display ? 0 : EZM_NO_DISPLAY);

  ezp_configure_monitoring_mode ();

  ezp_monitor = ezm_recorder_create (nb_thr, nb_gpus, ezp_bindings);

#ifdef ENABLE_TRACE
  if (trace_may_be_used) {
    char filename [PATH_MAX];

    if (easypap_mpirun)
      snprintf (filename, PATH_MAX, "data/traces/ezv_trace_current.%d.evt",
                easypap_mpi_rank ());
    else
      strcpy (filename, "data/traces/ezv_trace_current.evt");

    set_default_trace_label ();

    ezm_recorder_attach_tracerec (ezp_monitor, filename, easypap_trace_label);

    if (easypap_mode == EASYPAP_MODE_3D_MESHES) {
      ezm_recorder_store_mesh3d_filename (ezp_monitor, easypap_mesh_file);

      ezv_palette_name_t pal = mesh_data_get_palette ();
      if (pal != EZV_PALETTE_UNDEFINED && pal != EZV_PALETTE_CUSTOM)
        ezm_recorder_store_data_palette (ezp_monitor, pal);
    } else {
      ezm_recorder_store_img2d_dim (ezp_monitor, DIM, DIM);
    }

    // Save topo to data/topo/<machine>.xml
    ezv_topo_save_if_needed ("data/topo", filename, PATH_MAX);

    // And store file path into trace
    ezm_recorder_store_topo (ezp_monitor, filename);

    if (ezp_bindings != NULL) {
      // Store bindings
      ezm_recorder_store_thr_bindings (ezp_monitor, ezp_bindings);
    }

    if (trace_starting_iteration == 1)
      ezm_recorder_enable (ezp_monitor, 1);

    return;
  }
#endif

  if (do_gmonitor) {

    if (do_gmonitor_big_footprint) {
      // -M mode: Footprint full size, Image half size
      // Add footprint window (gets full size)
      ezm_helper_add_footprint (ezp_monitor, ctx, &nb_ctx, 1);
    } else {
      // -m mode: Image full size, Footprint half size
      ezm_helper_add_footprint (ezp_monitor, ctx, &nb_ctx, 0);
    }

    // Add perfmeter
    ezm_helper_add_perfmeter (ezp_monitor, ctx, &nb_ctx, monitoring_mode,
                              ezp_bindings);

    ezm_recorder_enable (ezp_monitor, 1);
  }
}

void ezp_monitoring_cleanup (void)
{
  ezm_recorder_delete (ezp_monitor);
  ezm_finalize ();

  if (ezp_bindings != NULL) {
    free (ezp_bindings);
    ezp_bindings = NULL;
  }
}
