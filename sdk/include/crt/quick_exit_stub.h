#ifndef _QUICK_EXIT_STUB_H
#define _QUICK_EXIT_STUB_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Stub implementations for GCC 15 compatibility */
static inline int at_quick_exit(void (*func)(void)) { return 0; }
static inline void quick_exit(int status) { exit(status); }

#ifdef __cplusplus
}
#endif

#endif /* _QUICK_EXIT_STUB_H */
