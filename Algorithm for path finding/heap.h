#ifndef HEAP_H
#define HEAP_H
#include <stdbool.h>

// Structure de tas binaire:
//
//  array = tableau de stockage des objets à partir de l'indice 1 (au lieu de 0)
//  n     = nombre d'objets (qui sont des void*) stockés dans le tas
//  nmax  = nombre maximum d'objets stockables dans le tas
//  f     = fonction de comparaison de deux objets (min, max, ..., cf. man qsort)
//
// Attention ! "heap" est défini comme un pointeur de structure pour
// optimiser les appels (empilement d'un mot (= 1 pointeur) au lieu de
// quatre sinon (= 1 mot pour chacun des 4 champs).

typedef struct{
  void* *array;
  int n, nmax;
  int (*f)(const void*, const void*);
} *heap;


// Crée un tas pouvant accueillir au plus k>0 objets avec une fonction
// de comparaison f() prédéfinie.
//
// NB: La taille d'un objet pointé par un pointeur h est sizeof(*h).
// Par exemple, si on déclare une variable T comme "char * T;" alors T
// est une variable de type "char*" (avec sizeof(T) = sizeof(1
// pointeur) = 8 octets) et *T est une variable de type "char" (avec
// sizeof(*T) = 1 octet).
heap heap_create(int k, int (*f)(const void *, const void *));


// Détruit le tas h. On supposera h!=NULL. Attention ! Il s'agit de
// libérer ce qui a été alloué par heap_create(). NB: Les objets
// stockés dans le tas n'ont pas à être libérés.
void heap_destroy(heap h);


// Renvoie vrai si le tas h est vide, faux sinon. On supposera
// h!=NULL.
bool heap_empty(heap h);


// Ajoute un objet au tas h. On supposera h!=NULL. Renvoie vrai s'il
// n'y a pas assez de place, et faux sinon.
bool heap_add(heap h, void *object);


// Renvoie l'objet en haut du tas h, c'est-à-dire l'élément minimal
// selon f(), sans le supprimer. On supposera h!=NULL. Renvoie NULL si
// le tas est vide.
void *heap_top(heap h);


// Comme heap_top() sauf que l'objet est en plus supprimé du tas.
// Renvoie NULL si le tas est vide.
void *heap_pop(heap h);

#endif

// Quelques pistes d'améliorations.
//
// 1. Usage de macros. C'est parfois une bonne pratique de définir
// remplacer des suites d'instructions qui se répètent souvent par une
// macro. Cela allège le code, et donc minimise les lieux d'erreurs.
// Mais, comme toutes les bonnes choses, il ne faut pas non plus en
// abuser si cela rend le code illisible et/ou incompréhensible. Par
// exemple, pensez déjà à utiliser la macro SWAP définie dans
// tools.h. Vous pourriez aussi définir FCMP(i,j) permettant de
// comparer les éléments d'indice i et j de votre tas tas. NB: Il est
// d'usage de mettre un #undef FCMP afin de supprimer la macro que
// vous auriez précédemment définie par un #define FCMP(i,j) ...
//
// 2. Pour le champs "array", il est possible d'allouer seulement k
// mots mémoires (au lieu de k+1), mais toujours sans utiliser
// l'indice 0, donc sans changer les fonctions heap_add() et
// heap_pop(). C'est possible en remarquant qu'après avoir fait
// l'allocation ...array = malloc(k * ...) on peut modifier le
// pointeur avec un ...array--. Ainsi, ...array[1] réfère alors à la
// première case du malloc(...). C'est normalement transparent, sauf
// pour heap_create() qu'il faut adapter.
//
// 3. Il est possible d'implémenter heap_add() et heap_pop() sans
// faire d'échanges (rendant l'usage de SWAP non nécessaire), en
// remarquant qu'on a besoin d'écrire l'objet qu'une seule fois à la
// bonne place.
//
// 4. Implémentez la Question 4 du TD sur le heap, c'est-à-dire un tas
// sans avoir à préciser la taille maximum. Allouer une taille par
// défaut, disons 16 mots, puis augmentez la taille lorsque nécessaire
// à l'aide d'un realloc(), cf. man realloc, qui fait la plus grosse
// partie du travail.
//
// 5. Une ruse spéciale peut être introduite pour heap_pop(),
// permettant de ne jamais tester le cas où l'élément courant, disons
// i, n'a qu'un seul fils. C'est lié au fait qu'au moment où l'on est
// améner à tester si i n'a qu'un fils, c'est que cet unique fils est
// précisément le dernier du tas, c'est-à-dire le dernier après
// l'opération de suppression. Mais bien évidemment, si c'est le cas
// l'élément, c'est que i avait 2 fils avant la suppression. Ainsi, on
// peut se restreindre aux cas i à 0 ou 2 fils. Il est clair que cette
// ruse est à implémenter lorsque la fonction heap_pop() est
// parfaitement fonctionnelle et maitrisé.
