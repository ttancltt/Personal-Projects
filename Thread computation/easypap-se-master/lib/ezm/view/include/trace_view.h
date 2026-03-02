#ifndef TRACE_VIEW_IS_DEF
#define TRACE_VIEW_IS_DEF

#include "trace_layout.h"

typedef struct
{
  int first_displayed_iter, last_displayed_iter;
} trace_ctrl_t;

extern trace_ctrl_t trace_ctrl [];
extern long view_start, view_end, view_duration;
extern long selection_start_time, selection_duration;
extern long max_time;
extern int max_iterations;
extern long mouse_orig_time;
extern int mouse_down;

extern int quick_nav_mode, horiz_mode, tracking_mode, footprint_mode, backlog_mode;

void trace_view_set (int start_iteration, int end_iteration);
void trace_view_set_quick_nav (int nav);
void trace_view_set_bounds (long start, long end);
void trace_view_set_widest_view (int first, int last);
void trace_view_scroll (int delta);
void trace_view_shift_left (void);
void trace_view_shift_right (void);
void trace_view_zoom_in (void);
void trace_view_zoom_out (void);
void trace_view_zoom_to_selection (void);
void trace_view_mouse_moved (int x, int y);
void trace_view_mouse_down (int x, int y);
void trace_view_mouse_up (int x, int y);
void trace_view_reset_zoom (void);

static inline int time_to_pixel (long time)
{
  return gantts_bounding_box.x + time * gantts_bounding_box.w / view_duration -
         view_start * gantts_bounding_box.w / view_duration;
}

static inline long pixel_to_time (int x)
{
  return view_start + (x - gantts_bounding_box.x) * view_duration / gantts_bounding_box.w;
}


#endif
