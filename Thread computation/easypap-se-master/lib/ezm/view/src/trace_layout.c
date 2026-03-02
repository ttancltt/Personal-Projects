#include "trace_layout.h"
#include "trace_graphics.h"
#include "trace_data.h"
#include "trace_topo.h"
#include "min_max.h"
#include "error.h"

display_info_t trace_display_info [MAX_TRACES];

SDL_Rect gantts_bounding_box;

#define WINDOW_MIN_WIDTH 1024
// no WINDOW_MIN_HEIGHT: needs to be automatically computed

#define MIN_TASK_HEIGHT 6
#define MAX_TASK_HEIGHT 44

#define TOP_MARGIN 48
#define TEXT_MARGIN 64
#define RIGHT_MARGIN SIDE_MARGIN
#define BOTTOM_MARGIN (FONT_HEIGHT + 4)
#define INTERTRACE_MARGIN (2 * FONT_HEIGHT + 2)

int TASK_HEIGHT   = MAX_TASK_HEIGHT;
int WINDOW_HEIGHT = -1;
int WINDOW_WIDTH  = -1;

static unsigned GANTT_HEIGHT;
static unsigned LEFT_MARGIN = SIDE_MARGIN + TEXT_MARGIN;

SDL_Rect align_rect, quick_nav_rect, track_rect, footprint_rect;

SDL_Point mouse         = {-1, -1};
int mouse_in_gantt_zone = 0;

static unsigned layout_get_height (unsigned task_height)
{
  unsigned need_left, gantt_h;

  if (nb_traces == 1) {
    gantt_h    = trace [0].nb_rows * cpu_row_height (task_height);
    need_left  = TOP_MARGIN + gantt_h + BOTTOM_MARGIN;
  } else {
    gantt_h =
        (trace [0].nb_rows + trace [1].nb_rows) * cpu_row_height (task_height) +
        INTERTRACE_MARGIN;
    need_left = TOP_MARGIN + gantt_h + BOTTOM_MARGIN;
  }
  
  return need_left;
}

static inline unsigned gantt_width (void)
{
  return WINDOW_WIDTH - (LEFT_MARGIN + RIGHT_MARGIN);
}

unsigned trace_layout_get_min_width (void)
{
  return WINDOW_MIN_WIDTH;
}

unsigned trace_layout_get_min_height (void)
{
  return layout_get_height (MIN_TASK_HEIGHT);
}

unsigned trace_layout_get_max_height (void)
{
  return layout_get_height (MAX_TASK_HEIGHT);
}

void trace_layout_place_buttons (void)
{
  quick_nav_rect.x = trace_display_info [0].gantt.x +
                     trace_display_info [0].gantt.w - quick_nav_rect.w;
  quick_nav_rect.y = 2;

  align_rect.x = quick_nav_rect.x - Y_MARGIN - align_rect.w;
  align_rect.y = 2;

  track_rect.x = align_rect.x - Y_MARGIN - track_rect.w;
  track_rect.y = 2;

  footprint_rect.x = track_rect.x - Y_MARGIN - footprint_rect.w;
  footprint_rect.y = 2;
}

void trace_layout_recompute (int at_init)
{
  unsigned need_left;

  if (at_init && easyview_show_topo)
    LEFT_MARGIN += ezv_topo_get_maxdepth () * (TOPO_COL_WIDTH + TOPO_COL_SPACING);

  if (nb_traces == 1) {
    // See how much space we have for GANTT chart
    unsigned space = WINDOW_HEIGHT - TOP_MARGIN - BOTTOM_MARGIN;
    space /= trace [0].nb_rows;
    TASK_HEIGHT = space - 2 * Y_MARGIN;
    if (TASK_HEIGHT < MIN_TASK_HEIGHT)
      exit_with_error ("Window height (%d) is not big enough to display so "
                       "many rows (%d)\n",
                       WINDOW_HEIGHT, trace [0].nb_rows);

    if (TASK_HEIGHT > MAX_TASK_HEIGHT)
      TASK_HEIGHT = MAX_TASK_HEIGHT;

    if (at_init)
      WINDOW_HEIGHT = layout_get_height (TASK_HEIGHT);

    GANTT_HEIGHT = trace [0].nb_rows * cpu_row_height (TASK_HEIGHT);

    trace_display_info [0].gantt.x = LEFT_MARGIN;
    trace_display_info [0].gantt.y = TOP_MARGIN;
    trace_display_info [0].gantt.w = gantt_width ();
    trace_display_info [0].gantt.h = GANTT_HEIGHT;
  } else {
    unsigned padding = 0;

    // See how much space we have for GANTT chart
    unsigned space =
        WINDOW_HEIGHT - TOP_MARGIN - BOTTOM_MARGIN - INTERTRACE_MARGIN;
    space /= (trace [0].nb_rows + trace [1].nb_rows);
    TASK_HEIGHT = space - 2 * Y_MARGIN;
    if (TASK_HEIGHT < MIN_TASK_HEIGHT)
      exit_with_error ("Window height (%d) is not big enough to display so "
                       "many rows (%d + %d)\n",
                       WINDOW_HEIGHT, trace [0].nb_rows, trace [1].nb_rows);

    if (TASK_HEIGHT > MAX_TASK_HEIGHT)
      TASK_HEIGHT = MAX_TASK_HEIGHT;

    if (at_init)
      WINDOW_HEIGHT = layout_get_height (TASK_HEIGHT);

    // First try with max task height
    GANTT_HEIGHT =
        (trace [0].nb_rows + trace [1].nb_rows) * cpu_row_height (TASK_HEIGHT) +
        INTERTRACE_MARGIN;
    need_left = TOP_MARGIN + GANTT_HEIGHT + BOTTOM_MARGIN;

    if (WINDOW_HEIGHT > need_left)
      padding = WINDOW_HEIGHT - need_left;

    trace_display_info [0].gantt.x = LEFT_MARGIN;
    trace_display_info [0].gantt.y = TOP_MARGIN + padding / 2;
    trace_display_info [0].gantt.w = gantt_width ();
    trace_display_info [0].gantt.h =
        trace [0].nb_rows * cpu_row_height (TASK_HEIGHT);

    trace_display_info [1].gantt.x = LEFT_MARGIN;
    trace_display_info [1].gantt.y = TOP_MARGIN + INTERTRACE_MARGIN +
                                     trace_display_info [0].gantt.h +
                                     padding / 2;
    trace_display_info [1].gantt.w = gantt_width ();
    trace_display_info [1].gantt.h =
        trace [1].nb_rows * cpu_row_height (TASK_HEIGHT);
  }

  gantts_bounding_box.x = trace_display_info [0].gantt.x;
  gantts_bounding_box.y = trace_display_info [0].gantt.y;
  gantts_bounding_box.w = trace_display_info [0].gantt.w;
  gantts_bounding_box.h = (trace_display_info [nb_traces - 1].gantt.y +
                           trace_display_info [nb_traces - 1].gantt.h) -
                          gantts_bounding_box.y;

  // printf ("GANTT_HEIGHT = %d, gantts_bounding_box = (%d,%d,%d,%d)\n",
  //         GANTT_HEIGHT, gantts_bounding_box.x, gantts_bounding_box.y,
  //         gantts_bounding_box.w, gantts_bounding_box.h);
}
