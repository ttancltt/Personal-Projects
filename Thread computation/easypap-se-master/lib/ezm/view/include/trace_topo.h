#ifndef TRACE_TOPO_IS_DEF
#define TRACE_TOPO_IS_DEF

// Calls to trace_topo_load should be done before any call to trace_topo_init
int trace_topo_load (const char *filename);

void trace_topo_init (void);
void trace_topo_finalize (void);

extern unsigned easyview_show_topo;

#endif
