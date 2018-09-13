/***************************** Module Header ******************************\
* Module Name: conexts.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains random debugging related functions.
*
\***************************************************************************/


#include "precomp.h"
#pragma hdrstop
#define NOEXTAPI
#include <wdbgexts.h>

/***************************************************************************\
* Global variables
\***************************************************************************/
PSTR pszAccessViolation = "CONEXTS: Access violation on \"%s\", switch to server context\n";
PSTR pszMoveException   = "CONEXTS: exception in move()\n";
PSTR pszReadFailure     = "CONEXTS: moveBlock(%x, %x, %x) failed!\n";

/***************************************************************************\
* Macros
\***************************************************************************/
#define move(dst, src)  moveBlock(dst, src, sizeof(dst))

#define moveBlock(dst, src, size)                                                                     \
try {                                                                                                 \
    if (lpExtensionApis->nSize >= sizeof(WINDBG_EXTENSION_APIS)) {                                    \
        if (!(*lpExtensionApis->lpReadProcessMemoryRoutine)((DWORD_PTR)(src), &(dst), (size), NULL)) {    \
            (*lpExtensionApis->lpOutputRoutine)(pszReadFailure, &dst, src, size);                     \
            return FALSE;                                                                             \
         }                                                                                            \
    } else {                                                                                          \
        if (!NT_SUCCESS(NtReadVirtualMemory(hCurrentProcess, (LPVOID)(src), &(dst), (size), NULL))) { \
            (*lpExtensionApis->lpOutputRoutine)(pszReadFailure, &dst, src, size);                     \
            return FALSE;                                                                             \
        }                                                                                             \
    }                                                                                                 \
} except (EXCEPTION_EXECUTE_HANDLER) {                                                                \
    (*lpExtensionApis->lpOutputRoutine)(pszMoveException);                                            \
    return FALSE;                                                                                     \
}

#define moveExpressionValue(dst, src)                                 \
try {                                                                 \
    DWORD dwGlobal = (DWORD)lpExtensionApis->lpGetExpressionRoutine(src);    \
    if (lpExtensionApis->nSize < sizeof(WINDBG_EXTENSION_APIS)) {     \
        move(dwGlobal, dwGlobal);                                     \
    }                                                                 \
    (DWORD)dst = dwGlobal;                                            \
} except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?          \
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {  \
    (*lpExtensionApis->lpOutputRoutine)(pszAccessViolation, src);     \
    return FALSE;                                                     \
}

#define moveExpressionValuePtr(dst, src)                              \
try {                                                                 \
    DWORD_PTR dwGlobal = lpExtensionApis->lpGetExpressionRoutine(src);\
    if (lpExtensionApis->nSize < sizeof(WINDBG_EXTENSION_APIS)) {     \
        move(dwGlobal, dwGlobal);                                     \
    }                                                                 \
    (DWORD_PTR)dst = dwGlobal;                                        \
} except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?          \
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {  \
    (*lpExtensionApis->lpOutputRoutine)(pszAccessViolation, src);     \
    return FALSE;                                                     \
}

#define moveExpressionAddress(dst, src)                               \
try {                                                                 \
    if (lpExtensionApis->nSize >= sizeof(WINDBG_EXTENSION_APIS)) {    \
        (DWORD_PTR)dst = lpExtensionApis->lpGetExpressionRoutine("&"src); \
    } else {                                                          \
        (DWORD_PTR)dst = lpExtensionApis->lpGetExpressionRoutine(src);    \
    }                                                                 \
} except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?          \
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {  \
    (*lpExtensionApis->lpOutputRoutine)(pszAccessViolation, src);     \
    return FALSE;                                                     \
}

BOOL DebugConvertToAnsi(
    HANDLE hCurrentProcess,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPWSTR psrc,
    ULONG cbSrc,
    LPSTR pdst,
    ULONG cbDst)
{
    WCHAR awch[80];
    ULONG cchText;

    cbSrc = min(cbSrc, sizeof(awch) - sizeof(WCHAR));
    moveBlock(awch, psrc, cbSrc);
    awch[cbSrc/sizeof(WCHAR)] = 0;

    cchText = wcslen(awch);
    if (cchText == 0) {
        strcpy(pdst, "<null>");
    } else {
        cbSrc = min(cbSrc + sizeof(WCHAR), (cchText + 1) * sizeof(WCHAR));
        RtlUnicodeToMultiByteN(pdst, cbDst, NULL,
                awch, cbSrc);
        pdst[cbDst-1] = 0;
    }
    return TRUE;
}

BOOL gbShowFlagNames = FALSE;

#define NO_FLAG (LPSTR)0xFFFFFFFF  // use this for non-meaningful entries.

#define GF_CONSOLE  1
LPSTR apszConsoleFlags[] = {
   "CONSOLE_IS_ICONIC"              , // 0x000001
   "CONSOLE_OUTPUT_SUSPENDED"       , // 0x000002
   "CONSOLE_HAS_FOCUS"              , // 0x000004
   "CONSOLE_IGNORE_NEXT_MOUSE_INPUT", // 0x000008
   "CONSOLE_SELECTING"              , // 0x000010
   "CONSOLE_SCROLLING"              , // 0x000020
   "CONSOLE_DISABLE_CLOSE"          , // 0x000040
   "CONSOLE_NOTIFY_LAST_CLOSE"      , // 0x000080
   "CONSOLE_NO_WINDOW"              , // 0x000100
   "CONSOLE_VDM_REGISTERED"         , // 0x000200
   "CONSOLE_UPDATING_SCROLL_BARS"   , // 0x000400
   "CONSOLE_QUICK_EDIT_MODE"        , // 0x000800
   "CONSOLE_TERMINATING"            , // 0x001000
   "CONSOLE_CONNECTED_TO_EMULATOR"  , // 0x002000
   "CONSOLE_FULLSCREEN_NOPAINT"     , // 0x004000
   "CONSOLE_SHUTTING_DOWN"          , // 0x008000
   "CONSOLE_AUTO_POSITION"          , // 0x010000
   "CONSOLE_IGNORE_NEXT_KEYUP"      , // 0x020000
   "CONSOLE_WOW_REGISTERED"         , // 0x040000
   "CONSOLE_USE_PRIVATE_FLAGS"      , // 0x080000
   "CONSOLE_HISTORY_NODUP"          , // 0x100000
   "CONSOLE_SCROLLBAR_TRACKING"     , // 0x200000
   "CONSOLE_IN_DESTRUCTION"         , // 0x400000
   "CONSOLE_SETTING_WINDOW_SIZE"    , // 0x800000
   "CONSOLE_DEFAULT_BUFFER_SIZE"    , // 0x0100000
    NULL                              // no more
};

#define GF_CONSOLESEL  2
LPSTR apszConsoleSelectionFlags[] = {
   "CONSOLE_SELECTION_NOT_EMPTY"    , // 1
   "CONSOLE_MOUSE_SELECTION"        , // 2
   "CONSOLE_MOUSE_DOWN"             , // 4
   "CONSOLE_SELECTION_INVERTED"     , // 8
   NULL                               // no more
};

#define GF_FULLSCREEN  3
LPSTR apszFullScreenFlags[] = {
   "CONSOLE_FULLSCREEN",             // 0
   "CONSOLE_FULLSCREEN_HARDWARE",    // 1
   NULL
};

#define GF_CMDHIST     4
LPSTR apszCommandHistoryFlags[] = {
   "CLE_ALLOCATED",                  // 0x01
   "CLE_RESET",                      // 0x02
   NULL
};


/*
 * Converts a 32bit set of flags into an appropriate string.
 * pszBuf should be large enough to hold this string, no checks are done.
 * pszBuf can be NULL, allowing use of a local static buffer but note that
 * this is not reentrant.
 * Output string has the form: " = FLAG1 | FLAG2 ..."
 */
LPSTR GetFlags(
WORD wType,
DWORD dwFlags,
LPSTR pszBuf)
{
    static char szT[400];
    DWORD i;
    BOOL fFirst = TRUE;
    BOOL fNoMoreNames = FALSE;
    LPSTR *apszFlags;

    if (pszBuf == NULL) {
        pszBuf = szT;
    }
    *pszBuf = '\0';

    if (!gbShowFlagNames) {
        return(pszBuf);
    }

    switch (wType) {
    case GF_CONSOLE:
        apszFlags = apszConsoleFlags;
        break;

    case GF_CONSOLESEL:
        apszFlags = apszConsoleSelectionFlags;
        break;

    case GF_FULLSCREEN:
        apszFlags = apszFullScreenFlags;
        break;

    case GF_CMDHIST:
        apszFlags = apszCommandHistoryFlags;
        break;

    default:
        strcpy(pszBuf, " = Invalid flag type.");
        return(pszBuf);
    }

    for (i = 0; dwFlags; dwFlags >>= 1, i++) {

        if (!fNoMoreNames && (apszFlags[i] == NULL)) {
            fNoMoreNames = TRUE;
        }
        if (dwFlags & 1) {
            if (!fFirst) {
                strcat(pszBuf, " | ");
            } else {
                strcat(pszBuf, " = ");
                fFirst = FALSE;
            }
            if (fNoMoreNames || (apszFlags[i] == NO_FLAG)) {
                char ach[16];
                sprintf(ach, "0x%lx", 1 << i);
                strcat(pszBuf, ach);
            } else {
                strcat(pszBuf, apszFlags[i]);
            }
        }
    }
    return pszBuf;
}


/***************************************************************************\
* help - list help for debugger extensions in CONEXTS.
*
*
* 04-Feb-1994 IanJa     Created.
\***************************************************************************/

BOOL help(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == '\0') {
        Print("conexts help:\n\n");
        Print("!help [cmd]            - this list, or details about cmd\n");
        Print("!dc [fvh] [pconsole]   - Dump CONSOLE_INFORMATION struct\n");
        Print("!dch <p>               - Dump Command History\n");
        Print("!dcpt [address]        - Dump CPTABLEINFO (default: GlyphCP)\n");
        Print("!dmem [v] [pconsole]   - Dump memory usage\n");
        Print("!df                    - Dump font cache\n");
        Print("!di <p>                - Dump input buffer info\n");
        Print("!dir <p>               - Dump input record ???\n");
        Print("!ds <pscreen>          - Dump SCREEN_INFORMATION struct\n");
        Print("!dt [f] [v[n]] <pcon>  - Dump screen buffer info\n");
    } else {
        if (*lpArgumentString == '!')
            lpArgumentString++;
        if (strcmp(lpArgumentString, "df") == 0) {
            Print("!df         - dumps Faces and then the cache\n");

        } else if (strcmp(lpArgumentString, "dc") == 0) {
            Print("!dc [fhv]          - dumps CONSOLE_INFORMATION struct for all consoles\n");
            Print("!dc [fhv] address  - dumps CONSOLE_INFORMATION struct for console at address\n");
            Print("   optional flags (must be sparated by spaces) :\n");
            Print("     f - show names of flags\n");
            Print("     h - show command histories\n");
            Print("     v - show verbose information\n");
            Print("eg: \"dc f h v\"\n");

        } else if (strcmp(lpArgumentString, "dt") == 0) {
            Print("!dt [fc] [v[n]] addr - dumps text buffer info for Console at addr\n");
            Print("!dt                  - dumps text buffer info for all Consoles\n");
            Print("  f    - show flags\n");
            Print("  c    - checks text buffer integrity\n");
            Print("  v[n] - show first n lines (default 10)\n");

        } else if (strcmp(lpArgumentString, "di") == 0) {
            Print("!di address - dumps text buffer info (INPUT_INFORMATION)\n");
            Print("!di         - dumps text buffer info for all Consoles\n");

        } else if (strcmp(lpArgumentString, "ds") == 0) {
            Print("!ds address - dumps SCREEN_INFORMATION struct at address\n");

        } else if (strcmp(lpArgumentString, "dmem") == 0) {
            Print("!dmem [v]       - dumps memory usage for all consoles\n");
            Print("!dmem [v] addr  - dumps memory usage for all console at addr\n");
            Print("     v - show verbose information\n");

        }
    }

    return 0;
}

BOOL dch(HANDLE, HANDLE, DWORD, PWINDBG_EXTENSION_APIS, LPSTR);

/***************************************************************************\
* dc - Dump Console - dump CONSOLE_INFORMATION struct
*
* dc address    - dumps simple info for console at address
*                 (takes handle too)
\***************************************************************************/

BOOL dc(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    char ach[120];
    char chVerbose, chShowFlags, chHistory;
    BOOL fPrintLine;
    ULONG i;
    ULONG NumberOfConsoleHandles;
    PCONSOLE_INFORMATION *ConsoleHandles;
    CONSOLE_INFORMATION Console;
    PCONSOLE_INFORMATION pConsole = NULL;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    gbShowFlagNames = FALSE;
    chVerbose   = ' ';
    chShowFlags = ' ';
    chHistory = ' ';

    do {
        /*
         * Skip white space
         */
        while (*lpArgString && *lpArgString == ' ')
            lpArgString++;

        /*
         * f : show flags
         */
        switch (*lpArgString) {
        case 'f': // show flags
            gbShowFlagNames = TRUE;
            chShowFlags = 'f';
            break;

        case 'v':
            chVerbose = 'v';
            break;

        case 'h':
            chHistory = 'h';
            break;

        case '\0':
            /*
             * If no console is specified, loop through all of them
             */
            moveExpressionValue(NumberOfConsoleHandles,
                                "winsrv!NumberOfConsoleHandles");
            moveExpressionValuePtr(ConsoleHandles,
                                "winsrv!ConsoleHandles");
            fPrintLine = FALSE;
            for (i = 0; i < NumberOfConsoleHandles; i++) {
                move(pConsole, ConsoleHandles);
                if (pConsole != NULL) {
                    if (fPrintLine)
                        Print("==========================================\n");
                    sprintf(ach, "%c %c %c %p", chVerbose, chShowFlags, chHistory, pConsole);
                    dc(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);
                    fPrintLine = TRUE;
                }
                ConsoleHandles++;
            }
            return TRUE;

        default:
            pConsole = (PCONSOLE_INFORMATION)EvalExpression(lpArgString);
            break;
        }
        lpArgString++;

    } while (pConsole == NULL);

    move(Console, pConsole);

    DebugConvertToAnsi(hCurrentProcess,
                       lpExtensionApis,
                       Console.Title,
                       Console.TitleLength,
                       ach,
                       sizeof(ach));
    Print("PCONSOLE @ 0x%lX   \"%s\"\n", pConsole, ach);

    if (chHistory == 'h') {
        PLIST_ENTRY ListHead, ListNext;
        LIST_ENTRY ListEntry;
        PCOMMAND_HISTORY History;

        ListHead = &(pConsole->CommandHistoryList);
        ListNext = Console.CommandHistoryList.Flink;
        while (ListNext != ListHead) {
            History = CONTAINING_RECORD( ListNext, COMMAND_HISTORY, ListLink );
            sprintf(ach, "%p", History);
            dch(hCurrentProcess, hCurrentThread, dwCurrentPc,
                    lpExtensionApis, ach);
            move(ListEntry, ListNext);
            ListNext = ListEntry.Flink;
            Print("----\n");
        }
        return TRUE;
    }

    Print("\t pConsoleLock           0x%08lX\n"
          "\t RefCount               0x%04lX\n"
          "\t WaitCount              0x%04lX\n"
          "\t pInputBuffer           0x%08lX\n"
          "\t pCurrentScreenBuffer   0x%08lX\n"
          "\t pScreenBuffers         0x%08lX\n"
          "\t hWnd                   0x%08lX\n"
          "\t hDC                    0x%08lX\n"
          "\t LastAttributes         0x%04lX\n",
          &pConsole->ConsoleLock,
          Console.RefCount,
          Console.WaitCount,
          &pConsole->InputBuffer,
          Console.CurrentScreenBuffer,
          Console.ScreenBuffers,
          Console.hWnd,
          Console.hDC,
          Console.LastAttributes);

    Print("\t Flags                  0x%08lX%s\n",
          Console.Flags, GetFlags(GF_CONSOLE, Console.Flags, NULL));
    Print("\t FullScreenFlags        0x%04lX%s\n",
          Console.FullScreenFlags,
          GetFlags(GF_FULLSCREEN, Console.FullScreenFlags, NULL));
    Print("\t ConsoleHandle          0x%08lX\n"
          "\t CtrlFlags              0x%08lX\n",
          Console.ConsoleHandle,
          Console.CtrlFlags
          );

    if (chVerbose == 'v') {
        Print("\t hMenu                  0x%08lX\n"
              "\t hHeirMenu              0x%08lX\n"
              "\t hSysPalette            0x%08lX\n"
              "\t WindowRect.L T R B     0x%08lX 0x%08lX 0x%08lX 0x%08lX\n"
              "\t ResizeFlags            0x%08lX\n"
              "\t OutputQueue.F B        0x%08lX 0x%08lX\n"
              "\t InitEvents[]           0x%08lX 0x%08lX\n"
              "\t ClientThreadHandle     0x%08lX\n"
              "\t ProcessHandleList.F B  0x%08lX 0x%08lX\n"
              "\t CommandHistoryList.F B 0x%08lX 0x%08lX\n"
              "\t ExeAliasList.F B       0x%08lX 0x%08lX\n",
              Console.hMenu,
              Console.hHeirMenu,
              Console.hSysPalette,
              Console.WindowRect.left,
              Console.WindowRect.top,
              Console.WindowRect.right,
              Console.WindowRect.bottom,
              Console.ResizeFlags,
              Console.OutputQueue.Flink,
              Console.OutputQueue.Blink,
              Console.InitEvents[0],
              Console.InitEvents[1],
              Console.ClientThreadHandle,
              Console.ProcessHandleList.Flink,
              Console.ProcessHandleList.Blink,
              Console.CommandHistoryList.Flink,
              Console.CommandHistoryList.Blink,
              Console.ExeAliasList.Flink,
              Console.ExeAliasList.Blink
              );
        DebugConvertToAnsi(hCurrentProcess,
                           lpExtensionApis,
                           Console.OriginalTitle,
                           Console.OriginalTitleLength,
                           ach,
                           sizeof(ach)
                           );
        Print("\t NumCommandHistories    0x%04lX\n"
              "\t MaxCommandHistories    0x%04lX\n"
              "\t CommandHistorySize     0x%04lX\n"
              "\t OriginalTitleLength    0x%04lX\n"
              "\t TitleLength            0x%04lX\n"
              "\t OriginalTitle          %s\n"
              "\t dwHotKey               0x%08lX\n"
              "\t hIcon                  0x%08lX\n"
              "\t iIcondId               0x%08lX\n"
              "\t ReserveKeys            0x%02lX\n"
              "\t WaitQueue              0x%08lX\n",
              Console.NumCommandHistories,
              Console.MaxCommandHistories,
              Console.CommandHistorySize,
              Console.OriginalTitleLength,
              Console.TitleLength,
              ach,
              Console.dwHotKey,
              Console.hIcon,
              Console.iIconId,
              Console.ReserveKeys,
              Console.WaitQueue
              );
        Print("\t SelectionFlags         0x%08lX%s\n"
              "\t SelectionRect.L T R B  0x%04lX 0x%04lX 0x%04lX 0x%04lX\n"
              "\t SelectionAnchor.X Y    0x%04lX 0x%04lX\n"
              "\t TextCursorPosition.X Y 0x%04lX 0x%04lX\n"
              "\t TextCursorSize         0x%08lX\n"
              "\t TextCursorVisible      0x%02lX\n"
              "\t InsertMode             0x%02lX\n"
              "\t wShowWindow            0x%04lX\n"
              "\t dwWindowOriginX        0x%08lX\n"
              "\t dwWindowOriginY        0x%08lX\n"
              "\t PopupCount             0x%04lX\n",
              Console.SelectionFlags,
              GetFlags(GF_CONSOLESEL, Console.SelectionFlags, NULL),
              Console.SelectionRect.Left,
              Console.SelectionRect.Top,
              Console.SelectionRect.Right,
              Console.SelectionRect.Bottom,
              Console.SelectionAnchor.X,
              Console.SelectionAnchor.Y,
              Console.TextCursorPosition.X,
              Console.TextCursorPosition.Y,
              Console.TextCursorSize,
              Console.TextCursorVisible,
              Console.InsertMode,
              Console.wShowWindow,
              Console.dwWindowOriginX,
              Console.dwWindowOriginY,
              Console.PopupCount
              );
        Print("\t VDMStartHardwareEvent  0x%08lX\n"
              "\t VDMEndHardwareEvent    0x%08lX\n"
              "\t VDMProcessHandle       0x%08lX\n"
              "\t VDMProcessId           0x%08lX\n"
              "\t VDMBufferSectionHandle 0x%08lX\n"
              "\t VDMBuffer              0x%08lX\n"
              "\t VDMBufferClient        0x%08lX\n"
              "\t VDMBufferSize.X Y      0x%04lX 0x%04lX\n"
              "\t StateSectionHandle     0x%08lX\n"
              "\t StateBuffer            0x%08lX\n"
              "\t StateBufferClient      0x%08lX\n"
              "\t StateLength            0x%08lX\n",
              Console.VDMStartHardwareEvent,
              Console.VDMEndHardwareEvent,
              Console.VDMProcessHandle,
              Console.VDMProcessId,
              Console.VDMBufferSectionHandle,
              Console.VDMBuffer,
              Console.VDMBufferClient,
              Console.VDMBufferSize.X,
              Console.VDMBufferSize.Y,
              Console.StateSectionHandle,
              Console.StateBuffer,
              Console.StateBufferClient,
              Console.StateLength
              );
        Print("\t CP                     0x%08lX\n"
              "\t OutputCP               0x%08lX\n"
              "\t hWndProgMan            0x%08lX\n"
              "\t bIconInit              0x%08lX\n"
              "\t LimitingProcessId      0x%08lX\n"
              "\t TerminationEvent       0x%08lX\n"
              "\t VerticalClientToWin    0x%04lX\n"
              "\t HorizontalClientToWin  0x%04lX\n",
              Console.CP,
              Console.OutputCP,
              Console.hWndProgMan,
              Console.bIconInit,
              Console.LimitingProcessId,
              Console.TerminationEvent,
              Console.VerticalClientToWindow,
              Console.HorizontalClientToWindow
              );
#if defined(FE_SB)
        Print("\t EudcInformation        0x%08lX\n"
              "\t FontCacheInformation   0x%08lX\n",
              Console.EudcInformation,
              Console.FontCacheInformation
              );
#if defined(FE_IME)
        Print("\tConsoleIme:\n"
              "\t ScrollFlag             0x%08lX\n"
              "\t ScrollWaitTimeout      0x%08lX\n"
              "\t ScrollWaitCountDown    0x%08lX\n"
              "\t CompStrData            0x%08lX\n"
              "\t ConvAreaMode           0x%08lX\n"
              "\t ConvAreaSystem         0x%08lX\n"
              "\t NumberOfConvAreaCompStr 0x%08lX\n"
              "\t ConvAreaCompStr         0x%08lX\n"
              "\t ConvAreaRoot           0x%08lX\n",
              Console.ConsoleIme.ScrollFlag,
              Console.ConsoleIme.ScrollWaitTimeout,
              Console.ConsoleIme.ScrollWaitCountDown,
              Console.ConsoleIme.CompStrData,
              Console.ConsoleIme.ConvAreaMode,
              Console.ConsoleIme.ConvAreaSystem,
              Console.ConsoleIme.NumberOfConvAreaCompStr,
              Console.ConsoleIme.ConvAreaCompStr,
              Console.ConsoleIme.ConvAreaRoot
              );
#endif
#endif
    }
    return TRUE;
}

/***************************************************************************\
* dt - Dump Text - dump text buffer information
*
* dt address    - dumps text buffer information  for console at address
*
\***************************************************************************/

BOOL dt(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    char ach[120];
    BOOL fPrintLine;
    ULONG i, nLines;
    SHORT sh;
    ULONG NumberOfConsoleHandles;
    PCONSOLE_INFORMATION *ConsoleHandles;
    CONSOLE_INFORMATION Console;
    SCREEN_INFORMATION Screen;
    PCONSOLE_INFORMATION pConsole;
    DWORD FrameBufPtr;
    char chVerbose = ' ';
    char chShowFlags = ' ';
    char chCheck = ' ';
    PROW pRow;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    /*
     * Get arguments
     */
    pConsole = NULL;
    do {
        /*
         * Skip white space
         */
        while (*lpArgString && *lpArgString == ' ')
            lpArgString++;

        switch (*lpArgString) {
        case 'f':
            gbShowFlagNames = TRUE;
            chShowFlags = 'f';
            break;

        case 'c':
            chCheck = 'c';
            break;

        case 'v':
            chVerbose = 'v';
            nLines = 0;
            lpArgString++;
            while ((*lpArgString >= '0') && (*lpArgString <= '9')) {
                nLines *= 10;
                nLines += *lpArgString - '0';
                lpArgString++;
            }
            lpArgString--;
            if (nLines == 0) {
                nLines = 10;
            }
            break;

        case '\0':
            /*
             * If no console is specified, dt all of them
             */
            moveExpressionValue(NumberOfConsoleHandles,
                                "winsrv!NumberOfConsoleHandles");
            moveExpressionValuePtr(ConsoleHandles,
                                "winsrv!ConsoleHandles");
            fPrintLine = FALSE;
            for (i = 0; i < NumberOfConsoleHandles; i++) {
                move(pConsole, ConsoleHandles);
                if (pConsole != NULL) {
                    if (fPrintLine)
                        Print("==========================================\n");
                    sprintf(ach, "%c %c %p", chVerbose, chShowFlags, pConsole);
                    if (!dt(hCurrentProcess, hCurrentThread,
                                     dwCurrentPc, lpExtensionApis, ach)) {
                        return FALSE;
                    }
                    fPrintLine = TRUE;
                }
                ConsoleHandles++;
            }
            return TRUE;

        default:
            pConsole = (PCONSOLE_INFORMATION)EvalExpression(lpArgString);
            break;
        }
        lpArgString++;
    } while (pConsole == NULL);

    move(Console, pConsole);

    Print("PCONSOLE @ 0x%lX\n", pConsole);
    DebugConvertToAnsi(hCurrentProcess,
                       lpExtensionApis,
                       Console.Title,
                       Console.TitleLength,
                       ach,
                       sizeof(ach));
    Print("\t Title                %s\n"
          "\t pCurrentScreenBuffer 0x%08lX\n"
          "\t pScreenBuffers       0x%08lX\n"
          "\t VDMBuffer            0x%08lx\n"
          "\t CP %d,  OutputCP %d\n",
          ach,
          Console.CurrentScreenBuffer,
          Console.ScreenBuffers,
          Console.VDMBuffer,
          Console.CP,
          Console.OutputCP
          );

    moveExpressionValue(FrameBufPtr, "winsrv!FrameBufPtr");
    move(Screen, Console.CurrentScreenBuffer);
    if (Screen.Flags & CONSOLE_TEXTMODE_BUFFER) {
        Print("\t TextInfo.Rows         0x%08X\n"
              "\t TextInfo.TextRows     0x%08X\n"
              "\t TextInfo.FirstRow     0x%08X\n"
              "\t FrameBufPtr           0x%08X\n",
              Screen.BufferInfo.TextInfo.Rows,
              Screen.BufferInfo.TextInfo.TextRows,
              Screen.BufferInfo.TextInfo.FirstRow,
              FrameBufPtr);
    }

    pRow = Screen.BufferInfo.TextInfo.Rows;
    if (chCheck) {
        Print("Checking BufferInfo...\n");
        for (sh = 0; sh < Screen.ScreenBufferSize.Y; sh++) {
            ROW Row;
            move(Row, pRow);

            /*
             * Check that Attrs points to the in-place AttrPair if there
             * if only one AttrPair for this Row.
             */
            if (Row.AttrRow.Length == 1) {
                if (Row.AttrRow.Attrs != &(pRow->AttrRow.AttrPair)) {
                    Print("Bad Row[%lx]:  Attrs %lx should be %lx\n",
                        sh, Row.AttrRow.Attrs, pRow->AttrRow.AttrPair);
                }
            }

            /*
             * Some other checks?
             */

            pRow++;
        }
        Print("...check completed\n");
    }

    if (chVerbose == ' ') {
        return TRUE;
    }

    pRow = Screen.BufferInfo.TextInfo.Rows;
    for (i = 0; i < nLines; i++) {
        ROW Row;
        move(Row, pRow);

        DebugConvertToAnsi(hCurrentProcess,
                           lpExtensionApis,
                           Row.CharRow.Chars,
                           -1,
                           ach,
                           sizeof(ach));
        Print("Row %2d: %4x %4x, %4x %4x, %lx:\"%.40s\"\n",
            i,
            (USHORT)Row.CharRow.Right, (USHORT)Row.CharRow.OldRight,
            (USHORT)Row.CharRow.Left, (USHORT)Row.CharRow.OldLeft,
            Row.CharRow.Chars, ach);
        Print("      %4x %4x,%04x or %lx\n",
            (USHORT)Row.AttrRow.Length,
            (USHORT)Row.AttrRow.AttrPair.Length,
            (WORD)Row.AttrRow.AttrPair.Attr,
            Row.AttrRow.Attrs);
        pRow++;
    }
    return TRUE;
}

/***************************************************************************\
* df - Dump Font - dump Font information
*
* df address    - dumps simple info for console at address
*                 (takes handle too)
\***************************************************************************/

BOOL df(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;
    BYTE Buff[sizeof(FACENODE) + (LF_FACESIZE * sizeof(WCHAR))];
    PFACENODE pFN;

    DWORD NumberOfFonts;
    DWORD FontInfoLength;
    FONT_INFO FontInfo;
    PFONT_INFO pFontInfo;
    DWORD dw;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    /*
     * Skip space
     */

    while (*lpArgumentString == ' ')
        lpArgumentString++;


    Print("Faces:\n");
    moveExpressionValuePtr(pFN, "winsrv!gpFaceNames");
    while (pFN != 0) {
        move(Buff, pFN);
        pFN = (PFACENODE)Buff;
        Print(" \"%ls\"\t%s %s %s %s %s %s\n",
              &pFN->awch[0],
              pFN->dwFlag & EF_NEW        ? "NEW"        : "   ",
              pFN->dwFlag & EF_OLD        ? "OLD"        : "   ",
              pFN->dwFlag & EF_ENUMERATED ? "ENUMERATED" : "          ",
              pFN->dwFlag & EF_OEMFONT    ? "OEMFONT"    : "       ",
              pFN->dwFlag & EF_TTFONT     ? "TTFONT"     : "      ",
              pFN->dwFlag & EF_DEFFACE    ? "DEFFACE"    : "       ");
        pFN = pFN->pNext;
    }

    moveExpressionValue(FontInfoLength, "winsrv!FontInfoLength");
    moveExpressionValue(NumberOfFonts, "winsrv!NumberOfFonts");
    moveExpressionValuePtr(pFontInfo, "winsrv!FontInfo");

    Print("0x%lx fonts cached, 0x%lx allocated:\n", NumberOfFonts, FontInfoLength);

    for (dw = 0; dw < NumberOfFonts; dw++, pFontInfo++) {
        WCHAR FaceName[LF_FACESIZE];
        move(FontInfo, pFontInfo);
        move(FaceName, FontInfo.FaceName);
        Print("%04x hFont    0x%08lX \"%ls\"\n"
              "     SizeWant (%d;%d)\n"
              "     Size     (%d;%d)\n"
              "     Family   %02X\n"
              "     Weight   0x%08lX\n",
              dw,
              FontInfo.hFont,
              FaceName,
              FontInfo.SizeWant.X, FontInfo.SizeWant.Y,
              FontInfo.Size.X, FontInfo.Size.Y,
              FontInfo.Family,
              FontInfo.Weight);
#if defined(FE_SB)
        Print("     CharSet  0x%02X\n",
              FontInfo.tmCharSet);
#endif
    }

    return TRUE;
}

/***************************************************************************\
* di - Dump Input - dump input buffer
*
* di address    - dumps simple info for input at address
*
\***************************************************************************/

BOOL di(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    INPUT_INFORMATION Input;
    PINPUT_INFORMATION pInput;

    PCONSOLE_INFORMATION pConsole;
    ULONG NumberOfConsoleHandles;
    PCONSOLE_INFORMATION *ConsoleHandles;
    BOOL fPrintLine;
    ULONG i;
    char ach[120];

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    /*
     * Skip with space
     */

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    /*
     * If no INPUT_INFORMATION is specified, dt all of them
     */
    if (*lpArgumentString == 0) {
        moveExpressionValue(NumberOfConsoleHandles,
                            "winsrv!NumberOfConsoleHandles");
        moveExpressionValuePtr(ConsoleHandles,
                            "winsrv!ConsoleHandles");
        fPrintLine = FALSE;
        for (i = 0; i < NumberOfConsoleHandles; i++) {
            move(pConsole, ConsoleHandles);
            if (pConsole != NULL) {
                if (fPrintLine)
                    Print("---\n");
                pInput = &pConsole->InputBuffer;
#ifdef _WIN64
                _i64toa((ULONG64)pInput, ach, 16);
#else
                _itoa((ULONG)pInput, ach, 16);
#endif
                if (!di(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach)) {
                    return FALSE;
                }
                fPrintLine = TRUE;
            }
            ConsoleHandles++;
        }
        return TRUE;
    } else {
        pInput = (PINPUT_INFORMATION)EvalExpression(lpArgumentString);
    }

    move(Input, pInput);

    Print("PINPUT @ 0x%lX\n", pInput);
    Print("\t pInputBuffer         0x%08lX\n"
          "\t InputBufferSize      0x%08lX\n"
          "\t AllocatedBufferSize  0x%08lX\n"
          "\t InputMode            0x%08lX\n"
          "\t RefCount             0x%08lX\n"
          "\t First                0x%08lX\n"
          "\t In                   0x%08lX\n"
          "\t Out                  0x%08lX\n"
          "\t Last                 0x%08lX\n"
          "\t ReadWaitQueue.Flink  0x%08lX\n"
          "\t ReadWaitQueue.Blink  0x%08lX\n"
          "\t InputWaitEvent       0x%08lX\n",
          Input.InputBuffer,
          Input.InputBufferSize,
          Input.AllocatedBufferSize,
          Input.InputMode,
          Input.RefCount,
          Input.First,
          Input.In,
          Input.Out,
          Input.Last,
          Input.ReadWaitQueue.Flink,
          Input.ReadWaitQueue.Blink,
          Input.InputWaitEvent
          );

    return TRUE;
}
/***************************************************************************\
* dir - Dump Input Record - dump input buffer
*
* dir address number    - dumps simple info for input at address
*
\***************************************************************************/

BOOL dir(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    INPUT_RECORD InputRecord;
    PINPUT_RECORD pInputRecord;

    char ach[80];
    int cch;
    LPSTR lpAddress;
    DWORD NumRecords,i;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    /*
     * Skip with space
     */

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    lpAddress = lpArgumentString;
    while (*lpArgumentString != ' ' && *lpArgumentString != 0)
        lpArgumentString++;

    cch = (int)(lpArgumentString - lpAddress);
    if (cch > 79)
        cch = 79;

    strncpy(ach, lpAddress, cch);

    pInputRecord = (PINPUT_RECORD)EvalExpression(lpAddress);
    NumRecords = (DWORD)EvalExpression(lpArgumentString);

    Print("%x PINPUTRECORDs @ 0x%lX\n", NumRecords, pInputRecord);
    for (i=0;i<NumRecords;i++) {
        move(InputRecord, pInputRecord);

        switch (InputRecord.EventType) {
            case KEY_EVENT:
                Print("\t KEY_EVENT\n");
                if (InputRecord.Event.KeyEvent.bKeyDown)
                    Print("\t  KeyDown\n");
                else
                    Print("\t  KeyUp\n");
                Print("\t  wRepeatCount %d\n",
                      InputRecord.Event.KeyEvent.wRepeatCount);
                Print("\t  wVirtualKeyCode %x\n",
                      InputRecord.Event.KeyEvent.wVirtualKeyCode);
                Print("\t  wVirtualScanCode %x\n",
                      InputRecord.Event.KeyEvent.wVirtualScanCode);
                Print("\t  aChar is %c",
                      InputRecord.Event.KeyEvent.uChar.AsciiChar);
                Print("\n");
                Print("\t  uChar is %x\n",
                      InputRecord.Event.KeyEvent.uChar.UnicodeChar);
                Print("\t  dwControlKeyState %x\n",
                      InputRecord.Event.KeyEvent.dwControlKeyState);
                break;
            case MOUSE_EVENT:
                Print("\t MOUSE_EVENT\n"
                      "\t   dwMousePosition %x %x\n"
                      "\t   dwButtonState %x\n"
                      "\t   dwControlKeyState %x\n"
                      "\t   dwEventFlags %x\n",
                      InputRecord.Event.MouseEvent.dwMousePosition.X,
                      InputRecord.Event.MouseEvent.dwMousePosition.Y,
                      InputRecord.Event.MouseEvent.dwButtonState,
                      InputRecord.Event.MouseEvent.dwControlKeyState,
                      InputRecord.Event.MouseEvent.dwEventFlags
                     );

                break;
            case WINDOW_BUFFER_SIZE_EVENT:
                Print("\t WINDOW_BUFFER_SIZE_EVENT\n"
                      "\t   dwSize %x %x\n",
                      InputRecord.Event.WindowBufferSizeEvent.dwSize.X,
                      InputRecord.Event.WindowBufferSizeEvent.dwSize.Y
                     );
                break;
            case MENU_EVENT:
                Print("\t MENU_EVENT\n"
                      "\t   dwCommandId %x\n",
                      InputRecord.Event.MenuEvent.dwCommandId
                     );
                break;
            case FOCUS_EVENT:
                Print("\t FOCUS_EVENT\n");
                if (InputRecord.Event.FocusEvent.bSetFocus)
                    Print("\t bSetFocus is TRUE\n");
                else
                    Print("\t bSetFocus is FALSE\n");
                break;
            default:
                Print("\t Unknown event type %x\n",InputRecord.EventType);
                break;
        }
        pInputRecord++;
    }
    return TRUE;
}

/***************************************************************************\
* ds - Dump Screen - dump SCREEN_INFORMATION struct
*
* ds address    - dumps simple info for input at address
*
\***************************************************************************/

BOOL ds(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    SCREEN_INFORMATION Screen;
    PSCREEN_INFORMATION pScreen;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    /*
     * Skip with space
     */

    while (*lpArgumentString == ' ')
        lpArgumentString++;


    pScreen = (PSCREEN_INFORMATION)EvalExpression(lpArgumentString);
    move(Screen, pScreen);

    Print("PSCREEN @ 0x%lX\n", pScreen);
    Print("\t pConsole             0x%08lX\n"
          "\t Flags                0x%08lX %s | %s\n"
          "\t OutputMode           0x%08lX\n"
          "\t RefCount             0x%08lX\n"
          "\t ScreenBufferSize.X Y 0x%08X 0x%08X\n"
          "\t Window.L T R B       0x%08X 0x%08X 0x%08X 0x%08X\n"
          "\t ResizingWindow       0x%08X\n",
          Screen.Console,
          Screen.Flags,
          Screen.Flags & CONSOLE_TEXTMODE_BUFFER ? "TEXTMODE" : "GRAPHICS",
          Screen.Flags & CONSOLE_OEMFONT_DISPLAY ? "OEMFONT" : "TT FONT",
          Screen.OutputMode,
          Screen.RefCount,
          (DWORD)Screen.ScreenBufferSize.X,
          (DWORD)Screen.ScreenBufferSize.Y,
          (DWORD)Screen.Window.Left,
          (DWORD)Screen.Window.Top,
          (DWORD)Screen.Window.Right,
          (DWORD)Screen.Window.Bottom,
          Screen.ResizingWindow
          );
    Print("\t Attributes           0x%08X\n"
          "\t PopupAttributes      0x%08X\n"
          "\t WindowMaximizedX     0x%08X\n"
          "\t WindowMaximizedY     0x%08X\n"
          "\t WindowMaximized      0x%08X\n"
          "\t CommandIdLow High    0x%08X 0x%08X\n"
          "\t CursorHandle         0x%08X\n"
          "\t hPalette             0x%08X\n"
          "\t dwUsage              0x%08X\n"
          "\t CursorDisplayCount   0x%08X\n"
          "\t WheelDelta           0x%08X\n",
          Screen.Attributes,
          Screen.PopupAttributes,
          Screen.WindowMaximizedX,
          Screen.WindowMaximizedY,
          Screen.WindowMaximized,
          Screen.CommandIdLow,
          Screen.CommandIdHigh,
          Screen.CursorHandle,
          Screen.hPalette,
          Screen.dwUsage,
          Screen.CursorDisplayCount,
          Screen.WheelDelta
          );
    if (Screen.Flags & CONSOLE_TEXTMODE_BUFFER) {
        Print("\t TextInfo.Rows         0x%08X\n"
              "\t TextInfo.TextRows     0x%08X\n"
              "\t TextInfo.FirstRow     0x%08X\n",
              Screen.BufferInfo.TextInfo.Rows,
              Screen.BufferInfo.TextInfo.TextRows,
              Screen.BufferInfo.TextInfo.FirstRow);

        Print("\t TextInfo.CurrentTextBufferFont.FontSize       0x%04X,0x%04X\n"
              "\t TextInfo.CurrentTextBufferFont.FontNumber     0x%08X\n",
              Screen.BufferInfo.TextInfo.CurrentTextBufferFont.FontSize.X,
              Screen.BufferInfo.TextInfo.CurrentTextBufferFont.FontSize.Y,
              Screen.BufferInfo.TextInfo.CurrentTextBufferFont.FontNumber);

        Print("\t TextInfo.CurrentTextBufferFont.Family, Weight 0x%08X, 0x%08X\n"
              "\t TextInfo.CurrentTextBufferFont.FaceName       %ls\n"
              "\t TextInfo.CurrentTextBufferFont.FontCodePage   %d\n",
              Screen.BufferInfo.TextInfo.CurrentTextBufferFont.Family,
              Screen.BufferInfo.TextInfo.CurrentTextBufferFont.Weight,
              Screen.BufferInfo.TextInfo.CurrentTextBufferFont.FaceName,
              Screen.BufferInfo.TextInfo.CurrentTextBufferFont.FontCodePage);

        if (Screen.BufferInfo.TextInfo.ListOfTextBufferFont != NULL) {
            PTEXT_BUFFER_FONT_INFO pLinkFont;
            TEXT_BUFFER_FONT_INFO  LinkFont;
            ULONG Count = 0;

            pLinkFont = Screen.BufferInfo.TextInfo.ListOfTextBufferFont;

            while (pLinkFont != 0) {
                move(LinkFont, pLinkFont);

                Print("\t Link Font #%d\n", Count++);
                Print("\t  TextInfo.LinkOfTextBufferFont.FontSize       0x%04X,0x%04X\n"
                      "\t  TextInfo.LinkOfTextBufferFont.FontNumber     0x%08X\n",
                      LinkFont.FontSize.X,
                      LinkFont.FontSize.Y,
                      LinkFont.FontNumber);

                Print("\t  TextInfo.LinkOfTextBufferFont.Family, Weight 0x%08X, 0x%08X\n"
                      "\t  TextInfo.LinkOfTextBufferFont.FaceName       %ls\n"
                      "\t  TextInfo.LinkOfTextBufferFont.FontCodePage   %d\n",
                      LinkFont.Family,
                      LinkFont.Weight,
                      LinkFont.FaceName,
                      LinkFont.FontCodePage);

                pLinkFont = LinkFont.NextTextBufferFont;
            }
            Print("\n");
        }

        Print("\t TextInfo.ModeIndex         0x%08X\n"
#ifdef i386
              "\t TextInfo.WindowedWindowSize.X Y 0x%08X 0x%08X\n"
              "\t TextInfo.WindowedScreenSize.X Y 0x%08X 0x%08X\n"
              "\t TextInfo.MousePosition.X Y 0x%08X 0x%08X\n"
#endif
              "\t TextInfo.Flags             0x%08X\n",

              Screen.BufferInfo.TextInfo.ModeIndex,
#ifdef i386
              Screen.BufferInfo.TextInfo.WindowedWindowSize.X,
              Screen.BufferInfo.TextInfo.WindowedWindowSize.Y,
              Screen.BufferInfo.TextInfo.WindowedScreenSize.X,
              Screen.BufferInfo.TextInfo.WindowedScreenSize.Y,
              Screen.BufferInfo.TextInfo.MousePosition.X,
              Screen.BufferInfo.TextInfo.MousePosition.Y,
#endif
              Screen.BufferInfo.TextInfo.Flags);

        Print("\t TextInfo.CursorVisible        0x%08X\n"
              "\t TextInfo.CursorOn             0x%08X\n"
              "\t TextInfo.DelayCursor          0x%08X\n"
              "\t TextInfo.CursorPosition.X Y   0x%08X 0x%08X\n"
              "\t TextInfo.CursorSize           0x%08X\n"
              "\t TextInfo.CursorYSize          0x%08X\n"
              "\t TextInfo.UpdatingScreen       0x%08X\n",
              Screen.BufferInfo.TextInfo.CursorVisible,
              Screen.BufferInfo.TextInfo.CursorOn,
              Screen.BufferInfo.TextInfo.DelayCursor,
              Screen.BufferInfo.TextInfo.CursorPosition.X,
              Screen.BufferInfo.TextInfo.CursorPosition.Y,
              Screen.BufferInfo.TextInfo.CursorSize,
              Screen.BufferInfo.TextInfo.CursorYSize,
              Screen.BufferInfo.TextInfo.UpdatingScreen);

    } else {
        Print("\t GraphicsInfo.BitMapInfoLength 0x%08X\n"
              "\t GraphicsInfo.lpBitMapInfo     0x%08X\n"
              "\t GraphicsInfo.BitMap           0x%08X\n"
              "\t GraphicsInfo.ClientBitMap     0x%08X\n"
              "\t GraphicsInfo.ClientProcess    0x%08X\n"
              "\t GraphicsInfo.hMutex           0x%08X\n"
              "\t GraphicsInfo.hSection         0x%08X\n"
              "\t GraphicsInfo.dwUsage          0x%08X\n",
              Screen.BufferInfo.GraphicsInfo.BitMapInfoLength,
              Screen.BufferInfo.GraphicsInfo.lpBitMapInfo,
              Screen.BufferInfo.GraphicsInfo.BitMap,
              Screen.BufferInfo.GraphicsInfo.ClientBitMap,
              Screen.BufferInfo.GraphicsInfo.ClientProcess,
              Screen.BufferInfo.GraphicsInfo.hMutex,
              Screen.BufferInfo.GraphicsInfo.hSection,
              Screen.BufferInfo.GraphicsInfo.dwUsage
        );
    }

    return TRUE;
}

/***************************************************************************\
* dcpt - Dump CPTABLEINFO
*
* dcpt address    - dumps CPTABLEINFO at address
* dcpt            - dumps CPTABLEINFO at GlyphCP
*
\***************************************************************************/

BOOL dcpt(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    PCPTABLEINFO pcpt;
    CPTABLEINFO cpt;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    /*
     * Skip with space
     */

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    /*
     * If no CPTABLEINFO is specified, use the GlyphCP
     */
    if (*lpArgumentString == 0) {
        lpArgumentString = "winsrv!GlyphCP";
    }
    pcpt = (PCPTABLEINFO)EvalExpression(lpArgumentString);
    move(cpt, pcpt);

    Print("CPTABLEINFO @ 0x%lX\n", pcpt);

    Print("  CodePage = 0x%x (%d)\n"
          "  MaximumCharacterSize = %x\n"
          "  DefaultChar = %x\n",
          cpt.CodePage, cpt.CodePage,
          cpt.MaximumCharacterSize,
          cpt.DefaultChar);

    Print("  UniDefaultChar = 0x%04x\n"
          "  TransDefaultChar = %x\n"
          "  TransUniDefaultChar = 0x%04x\n"
          "  DBCSCodePage = 0x%x (%d)\n",
          cpt.UniDefaultChar,
          cpt.TransDefaultChar,
          cpt.TransUniDefaultChar,
          cpt.DBCSCodePage, cpt.DBCSCodePage);

    Print("  LeadByte[MAXIMUM_LEADBYTES] = %04x,%04x,%04x,...\n"
          "  MultiByteTable = %x\n"
          "  WideCharTable = %lx\n"
          "  DBCSRanges = %lx\n"
          "  DBCSOffsets = %lx\n",
          cpt.LeadByte[0], cpt.LeadByte[1], cpt.LeadByte[2],
          cpt.MultiByteTable,
          cpt.WideCharTable,
          cpt.DBCSRanges,
          cpt.DBCSOffsets);

    return TRUE;
}

/***************************************************************************\
* dch  - Dump Command History
*
* dch address    - dumps COMMAND_HISTORY at address
* dch            - dumps some random COMMAND_HISTORY
*
\***************************************************************************/

BOOL dch(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    PCOMMAND_HISTORY pCmdHist;
    COMMAND_HISTORY CmdHist;
    PCOMMAND pCmd;
    union {
        COMMAND Cmd;
        WCHAR awch[80];
    } DbgCmd;

    int i;
    char ach[120];

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    /*
     * Skip spaces
     */
    while (*lpArgumentString == ' ')
        lpArgumentString++;

    /*
     * If no COMMAND_HISTORY is specified, use something ????
     */
    if (*lpArgumentString == 0) {
        Print("must specify address of COMMAND_HISTORY struct\n");
        Print("(a breakpoint on winsrv!AddCommand() may help)\n");
        return FALSE;
    }

    pCmdHist = (PCOMMAND_HISTORY)EvalExpression(lpArgumentString);
    move(CmdHist, pCmdHist);

    Print("COMMAND_HISTORY @ 0x%lX\n", pCmdHist);

    Print("  Flags            = 0x%08lX%s\n",
          CmdHist.Flags,  GetFlags(GF_CMDHIST, CmdHist.Flags, NULL));
    Print("  ListLink.F B     = 0x%08lX 0x%08lX\n",
          CmdHist.ListLink.Flink, CmdHist.ListLink.Blink);

    DebugConvertToAnsi(hCurrentProcess,
                       lpExtensionApis,
                       CmdHist.AppName,
                       -1,
                       ach,
                       sizeof(ach));
    Print("  AppName          = %s\n", ach);

    Print("  NumberOfCommands = 0x%lx\n"
          "  LastAdded        = 0x%lx\n"
          "  LastDisplayed    = 0x%lx\n"
          "  FirstCommand     = 0x%lx\n"
          "  MaximumNumberOfCommands = 0x%lx\n",
          CmdHist.NumberOfCommands,
          CmdHist.LastAdded,
          CmdHist.LastDisplayed,
          CmdHist.FirstCommand,
          CmdHist.MaximumNumberOfCommands);

    Print("  ProcessHandle    = 0x%08lX\n",
          CmdHist.ProcessHandle);

    Print("  PopupList.F B    = 0x%08lX 0x%08lX\n",
          CmdHist.PopupList.Flink, CmdHist.PopupList.Blink);

    for (i = 0; i < CmdHist.NumberOfCommands; i++) {
        move(pCmd, &(pCmdHist->Commands[i]));

        // first get the count of bytes in the string...
        moveBlock(DbgCmd, pCmd, sizeof(DbgCmd.Cmd.CommandLength));

        // ...then get the string (and the count of bytes again)
        moveBlock(DbgCmd, pCmd,
                DbgCmd.Cmd.CommandLength + sizeof(DbgCmd.Cmd.CommandLength));

        // DebugConvertToAnsi(hCurrentProcess,lpExtensionApis,
        //         DbgCmd.Cmd.Command, ach);
        DbgCmd.Cmd.CommandLength /= sizeof(WCHAR);
        DbgCmd.Cmd.Command[DbgCmd.Cmd.CommandLength] = L'\0';
        Print("   %03d: %d chars = \"%S\"\n", i,
                DbgCmd.Cmd.CommandLength,
                DbgCmd.Cmd.Command);
        if (i == CmdHist.LastAdded) {
            Print("        (Last Added)\n");
        } else if (i == CmdHist.LastDisplayed) {
            Print("        (Last Displayed)\n");
        } else if (i == CmdHist.FirstCommand) {
            Print("        (First Command)\n");
        }
    }

    return TRUE;
}

/***************************************************************************\
* dmem - Dump Memory Usage
\***************************************************************************/

#define HEAP_GRANULARITY    8
#define HEAP_SIZE(Size)     (((Size) + (HEAP_GRANULARITY - 1) + HEAP_GRANULARITY) & ~(HEAP_GRANULARITY - 1))

ULONG dmem(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    char ach[120];
    char chVerbose;
    ULONG i;
    ULONG NumberOfConsoleHandles;
    PCONSOLE_INFORMATION *ConsoleHandles;
    CONSOLE_INFORMATION Console;
    PCONSOLE_INFORMATION pConsole = NULL;
    SCREEN_INFORMATION Screen;
    PSCREEN_INFORMATION pScreen = NULL;
    ULONG cbTotal = 0;
    ULONG cbConsole = 0;
    ULONG cbInput = 0;
    ULONG cbOutput = 0;
    ULONG cbFE = 0;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    chVerbose   = ' ';

    do {
        /*
         * Skip white space
         */
        while (*lpArgString && *lpArgString == ' ')
            lpArgString++;

        switch (*lpArgString) {
        case 'v':
            chVerbose = 'v';
            break;

        case '\0':
            /*
             * If no console is specified, loop through all of them
             */
            moveExpressionValue(NumberOfConsoleHandles,
                                "winsrv!NumberOfConsoleHandles");
            moveExpressionValuePtr(ConsoleHandles,
                                "winsrv!ConsoleHandles");
            for (i = 0; i < NumberOfConsoleHandles; i++) {
                move(pConsole, ConsoleHandles);
                if (pConsole != NULL) {
                    sprintf(ach, "%c %p", chVerbose, pConsole);
                    cbTotal += dmem(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);
                    Print("==========================================\n");
                }
                ConsoleHandles++;
            }
            Print("Total Size for all consoles = %d bytes\n", cbTotal);
            return cbTotal;

        default:
            pConsole = (PCONSOLE_INFORMATION)EvalExpression(lpArgString);
            break;
        }
        lpArgString++;

    } while (pConsole == NULL);

    move(Console, pConsole);

    DebugConvertToAnsi(hCurrentProcess,
                       lpExtensionApis,
                       Console.Title,
                       Console.TitleLength,
                       ach,
                       sizeof(ach));
    Print("PCONSOLE @ 0x%lX   \"%s\"\n", pConsole, ach);

    cbConsole = HEAP_SIZE(sizeof(Console)) +
                HEAP_SIZE(Console.TitleLength) +
                HEAP_SIZE(Console.OriginalTitleLength);

    cbInput = HEAP_SIZE(Console.InputBuffer.AllocatedBufferSize);

    pScreen = Console.ScreenBuffers;
    while (pScreen != NULL) {

        move(Screen, pScreen);
        cbOutput += HEAP_SIZE(sizeof(Screen));

        if (Screen.Flags & CONSOLE_TEXTMODE_BUFFER) {
            cbOutput += HEAP_SIZE(Screen.ScreenBufferSize.Y * sizeof(ROW)) +
                        HEAP_SIZE(Screen.ScreenBufferSize.X * Screen.ScreenBufferSize.Y * sizeof(WCHAR));
            if (Screen.BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferCharacter) {
                cbFE += HEAP_SIZE(Screen.ScreenBufferSize.X * Screen.ScreenBufferSize.Y * sizeof(WCHAR));
            }
            if (Screen.BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferAttribute) {
                cbFE += HEAP_SIZE(Screen.ScreenBufferSize.X * Screen.ScreenBufferSize.Y * sizeof(BYTE));
            }
            if (Screen.BufferInfo.TextInfo.DbcsScreenBuffer.TransWriteConsole) {
                cbFE += HEAP_SIZE(Screen.ScreenBufferSize.X * Screen.ScreenBufferSize.Y * sizeof(WCHAR));
            }
            if (Screen.BufferInfo.TextInfo.DbcsScreenBuffer.KAttrRows) {
                cbFE += HEAP_SIZE(Screen.ScreenBufferSize.X * Screen.ScreenBufferSize.Y * sizeof(BYTE));
            }
        }

        pScreen = Screen.Next;
    }

    cbTotal = cbConsole + cbInput + cbOutput + cbFE;

    if (chVerbose == 'v') {
        Print("    Console Size = %7d\n", cbConsole);
        Print("    Input Size   = %7d\n", cbInput);
        Print("    Output Size  = %7d\n", cbOutput);
        Print("    FE Size      = %7d\n", cbFE);
    }

    Print("Total Size       = %7d\n", cbTotal);

    return cbTotal;
}

