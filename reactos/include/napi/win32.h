#ifndef __INCLUDE_NAPI_WIN32_H
#define __INCLUDE_NAPI_WIN32_H

typedef struct _W32THREAD
{
  PVOID MessageQueue;
} __attribute__((packed)) W32THREAD, *PW32THREAD;

typedef struct _W32PROCESS
{
  FAST_MUTEX ClassListLock;
  LIST_ENTRY ClassListHead;
  FAST_MUTEX WindowListLock;
  LIST_ENTRY WindowListHead;
  struct _USER_HANDLE_TABLE* HandleTable;
} W32PROCESS, *PW32PROCESS;

PW32THREAD STDCALL
PsGetWin32Thread(VOID);
PW32PROCESS STDCALL
PsGetWin32Process(VOID);

#endif /* __INCLUDE_NAPI_WIN32_H */
