/*

CONSIGNES:

- Commencez par lire le sujet(.pdf) ! Compiler simplement avec make.

- C'est une épreuve individuelle. Toute communication avec autrui est
  interdite. Les IA et assistants de programmation sont interdits. Les
  connexions sortantes de UB et entrantes depuis l'extérieur vers votre compte
  sont interdites (elles sont enregistrées et seront vérifiées après l'épreuve).

- Page Moodle de récupération des sources et soumission du fichier taquin.c :

     https://moodle.u-bordeaux.fr/course/view.php?id=6930#section-1

- Ne changez pas l'ordre des fonctions de taquin.c. Ne modifiez pas misc.h.

- Lisez le type config de misc.h. Il désigne un pointeur vers une structure.

- Complétez/codez les fonctions incomplètes, à savoir :

  - config copy_config(config C)
  - void set_zero(config C)
  - config move(config C, char m)
  - void random_permutation(int *P, int k)
  - int fcmp_config(const void *X, const void *Y)
  - int h1(config X)
  - int h2(config X)
  - int h3(config X) [EN BONUS]
  - config solve(config S, heuristic h)

  Une description de chaque fonction est fournie avant son squelette (que vous
  devez compléter). Certaines fonctions sont fournies. Vous n'avez pas besoin de
  les lire, sauf éventuellement pour new_config() (qui est la seule fonction
  fournie dans ce fichier, les autres sont décrites dans misc.h).

- Vous pouvez modifier la fonction main() pour vos propres tests.

- Avant de soumettre sur la page Moodle, assurez-vous que :

  - Les instructions de débugage (printf par exemple) de vos fonctions ont été
    supprimées ou commentées.

  - Votre code compile, même s'il ne fonctionne pas normalement, sans erreur et
    avec le moins de warnings possible.

  CONSEILS:

  - Testez chaque fonction AU FUR ET À MESURE, progressivement, sans attendre la
    dernière minute pour compiler.

  - Afin de simplifier, on ne vous demande pas de libérer la mémoire allouée.
    Votre code pourra comporter des fuites mémoires (c'est le cas pour le code
    qui vous est proposé).

*/

#include "misc.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <float.h>
// Crée une configuration n x n où toutes les cases sont à zéro. Tous les champs
// sont initialisés. On supposera que n > 0 (ce n'est donc pas à la fonction de
// gérer cette condition, mais à l'appelant de s'en assurer).
//
// [VOUS N'AVEZ PAS À LA CODER]
//
config new_config(int n) {
    config R = malloc(sizeof(*R));
    R->n     = n;
    R->G     = malloc(n * sizeof(*(R->G)));
    for (int i = 0; i < n; i++)
        R->G[i] = calloc(n, sizeof(int));
    R->i0 = R->j0 = 0;
    R->cost       = 0;
    R->score      = -1; // pour affichage intelligent avec print_config()
    R->parent     = NULL;
    return R;
}

// Crée une copie de la configuration C, supposée non NULL. Vous pouvez
// utiliser la fonction new_config ci-dessus. Copiez bien tous les
// champs.
//
// Attention : dans la copie C_copy, il n'est pas suffisant d'affecter C->G à
// C_copy->G car on veut *copier* les cases du tableau.
//
// [COMPLÉTEZ CETTE FONCTION]
//
config copy_config(config C) {
    config R = malloc(sizeof(*R));
    int n = C->n;
    R->n     = n;
    R->G     = malloc(n * sizeof(*(C->G)));
    for (int i = 0; i < n; i++)
        R->G[i] = calloc(n, sizeof(int));
    // Copy actual values from C->G to R->G
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            R->G[i][j] = C->G[i][j];
    R->i0 = C->i0;
    R->j0 = C->j0;
    R->cost       = C->cost;
    R->score      = C->score;
    R->parent     = C->parent;
    return R;
}

// Trouve les coordonnées i,j de la case C->G[i][j] qui vaut 0,
// et affecte i = C->i0 et j = C->j0. On suppose C non NULL, et
// que C->G contient une unique valeur 0.
//
// [COMPLÉTEZ CETTE FONCTION]
//
void set_zero(config C) {
    for (int i = 0; i < C->n; i++) {
        for (int j = 0; j < C->n; j++) {
            if (C->G[i][j] == 0) {
                C->i0 = i;
                C->j0 = j;
                return;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Dans la suite, on parle de "voisine" d'une configuration. //
//                                                                            //
// On travaille dans le graphe suivant (auquel on appliquera A* dans solve())://
// - Les sommets sont les configurations.                                     //
// - Il y a une arête C->C' si on peut aller de C en C' par un mouvement.     //
//   On dit dans ce cas que C' est une configuration voisine de C.            //
////////////////////////////////////////////////////////////////////////////////
//
//
//

//
//
//
// Renvoie la configuration voisine de C où le déplacement m a été effectué. Si
// le déplacement n'est pas possible, la fonction renvoie NULL. Dans tous les
// cas, C n'est pas modifiée (une nouvelle configuration est renvoyée).
//
// On supposera C non NULL et m un des 4 caractères dans "hbgd".
//
// On rappelle qu'un déplacement m signifie par convention qu'on déplace une
// case voisine de la case vide VERS la case vide. Par exemple 'h' signifie
// qu'on va monter une case d'une ligne vers le haut (si c'est possible), ce qui
// va avoir pour effet de descendre le point (i0,j0) d'une ligne vers le bas.
//
// Vous pouvez utiliser copy_config() et SWAP(). Vous devez mettre à jour
// correctement (i0,j0), en plus de modifier des cases de C->G.
//
// [COMPLÉTEZ CETTE FONCTION]
//
config move(config C, char m) {
    config G = copy_config(C);
    set_zero(G);
    int z = 0;
    int i0 = G->i0;
    int j0 = G->j0;

    // 'h' (haut): move tile from below UP → empty moves DOWN
    if (m == 'h' && i0 != G->n - 1) {
        SWAP(G->G[i0][j0], G->G[i0 + 1][j0], z);
        G->i0 = i0 + 1;
    }
    // 'b' (bas): move tile from above DOWN → empty moves UP
    else if (m == 'b' && i0 != 0) {
        SWAP(G->G[i0][j0], G->G[i0 - 1][j0], z);
        G->i0 = i0 - 1;
    }
    // 'g' (gauche): move tile from right LEFT → empty moves RIGHT
    else if (m == 'g' && j0 != G->n - 1) {
        SWAP(G->G[i0][j0], G->G[i0][j0 + 1], z);
        G->j0 = j0 + 1;
    }
    // 'd' (droite): move tile from left RIGHT → empty moves LEFT
    else if (m == 'd' && j0 != 0) {
        SWAP(G->G[i0][j0], G->G[i0][j0 - 1], z);
        G->j0 = j0 - 1;
    }
    else {
        // Invalid move
        free(G->G);
        free(G);
        return NULL; 
    }

    return G;
}

// Fonction qui génère rapidement une *permutation* aléatoire P de
// {0,1,...,k-1}. Cette fonction est utilisée par la fonction create_random()
// dejà écrite pour vous, qui génère une *configuration* aléatoire.
//
// La fonction random_permutation() à écrire suppose que P pointe vers k cases
// de type int, déjà allouées : P[0],...,P[k-1]. Elle consiste à :
//
// 1. Initialiser P avec la permutation identité (P[i] = i pour tout i de
// {0,1,...,k-1}).
//
// 2. Pour chaque position i de P, en partant de la dernière (k-1) et jusqu'à
//    i = 1 (il n'y a rien à faire pour i=0), il faut :
//
//    2.1 Choisir un indice j aléatoire dans {0,1,...,i};
//    2.2 Echanger P[j] avec P[i].
//
// Pour générer un entier aléatoire, utilisez la fonction random(). Vous devrez
// ensuite utiliser un modulo pour restreindre l'intervalle de valeurs cibles.
//
// [COMPLÉTEZ CETTE FONCTION]
//
void random_permutation(int *P, int k) {
    // Initialize identity permutation
    for (int i = 0; i < k; i++) {
        P[i] = i;
    }

    // Fisher-Yates shuffle
    for (int i = k - 1; i > 0; i--) {
        int j = random() % (i + 1);
        int z;
        SWAP(P[j], P[i], z);
    }
}

// Fonction de comparaison de deux configurations supposées non NULL selon leur
// champ score (les valeurs des scores seront calculées plus tard dans A*).
//
// La fonction fcmp_config doit renvoyer :
//
// * -1 si le score de la configuration pointée par X est < à celui de la
//   configuration pointée par Y,
// * 1 si le score de la configuration pointée par X est > à celui de la
//   configuration pointée par Y,
// * 0 si les scores des deux configurations sont égales,
//
// [COMPLÉTEZ CETTE FONCTION]
//
int fcmp_config(const void *X, const void *Y) {
    config C_X = (config)X;
    config C_Y = (config)Y;
    if (C_X->score > C_Y->score) return 1;
    if (C_X->score < C_Y->score) return -1;
    return 0;
}

typedef int (*heuristic)(config);

// Heuristique donnant le nombre de cases mal placées sans compter la case vide.
// Bien sûr, ce nombre est nul si et seulement si X est la configuration
// gagnante. On peut montrer que cette heuristique est monotone.
//
// NB: Pour être à la bonne place dans la configuration gagnante, la case (i,j)
// devrait contenir la valeur v = j + i*n, où n est la taille d'un côté d'une
// configuration.
//
// [COMPLÉTEZ CETTE FONCTION]
//
int h1(config X) {
    int n = X->n;
    int h = 0;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int val = X->G[i][j];
            if (val != 0 && val != j + i * n) {
                h++;
            }
        }
    }

    return h;
}

// Heuristique donnant la somme des distances des cases, pour toute case de X
// sauf la case vide, à leurs positions dans la configuration gagnante.
//
// Les distances doivent être calculées selon le 4-voisinage de la grille G[][]
// (où les diagonales sont interdites). C'est aussi la distance de Manhattan
// qui, on le rappelle vaut: d((i,j),(i',j')) = |i'-i| + |j'-j|. Bien sûr, cette
// somme de distances est nulle si et seulement si X est la configuration
// gagnante. On peut montrer que cette heuristique est monotone.
//
// NB: Une case contenant la valeur v doit se retrouver, dans la configuration
// gagnante, à la position (i,j) telle que i = v/n et j = v%n.
//
// [COMPLÉTEZ CETTE FONCTION]
//


int h2(config X) {
    int somme = 0;
    int n = X->n;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int val = X->G[i][j];
            if (val != 0) {
                int gagnant_i = val / n;
                int gagnant_j = val % n;
                int h_val = abs(gagnant_i - i) + abs(gagnant_j - j);
                somme += h_val;
            }
        }
    }
    return somme;
}

// Heuristique similaire à h2 sauf que la distance pour une case de
// valeur v est multipliée par v*v. L'idée est de forcer à trouver des
// solutions consistant à placer d'abord les petits numéros, ce qui
// revient à pénaliser ceux qui sont grands. On obtient des résultats
// encore plus rapides en remplaçant le facteur v par v^2. Cette
// heuristique n'est pas monotone.
//
// [COMPLÉTEZ CETTE FONCTION POUR UN BONUS]
//
int h3(config X) {
    int somme = 0;
    int n = X->n;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int val = X->G[i][j];
            if (val != 0) {
                int gagnant_i = val / n;
                int gagnant_j = val % n;
                int h_val = abs(gagnant_i - i) + abs(gagnant_j - j);
                somme += h_val * (val * val);
            }
        }
    }
    return somme;
}

// Applique l'algorithme A* muni d'une heuristique h pour le calcul
// d'un chemin allant d'une configuration de départ S à la
// configuration gagnante.
//
// La fonction doit renvoyer la configuration C correspondant à la
// configuration gagnante, le chemin étant déterminé par la suite de
// configurations: C, C->parent, C->parent->parent, ..., S. Si le
// chemin n'existe pas la fonction devra renvoyer NULL.
//
// [COMPLÉTEZ CETTE FONCTION]
//
config solve(config S, heuristic h) {
    heap H = NULL;

    // Il est important d'éviter ce cas là sinon mémoire+temps
    // explosent
    if (!can_win(S))
        return NULL;

    // Crée un tas min pour fcmp_config() et ajouter la configuration
    // de départ. NB: L'implémentation du tas qui vous est donnée le
    // redimensionne dynamiquement si besoin est. Vous pouvez donc
    // l'initialiser avec une petite taille, comme 4 par exemple.
    H = heap_create(4, fcmp_config);

    if (H == NULL) {
        fprintf(stderr, "%s  Error: heap not allocated.\n", KO);
        exit(1);
    }

    // Initialiser configuration de départ
    S->cost = 0;
    S->score = h(S);
    S->parent = NULL;
    heap_add(H, S);

    // Boucle principale de A*
    while (!heap_empty(H)) {
        config C = heap_pop(H); // C = config de score min

        // Kiểm tra đã đến đích chưa (dùng heuristic parameter h)
        if (h(C) == 0)
            return C;

        // Bỏ qua nếu đã xử lý
        if (is_marked(C))
            continue;

        char m[] = "bdgh"; // les 4 déplacements potentiels
        for (int i = 0; i < 4; i++) {
            // Tạo neighbor bằng cách di chuyển
            config V = move(C, m[i]);

            // Chỉ thêm nếu di chuyển hợp lệ
            if (V != NULL) {
                // Cập nhật cost = cost của parent + 1
                V->cost = C->cost + 1;
                // Cập nhật score = cost + heuristic
                V->score = V->cost + h(V);
                // Lưu parent để truy ngược đường đi
                V->parent = C;
                // Thêm vào heap
                heap_add(H, V);
            }
        }
    }

    return NULL; // Không tìm thấy đường đi
}

//
//  USAGE: taquin [n]
//
int main(int argc, char *argv[]) {
    int n         = (argc >= 2) ? atoi(argv[1]) : 3;
    unsigned seed = time(NULL) % 1000;
    printf("seed: %u\n", seed); // pour rejouer la même chose au cas où
    srandom(seed);

    config C;

    // exemple du sujet
    int ex1[3][3] = {
        {3, 1, 2},
        {4, 7, 5},
        {6, 8, 0},
    };

    // solvable en 5 mouvements
    int ex2[2][2] = {
        {0, 2},
        {3, 1},
    };

    // solvable en 15 mouvements
    int ex3[3][3] = {
        {3, 1, 2},
        {8, 0, 6},
        {4, 5, 7},
    };

    // solvable en 29 mouvements
    int ex4[3][3] = {
        {5, 2, 6},
        {8, 1, 4},
        {7, 3, 0},
    };

    // solvable en 53 mouvements (utiliser impérativement h2)
    int ex5[4][4] = {
        {14, 4, 7, 9},
        {13, 0, 1, 12},
        {3, 11, 10, 2},
        {6, 8, 5, 15},
    };

    // Test lancé avec la configuration ex1.
    // Vous pouvez changer pour une autre.
    //int (*ex)[3] = ex1;
     int (*ex)[2] = ex2;
    // int (*ex)[3] = ex3;
    // int (*ex)[3] = ex4;
    // int (*ex)[4] = ex5;

    int n_ex     = sizeof(ex[0]) / sizeof(ex[0][0]); // taille d'une ligne
    C            = new_config(n_ex);                 // crée une nouvelle config
    for (int i = 0; i < n_ex; i++)
        C->G[i] = ex[i]; // copies les lignes
    set_zero(C);         // important !

    // test de move()
    printf("====== Test de move ======\n");
    print_config(C);
    print_config(move(C, 'g'));
    printf("\n");

    // test de walk() pour ex1
    printf("====== Test de walk ======\n");
    walk(C, "gh");
    printf("\n");

    // test grille n x n aléatoire
    printf("====== Test de create_random ======\n");
    C = create_random(n);
    printf("Starting config\n");
    print_config(C);
    printf("Can be solved? %s\n", can_win(C) ? "yes" : "no");
    printf("\n");

    // test heuristique
    heuristic h = h3;
    printf("====== Test d'une heuristique ======\n");
    printf("h: %d\n", h(C));
    printf("\n");

    // test solve()
    printf("====== Test de solve ======\n");
    C       = solve(C, h); // très long si n>3, utiliser h3

    // affiche le chemin de la config gagnante à la config initiale
    int cfg = 0; // pour compter le nombre de mouvements

    while (C) {
        print_config(C);
        printf("h: %d\n", h(C));
        C = C->parent;
        cfg++;
    }

    printf("\n");
    printf("#mouvements: %d\n", cfg);
    printf("#configs in htable: %u\n", HTN ? *HTN : 0);
    printf("\n");

    return 0;
}
