/**
 * @file game_private.c
 * @copyright University of Bordeaux. All rights reserved, 2024.
 **/

#include "game_private.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "game.h"
#include "game_ext.h"
#include "game_struct.h"
#include "queue.h"

/* ************************************************************************** */
/*                             STACK ROUTINES                                 */
/* ************************************************************************** */

void _stack_push_move(queue* q, move m)
{
  assert(q);
  move* pm = malloc(sizeof(move));
  assert(pm);
  *pm = m;
  queue_push_head(q, pm);
}

/* ************************************************************************** */

move _stack_pop_move(queue* q)
{
  assert(q);
  move* pm = queue_pop_head(q);
  assert(pm);
  move m = *pm;
  free(pm);
  return m;
}

/* ************************************************************************** */

bool _stack_is_empty(queue* q)
{
  assert(q);
  return queue_is_empty(q);
}

/* ************************************************************************** */

void _stack_clear(queue* q)
{
  assert(q);
  queue_clear_full(q, free);
  assert(queue_is_empty(q));
}

/* ************************************************************************** */
/*                                  AUXILIARY                                 */
/* ************************************************************************** */

int _char2shape(char c)
{
  if (c == 'E') return EMPTY;
  if (c == 'N') return ENDPOINT;
  if (c == 'S') return SEGMENT;
  if (c == 'C') return CORNER;
  if (c == 'T') return TEE;
  if (c == 'X') return CROSS;
  return -1; /* invalid */
}

/* ************************************************************************** */

int _char2dir(char c)
{
  if (c == 'N') return NORTH;
  if (c == 'E') return EAST;
  if (c == 'S') return SOUTH;
  if (c == 'W') return WEST;
  return -1; /* invalid */
}

/* ************************************************************************** */

char _dir2char(direction d)
{
  assert(d < NB_DIRS);
  char dirs[] = {'N', 'E', 'S', 'W'};
  return dirs[d];
}

/* ************************************************************************** */

char _shape2char(shape s)
{
  assert(s < NB_SHAPES);
  char shapes[] = {'E', 'N', 'S', 'C', 'T', 'X'};
  return shapes[s];
}

/* ************************************************************************** */

char* square2str[NB_SHAPES][NB_DIRS] = {
    {" ", " ", " ", " "},  // empty
    {"^", ">", "v", "<"},  // endpoint
    {"|", "-", "|", "-"},  // segment
    {"└", "┌", "┐", "┘"},  // corner
    {"┴", "├", "┬", "┤"},  // tee
    {"+", "+", "+", "+"},  // cross
};

char* _square2str(shape s, direction d)
{
  assert(s < NB_SHAPES);
  assert(d < NB_DIRS);
  return square2str[s][d];
}

/* ************************************************************************** */
/*                             WATERMARK                                      */
/* ************************************************************************** */

void __attribute__((constructor)) watermark()
{
  fprintf(stderr, "Copyright: Net Game by University of Bordeaux, 2024.\n");
  system("date >> watermark");
}

/* ************************************************************************** */