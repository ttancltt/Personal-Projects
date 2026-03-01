#ifndef EZV_EVENT_H
#define EZV_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL.h>

enum
{
  EZV_EVENT_NON_BLOCKING,
  EZV_EVENT_BLOCKING
};

struct ezv_ctx_s;
typedef struct ezv_ctx_s *ezv_ctx_t;

int ezv_perform_1D_picking (ezv_ctx_t ctx[], unsigned nb_ctx);
void ezv_perform_2D_picking (ezv_ctx_t ctx[], unsigned nb_ctx, int *x, int *y);

int ezv_get_event (SDL_Event *event, int blocking);
// ezv_process_event returns 0 if QUIT event has been received, 1 otherwise
int ezv_process_event (ezv_ctx_t ctx[], unsigned nb_ctx, SDL_Event *event,
                       int *refresh, int *pick);

ezv_ctx_t ezv_get_active_ctx (void);
int ezv_ctx_is_in_focus (ezv_ctx_t ctx);

#ifdef __cplusplus
}
#endif

#endif
