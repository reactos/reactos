//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: err.c
//
//  This files contains all error handling routines.
//
// History:
//  01-17-93 ScottH     Copied from mdmui
//
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////  INCLUDES

#include "rnaui.h"     // common headers

/////////////////////////////////////////////////////  TYPEDEFS

/////////////////////////////////////////////////////  CONTROLLING DEFINES

/////////////////////////////////////////////////////  DEFINES

/////////////////////////////////////////////////////  MODULE DATA

#ifdef DEBUG

#pragma data_seg(DATASEG_READONLY)

char const FAR c_szNewline[] = "\r\n";
char const FAR c_szTrace[] = "t RNAUI  ";
char const FAR c_szDbg[] = "RNAUI  ";
char const FAR c_szAssertFailed[] = "RNAUI  Assertion failed in %s on line %d\r\n";

struct _RIIDMAP
    {
    REFIID  riid;
    LPCSTR  psz;
    } const c_rgriidmap[] = {
        { &IID_IUnknown,        "IID_IUnknown" },
        { &IID_IEnumUnknown,    "IID_IEnumUnknown" },
        { &IID_IShellBrowser,   "IID_IShellBrowser" },
        { &IID_IShellView,      "IID_IShellView" },
        { &IID_IContextMenu,    "IID_IContextMenu" },
        { &IID_IShellFolder,    "IID_IShellFolder" },
        { &IID_IShellExtInit,   "IID_IShellExtInit" },
        { &IID_IShellPropSheetExt, "IID_IShellPropSheetExt" },
        { &IID_IPersistFolder,  "IID_IPersistFolder" },
        { &IID_IExtractIcon,    "IID_IExtractIcon" },
        { &IID_IShellDetails,   "IID_IShellDetails" },
        { &IID_IDelayedRelease, "IID_IDelayedRelease" },
        { &IID_IShellLink,      "IID_IShellLink" },
        { &IID_IShellCopyHook,  "IID_IShellCopyHook" },
        { &IID_IFileViewer,     "IID_IFileViewer" },
        { &IID_ICommDlgBrowser, "IID_ICommDlgBrowser" },
        { &IID_IDropSource,     "IID_IDropSource" },
        { &IID_IDropTarget,     "IID_IDropTarget" },
        { &IID_IDataObject,     "IID_IDataObject" },
        { &IID_IShellFolderViewCB, "IID_IShellFolderViewCB" },
        };

struct _SCODEMAP
    {
    SCODE  sc;
    LPCSTR psz;
    } const c_rgscodemap[] = {
        { S_OK,             "S_OK" },
        { S_FALSE,          "S_FALSE" },
        { E_UNEXPECTED,     "E_UNEXPECTED" },
        { E_NOTIMPL,        "E_NOTIMPL" },
        { E_OUTOFMEMORY,    "E_OUTOFMEMORY" },
        { E_INVALIDARG,     "E_INVALIDARG" },
        { E_NOINTERFACE,    "E_NOINTERFACE" },
        { E_POINTER,        "E_POINTER" },
        { E_HANDLE,         "E_HANDLE" },
        { E_ABORT,          "E_ABORT" },
        { E_FAIL,           "E_FAIL" },
        { E_ACCESSDENIED,   "E_ACCESSDENIED" },
        };

#pragma data_seg()

#endif

/////////////////////////////////////////////////////  PUBLIC FUNCTIONS


#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Return English reason for the int 3
Returns: String
Cond:    --
*/
LPCSTR PRIVATE GetReasonString(
    UINT flag)      // One of BF_ flags
    {
    LPCSTR psz;

    if (IsFlagSet(flag, BF_ONOPEN))
        psz = "BREAK ON OPEN\r\n";

    else if (IsFlagSet(flag, BF_ONCLOSE))
        psz = "BREAK ON CLOSE\r\n";

    else if (IsFlagSet(flag, BF_ONRUNONCE))
        psz = "BREAK ON RunDLL_RunOnlyOnce\r\n";

    else if (IsFlagSet(flag, BF_ONVALIDATE))
        psz = "BREAK ON VALIDATION FAILURE\r\n";

    else if (IsFlagSet(flag, BF_ONTHREADATT))
        psz = "BREAK ON THREAD ATTACH\r\n";

    else if (IsFlagSet(flag, BF_ONTHREADDET))
        psz = "BREAK ON THREAD DETACH\r\n";

    else if (IsFlagSet(flag, BF_ONPROCESSATT))
        psz = "BREAK ON PROCESS ATTACH\r\n";

    else if (IsFlagSet(flag, BF_ONPROCESSDET))
        psz = "BREAK ON PROCESS DETACH\r\n";

    else
        psz = c_szNewline;

    return psz;
    }


/*----------------------------------------------------------
Purpose: Perform an int 3 based on the flag
Returns: --
Cond:    --
*/
void PUBLIC DEBUG_BREAK(
    UINT flag)      // One of BF_ flags
    {
    BOOL bBreak;
    LPCSTR psz;

    ENTEREXCLUSIVE()
        {
        bBreak = IsFlagSet(g_uBreakFlags, flag);
        psz = GetReasonString(flag);
        }
    LEAVEEXCLUSIVE()

    if (bBreak)
        {
        TRACE_MSG(TF_ALWAYS, psz);
        _asm  int 3;
        }
    }


void PUBLIC RnaAssertFailed(
    LPCSTR pszFile, 
    int line)
    {
    LPCSTR psz;
    char ach[256];
    UINT uBreakFlags;

    ENTEREXCLUSIVE()
        {
        uBreakFlags = g_uBreakFlags;
        }
    LEAVEEXCLUSIVE()

    // Strip off path info from filename string, if present.
    //
    for (psz = pszFile + lstrlen(pszFile); psz != pszFile; psz=AnsiPrev(pszFile, psz))
        {
#ifdef  DBCS
        if ((AnsiPrev(pszFile, psz) != (psz-2)) && *(psz - 1) == '\\')
#else
        if (*(psz - 1) == '\\')
#endif
            break;
        }
    wsprintf(ach, c_szAssertFailed, psz, line);
    OutputDebugString(ach);
    
    if (IsFlagSet(uBreakFlags, BF_ONVALIDATE))
        _asm  int 3;
    }


void CPUBLIC RnaAssertMsg(
    BOOL f, 
    LPCSTR pszMsg, ...)
    {
    char ach[MAXPATHLEN+40];    // Largest path plus extra

    if (!f)
        {
        lstrcpy(ach, c_szTrace);
        wvsprintf(&ach[sizeof(c_szTrace)-1], pszMsg, (va_list)(&pszMsg + 1));
        OutputDebugString(ach);
        OutputDebugString(c_szNewline);
        }
    }


void CPUBLIC RnaDebugMsg(
    UINT uFlag, 
    LPCSTR pszMsg, ...)
    {
    char ach[MAXPATHLEN+40];    // Largest path plus extra
    UINT uTraceFlags;

    ENTEREXCLUSIVE()
        {
        uTraceFlags = g_uTraceFlags;
        }
    LEAVEEXCLUSIVE()

    if (uFlag == TF_ALWAYS || IsFlagSet(uTraceFlags, uFlag))
        {
        lstrcpy(ach, c_szTrace);
        wvsprintf(&ach[sizeof(c_szTrace)-1], pszMsg, (va_list)(&pszMsg + 1));
        OutputDebugString(ach);
        OutputDebugString(c_szNewline);
        }
    }


/*----------------------------------------------------------
Purpose: Returns the string form of an known interface ID.

Returns: String ptr
Cond:    --
*/
LPCSTR PUBLIC Dbg_GetRiidName(
    REFIID riid)
    {
    int i;

    for (i = 0; i < ARRAYSIZE(c_rgriidmap); i++)
        {
        if (IsEqualIID(riid, c_rgriidmap[i].riid))
            return c_rgriidmap[i].psz;
        }
    return "Unknown riid";
    }


/*----------------------------------------------------------
Purpose: Returns the string form of an scode given an hresult.

Returns: String ptr
Cond:    --
*/
LPCSTR PUBLIC Dbg_GetScode(
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
    return "Unknown scode";
    }


/*----------------------------------------------------------
Purpose: Returns a string safe enough to print...and I don't
         mean swear words.

Returns: String ptr
Cond:    --
*/
LPCSTR PUBLIC Dbg_SafeStr(
    LPCSTR psz)
    {
    if (psz)
        return psz;
    else
        return "NULL";
    }

#endif  // DEBUG


/*----------------------------------------------------------
Purpose: Invoke a user error message box.  Default values are
         obtained from vappinfo struct.
Returns: value of MessageBox()
Cond:    --
*/
int PUBLIC MsgBoxSz(
    HWND hwndParent,    // parent window (may be NULL)
    LPCSTR lpsz,        // message
    UINT idsCaption,    // resource ID for caption
    UINT nBoxType,      // message box type (ERR_ERROR, ERR_INFO, ERR_QUESTION)
    HANDLE hres)        // Resource instance handle (may be NULL)
    {
    int nRet = 0;
    
    if (hres == NULL)
        {
        ENTEREXCLUSIVE()
            {
            hres = ghInstance;
            }
        LEAVEEXCLUSIVE()
        }
    
    ASSERT(hres != NULL);
    
    // Load in error description string
    //
    if (lpsz)
        {
        char szCap[MAXSTRINGLEN];

        UINT nType = MB_OK;
        
        // Load the caption string
        //
        LoadString(ghInstance, idsCaption, szCap, sizeof(szCap));

        // Determine type of message box
        //
        nType |= (nBoxType == MSG_ERROR) ? MB_ICONEXCLAMATION : 0;
        nType |= (nBoxType == MSG_INFO) ? MB_ICONINFORMATION : 0;
        if (nBoxType == MSG_QUESTION)
            nType = MB_ICONQUESTION | MB_YESNO;
        
        nRet = MessageBox(hwndParent, lpsz, szCap, nType | MB_SETFOREGROUND);
        }
    
    return nRet;
    }


/*----------------------------------------------------------
Purpose: Invoke a user error message box.  
Returns: value of MessageBox()
Cond:    --
*/
int PUBLIC MsgBoxIds(
    HWND hwndParent,    // parent window (may be NULL)
    UINT ids,           // message resource ID
    UINT idsCaption,    // resource ID for caption
    UINT nBoxType)      // message box type (ERR_ERROR, ERR_INFO, ERR_QUESTION)
    {
    char sz[MAXMSGLEN];
    int nRet = 0;
    HINSTANCE hinst;
    
    ENTEREXCLUSIVE()
        {
        hinst = ghInstance;
        }
    LEAVEEXCLUSIVE()
    
    // Load in error description string
    //
    if (LoadString(hinst, ids, sz, sizeof(sz)) > 0)
        nRet = MsgBoxSz(hwndParent, sz, idsCaption, nBoxType, hinst);
    
    return nRet;
    }


/*----------------------------------------------------------
Purpose: Pops up a messagebox if the dwValue is an error
         of some sort.  This function maps the dwValue to
         a dwValue in the error table, and displays the
         corresponding string.

Returns: return value of MessageBox
Cond:    --
*/
int PUBLIC ETMsgBox(
    HWND hwnd,
    UINT idsCaption,
    DWORD dwValue,
    PCERRTBL petTable,
    UINT cArraySize)        // Number of elements in table
    {
    PCERRTBL pet;
    PCERRTBL petEnd;

    pet = petTable;
    petEnd = &petTable[cArraySize-1];
    while (pet != petEnd)
        {
        if (pet->dwValue == dwValue)
            {
            return MsgBoxIds(hwnd, pet->ids, idsCaption, MSG_ERROR);
            }
        pet++;
        }

    // Cover last entry
    if (pet->dwValue == dwValue)
        {
        return MsgBoxIds(hwnd, pet->ids, idsCaption, MSG_ERROR);
        }

    return -1;
    }




