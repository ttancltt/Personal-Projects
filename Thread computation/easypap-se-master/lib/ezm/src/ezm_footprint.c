#include <stdlib.h>
#include <unistd.h>

#include "error.h"
#include "ezm.h"
#include "ezm_footprint.h"
#include "ezm_recorder_private.h"
#include "ezm_time.h"
#include "ezv.h"
#include "ezv_virtual.h"

struct ezm_footp_struct
{
  ezm_recorder_t parent;
  unsigned nb_pus;
  unsigned heat_mode;
  ezv_palette_t *palette;
  unsigned cyclic_mode;
  uint64_t prev_max_duration, prev_min_duration;
  uint64_t max_duration, min_duration;
  struct
  {
    uint64_t start_time;
  } *pu_stat;
  uint32_t *thr_color;
  ezv_ctx_t ezv_ctx;
};

/// Luminance computation
static const char LogTable256 [256] = {
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
    0,      0,      1,      1,      2,      2,      2,      2,
    3,      3,      3,      3,      3,      3,      3,      3,
    LT (4), LT (5), LT (5), LT (6), LT (6), LT (6), LT (6), LT (7),
    LT (7), LT (7), LT (7), LT (7), LT (7), LT (7), LT (7)};

static inline unsigned mylog2 (unsigned v)
{
  unsigned r; // r will be lg(v)
  unsigned int t;

  if ((t = (v >> 16)))
    r = 16 + LogTable256 [t];
  else if ((t = (v >> 8)))
    r = 8 + LogTable256 [t];
  else
    r = LogTable256 [v];

  return r;
}

static uint32_t apply_luminance (ezm_footprint_t rec, uint64_t duration,
                                 uint32_t color)
{
  const float scale = 1.0 / 14.0;

  if (duration > rec->max_duration)
    rec->max_duration = duration;
  if (duration < rec->min_duration)
    rec->min_duration = duration;

  if (rec->prev_max_duration >
      rec->prev_min_duration) { // not the first iteration
    // force duration to stay between bounds
    if (duration < rec->prev_min_duration) {
      duration = rec->prev_min_duration;
    } else if (duration > rec->prev_max_duration) {
      duration = rec->prev_max_duration;
    }

    uint64_t intensity = 8191UL * (duration - rec->prev_min_duration) /
                         (rec->prev_max_duration - rec->prev_min_duration);
    // log2(intensity) is in [0..12] so log2(intensity) + 2 is in [2..14]
    float f = (float)(mylog2 (intensity) + 2) * scale;
    color =
        ezv_rgb (ezv_c2r (color) * f, ezv_c2g (color) * f, ezv_c2b (color) * f);
  }

  return color;
}

static void ezm_footprint_set_cpu_colors (ezm_footprint_t rec)
{
  const unsigned nb_clusters =
      ezv_clustering_get_nb_clusters (rec->parent->clustering,
                                     rec->parent->folding_level);

  for (int th = 0; th < rec->parent->nb_thr; th++) {
    unsigned c          = ezv_clustering_get_cluster_index_from_thr (
        rec->parent->clustering, rec->parent->folding_level, th);
    rec->thr_color [th] = ezm_get_cpu_color (rec->parent, c);
  }

  for (int g = 0; g < rec->parent->nb_gpus; g++) {
    unsigned idx = rec->parent->nb_thr + g;

    rec->thr_color [idx] = ezm_get_cpu_color (rec->parent, nb_clusters + g);
  }
}

void ezm_footprint_folding_level_has_changed (ezm_footprint_t rec)
{
  ezm_footprint_set_cpu_colors (rec);
}

ezm_footprint_t ezm_footprint_create (ezm_recorder_t parent, ezv_ctx_t ctx)
{
  ezm_footprint_t rec = (ezm_footprint_t)malloc (sizeof (*rec));

  rec->parent            = parent;
  rec->nb_pus            = parent->nb_thr + parent->nb_gpus;
  rec->heat_mode         = 0;
  rec->palette           = &parent->palette;
  rec->cyclic_mode       = parent->palette_mode;
  rec->prev_min_duration = 0;
  rec->prev_max_duration = 0;
  rec->min_duration      = 0;
  rec->max_duration      = 0;
  rec->ezv_ctx           = ctx;

  rec->pu_stat = malloc ((rec->nb_pus) * sizeof (*rec->pu_stat));

  // precalculate thread colors
  rec->thr_color = malloc ((rec->nb_pus) * sizeof (*rec->thr_color));

  ezv_use_cpu_colors (ctx);
  ezm_footprint_set_cpu_colors (rec);

  return rec;
}

void ezm_footprint_enable (ezm_footprint_t rec)
{
  ezv_ctx_show (rec->ezv_ctx);
}

void ezm_footprint_disable (ezm_footprint_t rec)
{
  ezv_ctx_hide (rec->ezv_ctx);
}

void ezm_footprint_it_start (ezm_footprint_t rec, uint64_t now)
{
  rec->prev_min_duration = rec->min_duration;
  rec->min_duration      = UINT64_MAX;

  rec->prev_max_duration = rec->max_duration;
  rec->max_duration      = 0;

  ezv_reset_cpu_colors (rec->ezv_ctx);
}

void ezm_footprint_it_end (ezm_footprint_t rec, uint64_t now)
{
}

void ezm_footprint_start_work (ezm_footprint_t rec, uint64_t now, int who)
{
  rec->pu_stat [who].start_time = now;
}

void ezm_footprint_finish_work_1D (ezm_footprint_t rec, uint64_t now, int who,
                                   unsigned patch, unsigned count)
{
  if (!count)
    return;

  uint32_t color = rec->thr_color [who];

  if (rec->heat_mode)
    color = apply_luminance (
        rec, (now - rec->pu_stat [who].start_time) * 1024.0f / (float)count,
        color);

  ezv_set_cpu_color_1D (rec->ezv_ctx, patch, count, color);
}

void ezm_footprint_finish_work_2D (ezm_footprint_t rec, uint64_t now, int who,
                                   unsigned x, unsigned y, unsigned w,
                                   unsigned h)
{
  if (!w || !h)
    return;

  uint32_t color = rec->thr_color [who];

  if (rec->heat_mode)
    color = apply_luminance (rec,
                             (now - rec->pu_stat [who].start_time) *
                                 (32.0f * 32.0f) / (float)(w * h),
                             color);

  ezv_set_cpu_color_2D (rec->ezv_ctx, x, w, y, h, color);
}

int ezm_footprint_toggle_heat_mode (ezm_footprint_t rec)
{
  rec->heat_mode ^= 1;

  return rec->heat_mode;
}

void ezm_footprint_destroy (ezm_footprint_t rec)
{
  free (rec->pu_stat);
  free (rec->thr_color);
  free (rec);
}
