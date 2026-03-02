#ifndef EZV_MON_H
#define EZV_MON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mon_obj.h"

struct ezv_ctx_s;
typedef struct ezv_ctx_s *ezv_ctx_t;

void ezv_mon_set_moninfo (ezv_ctx_t ctx, mon_obj_t *moninfo);

// For private use only
void ezv_mon_precalc_view_parameters (mon_obj_t *mon, unsigned *width,
                                      unsigned *height);
unsigned ezv_mon_get_palette_size (mon_obj_t *mon);

void ezv_mon_folding_has_changed (ezv_ctx_t ctx);

#ifdef __cplusplus
}
#endif

#endif
