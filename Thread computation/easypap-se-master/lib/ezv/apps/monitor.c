
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "error.h"
#include "ezv.h"
#include "ezv_event.h"
#include "ezv_topo.h"
#include "ezv_virtual.h"

// settings
static unsigned NB_THR = 22;
static unsigned NB_GPU = 2;

unsigned int SCR_WIDTH  = 512;
unsigned int SCR_HEIGHT = 512;

#define MAX_CTX 1

static mon_obj_t monitor;
static ezv_ctx_t ctx [MAX_CTX] = {NULL};
static unsigned nb_ctx         = 1;

static int process_events (void)
{
  SDL_Event event;
  int ret;

  ret = ezv_get_event (&event, EZV_EVENT_NON_BLOCKING);

  if (ret > 0)
    return ezv_process_event (ctx, nb_ctx, &event, NULL, NULL);

  return 1;
}

#define delta 64.0

static void set_values (void)
{
  static float phase       = 0;
  const unsigned nb_colors = ezv_get_color_data_size (ctx [0]);
  float values [nb_colors];

  for (int i = 0; i < nb_colors; i++)
    values [i] =
        (1.0 + sin (phase + (float)i * M_PI / (float)(nb_colors - 1))) / 2.0;

  phase = fmod (phase + M_PI / delta, 2 * M_PI);

  ezv_set_data_colors (ctx [0], values);
}

static ezv_clustering_t clustering = NULL;

static void set_cpu_colors (ezv_ctx_t ctx, ezv_palette_name_t name, int cyclic)
{
  ezv_palette_t palette;
  const unsigned nb_colors = ezv_get_color_data_size (ctx);

  ezv_palette_init (&palette);
  ezv_palette_set_predefined (&palette, name);

  ezv_use_cpu_colors (ctx);
  for (int c = 0; c < nb_colors; c++)
    if (cyclic)
      ezv_set_cpu_color_1D (ctx, c, 1,
                            ezv_palette_get_color_from_index (&palette, c));
    else
      ezv_set_cpu_color_1D (ctx, c, 1,
                            ezv_palette_get_color_from_value (
                                &palette, (float)c / (float)(nb_colors - 1)));

  ezv_palette_free (&palette);
}

static ezv_topo_pu_block_t bindings [1024];

int main (int argc, char *argv [])
{
  ezv_init ();

  if (argc > 1 && (!strcmp (argv [1], "-t") || !strcmp (argv [1], "--topo"))) {
    NB_THR = ezv_topo_get_nb_pus ();

    for (unsigned i = 0; i < NB_THR + NB_GPU; i++) {
      bindings [i].first_pu = i;
      bindings [i].nb_pus   = 1;
    }

    clustering = ezv_clustering_create (NB_THR, bindings);

    mon_obj_init (&monitor, NB_THR, NB_GPU, EZV_MONITOR_TOPOLOGY,
                  EZV_TOPO_UNFOLDED_LEVEL, clustering);
  } else {
    clustering = ezv_clustering_create (NB_THR, NULL);

    mon_obj_init (&monitor, NB_THR, NB_GPU, EZV_MONITOR_DEFAULT,
                  EZV_TOPO_UNFOLDED_LEVEL, clustering);
  }

  ezv_mon_precalc_view_parameters (&monitor, &SCR_WIDTH, &SCR_HEIGHT);

  // Create SDL windows and initialize OpenGL context
  ctx [0] = ezv_ctx_create (EZV_CTX_TYPE_MONITOR, "Monitoring",
                            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_UNDEFINED,
                            SCR_WIDTH, SCR_HEIGHT, EZV_ENABLE_VSYNC);

  // Attach monitor info
  ezv_mon_set_moninfo (ctx [0], &monitor);

  set_cpu_colors (ctx [0], EZV_PALETTE_RAINBOW, 0);

  // render loop
  int cont = 1;
  while (cont) {
    // Set random values for CPU perfmeters
    set_values ();

    cont = process_events ();
    ezv_render (ctx, nb_ctx);
  }

  if (clustering) {
    ezv_clustering_destroy (clustering);
  }
  ezv_ctx_delete (ctx, nb_ctx);
  ezv_finalize ();

  return 0;
}
