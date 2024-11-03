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
#define W32PF_DISABLEIME              0x00800000
#define W32PF_ICONTITLEREGISTERED     0x10000000
#define W32PF_DPIAWARE                0x20000000
// ReactOS
#define W32PF_NOWINDOWGHOSTING       (0x01000000)
#define W32PF_MANUALGUICHECK         (0x02000000)
#define W32PF_CREATEDWINORDC         (0x04000000)
#define W32PF_APIHOOKLOADED          (0x08000000)

#define QSIDCOUNTS 7

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

struct _TL;
typedef struct _TL *PTL;

typedef struct _W32THREAD
{
    PETHREAD pEThread;
    LONG RefCount;
    PTL ptlW32;
    PVOID pgdiDcattr;
    PVOID pgdiBrushAttr;
    PVOID pUMPDObjs;
    PVOID pUMPDHeap;
    DWORD dwEngAcquireCount;
    PVOID pSemTable;
    PVOID pUMPDObj;
} W32THREAD, *PW32THREAD;

struct tagIMC;

/*
 * THREADINFO structure.
 * See also: https://reactos.org/wiki/Techwiki:Win32k/THREADINFO
 */
#ifdef __cplusplus
typedef struct _THREADINFO : _W32THREAD
{
#else
typedef struct _THREADINFO
{
    W32THREAD;
#endif
    PTL                 ptl;
    PPROCESSINFO        ppi;
    struct _USER_MESSAGE_QUEUE* MessageQueue;
    struct tagKL*       KeyboardLayout;
    struct _CLIENTTHREADINFO  * pcti;
    struct _DESKTOP*    rpdesk;
    struct _DESKTOPINFO  *  pDeskInfo;
    struct _CLIENTINFO * pClientInfo;
    FLONG               TIF_flags;
    PUNICODE_STRING     pstrAppName;
    struct _USER_SENT_MESSAGE *pusmSent;
    struct _USER_SENT_MESSAGE *pusmCurrent;
    /* Queue of messages sent to the queue. */
    LIST_ENTRY          SentMessagesListHead;    // psmsReceiveList
    /* Last message time and ID */
    LONG                timeLast;
    ULONG_PTR           idLast;
    /* True if a WM_QUIT message is pending. */
    BOOLEAN             QuitPosted;
    /* The quit exit code. */
    INT                 exitCode;
    HDESK               hdesk;
    UINT                cPaintsReady; /* Count of paints pending. */
    UINT                cTimersReady; /* Count of timers pending. */
    struct tagMENUSTATE* pMenuState;
    DWORD               dwExpWinVer;
    DWORD               dwCompatFlags;
    DWORD               dwCompatFlags2;
    struct _USER_MESSAGE_QUEUE* pqAttach;
    PTHREADINFO         ptiSibling;
    ULONG               fsHooks;
    struct tagHOOK *    sphkCurrent;
    LPARAM              lParamHkCurrent;
    WPARAM              wParamHkCurrent;
    struct tagSBTRACK*  pSBTrack;
    /* Set if there are new messages specified by WakeMask in any of the queues. */
    HANDLE              hEventQueueClient;
    /* Handle for the above event (in the context of the process owning the queue). */
    PKEVENT             pEventQueueServer;
    LIST_ENTRY          PtiLink;
    INT                 iCursorLevel;
    /* Last message cursor position */
    POINT               ptLast;
    /* Input context-related */
    struct _WND*        spwndDefaultIme;
    struct tagIMC*      spDefaultImc;
    HKL                 hklPrev;

    INT                 cEnterCount;
    /* Queue of messages posted to the queue. */
    LIST_ENTRY          PostedMessagesListHead; // mlPost
    WORD                fsChangeBitsRemoved;
    WCHAR               wchInjected;
    UINT                cWindows;
    UINT                cVisWindows;
#ifndef __cplusplus /// FIXME!
    LIST_ENTRY          aphkStart[NB_HOOKS];
    CLIENTTHREADINFO    cti;  // Used only when no Desktop or pcti NULL.

    /* ReactOS */

    /* Thread Queue state tracking */
    // Send list QS_SENDMESSAGE
    // Post list QS_POSTMESSAGE|QS_HOTKEY|QS_PAINT|QS_TIMER|QS_KEY
    // Hard list QS_MOUSE|QS_KEY only
    // Accounting of queue bit sets, the rest are flags. QS_TIMER QS_PAINT counts are handled in thread information.
    DWORD nCntsQBits[QSIDCOUNTS]; // QS_KEY QS_MOUSEMOVE QS_MOUSEBUTTON QS_POSTMESSAGE QS_SENDMESSAGE QS_HOTKEY

    LIST_ENTRY WindowListHead;
    LIST_ENTRY W32CallbackListHead;
    SINGLE_LIST_ENTRY  ReferencesList;
    ULONG cExclusiveLocks;
#if DBG
    USHORT acExclusiveLockCount[GDIObjTypeTotal + 1];
    UINT cRefObjectCo;
#endif
#endif // __cplusplus
} THREADINFO;


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
 * Information from STARTUPINFOW, psdk/winbase.h.
 * Set from PsGetCurrentProcess()->Peb->ProcessParameters.
 * See also: https://reactos.org/wiki/Techwiki:Win32k/PROCESSINFO
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

/*
 * PROCESSINFO structure.
 * See also: https://reactos.org/wiki/Techwiki:Win32k/PROCESSINFO
 */
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
