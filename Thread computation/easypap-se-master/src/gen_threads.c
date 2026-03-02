
#include "easypap.h"
#include "pthread_distrib.h"
#include "ezp_bind.h"

#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>

static unsigned nb_threads = -1;
static unsigned iterations = -1;
static pthread_barrier_t barrier;
static pthread_distrib_t distrib;
static trail_func_t trail_func         = NULL;
static gen_threads_schedule_t schedule = GEN_THREADS_SCHED_BLOCK;
static unsigned chunk_size             = 1;

static unsigned generic_thread_creation (unsigned nb_iter,
                                         void *(*thread_f) (void *))
{
  iterations = nb_iter;

  pthread_t pid [nb_threads - 1];

  for (int i = 0; i < nb_threads - 1; i++)
    ez_pthread_create (&pid [i], NULL, thread_f, (void *)(intptr_t)(i + 1));

  thread_f (0);

  for (int i = 0; i < nb_threads - 1; i++)
    ez_pthread_join (pid [i], NULL);

  return 0;
}

static void end_of_iteration (unsigned me)
{
#ifdef __APPLE__
  pthread_barrier_single (&barrier, trail_func);
#else
  pthread_barrier_wait (&barrier);
  if (me == 0 && trail_func)
    trail_func ();
  pthread_barrier_wait (&barrier);
#endif
}

// Block

static void *thread_2D_block (void *arg)
{
  unsigned me        = (unsigned)(intptr_t)arg;
  unsigned total     = NB_TILES_X * NB_TILES_Y;
  unsigned base      = total / nb_threads;
  unsigned remaining = total % nb_threads;
  unsigned first     = me * base + MIN (remaining, me);
  unsigned count     = base + (me < remaining ? 1 : 0);

  for (unsigned it = 1; it <= iterations; it++) {

    for (unsigned tile = first; tile < first + count; tile++) {
      unsigned y = (tile / NB_TILES_X) * TILE_H;
      unsigned x = (tile % NB_TILES_X) * TILE_W;
      do_tile (x, y, TILE_W, TILE_H, me);
    }

    end_of_iteration (me);
  }

  return NULL;
}

static void *thread_3D_block (void *arg)
{

  unsigned me        = (unsigned)(intptr_t)arg;
  unsigned base      = NB_PATCHES / nb_threads;
  unsigned remaining = NB_PATCHES % nb_threads;
  unsigned first     = me * base + MIN (remaining, me);
  unsigned count     = base + (me < remaining ? 1 : 0);

  for (unsigned it = 1; it <= iterations; it++) {

    for (unsigned p = first; p < first + count; p++)
      do_patch (p, me);

    end_of_iteration (me);
  }

  return NULL;
}

// Cyclic

static void *thread_2D_cyclic (void *arg)
{
  unsigned me = (unsigned)(intptr_t)arg;

  for (unsigned it = 1; it <= iterations; it++) {

    for (unsigned first = me * chunk_size; first < NB_TILES_X * NB_TILES_Y;
         first += nb_threads * chunk_size)
      for (unsigned tile = first;
           tile < MIN (first + chunk_size, NB_TILES_X * NB_TILES_Y); tile++) {
        unsigned y = (tile / NB_TILES_X) * TILE_H;
        unsigned x = (tile % NB_TILES_X) * TILE_W;
        do_tile (x, y, TILE_W, TILE_H, me);
      }

    end_of_iteration (me);
  }

  return NULL;
}

static void *thread_3D_cyclic (void *arg)
{
  unsigned me = (unsigned)(intptr_t)arg;

  for (unsigned it = 1; it <= iterations; it++) {

    for (unsigned firstp = me * chunk_size; firstp < NB_PATCHES;
         firstp += nb_threads * chunk_size)
      for (unsigned p = firstp; p < MIN (firstp + chunk_size, NB_PATCHES); p++)
        do_patch (p, me);

    end_of_iteration (me);
  }

  return NULL;
}

// Dynamic

static void *thread_2D_dyn (void *arg)
{
  unsigned me = (unsigned)(intptr_t)arg;

  for (unsigned it = 1; it <= iterations; it++) {

    for (;;) {
      int n = pthread_distrib_get (&distrib);
      if (n == -1)
        break;
      unsigned first = n * chunk_size;
      for (unsigned tile = first;
           tile < MIN (first + chunk_size, NB_TILES_X * NB_TILES_Y); tile++) {
        unsigned y = (tile / NB_TILES_X) * TILE_H;
        unsigned x = (tile % NB_TILES_X) * TILE_W;

        do_tile (x, y, TILE_W, TILE_H, me);
      }
    }
  }

  return NULL;
}

static void *thread_3D_dyn (void *arg)
{
  unsigned me = (unsigned)(intptr_t)arg;

  for (unsigned it = 1; it <= iterations; it++) {

    for (;;) {
      int n = pthread_distrib_get (&distrib);
      if (n == -1)
        break;
      unsigned first = n * chunk_size;
      for (unsigned p = first; p < MIN (first + chunk_size, NB_PATCHES); p++)
        do_patch (p, me);
    }
  }

  return NULL;
}

static void set_schedule (gen_threads_schedule_t sched)
{
  if (sched != GEN_THREADS_SCHED_RUNTIME) {
    schedule = sched;
    return;
  }

  char *str = getenv ("OMP_SCHEDULE");
  if (str != NULL) {
    if (strncmp (str, "static", 6) == 0) {
      char *remaining = str + 6;
      if (*remaining == '\0') {
        schedule = GEN_THREADS_SCHED_BLOCK;
      } else if (*remaining == ',') {
        chunk_size = (unsigned)atoi (remaining + 1);
        if (chunk_size == 0) {
          fprintf (stderr, "Warning: OMP_SCHEDULE has invalid chunk size.\n"
                           "Falling back to chunk size = 1\n");
          chunk_size = 1;
        }
        schedule = GEN_THREADS_SCHED_CYCLIC;
      } else {
        fprintf (stderr,
                 "Warning: OMP_SCHEDULE='%s' not supported.\n"
                 "Falling back to GEN_THREADS_SCHED_BLOCK\n",
                 str);
        schedule = GEN_THREADS_SCHED_BLOCK;
      }
    } else if (strncmp (str, "dynamic", 7) == 0) {
      char *remaining = str + 7;
      if (*remaining == '\0') {
        chunk_size = 1;
      } else if (*remaining == ',') {
        chunk_size = (unsigned)atoi (remaining + 1);
        if (chunk_size == 0) {
          fprintf (stderr, "Warning: OMP_SCHEDULE has invalid chunk size.\n"
                           "Falling back to chunk size = 1\n");
          chunk_size = 1;
        }
      } else {
        fprintf (stderr,
                 "Warning: OMP_SCHEDULE='%s' not supported.\n"
                 "Falling back to GEN_THREADS_SCHED_DYNAMIC\n",
                 str);
        chunk_size = 1;
      }
      schedule = GEN_THREADS_SCHED_DYN;
    } else {
      fprintf (stderr,
               "Warning: OMP_SCHEDULE='%s' not supported.\n"
               "Falling back to GEN_THREADS_SCHED_BLOCK\n",
               str);
      schedule = GEN_THREADS_SCHED_BLOCK;
    }
  }
}

void gen_threads_config_2D (gen_threads_schedule_t sched, trail_func_t trail)
{
  ez_pthread_init ();

  nb_threads = ezp_bind_get_nbthreads ();
  trail_func = trail;

  pthread_barrier_init (&barrier, NULL, nb_threads);

  set_schedule (sched);

  if (schedule == GEN_THREADS_SCHED_CYCLIC ||
      schedule == GEN_THREADS_SCHED_DYN) {
    unsigned nb_elements =
        (chunk_size > 1)
            ? (NB_TILES_X * NB_TILES_Y + chunk_size - 1) / chunk_size
            : NB_TILES_X * NB_TILES_Y;
    pthread_distrib_init (&distrib, nb_threads, nb_elements, trail_func);
  }
}

void gen_threads_config_3D (gen_threads_schedule_t sched, trail_func_t trail)
{
  ez_pthread_init ();

  nb_threads = ezp_bind_get_nbthreads ();
  trail_func = trail;

  pthread_barrier_init (&barrier, NULL, nb_threads);

  set_schedule (sched);

  if (schedule == GEN_THREADS_SCHED_CYCLIC ||
      schedule == GEN_THREADS_SCHED_DYN) {
    unsigned nb_elements = (chunk_size > 1)
                               ? (NB_PATCHES + chunk_size - 1) / chunk_size
                               : NB_PATCHES;
    pthread_distrib_init (&distrib, nb_threads, nb_elements, trail_func);
  }
}

unsigned gen_threads_compute_2D (unsigned nb_iter)
{
  switch (schedule) {
  case GEN_THREADS_SCHED_BLOCK:
    return generic_thread_creation (nb_iter, thread_2D_block);
  case GEN_THREADS_SCHED_CYCLIC:
    return generic_thread_creation (nb_iter, thread_2D_cyclic);
  case GEN_THREADS_SCHED_DYN:
    return generic_thread_creation (nb_iter, thread_2D_dyn);
  default:
    exit_with_error ("gen_threads_compute_2D: invalid schedule");
  }
}

unsigned gen_threads_compute_3D (unsigned nb_iter)
{
  switch (schedule) {
  case GEN_THREADS_SCHED_BLOCK:
    return generic_thread_creation (nb_iter, thread_3D_block);
  case GEN_THREADS_SCHED_CYCLIC:
    return generic_thread_creation (nb_iter, thread_3D_cyclic);
  case GEN_THREADS_SCHED_DYN:
    return generic_thread_creation (nb_iter, thread_3D_dyn);
  default:
    exit_with_error ("gen_threads_compute_3D: invalid schedule");
  }
}

void gen_threads_finalize (void)
{
  ez_pthread_finalize ();
}
