/**
 * @file game_aux.h
 * @brief Auxiliarry Game Functions.
 * @details See @ref index for further details.
 * @copyright University of Bordeaux. All rights reserved, 2024.
 **/

#ifndef __GAME_AUX_H__
#define __GAME_AUX_H__

#include "game.h"

/**
 * @brief Prints a game as text on the standard output stream.
 * @details The different squares are respectively displayed as text, as
 * described in @ref index.
 * @param g the game
 * @pre @p g must be a valid pointer toward a game structure.
 **/
void game_print(cgame g);

/**
 * @brief Creates the default game.
 * @details See the description of the default game in @ref index.
 * @return the created game
 **/
game game_default(void);

/**
 * @brief Creates the default game solution.
 * @details See the description of the default game in @ref index.
 * @return the created game
 **/
game game_default_solution(void);

/**
 * @brief Gets the coordinate of the adjacent square in a given direction.
 * @param g the game
 * @param i row index
 * @param j column index
 * @param d the direction
 * @param[out] pi_next the row index of the adjacent square (output)
 * @param[out] pj_next the column index of the adjacent square (output)
 * @pre @p g must be a valid pointer toward a game structure.
 * @pre @p i < game height
 * @pre @p j < game width
 * @return true if the adjacent square is inside the grid, false otherwise
 */
bool game_get_ajacent_square(cgame g, uint i, uint j, direction d,
                             uint* pi_next, uint* pj_next);

/**
 * @brief Checks if a piece has a half-edge in a given direction.
 * @param g the game
 * @param i row index
 * @param j column index
 * @param d the direction
 * @pre @p g must be a valid pointer toward a game structure.
 * @pre @p i < game height
 * @pre @p j < game width
 * @return true if the piece has a half-edge in the given direction, false
 * otherwise
 */
bool game_has_half_edge(cgame g, uint i, uint j, direction d);

/**
 * @brief Edge status enumeration.
 * @details See @ref index for further details.
 */
typedef enum {
  NOEDGE = 0,   /**< no edge */
  MISMATCH = 1, /**< mismatched edge */
  MATCH = 2,    /**< well-matched edge */
} edge_status;

/**
 * @brief Checks the status of an edge between two adjacent squares.
 * @details There are three possible statuses: NOEDGE, MATCH, MISMATCH.
 * @param g the game
 * @param i row index
 * @param j column index
 * @param d the direction of the adjacent square to consider
 * @pre @p g must be a valid pointer toward a game structure.
 * @pre @p i < game height
 * @pre @p j < game width
 * @return the status of the edge
 */
edge_status game_check_edge(cgame g, uint i, uint j, direction d);

/**
 * @brief Checks if the game is well paired.
 * @details This function checks that there is no edge mismatch, i.e. all the
 * half-edges are correctly paired.
 * @param g the game
 * @pre @p g must be a valid pointer toward a game structure.
 * @return true if the game is well paired, false otherwise
 */
bool game_is_well_paired(cgame g);

/**
 * @brief Checks if the game is connected.
 * @details This function checks that all the pieces are connected, i.e. there
 * is a path between any two non-empty pieces.
 * @param g the game
 * @pre @p g must be a valid pointer toward a game structure.
 * @pre The game @p g is assumed to be well paired.
 * @return true if the game is connected, false otherwise
 */
bool game_is_connected(cgame g);

#endif  // __GAME_AUX_H__
