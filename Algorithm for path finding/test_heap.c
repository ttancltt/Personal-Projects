/* 
   test_heap.c
   
   Permet de tester les fonctions de heap.c en remplissant un tas avec
   des éléments aléatoires d'un type donné, puis de les extraire en
   vérifiant qu'ils apparaissent dans l'ordre croissant. Plusieurs
   types et fonctions de comparaisons peuvent être testés.

   NB: La fonction de comparaison double2(), qui consiste à renvoyer
   "x-y" converti en "int", produit un comportement intéressant qui
   est doublement erroné. D'abord "0.5-0.3" (qui devrait être un test
   strictement positif), est interprété comme "0.5-0.3"<=0 car
   (int)(0.5-0.3) = (int)(0.2) = 0. Et donc le programme pourra penser
   que le tableau final est effectivement trié ("success!") par ordre
   croissant, alors que c'est visiblement pas le cas. Mais plus grave
   encore, le programme peut afficher un échec ("fail!") alors que
   l'implémentation du tas est tout à fait correcte. Pourtant,
   "success!" et "fail!" sont déterminés d'après la même fonction de
   comparaison: pour la gestion du tas et pour la vérification finale
   du tri. Cela vient du fait que la fonction double2() n'est pas
   transitive. On peut très bien avoir "x-y"=0 et "y-z"=0 mais avoir
   "x-z">0, par exemple avec x=1.2, y=0.7 et z=0.2. Donc, même avec
   une implémentation correcte, une fonction de comparaison non
   transitive peut faire échouer un tri par tas.
*/

#include "heap.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define NMAX 40  // taille max pour l'affichage avec print_heap()
#define BAR "-"  // un tiret
#define MAXSTR 7 // taille max d'une string
#define xstr(s) str(s) // permet l'expansion d'une macro
#define str(s) #s      // Ex: scanf("%"xstr(DMAX)"s",buffer);

// char UTF8
#define FAIL  "\xf0\x9f\x94\xa5 fail!"
#define NOWAY "\xf0\x9f\x9a\xab"
#define HEART "\xe2\x9d\xa4\xef\xb8\x8f"
#define OK    "\xf0\x9f\x91\x8c"


typedef int (*fcmp)(const void*,const void*); // type fonction de comparaison
typedef char* string;                         // type chaîne de caractères
typedef struct{ double x,y; } point;          // type point du plan

////////////////////////////////////////////////////

int fcmp_int(const void *x, const void *y) {
  return *(int*)x - *(int*)y;
}

int fcmp_char(const void *x, const void *y) {
  return *(char*)x - *(char*)y;
}

int fcmp_double(const void *x, const void *y) {
  const double a = *(double*)x;
  const double b = *(double*)y;
  return (a<b)? -1 : (a>b); // ou encore return (a>b) - (a<b);
}

int fcmp_double2(const void *x, const void *y) {
  return *(double*)x - *(double*)y; // incorecte !
}

int fcmp_string(const void *x, const void *y) {
  return strcmp( *(string*)x, *(string*)y );
}

int fcmp_pointx(const void *p, const void *q) {
  return fcmp_double( &(((point*)p)->x), &(((point*)q)->x) );
}

////////////////////////////////////////////////////

enum{ // les types possibles, pour les switch()
  INT=0,
  CHAR,
  DOUBLE,
  DOUBLE2,
  STRING,
  POINT,
};

int size[] = { // tableau des tailles des types
  sizeof(int),
  sizeof(char),
  sizeof(double),
  sizeof(double),
  sizeof(string),
  sizeof(point),
};

int psize[] = { // tableau des tailles d'affichage des types
  2,
  1,
  5,
  5,
  MAXSTR,
  9,
};

fcmp cmp[] = { // tableau des fonctions de comparaison
  fcmp_int,
  fcmp_char,
  fcmp_double,
  fcmp_double2,
  fcmp_string,
  fcmp_pointx,
};

char *type[] = { // tableau des types pour l'aide
  "int",     "integers in [0,100[ (default)",
  "char",    "capital letters in ['A','Z'[",
  "double",  "real numbers in ]-1.00,+1.00[",
  "double2", "same as 'double' but with 'x-y' like fcmp()",
  "string",  "strings of length at most "xstr(MAXSTR),
  "point",   "real points of [0,9.9]×[0,9.9] with 'x' like fcmp()",
};


////////////////////////////////////////////////////


// affiche T[i] avec le bon format suivant le type t
// en ajoutant un espace " " terminal
void print(int t, void *T, int i){
  switch(t){
  case INT:     printf("%02i ",    ((int*)    T)[i]); break;
  case CHAR:    printf("%c ",      ((char*)   T)[i]); break;
  case DOUBLE:
  case DOUBLE2: printf("%+1.2lf ", ((double*) T)[i]); break;
  case STRING:  printf("%s ",      ((string*) T)[i]); break;
  case POINT:   printf("(%.1lf,%.1lf) ",((point*)T)[i].x, ((point*)T)[i].y); break;
  }
}

// initialisation aléatoire de l'élément T[i]
void init(int t, void *T, int i){
  switch(t){

  case INT:
    { // entiers aléatoires dans [0,100[
      ((int*)T)[i] = random()%100;
    } break;

  case CHAR:
    { // lettres majuscules aléatoires
      ((char*)T)[i] = 'A' + (random()%('Z'-'A'+1));
    } break;

  case DOUBLE:
  case DOUBLE2:
    { // double aléatoires dans [-1,+1] avec 2 chiffres
      ((double*)T)[i] = ((random()%2000)-1000) / 1000.0;
    } break;

  case STRING:
    { // construit une chaîne aléatoire d'au plus MAXSTR char
      const int k = 1 + random()%MAXSTR; // k=1..MAXSTR
      string s = calloc(k+1,sizeof(char)); // remplit de '\0'
      for(int i=0; i<k; i++) s[i] = 'a' + (random()%('z'-'a'+1));
      ((string*)T)[i] = s;
    } break;
    
  case POINT:
    { // point aléatoire de [0,10[ x [0,10[ avec 1 chiffre
      ((point*)T)[i].x = random()%100 / 10.0;
      ((point*)T)[i].y = random()%100 / 10.0;
    } break;
  }
}


////////////////////////////////////////////////////


// affiche n fois la chaîne s
void rule(int n,string s){
  for(int i=0; i<n; i++) printf("%s",s);
}

// affiche le contenu du tas h d'éléments de type t sous la forme d'un
// arbre, sauf si le nombre d'éléments est est trop grand (>NMAX)
void print_heap(heap h, int t) {

  rule(psize[t],BAR);
  printf("\n");

  if (h == NULL) {
    printf("heap = NULL, the heap does not exist\n\n");
    return;
  }
  
  int j=h->n;
  if( j > NMAX ){
    printf(NOWAY" heap to large to be displayed!\n");
    return;
  }

  int haut=0; // hauteur du tas (=nombre de lignes)
  while(j>0) haut++, j>>=1;

  // On suppose que le pattern est de taille impaire 2s+1 avec s =
  // floor(psize[t]/2). Si taille est paire on fait pareil, ce qui
  // revient à ajouter virtuellement un espace terminal. On pourrait
  // faire une meilleure présentation dans le cas pair (= plus
  // compacte notamment sur la dernière ligne), mais cela complexifie.
  //
  // Ex: pattern PPP de taille psize[t] = 3
  //
  // y=3|              PPP
  // y=3|       ┌───────┴───────┐
  // y=2|      PPP             PPP
  // y=2|   ┌───┴───┐       ┌───┴───┐
  // y=1|  PPP     PPP     PPP     PPP
  // y=1| ┌─┴─┐   ┌─┴─┐   ┌─┴─┐   ┌─┴─┐
  // y=0|PPP PPP PPP PPP PPP PPP PPP PPP
  //
  // Variables (les positions démarrent à 0):
  //
  //  pm = position du milieu (┴)
  //  pp = position du début du pattern
  //  pb = position du début de branche (┌)
  //  lb = longueur de branche (entre ┌ et ┴)
  //  eb = espace entre deux branches (entre ┐ et ┌)
  //  ep = espace entre deux patterns
  //
  // Evolution des variables, à partir du bas (y=0,1,2,...):
  //
  //      y=0  y=1  y=2
  //
  //  pm = s   2s+1 4s+3 ... => pm(y) = 2*pm(y-1)+1 = (s+1)*2^y - 1
  //  lb = .   s    2s+1 ... => lb(y) = pm(y-1)
  //  pb = .   s    2s+1 ... => pb(y) = lb(y)
  //  pp = 0   s+1  3s+3 ... => pp(y) = pm(y)-s
  //  eb = .   2s+1 4s+3 ... => eb(y) = 2*eb(y-1)+1 = pm(y)
  //  ep = 1   ...  ...      => ep(y) = 2*lb(y+1)+1-2s = 2*pm(y)+1-2s

  int s1,s2;
  int s=psize[t]/2;  // nombre de caractères avant le milieu du pattern
  int B=1;           // B = nombre de patterns dans la ligne
  j=0;               // j = nombre d'éléments déjà affichés

  for(int y=haut-1; y>=0; y--, B<<=1){ // on part du haut

    int pm=(s+1)*(1<<y)-1;
    int lb=(s+1)*(1<<(y-1))-1;
    int ep=2*pm+1-2*s;

    // ligne avec les éléments
    rule(pm-s," ");
    for(int b=0; b<B && (j<h->n); b++){
      j++;
      if(psize[t]%2==0) printf(" "); // cas pair
      if(t==STRING){ // centrage si string
	s1=strlen(*(string*)h->array[j]);
	s2=(psize[t]-s1)/2;
	s1=psize[t]-s2-s1;
	rule(s1," "); // [s1 string s2]
      }
      print(t, h->array[j], 0);
      if(t==STRING) rule(s2," "); // centrage si string
      if(b<B-1) rule(ep-1," "); // -1 pour l'espace terminal
    }
    printf("\n");
    if(y==0) break; // fini si dernière ligne d'éléments

    // ligne avec les branchements
    rule(lb," ");
    for(int b=0; b<B; b++){
      printf("┌"); rule(lb,"─"); printf("┴"); rule(lb,"─"); printf("┐");
      if(b<B-1) rule(pm," ");
    }
    printf("\n");
    
  }
  
  rule(psize[t],BAR);
  printf("\n\n");
}


////////////////////////////////////////////////////


// NB: S'il y a des fflush(stdout) un peu partout, c'est qu'en cas
// d'erreur ("segmentation fault" par exemple) il arrive que le
// printf() précédant l'erreur n'a pas le temps d'être affiché (et
// donc ne s'affiche pas). Donc généralement les fflush() ne servent à
// rien, sauf si le programme est sujet à des erreurs.

int main(int argc, char *argv[]) {

  int i, t, r;
  int ntype = sizeof(cmp)/sizeof(*cmp);     // nombre de types possibles
  int n = (argc>=2)? atoi(argv[1]) : -1;    // n = nombre d'éléments
  char *s = (argc>=3)? argv[2] : type[INT]; // t = type des éléments

  // double x=1.2, y=0.7, z=0.2;
  // printf("%g %g -> %d\n", x, y, fcmp_double2(&x,&y)); // 0
  // printf("%g %g -> %d\n", y, z, fcmp_double2(&y,&z)); // 0
  // printf("%g %g -> %d\n", x, z, fcmp_double2(&x,&z)); // 1
  // exit(1);

  for(t=0; t<ntype; t++) if(!strcmp(s,type[2*t])) break; // type trouvé !
  if( n<0 || t==ntype ){ // erreur
    printf("\n Usage: %s n [t]",argv[0]);
    printf("\n   Ex.: %s 10 int\n\n",argv[0]);
    printf("    n = number of random elements\n");
    printf("    t = type of elements & fcmp():\n\n");
    for(t=0; t<ntype; t++){
      printf("        '%s' ",type[2*t]);
      rule(12-strlen(type[2*t]),".");
      printf(" %s\n",type[2*t+1]);
    }
    printf("\n");
    exit(1);
  }

  unsigned seed = time(NULL) % 1000;
  srandom(seed);
  printf("\nseed: %u\n", seed); // pour rejouer la même chose au cas où

  void *T = malloc(n*size[t]); // tableau initial
  void *S = malloc(n*size[t]); // tableau final supposé trié

  printf("test heap with %i elements of type '%s'\n",n,type[2*t]);
  printf("(%s)\n",type[2*t+1]);

  // affichage et création de T[]
  printf("input array: ");
  for (i = 0; i<n; i++) {
    init(t, T, i);
    print(t, T, i);
  }
  printf("\n");

  // crée le tas avec la bonne fonction
  printf("heap creation... ");
  fflush(stdout);
  heap h = heap_create(n, cmp[t]);
  if ( h == NULL ){
    printf(FAIL" the returned heap is NULL\n\n");
    fflush(stdout);
    goto fin;
  }
  printf("ok\n");
  fflush(stdout);

  // teste que la fonction de comparaison soit correcte
  printf("heap comparison function? ");
  fflush(stdout);
  if (h->f != cmp[t]){
    printf(FAIL" wrong function\n\n");
    fflush(stdout);
    goto fin;
  }
  printf("correct\n");
  fflush(stdout);

  // teste que le tas soit bien vide au départ
  printf("heap empty at the start? ");
  fflush(stdout);
  if ( !heap_empty(h) ){
    printf(FAIL" the heap should be empty at the start\n\n");
    fflush(stdout);
    goto fin;
  }
  printf("yes\n");
  fflush(stdout);

  // teste que le nombre max d'éléments soit correct
  printf("heap size initialization? ");
  fflush(stdout);
  if (h->nmax != n){
    printf(FAIL" heap size is %d whereas it should be %d\n\n",
	   h->nmax, n);
    fflush(stdout);
    goto fin;
  }
  printf("correct\n");
  fflush(stdout);

  // pour les tests de lecture/écriture dans la zone allouée on fait
  // une petite pause pour permettre de voir où le 'segmentation
  // fault' se produit, s'il se produit. Malheureusement passer le
  // test ne signifie pas que les malloc() dans heap_create() étaient
  // corrects. Ces tests pourraient peut-être affinés avec l'usage de
  // mmap() ou mprotect().

  // test lecture
  printf("reading test in the heap array[1..%d]... ",n);
  printf("\E[?25l"); // echo off
  fflush(stdout);
  for(i=1; i<=n; i++){
    int k = printf("%d",i);
    printf("\E[%dD",k); // retour de k colonnes à gauche
    fflush(stdout);
    usleep(1000000/n); // pause d'une seconde au total
    void *w = h->array[i]; // lecture
    (void)w; // pour éviter le warning 'w not used'
  }
  printf("\E[?25h"); // echo on
  printf("ok\E[K\n");
  fflush(stdout);

  // test écriture
  printf("writing test in the heap array[1..%d]... ",n);
  printf("\E[?25l"); // echo off
  fflush(stdout);
  for(i=1; i<=n; i++){
    int k = printf("%d",i);
    printf("\E[%dD",k); // retour de k colonnes à gauche
    fflush(stdout); // écriture
    usleep(1000000/n); // pause d'une seconde au total
    h->array[i] = (void*)main;
  }
  printf("\E[?25h"); // echo on
  printf("ok\E[K\n");
  fflush(stdout);

  // déplace les éléments de T[] vers h avec des add()
  printf("\ntesting insertions:\n\n");
  fflush(stdout);
  for (i = 0; i<n; i++) {
    printf("insert ");
    print(t, T, i);
    printf("\n");
    if (heap_add(h, T+i*size[t])) { // T+i*size[t] = &(T[i])
      printf(FAIL" insert aborted after %d items (instead of %d)\n",
	     i, n);
      fflush(stdout);
      break;
    }
    if (heap_empty(h)) {
      printf(FAIL" the heap cannot be empty after insertion");
      fflush(stdout);
    }
    print_heap(h, t);
  }
  
  // déplace les i éléments insérés de h vers S[] avec des pop()
  // et vérification que top() retourne bien le même élément
  printf("testing suppressions:\n\n");
  fflush(stdout);
  for (int j=0; j<i; j++) {
    void *x = heap_top(h);
    if ( x == NULL ){
      printf(FAIL" the heap looks empty after poping %d item(s)\n", j+1);
      fflush(stdout);
      break;
    }
    void *y = heap_pop(h);
    memcpy(S+j*size[t],y,size[t]); // S[j] = heap_pop(h)
    printf("pop ");
    print(t, S, j);
    printf("\n");
    if ( y != x ) {
      printf(FAIL" heap_top() and heap_pop() differ at the %d-th item\n", j+1);
      fflush(stdout);
    }
    print_heap(h, t);
  }

  // teste que le tas soit bien vide
  printf("heap empty? ");
  fflush(stdout);
  printf(heap_empty(h)? "yes it is\n" : FAIL" it should be empty\n");
  fflush(stdout);

  // teste qu'on ne peut pas faire heap_top() sur un tas vide
  printf("reading top element from empty heap? ");
  fflush(stdout);
  printf((heap_top(h)==NULL)? "NULL => correct!\n" : FAIL" a top element founded in an empty heap!\n");
  fflush(stdout);

  // teste qu'on ne peut ni faire heap_pop() sur un tas vide
  printf("poping element from empty heap? ");
  fflush(stdout);
  printf((heap_pop(h)==NULL)? "NULL => correct!\n" : FAIL" a top element removed from an empty heap!\n");
  fflush(stdout);

  // libère le tas
  printf("heap destruction... ");
  fflush(stdout);
  heap_destroy(h);
  printf("ok\n");
  fflush(stdout);

  // affichage du résultat S[] et vérifie qu'il est trié
  // affiche "... x [>] y ..." si x>y (pas le bon ordre)
  printf("output array: ");
  for (i=0, r=1; i<n; i++){
    if(i>0){ // seulement à partir du 2e élément
      int c = cmp[t](S+(i-1)*size[t], S+i*size[t]); // S[i-1] <= S[i] ?
      if( c>0 ){ r=0; printf("[>] "); } // y'a un problème
    }
    print(t, S, i); // affiche S[i]
  }
  printf("\n%s the above array is %ssorted\n",
	 r? "success!"OK : FAIL, r?"":"not ");
  fflush(stdout);

  // teste que S[] et T[] on bien les mêmes éléments
  // pour cela, on trie S et T avec qsort(), puis on teste l'égalité
  // des éléments avec cmp[t](x,y) == 0
  printf("final test: is output array a permutation of the input? ");
  qsort(S, n, size[t], cmp[t]);
  qsort(T, n, size[t], cmp[t]);
  for (i=r=0; i<n; i++) // r = compte les valeurs égales
    r += (cmp[t](S+i*size[t], T+i*size[t]) == 0);
  printf((r==n)? "\nthis is a total success! "HEART"\n\n" : "\n"FAIL" output differ from input!\n\n");
  fflush(stdout);
  
 fin:;
  // libération de la mémoire
  if(t==STRING) // cas d'un tableau de string
    for(i=0; i<n; i++)
      free(((string*)T)[i]); // inutile pour S car mêmes éléments que T
  free(T);
  free(S);

  return 0;
}
