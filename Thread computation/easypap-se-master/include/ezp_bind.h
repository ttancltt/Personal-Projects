#ifndef EZP_BIND_H
#define EZP_BIND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hwloc.h>

typedef enum {
    EZP_BIND_MODE_SET,
    EZP_BIND_MODE_GET
} ezp_bind_mode_t;

void ezp_bind_init (unsigned max, ezp_bind_mode_t mode);
void ezp_bind_finalize (void);

void ezp_bind_set_nbthreads (unsigned nbthreads);
unsigned ezp_bind_get_nbthreads (void);

void ezp_bind_get_logical_cpusets (hwloc_cpuset_t **sets, unsigned *count);
void ezp_bind_self (unsigned id);

#ifdef __cplusplus
}
#endif

#endif
