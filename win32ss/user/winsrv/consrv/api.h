/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/api.h
 * PURPOSE:         Public server APIs definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/*
 * Helper macros to define the CSR_API_ROUTINE-s defined in CONSRV,
 * factoring out the common prologue and epilogue code, and including
 * the necessary local variables.
 */

#define CON_API_NOCONSOLE_IMPL(Name, TYPE, RequestName)   \
    NTSTATUS                                    \
    Name##Impl(                                 \
        IN PCONSOLE_PROCESS_DATA ProcessData,   \
        IN OUT PCSR_API_MESSAGE ApiMessage,     \
        IN TYPE* RequestName, /* Request */     \
        IN OUT PCSR_REPLY_CODE ReplyCode OPTIONAL)

// NTSTATUS NTAPI
// Name(
    // IN OUT PCSR_API_MESSAGE ApiMessage,
    // IN OUT PCSR_REPLY_CODE  ReplyCode OPTIONAL)
#define CON_API_NOCONSOLE_ENTRY(Name, TYPE, RequestName)    \
    CON_API_NOCONSOLE_IMPL(Name, TYPE, RequestName);        \
    CSR_API(Name)                               \
    {                                           \
        TYPE* RequestName = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.RequestName;  \
        PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);    \
                                                \
        return Name##Impl(ProcessData, ApiMessage, RequestName, ReplyCode); \
    }

#define CON_API_NOCONSOLE(Name, TYPE, RequestName)      \
    CON_API_NOCONSOLE_ENTRY(Name, TYPE, RequestName);   \
    CON_API_NOCONSOLE_IMPL(Name, TYPE, RequestName)



#define CON_API_IMPL(Name, TYPE, RequestName)   \
    NTSTATUS                                    \
    Name##Impl(                                 \
        IN PCONSOLE_PROCESS_DATA ProcessData,   \
        IN PCONSRV_CONSOLE Console,             \
        IN OUT PCSR_API_MESSAGE ApiMessage,     \
        IN TYPE* RequestName, /* Request */     \
        IN OUT PCSR_REPLY_CODE ReplyCode OPTIONAL)

// NTSTATUS NTAPI
// Name(
    // IN OUT PCSR_API_MESSAGE ApiMessage,
    // IN OUT PCSR_REPLY_CODE  ReplyCode OPTIONAL)
#define CON_API_ENTRY(Name, TYPE, RequestName)  \
    CON_API_IMPL(Name, TYPE, RequestName);      \
    CSR_API(Name)                               \
    {                                           \
        NTSTATUS Status;                        \
        TYPE* RequestName = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.RequestName;  \
        PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);    \
        PCONSRV_CONSOLE Console;                \
                                                \
        Status = ConSrvGetConsole(ProcessData,  \
                                  /* RequestName->ConsoleHandle, */   \
                                  &Console, TRUE);              \
        if (!NT_SUCCESS(Status))                \
            return Status;                      \
                                                \
        Status = Name##Impl(ProcessData, Console,                   \
                            ApiMessage, RequestName, ReplyCode);    \
                                                \
        ConSrvReleaseConsole(Console, TRUE);    \
        return Status;                          \
    }

#define CON_API(Name, TYPE, RequestName)      \
    CON_API_ENTRY(Name, TYPE, RequestName);   \
    CON_API_IMPL(Name, TYPE, RequestName)


/*
 * List of CSR_API_ROUTINE-s defined in this module.
 */

/* alias.c */
CSR_API(SrvAddConsoleAlias);
CSR_API(SrvGetConsoleAlias);
CSR_API(SrvGetConsoleAliases);
CSR_API(SrvGetConsoleAliasesLength);
CSR_API(SrvGetConsoleAliasExes);
CSR_API(SrvGetConsoleAliasExesLength);

/* coninput.c */
CSR_API(SrvReadConsole);
CSR_API(SrvGetConsoleInput);
CSR_API(SrvWriteConsoleInput);
CSR_API(SrvFlushConsoleInputBuffer);
CSR_API(SrvGetConsoleNumberOfInputEvents);

/* conoutput.c */
CSR_API(SrvInvalidateBitMapRect);
CSR_API(SrvSetConsolePalette);
CSR_API(SrvReadConsoleOutput);
CSR_API(SrvWriteConsole);
CSR_API(SrvWriteConsoleOutput);
CSR_API(SrvReadConsoleOutputString);
CSR_API(SrvWriteConsoleOutputString);
CSR_API(SrvFillConsoleOutput);
CSR_API(SrvGetConsoleCursorInfo);
CSR_API(SrvSetConsoleCursorInfo);
CSR_API(SrvSetConsoleCursorPosition);
CSR_API(SrvSetConsoleTextAttribute);
CSR_API(SrvCreateConsoleScreenBuffer);
CSR_API(SrvGetConsoleScreenBufferInfo);
CSR_API(SrvSetConsoleActiveScreenBuffer);
CSR_API(SrvSetConsoleScreenBufferSize);
CSR_API(SrvScrollConsoleScreenBuffer);
CSR_API(SrvSetConsoleWindowInfo);

/* console.c */
CSR_API(SrvAllocConsole);
CSR_API(SrvAttachConsole);
CSR_API(SrvFreeConsole);
CSR_API(SrvGetConsoleMode);
CSR_API(SrvSetConsoleMode);
CSR_API(SrvGetConsoleTitle);
CSR_API(SrvSetConsoleTitle);
CSR_API(SrvGetConsoleCP);
CSR_API(SrvSetConsoleCP);
CSR_API(SrvGetConsoleProcessList);
CSR_API(SrvGenerateConsoleCtrlEvent);
CSR_API(SrvConsoleNotifyLastClose);

CSR_API(SrvGetConsoleMouseInfo);
CSR_API(SrvSetConsoleKeyShortcuts);
CSR_API(SrvGetConsoleKeyboardLayoutName);
CSR_API(SrvGetConsoleCharType);
CSR_API(SrvSetConsoleLocalEUDC);
CSR_API(SrvSetConsoleCursorMode);
CSR_API(SrvGetConsoleCursorMode);
CSR_API(SrvGetConsoleNlsMode);
CSR_API(SrvSetConsoleNlsMode);
CSR_API(SrvGetConsoleLangId);

/* frontendctl.c */
CSR_API(SrvGetConsoleHardwareState);
CSR_API(SrvSetConsoleHardwareState);
CSR_API(SrvGetConsoleDisplayMode);
CSR_API(SrvSetConsoleDisplayMode);
CSR_API(SrvGetLargestConsoleWindowSize);
CSR_API(SrvShowConsoleCursor);
CSR_API(SrvSetConsoleCursor);
CSR_API(SrvConsoleMenuControl);
CSR_API(SrvSetConsoleMenuClose);

/* Used by USERSRV!SrvGetThreadConsoleDesktop() */
NTSTATUS
NTAPI
GetThreadConsoleDesktop(
    IN ULONG_PTR ThreadId,
    OUT HDESK* ConsoleDesktop);

CSR_API(SrvGetConsoleWindow);
CSR_API(SrvSetConsoleIcon);
CSR_API(SrvGetConsoleSelectionInfo);

CSR_API(SrvGetConsoleNumberOfFonts);
CSR_API(SrvGetConsoleFontInfo);
CSR_API(SrvGetConsoleFontSize);
CSR_API(SrvGetConsoleCurrentFont);
CSR_API(SrvSetConsoleFont);

/* handle.c */
CSR_API(SrvOpenConsole);
CSR_API(SrvDuplicateHandle);
CSR_API(SrvGetHandleInformation);
CSR_API(SrvSetHandleInformation);
CSR_API(SrvCloseHandle);
CSR_API(SrvVerifyConsoleIoHandle);

/* history.c */
CSR_API(SrvGetConsoleCommandHistory);
CSR_API(SrvGetConsoleCommandHistoryLength);
CSR_API(SrvExpungeConsoleCommandHistory);
CSR_API(SrvSetConsoleNumberOfCommands);
CSR_API(SrvGetConsoleHistory);
CSR_API(SrvSetConsoleHistory);
CSR_API(SrvSetConsoleCommandHistoryMode);

/* subsysreg.c */
CSR_API(SrvRegisterConsoleVDM);
CSR_API(SrvVDMConsoleOperation);
CSR_API(SrvRegisterConsoleOS2);
CSR_API(SrvSetConsoleOS2OemFormat);
CSR_API(SrvRegisterConsoleIME);
CSR_API(SrvUnregisterConsoleIME);

/* EOF */
