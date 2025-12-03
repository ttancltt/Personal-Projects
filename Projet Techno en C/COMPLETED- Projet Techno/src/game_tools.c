/* game_tools.c */
#include "game_tools.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "game.h"
#include "game_aux.h"
#include "game_private.h"

shape shape_selec(char shape) {
  switch (shape) {
    case 'E':
      return EMPTY;
      break;
    case 'N':
      return ENDPOINT;
      break;
    case 'S':
      return SEGMENT;
      break;
    case 'C':
      return CORNER;
      break;
    case 'T':
      return TEE;
      break;
    case 'X':
      return CROSS;
      break;
    default:
      fprintf(stderr, "Wrong file format \n");
      exit(EXIT_FAILURE);
  }
}
char dir_to_char(direction o) {
  switch (o) {
    case NORTH:
      return ('N');
      break;
    case SOUTH:
      return ('S');
      break;
    case EAST:
      return ('E');
      break;
    case WEST:
      return ('W');
      break;
    default:
      fprintf(stderr, "Wrong file format \n");
      exit(EXIT_FAILURE);
  }
}
char shape_to_char(shape s) {
  switch (s) {
    case TEE:
      return ('T');
      break;
    case SEGMENT:
      return ('S');
      break;
    case CORNER:
      return ('C');
      break;
    case CROSS:
      return ('X');
      break;
    case EMPTY:
      return ('E');
      break;
    case ENDPOINT:
      return ('N');
      break;
    default:
      fprintf(stderr, "Wrong file format \n");
      exit(EXIT_FAILURE);
  }
}
direction orientation_selec(char dir) {
  switch (dir) {
    case 'E':
      return EAST;
      break;
    case 'N':
      return NORTH;
      break;
    case 'S':
      return SOUTH;
      break;
    case 'W':
      return WEST;
      break;
    default:
      fprintf(stderr, "Wrong file format \n");
      exit(EXIT_FAILURE);
  }
}

game game_load(char *filename) {
  FILE *map = fopen(filename, "r");
  assert(map);
  uint nb_rows;
  uint nb_cols;
  int wrapped;
  char char_shape;
  char char_dir;
  fscanf(map, "%d %d %d", &nb_rows, &nb_cols, &wrapped);
  direction orientaions[nb_rows * nb_cols];
  shape shapes[nb_rows * nb_cols];
  for (int i = 0; i < nb_rows; i++) {
    for (int j = 0; j < nb_cols; j++) {
      fscanf(map, " %c%c ", &char_shape, &char_dir);
      shapes[i * nb_cols + j] = shape_selec(char_shape);
      orientaions[i * nb_cols + j] = orientation_selec(char_dir);
    }
  }
  bool w = wrapped == 0 ? false : true;
  game g = game_new_ext(nb_rows, nb_cols, shapes, orientaions, w);
  fclose(map);
  // game_print(g);
  return g;
}

void game_save(cgame g, char *filename) {
  assert(g);
  FILE *f = fopen(filename, "w");
  assert(f);
  int w = game_is_wrapping(g) ? 1 : 0;
  int nb_rows = game_nb_rows(g);
  int nb_cols = game_nb_cols(g);
  fprintf(f, "%d %d %d", nb_rows, nb_cols, w);
  for (int i = 0; i < nb_rows; i++) {
    fprintf(f, "\n");
    for (int j = 0; j < nb_cols; j++) {
      direction orientation = game_get_piece_orientation(g, i, j);
      shape shape = game_get_piece_shape(g, i, j);
      char o = dir_to_char(orientation);
      char s = shape_to_char(shape);
      fprintf(f, "%c%c ", s, o);
    }
  }
  fclose(f);
}
// @copyright University of Bordeaux. All rights reserved, 2024.

/** @brief Hard-coding of pieces (shape & orientation) in an integer array.
 * @details The 4 least significant bits encode the presence of an half-edge in
 * the N-E-S-W directions (in that order). Thus, binary coding 1100 represents
 * the piece "└" (a corner in north orientation).
 */
static uint _code[NB_SHAPES][NB_DIRS] = {
    {0b0000, 0b0000, 0b0000, 0b0000},  // EMPTY {" ", " ", " ", " "}
    {0b1000, 0b0100, 0b0010, 0b0001},  // ENDPOINT {"^", ">", "v", "<"},
    {0b1010, 0b0101, 0b1010, 0b0101},  // SEGMENT {"|", "-", "|", "-"},
    {0b1100, 0b0110, 0b0011, 0b1001},  // CORNER {"└", "┌", "┐", "┘"}
    {0b1101, 0b1110, 0b0111, 0b1011},  // TEE {"┴", "├", "┬", "┤"}
    {0b1111, 0b1111, 0b1111, 0b1111}   // CROSS {"+", "+", "+", "+"}
};

/** encode a shape and an orientation into an integer code */
static uint _encode_shape(shape s, direction o) { return _code[s][o]; }

/** decode an integer code into a shape and an orientation */
static bool _decode_shape(uint code, shape *s, direction *o) {
  assert(code >= 0 && code < 16);
  assert(s);
  assert(o);
  for (int i = 0; i < NB_SHAPES; i++)
    for (int j = 0; j < NB_DIRS; j++)
      if (code == _code[i][j]) {
        *s = i;
        *o = j;
        return true;
      }
  return false;
}

/** add an half-edge in the direction d */
static void _add_half_edge(game g, uint i, uint j, direction d) {
  assert(g);
  assert(i < game_nb_rows(g));
  assert(j < game_nb_cols(g));
  assert(d < NB_DIRS);

  shape s = game_get_piece_shape(g, i, j);
  direction o = game_get_piece_orientation(g, i, j);
  uint code = _encode_shape(s, o);
  uint mask = 0b1000 >> d;     // mask with half-edge in the direction d
  assert((code & mask) == 0);  // check there is no half-edge in the direction d
  uint newcode = code | mask;  // add the half-edge in the direction d
  shape news;
  direction newo;
  bool ok = _decode_shape(newcode, &news, &newo);
  assert(ok);
  game_set_piece_shape(g, i, j, news);
  game_set_piece_orientation(g, i, j, newo);
}

#define OPPOSITE_DIR(d) ((d + 2) % NB_DIRS)

/**
 * @brief Add an edge between two adjacent squares.
 * @details This is done by modifying the pieces of the two adjacent squares.
 * More precisely, we add an half-edge to each adjacent square, so as to build
 * an edge between these two squares.
 * @param g the game
 * @param i row index
 * @param j column index
 * @param d the direction of the adjacent square
 * @pre @p g must be a valid pointer toward a game structure.
 * @pre @p i < game height
 * @pre @p j < game width
 * @return true if an edge can be added, false otherwise
 */
static bool _add_edge(game g, uint i, uint j, direction d) {
  assert(g);
  assert(i < game_nb_rows(g));
  assert(j < game_nb_cols(g));
  assert(d < NB_DIRS);

  uint nexti, nextj;
  bool next = game_get_ajacent_square(g, i, j, d, &nexti, &nextj);
  if (!next) return false;

  // check if the two half-edges are free
  bool he = game_has_half_edge(g, i, j, d);
  if (he) return false;
  bool next_he = game_has_half_edge(g, nexti, nextj, OPPOSITE_DIR(d));
  if (next_he) return false;

  _add_half_edge(g, i, j, d);
  _add_half_edge(g, nexti, nextj, OPPOSITE_DIR(d));

  return true;
}

game game_random(uint nb_rows, uint nb_cols, bool wrapping, uint nb_empty,
                 uint nb_extra) {
  /* Préconditions */
  assert(nb_rows * nb_cols >= 2);
  assert(nb_empty <= (nb_rows * nb_cols - 2));

  /* 1. Création d'un jeu vide */
  game g = game_new_empty_ext(nb_rows, nb_cols, wrapping);
  if (g == NULL) return NULL;

  /* Calcul du nombre total de cases et du nombre de cases non vides désiré */
  uint total_cells = nb_rows * nb_cols;
  uint non_empty_target = total_cells - nb_empty;

  /* 2. Allocation d'une matrice 2D pour stocker les positions non vides.
     Chaque ligne représente un couple {i, j} */
  int(*candidates)[2] = malloc(non_empty_target * sizeof(*candidates));
  if (candidates == NULL) {
    game_delete(g);
    return NULL;
  }
  uint count = 0;  // nombre actuel de cases non vides

  /* 3. Placement aléatoire d'une solution à 2 pièces */
  uint i0 = rand() % nb_rows;
  uint j0 = rand() % nb_cols;
  uint i1, j1;
  direction d = rand() % NB_DIRS;
  if (!game_get_ajacent_square(g, i0, j0, d, &i1, &j1)) {
    /* Si la direction initiale n'est pas valide, essayer la direction opposée
     */
    d = (d + 2) % NB_DIRS;
    if (!game_get_ajacent_square(g, i0, j0, d, &i1, &j1)) {
      /* En dernier recours, parcourir toutes les directions */
      for (uint k = 0; k < NB_DIRS; k++) {
        if (game_get_ajacent_square(g, i0, j0, k, &i1, &j1)) {
          d = k;
          break;
        }
      }
    }
  }
  /* Ajout de l'arête initiale pour créer les deux premières pièces */
  if (!_add_edge(g, i0, j0, d)) {
    free(candidates);
    game_delete(g);
    return NULL;
  }
  /* Stockage des positions initiales dans la matrice candidates */
  candidates[0][0] = i0;
  candidates[0][1] = j0;
  candidates[1][0] = i1;
  candidates[1][1] = j1;
  count = 2;

  /* 4. Extension de la solution (construction de l'arbre couvrant) */
  while (count < non_empty_target) {
    /* Choisir aléatoirement une case non vide parmi candidates */
    uint idx = rand() % count;  // indice dans la matrice 2D
    uint ci = candidates[idx][0];
    uint cj = candidates[idx][1];

    /* Choisir une direction aléatoire */
    direction d = rand() % NB_DIRS;
    uint ni, nj;
    /* Si la case adjacente existe et est vide */
    if (game_get_ajacent_square(g, ci, cj, d, &ni, &nj) &&
        game_get_piece_shape(g, ni, nj) == EMPTY) {
      if (_add_edge(g, ci, cj, d)) {
        /* Ajouter la nouvelle case dans candidates */
        candidates[count][0] = ni;
        candidates[count][1] = nj;
        count++;
      }
    }
    /* Sinon, continuer l'itération */
  }

  /* 5. Ajout des arêtes supplémentaires pour créer des cycles (nb_extra) */
  uint extra_added = 0;
  while (extra_added < nb_extra) {
    uint idx = rand() % count;
    uint ci = candidates[idx][0];
    uint cj = candidates[idx][1];

    direction d = rand() % NB_DIRS;
    uint ni, nj;
    /* Vérifier que la case adjacente existe, est non vide et qu'il n'y a pas
     * encore d'arête */
    if (game_get_ajacent_square(g, ci, cj, d, &ni, &nj) &&
        game_get_piece_shape(g, ni, nj) != EMPTY &&
        !game_has_half_edge(g, ci, cj, d)) {
      if (_add_edge(g, ci, cj, d)) extra_added++;
    }
  }

  /* Libérer la mémoire allouée pour candidates */
  free(candidates);

  /* Retourner le jeu généré */
  return g;
}

// Hàm đệ quy duyệt qua các ô của lưới trò chơi để tìm giải pháp.
// Mỗi ô được xem như một ký tự, và hàm sẽ gán hướng cho từng ô theo thứ tự
// duyệt hàng (row-major order). Nếu tất cả các ô đã được gán (row == nb_rows),
// kiểm tra xem trạng thái của game có thỏa mãn game_won() hay không. Nếu đúng,
// tăng biến count và in giải pháp.

/* ************************************************************************** */
/*                             HELPER FUNCTIONS                               */
/* ************************************************************************** */

// Kiểm tra các cạnh Bắc/Tây ngay khi xoay mảnh (pruning sớm)
bool _check_current_edges(cgame g, uint i, uint j) {
  uint ni, nj;
  // Kiểm tra cạnh Bắc
  if (game_get_ajacent_square(g, i, j, NORTH, &ni, &nj)) {
    if (game_check_edge(g, i, j, NORTH) == MISMATCH) return false;
  }
  // Kiểm tra cạnh Tây
  if (game_get_ajacent_square(g, i, j, WEST, &ni, &nj)) {
    if (game_check_edge(g, i, j, WEST) == MISMATCH) return false;
  }
  return true;
}

// Xác định số hướng xoay tối đa (xử lý đối xứng)
uint _get_max_rotation(shape s) {
  switch (s) {
    case ENDPOINT:
      return 4;
    case SEGMENT:
      return 2;  // 2 hướng do đối xứng 180°
    case CORNER:
      return 4;
    case TEE:
      return 4;
    case CROSS:
      return 1;  // 1 hướng do đối xứng 4 chiều
    case EMPTY:
      return 1;
    default:
      return 0;
  }
}

/* ************************************************************************** */
/*                         BACKTRACKING WITH PRUNING                          */
/* ************************************************************************** */

// Hàm đệ quy tìm giải pháp đầu tiên
bool _solve_rec(game g, uint index) {
  uint nb_rows = game_nb_rows(g);
  uint nb_cols = game_nb_cols(g);
  uint total = nb_rows * nb_cols;

  if (index == total) return game_won(g);

  uint i = index / nb_cols;
  uint j = index % nb_cols;
  shape s = game_get_piece_shape(g, i, j);
  direction original_dir = game_get_piece_orientation(g, i, j);
  uint max_rot = _get_max_rotation(s);

  for (uint r = 0; r < max_rot; r++) {
    direction new_dir = (original_dir + r) % NB_DIRS;
    game_set_piece_orientation(g, i, j, new_dir);

    if (!_check_current_edges(g, i, j)) {
      game_set_piece_orientation(g, i, j, original_dir);
      continue;
    }

    if (_solve_rec(g, index + 1)) return true;
  }

  game_set_piece_orientation(g, i, j, original_dir);
  return false;
}

// Hàm đệ quy đếm tất cả giải pháp
static void _nb_solutions_rec(cgame g, uint index, uint *count) {
  uint nb_rows = game_nb_rows(g);
  uint nb_cols = game_nb_cols(g);
  uint total = nb_rows * nb_cols;

  if (index == total) {
    if (game_won(g)) (*count)++;
    return;
  }

  uint i = index / nb_cols;
  uint j = index % nb_cols;
  shape s = game_get_piece_shape(g, i, j);
  direction original_dir = game_get_piece_orientation(g, i, j);
  uint max_rot = _get_max_rotation(s);

  for (uint r = 0; r < max_rot; r++) {
    direction new_dir = (original_dir + r) % NB_DIRS;
    game_set_piece_orientation((game)g, i, j, new_dir);

    if (!_check_current_edges(g, i, j)) {
      game_set_piece_orientation((game)g, i, j, original_dir);
      continue;
    }

    _nb_solutions_rec(g, index + 1, count);
  }

  game_set_piece_orientation((game)g, i, j, original_dir);
}

/* ************************************************************************** */
/*                             PUBLIC FUNCTIONS                               */
/* ************************************************************************** */

bool game_solve(game g) {
  if (!g) return false;
  if (game_won(g)) return true;
  game backup = game_copy(g);
  bool solved = _solve_rec(g, 0);
  if (!solved) {
    game tmp = game_copy(backup);
    g = game_copy(tmp);
    game_delete(tmp);
  }
  game_delete(backup);
  return solved;
}

uint game_nb_solutions(cgame g) {
  // Kiểm tra đầu vào hợp lệ
  if (!g) return 0;

  // Tạo bản sao để không làm thay đổi game gốc
  game copy = game_copy(g);
  if (!copy) return 0;  // Xử lý lỗi cấp phát

  uint count = 0;
  _nb_solutions_rec(copy, 0, &count);
  game_delete(copy);

  return count;
}

/**
void genSolutions(uint pos, game g_copy) {
  // Si toutes les pièces ont été traitées, vérifier si la configuration est une
solution if (pos == g_copy->nb_rows * g_copy->nb_cols) { if (game_won(g_copy)) {
  count++; // Augmenter le compteur de solutions si la configuration est
gagnante
  }
  return;
  }

  uint i = pos / g_copy->nb_cols; // Calculer la ligne
  uint j = pos % g_copy->nb_cols; // Calculer la colonne

  // Si la pièce est vide, passer à la suivante
  if (g_copy->shapes[i * g_copy->nb_cols + j] == EMPTY) {
  genSolutions(pos + 1, g_copy); // Passer à la prochaine pièce
  return;
  }

  // Essayer toutes les orientations possibles pour cette pièce
  for (direction d = NORTH; d <= WEST; d++) {
  // Normaliser les orientations pour ignorer les symétries des pièces
  if (g_copy->shapes[i * g_copy->nb_cols + j] == SEGMENT) {
  if (d == SOUTH || d == WEST) continue; // Ignorer les orientations symétriques
pour SEGMENT
  }
  if (g_copy->shapes[i * g_copy->nb_cols + j] == CROSS) {
  if (d == SOUTH || d == WEST) continue; // Ignorer les orientations symétriques
pour CROSS
  }

  g_copy->orientations[i * g_copy->nb_cols + j] = d; // Appliquer l'orientation
  genSolutions(pos + 1, g_copy); // Appel récursif pour la prochaine pièce
  }
 }

 uint game_nb_solutions(cgame g) {
  count = 0; // Réinitialiser le compteur avant de commencer le calcul

  // Crée une copie du jeu pour ne pas altérer l'original
  game *g_copy = game_copy(g); // Assurez-vous que game_copy effectue une copie
profonde de g

  // Démarrer la génération des solutions à partir de la première position
  genSolutions(0, g_copy);

  // Libérer la mémoire allouée pour la copie du jeu
  game_delete(g_copy);

  return count; // Retourner le nombre total de solutions trouvées
 }
 */