#ifndef __INCLUDE_NAPI_WIN32_H
#define __INCLUDE_NAPI_WIN32_H

typedef struct _W32THREAD
{
  PVOID MessageQueue;
  FAST_MUTEX WindowListLock;
  LIST_ENTRY WindowListHead;
  struct _KBDTABLES* KeyboardLayout;
  struct _DESKTOP_OBJECT* Desktop;
} __attribute__((packed)) W32THREAD, *PW32THREAD;

typedef struct _W32PROCESS
{
  FAST_MUTEX ClassListLock;
  LIST_ENTRY ClassListHead;
  FAST_MUTEX MenuListLock;
  LIST_ENTRY MenuListHead;
  struct _KBDTABLES* KeyboardLayout;
  struct _WINSTATION_OBJECT* WindowStation;
} W32PROCESS, *PW32PROCESS;

PW32THREAD STDCALL
PsGetWin32Thread(VOID);
PW32PROCESS STDCALL
PsGetWin32Process(VOID);

#endif /* __INCLUDE_NAPI_WIN32_H */
