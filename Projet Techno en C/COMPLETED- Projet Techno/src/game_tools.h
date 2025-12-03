/**
 * @file game_tools.h
 * @brief Game Tools.
 * @details See @ref index for further details.
 * @copyright University of Bordeaux. All rights reserved, 2024.
 *
 **/

#ifndef __GAME_TOOLS_H__
#define __GAME_TOOLS_H__
#include <stdbool.h>
#include <stdio.h>

#include "game.h"
#include "game_aux.h"
#include "game_ext.h"

/**
 * @name Game Tools
 * @{
 */

/**
 * @brief Creates a game by loading its description from a text file.
 * @details See details in the file format description.
 * @param filename input file
 * @return the loaded game
 **/
game game_load(char *filename);

/**
 * @brief Saves a game in a text file.
 * @details See details the file format description.
 * @param g game to save
 * @param filename output file
 **/
void game_save(cgame g, char *filename);

/**
 * @brief Creates a random game solution with a given size and options.
 * @param nb_rows number of rows in game
 * @param nb_cols number of columns in game
 * @param wrapping wrapping option
 * @param nb_empty number of empty squares
 * @param nb_extra number of extra edges, that make cycles (if possible)
 * @pre nb_cols * nb_rows >= 2
 * @pre nb_empty <= (nb_cols * nb_rows - 2)
 * @pre nb_extra must be small enough compared to the number of non-empty
 * squares
 * @return the generated random game (or NULL in case of error)
 */
game game_random(uint nb_rows, uint nb_cols, bool wrapping, uint nb_empty,
                 uint nb_extra);

/**
 * @brief Computes the solution of a given game.
 * @param g the game to solve
 * @details The game @p g is updated with the first solution found. If there are
 * no solution for this game, @p g must be unchanged.
 * @return true if a solution is found, false otherwise
 */
bool game_solve(game g);

/**
 * @brief Computes the total number of solutions of a given game.
 * @param g the game
 * @details Solutions with pieces in symmetrical positions (SEGMENT or CROSS)
 * should be counted only once.
 * @post The game @p g must be unchanged.
 * @return the number of solutions
 */

uint game_nb_solutions(cgame g);

void game_all_won(game g, uint i, uint j);

/**
 * @}
 */

#endif  // __GAME_TOOLS_H__
