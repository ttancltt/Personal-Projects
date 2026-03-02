#ifndef TRACE_GRAPHICS_IS_DEF
#define TRACE_GRAPHICS_IS_DEF


#include "trace_data.h"
#include "ezv_event.h"

void trace_graphics_init (unsigned w, unsigned h);
void trace_graphics_process_event (SDL_Event *event);
void trace_graphics_display (void);
void trace_graphics_display_all (void);
void trace_graphics_clean ();

extern SDL_Renderer *renderer;

extern unsigned char brightness;

#endif
