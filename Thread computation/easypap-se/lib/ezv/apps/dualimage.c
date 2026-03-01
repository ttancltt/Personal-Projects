
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "error.h"
#include "ezv.h"
#include "ezv_event.h"

// settings
const unsigned int SCR_WIDTH  = 1024;
const unsigned int SCR_HEIGHT = 768;

#define MAX_CTX 2

static char *filename = NULL;

static unsigned *image  = NULL;
static unsigned *luminance = NULL;

static img2d_obj_t img;
static ezv_ctx_t ctx[MAX_CTX] = {NULL, NULL};
static unsigned nb_ctx        = 2;
static int pos_hud            = -1;
static int val_hud            = -1;

static void do_pick (void)
{
  int x, y;

  ezv_perform_2D_picking (ctx, nb_ctx, &x, &y);

  ezv_hud_set (ctx[0], pos_hud, "xy: (%d, %d)", x, y);

  if (x != -1 && y != -1) {
    uint32_t v = image[y * img.width + x];
    ezv_hud_set (ctx[0], val_hud, "RGBA: %02X %02X %02X %02X", ezv_c2r (v),
                 ezv_c2g (v), ezv_c2b (v), ezv_c2a (v));
  } else
    ezv_hud_set (ctx[0], val_hud, NULL);
}

static int process_events (void)
{
  SDL_Event event;
  int pick;
  int ret;

  ezv_get_event (&event, EZV_EVENT_BLOCKING);

  ret = ezv_process_event (ctx, nb_ctx, &event, NULL, &pick);
  if (pick)
    do_pick ();

  return ret;
}

void update_lumin (void)
{
  int i, j;
  unsigned index = 0;

  for (j = 0; j < img.height; j++) {
    for (i = 0; i < img.width; i++) {
      float l = (float)(0.299f * ezv_c2r(image[index]) + 0.587f * ezv_c2g(image[index]) + 0.114f * ezv_c2b(image[index]));
      luminance[index] = ezv_rgb (l, l, l);
      index++;
    }
  }

  ezv_set_data_colors (ctx[1], luminance);
}

int main (int argc, char *argv[])
{
  ezv_init ();

  if (argc > 1)
    filename = argv[1];
  else
    filename = "../../../data/img/shibuya.png";

  img2d_obj_init_from_file (&img, filename);

  size_t s = img2d_obj_size (&img);
  image    = mmap (NULL, s, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
                   -1, 0);
  if (image == NULL)
    exit_with_error ("Cannot allocate main image: mmap failed");

  luminance = calloc (img.width * img.height, sizeof (unsigned));
  if (luminance == NULL)
    exit_with_error ("Cannot allocate luminance image: mmap failed");

  img2d_obj_load (&img, filename, image);

  printf ("Image: width=%d, height=%d, channels=%d (%lu bytes)\n", img.width,
          img.height, img.channels, s);

  // Create SDL windows and initialize OpenGL context
  ctx[0] = ezv_ctx_create (EZV_CTX_TYPE_IMG2D, "Image", 0,
                           SDL_WINDOWPOS_UNDEFINED, SCR_WIDTH, SCR_HEIGHT,
                           EZV_ENABLE_HUD | EZV_ENABLE_PICKING);

  ctx[1] = ezv_ctx_create (EZV_CTX_TYPE_IMG2D, "B&W", SCR_WIDTH,
                           SDL_WINDOWPOS_UNDEFINED, SCR_WIDTH * .75, SCR_HEIGHT * .75, 0);
  // Huds
  pos_hud = ezv_hud_alloc (ctx[0]);
  ezv_hud_on (ctx[0], pos_hud);
  val_hud = ezv_hud_alloc (ctx[0]);
  ezv_hud_on (ctx[0], val_hud);

  // Attach img
  ezv_img2d_set_img (ctx[0], &img);
  ezv_img2d_set_img (ctx[1], &img);

  ezv_use_data_colors_predefined (ctx[0], EZV_PALETTE_RGBA_PASSTHROUGH);
  ezv_use_data_colors_predefined (ctx[1], EZV_PALETTE_RGBA_PASSTHROUGH);

  ezv_set_data_colors (ctx[0], image);
  update_lumin ();

  // render loop
  int cont = 1;
  while (cont) {
    cont = process_events ();
    if (cont)
      ezv_render (ctx, nb_ctx);
  }

  ezv_ctx_delete (ctx, nb_ctx);
  ezv_finalize ();

  munmap (image, s);
  free (luminance);
  
  return 0;
}
