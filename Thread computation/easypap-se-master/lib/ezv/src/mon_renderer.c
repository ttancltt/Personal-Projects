#include <cglm/cglm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "error.h"
#include "ezv_ctx.h"
#include "ezv_hud.h"
#include "ezv_prefix.h"
#include "ezv_sdl_gl.h"
#include "ezv_shader.h"
#include "ezv_textures.h"
#include "ezv_topo.h"
#include "stb_image.h"

#include "ezv_mon_object.h"
#include "mon_renderer.h"

typedef struct mon_render_ctx_s
{
  GLuint UBO_DATACOL;
  GLuint UBO_MAT;
  GLuint UBO_CPU;
  GLuint TBO_COL; // Texture Buffer Object containing RGBA cpu colors
  GLuint VBO;     // Vertex Buffer Object (contains vertices)
  GLuint VAO;     // Vertex Array Object
  GLuint VBO_IND; // Vertex Buffer Object containing triangles (i.e. 3-tuples
                  // indexing vertices)
  GLuint VAO_CPU;
  GLuint VBO_CPU;
  GLuint dataTexture;
  GLuint cpu_shader, data_shader;
  GLuint tex_color; // TBO_COL
  GLuint cpu_colors_loc, cpu_ratios_loc, cpu_binding_loc, cpu_thickness_loc;
  GLuint data_imgtex_loc;
} mon_render_ctx_t;

// Model Matrix
static struct
{
  mat4 mvp;
  mat4 ortho;
  mat4 vp_unclipped;
  mat4 mvp_unclipped;
  mat4 mv;
} Matrices;

static struct
{
  float x_offset;
  float y_offset;
  float y_stride;
} CpuInfo;

#define PREFERRED_PERFMETER_HEIGHT 18
#define PREFERRED_INTERMARGIN 8

#define PERFMETER_WIDTH 256
#define MARGIN 16
#define LEGEND 64

static unsigned PERFMETER_HEIGHT = PREFERRED_PERFMETER_HEIGHT;
static unsigned INTERMARGIN      = PREFERRED_INTERMARGIN;
static unsigned TOPO_COL_WIDTH   = 12;
static unsigned TOPO_COL_SPACING = 6;
static unsigned TOPO             = 0;

// Returns height of window after adjustment, or 0 on failure
static unsigned adjust_perfmeter_size (unsigned nb_rows, unsigned max_height)
{
  PERFMETER_HEIGHT    = PREFERRED_PERFMETER_HEIGHT;
  INTERMARGIN         = PREFERRED_INTERMARGIN;

  for (;;) {
    unsigned h =
        2 * MARGIN + nb_rows * PERFMETER_HEIGHT + (nb_rows - 1) * INTERMARGIN;

    if (h <= max_height)
      return h;

    if (INTERMARGIN > 1)
      INTERMARGIN -= 1;
    else if (PERFMETER_HEIGHT > 4)
      PERFMETER_HEIGHT -= 2;
    else
      return 0; // Failure
  }
}

void ezv_mon_precalc_view_parameters (mon_obj_t *mon, unsigned *width,
                                      unsigned *height)
{
  unsigned w, h, rows;
  unsigned nb_cores  = ezv_topo_get_nb_pus ();
  unsigned nb_combos = ezv_topo_get_nb_combos (mon->folding_level);

  // Not yet supported
  if (mon->mode == EZV_MONITOR_TOPOLOGY_COMPACT)
    exit_with_error ("EZV_MONITOR_TOPOLOGY_COMPACT mode is not yet supported");

  if (mon->mode == EZV_MONITOR_TOPOLOGY) {
    if (mon->thr > nb_cores) {
      fprintf (stderr,
               "Warning: Number of threads (%d) exceeds available cores (%d)."
               " Falling back to EZV_MONITOR_DEFAULT.\n",
               mon->thr, nb_cores);
      mon->mode = EZV_MONITOR_DEFAULT;
    } else if (!ezv_clustering_binding_exists (mon->clustering))
      exit_with_error ("EZV_MONITOR_TOPOLOGY mode requires thread bindings");
  }

  switch (mon->mode) {
  case EZV_MONITOR_DEFAULT:
    rows = mon->thr + mon->gpu;
    break;
  case EZV_MONITOR_TOPOLOGY:
    rows = nb_combos + mon->gpu;
    break;
  default:
    exit_with_error ("Unknown monitor mode");
  }

  for (;;) {
    h = adjust_perfmeter_size (rows, ezv_get_max_display_height ());
    if (h > 0)
      break;

    if (mon->mode == EZV_MONITOR_TOPOLOGY) {
      // Current folding level cannot fit
      ezv_topo_disable_folding (mon->folding_level);
      // Try next folding level
      int next = ezv_topo_next_valid_folding (mon->folding_level);
      fprintf (stderr,
               "Warning: Cannot fit all CPU meters in the window. "
               "Trying %d-level folding…\n",
               next);
      if (next > 0) {
        mon->folding_level = next;
        nb_combos          = ezv_topo_get_nb_combos (mon->folding_level);
        rows               = nb_combos + mon->gpu;
        continue;
      }
      exit_with_error ("Sorry, I'm unable to display so many CPU meters");
    }
  }

  if (mon->mode == EZV_MONITOR_TOPOLOGY) {
    unsigned depth = ezv_topo_get_maxdepth ();

    TOPO = depth * (TOPO_COL_WIDTH + TOPO_COL_SPACING);
  }

  w = 2 * MARGIN + TOPO + LEGEND + PERFMETER_WIDTH;

  if (width)
    *width = w;
  if (height)
    *height = h;
}

static void recalc_view_parameters (ezv_ctx_t ctx)
{
  mon_obj_t *mon = ezv_mon_mon (ctx);
  unsigned h, rows;
  unsigned nb_combos = ezv_topo_get_nb_combos (mon->folding_level);

  if (mon->mode != EZV_MONITOR_TOPOLOGY)
    exit_with_error (
        "recalc_view_parameters expected to be called only in TOPOLOGY mode");

  rows = nb_combos + mon->gpu;

  h = adjust_perfmeter_size (rows, ctx->winh);
  if (h == 0)
    // Not supposed to fail
    exit_with_error ("Unexpected error: cannot fit CPU meters in the window");
}

// How many different colors do we need?
unsigned ezv_mon_get_palette_size (mon_obj_t *mon)
{
  return mon->thr + mon->gpu;
}

static void mon_renderer_mvp_init (ezv_ctx_t ctx)
{
  static int done          = 0;
  mon_render_ctx_t *renctx = ezv_mon_renderer (ctx);

  // Create matrices and vector once
  if (!done) {
    glm_ortho (0.0f, (float)ctx->winw, (float)ctx->winh, 0.0f, -2.0f, 2.0f,
               Matrices.ortho);
    done = 1;
  }

  glGenBuffers (1, &renctx->UBO_MAT);
  glBindBuffer (GL_UNIFORM_BUFFER, renctx->UBO_MAT);
  glBufferData (GL_UNIFORM_BUFFER, sizeof (Matrices), &Matrices,
                GL_STATIC_DRAW);
  glBindBufferBase (GL_UNIFORM_BUFFER, BINDING_POINT_MATRICES, renctx->UBO_MAT);

  CpuInfo.x_offset = MARGIN + TOPO + LEGEND;
  CpuInfo.y_offset = MARGIN;
  CpuInfo.y_stride = PERFMETER_HEIGHT + INTERMARGIN;

  glGenBuffers (1, &renctx->UBO_CPU);
  glBindBuffer (GL_UNIFORM_BUFFER, renctx->UBO_CPU);
  glBufferData (GL_UNIFORM_BUFFER, sizeof (CpuInfo), &CpuInfo, GL_STATIC_DRAW);
  glBindBufferBase (GL_UNIFORM_BUFFER, BINDING_POINT_CPUINFO, renctx->UBO_CPU);
}

static void update_cpuinfo (ezv_ctx_t ctx)
{
  mon_render_ctx_t *renctx = ezv_mon_renderer (ctx);

  CpuInfo.x_offset = MARGIN + TOPO + LEGEND;
  CpuInfo.y_offset = MARGIN;
  CpuInfo.y_stride = PERFMETER_HEIGHT + INTERMARGIN;

  glBindBuffer (GL_UNIFORM_BUFFER, renctx->UBO_CPU);
  glBufferSubData (GL_UNIFORM_BUFFER, 0, sizeof (CpuInfo), &CpuInfo);
}

// called by ctx_create: the mon is not defined yet, nor any palette
void mon_renderer_init (ezv_ctx_t ctx)
{
  ezv_switch_to_context (ctx);

  // configure global opengl state
  // -----------------------------
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable (GL_BLEND);

  // Allocate render_ctx
  mon_render_ctx_t *renctx = malloc (sizeof (mon_render_ctx_t));
  renctx->TBO_COL          = 0;
  renctx->UBO_DATACOL      = 0;
  renctx->UBO_MAT          = 0;
  renctx->VBO              = 0;
  renctx->VAO              = 0;
  renctx->VBO_IND          = 0;
  renctx->VAO_CPU          = 0;
  renctx->VBO_CPU          = 0;
  renctx->tex_color        = 0;
  renctx->dataTexture      = 0;

  ezv_mon_set_renderer (ctx, renctx);

  // compile shaders and build program
  renctx->cpu_shader = ezv_shader_create ("mon/cpu.vs", NULL, "mon/cpu.fs");
  renctx->data_shader =
      ezv_shader_create ("mon/generic.vs", NULL, "mon/generic.fs");

  // Uniform parameters
  ezv_shader_get_uniform_loc (renctx->data_shader, "dataTexture",
                              &renctx->data_imgtex_loc);

  ezv_shader_get_uniform_loc (renctx->cpu_shader, "RGBAColors",
                              &renctx->cpu_colors_loc);
  ezv_shader_get_uniform_loc (renctx->cpu_shader, "ratios",
                              &renctx->cpu_ratios_loc);
  ezv_shader_get_uniform_loc (renctx->cpu_shader, "binding",
                              &renctx->cpu_binding_loc);
  ezv_shader_get_uniform_loc (renctx->cpu_shader, "thickness",
                              &renctx->cpu_thickness_loc);

  // Bind Matrices to all shaders
  ezv_shader_bind_uniform_buf (renctx->cpu_shader, "Matrices",
                               BINDING_POINT_MATRICES);
  ezv_shader_bind_uniform_buf (renctx->data_shader, "Matrices",
                               BINDING_POINT_MATRICES);

  ezv_shader_bind_uniform_buf (renctx->cpu_shader, "CpuInfo",
                               BINDING_POINT_CPUINFO);
}

void mon_renderer_finalize (ezv_ctx_t ctx)
{
  mon_render_ctx_t *renctx = ezv_mon_renderer (ctx);

  glDeleteBuffers (1, &renctx->UBO_MAT);
  glDeleteBuffers (1, &renctx->UBO_CPU);
  glDeleteBuffers (1, &renctx->VBO);
  glDeleteBuffers (1, &renctx->VBO_IND);
  glDeleteBuffers (1, &renctx->TBO_COL);
  glDeleteVertexArrays (1, &renctx->VAO);
  glDeleteVertexArrays (1, &renctx->VAO_CPU);

  glDeleteTextures (1, &renctx->dataTexture);
  glDeleteTextures (1, &renctx->tex_color);

  glDeleteProgram (renctx->cpu_shader);
  glDeleteProgram (renctx->data_shader);

  free (renctx);
  ezv_mon_set_renderer (ctx, NULL);
}

static uint32_t *ascii_data       = NULL;
static SDL_Surface *ascii_surface = NULL;

static void load_ascii_surface (void)
{
  int nrChannels;
  char file [1024];
  int texture_width  = -1;
  int texture_height = -1;

  snprintf (file, sizeof (file), "%s/share/ezv/img/ascii.png", ezv_prefix);

  ascii_data = (uint32_t *)stbi_load (file, &texture_width, &texture_height,
                                      &nrChannels, 0);
  if (ascii_data == NULL)
    exit_with_error ("Cannot open %s", file);

  ascii_surface = SDL_CreateRGBSurfaceFrom (
      ascii_data, texture_width, texture_height, 32,
      texture_width * sizeof (uint32_t), ezv_red_mask (), ezv_green_mask (),
      ezv_blue_mask (), ezv_alpha_mask ());
  if (ascii_surface == NULL)
    exit_with_error ("SDL_CreateRGBSurfaceFrom failed: %s", SDL_GetError ());
}

static void free_ascii_surface (void)
{
  SDL_FreeSurface (ascii_surface);
  free (ascii_data);
}

static void blit_string (SDL_Surface *surface, unsigned x_offset,
                         unsigned y_offset, char *str)
{
  SDL_Rect dst, src;

  dst.x = x_offset;
  dst.y = y_offset;
  dst.h = PERFMETER_HEIGHT + 2;
  dst.w = dst.h / 2;

  src.y = 0;
  src.w = 10;
  src.h = 20;

  for (unsigned i = 0; str [i] != 0; i++) {
    unsigned n = str [i] - ' ';
    src.x      = n * 10;
    SDL_BlitScaled (ascii_surface, &src, surface, &dst);
    dst.x += dst.w;
  }
}

typedef struct
{
  unsigned winw;
  uint32_t *img_data;
  SDL_Surface *surface;
} blit_data_t;

// Warning: 'obj' may be NULL on case the callback is called explicitely
// for GPUs (not included in the topology)
static void blit_obj (hwloc_obj_t obj, int col, int row_inf, int row_sup,
                      void *arg)
{
  blit_data_t *bd = (blit_data_t *)arg;

  const unsigned x_offset =
      MARGIN + col * (TOPO_COL_WIDTH + TOPO_COL_SPACING);
  const unsigned y_offset = MARGIN + row_inf * (PERFMETER_HEIGHT + INTERMARGIN);
  const unsigned height   = (row_sup - row_inf + 1) * PERFMETER_HEIGHT +
                          ((row_sup - row_inf) * INTERMARGIN);
  const unsigned width = TOPO_COL_WIDTH;

  // printf ("Blitting object at col %d (rows %d-%d) offset (%u,%u) size
  // (%u,%u)\n",
  //         col, row_inf, row_sup, x_offset, y_offset, width, height);

  uint32_t color = ezv_rgb (184, 134, 11); // dark goldenrod

  if (ezv_topo_get_nb_pu_kinds () > 1 && col == ezv_topo_get_maxdepth () - 1) {
    int efficiency = ezv_topo_get_pu_efficiency (row_inf);
    if (!efficiency)
      efficiency = -1;
    // increase red component according to efficiency
    const unsigned amount = 25;
    color = ezv_rgb (184 + efficiency * amount, 134 - efficiency * amount, 11);
  }

  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++)
      bd->img_data [(y_offset + i) * bd->winw + x_offset + j] = color;
}

static void blit_fixed (uint32_t color, int col, int row_inf, int row_sup,
                        void *arg)
{
  blit_data_t *bd = (blit_data_t *)arg;

  const unsigned x_offset =
      MARGIN + col * (TOPO_COL_WIDTH + TOPO_COL_SPACING);
  const unsigned y_offset = MARGIN + row_inf * (PERFMETER_HEIGHT + INTERMARGIN);
  const unsigned height   = (row_sup - row_inf + 1) * PERFMETER_HEIGHT +
                          ((row_sup - row_inf) * INTERMARGIN);
  const unsigned width = TOPO_COL_WIDTH;

  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++)
      bd->img_data [(y_offset + i) * bd->winw + x_offset + j] = color;
}

static void draw_perfmeter (blit_data_t *bd, unsigned idx, unsigned block_size,
                            unsigned color, char *label)
{
  unsigned extra_thickness =
      (block_size - 1) * (PERFMETER_HEIGHT + INTERMARGIN);
  unsigned x_offset = MARGIN + TOPO + LEGEND;
  unsigned y_offset = MARGIN + idx * (PERFMETER_HEIGHT + INTERMARGIN);

  if (label != NULL)
    blit_string (bd->surface, MARGIN + TOPO, y_offset + extra_thickness / 2,
                 label);

  for (int i = 0; i < PERFMETER_HEIGHT + extra_thickness; i++)
    for (int j = 0; j < PERFMETER_WIDTH; j++)
      bd->img_data [(y_offset + i) * bd->winw + x_offset + j] = color;

  for (int i = 1; i < PERFMETER_HEIGHT + extra_thickness - 1; i++)
    for (int j = 1; j < PERFMETER_WIDTH - 1; j++)
      bd->img_data [(y_offset + i) * bd->winw + x_offset + j] =
          ezv_rgb (0, 0, 0);
}

static void build_texture (ezv_ctx_t ctx, unsigned update)
{
  mon_render_ctx_t *renctx = ezv_mon_renderer (ctx);
  mon_obj_t *mon           = ezv_mon_mon (ctx);
  blit_data_t bd;

  bd.winw     = ctx->winw;
  bd.img_data = calloc (ctx->winw * ctx->winh, sizeof (uint32_t));

  bd.surface = SDL_CreateRGBSurfaceFrom (
      bd.img_data, bd.winw, ctx->winh, 32, bd.winw * sizeof (uint32_t),
      ezv_red_mask (), ezv_green_mask (), ezv_blue_mask (), ezv_alpha_mask ());
  if (bd.surface == NULL)
    exit_with_error ("SDL_CreateRGBSurfaceFrom failed: %s", SDL_GetError ());

  load_ascii_surface ();

  unsigned nb_combos = ezv_topo_get_nb_combos (mon->folding_level);

  // draw topology
  if (mon->mode == EZV_MONITOR_TOPOLOGY) {

    // Blit CPU topology
    ezv_topo_for_each_obj (mon->folding_level, blit_obj, &bd);

    // blit grey bars for masked levels
    for (int l = 0; l < mon->folding_level; l++) {
      unsigned depth = ezv_topo_get_maxdepth ();
      unsigned col   = depth - l - 1;

      blit_fixed (ezv_rgb (70, 80, 70), col, 0, nb_combos - 1, &bd);
    }

    // Blit GPUs
    for (int g = 0; g < mon->gpu; g++)
      blit_fixed (ezv_rgb (255, 0, 0), ezv_topo_get_maxdepth () - 1,
                  nb_combos + g, nb_combos + g, &bd);
  }

  // draw perfmeters

  // start with thread clusters
  unsigned nb_clusters =
      ezv_clustering_get_nb_clusters (mon->clustering, mon->folding_level);
  for (int c = 0; c < nb_clusters; c++) {
    ezv_cluster_t cluster;
    char label [32];

    ezv_clustering_get_nth_cluster (mon->clustering, mon->folding_level, c,
                                    &cluster);

    if (mon->folding_level == 0)
      snprintf (label, sizeof (label), "Thr%3d ", c);
    else {
      char tmp [16];
      snprintf (tmp, sizeof (tmp), "x%d", cluster.nb_thr);
      snprintf (label, sizeof (label), "Cl%4s", tmp);
    }

    draw_perfmeter (&bd, cluster.first_idx, cluster.size, ctx->cpu_colors [c],
                    label);
  }

  // finish with GPUs
  for (int g = 0; g < mon->gpu; g++) {
    unsigned gpu_idx;
    char label [32];

    if (mon->mode == EZV_MONITOR_TOPOLOGY)
      gpu_idx = nb_combos + g;
    else
      gpu_idx = nb_clusters + g;

    snprintf (label, sizeof (label), "GPU%3d ", g);
    draw_perfmeter (&bd, gpu_idx, 1, ctx->cpu_colors [nb_clusters + g], label);
  }

  free_ascii_surface ();

  if (update) {
    glActiveTexture (GL_TEXTURE0 + EZV_DATA_TEXTURE_NUM);
    glBindTexture (GL_TEXTURE_2D, renctx->dataTexture);
    glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, bd.winw, ctx->winh, GL_RGBA,
                     GL_UNSIGNED_BYTE, bd.img_data);
  } else {
    glGenTextures (1, &renctx->dataTexture);
    glActiveTexture (GL_TEXTURE0 + EZV_DATA_TEXTURE_NUM);
    glBindTexture (GL_TEXTURE_2D, renctx->dataTexture);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, bd.winw, ctx->winh, 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, bd.img_data);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // bind uniform buffer object to data texture
    glProgramUniform1i (renctx->data_shader, renctx->data_imgtex_loc,
                        EZV_DATA_TEXTURE_NUM);
  }

  SDL_FreeSurface (bd.surface);
  free (bd.img_data);
}

static void transfer_binding (ezv_ctx_t ctx)
{
  mon_render_ctx_t *renctx = ezv_mon_renderer (ctx);
  mon_obj_t *mon           = ezv_mon_mon (ctx);

  unsigned nb_clusters =
      ezv_clustering_get_nb_clusters (mon->clustering, mon->folding_level);
  unsigned nb_combos = ezv_topo_get_nb_combos (mon->folding_level);

  int offset [nb_clusters + mon->gpu];
  float thickness [nb_clusters + mon->gpu];

  // start with thread clusters
  for (int c = 0; c < nb_clusters; c++) {
    ezv_cluster_t cluster;

    ezv_clustering_get_nth_cluster (mon->clustering, mon->folding_level, c,
                                    &cluster);

    offset [c]    = cluster.first_idx;
    thickness [c] = PERFMETER_HEIGHT +
                    (cluster.size - 1) * (PERFMETER_HEIGHT + INTERMARGIN);
  }

  // finish with GPUs
  for (int g = 0; g < mon->gpu; g++) {
    unsigned gpu_idx = nb_clusters + g;
    if (mon->mode == EZV_MONITOR_TOPOLOGY)
      offset [gpu_idx] = nb_combos + g;
    else
      offset [gpu_idx] = gpu_idx;
    thickness [gpu_idx] = PERFMETER_HEIGHT;
  }

  glProgramUniform1iv (renctx->cpu_shader, renctx->cpu_binding_loc,
                       nb_clusters + mon->gpu, (GLint *)offset);
  glProgramUniform1fv (renctx->cpu_shader, renctx->cpu_thickness_loc,
                       nb_clusters + mon->gpu, (GLfloat *)thickness);
}

void mon_renderer_set_mon (ezv_ctx_t ctx)
{
  mon_render_ctx_t *renctx = ezv_mon_renderer (ctx);
  mon_obj_t *mon           = ezv_mon_mon (ctx);

  ezv_switch_to_context (ctx);

  // Initialize 'Matrices'
  mon_renderer_mvp_init (ctx);

  float vertices [] = {
      // 2D positions     // tex coord
      0.0f,
      0.0f,
      0.0f,
      0.0f, // top left
      0.0f,
      (float)(ctx->winh),
      0.0f,
      1.0f, // bottom left
      (float)(ctx->winw),
      0.0f,
      1.0f,
      0.0f, // top right
      (float)(ctx->winw),
      (float)(ctx->winh),
      1.0f,
      1.0f, // bottom right
  };

  // Warning: use clockwise orientation
  unsigned int indices [] = {
      0, 3, 1, // first triangle
      0, 2, 3  // second triangle
  };

  // configure vertex attributes and misc buffers
  glGenVertexArrays (1, &renctx->VAO);
  glGenBuffers (1, &renctx->VBO);
  glGenBuffers (1, &renctx->VBO_IND);

  // bind the Vertex Array Object first, then bind and set vertex buffer(s)
  glBindVertexArray (renctx->VAO);

  glBindBuffer (GL_ARRAY_BUFFER, renctx->VBO);
  glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, renctx->VBO_IND);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices,
                GL_STATIC_DRAW);

  // configure vertex attributes(s).
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (float),
                         (void *)0);
  glEnableVertexAttribArray (0);

  // configure texture coordinates
  glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (float),
                         (void *)(2 * sizeof (float)));
  glEnableVertexAttribArray (1);

  // Perfmeters
  float cpuv [] = {
      // 2D positions
      0.0f,
      0.0f, // top left
      0.0f,
      1.0f, // bottom left
      (float)PERFMETER_WIDTH,
      0.0f, // top right
      (float)PERFMETER_WIDTH,
      1.0f // bottom right
  };

  glGenVertexArrays (1, &renctx->VAO_CPU);
  glGenBuffers (1, &renctx->VBO_CPU);

  // bind the Vertex Array Object first, then bind and set vertex buffer(s)
  glBindVertexArray (renctx->VAO_CPU);

  glBindBuffer (GL_ARRAY_BUFFER, renctx->VBO_CPU);
  glBufferData (GL_ARRAY_BUFFER, sizeof (cpuv), cpuv, GL_STATIC_DRAW);

  // We keep the same triangle indice buffer
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, renctx->VBO_IND);

  // configure vertex attributes(s).
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof (float),
                         (void *)0);
  glEnableVertexAttribArray (0);

  // Texture Buffer Object containing RGBA colors
  glGenBuffers (1, &renctx->TBO_COL);
  glBindBuffer (GL_TEXTURE_BUFFER, renctx->TBO_COL);
  glBufferData (GL_TEXTURE_BUFFER, (mon->thr + mon->gpu) * sizeof (int), NULL,
                GL_STATIC_DRAW); // Sent only once

  glActiveTexture (GL_TEXTURE0 + EZV_CPU_TEXTURE_NUM);
  glGenTextures (1, &renctx->tex_color);
  glBindTexture (GL_TEXTURE_BUFFER, renctx->tex_color);
  glTexBuffer (GL_TEXTURE_BUFFER, GL_R32I, renctx->TBO_COL);

  // bind uniform buffer object to cpu texture
  glProgramUniform1i (renctx->cpu_shader, renctx->cpu_colors_loc,
                      EZV_CPU_TEXTURE_NUM);

  transfer_binding (ctx);
}

void mon_set_data_colors (ezv_ctx_t ctx, void *values)
{
  mon_render_ctx_t *renctx = ezv_mon_renderer (ctx);
  mon_obj_t *mon           = ezv_mon_mon (ctx);
  unsigned nb_clusters =
      ezv_clustering_get_nb_clusters (mon->clustering, mon->folding_level);

  ezv_switch_to_context (ctx);

  if (mon->mode == EZV_MONITOR_TOPOLOGY) {
    float per_cluster_values [nb_clusters + mon->gpu];

    // Copy values for GPUs
    for (int g = 0; g < mon->gpu; g++)
      per_cluster_values [nb_clusters + g] = ((float *)values) [mon->thr + g];

    for (int c = 0; c < nb_clusters; c++)
      per_cluster_values [c] = 0.0f;

    for (int t = 0; t < mon->thr; t++) {
      unsigned cluster_idx = ezv_clustering_get_cluster_index_from_thr (
          mon->clustering, mon->folding_level, t);
      per_cluster_values [cluster_idx] += ((float *)values) [t];
    }

    // For clusters of weight > 1, compute average
    for (int c = 0; c < nb_clusters; c++) {
      unsigned weight = ezv_clustering_get_cluster_weight (
          mon->clustering, mon->folding_level, c);
      if (weight > 1)
        per_cluster_values [c] /= (float)weight;
    }

    glProgramUniform1fv (renctx->cpu_shader, renctx->cpu_ratios_loc,
                         nb_clusters + mon->gpu, per_cluster_values);

  } else
    glProgramUniform1fv (renctx->cpu_shader, renctx->cpu_ratios_loc,
                         nb_clusters + mon->gpu, values);
}

static void transfer_rgba_colors (ezv_ctx_t ctx, unsigned update)
{
  if (ezv_palette_is_defined (&ctx->cpu_palette)) {
    mon_render_ctx_t *renctx = ezv_mon_renderer (ctx);
    mon_obj_t *mon           = ezv_mon_mon (ctx);

    glBindBuffer (GL_TEXTURE_BUFFER, renctx->TBO_COL);
    glBufferSubData (GL_TEXTURE_BUFFER, 0, (mon->thr + mon->gpu) * sizeof (int),
                     ctx->cpu_colors);
  } else
    exit_with_error ("CPU palette unconfigured\n");

  build_texture (ctx, update);
}

void ezv_mon_folding_has_changed (ezv_ctx_t ctx)
{
  recalc_view_parameters (ctx);
  update_cpuinfo (ctx);
  transfer_rgba_colors (ctx, 1);
  transfer_binding (ctx);
}

void mon_render (ezv_ctx_t ctx)
{
  mon_render_ctx_t *renctx = ezv_mon_renderer (ctx);
  mon_obj_t *mon           = ezv_mon_mon (ctx);

  ezv_switch_to_context (ctx);

  glClearColor (0.0f, 0.2f, 0.2f, 1.0f);

  glClear (GL_COLOR_BUFFER_BIT |
           GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

  // transfer RGBA cpu colors only once
  static int done = 0;
  if (!done) {
    transfer_rgba_colors (ctx, 0);
    done = 1;
  }

  // Background image
  glBindVertexArray (renctx->VAO);
  glUseProgram (renctx->data_shader);

  glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  // Perf meters
  glBindVertexArray (renctx->VAO_CPU);
  glUseProgram (renctx->cpu_shader);

  unsigned nb_elems;
  if (mon->mode == EZV_MONITOR_TOPOLOGY) {
    unsigned nb_clusters =
        ezv_clustering_get_nb_clusters (mon->clustering, mon->folding_level);
    nb_elems = nb_clusters + mon->gpu;
  } else {
    nb_elems = mon->thr + mon->gpu;
  }

  glDrawElementsInstanced (GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, nb_elems);

  SDL_GL_SwapWindow (ctx->win);
}
