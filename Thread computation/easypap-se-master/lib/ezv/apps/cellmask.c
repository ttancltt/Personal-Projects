
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "ezv.h"
#include "ezv_event.h"

#define USE_CPU_COLORS

// settings
const unsigned int SCR_WIDTH  = 1024;
const unsigned int SCR_HEIGHT = 768;

#define MAX_CTX 2

static mesh3d_obj_t mesh;
static ezv_ctx_t ctx[MAX_CTX] = {NULL, NULL};
static unsigned nb_ctx        = 1;
static int hud                = -1;
static int mask_hud           = -1;
static uint8_t *mask = NULL;

static void do_pick (void)
{
#ifdef USE_CPU_COLORS
  ezv_reset_cpu_colors (ctx[0]);
#endif

  int p = ezv_perform_1D_picking (ctx, nb_ctx);

  if (p == -1)
    ezv_hud_off (ctx[0], hud);
  else {
    ezv_hud_on (ctx[0], hud);
    ezv_hud_set (ctx[0], hud, "Cell: %d", p);

#ifdef USE_CPU_COLORS
    if (mesh.nb_patches != 0) {
      int partoche = mesh3d_obj_get_patch_of_cell (&mesh, p);
      ezv_set_cpu_color_1D (ctx[0], mesh.patch_first_cell[partoche],
                            mesh.patch_first_cell[partoche + 1] -
                                mesh.patch_first_cell[partoche],
                            ezv_rgba (255, 255, 255, 200));
    }
    ezv_set_cpu_color_1D (ctx[0], p, 1, ezv_rgba (255, 0, 0, 255));
#endif
  }
}

static uint8_t *mask_alloc (mesh3d_obj_t *mesh)
{
  uint8_t *mask = calloc (mesh->nb_cells, sizeof (uint8_t));
  if (!mask)
    exit_with_error ("Cannot allocate memory for cell mask");

  for (int c = 0; c < mesh->nb_cells; c++)
    mask[c] = (c & 8) ? 0 : 1;
  
  return mask;
}

static void mask_free (uint8_t *mask)
{
  if (mask) {
    free (mask);
    mask = NULL;
  }
}
static void do_mask (void)
{
  static int mode = 0;

  mode = (mode + 1) % 5;
  switch (mode) {
  case 0:
    ezv_hud_set (ctx[0], mask_hud, "Mode: normal");
    ezv_cellmask_reset (ctx[0]);
    break;
  case 1:
    ezv_hud_set (ctx[0], mask_hud, "Mode: semi-transparent");
    ezv_cellmask_set1D_alpha (ctx[0], 100, mesh.nb_cells / 4,
                              mesh.nb_cells / 2, NULL);
    break;
  case 2:
    ezv_hud_set (ctx[0], mask_hud, "Mode: transparent");
    ezv_cellmask_set1D_alpha (ctx[0], 0, mesh.nb_cells / 4, mesh.nb_cells / 2, NULL);
    break;
  case 3:
    ezv_hud_set (ctx[0], mask_hud, "Mode: hidden");
    ezv_cellmask_set1D_hidden (ctx[0], mesh.nb_cells / 4, mesh.nb_cells / 2, NULL);
    break;
  case 4:
    ezv_hud_set (ctx[0], mask_hud, "Mode: mask");
    ezv_cellmask_reset (ctx[0]);
    ezv_cellmask_set1D_hidden (ctx[0], 0, mesh.nb_cells, mask);
    break;
  default:
    exit_with_error ("We should not be here");
    break;
  }
}

static int process_events (void)
{
  SDL_Event event;
  int pick = 0;
  int ret  = 1;

  ezv_get_event (&event, EZV_EVENT_BLOCKING);

  if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_m) {
    do_mask ();
    pick = 1;
  } else
    ret = ezv_process_event (ctx, nb_ctx, &event, NULL, &pick);

  if (pick)
    do_pick ();

  return ret;
}

static void load_mesh (int argc, char *argv[], mesh3d_obj_t *mesh)
{
  if (argc > 1) {
    unsigned group = 1;

    if (argc > 2)
      group = atoi (argv[2]);

    if (!strcmp (argv[1], "--help") || !strcmp (argv[1], "-h")) {
      printf ("Usage: %s [ -t | -c | -cv | -tv | -cy ] "
              "[cell_size]\n",
              argv[0]);
      exit (0);
    } else if (!strcmp (argv[1], "--torus") || !strcmp (argv[1], "-t"))
      mesh3d_obj_build_torus_surface (mesh, group);
    else if (!strcmp (argv[1], "--cube") || !strcmp (argv[1], "-c"))
      mesh3d_obj_build_cube (mesh, group);
    else if (!strcmp (argv[1], "--cubus-volumus") || !strcmp (argv[1], "-cv"))
      mesh3d_obj_build_cube_volume (mesh, group);
    else if (!strcmp (argv[1], "--torus-volumus") || !strcmp (argv[1], "-tv"))
      mesh3d_obj_build_torus_volume (mesh, 32, 16, 16);
    else if (!strcmp (argv[1], "--cylinder-volumus") ||
             !strcmp (argv[1], "-cy"))
      mesh3d_obj_build_cylinder_volume (mesh, 400, 200);
    else if (!strcmp (argv[1], "--wall") || !strcmp (argv[1], "-w"))
      mesh3d_obj_build_wall (mesh, 8);
    else
      mesh3d_obj_load (argv[1], mesh);
  } else
    mesh3d_obj_build_default (mesh);
}

int main (int argc, char *argv[])
{
  ezv_init ();

  mesh3d_obj_init (&mesh);
  load_mesh (argc, argv, &mesh);

  // Create SDL windows and initialize OpenGL context
  ctx[0] = ezv_ctx_create (EZV_CTX_TYPE_MESH3D, "Mesh", SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_UNDEFINED, SCR_WIDTH, SCR_HEIGHT,
                           EZV_ENABLE_PICKING | EZV_ENABLE_HUD |
                               EZV_ENABLE_CLIPPING | EZV_ENABLE_CELLMASK);

  mask_hud = ezv_hud_alloc (ctx[0]);
  ezv_hud_on (ctx[0], mask_hud);
  ezv_hud_set (ctx[0], mask_hud, "Mode: normal");
  hud = ezv_hud_alloc (ctx[0]);
  ezv_hud_on (ctx[0], hud);

  // Attach mesh
  ezv_mesh3d_set_mesh (ctx[0], &mesh);

  ezv_use_data_colors_predefined (ctx[0], EZV_PALETTE_RAINBOW);
#ifdef USE_CPU_COLORS
  ezv_use_cpu_colors (ctx[0]);
#endif

  float *values = malloc (mesh.nb_cells * sizeof (float));
  mask = mask_alloc (&mesh);

  // Color cells according to their position within the bounding box
  const int COORD = 2;
  for (int c = 0; c < mesh.nb_cells; c++) {
    bbox_t box;
    mesh3d_obj_get_bbox_of_cell (&mesh, c, &box);
    float z   = (box.min[COORD] + box.max[COORD]) / 2.0f;
    values[c] = (z - mesh.bbox.min[COORD]) /
                (mesh.bbox.max[COORD] - mesh.bbox.min[COORD]);
  }

  ezv_set_data_colors (ctx[0], values);

  // render loop
  int cont = 1;
  while (cont) {
    cont = process_events ();
    if (cont)
      ezv_render (ctx, nb_ctx);
  }

  free (values);
  free (mask);

  ezv_ctx_delete (ctx, nb_ctx);
  mesh3d_obj_delete (&mesh);
  ezv_finalize ();
  
  return 0;
}
