/**
 * @file game.c
 * @copyright University of Bordeaux. All rights reserved, 2024.
 **/

#include "game.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game_aux.h"
#include "game_ext.h"
#include "game_private.h"
#include "game_struct.h"
#include "queue.h"

/* ************************************************************************** */

game game_new(shape* shapes, direction* orientations)
{
  return game_new_ext(DEFAULT_SIZE, DEFAULT_SIZE, shapes, orientations, false);
}

/* ************************************************************************** */

game game_new_empty(void)
{
  return game_new_empty_ext(DEFAULT_SIZE, DEFAULT_SIZE, false);
}

/* ************************************************************************** */

game game_copy(cgame g)
{
  game gg = game_new_empty_ext(g->nb_rows, g->nb_cols, g->wrapping);
  memcpy(gg->squares, g->squares, g->nb_rows * g->nb_cols * sizeof(square));
  return gg;
}

/* ************************************************************************** */

bool game_equal(cgame g1, cgame g2, bool ignore_orientation)
{
  assert(g1 && g2);

  if (g1->nb_rows != g2->nb_rows) return false;
  if (g1->nb_cols != g2->nb_cols) return false;

  for (uint i = 0; i < g1->nb_rows; i++)
    for (uint j = 0; j < g1->nb_cols; j++) {
      shape s1 = SHAPE(g1, i, j);
      shape s2 = SHAPE(g2, i, j);
      if (s1 != s2) return false;
      if (ignore_orientation) continue;
      direction o1 = ORIENTATION(g1, i, j);
      direction o2 = ORIENTATION(g2, i, j);
      if (o1 != o2) return false;
    }

  if (g1->wrapping != g2->wrapping) return false;

  return true;
}

/* ************************************************************************** */

void game_delete(game g)
{
  if (!g) return;
  free(g->squares);
  queue_free_full(g->undo_stack, free);
  queue_free_full(g->redo_stack, free);
  free(g);
}

/* ************************************************************************** */

void game_set_piece_shape(game g, uint i, uint j, shape s)
{
  assert(g);
  assert(i < g->nb_rows);
  assert(j < g->nb_cols);
  assert(s >= 0 && s < NB_SHAPES);
  SHAPE(g, i, j) = s;
}

/* ************************************************************************** */

void game_set_piece_orientation(game g, uint i, uint j, direction o)
{
  assert(g);
  assert(i < g->nb_rows);
  assert(j < g->nb_cols);
  assert(o >= 0 && o < NB_DIRS);
  ORIENTATION(g, i, j) = o;
}

/* ************************************************************************** */

shape game_get_piece_shape(cgame g, uint i, uint j)
{
  assert(g);
  assert(i < g->nb_rows);
  assert(j < g->nb_cols);
  return SHAPE(g, i, j);
}

/* ************************************************************************** */

direction game_get_piece_orientation(cgame g, uint i, uint j)
{
  assert(g);
  assert(i < g->nb_rows);
  assert(j < g->nb_cols);
  return ORIENTATION(g, i, j);
}

/* ************************************************************************** */

#define MODULO(x, n) (((x) % (n) + (n)) % (n))

/* Warning: In C, the modulo operator '%' can return negative results. For
 * instance: '-5 % 4 = -1'. */

void game_play_move(game g, uint i, uint j, int nb_quarter_turns)
{
  assert(g);
  assert(i < g->nb_rows);
  assert(j < g->nb_cols);

  direction old = ORIENTATION(g, i, j);
  direction new = MODULO(old + nb_quarter_turns, NB_DIRS);
  ORIENTATION(g, i, j) = new;

  // save history
  _stack_clear(g->redo_stack);
  move m = {i, j, old, new};
  _stack_push_move(g->undo_stack, m);
}

/* ************************************************************************** */

void game_reset_orientation(game g)
{
  assert(g);

  for (uint i = 0; i < g->nb_rows; i++)
    for (uint j = 0; j < g->nb_cols; j++)
      game_set_piece_orientation(g, i, j, NORTH);

  // reset history
  _stack_clear(g->undo_stack);
  _stack_clear(g->redo_stack);
}

/* ************************************************************************** */

void game_shuffle_orientation(game g)
{
  assert(g);

  for (uint i = 0; i < g->nb_rows; i++)
    for (uint j = 0; j < g->nb_cols; j++) {
      direction o = rand() % NB_DIRS;
      game_set_piece_orientation(g, i, j, o);
    }

  // reset history
  _stack_clear(g->undo_stack);
  _stack_clear(g->redo_stack);
}

/* ************************************************************************** */

bool game_won(cgame g)
{
  assert(g);
  return game_is_well_paired(g) && game_is_connected(g);
}

/* ************************************************************************** */
