/**
 * @file game_aux.c
 * @copyright University of Bordeaux. All rights reserved, 2024.
 **/

#include "game_aux.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "game.h"
#include "game_ext.h"
#include "game_private.h"
#include "game_struct.h"

/* ************************************************************************** */

#define DN NORTH
#define DE EAST
#define DS SOUTH
#define DW WEST

#define SE EMPTY
#define SN ENDPOINT
#define SS SEGMENT
#define SC CORNER
#define ST TEE
#define SX CROSS

/* ************************************************************************** */
/*                             DEFAULT GAME                                   */
/* ************************************************************************** */

static shape default_p[] = {
    SC, SN, SN, SC, SN, /* row 0 */
    ST, ST, ST, ST, ST, /* row 1 */
    SN, SN, ST, SN, SS, /* row 2 */
    SN, ST, ST, SC, SS, /* row 3 */
    SN, ST, SN, SN, SN, /* row 4 */
};

static direction default_o[] = {
    DW, DN, DW, DN, DS, /* row 0 */
    DS, DW, DN, DE, DE, /* row 1 */
    DE, DN, DW, DW, DE, /* row 2 */
    DS, DS, DN, DW, DN, /* row 3 */
    DE, DW, DS, DE, DS, /* row 4 */
};

static direction default_s[] = {
    DE, DW, DE, DS, DS, /* row 0 */
    DE, DS, DS, DN, DW, /* row 1 */
    DN, DN, DE, DW, DS, /* row 2 */
    DE, DS, DN, DS, DN, /* row 3 */
    DE, DN, DW, DN, DN, /* row 4 */
};

/* ************************************************************************** */

game game_default(void) { return game_new(default_p, default_o); }

/* ************************************************************************** */

game game_default_solution(void) { return game_new(default_p, default_s); }

/* ************************************************************************** */

void game_print(cgame g)
{
  assert(g);
  printf("      ");
  for (uint j = 0; j < game_nb_cols(g); j++) printf("%d ", j);
  printf("\n");
  printf("     ");
  for (uint j = 0; j < 2 * game_nb_cols(g) + 1; j++) printf("-");
  printf("\n");
  for (uint i = 0; i < game_nb_rows(g); i++) {
    printf("  %d | ", i);
    for (uint j = 0; j < game_nb_cols(g); j++) {
      shape s = game_get_piece_shape(g, i, j);
      direction d = game_get_piece_orientation(g, i, j);
      char* ch = _square2str(s, d);
      printf("%s ", ch);
    }
    printf("|\n");
  }
  printf("     ");
  for (uint j = 0; j < 2 * game_nb_cols(g) + 1; j++) printf("-");
  printf("\n");
}

/* ************************************************************************** */

int DIR2OFFSET[][2] = {
    [NORTH] = {-1, 0},
    [EAST] = {0, 1},
    [SOUTH] = {1, 0},
    [WEST] = {0, -1},
};

/* ************************************************************************** */

bool game_get_ajacent_square(cgame g, uint i, uint j, direction d,  //
                             uint* pi_next, uint* pj_next)
{
  assert(g);
  assert(i < g->nb_rows);
  assert(j < g->nb_cols);

  // convert direction to offset
  int i_offset = DIR2OFFSET[d][0];
  int j_offset = DIR2OFFSET[d][1];

  // move to the next square in a given direction
  int ii = i + i_offset;
  int jj = j + j_offset;

  if (game_is_wrapping(g)) {
    ii = (ii + game_nb_rows(g)) % game_nb_rows(g);
    jj = (jj + game_nb_cols(g)) % game_nb_cols(g);
  }

  // check if next square at (ii,jj) is out of grid
  if (ii < 0 || ii >= (int)g->nb_rows) return false;
  if (jj < 0 || jj >= (int)g->nb_cols) return false;

  *pi_next = ii;
  *pj_next = jj;

  return true;
}

/* ************************************************************************** */

#define OPPOSITE_DIR(d) ((d + 2) % NB_DIRS)
#define NEXT_DIR_CW(d) ((d + 1) % NB_DIRS)
#define NEXT_DIR_CCW(d) ((d + 3) % NB_DIRS)

static bool has_half_edge(shape s, direction o, direction d)
{
  switch (s) {
    case EMPTY:
      return false;
    case ENDPOINT:
      return (d == o);
    case SEGMENT:
      return (d == o || d == OPPOSITE_DIR(o));
    case TEE:
      return (d != OPPOSITE_DIR(o));
    case CORNER:
      return (d == o || d == NEXT_DIR_CW(o));
    case CROSS:
      return true;
    default:
      assert(true);
      return false;
  }
}

/* ************************************************************************** */

bool game_has_half_edge(cgame g, uint i, uint j, direction d)
{
  assert(g);
  assert(i < g->nb_rows);
  assert(j < g->nb_cols);
  assert(d >= 0 && d < NB_DIRS);
  shape s = game_get_piece_shape(g, i, j);
  direction o = game_get_piece_orientation(g, i, j);
  return has_half_edge(s, o, d);
}

/* ************************************************************************** */

edge_status game_check_edge(cgame g, uint i, uint j, direction d)
{
  bool he = game_has_half_edge(g, i, j, d);
  uint nexti, nextj;
  bool next = game_get_ajacent_square(g, i, j, d, &nexti, &nextj);

  /* The status can be simply computed based on the number of half-hedges:
   *  - 0: no edge
   *  - 1: mismatched edge
   *  - 2: well-matched edge
   */

  edge_status status = NOEDGE;
  if (he) status += 1;
  if (next) {
    bool nexthe = game_has_half_edge(g, nexti, nextj, OPPOSITE_DIR(d));
    if (nexthe) status += 1;
  }

  return status;
}

/* ************************************************************************** */

static bool _mismatched(cgame g, uint i, uint j, direction d)
{
  edge_status status = game_check_edge(g, i, j, d);
  return status == MISMATCH;
}

/* ************************************************************************** */

// check if the game is well paired, ie. there is no edge mismatch
bool game_is_well_paired(cgame g)
{
  for (uint i = 0; i < game_nb_rows(g); i++)
    for (uint j = 0; j < game_nb_cols(g); j++)
      for (direction d = 0; d < NB_DIRS; d++) {
        if (_mismatched(g, i, j, d)) return false;
      }
  return true;
}

/* ************************************************************************** */

/* square colors used in BFS algorithm */
typedef enum {
  WHITE, /* square not yet visited  */
  GRAY,  /* square partially visited */
  BLACK, /* square fully visited (included all its neighbors) */
} bfscolor;

bool game_is_connected(cgame g)
{
  /* In this algorithm, we assume all pieces are well paired (no edge mismatch).
   */

  assert(g);
  uint nb_cols = g->nb_cols;
  uint nb_rows = g->nb_rows;

  // check precondition, but it should be already checked by the caller!
  if (!game_is_well_paired(g)) return false;

  /* initialize visited array */
  bfscolor visited[nb_rows][nb_cols];
  for (uint i = 0; i < nb_rows; i++)
    for (uint j = 0; j < nb_cols; j++) {
      shape s = game_get_piece_shape(g, i, j);
      visited[i][j] = WHITE;
      if (s == EMPTY) visited[i][j] = BLACK;
    }

  /* lookup for a first square to start BFS */
  bool start_found = false;
  for (uint i = 0; i < nb_rows; i++)
    for (uint j = 0; j < nb_cols; j++) {
      if (visited[i][j] == WHITE && !start_found) {
        visited[i][j] = GRAY;
        start_found = true;
      }
    }

  /* BFS Algorithm */
  bool new_visited = false;
  do {
    new_visited = false;
    for (uint i = 0; i < nb_rows; i++)
      for (uint j = 0; j < nb_cols; j++)
        /* if the square (i,j) has been already visited, then we mark all its
         * connected neighbors as visited...  */
        if (visited[i][j] == GRAY) {
          for (direction d = 0; d < NB_DIRS; d++) {
            if (!game_has_half_edge(g, i, j, d)) continue;
            uint nexti, nextj;
            bool next = game_get_ajacent_square(g, i, j, d, &nexti, &nextj);
            assert(next); /* Always true if the game is well paired! */
            edge_status es = game_check_edge(g, i, j, d);
            assert(es == MATCH); /* Always true... */

            if (visited[nexti][nextj] == WHITE) {
              visited[nexti][nextj] = GRAY;
              new_visited = true;
            }
          }
          /* now, we will not come back to this square... */
          visited[i][j] = BLACK;
        }
  } while (new_visited == true);

  // check all squares have been visited
  for (uint i = 0; i < nb_rows; i++)
    for (uint j = 0; j < nb_cols; j++) {
      if (visited[i][j] != BLACK) {
        return false;
      }
    }

  return true;
}

/* ************************************************************************** */
