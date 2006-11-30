#define NATIVE 0

#if NATIVE
#define _X86_
#include "ntndk.h"
#else
#include "stdio.h"
#include "windows.h"
#endif

VOID
Main(VOID)
{
#if NATIVE
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId;
    DBGUI_WAIT_STATE_CHANGE State;
#else
    DWORD Error, BytesRead;
    DEBUG_EVENT DebugEvent;
    WCHAR ImageName[MAX_PATH];
#endif
    HANDLE hProcess;
    BOOLEAN Alive = TRUE;

#if NATIVE
    printf("*** Native (DbgUi) Debugging Test Application\n");
    printf("Press any key to connect to Dbgk...");
    getchar();

    Status = DbgUiConnectToDbg();
    printf(" Connection Established. Status: %lx\n", Status);
    printf("Debug Object Handle: %lx\n", NtCurrentTeb()->DbgSsReserved[1]);
    printf("Press any key to debug services.exe...");
#else
    printf("*** Win32 (Debug) Debugging Test Application\n");
    printf("Press any key to debug services.exe...");
#endif
    getchar();

#if NATIVE
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, 0);
    ClientId.UniqueThread = 0;
    ClientId.UniqueProcess = UlongToHandle(168);
    Status = NtOpenProcess(&hProcess,
                           PROCESS_ALL_ACCESS,
                           &ObjectAttributes,
                           &ClientId);
    Status = DbgUiDebugActiveProcess(hProcess);
#else
    Error = DebugActiveProcess(168);
#endif

#if NATIVE
    printf(" Debugger Attached. Status: %lx\n", Status);
#else
    printf(" Debugger Attached. Error: %lx\n", Error);
#endif
    printf("Press any key to get first debug event... ");
    getchar();

    while (Alive)
    {
#if NATIVE
        Status = DbgUiWaitStateChange(&State, NULL);
        printf(" Event Received. Status: %lx\n", Status);
        printf("New State: %lx. Application Client ID: %lx/%lx\n",
               State.NewState,
               State.AppClientId.UniqueProcess, State.AppClientId.UniqueThread);
#else
        Error = WaitForDebugEvent(&DebugEvent, -1);
        printf(" Event Received. Error: %lx\n", Error);
        printf("New State: %lx. Application Client ID: %lx/%lx\n",
               DebugEvent.dwDebugEventCode,
               DebugEvent.dwProcessId, DebugEvent.dwThreadId);
#endif

#if NATIVE
        switch (State.NewState)
#else
        switch (DebugEvent.dwDebugEventCode)
#endif
        {
#if NATIVE
            case DbgCreateProcessStateChange:
                printf("Process Handle: %lx. Thread Handle: %lx\n",
                    State.StateInfo.CreateProcessInfo.HandleToProcess,
                    State.StateInfo.CreateProcessInfo.HandleToThread);
                printf("Process image handle: %lx\n",
                    State.StateInfo.CreateProcessInfo.NewProcess.FileHandle);
                printf("Process image base: %lx\n",
                    State.StateInfo.CreateProcessInfo.NewProcess.BaseOfImage);
#else
            case CREATE_PROCESS_DEBUG_EVENT:
                printf("Process Handle: %lx. Thread Handle: %lx\n",
                        DebugEvent.u.CreateProcessInfo.hProcess,
                        DebugEvent.u.CreateProcessInfo.hThread);
                printf("Process image handle: %lx\n",
                        DebugEvent.u.CreateProcessInfo.hFile);
                printf("Process image base: %lx\n",
                        DebugEvent.u.CreateProcessInfo.lpBaseOfImage);
                hProcess = DebugEvent.u.CreateProcessInfo.hProcess;
#endif
                break;

#if NATIVE
            case DbgCreateThreadStateChange:
                printf("New thread: %lx\n", State.StateInfo.CreateThread.HandleToThread);
                printf("Thread Start Address: %p\n", State.StateInfo.CreateThread.NewThread.StartAddress);
#else
            case CREATE_THREAD_DEBUG_EVENT:
                printf("New thread: %lx\n", DebugEvent.u.CreateThread.hThread);
                printf("Thread Start Address: %p\n",
                        DebugEvent.u.CreateThread.lpStartAddress);
#endif
                break;

#if NATIVE
            case DbgLoadDllStateChange:
                printf("New DLL: %lx\n", State.StateInfo.LoadDll.FileHandle);
                printf("DLL LoadAddress: %p\n", State.StateInfo.LoadDll.BaseOfDll);
#else
            case LOAD_DLL_DEBUG_EVENT:
                printf("New DLL: %lx\n", DebugEvent.u.LoadDll.hFile);
                printf("DLL LoadAddress: %p\n", DebugEvent.u.LoadDll.lpBaseOfDll);
                Error = ReadProcessMemory(hProcess,
                                          DebugEvent.u.LoadDll.lpImageName,
                                          &DebugEvent.u.LoadDll.lpImageName,
                                          sizeof(DebugEvent.u.LoadDll.lpImageName),
                                          &BytesRead);
                if (DebugEvent.u.LoadDll.lpImageName)
                {
                    Error = ReadProcessMemory(hProcess,
                                              DebugEvent.u.LoadDll.lpImageName,
                                              ImageName,
                                              sizeof(ImageName),
                                              &BytesRead);
                    printf("DLL Name: %S\n", ImageName);
                }
#endif
                break;

#if NATIVE
            case DbgBreakpointStateChange:
                printf("Initial breakpoint hit at: %p!\n",
                       State.StateInfo.Exception.ExceptionRecord.ExceptionAddress);
#else

#endif
                break;

#if NATIVE
            case DbgExitThreadStateChange:
                printf("Thread exited: %lx\n", State.StateInfo.ExitThread.ExitStatus);
#else

#endif
                break;

#if NATIVE
            case DbgExitProcessStateChange:
                printf("Process exited: %lx\n", State.StateInfo.ExitProcess.ExitStatus);
                Alive = FALSE;
#else

#endif
                break;
        }

        printf("Press any key to continue debuggee...");
        getchar();

#if NATIVE
        ClientId.UniqueProcess = State.AppClientId.UniqueProcess;
        ClientId.UniqueThread = State.AppClientId.UniqueThread;
        Status = DbgUiContinue(&ClientId, DBG_CONTINUE);
        printf(" Debuggee Resumed. Status: %lx\n", Status);
#else
        Error = ContinueDebugEvent(DebugEvent.dwProcessId,
                                   DebugEvent.dwThreadId,
                                   DBG_CONTINUE);
        printf(" Debuggee Resumed. Error: %lx\n", Error);
#endif

        printf("Press any key to get next debug event... ");
        getchar();
    };
    printf("*** End of test\n");
    getchar();
}
