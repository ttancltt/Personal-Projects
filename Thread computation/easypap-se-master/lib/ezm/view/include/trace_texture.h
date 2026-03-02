#ifndef TRACE_TEXTURES_IS_DEF
#define TRACE_TEXTURES_IS_DEF

#include "ezv_sdl_gl.h"

#include <SDL_ttf.h>

void trace_texture_init (void);
void trace_texture_clean (void);
TTF_Font *trace_texture_get_default_font (void);

void trace_texture_create_left_panel (void);

#define BUTTON_ALPHA 60

extern SDL_Color silver_color, backgrd_color;

extern SDL_Texture *black_square;
extern SDL_Texture *dark_square;
extern SDL_Texture *white_square;
extern SDL_Texture *stat_frame_tex;
extern SDL_Texture *stat_caption_tex;
extern SDL_Texture *stat_background;
extern SDL_Texture **perf_fill;
extern SDL_Texture *text_texture;
extern SDL_Texture *vertical_line;
extern SDL_Texture *horizontal_line;
extern SDL_Texture *horizontal_bis;
extern SDL_Texture *bulle_tex;
extern SDL_Texture *reduced_bulle_tex;
extern SDL_Texture *us_tex;
extern SDL_Texture *sigma_tex;
extern SDL_Texture *tab_left;
extern SDL_Texture *tab_right;
extern SDL_Texture *tab_high;
extern SDL_Texture *tab_low;
extern SDL_Texture *align_tex;
extern SDL_Texture *quick_nav_tex;
extern SDL_Texture *track_tex;
extern SDL_Texture *footprint_tex;
extern SDL_Texture *digit_tex [10];
extern SDL_Texture *mouse_tex;

extern unsigned digit_tex_width [10];
extern unsigned digit_tex_height;

extern char easyview_ezv_dir [1024];

#endif
