/*----------------------------------------------------------------------------
/ Title;
/   debug.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Provides printf style debug output
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#include "stdio.h"
#pragma hdrstop


#ifdef DEBUG

//#define MYDOCS_LOGFILE 1
#define MAX_CALL_DEPTH  64


#ifdef MYDOCS_LOGFILE
// Magic debug flags

#define TRACE_CORE          0x00000001
#define TRACE_FOLDER        0x00000002
#define TRACE_ENUM          0x00000004
#define TRACE_ICON          0x00000008
#define TRACE_DATAOBJ       0x00000010
#define TRACE_IDLIST        0x00000020
#define TRACE_CALLBACKS     0x00000040
#define TRACE_COMPAREIDS    0x00000080
#define TRACE_DETAILS       0x00000100
#define TRACE_QI            0x00000200
#define TRACE_EXT           0x00000400
#define TRACE_UTIL          0x00000800
#define TRACE_SETUP         0x00001000
#define TRACE_PROPS         0x00002000
#define TRACE_COPYHOOK      0x00004000
#define TRACE_FACTORY       0x00008000

#define LOG_TRACE_MASK (TRACE_SETUP|TRACE_UTIL|TRACE_FOLDER|TRACE_QI)
CRITICAL_SECTION cs;
TCHAR szLogFile[ MAX_PATH ];

LONG  g_cDepthLF = -1;
DWORD g_dwTraceMaskLF = 0;

struct
{
    BOOL    m_fTracedYet : 1;
    LPCTSTR m_pFunctionName;
    DWORD   m_dwMask;
}
g_CallStackLF[MAX_CALL_DEPTH];

#endif


/*-----------------------------------------------------------------------------
/ Locals & helper functions
/----------------------------------------------------------------------------*/

LONG  g_cDepth = -1;
DWORD g_dwTraceMask = 0;


struct
{
    BOOL    m_fTracedYet : 1;
    LPCTSTR m_pFunctionName;
    DWORD   m_dwMask;
}
g_CallStack[MAX_CALL_DEPTH];

#define BUFFER_SIZE 4096

static TCHAR szIndentBuffer[BUFFER_SIZE];
static TCHAR szTraceBuffer[BUFFER_SIZE];
#ifdef UNICODE
#ifdef MYDOCS_LOGFILE
static CHAR szAnsi[BUFFER_SIZE];
#endif
#endif


#ifdef MYDOCS_LOGFILE
/*-----------------------------------------------------------------------------
/ _indent_logfile
/ ---------------
/   Output to the debug stream indented by n columns.
/
/ In:
/   i = column to indent to.
/   pString -> string to be indented
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void _indent_logfile( LONG i, LPCTSTR pString )
{
    HANDLE hFile;

    EnterCriticalSection( &cs );
    hFile = CreateFile( szLogFile,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                       );

    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dw;


        szIndentBuffer[0] = TEXT('\0');

        wsprintf(szIndentBuffer, TEXT("%08x "), GetCurrentThreadId());

        for ( ; i > 0 ; i-- )
            lstrcat(szIndentBuffer, TEXT("  "));

        lstrcat(szIndentBuffer, pString);
        lstrcat(szIndentBuffer, TEXT("\r\n"));


        SetFilePointer( hFile, 0, NULL, FILE_END );
#ifdef UNICODE
        WideCharToMultiByte( CP_ACP, 0, szIndentBuffer, -1, szAnsi, ARRAYSIZE(szAnsi), NULL, NULL );
        WriteFile( hFile, szAnsi, lstrlenA(szAnsi), &dw, NULL );
#else
        WriteFile( hFile, szIndentBuffer, lstrlen(szIndentBuffer), &dw , NULL );
#endif
        FlushFileBuffers( hFile );
        CloseHandle( hFile );
    }
    LeaveCriticalSection( &cs );

}

/*-----------------------------------------------------------------------------
/ _output_proc_name_logfile
/ -------------------------
/   Handle the output of a procedure name, including indenting and handling
/   the opening braces.
/
/ In:
/   iCallDepth = callstack depth, defines the indent and the name index
/                to be extracted.
/   fOpenBrace = suffix with opening brace.
/ Out:
/   -
/----------------------------------------------------------------------------*/
void _output_proc_name_logfile(LONG iCallDepth)
{
    _indent_logfile(iCallDepth, g_CallStackLF[iCallDepth].m_pFunctionName);
}

/*-----------------------------------------------------------------------------
/ _trace_prolog_logfile
/ ---------------------
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
BOOL _trace_prolog_logfile(LONG iDepth, BOOL fForce)
{
    if  ( (LOG_TRACE_MASK & g_CallStackLF[iDepth].m_dwMask) || fForce )
    {
        if ( iDepth > 0 )
        {
            if ( !g_CallStackLF[iDepth-1].m_fTracedYet )
                _trace_prolog_logfile(iDepth-1, TRUE);
        }

        if ( !g_CallStackLF[iDepth].m_fTracedYet )
        {
            _output_proc_name_logfile(iDepth);
            g_CallStackLF[iDepth].m_fTracedYet = TRUE;
        }

        return TRUE;
    }

    return FALSE;
}

#endif


/*-----------------------------------------------------------------------------
/ _indent
/ -------
/   Output to the debug stream indented by n columns.
/
/ In:
/   i = column to indent to.
/   pString -> string to be indented
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

void _indent(LONG i, LPCTSTR pString)
{
    szIndentBuffer[0] = TEXT('\0');

    wsprintf(szIndentBuffer, TEXT("%08x "), GetCurrentThreadId());

    for ( ; i > 0 ; i-- )
        lstrcat(szIndentBuffer, TEXT("  "));

    lstrcat(szIndentBuffer, pString);
    lstrcat(szIndentBuffer, TEXT("\n"));

    OutputDebugString(szIndentBuffer);
}



/*-----------------------------------------------------------------------------
/ _output_proc_name
/ -----------------
/   Handle the output of a procedure name, including indenting and handling
/   the opening braces.
/
/ In:
/   iCallDepth = callstack depth, defines the indent and the name index
/                to be extracted.
/   fOpenBrace = suffix with opening brace.
/ Out:
/   -
/----------------------------------------------------------------------------*/
void _output_proc_name(LONG iCallDepth)
{
    _indent(iCallDepth, g_CallStack[iCallDepth].m_pFunctionName);
}

/*-----------------------------------------------------------------------------
/ _trace_prolog
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
BOOL _trace_prolog(LONG iDepth, BOOL fForce)
{
    if  ( (g_dwTraceMask & g_CallStack[iDepth].m_dwMask) || fForce )
    {
        if ( iDepth > 0 )
        {
            if ( !g_CallStack[iDepth-1].m_fTracedYet )
                _trace_prolog(iDepth-1, TRUE);
        }

        if ( !g_CallStack[iDepth].m_fTracedYet )
        {
            _output_proc_name(iDepth);
            g_CallStack[iDepth].m_fTracedYet = TRUE;
        }

        return TRUE;
    }

    return FALSE;
}


/*-----------------------------------------------------------------------------
/ MDDoTraceSetMask
/ ----------------
/   Adjust the trace mask to reflect the state given.
/
/ In:
/   dwMask = mask for enabling / disable trace output
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void MDDoTraceSetMask(DWORD dwMask)
{
    g_dwTraceMask = dwMask;
}


/*-----------------------------------------------------------------------------
/ MDDoTraceEnter
/ ------------
/   Set the debugging call stack up to indicate which function we are in.
/
/ In:
/   pName -> function name to be displayed in subsequent trace output.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void MDDoTraceEnter(DWORD dwMask, LPCTSTR pName)
{
    g_cDepth++;

    if ( g_cDepth < MAX_CALL_DEPTH )
    {
        if ( !pName )
            pName = TEXT("<no name>");         // no function name given

        g_CallStack[g_cDepth].m_fTracedYet = FALSE;
        g_CallStack[g_cDepth].m_pFunctionName = pName;
        g_CallStack[g_cDepth].m_dwMask = dwMask;

        if ( (g_cDepth > 0) && ( g_cDepth < MAX_CALL_DEPTH ) )
            _trace_prolog(g_cDepth-1, FALSE);
    }

#ifdef MYDOCS_LOGFILE
    g_cDepthLF++;

    if ( g_cDepthLF < MAX_CALL_DEPTH )
    {
        if ( !pName )
            pName = TEXT("<no name>");         // no function name given

        g_CallStackLF[g_cDepth].m_fTracedYet = FALSE;
        g_CallStackLF[g_cDepth].m_pFunctionName = pName;
        g_CallStackLF[g_cDepth].m_dwMask = dwMask;

        if ( (g_cDepthLF > 0) && ( g_cDepthLF < MAX_CALL_DEPTH ) )
            _trace_prolog_logfile(g_cDepth-1, FALSE);
    }
#endif
}


/*-----------------------------------------------------------------------------
/ MDDoTraceLeave
/ --------------
/   On exit from a function this will adjust the function stack pointer to
/   point to our previous function.  If no trace output has been made then
/   we will output the function name on a single line (to indicate we went somewhere).
/
/ In:
/    -
/ Out:
/   -
/----------------------------------------------------------------------------*/
void MDDoTraceLeave(void)
{
    if ( ( g_cDepth >= 0 ) && ( g_cDepth <= MAX_CALL_DEPTH ) )
        _trace_prolog(g_cDepth, FALSE);

    if ( !g_cDepth && g_CallStack[0].m_fTracedYet )
        OutputDebugString(TEXT("\n"));

    g_cDepth = max(g_cDepth-1, -1);         // account for underflow

#ifdef MYDOCS_LOGFILE
    if ( ( g_cDepthLF >= 0 ) && ( g_cDepthLF <= MAX_CALL_DEPTH ) )
        _trace_prolog_logfile(g_cDepthLF, FALSE);

    if ( !g_cDepthLF && g_CallStackLF[0].m_fTracedYet )
        _indent_logfile( g_cDepthLF, (TEXT("\r\n")) );

    g_cDepthLF = max(g_cDepthLF-1, -1);         // account for underflow
#endif
}


/*-----------------------------------------------------------------------------
/ MDDoTrace
/ ---------
/   Perform printf formatting to the debugging stream.  We indent the output
/   and stream the function name as required to give some indication of
/   call stack depth.
/
/ In:
/   pFormat -> printf style formatting string
/   ... = arguments as required for the formatting
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void MDDoTrace(LPCTSTR pFormat, ...)
{
    va_list va;

    if ( ( g_cDepth >= 0 ) && ( g_cDepth < MAX_CALL_DEPTH ) )
    {
        if ( _trace_prolog(g_cDepth, FALSE) )
        {
            va_start(va, pFormat);
            wvsprintf(szTraceBuffer, pFormat, va);
            va_end(va);

            _indent(g_cDepth+1, szTraceBuffer);
        }
    }
#ifdef MYDOCS_LOGFILE
    if ( ( g_cDepthLF >= 0 ) && ( g_cDepthLF < MAX_CALL_DEPTH ) )
    {
        if ( _trace_prolog_logfile(g_cDepthLF, FALSE) )
        {
            va_start(va, pFormat);
            wvsprintf(szTraceBuffer, pFormat, va);
            va_end(va);

            _indent_logfile(g_cDepthLF+1, szTraceBuffer);
        }
    }
#endif
}


/*-----------------------------------------------------------------------------
/ MDDoTraceGuid
/ -------------
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
    MAP_GUID(IID_IPersistFolder),
    MAP_GUID(IID_IPersistFile),
    MAP_GUID(IID_IOleWindow),

    MAP_GUID2(IID_INewShortcutHookA, IID_INewShortcutHookW),
    MAP_GUID(IID_IShellBrowser),
    MAP_GUID(IID_IShellView),
    MAP_GUID(IID_IContextMenu),
    MAP_GUID(IID_IShellIcon),
    MAP_GUID(IID_IShellFolder),
    MAP_GUID(IID_IShellExtInit),
    MAP_GUID(IID_IShellPropSheetExt),
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
    MAP_GUID(CLSID_MyDocumentsExt),
    MAP_GUID(IID_IShellDetails),
    MAP_GUID(IID_IShellExtInit),
    MAP_GUID(IID_IShellPropSheetExt),
    MAP_GUID(IID_IShellIconOverlay),
};

void MDDoTraceGUID(LPCTSTR pPrefix, REFGUID rGUID)
{
    TCHAR szGUID[GUIDSTR_MAX];
    TCHAR szBuffer[1024];
    LPCTSTR pName = NULL;
    int i;

    if ( ( g_cDepth >= 0 ) && ( g_cDepth < MAX_CALL_DEPTH ) )
    {
        if ( _trace_prolog(g_cDepth, FALSE) )
        {
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

            wsprintf(szBuffer, TEXT("%s %s"), pPrefix, pName);
            _indent(g_cDepth+1, szBuffer);
        }
    }
#ifdef MYDOCS_LOGFILE
    if ( ( g_cDepthLF >= 0 ) && ( g_cDepthLF < MAX_CALL_DEPTH ) )
    {
        if ( _trace_prolog_logfile(g_cDepthLF, FALSE) )
        {
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

            wsprintf(szBuffer, TEXT("%s %s"), pPrefix, pName);
            _indent_logfile(g_cDepthLF+1, szBuffer);
        }
    }
#endif
}

/*-----------------------------------------------------------------------------
/ MDDoTraceViewMsg
/ ----------------
/   Given a view msg (SFVM_ && DVM_), print out the corresponding text...
/
/ In:
/   uMsg -> msg to be streamed
/   wParam -> wParam value for message
/   lParam -> lParam value for message
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
#ifdef UNICODE
#define MAP_MSG(x)     x, TEXT(""L#x)
#else
#define MAP_MSG(x)     x, TEXT(""#x)
#endif

const struct
{
    UINT       m_uMsg;
    LPCTSTR    m_pName;
}
_view_msg_map[] =
{
    MAP_MSG(SFVM_MERGEMENU),
    MAP_MSG(SFVM_INVOKECOMMAND),
    MAP_MSG(SFVM_GETHELPTEXT),
    MAP_MSG(SFVM_GETTOOLTIPTEXT),
    MAP_MSG(SFVM_GETBUTTONINFO),
    MAP_MSG(SFVM_GETBUTTONS),
    MAP_MSG(SFVM_INITMENUPOPUP),
    MAP_MSG(SFVM_SELCHANGE),
    MAP_MSG(SFVM_DRAWITEM),
    MAP_MSG(SFVM_MEASUREITEM),
    MAP_MSG(SFVM_EXITMENULOOP),
    MAP_MSG(SFVM_PRERELEASE),
    MAP_MSG(SFVM_GETCCHMAX),
    MAP_MSG(SFVM_FSNOTIFY),
    MAP_MSG(SFVM_WINDOWCREATED),
    MAP_MSG(SFVM_WINDOWDESTROY),
    MAP_MSG(SFVM_REFRESH),
    MAP_MSG(SFVM_SETFOCUS),
    MAP_MSG(SFVM_QUERYCOPYHOOK),
    MAP_MSG(SFVM_NOTIFYCOPYHOOK),
    MAP_MSG(SFVM_GETDETAILSOF),
    MAP_MSG(SFVM_COLUMNCLICK),
    MAP_MSG(SFVM_QUERYFSNOTIFY),
    MAP_MSG(SFVM_DEFITEMCOUNT),
    MAP_MSG(SFVM_DEFVIEWMODE),
    MAP_MSG(SFVM_UNMERGEMENU),
    MAP_MSG(SFVM_INSERTITEM),
    MAP_MSG(SFVM_DELETEITEM),
    MAP_MSG(SFVM_UPDATESTATUSBAR),
    MAP_MSG(SFVM_BACKGROUNDENUM),
    MAP_MSG(SFVM_GETWORKINGDIR),
    MAP_MSG(SFVM_GETCOLSAVESTREAM),
    MAP_MSG(SFVM_SELECTALL),
    MAP_MSG(SFVM_DIDDRAGDROP),
    MAP_MSG(SFVM_SUPPORTSIDENTITY),
    MAP_MSG(SFVM_FOLDERISPARENT),
    MAP_MSG(SFVM_GETVIEWS),
    MAP_MSG(SFVM_THISIDLIST),
    MAP_MSG(SFVM_GETITEMIDLIST),
    MAP_MSG(SFVM_SETITEMIDLIST),
    MAP_MSG(SFVM_INDEXOFITEMIDLIST),
    MAP_MSG(SFVM_ODFINDITEM),
    MAP_MSG(SFVM_HWNDMAIN),
    MAP_MSG(SFVM_ADDPROPERTYPAGES),
    MAP_MSG(SFVM_BACKGROUNDENUMDONE),
    MAP_MSG(SFVM_GETNOTIFY),
    MAP_MSG(SFVM_ARRANGE),
    MAP_MSG(SFVM_QUERYSTANDARDVIEWS),
    MAP_MSG(SFVM_QUERYREUSEEXTVIEW),
    MAP_MSG(SFVM_GETSORTDEFAULTS),
    MAP_MSG(SFVM_GETEMPTYTEXT),
    MAP_MSG(SFVM_GETITEMICONINDEX),
};

void MDDoTraceViewMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPCTSTR pName = NULL;
    TCHAR szBuffer[1024];
    TCHAR szTmp[25];
    int i;

    if ( ( g_cDepth >= 0 ) && ( g_cDepth < MAX_CALL_DEPTH ) )
    {
        if ( _trace_prolog(g_cDepth, FALSE) )
        {
            for ( i = 0 ; i < ARRAYSIZE(_view_msg_map); i++ )
            {
                if ( _view_msg_map[i].m_uMsg == uMsg )
                {
                    pName = _view_msg_map[i].m_pName;
                    break;
                }
            }

            if (!pName)
            {
                wsprintf( szTmp, TEXT("SFVM_(%d)"), uMsg );
                pName = szTmp;
            }

            wsprintf(szBuffer, TEXT("%s w(%08X) l(%08X)"), pName, wParam, lParam);
            _indent(g_cDepth+1, szBuffer);
        }
    }

#ifdef MYDOCS_LOGFILE
    if ( ( g_cDepthLF >= 0 ) && ( g_cDepthLF < MAX_CALL_DEPTH ) )
    {
        if ( _trace_prolog_logfile(g_cDepth, FALSE) )
        {
            for ( i = 0 ; i < ARRAYSIZE(_view_msg_map); i++ )
            {
                if ( _view_msg_map[i].m_uMsg == uMsg )
                {
                    pName = _view_msg_map[i].m_pName;
                    break;
                }
            }

            if (!pName)
            {
                wsprintf( szTmp, TEXT("SFVM_(%d)"), uMsg );
                pName = szTmp;
            }

            wsprintf(szBuffer, TEXT("%s w(%08X) l(%08X)"), pName, wParam, lParam);
            _indent_logfile(g_cDepthLF+1, szBuffer);
        }
    }
#endif
}

const struct
{
    UINT       m_uMsg;
    LPCTSTR    m_pName;
}
_menu_msg_map[] =
{
    MAP_MSG(DFM_MERGECONTEXTMENU),
    MAP_MSG(DFM_INVOKECOMMAND),
    MAP_MSG(DFM_ADDREF),
    MAP_MSG(DFM_RELEASE),
    MAP_MSG(DFM_GETHELPTEXT),
    MAP_MSG(DFM_WM_MEASUREITEM),
    MAP_MSG(DFM_WM_DRAWITEM),
    MAP_MSG(DFM_WM_INITMENUPOPUP),
    MAP_MSG(DFM_VALIDATECMD),
    MAP_MSG(DFM_MERGECONTEXTMENU_TOP),
    MAP_MSG(DFM_GETHELPTEXTW),
    MAP_MSG(DFM_INVOKECOMMANDEX),
    MAP_MSG(DFM_MAPCOMMANDNAME),
};

const struct
{
    UINT       m_uMsg;
    LPCTSTR    m_pName;
}
_menu_invk_cmd_msg_map[] =
{
    MAP_MSG(DFM_CMD_RENAME),
    MAP_MSG(DFM_CMD_MODALPROP),
    MAP_MSG(DFM_CMD_PASTESPECIAL),
    MAP_MSG(DFM_CMD_PASTELINK),
    MAP_MSG(DFM_CMD_VIEWDETAILS),
    MAP_MSG(DFM_CMD_VIEWLIST),
    MAP_MSG(DFM_CMD_PASTE),
    MAP_MSG(DFM_CMD_NEWFOLDER),
    MAP_MSG(DFM_CMD_PROPERTIES),
    MAP_MSG(DFM_CMD_LINK),
    MAP_MSG(DFM_CMD_COPY),
    MAP_MSG(DFM_CMD_MOVE),
    MAP_MSG(DFM_CMD_DELETE),
};



void MDDoTraceMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPCTSTR pName = NULL;
    TCHAR szBuffer[1024];
    TCHAR szTmp[25];
    int i;

    if ( ( g_cDepth >= 0 ) && ( g_cDepth < MAX_CALL_DEPTH ) )
    {
        if ( _trace_prolog(g_cDepth, FALSE) )
        {
            for ( i = 0 ; i < ARRAYSIZE(_menu_msg_map); i++ )
            {
                if ( _menu_msg_map[i].m_uMsg == uMsg )
                {
                    pName = _menu_msg_map[i].m_pName;
                    break;
                }
            }

            if (!pName)
            {
                wsprintf( szTmp, TEXT("SFVM_(%d)"), uMsg );
                pName = szTmp;
            }

            if ((uMsg == DFM_INVOKECOMMAND) && (wParam >= DFM_CMD_RENAME))
            {
                wsprintf(szBuffer, TEXT("%s w(%s) l(%08X)"), pName, _menu_invk_cmd_msg_map[wParam-DFM_CMD_RENAME], lParam);
            }
            else
            {
                wsprintf(szBuffer, TEXT("%s w(%08X) l(%08X)"), pName, wParam, lParam);
            }
            _indent(g_cDepth+1, szBuffer);
        }
    }

#ifdef MYDOCS_LOGFILE
    if ( ( g_cDepthLF >= 0 ) && ( g_cDepthLF < MAX_CALL_DEPTH ) )
    {
        if ( _trace_prolog_logfile(g_cDepth, FALSE) )
        {
            for ( i = 0 ; i < ARRAYSIZE(_menu_msg_map); i++ )
            {
                if ( _menu_msg_map[i].m_uMsg == uMsg )
                {
                    pName = _menu_msg_map[i].m_pName;
                    break;
                }
            }

            if (!pName)
            {
                wsprintf( szTmp, TEXT("SFVM_(%d)"), uMsg );
                pName = szTmp;
            }

            if ((uMsg == DFM_INVOKECOMMAND) && (wParam >= DFM_CMD_RENAME))
            {
                wsprintf(szBuffer, TEXT("%s w(%s) l(%08X)"), pName, _menu_invk_cmd_msg_map[wParam-DFM_CMD_RENAME], lParam);
            }
            else
            {
                wsprintf(szBuffer, TEXT("%s w(%08X) l(%08X)"), pName, wParam, lParam);
            }
            _indent_logfile(g_cDepthLF+1, szBuffer);
        }
    }
#endif
}


/*-----------------------------------------------------------------------------
/ MDDoTraceAssert
/ ---------------
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
void MDDoTraceAssert(int iLine, LPTSTR pFilename)
{
    TCHAR szBuffer[1024];

    wsprintf(szBuffer, TEXT("Assert failed in %s, line %d"), pFilename, iLine);

    _trace_prolog(g_cDepth, TRUE);          // nb: TRUE asserts always displabed
    _indent(g_cDepth+1, szBuffer);

    if ( g_dwTraceMask & TRACE_COMMON_ASSERT )
        DebugBreak();
}


#endif
