#ifndef TAQUIN_H_
#define TAQUIN_H_

typedef struct _config *config;

config new_config(int n);

config new_config(int n);

config copy_config(config C);

void set_zero(config C);

config move(config C, char m);

void random_permutation(int *P, int k);

#endif // TAQUIN_H_
