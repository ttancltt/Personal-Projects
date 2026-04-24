#ifndef __HTABLE_H__
#define __HTABLE_H__

//////////////////////////////////////////////////////////////////////
//
// Table de hachage dynamique pour des entrées (key,value) de type
// (int,void*). Les opérations autorisées sont:
//
// - ht_create/ht_free: créer ou détruire une table de hachage
// - ht_write: T[key] = value, ajouter (key,value) à la table T 
// - ht_read: return T[key], lire dans T la value correspondant à key
// - ht_reset: supprime toutes les entrées de T, sans détruire T
//
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
//
// Aspects techniques.
//
// La taille de la table est de la forme 2^k, avec k entier > 0. Pour
// distribuer key sur une entrée de la table, disons h, on utilise le
// hash de Fibonacci, les k bits de poids forts d'une grande
// multiplication: h = (key * HT_MAGIC) >> (64-k) où HT_MAGIC est un
// entier sur 64 bits lié au nombre d'or (voir plus loin). La gestion
// des collisions se fait par un adressage ouvert, via un sondage
// linéaire: on prend la première entrée libre suivante (modulo 2^k).
// La table est dynamique et croît par doublement dès que son taux de
// remplissage dépasse 50% lors d'une collision. Il faut alors
// re-hacher l'ancienne table dans la nouvelle. On pourrait passer ce
// taux à 100%, pour consommer moins d'espace, mais cela augmenterait
// significativement le taux de collisions.
//
// Lectures et écritures peuvent prendre, dans le pire des cas, un
// temps linéaire en la taille de la table (2^k) à cause: (1) du
// sondage lors des collisions (mais c'est constant avec un hachage
// uniforme des clés -- en fait, pour un taux de remplissage t<1,
// c'est 1/(1-t) pour un échec et (1/t)*ln(1/(1-t)) pour un succès);
// (2) de la recopie possible et de l'initialisation de la table lors
// du doublement (mais c'est constant en moyenne car amorti sur le
// nombre d'insertions).
//
//////////////////////////////////////////////////////////////////////


typedef struct{
  int *key;       // liste des clés de la table
  void* *value;   // liste de valeurs de la table
  char *state;    // liste des états de chaque entrée (key,value)
  unsigned n;     // nombre d'entrées (key,value) présentes dans la table
  unsigned mask;  // mot binaire de la forme 2^k-1 pour un modulo rapide
  unsigned shift; // décalage pour le calcul rapide du hash
} *htable;


// Constante multiplicative pour le calcul du hash (soit l'indice h de
// key dans la table). Il s'agit du hash de Fibonacci. Cette constante
// correspond au nombre impair le plus proche de 2^64 / φ, où φ =
// (1+√5)/2 est le nombre d'or. Elle est sur 64 bits, de type
// "unsigned long long" (ou uint64_t), d'où le suffixe "llu".
#define HT_MAGIC 11400714819323198485llu


// Taille d'initialisation de la table. Elle doit impérativement être
// une puissance de 2. Elle influence directement le temps de
// ht_create() et ht_reset().
#define HT_INIT_SIZE 4


// Crée une table de hachage. Cette création nécessite une
// initialisation en temps O(HT_INIT_SIZE) = O(1).
htable ht_create(void);


// Supprime la table de hachage créer par ht_create(). Les objets
// insérés dans la table (value) ne sont pas désalloués.
void ht_free(htable T);


// Ajoute de (key,value) dans la table T en écrasant l'ancienne valeur
// (si elle existait). Renvoie la valeur écrasée ou bien NULL si key
// n'était pas dans la table.
void* ht_write(htable T, int key, void* value);


// Lit la valeur correspondant à key dans T. Renvoie NULL si key n'est
// pas dans la table.
void* ht_read(htable T, int key);


// Supprime toutes les entrées de la table sans modifier la taille de
// la table. Cela permet de réutiliser la même table en évitant la
// réallocation dynamique et le re-hachage. Prend un temps
// proportionnel à la taille de la table, pas au nombre d'entrées
// quelle contient.
void ht_reset(htable T);


#endif

