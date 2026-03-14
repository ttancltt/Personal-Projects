#include "heap.h"
#include <stdlib.h>

heap heap_create(int k, int (*f)(const void *, const void *)) {
  heap new_heap=malloc(sizeof(*(new_heap)));
  new_heap->f=f;
  new_heap->nmax=k;
  new_heap->n=0;
  new_heap->array=malloc(sizeof(*(new_heap->array))*(k+1));
  return new_heap;
}

void heap_destroy(heap h) {
  free(h->array);
  free(h);
}

bool heap_empty(heap h) {
  return (h->n==0);
}

bool heap_add(heap h, void *object) {

    if (h->n + 1 > h->nmax)
        return true;   // pas assez de place

    h->n++;
    int cur_index = h->n;
    h->array[cur_index] = object;

    int index_pere = cur_index / 2;
    void *tmp;

    while (cur_index > 1 &&
           h->f(h->array[index_pere], h->array[cur_index]) > 0) {

        tmp = h->array[index_pere];
        h->array[index_pere] = h->array[cur_index];
        h->array[cur_index] = tmp;

        cur_index = index_pere;
        index_pere = cur_index / 2;
    }

    return false;   // insertion ok
}


void *heap_top(heap h) {
  if (!heap_empty (h)){
     return  h->array[1];
  }
  return NULL;
}

void *heap_pop(heap h) {

  if (heap_empty(h))
    return NULL;

  void *top = h->array[1];

  // đưa phần tử cuối lên gốc
  h->array[1] = h->array[h->n];
  h->n--;

  int cur = 1;
  int left, right, smallest;
  void *tmp;

  while (1) {

    left  = 2 * cur;
    right = 2 * cur + 1;
    smallest = cur;

    // so với con trái
    if (left <= h->n &&
        h->f(h->array[smallest], h->array[left]) > 0)
      smallest = left;

    // so với con phải
    if (right <= h->n &&
        h->f(h->array[smallest], h->array[right]) > 0)
      smallest = right;

    if (smallest == cur)
      break;

    // swap
    tmp = h->array[cur];
    h->array[cur] = h->array[smallest];
    h->array[smallest] = tmp;

    cur = smallest;
  }

  return top;
}
