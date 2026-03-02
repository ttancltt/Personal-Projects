#include "sandPile_common.h"

#include <omp.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Asynchronous Kernel
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

// Use ALIAS macros to create wrapper functions for common functions
SANDPILE_ALIAS(asandPile, refresh_img);
SANDPILE_ALIAS(asandPile, draw_4partout);
SANDPILE_ALIAS(asandPile, draw_DIM);
SANDPILE_ALIAS(asandPile, draw_alea);
SANDPILE_ALIAS(asandPile, draw_big);
SANDPILE_ALIAS(asandPile, draw_spirals);
SANDPILE_DRAW_ALIAS(asandPile);

void asandPile_config (char *param)
{
  sandPile_config (param);
}

void asandPile_debug (int x, int y)
{
  sandPile_debug (x, y);
}

void asandPile_init (void)
{
  in = out = 0;
  if (TABLE == NULL) {
    const unsigned size = DIM * DIM * sizeof (TYPE);

    PRINT_DEBUG ('u', "Memory footprint = 1 x %d bytes\n", size);

    TABLE = ezp_alloc (size);
  }
}

void asandPile_finalize (void)
{
  const unsigned size = DIM * DIM * sizeof (TYPE);

  ezp_free (TABLE, size);
}

///////////////////////////// Version séquentielle simple (seq)
// Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0

int asandPile_do_tile_default (int x, int y, int width, int height)
{
  int change = 0;

  for (int i = y; i < y + height; i++)
    for (int j = x; j < x + width; j++)
      if (atable (i, j) >= 4) {
        atable (i, j - 1) += atable (i, j) / 4;
        atable (i, j + 1) += atable (i, j) / 4;
        atable (i - 1, j) += atable (i, j) / 4;
        atable (i + 1, j) += atable (i, j) / 4;
        atable (i, j) %= 4;
        change = 1;
      }
  return change;
}

unsigned asandPile_compute_seq (unsigned nb_iter)
{
  int change = 0;
  for (unsigned it = 1; it <= nb_iter; it++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    change = do_tile (1, 1, DIM - 2, DIM - 2);

    if (change == 0)
      return it;
  }
  return 0;
}

unsigned asandPile_compute_tiled (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it++) {
    int change = 0;

    for (int y = 0; y < DIM; y += TILE_H)
      for (int x = 0; x < DIM; x += TILE_W)
        change |= do_tile (x + (x == 0), y + (y == 0),
                           TILE_W - ((x + TILE_W == DIM) + (x == 0)),
                           TILE_H - ((y + TILE_H == DIM) + (y == 0)));
    if (change == 0)
      return it;
  }

  return 0;
}