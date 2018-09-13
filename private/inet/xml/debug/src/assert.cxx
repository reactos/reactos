//+---------------------------------------------------------------------------
//
//  Microsoft Windows
// Copyright (c) 1993 - 1999 Microsoft Corporation. All rights reserved.*///
//  File:       assert.cxx
//
//  Contents:   Assertion stuff
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#ifndef _INC_SHLWAPI
#include <shlwapi.h>
#endif // _INC_SHLWAPI

LONG g_cAssertThreadDisable = 0;
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
    DWORD_PTR  dwEip[50];

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
        pszModuleName = StrRChrA(achModuleName, achModuleName + lstrlenA(achModuleName), '\\');
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
    if (IsTagEnabled(tagAssertStacks))
    {
        pmbot->cSym = GetStackBacktrace(3, 50, (ULONG_PTR*)dwEip, pmbot->asiSym);
    }
    else
#endif //!WIN16
    {
        pmbot->cSym = 0;
    }

    GetTopURLForThread(pmbot->tid, pmbot->achTopUrl);

    pmbot->id = IDCANCEL;

    pmbot->szTitle = "MSXML Assert";
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


extern BOOL g_fOSIsNT;
//+------------------------------------------------------------
//
// Function:    IsDisplayContext
//
// Synopsis:    Returns TRUE if there is a display context
//              for the currently executing process/thread.
//              If not we shouldn't be popping anything up.
//
//-------------------------------------------------------------
BOOL IsDisplayContext(void)
{
#ifdef WIN16
    return TRUE;
#elif defined(UNIX)
    return TRUE;
#else
    
    WCHAR staName[10];
    WCHAR deskName[10];
    DWORD staNeed, deskNeed;
    BOOL bStation, bDesk;
    HWINSTA hStation;
    HDESK hDesk;

    // this technique only runs on NT, so assume display context otherwise
    if (!g_fOSIsNT) 
        return TRUE;

    // get the station for the current process
    hStation = GetProcessWindowStation();
    if (hStation == NULL)
    {
        // either some error, or we're on NT 3.51 and the process has not made a call to GDI/USER yet. 
        // assume we do not have a display context.
        return FALSE;
    }

    // get the active desktop for this station for the current thread
    hDesk = GetThreadDesktop(GetCurrentThreadId());
    if (hStation == NULL)
    {
        // either some error, or we're on NT 3.51 and the process has not made a call to GDI/USER yet.
        // assume we do not have a display context.
        return FALSE;
    }

    // get the name of the window station 
    bStation = GetUserObjectInformation(hStation, UOI_NAME, staName, sizeof(staName), &staNeed);
    if (!bStation)
    {
        // either some error, or the buffer was not large enough.  In the later case we know
        // the active window station is not winStat0 since our buffer was large enough.
	  return FALSE;
    }
    if (StrCmpI((LPCWSTR)staName, L"WinSta0"))    
        return FALSE;    // not the interactive workstation

    // ditto for the desktop
    bDesk = GetUserObjectInformation(hDesk, UOI_NAME, deskName, sizeof(deskName), &deskNeed);
    if (!bDesk)
    {
        // either some error, or the buffer was not large enough.  In the later case we know
        // the active desktop is not default since our buffer was large enough.
	  return FALSE;
    }
    if (StrCmpI((LPCWSTR)deskName, L"Default"))    
        return FALSE;    // not the interactive desktop

    return TRUE;

#endif
}


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
//  Function:   AssertImpl
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

BOOL
EXPORT WINAPI
AssertImpl(char const * szFile, int iLine, char const * szMessage)
{
#ifdef WIN16
    return TRUE;
#else
    MBOT mbot;
    char ach[2048];

    EnsureThreadState();

    InitMBOT(&mbot);

    mbot.szFile    = szFile;
    mbot.dwLine    = iLine;
    mbot.szMessage = szMessage;

    StringFromMBOT(&mbot, ach, TRUE);

    //  Send the assert text to the debug terminal.

    TraceTag((tagError, ach));
    SpitSzToDisk(ach);

    if (IsTagEnabled(tagAssertExit))
    {
        g_fAbnormalProcessTermination = TRUE;

        // If not in process attach or detach, then try to
        // shut the process down.

        if (g_cAssertThreadDisable == 0)
        {
            TerminateProcess(GetCurrentProcess(), 1);
        }

        return FALSE;
    }

    //  If appropriate, pop up an assert dialog for the user.  Note
    //    that we return TRUE (thereby causing an int 3 break) only
    //    if we pop up the dialog and the user hits Cancel.

    return IsTagEnabled(tagAssertPop) && IsDisplayContext() && (PopUpError(&mbot) == IDCANCEL);
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   AssertThreadDisable
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

void
AssertThreadDisable(BOOL fDisable)
{
    if (fDisable)
        InterlockedIncrement(&g_cAssertThreadDisable);
    else
        InterlockedDecrement(&g_cAssertThreadDisable);
}
