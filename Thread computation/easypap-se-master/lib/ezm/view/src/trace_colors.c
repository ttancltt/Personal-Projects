
#include <stdlib.h>

#include "ezv_palette.h"
#include "ezv_rgba.h"
#include "min_max.h"
#include "trace_colors.h"
#include "trace_data.h"
#include "trace_graphics.h"
#include "trace_topo.h"

static ezv_palette_t palette;
static int palette_mode; // 0: gradient mode, 1: indexed mode

unsigned TRACE_MAX_COLORS = 0;

static uint32_t *the_colors                    = NULL;
static unsigned *per_trace_index [MAX_TRACES] = {NULL, NULL};

static void set_palette (void)
{
  ezv_palette_init (&palette);

  if (TRACE_MAX_COLORS > 18) { // EasyPAP palette only has 18 colors
    ezv_palette_set_predefined (&palette, EZV_PALETTE_RAINBOW);
    palette_mode = 0;
  } else {
    ezv_palette_set_predefined (&palette, EZV_PALETTE_EASYPAP);
    palette_mode = 1;
  }
}

static void fill_colors (void)
{
  for (int c = 0; c < TRACE_MAX_COLORS; c++)
    if (palette_mode)
      // Indexed mode
      the_colors [c] = ezv_palette_get_color_from_index (&palette, c);
    else
      // Gradient mode
      the_colors [c] = ezv_palette_get_color_from_value (
          &palette, (float)c / (float)(TRACE_MAX_COLORS - 1));

  the_colors [TRACE_MAX_COLORS] = ezv_rgb (255, 255, 255); // Extra white color
}

static unsigned nb_colors_for_threads (trace_t *tr)
{
  return ezv_clustering_get_nb_clusters (tr->clustering, tr->folding_level);
}

static unsigned nb_colors_for_gpus (trace_t *tr)
{
  return tr->nb_gpu; // each GPU is its own PU block
}

uint32_t trace_colors_get_color (int index)
{
  return the_colors [index];
}

unsigned trace_colors_get_index (int trace_id, int pu)
{
  return per_trace_index [trace_id][pu];
}

void trace_colors_init (void)
{
  unsigned max_threads = 0;
  unsigned max_gpus    = 0;

  for (int c = 0; c < nb_traces; c++) {
    trace_t *tr = &trace [c];
    max_threads = MAX (max_threads, nb_colors_for_threads (tr));
    max_gpus    = MAX (max_gpus, nb_colors_for_gpus (tr));
  }

  TRACE_MAX_COLORS = max_threads + max_gpus;
  the_colors       = malloc ((TRACE_MAX_COLORS + 1) * sizeof (uint32_t));

  set_palette ();

  fill_colors ();

  for (int c = 0; c < nb_traces; c++) {
    unsigned nb_clusters = ezv_clustering_get_nb_clusters (
        trace [c].clustering, trace [c].folding_level);
    unsigned nb_gpus = trace [c].nb_gpu;
    
    per_trace_index [c] = malloc ((TRACE_MAX_COLORS + 1) * sizeof (unsigned));
    // Cluster colors
    for (unsigned i = 0; i < nb_clusters; i++)
      per_trace_index [c][i] = i;
    // GPU colors
    for (unsigned i = 0; i < nb_gpus; i++)
      per_trace_index [c][nb_clusters + i] = max_threads + i;
    // Extra white color
    per_trace_index [c][TRACE_MAX_COLORS] = TRACE_MAX_COLORS;
  }
}

void trace_colors_finalize (void)
{
  if (the_colors) {
    free (the_colors);
    the_colors = NULL;
  }

  for (int c = 0; c < nb_traces; c++) {
    if (per_trace_index [c]) {
      free (per_trace_index [c]);
      per_trace_index [c] = NULL;
    }
  }

  ezv_palette_free (&palette);
}