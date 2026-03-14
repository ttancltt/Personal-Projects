#include "tools.h"
#include <math.h>
#include <float.h>
#include <stdbool.h>
#include <string.h>

// Question 1 : Distance euclidienne
double dist(point A, point B) {
    double dx = A.x - B.x;
    double dy = A.y - B.y;
    return sqrt(dx * dx + dy * dy);
}

// Question 2 : Longueur d'une tournée
double value(point *V, int n, int *P) {
    double total = 0;
    for (int i = 0; i < n; i++) {
        // Distance entre le point courant et le suivant (avec retour au début)
        total += dist(V[P[i]], V[P[(i + 1) % n]]);
    }
    return total;
}

// Question 3 : TSP Brute-Force classique
double tsp_brute_force(point *V, int n, int *Q) {
    int P[n];
    for (int i = 0; i < n; i++) P[i] = i; // Permutation initiale : 0, 1, 2...

    double min_dist = DBL_MAX;

    do {
        double d = value(V, n, P);
        if (d < min_dist) {
            min_dist = d;
            memcpy(Q, P, n * sizeof(int)); // Sauvegarde la meilleure permutation
        }
        
        // Optionnel : Animation SDL
        // drawTour(V, n, P);
    } while (NextPermutation(P, n) && running);

    return min_dist;
}

// Question 4 : L'observation suppose-t-elle l'inégalité triangulaire ?
/* 
   Réponse : OUI. 
   L'optimisation qui consiste à dire que si la distance partielle (incluant le retour au point de départ) 
   dépasse le record actuel 'w', alors toute complétion de cette tournée sera aussi plus grande que 'w', 
   repose sur l'inégalité triangulaire : dist(v_i, v_0) <= dist(v_i, v_{i+1}) + ... + dist(v_{n-1}, v_0).
   Si cette règle n'est pas respectée, un détour pourrait théoriquement "raccourcir" le trajet.
*/

// Question 5 : Calcul de valeur optimisé avec arrêt précoce
double value_opt(point *V, int n, int *P, double w) {
    double current_dist = 0;
    for (int i = 0; i < n - 1; i++) {
        current_dist += dist(V[P[i]], V[P[i + 1]]);
        
        // On ajoute la distance de retour au point de départ P[0] pour tester
        if (current_dist + dist(V[P[i+1]], V[P[0]]) >= w) {
            return -(i + 2); // Retourne l'indice k du préfixe fautif (négatif)
        }
    }
    // Si on arrive ici, on ajoute le dernier segment de retour
    current_dist += dist(V[P[n - 1]], V[P[0]]);
    return (current_dist < w) ? current_dist : -n;
}

// Question 6 : MaxPermutation
// Transforme P en la plus grande permutation (ordre lexicographique) ayant le même préfixe de taille k.
// Pour cela, il suffit de trier le suffixe (de k à n-1) par ordre décroissant.
void MaxPermutation(int *P, int n, int k) {
    if (k >= n) return;
    // On inverse le suffixe pour le mettre en ordre décroissant
    int i = k, j = n - 1;
    while (i < j) {
        int temp = P[i];
        P[i] = P[j];
        P[j] = temp;
        // Note: Ici, on suppose que le suffixe était croissant (plus petite perm).
        // On le trie simplement en décroissant pour obtenir la plus grande.
        i++; j--;
    }
    // Complexité : O(n-k), donc O(n).
}

// Question 6 (suite) : TSP Brute-Force Optimisé
double tsp_brute_force_opt(point *V, int n, int *Q) {
    int P[n];
    for (int i = 0; i < n; i++) P[i] = i;

    double min_dist = DBL_MAX;

    // On fixe le premier point (P[0]) pour réduire la recherche par un facteur n.
    // On ne permute que les n-1 derniers éléments.
    while (running) {
        double d = value_opt(V, n, P, min_dist);

        if (d >= 0) { // On a trouvé une meilleure tournée complète
            min_dist = d;
            memcpy(Q, P, n * sizeof(int));
        } else {
            // d est négatif, sa valeur absolue est l'indice k où ça a dépassé
            int k = (int)(-d);
            if (k < n) {
                MaxPermutation(P, n, k); // On saute les permutations inutiles
            }
        }

        // On passe à la permutation suivante de P[1...n-1]
        if (!NextPermutation(P + 1, n - 1)) break;
    }

    return min_dist;
}