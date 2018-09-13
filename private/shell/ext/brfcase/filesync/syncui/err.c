//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: err.c
//
//  This files contains all error handling routines.
//
// History:
//  08-06-93 ScottH     Transferred from twin code
//
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"     // common headers

/////////////////////////////////////////////////////  TYPEDEFS

/////////////////////////////////////////////////////  CONTROLLING DEFINES

/////////////////////////////////////////////////////  DEFINES

/////////////////////////////////////////////////////  MODULE DATA

#ifdef DEBUG

#pragma data_seg(DATASEG_READONLY)

TCHAR const  c_szNewline[] = TEXT("\r\n");
TCHAR const  c_szTrace[] = TEXT("t BRIEFCASE  ");
TCHAR const  c_szDbg[] = TEXT("BRIEFCASE  ");
TCHAR const  c_szAssertFailed[] = TEXT("BRIEFCASE  Assertion failed in %s on line %d\r\n");

struct _RIIDMAP
    {
    REFIID  riid;
    LPCTSTR  psz;
    } const c_rgriidmap[] = {
        { &IID_IUnknown,        TEXT("IID_IUnknown") },
        { &IID_IBriefcaseStg,   TEXT("IID_IBriefcaseStg") },
        { &IID_IEnumUnknown,    TEXT("IID_IEnumUnknown") },
        { &IID_IShellBrowser,   TEXT("IID_IShellBrowser") },
        { &IID_IShellView,      TEXT("IID_IShellView") },
        { &IID_IContextMenu,    TEXT("IID_IContextMenu") },
        { &IID_IShellFolder,    TEXT("IID_IShellFolder") },
        { &IID_IShellExtInit,   TEXT("IID_IShellExtInit") },
        { &IID_IShellPropSheetExt, TEXT("IID_IShellPropSheetExt") },
        { &IID_IPersistFolder,  TEXT("IID_IPersistFolder") },
        { &IID_IExtractIcon,    TEXT("IID_IExtractIcon") },
        { &IID_IShellDetails,   TEXT("IID_IShellDetails") },
        { &IID_IDelayedRelease, TEXT("IID_IDelayedRelease") },
        { &IID_IShellLink,      TEXT("IID_IShellLink") },
        };

struct _SCODEMAP
    {
    SCODE  sc;
    LPCTSTR psz;
    } const c_rgscodemap[] = {
        { S_OK,             TEXT("S_OK") },
        { S_FALSE,          TEXT("S_FALSE") },
        { E_UNEXPECTED,     TEXT("E_UNEXPECTED") },
        { E_NOTIMPL,        TEXT("E_NOTIMPL") },
        { E_OUTOFMEMORY,    TEXT("E_OUTOFMEMORY") },
        { E_INVALIDARG,     TEXT("E_INVALIDARG") },
        { E_NOINTERFACE,    TEXT("E_NOINTERFACE") },
        { E_POINTER,        TEXT("E_POINTER") },
        { E_HANDLE,         TEXT("E_HANDLE") },
        { E_ABORT,          TEXT("E_ABORT") },
        { E_FAIL,           TEXT("E_FAIL") },
        { E_ACCESSDENIED,   TEXT("E_ACCESSDENIED") },
        };


#pragma data_seg()

#endif

/////////////////////////////////////////////////////  PUBLIC FUNCTIONS


#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Return English reason for the debug break
Returns: String
Cond:    --
*/
LPCTSTR PRIVATE GetReasonString(
    UINT flag)      // One of BF_ flags
    {
    LPCTSTR psz;

    if (IsFlagSet(flag, BF_ONOPEN))
        psz = TEXT("BREAK ON OPEN BRIEFCASE\r\n");

    else if (IsFlagSet(flag, BF_ONCLOSE))
        psz = TEXT("BREAK ON CLOSE BRIEFCASE\r\n");

    else if (IsFlagSet(flag, BF_ONRUNONCE))
        psz = TEXT("BREAK ON RunDLL_RunOnlyOnce\r\n");

    else if (IsFlagSet(flag, BF_ONVALIDATE))
        psz = TEXT("BREAK ON VALIDATION FAILURE\r\n");

    else if (IsFlagSet(flag, BF_ONTHREADATT))
        psz = TEXT("BREAK ON THREAD ATTACH\r\n");

    else if (IsFlagSet(flag, BF_ONTHREADDET))
        psz = TEXT("BREAK ON THREAD DETACH\r\n");

    else if (IsFlagSet(flag, BF_ONPROCESSATT))
        psz = TEXT("BREAK ON PROCESS ATTACH\r\n");

    else if (IsFlagSet(flag, BF_ONPROCESSDET))
        psz = TEXT("BREAK ON PROCESS DETACH\r\n");

    else
        psz = c_szNewline;

    return psz;
    }


/*----------------------------------------------------------
Purpose: Perform a debug break based on the flag
Returns: --
Cond:    --
*/
void PUBLIC DEBUG_BREAK(
    UINT flag)      // One of BF_ flags
    {
    BOOL bBreak;
    LPCTSTR psz;

    ENTEREXCLUSIVE()
        {
        bBreak = IsFlagSet(g_uBreakFlags, flag);
        psz = GetReasonString(flag);
        }
    LEAVEEXCLUSIVE()

    if (bBreak)
        {
        TRACE_MSG(TF_ALWAYS, psz);
        DebugBreak();
        }
    }


void PUBLIC BrfAssertFailed(
    LPCTSTR pszFile, 
    int line)
    {
    LPCTSTR psz;
    TCHAR ach[256];
    UINT uBreakFlags;

    ENTEREXCLUSIVE()
        {
        uBreakFlags = g_uBreakFlags;
        }
    LEAVEEXCLUSIVE()

    // Strip off path info from filename string, if present.
    //
    for (psz = pszFile + lstrlen(pszFile); psz != pszFile; psz=CharPrev(pszFile, psz))
        {
        if ((CharPrev(pszFile, psz) != (psz-2)) && *(psz - 1) == TEXT('\\'))
            break;
        }
    wsprintf(ach, c_szAssertFailed, psz, line);
    OutputDebugString(ach);
    
    if (IsFlagSet(uBreakFlags, BF_ONVALIDATE))
        DebugBreak();
    }


void CPUBLIC BrfAssertMsg(
    BOOL f, 
    LPCTSTR pszMsg, ...)
    {
    TCHAR ach[MAXPATHLEN+40];    // Largest path plus extra

    if (!f)
        {
        lstrcpy(ach, c_szTrace);
        wvsprintf(&ach[ARRAYSIZE(c_szTrace)-1], pszMsg, (va_list)(&pszMsg + 1));
        OutputDebugString(ach);
        OutputDebugString(c_szNewline);
        }
    }


void CPUBLIC BrfDebugMsg(
    UINT uFlag, 
    LPCTSTR pszMsg, ...)
    {
    TCHAR ach[MAXPATHLEN+40];    // Largest path plus extra
    UINT uTraceFlags;

    ENTEREXCLUSIVE()
        {
        uTraceFlags = g_uTraceFlags;
        }
    LEAVEEXCLUSIVE()

    if (uFlag == TF_ALWAYS || IsFlagSet(uTraceFlags, uFlag))
        {
        lstrcpy(ach, c_szTrace);
        wvsprintf(&ach[ARRAYSIZE(c_szTrace)-1], pszMsg, (va_list)(&pszMsg + 1));
        OutputDebugString(ach);
        OutputDebugString(c_szNewline);
        }
    }


/*----------------------------------------------------------
Purpose: Returns the string form of an known interface ID.

Returns: String ptr
Cond:    --
*/
LPCTSTR PUBLIC Dbg_GetRiidName(
    REFIID riid)
    {
    int i;

    for (i = 0; i < ARRAYSIZE(c_rgriidmap); i++)
        {
        if (IsEqualIID(riid, c_rgriidmap[i].riid))
            return c_rgriidmap[i].psz;
        }
    return TEXT("Unknown riid");
    }


/*----------------------------------------------------------
Purpose: Returns the string form of an scode given an hresult.

Returns: String ptr
Cond:    --
*/
LPCTSTR PUBLIC Dbg_GetScode(
    HRESULT hres)
    {
    int i;
    SCODE sc;

    sc = GetScode(hres);
    for (i = 0; i < ARRAYSIZE(c_rgscodemap); i++)
        {
        if (sc == c_rgscodemap[i].sc)
            return c_rgscodemap[i].psz;
        }
    return TEXT("Unknown scode");
    }


/*----------------------------------------------------------
Purpose: Returns a string safe enough to print...and I don't
         mean swear words.

Returns: String ptr
Cond:    --
*/
LPCTSTR PUBLIC Dbg_SafeStr(
    LPCTSTR psz)
    {
    if (psz)
        return psz;
    else
        return TEXT("NULL");
    }


/*----------------------------------------------------------
Purpose: Returns a string safe enough to print given an IDataObject.

Returns: String ptr
Cond:    --
*/
LPCTSTR PUBLIC Dbg_DataObjStr(
    LPDATAOBJECT pdtobj,
    LPTSTR pszBuf)
    {
    if (pdtobj)
        {
        DataObj_QueryPath(pdtobj, pszBuf);
        }
    else
        {
        lstrcpy(pszBuf, TEXT("NULL"));
        }
    return pszBuf;
    }


#endif  // DEBUG


/*----------------------------------------------------------
Purpose: This function maps the hresult to an hresult in the 
         error table, and displays the corresponding string
         in a messagebox.

Returns: return value of MessageBox
Cond:    --
*/
int PUBLIC SEMsgBox(
    HWND hwnd,
    UINT idsCaption,
    HRESULT hres,
    PCSETBL pTable,
    UINT cArraySize)        // Number of elements in table
    {
    PCSETBL p;
    PCSETBL pEnd;

    p = pTable;
    pEnd = &pTable[cArraySize-1];
    while (p != pEnd)
        {
        if (p->hres == hres)
            {
            return MsgBox(hwnd, MAKEINTRESOURCE(p->ids), MAKEINTRESOURCE(idsCaption), 
                NULL, p->uStyle);
            }
        p++;
        }

    // Cover last entry
    if (p->hres == hres)
        {
        return MsgBox(hwnd, MAKEINTRESOURCE(p->ids), MAKEINTRESOURCE(idsCaption), 
            NULL, p->uStyle);
        }

    return -1;
    }


/*----------------------------------------------------------
Purpose: Maps an hresult to a valid "official" hresult.  This
         is necessary because the SYNCUI uses a FACILITY_TR
         which is only good for us, but unknown to the outside
         world.

Returns: hresult
Cond:    --
*/
HRESULT PUBLIC MapToOfficialHresult(
    HRESULT hres)
    {
    if (IS_ENGINE_ERROR(hres))
        {
        SCODE sc = GetScode(hres);

        if (E_TR_OUT_OF_MEMORY == sc)
            hres = ResultFromScode(E_OUTOFMEMORY);
        else
            hres = ResultFromScode(E_FAIL);
        }

    return hres;
    }
