#ifndef __TOOLS_H__
#define __TOOLS_H__


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>


// alt: -Wno-deprecated-declarations 
#define GL_SILENCE_DEPRECATION

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>


// échange les variables x et y, via z
#define SWAP(x, y, z)  (z) = (x), (x) = (y), (y) = (z)

// réel aléatoire dans [0,1]
#define RAND01  ((double)random()/RAND_MAX)

// taille maximum du nom d'un fichier
#define MAX_FILE_NAME  256

// nombre maximum de points affichable à l'écran (avant erreur)
#define MAX_VERTICES 1000000

// nombre d'éléments d'un tableau statique
#define CARD(T) ((int)(sizeof(T)/sizeof(*T)))


////////////////////////
//
// SPÉCIFIQUE POUR TSP
//
////////////////////////

// Un point (x,y).
typedef struct {
  double x, y;
} point;


// Construit la prochaine permutation de P de taille n dans l'ordre
// lexicographique. Le résultat est écrit dans P. La fonction renvoie
// true sauf si à l'appel P était la dernière permutation. Dans ce cas
// false est renvoyé et P n'est pas modifiée. Il n'est pas nécessaire
// que les valeurs de P soit dans [0,n[.
bool NextPermutation(int *P, int n);

// Primitives de génération des points.
point *generatePoints(int n); // n points au hasard dans [0,width] × [0,height]
point *generateCircles(int n, int k); // n points au hasard sur k cercles concentriques
point *generatePower(int n, double p); // points selon loi puissance sur un disque
point *generateConvex(int n); // n points au hasard en position convexe
point *generateGrid(int *n, int p, int q); // points sur une grille régulière p × q
point *generateLoad(int *n, char *file); // points lus depuis un fichier

// Format d'un fichier de points: si n<0, alors les points seront
// recentrés dans la fenêtre. Les paramètres "factor" et "shift", qui
// sont optionnels, permettent de transformer chaque point (x_i,y_i)
// en (x_i*f + dx, y_i*f + dy). Les lignes vides ou commençant par
// '##' sont ignorées.
//
// ## ceci est un commentaires
// n
// factor = f
// shift = dx dy
//
// x_1 y_2
// ... ...
// x_n y_n

// Primitives de dessin.
void drawTour(point *V, int n, int *P); // affichage de la tournée P
void drawPath(point *V, int n, int *P, int k); // desine les k premiers points de P

// Un graphe G (pour MST).
typedef struct {
  int n;         // n=nombre de sommets
  int *deg;      // deg[u]=nombre de voisins du sommet u
  int **list;    // list[u][i]=i-ème voisin de u, i=0..deg[u]-1
} graph;

void drawGraph(point *V, int n, int *P, graph G); // affiche graphe, arbre et tournée


////////////////////////
//
// SPÉCIFIQUE POUR A*
//
////////////////////////


// Une position d'une case dans la grille.
typedef struct {
  int x, y;
} position; 

// Une grille.
typedef struct {
  int X, Y;       // dimensions: X et Y
  int **texture;  // texture des cases:  texture[x][y], 0<= x < X, 0 <= y < Y
  int **mark;     // marquage des cases: mark[x][y],    0<= x < X, 0 <= y < Y
  position start; // position de la source
  position end;   // position de la destination
  //
  // spécifique surface 3D
  //
  float** height; // hauteur des cases: height[x][y], 0<= x < X, 0 <= y < Y
  float hmin,hmax; // hauteur min/max: hmin <= height[x][y] <= hmax 
} grid;

// Codes possibles des cases d'une grille pour les champs .texture et
// .mark. L'ordre est important! Il doit être cohérent avec les
// tableaux color[] (de tools.c) et weight[] (de a_star.c).
enum {

  // pour .texture
  TX_FREE = 0, // case vide
  TX_WALL,     // Mur
  TX_SAND,     // Sable
  TX_WATER,    // Eau
  TX_MUD,      // Boue
  TX_GRASS,    // Herbe
  TX_TUNNEL,   // Tunnel
  TX_GRAVEL,   // Gravière
  TX_ROCK,     // Rocher
  TX_SNOW,     // Neige
  TX_ICE,      // Glace
  TX_LAKE,     // Lac
  TX_POOL,     // Fosse
  TX_MEADOW,   // Prairie
  TX_FOREST,   // Forêt
  TX_TRACK,    // Sentier

  // pour .mark
  MK_NULL,     // sommet non marqué
  MK_USED,     // sommet marqué dans P
  MK_FRONT,    // sommet marqué dans Q
  MK_PATH,     // sommet dans le chemin

  // divers
  C_START,    // couleur de la position de départ
  C_END,      // idem pour la destination
  C_FINAL,    // couleur de fin du dégradé pour MK_USED
  C_END_WALL, // couleur de destination si sur TX_WALL

  // extra
  MK_USED2,    // sommet marqué dans P_t
  C_FINAL2,   // couleur de fin du dégradé pour MK_USED2
  C_PATH2,    // couleur du chemin si 'c'
};

// Routines de dessin et de construction de grille. Le point (0,0) de
// la grille est le coin en haut à gauche. Pour plus de détails sur
// les fonctions, se reporter à tools.c.

void drawGrid(grid); // affiche une grille
grid initGridLaby(int,int,int w); // labyrithne x,y, w = largeur des couloirs
grid initGridPoints(int,int,int t,double p); // pts de texture t avec proba p
grid initGridFile(char*); // construit une grille à partir d'un fichier
grid initGrid3D(int,int); // construit une grille en fonction de l'altitude
void addRandomBlob(grid,int t,int n); // ajoute n blobs de texture t
void addRandomArc(grid,int t,int n); // ajoute n arcs de texture t
position randomPosition(grid,int t); // position aléatoire sur texture de type t
void freeGrid(grid); // libère la mémoire alouée par les fonctions initGridXXX()


////////////////////////
//
// SPÉCIFIQUE POUR SDL
//
////////////////////////


// Quelques variables globales.
//
// NB: Certains compilateurs râlent si on n'ajoute pas "extern" devant
//     les variables initialisées ailleurs (dans tools.c). Suivant la
//     même logique on pourrait ajouter des "extern" devant toutes les
//     fonctions des .h, mais cela semble inutile même sous les
//     compilateurs râleurs.

extern int width, height;  // taille de la fenêtre, mise à jour si redimensionnée
extern bool update;        // si vrai, force le dessin dans les fonctions drawXXX()
extern bool running;       // devient faux si 'q' est pressé
extern bool hover;         // si vrai, permet de déplacer un sommet
extern bool erase;         // pour A*: efface les couleurs à la fin ou pas
extern double scale;       // zoom courrant, 1 par défaut
extern GLfloat size_pt;    // taille des points

// pour la 3D
extern int mounts;         // nombre de montagnes ou de creux
extern float flatness;     // applatissement fixant la hauteur max à min(X,Y)/flatness
extern float steepness;    // raideur des pics: 0=plat, 10=raide, 50=très raide
extern float holeness;     // pourcentage de creux [0,1]
extern float disparity;    // pourcentage de disparité des hauteurs [0,1]

// couleurs pour drawX(), valeurs RGB dans [0,1]
extern GLfloat COLOR_GROUND[]; // fond
extern GLfloat COLOR_POINT[];  // point
extern GLfloat COLOR_LINE[];   // ligne de la tournée
extern GLfloat COLOR_PATH[];   // chemin
extern GLfloat COLOR_ROOT[];   // racine, point de départ
extern GLfloat COLOR_TREE[];   // arête de l'arbre

// Initialisation de SDL et OpenGL.
void init_SDL_OpenGL(void);

// Libération de la mémoire.
void cleaning_SDL_OpenGL(void);

// Gestion des évènements (souris, touche clavier, redimensionnement
// de la fenêtre, etc.). Renvoie true ssi la position d'un point a
// changé. Si wait_event = true, alors on attend qu'un évènement se
// produise (et on le gère). Sinon, on n'attend pas et on gère
// l'évènement courant. La touche 'h' affiche une aide dans la
// console, soit la chaîne HELP[] définie dans tools.c.
bool handleEvent(bool wait_event);

// Définit le nombre d'appels à DrawGrid par seconde (minimum 1),
// utile notamment quand vous trouvez un chemin et pour ralentir
// l'affichage à ce moment.
void speedSet(unsigned long speed);

// Fonction de chronométrage. Renvoie une durée (écrite sous forme de
// texte) depuis le dernier apppel à la fonction TopChrono(i) où i est
// le numéro de chronomètre souhaité. La précision varie entre la
// minute et le 1/1000e de seconde.
char *TopChrono(const int i);

#endif // __TOOLS_H__
