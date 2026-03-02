#ifndef MIN_MAX_IS_DEF
#define MIN_MAX_IS_DEF

#ifdef __cplusplus
extern "C" {
#endif

#define MIN(a, b)                                                              \
  ({                                                                           \
    __typeof__ (a) _a = (a);                                                   \
    __typeof__ (b) _b = (b);                                                   \
    _a < _b ? _a : _b;                                                         \
  })

#define MAX(a, b)                                                              \
  ({                                                                           \
    __typeof__ (a) _a = (a);                                                   \
    __typeof__ (b) _b = (b);                                                   \
    _a > _b ? _a : _b;                                                         \
  })

#define ROUND_TO_MULTIPLE(n, r) (((n) + (r) - 1U) & ~((r) - 1U))

#ifdef __cplusplus
}
#endif

#endif
