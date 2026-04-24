#ifndef PPPP_H
#define PPPP_H

#include "tools.h"           // pour la structure de point
#include "tsp_brute_force.h" // pour utiliser dist()
#include "htable.h"          // pour la table de hachage


// Un couple d'indices.
typedef struct{
  int i,j;
} couple;


// Algorithme selon la méthode naïve en O(n^2).
// Il s'agit d'examiner toutes les paires de points possibles.
// Suppose qu'il y a au moins n >= 2 points dans V.
couple pppp_naive(point* V, int n);


// Algorithme selon la méthode diviser-pour-régner en O(nlog(n)).
// Il s'agit de l'algorithme vu en cours, cf. section 5.2.3.
// Suppose qu'il y a au moins n >= 2 points dans V.
couple pppp_divide(point* V, int n);


// Algorithme probabiliste en O(n) en moyenne.
// Il s'agit de l'algorithme vu en TD, cf. aussi section 5.2.6 du cours.
// Suppose qu'il y a au moins n >= 2 points dans V.
couple pppp_random(point* V, int n);


#endif
