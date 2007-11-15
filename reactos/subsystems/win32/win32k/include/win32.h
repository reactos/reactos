#ifndef __INCLUDE_NAPI_WIN32_H
#define __INCLUDE_NAPI_WIN32_H

#include <pshpack1.h>

typedef struct _W32THREAD
{
  struct _USER_MESSAGE_QUEUE* MessageQueue;
  LIST_ENTRY WindowListHead;
  LIST_ENTRY W32CallbackListHead;
  struct _KBL* KeyboardLayout;
  struct _DESKTOP_OBJECT* Desktop;
  HANDLE hDesktop;
  DWORD MessagePumpHookValue;
  BOOLEAN IsExiting;
  SINGLE_LIST_ENTRY  ReferencesList;

  PW32THREADINFO ThreadInfo;
} W32THREAD, *PW32THREAD;

#include <poppack.h>

typedef struct _W32HEAP_USER_MAPPING
{
    struct _W32HEAP_USER_MAPPING *Next;
    PVOID KernelMapping;
    PVOID UserMapping;
    ULONG_PTR Limit;
    ULONG Count;
} W32HEAP_USER_MAPPING, *PW32HEAP_USER_MAPPING;

typedef struct _W32PROCESS
{
  LIST_ENTRY ClassList;
  LIST_ENTRY MenuListHead;
  FAST_MUTEX PrivateFontListLock;
  LIST_ENTRY PrivateFontListHead;
  FAST_MUTEX DriverObjListLock;
  LIST_ENTRY DriverObjListHead;
  struct _KBL* KeyboardLayout;
  ULONG Flags;
  LONG GDIObjects;
  LONG UserObjects;

  W32HEAP_USER_MAPPING HeapMappings;
  PW32PROCESSINFO ProcessInfo;
} W32PROCESS, *PW32PROCESS;


#endif /* __INCLUDE_NAPI_WIN32_H */
