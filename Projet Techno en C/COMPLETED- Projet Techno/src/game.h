/**
 * @file game.h
 * @brief Basic Game Functions.
 * @details See @ref index for further details.
 * @copyright University of Bordeaux. All rights reserved, 2024.
 **/

#ifndef __GAME_H__
#define __GAME_H__

#include <stdbool.h>

/**
 * @brief Standard unsigned integer type.
 **/
typedef unsigned int uint;

/**
 * @brief Size of the default game grid.
 **/
#define DEFAULT_SIZE 5

/**
 * @brief The different piece shapes in the game.
 **/
typedef enum {
  EMPTY = 0, /**< empty shape ' ' */
  ENDPOINT,  /**< endpoint shape '^' */
  SEGMENT,   /**< segment shape '|' */
  CORNER,    /**< corner shape '└' */
  TEE,       /**< tee shape '┴' */
  CROSS,     /**< cross shape '┼' */
  NB_SHAPES  /**< nb of shapes */
} shape;

/**
 * @brief The four cardinal directions.
 **/
typedef enum {
  NORTH = 0, /**< north */
  EAST,      /**< east */
  SOUTH,     /**< south */
  WEST,      /**< west */
  NB_DIRS    /**< nb of directions */
} direction;

/**
 * @brief The structure pointer that stores the game state.
 **/
typedef struct game_s* game;

/**
 * @brief The structure constant pointer that stores the game state.
 * @details That means that it is not possible to modify the game using this
 * pointer.
 **/
typedef const struct game_s* cgame;

/**
 * @brief Creates a new empty game with defaut size.
 * @details All squares are initialized with an empty shape, placed in the north
 * orientation.
 * @return the created game
 **/
game game_new_empty(void);

/**
 * @brief Creates a new game with default size and initializes it.
 * @param shapes an array describing the piece shape in each square
 * (or NULL to set all shapes to empty)
 * @param orientations an array describing the orientation of piece in each
 * square (or NULL to set all orientations to north)
 * @details Both the arrays for shapes and orientations are 1D arrays of size
 * DEFAULT_SIZE squared, using row-major storage convention.
 * @pre @p shapes must be an initialized array of DEFAULT_SIZE squared or NULL.
 * @pre @p orientations must be an initialized array of DEFAULT_SIZE squared or
 * NULL.
 * @return the created game
 **/
game game_new(shape* shapes, direction* orientations);

/**
 * @brief Duplicates a game.
 * @param g the game to copy
 * @return the copy of the game
 * @pre @p g must be a valid pointer toward a game structure.
 **/
game game_copy(cgame g);

/**
 * @brief Tests if two games are equal.
 * @param g1 the first game
 * @param g2 the second game
 * @param ignore_orientation if true, the orientation of pieces is ignored
 * @return true if the two games are equal, false otherwise
 * @pre @p g1 must be a valid pointer toward a game structure.
 * @pre @p g2 must be a valid pointer toward a game structure.
 **/
bool game_equal(cgame g1, cgame g2, bool ignore_orientation);

/**
 * @brief Deletes the game and frees the allocated memory.
 * @param g the game to delete
 * @pre @p g must be a valid pointer toward a game structure.
 **/
void game_delete(game g);

/**
 * @brief Sets the piece shape in a given square.
 * @details This function is useful for initializing the squares of an empty
 * game.
 * @param g the game
 * @param i row index
 * @param j column index
 * @param s shape
 * @pre @p g must be a valid pointer toward a game structure.
 * @pre @p i < game height
 * @pre @p j < game width
 **/
void game_set_piece_shape(game g, uint i, uint j, shape s);

/**
 * @brief Sets the piece orientation in a given square.
 * @details This function is useful for initializing the squares of an empty
 * game.
 * @param g the game
 * @param i row index
 * @param j column index
 * @param o orientation
 * @pre @p g must be a valid pointer toward a game structure.
 * @pre @p i < game height
 * @pre @p j < game width
 **/
void game_set_piece_orientation(game g, uint i, uint j, direction o);

/**
 * @brief Gets the piece shape in a given square.
 * @param g the game
 * @param i row index
 * @param j column index
 * @pre @p g must be a valid pointer toward a game structure.
 * @pre @p i < game height
 * @pre @p j < game width
 * @return the piece shape
 **/
shape game_get_piece_shape(cgame g, uint i, uint j);

/**
 * @brief Gets the piece orientation in a given square.
 * @param g the game
 * @param i row index
 * @param j column index
 * @pre @p g must be a valid pointer toward a game structure.
 * @pre @p i < game height
 * @pre @p j < game width
 * @return the piece orientation
 **/
direction game_get_piece_orientation(cgame g, uint i, uint j);

/**
 * @brief Plays a move in a given square.
 * @details Rotate a piece clockwise by some quarter turns. If
 * @p nb_quarter_turns is negative, the piece is rotated anti-clockwise.
 * @param g the game
 * @param i row index
 * @param j column index
 * @param nb_quarter_turns signed number of quarter turns
 * @pre @p g must be a valid pointer toward a game structure.
 * @pre @p i < game height
 * @pre @p j < game width
 **/
void game_play_move(game g, uint i, uint j, int nb_quarter_turns);

/**
 * @brief Checks if the game is won.
 * @param g the game
 * @details This function checks that all the game rules are satisfied. More
 * precisely, it checks that all the pieces in the grid are well paired and form
 * a connected graph (possibly with cycles).
 * @pre @p g must be a valid pointer toward a game structure.
 * @return true if the game is won, false otherwise
 **/
bool game_won(cgame g);

/**
 * @brief Resets all the piece orientations to the north.
 * @param g the game
 * @pre @p g must be a valid pointer toward a game structure.
 **/
void game_reset_orientation(game g);

/**
 * @brief Shuffles all the piece orientations.
 * @param g the game
 * @pre @p g must be a valid pointer toward a game structure.
 */
void game_shuffle_orientation(game g);

#endif  // __GAME_H__
