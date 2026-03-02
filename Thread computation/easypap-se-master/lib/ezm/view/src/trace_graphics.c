#include "trace_graphics.h"
#include "error.h"
#include "ezv.h"
#include "ezv_sdl_gl.h"
#include "min_max.h"
#include "trace_colors.h"
#include "trace_layout.h"
#include "trace_texture.h"
#include "trace_topo.h"
#include "trace_view.h"

#include <SDL_image.h>
#include <SDL_ttf.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static enum {
  EASYVIEW_MODE_UNDEFINED,
  EASYVIEW_MODE_2D_IMAGES,
  EASYVIEW_MODE_3D_MESHES
} easyview_mode = EASYVIEW_MODE_UNDEFINED;

#define MAX_FILENAME 1024

#define MAGNIFICATION 2
#define TILE_ALPHA 0xC0

#define MOSAIC_SIZE 512

static void **thumb_data [MAX_TRACES] = {NULL, NULL};

static Uint32 main_windowID = 0;

static SDL_Window *window = NULL;
SDL_Renderer *renderer    = NULL;

unsigned char brightness = 150;

extern char *trace_dir []; // Defined in main.c

static ezv_ctx_t ctx [MAX_TRACES] = {NULL, NULL};

static SDL_Point mouse_pick = {-1, 0};
// 3D meshes
static mesh3d_obj_t mesh;
// 2D images
static img2d_obj_t img2d;

static inline int point_inside_mosaic (const SDL_Point *p, unsigned trace_num)
{
  return ezv_ctx_is_in_focus (ctx [trace_num]);
}

static inline int point_inside_mosaics (const SDL_Point *p)
{
  return point_inside_mosaic (p, 0) ||
         ((nb_traces == 1) ? 0 : (point_inside_mosaic (p, 1)));
}

static inline int rects_do_intersect (const SDL_Rect *r1, const SDL_Rect *r2)
{
  return SDL_HasIntersection (r1, r2);
}

static inline void get_raw_rect (trace_task_t *t, SDL_Rect *dst)
{
  dst->x = t->x;
  dst->w = t->w;

  dst->y = t->y;
  if (easyview_mode != EASYVIEW_MODE_3D_MESHES)
    dst->h = t->h;
  else
    dst->h = 1;
}

static int get_gpu_from_index (trace_t *tr, unsigned idx, unsigned *row)
{
  if (idx >= tr->nb_gpu)
    return -1;

  if (!easyview_show_topo || !tr->thr_binding) {
    *row = tr->nb_thr - tr->nb_gpu + idx;
    return 0;
  }

  // topology display enabled
  unsigned nb_combos = ezv_topo_get_nb_combos (tr->folding_level);
  *row               = nb_combos + idx;
  return 0;
}

static int get_cluster_from_index (trace_t *tr, unsigned idx,
                                   ezv_cluster_t *cluster)
{
  if (!easyview_show_topo || !tr->thr_binding) {
    // No topology display: each thread is its own cluster
    if (idx >= tr->nb_thr - tr->nb_gpu)
      return -1;

    cluster->first_idx = idx;
    cluster->size      = 1;

    return 0;
  }

  // topology display enabled
  unsigned nb_clusters =
      ezv_clustering_get_nb_clusters (tr->clustering, tr->folding_level);

  if (idx < nb_clusters) {
    // thr cluster
    ezv_clustering_get_nth_cluster (tr->clustering, tr->folding_level, idx,
                                    cluster);
    return 0;
  }

  return -1;
}

static int mirror_mouse_using_clusters (trace_t *tr, int dy)
{
  const unsigned row_height = cpu_row_height (TASK_HEIGHT);
  ezv_cluster_t src_cluster, dst_cluster;
  int idx             = -1;
  int gpu             = 0;
  const unsigned slot = dy / row_height;

  if (!easyview_show_topo || !tr->thr_binding) {
    // is idx a GPU index?
    if (slot >= tr->nb_thr - tr->nb_gpu) {
      idx = slot - (tr->nb_thr - tr->nb_gpu);
      gpu = 1;
    } else {
      src_cluster.first_idx = slot;
      src_cluster.size      = 1;
      idx                   = slot;
    }
  } else {
    unsigned nb_combos = ezv_topo_get_nb_combos (tr->folding_level);

    if (slot < nb_combos) {
      idx = ezv_clustering_get_cluster_index_from_combo (
          tr->clustering, tr->folding_level, slot);
      if (idx == -1)
        return -1;

      ezv_clustering_get_nth_cluster (tr->clustering, tr->folding_level, idx,
                                      &src_cluster);
    } else {
      // GPU cluster
      idx = slot - nb_combos;
      gpu = 1;
    }
  }

  if (!gpu) {
    // We now get the corresponding cluster in the sibbling trace
    int r = get_cluster_from_index (trace + (1 - tr->num), idx, &dst_cluster);

    if (r == -1)
      return -1;

    // Compute dy so that its position in the sibbling cluster is proportional
    // to the one inside the source cluster
    dy = row_height * dst_cluster.first_idx +
         ((dy - row_height * src_cluster.first_idx + 1) * dst_cluster.size) /
             src_cluster.size -
         1;
  } else {
    unsigned row;
    int r = get_gpu_from_index (trace + (1 - tr->num), idx, &row);

    if (r == -1)
      return -1;

    dy = row_height * row + dy % row_height;
  }

  return dy;
}

static int get_y_mouse_sibbling (trace_t *tr)
{
  if (tr != NULL) {
    const unsigned num = tr->num;
    int dy             = mouse.y - trace_display_info [num].gantt.y;

    dy = mirror_mouse_using_clusters (tr, dy);

    if (dy != -1 && dy < trace_display_info [1 - num].gantt.h)
      return trace_display_info [1 - num].gantt.y + dy;
  }
  return mouse.y;
}

static inline int is_thr (trace_t *const tr, int cpu_num)
{
  return cpu_num < (tr->nb_thr - tr->nb_gpu);
}

static inline int is_gpu (trace_t *const tr, int cpu_num)
{
  return !is_thr (tr, cpu_num);
}

static int is_lane (trace_t *const tr, int cpu_num)
{
  int tot = tr->nb_thr - tr->nb_gpu;
  if (cpu_num >= tot)
    return (cpu_num - tot) & 1; // #GPU is odd

  return 0;
}

// Texture creation functions

static float *mesh_load_raw_data (const char *filename)
{
  int fd = open (filename, O_RDONLY);
  if (fd == -1)
    return NULL;

  off_t len = lseek (fd, 0L, SEEK_END);
  if (len % sizeof (float) != 0)
    exit_with_error ("%s size should be a multiple of sizeof(float)", filename);

  lseek (fd, 0L, SEEK_SET);

  float *data = malloc (len);
  int n       = read (fd, data, len);
  if (n != len)
    exit_with_error ("Read from %s returned less data than expected", filename);

  close (fd);
  return data;
}

static unsigned preload_thumbnails (unsigned nb_iter)
{
  unsigned success = 0, expected = 0;

  unsigned nb_dirs   = 0;
  unsigned bound [2] = {nb_iter, nb_iter};
  char *dir [2]      = {trace_dir [0], trace_dir [1]};

  if (trace_dir [1] ||
      trace [0].first_iteration != trace [nb_traces - 1].first_iteration) {
    // Ok, we have to use two separate arrays to store textures,
    // either because thumbnails are located in two separate folders
    // or because we compare two traces starting from a different iteration
    // number. (Note that in this latter case -- and if iteration ranges
    // overlap -- we could probably try to load once and shift the indexes in
    // the second array... I don't think it is worth it.)

    nb_dirs = 2;
    if (!dir [1])
      dir [1] = dir [0];
    bound [0]      = trace [0].nb_iterations;
    bound [1]      = trace [1].nb_iterations;
    expected       = bound [0] + bound [1];
    thumb_data [0] = malloc (bound [0] * sizeof (void *));
    thumb_data [1] = malloc (bound [1] * sizeof (void *));
  } else {
    // We use a unique array to store thumbnails, either because we're
    // displaying a single trace or because we compare two traces with one
    // iteration range being a subset of the other (in this case, nb_iter is
    // the maximum number of iterations).
    nb_dirs        = 1;
    expected       = bound [0];
    thumb_data [0] = malloc (nb_iter * sizeof (void *));
    thumb_data [1] = thumb_data [0];
  }

  for (int d = 0; d < nb_dirs; d++)
    for (int iter = 0; iter < bound [d]; iter++) {
      void *thumb = NULL;
      char filename [MAX_FILENAME];
      img2d_obj_t thumb2d;
      img2d_obj_init (&thumb2d, trace [0].dimensions, trace [0].dimensions);

      if (easyview_mode == EASYVIEW_MODE_2D_IMAGES) {
        sprintf (filename, "%s/thumb_%04d.png", dir [d],
                 trace [d].first_iteration + iter);
        if (access (filename, R_OK) != -1) {
          thumb = malloc (img2d_obj_size (&thumb2d));
          img2d_obj_load_resized (&thumb2d, filename, thumb);
        }
      } else {
        sprintf (filename, "%s/thumb_%04d.raw", dir [d],
                 trace [d].first_iteration + iter);
        thumb = mesh_load_raw_data (filename);
      }

      if (thumb != NULL) {
        success++;
        thumb_data [d][iter] = thumb;
      } else
        thumb_data [d][iter] = NULL;
    }

  printf ("%d/%u thumbnails successfully preloaded\n", success, expected);

  return success;
}

// Display functions

static void show_tile (trace_t *tr, trace_task_t *t, unsigned color_index,
                       unsigned highlight)
{
  uint32_t task_color = trace_colors_get_color (color_index);

  if (easyview_mode == EASYVIEW_MODE_3D_MESHES) {
    if (t->w) {
      ezv_set_cpu_color_1D (
          ctx [tr->num], t->x, t->w,
          (highlight ? task_color
                     : (task_color & ezv_rgb_mask ()) | ezv_a2c (TILE_ALPHA)));
    }
  } else {
    if (t->w && t->h) {
      ezv_set_cpu_color_2D (
          ctx [tr->num], t->x, t->w, t->y, t->h,
          (highlight ? task_color
                     : (task_color & ezv_rgb_mask ()) | ezv_a2c (TILE_ALPHA)));
    }
  }
}

static void display_tab (unsigned trace_num)
{
  SDL_Rect dst;

  dst.x = trace_display_info [trace_num].gantt.x;
  dst.y = trace_display_info [trace_num].gantt.y - 24;
  dst.w = 32;
  dst.h = 24;
  SDL_RenderCopy (renderer, tab_left, NULL, &dst);

  dst.x += 8;
  dst.y = trace_display_info [trace_num].gantt.y - 24;
  dst.w = trace_display_info [trace_num].label_width;
  dst.h = 24;
  SDL_RenderCopy (renderer, tab_high, NULL, &dst);

  dst.y += 2;
  dst.w = trace_display_info [trace_num].label_width;
  dst.h = trace_display_info [trace_num].label_height;
  SDL_RenderCopy (renderer, trace_display_info [trace_num].label_tex, NULL,
                  &dst);

  dst.x += dst.w;
  dst.y -= 2;
  dst.w = 32;
  dst.h = 24;
  SDL_RenderCopy (renderer, tab_right, NULL, &dst);

  dst.x += 32;
  dst.w =
      trace_display_info [trace_num].gantt.w; // We don't care, thanks clipping!
  SDL_RenderCopy (renderer, tab_low, NULL, &dst);
}

static void display_text (void)
{
  SDL_Rect dst;

  dst.x = 0;
  dst.y = 0;
  dst.w = gantts_bounding_box.x;
  dst.h = WINDOW_HEIGHT;

  SDL_RenderCopy (renderer, text_texture, NULL, &dst);
}

static void display_iter_number (unsigned iter, unsigned y_offset,
                                 unsigned x_offset, unsigned max_size)
{
  unsigned digits [10];
  unsigned nbd = 0, width;
  SDL_Rect dst;

  do {
    digits [nbd] = iter % 10;
    iter /= 10;
    nbd++;
  } while (iter > 0);

  width = nbd * digit_tex_width [0]; // approx

  dst.x = x_offset + max_size / 2 - width / 2;
  dst.y = y_offset;
  dst.h = digit_tex_height;

  for (int d = nbd - 1; d >= 0; d--) {
    unsigned the_digit = digits [d];
    dst.w              = digit_tex_width [the_digit];

    SDL_RenderCopy (renderer, digit_tex [the_digit], NULL, &dst);

    dst.x += digit_tex_width [the_digit];
  }
}

static void display_duration (unsigned long task_duration, unsigned x_offset,
                              unsigned y_offset, unsigned max_size,
                              unsigned with_sigma)
{
  unsigned digits [10];
  unsigned nbd = 0, width;
  SDL_Rect dst;

  do {
    digits [nbd] = task_duration % 10;
    task_duration /= 10;
    nbd++;
  } while (task_duration > 0);

  width = (nbd + 2 + 2 * with_sigma) * digit_tex_width [0]; // approx

  dst.x = x_offset + max_size / 2 - width / 2;
  dst.y = y_offset;
  dst.h = digit_tex_height;

  if (with_sigma) {
    dst.w = 19;
    SDL_RenderCopy (renderer, sigma_tex, NULL, &dst);
    dst.x += dst.w;
  }

  dst.w = digit_tex_width [0];

  for (int d = nbd - 1; d >= 0; d--) {
    unsigned the_digit = digits [d];

    SDL_RenderCopy (renderer, digit_tex [the_digit], NULL, &dst);

    dst.x += digit_tex_width [the_digit];
  }

  dst.w = 18;
  SDL_RenderCopy (renderer, us_tex, NULL, &dst);
}

static void display_selection (void)
{
  if (selection_duration > 0) {
    SDL_Rect dst;

    dst.x = time_to_pixel (selection_start_time);
    dst.y = trace_display_info [0].gantt.y;
    dst.w =
        time_to_pixel (selection_start_time + selection_duration) - dst.x + 1;
    dst.h = gantts_bounding_box.h;

    SDL_SetRenderDrawColor (renderer, 255, 255, 255, 100);
    SDL_RenderFillRect (renderer, &dst);

    display_duration (selection_duration, dst.x + dst.w / 2, dst.y + dst.h / 2,
                      0, 0);
  }
}

static void display_bubble (int x, int y, unsigned long duration,
                            unsigned with_sigma)
{
  SDL_Rect dst;

  dst.x = x - (REDUCED_BUBBLE_WIDTH >> 1);
  dst.y = y - REDUCED_BUBBLE_HEIGHT;
  dst.w = REDUCED_BUBBLE_WIDTH;
  dst.h = REDUCED_BUBBLE_HEIGHT;

  SDL_RenderCopy (renderer, reduced_bulle_tex, NULL, &dst);

  display_duration (duration, x - (REDUCED_BUBBLE_WIDTH >> 1) + 1, dst.y + 1,
                    dst.w - 3, with_sigma);
}

typedef struct
{
  trace_task_t *task;
  trace_t *trace;
  long cumulated_duration;
  SDL_Rect area;
} selected_task_info_t;

#define SELECTED_TASK_INFO_INITIALIZER                                         \
  {                                                                            \
      NULL,                                                                    \
      NULL,                                                                    \
      0,                                                                       \
      {0, 0, 0, 0},                                                            \
  }

static void display_mouse_selection (const selected_task_info_t *selected)
{
  SDL_Rect dst;

  if (horiz_mode) {
    if (!footprint_mode) {
      // horizontal bar
      if (mouse_in_gantt_zone) {
        dst.x = gantts_bounding_box.x;
        dst.y = mouse.y;
        dst.w = gantts_bounding_box.w;
        dst.h = 1;

        SDL_RenderCopy (renderer, horizontal_line, NULL, &dst);

        if (nb_traces > 1) {
          dst.y = get_y_mouse_sibbling (selected->trace);
          if (dst.y != mouse.y)
            SDL_RenderCopy (renderer, horizontal_bis, NULL, &dst);
        }
      }
    }
  } else {
    trace_t *tr     = selected->trace;
    trace_task_t *t = selected->task;

    // vertical bar
    if (mouse_in_gantt_zone) {
      dst.x = mouse.x;
      dst.w = 1;
      if (tracking_mode) {
        if (tr != NULL) {
          dst.x = mouse.x;
          dst.y = trace_display_info [tr->num].gantt.y;
          dst.w = 1;
          dst.h = trace_display_info [tr->num].gantt.h;
          SDL_RenderCopy (renderer, vertical_line, NULL, &dst);
        }
      } else {
        dst.x = mouse.x;
        dst.y = trace_display_info [0].gantt.y;
        dst.w = 1;
        dst.h = gantts_bounding_box.h;
        SDL_RenderCopy (renderer, vertical_line, NULL, &dst);
      }

      if (t != NULL) {
        display_bubble (mouse.x, trace_display_info [tr->num].gantt.y - 4,
                        t->end_time - t->start_time, 0);

        if (tracking_mode) {
          int y = trace_display_info [1 - tr->num].gantt.y - 4;
          display_bubble (mouse.x, y, selected->cumulated_duration, 1);
        }

        if (t->task_id) { // Do not display "anonymous" IDs
          dst.w = trace_display_info [tr->num].task_ids_tex_width [t->task_id];
          dst.h = FONT_HEIGHT;
          dst.x = mouse.x - dst.w / 2;
          dst.y = trace_display_info [tr->num].gantt.y +
                  trace_display_info [tr->num].gantt.h;
          SDL_RenderCopy (
              renderer, trace_display_info [tr->num].task_ids_tex [t->task_id],
              NULL, &dst);
        }
      }
    }
  }
}

static void display_misc_status (void)
{
  SDL_RenderCopy (renderer, quick_nav_tex, NULL, &quick_nav_rect);
  SDL_RenderCopy (renderer, footprint_tex, NULL, &footprint_rect);

  if (nb_traces > 1) {
    SDL_RenderCopy (renderer, align_tex, NULL, &align_rect);
    SDL_RenderCopy (renderer, track_tex, NULL, &track_rect);
  }
}

static void display_tile_background (int tr)
{
  static int displayed_iter [MAX_TRACES] = {-1, -1};
  static void *tex [MAX_TRACES]          = {NULL, NULL};

  if (mouse_in_gantt_zone) {
    long time = pixel_to_time (mouse.x);
    int iter  = trace_data_search_iteration (&trace [tr], time);

    if (iter != -1 && iter != displayed_iter [tr]) {
      displayed_iter [tr] = iter;
      tex [tr]            = thumb_data [tr][iter];
    }
  }

  if (tex [tr] != NULL) {
    if (easyview_mode == EASYVIEW_MODE_2D_IMAGES)
      ezv_set_data_colors (ctx [tr], tex [tr]);
    else
      ezv_set_data_colors (ctx [tr], tex [tr]);
  }
}

static void inline get_y_bounds_from_pu (trace_t *tr, unsigned pu,
                                         SDL_Rect *bounds)
{
  const unsigned nb_clusters =
      ezv_clustering_get_nb_clusters (tr->clustering, tr->folding_level);
  ezv_cluster_t cluster;

  if (pu < nb_clusters) {
    ezv_clustering_get_nth_cluster (tr->clustering, tr->folding_level, pu,
                                    &cluster);
  } else {
    unsigned g = pu - nb_clusters;

    if (easyview_show_topo && tr->thr_binding) {
      unsigned nb_combos = ezv_topo_get_nb_combos (tr->folding_level);
      cluster.first_idx  = nb_combos + g;
    } else {
      cluster.first_idx = nb_clusters + g;
    }
    cluster.size = 1;
  }

  bounds->y = trace_display_info [tr->num].gantt.y +
              cpu_row_height (TASK_HEIGHT) * cluster.first_idx + Y_MARGIN;
  bounds->h = cluster.size * TASK_HEIGHT + (cluster.size - 1) * 2 * Y_MARGIN;
}

static void inline get_y_bounds_from_thread (trace_t *tr, unsigned thr,
                                             SDL_Rect *bounds)
{
  if (is_thr (tr, thr)) {
    const unsigned cluster_idx = ezv_clustering_get_cluster_index_from_thr (
        tr->clustering, tr->folding_level, thr);

    get_y_bounds_from_pu (tr, cluster_idx, bounds);
  } else {
    const unsigned nb_clusters =
        ezv_clustering_get_nb_clusters (tr->clustering, tr->folding_level);
    unsigned gpu_idx = thr - (tr->nb_thr - tr->nb_gpu);
    get_y_bounds_from_pu (tr, gpu_idx + nb_clusters, bounds);
  }
}

static void display_gantt_background (trace_t *tr, int _t, int first_it)
{
  display_tab (_t);

  // Display iterations' background and number
  for (unsigned it = first_it;
       (it < tr->nb_iterations) && (iteration_start_time (tr, it) < view_end);
       it++) {
    SDL_Rect r;

    r.x = time_to_pixel (iteration_start_time (tr, it));
    r.y = trace_display_info [_t].gantt.y;
    r.w = time_to_pixel (iteration_end_time (tr, it)) - r.x + 1;
    r.h = trace_display_info [_t].gantt.h;

    // Background of iterations is either black (odd) or dark (even)
    {
      SDL_Texture *ptr_tex = (it % 2 ? dark_square : black_square);

      SDL_Rect dst = {r.x, 0, r.w, 0};

      for (int c = 0; c < tr->nb_thr; c++) {
        get_y_bounds_from_pu (tr, c, &dst);
        SDL_RenderCopy (renderer, ptr_tex, NULL, &dst);
      }
    }

    if (trace_data_align_mode && tr->iteration [it].gap > 0) {
      SDL_Rect gap;

      gap.x =
          time_to_pixel (iteration_end_time (tr, it) - tr->iteration [it].gap);
      gap.y = r.y;
      gap.w = r.x + r.w - gap.x;
      gap.h = r.h;

      SDL_SetRenderDrawColor (renderer, 0, 90, 0, 255);
      SDL_RenderFillRect (renderer, &gap);
    }

    display_iter_number (tr->first_iteration + it,
                         trace_display_info [_t].gantt.y +
                             trace_display_info [_t].gantt.h + 1,
                         r.x, r.w);
  }
}

static void inline magnify (SDL_Rect *r)
{
  r->x -= MAGNIFICATION;
  r->y -= MAGNIFICATION;
  r->w += MAGNIFICATION * 2;
  r->h += MAGNIFICATION * 2;
}

static void trace_graphics_display_trace (unsigned _t,
                                          selected_task_info_t *selected)
{
  trace_t *const tr              = trace + _t;
  const unsigned first_it        = trace_ctrl [_t].first_displayed_iter - 1;
  uint64_t mouse_time            = 0;
  unsigned mouse_iter            = 0;
  unsigned mosaic_tile_displayed = 0;

  ezv_reset_cpu_colors (ctx [_t]);

  // Set clipping region
  {
    SDL_Rect clip = trace_display_info [0].gantt;

    // We enlarge the clipping area along the y-axis to enable display of
    // iteration numbers
    clip.y = 0;
    clip.h = WINDOW_HEIGHT;

    SDL_RenderSetClipRect (renderer, &clip);
  }

  display_gantt_background (tr, _t, first_it);

  display_tile_background (_t);

  SDL_Point virt_mouse = mouse;
  int in_mosaic        = 0;

  // Normalize (virt_mouse.x, virt_mouse.y) mouse coordinates
  if (point_inside_mosaic (&mouse, _t)) {
    // Mouse is over our tile mosaic
    in_mosaic           = 1;
    mouse_in_gantt_zone = 0;
  } else if ((nb_traces > 1) && point_inside_mosaic (&mouse, 1 - _t)) {
    // Mouse is over the other tile mosaic
    in_mosaic           = 1;
    virt_mouse.x        = mouse.x;
    virt_mouse.y        = mouse.y;
    mouse_in_gantt_zone = 0;
  } else if (mouse_in_gantt_zone) {
    mouse_time = pixel_to_time (mouse.x);
    mouse_iter =
        tr->first_iteration + trace_data_search_iteration (tr, mouse_time);

    if (point_inside_gantt (&mouse, _t))
      selected->trace = tr;

    if (horiz_mode && nb_traces > 1 && point_inside_gantt (&mouse, 1 - _t)) {
      virt_mouse.y = get_y_mouse_sibbling (trace + (1 - _t));
      virt_mouse.x = -1;
    }
  }

  // We go through the range of iterations and we display tasks & associated
  // tiles
  if (first_it < tr->nb_iterations)
    for (int c = 0; c < tr->nb_thr; c++) {
      // We get a pointer on the first task executed by
      // CPU 'c' at first displayed iteration
      trace_task_t *first = tr->iteration [first_it].first_cpu_task [c];

      if (first != NULL)
        // We follow the list of tasks, starting from this first task
        list_for_each_entry_from (trace_task_t, t, tr->per_thr + c, first,
                                  thr_chain)
        {
          // Skip tasks which are out of the current view
          if (task_end_time (tr, t) < view_start)
            continue;

          // We stop if we encounter a task belonging to a greater iteration
          if (task_start_time (tr, t) > view_end)
            break;

          // Ok, this task should appear on the screen

          // Project the task in the Gantt chart
          SDL_Rect dst;
          dst.x = time_to_pixel (task_start_time (tr, t));
          dst.w = time_to_pixel (task_end_time (tr, t)) - dst.x + 1;

          get_y_bounds_from_thread (tr, c, &dst);

          unsigned col = trace_colors_get_index (_t, c);

          // If task is a GPU tranfer lane, modify height, y-offset and color
          if (is_lane (tr, c)) {
            dst.h = TASK_HEIGHT / 2;
            if (t->task_type == TASK_TYPE_READ)
              dst.y += TASK_HEIGHT / 2;
          }

          // Check if mouse is within the bounds of the gantt zone
          if (mouse_in_gantt_zone) {
            int done = 0;

            if (point_in_yrange (&dst, virt_mouse.y)) {
              if (point_in_xrange (&dst, virt_mouse.x)) {
                // Mouse pointer is over task t
                selected->task = t;

                if (tracking_mode)
                  get_raw_rect (t, &selected->area);

                // The task is under the mouse cursor: display it a little
                // bigger!
                magnify (&dst);

                show_tile (tr, t, col, 1);
                done = 1;
              } else if (horiz_mode) {
                show_tile (tr, t, col, 0);
                done = 1;
              }
            } else if (!horiz_mode && !tracking_mode &&
                       point_in_xrange (&dst, virt_mouse.x)) {
              show_tile (tr, t, col, 0);
              done = 1;
            }

            if (footprint_mode && !done)
              show_tile (tr, t, col, 0);

            if (backlog_mode) {
              if (!done && (t->iteration == mouse_iter) &&
                  (task_start_time (tr, t) <= mouse_time))
                show_tile (tr, t, col, 0);
            } else if (tracking_mode) {
              // If tracking mode is enabled, we highlight tasks which work on
              // tiles intersecting the working set of selected task
              if (selected->task != NULL && _t != selected->trace->num &&
                  selected->task->iteration == t->iteration &&
                  selected->task->task_id == t->task_id) {
                SDL_Rect r;

                get_raw_rect (t, &r);
                if (rects_do_intersect (&r, &selected->area)) {
                  selected->cumulated_duration += t->end_time - t->start_time;

                  show_tile (tr, t, col, 0);
                  col = TRACE_MAX_COLORS;
                }
              }
            }
          } else if (in_mosaic) {
            SDL_Rect r;

            get_raw_rect (t, &r);
            if (point_in_rect (&mouse_pick, &r)) {
              // Mouse in right window matches the footprint of current task
              if (!mosaic_tile_displayed) {
                if (easyview_mode == EASYVIEW_MODE_3D_MESHES)
                  ezv_set_cpu_color_1D (
                      ctx [_t], t->x, t->w,
                      ezv_rgba (0xFF, 0xFF, 0xFF, TILE_ALPHA));
                else
                  ezv_set_cpu_color_2D (
                      ctx [_t], t->x, t->w, t->y, t->h,
                      ezv_rgba (0xFF, 0xFF, 0xFF, TILE_ALPHA));
                mosaic_tile_displayed = 1;
              }
              // Display task a little bigger!
              magnify (&dst);
              col = TRACE_MAX_COLORS;
            }
          }

          SDL_RenderCopy (renderer, perf_fill [col], NULL, &dst);
        }
    }

  // Display mouse selection rectangle (if any)
  display_selection ();

  // Disable clipping region
  SDL_RenderSetClipRect (renderer, NULL);
}

void trace_graphics_display (void)
{
  selected_task_info_t selected = SELECTED_TASK_INFO_INITIALIZER;

  SDL_RenderClear (renderer);

  // Draw the dark grey background
  {
    SDL_Rect all = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};

    SDL_SetRenderDrawColor (renderer, backgrd_color.r, backgrd_color.g,
                            backgrd_color.b, backgrd_color.a);
    SDL_RenderFillRect (renderer, &all);
  }

  display_misc_status ();

  // Draw the text indicating CPU numbers
  display_text ();

  // Fix the loop "direction" so that the trace hovered by the
  // mouse pointer is displayed first
  if (nb_traces > 1 &&
      (point_inside_gantt (&mouse, 1) || point_inside_mosaic (&mouse, 1)))
    for (int _t = nb_traces - 1; _t != -1; _t--)
      trace_graphics_display_trace (_t, &selected);
  else
    for (int _t = 0; _t < nb_traces; _t++)
      trace_graphics_display_trace (_t, &selected);

  // Mouse
  display_mouse_selection (&selected);

  SDL_RenderPresent (renderer);

  // Render mosaic views
  ezv_render (ctx, nb_traces);
}

static void trace_graphics_relayout (unsigned w, unsigned h)
{
  WINDOW_WIDTH  = w;
  WINDOW_HEIGHT = h;

  trace_layout_recompute (0);
  trace_layout_place_buttons ();

  trace_texture_create_left_panel ();

  trace_graphics_display ();
}

static void check_comparison_consistency (void)
{
  if (nb_traces == 2) {
    if ((trace [0].mesh_file == NULL &&
         trace [nb_traces - 1].mesh_file != NULL) ||
        (trace [0].mesh_file != NULL &&
         trace [nb_traces - 1].mesh_file == NULL))
      exit_with_error (
          "Comparison between incompatible traces (w mesh vs w/o mesh).");

    if (trace [0].mesh_file != NULL &&
        trace [nb_traces - 1].mesh_file != NULL &&
        strcmp (trace [0].mesh_file, trace [1].mesh_file))
      exit_with_error ("Comparison between traces operating on different "
                       "meshes is not possible.");
    else if (trace [0].dimensions != trace [nb_traces - 1].dimensions)
      exit_with_error ("Both traces must use images of the same size.");
  }
}

void trace_graphics_init (unsigned width, unsigned height)
{
  check_comparison_consistency ();

  easyview_mode = (trace [0].mesh_file != NULL) ? EASYVIEW_MODE_3D_MESHES
                                                : EASYVIEW_MODE_2D_IMAGES;

  trace_topo_init ();

  // If no trace contains thread bindind info, disable topology view
  if (!trace [0].thr_binding && !trace [nb_traces - 1].thr_binding)
    easyview_show_topo = 0;

  if (SDL_Init (SDL_INIT_VIDEO) != 0)
    exit_with_error ("SDL_Init");

  SDL_DisplayMode dm;

  if (SDL_GetDesktopDisplayMode (0, &dm) != 0)
    exit_with_error ("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError ());

  dm.h -= 128; // to account for headers, footers, etc.

  width -= MOSAIC_SIZE;

  const unsigned min_width  = trace_layout_get_min_width ();
  const unsigned min_height = trace_layout_get_min_height ();

  WINDOW_WIDTH  = MAX (width, min_width);
  WINDOW_HEIGHT = MAX (dm.h, min_height);

  if (min_height > dm.h)
    exit_with_error ("Window height (%d) is not big enough to display so "
                     "many CPUS\n",
                     WINDOW_HEIGHT);

  WINDOW_HEIGHT = MIN (WINDOW_HEIGHT, trace_layout_get_max_height ());

  trace_layout_recompute (1);

  char wintitle [1024];

  if (nb_traces == 1)
    sprintf (wintitle, "EasyView Trace Viewer -- \"%s\"", trace [0].label);
  else
    sprintf (wintitle, "EasyView -- \"%s\" (top) VS \"%s\" (bottom)",
             trace [0].label, trace [1].label);

  window = SDL_CreateWindow (wintitle, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (window == NULL)
    exit_with_error ("SDL_CreateWindow");

  main_windowID = SDL_GetWindowID (window);

  SDL_SetWindowMinimumSize (window, min_width, min_height);

  int choosen_renderer = -1;

  unsigned drivers = SDL_GetNumRenderDrivers ();

  for (int d = 0; d < drivers; d++) {
    SDL_RendererInfo info;
    SDL_GetRenderDriverInfo (d, &info);
    // fprintf (stderr, "Available Renderer %d: [%s]\n", d, info.name);
#ifdef USE_GLAD
    if (!strcmp (info.name, "opengl"))
      choosen_renderer = d;
#endif
  }

  renderer =
      SDL_CreateRenderer (window, choosen_renderer, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL)
    exit_with_error ("SDL_CreateRenderer failed: %s", SDL_GetError ());

  SDL_RendererInfo info;
  SDL_GetRendererInfo (renderer, &info);
  // printf ("Renderer used: [%s]\n", info.name);

  max_iterations =
      MAX (trace [0].nb_iterations, trace [nb_traces - 1].nb_iterations);
  max_time = MAX (iteration_end_time (trace, trace [0].nb_iterations - 1),
                  iteration_end_time (trace + nb_traces - 1,
                                      trace [nb_traces - 1].nb_iterations - 1));

  trace_colors_init ();

  trace_texture_init ();

  trace_layout_place_buttons ();

  unsigned nbthumbs = preload_thumbnails (max_iterations);

  SDL_SetRenderDrawBlendMode (renderer, SDL_BLENDMODE_BLEND);

  _ezv_init (easyview_ezv_dir);

  if (easyview_mode == EASYVIEW_MODE_3D_MESHES) {
    mesh3d_obj_init (&mesh);
    mesh3d_obj_load (trace [0].mesh_file, &mesh);
  } else {
    img2d_obj_init (&img2d, trace [0].dimensions, trace [0].dimensions);
  }
  int x = -1, y = -1, w = 0, offset = 0;

  SDL_GetWindowPosition (window, &x, &y);
  SDL_GetWindowSize (window, &w, NULL);

  for (int c = 0; c < nb_traces; c++) {
    if (c > 0) {
      SDL_Window *win = ezv_sdl_window (ctx [c - 1]);
      SDL_GetWindowSize (win, NULL, &offset);
      offset += 30;
    }
    ctx [c] = ezv_ctx_create (
        easyview_mode == EASYVIEW_MODE_3D_MESHES ? EZV_CTX_TYPE_MESH3D
                                                 : EZV_CTX_TYPE_IMG2D,
        "Footprint", x + w, y + c * offset, MOSAIC_SIZE, MOSAIC_SIZE,
        EZV_ENABLE_PICKING | EZV_ENABLE_CLIPPING);

    if (easyview_mode == EASYVIEW_MODE_3D_MESHES)
      ezv_mesh3d_set_mesh (ctx [c], &mesh);
    else
      ezv_img2d_set_img (ctx [c], &img2d);

    // Color cell according to CPU
    ezv_use_cpu_colors (ctx [c]);

    if (nbthumbs > 0) {
      if (easyview_mode == EASYVIEW_MODE_3D_MESHES)
        ezv_use_data_colors_predefined (ctx [c], trace [c].palette);
      else
        ezv_use_data_colors_predefined (ctx [c], EZV_PALETTE_RGBA_PASSTHROUGH);

      ezv_set_data_brightness (ctx [c], (float)brightness / 255.0f);
    }
  }

  SDL_RaiseWindow (window);
}

void trace_view_set_quick_nav (int nav)
{
  quick_nav_mode = nav;

  SDL_SetTextureAlphaMod (quick_nav_tex, quick_nav_mode ? 255 : BUTTON_ALPHA);
}

void trace_graphics_display_all (void)
{
  trace_view_set_widest_view (1, max_iterations);

  trace_view_set_quick_nav (trace_data_align_mode);

  trace_graphics_display ();
}

// Control of view

static void trace_graphics_toggle_vh_mode (void)
{
  horiz_mode ^= 1;

  backlog_mode = 0;
  if (tracking_mode) {
    tracking_mode = 0;
    SDL_SetTextureAlphaMod (track_tex, BUTTON_ALPHA);
  }
  if (footprint_mode) {
    footprint_mode = 0;
    SDL_SetTextureAlphaMod (footprint_tex, BUTTON_ALPHA);
  }

  trace_graphics_display ();
}

static void trace_graphics_toggle_backlog_mode (void)
{
  backlog_mode ^= 1;
  if (backlog_mode) {
    horiz_mode     = 0;
    tracking_mode  = 0;
    footprint_mode = 0;
  }
  trace_graphics_display ();
}

static void trace_graphics_toggle_footprint_mode (void)
{
  static unsigned old_horiz, old_track, old_backlog;

  footprint_mode ^= 1;
  SDL_SetTextureAlphaMod (footprint_tex, footprint_mode ? 255 : BUTTON_ALPHA);

  if (footprint_mode) {
    old_horiz     = horiz_mode;
    old_track     = tracking_mode;
    old_backlog   = backlog_mode;
    horiz_mode    = 1;
    tracking_mode = 0;
    backlog_mode  = 0;
  } else {
    horiz_mode    = old_horiz;
    tracking_mode = old_track;
    backlog_mode  = old_backlog;
  }
  SDL_SetTextureAlphaMod (track_tex, tracking_mode ? 255 : BUTTON_ALPHA);

  trace_graphics_display ();
}

static void trace_graphics_toggle_tracking_mode (void)
{
  if (nb_traces > 1) {
    tracking_mode ^= 1;
    SDL_SetTextureAlphaMod (track_tex, tracking_mode ? 255 : BUTTON_ALPHA);

    if (tracking_mode) {
      horiz_mode   = 0;
      backlog_mode = 0;

      if (footprint_mode) {
        footprint_mode = 0;
        SDL_SetTextureAlphaMod (footprint_tex, BUTTON_ALPHA);
      }
    }

    trace_graphics_display ();
  } else
    printf ("Warning: tracking mode is only available when visualizing two "
            "traces\n");
}

static void trace_graphics_toggle_align_mode (void)
{
  if (nb_traces == 1)
    return;

  trace_data_align_mode ^= 1;

  SDL_SetTextureAlphaMod (align_tex,
                          trace_data_align_mode ? 255 : BUTTON_ALPHA);

  max_time = MAX (iteration_end_time (trace, trace [0].nb_iterations - 1),
                  iteration_end_time (trace + nb_traces - 1,
                                      trace [nb_traces - 1].nb_iterations - 1));

  if (view_end > max_time) {
    view_end      = max_time;
    view_duration = view_end - view_start;
  }

  trace_view_set_bounds (view_start, view_end);
  trace_view_set_quick_nav (0);

  trace_graphics_display ();
}

static void trace_graphics_save_screenshot (void)
{
  SDL_Rect rect;
  SDL_Surface *screen_surface = NULL;
  char filename [MAX_FILENAME];

  // Get viewport size
  SDL_RenderGetViewport (renderer, &rect);

  // Create SDL_Surface with depth of 32 bits
  screen_surface = SDL_CreateRGBSurface (0, rect.w, rect.h, 32, 0, 0, 0, 0);

  // Check if the surface is created properly
  if (screen_surface == NULL)
    exit_with_error ("Cannot create surface");

  // Display fake mouse cursor before screenshot
  rect.x = mouse.x - 3;
  rect.y = mouse.y;
  rect.w = 24;
  rect.h = 36;

  SDL_RenderCopy (renderer, mouse_tex, NULL, &rect);

  // Get data from SDL_Renderer and save them into surface
  if (SDL_RenderReadPixels (renderer, NULL, screen_surface->format->format,
                            screen_surface->pixels, screen_surface->pitch) != 0)
    exit_with_error ("Cannot read pixels from renderer");

  // append date & time to filename
  {
    time_t timer;
    struct tm *tm_info;
    timer   = time (NULL);
    tm_info = localtime (&timer);
    strftime (filename, MAX_FILENAME,
              "data/dump/screenshot-%Y_%m_%d-%H_%M_%S.png", tm_info);
  }

  // Save screenshot as PNG file
  if (IMG_SavePNG (screen_surface, filename) != 0)
    exit_with_error ("IMG_SavePNG (\"%s\") failed (%s)", filename,
                     SDL_GetError ());

  // Free memory
  SDL_FreeSurface (screen_surface);

  fprintf (stderr, "Screenshot captured into %s\n", filename);
}

void trace_graphics_process_event (SDL_Event *event)
{
  int refresh, pick;
  static int shifted = 0; // event->key.keysym.mod & KMOD_SHIFT;

  if (event->wheel.windowID != main_windowID ||
      (event->type == SDL_MOUSEWHEEL && shifted)) {
    // event is for OpenGL tiling window(s)
    ezv_process_event (ctx, nb_traces, event, &refresh, &pick);
    if (pick) {
      if (easyview_mode == EASYVIEW_MODE_3D_MESHES)
        mouse_pick.x = ezv_perform_1D_picking (ctx, nb_traces);
      else
        ezv_perform_2D_picking (ctx, nb_traces, &mouse_pick.x, &mouse_pick.y);
    }
    if (refresh | pick)
      trace_graphics_display ();
    return;
  }

  if (event->type == SDL_KEYDOWN) {
    switch (event->key.keysym.sym) {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
      shifted = 1;
      break;
    case SDLK_RIGHT:
      trace_view_shift_left ();
      break;
    case SDLK_LEFT:
      trace_view_shift_right ();
      break;
    case SDLK_MINUS:
    case SDLK_KP_MINUS:
    case SDLK_m:
      trace_view_zoom_out ();
      break;
    case SDLK_PLUS:
    case SDLK_KP_PLUS:
    case SDLK_p:
      trace_view_zoom_in ();
      break;
    case SDLK_SPACE:
      trace_view_reset_zoom ();
      break;
    case SDLK_w:
      trace_graphics_display_all ();
      break;
    case SDLK_a:
      trace_graphics_toggle_align_mode ();
      break;
    case SDLK_x:
      trace_graphics_toggle_vh_mode ();
      break;
    case SDLK_t:
      trace_graphics_toggle_tracking_mode ();
      break;
    case SDLK_f:
      trace_graphics_toggle_footprint_mode ();
      break;
    case SDLK_b:
      trace_graphics_toggle_backlog_mode ();
      break;
    case SDLK_z:
      trace_view_zoom_to_selection ();
      break;
    case SDLK_s:
      trace_graphics_save_screenshot ();
      break;
    default:
      ezv_process_event (ctx, nb_traces, event, &refresh, &pick);
      if (pick) {
        if (easyview_mode == EASYVIEW_MODE_3D_MESHES)
          mouse_pick.x = ezv_perform_1D_picking (ctx, nb_traces);
        else
          ezv_perform_2D_picking (ctx, nb_traces, &mouse_pick.x, &mouse_pick.y);
      }
      if (pick | refresh)
        trace_graphics_display ();
      break;
    }
  } else if (event->type == SDL_KEYUP) {
    if (event->key.keysym.sym == SDLK_LSHIFT ||
        event->key.keysym.sym == SDLK_RSHIFT)
      shifted = 0;
  } else if (event->type == SDL_MOUSEMOTION) {
    trace_view_mouse_moved (event->motion.x, event->motion.y);
  } else if (event->type == SDL_MOUSEBUTTONDOWN) {
    trace_view_mouse_down (event->button.x, event->button.y);
  } else if (event->type == SDL_MOUSEBUTTONUP) {
    trace_view_mouse_up (event->button.x, event->button.y);
  } else if (event->type == SDL_MOUSEWHEEL) {
    trace_view_scroll (event->wheel.x);
  } else if (event->type == SDL_WINDOWEVENT) {
    switch (event->window.event) {
    case SDL_WINDOWEVENT_RESIZED:
      trace_graphics_relayout (event->window.data1, event->window.data2);
      break;
    }
  }
}

void trace_graphics_clean ()
{
  trace_colors_finalize ();

  trace_texture_clean ();

  if (renderer != NULL)
    SDL_DestroyRenderer (renderer);
  else
    return;

  if (window != NULL)
    SDL_DestroyWindow (window);
  else
    return;
}