#include <stdlib.h>
#include <unistd.h>

#include "error.h"
#include "ezm.h"
#include "ezm_footprint.h"
#include "ezm_perfmeter.h"
#include "ezm_recorder_private.h"
#ifdef ENABLE_TRACE
#include "ezm_tracerec.h"
#endif
#include "ezm_time.h"

#include "ezv.h"
#include "ezv_clustering.h"
#include "ezv_img2d_object.h"
#include "ezv_mesh3d_object.h"

#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif

static int no_display = 0;

void _ezm_init (unsigned flags)
{
  if (flags & EZM_NO_DISPLAY)
    no_display = 1;
}

void ezm_finalize (void)
{
  if (!no_display)
    ezv_finalize ();
}

ezm_recorder_t ezm_recorder_create (unsigned nb_thr, unsigned nb_gpus,
                                    ezv_topo_pu_block_t *bindings)
{
  ezm_recorder_t rec = (ezm_recorder_t)malloc (sizeof (*rec));

  rec->nb_thr        = nb_thr;
  rec->nb_gpus       = nb_gpus;
  rec->bindings      = bindings;
  rec->folding_level = EZV_TOPO_UNFOLDED_LEVEL;
  rec->time_needed   = 0;
  rec->enabled       = 0;
  rec->perfmeter     = NULL;
  rec->footprint     = NULL;
#ifdef ENABLE_TRACE
  rec->tracerec = NULL;
#endif

  rec->clustering = ezv_clustering_create (nb_thr, bindings);

  ezv_palette_init (&rec->palette);

  return rec;
}

void ezm_recorder_delete (ezm_recorder_t rec)
{
  if (ezv_palette_is_defined (&rec->palette))
    ezv_palette_free (&rec->palette);

  if (rec->perfmeter)
    ezm_perfmeter_destroy (rec->perfmeter);

  if (rec->footprint)
    ezm_footprint_destroy (rec->footprint);

#ifdef ENABLE_TRACE
  if (rec->tracerec)
    ezm_tracerec_destroy (rec->tracerec);
#endif

  if (rec->clustering)
    ezv_clustering_destroy (rec->clustering);

  free (rec);
}

static void ezm_set_palette (ezm_recorder_t rec, int force)
{
  if (ezv_palette_is_defined (&rec->palette)) {
    if (!force)
      return;
    else 
      ezv_palette_free (&rec->palette);
  } 

  if (ezm_get_nb_cpu_colors (rec) > 18) { // EasyPAP palette only has 18 colors
    ezv_palette_set_predefined (&rec->palette, EZV_PALETTE_RAINBOW);
    rec->palette_mode = 0;
  } else {
    ezv_palette_set_predefined (&rec->palette, EZV_PALETTE_EASYPAP);
    rec->palette_mode = 1;
  }
}

void ezm_set_cpu_palette (ezm_recorder_t rec, ezv_palette_name_t name,
                          int cyclic_mode)
{
  ezv_palette_set_predefined (&rec->palette, name);
  rec->palette_mode = cyclic_mode;
}

unsigned ezm_get_nb_cpu_colors (ezm_recorder_t rec)
{
  const unsigned nb_clusters =
      ezv_clustering_get_nb_clusters (rec->clustering, rec->folding_level);
  return nb_clusters + rec->nb_gpus;
}

uint32_t ezm_get_cpu_color (ezm_recorder_t rec, unsigned c)
{
  unsigned total = ezm_get_nb_cpu_colors (rec);

  if (rec->palette_mode)
    return ezv_palette_get_color_from_index (&rec->palette, c);
  else
    return ezv_palette_get_color_from_value (
        &rec->palette, total > 1 ? (float)c / (float)(total - 1) : 0.5f);
}

void ezm_recorder_attach_perfmeter (ezm_recorder_t rec, ezv_ctx_t ctx)
{
  if (!ezv_palette_is_defined (&rec->palette))
    ezm_set_palette (rec, 0);

  if (rec->perfmeter != NULL)
    exit_with_error (
        "ezm_recorder_attach_perfmeter: perfmeter already attached");

  rec->perfmeter   = ezm_perfmeter_create (rec, ctx);
  rec->time_needed = 1;
}

void ezm_recorder_attach_footprint (ezm_recorder_t rec, ezv_ctx_t ctx)
{
  if (!ezv_palette_is_defined (&rec->palette))
    ezm_set_palette (rec, 0);

  if (rec->footprint != NULL)
    exit_with_error (
        "ezm_recorder_attach_footprint: footprint already attached");

  rec->footprint = ezm_footprint_create (rec, ctx);
}

void ezm_recorder_attach_tracerec (ezm_recorder_t rec, char *file, char *label)
{
#ifdef ENABLE_TRACE
  if (rec->tracerec != NULL)
    exit_with_error ("ezm_recorder_attach_tracerec: tracerec already attached");

  rec->tracerec = ezm_tracerec_create (rec->nb_thr, rec->nb_gpus, file, label);
#endif
}

ezm_perfmeter_t ezm_recorder_get_perfmeter (ezm_recorder_t rec)
{
  return rec->perfmeter;
}

ezm_footprint_t ezm_recorder_get_footprint (ezm_recorder_t rec)
{
  return rec->footprint;
}

#ifdef ENABLE_TRACE
ezm_tracerec_t ezm_recorder_get_tracerec (ezm_recorder_t rec)
{
  return rec->tracerec;
}
#endif

void ezm_recorder_enable (ezm_recorder_t rec, unsigned iter)
{
  rec->enabled = 1;

#ifdef ENABLE_TRACE
  if (rec->tracerec) {
    ezm_tracerec_enable (rec->tracerec, iter);
    return;
  }
#endif

  if (rec->perfmeter)
    ezm_perfmeter_enable (rec->perfmeter);

  if (rec->footprint)
    ezm_footprint_enable (rec->footprint);
}

void ezm_recorder_disable (ezm_recorder_t rec)
{
  if (rec->perfmeter)
    ezm_perfmeter_disable (rec->perfmeter);

  if (rec->footprint)
    ezm_footprint_disable (rec->footprint);

  rec->enabled = 0;
}

int ezm_recorder_is_enabled (ezm_recorder_t rec)
{
  return rec->enabled;
}

void ezm_start_iteration (ezm_recorder_t rec)
{
  uint64_t clock = 0;

  if (!rec->enabled)
    return;

#ifdef ENABLE_TRACE
  if (rec->tracerec) {
    ezm_tracerec_it_start (rec->tracerec);
    return;
  }
#endif

  if (rec->time_needed)
    clock = ezm_gettime ();

  if (rec->perfmeter)
    ezm_perfmeter_it_start (rec->perfmeter, clock);

  if (rec->footprint)
    ezm_footprint_it_start (rec->footprint, clock);
}

void ezm_end_iteration (ezm_recorder_t rec)
{
  uint64_t clock = 0;

  if (!rec->enabled)
    return;

#ifdef ENABLE_TRACE
  if (rec->tracerec) {
    ezm_tracerec_it_end (rec->tracerec);
    return;
  }
#endif

  if (rec->time_needed)
    clock = ezm_gettime ();

  if (rec->perfmeter)
    ezm_perfmeter_it_end (rec->perfmeter, clock);

  if (rec->footprint)
    ezm_footprint_it_end (rec->footprint, clock);
}

void ezm_start_task (ezm_recorder_t rec, unsigned cpu)
{
  uint64_t clock = 0;

  if (!rec->enabled)
    return;

#ifdef ENABLE_TRACE
  if (rec->tracerec) {
    ezm_tracerec_start_work (rec->tracerec, cpu);
    return;
  }
#endif

  if (rec->time_needed)
    clock = ezm_gettime ();

  if (rec->perfmeter)
    ezm_perfmeter_start_work (rec->perfmeter, clock, cpu);

  if (rec->footprint)
    ezm_footprint_start_work (rec->footprint, clock, cpu);
}

// 1D patches (i.e. meshes)

// Short version
void ezm_end_task_1D (ezm_recorder_t rec, unsigned cpu, unsigned patch,
                      unsigned count)
{
  uint64_t clock = 0;

  if (!rec->enabled)
    return;

#ifdef ENABLE_TRACE
  if (rec->tracerec) {
    ezm_tracerec_1D (rec->tracerec, cpu, patch, count);
    return;
  }
#endif

  if (rec->time_needed)
    clock = ezm_gettime ();

  if (rec->perfmeter)
    ezm_perfmeter_finish_work (rec->perfmeter, clock, cpu);

  if (rec->footprint)
    ezm_footprint_finish_work_1D (rec->footprint, clock, cpu, patch, count);

  if (rec->perfmeter)
    ezm_perfmeter_substract_overhead (rec->perfmeter, ezm_gettime () - clock,
                                      cpu);
}

void ezm_end_task_1D_id (ezm_recorder_t rec, unsigned cpu, unsigned patch,
                         unsigned count, int task_id)
{
  uint64_t clock = 0;

  if (!rec->enabled)
    return;

#ifdef ENABLE_TRACE
  if (rec->tracerec) {
    ezm_tracerec_1D_task (rec->tracerec, cpu, patch, count, task_id);
    return;
  }
#endif

  if (rec->time_needed)
    clock = ezm_gettime ();

  if (rec->perfmeter)
    ezm_perfmeter_finish_work (rec->perfmeter, clock, cpu);

  if (rec->footprint)
    ezm_footprint_finish_work_1D (rec->footprint, clock, cpu, patch, count);

  if (rec->perfmeter)
    ezm_perfmeter_substract_overhead (rec->perfmeter, ezm_gettime () - clock,
                                      cpu);
}

// Extended versions
void ezm_task_1D (ezm_recorder_t rec, uint64_t start_time, uint64_t end_time,
                  unsigned cpu, unsigned patch, unsigned count,
                  task_type_t task_type, int task_id)
{
  uint64_t clock = 0;

  if (!rec->enabled)
    return;

#ifdef ENABLE_TRACE
  if (rec->tracerec) {
    ezm_tracerec_1D_ext (rec->tracerec, start_time, end_time, cpu, patch, count,
                         task_type, task_id);
    return;
  }
#endif

  if (rec->time_needed)
    clock = ezm_gettime ();

  if (rec->perfmeter) { // TODO: && task_type == TASK_TYPE_COMPUTE
    ezm_perfmeter_start_work (rec->perfmeter, start_time, cpu);
    ezm_perfmeter_finish_work (rec->perfmeter, end_time, cpu);
  }

  if (rec->footprint) { // TODO: && task_type == TASK_TYPE_COMPUTE
    ezm_footprint_start_work (rec->footprint, start_time, cpu);
    ezm_footprint_finish_work_1D (rec->footprint, end_time, cpu, patch, count);
  }

  if (rec->perfmeter) // TODO: && task_type == TASK_TYPE_COMPUTE
    ezm_perfmeter_substract_overhead (rec->perfmeter, ezm_gettime () - clock,
                                      cpu);
}

// 2D tiles (i.e. images)

// Short version
void ezm_end_task_2D (ezm_recorder_t rec, unsigned cpu, unsigned x, unsigned y,
                      unsigned w, unsigned h)
{
  uint64_t clock = 0;

  if (!rec->enabled)
    return;

#ifdef ENABLE_TRACE
  if (rec->tracerec) {
    ezm_tracerec_2D (rec->tracerec, cpu, x, y, w, h);
    return;
  }
#endif

  if (rec->time_needed)
    clock = ezm_gettime ();

  if (rec->perfmeter)
    ezm_perfmeter_finish_work (rec->perfmeter, clock, cpu);

  if (rec->footprint)
    ezm_footprint_finish_work_2D (rec->footprint, clock, cpu, x, y, w, h);

  if (rec->perfmeter)
    ezm_perfmeter_substract_overhead (rec->perfmeter, ezm_gettime () - clock,
                                      cpu);
}

void ezm_end_task_2D_id (ezm_recorder_t rec, unsigned cpu, unsigned x,
                         unsigned y, unsigned w, unsigned h, int task_id)
{
  uint64_t clock = 0;

  if (!rec->enabled)
    return;

#ifdef ENABLE_TRACE
  if (rec->tracerec) {
    ezm_tracerec_2D_task (rec->tracerec, cpu, x, y, w, h, task_id);
    return;
  }
#endif

  if (rec->time_needed)
    clock = ezm_gettime ();

  if (rec->perfmeter)
    ezm_perfmeter_finish_work (rec->perfmeter, clock, cpu);

  if (rec->footprint)
    ezm_footprint_finish_work_2D (rec->footprint, clock, cpu, x, y, w, h);

  if (rec->perfmeter)
    ezm_perfmeter_substract_overhead (rec->perfmeter, ezm_gettime () - clock,
                                      cpu);
}

// extended version (with start/end time)
void ezm_task_2D (ezm_recorder_t rec, uint64_t start_time, uint64_t end_time,
                  unsigned cpu, unsigned x, unsigned y, unsigned w, unsigned h,
                  task_type_t task_type, int task_id)
{
  uint64_t clock = 0;

  if (!rec->enabled)
    return;

#ifdef ENABLE_TRACE
  if (rec->tracerec) {
    ezm_tracerec_2D_ext (rec->tracerec, start_time, end_time, cpu, x, y, w, h,
                         task_type, task_id);
    return;
  }
#endif

  if (rec->time_needed)
    clock = ezm_gettime ();

  if (rec->perfmeter) { // TODO: && task_type == TASK_TYPE_COMPUTE
    ezm_perfmeter_start_work (rec->perfmeter, start_time, cpu);
    ezm_perfmeter_finish_work (rec->perfmeter, end_time, cpu);
  }

  if (rec->footprint) { // TODO: && task_type == TASK_TYPE_COMPUTE
    ezm_footprint_start_work (rec->footprint, start_time, cpu);
    ezm_footprint_finish_work_2D (rec->footprint, end_time, cpu, x, y, w, h);
  }

  if (rec->perfmeter) // TODO: && task_type == TASK_TYPE_COMPUTE
    ezm_perfmeter_substract_overhead (rec->perfmeter, ezm_gettime () - clock,
                                      cpu);
}

void ezm_recorder_toggle_heat_mode (ezm_recorder_t rec)
{
  if (rec->footprint) {
    int m = ezm_footprint_toggle_heat_mode (rec->footprint);
    if (m)
      rec->time_needed++;
    else
      rec->time_needed--;
  }
}

void ezm_recorder_store_mesh3d_filename (ezm_recorder_t rec,
                                         const char *filename)
{
#ifdef ENABLE_TRACE
  if (rec->tracerec)
    ezm_tracerec_store_mesh3d_filename (rec->tracerec, filename);
#endif
}

void ezm_recorder_store_img2d_dim (ezm_recorder_t rec, unsigned width,
                                   unsigned height)
{
#ifdef ENABLE_TRACE
  if (rec->tracerec)
    ezm_tracerec_store_img2d_dim (rec->tracerec, width, height);
#endif
}

void ezm_recorder_store_data_palette (ezm_recorder_t rec,
                                      ezv_palette_name_t pal)
{
#ifdef ENABLE_TRACE
  if (rec->tracerec)
    ezm_tracerec_store_data_palette (rec->tracerec, pal);
#endif
}

void ezm_recorder_declare_task_ids (ezm_recorder_t rec, char *task_ids [])
{
#ifdef ENABLE_TRACE
  if (rec->tracerec)
    ezm_tracerec_declare_task_ids (rec->tracerec, task_ids);
#endif
}

void ezm_recorder_store_topo (ezm_recorder_t rec, const char *topo_filename)
{
#ifdef ENABLE_TRACE
  if (rec->tracerec)
    ezm_tracerec_store_topo (rec->tracerec, topo_filename);
#endif
}

void ezm_recorder_store_thr_bindings (ezm_recorder_t rec,
                                      ezv_topo_pu_block_t *bindings)
{
#ifdef ENABLE_TRACE
  if (rec->tracerec)
    ezm_tracerec_store_thr_bindings (rec->tracerec, bindings);
#endif
}

void ezm_recorder_switch_folding_level (ezm_recorder_t rec)
{
  // Only works if perfmeter is attached and monitor mode is TOPOLOGY
  if (!rec->perfmeter || rec->monitor.mode != EZV_MONITOR_TOPOLOGY)
    return;

  int new_level = ezv_topo_next_valid_folding (rec->folding_level);
  if (new_level == -1) {
    // Go back to first available level
    new_level = ezv_topo_first_valid_folding ();
    if (new_level == -1)
      exit_with_error ("ezm_recorder_switch_folding_level: no valid folding level");
  }
  
  rec->folding_level = rec->monitor.folding_level = new_level;

  ezm_set_palette (rec, 1); // force palette update

  if (rec->perfmeter)
    ezm_perfmeter_folding_level_has_changed (rec->perfmeter);
  if (rec->footprint)
    ezm_footprint_folding_level_has_changed (rec->footprint);
}

// Helpers
void ezm_helper_add_perfmeter (ezm_recorder_t rec, ezv_ctx_t ctx [],
                               unsigned *nb_ctx, ezv_mon_mode_t mon_mode,
                               ezv_topo_pu_block_t *bindings)
{
  unsigned x, y, w, h;
  ezv_ctx_t c;

  mon_obj_init (&rec->monitor, rec->nb_thr, rec->nb_gpus, mon_mode,
                rec->folding_level, rec->clustering);

  ezv_mon_precalc_view_parameters (&rec->monitor, &w, &h);

  if (rec->monitor.folding_level > EZV_TOPO_UNFOLDED_LEVEL) {
    // folding level has changed to fit in the window height
    rec->folding_level = rec->monitor.folding_level;
    ezm_set_palette (rec, 1); // force palette update

    // Update footprint object if any
    if (rec->footprint)
      ezm_footprint_folding_level_has_changed (rec->footprint);
  }

  ezv_helper_ctx_next_coord (ctx, *nb_ctx, &x, &y);

  c = ezv_ctx_create (EZV_CTX_TYPE_MONITOR, "Monitoring", x, y, w, h,
                      EZV_HIDE_WINDOW);
  ezv_mon_set_moninfo (c, &rec->monitor);

  ezm_recorder_attach_perfmeter (rec, c);

  ctx [(*nb_ctx)++] = c;
}

void ezm_helper_add_footprint (ezm_recorder_t rec, ezv_ctx_t ctx[],
                               unsigned *nb_ctx, int big_footprint)
{
  unsigned x, y, w, h;
  ezv_ctx_t c;

  if (big_footprint) {
    // -M mode: Footprint gets full size
    w = ezv_ctx_width (ctx[0]);
    h = ezv_ctx_height (ctx[0]);
  } else {
    // -m mode: Footprint gets half size
    w = ezv_ctx_width (ctx[0]) / 2;
    h = ezv_ctx_height (ctx[0]) / 2;
  }

  ezv_helper_ctx_next_coord (ctx, *nb_ctx, &x, &y);

  c = ezv_ctx_create (ezv_ctx_type (ctx [0]), "Footprint", x, y, w, h,
                      EZV_HIDE_WINDOW);

  switch (ezv_ctx_type (ctx [0])) {
  case EZV_CTX_TYPE_IMG2D: {
    img2d_obj_t *img = ezv_img2d_img (ctx [0]);
    ezv_img2d_set_img (c, img);
    break;
  }
  case EZV_CTX_TYPE_MESH3D: {
    mesh3d_obj_t *mesh = ezv_mesh3d_mesh (ctx [0]);
    ezv_mesh3d_set_mesh (c, mesh);
    break;
  }
  default:
    exit_with_error ("ezm_helper_add_footprint: unsupported context type (%s)",
                     ezv_ctx_typestr (ctx [0]));
  }

  ezm_recorder_attach_footprint (rec, c);

  ctx [(*nb_ctx)++] = c;
}
