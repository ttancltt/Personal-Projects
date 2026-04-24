#ifndef MISC_H
#define MISC_H

#include "taquin.h"

#include <stdbool.h>

/** @brief Symbole ❌. */
#define KO            "\xE2\x9D\x8C"

// échange les variables x et y, via z
#define SWAP(x, y, z) (z) = (x), (x) = (y), (y) = (z)

typedef struct _config {
    int n;           // largeur du jeu
    int **G;         // tableau n x n
    int i0, j0;      // position de la case vide (void), i0=ligne, j0=colonne
    int cost, score; // pour A*: coût et score
    struct _config *parent; // pour A*: pointeur vers le parent
} *config;

typedef struct {
    void **array;
    int n, nmax;
    int (*f)(const void *, const void *);
} *heap;

// Affiche la configuration C sous la forme d'une grille comme
// ci-dessous:
//
// ╔═══╦═══╦═══╗
// ║ • ║ 1 ║ 2 ║
// ╠═══╬═══╬═══╣
// ║ 3 ║ 4 ║ 5 ║
// ╠═══╬═══╬═══╣
// ║ 6 ║ 7 ║ 8 ║
// ╚═══╩═══╩═══╝
// void: (0,0)
// cost: 0
// score: 0
//
// À noter: cost et score ne sont pas affichés si score < 0. Si n > 10,
// alors la configuration n'est pas affichée (trop grande !). De même
// si C est NULL. Un warning est affiché si la case vide et (i0, j0) ne
// correspondent pas.
void print_config(config C);

// Crée un tas dynamique minimum selon une fonction de comparaison f()
// prédéfinie. Le paramètre k est la taille initiale.
heap heap_create(int k, int (*f)(const void *, const void *));

// Renvoie vrai si le tas h est vide, faux sinon. On supposera
// h!=NULL.
bool heap_empty(heap h);

// Ajoute un objet au tas h. On supposera h!=NULL. Renvoie vrai s'il
// n'y a pas assez de place, et faux sinon. La taille est doublé
// dynamiquement.
bool heap_add(heap h, void *object);

// Renvoie l'objet en haut du tas h, c'est-à-dire l'élément minimal
// selon f(), tout en le supprimant. On supposera h!=NULL. Renvoie
// NULL si le tas est vide. La taille du tas n'est pas diminué
// dynamiquement.
void *heap_pop(heap h);

// Pointeur vers le nombre d'éléments dans la table de hachage
// utilisée par is_marked().
extern unsigned *HTN;

// Indique si la configuration C a déjà été traitée. Si c'est le cas,
// la fonction renvoie true. Si elle n'a pas été traité, on la marque
// (via une table de hachage) avant de renvoyer false. On supposera C
// non NULL.
bool is_marked(config C);

//////////////////////////////
// FONCTIONS DÉJÀ FOURNIES, //
//////////////////////////////

// Effectue une suite de déplacements tout en affichant les grilles obtenues.
//
// Cette fonction peut être utilisée pour tester move(), cf. fonction main().
//
// C'est une généralisation de move(), renvoyant la configuration obtenue après
// une suite de déplacements codée par la chaîne de caractères s (qui, comme
// d'habitude en C, se termine par le caractère '\0'). Si un mouvement est
// invalide, il est sauté. Normalement, à part l'affichage, walk(C,"d") devrait
// avoir le même effet que move(C,'d') (de même pour les 3 autres déplacements).
//
config walk(config C, char *S);

// Fonction qui détermine si la configuration C, supposée non NULL,
// peut atteindre la configuration gagnante (en fait, seule la moitié
// des configurations le peuvent).
//
// Explication technique (que vous pouvez ignorer sauf si vous avez le temps) :
//
// On peut gagner si et seulement si les PARITÉS des deux nombres suivants sont
// égales:
//
// - 1er nombre : la distance de Manhattan entre la position de la case vide de
//   C et finale. Comme cette distance est (i0+j0), sa parité se calcule
//   facilement : c'est ((i0+j0) % 2).
//
// - 2ème nombre : le nombre d'inversions de la permutation définie par C.
//   Pour calculer ce nombre, on voit la configuration comme une permutation
//   P: {0,1,...,k-1} -> {0,...,k-1}, où k = n*n, en lisant C->G ligne par
//   ligne, de gauche à droite.
//
//   Le nombre d'inversions d'une telle permutation P: {0,...,k-1} ->
//   {0,...,k-1} est le nombre de paires d'indices {i,j} telles que
//   i<j et P[i]>P[j]. Notons que le nombre d'inversions de P est
//   indépendant de la représentation de P en tableau linéaire ou
//   carré ou autre.
//
bool can_win(config C);

// Construit une configuration aléatoire n x n pouvant atteindre la
// configuration gagnante. Pour cela, il faut générer des configurations
// aléatoires jusqu'à en obtenir une qui permette d'atteindre la configuration
// gagnante, ce qui est testable avec can_win(). On supposera n > 0.
//
// [VOUS N'AVEZ PAS À LA CODER]
//
config create_random(int n);

#endif
