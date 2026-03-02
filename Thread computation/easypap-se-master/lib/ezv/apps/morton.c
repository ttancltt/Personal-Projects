
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
static ezv_ctx_t ctx[MAX_CTX] = {NULL, NULL};
static unsigned nb_ctx        = 1;
static int hud                = -1;

static float *data            = NULL;
static int datasize           = 0;
static unsigned selected_cell = -1;

static void do_pick (void)
{
  selected_cell = ezv_perform_1D_picking (ctx, nb_ctx);

  memset (data, 0, datasize);

  if (selected_cell != -1) {
    ezv_hud_set (ctx[0], hud, "Cell: %d", selected_cell);

    data[selected_cell] = 1.0; // selected triangle

    for (int v = mesh.index_first_neighbor[selected_cell];
         v < mesh.index_first_neighbor[selected_cell + 1]; v++)
      data[mesh.neighbors[v]] = 0.2;
  } else
    ezv_hud_set (ctx[0], hud, NULL);

  ezv_set_data_colors (ctx[0], data);
}

static void display_cell_stats (void)
{
  if (selected_cell == -1)
    return;

  printf ("Selected cell: %d\n", selected_cell);

  for (int index = mesh.cells[selected_cell];
       index < mesh.cells[selected_cell + 1]; index++) {
    int vind[3];
    printf ("   Triangle %d:\n", index);
    for (int v = 0; v < 3; v++) {
      vind[v] = mesh.triangles[3 * index + v];
      printf ("\tVertex[%d]: %f, %f, %f\n", vind[v],
              mesh.vertices[3 * vind[v] + 0], mesh.vertices[3 * vind[v] + 1],
              mesh.vertices[3 * vind[v] + 2]);
    }
  }
  printf ("   Neighbors:\n");
  for (int v = mesh.index_first_neighbor[selected_cell];
       v < mesh.index_first_neighbor[selected_cell + 1]; v++)
    printf ("\tneigh[%d] = %d\n", v, mesh.neighbors[v]);
}

static int process_events (void)
{
  SDL_Event event;
  int pick;
  int ret;

  ezv_get_event (&event, EZV_EVENT_BLOCKING);

  if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
    display_cell_stats ();
    return 1;
  }

  ret = ezv_process_event (ctx, nb_ctx, &event, NULL, &pick);
  if (pick)
    do_pick ();

  return ret;
}

static void load_mesh (int argc, char *argv[], mesh3d_obj_t *mesh)
{
  if (argc > 1) {

    if (!strcmp (argv[1], "--help") || !strcmp (argv[1], "-h")) {
      printf ("Usage: %s [ -t1 | -t2 | -tv | -cv | -cy | <file> ]\n", argv[0]);
      exit (0);
    } else if (!strcmp (argv[1], "--torus-1") || !strcmp (argv[1], "-t1"))
      mesh3d_obj_build_torus_surface (mesh, 1);
    else if (!strcmp (argv[1], "--torus-2") || !strcmp (argv[1], "-t2"))
      mesh3d_obj_build_torus_surface (mesh, 2);
    else if (!strcmp (argv[1], "--torus-volumus") || !strcmp (argv[1], "-tv"))
      mesh3d_obj_build_torus_volume (mesh, 32, 16, 16);
    else if (!strcmp (argv[1], "--cubus-volumus") || !strcmp (argv[1], "-cv"))
      mesh3d_obj_build_cube_volume (mesh, 16);
    else if (!strcmp (argv[1], "--cylinder-volumus") ||
             !strcmp (argv[1], "-cy"))
      mesh3d_obj_build_cylinder_volume (mesh, 400, 200);
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
  ctx[0] = ezv_ctx_create (
      EZV_CTX_TYPE_MESH3D, "Morton", SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_UNDEFINED, SCR_WIDTH, SCR_HEIGHT,
      EZV_ENABLE_PICKING | EZV_ENABLE_HUD | EZV_ENABLE_CLIPPING);

  hud = ezv_hud_alloc (ctx[0]);
  ezv_hud_on (ctx[0], hud);

  // Attach mesh
  ezv_mesh3d_set_mesh (ctx[0], &mesh);

  // Heat Palette (as simple as defining these five key colors)
  float colors[] = {0.0f, 0.0f, 0.0f, 0.0f,  // transparent
                    0.0f, 1.0f, 1.0f, 1.0f,  // cyan
                    0.0f, 1.0f, 0.0f, 1.0f,  // green
                    1.0f, 1.0f, 0.0f, 1.0f,  // yellow
                    1.0f, 0.0f, 0.0f, 1.0f}; // red

  ezv_use_data_colors (ctx[0], colors, 5);

  datasize = mesh.nb_cells * sizeof (float);
  data     = malloc (datasize);

  memset (data, 0, datasize);

  ezv_set_data_colors (ctx[0], data);

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
