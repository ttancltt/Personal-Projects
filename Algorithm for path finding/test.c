#include <stdio.h>
#include <stdlib.h>
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

//
//  TSP - HEURISTIQUES
//
typedef struct {
  double x, y;
} point;

void reverse(int *T, int p, int q) {
  // Renverse la partie T[p]...T[q] du tableau T avec p<q si
  // T={0,1,2,3,4,5,6} et p=2 et q=5, alors le nouveau tableau T sera
  // {0,1, 5,4,3,2, 6}.
  while(p<q){
        int tmp=T[p];
        T[p]=T[q];
        T[q]=tmp;
        p++;
        q--;
    }
  
  
}


double first_flip(point *V, int n, int *P) {
  // Renvoie le gain>0 du premier flip rÃ©alisable, tout en rÃ©alisant
  // le flip, et 0 s'il n'y en a pas.
  ;
  ;
  ;
  return 0.0;
}

double tsp_flip(point *V, int n, int *P) {
  // La fonction doit renvoyer la valeur de la tournÃ©e obtenue. Pensez
  // Ã  initialiser P, par exemple Ã  P[i]=i. Pensez aussi faire
  // drawTour() pour visualiser chaque flip.
  ;
  ;
  ;
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
void print_array(int*T, int n){
    
    for (int i=0; i<n; i++){
        printf("%d, ", T[i]);
    } 
}

int main (){
    int T[]={0,1,2,3,4,5,6};
    reverse(T,2,5);
    print_array(T,7);
    return 1;
}