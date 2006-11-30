#define _X86_
#include "ntndk.h"

VOID
Main(VOID)
{
    NTSTATUS Status;
    HANDLE hProcess;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId;
    DBGUI_WAIT_STATE_CHANGE State;
    BOOLEAN Alive = TRUE;

    printf("*** Native (DbgUi) Debugging Test Application\n");
    printf("Press any key to connect to Dbgk...");
    getchar();

    Status = DbgUiConnectToDbg();
    printf(" Connection Established. Status: %lx\n", Status);
    printf("Debug Object Handle: %lx\n", NtCurrentTeb()->DbgSsReserved[1]);
    printf("Press any key to debug services.exe...");
    getchar();

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, 0);
    ClientId.UniqueThread = 0;
    ClientId.UniqueProcess = UlongToHandle(168);
    Status = NtOpenProcess(&hProcess,
                           PROCESS_ALL_ACCESS,
                           &ObjectAttributes,
                           &ClientId);
    Status = DbgUiDebugActiveProcess(hProcess);
    printf(" Debugger Attached. Status: %lx\n", Status);
    printf("Press any key to get first debug event... ");
    getchar();

    while (Alive)
    {
        Status = DbgUiWaitStateChange(&State, NULL);
        printf(" Event Received. Status: %lx\n", Status);
        printf("New State: %lx. Application Client ID: %lx/%lx\n",
               State.NewState,
               State.AppClientId.UniqueProcess, State.AppClientId.UniqueThread);

        switch (State.NewState)
        {
            case DbgCreateProcessStateChange:
                printf("Process Handle: %lx. Thread Handle: %lx\n",
                    State.StateInfo.CreateProcessInfo.HandleToProcess,
                    State.StateInfo.CreateProcessInfo.HandleToThread);
                printf("Process image handle: %lx\n",
                    State.StateInfo.CreateProcessInfo.NewProcess.FileHandle);
                printf("Process image base: %lx\n",
                    State.StateInfo.CreateProcessInfo.NewProcess.BaseOfImage);
                break;

            case DbgCreateThreadStateChange:
                printf("New thread: %lx\n", State.StateInfo.CreateThread.HandleToThread);
                printf("Thread Start Address: %p\n", State.StateInfo.CreateThread.NewThread.StartAddress);
                break;

            case DbgLoadDllStateChange:
                printf("New DLL: %lx\n", State.StateInfo.LoadDll.FileHandle);
                printf("DLL LoadAddress: %p\n", State.StateInfo.LoadDll.BaseOfDll);
                break;

            case DbgBreakpointStateChange:
                printf("Initial breakpoint hit at: %p!\n",
                       State.StateInfo.Exception.ExceptionRecord.ExceptionAddress);
                break;

            case DbgExitThreadStateChange:
                printf("Thread exited: %lx\n", State.StateInfo.ExitThread.ExitStatus);
                break;

            case DbgExitProcessStateChange:
                printf("Process exited: %lx\n", State.StateInfo.ExitProcess.ExitStatus);
                Alive = FALSE;
                break;
        }

        printf("Press any key to continue debuggee...");
        getchar();

        ClientId.UniqueProcess = State.AppClientId.UniqueProcess;
        ClientId.UniqueThread = State.AppClientId.UniqueThread;
        Status = DbgUiContinue(&ClientId, DBG_CONTINUE);
        printf(" Debuggee Resumed. Status: %lx\n", Status);
        printf("Press any key to get next debug event... ");
        getchar();
    };
    printf("*** End of test\n");
    getchar();
}
