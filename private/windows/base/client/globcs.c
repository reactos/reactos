#include <stdio.h>
#include <windows.h>


//
// Global Critical Sections have two components. One piece is shared between all
// applications using the global lock. This portion will typically reside in some
// sort of shared memory
//
// The second piece is per-process. This contains a per-process handle to the shared
// critical section lock semaphore. The semaphore is itself shared, but each process
// may have a different handle value to the semaphore.
//
// Global critical sections are attached to by name. The application wishing to
// attach must know the name of the critical section (actually the name of the shared
// lock semaphore, and must know the address of the global portion of the critical
// section
//

typedef struct _GLOBAL_SHARED_CRITICAL_SECTION {
    LONG LockCount;
    LONG RecursionCount;
    DWORD OwningThread;
    DWORD Reserved;
} GLOBAL_SHARED_CRITICAL_SECTION, *PGLOBAL_SHARED_CRITICAL_SECTION;

typedef struct _GLOBAL_LOCAL_CRITICAL_SECTION {
    PGLOBAL_SHARED_CRITICAL_SECTION GlobalPortion;
    HANDLE LockSemaphore;
    DWORD Reserved1;
    DWORD Reserved2;
} GLOBAL_LOCAL_CRITICAL_SECTION, *PGLOBAL_LOCAL_CRITICAL_SECTION;


BOOL
WINAPI
AttachToGlobalCriticalSection(
    PGLOBAL_LOCAL_CRITICAL_SECTION lpLocalPortion,
    PGLOBAL_SHARED_CRITICAL_SECTION lpGlobalPortion,
    LPCSTR lpName
    )

/*++

Routine Description:

    This routine attaches to an existing global critical section, or creates and
    initializes the global critical section if it does not already exist.

Arguments:

    lpLocalPortion - Supplies the address of a per-app local portion of the global
        critical section.

    lpGlobalPortion - Supplies the address of the global shared portion of the
        critical section. If the critical section is new, the caller will initialize it.

    lpName - Supplies the name of the critical section.  If an existing
        critical section with this name already exists, then it is not
        reinitialized.  In this case, the caller simply attaches to it.

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed.

--*/

{

    HANDLE GlobalMutex;
    HANDLE LockSemaphore;
    BOOL rv;
    DWORD WaitResult;

    //
    // Serialize all global critical section initialization
    //

    GlobalMutex = CreateMutex(NULL,TRUE,"GlobalCsMutex");

    //
    // If the mutex create/open failed, then bail
    //

    if ( !GlobalMutex ) {
        return FALSE;
        }

    if ( GetLastError() == ERROR_ALREADY_EXISTS ) {

        //
        // Since the mutex already existed, the request for ownership has no effect.
        // wait for the mutex
        //

        WaitResult = WaitForSingleObject(GlobalMutex,INFINITE);
        if ( WaitResult == WAIT_FAILED ) {
            CloseHandle(GlobalMutex);
            return FALSE;
            }
        }

    //
    // We now own the global critical section creation mutex. Create/Open the
    // named semaphore. If we are the creator, then initialize the critical
    // section. Otherwise just point to it. The global critical section creation
    // allows us to do this safely.
    //

    rv = FALSE;
    LockSemaphore = NULL;
    try {
        LockSemaphore = CreateSemaphore(NULL,0,MAXLONG-1,lpName);

        //
        // If the semaphore create/open failed, then bail
        //

        if ( !GlobalMutex ) {
            rv = FALSE;
            goto finallyexit;
            }

        //
        // See if we attached to the semaphore, or if we created it. If we created it,
        // then we need to init the global structure.
        //

        if ( GetLastError() != ERROR_ALREADY_EXISTS ) {

            //
            // We Created the semaphore, so init the global portion.
            //

            lpGlobalPortion->LockCount = -1;
            lpGlobalPortion->RecursionCount = 0;
            lpGlobalPortion->OwningThread = 0;
            lpGlobalPortion->Reserved = 0;
            }

        lpLocalPortion->LockSemaphore = LockSemaphore;
        LockSemaphore = NULL;
        lpLocalPortion->GlobalPortion = lpGlobalPortion;
        lpLocalPortion->Reserved1 = 0;
        lpLocalPortion->Reserved2 = 0;
        rv = TRUE;
finallyexit:;
        }
    finally {
        ReleaseMutex(GlobalMutex);
        CloseHandle(GlobalMutex);
        if ( LockSemaphore ) {
            CloseHandle(LockSemaphore);
            }
        }

    return rv;
}

BOOL
WINAPI
DetachFromGlobalCriticalSection(
    PGLOBAL_LOCAL_CRITICAL_SECTION lpLocalPortion
    )

/*++

Routine Description:

    This routine detaches from an existing global critical section.

Arguments:

    lpLocalPortion - Supplies the address of a per-app local portion of the global
        critical section.

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed.

--*/

{

    HANDLE LockSemaphore;
    HANDLE GlobalMutex;
    DWORD WaitResult;
    BOOL rv;


    //
    // Serialize all global critical section initialization
    //

    GlobalMutex = CreateMutex(NULL,TRUE,"GlobalCsMutex");

    //
    // If the mutex create/open failed, then bail
    //

    if ( !GlobalMutex ) {
        return FALSE;
        }

    if ( GetLastError() == ERROR_ALREADY_EXISTS ) {

        //
        // Since the mutex already existed, the request for ownership has no effect.
        // wait for the mutex
        //

        WaitResult = WaitForSingleObject(GlobalMutex,INFINITE);
        if ( WaitResult == WAIT_FAILED ) {
            CloseHandle(GlobalMutex);
            return FALSE;
            }
        }
    LockSemaphore = NULL;
    rv = FALSE;
    try {
        LockSemaphore = lpLocalPortion->LockSemaphore;
        ZeroMemory(lpLocalPortion,sizeof(*lpLocalPortion));
        rv = TRUE;
        }
    finally {
        if ( LockSemaphore ) {
            CloseHandle(LockSemaphore);
            }
        ReleaseMutex(GlobalMutex);
        CloseHandle(GlobalMutex);
        }
    return rv;
}

VOID
WINAPI
EnterGlobalCriticalSection(
    PGLOBAL_LOCAL_CRITICAL_SECTION lpLocalPortion
    )
{
    PGLOBAL_SHARED_CRITICAL_SECTION GlobalPortion;
    DWORD ThreadId;
    LONG IncResult;
    DWORD WaitResult;

    ThreadId = GetCurrentThreadId();
    GlobalPortion = lpLocalPortion->GlobalPortion;

    //
    // Increment the lock variable. On the transition to 0, the caller
    // becomes the absolute owner of the lock. Otherwise, the caller is
    // either recursing, or is going to have to wait
    //

    IncResult = InterlockedIncrement(&GlobalPortion->LockCount);
    if ( !IncResult ) {

        //
        // lock count went from 0 to 1, so the caller
        // is the owner of the lock
        //

        GlobalPortion->RecursionCount = 1;
        GlobalPortion->OwningThread = ThreadId;
        }
    else {

        //
        // If the caller is recursing, then increment the recursion count
        //

        if ( GlobalPortion->OwningThread == ThreadId ) {
            GlobalPortion->RecursionCount++;
            }
        else {
            WaitResult = WaitForSingleObject(lpLocalPortion->LockSemaphore,INFINITE);
            if ( WaitResult == WAIT_FAILED ) {
                RaiseException(GetLastError(),0,0,NULL);
                }
            GlobalPortion->RecursionCount = 1;
            GlobalPortion->OwningThread = ThreadId;
            }
        }
}

VOID
WINAPI
LeaveGlobalCriticalSection(
    PGLOBAL_LOCAL_CRITICAL_SECTION lpLocalPortion
    )
{
    PGLOBAL_SHARED_CRITICAL_SECTION GlobalPortion;
    LONG DecResult;

    GlobalPortion = lpLocalPortion->GlobalPortion;


    //
    // decrement the recursion count. If it is still non-zero, then
    // we are still the owner so don't do anything other than dec the lock
    // count
    //

    if (--GlobalPortion->RecursionCount) {
        InterlockedDecrement(&GlobalPortion->LockCount);
        }
    else {

        //
        // We are really leaving, so give up ownership and decrement the
        // lock count
        //

        GlobalPortion->OwningThread = 0;
        DecResult = InterlockedDecrement(&GlobalPortion->LockCount);

        //
        // Check to see if there are other waiters. If so, then wake up a waiter
        //

        if ( DecResult >= 0 ) {
            ReleaseSemaphore(lpLocalPortion->LockSemaphore,1,NULL);
            }

        }
}

GLOBAL_LOCAL_CRITICAL_SECTION LocalPortion;

int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    HANDLE hFileMap;
    LPVOID SharedMem;
    BOOL b;
    int i;
    DWORD Start,End;
    HANDLE Mutex1;

    //
    // open or create a shared file mapping object
    //

    hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,1024,"MyMem");

    if ( !hFileMap ) {
        printf("create file map failed\n");
        ExitProcess(1);
        }

    SharedMem = MapViewOfFile(hFileMap,FILE_MAP_WRITE,0,0,0);

    if ( !SharedMem ) {
        printf("map view failed\n");
        ExitProcess(1);
        }

    b = AttachToGlobalCriticalSection(&LocalPortion,SharedMem,"MyGlobalCs");

    if ( !b ) {
        printf("attach failed\n");
        ExitProcess(1);
        }

    if ( argc > 1 ) {

        for(i=0;i<30;i++){
            EnterGlobalCriticalSection(&LocalPortion);
            printf("Thread %x is in\n",GetCurrentThreadId());
            Sleep(500);
            LeaveGlobalCriticalSection(&LocalPortion);
            }
        }

    Start = GetTickCount();
    for(i=0;i<1000000;i++){
        EnterGlobalCriticalSection(&LocalPortion);
        LeaveGlobalCriticalSection(&LocalPortion);
        }
    End = GetTickCount();
    printf("Global CS Time %dms\n",End-Start);

    Mutex1 = CreateMutex(NULL,FALSE,NULL);
    Start = GetTickCount();
    for(i=0;i<100000;i++){
        WaitForSingleObject(Mutex1,INFINITE);
        ReleaseMutex(Mutex1);
        }
    End = GetTickCount();
    printf("Mutex Time     %dms\n",(End-Start)*10);

    DetachFromGlobalCriticalSection(&LocalPortion);
}
