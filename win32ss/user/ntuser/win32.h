#pragma once

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
#define W32PF_SETFOREGROUNDALLOWED    0x00008000
#define W32PF_WAKEWOWEXEC             0x00010000
#define W32PF_WAITFORINPUTIDLE        0x00020000
#define W32PF_IOWINSTA                0x00040000
#define W32PF_CONSOLEFOREGROUND       0x00080000
#define W32PF_OLELOADED               0x00100000
#define W32PF_SCREENSAVER             0x00200000
#define W32PF_IDLESCREENSAVER         0x00400000
#define W32PF_ICONTITLEREGISTERED     0x10000000
#define W32PF_DPIAWARE                0x20000000
// ReactOS
#define W32PF_NOWINDOWGHOSTING       (0x01000000)
#define W32PF_MANUALGUICHECK         (0x02000000)
#define W32PF_CREATEDWINORDC         (0x04000000)
#define W32PF_APIHOOKLOADED          (0x08000000)

typedef enum _QS_ROS_TYPES
{
    QSRosKey = 0,
    QSRosMouseMove,
    QSRosMouseButton,
    QSRosPostMessage,
    QSRosSendMessage,
    QSRosHotKey,
    QSRosEvent,
} QS_ROS_TYPES, *PQS_ROS_TYPES;

extern BOOL ClientPfnInit;
extern HINSTANCE hModClient;
extern HANDLE hModuleWin;    // This Win32k Instance.
extern struct _CLS *SystemClassList;
extern BOOL RegisteredSysClasses;

#define IntReferenceThreadInfo(pti) \
    InterlockedIncrement(&(pti)->RefCount)

VOID UserDeleteW32Thread(PTHREADINFO);

#define IntDereferenceThreadInfo(pti) \
do { \
    if (InterlockedDecrement(&(pti)->RefCount) == 0) \
    { \
        ASSERT(((pti)->TIF_flags & (TIF_INCLEANUP|TIF_DONTATTACHQUEUE)) == (TIF_INCLEANUP|TIF_DONTATTACHQUEUE)); \
        UserDeleteW32Thread(pti); \
    } \
} while(0)

#define IntReferenceProcessInfo(ppi) \
    InterlockedIncrement((volatile LONG*)(&(ppi)->RefCount))

VOID UserDeleteW32Process(_Pre_notnull_ __drv_freesMem(Mem) PPROCESSINFO);

#define IntDereferenceProcessInfo(ppi) \
do { \
    if (InterlockedDecrement((volatile LONG*)(&(ppi)->RefCount)) == 0) \
    { \
        ASSERT(((ppi)->W32PF_flags & W32PF_TERMINATED) != 0); \
        UserDeleteW32Process(ppi); \
    } \
} while(0)


typedef struct _W32HEAP_USER_MAPPING
{
    struct _W32HEAP_USER_MAPPING* Next;
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
    struct _W32PROCESS* NextStart;
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

#define CLIBS 32

#ifdef __cplusplus
typedef struct _PROCESSINFO : _W32PROCESS
{
#else
typedef struct _PROCESSINFO
{
    W32PROCESS;
#endif
    PTHREADINFO ptiList;
    PTHREADINFO ptiMainThread;
    struct _DESKTOP* rpdeskStartup;
    struct _CLS *pclsPrivateList;
    struct _CLS *pclsPublicList;
    PPROCESSINFO ppiNext;
    INT cThreads;
    HDESK hdeskStartup;
    DWORD dwhmodLibLoadedMask;
    HANDLE ahmodLibLoaded[CLIBS];
    struct _WINSTATION_OBJECT* prpwinsta;
    HWINSTA hwinsta;
    ACCESS_MASK amwinsta;
    DWORD dwHotkey;
    HMONITOR hMonitor;
    UINT iClipSerialNumber;
    struct _CURICON_OBJECT* pCursorCache;
    PVOID pClientBase;
    DWORD dwLpkEntryPoints;
    PVOID pW32Job;
    DWORD dwImeCompatFlags;
    LUID luidSession;
    USERSTARTUPINFO usi;
    DWORD dwLayout;
    DWORD dwRegisteredClasses;

    /* ReactOS */
    FAST_MUTEX PrivateFontListLock;
    LIST_ENTRY PrivateFontListHead;
    LIST_ENTRY PrivateMemFontListHead;
    UINT PrivateMemFontHandleCount;

    FAST_MUTEX DriverObjListLock;
    LIST_ENTRY DriverObjListHead;
    W32HEAP_USER_MAPPING HeapMappings;
    struct _GDI_POOL* pPoolDcAttr;
    struct _GDI_POOL* pPoolBrushAttr;
    struct _GDI_POOL* pPoolRgnAttr;

#if DBG
    BYTE DbgChannelLevel[DbgChCount];
#ifndef __cplusplus
    DWORD DbgHandleCount[TYPE_CTYPES];
#endif // __cplusplus
#endif // DBG
} PROCESSINFO;

#if DBG
void NTAPI UserDbgPreServiceHook(ULONG ulSyscallId, PULONG_PTR pulArguments);
ULONG_PTR NTAPI UserDbgPostServiceHook(ULONG ulSyscallId, ULONG_PTR ulResult);
#endif

/* Helper function used by some wine code */

__forceinline
int
lstrlenW(
    _In_ LPCWSTR lpString)
{
    size_t size = wcslen(lpString);
    if (size > ULONG_MAX) __fastfail(FAST_FAIL_RANGE_CHECK_FAILURE);
    return (int)size;
}

#define strlenW lstrlenW
