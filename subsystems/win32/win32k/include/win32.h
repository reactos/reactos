#ifndef __INCLUDE_NAPI_WIN32_H
#define __INCLUDE_NAPI_WIN32_H

/* W32PROCESS flags */
#define W32PF_CONSOLEAPPLICATION      0x00000001
#define W32PF_FORCEOFFFEEDBACK        0x00000002
#define W32PF_STARTGLASS              0x00000004
#define W32PF_WOW                     0x00000008
#define W32PF_READSCREENACCESSGRANTED 0x00000010
#define W32PF_INITIALIZED             0x00000020
#define W32PF_APPSTARTING             0x00000040
#define W32PF_WOW64                   0x00000080
#define W32PF_ALLOWFOREGROUNDACTIVATE 0x00000100
#define W32PF_OWNDCCLEANUP            0x00000200
#define W32PF_SHOWSTARTGLASSCALLED    0x00000400
#define W32PF_FORCEBACKGROUNDPRIORITY 0x00000800
#define W32PF_TERMINATED              0x00001000
#define W32PF_CLASSESREGISTERED       0x00002000
#define W32PF_THREADCONNECTED         0x00004000
#define W32PF_PROCESSCONNECTED        0x00008000
#define W32PF_WAKEWOWEXEC             0x00010000
#define W32PF_WAITFORINPUTIDLE        0x00020000
#define W32PF_IOWINSTA                0x00040000
#define W32PF_CONSOLEFOREGROUND       0x00080000
#define W32PF_OLELOADED               0x00100000
#define W32PF_SCREENSAVER             0x00200000
#define W32PF_IDLESCREENSAVER         0x00400000
#define W32PF_ICONTITLEREGISTERED     0x10000000
// ReactOS
#define W32PF_NOWINDOWGHOSTING       (0x01000000)
#define W32PF_MANUALGUICHECK         (0x02000000)
#define W32PF_CREATEDWINORDC         (0x04000000)

extern BOOL ClientPfnInit;
extern HINSTANCE hModClient;
extern HANDLE hModuleWin;    // This Win32k Instance.
extern PCLS SystemClassList;
extern BOOL RegisteredSysClasses;

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
    W32THREAD;
    PTL                 ptl;
    PPROCESSINFO        ppi;
    struct _USER_MESSAGE_QUEUE* MessageQueue;
    struct _KBL*        KeyboardLayout;
    PCLIENTTHREADINFO   pcti;
    struct _DESKTOP*    rpdesk;
    PDESKTOPINFO        pDeskInfo;
    PCLIENTINFO         pClientInfo;
    FLONG               TIF_flags;
    PUNICODE_STRING     pstrAppName;
    LONG                timeLast;
    ULONG_PTR           idLast;
    INT                 exitCode;
    HDESK               hdesk;
    UINT                cPaintsReady; /* Count of paints pending. */
    UINT                cTimersReady; /* Count of timers pending. */
    DWORD               dwExpWinVer;
    DWORD               dwCompatFlags;
    DWORD               dwCompatFlags2;
    struct _USER_MESSAGE_QUEUE* pqAttach;
    PTHREADINFO         ptiSibling;
    ULONG               fsHooks;
    PHOOK               sphkCurrent;
    LPARAM              lParamHkCurrent;
    WPARAM              wParamHkCurrent;
    struct tagSBTRACK*  pSBTrack;
    HANDLE              hEventQueueClient;
    PKEVENT             pEventQueueServer;
    LIST_ENTRY          PtiLink;

    CLIENTTHREADINFO    cti;  // Used only when no Desktop or pcti NULL.
  /* ReactOS */
  LIST_ENTRY WindowListHead;
  LIST_ENTRY W32CallbackListHead;
  SINGLE_LIST_ENTRY  ReferencesList;
} THREADINFO;

#include <poppack.h>

typedef struct _W32HEAP_USER_MAPPING
{
    struct _W32HEAP_USER_MAPPING *Next;
    PVOID KernelMapping;
    PVOID UserMapping;
    ULONG_PTR Limit;
    ULONG Count;
} W32HEAP_USER_MAPPING, *PW32HEAP_USER_MAPPING;


/*
 Information from STARTUPINFOW, psdk/winbase.h.
 Set from PsGetCurrentProcess()->Peb->ProcessParameters.
*/
typedef struct tagUSERSTARTUPINFO
{
    ULONG cb;
    DWORD dwX;        // STARTF_USEPOSITION StartupInfo->dwX/Y
    DWORD dwY;
    DWORD dwXSize;    // STARTF_USESIZE StartupInfo->dwX/YSize
    DWORD dwYSize;
    DWORD dwFlags;    // STARTF_ StartupInfo->dwFlags
    WORD wShowWindow; // StartupInfo->wShowWindow
    USHORT cbReserved2;
} USERSTARTUPINFO, *PUSERSTARTUPINFO;

typedef struct _W32PROCESS
{
  PEPROCESS     peProcess;
  DWORD         RefCount;
  ULONG         W32PF_flags;
  PKEVENT       InputIdleEvent;
  DWORD         StartCursorHideTime;
  struct _W32PROCESS * NextStart;
  PVOID         pDCAttrList;
  PVOID         pBrushAttrList;
  DWORD         W32Pid;
  LONG          GDIHandleCount;
  LONG          UserHandleCount;
  PEX_PUSH_LOCK GDIPushLock;  /* Locking Process during access to structure. */
  RTL_AVL_TABLE GDIEngUserMemAllocTable;  /* Process AVL Table. */
  LIST_ENTRY    GDIDcAttrFreeList;
  LIST_ENTRY    GDIBrushAttrFreeList;
} W32PROCESS, *PW32PROCESS;

typedef struct _PROCESSINFO
{
  W32PROCESS;
  PTHREADINFO ptiList;
  PTHREADINFO ptiMainThread;
  struct _DESKTOP* rpdeskStartup;
  PCLS pclsPrivateList;
  PCLS pclsPublicList;

  HMONITOR hMonitor;

  USERSTARTUPINFO usi;
  ULONG Flags;
  DWORD dwLayout;
  DWORD dwRegisteredClasses;
  /* ReactOS */
  LIST_ENTRY ClassList;
  LIST_ENTRY MenuListHead;
  FAST_MUTEX PrivateFontListLock;
  LIST_ENTRY PrivateFontListHead;
  FAST_MUTEX DriverObjListLock;
  LIST_ENTRY DriverObjListHead;
  struct _KBL* KeyboardLayout; // THREADINFO only
  W32HEAP_USER_MAPPING HeapMappings;
} PROCESSINFO;

#endif /* __INCLUDE_NAPI_WIN32_H */
