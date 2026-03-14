#ifndef TSP_PROG_DYN_H
#define TSP_PROG_DYN_H

#include "tools.h"

// Une cellule de la table.
typedef struct {
  double length; // longueur du chemin minimum D[t][S]
  int pred;      // point précédant t dans la solution D[t][S]
} cell;

int DeleteSet(int S, int i);
int ExtractPath(cell **D, int t, int S, int n, int *Q);
double tsp_prog_dyn(point *V, int n, int *Q);

#endif /* TSP_PROG_DYN_H */
