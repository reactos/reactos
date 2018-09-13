/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tfile.c

Abstract:

    Test program for Win32 Base File API calls

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <memory.h>
#include <process.h>

#define xassert ASSERT
int izero;
int i,j;
#define BASESPIN 1000000

#define NULL_SERVER_SWITCHES 10000
#define PATH_CONVERSION_TEST 1000

//
// Define local types.
//

typedef struct _PERFINFO {
    LARGE_INTEGER StartTime;
    LARGE_INTEGER StopTime;
    ULONG ContextSwitches;
    ULONG FirstLevelFills;
    ULONG SecondLevelFills;
    ULONG SystemCalls;
    PCHAR Title;
    ULONG Iterations;
} PERFINFO, *PPERFINFO;

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    )

{

    ULONG ContextSwitches;
    LARGE_INTEGER Duration;
    ULONG FirstLevelFills;
    ULONG Length;
    ULONG Performance;
    ULONG SecondLevelFills;
    NTSTATUS Status;
    ULONG SystemCalls;
    SYSTEM_PERFORMANCE_INFORMATION SystemInfo;


    //
    // Print results and announce end of test.
    //

    NtQuerySystemTime((PLARGE_INTEGER)&PerfInfo->StopTime);
    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      (PVOID)&SystemInfo,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to query performance information, status = %lx\n", Status);
        return;
    }

    Duration = RtlLargeIntegerSubtract(PerfInfo->StopTime, PerfInfo->StartTime);
    Length = Duration.LowPart / 10000;
    printf("        Test time in milliseconds %d\n", Length);
    printf("        Number of iterations      %d\n", PerfInfo->Iterations);

    Performance = PerfInfo->Iterations * 1000 / Length;
    printf("        Iterations per second     %d\n", Performance);

    ContextSwitches = SystemInfo.ContextSwitches - PerfInfo->ContextSwitches;
    FirstLevelFills = SystemInfo.FirstLevelTbFills - PerfInfo->FirstLevelFills;
    SecondLevelFills = SystemInfo.SecondLevelTbFills - PerfInfo->SecondLevelFills;
    SystemCalls = SystemInfo.SystemCalls - PerfInfo->SystemCalls;
    printf("        First Level TB Fills      %d\n", FirstLevelFills);
    printf("        Second Level TB Fills     %d\n", SecondLevelFills);
    printf("        Total Context Switches    %d\n", ContextSwitches);
    printf("        Number of System Calls    %d\n", SystemCalls);

    printf("*** End of Test ***\n\n");
    return;
}

VOID
StartBenchMark (
    IN PCHAR Title,
    IN ULONG Iterations,
    IN PPERFINFO PerfInfo
    )

{

    NTSTATUS Status;
    SYSTEM_PERFORMANCE_INFORMATION SystemInfo;

    //
    // Announce start of test and the number of iterations.
    //

    printf("*** Start of test ***\n    %s\n", Title);
    PerfInfo->Title = Title;
    PerfInfo->Iterations = Iterations;
    NtQuerySystemTime((PLARGE_INTEGER)&PerfInfo->StartTime);
    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      (PVOID)&SystemInfo,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to query performance information, status = %lx\n", Status);
        return;
    }

    PerfInfo->ContextSwitches = SystemInfo.ContextSwitches;
    PerfInfo->FirstLevelFills = SystemInfo.FirstLevelTbFills;
    PerfInfo->SecondLevelFills = SystemInfo.SecondLevelTbFills;
    PerfInfo->SystemCalls = SystemInfo.SystemCalls;
    return;
}

VOID
ScrollTest()
{
    COORD dest,cp;
    SMALL_RECT Sm;
    CHAR_INFO ci;
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    HANDLE ScreenHandle;
    SMALL_RECT Window;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbi);

    Window.Left = 0;
    Window.Top = 0;
    Window.Right = 79;
    Window.Bottom = 49;

    dest.X = 0;
    dest.Y = 0;

    ci.Char.AsciiChar = ' ';
    ci.Attributes = sbi.wAttributes;

    SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE),
                         TRUE,
                         &Window);

    cp.X = 0;
    cp.Y = 0;

    Sm.Left      = 0;
    Sm.Top       = 1;
    Sm.Right     = 79;
    Sm.Bottom    = 49;

    ScrollConsoleScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE),
                              &Sm,
                              NULL,
                              dest,
                              &ci);

}






VOID
WinWordOpenFileTest()
{
    PERFINFO PerfInfo;
    ULONG Index;
    OFSTRUCT ofstr;
    HANDLE iFile;

    StartBenchMark("WinWord OpenFile)",
                   3,
                   &PerfInfo);

    for ( Index=0;Index<3;Index++){
        iFile = (HANDLE)OpenFile("foo",&ofstr, OF_PARSE);
        iFile = (HANDLE)OpenFile("E:\\xxxxxxx\\winword.ini",&ofstr, 0x20);
        iFile = (HANDLE)OpenFile("E:\\xxxxxxx\\perftest.doc",&ofstr, 0x22);
        iFile = (HANDLE)OpenFile("E:foo",&ofstr, OF_PARSE);
        iFile = (HANDLE)OpenFile("E:foo",&ofstr, OF_PARSE);
        iFile = (HANDLE)OpenFile("E:foo",&ofstr, OF_PARSE);
        iFile = (HANDLE)OpenFile("E:foo",&ofstr, OF_PARSE);
        iFile = (HANDLE)OpenFile("E:\\xxxxxxx\\custom.dic",&ofstr, 0x4022 );
        iFile = (HANDLE)OpenFile("E:\\xxxxxxx\\sp_am.exc",&ofstr, 0x4040 );
        iFile = (HANDLE)OpenFile("E:foo",&ofstr, OF_PARSE);
        iFile = (HANDLE)OpenFile("foo",&ofstr, OF_PARSE);
        iFile = (HANDLE)OpenFile("E:~doc3d08.tmp",&ofstr, 0x1022);
        iFile = (HANDLE)OpenFile("E:\\xxxxxxx\\tempx.doc",&ofstr, 0xa022 );
        iFile = (HANDLE)OpenFile("E:\\xxxxxxx\\~$rftest.doc",&ofstr, 0x4012 );
        iFile = (HANDLE)OpenFile("foo",&ofstr, OF_PARSE);
        iFile = (HANDLE)OpenFile("E:~doc391f.tmp",&ofstr, 0x1022);
        iFile = (HANDLE)OpenFile("E:\\xxxxxxx\\tempy.doc",&ofstr, 0xa022 );
        iFile = (HANDLE)OpenFile("E:\\xxxxxxx\\winword.ini",&ofstr, 0x12);
    }

    FinishBenchMark(&PerfInfo);
}

VOID
gettictst(int x)
{
    PERFINFO PerfInfo;
    ULONG i,j;
    ULONG tnt,tw32;

    if ( !x ) {
        StartBenchMark("NtGetTickCount)",
                       100000,
                       &PerfInfo);
        for ( i=0;i<100000;i++){
            j = GetTickCount();
        }

        FinishBenchMark(&PerfInfo);

        }
    else {
        while(1)GetTickCount();
        }
}

VOID
latst()
{
    PERFINFO PerfInfo;
    ULONG i,j;
    HANDLE h1, h2, h3, h4, h5;

    StartBenchMark("LocalAlloc/Free)",
                   200,
                   &PerfInfo);
    for ( i=0;i<200/5;i++){
        h1 = LocalAlloc(0, 500);
        h2 = LocalAlloc(0, 600);
        h3 = LocalAlloc(0, 700);
        LocalFree(h2);
        h4 = LocalAlloc(0, 1000);
        h5 = LocalAlloc(0, 100);
        LocalFree(h1);
        LocalFree(h3);
        LocalFree(h4);
        LocalFree(h5);
    }

    FinishBenchMark(&PerfInfo);

}

VOID
WinWordGetDriveTypeTest()
{
    PERFINFO PerfInfo;
    ULONG Index,Reps;
    OFSTRUCT ofstr;
    HANDLE iFile;
    CHAR DiskName[4];
    WCHAR WDiskName[4];

//    StartBenchMark("WinWord GetDriveType (1-26)",
//                   26,
//                   &PerfInfo);
//
//    for ( Index=1;Index<27;Index++){
//        GetDriveType(Index);
//    }
//
//    FinishBenchMark(&PerfInfo);

    DiskName[0]='a';
    DiskName[1]=':';
    DiskName[2]='\\';
    DiskName[3]='\0';
    StartBenchMark("WinWord GetDriveTypeA (a-z)",
                   100,
                   &PerfInfo);

    for(Reps=0;Reps<100;Reps++){
        for ( Index=0;Index<26;Index++){
            DiskName[0]='a'+Index;
            GetDriveTypeA(DiskName);
            }
        }

    FinishBenchMark(&PerfInfo);

    WDiskName[0]=(WCHAR)'a';
    WDiskName[1]=(WCHAR)':';
    WDiskName[2]=(WCHAR)'\\';
    WDiskName[3]=(WCHAR)'\0';
    StartBenchMark("WinWord GetDriveTypeW (a-z)",
                   100,
                   &PerfInfo);

    for(Reps=0;Reps<100;Reps++){
        for ( Index=0;Index<26;Index++){
            WDiskName[0]=(WCHAR)'a'+Index;
            GetDriveTypeW(WDiskName);
            }
        }

    FinishBenchMark(&PerfInfo);
}

VOID
BogusOrdinalTest()
{
    HANDLE hBase;
    FARPROC z;

    WaitForSingleObject(0,-2);
    hBase = GetModuleHandle("base");
    xassert(hBase);
    z = GetProcAddress(hBase,0x00001345);
}


VOID
NullServerSwitchTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    NTSTATUS Status;
    ULONG Index;

    StartBenchMark("Null Server Call Benchmark)",
                   NULL_SERVER_SWITCHES,
                   &PerfInfo);


    for (Index = 0; Index < NULL_SERVER_SWITCHES; Index += 1) {
        CsrIdentifyAlertableThread();
    }
    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    return;
}

VOID
PathConvertTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    NTSTATUS Status;
    ULONG Index;
    UNICODE_STRING FileName;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;

    StartBenchMark("Path Conversion Test (foo)",
                   PATH_CONVERSION_TEST,
                   &PerfInfo);


    for (Index = 0; Index < PATH_CONVERSION_TEST; Index += 1) {
        RtlDosPathNameToNtPathName_U(
            L"foo",
            &FileName,
            NULL,
            &RelativeName
            );
        RtlFreeHeap(RtlProcessHeap(),FileName.Buffer);
    }
    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    StartBenchMark("Path Conversion Test (e:\\nt\\windows\\foo)",
                   PATH_CONVERSION_TEST,
                   &PerfInfo);


    for (Index = 0; Index < PATH_CONVERSION_TEST; Index += 1) {
        RtlDosPathNameToNtPathName_U(
            L"e:\\nt\\windows\\foo",
            &FileName,
            NULL,
            &RelativeName
            );
        RtlFreeHeap(RtlProcessHeap(),FileName.Buffer);
    }
    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    return;
}

z(){}

bar()
{
    for (i=0;i<2*BASESPIN;i++)j = i++;
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
}
foo()
{
    for (i=0;i<BASESPIN;i++)j = i++;
    bar();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
}
proftst()
{
    for (i=0;i<BASESPIN;i++)j = i++;
    foo();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
    z();
}

VOID
probtst(
    VOID
    )
{
    LPVOID ReadOnly;
    LPVOID ReadWrite;
    LPVOID ReadWrite2;
    LPVOID NoReadWrite;
    LPVOID MappedReadWrite;
    LPVOID p;
    HANDLE MappedFile;
    LPSTR l;
    LPWSTR w;
    BOOL b;

    ReadOnly = VirtualAlloc(NULL,4096,MEM_COMMIT,PAGE_READONLY);
    ASSERT(ReadOnly);

    ASSERT(!IsBadReadPtr(ReadOnly,1024));
    ASSERT(!IsBadReadPtr(ReadOnly,4096));
    ASSERT(IsBadReadPtr(ReadOnly,4097));
    ASSERT(!IsBadHugeReadPtr(ReadOnly,1024));
    ASSERT(!IsBadHugeReadPtr(ReadOnly,4096));
    ASSERT(IsBadHugeReadPtr(ReadOnly,4097));

    ASSERT(IsBadWritePtr(ReadOnly,1024));
    ASSERT(IsBadWritePtr(ReadOnly,4096));
    ASSERT(IsBadWritePtr(ReadOnly,4097));
    ASSERT(IsBadHugeWritePtr(ReadOnly,1024));
    ASSERT(IsBadHugeWritePtr(ReadOnly,4096));
    ASSERT(IsBadHugeWritePtr(ReadOnly,4097));

    ReadWrite = VirtualAlloc(NULL,4096,MEM_COMMIT,PAGE_READWRITE);
    ASSERT(ReadWrite);

    ASSERT(!IsBadReadPtr(ReadWrite,1024));
    ASSERT(!IsBadReadPtr(ReadWrite,4096));
    ASSERT(IsBadReadPtr(ReadWrite,4097));
    ASSERT(!IsBadHugeReadPtr(ReadWrite,1024));
    ASSERT(!IsBadHugeReadPtr(ReadWrite,4096));
    ASSERT(IsBadHugeReadPtr(ReadWrite,4097));

    ASSERT(!IsBadWritePtr(ReadWrite,1024));
    ASSERT(!IsBadWritePtr(ReadWrite,4096));
    ASSERT(IsBadWritePtr(ReadWrite,4097));
    ASSERT(!IsBadHugeWritePtr(ReadWrite,1024));
    ASSERT(!IsBadHugeWritePtr(ReadWrite,4096));
    ASSERT(IsBadHugeWritePtr(ReadWrite,4097));

    NoReadWrite = VirtualAlloc(NULL,4096,MEM_COMMIT,PAGE_NOACCESS);
    ASSERT(NoReadWrite);

    ASSERT(IsBadReadPtr(NoReadWrite,1024));
    ASSERT(IsBadReadPtr(NoReadWrite,4096));
    ASSERT(IsBadReadPtr(NoReadWrite,4097));
    ASSERT(IsBadHugeReadPtr(NoReadWrite,1024));
    ASSERT(IsBadHugeReadPtr(NoReadWrite,4096));
    ASSERT(IsBadHugeReadPtr(NoReadWrite,4097));

    ASSERT(IsBadWritePtr(NoReadWrite,1024));
    ASSERT(IsBadWritePtr(NoReadWrite,4096));
    ASSERT(IsBadWritePtr(NoReadWrite,4097));
    ASSERT(IsBadHugeWritePtr(NoReadWrite,1024));
    ASSERT(IsBadHugeWritePtr(NoReadWrite,4096));
    ASSERT(IsBadHugeWritePtr(NoReadWrite,4097));

    l = ReadWrite;
    l[4092]='a';
    l[4093]='b';
    l[4094]='c';
    l[4095]='\0';
    ASSERT(!IsBadStringPtrA(&l[4092],2));
    ASSERT(!IsBadStringPtrA(&l[4092],3));
    ASSERT(!IsBadStringPtrA(&l[4092],4));
    ASSERT(!IsBadStringPtrA(&l[4092],5));
    l[4095]='d';
    ASSERT(!IsBadStringPtrA(&l[4092],2));
    ASSERT(!IsBadStringPtrA(&l[4092],3));
    ASSERT(!IsBadStringPtrA(&l[4092],4));
    ASSERT(IsBadStringPtrA(&l[4092],5));

    w = ReadWrite;
    w[2044]=(WCHAR)'a';
    w[2045]=(WCHAR)'b';
    w[2046]=(WCHAR)'c';
    w[2047]=UNICODE_NULL;
    ASSERT(!IsBadStringPtrW(&w[2044],2));
    ASSERT(!IsBadStringPtrW(&w[2044],3));
    ASSERT(!IsBadStringPtrW(&w[2044],4));
    ASSERT(!IsBadStringPtrW(&w[2044],5));
    w[2047]=(WCHAR)'d';
    ASSERT(!IsBadStringPtrW(&w[2044],2));
    ASSERT(!IsBadStringPtrW(&w[2044],3));
    ASSERT(!IsBadStringPtrW(&w[2044],4));
    ASSERT(IsBadStringPtrW(&w[2044],5));

    ReadWrite2 = VirtualAlloc(NULL,4096,MEM_COMMIT,PAGE_READWRITE);
    ASSERT(ReadWrite2);

    ASSERT(VirtualLock(ReadWrite2,4096));
    ASSERT(VirtualUnlock(ReadWrite2,4));
    ASSERT(!VirtualUnlock(ReadWrite2,4));
    ASSERT(!VirtualLock(ReadWrite2,4097));
    ASSERT(!VirtualUnlock(ReadWrite2,4097));
    ASSERT(VirtualLock(ReadWrite2,4096));
    ASSERT(VirtualUnlock(ReadWrite2,4096));
    ASSERT(!VirtualUnlock(ReadWrite2,4096));

    MappedFile = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,8192,NULL);
    ASSERT(MappedFile);
    MappedReadWrite = MapViewOfFileEx(MappedFile,FILE_MAP_WRITE,0,0,0,(LPVOID)0x50000000);
    ASSERT(MappedReadWrite);

    p = MapViewOfFileEx(MappedFile,FILE_MAP_WRITE,0,0,0,(LPVOID)GetModuleHandle(NULL));
    ASSERT(!p);

    ASSERT(SetPriorityClass(GetCurrentProcess(),IDLE_PRIORITY_CLASS));
    ASSERT(GetPriorityClass(GetCurrentProcess()) == IDLE_PRIORITY_CLASS);

    ASSERT(SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS));
    ASSERT(GetPriorityClass(GetCurrentProcess()) == NORMAL_PRIORITY_CLASS);

    ASSERT(SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS));
    ASSERT(GetPriorityClass(GetCurrentProcess()) == HIGH_PRIORITY_CLASS);

    ASSERT(SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS));
    ASSERT(GetPriorityClass(GetCurrentProcess()) == NORMAL_PRIORITY_CLASS);

}


void
notifytst()
{
    HANDLE nHandle;
    DWORD wret;
    HANDLE fFile;
    WIN32_FIND_DATA FindFileData;
    int n;
    BOOL b;

    fFile =  FindFirstFile(
                "c:\\*.*",
                &FindFileData
                );
    xassert(fFile != INVALID_HANDLE_VALUE);
    n = 0;
    b = TRUE;
    while(b) {
        n++;
        b = FindNextFile(fFile,&FindFileData);
        }
    FindClose(fFile);
    printf("%d files\n",n);

    nHandle = FindFirstChangeNotification(
                "C:\\",
                TRUE,
                FILE_NOTIFY_CHANGE_NAME
                );
    xassert(nHandle != INVALID_HANDLE_VALUE);

    wret = WaitForSingleObject(nHandle,-1);
    xassert(wret == 0);

    fFile =  FindFirstFile(
                "c:\\*.*",
                &FindFileData
                );
    xassert(fFile != INVALID_HANDLE_VALUE);
    n = 0;
    b = TRUE;
    while(b) {
        n++;
        b = FindNextFile(fFile,&FindFileData);
        }
    FindClose(fFile);
    printf("%d files\n",n);

    b = FindNextChangeNotification(nHandle);
    xassert(b);

    wret = WaitForSingleObject(nHandle,-1);
    xassert(wret == 0);

    fFile =  FindFirstFile(
                "c:\\*.*",
                &FindFileData
                );
    xassert(fFile != INVALID_HANDLE_VALUE);
    n = 0;
    b = TRUE;
    while(b) {
        n++;
        b = FindNextFile(fFile,&FindFileData);
        }
    FindClose(fFile);
    printf("%d files\n",n);

    xassert(FindCloseChangeNotification(nHandle));
    xassert(!FindCloseChangeNotification(nHandle));
}

void
openiftst()
{
    HANDLE NEvent, NSemaphore, NMutex;
    HANDLE sEvent, sSemaphore, sMutex;

    NEvent = CreateEvent(NULL,TRUE,TRUE,"named-event");
    xassert(NEvent);
    xassert(GetLastError()==0);
    sEvent = CreateEvent(NULL,TRUE,TRUE,"named-event");
    xassert(sEvent);
    xassert(GetLastError()==ERROR_ALREADY_EXISTS);
    NSemaphore = CreateSemaphore(NULL,1,256,"named-event");

    NSemaphore = CreateSemaphore(NULL,1,256,"named-semaphore");
    xassert(NSemaphore);
    xassert(GetLastError()==0);
    sSemaphore = CreateSemaphore(NULL,1,256,"named-semaphore");
    xassert(sSemaphore);
    xassert(GetLastError()==ERROR_ALREADY_EXISTS);

    NMutex = CreateMutex(NULL,FALSE,"named-mutex");
    xassert(NMutex);
    xassert(GetLastError()==0);
    sMutex = CreateMutex(NULL,FALSE,"named-mutex");
    xassert(sMutex);
    xassert(GetLastError()==ERROR_ALREADY_EXISTS);

}

void
NewRip(int flag, LPSTR str)
{
    DWORD ExceptionArguments[3];
    try {
        ExceptionArguments[0]=strlen(str);
        ExceptionArguments[1]=(DWORD)str;
        ExceptionArguments[2]=(DWORD)flag;
        RaiseException(0x0eab7190,0,3,ExceptionArguments);
        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        ;
        }
}

void
Ofprompt()
{
    HFILE h;
    OFSTRUCT of;

    SetErrorMode(SEM_NOOPENFILEERRORBOX);
    h = OpenFile("e:\\nt\\xt.cfg",&of,OF_PROMPT);
    printf("OpenFile(e:\\nt\\xt.cfg) h = %lx, GLE = %d\n",h,GetLastError());

    h = OpenFile("e:\\zznt\\xt.cfg",&of,OF_PROMPT);
    printf("OpenFile(e:\\zznt\\xt.cfg) h = %lx, GLE = %d\n",h,GetLastError());

    h = OpenFile("e:\\nt\\xt.cfg",&of,OF_PROMPT | OF_CANCEL);
    printf("OpenFile(e:\\nt\\xt.cfg) h = %lx, GLE = %d\n",h,GetLastError());
    SetErrorMode(0);
    h = OpenFile("e:\\nt\\xt.cfg",&of,OF_PROMPT);
    printf("OpenFile(e:\\nt\\xt.cfg) h = %lx, GLE = %d\n",h,GetLastError());

    h = OpenFile("e:\\zznt\\xt.cfg",&of,OF_PROMPT);
    printf("OpenFile(e:\\zznt\\xt.cfg) h = %lx, GLE = %d\n",h,GetLastError());

    h = OpenFile("e:\\nt\\xt.cfg",&of,OF_PROMPT | OF_CANCEL);
    printf("OpenFile(e:\\nt\\xt.cfg) h = %lx, GLE = %d\n",h,GetLastError());
}
void
rtldevn()
{
    UNICODE_STRING ustr;
    ANSI_STRING astr;
    CHAR buf[256];
    DWORD dw;

    printf("name -> ");
    scanf("%s",buf);
    RtlInitAnsiString(&astr,buf);
    RtlAnsiStringToUnicodeString(&ustr,&astr,TRUE);

    dw = RtlIsDosDeviceName_U(ustr.Buffer);

    printf("dw %x Name %s \n",dw,buf);
}

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

typedef DWORD (*PFNWAITFORINPUTIDLE)(HANDLE hProcess, DWORD dwMilliseconds);
void
cptst()
{
    CHAR buf[256];
    CHAR cline[256];
    DWORD dw;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    LOAD_MODULE_PARAMS lmp;
    CHAR Environment[256];
    CMDSHOW cs;
    PFNWAITFORINPUTIDLE WaitForInputIdleRoutine;
    HANDLE hMod;

    hMod = LoadLibrary("user32");
    WaitForInputIdleRoutine = GetProcAddress(hMod,"WaitForInputIdle");

    printf("name -> ");
    scanf("%s",buf);

    RtlZeroMemory(&StartupInfo,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    SetLastError(0);
    CreateProcess(
        NULL,
        buf,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInformation
        );
    (WaitForInputIdleRoutine)(ProcessInformation.hProcess,10000);
    printf("GLE %d\n",GetLastError());
    SetLastError(0);
    printf("WINEXEC %d\n",WinExec(buf,0));

    SetLastError(0);
    lmp.lpEnvAddress = Environment;
    lmp.lpCmdLine = cline;
    lmp.dwReserved = 0;
    lmp.lpCmdShow = &cs;
    cs.wMustBe2 = 2;
    cs.wShowWindowValue = 3;
    cline[0] = strlen(buf);
    RtlMoveMemory(&cline[1],buf,cline[0]);
    cline[cline[0]+1] = 0x0d;
    printf("LOADMOD %d\n",LoadModule(buf,&lmp));
}

void
spawntst()
{
    CHAR buf[256];
    int i;

    printf("name -> ");
    scanf("%s",buf);
    i = _spawnlp(_P_WAIT,buf,"-l",NULL);
}

void
badproctst()
{
    CHAR buf[256];
    DWORD dw;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    LOAD_MODULE_PARAMS lmp;
    CHAR Environment[256];
    CMDSHOW cs;

    printf("name -> ");
    scanf("%s",buf);

    RtlZeroMemory(&StartupInfo,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    SetLastError(0);
    CreateProcess(
        NULL,
        buf,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        "*",
        &StartupInfo,
        &ProcessInformation
        );
    printf("GLE %d\n",GetLastError());
}

void
copytst()
{
    CHAR src[256];
    CHAR dst[256];
    BOOL b;

    printf("src -> ");
    scanf("%s",src);
    printf("dst -> ");
    scanf("%s",dst);

    b = CopyFile(src,dst,FALSE);
}

void
fftst()
{
    CHAR buf[256];
    HANDLE fFile;
    WIN32_FIND_DATA FindFileData;
    BOOL b;

    printf("pattern -> ");
    scanf("%s",buf);

    fFile =  FindFirstFile(
                buf,
                &FindFileData
                );
    if ( fFile == INVALID_HANDLE_VALUE ){
        printf("findfirst %s failed %d\n",buf,GetLastError());
        return;
        }

    b = TRUE;
    while(b) {
        printf("0x%08x %08d %s\n",
            FindFileData.dwFileAttributes,
            FindFileData.nFileSizeLow,
            FindFileData.cFileName
            );
        b = FindNextFile(fFile,&FindFileData);
        }
    FindClose(fFile);
}

void
oftst()
{
    OFSTRUCT OfStruct;
    HFILE rv;

    rv = OpenFile("",&OfStruct, OF_EXIST);
    printf("rv %d\n",rv);

    rv = OpenFile(NULL,&OfStruct, OF_EXIST);
    printf("rv %d\n",rv);

    rv = OpenFile(" ",&OfStruct, OF_EXIST);
    printf("rv %d\n",rv);
}

void
spath()
{

    char cbuff[512];

    SearchPath(
        "c:\\nt;c:\\xytty;c:\\nt\\system",
        "kernel32",
        ".dll",
        512,
        cbuff,
        NULL
        );
    printf("%s\n",cbuff);
}

void
muldivtst()
{
    int answer,number,numerator,denom,result;
    PERFINFO PerfInfo;
    ULONG Index;

    StartBenchMark("MulDiv)",
                   50000,
                   &PerfInfo);

    for(Index=0;Index<50000;Index++){
    //
    // answer = -24
    //
    number = -18;
    numerator = 96;
    denom = 72;
    answer = -24;
    result = MulDiv(number,numerator,denom);
    if ( answer != result ) printf("MulDiv(%ld,%ld,%ld)=%ld %s\n",number,numerator,denom,result,answer == result ? "SUCCESS" : "FAILED");

    //
    // answer = -24
    //
    number = 18;
    numerator = -96;
    denom = 72;
    answer = -24;
    result = MulDiv(number,numerator,denom);
    if ( answer != result ) printf("MulDiv(%ld,%ld,%ld)=%ld %s\n",number,numerator,denom,result,answer == result ? "SUCCESS" : "FAILED");

    //
    // answer = 24
    //
    number = -18;
    numerator = -96;
    denom = 72;
    answer = 24;
    result = MulDiv(number,numerator,denom);
    if ( answer != result ) printf("MulDiv(%ld,%ld,%ld)=%ld %s\n",number,numerator,denom,result,answer == result ? "SUCCESS" : "FAILED");

    //
    // answer = -24
    //
    number = -18;
    numerator = -96;
    denom = -72;
    answer = -24;
    result = MulDiv(number,numerator,denom);
    if ( answer != result ) printf("MulDiv(%ld,%ld,%ld)=%ld %s\n",number,numerator,denom,result,answer == result ? "SUCCESS" : "FAILED");

    //
    // answer = -24
    //
    number = -18;
    numerator = -96;
    denom = -72;
    answer = -24;
    result = MulDiv(number,numerator,denom);
    if ( answer != result ) printf("MulDiv(%ld,%ld,%ld)=%ld %s\n",number,numerator,denom,result,answer == result ? "SUCCESS" : "FAILED");

    //
    // answer = 24
    //
    number = 18;
    numerator = -96;
    denom = -72;
    answer = 24;
    result = MulDiv(number,numerator,denom);
    if ( answer != result ) printf("MulDiv(%ld,%ld,%ld)=%ld %s\n",number,numerator,denom,result,answer == result ? "SUCCESS" : "FAILED");


    //
    // answer = 2
    //
    number = 4;
    numerator = 2;
    denom = 5;
    answer = 2;
    result = MulDiv(number,numerator,denom);
    if ( answer != result ) printf("MulDiv(%ld,%ld,%ld)=%ld %s\n",number,numerator,denom,result,answer == result ? "SUCCESS" : "FAILED");

    //
    // answer = 500
    //

    number = 100;
    numerator = 10;
    denom = 2;
    answer = 500;
    result = MulDiv(number,numerator,denom);
    if ( answer != result ) printf("MulDiv(%ld,%ld,%ld)=%ld %s\n",number,numerator,denom,result,answer == result ? "SUCCESS" : "FAILED");

    //
    // answer = 3b9aca00
    //

    number = 1000000;
    numerator = 1000000;
    denom = 1000;
    answer = 0x3b9aca00;
    result = MulDiv(number,numerator,denom);
    if ( answer != result ) printf("MulDiv(%ld,%ld,%ld)=0x%lx %s\n",number,numerator,denom,result,answer == result ? "SUCCESS" : "FAILED");

    }
    FinishBenchMark(&PerfInfo);

}

void
dname()
{
    UNICODE_STRING LinkName;
    UNICODE_STRING DeviceName;
    OBJECT_ATTRIBUTES Obja;
    HANDLE LinkHandle;
    NTSTATUS Status;
    ULONG i;
    PWCHAR p;
    WCHAR DeviceNameBuffer[MAXIMUM_FILENAME_LENGTH];

    RtlInitUnicodeString(&LinkName,L"\\DosDevices\\A:");
    p = (PWCHAR)LinkName.Buffer;
    p = p+12;
    for(i=0;i<26;i++){
        *p = (WCHAR)'A'+i;

        InitializeObjectAttributes(
            &Obja,
            &LinkName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
        Status = NtOpenSymbolicLinkObject(
                    &LinkHandle,
                    SYMBOLIC_LINK_QUERY,
                    &Obja
                    );
        if (NT_SUCCESS( Status )) {

            //
            // Open succeeded, Now get the link value
            //

            DeviceName.Length = 0;
            DeviceName.MaximumLength = sizeof(DeviceNameBuffer);
            DeviceName.Buffer = DeviceNameBuffer;

            Status = NtQuerySymbolicLinkObject(
                        LinkHandle,
                        &DeviceName,
                        NULL
                        );
            NtClose(LinkHandle);
            if ( NT_SUCCESS(Status) ) {
                printf("%wZ -> %wZ\n",&LinkName,&DeviceName);
                }
            }
        }

}

void
mfextst()
{
    MoveFileExW(L"C:\\tmp\\xx.xx", NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
}

CRITICAL_SECTION cs;

VOID
StartBounce(PVOID pThreadBlockInfo)
{
    EnterCriticalSection(&cs);
    Sleep(-1);
}

void
lockuptst()
{
    HANDLE hThread;
    DWORD id;

    InitializeCriticalSection(&cs);

    hThread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)StartBounce,
                    0,
                    0,
                    &id
                    );
    EnterCriticalSection(&cs);
    Sleep(-1);
}
void
getdisktst()
{
    BOOL b;
    DWORD spc,bps,fc,tc;

    b = GetDiskFreeSpace(NULL,&spc,&bps,&fc,&tc);
    printf("GetDiskFreeSpace NULL %s\n",b ? "WORKED" : "FAILED" );

    b = GetDiskFreeSpace("C:\\",&spc,&bps,&fc,&tc);
    printf("GetDiskFreeSpace C:\\ %s\n",b ? "WORKED" : "FAILED" );

    b = GetDiskFreeSpace("C:\\WINNT\\",&spc,&bps,&fc,&tc);
    printf("GetDiskFreeSpace C:\\winnt\\ %s\n",b ? "WORKED" : "FAILED" );
}

void
DoChoice(
    int Choice
    )
{
    NTSTATUS Status;
    LONG *p;

top:
    printf("exception test\n");
    printf("1 Access Violation(r)\n");
    printf("2 Access Violation(w)\n");
    printf("3 Array Bounds    \n");
    printf("4 Int Divide By Zero\n");
    printf("5 Software 0x77\n");
    printf("6 bigpath\n");
    printf("7 set default harderror\n");
    printf("8 proftests\n");
    printf("9 probetests\n");
    printf("10 notifytests\n");
    printf("11 openif\n");
    printf("12 null server\n");
    printf("13 path convert\n");
    printf("14 bogus ordinal\n");
    printf("15 winword openfile\n");
    printf("16 scroll test\n");
    printf("17 winword getdrivetype\n");
    printf("18 dorip\n");
    printf("19 Ofprompt\n");
    printf("20 rtldevn\n");
    printf("21 cptst\n");
    printf("22 oftst\n");
    printf("23 dname\n");
    printf("24 fftst\n");
    printf("25 copy\n");
    printf("26 badproc\n");
    printf("27 loadlib\n");
    printf("28 gettictst(0)\n");
    printf("29 latst\n");
    printf("30 gettictst(1)\n");
    printf("31 spath\n");
    printf("32 spawntst\n");
    printf("33 muldivtst\n");
    printf("34 mfextst\n");
    printf("35 lockuptst\n");
    printf("36 getdisktst\n");

    printf("Enter Choice --> ");
    scanf("%d",&Choice);
    printf("Good Choice... %d\n",Choice);

    switch ( Choice ) {
    case 1:
        SetErrorMode(SEM_NOGPFAULTERRORBOX);
        printf("Good Choice... %d\n",Choice);
        p = (int *)0xbaadadd0;
        Choice = *p;
        break;

    case 2:
        printf("Good Choice... %d\n",Choice);
        p = (int *)0xbaadadd0;
        *p = Choice;
        break;

    case 3:
        printf("Good Choice... %d\n",Choice);
        RtlRaiseStatus(STATUS_ARRAY_BOUNDS_EXCEEDED);
        break;

    case 4:
        printf("Good Choice... %d\n",Choice);
        Choice = Choice/izero;
        break;

    case 5:
        printf("Good Choice... %d\n",Choice);
        {
            UINT b;
            b = SetErrorMode(SEM_FAILCRITICALERRORS);
            xassert(b == 0);
            b = SetErrorMode(0);
            xassert(b == SEM_FAILCRITICALERRORS);
        }
        RtlRaiseStatus(0x77);
        break;

    case 6:
        printf("Good Choice... %d\n",Choice);
        {
            DWORD Bsize;
            DWORD Rsize;
            LPSTR Buff;
            LPSTR Ruff;
            DWORD Rvalue;
            LPSTR whocares;
            int i;

            printf("Enter Size --> ");
            scanf("%d",&Bsize);
            printf("Enter RSize --> ");
            scanf("%d",&Rsize);

            Buff = LocalAlloc(0,Bsize+1);
            xassert(Buff);
            Ruff = LocalAlloc(0,Bsize+1);
            xassert(Buff);
            RtlFillMemory(Buff,Bsize,'a');
            Buff[0]='c';
            Buff[1]=':';
            Buff[2]='\\';
            Buff[Bsize+1] = '\0';
            Rvalue = GetFullPathName(Buff,Rsize,Ruff,&whocares);
            i = strcmp(Buff,Ruff);
            printf("Bsize %d Rsize %d Rvalue %d i=%d \n",Bsize,Rsize,Rvalue,i);

        }
        break;

    case 7:
        printf("Good Choice... %d\n",Choice);
        Status = NtSetDefaultHardErrorPort(NULL);
        xassert(Status == STATUS_PRIVILEGE_NOT_HELD);
        break;
    case 8:
        printf("Good Choice... %d\n",Choice);
        proftst();
        break;
    case 9:
        printf("Good Choice... %d\n",Choice);
        probtst();
        break;

    case 10:
        printf("Good Choice... %d\n",Choice);
        notifytst();
        break;

    case 11:
        printf("Good Choice... %d\n",Choice);
        openiftst();
        break;

    case 12:
        printf("Good Choice... %d\n",Choice);
        NullServerSwitchTest();
        break;

    case 13:
        PathConvertTest();
        break;

    case 14:
        BogusOrdinalTest();
        break;

    case 15:
        WinWordOpenFileTest();
        break;

    case 16:
        ScrollTest();
        break;

    case 17:
        WinWordGetDriveTypeTest();
        break;
    case 18:
        NewRip(0,"Just a warning\n");
        NewRip(1,"We Are Hosed\n");
        break;

    case 19:
        Ofprompt();
        break;

    case 20:
        rtldevn();
        break;

    case 21:
        cptst();
        break;

    case 22:
        oftst();
        break;

    case 23:
        dname();
        break;

    case 24:
        fftst();
        break;

    case 25:
        copytst();
        break;

    case 26:
        badproctst();
        break;

    case 27:
        {
        HANDLE hmods,hmodc,hmodw;
        hmods = LoadLibrary("shell32");
        hmodc = LoadLibrary("cmd.exe");
        hmodw = LoadLibrary("winspool.drv");
        FreeLibrary(hmods);
        FreeLibrary(hmodc);
        FreeLibrary(hmodw);
        }
        break;

    case 28:
        gettictst(0);
        break;

    case 29:
        latst();
        break;

    case 30:
        gettictst(1);
        break;

    case 31:
        spath();
        break;

    case 32:
        spawntst();
        break;

    case 33:
        muldivtst();
        break;

    case 34:
        mfextst();
        break;

    case 35:
        lockuptst();
        break;

    case 36:
        getdisktst();
        break;

    default:
        printf( "Bad choice: %d\n", Choice );
        return;
    }

    return;
}

//#define NtCurrentTebAsm() {PTEB Teb;_asm{mov eax,fs:[0x24]};,Teb;}

DWORD
_cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    int Choice;
    char b[512];


  //  PTEB x;
  //
  //  x = NtCurrentTebAsm();

    GetDriveTypeW(L"A:\\");
    xassert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST));
    xassert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_HIGHEST);
    xassert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_LOWEST));
    xassert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_LOWEST);
    xassert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL));
    xassert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_ABOVE_NORMAL);
    xassert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL));
    xassert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_BELOW_NORMAL);
    xassert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL));
    xassert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_NORMAL);

    xassert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_IDLE));
    xassert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_IDLE);
    xassert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL));
    xassert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_TIME_CRITICAL);

    xassert(SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL));
    xassert(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_NORMAL);

    xassert(!SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST+1));
    xassert(GetThreadPriority(GetCurrentProcess()) == THREAD_PRIORITY_ERROR_RETURN);

    SetErrorMode(0);

    GetSystemDirectory(b,512);
    printf("%s\n",b);
    GetWindowsDirectory(b,512);
    printf("%s\n",b);
    printf("TEBSIZE %d\n",sizeof(TEB));
    Choice = GetModuleFileName(NULL,b,512);
    if ( strlen(b) != Choice ) {
        printf("BAD strlen(b) = %d Choice %d b= %s\n",strlen(b),Choice,b);
        }
    else {
        printf("OK strlen(b) = %d Choice %d b= %s\n",strlen(b),Choice,b);
        }
    if (argc > 1) {
        while (--argc) {
            DoChoice( atoi( *++argv ) );
            }
        }
    else {
        while (TRUE) {
            DoChoice( Choice );
            }
        }
    //GetUserNameW(b,1);
    return 0;
}
