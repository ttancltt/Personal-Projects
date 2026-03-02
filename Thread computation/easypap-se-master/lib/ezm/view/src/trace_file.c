#include <assert.h>
#include <fcntl.h>
#include <fut.h>
#include <fxt-tools.h>
#include <fxt.h>
#include <getopt.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "error.h"
#include "ezm_pack.h"
#include "ezm_trace_codes.h"
#include "trace_data.h"
#include "trace_file.h"

static long *last_start_times = NULL;

void trace_file_load (char *file)
{
  fxt_t fxt;
  fxt_blockev_t evs;
  struct fxt_ev_native ev;
  int ret;

  if (!(fxt = fxt_open (file)))
    exit_with_error ("Cannot open \"%s\" trace file (%s)", file,
                     strerror (errno));

  trace_data_init (&trace [nb_traces], nb_traces);

  evs = fxt_blockev_enter (fxt);

  while (FXT_EV_OK ==
         (ret = fxt_next_ev (evs, FXT_EV_TYPE_NATIVE, (struct fxt_ev *)&ev))) {

    unsigned cpu = ev.param [1];

    switch (ev.code) {
    case TRACE_BEGIN_ITER:
      trace_data_start_iteration (&trace [nb_traces], ev.time / 1000);
      break;

    case TRACE_END_ITER:
      trace_data_end_iteration (&trace [nb_traces], ev.time / 1000);
      break;

    case TRACE_NB_THREADS: {
      unsigned nc = ev.param [0];
      unsigned ng = ev.param [1];
      if (ng & 1) // number of GPU lanes must be even
        exit_with_error ("Bad trace header: #GPU (= %d) expected to be even",
                         ng);

      last_start_times = malloc ((nc + ng) * sizeof (long));
      for (int c = 0; c < nc + ng; c++)
        last_start_times [c] = 0;
      trace_data_set_nb_threads (&trace [nb_traces], nc, ng);
      break;
    }

    case TRACE_BEGIN_TILE:
      last_start_times [cpu] = ev.param [0];
      break;

    case TRACE_END_TILE:
      trace_data_add_task (
          &trace [nb_traces], last_start_times [cpu], ev.param [0],
          ev.param [2], ev.param [3], ev.param [4], ev.param [5], cpu,
          INT_EXTRACT_LOW (ev.param [6]), INT_EXTRACT_HIGH (ev.param [6]));
      break;

    case TRACE_TILE:
      trace_data_add_task (
          &trace [nb_traces], ev.param [0], ev.time / 1000,
          INT_EXTRACT_LOW (ev.param [2]), INT_EXTRACT_HIGH (ev.param [2]),
          INT_EXTRACT_LOW (ev.param [3]), INT_EXTRACT_HIGH (ev.param [3]), cpu,
          INT_EXTRACT_LOW (ev.param [4]), INT_EXTRACT_HIGH (ev.param [4]));
      break;

    case TRACE_TILE_EXT:
      trace_data_add_task (
          &trace [nb_traces], ev.param [0], ev.param [2],
          INT_EXTRACT_LOW (ev.param [3]), INT_EXTRACT_HIGH (ev.param [3]),
          INT_EXTRACT_LOW (ev.param [4]), INT_EXTRACT_HIGH (ev.param [4]), cpu,
          INT_EXTRACT_LOW (ev.param [5]), INT_EXTRACT_HIGH (ev.param [5]));
      break;

    case TRACE_TILE_MIN:
      trace_data_add_task (
          &trace [nb_traces], ev.param [0], ev.time / 1000,
          INT_EXTRACT_LOW (ev.param [2]), INT_EXTRACT_HIGH (ev.param [2]),
          INT_EXTRACT_LOW (ev.param [3]), INT_EXTRACT_HIGH (ev.param [3]), cpu,
          TASK_TYPE_COMPUTE, 0);
      break;

    case TRACE_PATCH:
      trace_data_add_task (
          &trace [nb_traces], ev.param [0], ev.time / 1000,
          INT_EXTRACT_LOW (ev.param [2]), 0, INT_EXTRACT_HIGH (ev.param [2]), 0,
          cpu, INT_EXTRACT_LOW (ev.param [3]), INT_EXTRACT_HIGH (ev.param [3]));
      break;

    case TRACE_PATCH_EXT:
      trace_data_add_task (
          &trace [nb_traces], ev.param [0], ev.param [2],
          INT_EXTRACT_LOW (ev.param [3]), 0, INT_EXTRACT_HIGH (ev.param [3]), 0,
          cpu, INT_EXTRACT_LOW (ev.param [4]), INT_EXTRACT_HIGH (ev.param [4]));
      break;

    case TRACE_PATCH_MIN:
      trace_data_add_task (&trace [nb_traces], ev.param [0], ev.time / 1000,
                           INT_EXTRACT_LOW (ev.param [2]), 0,
                           INT_EXTRACT_HIGH (ev.param [2]), 0, cpu,
                           TASK_TYPE_COMPUTE, 0);
      break;

    case TRACE_DIM:
      trace_data_set_dim (&trace [nb_traces], ev.param [0]);
      break;

    case TRACE_FIRST_ITER:
      trace_data_set_first_iteration (&trace [nb_traces], ev.param [0]);
      break;

    case TRACE_LABEL:
      trace_data_set_label (&trace [nb_traces], (char *)ev.raw);
      break;

    case TRACE_TASKID_COUNT:
      trace_data_alloc_task_ids (&trace [nb_traces], ev.param [0]);
      break;

    case TRACE_TASKID:
      trace_data_add_taskid (&trace [nb_traces], (char *)ev.raw);
      break;

    case TRACE_MESHFILE:
      trace_data_set_meshfile (&trace [nb_traces], (char *)ev.raw);
      break;

    case TRACE_PALETTE:
      trace_data_set_palette (&trace [nb_traces], ev.param [0]);
      break;

    case TRACE_TOPOFILE:
      trace_data_load_topo (&trace [nb_traces], (char *)ev.raw);
      break;

    case TRACE_BINDING_HEADER:
      trace_data_alloc_bindings (&trace [nb_traces]);
      break;

    case TRACE_THR_BINDING:
      trace_data_set_binding (&trace [nb_traces], ev.param [0], ev.param [1],
                              ev.param [2]);
      break;

    default:
      break;
    }
  }

  if (ret != FXT_EV_EOT)
    fprintf (stderr, "Warning: FXT stopping on code %i\n", ret);

  // On some systems, fxt_close is not implemented... Curious.
  // fxt_close (fxt);

  free (last_start_times);
  last_start_times = NULL;

  // Set a default label
  if (trace [nb_traces].label == NULL) {
    char *name    = basename (file);
    char *lastdot = strrchr (name, '.');
    if (lastdot != NULL)
      *lastdot = '\0';

    trace_data_set_label (&trace [nb_traces], name);
  }

  trace_data_no_more_data (&trace [nb_traces]);

  printf ("Trace #%d \"%s\": %d iterations with %d threads "
          "%s"
          "(%s)\n",
          nb_traces, trace [nb_traces].label, trace [nb_traces].nb_iterations,
          trace [nb_traces].nb_thr,
          trace [nb_traces].thr_binding ? "- with binding info " : "", file);

  nb_traces++;
}
