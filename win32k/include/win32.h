#ifndef __INCLUDE_NAPI_WIN32_H
#define __INCLUDE_NAPI_WIN32_H

#include <pshpack1.h>

typedef struct _W32THREAD
{
  USER_MESSAGE_QUEUE Queue;
  LIST_ENTRY WindowListHead;
  LIST_ENTRY W32CallbackListHead;
  struct _KBDTABLES* KeyboardLayout;
  
  struct _DESKTOP_OBJECT* Desktop;
  HANDLE hDesktop;
  SINGLE_LIST_ENTRY UserObjReferencesStack;
  DWORD MessagePumpHookValue;
  BOOLEAN IsExiting;
  PETHREAD Thread;
  
  //obj_handle_t           desktop;       /* desktop handle */
  //int                    desktop_users; /* number of objects using the thread desktop */
    
  struct _W32PROCESS* WProcess;
} W32THREAD, *PW32THREAD;

#include <poppack.h>


typedef struct _W32PROCESS
{
  PEPROCESS Process;
  LIST_ENTRY ClassListHead;
  LIST_ENTRY ExpiredTimersList;
  LIST_ENTRY MenuListHead;
  FAST_MUTEX PrivateFontListLock;
  LIST_ENTRY PrivateFontListHead;
  FAST_MUTEX DriverObjListLock;
  LIST_ENTRY DriverObjListHead;
  struct _KBDTABLES* KeyboardLayout;
  ULONG Flags;
  LONG GDIObjects;
  LONG UserObjects;
  
  //obj_handle_t         winstation;      /* main handle to process window station */
  //obj_handle_t         desktop;         /* handle to desktop to use for new threads */

} W32PROCESS, *PW32PROCESS;


#endif /* __INCLUDE_NAPI_WIN32_H */
