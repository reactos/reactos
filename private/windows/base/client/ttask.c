/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ttask.c

Abstract:

    Test program for Win32 Base File API calls

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>

typedef struct _CMDSHOW {
    WORD wMustBe2;
    WORD wShowWindowValue;
} CMDSHOW, *PCMDSHOW;

typedef struct _LOAD_MODULE_PARAMS {
    LPSTR lpEnvAddress;
    LPSTR lpCmdLine;
    PCMDSHOW lpCmdShow;
    DWORD dwReserved;
} LOAD_MODULE_PARAMS, *PLOAD_MODULE_PARAMS;

HANDLE Event1, Event2;

VOID
WaitTestThread(
    LPVOID ThreadParameter
    )
{
    DWORD st;
    printf("In Test Thread... Parameter %ld\n",ThreadParameter);

    assert(SetEvent(Event1));

    st = WaitForSingleObject(Event2,-1);
    assert(st == 0);

    printf("Test Thread Exiting... Parameter %ld\n",ThreadParameter);

    ExitThread((DWORD)ThreadParameter);
}

VOID
TestThread(
    LPVOID ThreadParameter
    )
{
    LPSTR s;
    SYSTEMTIME DateAndTime;
    CHAR ImageName[256];
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;

    printf("In Test Thread... Parameter %ld\n",ThreadParameter);
    GetSystemTime(&DateAndTime);
    printf("%d/%d/%d @ %d:%d.%d\n",
        DateAndTime.wMonth,
        DateAndTime.wDay,
        DateAndTime.wYear,
        DateAndTime.wHour,
        DateAndTime.wMinute,
        DateAndTime.wSecond
        );

    DateAndTime.wMonth = 3;
    DateAndTime.wDay = 23;
    DateAndTime.wYear = 1961;
    DateAndTime.wHour = 7;
    DateAndTime.wMinute = 31;
    DateAndTime.wSecond = 0;

#if 0
    assert(SetSystemTime(&DateAndTime));
    GetSystemTime(&DateAndTime);

    assert(DateAndTime.wMonth == 3);
    assert(DateAndTime.wDay == 23);
    assert(DateAndTime.wYear == 1961);
    assert(DateAndTime.wHour == 7);

    DateAndTime.wMonth = 13;
    assert(!SetSystemTime(&DateAndTime));

    printf("%s\n",GetCommandLine());

    assert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST));
    assert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_HIGHEST);
    assert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_LOWEST));
    assert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_LOWEST);
    assert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL));
    assert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_ABOVE_NORMAL);
    assert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL));
    assert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_BELOW_NORMAL);
    assert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL));
    assert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_NORMAL);
    assert(!SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST+1));
    assert(GetThreadPriority(GetCurrentProcess()) == THREAD_PRIORITY_ERROR_RETURN);

    assert(GetModuleFileName(0,ImageName,256) < 255);

    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpReserved = "UsedByShell";
    StartupInfo.lpDesktop = "MarksDesktop";
    StartupInfo.lpTitle = "MarksTestTitle";
    StartupInfo.dwX = 0;
    StartupInfo.dwY = 1;
    StartupInfo.dwXSize = 10;
    StartupInfo.dwYSize = 10;
    StartupInfo.dwFlags = 0;//STARTF_SHELLOVERRIDE;
    StartupInfo.wShowWindow = 0;//SW_SHOWDEFAULT;
    StartupInfo.lpReserved2 = 0;
    StartupInfo.cbReserved2 = 0;

    assert( CreateProcess(
                NULL,
                "ttask +",
                NULL,
                NULL,
                TRUE,
                0,
                NULL,
                NULL,
                &StartupInfo,
                &ProcessInformation
                ) );
    WaitForSingleObject(ProcessInformation.hProcess,-1);
#endif
    ExitThread((DWORD)ThreadParameter);
}


DWORD
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    CRITICAL_SECTION Crit;
    HANDLE Event, Semaphore, Mutex, Thread, Process;
    HANDLE NEvent, NSemaphore, NMutex;
    HANDLE OEvent, OSemaphore, OMutex;
    DWORD st;
    DWORD ThreadId;
    CHAR ImageName[256];
    CHAR CommandLine[256];
    CHAR Environment[256];
    CMDSHOW cs;
    LOAD_MODULE_PARAMS lmp;
    LPSTR *s;
    int i;
    DWORD psp;

    (VOID)envp;

    try {
        RaiseException(4,0,0,NULL);
        }
    except(EXCEPTION_EXECUTE_HANDLER){
        printf("In Handler %lx\n",GetExceptionCode());
    }

    i = 0;
    s = argv;
    while(i < argc) {
        printf("argv[%ld] %s\n",i,*s);
        i++;
        s++;
        }
#if 0
    printf("TTASK CommandLine %s\n",GetCommandLine());
    if ( strchr(GetCommandLine(),'+') ) {
        printf("TTASK CommandLine %s\n",GetCommandLine());
        return 1;
        }
    Process=OpenProcess(PROCESS_ALL_ACCESS,FALSE,GetCurrentProcessId());
    assert(Process);
    assert(GetModuleFileName(0,ImageName,256) < 255);
    assert(GetModuleFileName(0,CommandLine,256) < 255);
    strcat(CommandLine," -- + --");

    assert(WinExec(CommandLine,0) == 32);

    lmp.lpEnvAddress = Environment;
    lmp.lpCmdLine = CommandLine;
    lmp.dwReserved = 0;
    lmp.lpCmdShow = &cs;
    cs.wMustBe2 = 2;
    cs.wShowWindowValue = 3;

    RtlFillMemory(Environment,256,'\0');
    strcpy(Environment,"PATH=C:\\FOOBAR;C:\\NT\\DLL");
    strcpy(&Environment[strlen("PATH=C:\\FOOBAR;C:\\NT\\DLL")+1],"XYZZY=X");

    assert(LoadModule(ImageName,&lmp) == 32);
#endif
    InitializeCriticalSection(&Crit);
    Event = CreateEvent(NULL,TRUE,TRUE,NULL);
    Semaphore = CreateSemaphore(NULL,1,256,NULL);
    Mutex = CreateMutex(NULL,FALSE,NULL);

    assert(Event);
    assert(Semaphore);
    assert(Mutex);

    NEvent = CreateEvent(NULL,TRUE,TRUE,"named-event");
    NSemaphore = CreateSemaphore(NULL,1,256,"named-semaphore");
    NMutex = CreateMutex(NULL,FALSE,"named-mutex");

    assert(NEvent);
    assert(NSemaphore);
    assert(NMutex);

    OEvent = OpenEvent(EVENT_ALL_ACCESS,FALSE,"named-event");
    OSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS,FALSE,"named-semaphore");
    OMutex = OpenMutex(MUTEX_ALL_ACCESS,FALSE,"named-mutex");

    assert(OEvent);
    assert(OSemaphore);
    assert(OMutex);

    EnterCriticalSection(&Crit);
    LeaveCriticalSection(&Crit);

    st = WaitForSingleObject(Event,-1);
    assert(st == 0);

    st = WaitForSingleObject(Semaphore,-1);
    assert(st == 0);

    st = WaitForSingleObject(Semaphore,0);
    assert(st == WAIT_TIMEOUT);

    assert(ReleaseSemaphore(Semaphore,1,NULL));

    st = WaitForSingleObject(Mutex,-1);
    assert(st == 0);

    assert(ReleaseMutex(Mutex));

    st = WaitForSingleObject(OEvent,-1);
    assert(st == 0);

    st = WaitForSingleObject(OSemaphore,-1);
    assert(st == 0);

    st = WaitForSingleObject(NSemaphore,0);
    assert(st == WAIT_TIMEOUT);

    assert(ReleaseSemaphore(NSemaphore,1,NULL));

    st = WaitForSingleObject(OMutex,-1);
    assert(st == 0);

    assert(ReleaseMutex(NMutex));

    Thread = CreateThread(NULL,0L,TestThread,(LPVOID)99,0,&ThreadId);
    assert(Thread);

    st = WaitForSingleObject(Thread,-1);
    assert(st == 0);

    assert(GetExitCodeThread(Thread,&st));
    assert(st = 99);

    CloseHandle(Thread);

    Event1 = CreateEvent(NULL,TRUE,FALSE,NULL);
    Event2 = CreateEvent(NULL,TRUE,FALSE,NULL);

    Thread = CreateThread(NULL,0L,WaitTestThread,(LPVOID)99,0,&ThreadId);
    assert(Thread);

    st = WaitForSingleObject(Event1,-1);
    assert(st == 0);

    //
    // thread should now be waiting on event2
    //

    psp = SuspendThread(Thread);
    assert(psp==0);

    assert(SetEvent(Event2));

    psp = SuspendThread(Thread);
    assert(psp==1);

    psp = ResumeThread(Thread);
    assert(psp==2);

    psp = ResumeThread(Thread);
    assert(psp==1);

    st = WaitForSingleObject(Thread,-1);
    assert(st == 0);

    assert(GetExitCodeThread(Thread,&st));
    assert(st = 99);

    CloseHandle(Thread);

    return 1;
}
