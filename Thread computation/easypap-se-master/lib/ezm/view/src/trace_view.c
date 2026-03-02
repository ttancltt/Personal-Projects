#include "trace_view.h"
#include "min_max.h"
#include "trace_data.h"
#include "trace_graphics.h"
#include "trace_layout.h"

// How much percentage of view_duration should we shift ?
#define SHIFT_FACTOR 0.02
#define MIN_DURATION 100.0


trace_ctrl_t trace_ctrl [MAX_TRACES];

long view_start           = 0;
long view_end             = 0;
long view_duration        = 0;
long selection_start_time = 0;
long selection_duration   = 0;
long max_time             = -1;
int max_iterations        = -1;
long mouse_orig_time      = 0;
int mouse_down            = 0;

int quick_nav_mode = 0;
int horiz_mode     = 0;
int tracking_mode  = 0;
int footprint_mode = 0;
int backlog_mode   = 0;

void trace_view_set_bounds (long start, long end)
{
  view_start    = start;
  view_end      = end;
  view_duration = view_end - view_start;

  for (int t = 0; t < nb_traces; t++) {
    trace_ctrl [t].first_displayed_iter =
        trace_data_search_next_iteration (&trace [t], view_start) + 1;
    trace_ctrl [t].last_displayed_iter =
        trace_data_search_prev_iteration (&trace [t], view_end) + 1;
  }
}

static void update_bounds (void)
{
  long start, end;
  int li [2];

  if (trace_ctrl [0].first_displayed_iter > trace [0].nb_iterations)
    start = iteration_start_time (
        trace + nb_traces - 1,
        trace_ctrl [nb_traces - 1].first_displayed_iter - 1);
  else if (trace_ctrl [nb_traces - 1].first_displayed_iter >
           trace [nb_traces - 1].nb_iterations)
    start =
        iteration_start_time (trace, trace_ctrl [0].first_displayed_iter - 1);
  else
    start = MIN (
        iteration_start_time (trace, trace_ctrl [0].first_displayed_iter - 1),
        iteration_start_time (trace + nb_traces - 1,
                              trace_ctrl [nb_traces - 1].first_displayed_iter -
                                  1));

  if (trace_ctrl [0].last_displayed_iter > trace [0].nb_iterations)
    li [0] = trace [0].nb_iterations - 1;
  else
    li [0] = trace_ctrl [0].last_displayed_iter - 1;

  if (trace_ctrl [nb_traces - 1].last_displayed_iter >
      trace [nb_traces - 1].nb_iterations)
    li [1] = trace [nb_traces - 1].nb_iterations - 1;
  else
    li [1] = trace_ctrl [nb_traces - 1].last_displayed_iter - 1;

  end = MAX (iteration_end_time (trace, li [0]),
             iteration_end_time (trace + nb_traces - 1, li [1]));

  trace_view_set_bounds (start, end);
}

void trace_view_set_widest_view (int first, int last)
{
  trace_ctrl [0].first_displayed_iter             = first;
  trace_ctrl [nb_traces - 1].first_displayed_iter = first;

  trace_ctrl [0].last_displayed_iter             = last;
  trace_ctrl [nb_traces - 1].last_displayed_iter = last;

  update_bounds ();
}

static void set_iteration_range (int trace_num)
{
  if (nb_traces > 1) {
    int other = 1 - trace_num;

    trace_ctrl [other].first_displayed_iter =
        trace_ctrl [trace_num].first_displayed_iter;
    trace_ctrl [other].last_displayed_iter =
        trace_ctrl [trace_num].last_displayed_iter;
  }

  view_start = iteration_start_time (
      trace + trace_num, trace_ctrl [trace_num].first_displayed_iter - 1);
  view_end = iteration_end_time (
      trace + trace_num, trace_ctrl [trace_num].last_displayed_iter - 1);

  view_duration = view_end - view_start;
}

void trace_view_scroll (int delta)
{
  long start = view_start + view_duration * SHIFT_FACTOR * delta;
  long end   = start + view_duration;

  if (start < 0) {
    start = 0;
    end   = view_duration;
  }

  if (end > max_time) {
    end   = max_time;
    start = end - view_duration;
  }

  if (start != view_start || end != view_end) {
    trace_view_set_bounds (start, end);
    trace_view_set_quick_nav (0);

    trace_graphics_display ();
  }
}

void trace_view_shift_left (void)
{
  if (quick_nav_mode) {
    int longest =
        (trace [0].nb_iterations >= trace [nb_traces - 1].nb_iterations)
            ? 0
            : nb_traces - 1;

    if (trace_ctrl [longest].last_displayed_iter < max_iterations) {
      trace_ctrl [longest].first_displayed_iter++;
      trace_ctrl [longest].last_displayed_iter++;

      set_iteration_range (longest);

      trace_graphics_display ();
    }
  } else {
    trace_view_scroll (1);
  }
}

void trace_view_shift_right (void)
{
  if (quick_nav_mode) {
    int longest =
        (trace [0].nb_iterations >= trace [nb_traces - 1].nb_iterations)
            ? 0
            : nb_traces - 1;

    if (trace_ctrl [longest].first_displayed_iter > 1) {
      trace_ctrl [longest].first_displayed_iter--;
      trace_ctrl [longest].last_displayed_iter--;

      set_iteration_range (longest);

      trace_graphics_display ();
    }
  } else {
    trace_view_scroll (-1);
  }
}

void trace_view_zoom_in (void)
{
  if (quick_nav_mode && (trace_ctrl [0].last_displayed_iter >
                         trace_ctrl [0].first_displayed_iter)) {

    int longest =
        (trace [0].nb_iterations >= trace [nb_traces - 1].nb_iterations)
            ? 0
            : nb_traces - 1;

    trace_ctrl [longest].last_displayed_iter--;

    set_iteration_range (longest);

    trace_graphics_display ();
  } else if (view_end > view_start + MIN_DURATION) {
    long start = view_start + view_duration * SHIFT_FACTOR;
    long end   = view_end - view_duration * SHIFT_FACTOR;

    if (end < start + MIN_DURATION)
      end = start + MIN_DURATION;

    trace_view_set_bounds (start, end);
    trace_view_set_quick_nav (0);

    trace_graphics_display ();
  }
}

void trace_view_zoom_out (void)
{
  if (quick_nav_mode) {
    int longest =
        (trace [0].nb_iterations >= trace [nb_traces - 1].nb_iterations)
            ? 0
            : nb_traces - 1;

    if (trace_ctrl [longest].last_displayed_iter < max_iterations) {
      trace_ctrl [longest].last_displayed_iter++;

      set_iteration_range (longest);

      trace_graphics_display ();
    } else if (trace_ctrl [longest].first_displayed_iter > 1) {
      trace_ctrl [longest].first_displayed_iter--;

      set_iteration_range (longest);

      trace_graphics_display ();
    }
  } else {
    long start = view_start - view_duration * SHIFT_FACTOR;
    long end   = view_end + view_duration * SHIFT_FACTOR;

    if (start < 0)
      start = 0;

    if (end > max_time)
      end = max_time;

    if (start != view_start || end != view_end) {
      trace_view_set_bounds (start, end);
      trace_view_set_quick_nav (0);

      trace_graphics_display ();
    }
  }
}

void trace_view_zoom_to_selection (void)
{
  if (selection_duration > 0) {
    long start = selection_start_time;
    long end   = selection_start_time + selection_duration;

    if (selection_duration < MIN_DURATION) {
      long delta = (MIN_DURATION - selection_duration) / 2;
      if (start < delta)
        start = 0;
      else
        start -= delta;
      end = start + MIN_DURATION;
      if (end > max_time) {
        end   = max_time;
        start = end - MIN_DURATION;
      }
    }

    trace_view_set_bounds (start, end);
    trace_view_set_quick_nav (0);

    selection_duration = 0;

    trace_graphics_display ();
  }
}

void trace_view_mouse_moved (int x, int y)
{
  mouse.x = x;
  mouse.y = y;

  if (point_inside_gantts (&mouse))
    mouse_in_gantt_zone = 1;
  else
    mouse_in_gantt_zone = 0;

  if (mouse_down) {
    // Check if mouse in out-of-range on the x-axis
    if (point_in_yrange (&gantts_bounding_box, y)) {
      if (x < trace_display_info [0].gantt.x) {
        x = trace_display_info [0].gantt.x;
        trace_view_scroll (-1);
      } else if (x > trace_display_info [0].gantt.x +
                         trace_display_info [0].gantt.w - 1) {
        x = trace_display_info [0].gantt.x + trace_display_info [0].gantt.w - 1;
        trace_view_scroll (1);
      }
    }

    long new_pos = pixel_to_time (x);

    if (new_pos < mouse_orig_time) {
      selection_start_time = new_pos;
      selection_duration   = mouse_orig_time - new_pos;
    } else {
      selection_start_time = mouse_orig_time;
      selection_duration   = new_pos - mouse_orig_time;
    }
  }

  trace_graphics_display ();
}

void trace_view_mouse_down (int x, int y)
{
  mouse.x = x;
  mouse.y = y;

  if (point_inside_gantts (&mouse)) {

    mouse_orig_time    = pixel_to_time (x);
    selection_duration = 0;

    mouse_down = 1;

    trace_graphics_display ();
  }
}

void trace_view_mouse_up (int x, int y)
{
  mouse_down = 0;
}

void trace_view_set (int first, int last)
{
  int last_disp_it, first_disp_it;

  // Check parameters and make sure iteration range is correct
  if (last < 1)
    last_disp_it = 1;
  else if (last > max_iterations)
    last_disp_it = max_iterations;
  else
    last_disp_it = last;

  if (first < 1)
    first_disp_it = 1;
  else if (first > last_disp_it)
    first_disp_it = last_disp_it;
  else
    first_disp_it = first;

  trace_view_set_quick_nav (trace_data_align_mode);

  trace_view_set_widest_view (first_disp_it, last_disp_it);

  trace_graphics_display ();
}

void trace_view_reset_zoom (void)
{
  if (trace_data_align_mode) {

    if (!quick_nav_mode) {
      int first, last;

      if (trace_ctrl [0].first_displayed_iter >
          trace_ctrl [nb_traces - 1].last_displayed_iter)
        first = trace_ctrl [0].first_displayed_iter;
      else if (trace_ctrl [nb_traces - 1].first_displayed_iter >
               trace_ctrl [0].last_displayed_iter)
        first = trace_ctrl [nb_traces - 1].first_displayed_iter;
      else
        first = MIN (trace_ctrl [0].first_displayed_iter,
                     trace_ctrl [nb_traces - 1].first_displayed_iter);

      last = MAX (trace_ctrl [0].last_displayed_iter,
                  trace_ctrl [nb_traces - 1].last_displayed_iter);

      trace_view_set_widest_view (first, last);

      trace_view_set_quick_nav (1);

      trace_graphics_display ();
    } else {
      trace_view_set_quick_nav (0);

      trace_graphics_display ();
    }
  }
}
