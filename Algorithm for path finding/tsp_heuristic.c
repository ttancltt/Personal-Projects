#include "tools.h"
#include "tsp_brute_force.h"

//
//  TSP - HEURISTIQUES
//

void reverse(int *T, int p, int q) {
  // Renverse la partie T[p]...T[q] du tableau T avec p<q si
  // T={0,1,2,3,4,5,6} et p=2 et q=5, alors le nouveau tableau T sera
  // {0,1, 5,4,3,2, 6}.
  for (int i=p,j=q; i<j;i++,j--){
    int tmp=T[i];
        T[i]=T[j];
        T[j]=tmp;
  }
}

double first_flip(point *V, int n, int *P) {
  // Renvoie le gain>0 du premier flip rÃ©alisable, tout en rÃ©alisant
  // le flip, et 0 s'il n'y en a pas.
  
  
  for(int i=0;i<(n-1);i++){
    for(int j=i+2;j<n;j++){
    double new=dist(V[P[i]],V[P[i+1]])+ dist(V[P[j]],V[P[(j+1)%n]]);

    double old=dist(V[P[i]],V[P[j]]) + dist(V[P[i+1]],V[P[(j+1)%n]]) ;
        if (old<new) {
        reverse(P,i+1,j);
        return old-new;
        }         
    }
    }
    return 0.0;
}  

double tsp_flip(point *V, int n, int *P) {
  // La fonction doit renvoyer la valeur de la tournÃ©e obtenue. Pensez
  // Ã  initialiser P, par exemple Ã  P[i]=i. Pensez aussi faire
  // ) pour visualiser chaque flip.
  for(int i=0;i<n;i++) P[i]=i;

  
  while (first_flip(V,n,P)>0.0){
    drawTour(V, n, P);
  }

  return 0.0;
}

double tsp_greedy(point *V, int n, int *P) {
  // La fonction doit renvoyer la valeur de la tournÃ©e obtenue. Pensez
  // Ã  initialiser P, par exemple Ã  P[i]=i.
  ;
  ;
  ;
  return 0.0;
}
