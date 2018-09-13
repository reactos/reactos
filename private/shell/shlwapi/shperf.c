#include "priv.h"
#include "shlwapip.h"
#include "mshtmdbg.h"

#define STOPWATCH_MAX_DESC                  256
#define STOPWATCH_MAX_TITLE                 192
#define STOPWATCH_MAX_BUF                  1024

// Perftags defines and typedefs
typedef PERFTAG (WINAPI *PFN_PERFREGISTER)(char *, char *, char *);
typedef void (WINAPIV *PFN_PERFLOGFN)(PERFTAG, void *, const char *, ...);
typedef char *(WINAPI *PFN_DECODEMESSAGE)(INT);

// IceCAP function typedefs
typedef void (WINAPI *PFN_ICAP)(void);

// MemWatch function typedefs
typedef HRESULT (WINAPI *PFN_MWCONFIG)(DWORD, DWORD, DWORD);
typedef HRESULT (WINAPI *PFN_MWBEGIN)(BOOL, BOOL);
typedef HRESULT (WINAPI *PFN_MWSNAPSHOT)();
typedef HRESULT (WINAPI *PFN_MWEND)(char *);
typedef HRESULT (WINAPI *PFN_MWMARK)(char *);
typedef HRESULT (WINAPI *PFN_MWEXIT)();

// Stopwatch memory buffer
typedef struct _STOPWATCH
{
    DWORD dwId;     // Node identifier
    DWORD dwTID;    // Thread ID;
    DWORD dwType;   // Node type - start, lap, stop, emtpy
    DWORD dwCount;  // Tick count
    DWORD dwFlags;  // Node flags - memlog, debugout
    TCHAR szDesc[STOPWATCH_MAX_DESC];
} STOPWATCH, *PSTOPWATCH;

// Global stopwatch info data
typedef struct _STOPWATCHINFO
{
    DWORD dwStopWatchMode;
    DWORD dwStopWatchProfile;
    DWORD dwStopWatchListIndex;
    DWORD dwStopWatchListMax;
    DWORD dwStopWatchPaintInterval;

    // SPMODE_MSGTRACE data
    DWORD dwStopWatchMaxDispatchTime;
    DWORD dwStopWatchMaxMsgTime;
    DWORD dwStopWatchMsgInterval;
    DWORD dwcStopWatchOverflow;
    DWORD dwStopWatchLastLocation;
    DWORD dwStopWatchTraceMsg;
    DWORD dwStopWatchTraceMsgCnt;
    DWORD *pdwStopWatchMsgTime;

    // SPMODE_MEMWATCH config data and function pointers
    DWORD dwMemWatchPages;
    DWORD dwMemWatchTime;
    DWORD dwMemWatchFlags;
    BOOL fMemWatchConfig;
    HMODULE hModMemWatch;
    PFN_MWCONFIG pfnMemWatchConfig;
    PFN_MWBEGIN pfnMemWatchBegin;
    PFN_MWSNAPSHOT pfnMemWatchSnapShot;
    PFN_MWEND pfnMemWatchEnd;
    PFN_MWMARK pfnMemWatchMark;
    PFN_MWEXIT pfnMemWatchExit;

    // Perftag data and function pointers
    PERFTAG tagStopWatchStart;
    PERFTAG tagStopWatchStop;
    PERFTAG tagStopWatchLap;
    PFN_PERFREGISTER pfnPerfRegister;
    PFN_PERFLOGFN pfnPerfLogFn;
    PFN_DECODEMESSAGE pfnDecodeMessage;

    LPTSTR pszClassNames;

    PSTOPWATCH pStopWatchList;

    // IceCAP data and function pointers
    HMODULE hModICAP;
    PFN_ICAP pfnStartCAPAll;
    PFN_ICAP pfnStopCAPAll;

    HANDLE hMapHtmPerfCtl;
    HTMPERFCTL *pHtmPerfCtl;    
} STOPWATCHINFO, *PSTOPWATCHINFO;

PSTOPWATCHINFO g_pswi = NULL;

const TCHAR c_szDefClassNames[] = {STOPWATCH_DEFAULT_CLASSNAMES};

void StopWatch_SignalEvent();

//===========================================================================================
// INTERNAL FUNCTIONS
//===========================================================================================

//===========================================================================================
//===========================================================================================
void PerfCtlCallback(DWORD dwArg1, void * pvArg2)
{
    const TCHAR c_szFmtBrowserStop[] = TEXT("Browser Frame Stop (%s)");
    TCHAR szTitle[STOPWATCH_MAX_TITLE];
    TCHAR szText[STOPWATCH_MAX_TITLE + ARRAYSIZE(c_szFmtBrowserStop) + 1];
    LPTSTR ptr = szTitle;
#ifndef UNICODE    
    INT rc;
#endif
    if(g_pswi->dwStopWatchMode & SPMODE_BROWSER)  // Temp hack to deal with ansi,unicode.  This code will go away when we impl hook in mshtml.
    {
//        GetWindowText(hwnd, szTitle, ARRAYSIZE(szTitle)-1);

#ifndef UNICODE    
        rc = WideCharToMultiByte(CP_ACP, 0, pvArg2, -1, szTitle, STOPWATCH_MAX_TITLE - 1, NULL, NULL);

        if(!rc)
            StrCpyN(szTitle, "ERROR converting wide to multi", ARRAYSIZE(szTitle) - 1);
#else
        ptr = (LPTSTR) pvArg2;
#endif
        wnsprintf(szText, ARRAYSIZE(szText), c_szFmtBrowserStop, ptr);
        StopWatch_Stop(SWID_BROWSER_FRAME, szText, SPMODE_BROWSER | SPMODE_DEBUGOUT);
        if((g_pswi->dwStopWatchMode & (SPMODE_EVENT | SPMODE_BROWSER)) == (SPMODE_EVENT | SPMODE_BROWSER))
        {
            StopWatch_SignalEvent();
        }
    }
}

//===========================================================================================
//===========================================================================================
HRESULT SetPerfCtl(DWORD dwFlags)
{
    if (dwFlags == HTMPF_CALLBACK_ONLOAD)
    {
        char achName[sizeof(HTMPERFCTL_NAME) + 8 + 1];
        wsprintfA(achName, "%s%08lX", HTMPERFCTL_NAME, GetCurrentProcessId());

        if (g_pswi->hMapHtmPerfCtl == NULL)
            g_pswi->hMapHtmPerfCtl = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, achName);
        if (g_pswi->hMapHtmPerfCtl == NULL)
            return(E_FAIL);
        if (g_pswi->pHtmPerfCtl == NULL)
            g_pswi->pHtmPerfCtl = (HTMPERFCTL *)MapViewOfFile(g_pswi->hMapHtmPerfCtl, FILE_MAP_WRITE, 0, 0, 0);
        if (g_pswi->pHtmPerfCtl == NULL)
            return(E_FAIL);

        g_pswi->pHtmPerfCtl->dwSize  = sizeof(HTMPERFCTL);
        g_pswi->pHtmPerfCtl->dwFlags = dwFlags;
        g_pswi->pHtmPerfCtl->pfnCall = PerfCtlCallback;
        g_pswi->pHtmPerfCtl->pvHost  = NULL;
    }

    return S_OK;
}


//===========================================================================================
//===========================================================================================
void StopWatch_SignalEvent()
{
    static HANDLE hEvent = NULL;

    if(hEvent == NULL)
    {
        TCHAR szEventName[256];
        wnsprintf(szEventName, ARRAYSIZE(szEventName), TEXT("%s%x"), TEXT("STOPWATCH_STOP_EVENT"), GetCurrentProcessId());
        hEvent = CreateEvent((LPSECURITY_ATTRIBUTES)NULL, FALSE, FALSE, szEventName);
    }
    if(hEvent != NULL)
        SetEvent(hEvent);
}

//===========================================================================================
//===========================================================================================
HRESULT DoMemWatchConfig(VOID)
{
    HRESULT hr = ERROR_SUCCESS;

    if(g_pswi->hModMemWatch == NULL)
    {
        if((g_pswi->hModMemWatch = LoadLibrary("mwshelp.dll")) != NULL)
        {
            g_pswi->pfnMemWatchConfig = (PFN_MWCONFIG) GetProcAddress(g_pswi->hModMemWatch, "MemWatchConfigure");
            g_pswi->pfnMemWatchBegin = (PFN_MWBEGIN) GetProcAddress(g_pswi->hModMemWatch, "MemWatchBegin");
            g_pswi->pfnMemWatchSnapShot = (PFN_MWSNAPSHOT) GetProcAddress(g_pswi->hModMemWatch, "MemWatchSnapShot");
            g_pswi->pfnMemWatchEnd = (PFN_MWEND) GetProcAddress(g_pswi->hModMemWatch, "MemWatchEnd");
            g_pswi->pfnMemWatchMark = (PFN_MWMARK) GetProcAddress(g_pswi->hModMemWatch, "MemWatchMark");
            g_pswi->pfnMemWatchExit = (PFN_MWEXIT) GetProcAddress(g_pswi->hModMemWatch, "MemWatchExit");
        
            if(g_pswi->pfnMemWatchConfig != NULL)
            {
                hr = g_pswi->pfnMemWatchConfig(g_pswi->dwMemWatchPages, g_pswi->dwMemWatchTime, g_pswi->dwMemWatchFlags);
                if(FAILED(hr))
                    g_pswi->dwStopWatchMode &= ~SPMODE_MEMWATCH;
                else
                    g_pswi->fMemWatchConfig = TRUE;
            }
        }
        else
        {
            g_pswi->hModMemWatch = (HMODULE)1;
        }
    }

    return(hr);
}

//===========================================================================================
// Function: VOID InitStopWatchMode(VOID)
//
// If HKLM\software\microsoft\windows\currentversion\explorer\performance\mode key value
// is set to one of the values described below, the stopwatch mode will be enabled by
// setting the global variable g_pswi->dwStopWatchMode.
//
// SPMODE_SHELL    - Allows the flushing of stopwatch timings to a log file
// SPMODE_DEBUGOUT  - Display timing via OutputDebugString. Only timings marked with SPMODE_DEBUGOUT
//                    through the StopWatch_* calls will be displayed.
// SPMODE_TEST      - Used to display test output.  This allow another level of SPMODE_DEBUGOUT
//                    like output.
//
// If HKLM\software\microsoft\windows\currentversion\explorer\performance\nodes key value
// is set, the size of the timing array will be set to this value.  The default is 100 nodes.
//===========================================================================================
#define REGKEY_PERFMODE        REGSTR_PATH_EXPLORER TEXT("\\Performance")

VOID InitStopWatchMode(VOID)
{
    HKEY hkeyPerfMode;
    DWORD dwVal = 0;
    DWORD cbBuffer;
    DWORD dwType;
    TCHAR szClassNames[256];
#if STOPWATCH_DEBUG
    TCHAR szDbg[256];
#endif

    if(NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_PERFMODE, 0L, MAXIMUM_ALLOWED, &hkeyPerfMode))
    {
        cbBuffer = SIZEOF(dwVal);
        if(NO_ERROR == RegQueryValueEx(hkeyPerfMode, TEXT("Mode"), NULL, &dwType, (LPBYTE)&dwVal, &cbBuffer))
        {
            if((dwVal & SPMODES) == 0)    // Low word is mode, high word is paint timer interval
                dwVal |= SPMODE_SHELL;
                
            if((g_pswi = (PSTOPWATCHINFO)LocalAlloc(LPTR, SIZEOF(STOPWATCHINFO))) == NULL)
                dwVal = 0;
        }

        if(dwVal != 0)
        {
            g_pswi->dwStopWatchMode = dwVal;
            g_pswi->dwStopWatchListMax = STOPWATCH_MAX_NODES;
            g_pswi->dwStopWatchPaintInterval = STOPWATCH_DEFAULT_PAINT_INTERVAL;
            g_pswi->dwStopWatchMaxDispatchTime = STOPWATCH_DEFAULT_MAX_DISPATCH_TIME;
            g_pswi->dwStopWatchMaxMsgTime = STOPWATCH_DEFAULT_MAX_MSG_TIME;
            g_pswi->dwStopWatchMsgInterval = STOPWATCH_DEFAULT_MAX_MSG_INTERVAL;
            g_pswi->pszClassNames = (LPTSTR)c_szDefClassNames;
            g_pswi->dwMemWatchPages = MEMWATCH_DEFAULT_PAGES;
            g_pswi->dwMemWatchTime = MEMWATCH_DEFAULT_TIME;
            g_pswi->dwMemWatchFlags = MEMWATCH_DEFAULT_FLAGS;
            
            cbBuffer = SIZEOF(dwVal);
            if(NO_ERROR == RegQueryValueEx(hkeyPerfMode, TEXT("Profile"), NULL, &dwType, (LPBYTE)&dwVal, &cbBuffer))
                g_pswi->dwStopWatchProfile = dwVal;
            cbBuffer = SIZEOF(dwVal);
            if(NO_ERROR == RegQueryValueEx(hkeyPerfMode, TEXT("Nodes"), NULL, &dwType, (LPBYTE)&dwVal, &cbBuffer))
                g_pswi->dwStopWatchListMax = dwVal;
            cbBuffer = SIZEOF(szClassNames);
            if(NO_ERROR == RegQueryValueEx(hkeyPerfMode, TEXT("ClassNames"), NULL, &dwType, (LPBYTE)&szClassNames, &cbBuffer))
            {
                if((g_pswi->pszClassNames = (LPTSTR)LocalAlloc(LPTR, SIZEOF(LPTSTR) * cbBuffer)) != NULL)
                    CopyMemory(g_pswi->pszClassNames, szClassNames, SIZEOF(LPTSTR) * cbBuffer);
            }
            cbBuffer = SIZEOF(dwVal);
            // begin - Remove this after StopWatch users convert to using PaintInterval key
            g_pswi->dwStopWatchPaintInterval = HIWORD(g_pswi->dwStopWatchMode) ?HIWORD(g_pswi->dwStopWatchMode) :STOPWATCH_DEFAULT_PAINT_INTERVAL;    // Use high word of mode reg key value for interval
            // end - Remove this after StopWatch users convert to using PaintInterval key
            if(NO_ERROR == RegQueryValueEx(hkeyPerfMode, TEXT("PaintInterval"), NULL, &dwType, (LPBYTE)&dwVal, &cbBuffer))
                g_pswi->dwStopWatchPaintInterval = dwVal;

            // Get MemWatch data
            cbBuffer = SIZEOF(dwVal);
            if(NO_ERROR == RegQueryValueEx(hkeyPerfMode, TEXT("MWPages"), NULL, &dwType, (LPBYTE)&dwVal, &cbBuffer))
                g_pswi->dwMemWatchPages = dwVal;
            cbBuffer = SIZEOF(dwVal);
            if(NO_ERROR == RegQueryValueEx(hkeyPerfMode, TEXT("MWTime"), NULL, &dwType, (LPBYTE)&dwVal, &cbBuffer))
                g_pswi->dwMemWatchTime = dwVal;
            cbBuffer = SIZEOF(dwVal);
            if(NO_ERROR == RegQueryValueEx(hkeyPerfMode, TEXT("MWFlags"), NULL, &dwType, (LPBYTE)&dwVal, &cbBuffer))
                g_pswi->dwMemWatchFlags = dwVal;

            if(g_pswi->dwStopWatchMode & SPMODES)
            {
                SetPerfCtl(HTMPF_CALLBACK_ONLOAD);
            }
            
            if(g_pswi->dwStopWatchMode & SPMODE_MSGTRACE)
            {
                cbBuffer = SIZEOF(dwVal);
                if(NO_ERROR == RegQueryValueEx(hkeyPerfMode, TEXT("MaxDispatchTime"), NULL, &dwType, (LPBYTE)&dwVal, &cbBuffer))
                    g_pswi->dwStopWatchMaxDispatchTime = dwVal;
                cbBuffer = SIZEOF(dwVal);
                if(NO_ERROR == RegQueryValueEx(hkeyPerfMode, TEXT("MaxMsgTime"), NULL, &dwType, (LPBYTE)&dwVal, &cbBuffer))
                    g_pswi->dwStopWatchMaxMsgTime = dwVal;
                cbBuffer = SIZEOF(dwVal);
                if(NO_ERROR == RegQueryValueEx(hkeyPerfMode, TEXT("MsgInterval"), NULL, &dwType, (LPBYTE)&dwVal, &cbBuffer))
                    g_pswi->dwStopWatchMsgInterval = dwVal;
                cbBuffer = SIZEOF(dwVal);
                if(NO_ERROR == RegQueryValueEx(hkeyPerfMode, TEXT("TraceMsg"), NULL, &dwType, (LPBYTE)&dwVal, &cbBuffer))
                    g_pswi->dwStopWatchTraceMsg = dwVal;

                if((g_pswi->pdwStopWatchMsgTime = (DWORD *)LocalAlloc(LPTR, sizeof(DWORD) * (g_pswi->dwStopWatchMaxMsgTime / g_pswi->dwStopWatchMsgInterval))) == NULL)
                    g_pswi->dwStopWatchMode &= ~SPMODE_MSGTRACE;
            }

            if((g_pswi->pStopWatchList = (PSTOPWATCH)LocalAlloc(LPTR, sizeof(STOPWATCH)* g_pswi->dwStopWatchListMax)) == NULL)
                g_pswi->dwStopWatchMode = 0;

            if(g_pswi->dwStopWatchMode & SPMODE_PERFTAGS)
            {
                HMODULE hMod;
                if((hMod = LoadLibrary("mshtmdbg.dll")) != NULL)
                {
                    g_pswi->pfnPerfRegister = (PFN_PERFREGISTER) GetProcAddress(hMod, "DbgExPerfRegister");
                    g_pswi->pfnPerfLogFn = (PFN_PERFLOGFN) GetProcAddress(hMod, "DbgExPerfLogFn");
                    g_pswi->pfnDecodeMessage = (PFN_DECODEMESSAGE) GetProcAddress(hMod, "DbgExDecodeMessage");
                }
                else
                {
#if STOPWATCH_DEBUG
                    wnsprintf(szDbg, ARRAYSIZE(szDbg) - 1, "~SPMODE_PERFTAGS loadlib mshtmdbg.dll failed GLE=0x%x\n", GetLastError());
                    OutputDebugString(szDbg);
#endif
                    g_pswi->dwStopWatchMode &= ~SPMODE_PERFTAGS;
                }
                
                if(g_pswi->pfnPerfRegister != NULL)
                {
                    g_pswi->tagStopWatchStart = g_pswi->pfnPerfRegister("tagStopWatchStart", "StopWatchStart", "SHLWAPI StopWatch start time");
                    g_pswi->tagStopWatchStop = g_pswi->pfnPerfRegister("tagStopWatchStop", "StopWatchStop", "SHLWAPI StopWatch stop time");
                    g_pswi->tagStopWatchLap = g_pswi->pfnPerfRegister("tagStopWatchLap", "StopWatchLap", "SHLWAPI StopWatch lap time");
                }
            }
            
#ifdef STOPWATCH_DEBUG
            // Display option values
            {
                LPCTSTR ptr;
                
                wnsprintf(szDbg, ARRAYSIZE(szDbg) - 1, TEXT("StopWatch Mode=0x%x Profile=0x%x Nodes=%d PaintInterval=%d MemBuf=%d bytes\n"),
                    g_pswi->dwStopWatchMode, g_pswi->dwStopWatchProfile, g_pswi->dwStopWatchListMax, g_pswi->dwStopWatchPaintInterval, g_pswi->dwStopWatchListMax * sizeof(STOPWATCH));
                OutputDebugString(szDbg);

                OutputDebugString(TEXT("Stopwatch ClassNames="));
                ptr = g_pswi->pszClassNames;
                while(*ptr)
                {
                    wnsprintf(szDbg, ARRAYSIZE(szDbg) - 1, TEXT("'%s' "), ptr);
                    OutputDebugString(szDbg);
                    ptr = ptr + (lstrlen(ptr) + 1);
                }
                OutputDebugString(TEXT("\n"));
                
                if(g_pswi->dwStopWatchMode & SPMODE_MSGTRACE)
                {
                    wnsprintf(szDbg, ARRAYSIZE(szDbg)-1, TEXT("StopWatch MaxDispatchTime=%dms MaxMsgTime=%dms MsgInterval=%dms TraceMsg=0x%x MemBuf=%d bytes\n"),
                        g_pswi->dwStopWatchMaxDispatchTime, g_pswi->dwStopWatchMaxMsgTime, g_pswi->dwStopWatchMsgInterval, g_pswi->dwStopWatchTraceMsg, sizeof(DWORD) * (g_pswi->dwStopWatchMaxMsgTime / g_pswi->dwStopWatchMsgInterval));
                    OutputDebugString(szDbg);
                }
                
                if(g_pswi->dwStopWatchMode & SPMODE_MEMWATCH)
                {
                    wnsprintf(szDbg, ARRAYSIZE(szDbg)-1, TEXT("StopWatch MemWatch Pages=%d Time=%dms Flags=%d\n"),
                        g_pswi->dwMemWatchPages, g_pswi->dwMemWatchTime, g_pswi->dwMemWatchFlags);
                    OutputDebugString(szDbg);
                }
            }
#endif
        }       //         if(dwVal != 0)
        
        RegCloseKey(hkeyPerfMode);
    }
}

//===========================================================================================
// EXPORTED FUNCTIONS
//===========================================================================================

//===========================================================================================
// Function: DWORD WINAPI StopWatchMode(VOID)
//
// Returns:  The value of the global mode variable.  Modules should use this call, set their
//           own global, and use this global to minimize and overhead when stopwatch mode
//           is not enabled.
//===========================================================================================
DWORD WINAPI StopWatchMode(VOID)
{
    if(g_pswi != NULL)
        return(g_pswi->dwStopWatchMode);
    else
        return(0);
}


//===========================================================================================
//===========================================================================================
const TCHAR c_szBrowserStop[] = TEXT("Browser Frame Stop (%s)");

DWORD MakeStopWatchDesc(DWORD dwId, DWORD dwMarkType, LPCTSTR pszDesc, LPTSTR *ppszText, DWORD dwTextLen)
{
    LPSTR lpszFmt = NULL;
    DWORD dwRC = 0;
    
    switch(SWID(dwId))
    {
        case SWID_BROWSER_FRAME:
            lpszFmt = (LPSTR)c_szBrowserStop;
            break;
        case SWID_FRAME:    // Shell frame
            break;
        default:
            return(dwRC);
    }

    if(((DWORD)(lstrlen(lpszFmt) + lstrlen(pszDesc)) - 1) < dwTextLen)
        dwRC = wnsprintf(*ppszText, dwTextLen - 1, lpszFmt, pszDesc);
    else
        StrCpyN(*ppszText, TEXT("ERROR:Desc too long!"), dwTextLen -1);

    return(dwRC);
}

#define STARTCAPALL 1
#define STOPCAPALL 2
#define iStartCAPAll() CallICAP(STARTCAPALL)
#define iStopCAPAll() CallICAP(STOPCAPALL)

//===========================================================================================
//===========================================================================================
VOID CallICAP(DWORD dwFunc)
{
    if(g_pswi->hModICAP == NULL)
    {
        if((g_pswi->hModICAP = LoadLibrary("icap.dll")) != NULL)
        {
            g_pswi->pfnStartCAPAll = (PFN_ICAP) GetProcAddress(g_pswi->hModICAP, "StartCAPAll");
            g_pswi->pfnStopCAPAll = (PFN_ICAP) GetProcAddress(g_pswi->hModICAP, "StopCAPAll");
        }
        else
        {
            g_pswi->hModICAP = (HMODULE)1;
        }
    }

    switch(dwFunc)
    {
        case STARTCAPALL:
            if(g_pswi->pfnStartCAPAll != NULL)
                g_pswi->pfnStartCAPAll();
            break;
        case STOPCAPALL:
            if(g_pswi->pfnStopCAPAll != NULL)
                g_pswi->pfnStopCAPAll();
            break;
    }
}

//===========================================================================================
//===========================================================================================
VOID CapBreak(BOOL fStart)
{
    if((g_pswi->dwStopWatchMode & SPMODE_PROFILE) || (g_pswi->pHtmPerfCtl->dwFlags & HTMPF_ENABLE_PROFILE))
    {
        if(fStart)
            iStartCAPAll();
        else
            iStopCAPAll();
    }

    if((g_pswi->dwStopWatchMode & SPMODE_MEMWATCH) || (g_pswi->pHtmPerfCtl->dwFlags & HTMPF_ENABLE_MEMWATCH))
    {
        if(g_pswi->hModMemWatch == NULL)
            DoMemWatchConfig();
            
        if(fStart)
        {
            if(g_pswi->pfnMemWatchBegin != NULL)
            {
                g_pswi->pfnMemWatchBegin(TRUE, FALSE);  // synchronous and don't use timer
            }
        }
        else
        {
            if(g_pswi->pfnMemWatchSnapShot != NULL)
            {
                g_pswi->pfnMemWatchSnapShot();
            }
            
            if(g_pswi->pfnMemWatchEnd != NULL)
            {
                CHAR szOutFile[MAX_PATH];
                DWORD dwLen;
                HRESULT hr;
#if STOPWATCH_DEBUG
                CHAR szDbg[256];
#endif
#ifndef UNIX
                GetWindowsDirectoryA(szOutFile, ARRAYSIZE(szOutFile) - 1);
                dwLen = lstrlenA(szOutFile);
                if(szOutFile[dwLen-1] == '\\')   // See if windows is installed in the root
                    szOutFile[dwLen-1] = '\0';
                StrNCatA(szOutFile, "\\shperf.mws", ARRAYSIZE(szOutFile) - 1);
#else
                StrCpyNA(szOutFile, ARRAYSIZE(szOutFile) - 1, "shperf.mws");
#endif

                hr = g_pswi->pfnMemWatchEnd(szOutFile);
                
#if STOPWATCH_DEBUG
                switch(hr)
                {
                    case E_FAIL:
                        wnsprintfA(szDbg,  ARRAYSIZE(szDbg) - 1, "MemWatch SaveBuffer:%s failed. GLE:0x%x\n", szOutFile, GetLastError());
                        OutputDebugStringA(szDbg);
                        break;
                    case E_ABORT:
                        wnsprintfA(szDbg,  ARRAYSIZE(szDbg) - 1, "MemWatch SaveBuffer: No data to save.\n");
                        OutputDebugStringA(szDbg);
                        break;
                }
#endif
                if(g_pswi->pfnMemWatchExit != NULL)
                    g_pswi->pfnMemWatchExit();
            }
        }
    }
    
    if(g_pswi->dwStopWatchMode & SPMODE_DEBUGBREAK)
    {
        DebugBreak();
    }
}

//===========================================================================================
// Function: DWORD WINAPI StopWatch(
//               DWORD dwId,        // The unique identifier, SWID_*, used to associate start, lap, and
//                                  // stop timings for a given timing sequence.
//               LPCSTR pszDesc,    // Descriptive text for the timing.
//               DWORD dwMarkType,  // START_NODE, LAP_NODE, STOP_NODE
//               DWORD dwFlags)     // SPMODE_SHELL, SPMODE_DEBUGOUT, SPMODE_*. The timing call is used
//                                     only if g_pswi->dwStopWatchMode contains dwFlags
//
// Macros:   StopWatch_Start(dwId, pszDesc, dwFlags)
//           StopWatch_Lap(dwId, pszDesc, dwFlags)
//           StopWatch_Stop(dwId, pszDesc, dwFlags)
//
// Returns:  ERROR_SUCCESS or ERROR_NOT_ENOUGH_MEMORY if out of nodes
//===========================================================================================
DWORD _StopWatch(DWORD dwId, LPCTSTR pszDesc, DWORD dwMarkType, DWORD dwFlags, DWORD dwCount)
{
    PSTOPWATCH psp;
#ifdef STOPWATCH_DEBUG
    PSTOPWATCH pspPrev;
#endif    
    DWORD dwDelta = 0;
    DWORD dwRC = ERROR_SUCCESS;
    DWORD dwIndex;
    TCHAR szText[STOPWATCH_MAX_DESC];
    LPTSTR psz;

    if((SWID(dwId) & g_pswi->dwStopWatchProfile) && (dwMarkType == STOP_NODE))
    {
        CapBreak(FALSE);
    }

    if((g_pswi->pStopWatchList != NULL) && ((dwFlags & g_pswi->dwStopWatchMode) & SPMODES))
    {
        ENTERCRITICAL;
        dwIndex = g_pswi->dwStopWatchListIndex++;
        LEAVECRITICAL;

        if((dwIndex >= 0) && (dwIndex < (g_pswi->dwStopWatchListMax-1)))
        {
            psp = g_pswi->pStopWatchList + (dwIndex);

            psp->dwCount = (dwCount != 0 ?dwCount :GetPerfTime());       // Save the data
            psp->dwId = dwId;
            psp->dwTID = GetCurrentThreadId();
            psp->dwType = dwMarkType;
            psp->dwFlags = dwFlags;

            psz = (LPTSTR)pszDesc;
            if(dwFlags & SPMODE_FORMATTEXT)
            {
                psz = (LPTSTR)szText;
                MakeStopWatchDesc(dwId, dwMarkType, pszDesc, &psz, ARRAYSIZE(szText));
            }
            
            StrCpyN(psp->szDesc, psz, ARRAYSIZE(psp->szDesc)-1);

            if((g_pswi->dwStopWatchMode & SPMODE_PERFTAGS) && (g_pswi->pfnPerfLogFn != NULL))
            {
                if(dwMarkType == START_NODE)
                    g_pswi->pfnPerfLogFn(g_pswi->tagStopWatchStart, (void *)dwId, psz);
                    
                if(dwMarkType == STOP_NODE)
                    g_pswi->pfnPerfLogFn(g_pswi->tagStopWatchStop, (void *)dwId, psz);

                if(dwMarkType == LAP_NODE)
                    g_pswi->pfnPerfLogFn(g_pswi->tagStopWatchLap, (void *)dwId, psz);
            }
    
#ifdef STOPWATCH_DEBUG
            if(g_pswi->dwStopWatchMode & SPMODE_DEBUGOUT)
            {
                const TCHAR c_szFmt_StopWatch_DbgOut[] = TEXT("StopWatch: 0x%x: %s: Time: %u ms\r\n");
                TCHAR szBuf[STOPWATCH_MAX_DESC + ARRAYSIZE(c_szFmt_StopWatch_DbgOut) + 40];    // 8=dwTID 10=dwDelta
                
                if(psp->dwType > START_NODE)   // Find the previous associated node to get delta time
                {
                    pspPrev = psp - 1;
                    while(pspPrev >= g_pswi->pStopWatchList)
                    {
                        if((SWID(pspPrev->dwId) == SWID(psp->dwId)) &&  // Found a match
                           (pspPrev->dwTID == psp->dwTID) &&
                           (pspPrev->dwType == START_NODE))
                        {
                            dwDelta = psp->dwCount - pspPrev->dwCount;
                            break;
                        }
                        pspPrev--;
                    }
                }

                wnsprintf((LPTSTR)szBuf, ARRAYSIZE(szBuf), c_szFmt_StopWatch_DbgOut, psp->dwTID, psp->szDesc, dwDelta);
                OutputDebugString(szBuf);
            }
#endif

            if((dwMarkType == STOP_NODE) && (g_pswi->dwStopWatchMode & SPMODE_FLUSH) && (SWID(dwId) == SWID_FRAME))
            {
                StopWatchFlush();
            }
        }
        else
        {
            psp = g_pswi->pStopWatchList + (g_pswi->dwStopWatchListMax-1);  // Set the last node to a message so the user knows we ran out or mem
            psp->dwId = 0;
            psp->dwType = OUT_OF_NODES;
            psp->dwFlags = dwFlags;
            wnsprintf(psp->szDesc, STOPWATCH_MAX_DESC, TEXT("Out of perf timing nodes."));

#ifdef STOPWATCH_DEBUG
            if(g_pswi->dwStopWatchMode & SPMODE_DEBUGOUT)
                OutputDebugString(psp->szDesc);
#endif

            dwRC = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if((SWID(dwId) & g_pswi->dwStopWatchProfile) && (dwMarkType == START_NODE))
    {
        CapBreak(TRUE);
    }

    return(dwRC);
}

//===========================================================================================
//===========================================================================================
DWORD WINAPI StopWatchA(DWORD dwId, LPCSTR pszDesc, DWORD dwMarkType, DWORD dwFlags, DWORD dwCount)
{
#ifdef UNICODE
    INT rc;
    WCHAR wszDesc[STOPWATCH_MAX_DESC];

    rc = MultiByteToWideChar(CP_ACP, 0, pszDesc, -1, wszDesc, STOPWATCH_MAX_DESC);
  
    if(!rc)
        return(ERROR_NOT_ENOUGH_MEMORY);

    return(_StopWatch(dwId, (LPCTSTR)wszDesc, dwMarkType, dwFlags, dwCount));
#else
    return(_StopWatch(dwId, (LPCTSTR)pszDesc, dwMarkType, dwFlags, dwCount));
#endif
}

//===========================================================================================
//===========================================================================================
DWORD WINAPI StopWatchW(DWORD dwId, LPCWSTR pwszDesc, DWORD dwMarkType, DWORD dwFlags, DWORD dwCount)
{
#ifndef UNICODE    
    INT rc;
    CHAR szDesc[STOPWATCH_MAX_DESC];

    rc = WideCharToMultiByte(CP_ACP, 0, pwszDesc, -1, szDesc, STOPWATCH_MAX_DESC, NULL, NULL);
  
    if(!rc)
        return(ERROR_NOT_ENOUGH_MEMORY);

    return(_StopWatch(dwId, (LPCTSTR)szDesc, dwMarkType, dwFlags, dwCount));
#else
    return(_StopWatch(dwId, (LPCTSTR)pwszDesc, dwMarkType, dwFlags, dwCount));
#endif
}

//===========================================================================================
// Function: DWORD WINAPI StopWatchFlush(VOID)
//
// This function will flush any SPMODE_SHELL nodes to windir\shperf.log.  Calling this function
// will also clear all nodes.
//
// Return:   ERROR_SUCCESS if the log file was generated
//           ERROR_NO_DATA if the timing array is empty
//           ERROR_INVALID_DATA if stopwatch mode is not enabled or the timing array does
//              not exist.
//===========================================================================================
DWORD WINAPI StopWatchFlush(VOID)
{
    PSTOPWATCH psp;
    PSTOPWATCH psp1 = NULL;
    BOOL fWroteStartData;
    DWORD dwRC = ERROR_SUCCESS;
    DWORD dwWritten;
    DWORD dwDelta;
    DWORD dwPrevCount;
    DWORD dwCummDelta;
    DWORD dwLen = 0;
    HANDLE hFile;
    SYSTEMTIME st;
    TCHAR szBuf[STOPWATCH_MAX_BUF];
    TCHAR szFileName[MAX_PATH];
#ifdef STOPWATCH_DEBUG
    TCHAR szDbg[512];
#endif

    if((!g_pswi->dwStopWatchMode) || (g_pswi->pStopWatchList == NULL))
    {
        SetLastError(ERROR_INVALID_DATA);
        return(ERROR_INVALID_DATA);
    }

    GetSystemTime(&st);

    if(g_pswi->dwStopWatchListIndex > 0)
    {
        ENTERCRITICAL;
        if(g_pswi->dwStopWatchListIndex > 0)
        {
            g_pswi->dwStopWatchListIndex = 0;

            if(g_pswi->dwStopWatchMode & SPMODES)
            {

#ifdef STOPWATCH_DEBUG
                if(g_pswi->dwStopWatchMode & SPMODE_DEBUGOUT)
                    OutputDebugString(TEXT("Flushing shell perf data to shperf.log\r\n"));
#endif

#ifndef UNIX
                GetWindowsDirectory(szFileName, MAX_PATH);
                dwLen = lstrlen(szFileName);    // Used below as well to create msg trace log file
                if(szFileName[dwLen-1] == TEXT('\\'))   // See if windows is installed in the root
                    szFileName[dwLen-1] = TEXT('\0');
                StrNCat(szFileName, TEXT("\\shperf.log"), ARRAYSIZE(szFileName) -1);
#else
                StrCpyN(szFileName, TEXT("shperf.log"), ARRAYSIZE(szFileName)-1);
#endif

                if((hFile = CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
                {
                    SetFilePointer(hFile, 0, NULL, FILE_END);

                    psp = g_pswi->pStopWatchList;
                    while(psp->dwType != EMPTY_NODE)
                    {
#ifdef STOPWATCH_DEBUG
                        if(g_pswi->dwStopWatchMode & SPMODE_DEBUGOUT)
                        {
                            wnsprintf(szDbg, ARRAYSIZE(szDbg), TEXT("ID:%d TID:0x%x Type:%d Flgs:%d %s\r\n"),
                                psp->dwId, psp->dwTID, psp->dwType, psp->dwFlags, psp->szDesc);
                            OutputDebugString(szDbg);
                        }
#endif
                        if(psp->dwType == START_NODE)
                        {
                            wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("%02d%02d%02d%02d%02d%02d\t0x%x\t%s\t%lu\t"), 
                                st.wYear, st.wMonth, st.wDay,
                                st.wHour, st.wMinute, st.wSecond,
                                psp->dwId, psp->szDesc, psp->dwCount);

#ifdef STOPWATCH_DEBUG
                            if(g_pswi->dwStopWatchMode & SPMODE_DEBUGOUT)
                                OutputDebugString(TEXT("Found Start Node\r\n"));
#endif

                            dwDelta = dwCummDelta = 0;
                            dwPrevCount = psp->dwCount;

                            psp1 = psp + 1;
                            fWroteStartData = FALSE;
                            while(psp1->dwType != EMPTY_NODE)
                            {
#ifdef STOPWATCH_DEBUG
                                if(g_pswi->dwStopWatchMode & SPMODE_DEBUGOUT)
                                {
                                    wnsprintf(szDbg, ARRAYSIZE(szDbg), TEXT("  ID:%d TID:0x%x Type:%d Flgs:%d %s\r\n"),
                                        psp1->dwId, psp1->dwTID, psp1->dwType, psp1->dwFlags, psp1->szDesc);
                                    OutputDebugString(szDbg);
                                }
#endif
                                if((SWID(psp1->dwId) == SWID(psp->dwId)) && 
                                   (psp1->dwTID == psp->dwTID))     // Found a matching LAP or STOP node
                                {
                                    if(psp1->dwType != START_NODE)
                                    {
                                        dwDelta = psp1->dwCount - dwPrevCount;
                                        dwCummDelta += dwDelta;

                                        if(!fWroteStartData)
                                        {
                                            fWroteStartData = TRUE;
                                            WriteFile(hFile, szBuf, lstrlen(szBuf), &dwWritten, NULL);  // Write out start node data
                                        }
                                        wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("%s\t%lu,%lu,%lu\t"), psp1->szDesc, psp1->dwCount, dwDelta, dwCummDelta);
                                        WriteFile(hFile, szBuf, lstrlen(szBuf), &dwWritten, NULL);
#ifdef STOPWATCH_DEBUG
                                        if(g_pswi->dwStopWatchMode & SPMODE_DEBUGOUT)
                                            OutputDebugString(TEXT("  Found Lap/Stop Node\r\n"));
#endif
                                        dwPrevCount = psp1->dwCount;

                                        if(psp1->dwType == STOP_NODE)
                                            break;
                                    }
                                    else    // We have another start node that matches our Id/TID and we haven't had a stop.  Log as a missing stop.
                                    {
                                        if(!fWroteStartData)
                                        {
                                            fWroteStartData = TRUE;
                                            WriteFile(hFile, szBuf, lstrlen(szBuf), &dwWritten, NULL);  // Write out start node data
                                        }
                                        wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("ERROR: missing stop time"), psp1->szDesc, psp1->dwCount, dwDelta, dwCummDelta);
                                        WriteFile(hFile, szBuf, lstrlen(szBuf), &dwWritten, NULL);
                                        break;
                                    }
                                }
                
                                psp1++;
                            }

                            WriteFile(hFile, TEXT("\r\n"), 2, &dwWritten, NULL);
                        }
                        else if(psp->dwType == OUT_OF_NODES)
                        {
                            wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("%02d%02d%02d%02d%02d%02d\t0x%x\t%s\n"), 
                                st.wYear, st.wMonth, st.wDay,
                                st.wHour, st.wMinute, st.wSecond,
                                psp->dwId, psp->szDesc);
                            WriteFile(hFile, szBuf, lstrlen(szBuf), &dwWritten, NULL);
                        }
                        psp->dwType = EMPTY_NODE;
                        psp++;
                    }
                    FlushFileBuffers(hFile);
                    CloseHandle(hFile);
                }
                else
                {
#ifdef STOPWATCH_DEBUG
                    wnsprintf(szBuf, ARRAYSIZE(szBuf) - 1, TEXT("CreateFile failed on '%s'.  GLE=%d\n"), szFileName, GetLastError());
                    OutputDebugString(szBuf);
#endif
                    dwRC = ERROR_NO_DATA;
                }
            }
            else    // !(g_pswi->dwStopWatchMode)
            {
                psp = g_pswi->pStopWatchList;
                while(psp->dwType != EMPTY_NODE)
                {
                    psp->dwType = EMPTY_NODE;
                    psp++;
                }
            }
        }           // (g_pswi->dwStopWatchListIndex > 0)
        LEAVECRITICAL;
    }

    if(g_pswi->dwStopWatchMode & SPMODE_MSGTRACE)
    {
        int i;

#ifndef UNIX
        lstrcpy(&szFileName[dwLen], TEXT("\\msgtrace.log"));
#else
        StrCpy(szFileName, TEXT("msgtrace.log"));
#endif

        if((hFile = CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
        {
            SetFilePointer(hFile, 0, NULL, FILE_END);
            
            for (i = 0; i < (int)(g_pswi->dwStopWatchMaxMsgTime / g_pswi->dwStopWatchMsgInterval); ++i)
            {
                wnsprintf(szBuf, ARRAYSIZE(szBuf) - 1, TEXT("%02d%02d%02d%02d%02d%02d\tMsgTrace\t%4d - %4dms\t%d\r\n"),
                    st.wYear, st.wMonth, st.wDay,
                    st.wHour, st.wMinute, st.wSecond,
                    i * g_pswi->dwStopWatchMsgInterval, (i+1)*g_pswi->dwStopWatchMsgInterval-1, *(g_pswi->pdwStopWatchMsgTime + i));
                WriteFile(hFile, szBuf, lstrlen(szBuf), &dwWritten, NULL);
#ifdef STOPWATCH_DEBUG
                if(g_pswi->dwStopWatchMode & SPMODE_DEBUGOUT)
                    OutputDebugString(szBuf);
#endif
            }
              
            wnsprintf(szBuf, ARRAYSIZE(szBuf) - 1, TEXT("%02d%02d%02d%02d%02d%02d\tMsgTrace\tmsgs >= %dms\t%d\r\n"), 
                st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond,
                g_pswi->dwStopWatchMaxMsgTime, g_pswi->dwcStopWatchOverflow);
            WriteFile(hFile, szBuf, lstrlen(szBuf), &dwWritten, NULL);
#ifdef STOPWATCH_DEBUG
            if(g_pswi->dwStopWatchMode & SPMODE_DEBUGOUT)
                OutputDebugString(szBuf);
#endif

            if(g_pswi->dwStopWatchTraceMsg > 0)
            {
                wnsprintf(szBuf, ARRAYSIZE(szBuf) - 1, TEXT("%02d%02d%02d%02d%02d%02d\tMsgTrace\tmsg 0x%x occured %d times.\r\n"), 
                    st.wYear, st.wMonth, st.wDay,
                    st.wHour, st.wMinute, st.wSecond,
                    g_pswi->dwStopWatchTraceMsg, g_pswi->dwStopWatchTraceMsgCnt);
                WriteFile(hFile, szBuf, lstrlen(szBuf), &dwWritten, NULL);
#ifdef STOPWATCH_DEBUG
                if(g_pswi->dwStopWatchMode & SPMODE_DEBUGOUT)
                    OutputDebugString(szBuf);
#endif
            }
            
            FlushFileBuffers(hFile);
            CloseHandle(hFile);
        }
        else
        {
#ifdef STOPWATCH_DEBUG
            wnsprintf(szBuf, ARRAYSIZE(szBuf) - 1, TEXT("CreateFile failed on '%s'.  GLE=%d\n"), szFileName, GetLastError());
            OutputDebugString(szBuf);
#endif
            dwRC = ERROR_NO_DATA;
        }
    }

    return(dwRC);
}

//===========================================================================================
// The following StopWatch messages are used to drive the timer msg handler.  The timer proc is used
// as a means of delaying while watching paint messages.  If the defined number of timer ticks has 
// passed without getting any paint messages, then we mark the time of the last paint message we've 
// saved as the stop time.
//===========================================================================================
VOID CALLBACK StopWatch_TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    StopWatch_TimerHandler(hwnd, 1, SWMSG_TIMER, NULL);
}

//===========================================================================================
//===========================================================================================
BOOL WINAPI StopWatch_TimerHandler(HWND hwnd, UINT uInc, DWORD dwFlag, MSG* pmsg)
{
    static INT iNumTimersRcvd = 0;
    static DWORD dwCnt = 0;
    static BOOL bActive = FALSE;
    static BOOL bHaveFirstPaintMsg = FALSE;

    switch(dwFlag)
    {
        case SWMSG_PAINT:
            if(bActive)
            {
                dwCnt = GetPerfTime();  // Save tick for last paint message
                iNumTimersRcvd = 0;     // Reset timers received count

                if(!bHaveFirstPaintMsg)
                {
                    TCHAR szClassName[40];  // If the class matches and its the first paint msg mark a lap time
                    LPCTSTR ptr;
                    GetClassName(pmsg->hwnd, szClassName, ARRAYSIZE(szClassName)-1);

                    ptr = g_pswi->pszClassNames;
                    while(*ptr)
                    {
                        if(lstrcmpi(szClassName, ptr) == 0)
                        {
                            bHaveFirstPaintMsg = TRUE;
                            StopWatch_LapTimed(SWID_FRAME, TEXT("Shell Frame 1st Paint"), SPMODE_SHELL | SPMODE_DEBUGOUT, dwCnt);
                            break;
                        }
                        ptr = ptr + (lstrlen(ptr) + 1);
                    }
                }
            }
            break;

        case SWMSG_TIMER:
            iNumTimersRcvd += uInc;
            if(iNumTimersRcvd >= 3)     // If we've received this arbitrary # of timer msgs, mark stop time using the saved last paint tick count
            {
                const TCHAR c_szFmtShellStop[] = TEXT("Shell Frame Stop (%s)");
                TCHAR szTitle[STOPWATCH_MAX_TITLE];
                TCHAR szText[ARRAYSIZE(c_szFmtShellStop) + STOPWATCH_MAX_TITLE + 1];

                KillTimer(hwnd, ID_STOPWATCH_TIMER);
                GetWindowText(hwnd, szTitle, ARRAYSIZE(szTitle)-1);
                wnsprintf(szText, ARRAYSIZE(szText), c_szFmtShellStop, szTitle);
                StopWatch_StopTimed(SWID_FRAME, szText, SPMODE_SHELL | SPMODE_DEBUGOUT, dwCnt);
                bHaveFirstPaintMsg = FALSE;
                bActive = FALSE;  // Done timing

                if((g_pswi->dwStopWatchMode & (SPMODE_EVENT | SPMODE_SHELL)) == (SPMODE_EVENT | SPMODE_SHELL))
                {
                    StopWatch_SignalEvent();
                }
            }
            break;

        case SWMSG_CREATE:
            dwCnt = GetPerfTime();      // Save initial tick in case we don't have a paint when we exceed the # of SWMSG_TIMER above
            iNumTimersRcvd = 0;
            bHaveFirstPaintMsg = FALSE;
            bActive = (BOOL)SetTimer(hwnd, ID_STOPWATCH_TIMER, g_pswi->dwStopWatchPaintInterval, StopWatch_TimerProc);   // Use timer to determine when painting is done
            break;

        case SWMSG_STATUS:
            break;
    }

    return(bActive);   // Timing status active or not
}

//===========================================================================================
// This function is used to key off of WM_KEYDOWN to start timing when navigating inplace
//===========================================================================================
VOID WINAPI StopWatch_CheckMsg(HWND hwnd, MSG msg, LPCSTR lpStr)
{
    TCHAR szText[80];
    
#ifdef STOPWATCH_DEBUG
    if(g_pswi->dwStopWatchMode & SPMODE_TEST)    // Used to verify message assumptions
    {
        wnsprintf((LPTSTR)szText, ARRAYSIZE(szText), TEXT("Hwnd=0x%08x Msg=0x%x\r\n"), msg.hwnd, msg.message);
        OutputDebugString(szText);
    }
#endif

    if(g_pswi->dwStopWatchMode & SPMODE_SHELL)
    {
        if(!StopWatch_TimerHandler(hwnd, 0, SWMSG_STATUS, &msg) &&
            (((msg.message == WM_KEYDOWN) && (msg.wParam == VK_RETURN)) ||
            ((msg.message == WM_KEYDOWN) && (msg.wParam == VK_BACK)))
            )  // Navigating within the same window
        {
            wnsprintf(szText, ARRAYSIZE(szText), TEXT("Shell Frame Same%s"), lpStr);
            StopWatch_TimerHandler(hwnd, 0, SWMSG_CREATE, &msg);
            StopWatch_Start(SWID_FRAME, szText, SPMODE_SHELL | SPMODE_DEBUGOUT);
        }
    }

    // Compute the time it took to get the message. Then increment approp MsgTime bucket
    if(g_pswi->dwStopWatchMode & SPMODE_MSGTRACE)
    {
        DWORD dwTick = GetTickCount();
        DWORD dwElapsed;
#ifdef STOPWATCH_DEBUG
        TCHAR szMsg[256];
#endif

        g_pswi->dwStopWatchLastLocation = 0;
        
        if(dwTick > msg.time)
        {
            dwElapsed = dwTick - msg.time;

            if(dwElapsed >= g_pswi->dwStopWatchMaxMsgTime)
            {
                ++g_pswi->dwcStopWatchOverflow;
                
#ifdef STOPWATCH_DEBUG
                if(g_pswi->dwStopWatchMode & SPMODE_DEBUGOUT)
                {
                    TCHAR szClassName[40]; 
                    TCHAR szMsgName[20];
                
                    GetClassName(msg.hwnd, szClassName, ARRAYSIZE(szClassName) - 1);
                    if(g_pswi->pfnDecodeMessage != NULL)
                        StrCpyN(szMsgName, g_pswi->pfnDecodeMessage(msg.message), ARRAYSIZE(szMsgName) - 1);
                    else
                        wnsprintf(szMsgName, ARRAYSIZE(szMsgName) - 1, "0x%x", msg.message);
                    wnsprintf(szMsg, ARRAYSIZE(szMsg) - 1, TEXT("MsgTrace (%s) loc=%d, ms=%d >= %d, hwnd=%x, wndproc=%x, msg=%s, w=%x, l=%x\r\n"), 
                        szClassName, g_pswi->dwStopWatchLastLocation, dwElapsed, g_pswi->dwStopWatchMaxMsgTime, msg.hwnd, GetClassLongPtr(msg.hwnd, GCLP_WNDPROC), szMsgName, msg.wParam, msg.lParam);
                    OutputDebugString(szMsg);                                
                }
#endif
            }
            else
            {
                ++(*(g_pswi->pdwStopWatchMsgTime + (dwElapsed / g_pswi->dwStopWatchMsgInterval)));
            }
        }

        if(g_pswi->dwStopWatchTraceMsg == msg.message)
            ++g_pswi->dwStopWatchTraceMsgCnt;
            
        g_pswi->dwStopWatchLastLocation = 0;
    }
}

//===========================================================================================
//===========================================================================================
VOID WINAPI StopWatch_SetMsgLastLocation(DWORD dwLast)
{
    g_pswi->dwStopWatchLastLocation = dwLast;
}

//===========================================================================================
// Logs messages that took longer than g_pswi->dwStopWatchMaxDispatchTime to be dispatched
//===========================================================================================
DWORD WINAPI StopWatch_DispatchTime(BOOL fStartTime, MSG msg, DWORD dwStart)
{
    DWORD dwTime = 0;
    TCHAR szMsg[256];
    
    if(fStartTime)
    {
        if(g_pswi->dwStopWatchTraceMsg == msg.message)
            CapBreak(TRUE);

        StopWatch(SWID_MSGDISPATCH, TEXT("+Dispatch"), START_NODE, SPMODE_MSGTRACE | SPMODE_DEBUGOUT, dwStart);

        dwTime = GetPerfTime();

    }
    else
    {
        dwTime = GetPerfTime();
        
        if(g_pswi->dwStopWatchTraceMsg == msg.message)
            CapBreak(FALSE);
            
        if((dwTime - dwStart) >= g_pswi->dwStopWatchMaxDispatchTime)
        {
            TCHAR szClassName[40];
            TCHAR szMsgName[20];

            GetClassName(msg.hwnd, szClassName, ARRAYSIZE(szClassName) - 1);
            if(g_pswi->pfnDecodeMessage != NULL)
                StrCpyN(szMsgName, g_pswi->pfnDecodeMessage(msg.message), ARRAYSIZE(szMsgName) - 1);
            else
                wnsprintf(szMsgName, ARRAYSIZE(szMsgName) - 1, "0x%x", msg.message);
            wnsprintf(szMsg, ARRAYSIZE(szMsg) - 1, TEXT("-Dispatch (%s) ms=%d > %d, hwnd=%x, wndproc=%x, msg=%s(%x), w=%x, l=%x"), 
                szClassName, dwTime - dwStart, g_pswi->dwStopWatchMaxDispatchTime, msg.hwnd, GetClassLongPtr(msg.hwnd, GCLP_WNDPROC), szMsgName, msg.message, msg.wParam, msg.lParam);
                
            StopWatch(SWID_MSGDISPATCH, szMsg, STOP_NODE, SPMODE_MSGTRACE | SPMODE_DEBUGOUT, dwTime);

#ifdef STOPWATCH_DEBUG
            if(g_pswi->dwStopWatchMode & SPMODE_DEBUGOUT)
            {
                lstrcat(szMsg, "\n");
                OutputDebugString(szMsg);
            }
#endif
        }
    }
    return(dwTime);
}

//===========================================================================================
// Mark shell/browser frame creation start time
//===========================================================================================
VOID WINAPI StopWatch_MarkFrameStart(LPCSTR lpExplStr)
{
    TCHAR szText[80];
    DWORD dwTime = GetPerfTime();
    if(g_pswi->dwStopWatchMode & SPMODE_SHELL)
    {
        wnsprintf(szText, ARRAYSIZE(szText), TEXT("Shell Frame Start%s"), lpExplStr);
        StopWatch_StartTimed(SWID_FRAME, szText, SPMODE_SHELL | SPMODE_DEBUGOUT, dwTime);
    }
    if(g_pswi->dwStopWatchMode & SPMODE_BROWSER)  // Used to get the start of browser total download time
    {
        StopWatch_LapTimed(SWID_BROWSER_FRAME, TEXT("Thread Start"), SPMODE_BROWSER | SPMODE_DEBUGOUT, dwTime);
    }
    if(g_pswi->dwStopWatchMode & SPMODE_JAVA)  // Used to get the start of java applet load time
    {
        StopWatch_StartTimed(SWID_JAVA_APP, TEXT("Java Applet Start"), SPMODE_JAVA | SPMODE_DEBUGOUT, dwTime);
    }
}

//===========================================================================================
// Mark shell/browser navigate in same frame start time
//===========================================================================================
VOID WINAPI StopWatch_MarkSameFrameStart(HWND hwnd)
{
    DWORD dwTime = GetPerfTime();
    
    if(g_pswi->dwStopWatchMode & SPMODE_SHELL)
    {
        StopWatch_TimerHandler(hwnd, 0, SWMSG_CREATE, NULL);
        StopWatch_StartTimed(SWID_FRAME, TEXT("Shell Frame Same"), SPMODE_SHELL | SPMODE_DEBUGOUT, dwTime);
    }
    if(g_pswi->dwStopWatchMode & SPMODE_BROWSER)  // Used to get browser total download time
    {
        StopWatch_StartTimed(SWID_BROWSER_FRAME, TEXT("Browser Frame Same"), SPMODE_BROWSER | SPMODE_DEBUGOUT, dwTime);
    }
    if(g_pswi->dwStopWatchMode & SPMODE_JAVA)  // Used to get java applet load time
    {
        StopWatch_StartTimed(SWID_JAVA_APP, TEXT("Java Applet Same"), SPMODE_JAVA | SPMODE_DEBUGOUT, dwTime);
    }
}

//===========================================================================================
// When browser or java perf timing mode is enabled, use "Done" or "Applet Started" 
// in the status bar to get load time.
//===========================================================================================
VOID WINAPI StopWatch_MarkJavaStop(LPCSTR  lpStringToSend, HWND hwnd, BOOL fChType)
{
    const TCHAR c_szFmtJavaStop[] = TEXT("Java Applet Stop (%s)");
    TCHAR szTitle[STOPWATCH_MAX_TITLE];
    TCHAR szText[STOPWATCH_MAX_TITLE + ARRAYSIZE(c_szFmtJavaStop) + 1];

    if(g_pswi->dwStopWatchMode & SPMODE_JAVA)
    {
        if((lpStringToSend != NULL) && (lstrncmpW((LPWSTR)lpStringToSend, TEXTW("Applet started"), ARRAYSIZE(TEXTW("Applet started"))) == 0))
        {
            GetWindowText(hwnd, szTitle, ARRAYSIZE(szTitle)-1);
            wnsprintf(szText, ARRAYSIZE(szText), c_szFmtJavaStop, szTitle);
            StopWatch_Stop(SWID_JAVA_APP, szText, SPMODE_SHELL | SPMODE_DEBUGOUT);
        }
    }
}

//===========================================================================================
//===========================================================================================
DWORD WINAPI GetPerfTime(VOID)
{
    static __int64 freq;
    __int64 curtime;

    if (!freq)
        QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

    QueryPerformanceCounter((LARGE_INTEGER *)&curtime);

    ASSERT((((curtime * 1000) / freq) >> 32) == 0);
    
    return (DWORD)((curtime * 1000) / freq);
}

