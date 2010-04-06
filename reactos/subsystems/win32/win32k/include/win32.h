#ifndef __INCLUDE_NAPI_WIN32_H
#define __INCLUDE_NAPI_WIN32_H

typedef struct _WIN32HEAP WIN32HEAP, *PWIN32HEAP;

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
  PEPROCESS            peProcess;
  W32HEAP_USER_MAPPING HeapMappings;
  struct list          Classes;         /* window classes owned by the process */
  struct handle_table *handles;         /* handle entries */
  PKEVENT              idle_event;      /* event for input idle */
  struct msg_queue    *queue;           /* main message queue */
  obj_handle_t         winstation;      /* main handle to process window station */
  obj_handle_t         desktop;         /* handle to desktop to use for new threads */
  LONG                 GDIHandleCount;  /* kernelmode GDI handles count */
} PROCESSINFO;

#include <pshpack1.h>

typedef struct _THREADINFO
{
    PETHREAD            peThread;
    PPROCESSINFO        process;
    struct msg_queue    *queue;         /* message queue */
    obj_handle_t        desktop;       /* desktop handle */
    int                 desktop_users; /* number of objects using the thread desktop */
    struct _KBL*        KeyboardLayout;
} THREADINFO;

#include <poppack.h>

#endif /* __INCLUDE_NAPI_WIN32_H */
