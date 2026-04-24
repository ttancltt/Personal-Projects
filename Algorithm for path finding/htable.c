#include "htable.h"
#include <stdlib.h>
#include <strings.h>


// Fonction permettant de calculer la position du bit de poids fort
// d'une valeur, les positions commençant à 1. Renvoie 0 si la valeur
// est nulle. C'est littéralement la même fonction que fls(), fonction
// hardware en temps constant qui existe sous Mac OS, mais pas
// forcément sous Linux. La déclaration en 'weak' permet d'utiliser la
// fonction d'origine si elle est disponible.
int __attribute__((weak)) fls(int value){
  if (value == 0) return 0;
  int p = 1;
  unsigned x = value;
  while( x != 1 ) x >>= 1, p++;
  return p;
}


inline void ht_reset(htable T){
    bzero(T->state,(T->mask+1)*sizeof(*T->state)); // reset complet
    T->n = 0; // plus aucune entrée
}


htable ht_create(void){
  htable T = malloc(sizeof(*T));
  T->key   = malloc(HT_INIT_SIZE*sizeof(*T->key));
  T->value = malloc(HT_INIT_SIZE*sizeof(*T->value));
  T->state = malloc(HT_INIT_SIZE*sizeof(*T->state));
  T->mask = HT_INIT_SIZE-1; // pour un modulo rapide
  T->shift = 8*sizeof(HT_MAGIC) - (fls(HT_INIT_SIZE)-1); // pour le hash
  ht_reset(T); // vide la table
  return T;
}


void ht_free(htable T){
  free(T->key);
  free(T->value);
  free(T->state);
  free(T);
}


enum{
  ST_FREE = 0, // l'entrée est libre, doit être = 0 pour bzero()
  ST_SET,      // l'entrée est occupé
  //ST_DEL,      // l'entrée a été supprimée
};


// Code d'opérations pour ht_lookup()
typedef enum{
  OP_READ,   // une lecture
  OP_WRITE,  // une écriture
  //OP_DELETE, // une suppression
} ht_opcode;


// Effectue une opération de lecture ou d'écriture dans la table T.
// L'opération est spécifiée par le paramètre op. Si op = OP_READ,
// alors la valeur associée à key est renvoyée (renvoie NULL si aucune
// entrée (key,value) n'a été écrite précédemment dans T). Si op =
// OP_WRITE, alors l'entrée (key,value) est écrite dans la table
// T. Pour l'écriture, l'ancienne valeur est retournée (renvoie NULL
// si l'entrée était libre).
void* ht_lookup(htable T, int key, void *value, ht_opcode op){

  void* v;

  // détermine l'index h de T où mettre l'entrée (key,value)
  unsigned h = (key * HT_MAGIC) >> T->shift; // multiplication uint64_t

  // CAS 1: l'entrée est libre
  // Il y a de bonne chance que cela se produise

  if(T->state[h] == ST_FREE)
    switch(op){
      
    case OP_READ: // lecture
      return NULL;
      
    case OP_WRITE: // écriture
      T->key[h] = key;
      T->value[h] = value;
      T->state[h] = ST_SET;
      T->n++; // une nouvelle entrée (key,value)
      return NULL; // ancienne valeur
      
    //case OP_DELETE: // suppression
    //  T->state[h] = ST_DEL; // NB: T->n ne change pas
    //  return NULL;

    }
  
  // CAS 2: l'entrée n'est pas libre
  // Il y a beaucoup de chance que cela soit la bonne clé

  if( key == T->key[h] )
    switch(op){
      
    case OP_READ: // lecture
      return T->value[h];
      
    case OP_WRITE:; // écriture (mise à jour)
      v = T->value[h]; // ancienne valeur à renvoyer
      T->value[h] = value; // NB: ici n et T->state sont corrects
      return v;

    //case OP_DELETE: // suppression, NB: T->n>0
    //  T->state[h] = ST_DEL;
    //  T->n--; // un élément de moins
    //  return T->value[h];
      
    }
  
  // CAS 3: il y a collision

  // Il faut donc chercher la clé dans la table grâce au sondage
  // (linéaire). On s'arrête à la 1ère entrée libre. Pour l'écriture,
  // il faut faire attention que la table ne soit pas pleine sinon la
  // recherche ne déminera pas.
  
  if( 2*T->n > T->mask ){ // si remplissage de la table est >= 50%
    const unsigned m = T->mask + 1; // taille de la table actuelle
    const unsigned m2 = m<<1;       // nouvelle taille (x2)

    // sauvegarde les données (entrées + états)
    int   *K = T->key;
    void* *V = T->value;
    char  *S = T->state;
    
    // nouvelle table 2 fois plus grande
    T->key   = malloc(m2*sizeof(*T->key));
    T->value = malloc(m2*sizeof(*T->value));
    T->state = malloc(m2*sizeof(*T->state));
    T->mask = m2-1; // nouveau mask
    ht_reset(T);    // reset complet de T->state et de T->n
    T->shift--;     // nouveau décalage pour le hash
    // ici T est la nouvelle table, 2 fois plus grande et vidée

    // re-hachage: réécrit l'ancienne table dans la nouvelle
    for(int i=0; i<m; i++) // parcoure les (key,value) de l'ancienne table
      if( S[i] == ST_SET ) ht_lookup(T,K[i],V[i],OP_WRITE);

    // libère les données de l'ancienne table
    free(K); free(V); free(S);

    // on recommence l'opération, nécessairement une écriture, pour
    // (key,value) car le hash courant h pour key n'est pas bon pour
    // la nouvelle table
    return ht_lookup(T,key,value,op);
  }

  // sondage linéaire pour trouver l'indice h d'une entrée libre, qui
  // exite forcément car ici la table n'est pas pleine (taux <= 50%)

  for(;;){ // boucle infinie qui va s'arrêter 

    h = (h+1)&T->mask; // entrée suivante (modulo la taille)
    switch(T->state[h]){ // dépend de l'état de h
      
    case ST_FREE: // entrée libre
      switch(op){
      case OP_READ: return NULL; // key n'est pas dans la table
      case OP_WRITE:
	v = T->value[h]; // ancienne valeur à renvoyer
	T->key[h] = key;
	T->value[h] = value;
	T->state[h] = ST_SET;
	T->n++; // une nouvelle entrée (key,value)
	return v;
      }
      
    case ST_SET: // entrée occupée
      if( key != T->key[h]) continue; // on a pas trouvé la clé
      // on a trouvé la clé
      switch(op){
      case OP_READ: return T->value[h];
      case OP_WRITE:
	v = T->value[h]; // ancienne valeur à renvoyer
	T->value[h] = value; // NB: key et state sont corrects
	T->n++; // une nouvelle entrée (key,value)
	return v;
      }
      
    }
  }
}


inline void* ht_read(htable T, int key){
  return ht_lookup(T, key, NULL, OP_READ);
}


inline void* ht_write(htable T, int key, void* value){
  return ht_lookup(T, key, value, OP_WRITE);
}


//inline void* ht_delete(htable T, int key){
//  return ht_lookup(T, key, NULL, OP_DELETE);
//}
