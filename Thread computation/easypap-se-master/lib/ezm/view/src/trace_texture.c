#include "trace_texture.h"
#include "error.h"
#include "ezv_sdl_gl.h"
#include "trace_colors.h"
#include "trace_graphics.h"
#include "trace_view.h"
#include "trace_topo.h"

#include <SDL_image.h>
#include <SDL_ttf.h>

SDL_Texture *black_square      = NULL;
SDL_Texture *dark_square       = NULL;
SDL_Texture *white_square      = NULL;
SDL_Texture **perf_fill        = NULL;
SDL_Texture *text_texture      = NULL;
SDL_Texture *vertical_line     = NULL;
SDL_Texture *horizontal_line   = NULL;
SDL_Texture *horizontal_bis    = NULL;
SDL_Texture *bulle_tex         = NULL;
SDL_Texture *reduced_bulle_tex = NULL;
SDL_Texture *us_tex            = NULL;
SDL_Texture *sigma_tex         = NULL;
SDL_Texture *tab_left          = NULL;
SDL_Texture *tab_right         = NULL;
SDL_Texture *tab_high          = NULL;
SDL_Texture *tab_low           = NULL;
SDL_Texture *align_tex         = NULL;
SDL_Texture *quick_nav_tex     = NULL;
SDL_Texture *track_tex         = NULL;
SDL_Texture *footprint_tex     = NULL;
SDL_Texture *digit_tex [10]    = {NULL};
SDL_Texture *mouse_tex         = NULL;

static TTF_Font *the_font = NULL;

unsigned digit_tex_width [10];
unsigned digit_tex_height;

static char easyview_img_dir [1024];
static char easyview_font_dir [1024];
char easyview_ezv_dir [1024];

#define BLACK_COL ezv_rgb (0, 0, 0)
#define DARK_COL ezv_rgb (60, 60, 60)
#define WHITE_COL ezv_rgb (255, 255, 255)

#define SQUARE_SIZE 16

SDL_Color silver_color  = {192, 192, 192, 255};
SDL_Color backgrd_color = {0, 51, 51, 255}; //{50, 50, 65, 255};

int BUBBLE_WIDTH, BUBBLE_HEIGHT, REDUCED_BUBBLE_WIDTH, REDUCED_BUBBLE_HEIGHT;

static void find_shared_directories (void)
{
  char *pi = stpcpy (easyview_img_dir, SDL_GetBasePath ());
  char *pf = stpcpy (easyview_font_dir, easyview_img_dir);
  char *pv = stpcpy (easyview_ezv_dir, easyview_img_dir);

#ifdef INSTALL_DIR // compiled with CMAKE
  strcpy (pi, "../share/ezm/img/");
  strcpy (pf, "../share/ezm/fonts/");
  strcpy (pv, "../");
#else
  strcpy (pi, "../share/ezm/img/");
  strcpy (pf, "../share/ezm/fonts/");
  strcpy (pv, "../../../ezv");
#endif
}

static TTF_Font *load_font (const char *filename, int ptsize)
{
  char path [1024];
  TTF_Font *f;
  char *p = stpcpy (path, easyview_font_dir);
  strcpy (p, filename);

  f = TTF_OpenFont (path, ptsize);
  if (f == NULL)
    exit_with_error ("TTF_OpenFont (%s) failed: %s", filename, TTF_GetError ());

  return f;
}

static SDL_Surface *load_img (const char *filename)
{
  char path [1024];
  SDL_Surface *s;
  char *p = stpcpy (path, easyview_img_dir);
  strcpy (p, filename);

  s = IMG_Load (path);
  if (s == NULL)
    exit_with_error ("IMG_Load (%s) failed: %s", filename, SDL_GetError ());

  return s;
}

TTF_Font *trace_texture_get_default_font (void)
{
  return the_font;
}

static void create_task_textures (void)
{
  Uint32 *restrict img =
      malloc (gantts_bounding_box.w * TASK_HEIGHT * sizeof (Uint32));

  perf_fill = malloc ((TRACE_MAX_COLORS + 1) * sizeof (SDL_Texture *));

  SDL_Surface *s = SDL_CreateRGBSurfaceFrom (
      img, gantts_bounding_box.w, TASK_HEIGHT, 32,
      gantts_bounding_box.w * sizeof (Uint32), ezv_red_mask (),
      ezv_green_mask (), ezv_blue_mask (), ezv_alpha_mask ());
  if (s == NULL)
    exit_with_error ("SDL_CreateRGBSurfaceFrom () failed");

  unsigned largeur_couleur_origine = gantts_bounding_box.w / 4;
  unsigned largeur_degrade = gantts_bounding_box.w - largeur_couleur_origine;
  float attenuation_depart = 1.0;
  float attenuation_finale = 0.3;

  for (int c = 0; c < TRACE_MAX_COLORS + 1; c++) {
    bzero (img, gantts_bounding_box.w * TASK_HEIGHT * sizeof (Uint32));

    if (c == TRACE_MAX_COLORS) // special treatment for white color
      attenuation_finale = 0.5;

    for (int j = 0; j < gantts_bounding_box.w; j++) {
      uint32_t couleur = trace_colors_get_color (c);
      uint8_t r        = ezv_c2r (couleur);
      uint8_t g        = ezv_c2g (couleur);
      uint8_t b        = ezv_c2b (couleur);

      if (j >= largeur_couleur_origine) {
        float coef =
            attenuation_depart -
            ((((float)(j - largeur_couleur_origine)) / largeur_degrade)) *
                (attenuation_depart - attenuation_finale);
        r = r * coef;
        g = g * coef;
        b = b * coef;
      }

      for (int i = 0; i < TASK_HEIGHT; i++)
        img [i * gantts_bounding_box.w + j] = ezv_rgb (r, g, b);
    }

    perf_fill [c] = SDL_CreateTextureFromSurface (renderer, s);
  }

  SDL_FreeSurface (s);
  free (img);

  s = SDL_CreateRGBSurface (0, SQUARE_SIZE, SQUARE_SIZE, 32, ezv_red_mask (),
                            ezv_green_mask (), ezv_blue_mask (),
                            ezv_alpha_mask ());
  if (s == NULL)
    exit_with_error ("SDL_CreateRGBSurface () failed");

  SDL_FillRect (s, NULL, BLACK_COL); // back
  black_square = SDL_CreateTextureFromSurface (renderer, s);

  SDL_FillRect (s, NULL, DARK_COL); // dark
  dark_square = SDL_CreateTextureFromSurface (renderer, s);

  SDL_FillRect (s, NULL, WHITE_COL); // white
  white_square = SDL_CreateTextureFromSurface (renderer, s);

  SDL_FreeSurface (s);
}

static void create_digit_textures (TTF_Font *font)
{
  SDL_Color white_color = {255, 255, 255, 255};
  SDL_Surface *s        = NULL;

  for (int c = 0; c < 10; c++) {
    char msg [32];
    snprintf (msg, 32, "%d", c);

    s = TTF_RenderUTF8_Blended (font, msg, white_color);
    if (s == NULL)
      exit_with_error ("TTF_RenderText_Solid failed: %s", SDL_GetError ());

    digit_tex_width [c] = s->w;
    digit_tex_height    = s->h;
    digit_tex [c]       = SDL_CreateTextureFromSurface (renderer, s);

    SDL_FreeSurface (s);
  }

  s = TTF_RenderUTF8_Blended (font, "µs", white_color);
  if (s == NULL)
    exit_with_error ("TTF_RenderText_Solid failed: %s", SDL_GetError ());

  us_tex = SDL_CreateTextureFromSurface (renderer, s);
  SDL_FreeSurface (s);

  s = TTF_RenderUTF8_Blended (font, "Σ: ", white_color);
  if (s == NULL)
    exit_with_error ("TTF_RenderText_Solid failed: %s", SDL_GetError ());

  sigma_tex = SDL_CreateTextureFromSurface (renderer, s);
  SDL_FreeSurface (s);
}

static void create_tab_textures (TTF_Font *font)
{
  SDL_Surface *surf = load_img ("tab-left.png");
  tab_left          = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_FreeSurface (surf);

  surf     = load_img ("tab-high.png");
  tab_high = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_FreeSurface (surf);

  surf      = load_img ("tab-right.png");
  tab_right = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_FreeSurface (surf);

  surf    = load_img ("tab-low.png");
  tab_low = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_FreeSurface (surf);

  for (int t = 0; t < nb_traces; t++) {
    SDL_Surface *s =
        TTF_RenderUTF8_Blended (font, trace [t].label, backgrd_color);
    if (s == NULL)
      exit_with_error ("TTF_RenderUTF8_Blended failed: %s", SDL_GetError ());

    trace_display_info [t].label_tex =
        SDL_CreateTextureFromSurface (renderer, s);
    trace_display_info [t].label_width  = s->w;
    trace_display_info [t].label_height = s->h;
    SDL_FreeSurface (s);
  }
}

static void create_task_ids_textures (TTF_Font *font)
{
  for (int t = 0; t < nb_traces; t++) {
    trace_display_info [t].task_ids_tex =
        calloc (trace [t].task_ids_count, sizeof (SDL_Texture *));
    trace_display_info [t].task_ids_tex_width =
        calloc (trace [t].task_ids_count, sizeof (unsigned));

    for (int i = 0; i < trace [t].task_ids_count; i++) {
      SDL_Surface *s =
          TTF_RenderUTF8_Blended (font, trace [t].task_ids [i], silver_color);
      if (s == NULL)
        exit_with_error ("TTF_RenderUTF8_Blended failed: %s", SDL_GetError ());

      trace_display_info [t].task_ids_tex [i] =
          SDL_CreateTextureFromSurface (renderer, s);
      trace_display_info [t].task_ids_tex_width [i] = s->w;
      SDL_FreeSurface (s);
    }
  }
}

static void to_sdl_color (uint32_t src, SDL_Color *dst)
{
  dst->r = ezv_c2r (src);
  dst->g = ezv_c2g (src);
  dst->b = ezv_c2b (src);
  dst->a = ezv_c2a (src);
}

static void blit_string (SDL_Surface *surface, TTF_Font *font, unsigned y,
                         char *msg, unsigned color)
{
  SDL_Rect dst;
  SDL_Color col;

  to_sdl_color (color, &col);

  SDL_Surface *s = TTF_RenderUTF8_Blended (font, msg, col);
  if (s == NULL)
    exit_with_error ("TTF_RenderUTF8_Blended failed: %s", SDL_GetError ());

  if (s->h > cpu_row_height (TASK_HEIGHT)) {
    // reduce dst.w and dst.h to fit in the row height
    float ratio = (float)cpu_row_height (TASK_HEIGHT) / (float)s->h;
    dst.w       = s->w * ratio;
    dst.h       = cpu_row_height (TASK_HEIGHT);
  } else {
    dst.w = s->w;
    dst.h = s->h;
  }
  dst.x = gantts_bounding_box.x - dst.w;
  dst.y = y - (dst.h / 2);

  SDL_BlitScaled (s, NULL, surface, &dst);
  SDL_FreeSurface (s);
}

typedef struct
{
  unsigned width;
  unsigned y;
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
      SIDE_MARGIN + col * (TOPO_COL_WIDTH + TOPO_COL_SPACING);
  const unsigned y_offset =
      bd->y + row_inf * (cpu_row_height (TASK_HEIGHT)) + Y_MARGIN;
  const unsigned height =
      (row_sup - row_inf + 1) * cpu_row_height (TASK_HEIGHT) - 2 * Y_MARGIN;
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
      bd->img_data [(y_offset + i) * bd->width + x_offset + j] = color;
}

static void blit_fixed (uint32_t color, int col, int row_inf, int row_sup,
                        void *arg)
{
  blit_data_t *bd = (blit_data_t *)arg;

  const unsigned x_offset =
      SIDE_MARGIN + col * (TOPO_COL_WIDTH + TOPO_COL_SPACING);
  const unsigned y_offset =
      bd->y + row_inf * (cpu_row_height (TASK_HEIGHT)) + Y_MARGIN;
  const unsigned height =
      (row_sup - row_inf + 1) * cpu_row_height (TASK_HEIGHT) - 2 * Y_MARGIN;
  const unsigned width = TOPO_COL_WIDTH;

  for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++)
      bd->img_data [(y_offset + i) * bd->width + x_offset + j] = color;
}

void trace_texture_create_left_panel (void)
{
  TTF_Font *font = trace_texture_get_default_font ();
  blit_data_t bd;

  bd.width    = gantts_bounding_box.x;
  bd.img_data = calloc (bd.width * WINDOW_HEIGHT, sizeof (uint32_t));

  bd.surface = SDL_CreateRGBSurfaceFrom (
      bd.img_data, bd.width, WINDOW_HEIGHT, 32, bd.width * sizeof (uint32_t),
      ezv_red_mask (), ezv_green_mask (), ezv_blue_mask (), ezv_alpha_mask ());
  if (bd.surface == NULL)
    exit_with_error ("SDL_CreateRGBSurfaceFrom failed: %s", SDL_GetError ());

  for (int t = 0; t < nb_traces; t++) {

    bd.y = trace_display_info [t].gantt.y;

    // Display topology only if thread binding info is available
    if (easyview_show_topo && trace [t].thr_binding) {
      unsigned nb_combos = ezv_topo_get_nb_combos (trace [t].folding_level);

      // Blit CPU topology
      ezv_topo_for_each_obj (trace [t].folding_level, blit_obj, &bd);

      // blit grey bars for masked levels
      for (int l = 0; l < trace [t].folding_level; l++) {
        unsigned depth = ezv_topo_get_maxdepth ();
        unsigned col   = depth - l - 1;

        blit_fixed (ezv_rgb (70, 80, 70), col, 0, nb_combos - 1, &bd);
      }

      // Blit GPUs
      for (int g = 0; g < (trace [t].nb_gpu >> 1); g++)
        blit_fixed (ezv_rgb (255, 0, 0), ezv_topo_get_maxdepth () - 1,
                    nb_combos + 2 * g, nb_combos + 2 * g + 1, &bd);
    }

    // Draw labels and start with thread clusters
    unsigned nb_clusters = ezv_clustering_get_nb_clusters (
        trace [t].clustering, trace [t].folding_level);
    for (int c = 0; c < nb_clusters; c++) {
      ezv_cluster_t cluster;
      char label [32];
      unsigned col = trace_colors_get_index (trace [t].num, c);

      ezv_clustering_get_nth_cluster (trace [t].clustering,
                                      trace [t].folding_level, c, &cluster);

      if (trace [t].folding_level == 0)
        snprintf (label, sizeof (label), "Thr%3d ", c);
      else {
        char tmp [16];
        snprintf (tmp, sizeof (tmp), "x%d", cluster.nb_thr);
        snprintf (label, sizeof (label), "Cl%4s", tmp);
      }

      blit_string (bd.surface, font,
                   bd.y + cluster.first_idx * cpu_row_height (TASK_HEIGHT) +
                       (cluster.size * cpu_row_height (TASK_HEIGHT)) / 2,
                   label, trace_colors_get_color (col));
    }

    // finish with GPUs
    for (int g = 0; g < trace [t].nb_gpu; g++) {
      unsigned gpu_idx;
      char label [32];
      unsigned col = trace_colors_get_index (trace [t].num, nb_clusters + g);

      if (easyview_show_topo && trace [t].thr_binding)
        gpu_idx = ezv_topo_get_nb_combos (trace [t].folding_level) + g;
      else
        gpu_idx = nb_clusters + g;

      if (g % 2 == 0) {
        snprintf (label, sizeof (label), "GPU%3d ", g >> 1);
        blit_string (bd.surface, font,
                     bd.y + gpu_idx * cpu_row_height (TASK_HEIGHT) +
                         cpu_row_height (TASK_HEIGHT) / 2,
                     label, trace_colors_get_color (col));
      } else {
        snprintf (label, sizeof (label), "in");
        blit_string (bd.surface, font,
                     bd.y + gpu_idx * cpu_row_height (TASK_HEIGHT) +
                         cpu_row_height (TASK_HEIGHT) / 4,
                     label, trace_colors_get_color (col));
        snprintf (label, sizeof (label), "out");
        blit_string (bd.surface, font,
                     bd.y + gpu_idx * cpu_row_height (TASK_HEIGHT) +
                         (3 * cpu_row_height (TASK_HEIGHT)) / 4,
                     label, trace_colors_get_color (col));
      }
    }
  }

  if (text_texture != NULL)
    SDL_DestroyTexture (text_texture);

  text_texture = SDL_CreateTextureFromSurface (renderer, bd.surface);
  if (text_texture == NULL)
    exit_with_error ("SDL_CreateTexture failed: %s", SDL_GetError ());

  SDL_FreeSurface (bd.surface);
  free (bd.img_data);
}

static void create_text_texture (TTF_Font *font)
{
  trace_texture_create_left_panel ();
  create_digit_textures (font);
  create_tab_textures (font);
  create_task_ids_textures (font);
}

static void create_misc_tex (void)
{
  SDL_Surface *surf = SDL_CreateRGBSurface (
      0, 2, WINDOW_HEIGHT, 32, ezv_red_mask (), ezv_green_mask (),
      ezv_blue_mask (), ezv_alpha_mask ());
  if (surf == NULL)
    exit_with_error ("SDL_CreateRGBSurface failed: %s", SDL_GetError ());

  SDL_FillRect (surf, NULL, SDL_MapRGB (surf->format, 0, 255, 255));
  vertical_line = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_SetTextureBlendMode (vertical_line, SDL_BLENDMODE_BLEND);
  SDL_FreeSurface (surf);

  surf = SDL_CreateRGBSurface (0, gantts_bounding_box.w, 2, 32, ezv_red_mask (),
                               ezv_green_mask (), ezv_blue_mask (),
                               ezv_alpha_mask ());
  if (surf == NULL)
    exit_with_error ("SDL_CreateRGBSurface failed: %s", SDL_GetError ());

  SDL_FillRect (surf, NULL, SDL_MapRGB (surf->format, 0, 255, 255));
  horizontal_line = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_SetTextureBlendMode (horizontal_line, SDL_BLENDMODE_BLEND);

  SDL_FillRect (surf, NULL, SDL_MapRGB (surf->format, 150, 150, 200));
  horizontal_bis = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_SetTextureBlendMode (horizontal_bis, SDL_BLENDMODE_BLEND);

  SDL_FreeSurface (surf);

  surf      = load_img ("mouse-cursor.png");
  mouse_tex = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_FreeSurface (surf);

  surf = load_img ("frame.png");

  BUBBLE_WIDTH  = surf->w;
  BUBBLE_HEIGHT = surf->h;

  bulle_tex = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_FreeSurface (surf);

  surf = load_img ("frame-reduced.png");

  REDUCED_BUBBLE_WIDTH  = surf->w;
  REDUCED_BUBBLE_HEIGHT = surf->h;

  reduced_bulle_tex = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_FreeSurface (surf);

  surf = load_img ("quick-nav.png");

  quick_nav_tex = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_FreeSurface (surf);
  SDL_SetTextureAlphaMod (quick_nav_tex, quick_nav_mode ? 255 : BUTTON_ALPHA);

  SDL_QueryTexture (quick_nav_tex, NULL, NULL, &quick_nav_rect.w,
                    &quick_nav_rect.h);

  surf = load_img ("auto-align.png");

  align_tex = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_FreeSurface (surf);
  SDL_SetTextureAlphaMod (align_tex,
                          trace_data_align_mode ? 255 : BUTTON_ALPHA);

  SDL_QueryTexture (align_tex, NULL, NULL, &align_rect.w, &align_rect.h);

  surf = load_img ("track-mode.png");

  track_tex = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_FreeSurface (surf);
  SDL_SetTextureAlphaMod (track_tex, tracking_mode ? 255 : BUTTON_ALPHA);

  SDL_QueryTexture (track_tex, NULL, NULL, &track_rect.w, &track_rect.h);

  surf = load_img ("footprint.png");

  footprint_tex = SDL_CreateTextureFromSurface (renderer, surf);
  SDL_FreeSurface (surf);
  SDL_SetTextureAlphaMod (footprint_tex, footprint_mode ? 255 : BUTTON_ALPHA);

  SDL_QueryTexture (footprint_tex, NULL, NULL, &footprint_rect.w,
                    &footprint_rect.h);
}

void trace_texture_init (void)
{
  find_shared_directories ();

  if (TTF_Init () < 0)
    exit_with_error ("TTF_Init failed: %s", SDL_GetError ());

  the_font = load_font ("FreeSansBold.ttf", FONT_HEIGHT - 4);
  if (the_font == NULL)
    exit_with_error ("TTF_OpenFont failed: %s", SDL_GetError ());

  create_task_textures ();

  create_text_texture (the_font);

  create_misc_tex ();
}

void trace_texture_clean (void)
{

  if (the_font != NULL) {
    TTF_CloseFont (the_font);
    the_font = NULL;
  }

  TTF_Quit ();
}
