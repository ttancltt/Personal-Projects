#ifndef EZP_ALLOC_H
#define EZP_ALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

void *ezp_alloc (size_t size);

void ezp_free (void *ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif
