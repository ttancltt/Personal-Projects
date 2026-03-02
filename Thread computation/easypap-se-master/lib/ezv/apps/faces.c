
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

static void do_pick (void)
{
  int p = ezv_perform_1D_picking (ctx, nb_ctx);

  ezv_reset_cpu_colors (ctx[0]);

  if (p == -1)
    ezv_hud_off (ctx[0], hud);
  else {
    ezv_hud_on (ctx[0], hud);
    ezv_hud_set (ctx[0], hud, "Cell: %d", p);

    // Highlight selected cell
    ezv_set_cpu_color_1D (ctx[0], p, 1, ezv_rgb (255, 255, 255));
  }
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

static unsigned vi  = 0;
static unsigned ti  = 0;
static unsigned nbt = 0;
static unsigned nbv = 0;

static int add_vertice (mesh3d_obj_t *mesh, float x, float y, float z)
{
  mesh->vertices[vi++] = x;
  mesh->vertices[vi++] = y;
  mesh->vertices[vi++] = z;

  return nbv++;
}

static int add_triangle (mesh3d_obj_t *mesh, unsigned v1, unsigned v2,
                         unsigned v3)
{
  mesh->triangles[ti++] = v1;
  mesh->triangles[ti++] = v2;
  mesh->triangles[ti++] = v3;

  return nbt++;
}

static void display_stats (mesh3d_obj_t *mesh)
{
  printf ("Mesh: %d cells, %d triangles, %d vertices, %d total neighbors\n",
          mesh->nb_cells, mesh->nb_triangles, mesh->nb_vertices,
          mesh->total_neighbors);
  if (mesh->nb_patches > 0)
    printf ("Mesh already partitionned into %d patches\n", mesh->nb_patches);
}

static void sanity_check (mesh3d_obj_t *mesh)
{
  // for each cell
  for (int c = 0; c < mesh->nb_cells; c++) {
    // for each triangle of cell c
    for (int t = mesh->cells[c]; t < mesh->cells[c + 1]; t++) {
      unsigned encoded_cell = mesh->triangle_info[t] >> CELLNO_SHIFT;
      if (encoded_cell != c)
        exit_with_error (
            "Inconsistency detected between cells[] and triangle_info[]: "
            "triangle_info[%d].cell == %d, %d expected\n",
            t, encoded_cell, c);
    }
  }
  printf ("No inconsistency detected\n");
}

static void create_mesh (mesh3d_obj_t *mesh, unsigned nbx)
{
  const unsigned line    = nbx;
  const unsigned surface = 2 * line;
  const unsigned volume  = 2 * surface;

  mesh->mesh_type     = MESH3D_TYPE_SURFACE;
  mesh->nb_vertices   = volume;
  mesh->vertices      = malloc (mesh->nb_vertices * 3 * sizeof (float));
  mesh->nb_triangles  = 2 * nbx;
  mesh->triangles     = malloc (mesh->nb_triangles * 3 * sizeof (unsigned));
  mesh->triangle_info = calloc (mesh->nb_triangles, sizeof (unsigned));

  float i, j, k;
  float xinc;

  xinc = 1.0f / (float)(nbx - 1);

  // First, generate all vertices
  for (int z = 1; z >= 0; z--) {
    k = -0.5f + z * xinc;
    for (int y = 0; y <= 1; y++) {
      j = -0.5f + y * xinc;
      for (int x = 0; x < nbx; x++) {
        i = -0.1f + x * 0.2f / (float)(nbx - 1);
        add_vertice (mesh, i, j, k);
      }
    }
  }

  if (mesh->nb_vertices != nbv)
    exit_with_error ("mesh3d: the final number of vertices (%d) does not match "
                     "the initial one (%d)",
                     nbv, mesh->nb_vertices);

  unsigned v[4];

  // Now create triangles
  for (int z = 0; z < 1; z++) {
    for (int y = 0; y < 1; y++) {
      for (int x = 0; x < nbx; x++) {
        v[0] = z * surface + y * line + x;
        v[1] = (z + 1) * surface + y * line + x;
        v[2] = z * surface + (y + 1) * line + x;
        v[3] = (z + 1) * surface + (y + 1) * line + x;

        // Create triangles Clockwise
        mesh->triangle_info[nbt] |= EDGE1;
        mesh->triangle_info[nbt + 1] |= EDGE0;
        add_triangle (mesh, v[0], v[2], v[1]); // front
        add_triangle (mesh, v[1], v[2], v[3]);
      }
    }
  }

  // Et maintenant, formons les cellules Alice!
  int indt       = 0, c;
  mesh->nb_cells = nbx;
  // cells is an array of indexes. Cells[i] contains the index of its first
  // triangle. Triangles belonging to the same cell are stored contiguously. We
  // allocate one more index so that the triangle range is simply
  // cell[i]..cell[i+1]-1
  mesh->cells = malloc ((mesh->nb_cells + 1) * sizeof (unsigned));
  for (c = 0; c < mesh->nb_cells; c++) {
    mesh->cells[c] = indt;
    for (int g = 0; g < 2; g++)
      mesh->triangle_info[indt++] |= (c << CELLNO_SHIFT);
  }
  // extra cell
  mesh->cells[c] = indt;

  // neighbors
  mesh->min_neighbors = 0;
  mesh->max_neighbors = 0;

  display_stats (mesh);

  sanity_check (mesh);
}

int main (int argc, char *argv[])
{
  unsigned nb_cells = 5;

  if (argc > 1) {
    int n = atoi (argv[1]);
    if (n > 1 && n <= 64)
      nb_cells = n;
  }

  ezv_init ();

  mesh3d_obj_init (&mesh);
  create_mesh (&mesh, nb_cells);

  // mesh3d_obj_store ("output.obj", &mesh, 0);

  // Create SDL windows and initialize OpenGL context
  ctx[0] = ezv_ctx_create (EZV_CTX_TYPE_MESH3D, "Mesh", SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_UNDEFINED, SCR_WIDTH, SCR_HEIGHT,
                           EZV_ENABLE_PICKING | EZV_ENABLE_HUD |
                               EZV_ENABLE_CLIPPING);
  hud    = ezv_hud_alloc (ctx[0]);
  ezv_hud_on (ctx[0], hud);

  // Attach mesh
  ezv_mesh3d_set_mesh (ctx[0], &mesh);

    // Heat Palette (as simple as defining these five key colors)
  float colors[] = {1.0f, 0.5f, 0.0f, 0.5f,  // orange
                    0.0f, 1.0f, 1.0f, 0.5f}; // cyan

  ezv_use_data_colors (ctx[0], colors, 2);

  ezv_use_cpu_colors (ctx[0]);

  float *values = malloc (mesh.nb_cells * sizeof (float));

  // Color cells
  for (int c = 0; c < mesh.nb_cells; c++)
    values[c] = 1.0f;

  ezv_set_data_colors (ctx[0], values);

  // render loop
  int cont = 1;
  while (cont) {
    cont = process_events ();
    if (cont)
      ezv_render (ctx, nb_ctx);
  }

  free (values);
  
  ezv_ctx_delete (ctx, nb_ctx);
  mesh3d_obj_delete (&mesh);
  ezv_finalize ();
  
  return 0;
}
