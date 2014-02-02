#ifndef __CRT_INTERNAL_ATEXIT_H
#define __CRT_INTERNAL_ATEXIT_H

#ifndef _CRT_PRECOMP_H
#error DO NOT INCLUDE THIS HEADER DIRECTLY
#endif

#define LOCK_EXIT   _mlock(_EXIT_LOCK1)
#define UNLOCK_EXIT _munlock(_EXIT_LOCK1)

extern _onexit_t *atexit_table;
extern int atexit_table_size;
extern int atexit_registered; /* Points to free slot */

void __call_atexit(void);

#endif
