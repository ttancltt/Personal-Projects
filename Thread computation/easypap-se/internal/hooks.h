#ifndef HOOKS_IS_DEF
#define HOOKS_IS_DEF

#include <omp.h>

#include "ezp_helpers.h"

typedef unsigned (*int_func_t) (unsigned);
typedef void (*draw_func_t) (char *);
typedef int (*tile_func_t) (int, int, int, int);
typedef int (*patch_func_t) (int, int);
typedef void (*debug_1d_t) (int);
typedef void (*debug_2d_t) (int, int);

extern draw_func_t the_config;
extern void_func_t the_init;
extern void_func_t the_first_touch;
extern draw_func_t the_draw;
extern void_func_t the_finalize;
extern int_func_t the_compute;
extern void_func_t the_tile_check;
extern debug_1d_t the_1d_debug;
extern debug_2d_t the_2d_debug;
extern debug_1d_t the_1d_overlay;
extern debug_2d_t the_2d_overlay;
extern void_func_t the_send_data;

void *bind_it (const char *kernel, const char *s, const char *variant, int print_error);
void *hooks_find_symbol (char *symbol);
void hooks_establish_bindings (int silent);

int hooks_refresh_img (void);


#endif
