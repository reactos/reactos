#ifndef __INCLUDE_NAPI_WIN32_H
#define __INCLUDE_NAPI_WIN32_H

typedef struct _WIN32HEAP WIN32HEAP, *PWIN32HEAP;

#include <pshpack1.h>
// FIXME! Move to ntuser.h
typedef struct _TL
{
    struct _TL* next;
    PVOID pobj;
    PVOID pfnFree;
} TL, *PTL;

typedef struct _W32THREAD
{
    PETHREAD pEThread;
    ULONG RefCount;
    PTL ptlW32;
    PVOID pgdiDcattr;
    PVOID pgdiBrushAttr;
    PVOID pUMPDObjs;
    PVOID pUMPDHeap;
    DWORD dwEngAcquireCount;
    PVOID pSemTable;
    PVOID pUMPDObj;
} W32THREAD, *PW32THREAD;

typedef struct _THREADINFO
{
    W32THREAD           W32Thread;
    PTL                 ptl;
    PVOID               ppi; // FIXME: use PPROCESSINFO
    struct _USER_MESSAGE_QUEUE* MessageQueue;
    struct _KBL*        KeyboardLayout;
    PCLIENTTHREADINFO   pcti;
    struct _DESKTOP*    Desktop;
    PDESKTOPINFO        pDeskInfo;
    PCLIENTINFO         pClientInfo;
    FLONG               TIF_flags;
    LONG                timeLast;
    HANDLE              hDesktop;
    UINT                cPaintsReady; /* Count of paints pending. */
    UINT                cTimersReady; /* Count of timers pending. */
    ULONG               fsHooks;
    PHOOK               sphkCurrent;
    LIST_ENTRY          PtiLink;

    CLIENTTHREADINFO    cti;  // Used only when no Desktop or pcti NULL.

  LIST_ENTRY WindowListHead;
  LIST_ENTRY W32CallbackListHead;
  BOOLEAN IsExiting;
  SINGLE_LIST_ENTRY  ReferencesList;
  PW32THREADINFO ThreadInfo;
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

typedef struct _W32PROCESS
{
  PEPROCESS     peProcess;
  DWORD         RefCount;
  ULONG         W32PF_flags;
  PKEVENT       InputIdleEvent;
  DWORD         StartCursorHideTime;
  DWORD         NextStart;
  PVOID         pDCAttrList;
  PVOID         pBrushAttrList;
  DWORD         W32Pid;
  LONG          GDIHandleCount;
  LONG          UserHandleCount;
  DWORD         cSimpleLock;  /* Locking Process during access to structure. */
  RTL_AVL_TABLE rtlAvlTable;  /* Process AVL Table. */
  LIST_ENTRY    leDCAttrList;
  LIST_ENTRY    leObjAttrList;
/* ReactOS */
  LIST_ENTRY ClassList;
  LIST_ENTRY MenuListHead;
  FAST_MUTEX PrivateFontListLock;
  LIST_ENTRY PrivateFontListHead;
  FAST_MUTEX DriverObjListLock;
  LIST_ENTRY DriverObjListHead;
  struct _KBL* KeyboardLayout;
  W32HEAP_USER_MAPPING HeapMappings;
} W32PROCESS, *PW32PROCESS;

typedef struct _PROCESSINFO
{
    W32PROCESS          XzyxW32Process; /* Place holder. */
    /* ReactOS */
    HINSTANCE    hModUser;
    PWINDOWCLASS LocalClassList;
    PWINDOWCLASS GlobalClassList;
    PWINDOWCLASS SystemClassList;
                    
    UINT RegisteredSysClasses : 1;
                        
} PROCESSINFO;

#endif /* __INCLUDE_NAPI_WIN32_H */
