/*
 * FILE:            ntoskrnl/include/internal/napi.h
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PURPOSE:         System Call Table for Native API
 * PROGRAMMER:      Timo Kreuzer
 */

#define SVC_(name, argcount) (ULONG_PTR)Nt##name,
ULONG_PTR MainSSDT[] = {
#include "sysfuncs.h"
};
#undef SVC_

#define SVC_(name, argcount) argcount * sizeof(void *),
UCHAR MainSSPT[] = {
#include "sysfuncs.h"
};

#define MIN_SYSCALL_NUMBER    0
#define NUMBER_OF_SYSCALLS    (sizeof(MainSSPT) / sizeof(MainSSPT[0]))
#define MAX_SYSCALL_NUMBER    (NUMBER_OF_SYSCALLS - 1)
ULONG MainNumberOfSysCalls = NUMBER_OF_SYSCALLS;
