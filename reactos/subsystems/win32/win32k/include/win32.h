#ifndef __INCLUDE_NAPI_WIN32_H
#define __INCLUDE_NAPI_WIN32_H

typedef struct _WIN32HEAP WIN32HEAP, *PWIN32HEAP;

#include <pshpack1.h>
typedef struct _W32THREAD
{
    PETHREAD pEThread;
} W32THREAD, *PW32THREAD;

typedef struct _THREADINFO
{
    W32THREAD           W32Thread;
    PVOID               ppi; // FIXME: use PPROCESSINFO
} THREADINFO, *PTHREADINFO;

#include <poppack.h>

typedef struct _W32HEAP_USER_MAPPING
{
    struct _W32HEAP_USER_MAPPING *Next;
    PVOID KernelMapping;
    PVOID UserMapping;
    ULONG_PTR Limit;
    ULONG Count;
} W32HEAP_USER_MAPPING, *PW32HEAP_USER_MAPPING;

typedef struct _PROCESSINFO
{
  PEPROCESS     peProcess;
  W32HEAP_USER_MAPPING HeapMappings;
} PROCESSINFO, *PPROCESSINFO;

#endif /* __INCLUDE_NAPI_WIN32_H */
