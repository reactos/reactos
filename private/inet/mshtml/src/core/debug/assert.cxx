//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       assert.cxx
//
//  Contents:   Assertion stuff
//
//----------------------------------------------------------------------------

#include "headers.hxx"

static LONG g_cAssertThreadDisable = 0;
void SpitSzToDisk(CHAR * sz);


#ifndef _MAC

extern HMODULE g_hModule;

int  DoAssertDialog(MBOT *pmbot);

//+---------------------------------------------------------------------------
//
//  Function:   InitMBOT
//
//  Synopsis:   Initializes some data in the MBOT struct, like module name
//              stacktrace, etc.
//
//  Arguments:  [pmbot]      -- Pointer to struct to initialize
//
//----------------------------------------------------------------------------

void
InitMBOT(MBOT *pmbot)
{
    char   achModuleName[MAX_PATH];
    char * pszModuleName;
    DWORD  dwEip[50];

    memset(pmbot, 0, sizeof(pmbot));

    pmbot->tid = GetCurrentThreadId();
    pmbot->pid = GetCurrentProcessId();

#ifndef _MAC
    if (GetModuleFileNameA(g_hModule, achModuleName, MAX_PATH))
#else
    char  achAppLoc[MAX_PATH];

    if (GetModuleFileNameA(NULL, achAppLoc, ARRAY_SIZE(achAppLoc))
        && !GetFileTitleA(achAppLoc,achModuleName,ARRAY_SIZE(achModuleName)) )
#endif
    {
        pszModuleName = strrchr(achModuleName, '\\');
        if (!pszModuleName)
        {
            pszModuleName = achModuleName;
        }
        else
        {
            pszModuleName++;
        }
    }
    else
    {
        pszModuleName = "Unknown";
    }

    strcpy(pmbot->achModule, pszModuleName);

#ifndef WIN16
    if (!g_cAssertThreadDisable && DbgExIsTagEnabled(tagAssertStacks))
    {
        pmbot->cSym = GetStackBacktrace(3, 50, dwEip, pmbot->asiSym);
    }
    else
#endif //!WIN16
    {
        pmbot->cSym = 0;
    }

    GetTopURLForThread(pmbot->tid, pmbot->achTopUrl);

    pmbot->id = IDCANCEL;

    pmbot->szTitle = "Trident/MSHTML Assert";
}

//+---------------------------------------------------------------------------
//
//  Function:   StringFromMBOT
//
//  Synopsis:   Fills a string with a textual representation of information
//              in an MBOT struct
//
//  Arguments:  [pmbot]      --  MBOT to represent
//              [pch]        --  Buffer to put text into.
//              [fShortSyms] --  If TRUE, only SHORT_SYM_COUNT symbols are
//                               used.
//
//----------------------------------------------------------------------------

void
StringFromMBOT(MBOT *pmbot, char * pch, BOOL fShortSyms)
{
    int i;
    int cSyms = pmbot->cSym;

    if (fShortSyms && (cSyms > SHORT_SYM_COUNT))
    {
        cSyms = SHORT_SYM_COUNT;
    }

    wsprintfA(pch, "Assert Failed: Process: %s PID:%x TID:%x\r\nFile: %s (%d)\r\nTop Url: %s\r\n%s",
            pmbot->achModule,
            pmbot->pid,
            pmbot->tid,
            pmbot->szFile,
            pmbot->dwLine,
            pmbot->achTopUrl,
            pmbot->szMessage);

    if (cSyms)
    {
        char achSymbol[200];

        strcat(pch, "\r\n\r\nStacktrace:\r\n");

        for (i = 0; i < cSyms; i++)
        {
            wsprintfA(achSymbol, "   %-11s %s\r\n",
                      pmbot->asiSym[i].achModule,
                      pmbot->asiSym[i].achSymbol);
            strcat(pch, achSymbol);
        }
    }
}

//+------------------------------------------------------------
//
// Function:    MessageBoxOnThreadFn
//
// Synopsis:    ThreadMain function for MessageBoxOnThread.
//
//-------------------------------------------------------------

DWORD WINAPI
MessageBoxOnThreadFn(MBOT *pmbot)
{
#if 0
    char ach[MAX_PATH * 3];

    StringFromMBOT(pmbot, ach, TRUE);

    pmbot->id = MessageBoxA(NULL,
                            ach,
                            pmbot->szTitle,
                            pmbot->dwFlags);
#endif

    pmbot->id = DoAssertDialog(pmbot);

    if (pmbot->id == 0)
        pmbot->dwErr = GetLastError();

    return(0);
}

//+------------------------------------------------------------
//
// Function:    MessageBoxOnThread
//
// Synopsis:
//
//-------------------------------------------------------------

int MessageBoxOnThread(MBOT *pmbot)
{
    THREAD_HANDLE  hThread;
    DWORD   dwThread;

    if (g_cAssertThreadDisable)
    {
        // Flush all user input to prevent auto-closing of this message box
        MSG msg;
        for (int n = 0; n < 100; ++n)
            PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

        MessageBoxOnThreadFn(pmbot);
    }
    else
    {
        hThread = CreateThread(
                NULL,
                0,
                (LPTHREAD_START_ROUTINE)MessageBoxOnThreadFn,
                pmbot,
                0,
                &dwThread);
        if (!hThread)
        {
            // Flush all user input to prevent auto-closing of this message box
            MSG msg;
            for (int n = 0; n < 100; ++n)
                PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

            MessageBoxOnThreadFn(pmbot);
        }
        else
        {
            WaitForSingleObject(hThread, INFINITE);
            CloseThread(hThread);
        }
    }

#ifndef WIN16
    if (pmbot->id == 0)
        SetLastError(pmbot->dwErr);
#endif //!WIN16

    return(pmbot->id);
}

#endif // !MAC

//
// Bookmark array that can set and retrieve a mark on a (filename, line) pair
// Removing a bookmark is not implemented here
//
#pragma auto_inline(off)
#pragma inline_depth(0)

template <int _cMax>
class CDbgBookmarkArray
{
protected:
    int _cUsed;         // used entries
    int _rgcrc[_cMax];  // the array

    //+------------------------------------------------------------
    //
    // Function:    CrcAddInt
    //
    // Synopsis:    Add an integer to CRC
    //
    //-------------------------------------------------------------
    int CrcAddInt(unsigned int crc, unsigned int i)
    {
        return (crc ^ i) * 16807 % 0x7fffffff;
    }

    //+------------------------------------------------------------
    //
    // Function:    CrcFromSz
    //
    // Synopsis:    Compute CRC for a string.
    //
    //-------------------------------------------------------------
    int CrcFromSz(const char *rgb)
    {
        int crc = 0;
        while (*rgb)
            crc = CrcAddInt(crc, *rgb++);

        return crc;
    }

    //+------------------------------------------------------------
    //
    // Function:    CrcFromFileLine
    //
    // Synopsis:    Compute CRC for (filename, line).
    //
    //-------------------------------------------------------------
    int CrcFromFileLine(const char *szFile, int iLine)
    {
        return CrcAddInt(CrcFromSz(szFile), iLine);
    }

public:
    //+------------------------------------------------------------
    //
    // Function:    IsMarked
    //
    // Synopsis:    returns TRUE if there is a bookmark at (file, line).
    //
    //-------------------------------------------------------------
    BOOL IsMarked(const char *szFile, int iLine)
    {
        int crc = CrcFromFileLine(szFile, iLine);
        for (int i = 0; i < _cUsed; i++)
            {
            if (_rgcrc[i] == crc)
                return TRUE;
            }
        return FALSE;
    }

    //+------------------------------------------------------------
    //
    // Function:    Mark
    //
    // Synopsis:    Set bookmark at (file, line)
    //
    //-------------------------------------------------------------
    BOOL Mark(const char *szFile, int iLine)
    {
        if (_cUsed >= _cMax || IsMarked(szFile, iLine))
            return TRUE;

        _rgcrc[_cUsed++] = CrcFromFileLine(szFile, iLine);

        return FALSE;
    }
};

//
// Array of disabled asserts.
// No, you can't disable more than 1000 asserts.
//
CDbgBookmarkArray<1000> rgbmkDisabledAsserts;
BOOL _fDisableAllAsserts = FALSE;


//+------------------------------------------------------------
//
// Function:    PopUpError
//
// Synopsis:    Displays a dialog box using provided text,
//              and presents the user with the option to
//              continue or cancel.
//
// Arguments:
//      pmbot --  Structure containing assert information
//
// Returns:
//      IDCANCEL --     User selected the BREAK  button
//      IDOK     --     User selected the IGNORE button
//
//-------------------------------------------------------------

int
PopUpError(MBOT *pmbot)
{
    int  id  = IDOK;

#ifdef WIN16
    id = DoAssertDialog(pmbot);
#elif defined(UNIX)
    printf( "[ Assert failed. Calling DebugBreak ]" );
    DebugBreak();
#else
    id = MessageBoxOnThread(pmbot);
#endif

    if (id == IDOK && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
        {
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            {
            // ctrl+shift+Ignore - disable all asserts
            _fDisableAllAsserts = TRUE;
            }
        else
            {
            // ctrl+Ignore - disable this one assert
            rgbmkDisabledAsserts.Mark(pmbot->szFile, pmbot->dwLine);
            }
        }

    return id;
#if 0
    pmbot->dwFlags = MB_SETFOREGROUND | MB_TASKMODAL |
                     MB_ICONEXCLAMATION | MB_OKCANCEL;

// bugbug Mac MessageBox function fails with the following message:
//  Scratch DC already in use (wlmdc-1319)
#ifndef _MAC
    id = MessageBoxOnThread(pmbot);

    //
    // If id == 0, then an error occurred.  There are two possibilities
    // that can cause the error:  Access Denied, which means that this
    // process does not have access to the default desktop, and everything
    // else (usually out of memory).
    //

    if (!id && GetLastError() == ERROR_ACCESS_DENIED)
    {
        //
        // Retry this one with the SERVICE_NOTIFICATION flag on.  That
        // should get us to the right desktop.
        //
        pmbot->dwFlags = MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION |
                         MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OKCANCEL;

        id = MessageBoxOnThread(pmbot);
    }
#endif
    return id;

#endif
}

//+------------------------------------------------------------------------
//
//  Function:   DbgExAssertImpl
//
//  Synopsis:   Function called for all asserts.  Checks value, tracing
//              and/or popping up a message box if the condition is
//              FALSE.
//
//  Arguments:
//              szFile
//              iLine
//              szMessage
//
//-------------------------------------------------------------------------

BOOL WINAPI
DbgExAssertImpl(char const * szFile, int iLine, char const * szMessage)
{
#ifdef WIN16
    return TRUE;
#else
    MBOT  mbot;
    char  ach[2048];
    DWORD cbWrite;

    EnsureThreadState();

    InitMBOT(&mbot);

    mbot.szFile    = szFile;
    mbot.dwLine    = iLine;
    mbot.szMessage = szMessage;

    StringFromMBOT(&mbot, ach, TRUE);

    //  Send the assert text to the debug terminal.

    TraceTag((tagError, "%s", ach));
    SpitSzToDisk(ach);

    //
    // Write to STDOUT. Used by the DRTDaemon process to relay information
    // back to the build machine. We ignore any errors.
    //
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
              ach,
              strlen(ach),
              &cbWrite,
              NULL);

    FlushFileBuffers(GetStdHandle(STD_OUTPUT_HANDLE));

    if (DbgExIsTagEnabled(tagAssertExit))
    {
        g_fAbnormalProcessTermination = TRUE;

        // If not in process attach or detach, then try to
        // shut the process down.

        if (g_cAssertThreadDisable == 0)
        {
            TerminateProcess(GetCurrentProcess(), 1);
        }
        else
        {
            static char *s_pszExitCode = "\nPROCESS_EXIT_CODE=1";

            // Since we can't set our process exit code by calling
            // TerminateProcess, we have to tell the DRTDaemon that we've
            // failed another way.

            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
                      s_pszExitCode,
                      strlen(s_pszExitCode),
                      &cbWrite,
                      NULL);

            FlushFileBuffers(GetStdHandle(STD_OUTPUT_HANDLE));
        }

        return FALSE;
    }

    // Check if this occurrance has been manually disabled
    if (_fDisableAllAsserts || rgbmkDisabledAsserts.IsMarked(szFile, iLine))
        return FALSE;

    //  If appropriate, pop up an assert dialog for the user.  Note
    //    that we return TRUE (thereby causing an int 3 break) only
    //    if we pop up the dialog and the user hits Cancel.

    return DbgExIsTagEnabled(tagAssertPop) && (PopUpError(&mbot) == IDCANCEL);
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   DbgExAssertThreadDisable
//
//  Synopsis:   Disables or Enables assert message box spinning a thread.
//
//  Arguments:  [fDisable] -- TRUE if assert on thread should be disabled.
//                            FALSE to re-enable it.
//
//  Returns:    void
//
//  Notes:      Multiple calls giving TRUE require the same amount of calls
//              giving FALSE before assert on thread is actually re-enabled.
//
//----------------------------------------------------------------------------

void WINAPI
DbgExAssertThreadDisable(BOOL fDisable)
{
    if (fDisable)
        InterlockedIncrement(&g_cAssertThreadDisable);
    else
        InterlockedDecrement(&g_cAssertThreadDisable);
}
