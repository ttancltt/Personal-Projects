#ifndef EZM_TYPES_H
#define EZM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TASK_TYPE_COMPUTE,
    TASK_TYPE_WRITE,
    TASK_TYPE_READ
} task_type_t;

#ifdef __cplusplus
}
#endif

#endif
