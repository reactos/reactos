
// Some nice shared memory macros
//
// Changed by DaveHart 25-Feb-96 so that the shared memory
// is kept open and mapped for the duration of the process.
// At the same time added needed synchronization.
//

#define LOCKSHAREWOW()           LockWowSharedMemory()
#define UNLOCKSHAREWOW()         ReleaseMutex(hSharedWOWMutex)

#define MAX_SHARED_OBJECTS  200

typedef struct _SHAREDTASKMEM {
    BOOL             fInitialized;
    DWORD            dwFirstProcess; // Offset into shared memory where 1st shared process struct begins
} SHAREDTASKMEM, *LPSHAREDTASKMEM;

typedef struct _SHAREDPROCESS {
    DWORD   dwType;
    DWORD   dwProcessId;
    DWORD   dwAttributes;           // WOW_SYSTEM for shared WOW
    DWORD   dwNextProcess;          // Offset into shared memory where next shared process struct begins
    DWORD   dwFirstTask;            // Offset into shared memory where 1st task for this process begins
    LPTHREAD_START_ROUTINE pfnW32HungAppNotifyThread;  // For VDMTerminateTaskWOW
} SHAREDPROCESS, *LPSHAREDPROCESS;

typedef struct _SHAREDTASK {
    DWORD   dwType;
    DWORD   dwThreadId;
    WORD    hTask16;
    WORD    hMod16;
    DWORD   dwNextTask;             // Offset into shared memory where next task for this process begins
    CHAR    szModName[9];           // null terminated
    CHAR    szFilePath[128];        // null terminated
} SHAREDTASK, *LPSHAREDTASK;

typedef union _SHAREDMEMOBJECT {
    SHAREDPROCESS   sp;
    SHAREDTASK      st;
    DWORD           dwType;
} SHAREDMEMOBJECT, *LPSHAREDMEMOBJECT;

#define SMO_AVAILABLE   0
#define SMO_PROCESS     1
#define SMO_TASK        2

#ifndef SHAREWOW_MAIN
extern HANDLE hSharedWOWMem;
extern LPSHAREDTASKMEM lpSharedWOWMem;
extern CHAR szWowSharedMemName[];
extern HANDLE hSharedWOWMutex;
LPSHAREDTASKMEM LockWowSharedMemory(VOID);
#else
HANDLE hSharedWOWMem = NULL;
LPSHAREDTASKMEM lpSharedWOWMem = NULL;
CHAR szWowSharedMemName[] = "msvdmdbg.wow";
CHAR szWowSharedMutexName[] = "msvdmdbg.mtx";
HANDLE hSharedWOWMutex = NULL;

LPSHAREDTASKMEM LockWowSharedMemory(VOID)
{
    DWORD dwWaitResult;
    LPSHAREDTASKMEM RetVal = NULL;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);  // NULL DACL == wide open

    RtlZeroMemory(&sa, sizeof sa);
    sa.nLength = sizeof sa;
    sa.lpSecurityDescriptor = &sd;

    if (! lpSharedWOWMem ) {

        if (! hSharedWOWMutex) {
            hSharedWOWMutex = CreateMutex(&sa, FALSE, szWowSharedMutexName);  // will open if exists
            if (! hSharedWOWMutex) {
                goto Fail;
            }
        }

        if (! hSharedWOWMem) {

            hSharedWOWMem = CreateFileMapping(          // will open if it exists
                    (HANDLE)-1,
                    &sa,
                    PAGE_READWRITE,
                    0,
                    sizeof(SHAREDTASKMEM) +
                      (MAX_SHARED_OBJECTS *
                       sizeof(SHAREDMEMOBJECT)),
                    szWowSharedMemName);

            if (! hSharedWOWMem) {
                goto Fail;
            }
        }

        lpSharedWOWMem = MapViewOfFile(hSharedWOWMem, FILE_MAP_WRITE, 0, 0, 0);
        if (! lpSharedWOWMem) {
            goto Fail;
        }
    }

    dwWaitResult = WaitForSingleObject(hSharedWOWMutex, INFINITE);

    if (dwWaitResult != WAIT_OBJECT_0 && dwWaitResult != WAIT_ABANDONED) {
        goto Fail;
    }

    RetVal = lpSharedWOWMem;
    goto Succeed;

Fail:
    if (! RetVal) {
        if (hSharedWOWMutex) {
            CloseHandle(hSharedWOWMutex);
            hSharedWOWMutex = NULL;
        }
        if (lpSharedWOWMem) {
            UnmapViewOfFile(lpSharedWOWMem);
            lpSharedWOWMem = NULL;
        }
        if (hSharedWOWMem) {
            CloseHandle(hSharedWOWMem);
            hSharedWOWMem = NULL;
        }
    }

Succeed:
    return RetVal;
}
#endif
