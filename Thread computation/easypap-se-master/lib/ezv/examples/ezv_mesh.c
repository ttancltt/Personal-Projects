
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "ezv.h"
#include "ezv_event.h"

// settings
const unsigned int SCR_WIDTH  = 1024;
const unsigned int SCR_HEIGHT = 768;

#define MAX_CTX 2

static mesh3d_obj_t mesh;
static ezv_ctx_t ctx [MAX_CTX] = {NULL, NULL};
static unsigned nb_ctx         = 1;
static int hud                 = -1;

static float *data  = NULL;
static int datasize = 0;

static void set_value_for_partition (unsigned part, uint32_t color)
{
  for (int c = mesh.patch_first_cell [part];
       c < mesh.patch_first_cell [part + 1]; c++)
    ezv_set_cpu_color_1D (ctx [0], c, 1, color);
}

static void set_value_for_neighbors (unsigned part, uint32_t color)
{
  for (int ni = mesh.index_first_patch_neighbor [part];
       ni < mesh.index_first_patch_neighbor [part + 1]; ni++)
    set_value_for_partition (mesh.patch_neighbors [ni], color);
}

static void do_pick (void)
{
  int p = ezv_perform_1D_picking (ctx, nb_ctx);

  ezv_reset_cpu_colors (ctx [0]);

  if (p != -1) {
    int partoche = mesh3d_obj_get_patch_of_cell (&mesh, p);
    ezv_hud_set (ctx [0], hud, "Part: %d", partoche);

    // Highlight selected patch
    set_value_for_partition (partoche, ezv_rgb (235, 235, 235));

    // Highlight patch neighbors (if any)
    if (mesh.patch_neighbors != NULL)
      set_value_for_neighbors (partoche, ezv_rgb (128, 128, 128));

    ezv_set_cpu_color_1D (ctx [0], p, 1, ezv_rgb (255, 0, 0));
  } else
    ezv_hud_set (ctx [0], hud, NULL);
}

static int process_events (void)
{
  SDL_Event event;
  int pick;
  int ret;

  ezv_get_event (&event, EZV_EVENT_BLOCKING);

  ret = ezv_process_event (ctx, nb_ctx, &event, NULL, &pick);
  if (pick)
    do_pick ();

  return ret;
}

static void load_mesh (int argc, char *argv [], mesh3d_obj_t *mesh)
{
  int nb_patches = -1;

  if (argc > 1) {

    if (argc > 2)
      nb_patches = atoi (argv [2]);

    if (!strcmp (argv [1], "--help") || !strcmp (argv [1], "-h")) {
      printf ("Usage: %s [ <file> ] [ #patches ]\n", argv [0]);
      exit (0);
    } else
      mesh3d_obj_load (argv [1], mesh);
  } else
    mesh3d_obj_load ("data/mesh/octopus.obj", mesh);

  if (nb_patches == -1) {
    if (mesh->nb_patches == 0)
      mesh3d_obj_partition (
          mesh, 2, MESH3D_PART_USE_SCOTCH | MESH3D_PART_SHOW_FRONTIERS);
    else
      mesh3d_obj_partition (mesh, 0,
                            MESH3D_PART_USE_SCOTCH | MESH3D_PART_SHOW_FRONTIERS);
  } else {
    mesh3d_obj_partition (mesh, nb_patches,
                          MESH3D_PART_USE_SCOTCH | MESH3D_PART_SHOW_FRONTIERS);
  }
}

int main (int argc, char *argv [])
{
  _ezv_init ("lib/ezv");

  mesh3d_obj_init (&mesh);
  load_mesh (argc, argv, &mesh);

  // Create SDL windows and initialize OpenGL context
  ctx [0] = ezv_ctx_create (
      EZV_CTX_TYPE_MESH3D, "Patch", SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_UNDEFINED, SCR_WIDTH, SCR_HEIGHT,
      EZV_ENABLE_PICKING | EZV_ENABLE_HUD | EZV_ENABLE_CLIPPING);
  hud = ezv_hud_alloc (ctx [0]);
  ezv_hud_on (ctx [0], hud);

  // Attach mesh
  ezv_mesh3d_set_mesh (ctx [0], &mesh);

  ezv_use_data_colors_predefined (ctx [0], EZV_PALETTE_RAINBOW);

  // Cell values
  datasize = mesh.nb_cells * sizeof (float);
  data     = malloc (datasize);

  for (unsigned c = 0; c < mesh.nb_cells; c++)
    data [c] = 0.65f;

  ezv_set_data_colors (ctx [0], data);

  // Second color layer
  ezv_use_cpu_colors (ctx [0]);

  // Some initial colors…
  for (int c = 0; c < mesh.nb_cells; c++)
    ezv_set_cpu_color_1D (ctx [0], c, 1, ezv_rgba (0, 0, 0, 0));

  // render loop
  int cont = 1;
  while (cont) {
    cont = process_events ();
    if (cont)
      ezv_render (ctx, nb_ctx);
  }

  free (data);

  ezv_ctx_delete (ctx, nb_ctx);
  mesh3d_obj_delete (&mesh);
  ezv_finalize ();

  return 0;
}
