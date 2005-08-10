#ifndef __INCLUDE_NAPI_WIN32_H
#define __INCLUDE_NAPI_WIN32_H

#include <pshpack1.h>

typedef struct _W32THREAD
{
  struct _USER_MESSAGE_QUEUE* MessageQueue;
  FAST_MUTEX WindowListLock;
  LIST_ENTRY WindowListHead;
  LIST_ENTRY W32CallbackListHead;
  struct _KBDTABLES* KeyboardLayout;
  struct _DESKTOP_OBJECT* Desktop;
  HANDLE hDesktop;
  DWORD MessagePumpHookValue;
  BOOLEAN IsExiting;
} W32THREAD, *PW32THREAD;

#include <poppack.h>


typedef struct _W32PROCESS
{
  FAST_MUTEX ClassListLock;
  LIST_ENTRY ClassListHead;
  FAST_MUTEX MenuListLock;
  LIST_ENTRY MenuListHead;
  FAST_MUTEX PrivateFontListLock;
  LIST_ENTRY PrivateFontListHead;
  FAST_MUTEX DriverObjListLock;
  LIST_ENTRY DriverObjListHead;
  struct _KBDTABLES* KeyboardLayout;
  ULONG Flags;
  LONG GDIObjects;
  LONG UserObjects;
} W32PROCESS, *PW32PROCESS;


#endif /* __INCLUDE_NAPI_WIN32_H */
