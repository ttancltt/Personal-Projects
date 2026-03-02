#include "ezv_event.h"
#include "error.h"
#include "ezv_ctx.h"
#include "ezv_virtual.h"

static int mouse[2]         = {-1, -1};
static ezv_ctx_t active_ctx = NULL;
static int mouse_click_x    = -1;
static int mouse_click_y    = -1;
static int button_down      = 0;

static void mouse_enter (ezv_ctx_t ctx[], unsigned nb_ctx, SDL_Event *event)
{
  for (int c = 0; c < nb_ctx; c++)
    if (event->window.windowID == ctx[c]->windowID) {
      active_ctx = ctx[c];
      return;
    }

  exit_with_error ("We should not be here: no active context found");
}

static void mouse_leave (ezv_ctx_t ctx[], unsigned nb_ctx, SDL_Event *event)
{
  mouse[0]   = -1;
  mouse[1]   = -1;
  active_ctx = NULL;
}

static void mouse_focus (ezv_ctx_t ctx[], unsigned nb_ctx, SDL_Event *event)
{
  mouse[0] = event->motion.x;
  mouse[1] = event->motion.y;
}

ezv_ctx_t ezv_get_active_ctx (void)
{
  return active_ctx;
}

int ezv_ctx_is_in_focus (ezv_ctx_t ctx)
{
  return active_ctx == ctx;
}

static inline int active_ctx_enables_picking (void)
{
  return active_ctx && active_ctx->picking_enabled;
}

int ezv_perform_1D_picking (ezv_ctx_t ctx[], unsigned nb_ctx)
{
  if (active_ctx_enables_picking ())
    return ezv_do_1D_picking (active_ctx, mouse[0], mouse[1]);

  return -1;
}

void ezv_perform_2D_picking (ezv_ctx_t ctx[], unsigned nb_ctx, int *x, int *y)
{
  if (active_ctx_enables_picking ()) {
    ezv_do_2D_picking (active_ctx, mouse[0], mouse[1], x, y);
    return;
  }

  *x = -1;
  *y = -1;
}

// pick is set to 1 if picking should be re-done, 0 otherwise
// refresh is set to 1 if rendering should be performed
int ezv_process_event (ezv_ctx_t ctx[], unsigned nb_ctx, SDL_Event *event,
                       int *refresh, int *pick)
{
  int do_pick    = 0;
  int do_refresh = 0;

  switch (event->type) {
  case SDL_QUIT: // normally handled by easypap/easyview
    return 0;
  case SDL_KEYDOWN:
    switch (event->key.keysym.sym) {
    case SDLK_ESCAPE:
    case SDLK_q:
      return 0;
    case SDLK_r:
      // Reset view
      ezv_reset_view (ctx, nb_ctx);
      do_pick    = active_ctx_enables_picking ();
      do_refresh = 1;
      break;
    case SDLK_MINUS:
    case SDLK_KP_MINUS:
    case SDLK_m:
    case SDLK_l:
      if (ezv_zoom (ctx, nb_ctx, event->key.keysym.mod & KMOD_SHIFT, 0)) {
        do_pick    = active_ctx_enables_picking ();
        do_refresh = 1;
      }
      break;
    case SDLK_PLUS:
    case SDLK_KP_PLUS:
    case SDLK_EQUALS:
    case SDLK_p:
    case SDLK_o:
      if (ezv_zoom (ctx, nb_ctx, event->key.keysym.mod & KMOD_SHIFT, 1)) {
        do_pick    = active_ctx_enables_picking ();
        do_refresh = 1;
      }
      break;
    case SDLK_c:
      // Toggle clipping
      ezv_toggle_clipping (ctx, nb_ctx);
      do_pick    = active_ctx_enables_picking ();
      do_refresh = 1;
      break;
    }
    break;
  case SDL_MOUSEMOTION:
    mouse_focus (ctx, nb_ctx, event);
    do_pick = active_ctx_enables_picking ();
    if (button_down) {
      if (ezv_motion (ctx, nb_ctx, (event->motion.x - mouse_click_x),
                      (event->motion.y - mouse_click_y), 0)) {
        do_refresh = 1;
      }
      mouse_click_x = event->motion.x;
      mouse_click_y = event->motion.y;
    }
    break;
  case SDL_MOUSEWHEEL: {
    if (ezv_motion (ctx, nb_ctx, -event->wheel.x, event->wheel.y, 1)) {
      do_pick    = active_ctx_enables_picking ();
      do_refresh = 1;
    }
  } break;
  case SDL_MOUSEBUTTONDOWN:
    mouse_click_x = event->button.x;
    mouse_click_y = event->button.y;
    button_down   = 1;
    break;
  case SDL_MOUSEBUTTONUP: {
    button_down = 0;
    break;
  }
  case SDL_WINDOWEVENT:
    switch (event->window.event) {
    case SDL_WINDOWEVENT_ENTER:
      mouse_enter (ctx, nb_ctx, event);
      do_pick = active_ctx_enables_picking ();
      break;
    case SDL_WINDOWEVENT_LEAVE:
      mouse_leave (ctx, nb_ctx, event);
      do_pick = 1;
      break;
    default:;
    }
  }
  if (refresh)
    *refresh = do_refresh;
  if (pick)
    *pick = do_pick;

  return 1;
}

static int get_event (SDL_Event *event, int blocking)
{
  return blocking ? SDL_WaitEvent (event) : SDL_PollEvent (event);
}

int ezv_get_event (SDL_Event *event, int blocking)
{
#ifdef __APPLE__
  return get_event (event, blocking);
#else
  int r;
  static int prefetched = 0;
  static SDL_Event pr_event; // prefetched event

  if (prefetched) {
    *event     = pr_event;
    prefetched = 0;
    return 1;
  }

  r = get_event (event, blocking);

  if (r != 1)
    return r;

  // check if successive, similar events can be dropped
  if (event->type == SDL_MOUSEMOTION) {

    do {
      int ret_code = get_event (&pr_event, 0);
      if (ret_code == 1) {
        if (pr_event.type == SDL_MOUSEMOTION) {
          *event     = pr_event;
          prefetched = 0;
        } else {
          prefetched = 1;
        }
      } else
        return 1;
    } while (prefetched == 0);
  }

  return 1;
#endif
}
