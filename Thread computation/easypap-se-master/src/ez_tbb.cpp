#include <tbb/tbb.h>

#include "debug.h"
#include "error.h"
#include "ez_tbb.hpp"
#include "ezp_bind.h"
#include "ezv_topo.h"
#include "pthread_barrier.h"

tbb::task_arena *ezp_pinned_arena = nullptr;

void ez_tbb_init (void)
{
  unsigned max_threads = ezv_topo_get_nb_pus ();

  if (getenv ("OMP_PLACES") != NULL || getenv ("OMP_PROC_BIND") != NULL) {
    fprintf (stderr,
             "Warning: OMP_PLACES or OMP_PROC_BIND is defined, "
             "Undefined TBB behavior is possible.\n"
             "         Please use EZ_PLACES or EZ_PROC_BIND instead.\n");

    ezp_bind_set_nbthreads (max_threads);

    ezp_pinned_arena = new tbb::task_arena (max_threads);

    return;
  }

  ezp_bind_init (max_threads, EZP_BIND_MODE_SET);

  unsigned nb_threads = ezp_bind_get_nbthreads ();
  ezp_pinned_arena = new tbb::task_arena (nb_threads);

  {
    pthread_barrier_t barrier;
    pthread_barrier_init (&barrier, NULL, nb_threads);

    ezp_pinned_arena->execute ([&] {
      tbb::parallel_for (
          tbb::blocked_range<int> (0, nb_threads, 1),
          [&] (const tbb::blocked_range<int> &r) {
            for (int i = r.begin (); i != r.end (); ++i) {
              // Force every thread to enter the arena and be pinned
              auto id = tbb::this_task_arena::current_thread_index ();
              ezp_bind_self (id);
              PRINT_DEBUG ('t', "TBB thread pinned %u\n", id);
              pthread_barrier_wait (&barrier);
            }
          },
          tbb::static_partitioner{});
    });
  }
}

void ez_tbb_finalize (void)
{
  if (ezp_pinned_arena) {
    delete ezp_pinned_arena;
    ezp_pinned_arena = nullptr;
  }
}