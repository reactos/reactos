//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       debug.cpp
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
/ Title;
/   debug.cpp
/
/ Authors;
/   Jeffrey Saathoff (jeffreys)
/
/ Notes;
/   Provides printf style debug output
/----------------------------------------------------------------------------*/
#include "pch.h"
#include <stdio.h>
#include <comctrlp.h>
#pragma hdrstop


#ifdef DEBUG

DWORD g_dwTraceMask = 0;
DWORD g_tlsDebug = 0xffffffffL;

#define MAX_CALL_DEPTH  64


class CDebugStack
{
private:
    DWORD m_dwThreadID;
    LONG m_cDepth;
    struct
    {
        BOOL    fTracedYet;
        LPCTSTR pszFunctionName;
        DWORD   dwMask;
    }
    m_CallStack[MAX_CALL_DEPTH];

public:
    CDebugStack() : m_dwThreadID(GetCurrentThreadId()), m_cDepth(-1)
    { ZeroMemory(&m_CallStack, SIZEOF(m_CallStack)); }

public:
    void _Indent(LONG iDepth, LPCTSTR pszFormat, ...);
    void _vIndent(LONG iDepth, LPCTSTR pszFormat, va_list *pva);
    BOOL _TraceProlog(LONG iDepth, BOOL fForce);
    void _TraceEnter(DWORD dwMask, LPCTSTR pName);
    void _TraceLeave(void);
    void _Trace(BOOL bForce, LPCTSTR pszFormat, ...);
    void _vTrace(BOOL bForce, LPCTSTR pszFormat, va_list *pva);
    void _TraceGUID(LPCTSTR pPrefix, REFGUID rGUID);
    void _TraceAssert(int iLine, LPTSTR pFilename, LPTSTR pCondition);
};
typedef CDebugStack *PDEBUGSTACK;

class CDebugStackHolder
{
private:
    HDPA m_hDebugStackList;
    CRITICAL_SECTION m_csStackList;

public:
    CDebugStackHolder() : m_hDebugStackList(NULL) { InitializeCriticalSection(&m_csStackList); }
    ~CDebugStackHolder();

public:
    void Add(PDEBUGSTACK pDebugStack);
    void Remove(PDEBUGSTACK pDebugStack);
};
typedef CDebugStackHolder *PDEBUGSTACKHOLDER;

PDEBUGSTACKHOLDER g_pStackHolder = NULL;


/*-----------------------------------------------------------------------------
/ _Indent
/ -------
/   Output to the debug stream indented by n columns.
/
/ In:
/   i = column to indent to.
/   pszFormat -> string to be indented
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

void CDebugStack::_Indent(LONG iDepth, LPCTSTR pszFormat, ...)
{
    va_list va;

    va_start(va, pszFormat);
    _vIndent(iDepth, pszFormat, &va);
    va_end(va);
}


void CDebugStack::_vIndent(LONG iDepth, LPCTSTR pszFormat, va_list *pva)
{
    TCHAR szStringBuffer[2048];
    szStringBuffer[0] = TEXT('\0');

    wsprintf(szStringBuffer, TEXT("%08x "), m_dwThreadID);

    iDepth = min(iDepth, MAX_CALL_DEPTH - 1);
    for ( ; iDepth > 0 ; iDepth-- )
        lstrcat(szStringBuffer, TEXT("  "));

    wvsprintf(szStringBuffer + lstrlen(szStringBuffer), pszFormat, *pva);
    lstrcat(szStringBuffer, TEXT("\n"));

    OutputDebugString(szStringBuffer);
}


/*-----------------------------------------------------------------------------
/ _TraceProlog
/ -------------
/   Handle the prolog to a prefix string, including outputting the
/   function name if we haven't already.
/
/ In:
/   iDepth = depth in the call stack
/   fForce = ignore flags
/
/ Out:
/   BOOL if trace output should be made
/----------------------------------------------------------------------------*/
BOOL CDebugStack::_TraceProlog(LONG iDepth, BOOL fForce)
{
    if ( iDepth < 0 || iDepth >= MAX_CALL_DEPTH )
        return FALSE;

    if  ( (g_dwTraceMask & m_CallStack[iDepth].dwMask) || fForce )
    {
        if ( !m_CallStack[iDepth].fTracedYet )
        {
            if ( iDepth > 0 )
                _TraceProlog(iDepth-1, TRUE);

            _Indent(iDepth, m_CallStack[iDepth].pszFunctionName);
            m_CallStack[iDepth].fTracedYet = TRUE;
        }

        return TRUE;
    }

    return FALSE;
}


/*-----------------------------------------------------------------------------
/ _TraceEnter
/ ------------
/   Set the debugging call stack up to indicate which function we are in.
/
/ In:
/   pName -> function name to be displayed in subsequent trace output.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void CDebugStack::_TraceEnter(DWORD dwMask, LPCTSTR pName)
{
    m_cDepth++;

    if ( m_cDepth < MAX_CALL_DEPTH )
    {
        if ( !pName )    
            pName = TEXT("<no name>");         // no function name given

        m_CallStack[m_cDepth].fTracedYet = FALSE;
        m_CallStack[m_cDepth].pszFunctionName = pName;
        m_CallStack[m_cDepth].dwMask = dwMask;

        //if ( m_cDepth > 0 )
        //    _TraceProlog(m_cDepth-1, FALSE);
    }
}


/*-----------------------------------------------------------------------------
/ _TraceLeave
/ ------------
/   On exit from a function this will adjust the function stack pointer to 
/   point to our previous function.  If no trace output has been made then 
/   we will output the function name on a single line (to indicate we went somewhere).
/
/ In:
/    -
/ Out:
/   -
/----------------------------------------------------------------------------*/
void CDebugStack::_TraceLeave(void)
{
    //_TraceProlog(m_cDepth, FALSE);

    //if ( !m_cDepth && m_CallStack[0].fTracedYet )
    //    OutputDebugString(TEXT("\n"));
    
    m_cDepth = max(m_cDepth-1, -1);         // account for underflow
}


/*-----------------------------------------------------------------------------
/ _Trace
/ -------
/   Perform printf formatting to the debugging stream.  We indent the output
/   and stream the function name as required to give some indication of 
/   call stack depth.
/
/ In:
/   pszFormat -> printf style formatting string
/   ... = arguments as required for the formatting
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void CDebugStack::_Trace(BOOL bForce, LPCTSTR pszFormat, ...)
{
    va_list va;

    va_start(va, pszFormat);
    _vTrace(bForce, pszFormat, &va);
    va_end(va);
}


void CDebugStack::_vTrace(BOOL bForce, LPCTSTR pszFormat, va_list *pva)
{
    if ( _TraceProlog(m_cDepth, bForce) || bForce )
        _vIndent(m_cDepth+1, pszFormat, pva);
}


/*-----------------------------------------------------------------------------
/ _TraceGUID
/ -----------
/   Given a GUID output it into the debug string, first we try and map it
/   to a name (ie. IShellFolder), if that didn't work then we convert it
/   to its human readable form.
/
/ In:
/   pszPrefix -> prefix string
/   lpGuid -> guid to be streamed   
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
#ifdef UNICODE
#define MAP_GUID(x)     &x, TEXT(""L#x)
#else
#define MAP_GUID(x)     &x, TEXT(""#x)
#endif

#define MAP_GUID2(x,y)  MAP_GUID(x), MAP_GUID(y)

const struct 
{
    const GUID* m_pGUID;
    LPCTSTR     m_pName;
}
_guid_map[] = 
{
    MAP_GUID(IID_IUnknown),
    MAP_GUID(IID_IClassFactory),
    MAP_GUID(IID_IDropTarget),
    MAP_GUID(IID_IDataObject),
    MAP_GUID(IID_IPersist),
    MAP_GUID(IID_IOleWindow),

    MAP_GUID2(IID_INewShortcutHookA, IID_INewShortcutHookW),
    MAP_GUID(IID_IShellBrowser),
    MAP_GUID(IID_IShellView),
    MAP_GUID(IID_IContextMenu),
    MAP_GUID(IID_IShellIcon),
    MAP_GUID(IID_IShellFolder),
    MAP_GUID(IID_IShellExtInit),
    MAP_GUID(IID_IShellPropSheetExt),
    MAP_GUID(IID_IPersistFolder),  
    MAP_GUID2(IID_IExtractIconA, IID_IExtractIconW),
    MAP_GUID2(IID_IShellLinkA, IID_IShellLinkW),
    MAP_GUID2(IID_IShellCopyHookA, IID_IShellCopyHookW),
    MAP_GUID2(IID_IFileViewerA, IID_IFileViewerW),
    MAP_GUID(IID_ICommDlgBrowser),
    MAP_GUID(IID_IEnumIDList),
    MAP_GUID(IID_IFileViewerSite),
    MAP_GUID(IID_IContextMenu2),
    MAP_GUID2(IID_IShellExecuteHookA, IID_IShellExecuteHookW),
    MAP_GUID(IID_IPropSheetPage),
    MAP_GUID(IID_IShellView2),
    MAP_GUID(IID_IUniformResourceLocator),
};

void CDebugStack::_TraceGUID(LPCTSTR pPrefix, REFGUID rGUID)
{
    TCHAR szGUID[40];
    LPCTSTR pName = NULL;
    int i;
    
    for ( i = 0 ; i < ARRAYSIZE(_guid_map); i++ )
    {
        if ( IsEqualGUID(rGUID, *_guid_map[i].m_pGUID) )
        {
            pName = _guid_map[i].m_pName;
            break;
        }
    }

    if ( !pName )
    {
        SHStringFromGUID(rGUID, szGUID, ARRAYSIZE(szGUID));
        pName = szGUID;
    }

    _Trace(FALSE, TEXT("%s %s"), pPrefix, pName);
}


/*-----------------------------------------------------------------------------
/ _TraceAssert
/ -------------
/   Our assert handler, always prints messages but only breaks if enabled
/   in the trace mask.
/
/ In:
/   iLine = line 
/   pFilename -> filename of the file we asserted in
/   pCondition -> assert condition that failed
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void CDebugStack::_TraceAssert(int iLine, LPTSTR pFilename, LPTSTR pCondition)
{
    // nb: TRUE --> asserts always displayed
    _Trace(TRUE, TEXT("Assert failed: \"%s\""), pCondition);
    _Trace(TRUE, TEXT("File: %s, Line %d"), pFilename, iLine);

    if ( g_dwTraceMask & TRACE_COMMON_ASSERT )
        DebugBreak();
}


/*-----------------------------------------------------------------------------
/ ~CDebugStackHolder
/ ------------------
/   Free any DebugStack objects that exist
/
/ In:
/   -
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
int CALLBACK
_DeleteCB(LPVOID pVoid, LPVOID /*pData*/)
{
    PDEBUGSTACK pDebugStack = (PDEBUGSTACK)pVoid;
    if (pDebugStack)
    {
        //pDebugStack->_Trace(TRUE, TEXT("~CDebugStackHolder destroying DebugStack"));
        delete pDebugStack;
    }
    return 1;
}

CDebugStackHolder::~CDebugStackHolder()
{
    EnterCriticalSection(&m_csStackList);

    if (NULL != m_hDebugStackList)
    {
        DPA_DestroyCallback(m_hDebugStackList, _DeleteCB, NULL);
        m_hDebugStackList = NULL;
    }

    LeaveCriticalSection(&m_csStackList);
    DeleteCriticalSection(&m_csStackList);
}


/*-----------------------------------------------------------------------------
/ CDebugStackHolder::Add
/ ----------------------
/   Saves the DebugStack object in a list
/
/ In:
/   PDEBUGSTACK pointer to the thread's debug stack object
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void
CDebugStackHolder::Add(PDEBUGSTACK pDebugStack)
{
    EnterCriticalSection(&m_csStackList);

    if (NULL == m_hDebugStackList)
        m_hDebugStackList = DPA_Create(4);

    if (NULL != m_hDebugStackList)
        DPA_AppendPtr(m_hDebugStackList, pDebugStack);

    LeaveCriticalSection(&m_csStackList);
}


/*-----------------------------------------------------------------------------
/ CDebugStackHolder::Remove
/ -------------------------
/   Removes the DebugStack object from the list
/
/ In:
/   PDEBUGSTACK pointer to the thread's debug stack object
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void
CDebugStackHolder::Remove(PDEBUGSTACK pDebugStack)
{
    EnterCriticalSection(&m_csStackList);

    if (NULL != m_hDebugStackList)
    {
        int iStack = DPA_GetPtrIndex(m_hDebugStackList, pDebugStack);

        if (-1 != iStack)
            DPA_DeletePtr(m_hDebugStackList, iStack);
    }

    LeaveCriticalSection(&m_csStackList);
}


/*-----------------------------------------------------------------------------
/ GetThreadStack
/ --------------
/   Create (if necessary) and return the per-thread debug stack object.
/
/ In:
/   -
/
/ Out:
/   PDEBUGSTACK pointer to the thread's debug stack object
/----------------------------------------------------------------------------*/
PDEBUGSTACK GetThreadStack()
{
    PDEBUGSTACK pDebugStack;

    if (0xffffffffL == g_tlsDebug)
        return NULL;

    pDebugStack = (PDEBUGSTACK)TlsGetValue(g_tlsDebug);

    if (!pDebugStack)
    {
        pDebugStack = new CDebugStack;
        TlsSetValue(g_tlsDebug, pDebugStack);

        if (!g_pStackHolder)
            g_pStackHolder = new CDebugStackHolder;

        if (g_pStackHolder)
            g_pStackHolder->Add(pDebugStack);
    }

    return pDebugStack;
}
    

/*-----------------------------------------------------------------------------
/ DoTraceSetMask
/ --------------
/   Adjust the trace mask to reflect the state given.
/
/ In:
/   dwMask = mask for enabling / disable trace output
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceSetMask(DWORD dwMask)
{
    g_dwTraceMask = dwMask;
}


/*-----------------------------------------------------------------------------
/ DoTraceSetMaskFromRegKey
/ ------------------------
/   Pick up the TraceMask value from the given registry key and
/   set the trace mask using that.
/
/ In:
/   hkRoot = handle of open key
/   pszSubKey = name of subkey to open
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceSetMaskFromRegKey(HKEY hkRoot, LPCTSTR pszSubKey)
{
    HKEY hKey;

    if (ERROR_SUCCESS == RegOpenKey(hkRoot, pszSubKey, &hKey))
    {
        DWORD dwTraceMask = 0;
        DWORD cbTraceMask = SIZEOF(dwTraceMask);

        RegQueryValueEx(hKey,
                        TEXT("TraceMask"),
                        NULL,
                        NULL,
                        (LPBYTE)&dwTraceMask,
                        &cbTraceMask);
        DoTraceSetMask(dwTraceMask);
        RegCloseKey(hKey);
    }
}


/*-----------------------------------------------------------------------------
/ DoTraceSetMaskFromCLSID
/ -----------------------
/   Pick up the TraceMask value from the given CLSID value and
/   set the trace mask using that.
/
/ In:
/   rCLSID = CLSID to query the value from
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceSetMaskFromCLSID(REFCLSID rCLSID)
{
    TCHAR szClsidKey[48] = TEXT("CLSID\\");
    int nLength = lstrlen(szClsidKey);

    if (!SHStringFromGUID(rCLSID, szClsidKey + nLength, ARRAYSIZE(szClsidKey) - nLength))
        return;

    DoTraceSetMaskFromRegKey(HKEY_CLASSES_ROOT, szClsidKey);
}


/*-----------------------------------------------------------------------------
/ DoTraceEnter
/ ------------
/   Set the debugging call stack up to indicate which function we are in.
/
/ In:
/   pName -> function name to be displayed in subsequent trace output.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceEnter(DWORD dwMask, LPCTSTR pName)
{
    PDEBUGSTACK pDebugStack = GetThreadStack();

    if (pDebugStack)
        pDebugStack->_TraceEnter(dwMask, pName);
}


/*-----------------------------------------------------------------------------
/ DoTraceLeave
/ ------------
/   On exit from a function this will adjust the function stack pointer to 
/   point to our previous function.  If no trace output has been made then 
/   we will output the function name on a single line (to indicate we went somewhere).
/
/ In:
/    -
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceLeave(void)
{
    PDEBUGSTACK pDebugStack = GetThreadStack();

    if (pDebugStack)
        pDebugStack->_TraceLeave();
}


/*-----------------------------------------------------------------------------
/ DoTrace
/ -------
/   Perform printf formatting to the debugging stream.  We indent the output
/   and stream the function name as required to give some indication of 
/   call stack depth.
/
/ In:
/   pszFormat -> printf style formatting string
/   ... = arguments as required for the formatting
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTrace(LPCTSTR pszFormat, ...)
{
    PDEBUGSTACK pDebugStack = GetThreadStack();
    va_list va;

    if (pDebugStack)
    {
        va_start(va, pszFormat);
        pDebugStack->_vTrace(FALSE, pszFormat, &va);
        va_end(va);
    }
}


/*-----------------------------------------------------------------------------
/ DoTraceGuid
/ -----------
/   Given a GUID output it into the debug string, first we try and map it
/   to a name (ie. IShellFolder), if that didn't work then we convert it
/   to its human readable form.
/
/ In:
/   pszPrefix -> prefix string
/   lpGuid -> guid to be streamed   
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceGUID(LPCTSTR pPrefix, REFGUID rGUID)
{
    PDEBUGSTACK pDebugStack = GetThreadStack();

    if (pDebugStack)
        pDebugStack->_TraceGUID(pPrefix, rGUID);
}


/*-----------------------------------------------------------------------------
/ DoTraceAssert
/ -------------
/   Our assert handler, out faults it the trace mask as enabled assert
/   faulting.
/
/ In:
/   iLine = line 
/   pFilename -> filename of the file we asserted in
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceAssert(int iLine, LPTSTR pFilename, LPTSTR pCondition)
{
    PDEBUGSTACK pDebugStack = GetThreadStack();

    if (pDebugStack)
        pDebugStack->_TraceAssert(iLine, pFilename, pCondition);
}


/*-----------------------------------------------------------------------------
/ DebugThreadDetach
/ DebugProcessAttach
/ DebugProcessDetach
/ -------------
/   These must be called from DllMain
/
/ In:
/   -
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DebugThreadDetach(void)
{
    PDEBUGSTACK pDebugStack;

    if (0xffffffffL == g_tlsDebug)
        return;

    pDebugStack = (PDEBUGSTACK)TlsGetValue(g_tlsDebug);

    if (pDebugStack)
    {
        if (g_pStackHolder)
            g_pStackHolder->Remove(pDebugStack);

        delete pDebugStack;
        TlsSetValue(g_tlsDebug, NULL);
    }
}

void DebugProcessAttach(void)
{
    g_tlsDebug = TlsAlloc();
}

void DebugProcessDetach(void)
{
    DebugThreadDetach();

    if (NULL != g_pStackHolder)
    {
        delete g_pStackHolder;
        g_pStackHolder = NULL;
    }

    if (0xffffffffL != g_tlsDebug)
    {
        TlsFree(g_tlsDebug);
        g_tlsDebug = 0xffffffffL;
    }
}


#endif  // DEBUG
