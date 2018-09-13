#define MAX_PROCESS_NAME_LENGTH (MAX_PATH*sizeof(WCHAR))
#define MAX_THREAD_NAME_LENGTH  (10*sizeof(WCHAR))

//
//  Value to decide if process names should be collected from:
//      the SystemProcessInfo structure (fastest)
//          -- or --
//      the process's image file (slower, but shows Unicode filenames)
//
#define PNCM_NOT_DEFINED    ((LONG)-1)
#define PNCM_SYSTEM_INFO    0L
#define PNCM_MODULE_FILE    1L
extern  LONG    lProcessNameCollectionMethod;

#define IDLE_PROCESS_ID     ((DWORD)0)
#define SYSTEM_PROCESS_ID   ((DWORD)7)

//
//  VA structures & defines
//
#define NOACCESS            0
#define READONLY            1
#define READWRITE           2
#define WRITECOPY           3
#define EXECUTE             4
#define EXECUTEREAD         5
#define EXECUTEREADWRITE    6
#define EXECUTEWRITECOPY    7
#define MAXPROTECT          8

typedef struct _MODINFO {
    PVOID   BaseAddress;
    ULONG_PTR VirtualSize;
    PUNICODE_STRING InstanceName;
    PUNICODE_STRING LongInstanceName;
    ULONG_PTR TotalCommit;
    ULONG_PTR CommitVector[MAXPROTECT];
    struct _MODINFO   *pNextModule;
} MODINFO, *PMODINFO;

typedef struct _PROCESS_VA_INFO {
    PUNICODE_STRING      pProcessName;
    HANDLE               hProcess;
    ULONG_PTR             dwProcessId;
    //  process VA information
    PPROCESS_BASIC_INFORMATION BasicInfo;
    //  process VA statistics
    ULONG_PTR            ImageReservedBytes;
    ULONG_PTR            ImageFreeBytes;
    ULONG_PTR            ReservedBytes;
    ULONG_PTR            FreeBytes;
    ULONG_PTR            MappedGuard;
    ULONG_PTR            MappedCommit[MAXPROTECT];
    ULONG_PTR            PrivateGuard;
    ULONG_PTR            PrivateCommit[MAXPROTECT];
    //  process image statistics
    PMODINFO            pMemBlockInfo;  // pointer to image list
    MODINFO             OrphanTotals;   // blocks with no image
    MODINFO             MemTotals;      // sum of image data
    DWORD               LookUpTime;
    struct _PROCESS_VA_INFO    *pNextProcess;
} PROCESS_VA_INFO, *PPROCESS_VA_INFO;

extern PPROCESS_VA_INFO     pProcessVaInfo;    // list head

extern const WCHAR IDLE_PROCESS[];
extern const WCHAR SYSTEM_PROCESS[];

extern  PUNICODE_STRING pusLocalProcessNameBuffer;

extern  HANDLE                          hEventLog;       // handle to event log
extern  HANDLE                          hLibHeap;       // local heap
extern  LPWSTR  wszTotal;

extern  LPBYTE                          pProcessBuffer;
extern  SYSTEM_TIMEOFDAY_INFORMATION    SysTimeInfo;

PM_LOCAL_COLLECT_PROC CollectProcessObjectData;
PM_LOCAL_COLLECT_PROC CollectThreadObjectData;
PM_LOCAL_COLLECT_PROC CollectExProcessObjectData;
PM_LOCAL_COLLECT_PROC CollectImageObjectData;
PM_LOCAL_COLLECT_PROC CollectLongImageObjectData;
PM_LOCAL_COLLECT_PROC CollectThreadDetailsObjectData;
PM_LOCAL_COLLECT_PROC CollectJobObjectData;
PM_LOCAL_COLLECT_PROC CollectJobDetailData;

PUNICODE_STRING
GetProcessShortName (
    PSYSTEM_PROCESS_INFORMATION pProcess
);

PUNICODE_STRING
GetProcessSlowName (
    PSYSTEM_PROCESS_INFORMATION pProcess
);

BOOL
GetProcessExeName(
    HANDLE  hProcessID,
    PUNICODE_STRING pusName
);

PPROCESS_VA_INFO
GetSystemVaData (
    IN PSYSTEM_PROCESS_INFORMATION
);

BOOL
FreeSystemVaData (
    IN PPROCESS_VA_INFO
);
