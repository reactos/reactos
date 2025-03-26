/*
 * FILE:            win32ss/napi.h
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PURPOSE:         System Call Table for Native API
 * PROGRAMMER:      Timo Kreuzer
 */

#define SVC_(name, argcount) (ULONG_PTR)Nt##name,
ULONG_PTR Win32kSSDT[] = {
#ifdef _WIN64
#include "w32ksvc64.h"
#else
#include "w32ksvc32.h"
#endif
};
#undef SVC_

#define SVC_(name, argcount) argcount * sizeof(void *),
UCHAR Win32kSSPT[] = {
#ifdef _WIN64
#include "w32ksvc64.h"
#else
#include "w32ksvc32.h"
#endif
};

#define MIN_SYSCALL_NUMBER    0x1000
#define NUMBER_OF_SYSCALLS    (sizeof(Win32kSSPT) / sizeof(Win32kSSPT[0]))
#define MAX_SYSCALL_NUMBER    0x1000 + (NUMBER_OF_SYSCALLS - 1)
ULONG Win32kNumberOfSysCalls = NUMBER_OF_SYSCALLS;
