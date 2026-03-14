# Techniques Algorithmiques et Programmation  
## Travaux Pratiques – TSP & Algorithme A*

Ce dépôt regroupe l’ensemble des travaux pratiques réalisés dans le cadre du module **Techniques Algorithmiques et Programmation**.  
Il contient plusieurs implémentations du **problème du voyageur de commerce (TSP)** ainsi qu’un projet final consacré à l’algorithme **A\*** appliqué à la recherche de chemin optimal sur une grille.

---

## 📁 Structure du dépôt

```
.
├── a_star.c
├── heap.c
├── heap.h
├── Makefile
├── mygrid.txt
├── README.md
├── td-heap-1.pdf
├── test/
├── test.c
├── test_heap.c
├── tools.c
├── tools.h
├── tsp_brute_force.c
├── tsp_brute_force.h
├── tsp_heuristic.c
├── tsp_heuristic.h
├── tsp_main.c
├── tsp_mst.c
├── tsp_mst.h
├── tsp_prog_dyn.c
└── tsp_prog_dyn.h
```

---

## 🧭 Description des modules TSP

Le dépôt contient plusieurs approches différentes pour résoudre le **problème du voyageur de commerce (TSP)**.  
Chaque fichier correspond à une méthode algorithmique spécifique.

### 🔹 1. **TSP – Brute Force**  
**Fichiers :** `tsp_brute_force.c`, `tsp_brute_force.h`

Cette implémentation explore **toutes les permutations possibles** des villes afin de déterminer le chemin optimal.  
Elle sert principalement à illustrer la complexité exponentielle du TSP.

Points importants :
- génération exhaustive des solutions  
- calcul systématique des distances  
- optimalité garantie mais temps d’exécution très élevé  

---

### 🔹 2. **TSP – Heuristiques**  
**Fichiers :** `tsp_heuristic.c`, `tsp_heuristic.h`

Ce module regroupe plusieurs heuristiques classiques permettant d’obtenir rapidement une solution approchée.

Méthodes typiques :
- plus proche voisin (Nearest Neighbor)  
- insertion la plus proche / la plus éloignée  
- améliorations locales possibles  

Objectif : **rapidité** au détriment de l’optimalité.

---

### 🔹 3. **TSP – Arbre couvrant minimal (MST)**  
**Fichiers :** `tsp_mst.c`, `tsp_mst.h`

Cette approche utilise un **arbre couvrant minimal** pour construire un circuit approximatif.  
Elle repose sur des propriétés structurelles du graphe.

Concepts abordés :
- Prim / Kruskal  
- parcours de l’arbre pour générer un tour  
- approximation garantie dans certains cas  

---

### 🔹 4. **TSP – Programmation dynamique**  
**Fichiers :** `tsp_prog_dyn.c`, `tsp_prog_dyn.h`

Implémentation de l’algorithme de **Held-Karp**, une solution exacte en temps pseudo-exponentiel.

Caractéristiques :
- utilisation de sous-ensembles et mémoïsation  
- complexité \( O(n^2 2^n) \)  
- beaucoup plus efficace que brute force pour des tailles moyennes  

---

## ⭐ Projet final – Algorithme A\*

Le fichier `a_star.c` contient une implémentation complète de l’algorithme **A\*** permettant de trouver un chemin optimal sur une grille.

### ▶️ Compilation

Pour compiler l’exécutable :

```bash
make a_star
```

### ▶️ Exécution

Pour lancer le programme :

```bash
./a_star
```

### 🗺️ Modification de la carte utilisée

Le programme lit une carte depuis un fichier texte (ex. `mygrid.txt`).  
Pour changer la carte utilisée, il suffit de modifier la fonction `main()` dans `a_star.c` et d’y indiquer un autre fichier.
Ainsi, il suffit de décommenter les codes pour visualiser d'autres cartes.

Exemple :

```c
grid G = initGridFile("mygrid.txt"); 
```

---

## 🧰 Autres fichiers utiles

- `heap.c`, `heap.h` : implémentation d’un tas binaire utilisé par A\*  
- `tools.c`, `tools.h` : fonctions utilitaires  
- `test_heap.c`, `test.c` : programmes de test  
- `td-heap-1.pdf` : support de cours sur les tas  

---

## 🎯 Objectif pédagogique

Ce projet permet de :

- comparer différentes stratégies de résolution du TSP  
- comprendre les compromis entre optimalité et performance  
- manipuler des structures de données avancées (tas, listes, tableaux dynamiques)  
- visualiser un algorithme de pathfinding (A\*)  
- développer une approche algorithmique rigoureuse  

---

## 👨‍💻 Auteur  TRUONG Thien An -
Travaux réalisés dans le cadre du module **Techniques Algorithmiques et Programmation**. Université de Bordeaux, 2026
```

