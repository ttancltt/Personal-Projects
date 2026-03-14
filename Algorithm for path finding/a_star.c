#include "tools.h"
#include "heap.h" // il faut aussi votre code pour heap.c
static double diagonal_penalty = 0.8; 

// Une fonction de type "heuristic" est une fonction h() qui renvoie
// une distance (double) entre une position de départ et une position
// de fin de la grille. La fonction pourrait aussi dépendre de la
// grille (comme par exemple du nombre de murs rencontrés par le
// segment départ-fin), mais on n'utilisera pas forcément ce dernier
// paramètre. Vous pouvez définir et tester votre propre heuristique.
typedef double (*heuristic)(position,position,grid*);


// Heuristique "nulle" pour Dijkstra.
double h0(position s, position t, grid *G){
  return 0.0;
}


// Heuristique "vol d'oiseau" pour A*.
double hvo(position s, position t, grid *G){
  return fmax(abs(t.x-s.x),abs(t.y-s.y));
}


// Heuristique "alpha x vol d'oiseau" pour A*.
static double alpha=0; // 0 = h0, 1 = hvo, 2 = approximation ...
double halpha(position s, position t, grid *G) {
  return alpha*hvo(s,t,G);
}


// Structure "noeud" pour le tas min Q.
typedef struct _node {
  position pos;        // position (.x,.y) d'un noeud u
  double cost;         // coût[u] dist giua diem goc va current
  double score;        // score[u] = coût[u] + h(u,end)
  struct _node* parent;// parent[u] = pointeur vers le père, NULL pour start
} *node;


// Les arêtes, connectant les 8 cases voisines de la grille, sont
// valuées seulement par certaines valeurs correspondant aux
// différentes textures possibles des cases (il y en a 7). Le poids de
// l'arête u->v, noté ω(u,v) dans le cours, entre deux cases u et v
// voisines est déterminé par la texture de la case finale v. Plus
// précisément, si la case v de la grille est de texture t, le poids
// de u->v vaudra ω(u,v) = weight[t] dont les valeurs numériques
// exactes sont définies ci-après. Notez bien que t est un indice
// ('int'), alors que weight[t] est un coût ('double').



int compare_nodes(const void* a, const void* b) {
    node x = (node)a;
    node y = (node)b;
    if (x->score < y->score) return -1; // Score nhỏ hơn thì ưu tiên cao hơn
    if (x->score > y->score) return 1;
    
    // Cải tiến (1): Nếu score bằng nhau, ưu tiên node có cost lớn hơn 
    // (để giảm số lượng node phải duyệt - tùy chiến thuật)
  
    return 0;
}


// La liste des textutres possibles d'une case est donnée dans
// "tools.h": TX_FREE, TX_WALL, TX_WATER, ... Remarquez que
// weight[TX_WALL]<0 ce qui n'est pas a priori une valuation correcte.
// En effet A* ne marche qu'avec des poids positifs ! Mais ce n'est
// pas un problème, puisqu'en position (x,y), si G.texture[x][y] =
// TX_WALL, alors c'est que le sommet à cette position n'existe pas
// dans le graphe ! Et donc aucune arête ne peut donc être incidente à
// (x,y).

double weight[]={
    1.0,  // TX_FREE
    -99,  // TX_WALL
    1.0,  // TX_SAND
    9.0,  // TX_WATER
    2.3,  // TX_MUD
    1.5,  // TX_GRASS
    0.1,  // TX_TUNNEL
    2.0,  // TX_GRAVEL
    3.8,  // TX_ROCK
    6.0,  // TX_SNOW
    8.0,  // TX_ICE
    9.1,  // TX_LAKE
    9.2,  // TX_POOL
    1.4,  // TX_MEADOW
    1.6,  // TX_FOREST
    1.1,  // TX_TRACK
};




// Que doit faire la fonction A_star(G,h) ?
//------------------------------------------
//
// Votre fonction A_star(G,h) doit construire un chemin dans la grille
// G, entre la position G.start (s) et G.end (t), selon l'heuristique
// h(). Le chemin doit être calculé selon l'algorithme A* vu en cours
// (utilisez les notes de cours !). L'heuristique h() est une fonction
// à choisir lors de l'appel dans le main() parmi h0(), hvo(),
// halpha(). Vous pouvez aussi définir votre propre heuristique et la
// tester.
//
//
// Que doit renvoyer la fonction A_star(G,h) ?
//---------------------------------------------
//
// Si le chemin n'a pas été trouvé (par exemple si la destination est
// enfermée entre 4 murs ou si G.end est sur un mur), il faut renvoyer
// une valeur < 0.
//
// Sinon, il faut renvoyer le coût du chemin trouvé et remplir le
// champs .mark de G pour que le chemin trouvé puisse être visualisé
// par drawGrid(G) (plutard dans le main). Il faut, par convention,
// poser G.mark[x][y] = MK_PATH ssi la case (x,y) appartient au chemin
// trouvé. Pour que la visualisation soit complète, faite attention à
// ce que G.end et G.start soient bien marqués comme appartenant au
// chemin.
//
// Utilisez les touches a,z,+,-,p,c pour gérer la vitesse d'affichage
// et de progression de l'algorithme par exemple. Repportez-vous à
// "tools.h" pour avoir la liste des differentes touches et leurs
// actions, ainsi que les différents marquages possibles G.mark[x][y]
// pour une case (x,y).
//
//
// Comment gérer les ensembles P et Q ?
//--------------------------------------
//
// Pour gérer l'ensemble P, servez-vous du champs G.mark[x][y]. On
// aura G.mark[x][y] = MK_USED ssi le noeud en position (x,y) est dans
// P. Par défaut, ce champs est automatiquement initialisé partout à
// MK_NULL par toute fonction d'initialisation de la grille
// (initGridXXX()).
//
// Pour gérer l'ensemble Q, vous devez utiliser un tas min de noeuds
// (type node) avec une fonction de comparaison (à créer) qui dépend
// du champs .score des noeuds. Pour la fonction de comparaison
// inspirez vous de test_heap.c et faites attention au fait que
// l'expression '2.1 - 2.2' une fois castée en 'int' n'est pas
// négative, mais nulle !
//
// Vous devez utilisez la gestion paresseuse du tas (cf. le paragraphe
// du cours à ce sujet, dans l'implémentation de Dijkstra). Pensez
// qu'avec cette gestion paresseuse, la taille de Q est au plus la
// somme des degrés des sommets dans la grille. On peut être plus fin
// en remarquant que tout sommet, sauf s, aura toujours un voisin au
// moins déjà dans P (son parent). Pour visualiser un noeud de
// coordonnées (x,y) qui passe dans le tas Q vous pourrez mettre
// G.mark[x][y] = MK_FRONT au moment où vous l'ajoutez.
//
// Attention ! Même si cela est tentant, il ne faut pas utiliser la
// marque MK_FRONT pour savoir si un sommet (x,y) se trouve déjà dans
// Q. Cela ne sert pas à grand chose, car s'il est dans Q, vous ne
// pourrez pas déplacer le noeud correspondant pour le mettre au bon
// endroit dans Q en fonction de la mise à jour de son score.

node create_node(position pos, double cost, double score, node parent){
  node s = malloc(sizeof(*s));
  s->pos = pos;
  s->cost = cost;
  s->score = score;
  s->parent = parent;
  return s;
}
//NOTE quan trong
//Q la mot tas min, co nghia la cay con nho hon root
//P la G.mark
//continue de thoat khoi loop,
//dung chat gpt voi thienan28092003

double A_star(grid G, heuristic h){

  position start = G.start;
  position end   = G.end;

  if (G.texture[end.x][end.y] == TX_WALL)
    return -1;

  heap Q = heap_create(8*G.X * G.Y, compare_nodes);

  node s = create_node(start, 0., h(start,end,&G),NULL);

  heap_add(Q, s);
  G.mark[start.x][start.y] = MK_FRONT;

  node current = NULL;
  double terrain_penalty[16][16];
  for(int i=0;i<16;i++){
    for(int j=0;j<16;j++){
        terrain_penalty[i][j] = fabs(weight[j] - weight[i]) * 0.2;
    }
  }


  while (!heap_empty(Q)){

    current = heap_pop(Q);
//    if (current == NULL) break;

    int x = current->pos.x;
    int y = current->pos.y;

    if (x == end.x && y == end.y)
      break;

    if (G.mark[x][y] == MK_USED)
      continue;

    G.mark[x][y] = MK_USED;
    
    drawGrid(G);
    //Optimisation ici en reduisant le trajeet en diagonal 
    for (int dy = -1; dy <= 1; dy++){
      for (int dx = -1; dx <= 1; dx++){

        int nx = x + dx;
        int ny = y + dy;
        
        int from = G.texture[x][y]; //terrain case actuelle
        int to   = G.texture[nx][ny];  //terrain neighbor

        double terrain_pen = 0;
        terrain_pen= terrain_penalty[from][to];
        

        if (dx == 0 && dy == 0) //case au milieu n'est pas a calculer
          continue;

        if (G.texture[nx][ny] == TX_WALL) //no wall 
          continue;

        if (G.mark[nx][ny] == MK_USED) //used no calcul
          continue;
        double pen=(abs(dx)+abs(dy)==2)? diagonal_penalty: 0.0; 
        
        
        double new_cost =
          current->cost + weight[G.texture[nx][ny]];

        node v = create_node((position){nx,ny},
                              new_cost, 
                              new_cost + h((position){nx,ny},end,&G)*1.5+pen+terrain_pen,
                              current);

        heap_add(Q, v);

        if (G.mark[nx][ny] == MK_NULL)
          G.mark[nx][ny] = MK_FRONT;
      }
    }
  }

  if (current == NULL ||
      current->pos.x != end.x ||
      current->pos.y != end.y)
    return -1;

  double total_cost = current->cost;

  node p = current;
  while (p != NULL){
    G.mark[p->pos.x][p->pos.y] = MK_PATH;
    //printf("Tracking of parents position x:%d and y:%d",p->pos.x,p->pos.y);
    p = p->parent;
  }

  drawGrid(G);
  heap_destroy(Q);

  return total_cost;
}
 
  //Duyet ca grid
  
  // Après avoir extrait un noeud de Q, il ne faut pas le détruire
  // (free), sous peine de ne plus pouvoir reconstruire le chemin
  // trouvé ! Une fonction createNode() peut simplifier votre code.

  // Les bords de la grille sont toujours constitués de murs (texture
  // TX_WALL) ce qui évite d'avoir à tester la validité des indices
  // des positions (sentinelle). Dit autrement, un chemin ne peut pas
  // s'échapper de la grille.

  // Lorsque que vous ajoutez un élément au tas, pensez à tester la
  // valeur de retour heap_add() afin de détecter d'éventuellement
  // dépassement de capacité du tas, situation qui n'est pas censé
  // arriver. Si cela se produit, l'erreur vient soit du tas qui n'est
  // pas assez grand à son initialisation ou que l'algorithme visite
  // anormalement trop de sommets. De même, lorsque vous extrayer un
  // élément, vérifiez qu'il n'est pas NULL, ce qui n'est pas censé
  // arriver. C'est autant de "segmentation fault" que vous pouvez
  // éviter.



// Améliorations à faire seulement quand vous aurez bien avancé:
//
// (1) Lorsqu'il y a peu d'obstacles, l'ensemble des sommets visités
//     peut être relativement chaotique (on dirait qu'A* oublie de
//     visiter des sommets), ce qui peut être tout à fait normal.
//     Modifiez la fonction de comparaison pour qu'en cas d'égalité
//     des scores, elle tienne compte des coûts. Observez la
//     différence de comportement si vous privilégiez les coûts
//     croissant ou décroissant.
//
// (2) Le chemin a tendance à zigzaguer, c'est-à-dire à utiliser aussi
//     bien des arêtes horizontales que diagonales (qui peuvent avoir
//     le même coût), même pour des chemins en ligne droite. Essayez
//     de rectifier ce problème d'esthétique en modifiant le calcul de
//     score[v] de sorte qu'à coût[v] égale les arêtes (u,v)
//     horizontales ou verticales soient favorisées (un score plus
//     faible) ou de manière équivante que les diagonales soient
//     pénalisées (avec un score légèrement plus élevé). Il est aussi
//     possible d'intervenir au niveau de la fonction de comparaison
//     des noeuds pour tenir compte de la position du parent. Bien
//     sûr, votre modification ne doit en rien changer la distance (la
//     somme des coûts) entre .start et .end.
//
// (3) Modifier votre implémentation du tas dans heap.c de façon à
//     utiliser un tableau de taille variable, en utilisant realloc()
//     et une stratégie "doublante": lorsqu'il n'y a pas plus assez de
//     place dans le tableau, on double sa taille avec un realloc().
//     On peut imaginer que l'ancien paramètre 'k' dans heap_create()
//     devienne non pas le nombre maximal d'éléments total au cours de
//     l'algorihtme, mais le nombre maximal d'éléments au départ. On
//     pourra prendre k=4 par exemple.
//
// (4) Gérer plus efficacement la mémoire en libérant les noeuds
//     devenus inutiles. Pour cela on ajoute un champs .nchild à la
//     structure node, permettant de gérer le nombre de fils qu'un
//     node de P ou Q possède. C'est relativement léger et facile à
//     gérer puisqu'on augmente .nchild de u chaque fois qu'on fait
//     parent[v]=p, soit juste après "node v = createNode(p,...)".
//     Pensez à faire .nchild=0 dans createNode(). Notez bien qu'ici
//     on parle de "node", donc de copie de sommet.
//
//     L'observation utile est que tous les nodes de Q sont des
//     feuilles dans l'arbre des nodes de racine s. On va alors
//     pouvoir se débarrasser de tous les nodes ancêtres de ces
//     feuilles (qui ne sont plus dans Q car extraits avec heap_pop)
//     simplement en extrayant les nodes de Q dans n'importe quel
//     ordre, par exemple avec une boucle (*) sur heap_pop(Q). On
//     supprime alors chaque node, en mettant à jour le nombre de fils
//     de son père, puis en supprimant le père s'il devient feuille à
//     son tour (son .nchild passant à 0) et ainsi de suite
//     récursivement. On élimine ainsi totalement l'arbre par
//     l'élagage des branches qui se terminent dans Q.
//
//     (*) Si on veut être plus efficace que O(|Q|×log|Q|), on peut en
//         temps O(|Q|) vider le tableau .array[] directement sans
//         passer par heap_pop(). Mais pour être propre, c'est-à-dire
//         n'utiliser que des primitives de la structure de données et
//         pas son implémentation en accédant à h->array, il est bien
//         vu d'ajouter une primitive "void* heap_tail(heap h)" qui
//         permettrait d'extraire en temps constant le dernier élément
//         du tas (s'il existe). Cela a comme avantage de garder à
//         tout moment la structure de tas.
//
//     Ce processus peut ne pas éliminer tous les nodes car il peut
//     rester des branches de l'arbre qui se terminent par une feuille
//     qui est dans P (et donc qui n'ont pas de fils dans Q).
//     L'observation est que de tels nodes ne peuvent pas être sur le
//     chemin s->t. On peut donc les supprimer au fur et à mesure
//     directement dans la boucle principale quand on les détecte. On
//     voit qu'un tel node apparaît si après avoir parcouru tous les
//     voisins de u aucun node v n'est créé (et ajouté dans Q). Il
//     suffit donc de savoir si on est passé par heap_add() (ou encore
//     de comparer la taille de Q avant et après la boucle sur les
//     voisins). Si u est une feuille, on peut alors supprimer le node
//     u, mettre à jour .nchild de son père et remonter la branche
//     jusqu'à trouver un node qui n'est plus une feuille. C'est donc
//     la même procédure d'élagage que précédemment qu'on pourrait
//     capturer par une fonction freeNode(node p).


int main(int argc, char *argv[]){

  alpha = (argc>=2)? atof(argv[1]) : 0; // alpha = 0 => Dijkstra par défaut
  unsigned seed=time(NULL)%1000;
  printf("seed: %u\n",seed); // pour rejouer la même grille au cas où
  srandom(seed);

  // testez différentes grilles ...

  //grid G = initGridPoints(80,60,TX_FREE,1); // petite grille vide, sans mur
  //grid G = initGridPoints(width,height,TX_FREE,1); // grande grille vide, sans mur
  //grid G = initGridPoints(32,24,TX_WALL,0.2); // petite grille avec quelques murs
  //grid G = initGridLaby(12,9,8); // petit labyrinthe aléatoire
  //grid G = initGridLaby(width/8,height/8,3); // grand labyrinthe aléatoire
  //grid G = initGridFile("mygrid.txt"); // grille à partir d'un fichier modifiable
  grid G = initGrid3D(width/4,height/4); // grille avec texture réaliste pour 3D
  // ajoutez à G une (ou plusieurs) "région" de texture donnée ...
  // (déconseillé pour initGridLaby() et initGridFile())

  // addRandomBlob(G, TX_WALL,   (G.X+G.Y)/20);
  // addRandomBlob(G, TX_SAND,   (G.X+G.Y)/15);
  // addRandomBlob(G, TX_WATER,  (G.X+G.Y)/6);
  // addRandomBlob(G, TX_MUD,    (G.X+G.Y)/3);
  // addRandomBlob(G, TX_GRASS,  (G.X+G.Y)/15);
  //addRandomBlob(G, TX_TUNNEL, (G.X+G.Y)/4);
  //addRandomArc(G, TX_WALL,    (G.X+G.Y)/25);

  // sélectionnez des positions s->t ...
  // (inutile pour initGridLaby() et initGridFile())

  G.start=(position){0.1*G.X,0.2*G.Y}, G.end=(position){0.8*G.X,0.9*G.Y};
  //G.start=randomPosition(G,TX_FREE); G.end=randomPosition(G,TX_FREE);

  // constantes à initialiser avant init_SDL_OpenGL()
  scale = fmin((double)width/G.X,(double)height/G.Y); // zoom courant
  hover = false; // interdire les déplacements de points
  update = false; // accélère les dessins répétitifs

  init_SDL_OpenGL(); // à mettre avant le 1er "draw"
  drawGrid(G); // dessin de la grille avant l'algo

  // heuristiques: h0, hvo, halpha (=alpha×hvo), ...
  double d = A_star(G,hvo);

  // chemin trouvé ou pas ?
  if (d < 0){
    printf("path not found!\n");
    if(G.end.x<0 || G.end.y<0 || G.end.x>=G.X || G.end.y>=G.Y )
      printf("(destination out of the grid)\n");
    if(G.texture[G.end.x][G.end.y] == TX_WALL)
      printf("(destination on a wall)\n");
  }
  else printf("bingo!!! cost of the path: %g\n", d);

  // compte le nombre de sommets explorés pour comparer les
  // heuristiques
  int m = 0;
  for (int i=0; i<G.X; i++)
    for (int j=0; j<G.Y; j++)
      m += (G.mark[i][j] != MK_NULL);
  printf("#nodes explored: %i\n", m);

  while (running) {     // affiche le résultat et attend
    update = true;      // force l'affichage de chaque dessin
    drawGrid(G);        // dessine la grille (/!\ passe update à false)
    handleEvent(false); // ne pas attendre d'évènement
  }

  freeGrid(G);
  cleaning_SDL_OpenGL();
  return 0;
}