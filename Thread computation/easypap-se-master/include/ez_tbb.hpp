#ifndef EZ_TBB_HPP
#define EZ_TBB_HPP

#include <tbb/tbb.h>

void ez_tbb_init (void);
void ez_tbb_finalize (void);

extern class tbb::task_arena *ezp_pinned_arena;

#endif
