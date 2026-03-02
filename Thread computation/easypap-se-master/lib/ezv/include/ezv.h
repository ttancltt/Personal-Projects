#ifndef EZV_H
#define EZV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ezv_hud.h"
#include "ezv_palette.h"
#include "ezv_rgba.h"

#include "ezv_img2d.h"
#include "ezv_mesh3d.h"
#include "ezv_mon.h"

typedef enum
{
  EZV_CTX_TYPE_MESH3D,
  EZV_CTX_TYPE_IMG2D,
  EZV_CTX_TYPE_MONITOR
} ezv_ctx_type_t;

#define EZV_ENABLE_PICKING (1U << 0)
#define EZV_ENABLE_VSYNC (1U << 1)
#define EZV_ENABLE_HUD (1U << 2)
#define EZV_ENABLE_CLIPPING (1U << 3)
#define EZV_HIDE_WINDOW (1U << 4)
#define EZV_ENABLE_CELLMASK (1U << 5)
#define EZV_VERBOSE (1U << 6)

struct ezv_ctx_s;
typedef struct ezv_ctx_s *ezv_ctx_t;

#ifdef EZP_INSTALLED
#define ezv_init() _ezv_init (NULL)
#else
#define ezv_init() _ezv_init (".")
#endif

void _ezv_init (const char *prefix);
void ezv_finalize (void);

unsigned ezv_get_max_display_height (void);

ezv_ctx_t ezv_ctx_create (ezv_ctx_type_t ctx_type, const char *win_title, int x,
                          int y, int w, int h, int flags);
void ezv_ctx_delete (ezv_ctx_t ctx[], unsigned nb_ctx);

ezv_ctx_type_t ezv_ctx_type (ezv_ctx_t ctx);
char *ezv_ctx_typestr (ezv_ctx_t ctx);
unsigned ezv_ctx_width (ezv_ctx_t ctx);
unsigned ezv_ctx_height (ezv_ctx_t ctx);

void ezv_ctx_raise (ezv_ctx_t ctx);
void ezv_ctx_show (ezv_ctx_t ctx);
void ezv_ctx_hide (ezv_ctx_t ctx);

void ezv_switch_to_context (ezv_ctx_t ctx);
void ezv_ctx_destroy (ezv_ctx_t ctx);
void ezv_toggle_clipping (ezv_ctx_t ctx[], unsigned nb_ctx);

void ezv_use_cpu_colors (ezv_ctx_t ctx);
void ezv_reset_cpu_colors (ezv_ctx_t ctx);

void ezv_set_cpu_color_1D (ezv_ctx_t ctx, unsigned offset, unsigned size,
                           uint32_t color);
void ezv_set_cpu_color_2D (ezv_ctx_t ctx, unsigned x, unsigned width,
                           unsigned y, unsigned height, uint32_t color);

void ezv_use_data_colors (ezv_ctx_t ctx, float *data, unsigned size);
void ezv_use_data_colors_predefined (ezv_ctx_t ctx, ezv_palette_name_t palette);

void ezv_set_data_colors (ezv_ctx_t ctx, void *values);

// Virtual methods
void ezv_render (ezv_ctx_t ctx[], unsigned nb_ctx);
void ezv_reset_view (ezv_ctx_t ctx[], unsigned nb_ctx);
void ezv_get_shareable_buffer_ids (ezv_ctx_t ctx, int buffer_ids[]);
void ezv_set_data_brightness (ezv_ctx_t ctx, float brightness);
void ezv_take_screenshot (ezv_ctx_t ctx, const char *filename);
void ezv_cellmask_reset (ezv_ctx_t ctx);
void ezv_cellmask_set1D_hidden (ezv_ctx_t ctx, unsigned first_cell,
                                unsigned nb_cells, uint8_t *mask);
void ezv_cellmask_set1D_alpha (ezv_ctx_t ctx, uint8_t alpha,
                               unsigned first_cell, unsigned nb_cells,
                               uint8_t *mask);

// Helpers
void ezv_helper_ctx_next_coord (ezv_ctx_t ctx[], unsigned ctx_no,
                                unsigned *xwin, unsigned *ywin);

#ifdef __cplusplus
}
#endif

#endif
