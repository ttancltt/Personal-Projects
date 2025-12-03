/**
 * @file game_struct.h
 * @brief Private Game Structure.
 * @copyright University of Bordeaux. All rights reserved, 2024.
 **/

#ifndef __GAME_STRUCT_H__
#define __GAME_STRUCT_H__

#include <stdbool.h>

#include "game.h"
#include "game_ext.h"
#include "queue.h"

/* ************************************************************************** */
/*                             DATA TYPES                                     */
/* ************************************************************************** */

typedef struct square_s {
  shape s;     /**< piece shape */
  direction o; /**< piece orientation */
} square;

/**
 * @brief Game structure.
 * @details This is an opaque data type.
 */
struct game_s {
  char offset[100];  /**< offset to prevent direct access to struct fields */
  uint nb_rows;      /**< number of rows in the game */
  uint nb_cols;      /**< number of columns in the game */
  square* squares;   /**< the grid of squares using row-major storage */
  bool wrapping;     /**< the wrapping option */
  queue* undo_stack; /**< stack to undo moves */
  queue* redo_stack; /**< stack to redo moves */
};

/* ************************************************************************** */
/*                                MACRO                                       */
/* ************************************************************************** */

#define INDEX(g, i, j) ((i) * (g->nb_cols) + (j))
#define SQUARE(g, i, j) ((g)->squares[(INDEX(g, i, j))])
#define SHAPE(g, i, j) (SQUARE(g, i, j).s)
#define ORIENTATION(g, i, j) (SQUARE(g, i, j).o)

#endif  // __GAME_STRUCT_H__
