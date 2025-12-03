// wrapper.c
#include <emscripten.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"
#include "game_aux.h"
#include "game_ext.h"
#include "game_tools.h"

//
// --- THÊM DÒNG NÀY Ở ĐẦU ---
#ifdef __cplusplus
extern "C" {
#endif
// ---------------------------
/* ******************** Game WASM API ******************** */

EMSCRIPTEN_KEEPALIVE
game new_default(void) {
  return game_default();
}


EMSCRIPTEN_KEEPALIVE
void delete(game g) {
  game_delete(g);
}

EMSCRIPTEN_KEEPALIVE
void play_move(game g, uint i, uint j, int nb_quarter_turns) {
  game_play_move(g, i, j, nb_quarter_turns);
}

EMSCRIPTEN_KEEPALIVE
void restart(game g) {
  game_shuffle_orientation(g);
}

EMSCRIPTEN_KEEPALIVE
bool won(game g) {
  return game_won(g);
}

EMSCRIPTEN_KEEPALIVE
shape get_piece_shape(cgame g, uint i, uint j) {
  return game_get_piece_shape(g, i, j);
}

EMSCRIPTEN_KEEPALIVE
direction get_piece_orientation(cgame g, uint i, uint j) {
  return game_get_piece_orientation(g, i, j);
}

EMSCRIPTEN_KEEPALIVE
uint nb_rows(cgame g) {
  return game_nb_rows(g);
}

EMSCRIPTEN_KEEPALIVE
uint nb_cols(cgame g) {
  return game_nb_cols(g);
}

EMSCRIPTEN_KEEPALIVE
void undo(game g) {
  game_undo(g);
}

EMSCRIPTEN_KEEPALIVE
void redo(game g) {
  game_redo(g);
}

EMSCRIPTEN_KEEPALIVE
bool solve(game g) {
  game_solve(g);
  return game_won(g);
}

EMSCRIPTEN_KEEPALIVE
game new_random(uint nb_rows, uint nb_cols, bool wrapping, uint nb_empty, uint nb_extra) {
  return game_random(nb_rows, nb_cols, wrapping, nb_empty, nb_extra);
}

EMSCRIPTEN_KEEPALIVE
game load(char *filename) {
  return game_load(filename);
}

EMSCRIPTEN_KEEPALIVE
void save(cgame g, char *filename) {
  game_save(g, filename);
}
// --- THÊM DÒNG NÀY Ở CUỐI ---
#ifdef __cplusplus
}
#endif
// ----------------------------

