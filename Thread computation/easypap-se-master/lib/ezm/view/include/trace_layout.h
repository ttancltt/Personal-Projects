#ifndef TRACE_LAYOUT_IS_DEF
#define TRACE_LAYOUT_IS_DEF

#include "ezv_sdl_gl.h"

void trace_layout_place_buttons (void);
void trace_layout_recompute (int at_init);
unsigned trace_layout_get_min_height (void);
unsigned trace_layout_get_max_height (void);
unsigned trace_layout_get_min_width (void);

typedef struct
{
  SDL_Rect gantt;
  SDL_Texture *label_tex;
  unsigned label_width, label_height;
  SDL_Texture **task_ids_tex;
  unsigned *task_ids_tex_width;
} display_info_t;

extern display_info_t trace_display_info [];

extern SDL_Rect gantts_bounding_box;

extern SDL_Point mouse;
extern int mouse_in_gantt_zone;

static inline int point_in_xrange (const SDL_Rect *r, int x)
{
  return x >= r->x && x < (r->x + r->w);
}

static inline int point_in_yrange (const SDL_Rect *r, int y)
{
  return y >= r->y && y < (r->y + r->h);
}

static inline int point_in_rect (const SDL_Point *p, const SDL_Rect *r)
{
  return point_in_xrange (r, p->x) && point_in_yrange (r, p->y);
}

static inline int point_inside_gantt (const SDL_Point *p, unsigned trace_num)
{
  return point_in_rect (p, &trace_display_info [trace_num].gantt);
}

static inline int point_inside_gantts (const SDL_Point *p)
{
  return point_in_rect (p, &gantts_bounding_box);
}

#define Y_MARGIN 1
#define cpu_row_height(taskh) ((taskh) + 2 * Y_MARGIN)
#define SIDE_MARGIN 16
#define FONT_HEIGHT 20

#define TOPO_COL_WIDTH 12
#define TOPO_COL_SPACING 6

extern int TASK_HEIGHT;
extern int WINDOW_HEIGHT;
extern int WINDOW_WIDTH;

extern SDL_Rect align_rect, quick_nav_rect, track_rect, footprint_rect;

extern int BUBBLE_WIDTH, BUBBLE_HEIGHT, REDUCED_BUBBLE_WIDTH,
    REDUCED_BUBBLE_HEIGHT;

#endif
