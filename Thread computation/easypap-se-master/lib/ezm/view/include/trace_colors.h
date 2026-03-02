#ifndef TRACE_COLORS_H
#define TRACE_COLORS_H

#include <stdint.h>

extern unsigned TRACE_MAX_COLORS;

void trace_colors_init (void);
void trace_colors_finalize (void);

uint32_t trace_colors_get_color (int pu);
unsigned trace_colors_get_index (int trace_id, int pu);

#endif