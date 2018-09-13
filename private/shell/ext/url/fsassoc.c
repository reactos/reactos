/* Look for BUILDBUILD to find build hacks. */

/* BUILDBUILD: My hacks to get this to build. */

#pragma warning(disable:4001) /* "single line comment" warning */

#include "project.h"
#pragma hdrstop

#include "urlshell.h"
#include <windowsx.h>

#include <mluisupp.h>

#undef NO_HELP
#include <help.h>
#pragma pack(1)
//#include <newexe.h>             // newexe.h is broken with pack()!!!
#pragma pack()
#ifdef STRICT
#undef STRICT
#endif
#define NO_SHELL_HEAP_ALLOCATOR  //Kludge to get around new mem heaps in nt

#define NO_MULTIMON         // BUILDBUILD (scotth): no multimon stub functions
#define NO_SHELL_VALIDATION
#include <newexe.h>         // BUILDBUILD: added this from shellprv.h
#include "resource.h"       // BUILDBUILD: use local resource IDs

#ifdef RegQueryValue
#undef RegQueryValue
#endif

#define RegQueryValue(hkey, pszSubkey, pvValue, pcbSize)    SHGetValue(hkey, pszSubkey, NULL, NULL, pvValue, pcbSize)

// BUILDBUILD: add from shsemip.h
#define SIZEOF(a)       sizeof(a)

// BUILDBUILD: was from cstrings.h
extern CCHAR c_szNULL[];

#include "openas.h"

#undef HINST_THISDLL
#define HINST_THISDLL            (GetThisModulesHandle())

#undef ASSERTNONCRITICAL
#define ASSERTNONCRITICAL        ASSERT(! AccessIsExclusive());

#define MZMAGIC         ((WORD)'M'+((WORD)'Z'<<8))
#define LEMAGIC         ((WORD)'L'+((WORD)'E'<<8))

#define FCC(c0,c1,c2,c3) ((DWORD)(c0)|((DWORD)(c1)<<8)|((DWORD)(c2)<<16)|((DWORD)(c3)<<24))

#define COM_FILE        FCC('.', 'c', 'o', 'm')
#define BAT_FILE        FCC('.', 'b', 'a', 't')
#define EXE_FILE        FCC('.', 'e', 'x', 'e')

#define hinstCabinet       GetThisModulesHandle()
BOOL GetClassDescription(HKEY hkClasses, LPCSTR pszClass, LPSTR szDisplayName, int cbDisplayName, UINT uFlags);

/* needed because the NT shell only understands unicode for these
 * functions...
 */

/* define the local versions */
extern BOOL  lPathIsExeA(LPCSTR);
extern int   lShell_GetCachedImageIndexA(LPCSTR, int, UINT);
extern BOOL  lPathYetAnotherMakeUniqueNameA(LPSTR,LPCSTR,LPCSTR,LPCSTR);
extern LONG  lRegDeleteKeyA(HKEY, LPCSTR);

/***********************************************************
 * Needed because under NT, deleting a subkey will fail.
 *
 * Stolen from the SDK:
 *   Windows 95: The RegDeleteKey function deletes a key and 
 *               all its descendents.
 *   Windows NT: The RegDeleteKey function deletes the specified
 *               key. This function cannot delete a key that has
 *               subkeys. 
 **********************************************************/
// On Win95, RegDeleteKey deletes the key and all subkeys.  On NT, RegDeleteKey 
// fails if there are any subkeys.  On NT, we'll make shell code that assumes 
// the Win95 behavior work by mapping SHRegDeleteKey to a helper function that
// does the recursive delete.
// The reason we do it here, instead of calling the shell is so that we don't
// have any bogus dynalinks for the X86 version, which must also run on W95.

LONG ShRegDeleteKey(HKEY hKey, LPCSTR lpSubKey)
{
    LONG    lResult;
    HKEY    hkSubKey;
    DWORD   dwIndex;
    char    szSubKeyName[MAX_PATH + 1];
    DWORD   cchSubKeyName = ARRAYSIZE(szSubKeyName);
    char    szClass[MAX_PATH];
    DWORD   cbClass = ARRAYSIZE(szClass);
    DWORD   dwDummy1, dwDummy2, dwDummy3, dwDummy4, dwDummy5, dwDummy6;
    FILETIME ft;

    // Open the subkey so we can enumerate any children
    lResult = RegOpenKeyExA(hKey, lpSubKey, 0, KEY_ALL_ACCESS, &hkSubKey);
    if (ERROR_SUCCESS == lResult)
    {
    // I can't just call RegEnumKey with an ever-increasing index, because
    // I'm deleting the subkeys as I go, which alters the indices of the
    // remaining subkeys in an implementation-dependent way.  In order to
    // be safe, I have to count backwards while deleting the subkeys.

    // Find out how many subkeys there are
    lResult = RegQueryInfoKey(hkSubKey, 
                  szClass, 
                  &cbClass, 
                  NULL, 
                  &dwIndex, // The # of subkeys -- all we need
                  &dwDummy1,
                  &dwDummy2,
                  &dwDummy3,
                  &dwDummy4,
                  &dwDummy5,
                  &dwDummy6,
                  &ft);

    if (ERROR_SUCCESS == lResult)
    {
        // dwIndex is now the count of subkeys, but it needs to be 
        // zero-based for RegEnumKey, so I'll pre-decrement, rather
        // than post-decrement.
        while (ERROR_SUCCESS == RegEnumKey(hkSubKey, --dwIndex, szSubKeyName, cchSubKeyName))
        {
        ShRegDeleteKey(hkSubKey, szSubKeyName);
        }
    }

// BUGBUG
// Issue with shellprv.  For some reason someone commented out the definition of SHRegCloseKey
// SHRegCloseKey is not in Win95.  Doing an undef here puts it back to RegCloseKey
// Works now on both NT and Win95
//
#undef  RegCloseKey
    RegCloseKey(hkSubKey);

    lResult = RegDeleteKey(hKey, lpSubKey);
    }
    
    return lResult;
}


LONG lRegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey)
{
    if (RUNNING_NT)
        return(ShRegDeleteKey(hKey, lpSubKey));
    else
        return(RegDeleteKey(hKey, lpSubKey));
}

// in:
//      pszPath         directory to do this into or full dest path
//                      if pszShort is NULL
//      pszShort        file name (short version) if NULL assumes
//                      pszPath is both path and spec
//      pszFileSpec     file name (long version)
//
// out:
//      pszUniqueName
//
// returns:
//      TRUE    success, name can be used

BOOL lPathYetAnotherMakeUniqueNameA(LPSTR  pszUniqueName,
                 LPCSTR pszPath,
                 LPCSTR pszShort,
                 LPCSTR pszFileSpec)
{
    if (RUNNING_NT)
    {
        LPSTR lpPath,lpShort,lpFileSpec;
        WCHAR tmpBuf[MAX_PATH];
        WCHAR tmpPath[MAX_PATH];
        WCHAR tmpShort[MAX_PATH];
        WCHAR tmpFileSpec[MAX_PATH];
        BOOL retVal;

        if((lpPath = (LPSTR)pszPath)!= NULL)
        {
            lpPath = (LPSTR)tmpPath;
            MultiByteToWideChar(CP_ACP, 0, pszPath, -1, (LPWSTR)lpPath,
                ARRAYSIZE(tmpPath));
        }

        if((lpShort = (LPSTR)pszShort)!= NULL)
        {
            lpShort = (LPSTR)tmpShort;
            MultiByteToWideChar(CP_ACP, 0, pszShort, -1, (LPWSTR)lpShort,
                ARRAYSIZE(tmpShort));
        }

        if((lpFileSpec = (LPSTR)pszFileSpec)!= NULL)
        {
            lpFileSpec = (LPSTR)tmpFileSpec;
            MultiByteToWideChar(CP_ACP, 0, pszFileSpec, -1, (LPWSTR)lpFileSpec,
                ARRAYSIZE(tmpFileSpec));
        }

        MultiByteToWideChar(CP_ACP, 0, pszUniqueName, -1, tmpBuf,
            ARRAYSIZE(tmpBuf));

        retVal = PathYetAnotherMakeUniqueName((LPSTR)tmpBuf, lpPath, 
                                            lpShort, lpFileSpec);
        if(retVal)
            WideCharToMultiByte(CP_ACP, 0, tmpBuf, -1, 
                pszUniqueName, ARRAYSIZE(tmpBuf), 
                NULL, NULL);
        return(retVal);
    }
    else
        return(PathYetAnotherMakeUniqueName(pszUniqueName, pszPath, 
                                            pszShort, pszFileSpec));
}

////////////////////////////////////////////////////////////////
//
// in:
//      pszIconPath     file to get icon from (eg. cabinet.exe)
//      iIconIndex      icon index in pszIconPath to get
//      uIconFlags      GIL_ values indicating simulate doc icon, etc.

int lShell_GetCachedImageIndexA(LPCSTR pszIconPath, int iIconIndex, UINT uIconFlags)
{
    if (RUNNING_NT)
    {
        WCHAR uPath[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0, pszIconPath, -1, uPath,
            ARRAYSIZE(uPath));

        return Shell_GetCachedImageIndex((LPSTR)uPath, iIconIndex, uIconFlags);
    }
    else
        return Shell_GetCachedImageIndex(pszIconPath, iIconIndex, uIconFlags);
}

WINSHELLAPI BOOL  WINAPI PathIsExe(LPCSTR lpszPath);

// determine if a path is a program by looking at the extension
//
BOOL lPathIsExeA(LPCSTR szFile)
{
    if (RUNNING_NT)
    {
        WCHAR uPath[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0, szFile, -1, uPath,
            ARRAYSIZE(uPath));

        return (PathIsExe((LPCSTR)uPath));
    }
    else
        return PathIsExe(szFile);
}

/*
 * BUILDBUILD: Replacements for shell32.dll critical section functions for
 * shell32.dll!hash.c.
 */

void Shell_EnterCriticalSection(void)
{
    BeginExclusiveAccess();

    return;
}

void Shell_LeaveCriticalSection(void)
{
    EndExclusiveAccess();

    return;
}

#define DATA_SEG_READ_ONLY       ".text"

#pragma data_seg(DATA_SEG_READ_ONLY)

/*
 * This is silly.  They meant to use const char arrays here, rather than char
 * const arrays.  Arrays are implicitly const.  But this is how these array are
 * declared in shell32.dll!cstrings.h.
 */

char const c_szPercentOne[]         = "%1";
char const c_szPercentl[]           = "%l";
char const c_szPercentL[]           = "%L";
char const c_szRunDll[]             = "rundll32.exe";
char const c_szShellOpenCmd[]       = "shell\\open\\command";
char const c_szShellOpenDDEExec[]   = "shell\\open\\ddeexec";
char const c_szSlashCommand[]       = "\\command";
char const c_szSlashDDEExec[]       = "\\ddeexec";
char const c_szStar[]               = "*";

#pragma data_seg()

char g_szFileTypeName[32] = " ";  // First char is blank such that can append...

/* BUILDBUILD: LVUtil_GetLParam() swiped from shell32.dll!lvutil.c. */

//
// Note that it returns NULL, if iItem is -1.
//
LPARAM PASCAL LVUtil_GetLParam(HWND hwndLV, int i)
{
    LV_ITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = i;
    item.iSubItem = 0;
    item.lParam = 0;
    if (i != -1)
    {
    ListView_GetItem(hwndLV, &item);
    }

    return item.lParam;
}

/* BUILDBUILD: App_IsLFNAware() swiped from shell32.dll!shlexec.c. */

//----------------------------------------------------------------------------
#define PEMAGIC         ((WORD)'P'+((WORD)'E'<<8))
#if 0
/* BUILDBUILD: NEMAGIC is defined in newexe.h, #included for GetExeType(). */
#define NEMAGIC         ((WORD)'N'+((WORD)'E'<<8))
#endif
//----------------------------------------------------------------------------
// Returns TRUE is app is LFN aware.
// NB This simply assumes all Win4.0 and all Win32 apps are LFN aware.
BOOL App_IsLFNAware(LPCSTR pszFile)
{
    DWORD dw;

    ASSERT(pszFile);
    ASSERT(*pszFile);

    // Assume Win 4.0 apps and Win32 apps are LFN aware.
    dw = GetExeType(pszFile);
    // DebugMsg(DM_TRACE, "s.aila: %s %s %x", lpszFile, szFile, dw);
    if ((LOWORD(dw) == PEMAGIC) || ((LOWORD(dw) == NEMAGIC) && (HIWORD(dw) >= 0x0400)))
    {
    return TRUE;
    }
    else
    {
    return FALSE;
    }
}

/* BUILDBUILD: SendMessageD() swiped from shell32.dll!init.c. */

#ifdef DEBUG
LRESULT
WINAPI
SendMessageD(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    ASSERTNONCRITICAL;
    return SendMessageA(hWnd, Msg, wParam, lParam);
}
#endif // DEBUG

/* BUILDBUILD: HasExtension() swiped from shell32.dll!extract.c. */

DWORD HasExtension(LPCSTR pszPath)
{
    LPCSTR p = PathFindExtension(pszPath);

    if (*p == '.')
        return *((UNALIGNED DWORD *)p) | 0x20202000;  // make lower case
    else
        return 0;
}

/* BUILDBUILD: GetExeType() swiped from shell32.dll!extract.c. */

/****************************************************************************
 * GetExeType   - get the EXE type of the passed file (DOS, Win16, Win32)
 *
 *  returns:
 *      0 = not a exe of any type.
 *
 *      if a windows app
 *          LOWORD = NE or PE
 *          HIWORD = windows version 3.0, 3.5, 4.0
 *
 *      if a DOS app (or a .com or batch file)
 *          LOWORD = MZ
 *          HIWORD = 0
 *
 *      if a Win32 console app
 *          LOWORD = PE
 *          HIWORD = 0
 *
 *  BUGBUG this is so similar to the Win32 API GetBinaryType() too bad Win95
 *  kernel does not support it.
 *
 ****************************************************************************/

DWORD WINAPI GetExeType(LPCSTR szFile)
{
    HANDLE      fh;
    DWORD       dw;
    struct exe_hdr exehdr;
    struct new_exe newexe;
    FILETIME ftAccess;
    DWORD dwRead;

    //
    //  check for special extensions, and fail quick
    //
    switch (HasExtension(szFile))
    {
    case COM_FILE:
    case BAT_FILE:
        return MAKELONG(MZMAGIC, 0);  // DOS exe

    case EXE_FILE:                   // we need to open it.
        break;

    default:
        return 0;                    // not a exe, or if it is we dont care
    }

    newexe.ne_expver = 0;

    fh = CreateFile(szFile, GENERIC_READ | FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        0, OPEN_EXISTING, 0, 0);

    if (fh == INVALID_HANDLE_VALUE)
    {
    return 0;
    }

    // preserve the access time

    if (GetFileTime(fh, NULL, &ftAccess, NULL))
    SetFileTime(fh, NULL, &ftAccess, NULL);

    if (!ReadFile(fh, &exehdr, sizeof(exehdr), &dwRead, NULL) ||
        (dwRead != sizeof(exehdr)))
    goto error;

    if (exehdr.e_magic != EMAGIC)
        goto error;

    SetFilePointer(fh, exehdr.e_lfanew, NULL, FILE_BEGIN);
    ReadFile(fh,&newexe, sizeof(newexe), &dwRead, NULL);

    if (newexe.ne_magic == PEMAGIC)
    {
    // read the SubsystemVersion
    SetFilePointer(fh, exehdr.e_lfanew+18*4, NULL, FILE_BEGIN);
    ReadFile(fh,&dw,4, &dwRead, NULL);
    newexe.ne_expver = LOBYTE(LOWORD(dw)) << 8 | LOBYTE(HIWORD(dw));

    // read the Subsystem
    SetFilePointer(fh, exehdr.e_lfanew+23*4, NULL, FILE_BEGIN);
    ReadFile(fh,&dw,4, &dwRead, NULL);

    // if it is not a Win32 GUI app return a version of 0
    if (LOWORD(dw) != 2) // IMAGE_SUBSYSTEM_WINDOWS_GUI
        newexe.ne_expver = 0;

    goto exit;
    }
    else if (newexe.ne_magic == LEMAGIC)
    {
    newexe.ne_magic = MZMAGIC;      // just a DOS exe
    newexe.ne_expver = 0;
    }
    else if (newexe.ne_magic == NEMAGIC)
    {
    //
    //  we found a 'NE' it still might not be a windows
    //  app, it could be.....
    //
    //      a OS/2 app      ne_exetyp==NE_OS2
    //      a DOS4 app      ne_exetyp==NE_DOS4
    //      a VxD           ne_exetyp==DEV386
    //
    //      only treat it as a Windows app if the exetype
    //      is NE_WINDOWS or NE_UNKNOWN
    //
    if (newexe.ne_exetyp != NE_WINDOWS && newexe.ne_exetyp != NE_UNKNOWN)
    {
        newexe.ne_magic = MZMAGIC;      // just a DOS exe
        newexe.ne_expver = 0;
    }

    //
    //  if could also have a bogus expected windows version
    //  (treat 0 as invalid)
    //
    if (newexe.ne_expver == 0)
    {
        newexe.ne_magic = MZMAGIC;      // just a DOS exe
        newexe.ne_expver = 0;
    }
    }
    else // if (newexe.ne_magic != NEMAGIC)
    {
    newexe.ne_magic = MZMAGIC;      // just a DOS exe
    newexe.ne_expver = 0;
    }

exit:
    CloseHandle(fh);
    return MAKELONG(newexe.ne_magic, newexe.ne_expver);

error:
    CloseHandle(fh);
    return 0;
}

/******************************************************************************
             Original Shell32.dll code starts here.
******************************************************************************/

#if 0

/* BUILDBUILD: Remove old headers. */

#include "..\..\inc\help.h"
#include "views.h"
#include "ids.h"
#include "lvutil.h"

#endif

// BUGBUG: duplicate strings as in cstrings.c
#pragma data_seg(".text", "CODE")
char const c_szSSlashS[] = "%s\\%s";
#pragma data_seg()

extern char g_szFileTypeName[];     // used to build type name...
extern BOOL App_IsLFNAware(LPCSTR pszFile);

void _GenerateAssociateNotify(LPSTR pszExt)
{
    char szFakePath[MAX_PATH];
    LPITEMIDLIST pidl;
    //
    // This is a real hack, but for now we generate an idlist that looks
    // something like: C:\*.ext which is the extension for the IDList.
    // We use the simple IDList as to not hit the disk...
    //
    // All I do is make an ANSI string, and then jam the unicode
    // version into the skinny-char buffer. Saves on stack.
    if (RUNNING_NT)
    {
        char tmpBuf[20];

        lstrcpy(tmpBuf, "c:\\");
        lstrcat(tmpBuf, c_szStar);
        lstrcat(tmpBuf, pszExt);
        MultiByteToWideChar(CP_ACP, 0, tmpBuf, -1, (LPWSTR)szFakePath, 
            sizeof(szFakePath)/sizeof(WCHAR));
    }
    else
    {
        PathBuildRoot(szFakePath, 2);   // (c:\)
        lstrcat(szFakePath, c_szStar);
        lstrcat(szFakePath, pszExt);
    }
    pidl = SHSimpleIDListFromPath(szFakePath);

    // Now call off to the notify function.
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, pidl, NULL);
    ILFree(pidl);
}

// Given a class key returns the shell\open\command string in szValue
// and the number of chars copied in cbMaxValue. cbMaxValue should
// be initialised to the max siz eof szValue.
void GetCmdLine(LPCSTR szKey, LPSTR szValue, LONG cbValue)
{
    char szTemp[MAX_PATH+40];   // Leave room for both extension plus junk on at end...

    wsprintf(szTemp, c_szSSlashS, szKey, c_szShellOpenCmd);

    szValue[0] = 0;
    RegQueryValue(HKEY_CLASSES_ROOT, szTemp, szValue, &cbValue);
}

// uFlags GCD_ flags from GetClassDescription uFlags

void FillListWithClasses(HWND hwnd, BOOL fComboBox, UINT uFlags)
{
    int i;
    char szClass[CCH_KEYMAX];
    char szDisplayName[CCH_KEYMAX];
    LONG lcb;

    SendMessage(hwnd, fComboBox ? CB_RESETCONTENT : LB_RESETCONTENT, 0, 0L);

    if (uFlags & GCD_MUSTHAVEEXTASSOC)
    {
    char szExt[CCH_KEYMAX];

    // The caller stated that they only want those classes that
    // have have at least one extension associated with it.
    //
    for (i = 0; RegEnumKey(HKEY_CLASSES_ROOT, i, szClass, sizeof(szClass)) == ERROR_SUCCESS; i++)
    {
        // Is this an extension
        if (szClass[0] != '.')
        continue;   // go process the next one...

        // Get the class name
        lstrcpy(szExt, szClass);
        lcb = sizeof(szClass);
        if ((RegQueryValue(HKEY_CLASSES_ROOT, szExt, szClass, &lcb) != ERROR_SUCCESS) || (lcb == 0))
        continue;   // Again we are not interested.

        // use uFlags passed in to filter
        if (GetClassDescription(HKEY_CLASSES_ROOT, szClass, szDisplayName, sizeof(szDisplayName), uFlags))
        {
        int iItem;

        // Now make sure it is not already in the list...
        if ((int)SendMessage(hwnd, fComboBox ? CB_FINDSTRINGEXACT : LB_FINDSTRINGEXACT,
                     (WPARAM)-1, (LPARAM)(LPSTR)szDisplayName) >= 0)
            continue;       // allready in the list.

        // sorted
        iItem = (int)SendMessage(hwnd, fComboBox ? CB_ADDSTRING : LB_ADDSTRING,
                     0, (LPARAM)(LPSTR)szDisplayName);

        if (iItem >= 0)
            SendMessage(hwnd, fComboBox ? CB_SETITEMDATA : LB_SETITEMDATA, iItem, (LPARAM)AddHashItem(NULL, szClass));

        }
    }


    }
    else
    {
    for (i = 0; RegEnumKey(HKEY_CLASSES_ROOT, i, szClass, sizeof(szClass)) == ERROR_SUCCESS; i++)
    {
        // use uFlags passed in to filter
        if (GetClassDescription(HKEY_CLASSES_ROOT, szClass, szDisplayName, sizeof(szDisplayName), uFlags))
        {
        // sorted
        int iItem = (int)SendMessage(hwnd, fComboBox ? CB_ADDSTRING : LB_ADDSTRING,
                         0, (LPARAM)(LPSTR)szDisplayName);

        if (iItem >= 0)
            SendMessage(hwnd, fComboBox ? CB_SETITEMDATA : LB_SETITEMDATA, iItem, (LPARAM)AddHashItem(NULL, szClass));

        }
    }
    }
}

// get the displayable name for file types "classes"
//
//
// uFlags:
//     GCD_MUSTHAVEOPENCMD  only returns things with open verbs
//     GCD_ADDEXETODISPNAME append the name of the ext that is in the open cmd
//              (GCD_MUSTHAVEOPENCMD)
//     GCD_ALLOWPSUDEOCLASSES   return psudeo classes, those with stuff haning
//              off the .ext key

BOOL GetClassDescription(HKEY hkClasses, LPCSTR pszClass, LPSTR szDisplayName, int cbDisplayName, UINT uFlags)
{
    char szExe[MAX_PATH];
    char szClass[CCH_KEYMAX];
    LPSTR pszExeFile;
    LONG lcb;

    // Skip things that aren't classes (extensions).

    if (pszClass[0] == '.')
    {
    if (uFlags & GCD_ALLOWPSUDEOCLASSES)
    {
    lcb = sizeof(szClass);
        if ((RegQueryValue(hkClasses, pszClass, szClass, &lcb) != ERROR_SUCCESS) || (lcb == 0))
    {
        // look for .ext\shel\open\command directly (hard wired association)
        // if this extenstion does not name a real class

        GetCmdLine(pszClass, szExe, sizeof(szExe));
        if (szExe[0]) {
        lstrcpyn(szDisplayName, PathFindFileName(szExe), cbDisplayName);
        return TRUE;
        }

        return FALSE;
    }
    pszClass = szClass;
    }
    else
    {
    return FALSE;   // don't return psudeo class
    }
    }

    // REVIEW: we should really special case the OLE junk here.  if pszClass is
    // CLSID, Interface, TypeLib, etc we should skip it

    // REVIEW: we really need to verify that some extension points at this type to verfy
    // that it is valid.  perhaps the existance of a "shell" key is enough.

    // get the classes displayable name
    lcb = cbDisplayName;
    if (RegQueryValue(hkClasses, pszClass, szDisplayName, &lcb) != ERROR_SUCCESS || (lcb < 2))
    return FALSE;

    if (uFlags & GCD_MUSTHAVEOPENCMD)
    {
    // verify that it has an open command
    GetCmdLine(pszClass, szExe, sizeof(szExe));
    if (!szExe[0])
    return FALSE;

    // BUGBUG: currently this is dead functionallity
    if (uFlags & GCD_ADDEXETODISPNAME)
    {
        PathRemoveArgs(szExe);

        // eliminate per instance type things (programs, pif, etc)
        // Skip things that aren't relevant ot the shell.
        if (szExe[0] == '%')
        return FALSE;

        // skip things with per-instance type associations
        pszExeFile = PathFindFileName(szExe);

        if (lstrlen(szDisplayName) + lstrlen(pszExeFile) + 2 < cbDisplayName)
        {
#pragma data_seg(".text", "CODE")
        wsprintf(szDisplayName + lstrlen(szDisplayName), " (%s)", pszExeFile);
#pragma data_seg()
        }
    }
    }
    return TRUE;
}

void DeleteListAttoms(HWND hwnd, BOOL fComboBox)
{
    int cItems;
    PHASHITEM phiClass;
    int iGetDataMsg;

    iGetDataMsg = fComboBox ? CB_GETITEMDATA : LB_GETITEMDATA;

    cItems = (int)SendMessage(hwnd, fComboBox ? CB_GETCOUNT : LB_GETCOUNT, 0, 0) - 1;

    /* clean out them atoms except for "(none)".
     */
    for (; cItems >= 0; cItems--)
    {
    phiClass = (PHASHITEM)SendMessage(hwnd, iGetDataMsg, cItems, 0L);
    if (phiClass != (PHASHITEM)LB_ERR && phiClass)
    DeleteHashItem(NULL, phiClass);
    }
}

// BEGIN new stuff

typedef struct {    // oad
    // params
    HWND hwnd;
    POPENASINFO poainfo;
    // local data
    int idDlg;
    HWND hDlg;
    HWND hwndList;
    LPSTR lpszExt;
    LPCSTR lpcszClass;
    HRESULT hr;
    char szTypeName[CCH_KEYMAX]; // type name
    char szDescription[CCH_KEYMAX]; // type description
} OPENAS_DATA, *POPENAS_DATA;

typedef struct {
    UINT bHasQuote;             // Did we find a quote around param? "%1"
    char szApp[MAX_PATH+2];     // May need room for quotes
} APPINFO;

#define HASQUOTE_NO     0
#define HASQUOTE_MAYBE  1
#define HASQUOTE_YES    3

BOOL QuotesAroundArg(PCSTR pcszCmdLine)
{
    BOOL bQuotes = FALSE;
    PCSTR pcsz;

    /* Is there a %1, %l, or %L on the command line? */

    if ((pcsz = StrStr(pcszCmdLine, c_szPercentOne)) != NULL ||
    (pcsz = StrStr(pcszCmdLine, c_szPercentl))   != NULL ||
    (pcsz = StrStr(pcszCmdLine, c_szPercentL))   != NULL)
    {
    /* Yes.  Is it preceded by double quotes? */

    if (*(pcsz - 1) == '"')
        /* Yes. */
        bQuotes = TRUE;
    }

    return(bQuotes);
}

void FillListWithApps(HWND hwndList)
{
    int i, iMax;
    char szClass[CCH_KEYMAX];
    char szKey[CCH_KEYMAX];
    char szValue[MAX_PATH];
    APPINFO *paiLast;
    LV_ITEM item;
    int iLast;
    BOOL fLastExists = FALSE;

    for (i = 0; RegEnumKey(HKEY_CLASSES_ROOT, i, szClass, sizeof(szClass)) == ERROR_SUCCESS; i++)
    {
    LONG lTmp;

    wsprintf(szKey, c_szSSlashS, szClass, c_szShellOpenCmd);
    lTmp = sizeof(szValue);
    if (RegQueryValue(HKEY_CLASSES_ROOT, szKey, szValue, &lTmp) == ERROR_SUCCESS)
    {
    // filter out stuff we know is bogus
    // strings that start with: %1, "%1"
    // strings that contain: rundll
    //

        if ((szValue[0] != '%') &&
        ((szValue[0] != '"') || (szValue[1] != '%')) &&
        (StrStr(szValue, c_szRunDll) == NULL))
        {
        APPINFO *pai = (APPINFO *)LocalAlloc(LPTR, sizeof(APPINFO));
        if (pai)
        {
            if (QuotesAroundArg(szValue))
            pai->bHasQuote = HASQUOTE_YES;
            else
            {
            char szDDEExecValue[MAX_PATH];

            wsprintf(szKey, c_szSSlashS, szClass,
                 c_szShellOpenDDEExec);
            lTmp = sizeof(szDDEExecValue);
            if (RegQueryValue(HKEY_CLASSES_ROOT, szKey,
                      szDDEExecValue, &lTmp)
                == ERROR_SUCCESS)
            {
                if (QuotesAroundArg(szDDEExecValue))
                pai->bHasQuote = HASQUOTE_YES;
            }
            }

            PathRemoveArgs(szValue);
            lstrcpy(pai->szApp, szValue);

            item.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
            item.iItem = 0x7FFF;
            item.iSubItem = 0;
            item.state = 0;
            item.iImage = I_IMAGECALLBACK;
            PathRemoveExtension(szValue);
            item.pszText = PathFindFileName(szValue);
            item.lParam = (LPARAM)pai;
            ListView_InsertItem(hwndList, &item);
        }
        }
    }
    }

    // punt dups
    ListView_SortItems(hwndList, NULL, 0);
    paiLast = NULL;

    // szKey will hold the lpszLast's display name
    for (i = 0, iMax = ListView_GetItemCount(hwndList); i < iMax; i++)
    {
    APPINFO *pai;

    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = i;
    item.iSubItem = 0;
    item.pszText = szValue;
    item.cchTextMax = sizeof(szValue);
    // get the text string
    ListView_GetItem(hwndList, &item);
    pai = (APPINFO *)item.lParam;

    if (paiLast && (!lstrcmpi(szKey, szValue))) {
        int iDelete = i;

        // if they are different in path, delete the one that doesn't exit (or the new one if they both do)
        if (lstrcmpi(paiLast->szApp, pai->szApp)) {
        if (!fLastExists && !(fLastExists = PathFileExists(pai->szApp))) {
            if (paiLast->bHasQuote > pai->bHasQuote)
            pai->bHasQuote = paiLast->bHasQuote;
            iDelete = iLast;
        }
        }

        // Will assume that if either item has quote set that it will work...
        if (pai->bHasQuote > paiLast->bHasQuote)
        paiLast->bHasQuote =pai->bHasQuote;

        ListView_DeleteItem(hwndList, iDelete);
        i--; iMax--;

        // we deleted the iLast, we need to set a new iLast
        if (iDelete == iLast)
        goto NewLastExe;

    } else {

NewLastExe:
        iLast = i;

        paiLast = pai;
        lstrcpy(szKey, szValue);
        fLastExists = TRUE;
    }
    }

    // Lets set the focus to first item, but not the focus as some users
    // have made the mistake and type in the name and hit return and it
    // runs the first guy in the list which in the current cases tries to
    // run the backup app...
    ListView_SetItemState(hwndList, 0, LVNI_FOCUSED, LVNI_FOCUSED | LVNI_SELECTED);
    SetFocus(hwndList);
}

void _InitOpenAsDlg(POPENAS_DATA poad)
{
    char szFormat[200];
    char szFileName[MAX_PATH];
    char szTemp[MAX_PATH + sizeof(szFormat)];
    BOOL fDisableAssociate;
    HIMAGELIST himlLarge = NULL;
    HIMAGELIST himlSmall = NULL;
    LV_COLUMN col = {LVCF_FMT | LVCF_WIDTH, LVCFMT_LEFT};
    RECT rc;

    // Don't let the file name go beyond the width of one line...
    GetDlgItemText(poad->hDlg, IDD_TEXT, szFormat, sizeof(szFormat));
    lstrcpy(szFileName, PathFindFileName(poad->poainfo->pcszFile));
    GetClientRect(GetDlgItem(poad->hDlg, IDD_TEXT), &rc);

    PathCompactPath(NULL, szFileName, rc.right - 4 * GetSystemMetrics(SM_CXBORDER));

    wsprintf(szTemp, szFormat, szFileName);
    SetDlgItemText(poad->hDlg, IDD_TEXT, szTemp);

    // Don't allow associations to be made for things we consider exes...
    fDisableAssociate = (! (poad->poainfo->dwInFlags & OPENASINFO_FL_ALLOW_REGISTRATION) ||
             lPathIsExeA(poad->poainfo->pcszFile));

    if (poad->idDlg == DLG_OPENAS_NOTYPE) {
    GetDlgItemText(poad->hDlg, IDD_DESCRIPTIONTEXT, szFormat, sizeof(szFormat));
    wsprintf(szTemp, szFormat, poad->lpcszClass);
    SetDlgItemText(poad->hDlg, IDD_DESCRIPTIONTEXT, szTemp);

    // Default to Set the association here if we do not know what
    // the file is...
    if (!fDisableAssociate)
        CheckDlgButton(poad->hDlg, IDD_MAKEASSOC, TRUE);
    }

    if (fDisableAssociate)
    EnableWindow(GetDlgItem(poad->hDlg, IDD_MAKEASSOC), FALSE);

    poad->hwndList = GetDlgItem(poad->hDlg, IDD_APPLIST);
    Shell_GetImageLists(&himlLarge, &himlSmall);
    if(himlLarge != NULL)
        ListView_SetImageList(poad->hwndList, himlLarge, LVSIL_NORMAL);
    if(himlSmall != NULL)
        ListView_SetImageList(poad->hwndList, himlSmall, LVSIL_SMALL);
    SetWindowLong(poad->hwndList, GWL_EXSTYLE,
        GetWindowLong(poad->hwndList, GWL_EXSTYLE) | WS_EX_CLIENTEDGE);

    GetClientRect(poad->hwndList, &rc);
    col.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL)
        - 4 * GetSystemMetrics(SM_CXEDGE);
    ListView_InsertColumn(poad->hwndList, 0, &col);
    FillListWithApps(poad->hwndList);

    // and initialize the OK button
    EnableWindow(GetDlgItem(poad->hDlg, IDOK),
        (ListView_GetNextItem(poad->hwndList, -1, LVNI_SELECTED) != -1));
}

void RunAs(POPENAS_DATA poad)
{
    SHELLEXECUTEINFO ExecInfo;

    // If this run created a new type we should use it, otherwise we should
    // construct a command using the exe that we selected...
    if (*poad->lpszExt && IsDlgButtonChecked(poad->hDlg, IDD_MAKEASSOC))
    {
    FillExecInfo(ExecInfo, poad->hwnd, NULL, poad->poainfo->pcszFile, NULL, NULL, SW_NORMAL);
    }
    else
    {
    char szApp[MAX_PATH];
    char szQuotedFile[MAX_PATH+2];
    APPINFO *pai;
    int iItemFocus = ListView_GetNextItem(poad->hwndList, -1, LVNI_SELECTED);
    pai = (APPINFO *)LVUtil_GetLParam(poad->hwndList, iItemFocus);
    lstrcpy(szQuotedFile, poad->poainfo->pcszFile);

    lstrcpy(szApp, pai->szApp);
    PathUnquoteSpaces(szApp);

    // Try to intellegently quote blanks or not...
    if (!App_IsLFNAware(szApp))
    {
        // We better also make sure that this a short path name
        // pass off to the application...
        char tmpBuf[MAX_PATH];

        GetShortPathName(szQuotedFile, tmpBuf, SIZECHARS(tmpBuf));
        lstrcpy(szQuotedFile, tmpBuf);
    }
    else
    {
        // Either maybe or yes is we should quote
        if (pai->bHasQuote)
        PathQuoteSpaces(szQuotedFile);
    }

    FillExecInfo(ExecInfo, poad->hwnd, NULL, szApp, szQuotedFile, NULL, SW_NORMAL);
    }
    ShellExecuteEx(&ExecInfo);
}

void OpenAsOther(POPENAS_DATA poad)
{
    char szText[MAX_PATH];
    BOOL retval;

#if 0
    /*
     * BUILDBUILD: There is no IDD_COMMAND control in DLG_OPENAS or
     * DLG_OPENAS_NOTYPE.
     */
    GetDlgItemText(poad->hDlg, IDD_COMMAND, szText, sizeof(szText));
#else
    *szText = '\0\0';
#endif
    // now it will work on NT: unicode 
    // do a file open browse
    if (RUNNING_NT)
    {
        WCHAR szuPath[MAX_PATH];
        WCHAR szuExe[16];
        WCHAR szuFilters[MAX_PATH];
        WCHAR szuTitle[80];
        LPWSTR psz;

        szuPath[0] = szuTitle[0] = szuExe[0] = szuFilters[0]= 0;
        MLLoadStringW(IDS_OPENAS, szuTitle, 80);
        MLLoadStringW(IDS_EXE, szuExe, 16);
        MLLoadStringW(IDS_PROGRAMSFILTER, szuFilters, MAX_PATH);

        /* hack up the array... */ 
        psz = szuFilters;
        while (*psz)
        {
            if (*psz == (WCHAR)('#'))
                *psz = (WCHAR)('\0');
            psz++;
        }

        retval = GetFileNameFromBrowse(poad->hDlg, (char *)szuPath, 
                        ARRAYSIZE(szuPath), NULL, (LPCSTR)szuExe, 
                        (LPCSTR)szuFilters, (LPCSTR)szuTitle);
        /* make certain we convert back to ANSI chars! */
        if (retval)
            WideCharToMultiByte(CP_ACP,0,szuPath,-1,szText,
                ARRAYSIZE(szuPath),NULL,NULL);
    }
    else
    {
        CHAR szExe[16];
        CHAR szFilters[MAX_PATH];
        CHAR szTitle[80];
        LPSTR psz;

        szTitle[0] = szExe[0] = szFilters[0]= 0;
        MLLoadStringA(IDS_OPENAS, szTitle, 80);
        MLLoadStringA(IDS_EXE, szExe, 16);
        MLLoadStringA(IDS_PROGRAMSFILTER, szFilters, MAX_PATH);

        /* hack up the array... */ 
        psz = szFilters;
        while (*psz)
        {
            if (*psz == (CHAR)('#'))
                *psz = (CHAR)('\0');
            psz++;
        }

        retval = GetFileNameFromBrowse(poad->hDlg, szText, 
                    sizeof(szText), NULL,
                    szExe, szFilters, szTitle);
    }
    if (retval)
    {
    // then add it to the list view and select it.
    APPINFO *pai = (APPINFO *)LocalAlloc(LPTR, sizeof(APPINFO));
    if (pai)
    {
        LV_ITEM item;
        int i;
        int iItem;
        APPINFO *paiT;


        pai->bHasQuote = HASQUOTE_MAYBE;
        lstrcpy(pai->szApp, szText);
        PathQuoteSpaces(pai->szApp);

        item.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
        item.iItem = 0x7FFF;
        item.iSubItem = 0;
        item.state = 0;
        item.iImage = I_IMAGECALLBACK;
        PathRemoveExtension(szText);
        item.pszText = PathFindFileName(szText);
        item.lParam = (LPARAM)pai;
        i = ListView_InsertItem(poad->hwndList, &item);
        ListView_SetItemState(poad->hwndList, i, LVNI_SELECTED | LVNI_FOCUSED, LVNI_SELECTED | LVNI_FOCUSED);
        ListView_EnsureVisible(poad->hwndList, i, FALSE);
        SetFocus(poad->hwndList);

        // We also want to see if we find another entry in the listview for the
        // application.  We do this such that we can see if the app supports
        // quotes or not.
        for (iItem = ListView_GetItemCount(poad->hwndList) - 1; iItem >= 0; iItem--)
        {
        if (iItem == i)
            continue;
        item.mask = LVIF_PARAM;
        item.iItem = iItem;
        item.iSubItem = 0;
        ListView_GetItem(poad->hwndList, &item);
        paiT = (APPINFO *)item.lParam;
        if (lstrcmpi(pai->szApp, paiT->szApp) == 0)
        {
            pai->bHasQuote = paiT->bHasQuote;
            break;
        }
        }
    }
    }
}

// return true if ok to continue
BOOL OpenAsMakeAssociation(POPENAS_DATA poad)
{
    UINT err;

    // See if we should make an association or not...
    if (!IsDlgButtonChecked(poad->hDlg, IDD_MAKEASSOC))
    return(TRUE);

    if (poad->idDlg == DLG_OPENAS_NOTYPE) {
    GetDlgItemText(poad->hDlg, IDD_DESCRIPTION, poad->szDescription, sizeof poad->szDescription);
    if (*poad->szDescription == '\0') {
        // Another place to make sure Ivans tests don't catch...
        /* BUILDBUILD: Load IDS_FILETYPENAME here. */
        if (lstrlen(g_szFileTypeName) <2)
        {
            MLLoadStringA(IDS_FILETYPENAME,
                   g_szFileTypeName+1, sizeof(g_szFileTypeName)-1);
        }
        lstrcpyn(poad->szDescription, AnsiNext(poad->lpszExt),
            ARRAYSIZE(poad->szDescription) - lstrlen(g_szFileTypeName) -1);
        AnsiUpper(poad->szDescription);

        // Likewise we add a blank at start for appending for FOO Files..
        lstrcat(poad->szDescription, g_szFileTypeName);
    }

#pragma data_seg(".text", "CODE")
    lstrcpyn(poad->szTypeName, poad->lpszExt+1, sizeof(poad->szTypeName)
        - sizeof("_auto_file"));
    lstrcat(poad->szTypeName, "_auto_file");
#pragma data_seg()

    err = RegSetValueA(HKEY_CLASSES_ROOT, poad->lpszExt, REG_SZ, poad->szTypeName, 0L);
    err |= RegSetValueA(HKEY_CLASSES_ROOT, poad->szTypeName, REG_SZ, poad->szDescription, 0L);
    ASSERT(err == ERROR_SUCCESS);
    if (err != ERROR_SUCCESS)
    {
        MessageBeep(MB_ICONEXCLAMATION);
        return(FALSE);
    }
    }

    if (*poad->lpszExt) {
    char szTemp[MAX_PATH];
    char szCommand[MAX_PATH + 10];
    int iItemFocus = ListView_GetNextItem(poad->hwndList, -1, LVNI_FOCUSED);
    APPINFO *pai = (APPINFO *)LVUtil_GetLParam(poad->hwndList, iItemFocus);

#pragma data_seg(".text", "CODE")
    // We need to set the open commands value to empty to take care of cases
    // that have something like: open=&Merge
    wsprintf(szTemp, "%s\\shell\\open", poad->szTypeName);
    err = RegSetValueA(HKEY_CLASSES_ROOT, szTemp, REG_SZ, c_szNULL, 0L);
    lstrcat(szTemp, c_szSlashCommand);

    if ((pai->bHasQuote == HASQUOTE_YES) ||
        ((pai->bHasQuote == HASQUOTE_MAYBE) && App_IsLFNAware(pai->szApp)))
        wsprintf(szCommand, "%s \"%%1\"", pai->szApp);
    else
        wsprintf(szCommand, "%s %%1", pai->szApp);

    err = RegSetValueA(HKEY_CLASSES_ROOT, szTemp, REG_SZ, szCommand, 0L);
    ASSERT(err == ERROR_SUCCESS);
    if (err != ERROR_SUCCESS)
    {
        MessageBeep(MB_ICONEXCLAMATION);
        return(FALSE);
    }

    // Need to delete any ddeexec information that might also exist...
    PathRemoveFileSpec(szTemp);     // Remove the command (last component)
    lstrcat(szTemp, c_szSlashDDEExec);
    lRegDeleteKeyA(HKEY_CLASSES_ROOT, szTemp);
#pragma data_seg()
    // notify views
    _GenerateAssociateNotify(poad->lpszExt);
    }

    return TRUE;
}


#pragma data_seg(DATASEG_READONLY)
const static DWORD aOpenAsHelpIDs[] = {  // Context Help IDs
    IDD_TEXT,             IDH_FCAB_OPENAS_DESCRIPTION,
    IDD_DESCRIPTIONTEXT,  IDH_FCAB_OPENAS_DESCRIPTION,
    IDD_DESCRIPTION,      IDH_FCAB_OPENAS_DESCRIPTION,
    IDD_APPLIST,          IDH_FCAB_OPENAS_APPLIST,
    IDD_MAKEASSOC,        IDH_FCAB_OPENAS_MAKEASSOC,
    IDD_OTHER,            IDH_FCAB_OPENAS_OTHER,

    0, 0
};
#pragma data_seg()

INT_PTR CALLBACK OpenAsDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    POPENAS_DATA poad = (POPENAS_DATA)GetWindowLongPtr(hDlg, DWLP_USER);
    APPINFO *pai;
    int iItemFocus;

    switch (wMsg) {

    case WM_INITDIALOG:
    SetWindowLongPtr(hDlg, DWLP_USER, lParam);
    poad = (POPENAS_DATA)lParam;
    poad->hDlg = hDlg;
    _InitOpenAsDlg(poad);
    break;

    case WM_HELP:
    SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
        HELP_WM_HELP, (DWORD_PTR)(LPSTR) aOpenAsHelpIDs);
    break;

    case WM_CONTEXTMENU:
    if ((int)SendMessage(hDlg, WM_NCHITTEST, 0, lParam) != HTCLIENT)
        return FALSE;   // don't process it
    SHWinHelpOnDemandWrap((HWND) wParam, NULL, HELP_CONTEXTMENU,
        (DWORD_PTR)(LPVOID)aOpenAsHelpIDs);
    break;

   case WM_NOTIFY:

    switch (((LPNMHDR)lParam)->code)
    {
    case LVN_GETDISPINFO:
    {
#define pdi ((LV_DISPINFO *)lParam)
        char szApp[MAX_PATH];
        APPINFO *pai = (APPINFO *)pdi->item.lParam;
        lstrcpy(szApp, pai->szApp);
        PathUnquoteSpaces(szApp);
        pdi->item.iImage = lShell_GetCachedImageIndexA(szApp, 0, 0);
        if (pdi->item.iImage == -1)
        pdi->item.iImage = II_APPLICATION;
        break;
#undef pdi
    }

    case LVN_DELETEITEM:
        LocalFree((HLOCAL)((NM_LISTVIEW *)lParam)->lParam);
        break;

    case LVN_ITEMCHANGED:
        EnableWindow(GetDlgItem(hDlg, IDOK),
    (ListView_GetNextItem(poad->hwndList, -1, LVNI_SELECTED) != -1));
        break;

    case NM_DBLCLK:
        if (IsWindowEnabled(GetDlgItem(hDlg, IDOK)))
        PostMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDOK, hDlg, 0));
        break;
    }
    break;

    case WM_COMMAND:
    ASSERT(poad);
    switch (GET_WM_COMMAND_ID(wParam, lParam)) {
    case IDD_OTHER:
        OpenAsOther(poad);
        break;

    case IDOK:
        /* Register association if requested. */

        // The NT URL has this commented out...
        if (//(poad->poainfo->dwInFlags & OPENASINFO_FL_REGISTER_EXT) &&
        ! OpenAsMakeAssociation(poad))
        break;

        /* Copy name of associated application. */

        iItemFocus = ListView_GetNextItem(poad->hwndList, -1, LVNI_SELECTED);
        pai = (APPINFO *)LVUtil_GetLParam(poad->hwndList, iItemFocus);
        lstrcpyn(poad->poainfo->szApp, pai->szApp, sizeof(poad->poainfo->szApp));
        PathUnquoteSpaces(poad->poainfo->szApp);

        /* Did we register the association? */

        poad->hr = IsDlgButtonChecked(poad->hDlg, IDD_MAKEASSOC) ? S_OK : S_FALSE;

        /* Exec if requested. */

        if (poad->poainfo->dwInFlags & OPENASINFO_FL_EXEC)
        {
        RunAs(poad);
        SHAddToRecentDocs(RUNNING_NT ? SHARD_PATHA : SHARD_PATH, 
                            poad->poainfo->pcszFile);
        }

        EndDialog(hDlg, TRUE);
        break;

    case IDCANCEL:
        poad->hr = E_ABORT;
        EndDialog(hDlg, FALSE);
        break;

    }
    break;

    default:
    return FALSE;
    }
    return TRUE;
}

// external API version

HRESULT MyOpenAsDialog(HWND hwnd, POPENASINFO poainfo)
{
    OPENAS_DATA oad;
    int idDlg;

    oad.hwnd = hwnd;
    oad.poainfo = poainfo;
    oad.szDescription[0] = 0;
    oad.szTypeName[0] = 0;

    TRACE_OUT(("Enter OpenAs for %s", oad.poainfo->pcszFile));

    oad.lpszExt = PathFindExtension(oad.poainfo->pcszFile);
    oad.lpcszClass = poainfo->pcszClass ? poainfo->pcszClass : oad.lpszExt;
    if (*oad.lpszExt) {
    LONG lTmp = sizeof(oad.szTypeName);

    if ((RegQueryValue(HKEY_CLASSES_ROOT, oad.lpszExt, oad.szTypeName, &lTmp) == ERROR_SUCCESS)
        && (lTmp != 0) && (*oad.szTypeName)) {
        idDlg = DLG_OPENAS;
    } else
        idDlg = DLG_OPENAS_NOTYPE;
    } else {
    idDlg = DLG_OPENAS;
    }
    oad.idDlg = idDlg;
    return((DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(idDlg), hwnd,
               OpenAsDlgProc, (LPARAM)(POPENAS_DATA)&oad) != -1)
       ? oad.hr : E_OUTOFMEMORY);
}

#if 0

/* BUILDBUILD: Not used in url.dll.  Restore if added to shell32.dll. */

void WINAPI OpenAs_RunDLL(HWND hwnd, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    OPENASINFO oainfo;

    TRACE_OUT(("OpenAs_RunDLL is called with (%s)", lpszCmdLine));

    oainfo.pcszFile = lpszCmdLine;
    oainfo.pcszClass = NULL;
    oainfo.dwInFlags = (OPENASINFO_FL_ALLOW_REGISTRATION |
            OPENASINFO_FL_REGISTER_EXT |
            OPENASINFO_FL_EXEC);

    MyOpenAsDialog(hwnd, &oainfo);
}

#ifdef DEBUG
//
// Type checking
//
static RUNDLLPROC lpfnRunDLL = OpenAs_RunDLL;
#endif

#endif
