#include "shellprv.h"
#pragma  hdrstop

#include "copy.h"

#include <regstr.h>
#include <comcat.h>
#include <fsmenu.h>
#include <intshcut.h>
#include "_security.h"

#include "ovrlaymn.h"

#include "fstreex.h"
#include "drives.h"
#include "netview.h"

#include <filetype.h>

#include "infotip.h"
#include "recdocs.h"
#include <idhidden.h>
#include "datautil.h"
#include "deskfldr.h"
#include "prop.h"           // COL_DATA
#include <filter.h>         // COL_DATA
#include "folder.h"

#ifdef DEBUG // For leak detection
#include <dbgmem.h>

EXTERN_C void GetAndRegisterLeakDetection(void);
EXTERN_C BOOL g_fInitTable;
EXTERN_C LEAKDETECTFUNCS LeakDetFunctionTable;
#endif

#define SHCF_IS_BROWSABLE           (SHCF_IS_SHELLEXT | SHCF_IS_DOCOBJECT)


BOOL FS_GetCLSID(LPCIDFOLDER pidf, CLSID *pclsid);

HRESULT FSGetClassKey(LPCIDFOLDER pidf, HKEY *phkeyProgID);

HRESULT FS_GetJunctionForBind(LPCIDFOLDER pidf, LPIDFOLDER *ppidfBind, LPCITEMIDLIST *ppidlRight);
HRESULT FS_Bind(CFSFolder *this, LPBC pbc, LPCIDFOLDER pidf, REFIID riid, void **ppv);

// exedrop.c
STDAPI CExeDropTarget_CreateInstance(HWND hwnd, LPCITEMIDLIST pidlFirst, IDropTarget **ppv);

// in stdenum.cpp
void* CStandardEnum_CreateInstance(REFIID riid, BOOL bInterfaces, int cElement, int cbElement, void *rgElements,
                 void (WINAPI * pfnCopyElement)(void *, const void *, DWORD));

// in defviewx.c
STDAPI SHGetIconFromPIDL(IShellFolder *psf, IShellIcon *psi, LPCITEMIDLIST pidl, UINT flags, int *piImage);

// in filetype.cpp
STDAPI_(DWORD) GetFileTypeAttributes(HKEY hkeyFT);

STDAPI CFolderExtractImage_Create(LPCTSTR pszPath, REFIID riid, void **ppvObj);

#define CSIDL_NORMAL    ((UINT)-2)  // has to not be -1

// in fldrscut.cpp
BOOL IsFolderShortcut(LPCTSTR pszName);

// in mtpt.cpp
HRESULT MountPoint_RegisterChangeNotifyAlias(int iDrive);

// File-scope pointer to a ShellIconOverlayManager
// Callers access this pointer through GetIconOverlayManager().
static IShellIconOverlayManager * g_psiom = NULL;

//#define FULL_DEBUG

TCHAR const c_szIconHandler[] = TEXT("shellex\\IconHandler");

TCHAR const c_szCLSIDSlash[] = TEXT("CLSID\\");
TCHAR const c_szDirectoryClass[] = TEXT("Directory");
TCHAR const c_szShellOpenCmd[] = TEXT("shell\\open\\command");
TCHAR const c_szPercentOne[] = TEXT("%1");
TCHAR const c_szPercentOneInQuotes[] = TEXT("\"%1\"");

HKEY SHOpenCLSID(HKEY hkeyProgID);
BOOL SHGetClass(LPCIDFOLDER pidf, LPTSTR pszClass, UINT cch);

HRESULT FSLoadHandler(CFSFolder *this, LPCIDFOLDER pidf, LPCTSTR pszHandlerType, REFIID riid, void **ppv);
LPCITEMIDLIST FSFindJunctionNext(LPCIDFOLDER pidf);

TCHAR g_szFolderTypeName[32] = TEXT("");    // "Folder" 
TCHAR g_szFileTypeName[32] = TEXT("");      // "File"
TCHAR g_szFileTemplate[32] = TEXT("");      // "ext File"

void DestroyColHandlers(HDSA *phdsa);

HRESULT GetToolTipForItem(CFSFolder *this, LPCIDFOLDER pidf, REFIID riid, void **ppv);

#define MAX_CLASS   80

enum
{
    FS_ICOL_NAME = 0,
    FS_ICOL_SIZE,
    FS_ICOL_TYPE,
    FS_ICOL_MODIFIED,
    FS_ICOL_ATTRIB,
};

const COL_DATA c_fs_cols[] = {
    {FS_ICOL_NAME,      IDS_NAME_COL,       20, LVCFMT_LEFT,    &SCID_NAME},
    {FS_ICOL_SIZE,      IDS_SIZE_COL,       16, LVCFMT_RIGHT,   &SCID_SIZE},
    {FS_ICOL_TYPE,      IDS_TYPE_COL,       20, LVCFMT_LEFT,    &SCID_TYPE},
    {FS_ICOL_MODIFIED,  IDS_MODIFIED_COL,   20, LVCFMT_LEFT,    &SCID_WRITETIME},
    {FS_ICOL_ATTRIB,    IDS_ATTRIB_COL,     10, LVCFMT_LEFT,    &SCID_ATTRIBUTES}
};

//
// List of file attribute bit values.  The order (with respect
// to meaning) must match that of the characters in g_szAttributeChars[].
//
const DWORD g_adwAttributeBits[] =
{
    FILE_ATTRIBUTE_READONLY,
    FILE_ATTRIBUTE_HIDDEN,
    FILE_ATTRIBUTE_SYSTEM,
    FILE_ATTRIBUTE_ARCHIVE,
    FILE_ATTRIBUTE_COMPRESSED,
    FILE_ATTRIBUTE_ENCRYPTED,
    FILE_ATTRIBUTE_OFFLINE
};

//
// Buffer for characters that represent attributes in Details View attributes
// column.  Must provide room for 1 character for each bit a NUL.  The current 5
// represent Read-only, Archive, Compressed, Hidden and System in that order.
// This can't be const because we overwrite it using LoadString.
//
TCHAR g_szAttributeChars[ARRAYSIZE(g_adwAttributeBits) + 1] = { 0 } ;

#define FS_GetType(_pidf)       ((_pidf)->bFlags & SHID_FS_TYPEMASK)

// file system, non junction point folder
#define FS_IsJunction(_pidf)    ((_pidf)->bFlags & SHID_JUNCTION)

// old NT4 style simple pidls used SHID_FS, this tests for those
#define FS_IsNT4StyleSimpleID(_pidf)   (((_pidf)->bFlags & SHID_FS_TYPEMASK) == SHID_FS)

#define FS_Combine(_pidl, _pidf2) \
                                ILCombine(_pidl, (LPITEMIDLIST)(_pidf2))
#define FS_FindLastID(pidf)     ((LPIDFOLDER)ILFindLastID((LPITEMIDLIST)pidf))
#define FS_Clone(pidf)          ((LPIDFOLDER)ILClone((LPITEMIDLIST)pidf))
#define FS_Free(pidf)           ILFree((LPITEMIDLIST)pidf)

#define FS_Next(_pidf)          ((LPIDFOLDER)_ILNext((LPITEMIDLIST)_pidf))
#define FS_IsEmpty(_pidf)       ILIsEmpty((LPITEMIDLIST)_pidf)

#if (defined(DBCS) || (defined(FE_SB) && !defined(UNICODE)))
// We don't want to take capital roman characters and small roman characters
// in DBCS as the same because our file system doesn't.
#ifdef lstrcmpi
#undef lstrcmpi
#endif

#define lstrcmpi(lpsz1, lpsz2)  lstrcmpiNoDBCS(lpsz1, lpsz2)
#endif



// order here is important, first one found will terminate the search
const int c_csidlSpecial[] = {
    CSIDL_STARTMENU | TEST_SUBFOLDER,
    CSIDL_COMMON_STARTMENU | TEST_SUBFOLDER,
    CSIDL_RECENT,
    CSIDL_WINDOWS,
    CSIDL_SYSTEM,
    CSIDL_PERSONAL,
};

BOOL CFSFolder_IsCSIDL(CFSFolder *this, UINT csidl)
{
    BOOL bRet = (this->_csidl == csidl);
    if (!bRet)
    {
        TCHAR szThisFolder[MAX_PATH];

        CFSFolder_GetPath(this, szThisFolder);
        bRet = PathIsEqualOrSubFolder(MAKEINTRESOURCE(csidl), szThisFolder);
        if (bRet)
            this->_csidl = csidl;
    }
    return bRet;
}

UINT CFSFolder_GetCSIDL(CFSFolder *this)
{
    // Cache the special folder ID, if it is not cached yet.
    if (this->_csidl == -1)
    {
        TCHAR szThisFolder[MAX_PATH];

        CFSFolder_GetPath(this, szThisFolder);

        // Always cache the real Csidl.
        this->_csidl = GetSpecialFolderID(szThisFolder, c_csidlSpecial, ARRAYSIZE(c_csidlSpecial));

        if (this->_csidl == -1)
        {
            this->_csidl = CSIDL_NORMAL;   // default
        }
    }

    return this->_csidl;
}

HRESULT _AppendItemToPath(LPTSTR pszPath, LPCIDFOLDER pidf);

STDAPI_(LPCIDFOLDER) FS_IsValidID(LPCITEMIDLIST pidl)
{
    if (pidl && pidl->mkid.cb && (((LPCIDFOLDER)pidl)->bFlags & SHID_GROUPMASK) == SHID_FS)
        return (LPCIDFOLDER)pidl;
    return NULL;
}

STDAPI_(BOOL) FS_IsCommonItem(LPCITEMIDLIST pidl)
{
    if (pidl && pidl->mkid.cb && (((LPCIDFOLDER)pidl)->bFlags & (SHID_GROUPMASK | SHID_FS_COMMONITEM)) == SHID_FS_COMMONITEM)
        return TRUE;
    return FALSE;
}

STDAPI_(BOOL) FS_MakeCommonItem(LPITEMIDLIST pidl)
{
    LPIDFOLDER pidf = (LPIDFOLDER)FS_IsValidID(pidl);
    if (pidf)
    {
        pidf->bFlags |= SHID_FS_COMMONITEM;
    }
    return pidf ? TRUE : FALSE;
}


STDAPI_(BOOL) FS_IsFile(LPCIDFOLDER pidf)
{
    return FS_GetType(pidf) == SHID_FS_FILE || FS_GetType(pidf) == SHID_FS_FILEUNICODE;
}

STDAPI_(BOOL) FS_IsFolder(LPCIDFOLDER pidf)
{
    return FS_GetType(pidf) == SHID_FS_DIRECTORY || FS_GetType(pidf) == SHID_FS_DIRUNICODE;
}

STDAPI_(BOOL) FS_IsFileFolder(LPCIDFOLDER pidf)
{
    return pidf->bFlags == SHID_FS_DIRECTORY || pidf->bFlags == SHID_FS_DIRUNICODE;
}

BOOL FS_IsSystemFolder(LPCIDFOLDER pidf)
{
    return FS_IsFileFolder(pidf) && (pidf->fs.wAttrs & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY));
}

STDAPI_(DWORD) FS_GetUID(LPCIDFOLDER pidf)
{
    return pidf->fs.dwSize + ((DWORD)pidf->fs.dateModified << 8) + ((DWORD)pidf->fs.timeModified << 12);
}

void FS_GetSize(LPCITEMIDLIST pidlParent, LPCIDFOLDER pidf, ULONGLONG *pcbSize)
{
    ULONGLONG cbSize = pidf->fs.dwSize;
    if (cbSize != 0xFFFFFFFF)
        *pcbSize = cbSize;
    else if (pidlParent == NULL)
        *pcbSize = 0;
    else
    {
        HANDLE hfind;
        ULARGE_INTEGER uli;
        WIN32_FIND_DATA wfd;
        TCHAR szPath[MAX_PATH];

        // Get the real size by asking the file system
        SHGetPathFromIDList(pidlParent, szPath);
        _AppendItemToPath(szPath, pidf);

        // BUGBUG: We should supply a punkEnableModless in order to go modal during UI.
        if (SHFindFirstFileRetry(NULL, NULL, szPath, &wfd, &hfind, SHPPFW_NONE) != S_OK)
            *pcbSize = 0;
        else
        {
            FindClose(hfind);

            uli.LowPart = wfd.nFileSizeLow;
            uli.HighPart = wfd.nFileSizeHigh;

            *pcbSize = uli.QuadPart;
        }
    }
}

LPWSTR FS_CopyNameW(LPCIDFOLDER pidf, LPWSTR pszName, UINT cchName)
{
    VDATEINPUTBUF(pszName, WCHAR, cchName);

    *pszName = NULL;

    //  unicode id?
    if ((FS_GetType(pidf) & SHID_FS_UNICODE) == SHID_FS_UNICODE)
    {
        ualstrcpynW(pszName, ((LPCIDFOLDERW)pidf)->fs.cFileName, cchName);
        return pszName;
    }

    //  std ANSI id.
    MultiByteToWideChar(CP_ACP, 0, ((LPIDFOLDERA)pidf)->fs.cFileName, -1, pszName, cchName);
    return pszName;
}

LPTSTR FS_CopyName(LPCIDFOLDER pidf, LPTSTR pszName, UINT cchName)
{
    VDATEINPUTBUF(pszName, TCHAR, cchName);

#ifdef UNICODE
    return FS_CopyNameW( pidf, pszName, cchName ) ;
#else
    if ((FS_GetType(pidf) & SHID_FS_UNICODE) == SHID_FS_UNICODE)
    {
        ASSERT(0);  // I don't think this works, as this buffer is packed
        return lstrcpyn(pszName, pidf->fs.cAltFileName, cchName);
    }
    else
    {
        return lstrcpyn(pszName, pidf->fs.cFileName, cchName);
    }
#endif

}

//
//  FS_GetAltName
//
//  Returns a pointer to the alt name for the pidf.  Warning!  This
//  pointer points directly into the pidf, so you'd better copy it
//  out if you plan on using it for anything important.
//
//  Note that the alt name is always ANSI.
//

LPCSTR FS_GetAltName(LPCIDFOLDER pidf)
{
    UINT cbName;

    if ((FS_GetType(pidf) & SHID_FS_UNICODE) == SHID_FS_UNICODE)
    {
        cbName = (ualstrlenW(((LPIDFOLDERW)pidf)->fs.cFileName) + 1) * SIZEOF(TCHAR);
    }
    else
    {
        cbName = (lstrlenA(((LPIDFOLDERA)pidf)->fs.cFileName) + 1);
    }

    return ((LPIDFOLDERA)pidf)->fs.cFileName + cbName;
}

#define FS_HasAltName(pidf)     FS_GetAltName(pidf)[0]

LPTSTR FS_CopyAltName(LPCIDFOLDER pidf, LPTSTR pszName, UINT cchName)
{
    VDATEINPUTBUF(pszName, TCHAR, cchName);

    SHAnsiToTChar(FS_GetAltName(pidf), pszName, cchName);
    return pszName;
}

BOOL FS_ShowExtension(LPCIDFOLDER pidf)
{
    SHELLSTATE ss;
    DWORD dwFlags = SHGetClassFlags(pidf);

    if (dwFlags & SHCF_NEVER_SHOW_EXT)
        return FALSE;

    SHGetSetSettings(&ss, SSF_SHOWEXTENSIONS, FALSE);
    if (ss.fShowExtensions)
        return TRUE;

    return dwFlags & (SHCF_ALWAYS_SHOW_EXT | SHCF_UNKNOWN);
}

//
// get the type name from the registry, if the name is blank make
// up a default.
//
//      directory       ==> "Folder"
//      foo             ==> "File"
//      foo.xyz         ==> "XYZ File"
//
void SHGetTypeName(LPCTSTR pszFile, HKEY hkey, BOOL fFolder, LPTSTR pszName, int cchNameMax)
{
    ULONG cb = cchNameMax * SIZEOF(TCHAR);
    VDATEINPUTBUF(pszName, TCHAR, cchNameMax);

    if (SHRegQueryValue(hkey, NULL, pszName, &cb) != ERROR_SUCCESS || pszName[0] == 0)
    {
        if (fFolder)
        {
            // NOTE the registry doesn't have a name for Folder
            // because old apps would think it was a file type.
            lstrcpy(pszName, g_szFolderTypeName);
        }
        else
        {
            LPTSTR pszExt = PathFindExtension(pszFile);

            if (*pszExt == 0)
            {
                // Probably don't need the cchmax here, but...
                lstrcpyn(pszName, g_szFileTypeName, cchNameMax);
            }
            else
            {
                TCHAR szExt[_MAX_EXT];
                int cchMaxExtCopy = min((cchNameMax - lstrlen(g_szFileTemplate)), ARRAYSIZE(szExt));

                // Compose '<ext> File' (or what ever the template defines we do)

                lstrcpyn(szExt, pszExt + 1, cchMaxExtCopy);
                CharUpperNoDBCS(szExt);
                wsprintf(pszName, g_szFileTemplate, szExt);
            }
        }
    }
}

//
// return a pointer to the type name for the given PIDL
// the pointer is only valid while in a critical section
//
LPCTSTR _GetTypeName(LPCIDFOLDER pidf)
{
    TCHAR szClass[MAX_PATH];
    LPCTSTR pszClassName;

    ASSERTCRITICAL

    SHGetClass(pidf, szClass, ARRAYSIZE(szClass));

    pszClassName = LookupFileClassName(szClass);
    if (pszClassName == NULL)
    {
        HKEY hkey;
        TCHAR ach[MAX_CLASS], szTmp[MAX_PATH];

        FSGetClassKey(pidf, &hkey);

        SHGetTypeName(FS_CopyName(pidf, szTmp, ARRAYSIZE(szTmp)), hkey, FS_IsFolder(pidf), ach, ARRAYSIZE(ach));

        SHCloseClassKey(hkey);

        pszClassName = AddFileClassName(szClass, ach);
    }

    return pszClassName;
}

//
// return the type name for the given PIDL
//
void FS_GetTypeName(LPCIDFOLDER pidf, LPTSTR pszName, int cchNameMax)
{
    VDATEINPUTBUF(pszName, TCHAR, cchNameMax);

    ENTERCRITICAL;
    lstrcpyn(pszName, _GetTypeName(pidf), cchNameMax);
    LEAVECRITICAL;
}

//
// Build a text string containing characters that represent attributes of a file.
// The attribute characters are assigned as follows:
// (R)eadonly, (H)idden, (S)ystem, (A)rchive, (H)idden.
//
void BuildAttributeString(DWORD dwAttributes, LPTSTR pszString, UINT nChars)
{
    int i;

    // Make sure we have attribute chars to build this string out of
    if (!g_szAttributeChars[0])
        LoadString(HINST_THISDLL, IDS_ATTRIB_CHARS, g_szAttributeChars, ARRAYSIZE(g_szAttributeChars));

    // Make sure buffer is big enough to hold worst-case attributes
    ASSERT(nChars >= ARRAYSIZE(g_adwAttributeBits) + 1);

    for (i = 0; i < ARRAYSIZE(g_adwAttributeBits); i++)
    {
        if (dwAttributes & g_adwAttributeBits[i])
            *pszString++ = g_szAttributeChars[i];
    }
    *pszString = 0;     // null terminate

}


int g_iUseLinkPrefix = -1;

#define INITIALLINKPREFIXCOUNT 20
#define MAXLINKPREFIXCOUNT  30

void LoadUseLinkPrefixCount()
{
    TraceMsg(TF_FSTREE, "LoadUseLinkPrefixCount %d", g_iUseLinkPrefix);
    if (g_iUseLinkPrefix < 0)
    {
        HKEY hkey;
        g_iUseLinkPrefix = INITIALLINKPREFIXCOUNT;  // the default

        hkey = SHGetExplorerHkey(HKEY_CURRENT_USER, TRUE);
        if (hkey)
        {
            int iUseLinkPrefix;
            DWORD dwSize = SIZEOF(iUseLinkPrefix);

            // read in the registry value
            if ((SHQueryValueEx(hkey, c_szLink, NULL, NULL, (BYTE *)&iUseLinkPrefix, &dwSize) == ERROR_SUCCESS)
                && iUseLinkPrefix >= 0)
            {
                g_iUseLinkPrefix = iUseLinkPrefix;
            }
            RegCloseKey( hkey);
        }
    }
}

void SaveUseLinkPrefixCount()
{
    if (g_iUseLinkPrefix >= 0)
    {
        HKEY hkey = SHGetExplorerHkey(HKEY_CURRENT_USER, TRUE);
        if (hkey) {
            RegSetValueEx(hkey, c_szLink, 0, REG_BINARY, (BYTE *)&g_iUseLinkPrefix, SIZEOF(g_iUseLinkPrefix));
            RegCloseKey(hkey);
        }   
    }
}

#define ISDIGIT(c)  ((c) >= TEXT('0') && (c) <= TEXT('9'))

// lpsz2 = destination
// lpsz1 = source
void StripNumber(LPTSTR lpsz2, LPCTSTR lpsz1)
{
    // strip out the '(' and the numbers after it
    // We need to verify that it is either simply () or (999) but not (A)
    for (; *lpsz1; lpsz1 = CharNext(lpsz1), lpsz2 = CharNext(lpsz2)) {
        if (*lpsz1 == TEXT('(')) {
            LPCTSTR lpszT = lpsz1;
            do {
                lpsz1 = CharNext(lpsz1);
            } while (*lpsz1 && ISDIGIT(*lpsz1));

            if (*lpsz1 == TEXT(')'))
            {
                lpsz1 = CharNext(lpsz1);
                if (*lpsz1 == TEXT(' '))
                    lpsz1 = CharNext(lpsz1);  // skip the extra space
                lstrcpy(lpsz2, lpsz1);
                return;
            }

            // We have a ( that does not match the format correctly!
            lpsz1 = lpszT;  // restore pointer back to copy this char through and continue...
        }
        *lpsz2 = *lpsz1;
    }
    *lpsz2 = *lpsz1;
}

#define SHORTCUT_PREFIX_DECR 5
#define SHORTCUT_PREFIX_INCR 1

// this checks to see if you've renamed 'Shortcut #x To Foo' to 'Foo'

void CheckShortcutRename(LPCTSTR lpszOldPath, LPCTSTR lpszNewPath)
{
    LPCTSTR lpszOldName = PathFindFileName(lpszOldPath);
    LPCTSTR lpszNewName = PathFindFileName(lpszNewPath);

    // already at 0.
    if (!g_iUseLinkPrefix)
        return;

    if (PathIsLnk(lpszOldName)) {
        TCHAR szBaseName[MAX_PATH];
        TCHAR szLinkTo[80];
        TCHAR szMockName[MAX_PATH];


        lstrcpy(szBaseName, lpszNewName);
        PathRemoveExtension(szBaseName);


        // mock up a name using the basename and the linkto template
        LoadString(HINST_THISDLL, IDS_LINKTO, szLinkTo, ARRAYSIZE(szLinkTo));
        wnsprintf(szMockName, ARRAYSIZE(szMockName), szLinkTo, szBaseName);

        StripNumber(szMockName, szMockName);
        StripNumber(szBaseName, lpszOldName);

        // are the remaining gunk the same?
        if (!lstrcmp(szMockName, szBaseName)) {

            // yes!  do the link count magic
            LoadUseLinkPrefixCount();
            ASSERT(g_iUseLinkPrefix >= 0);
            g_iUseLinkPrefix -= SHORTCUT_PREFIX_DECR;
            if (g_iUseLinkPrefix < 0)
                g_iUseLinkPrefix = 0;
            SaveUseLinkPrefixCount();
        }
    }
}

STDAPI_(LRESULT) SHRenameFileEx(HWND hwnd, IUnknown * punkEnableModless, LPCTSTR pszDir, LPCTSTR pszOldName, LPCTSTR pszNewName,
                            BOOL bRetainExtension)
{
    TCHAR szOldPathName[MAX_PATH+1];    // +1 for double nul terminating
    TCHAR szNewPathName[MAX_PATH+1];    // +1 for double nul terminating
    TCHAR szTempNewPath[MAX_PATH+1];    // +1 for double nul terminating
    int iret = 0;
    LPTSTR pszExt;
    int    iRet;

    // do a binary compare, locale insenstive compare to avoid mappings of
    // single chars into multiple and the reverse. specifically german
    // sharp-S and "ss"

    if (StrCmpC(pszOldName, pszNewName) == 0)
        return -1;         // Not zero so to not to update item...

    PathCombine(szOldPathName, pszDir, pszOldName);
    szOldPathName[lstrlen(szOldPathName) + 1] = TEXT('\0');

    lstrcpy(szTempNewPath, pszNewName);
    if (iRet = PathCleanupSpec(pszDir, szTempNewPath))
    {
        IUnknown_EnableModless(punkEnableModless, FALSE);
        ShellMessageBox(HINST_THISDLL, hwnd,
                iRet & PCS_PATHTOOLONG ?
                    MAKEINTRESOURCE(IDS_REASONS_INVFILES) :
                    IsLFNDrive(pszDir)?
                        MAKEINTRESOURCE(IDS_INVALIDFN) :
                        MAKEINTRESOURCE(IDS_INVALIDFNFAT),
                MAKEINTRESOURCE(IDS_RENAME), MB_OK | MB_ICONHAND);
        IUnknown_EnableModless(punkEnableModless, TRUE);
        return ERROR_CANCELLED; // user saw the error, don't report again
    }

    // We want to strip off leading and trailing blanks off of the new
    // file name.
    lstrcpy(szTempNewPath, pszNewName);
    PathRemoveBlanks(szTempNewPath);
    if ( !szTempNewPath[0] || (szTempNewPath[0] == TEXT('.')) )
    {
        IUnknown_EnableModless(punkEnableModless, FALSE);
        ShellMessageBox(HINST_THISDLL, hwnd,
                        MAKEINTRESOURCE(IDS_NONULLNAME),
                        MAKEINTRESOURCE(IDS_RENAME), MB_OK | MB_ICONHAND);
        IUnknown_EnableModless(punkEnableModless, TRUE);
        return ERROR_CANCELLED; // user saw the error, don't report again
    }
    PathCombine(szNewPathName, pszDir, szTempNewPath);

    // if there was an old extension and the new and old don't match complain
    pszExt = PathFindExtension(pszOldName);
    if (*pszExt &&
        lstrcmpi(pszExt, PathFindExtension(szTempNewPath)))
    {
        TCHAR szTemp[MAX_PATH];
        if (!PathIsDirectory(szOldPathName) &&
            GetClassDescription(HKEY_CLASSES_ROOT, pszExt, szTemp, ARRAYSIZE(szTemp), GCD_ALLOWPSUDEOCLASSES | GCD_MUSTHAVEOPENCMD))
        {
            int nResult;

            IUnknown_EnableModless(punkEnableModless, FALSE);
            nResult = ShellMessageBox(HINST_THISDLL, hwnd,
                                MAKEINTRESOURCE(IDS_WARNCHANGEEXT),
                                MAKEINTRESOURCE(IDS_RENAME), MB_YESNO | MB_ICONEXCLAMATION);
            IUnknown_EnableModless(punkEnableModless, TRUE);

            if (nResult != IDYES)
                return ERROR_CANCELLED; // user saw the error, don't report again
        }
    }


    // BUGBUG: we need UI to warn if they are trying to change extension
    if (bRetainExtension)
    {
        // Retain the extension from the old name.
        //  If the user wanted a different extension, tough.  Play
        //  a little violin for them, then get on with life...
        //
        PathRenameExtension(szNewPathName, PathFindExtension(szOldPathName));
    }

    szNewPathName[lstrlen(szNewPathName) + 1] = 0;     // double NULL terminate

    {
        SHFILEOPSTRUCT sFileOp =
        {
            hwnd,
            FO_RENAME,
            szOldPathName,
            szNewPathName,
            FOF_SILENT | FOF_ALLOWUNDO,
        };

        IUnknown_EnableModless(punkEnableModless, TRUE);
        iret = SHFileOperation(&sFileOp);
        IUnknown_EnableModless(punkEnableModless, FALSE);
    }

    if (!iret)
        CheckShortcutRename(szOldPathName, szNewPathName);

    return iret;
}


STDAPI_(LRESULT) SHRenameFile(HWND hwnd, LPCTSTR pszDir, LPCTSTR pszOldName, LPCTSTR pszNewName,
                            BOOL bRetainExtension)
{
    return SHRenameFileEx(hwnd, NULL, pszDir, pszOldName, pszNewName, bRetainExtension);
}



extern const IUnknownVtbl c_FSFolderUnkVtbl;
extern IShellFolder2Vtbl c_FSFolderVtbl;
extern IPersistFolder3Vtbl c_CFSFolderPFVtbl;
extern IShellIconVtbl c_FSFolderIconVtbl;
extern IShellIconOverlayVtbl c_FSFolderIconOverlayVtbl;



// BUGBUG: BryanSt: This doesn't work with FRAGMENTs.  We should return the path
// without the Fragment for backward compatibility and then callers that care,
// can later determine that and take care of it.
//

// in/out:
//      pszPath path to append pidf names to
// in:
//      pidf        relative pidl fragment

HRESULT _AppendItemToPath(LPTSTR pszPath, LPCIDFOLDER pidf)
{
    HRESULT hr = S_OK;
    LPTSTR pszPathCur = pszPath + lstrlen(pszPath);

    // BUGBUG: we want to do this, but we stil have broken code in SHGetPathFromIDList
    // ASSERT(FSFindJunctionNext(pidf) == NULL);     // no extra goo please

    for (; SUCCEEDED(hr) && !FS_IsEmpty(pidf); pidf = FS_Next(pidf))
    {
        TCHAR szName[MAX_PATH];
        int cchName;    // store the length of szName, to avoid calculating it twice

        FS_CopyName(pidf, szName, ARRAYSIZE(szName));
        cchName = lstrlen(szName);

        // ASSERT(lstrlen(pszPath)+lstrlen(szName)+2 <= MAX_PATH);
        if (((pszPathCur - pszPath ) + cchName + 2) > MAX_PATH)
        {
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW); // FormatMessage = "The file name is too long"
            break;
        }

        if (*(pszPathCur-1) != TEXT('\\'))
            *(pszPathCur++) = TEXT('\\');

        // don't need lstrncpy cause we verified size above
        lstrcpy(pszPathCur, szName);

        pszPathCur += cchName;
    }

    if (FAILED(hr))
        *pszPath = 0;

    return hr;
}

// get the file system folder path for this
//
// HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) is returned if we are a tracking
// folder that does not (yet) have a valid target.


HRESULT CFSFolder_GetPath(CFSFolder *this, LPTSTR pszPath)
{
    if (this->_csidlTrack >= 0)
    {
        if (SHGetFolderPath(NULL, this->_csidlTrack | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, pszPath) == S_FALSE)
            return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
    }
    else if (this->_pszPath)
    {
        lstrcpyn(pszPath, this->_pszPath, MAX_PATH);
    }
    else
    {
        if ( this->_pidlTarget &&  
                SUCCEEDED(SHGetNameAndFlags(this->_pidlTarget, SHGDN_FORPARSING, pszPath, MAX_PATH, NULL)) )
        {
            this->_pszPath = StrDup(pszPath);
        }
        else if (SUCCEEDED(SHGetNameAndFlags(this->_pidl, SHGDN_FORPARSING, pszPath, MAX_PATH, NULL)))
        {
            this->_pszPath = StrDup(pszPath);
        }
    }

    return *pszPath ? S_OK : E_FAIL;
}

#ifdef WINNT
// Will fail (return FALSE) if not a mount point
STDAPI_(BOOL) FS_GetMountingPointInfo(CFSFolder* this, LPCIDFOLDER pidf,
                                      LPTSTR pszMountPoint, DWORD cchMountPoint)
{
    BOOL bRet = FALSE;

    // Is this a reparse point?
    if (FILE_ATTRIBUTE_REPARSE_POINT & pidf->fs.wAttrs)
    {
        // Yes
        TCHAR szLocalMountPoint[MAX_PATH];

        if (SUCCEEDED(CFSFolder_GetPathForItemW(this, pidf, szLocalMountPoint)))
        {
            TCHAR szVolumeName[50]; //50 according to doc
            PathAddBackslash(szLocalMountPoint);

            // Check if it is a mounting point
            if (GetVolumeNameForVolumeMountPoint(szLocalMountPoint, szVolumeName,
                ARRAYSIZE(szVolumeName)))
            {
                // Yes
                bRet = TRUE;

                if (pszMountPoint && cchMountPoint)
                    lstrcpyn(pszMountPoint, szLocalMountPoint, cchMountPoint);
            }
        }
    }
    return bRet;
}
#else

#define FS_GetMountingPointInfo(t, pidf, pszMountPoint, cchMountPoint) (FALSE)

#endif //WINNT

// in:
//      pidf    may be NULL, item to append to path for this folder
// out:
//      pszPath MAX_PATH buffer to receive the fully qualified file path for the item
//


STDAPI CFSFolder_GetPathForItem(CFSFolder *this, LPCIDFOLDER pidf, LPTSTR pszPath)
{
    if (SUCCEEDED(CFSFolder_GetPath(this, pszPath)))
    {
        if (pidf)
            return _AppendItemToPath(pszPath, pidf);
        return S_OK;
    }
    return E_FAIL;
}

STDAPI CFSFolder_GetPathForItemW(CFSFolder *this, LPCIDFOLDER pidf, LPWSTR pszPath)
{
    HRESULT hr;
#ifdef UNICODE
    hr = CFSFolder_GetPathForItem(this, pidf, pszPath);
#else
    TCHAR szPath[MAX_PATH];

    *pszPath = 0;
    hr = CFSFolder_GetPathForItem(this, pidf, szPath);
    if (SUCCEEDED(hr))
    {
        SHTCharToUnicode(szPath, pszPath, MAX_PATH);
        hr = S_OK;
    }
#endif
    return hr;
}

//
// This function retrieves the private profile strings from the desktop.ini file and
// return it through pszOut
//
// This function uses SHGetIniStringUTF7 to get the string, so it is valid
// to use SZ_CANBEUNICODE on the key name.

BOOL _GetFolderString(LPCTSTR pszFolder, LPCTSTR pszProvider, LPTSTR pszOut, int cch, LPCTSTR pszKey)
{
    BOOL fExists, fRet = FALSE;
    TCHAR szPath[MAX_PATH];

    PathCombine(szPath, pszFolder, c_szDesktopIni);

    // CHECK for PathFileExists BEFORE calling to GetPrivateProfileString
    // because if the file isn't there (which is the majority of cases)
    // GetPrivateProfileString hits the disk twice looking for the file

    if (pszProvider && *pszProvider)
    {
        union {
            NETRESOURCE nr;
            TCHAR buf[512];
        } nrb;
        LPTSTR lpSystem;
        DWORD dwRes, dwSize = SIZEOF(nrb);

        nrb.nr.dwType = RESOURCETYPE_ANY;
        nrb.nr.lpRemoteName = szPath;
        nrb.nr.lpProvider = (LPTSTR)pszProvider;    // const -> non const
        dwRes = WNetGetResourceInformation(&nrb.nr, &nrb, &dwSize, &lpSystem);

        fExists = (dwRes == WN_SUCCESS) || (dwRes == WN_MORE_DATA);
    }
    else
    {
        fExists = PathFileExists(szPath);
    }

    if (fExists)
    {
        TCHAR szTemp[INFOTIPSIZE];
        fRet = SHGetIniStringUTF7(c_szClassInfo, pszKey, szTemp, ARRAYSIZE(szTemp), szPath);
        if (fRet)
        {
            SHExpandEnvironmentStrings(szTemp, pszOut, cch);   // This could be a path, so expand the env vars in it
        }
    }
    return fRet;
}

//
// This function retrieves the CLSID from desktop.ini file.
//
BOOL _GetFolderCLSID(LPCTSTR pszFolder, LPCTSTR pszProvider, CLSID* pclsid, LPCTSTR pszKey)
{
    TCHAR szCLSID[40];
    if (_GetFolderString(pszFolder, pszProvider, szCLSID, ARRAYSIZE(szCLSID), pszKey))
    {
        return SUCCEEDED(SHCLSIDFromString(szCLSID, pclsid));
    }
    return FALSE;
}

LPTSTR PathFindCLSIDExtension(LPCTSTR pszFile, CLSID *pclsid)
{
    LPCTSTR pszExt = PathFindExtension(pszFile);
    if (*pszExt == TEXT('.') && *(pszExt + 1) == CH_GUIDFIRST)
    {
        CLSID clsid;

        if (pclsid == NULL)
            pclsid = &clsid;

        if (SUCCEEDED(SHCLSIDFromString(pszExt + 1, pclsid)))
            return (LPTSTR)pszExt;      // const -> non const
    }
    return NULL;
}

//
// This function retrieves the CLSID from a filename
// file.{GUID}
//
BOOL _GetFileCLSID(LPCTSTR pszFile, CLSID* pclsid)
{
    return PathFindCLSIDExtension(pszFile, pclsid) != NULL;
}

//
//  In:     pidf to examine
//  Out:    pclsid filled with the CLSID
//  Ret:    TRUE on success
//
BOOL FSGetCLSIDFromPidf(LPCIDFOLDER pidf, CLSID *pclsid)
{
    HKEY hkey;
    BOOL fRet;
    if (SUCCEEDED(FSGetClassKey(pidf, &hkey)))
    {
        TCHAR szCLSID[MAX_CLASS];
        DWORD cb = SIZEOF(szCLSID);

        fRet = (SHRegQueryValue(hkey, c_szCLSID, szCLSID, &cb) == ERROR_SUCCESS) &&
               SUCCEEDED(SHCLSIDFromString(szCLSID, pclsid));
        SHCloseClassKey(hkey);
    }
    else
        fRet = FALSE;
    return fRet;
}


LPIDFOLDER CFSFolder_TryAppendJunctionID(CFSFolder *this, LPIDFOLDER pidf, LPCTSTR pszName)
{
    CLSID clsid;
    //
    // check for a junction point, junctions are either
    //
    //  Folder.{guid} or File.{guid} both fall into this case
    //
    if (_GetFileCLSID(pszName, &clsid))
    {
        pidf->bFlags |= SHID_JUNCTION;
    }
    //
    // look for the desktop.ini in a folder, those are canidates for junction points
    //
    else if (FS_IsSystemFolder(pidf))
    {
        TCHAR szPath[MAX_PATH];
        if (SUCCEEDED(CFSFolder_GetPathForItem(this, pidf, szPath)))
        {
            // CLSID2 makes folders work on Win95 if the CLSID does not exist on the machine
            if (_GetFolderCLSID(szPath, this->_pszNetProvider, &clsid, TEXT("CLSID2"))
            || _GetFolderCLSID(szPath, this->_pszNetProvider, &clsid, c_szCLSID))
            {
                pidf->bFlags |= SHID_JUNCTION;
            }
        }
    }
    //
    // File.ext where ext corresponds to a shell extension (such as .cab)
    //
    else if (SHCF_IS_SHELLEXT & SHGetClassFlags(pidf))
    {
        if (FSGetCLSIDFromPidf(pidf, &clsid))
            pidf->bFlags |= SHID_JUNCTION;
    }

    if (FS_IsJunction(pidf))
        pidf = (LPIDFOLDER) ILAppendHiddenClsid((LPITEMIDLIST)pidf, IDLHID_JUNCTION, &clsid);

    return pidf;
}

BOOL FS_GetCLSID(LPCIDFOLDER pidf, CLSID *pclsidRet)
{
    //  
    //  if this is a junction point that was created on NT5
    //  then it should be stored with IDLHID_JUNCTION
    //
    if (FS_IsJunction(pidf) 
    &&  ILGetHiddenClsid((LPCITEMIDLIST)pidf, IDLHID_JUNCTION, pclsidRet))
    {
        return TRUE;
    }
    
    //
    //  else it might be an oldstyle JUNCTION point that
    //  was persisted out.  or a ROOT_REGITEM
    //
    if (FS_IsJunction(pidf) || (SIL_GetType((LPITEMIDLIST)pidf) == SHID_ROOT_REGITEM))
    {
        const UNALIGNED CLSID * pclsid = (UNALIGNED CLSID *)(((BYTE *)pidf) + pidf->cb - SIZEOF(CLSID));
        *pclsidRet = *pclsid;
        return TRUE;
    }
    
    *pclsidRet = CLSID_NULL;
    return FALSE;
}

#ifdef FEATURE_LOCALIZED_FOLDERS
LPIDFOLDER CFSFolder_TryAppendLocalizedNameID(CFSFolder *this, LPIDFOLDER pidf)
{
    if (FS_IsSystemFolder(pidf))
    {
        TCHAR szPath[MAX_PATH];
        TCHAR szName[MAX_PATH];
        
        if (SUCCEEDED(CFSFolder_GetPathForItem(this, pidf, szPath))
        &&  _GetFolderString(szPath, this->_pszNetProvider, szName, SIZECHARS(szName), SZ_CANBEUNICODE TEXT("LocalizedResourceName"))
        && *szName && StrChrW(szName, TEXT(',')))
        {
            //  we have a winner!

            pidf = (LPIDFOLDER) ILAppendHiddenString((LPITEMIDLIST)pidf, IDLHID_LOCALIZEDNAME, szName);
        }

    }

    return pidf;
}
#endif // FEATURE_LOCALIZED_FOLDERS


//
//  returns a unique name for a class, dont use this function to get
//  the ProgID for a class call SHGetClassKey() for that
//
// Returns: class name in pszClass
//
//  foo.ext             ".ext"
//  foo                 "."
//  (empty)             "Folder"
//  directory           "Directory"
//  junction            "CLSID\{clsid}"
//

BOOL SHGetClass(LPCIDFOLDER pidf, LPTSTR pszClass, UINT cch)
{
    if (pidf->cb == 0)      // ILIsEmpty()
    {
        // the desktop. Always use the "Folder" class.
        lstrcpyn(pszClass, c_szFolderClass, cch);
    }
    else
    {
        UINT uType = pidf->bFlags;
        CLSID clsid;
        //
        // BUGBUG: git rid of the old FS type flags we dont use any more
        // Do not include SHID_FS_COMMONITEM in this list.
        //
        if ((pidf->bFlags & SHID_GROUPMASK) == SHID_FS)
        {
            uType &= SHID_FS_DIRECTORY | SHID_FS_FILE | SHID_FS_UNICODE | SHID_JUNCTION;
        }
        else if (uType == (SHID_NET_SHARE | SHID_JUNCTION))
        {
            //
            // If this is a network share point, we should treat it as
            // a standard folder.
            //
            // BUGBUG - BobDay what about net shares whose name is unicode?
            uType = SHID_FS_DIRECTORY;
        }

        if (FS_GetCLSID(pidf, &clsid))
        {
            // This is a junction point, get the CLSID from it.

            // Put the class ID at the end of "CLSID\\"
            lstrcpyn(pszClass, c_szCLSIDSlash, cch);
            SHStringFromGUID(&clsid, pszClass + 6, cch - ARRAYSIZE(c_szCLSIDSlash));
        }
        else if ((uType == SHID_FS_FILE) || (uType == SHID_FS) ||
                 (uType == SHID_FS_FILEUNICODE) || (uType == SHID_FS_UNICODE))
        {
            // This is a file. Get the class based on the extension.
            TCHAR szName[MAX_PATH];
            LPCTSTR pszExt = PathFindExtension(FS_CopyName(pidf, szName, ARRAYSIZE(szName)));
            if (*pszExt == 0)
            {
                if ((pidf->fs.wAttrs & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY)) == FILE_ATTRIBUTE_SYSTEM)
                    pszExt = TEXT(".sys");
                else
                    pszExt = TEXT(".");
            }

            lstrcpyn(pszClass, pszExt, cch);
        }
        else
        {
            // This is a directory. Always use the "Directory" class.
            // This can also be a Drive id.
            ASSERT((uType == SHID_FS_DIRECTORY) ||
                   (uType == SHID_FS_DIRUNICODE) ||
                    ((uType & SHID_GROUPMASK) == SHID_COMPUTER) ||
                    (uType == SHID_NET_SERVER));

            lstrcpyn(pszClass, c_szDirectoryClass, cch);
        }
    }
    return TRUE;
}

// reverse the OLE CLSID for the file to the ProgID and return an open key
// on that ProgID.  if there is no ProdID section use the CLSID instead.  this
// way you can hang shell things off the CLSID\{GUID}
//

HKEY ProgIDKeyFromCLSIDStr(LPCTSTR pszClass)
{
    HKEY hkeyCLSID = NULL;
    HKEY hkeyProgID = NULL;

    ASSERT(pszClass[5] == TEXT('\\') && pszClass[6] == CH_GUIDFIRST);

    if (RegOpenKey(HKEY_CLASSES_ROOT, pszClass, &hkeyCLSID) == ERROR_SUCCESS)
    {
        // Get the progID from the specified CLSID
        TCHAR szProgID[80];
        ULONG cb = SIZEOF(szProgID);
        if (SHRegQueryValue(hkeyCLSID, TEXT("ProgID"), szProgID, &cb) == ERROR_SUCCESS)
        {
            // CLSID has a ProgID entry, use that.
            RegCloseKey(hkeyCLSID);    // close CLSID key
            if (RegOpenKey(HKEY_CLASSES_ROOT, szProgID, &hkeyProgID) == ERROR_SUCCESS)
            {
                // Check for a newer version of the ProgID
                cb = SIZEOF(szProgID);
                if (SHRegQueryValue(hkeyProgID, TEXT("CurVer"), szProgID, &cb) == ERROR_SUCCESS)
                {
                    HKEY hkeyNewProgID = NULL;
                    if (RegOpenKey(HKEY_CLASSES_ROOT, szProgID, &hkeyNewProgID) == ERROR_SUCCESS)
                    {
                        RegCloseKey(hkeyProgID);    // close old ProgID key
                        hkeyProgID = hkeyNewProgID;
                    }
                }
            }
        }
        else
        {
            // This extension has CLSID only (like Control panel).
            // use the hkeyCLSID as the hkeyProgID.
            // It will allow us to have "shell" stuff here.
            hkeyProgID = hkeyCLSID;
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "%s not found in registry", pszClass);
    }

    return hkeyProgID;
}

HKEY ProgIDKeyFromCLSID(const CLSID *pclsid)
{
    TCHAR szClass[GUIDSTR_MAX + 10];

    lstrcpy(szClass, c_szCLSIDSlash);

    SHStringFromGUID(pclsid, szClass + 6, GUIDSTR_MAX);

    return ProgIDKeyFromCLSIDStr(szClass);
}

STDAPI SHAssocCreateForFile(LPCWSTR pszFile, DWORD dwAttributes, const CLSID *pclsid, REFIID riid, void **ppv)
{
    IQueryAssociations *pqa;
    HRESULT hr;

    *ppv = NULL;

    hr = AssocCreate(CLSID_QueryAssociations, &IID_IQueryAssociations, (void **)&pqa);
    if (SUCCEEDED(hr))
    {
        WCHAR wz[128];
        LPCWSTR pszInit = NULL;
        ASSOCF flags = 0;

        if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            //  Directory already has the baseclass set...
            flags = ASSOCF_INIT_DEFAULTTOFOLDER;
            pszInit = L"Directory";
        }
        else
        {
            // This is a file. Get the class based on the extension.
            pszInit = PathFindExtensionW(pszFile);
            if (*pszInit == 0)
            {
                if ((dwAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY)) == FILE_ATTRIBUTE_SYSTEM)
                    pszInit = L".sys";
            }
            flags = ASSOCF_INIT_DEFAULTTOSTAR;
        }

        if (pclsid)
        {
            // This is a junction point, get the CLSID from it.
            // we take precedence over the other types

            SHStringFromGUIDW(pclsid, wz, SIZECHARS(wz));
            pszInit = wz;
        }

        ASSERT(pszInit);
        
        hr = pqa->lpVtbl->Init(pqa, flags, pszInit, NULL, NULL);
        if (SUCCEEDED(hr))
            hr = pqa->lpVtbl->QueryInterface(pqa, riid, ppv);
        pqa->lpVtbl->Release(pqa);
    }

    return hr;
}

HRESULT FS_AssocCreate(LPCIDFOLDER pidf, REFIID riid, void **ppv)
{
    CLSID clsid;
    WCHAR szFile[MAX_PATH];

    FS_CopyNameW(pidf, szFile, ARRAYSIZE(szFile));

    ASSERT(szFile[0]);

    return SHAssocCreateForFile(szFile, pidf->fs.wAttrs, FS_GetCLSID(pidf, &clsid) ? &clsid : NULL, riid, ppv);
}

//
//  get the class key to be used for this class
// in:
//      pidl    fully qualified in the name space
//              
// out:
//      *phkeyProgID    "class" key akk ProgID
//      *phkeyBaseID    the base class of an object, currently we only
//                      for example '*' (for files) and 'Folder' (for folders)
// Notes:
//   The caller should close returned keys (via SHCloseClassKey)
//

STDAPI_(BOOL) SHGetClassKey(LPCITEMIDLIST pidl, HKEY *phkeyProgID, HKEY *phkeyBaseID)
{
    BOOL bRet = FALSE;
    IQueryAssociations *pqa;

    if (phkeyProgID)
        *phkeyProgID = NULL;

    if (phkeyBaseID)
        *phkeyBaseID = NULL;

    if (SUCCEEDED(SHGetAssociations(pidl, &pqa)))
    {
        if (phkeyProgID)
            pqa->lpVtbl->GetKey(pqa, ASSOCF_IGNOREBASECLASS, ASSOCKEY_CLASS, NULL, phkeyProgID);
        if (phkeyBaseID)
            pqa->lpVtbl->GetKey(pqa, 0, ASSOCKEY_BASECLASS, NULL, phkeyBaseID);
        pqa->lpVtbl->Release(pqa);
        bRet = TRUE;
    }
    return bRet;
}

HRESULT FSGetClassKey(LPCIDFOLDER pidf, HKEY *phkeyProgID)
{
    IQueryAssociations *pqa;
    HRESULT hr = FS_AssocCreate(pidf, &IID_IQueryAssociations, (void **)&pqa);
    if (SUCCEEDED(hr))
    {
        hr = pqa->lpVtbl->GetKey(pqa, ASSOCF_IGNOREBASECLASS, ASSOCKEY_CLASS, NULL, phkeyProgID);
        pqa->lpVtbl->Release(pqa);
    }
    else
        *phkeyProgID = NULL;
    return hr;
}


//
//  IsMemberOfCategory
//
//  Description: This simulates the ComponentCategoryManager
//  call which checks to see if a CLSID is a member of a CATID.
//
STDAPI_(BOOL) IsMemberOfCategory(HKEY hkeyProgID, REFCATID rcatid)
{
    BOOL fRet = FALSE;
    DWORD cb;
    TCHAR szCLSID[GUIDSTR_MAX], szCATID[GUIDSTR_MAX];
    TCHAR szKey[GUIDSTR_MAX * 4];

    //
    // From the ProgID, get the CLSID.
    //
    cb = SIZEOF(szCLSID);
    if (SHRegQueryValue(hkeyProgID, c_szCLSID, szCLSID, &cb) == ERROR_SUCCESS)
    {
        //
        // Construct the registry key that detects if
        // a CLSID is a member of a CATID.
        //
        SHStringFromGUID(rcatid, szCATID, ARRAYSIZE(szCATID));
        wsprintf(szKey, TEXT("CLSID\\%s\\Implemented Categories\\%s"),
                 szCLSID, szCATID);

        //
        // See if it's there.
        //
        cb = 0;
        if (SHRegQueryValue(HKEY_CLASSES_ROOT, szKey, NULL, &cb) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }
    }

    return fRet;
}


//
// Description:
//  close a key open'ed via SHGetClassKey
//
//  the idea here is we can cache a few class related hkeys, but all we
//  do right now is call RegCloseKey
//
STDAPI_(void) SHCloseClassKey(HKEY hkeyProgID)
{
    if (hkeyProgID)
        RegCloseKey(hkeyProgID);
}

// forward
BOOL _GetFolderIconPath(CFSFolder *this, LPCIDFOLDER pidf, LPTSTR pszIcon, int cchMax, int * pIndex);

//===========================================================================
// SHGetClassFlags  -  get flags for a file class.
//
// given a FS PIDL returns a DWORD of flags, or 0 for error
//
//      SHCF_ICON_INDEX         this is this sys image index for per class
//      SHCF_ICON_PERINSTANCE   icons are per instance (one per file)
//      SHCF_ICON_DOCICON       icon is in shell\open\command (simulate doc icon)
//
//      SHCF_HAS_ICONHANDLER    set if class has a IExtractIcon handler
//
//      SHCF_UNKNOWN            set if extenstion is not registered
//
//      SHCF_IS_LINK            set if class is a link
//      SHCF_ALWAYS_SHOW_EXT    always show the extension
//      SHCF_NEVER_SHOW_EXT     never show the extension
//
//===========================================================================

STDAPI_(DWORD) SHGetClassFlags(LPCIDFOLDER pidf)
{
    HKEY hkey = NULL;
    TCHAR ach[MAX_PATH], szClass[MAX_CLASS];
    DWORD dwFlags, cb;
#ifdef DEBUG
    DWORD dwCachedFlags = -1;
#endif

    pidf = (LPCIDFOLDER)ILFindLastID((LPCITEMIDLIST)pidf);

    //
    // look up the file type in the cache.
    //
    SHGetClass(pidf, szClass, ARRAYSIZE(szClass));

    dwFlags = LookupFileClass(szClass);

    //
    // if we got a cache hit we are done
    //
    if (dwFlags != 0)
    {
#ifdef DEBUG
        ASSERT(dwFlags != -1);
        // on debug builds we verify all cache hits
        dwCachedFlags = dwFlags;
#else
        return dwFlags;
#endif // !DEBUG
    }

    //  default flag values
    if (FS_IsJunction(pidf) &&
        _GetFileCLSID(FS_CopyName(pidf, ach, ARRAYSIZE(ach)), NULL))
    {
        // always hide extension for .{guid} junction points:
        dwFlags = SHCF_NEVER_SHOW_EXT;
    }
    else if (FS_IsFolder(pidf))
    {
        dwFlags = SHCF_ALWAYS_SHOW_EXT;
    }
    else
    {
        dwFlags = 0;
    }

    //
    // open the class key.
    //
    if (FAILED(FSGetClassKey(pidf, &hkey)))
    {
        int iIcon = FS_IsFolder(pidf) ? II_FOLDER : II_DOCNOASSOC;
        int iImage = Shell_GetCachedImageIndex(c_szShell32Dll, iIcon, 0);

        // Shell_GetCachedImageIndex can return -1 for failure cases. We
        // dont want to or that in, so check to make sure the index is valid.
        if ((iImage & ~SHCF_ICON_INDEX) == 0)
        {
            // no higher bits set so its ok to or the index in
            dwFlags |= iImage;
        }

        //
        // unknown type - pick defaults and get out.
        //
        dwFlags |= SHCF_UNKNOWN | SHCF_ALWAYS_SHOW_EXT;

        goto done;
    }

    // see what handlers exist
    if ((0 != (cb = SIZEOF(ach))) && SHRegQueryValue(hkey, c_szIconHandler, ach, &cb) == ERROR_SUCCESS)
        dwFlags |= SHCF_HAS_ICONHANDLER;

    // check for browsability
    if ((cb = SIZEOF(ach)) && SHRegQueryValue(hkey, TEXT("DocObject"), ach, &cb) == ERROR_SUCCESS &&
        !(SHGetAppCompatFlags(ACF_DOCOBJECT) & ACF_DOCOBJECT))
        dwFlags |= SHCF_IS_DOCOBJECT;
    else if ((cb = SIZEOF(ach)) && SHRegQueryValue(hkey, TEXT("BrowseInPlace"), ach, &cb) == ERROR_SUCCESS &&
        !(SHGetAppCompatFlags(ACF_DOCOBJECT) & ACF_DOCOBJECT))
        dwFlags |= SHCF_IS_DOCOBJECT;

    if (IsMemberOfCategory(hkey, &CATID_BrowsableShellExt))
        dwFlags |= SHCF_IS_SHELLEXT;

    //  get attributes
    if ((0 != (cb = SIZEOF(ach))) && SHQueryValueEx(hkey, TEXT("IsShortcut"), NULL, NULL, (BYTE *)ach, &cb) == ERROR_SUCCESS)
        dwFlags |= SHCF_IS_LINK;

    if ((0 != (cb = SIZEOF(ach))) && SHQueryValueEx(hkey, TEXT("AlwaysShowExt"), NULL, NULL, (BYTE *)ach, &cb) == ERROR_SUCCESS)
        dwFlags |= SHCF_ALWAYS_SHOW_EXT;

    if ((0 != (cb = SIZEOF(ach))) && SHQueryValueEx(hkey, TEXT("NeverShowExt"), NULL, NULL, (BYTE *)ach, &cb) == ERROR_SUCCESS)
        dwFlags |= SHCF_NEVER_SHOW_EXT;

    // figure out what type of icon this type of file uses.
    if (dwFlags & SHCF_HAS_ICONHANDLER)
    {
        dwFlags |= SHCF_ICON_PERINSTANCE;
    }
    else
    {
        HKEY hkeyCLSID;
        // check for icon in ProgID
        ach[0] = 0;
        cb = SIZEOF(ach);
        SHRegQueryValue(hkey, c_szDefaultIcon, ach, &cb);

        // Then, check if the default icon is specified in OLE-style.

        if (ach[0] == 0 && (NULL != (hkeyCLSID = SHOpenCLSID(hkey))))
        {
            cb = SIZEOF(ach);
            SHRegQueryValue(hkeyCLSID, c_szDefaultIcon, ach, &cb);
            RegCloseKey(hkeyCLSID);
        }

        if (ach[0] == 0 && (0 != (cb = SIZEOF(ach))) && SHRegQueryValue(hkey, c_szShellOpenCmd, ach, &cb) == ERROR_SUCCESS && ach[0])
        {
            PathRemoveBlanks(ach);
            PathRemoveArgs(ach);
            dwFlags |= SHCF_ICON_DOCICON;
        }

        // Check if this is a per-instance icon

        if (lstrcmp(ach, c_szPercentOne) == 0 ||
            lstrcmp(ach, c_szPercentOneInQuotes) == 0)
        {
            dwFlags &= ~SHCF_ICON_DOCICON;
            dwFlags |= SHCF_ICON_PERINSTANCE;
        }
        else if (ach[0])
        {
            int iIcon = PathParseIconLocation(ach);
            int iImage = Shell_GetCachedImageIndex(ach, iIcon, dwFlags & SHCF_ICON_DOCICON ? GIL_SIMULATEDOC : 0);

            if (iImage == -1)
            {
                iIcon = dwFlags & SHCF_ICON_DOCICON ? II_DOCUMENT : II_DOCNOASSOC;
                iImage = Shell_GetCachedImageIndex(c_szShell32Dll, iIcon, 0);
            }

            // Shell_GetCachedImageIndex can return -1 for failure cases. We
            // dont want to or -1 in, so check to make sure the index is valid.
            if ((iImage & ~SHCF_ICON_INDEX) == 0)
            {
                // no higher bits set so its ok to or the index in
                dwFlags |= iImage;
            }
        }
        else
        {
            int iIcon = FS_IsFolder(pidf) ? II_FOLDER : II_DOCNOASSOC;
            int iImage = Shell_GetCachedImageIndex(c_szShell32Dll, iIcon, 0);

            // Shell_GetCachedImageIndex can return -1 for failure cases. We
            // dont want to or -1 in, so check to make sure the index is valid.
            if ((iImage & ~SHCF_ICON_INDEX) == 0)
            {
                // no higher bits set so its ok to or the index in
                dwFlags |= iImage;
            }

            dwFlags |= SHCF_ICON_DOCICON;   // make dwFlags non-zero
        }
    }

done:
    SHCloseClassKey(hkey);

#ifdef DEBUG
    if (IsFlagSet(g_dwDumpFlags, DF_CLASSFLAGS))
    {
        TCHAR szTmp[MAX_PATH];

        if (dwCachedFlags != -1)
        {
            // if we had a cache hit above, then it better be the case that our cache hit matches our 
            ASSERTMSG(dwCachedFlags == dwFlags, "SHGetClassFlags: !!!! the file class cache is out of sync !!!! (%s: %08X != %08X)", szClass, dwCachedFlags, dwFlags);

            // make us behave just like retail builds
            dwFlags = dwCachedFlags;
        }

        FS_CopyName(FS_FindLastID(pidf), szTmp, ARRAYSIZE(szTmp));

        TraceMsg(TF_FSTREE, "SHGetClassFlags(%s) '%s' %08lX", szTmp, szClass, dwFlags);

        if (dwFlags & SHCF_UNKNOWN            ) ach[0] = 0;
        if (dwFlags & SHCF_UNKNOWN            ) TraceMsg(TF_FSTREE, "    is unknown type    ");

        if (dwFlags & SHCF_ICON_PERINSTANCE   ) TraceMsg(TF_FSTREE, "    icon is per instance");
        if (!(dwFlags & SHCF_ICON_PERINSTANCE)) TraceMsg(TF_FSTREE, "    icon is per class %s,%d", ach, dwFlags & SHCF_ICON_INDEX);

        if (dwFlags & SHCF_ALWAYS_SHOW_EXT    ) TraceMsg(TF_FSTREE, "    always show extension     ");
        if (dwFlags & SHCF_NEVER_SHOW_EXT     ) TraceMsg(TF_FSTREE, "    never show extension     ");

        if (dwFlags & SHCF_IS_LINK            ) TraceMsg(TF_FSTREE, "    is a link          ");

        if (dwFlags & SHCF_HAS_ICONHANDLER    ) TraceMsg(TF_FSTREE, "    has ICONHANDLER    ");
    }
#endif

    if(0 == dwFlags)
    {
        // If we hit this, the extension for this file type is incorrectly installed
        // and it will cause double clicking on such files to open the "Open With..."
        // file associatins dialog.
        //
        // IF YOU HIT THIS:
        // 1. Find the file type by checking szClass.
        // 2. Contact the person that installed that file type and have them fix
        //    the install to have an icon and an associated program.
        TraceMsg(TF_WARNING, "SHGetClassFlags() has detected an improperly registered class: '%s'", szClass);
    }
    
    AddFileClass(szClass, dwFlags);
    return dwFlags;
}

const TCHAR c_szClassInfo[] = STRINI_CLASSINFO;

//
// this function checks for flags in desktop.ini
//

#define GFF_DEFAULT_TO_FS          0x0001      // the shell-xtension permits FS as the default where it cannot load
#define GFF_ICON_FOR_ALL_FOLDERS   0x0002      // use the icon specified in the desktop.ini for all sub folders

BOOL _GetFolderFlags(CFSFolder *this, LPCIDFOLDER pidf, UINT *prgfFlags)
{
    TCHAR szPath[MAX_PATH];

    *prgfFlags = 0;

    if (FAILED(CFSFolder_GetPathForItem(this, pidf, szPath)))
        return FALSE;

    if (PathAppend(szPath, c_szDesktopIni))
    {
        if (GetPrivateProfileInt(c_szClassInfo, TEXT("DefaultToFS"), 1, szPath))
        {
            *prgfFlags |= GFF_DEFAULT_TO_FS;
        }
#if 0
        if (GetPrivateProfileInt(c_szClassInfo, TEXT("SubFoldersUseIcon"), 1, szPath))
        {
            *prgfFlags |= GFF_ICON_FOR_ALL_FOLDERS;
        }
#endif
    }
    return TRUE;
}

//
// This funtion retrieves the ICONPATh from desktop.ini file.
// It takes a pidl as an input.
// NOTE: There is code in SHDOCVW--ReadIconLocation that does almost the same thing
// only that code looks in .URL files instead of desktop.ini
BOOL _GetFolderIconPath(CFSFolder *this, LPCIDFOLDER pidf, LPTSTR pszIcon, int cchMax, int * pIndex)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szIcon[MAX_PATH];
    BOOL fSuccess = FALSE;
    int iIndex;

    if (pszIcon == NULL)
    {
        pszIcon = szIcon;
        cchMax = ARRAYSIZE(szPath);
    }

    if (pIndex == NULL)
        pIndex = &iIndex;

    *pIndex = II_FOLDER;    // Default the index to II_FOLDER (default folder icon)

    if (SUCCEEDED(CFSFolder_GetPathForItem(this, pidf, szPath)))
    {
        if (_GetFolderString(szPath, this->_pszNetProvider, pszIcon, cchMax, SZ_CANBEUNICODE TEXT("IconFile")))
        {
            TCHAR szIndex[16];
            if (_GetFolderString(szPath, this->_pszNetProvider, szIndex, ARRAYSIZE(szIndex), TEXT("IconIndex")))
            {
                StrToIntEx(szIndex, 0, pIndex);
            }
            //
            // Fix the relative path and return
            // We consider this a success even if no IconIndex was stored in the desktop.ini file
            //

            PathCombine(pszIcon, szPath, pszIcon);
            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

// IDList factory

STDAPI CFSFolder_CreateIDList(CFSFolder *this, const WIN32_FIND_DATA *pfd, LPITEMIDLIST *ppidl)
{
    UINT cbFileName, cbAltFileName, cb, cchFileName;
    CHAR szFileName[MAX_PATH];
    BOOL fUnicode;
    LPIDFOLDER pidf;
    BYTE bFlags;
    WORD dateModified, timeModified;

#ifdef UNICODE
    cchFileName = WideCharToMultiByte(CP_ACP, 0, pfd->cFileName, -1, NULL, 0, NULL, NULL);
    cbAltFileName = WideCharToMultiByte(CP_ACP, 0, pfd->cAlternateFileName, -1, NULL, 0, NULL, NULL);    // Size of ansi part of id;
#else
    cchFileName  = lstrlen(pfd->cFileName) + 1;
    cbAltFileName= lstrlen(pfd->cAlternateFileName) + 1;   // Size of ansi part of id
#endif

    if (DoesStringRoundTrip(pfd->cFileName, szFileName, ARRAYSIZE(szFileName)))
    {
        cbFileName = cchFileName;
        fUnicode = FALSE;   // Ok to create an ansi idl
    }
    else
    {
        cbFileName = cchFileName * SIZEOF(WCHAR);
        fUnicode = TRUE;    // Have to create a complete unicode idl
    }

    cb = FIELD_OFFSET(IDFOLDER, fs.cFileName) + cbFileName + cbAltFileName;

    if (pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        bFlags = SHID_FS_DIRECTORY;
    else
        bFlags = SHID_FS_FILE;

    if (fUnicode)
        bFlags |= SHID_FS_UNICODE;

    // if (CFSFolder_IsCSIDL(this, CSIDL_COMMON_DESKTOPDIRECTORY))
    //     bFlags |= SHID_FS_COMMONITEM;

    pidf = (LPIDFOLDER)_ILCreate(cb + SIZEOF(USHORT));
    if (pidf)
    {
        // We pack the 2 string in
        pidf->cb = (SHORT) cb;

        // tag files > 4G so we can do a full find first when we need to know the real size
        pidf->fs.dwSize = pfd->nFileSizeHigh ? 0xFFFFFFFF : pfd->nFileSizeLow;
        pidf->fs.wAttrs = (WORD)pfd->dwFileAttributes;

        // Since the idl entry is not aligned, we cannot just send the address
        // of one of its members blindly into FileTimeToDosDateTime.

        if (FileTimeToDosDateTime(&pfd->ftLastWriteTime,  &dateModified, &timeModified))
        {
            *((UNALIGNED WORD *)&pidf->fs.dateModified) = dateModified;
            *((UNALIGNED WORD *)&pidf->fs.timeModified) = timeModified;
        }

#ifdef UNICODE
        if (fUnicode)
            ualstrcpy(pidf->fs.cFileName, pfd->cFileName);
        else
            lstrcpyA((LPSTR)pidf->fs.cFileName, szFileName);
        SHUnicodeToAnsi(pfd->cAlternateFileName, (LPSTR)pidf->fs.cFileName+cbFileName, cbAltFileName);
#else
        lstrcpy(pidf->fs.cFileName, pfd->cFileName);
        lstrcpy((LPSTR)pidf->fs.cFileName + cbFileName, pfd->cAlternateFileName);
#endif

        pidf->bFlags = bFlags;

        pidf = CFSFolder_TryAppendJunctionID(this, pidf, pfd->cFileName);

#ifdef FEATURE_LOCALIZED_FOLDERS
        if (pidf)
            pidf = CFSFolder_TryAppendLocalizedNameID(this, pidf);
#endif //FEATURE_LOCALIZED_FOLDERS
    }

    *ppidl = (LPITEMIDLIST)pidf;
    return *ppidl != NULL ? S_OK : E_OUTOFMEMORY;
}

BOOL _ValidPathSegment(LPCTSTR pszSegment)
{
    if (*pszSegment && !PathIsDotOrDotDot(pszSegment))
    {
        LPCTSTR psz;
        for (psz = pszSegment; *psz; psz = CharNext(psz))
        {
            if (!PathIsValidChar(*psz, PIVC_LFN_NAME))
                return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}



// used to parse up file path like strings:
//      "folder\folder\file.txt"
//      "file.txt"
//
// in/out:
//      *ppszIn   in: pointer to start of the buffer, 
//                output: advanced to next location, NULL on last segment
// out:
//      *pszSegment NULL if nothing left
//
// returns:
//      S_OK            got a segment
//      S_FALSE         loop done, *pszSegment emtpy
//      E_INVALIDARG    invalid input "", "\foo", "\\foo", "foo\\bar", "?<>*" chars in seg
 
HRESULT _NextSegment(LPCWSTR *ppszIn, LPTSTR pszSegment, UINT cchSegment, BOOL bValidate)
{
    HRESULT hres;

    *pszSegment = 0;

    if (*ppszIn)
    {
        LPWSTR pszSlash = StrChrW(*ppszIn, L'\\');
        if (pszSlash)
        {
            if (pszSlash > *ppszIn) // make sure well formed (no dbl slashes)
            {
                OleStrToStrN(pszSegment, cchSegment, *ppszIn, (int)(pszSlash - *ppszIn));

                //  make sure that there is another segment to return
                if (!*(++pszSlash))
                    pszSlash = NULL;
                hres = S_OK;       
            }
            else
            {
                pszSlash = NULL;
                hres = E_INVALIDARG;    // bad input
            }
        }
        else
        {
            SHUnicodeToTChar(*ppszIn, pszSegment, cchSegment);
            hres = S_OK;       
        }
        *ppszIn = pszSlash;

        if (hres == S_OK && bValidate && !_ValidPathSegment(pszSegment))
        {
            *pszSegment = 0;
            hres = E_INVALIDARG;
        }
    }
    else
        hres = S_FALSE;     // done with loop

    return hres;
}

//  this makes a fake wfd and then uses the normal
//  FillIDFolder as if it were a real found path.

HRESULT CFSFolder_ParseSimple(CFSFolder *this, LPCWSTR pszPath, const WIN32_FIND_DATA *pfdLast, LPITEMIDLIST *ppidl)
{
    WIN32_FIND_DATA wfd = {0};
    HRESULT hr;

    *ppidl = NULL;

    while (S_OK == (hr = _NextSegment(&pszPath, wfd.cFileName, ARRAYSIZE(wfd.cFileName), FALSE)))
    {
        LPITEMIDLIST pidl;

        if (pszPath)
        {
            // internal componets must be folders
            wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        }
        else
        {
            // last segment takes the find data from that passed in
            // copy everything except the cFileName field
            memcpy(&wfd, pfdLast, FIELD_OFFSET(WIN32_FIND_DATA, cFileName));
            lstrcpyn(wfd.cAlternateFileName, pfdLast->cAlternateFileName, ARRAYSIZE(wfd.cAlternateFileName));
        }

        hr = CFSFolder_CreateIDList(this, &wfd, &pidl);
        if (SUCCEEDED(hr))
            hr = SHILAppend(pidl, ppidl);
    }

    if (FAILED(hr))
    {
        if (*ppidl)
        {
            ILFree(*ppidl);
            *ppidl = NULL;
        }
    }
    else
        hr = S_OK;      // pin all success to S_OK
    return hr;
}


HRESULT CFSFolder_FindDataFromName(CFSFolder *this, LPCTSTR pszName, WIN32_FIND_DATA *pfd)
{
    TCHAR szPath[MAX_PATH];

    HRESULT hr = CFSFolder_GetPath(this, szPath);
    if (SUCCEEDED(hr))
    {
        HANDLE hfind;
        PathAppend(szPath, pszName);

        // BUGBUG: We should supply a punkEnableModless in order to go modal during UI.
        hr = SHFindFirstFileRetry(NULL, NULL, szPath, pfd, &hfind, SHPPFW_NONE);
        if (hr == S_OK)
            FindClose(hfind);
    }
    else
        hr = E_FAIL;

    return hr;
}

//
// This function returns a relative pidl for the specified file/folder
//
HRESULT CFSFolder_CreateIDListFromName(CFSFolder *this, LPCTSTR pszName, LPITEMIDLIST *ppidl)
{
    WIN32_FIND_DATA fd;
    HRESULT hr = CFSFolder_FindDataFromName(this, pszName, &fd);
    if (SUCCEEDED(hr))
        hr = CFSFolder_CreateIDList(this, &fd, ppidl);
    else
        *ppidl = NULL;

    return hr;
}

// used to detect if a name is a folder. this is used in the case that the
// security for this folders parent is set so you can't enum it's contents

BOOL CFSFolder_CanSeeInThere(CFSFolder *this, LPCTSTR pszName)
{
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(CFSFolder_GetPath(this, szPath)))
    {
        HANDLE hfind;
        WIN32_FIND_DATA fd;

        PathAppend(szPath, pszName);
        PathAppend(szPath, TEXT("*.*"));

        hfind = FindFirstFile(szPath, &fd);
        if (hfind != INVALID_HANDLE_VALUE)
            FindClose(hfind);
        return hfind != INVALID_HANDLE_VALUE;
    }
    return FALSE;
}


STDAPI_(CFSFolder *) FS_GetFSFolderFromShellFolder(IShellFolder *psf)
{
    CFSFolder *this;
    if (psf && SUCCEEDED(psf->lpVtbl->QueryInterface(psf, &IID_INeedRealCFSFolder, (void **)&this)))
    {
        return this;
    }
    return NULL;
}

//
// QueryInterface
//
STDMETHODIMP CFSFolderUnk_QueryInterface(IUnknown *punk, REFIID riid, void **ppvObj)
{
    CFSFolder *this = IToClass(CFSFolder, iunk, punk);

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = &this->iunk;
    }
    else if (IsEqualIID(riid, &IID_IShellFolder) ||
             IsEqualIID(riid, &IID_IShellFolder2))
    {
        *ppvObj = &this->sf;
    }
    else if (IsEqualIID(riid, &IID_IShellIcon))
    {
        *ppvObj = &this->si;
    }
    else if (IsEqualIID(riid, &IID_IShellIconOverlay))
    {
        *ppvObj = &this->sio;
    }
    else if (IsEqualIID(riid, &IID_IPersist) ||
             IsEqualIID(riid, &IID_IPersistFolder) ||
             IsEqualIID(riid, &IID_IPersistFolder2) ||
             IsEqualIID(riid, &IID_IPersistFolder3))
    {
        *ppvObj = &this->pf;
    }
    else if (IsEqualIID(riid, &IID_IPersistFreeThreadedObject) && (this->punkOuter == &this->iunk))
    {
        // only respond to this if we are not agregated since we can't know if our
        // agregator is free threaded
        *ppvObj = &this->pf;
    }
    else if (IsEqualIID(riid, &IID_INeedRealCFSFolder))
    {
        *ppvObj = this;     // return unreffed pointer
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown*)(*ppvObj))->lpVtbl->AddRef(*ppvObj);
    return S_OK;
}

STDMETHODIMP_(ULONG) CFSFolderUnk_AddRef(IUnknown *punk)
{
    CFSFolder *this = IToClass(CFSFolder, iunk, punk);
    return InterlockedIncrement(&this->cRef);
}

// briefcase and file system folder call to reset data

STDAPI_(void) CFSFolder_Reset(CFSFolder *this)
{
    if (this->_hdsaColHandlers)
        DestroyColHandlers(&this->_hdsaColHandlers);

    if (this->_pidl)
    {
        ILFree(this->_pidl);
        this->_pidl = NULL;
    }

    if (this->_pidlTarget)
    {
        ILFree(this->_pidlTarget);
        this->_pidlTarget = NULL;   
    }

    if (this->_pszPath)
    {
        LocalFree(this->_pszPath);
        this->_pszPath = NULL;
    }

    if (this->_pszNetProvider)
    {
        LocalFree(this->_pszNetProvider);
        this->_pszNetProvider;
    }

    this->_csidl = -1;
    this->_dwAttributes = -1;

    this->_csidlTrack = -1;
}

STDMETHODIMP_(ULONG) CFSFolderUnk_Release(IUnknown *punk)
{
    CFSFolder *this = IToClass(CFSFolder, iunk, punk);
    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

    CFSFolder_Reset(this);
    LocalFree((HLOCAL)this);
    return 0;
}

const IUnknownVtbl c_FSFolderUnkVtbl =
{
    CFSFolderUnk_QueryInterface, CFSFolderUnk_AddRef, CFSFolderUnk_Release
};


//===========================================================================
// CFSFolder : Members
//===========================================================================

STDMETHODIMP CFSFolder_QueryInterface(IShellFolder2 *psf, REFIID riid, void **ppvObj)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    return this->punkOuter->lpVtbl->QueryInterface(this->punkOuter, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CFSFolder_AddRef(IShellFolder2 *psf)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    return this->punkOuter->lpVtbl->AddRef(this->punkOuter);
}

STDMETHODIMP_(ULONG) CFSFolder_Release(IShellFolder2 *psf)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    return this->punkOuter->lpVtbl->Release(this->punkOuter);
}

// we need to fail relative type paths since we use PathCombine
// and we don't want that and the Win32 APIs to give us relative path behavior
// ShellExecute() depends on this so it falls back and resolves the relative paths itself

STDMETHODIMP CFSFolder_ParseDisplayName(IShellFolder2 *psf, HWND hwnd, LPBC pbc, 
                                        WCHAR *pszName, ULONG *pchEaten, 
                                        LPITEMIDLIST *ppidl, DWORD *pdwAttributes)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    HRESULT hres;
    WIN32_FIND_DATA fd;

    *ppidl = NULL;   // assume error

    if (pszName == NULL)
        return E_INVALIDARG;

    if (S_OK == SHIsFileSysBindCtx(pbc, &fd))
    {
        hres = CFSFolder_ParseSimple(this, pszName, &fd, ppidl);
    }
    else
    {
        TCHAR szName[MAX_PATH];

        hres = _NextSegment(&pszName, szName, ARRAYSIZE(szName), TRUE);
        if (SUCCEEDED(hres))
        {
            hres = CFSFolder_CreateIDListFromName(this, szName, ppidl);

            if (hres == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
            {
                // security "List folder contents" may be disabled for
                // this items parent. so see if this is really there
                if (pszName || CFSFolder_CanSeeInThere(this, szName))
                {
                    // smells like a folder
                    ZeroMemory(&fd, sizeof(fd));
                    fd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                    lstrcpyn(fd.cFileName, szName, ARRAYSIZE(fd.cFileName));
                    hres = CFSFolder_CreateIDList(this, &fd, ppidl);
                }
            }
            else if ((hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) && 
                     (pszName == NULL) && 
                     (BindCtx_GetMode(pbc, 0) & STGM_CREATE))
            {
                // create a pidl to something that doesnt exist.
                ZeroMemory(&fd, sizeof(fd));
                fd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;        // a file, not a folder
                lstrcpyn(fd.cFileName, szName, ARRAYSIZE(fd.cFileName));
                hres = CFSFolder_CreateIDList(this, &fd, ppidl);
            }

            if (SUCCEEDED(hres))
            {
                if (pszName) // more stuff to parse?
                {
                    IShellFolder *psfFolder;
                    hres = CFSFolder_BindToObject(psf, *ppidl, pbc, &IID_IShellFolder, &psfFolder);
                    if (SUCCEEDED(hres))
                    {
                        ULONG chEaten;
                        LPITEMIDLIST pidlNext;

                        hres = psfFolder->lpVtbl->ParseDisplayName(psfFolder, hwnd, pbc, 
                            pszName, &chEaten, &pidlNext, pdwAttributes);
                        if (SUCCEEDED(hres))
                        {
                            hres = SHILAppend(pidlNext, ppidl);
                        }
                        psfFolder->lpVtbl->Release(psfFolder);
                    }
                }
                else
                {
                    if (pdwAttributes && *pdwAttributes)
                        CFSFolder_GetAttributesOf(psf, 1, ppidl, pdwAttributes);
                }
            }
        }
    }

    if (FAILED(hres) && *ppidl)
    {
        // This is needed if psfFolder->lpVtbl->ParseDisplayName() or CFSFolder_BindToObject()
        // fails because the pidl is already allocated.
        ILFree(*ppidl);
        *ppidl = NULL;
    }
    ASSERT(SUCCEEDED(hres) ? *ppidl : *ppidl == NULL);

    if (FAILED(hres))
        TraceMsg(TF_ALWAYS, "CFSFolder_ParseDisplayName(), hres:%x %ls", hres, pszName);
    return hres;
}

void _InitFileFolderClassNames (void)
{
    if (g_szFileTemplate[0] == 0)    // test last one to avoid race
    {
        LoadString(HINST_THISDLL, IDS_FOLDERTYPENAME, g_szFolderTypeName,  ARRAYSIZE(g_szFolderTypeName));
        LoadString(HINST_THISDLL, IDS_FILETYPENAME, g_szFileTypeName, ARRAYSIZE(g_szFileTypeName));
        LoadString(HINST_THISDLL, IDS_EXTTYPETEMPLATE, g_szFileTemplate, ARRAYSIZE(g_szFileTemplate));
    }
}

STDMETHODIMP CFSFolder_EnumObjects(IShellFolder2 *psf, HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    TCHAR szThisFolder[MAX_PATH];

    _InitFileFolderClassNames();

    CFSFolder_GetPath(this, szThisFolder);
    if (!PathIsUNC(szThisFolder))
    {
        // For mapped net drives, register a change
        // notify alias for the corresponding UNC path.
        MountPoint_RegisterChangeNotifyAlias(DRIVEID(szThisFolder));
    }                            

    return CFSFolder_CreateEnum(this, (IUnknown *)psf, hwnd, grfFlags, ppenum);
}

// this is a heuristic to determine if the IDList was created
// normally or with a simple bind context

BOOL FS_IsReal(LPCIDFOLDER pidf)
{
    return pidf->fs.dwSize | pidf->fs.dateModified ? TRUE : FALSE;
}

STDMETHODIMP CFSFolder_BindToObject(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    HRESULT hres = E_INVALIDARG;
    LPCIDFOLDER pidf = FS_IsValidID(pidl);

    *ppv = NULL;

    if (pidf)
    {
        // BUGBUG: we should allow bind to non folders on interfaces other that IShellFolder

        if (FS_IsFolder(pidf) ||
            FS_IsJunction(pidf) ||
            FS_IsNT4StyleSimpleID(pidf) || 
            (SHGetClassFlags(pidf) & SHCF_IS_BROWSABLE))
        {
            LPCITEMIDLIST pidlRight;
            LPIDFOLDER pidfBind;

            hres = FS_GetJunctionForBind(pidf, &pidfBind, &pidlRight);
            if (SUCCEEDED(hres))
            {
                if (hres == S_OK)
                {
                    IShellFolder *psfJunction;
                    hres = FS_Bind(this, pbc, pidfBind, &IID_IShellFolder, &psfJunction);
                    if (SUCCEEDED(hres))
                    {
                        // now bind to the stuff below the junction point
                        hres = psfJunction->lpVtbl->BindToObject(psfJunction, pidlRight, pbc, riid, ppv);
                        psfJunction->lpVtbl->Release(psfJunction);
                    }
                    FS_Free(pidfBind);
                }
                else
                {
                    ASSERT(pidfBind == NULL);
                    hres = FS_Bind(this, pbc, pidf, riid, ppv);
                }
            }
        }

        //
        //  if nobody else wanted to pick this up, then we know how to handle it..
        //
        if (FAILED(hres) && IsEqualIID(&IID_IMoniker, riid))
        {
            WCHAR szName[MAX_PATH];

            hres = CFSFolder_GetPathForItemW(this, pidf, szName);
            if (SUCCEEDED(hres))
                hres = CreateFileMoniker(szName, (IMoniker **)ppv);
        }
    }
    else
    {
       TraceMsg(TF_WARNING, "CFSFolder_BindToObject(), hres:%x bad PIDL %s", hres, DumpPidl(pidl));
    }
    return hres;
}

STDMETHODIMP CFSFolder_BindToStorage(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    HRESULT hr;
    LPCIDFOLDER pidf = FS_IsValidID(pidl);
    if (pidf)
    {
        LPCTSTR pszHandler;
        
        // map some handler types into a string instead of the GUID
        if (IsEqualIID(riid, &IID_IPropertySetStorage))
            pszHandler = TEXT("shellex\\PropertyHandler");
        // else if (IsEqualIID(riid, &IID_IFilter))
        //    pszHandler = TEXT("PersistentHandler");
        else
            pszHandler = NULL;
        
        hr = FSLoadHandler(this, pidf, pszHandler, riid, ppv);
        if (FAILED(hr))
        {
            DWORD grfMode = BindCtx_GetMode(pbc, STGM_READ | STGM_SHARE_DENY_WRITE);
            WCHAR wszPath[MAX_PATH];
            
            hr = CFSFolder_GetPathForItemW(this, pidf, wszPath);
            if (SUCCEEDED(hr))
            {
                if (IsEqualIID(riid, &IID_IStream))
                    hr = SHCreateStreamOnFileW(wszPath, grfMode, (IStream **)ppv);
                else
                    hr = StgOpenStorageEx(wszPath, grfMode, STGFMT_ANY, 0, NULL, NULL, riid, ppv);
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
        *ppv = NULL;
    }
    return hr;
}

// _BuildLinkName
//
// Used during the creation of a shortcut, this function determines an appropriate name for the shortcut.
// This is not the exact name that will be used becuase it will usually contain "() " which will either
// get removed or replaced with "(x) " where x is a number that makes the name unique.  This removal is done
// elsewhere (currently in PathYetAnotherMakeUniqueName).
//
// in:
//      pszName file spec part
//      pszDir  path part of name to know how to limit the long name...
//
// out:
//      pszLinkName - Full path to link name (May fit in 8.3...).  Can be the same buffer as pszName.
//
// NOTES: If pszDir + pszLinkName is greater than MAX_PATH we will fail to create the shortcut.
// In an effort to prevent 
void _BuildLinkName(LPTSTR pszLinkName, LPCTSTR pszName, LPCTSTR pszDir, BOOL fLinkTo)
{
    TCHAR szLinkTo[40]; // "Shortcut to %s.lnk"
    TCHAR szTemp[MAX_PATH + 40];

    if (fLinkTo)
    {
        // check to see if we're in the "don't ever say 'shortcut to' mode"
        LoadUseLinkPrefixCount();

        if (!g_iUseLinkPrefix)
        {
            fLinkTo = FALSE;
        }
        else if (g_iUseLinkPrefix > 0)
        {
            if (g_iUseLinkPrefix < MAXLINKPREFIXCOUNT)
            {
                g_iUseLinkPrefix += SHORTCUT_PREFIX_INCR;
                SaveUseLinkPrefixCount();
            }
        }
    }

    if (!fLinkTo)
    {
        // Generate the title of this link ("XX.lnk")
        LoadString(HINST_THISDLL, IDS_LINKEXTENSION, szLinkTo, ARRAYSIZE(szLinkTo));
    }
    else
    {
        // Generate the title of this link ("Shortcut to XX.lnk")
        LoadString(HINST_THISDLL, IDS_LINKTO, szLinkTo, ARRAYSIZE(szLinkTo));
    }
    wnsprintf(szTemp, ARRAYSIZE(szTemp), szLinkTo, pszName);

    PathCleanupSpecEx(pszDir, szTemp);      // get rid of illegal chars AND ensure correct filename length
    lstrcpyn(pszLinkName, szTemp, MAX_PATH);

    ASSERT(PathIsLnk(pszLinkName));
}

// return a new destination path for a link
//
// in:
//      fErrorSoTryDesktop      we are called because there was an error saving
//                              the shortcut and we want to prompt to see if the
//                              desktop should be used.
//
// in/out:
//      pszPath     on input the place being tried, on output the desktop folder
//
// returns:
//
//      IDYES       user said yes to creating a link at new place
//      IDNO        user said no to creating a link at new place
//      -1          error
//

int _PromptTryDesktopLinks(HWND hwnd, LPTSTR pszPath, BOOL fErrorSoTryDesktop)
{
    TCHAR szPath[MAX_PATH];
    int idOk;

    if (!SHGetSpecialFolderPath(hwnd, szPath, CSIDL_DESKTOPDIRECTORY, FALSE))
        return -1;      // fail no desktop dir

    if (fErrorSoTryDesktop)
    {
        // Fail, if pszPath already points to the desktop directory.
        if (lstrcmpi(szPath, pszPath) == 0)
            return -1;

        idOk = ShellMessageBox(HINST_THISDLL, hwnd,
                        MAKEINTRESOURCE(IDS_TRYDESKTOPLINK),
                        MAKEINTRESOURCE(IDS_LINKTITLE),
                        MB_YESNO | MB_ICONQUESTION);
    }
    else
    {
        ShellMessageBox(HINST_THISDLL, hwnd,
                        MAKEINTRESOURCE(IDS_MAKINGDESKTOPLINK),
                        MAKEINTRESOURCE(IDS_LINKTITLE),
                        MB_OK | MB_ICONASTERISK);
        idOk = IDYES;
    }

    if (idOk == IDYES)
        lstrcpy(pszPath , szPath);  // output

    return idOk;    // return yes or no
}

// in:
//      pszpdlLinkTo    LPCITEMIDLIST or LPCTSTR, target of link to create
//      pszDir          where we will put the link
//      uFlags          SHGNLI_ flags
//       
// out:
//      pszName         file name to create "c:\Shortcut to Foo.lnk"
//      pfMustCopy      pszpdlLinkTo was a link itself, make a copy of this

STDAPI_(BOOL) SHGetNewLinkInfo(LPCTSTR pszpdlLinkTo, LPCTSTR pszDir, LPTSTR pszName,
                               BOOL *pfMustCopy, UINT uFlags)
{
    BOOL fDosApp = FALSE;
    BOOL fLongFileNames = IsLFNDrive(pszDir);
    SHFILEINFO sfi;

    *pfMustCopy = FALSE;

    sfi.dwAttributes = SFGAO_FILESYSTEM | SFGAO_LINK;

    if (uFlags & SHGNLI_PIDL)
    {
        if (FAILED(SHGetNameAndFlags((LPCITEMIDLIST)pszpdlLinkTo, SHGDN_NORMAL,
                            pszName, MAX_PATH, &sfi.dwAttributes)))
            return FALSE;
    }
    else
    {
        if (SHGetFileInfo(pszpdlLinkTo, 0, &sfi, SIZEOF(sfi),
                          SHGFI_DISPLAYNAME | SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED |
                          ((uFlags & SHGNLI_PIDL) ? SHGFI_PIDL : 0)))
            lstrcpy(pszName, sfi.szDisplayName);
        else
            return FALSE;
    }

    if (PathCleanupSpecEx(pszDir, pszName) & PCS_FATAL)
        return FALSE;

    //
    //  WARNING:  From this point on, sfi.szDisplayName may be re-used to
    //  contain the file path of the PIDL we are linking to.  Don't rely on
    //  it containing the display name.
    //
    if (sfi.dwAttributes & SFGAO_FILESYSTEM)
    {
        LPTSTR pszPathSrc;

        if (uFlags & SHGNLI_PIDL)
        {
            pszPathSrc = sfi.szDisplayName;
            SHGetPathFromIDList((LPCITEMIDLIST)pszpdlLinkTo, pszPathSrc);
        }
        else
        {
            pszPathSrc = (LPTSTR)pszpdlLinkTo;
        }
        fDosApp = (lstrcmpi(PathFindExtension(pszPathSrc), TEXT(".pif")) == 0) ||
                  (LOWORD(GetExeType(pszPathSrc)) == 0x5A4D); // 'MZ'

        if (sfi.dwAttributes & SFGAO_LINK)
        {
            *pfMustCopy = TRUE;
            lstrcpy(pszName, PathFindFileName(pszPathSrc));
        }
        else
        {
            //
            // when making a link to a drive root. special case a few things
            //
            // if we are not on a LFN drive, dont use the full name, just
            // use the drive letter.    "C.LNK" not "Label (C).LNK"
            //
            // if we are making a link to removable media, we dont want the
            // label as part of the name, we want the media type.
            //
            // CD-ROM drives are currently the only removable media we
            // show the volume label for, so we only need to special case
            // cdrom drives here.
            //
            if (PathIsRoot(pszPathSrc) && !PathIsUNC(pszPathSrc))
            {
                if (!fLongFileNames)
                    lstrcpy(pszName, pszPathSrc);
                else if (IsCDRomDrive(DRIVEID(pszPathSrc)))
                    LoadString(HINST_THISDLL, IDS_DRIVES_CDROM, pszName, MAX_PATH);
            }
        }
        if (fLongFileNames && fDosApp)
        {
            HANDLE hPif = PifMgr_OpenProperties(pszPathSrc, NULL, 0, OPENPROPS_INHIBITPIF);
            if (hPif)
            {
                PROPPRG PP = {0};
                if (PifMgr_GetProperties(hPif, MAKELP(0, GROUP_PRG), &PP, SIZEOF(PP), 0) &&
                    ((PP.flPrgInit & PRGINIT_INFSETTINGS) ||
                    ((PP.flPrgInit & (PRGINIT_NOPIF | PRGINIT_DEFAULTPIF)) == 0)))
                {
                    SHAnsiToTChar(PP.achTitle, pszName, MAX_PATH);
                }
                PifMgr_CloseProperties(hPif, 0);
            }
        }
    }
    if (!*pfMustCopy)
    {
        // create full dest path name.  only use template iff long file names
        // can be created and the caller requested it.  _BuildLinkName will
        // truncate files on non-lfn drives and clean up any invalid chars.
        _BuildLinkName(pszName, pszName, pszDir,
           (!(*pfMustCopy) && fLongFileNames && (uFlags & SHGNLI_PREFIXNAME)));
    }

    if (fDosApp)
        PathRenameExtension(pszName, TEXT(".pif"));

    // make sure the name is unique
    // NOTE: PathYetAnotherMakeUniqueName will return the directory+filename in the pszName buffer.
    // It returns FALSE if the name is not unique or the dir+filename is too long.  If it returns
    // false then this function should return false because creation will fail.
    if (!(uFlags & SHGNLI_NOUNIQUE))
        return PathYetAnotherMakeUniqueName(pszName, pszDir, pszName, pszName);

    return TRUE;
}

#ifdef UNICODE

STDAPI_(BOOL) SHGetNewLinkInfoA(LPCSTR pszpdlLinkTo, LPCSTR pszDir, LPSTR pszName,
                                BOOL *pfMustCopy, UINT uFlags)
{
    ThunkText * pThunkText;
    BOOL bResult = FALSE;

    if (uFlags & SHGNLI_PIDL) 
    {
        // 1 string (pszpdlLinkTo is a pidl)
        pThunkText = ConvertStrings(2, NULL, pszDir);
        pThunkText->m_pStr[0] = (LPWSTR)pszpdlLinkTo;
    } 
    else 
    {
        // 2 strings
        pThunkText = ConvertStrings(2, pszpdlLinkTo, pszDir);
    }

    if (pThunkText)
    {
        WCHAR wszName[MAX_PATH];
        bResult = SHGetNewLinkInfoW(pThunkText->m_pStr[0],
                                          pThunkText->m_pStr[1],
                                          wszName, pfMustCopy, uFlags);
        LocalFree(pThunkText);
        if (bResult)
        {
            if (0 == WideCharToMultiByte(CP_ACP, 0, wszName, -1,
                                         pszName, MAX_PATH, NULL, NULL))
            {
                SetLastError((DWORD)E_FAIL);    // BUGBUG - need better error value
                bResult = FALSE;
            }
        }
    }
    return bResult;
}

#else

STDAPI_(BOOL) SHGetNewLinkInfoW(LPCWSTR pszpdlLinkTo, LPCWSTR pszDir, LPWSTR pszName,
                                BOOL *pfMustCopy, UINT uFlags)
{
    return FALSE;
}
#endif

//
// in:
//      pidlTo

STDAPI CreateLinkToPidl(LPCITEMIDLIST pidlTo, LPCTSTR pszDir, LPITEMIDLIST *ppidl, UINT uFlags)
{
    HRESULT hr = E_FAIL;
    TCHAR szPathDest[MAX_PATH];
    BOOL fCopyLnk;
    BOOL fUseLinkTemplate = (SHCL_USETEMPLATE & uFlags);

    if (SHGetNewLinkInfo((LPTSTR)pidlTo, pszDir, szPathDest, &fCopyLnk,
                         fUseLinkTemplate ? SHGNLI_PIDL | SHGNLI_PREFIXNAME : SHGNLI_PIDL))
    {
        TCHAR szPathSrc[MAX_PATH];
        IShellLink *psl = NULL;

        DWORD dwAttributes = SFGAO_FILESYSTEM | SFGAO_FOLDER;
        SHGetNameAndFlags(pidlTo, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, szPathSrc, ARRAYSIZE(szPathSrc), &dwAttributes);

        if (fCopyLnk)
        {
            // if it is file system and not a folder (CopyFile does not work on folders)
            // just copy it.
            if (((dwAttributes & (SFGAO_FILESYSTEM | SFGAO_FOLDER)) == SFGAO_FILESYSTEM) &&
                CopyFile(szPathSrc, szPathDest, TRUE))
            {
                TouchFile(szPathDest);

                SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szPathDest, NULL);
                SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATH, szPathDest, NULL);
                hr = S_OK;
            }
            else
            {
                // load the source object that will be "copied" below (with the ::Save call)
                hr = SHGetUIObjectFromFullPIDL(pidlTo, NULL, &IID_IShellLink, (void **)&psl);
            }
        } 
        else
        {
            hr = SHCoCreateInstance(NULL, uFlags & SHCL_MAKEFOLDERSHORTCUT ?
                &CLSID_FolderShortcut : &CLSID_ShellLink, NULL, &IID_IShellLink, (void**)&psl);
            if (SUCCEEDED(hr))
            {
                hr = psl->lpVtbl->SetIDList(psl, pidlTo);
                // set the working directory to the same path
                // as the file we are linking too
                if (szPathSrc[0] && ((dwAttributes & (SFGAO_FILESYSTEM | SFGAO_FOLDER)) == SFGAO_FILESYSTEM))
                {
                    PathRemoveFileSpec(szPathSrc);
                    psl->lpVtbl->SetWorkingDirectory(psl, szPathSrc);
                }
            }
        }

        if (psl)
        {
            if (SUCCEEDED(hr))
            {
                IPersistFile *ppf;
                hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf);
                if (SUCCEEDED(hr))
                {
                    USES_CONVERSION;
                    hr = ppf->lpVtbl->Save(ppf, T2CW(szPathDest), TRUE);
                    if (SUCCEEDED(hr))
                    {
                        // in case ::Save translated the name of the 
                        // file (.LNK -> .PIF, or Folder Shortcut)
                        WCHAR *pwsz;
                        if (SUCCEEDED(ppf->lpVtbl->GetCurFile(ppf, &pwsz)) && pwsz)
                        {
                            SHUnicodeToTChar(pwsz, szPathDest, ARRAYSIZE(szPathDest));
                            SHFree(pwsz);
                        }
                    }
                    ppf->lpVtbl->Release(ppf);
                }
            }
            psl->lpVtbl->Release(psl);
        }
    }

    if (ppidl)
    {
        *ppidl = SUCCEEDED(hr) ? SHSimpleIDListFromPath(szPathDest) : NULL;
    }
    return hr;
}


// in/out:
//      pszDir         inital folder to try, output new folder (desktop)
// out:
//      ppidl          optional output PIDL of thing created

HRESULT _CreateLinkRetryDesktop(HWND hwnd, LPCITEMIDLIST pidlTo, LPTSTR pszDir, UINT fFlags, LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    if (ppidl)
        *ppidl = NULL;          // assume error

    if (*pszDir && (fFlags & SHCL_CONFIRM))
    {
        hr = CreateLinkToPidl(pidlTo, pszDir, ppidl, fFlags);
    }
    else
    {
        hr = E_FAIL;
    }

    // if we were unable to save, ask user if they want us to
    // try it again but change the path to the desktop.

    if (FAILED(hr))
    {
        int id;

        if (hr == STG_E_MEDIUMFULL)
        {
            DebugMsg(TF_ERROR, TEXT("failed to create link because disk is full"));
            id = IDYES;
        }
        else
        {
            if (fFlags & SHCL_CONFIRM)
            {
                id = _PromptTryDesktopLinks(hwnd, pszDir, (fFlags & SHCL_CONFIRM));
            }
            else
            {
                id = (SUCCEEDED(SHGetSpecialFolderPath(hwnd, pszDir, CSIDL_DESKTOPDIRECTORY, FALSE))) ? IDYES : IDNO;
            }

            if (id == IDYES && *pszDir)
            {
                hr = CreateLinkToPidl(pidlTo, pszDir, ppidl, fFlags);
            }
        }

        //  we failed to create the link complain to the user.
        if (FAILED(hr) && id != IDNO)
        {
            ShellMessageBox(HINST_THISDLL, hwnd,
                            MAKEINTRESOURCE(IDS_CANNOTCREATELINK),
                            MAKEINTRESOURCE(IDS_LINKTITLE),
                            MB_OK | MB_ICONASTERISK);
        }
    }

#ifdef DEBUG
    if (FAILED(hr) && ppidl)
        ASSERT(*ppidl == NULL);
#endif

    return hr;
}

//
// This function creates links to the stuff in the IDataObject
//
// Arguments:
//  hwnd        for any UI
//  pszDir      optional target directory (where to create links)
//  pDataObj    data object describing files (array of idlist)
//  ppidl       optional pointer to an array that receives pidls pointing to the new links
//              or NULL if not interested
STDAPI SHCreateLinks(HWND hwnd, LPCTSTR pszDir, IDataObject *pDataObj, UINT fFlags, LPITEMIDLIST* ppidl)
{
    DECLAREWAITCURSOR;
    STGMEDIUM medium;
    HRESULT hr;
    LPIDA pida;

    SetWaitCursor();

    pida = DataObj_GetHIDA(pDataObj, &medium);
    if (pida)
    {
        UINT i;
        TCHAR szTargetDir[MAX_PATH];

        szTargetDir[0] = 0;

        if (pszDir)
            lstrcpyn(szTargetDir, pszDir, ARRAYSIZE(szTargetDir));

        if (!(fFlags & SHCL_USEDESKTOP))
            fFlags |= SHCL_CONFIRM;

        for (i = 0; i < pida->cidl; i++)
        {
            LPITEMIDLIST pidlTo = IDA_ILClone(pida, i);
            if (pidlTo)
            {
                hr = _CreateLinkRetryDesktop(hwnd, pidlTo, szTargetDir, fFlags, ppidl ? &ppidl[i] : NULL);

                ILFree(pidlTo);

                if (FAILED(hr))
                    break;
            }
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    else
        hr = E_OUTOFMEMORY;

    SHChangeNotifyHandleEvents();
    ResetWaitCursor();

    return hr;
}

int FSSortIDToICol(int x)
{
    return x - FSIDM_SORT_FIRST + FS_ICOL_NAME;
}

//
// right click context menu on the background handler
//
// Returns:
//      S_OK, if successfully processed.
//      S_FALSE, if default code should be used.
//
STDAPI _BackgroundMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = S_OK;
    static const QCMINFO_IDMAP idMap =
    {2,
        {
            {FSIDM_FOLDER_SEP,QCMINFO_PLACE_BEFORE},
            {FSIDM_VIEW_SEP,QCMINFO_PLACE_AFTER}
        }
    };

    switch(uMsg) {
    case DFM_MERGECONTEXTMENU:
        {
            int idMenu = POPUP_FSVIEW_BACKGROUND;
            // nt5:169740 (adp 980612)
            // need to do this for file menu too (so that File.New finds
            // the FSIDM_FOLDER_SEP etc. markers which merge it into the
            // right place).
            //
            // reljai thinks he put in the CMF_DVFILE stuff (which breaks
            // the merge) to workaround a lamadio tmp hack.
            //
            // also for a more complete fix we might want to (should?) do
            // our own background+selection menu-merge in explorer band's
            // OnInitMenuPopup (like defview does).

            // (lamadio) 6.25.98: The fix above adds extra hmenu baggage.
            // What they really intended to do was merge in the named 
            // seperators that demote position within the menu. I've
            // created a menu that does not have an excess baggage
            if (wParam & CMF_DVFILE) //In the case of the file menu
                idMenu = POPUP_FSVIEW_BACKGROUND_MERGE;
                
            CDefFolderMenu_MergeMenu(HINST_THISDLL, idMenu,
                    POPUP_FSVIEW_POPUPMERGE, (LPQCMINFO)lParam);

            ((LPQCMINFO)lParam)->pIdMap = &idMap;
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_VALIDATECMD:
        switch (wParam)
        {
        case DFM_CMD_NEWFOLDER:
            break;

        default:
            hres = S_FALSE;
        }
        break;

    case DFM_INVOKECOMMAND:
        {
            CFSFolder *this = FS_GetFSFolderFromShellFolder(psf);

            switch(wParam)
            {
            case FSIDM_SORTBYNAME:
            case FSIDM_SORTBYSIZE:
            case FSIDM_SORTBYTYPE:
            case FSIDM_SORTBYDATE:
                ShellFolderView_ReArrange(hwnd, FSSortIDToICol((int)wParam));
                break;

            case FSIDM_PROPERTIESBG:
                hres = SHPropertiesForPidl(hwnd, this->_pidl, (LPCTSTR)lParam);
                break;

            default:
                // This is one of view menu items, use the default code.
                hres = S_FALSE;
                break;
            }
        }

        break;

    default:
        hres = E_NOTIMPL;
        break;
    }

    return hres;
}

STDMETHODIMP CFSFolder_CreateViewObject(IShellFolder2 *psf, HWND hwnd, REFIID riid, void **ppv)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    HRESULT hres;

    if (IsEqualIID(riid, &IID_IShellView) || IsEqualIID(riid, &IID_IDropTarget))
    {
        TCHAR szPath[MAX_PATH];
        DWORD dwRest;        

        hres = CFSFolder_GetPath(this, szPath);
        if (FAILED(hres))
            return hres;

        dwRest = SHRestricted(REST_NOVIEWONDRIVE);
        if (dwRest)
        {
            int iDrive = PathGetDriveNumber(szPath);
            if (iDrive != -1)
            {
                // is the drive restricted
                if (dwRest & (1 << iDrive))
                {
                    if (hwnd)
                    {
                        ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_RESTRICTIONS),
                                        MAKEINTRESOURCE(IDS_RESTRICTIONSTITLE), MB_OK|MB_ICONSTOP);
                    }
                    return E_ACCESSDENIED;
                }
            }
        }

        // Cache the view CLSID if not cached.
        //
        if (!this->fCachedCLSID)
        {
            if (CFSFolder_Attributes(this) & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
                this->fHasCLSID = _GetFolderCLSID(szPath, this->_pszNetProvider, &this->_clsidView, TEXT("UICLSID"));
            this->fCachedCLSID = TRUE;
        }

        //
        // Use the view handler if it exists.
        //
        if (this->fHasCLSID)
        {
            IPersistFolder *ppf;
            hres = SHExtCoCreateInstance(NULL, &this->_clsidView, NULL, &IID_IPersistFolder, &ppf);

            DebugMsg(TF_FSTREE, TEXT("CFSFolder::CreateViewObject created a view instance for a CLSID (%x)"), hres);

            if (SUCCEEDED(hres))
            {
                hres = ppf->lpVtbl->Initialize(ppf, this->_pidl);

                if (FAILED(hres))
                {
                    // It may have failed because the pidl is not a FSFolder, create another pidl from the path
                    // this was required for the Fonts FolderShortcut in the ControlPanel (stephstm)

                    LPITEMIDLIST pidl = NULL;

                    pidl = ILCreateFromPath(szPath);

                    if (pidl)
                    {
                        hres = ppf->lpVtbl->Initialize(ppf, pidl);

                        ILFree(pidl);
                    }
                }

                if (SUCCEEDED(hres))
                {
                    hres = ppf->lpVtbl->QueryInterface(ppf, riid, ppv);
                }
                ppf->lpVtbl->Release(ppf);
            }

            if (SUCCEEDED(hres))
            {
                DebugMsg(TF_FSTREE, TEXT("external code supplied IShellView"));
                return hres;
            }
        }

        if (IsEqualIID(riid, &IID_IDropTarget))
        {
            return CFSDropTarget_CreateInstance(this, hwnd, (IDropTarget **)ppv);
        }
        else
        {
            SFV_CREATE csfv;
            LONG lEvents = SHCNE_DISKEVENTS | SHCNE_ASSOCCHANGED | SHCNE_NETSHARE | SHCNE_NETUNSHARE;
            HWND hwndTree;
            IShellBrowser *psb = FileCabinet_GetIShellBrowser(hwnd);

            // WARNING: shell32.dll shipped in IE 4.01 with a bug that calling
            //     IShellFolder::CreateViewObject(hwnd, IID_IShellView) for FS folders, will crash
            //     if the hwnd provided didn't respond to CWM_GETISHELLBROWSER with a valid
            //     IShellBrowser interface pointer.  (FileCabinet_GetIShellBrowser() gets it)
            //     If you hit this assert, you need to verify you work correctly in this case
            //     or your code doesn't call the IE4 SI's shell32.
            //     Don't remove this comment, but you can comment it out.
            // ASSERT(psb); (dosent apply for win2k, since we ship shell32.dll)

            // if in explorer mode, we want to register for freespace changes too
            if (psb && SUCCEEDED(psb->lpVtbl->GetControlWindow(psb, FCW_TREE, &hwndTree)) && hwndTree)
            {
                lEvents |= SHCNE_FREESPACE;
            }

            csfv.cbSize   = sizeof(SFV_CREATE);
            csfv.psvOuter = NULL;

            hres = psf->lpVtbl->QueryInterface(psf, &IID_IShellFolder, (void **)&csfv.pshf);
            if (SUCCEEDED(hres))
            {
                CFSFolderCallback_Create((IShellFolder *)psf, this, lEvents, &csfv.psfvcb);
                hres = SHCreateShellFolderView(&csfv, (IShellView**)ppv);
                csfv.pshf->lpVtbl->Release(csfv.pshf);
                if (csfv.psfvcb)
                    csfv.psfvcb->lpVtbl->Release(csfv.psfvcb);
            }
            return hres;
        }
    }
    else if (IsEqualIID(riid, &IID_IContextMenu))
    {
        // do background menu.
        IShellFolder *psfToPass;        // May be an Aggregate...
        hres = psf->lpVtbl->QueryInterface(psf, &IID_IShellFolder, (void **)&psfToPass);
        if (SUCCEEDED(hres))
        {
            HKEY hkNoFiles;
            RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Directory\\Background"), &hkNoFiles);
            hres = CDefFolderMenu_Create2(this->_pidl, hwnd,
                    0, NULL, psfToPass, _BackgroundMenuCB,
                    1, &hkNoFiles, (IContextMenu **)ppv);
            psfToPass->lpVtbl->Release(psfToPass);
            if (hkNoFiles)                          // CDefFolderMenu_Create can handle NULL ok
                RegCloseKey(hkNoFiles);
        }
        return hres;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

HRESULT FS_CompareNames(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    TCHAR szName1[MAX_PATH];        // We need to have a subfunction
    TCHAR szName2[MAX_PATH];        // because of this large stack usage

    // Sort it based on the primary (long) name -- ignore case.
    FS_CopyName(pidf1, szName1, ARRAYSIZE(szName1));
    FS_CopyName(pidf2, szName2, ARRAYSIZE(szName2));

    return ResultFromShort(ustrcmpi(szName1,szName2));
}

// this does the case sensitive or insensitive test as needed
// based on if the items are real or not

HRESULT FS_CompareNamesCase(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    HRESULT hres;
    TCHAR szName1[MAX_PATH], szName2[MAX_PATH];

    FS_CopyName(pidf1, szName1, ARRAYSIZE(szName1));
    FS_CopyName(pidf2, szName2, ARRAYSIZE(szName2));

    hres = ResultFromShort(ustrcmpi(szName1, szName2));

    //
    //  This block of code is added to support pseudo IDList
    // which does not have the alternate name. If only one
    // of idlists is (or at least looks like) such a IDList,
    // we compare its name with the alternate of the other.
    //
    if (hres != ResultFromShort(0))
    {
        // if one or the other but not both are real ids
        if (FS_IsReal(pidf1) ^ FS_IsReal(pidf2))
        {
            // try the alternate name on the real id
            if (FS_IsReal(pidf1))
                FS_CopyAltName(pidf1, szName1, ARRAYSIZE(szName1));
            else
                FS_CopyAltName(pidf2, szName2, ARRAYSIZE(szName2));

            if (ustrcmpi(szName1, szName2) == 0)
                hres = ResultFromShort(0);
        }
    }
    else if (FS_IsReal(pidf1) && FS_IsReal(pidf2))
    {
        // If both are real and if they compared the same in the case
        // INsensitive search, try case sensitive search
        hres = ResultFromShort(ustrcmp(szName1, szName2));
    }
    return hres;
}

short _CompareFileTypes(IShellFolder *psf, LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    LPCTSTR psz1, psz2;
    short result = 0;

    ENTERCRITICAL;

    psz1 = _GetTypeName(pidf1);
    psz2 = _GetTypeName(pidf2);

    if (psz1 != psz2)
        result = (short) ustrcmpi(psz1, psz2);

    LEAVECRITICAL;

    return result;
}

HRESULT FS_CompareModifiedDate(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{

    if ((DWORD)MAKELONG(pidf1->fs.timeModified, pidf1->fs.dateModified) <
        (DWORD)MAKELONG(pidf2->fs.timeModified, pidf2->fs.dateModified))
    {
        return ResultFromShort(-1);
    }
    if ((DWORD)MAKELONG(pidf1->fs.timeModified, pidf1->fs.dateModified) >
        (DWORD)MAKELONG(pidf2->fs.timeModified, pidf2->fs.dateModified))
    {
        return ResultFromShort(1);
    }

    return ResultFromShort(0);
}

HRESULT FS_CompareAttribs(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    DWORD mask = FILE_ATTRIBUTE_READONLY  |
                 FILE_ATTRIBUTE_HIDDEN    |
                 FILE_ATTRIBUTE_SYSTEM    |
                 FILE_ATTRIBUTE_ARCHIVE   |
                 FILE_ATTRIBUTE_COMPRESSED|
                 FILE_ATTRIBUTE_ENCRYPTED |
                 FILE_ATTRIBUTE_OFFLINE;

    //
    // Calculate value of desired bits in attribute DWORD.
    //
    DWORD dwValueA = pidf1->fs.wAttrs & mask;
    DWORD dwValueB = pidf2->fs.wAttrs & mask;

    if (dwValueA != dwValueB)
    {
        //
        // If the values are not equal,
        // sort alphabetically based on string representation.
        //
        int diff = 0;
        TCHAR szTempA[ARRAYSIZE(g_adwAttributeBits) + 1];
        TCHAR szTempB[ARRAYSIZE(g_adwAttributeBits) + 1];

        //
        // Create attribute string for objects A and B.
        //
        BuildAttributeString(pidf1->fs.wAttrs, szTempA, ARRAYSIZE(szTempA));
        BuildAttributeString(pidf2->fs.wAttrs, szTempB, ARRAYSIZE(szTempB));

        //
        // Compare attribute strings and determine difference.
        //
        diff = ustrcmp(szTempA, szTempB);

        if (diff > 0)
           return ResultFromShort(1);
        if (diff < 0)
           return ResultFromShort(-1);
    }
    return ResultFromShort(0);
}

STDAPI FS_CompareFolderness(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)
{
    if (FS_IsReal(pidf1) && FS_IsReal(pidf2))
    {
        // Always put the folders first
        if (FS_IsFolder(pidf1))
        {
            if (!FS_IsFolder(pidf2))
                return ResultFromShort(-1);
        }
        else if (FS_IsFolder(pidf2))
            return ResultFromShort(1);
    }
    return ResultFromShort(0);    // same
}

HRESULT FS_CompareExtendedDates (CFSFolder *this, LPARAM lParam, LPCIDFOLDER pidf1, LPCIDFOLDER pidf2);

STDMETHODIMP CFSFolder_CompareIDs(IShellFolder2 *psf, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    HRESULT hres;
    short nCmp;
    LPCIDFOLDER pidf1 = FS_IsValidID(pidl1);
    LPCIDFOLDER pidf2 = FS_IsValidID(pidl2);

    if (!pidf1 || !pidf2)
    {
        // ASSERT(0);      // we hit this often... who is the bad guy?
        return E_INVALIDARG;
    }

    hres = FS_CompareFolderness(pidf1, pidf2);
    if (hres != ResultFromShort(0))
        return hres;

    // SHCIDS_ALLFIELDS means to compare absolutely, ie: even if only filetimes
    // are different, we rule file pidls to be different

    switch (lParam & SHCIDS_COLUMNMASK)
    {
    case FS_ICOL_SIZE:
        {
            ULONGLONG ull1, ull2;

            FS_GetSize(this->_pidl, pidf1, &ull1);
            FS_GetSize(this->_pidl, pidf2, &ull2);

            if (ull1 < ull2)
                return ResultFromShort(-1);
            if (ull1 > ull2)
                return ResultFromShort(1);
        }
        goto DoDefault;

    case FS_ICOL_TYPE:
        nCmp = _CompareFileTypes((IShellFolder *)psf, pidf1, pidf2);
        if (nCmp)
            return ResultFromShort(nCmp);
        goto DoDefault;

    case FS_ICOL_MODIFIED:
        hres = FS_CompareModifiedDate(pidf1, pidf2);
        if (!hres)
            goto DoDefault;
        break;

    case FS_ICOL_NAME:
        hres = FS_CompareNamesCase(pidf1, pidf2);

        // REVIEW: (Possible performance gain with some extra code)
        //   We should probably aviod bindings by walking down
        //  the IDList here instead of calling this helper function.
        //
        if (hres == ResultFromShort(0))
        {
            // pidl1 is not simple
            hres = ILCompareRelIDs((IShellFolder *)psf, pidl1, pidl2);
            goto DoDefaultModification;
        }
        break;

    case FS_ICOL_ATTRIB:
        hres = FS_CompareAttribs(pidf1, pidf2);
        if (hres)
            return hres;

        goto DoDefault;

    default:
        {
            int     iColumn;

            iColumn = ((DWORD)lParam & SHCIDS_COLUMNMASK) - ARRAYSIZE(c_fs_cols);

            // 99/03/24 #295631 vtan: If not one of the standard columns then
            // it's probably an extended column. Make a check for dates.

            // 99/05/18 #341468 vtan: But also fail if it is an extended column
            // because this implementation of IShellFolder::CompareIDs only
            // understands basic file system columns and extended date columns.

            hres = FS_CompareExtendedDates(this, lParam, pidf1, pidf2);
            if ((iColumn >= 0) || (SUCCEEDED(hres) && ((short)HRESULT_CODE(hres) != 0)))
                return(hres);
        }
DoDefault:
        hres = FS_CompareNames(pidf1, pidf2);
    }

DoDefaultModification:

    // If they were equal so far, but the caller wants SHCIDS_ALLFIELDS,
    // then look closer.
    if (hres == S_OK && (lParam & SHCIDS_ALLFIELDS)) 
    {
        // Must sort by modified date to pick up any file changes!
        hres = FS_CompareModifiedDate(pidf1, pidf2);
        if (!hres)
            hres = FS_CompareAttribs(pidf1, pidf2);
    }

    return hres;
}


//
// REVIEW: This code must be in ultrootx.c
//
BOOL CFSFolder_IsNetPath(LPCITEMIDLIST pidlAbs)
{
    if (ILIsEmpty(pidlAbs))
        return FALSE;       // desktop is local

    if (IsIDListInNameSpace(pidlAbs, &CLSID_NetworkPlaces))
        return TRUE;

    if (IsIDListInNameSpace(pidlAbs, &CLSID_MyComputer))
    {
        LPCITEMIDLIST pidlDrive = _ILNext(pidlAbs);
        ASSERT(!ILIsEmpty(pidlDrive));
        return SIL_GetType(pidlDrive) == SHID_COMPUTER_NETDRIVE;
    }
    return FALSE;   // local
}

#ifdef WINNT
//
// see if the pidf is a strange network junction. those things are really slow
//

BOOL CFSFolder_IsDfsJP(CFSFolder *this, LPCIDFOLDER pidf)
{
    BOOL bIsJP = FALSE;
    WCHAR wszPath[MAX_PATH];
    UNICODE_STRING str;

    if (SUCCEEDED(CFSFolder_GetPathForItemW(this, pidf, wszPath)) &&
        RtlDosPathNameToNtPathName_U(wszPath, &str, NULL, NULL))
    {
        IO_STATUS_BLOCK     iosb;
        OBJECT_ATTRIBUTES   oa;
        NTSTATUS            status;
        HANDLE              fh;
        CHAR EaBuffer[ SIZEOF(FILE_FULL_EA_INFORMATION) + SIZEOF(EA_NAME_OPENIFJP) ];
        PFILE_FULL_EA_INFORMATION pOpenIfJPEa = (PFILE_FULL_EA_INFORMATION) EaBuffer;

        pOpenIfJPEa->NextEntryOffset = 0;
        pOpenIfJPEa->Flags = 0;
        pOpenIfJPEa->EaNameLength = (UCHAR) lstrlenA(EA_NAME_OPENIFJP);
        pOpenIfJPEa->EaValueLength = 0;
        lstrcpyA(pOpenIfJPEa->EaName, EA_NAME_OPENIFJP);

        InitializeObjectAttributes(&oa, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);

        //
        // Do a generic read open.  If it fails with STATUS_DFS_EXIT_PATH_FOUND, we
        // are dealing with a junction point.
        //

        status = NtCreateFile(&fh,
                     FILE_GENERIC_READ | FILE_GENERIC_WRITE | SYNCHRONIZE | DELETE,
                     &oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, pOpenIfJPEa, SIZEOF(EaBuffer));

        if ( NT_SUCCESS(status) )
        {
            NtClose(fh);
        }
        else if (status == STATUS_DFS_EXIT_PATH_FOUND)
        {
            bIsJP = TRUE;
        }
        RtlFreeUnicodeString(&str);
    }
    return bIsJP;
}
#else
#define CFSFolder_IsDfsJP(pidlFolder, pidfSubFolder)    (FALSE)
#endif


HKEY SHOpenShellFolderKey(const CLSID *pclsid)
{
    HKEY hkey;
    return SUCCEEDED(SHRegGetCLSIDKey(pclsid, TEXT("ShellFolder"), FALSE, FALSE, &hkey)) ? hkey : NULL;
}

BOOL SHQueryShellFolderValue(const CLSID *pclsid, LPCTSTR pszValueName)
{
    BOOL bRet = FALSE;      // assume no
    HKEY hkey = SHOpenShellFolderKey(pclsid);
    if (hkey)
    {
        DWORD cbSize;
        bRet = SHQueryValueEx(hkey, pszValueName, NULL, NULL, NULL, &cbSize) == ERROR_SUCCESS;
        RegCloseKey(hkey);
    }
    return bRet;
}


BOOL _IsNonEnumPolicySet(const CLSID *pclsid)
{
    BOOL fPolicySet = FALSE;
    TCHAR szCLSID[GUIDSTR_MAX];
    DWORD dwDefault = 0;
    DWORD dwPolicy = 0;
    DWORD cbSize = sizeof(dwPolicy);

    if (EVAL(SHStringFromGUID(pclsid, szCLSID, ARRAYSIZE(szCLSID))) &&
       (ERROR_SUCCESS == SHRegGetUSValue(SZ_REGKEY_MYCOMPUTER_NONENUM_POLICY, szCLSID, NULL, &dwPolicy, &cbSize, FALSE, &dwDefault, sizeof(dwDefault))) &&
       dwPolicy)
    {
        fPolicySet = TRUE;
    }

    return fPolicySet;
}

//
//  This function returns the attributes (to be returned IShellFolder::
// GetAttributesOf) of the junction point specified by the class ID.
//
DWORD SHGetAttributesFromCLSID(const CLSID *pclsid, DWORD dwDefault)
{
    return SHGetAttributesFromCLSID2(pclsid, dwDefault, (DWORD)-1);
}

DWORD QueryCallForAttributes(HKEY hkey, const CLSID *pclsid, DWORD dwDefAttrs, DWORD dwRequested)
{
    DWORD dwAttr = dwDefAttrs;
    DWORD dwData, cbSize = SIZEOF(dwAttr);
    // BUGBUG: we don't init this folder. maybe we should
    // also consider caching this folder to avoid creating over and over
    // mydocs.dll uses this for compat with old apps

    // See if this folder has asked us specifically to call and get
    // the attributes...
    //
    if (SHQueryValueEx(hkey, TEXT("CallForAttributes"), NULL, NULL, &dwData, &cbSize) == ERROR_SUCCESS)
    {
        IShellFolder *psf;

        // CallForAttributes can be a masked value. See if it's being supplied in the value.
        // NOTE: MyDocs.dll registers with a NULL String, so this check works.
        DWORD dwMask = (DWORD)-1;
        if (sizeof(dwData) == cbSize)
        {
            // There is a mask, Use this.
            dwMask = dwData;
        }

        // Is the requested bit contained in the specified mask?
        if (dwMask & dwRequested)
        {
            // Yes. Then CoCreate and Query.
            if (SUCCEEDED(SHExtCoCreateInstance(NULL, pclsid, NULL, &IID_IShellFolder, &psf)))
            {
                dwAttr = dwRequested;
                psf->lpVtbl->GetAttributesOf(psf, 0, NULL, &dwAttr);
                psf->lpVtbl->Release(psf);
            }
            else
            {
                 dwAttr |= SFGAO_FILESYSTEM;
            }
        }
    }

    return dwAttr;
}

DWORD SHGetAttributesFromCLSID2(const CLSID *pclsid, DWORD dwDefAttrs, DWORD dwRequested)
{
    DWORD dwAttr = dwDefAttrs;
    HKEY hkey = SHOpenShellFolderKey(pclsid);
    if (hkey)
    {
        DWORD dwData, cbSize = SIZEOF(dwAttr);

        // We are looking for some attributes on a shell folder. These attributes can be in two locations:
        // 1) In the "Attributes" value in the registry.
        // 2) Stored in a the shell folder's GetAttributesOf.

        // First, Check to see if the reqested value is contained in the registry.
        if (SHQueryValueEx(hkey, TEXT("Attributes"), NULL, NULL, (BYTE *)&dwData, &cbSize) == ERROR_SUCCESS &&
            cbSize == SIZEOF(dwData))
        {
            // We have data there, but it may not contain the data we are looking for
            dwAttr = dwData & dwRequested;

            // Does it contain the bit we are looking for?
            if (((dwAttr & dwRequested) != dwRequested) && dwRequested != 0)
            {
                // No. Check to see if it is in the shell folder implementation
                goto CallForAttributes;
            }
        }
        else
        {
CallForAttributes:
            // See if we have to talk to the shell folder.
            // I'm passing dwAttr, because if the previous case did not generate any attributes, then it's
            // equal to dwDefAttrs. If the call to CallForAttributes fails, then it will contain the value of
            // dwDefAttrs or whatever was in the shell folder's Attributes key
            dwAttr = QueryCallForAttributes(hkey, pclsid, dwAttr, dwRequested);
        }

        RegCloseKey(hkey);
    }

    if (_IsNonEnumPolicySet(pclsid))
        dwAttr |= SFGAO_NONENUMERATED;

    if (SHGetObjectCompatFlags(NULL, pclsid) & OBJCOMPATF_NOTAFILESYSTEM)
        dwAttr &= ~SFGAO_FILESYSTEM;

    return dwAttr;
}

STDAPI_(LPCIDFOLDER) FS_IsValidIDHack(LPCITEMIDLIST pidl)
{
    if (!(ACF_NOVALIDATEFSIDS & SHGetAppCompatFlags(ACF_NOVALIDATEFSIDS)))
    {
        return FS_IsValidID(pidl);
    }
    else if (pidl)
    {
        //  old behavior was that we didnt validate, we just
        //  looked for the last id and casted it
        return (LPCIDFOLDER)ILFindLastID(pidl);
    }
    return NULL;
}

#define SFGAO_NOT_RECENT    (SFGAO_CANRENAME | SFGAO_CANLINK)
#define SFGAO_REQ_MASK      (SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR | SFGAO_CANMONIKER | SFGAO_FOLDER | SFGAO_DROPTARGET | SFGAO_LINK)

STDMETHODIMP CFSFolder_GetAttributesOf(IShellFolder2 *psf, UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    LPCIDFOLDER pidf = cidl ? FS_IsValidIDHack(apidl[0]) : NULL;

    ULONG rgfOut = SFGAO_CANDELETE | SFGAO_CANMOVE | SFGAO_CANCOPY | SFGAO_HASPROPSHEET 
                    | SFGAO_FILESYSTEM | SFGAO_DROPTARGET | SFGAO_CANRENAME | SFGAO_CANLINK | SFGAO_CANMONIKER;

    ASSERT(cidl ? apidl[0] == ILFindLastID(apidl[0]) : TRUE); // should be single level IDs only
    ASSERT(cidl ? BOOLFROMPTR(pidf) : TRUE); // should always be FS PIDLs

    //  the RECENT folder doesnt like items in it renamed or linked to.
    if ((*prgfInOut & (SFGAO_NOT_RECENT)) &&
        CFSFolder_IsCSIDL(this, CSIDL_RECENT))
    {
        rgfOut &= ~SFGAO_NOT_RECENT;
    }
        
    if (cidl == 1 && pidf)
    {
        TCHAR szPath[MAX_PATH];
        CLSID clsid;
        HRESULT hr;

        hr = CFSFolder_GetPathForItem(this, pidf, szPath);
        if (FAILED(hr))
            return hr;

        if (*prgfInOut & SFGAO_VALIDATE)
        {
            DWORD dwAttribs;
            if (!PathFileExistsAndAttributes(szPath, &dwAttribs))
                return E_FAIL;

            // hackhack.  if they pass in validate, we party into it and update
            // the attribs
            if (IS_VALID_WRITE_PTR(&pidf->fs, DWORD))
                ((LPIDFOLDER)pidf)->fs.wAttrs = (WORD)dwAttribs;
        }

        if (*prgfInOut & SFGAO_COMPRESSED)
        {
            if (pidf->fs.wAttrs & FILE_ATTRIBUTE_COMPRESSED)
            {
                rgfOut |= SFGAO_COMPRESSED;
            }
        }

        if (*prgfInOut & SFGAO_READONLY)
        {
            if (pidf->fs.wAttrs & FILE_ATTRIBUTE_READONLY)
            {
                rgfOut |= SFGAO_READONLY;
            }
        }

        if (FS_IsFolder(pidf))
            rgfOut |= SFGAO_FOLDER | SFGAO_FILESYSANCESTOR;

        if (*prgfInOut & SFGAO_LINK)
        {
            DWORD dwFlags = SHGetClassFlags(pidf);
            if (dwFlags & SHCF_IS_LINK)
            {
                rgfOut |= SFGAO_LINK;
            }
        }

        if (FS_GetCLSID(pidf, &clsid))
        {
            // NOTE: here we are always including SFGAO_FILESYSTEM. this was not the original
            // shell behavior. but since these things will succeeded on SHGetPathFromIDList()
            // it is the right thing to do. to filter out SFGAO_FOLDER things that might 
            // have files in them use SFGAO_FILESYSANCESTOR.
            //
            // clear out the things we want the extension to be able to optionally have
            rgfOut &= ~(SFGAO_DROPTARGET | SFGAO_FILESYSANCESTOR | SFGAO_CANMONIKER);

            // let folder shortcuts yank the folder bit too for bad apps.
            if (IsEqualGUID (&clsid, &CLSID_FolderShortcut) &&
                (SHGetAppCompatFlags (ACF_FOLDERSCUTASLINK) & ACF_FOLDERSCUTASLINK))
                {
                rgfOut &= ~SFGAO_FOLDER;
                }

            // and let him add some bits in
            rgfOut |= SHGetAttributesFromCLSID2(&clsid, SFGAO_HASSUBFOLDER, SFGAO_REQ_MASK) & SFGAO_REQ_MASK;
            
            //Check if this folder needs File System Ancestor bit
            if (SHGetObjectCompatFlags(NULL,&clsid) & OBJCOMPATF_NEEDSFILESYSANCESTOR)
            {
                rgfOut |= SFGAO_FILESYSANCESTOR;
            }
        }

        // it can only have subfolders if we've first found it's a folder
        if ((*prgfInOut & SFGAO_HASSUBFOLDER) && (rgfOut & SFGAO_FOLDER))
        {
            if (CFSFolder_IsNetPath(this->_pidl) || CFSFolder_IsDfsJP(this, pidf))
            {
                rgfOut |= SFGAO_HASSUBFOLDER;   // assume yes because these are slow
            }
            else if (!(rgfOut & SFGAO_HASSUBFOLDER))
            {
                IShellFolder *psf;
                if (SUCCEEDED(FS_Bind(this, NULL, pidf, &IID_IShellFolder, &psf)))
                {
                    IEnumIDList *peunk;
                    if (SUCCEEDED(psf->lpVtbl->EnumObjects(psf, NULL, SHCONTF_FOLDERS, &peunk)))
                    {
                        LPITEMIDLIST pidlT;
                        if (peunk->lpVtbl->Next(peunk, 1, &pidlT, NULL) == S_OK)
                        {
                            rgfOut |= SFGAO_HASSUBFOLDER;
                            SHFree(pidlT);
                        }
                        peunk->lpVtbl->Release(peunk);
                    }
                    psf->lpVtbl->Release(psf);
                }
            }

            if (pidf->fs.wAttrs & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                rgfOut |= SFGAO_HASSUBFOLDER;
            }
        }

        if (FS_IsFolder(pidf))
        {
            if ((*prgfInOut & SFGAO_REMOVABLE) && PathIsRemovable(szPath))
            {
                rgfOut |= SFGAO_REMOVABLE;
            }

            if (*prgfInOut & SFGAO_SHARE)
            {
                if (IsShared(szPath, FALSE))
                    rgfOut |= SFGAO_SHARE;
            }
        }

        if (*prgfInOut & SFGAO_GHOSTED)
        {
            if (pidf->fs.wAttrs & FILE_ATTRIBUTE_HIDDEN)
                rgfOut |= SFGAO_GHOSTED;
        }

        if ((*prgfInOut & SFGAO_BROWSABLE) &&
            (FS_IsFile(pidf)) &&
            (SHGetClassFlags(pidf) & SHCF_IS_BROWSABLE))
        {
            rgfOut |= SFGAO_BROWSABLE;
        }
    }

    *prgfInOut = rgfOut;
    return S_OK;
}

// load handler for an item based on the handler type:
//     shellex\DropHandler, shellex\IconHandler, etc.
// in:
//      pidl            type of this object specifies the type of handler
//      pszHandlerType  handler type name "DropTarget", may be NULL
//      riid            interface to talk on
// out:
//      ppv             output object
//
// BUGBUG: callers who cast are bad

HRESULT FSLoadHandler(CFSFolder *this, LPCIDFOLDER pidf, LPCTSTR pszHandlerType, REFIID riid, void **ppv)
{
    HRESULT hres = E_FAIL;
    TCHAR szHandlerCLSID[40];   // enough for CLSID
    ULONG cbValue;
    TCHAR szIID[40];
    TCHAR szHandler[MAX_CLASS + 60];
    HKEY hkeyProgID = NULL;

    *ppv = NULL;

    FSGetClassKey(pidf, &hkeyProgID);

    // empty handler type, use the stringized IID as the handler name
    if (NULL == pszHandlerType)
    {
        SHStringFromGUID(riid, szIID, ARRAYSIZE(szIID));
        lstrcpy(szHandler, TEXT("ShellEx\\"));
        lstrcat(szHandler, szIID);
        pszHandlerType = szHandler;
    }

    szHandlerCLSID[0] = 0;

    if (hkeyProgID)
    {
        cbValue = SIZEOF(szHandlerCLSID);
        SHRegQueryValue(hkeyProgID, pszHandlerType, szHandlerCLSID, &cbValue);
    }

    if (szHandlerCLSID[0] == 0)
    {
        // try under the file extension if the ProgID is missing
        TCHAR szClass[MAX_CLASS];
        HKEY hkeyExt;

        SHGetClass(pidf, szClass, ARRAYSIZE(szClass));

        if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, szClass, &hkeyExt))
        {
            cbValue = SIZEOF(szHandlerCLSID);
            SHRegQueryValue(hkeyExt, pszHandlerType, szHandlerCLSID, &cbValue);
            if (hkeyProgID)
                RegCloseKey(hkeyProgID);

            hkeyProgID = hkeyExt;
        }
    }

    if (szHandlerCLSID[0])
    {
        IPersistFile *ppf;

        // ask for the interfaces in this order becuase CLSID_ShellLink
        // cheats on IDropTarget, it does not implement COM identity properly!

        hres = SHExtCoCreateInstance(szHandlerCLSID, NULL, NULL, &IID_IPersistFile, &ppf);
        if (SUCCEEDED(hres))
        {
            WCHAR wszPath[MAX_PATH];

            hres = CFSFolder_GetPathForItemW(this, pidf, wszPath);
            if (SUCCEEDED(hres))
            {
                hres = ppf->lpVtbl->Load(ppf, wszPath, STGM_READ);
                if (SUCCEEDED(hres))
                {
                    hres = ppf->lpVtbl->QueryInterface(ppf, riid, ppv);
                }
            }
            ppf->lpVtbl->Release(ppf);
        }
    }

    SHCloseClassKey(hkeyProgID);
    return hres;
}

//
// opens the CLSID key given the ProgID
//

HKEY SHOpenCLSID(HKEY hkeyProgID)
{
    HKEY hkeyCLSID = NULL;
    TCHAR szCLSID[MAX_CLASS];
    DWORD cb;

    lstrcpy(szCLSID, c_szCLSIDSlash);
    ASSERT(lstrlen(c_szCLSIDSlash) == 6);

    cb = SIZEOF(szCLSID)-6;
    if (SHRegQueryValue(hkeyProgID, c_szCLSID, szCLSID + 6, &cb) == ERROR_SUCCESS)
    {
        RegOpenKey(HKEY_CLASSES_ROOT, szCLSID, &hkeyCLSID);
    }

    return hkeyCLSID;
}

int CFSFolder_GetDefaultFolderIcon(CFSFolder *this)
{
    int iIcon = II_FOLDER;
    UINT csidlFolder = CFSFolder_GetCSIDL(this);

    // We're removing the icon distinction between per user and common folders.
    switch (csidlFolder)
    {
    case CSIDL_STARTMENU:
    case CSIDL_COMMON_STARTMENU:
    case CSIDL_PROGRAMS:
    case CSIDL_COMMON_PROGRAMS:
        iIcon = II_STSPROGS;
        break;
    }

    return iIcon;
}

STDAPI_(DWORD) CFSFolder_Attributes(CFSFolder *this)
{
    if (this->_dwAttributes == -1)
    {
        TCHAR szPath[MAX_PATH];

        if (SUCCEEDED(CFSFolder_GetPath(this, szPath)))
            this->_dwAttributes = GetFileAttributes(szPath);
        if (this->_dwAttributes == -1)
            this->_dwAttributes = FILE_ATTRIBUTE_DIRECTORY;     // assume this on failure
    }
    return this->_dwAttributes;
}

// This function creates a default IExtractIcon object for either
// a file or a junction point. We should not supposed to call this function
// for a non-junction point directory (we don't want to hit the disk!).

HRESULT CFSFolder_CreateDefExtIcon(CFSFolder *this, LPCIDFOLDER pidf, REFIID riid, void **ppxicon)
{
    HRESULT hres = E_OUTOFMEMORY;
    DWORD dwFlags;

    // WARNING: don't replace this if-statement with FS_IsFolder(pidf))!!!
    // otherwise all junctions (like briefcase) will get the Folder icon.
    //
    if (FS_IsFileFolder(pidf))
    {
        UINT iIcon = CFSFolder_GetDefaultFolderIcon(this);
        UINT iIconOpen = II_FOLDEROPEN;

        TCHAR szMountPoint[MAX_PATH];
        TCHAR szModule[MAX_PATH];

        szModule[0] = 0;

        if (FS_GetMountingPointInfo(this, pidf, szMountPoint, ARRAYSIZE(szMountPoint)))
        {
            // We want same icon for open and close moun point (kind of drive)
            iIconOpen = iIcon = GetMountedVolumeIcon(szMountPoint, szModule, ARRAYSIZE(szModule));
        }
        else
        {
            if (FS_IsSystemFolder(pidf))
            {
                TCHAR szPath[MAX_PATH];

                if (_GetFolderIconPath(this, pidf, szPath, ARRAYSIZE(szPath), &iIcon))
                {
                    return SHCreateDefExtIcon(szPath, iIcon, iIcon, GIL_PERINSTANCE, riid, ppxicon);
                }
            }
        }

        return SHCreateDefExtIcon(szModule[0] ? szModule : NULL, iIcon, iIconOpen, GIL_PERCLASS, riid, ppxicon);
    }


    //
    //  not a folder, get IExtractIcon and extract it.
    //  (might be a ds folder)
    //
    dwFlags = SHGetClassFlags(pidf);
    if (dwFlags & SHCF_ICON_PERINSTANCE)
    {
        if (dwFlags & SHCF_HAS_ICONHANDLER)
        {
            IUnknown *punk;
            hres = FSLoadHandler(this, pidf, c_szIconHandler, &IID_IUnknown, (void **)&punk);
            if (SUCCEEDED(hres))
            {
                hres = punk->lpVtbl->QueryInterface(punk, riid, (void **)ppxicon);
                punk->lpVtbl->Release(punk);
            }
            else
                *ppxicon = NULL;
        }
        else
        {
            DWORD uid = FS_GetUID(pidf);
            TCHAR szPath[MAX_PATH];
            hres = CFSFolder_GetPathForItem(this, pidf, szPath);
            if (SUCCEEDED(hres))
                hres = SHCreateDefExtIcon(szPath, uid, uid, GIL_PERINSTANCE | GIL_NOTFILENAME, riid, ppxicon);
        }
    }
    else
    {
        UINT iIcon = (dwFlags & SHCF_ICON_INDEX);
        if (II_FOLDER == iIcon)
            iIcon = CFSFolder_GetDefaultFolderIcon(this);
        hres = SHCreateDefExtIcon(c_szStar, iIcon, iIcon, GIL_PERCLASS | GIL_NOTFILENAME, riid, ppxicon);
    }
    return hres;
}


//
// This function is called from CFSIDLData_GetData().
//
// Paramters:
//  this    -- Specifies the IDLData object (selected objects)
//  pmedium -- Pointer to STDMEDIUM to be filled; NULL if just querying.
//
HRESULT CDesktopIDLData_GetNetResourceForFS(IDataObject *pdtobj, LPSTGMEDIUM pmedium)
{
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        BOOL bIsMyNet = IsIDListInNameSpace(IDA_GetIDListPtr(pida, (UINT)-1), &CLSID_NetworkPlaces);

        HIDA_ReleaseStgMedium(pida, &medium);

        if (!bIsMyNet)
            return DV_E_FORMATETC;

        if (!pmedium)
            return NOERROR; // query, yes we have it

        return CNETIDLData_GetNetResourceForFS(pdtobj, pmedium);
    }
    return E_FAIL;
}

// subclass member function to support CF_HDROP and CF_NETRESOURCE

HRESULT CFSIDLData_QueryGetData(IDataObject *pdtobj, LPFORMATETC pformatetc)
{
    if (pformatetc->cfFormat == CF_HDROP && (pformatetc->tymed & TYMED_HGLOBAL))
    {
        return S_OK; // same as S_OK
    }
    else if (pformatetc->cfFormat == g_cfFileName && (pformatetc->tymed & TYMED_HGLOBAL))
    {
        return S_OK;
    }
#ifdef UNICODE
    else if (pformatetc->cfFormat == g_cfFileNameW && (pformatetc->tymed & TYMED_HGLOBAL))
    {
        return S_OK;
    }
#endif
    else if (pformatetc->cfFormat == g_cfNetResource && (pformatetc->tymed & TYMED_HGLOBAL))
    {
        return CDesktopIDLData_GetNetResourceForFS(pdtobj, NULL);
    }

    return CIDLData_QueryGetData(pdtobj, pformatetc);
}

HRESULT CFSIDLData_SetData(IDataObject *pdtobj, FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    HRESULT hr = CIDLData_SetData(pdtobj, pformatetc, pmedium, fRelease);

    // this enables:
    //      1) in the shell "cut" some files
    //      2) in an app "paste" to copy the data
    //      3) here we complete the "cut" by deleting the files

    if ((pformatetc->cfFormat == g_cfPasteSucceeded) &&
        (pformatetc->tymed == TYMED_HGLOBAL))
    {
        DWORD *pdw = (DWORD *)GlobalLock(pmedium->hGlobal);
        if (pdw)
        {
            // NOTE: the old code use g_cfPerformedDropEffect == DROPEFFECT_MOVE here
            // so to work on downlevel shells be sure to set the "Performed Effect" before
            // using "Paste Succeeded".

            // complete the "unoptimized move"
            if (DROPEFFECT_MOVE == *pdw)
            {
                DeleteFilesInDataObject(NULL, CMIC_MASK_FLAG_NO_UI, pdtobj);
            }
            GlobalUnlock(pmedium->hGlobal);
        }
    }
    return hr;
}


//from idlist.c
#define HIDA_GetPIDLFolder(pida)        (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i)       (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

//
// Creates a HDROP (Win 3.1 compatible file list) from HIDA.
//
// WARNING: This function is called from netviewx.c
//
HRESULT CFSIDLData_CreateHDrop(IDataObject *pdtobj, STGMEDIUM *pmedium, BOOL fAltName)
{
    HRESULT hres;
    STGMEDIUM medium;
    TCHAR szPath[MAX_PATH];
    UINT i, cbAlloc = SIZEOF(DROPFILES) + SIZEOF(TCHAR);        // header + null terminator
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    LPCITEMIDLIST pidlFolder;
    IShellFolder *psfFolder = NULL;

    ASSERT(pida && pida->cidl); // we created this

    pidlFolder = HIDA_GetPIDLFolder(pida);
    ASSERT(pidlFolder);
    if (FAILED(hres = SHBindToObject(NULL, &IID_IShellFolder, pidlFolder, &psfFolder)))
        goto Abort;

    // Allocate too much to start out with, then re-alloc when we are done
    pmedium->hGlobal = GlobalAlloc(GPTR, cbAlloc + pida->cidl * MAX_PATH * SIZEOF(TCHAR));
    if (pmedium->hGlobal)
    {
        LPDROPFILES pdf = (LPDROPFILES)pmedium->hGlobal;
        LPTSTR pszFiles = (LPTSTR)(pdf + 1);
        pdf->pFiles = SIZEOF(DROPFILES);
#ifdef UNICODE
        pdf->fWide = TRUE;
#endif
        for (i = 0; i < pida->cidl; i++)
        {
            LPCITEMIDLIST pidlItem = HIDA_GetPIDLItem(pida, i);
            STRRET strret;
            int cch;

            ASSERT(ILIsEmpty(_ILNext(pidlItem)) || ILIsEmpty(pidlFolder)); // otherwise GDNO will fail
            hres = psfFolder->lpVtbl->GetDisplayNameOf(psfFolder, pidlItem, SHGDN_FORPARSING, &strret);
            if (FAILED(hres))
            {
                DebugMsg(TF_FSTREE, TEXT("CFSIDLData_GetHDrop: SHGetPathFromIDList failed."));
                goto Abort;
            }
            StrRetToBuf(&strret, pidlItem, szPath, ARRAYSIZE(szPath)); // will free the strret
            if (fAltName)
                GetShortPathName(szPath, szPath, ARRAYSIZE(szPath));
            cch = lstrlen(szPath) + 1;

            // prevent buffer overrun
            if ((LPBYTE)(pszFiles + cch) > ((LPBYTE)pmedium->hGlobal) + cbAlloc + pida->cidl * MAX_PATH * SIZEOF(TCHAR))
            {
                TraceMsg(TF_WARNING, "hdrop:%d'th file caused us to exceed allocated memory, breaking", i);
                break;
            }
            lstrcpy(pszFiles, szPath); // will write NULL terminator for us
            pszFiles += cch;
            cbAlloc += cch * SIZEOF(TCHAR);
        }
        *pszFiles = 0; // double NULL terminate
        ASSERT((LPTSTR)((BYTE *)pdf + cbAlloc - SIZEOF(TCHAR)) == pszFiles);

        // re-alloc down to the amount we actually need
        // note that pdf and pszFiles are now both invalid (and not used anymore)
        pmedium->hGlobal = GlobalReAlloc(pdf, cbAlloc, GMEM_MOVEABLE);

        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->pUnkForRelease = NULL;

        hres = S_OK;
    }
    else
        hres = E_OUTOFMEMORY;
Abort:
    if (psfFolder)
        psfFolder->lpVtbl->Release(psfFolder);
    HIDA_ReleaseStgMedium(pida, &medium);

    return hres;
}


// Attempt to get the HDrop format: Create one from the HIDA if necessary
HRESULT CFSIDLData_GetHDrop(IDataObject *pdtobj, LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    STGMEDIUM tempmedium;
    HRESULT hres = CIDLData_GetData(pdtobj, pformatetcIn, &tempmedium);

    // Couldn't get HDROP format, create it
    if (FAILED(hres))
    {
        // Set up a dummy formatetc to save in case multiple tymed's were specified
        FORMATETC fmtTemp = *pformatetcIn;
        fmtTemp.tymed = TYMED_HGLOBAL;

        hres = CFSIDLData_CreateHDrop(pdtobj, &tempmedium, pformatetcIn->dwAspect == DVASPECT_SHORTNAME);

        if (SUCCEEDED(hres))
        {
            // And we also want to cache this new format
            // .. Ensure that we actually free the memory associated with the HDROP
            //    when the data object destructs (pUnkForRelease = NULL)
            ASSERT(tempmedium.pUnkForRelease == NULL);

            if (SUCCEEDED(CIDLData_SetData(pdtobj, &fmtTemp, &tempmedium, TRUE)))
            {
                // Now the old medium that we just set is owned by the data object - call
                // GetData to get a medium that is safe to release when we're done.
                hres = CIDLData_GetData(pdtobj, pformatetcIn, &tempmedium);
            }
            else
            {
                TraceMsg(TF_WARNING, "Couldn't save the HDrop format to the data object - returning private version");
            }
        }
    }
    

    // HACKHACK
    // Some context menu extensions just release the hGlobal instead of
    // calling ReleaseStgMedium. This causes a reference counted data
    // object to fail. Therefore, we always allocate a new HGLOBAL for
    // each request. This sucks, but is needed. Specifically Quickview
    // Pro does this.
    //
    // Ideally we'd like to set the pUnkForRelease and not have to
    // dup the hGlobal each time, but alas Quickview has called our bluff
    // and LocalFree's it.
    if (SUCCEEDED(hres))
    {
        if (NULL != pmedium)
        {
            SIZE_T cbhGlobal;
            *pmedium = tempmedium;
            pmedium->pUnkForRelease = NULL;

            // Make a copy of this hglobal to pass back
            cbhGlobal = LocalSize(tempmedium.hGlobal);

            if (0 != cbhGlobal)
            {
                pmedium->hGlobal = LocalAlloc(0, (UINT) cbhGlobal);

                if (NULL != pmedium->hGlobal)
                {
                    CopyMemory(pmedium->hGlobal, tempmedium.hGlobal, cbhGlobal);
                }
                else
                {
                    hres = E_OUTOFMEMORY;
                }
            }
            else
            {
                hres = E_UNEXPECTED;
            }
        }
        else
        {
            hres = E_INVALIDARG;
        }

        // Release the medium
        ReleaseStgMedium(&tempmedium);
    }

    return hres;
}

// subclass member function to support CF_HDROP and CF_NETRESOURCE

HRESULT CFSIDLData_GetData(IDataObject *pdtobj, LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    HRESULT hres = E_INVALIDARG;

    if (pformatetcIn->cfFormat == CF_HDROP && (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        hres = CFSIDLData_GetHDrop(pdtobj, pformatetcIn, pmedium);
    }
    else if ((pformatetcIn->cfFormat == g_cfFileName ||
#ifdef UNICODE
                 pformatetcIn->cfFormat == g_cfFileNameW
#else
                 FALSE
#endif
                               ) && (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        STGMEDIUM mediumT;
        FORMATETC formatT = *pformatetcIn;
        formatT.dwAspect = DVASPECT_SHORTNAME;

        //
        // NOTES: Since we don't know the destination, we should use
        //  short name. New apps should use CF_HDROP anyway...
        //
        hres = CFSIDLData_GetHDrop(pdtobj, &formatT, &mediumT);
        if (SUCCEEDED(hres))
        {
            TCHAR szPath[MAX_PATH];
            if (DragQueryFile(mediumT.hGlobal, 0, szPath, ARRAYSIZE(szPath)))
            {
                HGLOBAL hmem;
                UINT uSize = lstrlen(szPath) + 1;
#ifdef UNICODE
                if (pformatetcIn->cfFormat == g_cfFileNameW)
                    uSize *= sizeof(WCHAR);
#endif
                hmem = GlobalAlloc(GPTR, uSize);
                if (hmem)
                {
#ifdef UNICODE
                    if (pformatetcIn->cfFormat == g_cfFileNameW)
                        lstrcpy((LPWSTR)hmem, szPath);
                    else
                        SHTCharToAnsi(szPath, (LPSTR)hmem, uSize);
#else
                    lstrcpy((LPTSTR)hmem, szPath);
#endif
                    pmedium->tymed = TYMED_HGLOBAL;
                    pmedium->hGlobal = hmem;
                    pmedium->pUnkForRelease =NULL;
                    hres = S_OK;
                }
                else
                {
                    hres = E_OUTOFMEMORY;
                }
            }
            else
            {
                hres = E_UNEXPECTED;
            }
            ReleaseStgMedium(&mediumT);
        }
    }
    else if (pformatetcIn->cfFormat == g_cfNetResource && (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        //
        //  We should return HNRES if the selected file system objects
        // are in one of network folders.
        //
        hres = CDesktopIDLData_GetNetResourceForFS(pdtobj, pmedium);
    }
    else
    {
        hres = CIDLData_GetData(pdtobj, pformatetcIn, pmedium);
    }

    return hres;
}

const IDataObjectVtbl c_CFSIDLDataVtbl = {
    CIDLData_QueryInterface,
    CIDLData_AddRef,
    CIDLData_Release,
    CFSIDLData_GetData,
    CIDLData_GetDataHere,
    CFSIDLData_QueryGetData,
    CIDLData_GetCanonicalFormatEtc,
    CFSIDLData_SetData,
    CIDLData_EnumFormatEtc,
    CIDLData_Advise,
    CIDLData_Unadvise,
    CIDLData_EnumAdvise
};

STDAPI CFSFolder_CreateDataObject(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[], IDataObject **ppdtobj)
{
    return CIDLData_CreateFromIDArray2(&c_CFSIDLDataVtbl, pidlFolder, cidl, apidl, ppdtobj);
}

HRESULT FS_CreateFSIDArray(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST *apidl,
                           IDataObject *pdtInner, IDataObject **pdtobjOut)
{
    return CIDLData_CreateFromIDArray3(&c_CFSIDLDataVtbl, pidlFolder, cidl, apidl, pdtInner, pdtobjOut);
}

DWORD CALLBACK _CFSFolder_PropertiesThread(PROPSTUFF * pps)
{
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pps->pdtobj, &medium);
    if (pida)
    {
        LPITEMIDLIST pidl = IDA_ILClone(pida, 0);
        if (pidl)
        {
            TCHAR szPath[MAX_PATH];
            LPTSTR pszCaption;

            // Yes, do context menu.
            HKEY ahkeys[2] = { NULL, NULL };

            // Get the hkeyProgID and hkeyBaseProgID from the first item.
            SHGetClassKey(pidl, &ahkeys[1], &ahkeys[0]);

            // REVIEW: psb?
            pszCaption = SHGetCaption(medium.hGlobal);
            SHOpenPropSheet(pszCaption, ahkeys, ARRAYSIZE(ahkeys),
                                &CLSID_ShellFileDefExt, pps->pdtobj, NULL, pps->pStartPage);
            if (pszCaption)
                SHFree(pszCaption);

            SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));

            if (SHGetPathFromIDList(pidl, szPath))
            {
                if (lstrcmpi(PathFindExtension(szPath), TEXT(".pif")) == 0)
                {
                    DebugMsg(TF_FSTREE, TEXT("cSHCNRF_pt: DOS properties done, generating event."));
                    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST, pidl, NULL);
                }
            }

            ILFree(pidl);
        }

        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return 0;
}



//
// Display a property sheet for a set of files.
// The data object supplied must provide the "Shell IDList Array"
// clipboard format.
// The dwFlags argument is provided for future expansion.  It is
// currently unused.
//
HRESULT SHMultiFileProperties(
    IDataObject *pdtobj,
    DWORD dwFlags
    )
{
    SHLaunchPropSheet(_CFSFolder_PropertiesThread, pdtobj, (LPCTSTR)0, NULL, NULL);
    return S_OK;
}



HMENU FindMenuBySubMenuID(HMENU hmenu, UINT id, LPINT pIndex)
{
    int cMax;
    HMENU hmenuReturn = NULL;
    MENUITEMINFO mii;

    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_ID;
    mii.cch = 0;        // just in case...

    for (cMax = GetMenuItemCount(hmenu) - 1 ; cMax >= 0 ; cMax--)
    {
        HMENU hmenuSub = GetSubMenu(hmenu, cMax);
        if (hmenuSub && GetMenuItemInfo(hmenuSub, 0, TRUE, &mii))
        {
            if (mii.wID == id) {
                // found it!
                hmenuReturn = hmenuSub;
                break;
            }
        }
    }
    if (hmenuReturn && pIndex)
        *pIndex = cMax;
    return hmenuReturn;
}

void DeleteMenuBySubMenuID(HMENU hmenu, UINT id)
{
    int i;

    FindMenuBySubMenuID(hmenu, id, &i);
    DeleteMenu(hmenu, i, MF_BYPOSITION);
}

// fMask is from CMIC_MASK_*
HRESULT FS_CreateLinks(HWND hwnd, IShellFolder *psf, IDataObject *pdtobj, LPCTSTR pszDir, DWORD fMask)
{
    CFSFolder *this = FS_GetFSFolderFromShellFolder(psf);
    HRESULT hres;
    TCHAR szPath[MAX_PATH];
    int cItems;
    LPITEMIDLIST *ppidl;
    UINT fCreateLinkFlags;

    if (this == NULL)
        return E_FAIL;

    hres = CFSFolder_GetPath(this, szPath);
    if (FAILED(hres))
        return hres;

    cItems = DataObj_GetHIDACount(pdtobj);
    ppidl = (LPITEMIDLIST *)LocalAlloc(LPTR, SIZEOF(LPITEMIDLIST) * cItems);
    // passing ppidl == NULL is correct in failure case

    if ((pszDir == NULL) || (lstrcmpi(pszDir, szPath) == 0))
    {
        // create the link in the current folder
        fCreateLinkFlags = SHCL_USETEMPLATE;
    }
    else
    {
        // this is a sys menu, ask to create on desktop
        fCreateLinkFlags = SHCL_USETEMPLATE | SHCL_USEDESKTOP;
        if (!(fMask & CMIC_MASK_FLAG_NO_UI))
        {
            fCreateLinkFlags |= SHCL_CONFIRM;
        }
    }

    hres = SHCreateLinks(hwnd, szPath, pdtobj, fCreateLinkFlags, ppidl);

    if (ppidl)
    {
        int i;
        // select those objects;
        HWND hwndSelect = DV_HwndMain2HwndView(hwnd);

        // select the new links, but on the first one deselect all other selected things

        for (i = 0; i < cItems; i++)
        {
            if (ppidl[i])
            {
                SendMessage(hwndSelect, SVM_SELECTITEM,
                    i == 0 ? SVSI_SELECT | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS | SVSI_FOCUSED :
                             SVSI_SELECT,
                    (LPARAM)ILFindLastID(ppidl[i]));
                ILFree(ppidl[i]);
            }
        }
        LocalFree((HLOCAL)ppidl);
    }

    return hres;
}

//
// right click context menu for items handler
//
// Returns:
//      S_OK, if successfully processed.
//      S_FALSE, if default code should be used.
//
STDAPI _ItemsMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    HRESULT hres = S_OK;
    static const QCMINFO_IDMAP idMap =
    {2,
        {
            {FSIDM_FOLDER_SEP,QCMINFO_PLACE_BEFORE},
            {FSIDM_VIEW_SEP,QCMINFO_PLACE_AFTER}
        }
    };

    switch (uMsg) {
    case DFM_MERGECONTEXTMENU:
        //
        // We need to avoid adding SendTo
        //
        if (!(wParam & CMF_VERBSONLY))
        {
            LPQCMINFO pqcm = (LPQCMINFO)lParam;
            UINT      idCmdBase = pqcm->idCmdFirst;
            BOOL      bCorelSuite7Hack = (SHGetAppCompatFlags(ACF_CONTEXTMENU) & ACF_CONTEXTMENU);

            // This is a context menu.

            // corel relies on the hard coded send to menu so we give them one
            if (!bCorelSuite7Hack)
            {
                HMENU     hmenu = CreateMenu();

                if (hmenu)
                {
                    UINT  uPos = GetMenuPosFromID(pqcm->hmenu, FSIDM_VIEW_SEP) + 1;
                    InsertMenu(hmenu, 0, MF_BYPOSITION | MF_SEPARATOR, -1, TEXT(""));
                    if (uPos == 0)
                    {   // could not find FSIDM_VIEW_SEP, add it at the top
                        InsertMenu(hmenu, 0, MF_BYPOSITION | MF_SEPARATOR, FSIDM_VIEW_SEP, TEXT(""));
                    }
                    Shell_MergeMenus(pqcm->hmenu, hmenu, uPos,
                            pqcm->idCmdFirst, pqcm->idCmdLast,
                            MM_SUBMENUSHAVEIDS|MM_DONTREMOVESEPS);
                    DestroyMenu(hmenu);
                }
            }
            else
            {
                CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_FSVIEW_ITEM_COREL7_HACK, 0, pqcm);
            }

            pqcm->pIdMap = &idMap;
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_INVOKECOMMANDEX:
        {
            DFMICS *pdfmics = (DFMICS *)lParam;
            switch (wParam)
            {
            case DFM_CMD_DELETE:
                //  dont allow undo in the recent folder.
                hres = DeleteFilesInDataObjectEx(hwnd, pdfmics->fMask, pdtobj,
                    (CFSFolder_GetCSIDL(this) == CSIDL_RECENT) ? SD_NOUNDO : 0);
                break;

            case DFM_CMD_LINK:
                hres = FS_CreateLinks(hwnd, psf, pdtobj, (LPCTSTR)pdfmics->lParam, pdfmics->fMask);
                break;

            case DFM_CMD_PROPERTIES:
                SHLaunchPropSheet(_CFSFolder_PropertiesThread, pdtobj, (LPCTSTR)pdfmics->lParam, NULL, this->_pidl);
                break;

            default:
                // This is common menu items, use the default code.
                hres = S_FALSE;
                break;
            }
        }
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }
    return hres;
}

//#define CCM_FILE     0x0001
//#define CCM_EDIT     0x0002
//#define CCM_FILEDIT  0x0003

// return the array of keys that we use to load the context menu extensions for an item.
// this is based on the file type and other info we know about the selection
//
void _GetContextMenuKeys(CFSFolder *this, LPCITEMIDLIST apidl[], UINT cidl, PHKEY ahkeys, UINT chkeys)
{
    BOOL bCorelSuite7Hack = SHGetAppCompatFlags(ACF_CONTEXTMENU) & ACF_CONTEXTMENU;
    LPCIDFOLDER pidf = FS_IsValidID(apidl[0]);
    IQueryAssociations *pqa;

    RIP(chkeys >= 3);
    
    ahkeys[0] = 0;  // file type or unknown
    ahkeys[1] = 0;  // base type: folder for directory and star for others
    ahkeys[2] = 0;  // AllFilesystemObjects
 
    if (pidf && SUCCEEDED(FS_AssocCreate(pidf, &IID_IQueryAssociations, (void **)&pqa)))
    {
        DWORD dwFlags = SHGetClassFlags(pidf);

        // Get the hkeyProgID and hkeyBaseProgID from the first item.
        if (dwFlags & SHCF_UNKNOWN)
        {
            if (!PathIsHighLatency(NULL, ((LPIDFOLDER)pidf)->fs.wAttrs))
            {                
                WCHAR wszPath[MAX_PATH];

                if (SUCCEEDED(CFSFolder_GetPathForItemW(this, pidf, wszPath)))
                {
                    CLSID clsid;

                    if (SUCCEEDED(GetClassFile(wszPath, &clsid)))
                    {
                        ahkeys[0] = ProgIDKeyFromCLSID(&clsid);
                    }
                }
            }
        }

        if (ahkeys[0] == NULL)
        {
            pqa->lpVtbl->GetKey(pqa, ASSOCF_IGNOREBASECLASS | ASSOCF_VERIFY, ASSOCKEY_SHELLEXECCLASS, NULL, &ahkeys[0]);
        }

        if (ahkeys[0] == NULL)
        {
            pqa->lpVtbl->GetKey(pqa, ASSOCF_IGNOREBASECLASS | ASSOCF_NOUSERSETTINGS, ASSOCKEY_CLASS, NULL, &ahkeys[0]);
        }

        if (ahkeys[0] == NULL)
        {
            RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("Unknown"), 0, MAXIMUM_ALLOWED, &ahkeys[0]);
        }

        //
        // If the class is not a link AND SHGetClassKey didn't use the Base class
        // as a default, get the base class key.
        //
        if (!(dwFlags & SHCF_IS_LINK))
            pqa->lpVtbl->GetKey(pqa, 0, ASSOCKEY_BASECLASS, NULL, &ahkeys[1]);

        // corel wp suite 7 relies on the fact that send to menu is hard coded
        // not an extension so do not insert it (and the similar items)
        if (!bCorelSuite7Hack)
        {
            RegOpenKey(HKEY_CLASSES_ROOT, TEXT("AllFilesystemObjects"), &ahkeys[2]);
        }

        pqa->lpVtbl->Release(pqa);
    }
}

HRESULT _CreateContextMenu(CFSFolder *this, HWND hwnd, LPCITEMIDLIST *apidl, UINT cidl, void **ppv)
{
    HKEY ahkeys[3];
    HRESULT hres;

    _GetContextMenuKeys(this, apidl, cidl, ahkeys, ARRAYSIZE(ahkeys));

    hres = CDefFolderMenu_Create2(this->_pidl, hwnd,
                cidl, apidl, (IShellFolder *)&this->sf, _ItemsMenuCB,
                ARRAYSIZE(ahkeys), ahkeys, (IContextMenu **)ppv);

    SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));

    return hres;
}

HRESULT CFSFolder_PerFolderLogoSupport(CFSFolder *this, LPCIDFOLDER pidf, REFIID riid, void **ppv)
{
    TCHAR szPath[MAX_PATH];
    HRESULT hr = CFSFolder_GetPathForItem(this, pidf, szPath);
    if (SUCCEEDED(hr))
    {
        hr = CFolderExtractImage_Create(szPath, riid, ppv);
    }
    return hr;
}

STDMETHODIMP CFSFolder_GetUIObjectOf(IShellFolder2 *psf, HWND hwnd,
                                 UINT cidl, LPCITEMIDLIST * apidl,
                                 REFIID riid, UINT *prgfInOut, void **ppv)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    HRESULT hr = E_INVALIDARG;
    LPCIDFOLDER pidf = cidl ? FS_IsValidID(apidl[0]) : NULL;

    *ppv = NULL;

    if (pidf &&
        (IsEqualIID(riid, &IID_IExtractIconA) || IsEqualIID(riid, &IID_IExtractIconW)))
    {
        hr = CFSFolder_CreateDefExtIcon(this, pidf, riid, ppv);
    }
    else if (IsEqualIID(riid, &IID_IContextMenu) && cidl)
    {
        hr = _CreateContextMenu(this, hwnd, apidl, cidl, ppv);
    }
    else if (IsEqualIID(riid, &IID_IDataObject) && cidl)
    {
        IDataObject *pdtInner = NULL;
        if ((cidl == 1) && pidf)
        {
            FSLoadHandler(this, pidf, TEXT("shellex\\DataHandler"), &IID_IDataObject, &pdtInner);
        }

        hr = FS_CreateFSIDArray(this->_pidl, cidl, apidl, pdtInner, (IDataObject **)ppv);

        if (pdtInner)
            pdtInner->lpVtbl->Release(pdtInner);
    }
    else if (IsEqualIID(riid, &IID_IDropTarget) && pidf)
    {
        if (FS_IsFolder(pidf) || FS_IsJunction(pidf))
        {
            IShellFolder *psfT;
            hr = CFSFolder_BindToObject(psf, apidl[0], NULL, &IID_IShellFolder, &psfT);
            if (SUCCEEDED(hr))
            {
                hr = psfT->lpVtbl->CreateViewObject(psfT, hwnd, &IID_IDropTarget, ppv);
                psfT->lpVtbl->Release(psfT);
            }
        }
        else
        {
            // old code supported absolute PIDLs here. that was bogus...
            ASSERT(ILIsEmpty(apidl[0]) || (ILFindLastID(apidl[0]) == apidl[0]));
            ASSERT(FS_IsFile(pidf) || FS_IsNT4StyleSimpleID(pidf));

            hr = FSLoadHandler(this, pidf, TEXT("shellex\\DropHandler"), &IID_IDropTarget, ppv);
        }
    }
    else if (pidf)
    {
        hr = FSLoadHandler(this, pidf, NULL, riid, ppv);
        if (FAILED(hr))
        {
            if (IsEqualIID(riid, &IID_IQueryInfo))
            {
                hr = GetToolTipForItem(this, pidf, riid, ppv);
            }
            else if (IsEqualIID(riid, &IID_IQueryAssociations))
            {
                hr = FS_AssocCreate(pidf, riid, ppv);
            }
            else if (IsEqualIID(riid, &IID_IExtractImage) || IsEqualIID(riid, &IID_IExtractLogo))
            {
                // default handler type, use the IID_ as the key to open for the handler
                // if it is an image extractor, then check to see if it is a per-folder logo...
                hr = CFSFolder_PerFolderLogoSupport(this, pidf, riid, ppv);
            }
        }
    }

    return hr;
}

// in netviewx.c
void WINAPI CopyEnumElement(void *pDest, const void *pSource, DWORD dwSize);

STDMETHODIMP CFSFolder_EnumSearches(IShellFolder2 *psf, LPENUMEXTRASEARCH *ppenum)
{
    *ppenum = NULL;
    return  E_NOTIMPL;
}

LPCIDFOLDER FSFindJunction(LPCIDFOLDER pidf)
{
    for (; pidf->cb; pidf = FS_Next(pidf))
    {
        //
        // Check for true junctions and browsable files
        //
        if (FS_IsJunction(pidf))
            return pidf;

        if (FS_IsFile(pidf))
        {
            DWORD dwFlags = SHGetClassFlags(pidf);

            if (dwFlags & SHCF_IS_BROWSABLE)
                return pidf;
        }
    }

    return NULL;
}

// return IDLIST of item just past the junction point (if there is one)
// if there's no next pointer, return NULL.

LPCITEMIDLIST FSFindJunctionNext(LPCIDFOLDER pidf)
{
    pidf = FSFindJunction(pidf);
    if (pidf)
    {
        // cast here represents the fact that this data is opaque
        LPCITEMIDLIST pidl = (LPCITEMIDLIST)FS_Next(pidf);
        if (!ILIsEmpty(pidl))
            return pidl;        // first item past junction
    }
    return NULL;
}

#ifdef FEATURE_LOCALIZED_FOLDERS
HRESULT FS_GetLocalizedDisplayName(LPCIDFOLDER pidf, LPTSTR pszName, DWORD cchName)
{
    TCHAR szCanon[MAX_PATH];
    HRESULT hr = E_FAIL;

    if (ILGetHiddenString((LPCITEMIDLIST)pidf, IDLHID_LOCALIZEDNAME, szCanon, SIZECHARS(szCanon)))
    {
        DWORD cb = CbFromCch(cchName);
        //  we got one

        //  first check the registry for overrides
        if (NOERROR == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER TEXT("\\LocalizedResourceName"), szCanon, NULL, pszName, &cb)
        && *pszName)
        {
            hr = S_OK;
        }
        else
        {
            int iName = PathParseIconLocation(szCanon);
            HINSTANCE hinst = GetModuleHandle(szCanon); 

            if (hinst && iName < 0)
            {
                if (LoadString(hinst, (UINT)-1 * iName, pszName, cchName))
                    hr = S_OK;
            }
        }
    }
    return hr;
}
#endif // FEATURE_LOCALIZED_FOLDERS

HRESULT FS_NormalGetDisplayNameOf(LPCIDFOLDER pidf, STRRET *pStrRet)
{
    TCHAR szPath[MAX_PATH];

#ifdef FEATURE_LOCALIZED_FOLDERS
    if (FAILED(FS_GetLocalizedDisplayName(pidf, szPath, SIZECHARS(szPath))))
#endif // FEATURE_LOCALIZED_FOLDERS
    FS_CopyName(pidf, szPath, ARRAYSIZE(szPath));

    if (!FS_ShowExtension(pidf))
        PathRemoveExtension(szPath);

    //
    //  NT B#161431 - Some apps (e.g., Norton Uninstall Deluxe)
    //  don't handle STRRET_WSTR properly.  NT4's shell32
    //  returned STRRET_WSTR only if it had no choice, so these apps
    //  seemed to run just fine on NT as long as you never had any
    //  UNICODE filenames.  We must preserve the NT4 behavior or
    //  these buggy apps start blowing chunks.
    //

    if ((FS_GetType(pidf) & SHID_FS_UNICODE) == SHID_FS_UNICODE)
    {
        return StringToStrRet(szPath, pStrRet);
    }
    else
    {
        // We started with an ANSI filename of length at most
        // MAX_PATH, and removing the extension and making the
        // filename pretty will
        //
        // 1. preserve ANSI-ness,
        // 2. never make the string longer.
        //
        // Therefore, the following conversion will always succeed.
        //
        pStrRet->uType = STRRET_CSTR;
        SHTCharToAnsi(szPath, pStrRet->cStr, ARRAYSIZE(pStrRet->cStr));
        return S_OK;
    }
}

STDMETHODIMP CFSFolder_GetDisplayNameOf(IShellFolder2 *psf, LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET pStrRet)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    HRESULT hres = S_FALSE;
    LPCIDFOLDER pidf = FS_IsValidID(pidl);
    if (pidf)
    {
        TCHAR szPath[MAX_PATH];
        LPCITEMIDLIST pidlNext = _ILNext(pidl);

        if (dwFlags & SHGDN_FORPARSING)
        {
            if (dwFlags & SHGDN_INFOLDER)
            {
                FS_CopyName(pidf, szPath, ARRAYSIZE(szPath));

                if (ILIsEmpty(pidlNext))    // single level idlist
                    hres = StringToStrRet(szPath, pStrRet);
                else
                    hres = ILGetRelDisplayName((IShellFolder *)psf, pStrRet, pidl, szPath, MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_BACKSLASH));
            }
            else
            {
                LPIDFOLDER pidfBind;
                LPCITEMIDLIST pidlRight;

                hres = FS_GetJunctionForBind(pidf, &pidfBind, &pidlRight);
                if (SUCCEEDED(hres))
                {
                    if (hres == S_OK)
                    {
                        IShellFolder *psfJctn;
                        hres = FS_Bind(this, NULL, pidfBind, &IID_IShellFolder, &psfJctn);
                        if (SUCCEEDED(hres))
                        {
                            hres = psfJctn->lpVtbl->GetDisplayNameOf(psfJctn, pidlRight, dwFlags, pStrRet);
                            psfJctn->lpVtbl->Release(psfJctn);
                        }
                        FS_Free(pidfBind);
                    }
                    else
                    {
                        hres = CFSFolder_GetPathForItem(this, pidf, szPath);
                        if (SUCCEEDED(hres))
                        {
                            if (dwFlags & SHGDN_FORADDRESSBAR)
                            {
                                LPTSTR pszExt = PathFindCLSIDExtension(szPath, NULL);
                                if (pszExt)
                                    *pszExt = 0;
                            }
                            hres = StringToStrRet(szPath, pStrRet);
                        }
                    }
                }
            }
        }
        else if (CFSFolder_IsCSIDL(this, CSIDL_RECENT) && 
                 SUCCEEDED(RecentDocs_GetDisplayName((LPCITEMIDLIST)pidf, szPath, SIZECHARS(szPath))))
        {
            LPITEMIDLIST pidlRecent;
            WIN32_FIND_DATA wfd = {0};

            StrCpyN(wfd.cFileName, szPath, SIZECHARS(wfd.cFileName));

            if (SUCCEEDED(CFSFolder_CreateIDList(NULL, &wfd, &pidlRecent)))
            {
                hres = FS_NormalGetDisplayNameOf((LPCIDFOLDER)pidlRecent, pStrRet);
                ILFree(pidlRecent);
            }
                        
        }
        else
        {
            ASSERT(ILIsEmpty(pidlNext));    // this variation should be single level

            hres = FS_NormalGetDisplayNameOf(pidf, pStrRet);
        }
    }
    else
    {
        if (IsSelf(1, &pidl) && 
            ((dwFlags & (SHGDN_FORADDRESSBAR | SHGDN_INFOLDER | SHGDN_FORPARSING)) == SHGDN_FORPARSING))
        {
            TCHAR szPath[MAX_PATH];
            hres = CFSFolder_GetPath(this, szPath);
            if (SUCCEEDED(hres))
                hres = StringToStrRet(szPath, pStrRet);
        }
        else
        {
            hres = E_INVALIDARG;
            TraceMsg(TF_WARNING, "CFSFolder_GetDisplayNameOf() failing on PIDL %s", DumpPidl(pidl));
        }
    }
    return hres;
}


STDMETHODIMP CFSFolder_SetNameOf(IShellFolder2 *psf, HWND hwnd, LPCITEMIDLIST pidl, 
                                 LPCOLESTR pszName, DWORD dwFlags, LPITEMIDLIST *ppidl)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    HRESULT hr;
    LPCIDFOLDER pidf;

    if (ppidl) 
        *ppidl = NULL;

    pidf = FS_IsValidID(pidl);
    if (pidf)
    {
        TCHAR szNewName[MAX_PATH], szDir[MAX_PATH], szOldName[MAX_PATH];

        FS_CopyName(pidf, szOldName, ARRAYSIZE(szOldName));

        SHUnicodeToTChar(pszName, szNewName, ARRAYSIZE(szNewName));

        // If the extension is hidden
        if (!FS_ShowExtension(pidf))
        {
            // copy it from the old name
            StrCatBuff(szNewName, PathFindExtension(szOldName), ARRAYSIZE(szNewName));
        }

        hr = CFSFolder_GetPath(this, szDir);
        if (SUCCEEDED(hr))
        {
            UINT cchDirLen = lstrlen(szDir);
            DWORD dwRes;

            // There are cases where the old name exceeded the maximum path, which
            // would give a bogus error message.  To avoid this we should check for
            // this case and see if using the short name for the file might get
            // around this...
            //
            if (cchDirLen + lstrlen(szOldName) + 2 > MAX_PATH)
            {
                LPCSTR pszOldAltName = FS_GetAltName(pidf);
                if (cchDirLen + lstrlenA(pszOldAltName) + 2 <= MAX_PATH)
                    SHAnsiToTChar(pszOldAltName, szOldName, MAX_PATH);
            }

            // BUGBUG: We need to impl ::SetSite() and pass it to SHRenameFile
            //         to go modal if we display UI.
            dwRes = (DWORD) SHRenameFileEx(hwnd, NULL, szDir, szOldName, szNewName, FALSE);
            hr = HRESULT_FROM_WIN32(dwRes);
            if (SUCCEEDED(hr) && ppidl)
            {
                // Return the new pidl if ppidl is specified.
                hr = CFSFolder_CreateIDListFromName(this, szNewName, ppidl);
            }
        }
    }
    else
        hr = E_INVALIDARG;
    return hr;
}

HRESULT FindDataFromIDFolder(LPCIDFOLDER pidf, WIN32_FIND_DATAW *pfd)
{
    ZeroMemory(pfd, SIZEOF(*pfd));

    if (!FS_IsReal(pidf)) // if its a simple ID, there's no data in it
        return E_INVALIDARG;

    // alpha bugbug?  does pidf->fs.timeModified need to be copied into a temp var to be aligned?
    // Note that COFSFolder doesn't provide any times _but_ COFSFolder
    DosDateTimeToFileTime(pidf->fs.dateModified, pidf->fs.timeModified, &pfd->ftLastWriteTime);
    pfd->dwFileAttributes = pidf->fs.wAttrs;
    pfd->nFileSizeLow = pidf->fs.dwSize;

    FS_CopyNameW(pidf, pfd->cFileName, ARRAYSIZE(pfd->cFileName));

    SHAnsiToUnicode(FS_GetAltName(pidf), pfd->cAlternateFileName, ARRAYSIZE(pfd->cAlternateFileName));

    return NOERROR;
}


/***

To avoid registry explosion, each pidl is passed to each handler.

    HKCR\Folder\ColumnHandlers
      <clsid>
        "" = "Docfile handler"
      <clsid>
        "" = "Imagefile handler"

***/

// This struct is used for caching the column info
typedef struct {
    SHCOLUMNINFO shci;
    IColumnProvider *pcp;
    UINT iColumnId;  // This is the 'real' column number, think of it as an index to the scid, which can be provided multiple times
                     //  ie 3 column handlers each provide the same 5 cols, this goes from 0-4
} COLUMNLISTENTRY;

void DestroyColHandlers(HDSA *phdsa)
{
    int i;

    for(i=0;i<DSA_GetItemCount(*phdsa);i++)
    {
        COLUMNLISTENTRY *pcle = DSA_GetItemPtr(*phdsa, i);
        if (pcle->pcp)
            pcle->pcp->lpVtbl->Release(pcle->pcp);
    }
    TraceMsg(TF_FSTREE, "destroying (per sf) column handler cache");
    DSA_Destroy(*phdsa);
    *phdsa = NULL;
}

// returns the n'th handler for a given column
BOOL CFSFolder_FindColHandler(CFSFolder *this, UINT iCol, UINT iN, COLUMNLISTENTRY *pcle)
{
    int i;
    for(i = 0; i < DSA_GetItemCount(this->_hdsaColHandlers); i++)
    {
        COLUMNLISTENTRY *pcleWalk = DSA_GetItemPtr(this->_hdsaColHandlers, i);
        if (pcleWalk->iColumnId == iCol)
        {
            if (iN-- == 0)
            {
                *pcle = *pcleWalk;
                return TRUE;
            }
        }
    }
    return FALSE;
}

// returns the number of unique columns
DWORD CFSFolder_LoadColumnHandlers( CFSFolder* this )
{
    int iUniqueColumnCount = 0;
    HKEY hkCH;
    SHCOLUMNINIT shci;

    //  Have we been here?
    if( NULL != this->_hdsaColHandlers )
        return this->_dwColCount ;   // nothing to do.
    
    ASSERT( 0 == this->_dwColCount ) ;

    ZeroMemory( &shci, sizeof(shci) ) ;

    //  retrieve folder path for provider init
    if (FAILED(CFSFolder_GetPathForItemW(this, NULL, shci.wszFolder)))
        return 0;

    this->_hdsaColHandlers = DSA_Create(sizeof(COLUMNLISTENTRY), 5);
    if (NULL == this->_hdsaColHandlers)
        return 0;
        
    // Enumerate HKCR\Folder\Shellex\ColumnProviders
    // note: this really should have been "Directory", not "Folder"
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Folder\\shellex\\ColumnHandlers"), &hkCH))
    {
        TCHAR szHandlerCLSID[GUIDSTR_MAX];
        int iHandler = 0;

        while (ERROR_SUCCESS == RegEnumKey(hkCH, iHandler++, szHandlerCLSID, ARRAYSIZE(szHandlerCLSID)))
        {
            CLSID clsid;
            IColumnProvider *pcp;

            if (SUCCEEDED(SHCLSIDFromString(szHandlerCLSID, &clsid)) && 
                SUCCEEDED(CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IColumnProvider, &pcp)))
            {
                if (SUCCEEDED(pcp->lpVtbl->Initialize(pcp, &shci)))
                {
                    int iCol = 0;
                    COLUMNLISTENTRY cle;

                    cle.pcp = pcp;
                    while (S_OK == pcp->lpVtbl->GetColumnInfo(pcp, iCol++, &cle.shci))
                    {
                        int i;

                        cle.pcp->lpVtbl->AddRef(cle.pcp);
                        cle.iColumnId = iUniqueColumnCount++;

                        // Check if there's already a handler for this column ID,
                        for (i = 0;i < DSA_GetItemCount(this->_hdsaColHandlers); i++)
                        {
                            COLUMNLISTENTRY *pcleLoop = DSA_GetItemPtr(this->_hdsaColHandlers, i);
                            if (IsEqualSCID(pcleLoop->shci.scid, cle.shci.scid))
                            {
                                cle.iColumnId = pcleLoop->iColumnId;    // set the iColumnId to the same as the first one
                                iUniqueColumnCount--; // so our count stays right
                                break;
                            }
                        }
                        DSA_AppendItem(this->_hdsaColHandlers, &cle);
                    }
                }
                pcp->lpVtbl->Release(pcp);
            }
        }
        RegCloseKey(hkCH);
    }

    // Sanity check
    if (!DSA_GetItemCount(this->_hdsaColHandlers))
    {
        // DSA_Destroy(*phdsa);
        ASSERT(iUniqueColumnCount==0);
        iUniqueColumnCount = 0;
    }
    this->_dwColCount = (DWORD)iUniqueColumnCount;

    return this->_dwColCount ;
}

//  Initializes a SHCOLUMNDATA block.
HRESULT FS_InitShellColumnData( CFSFolder* this, LPCIDFOLDER pidf, SHCOLUMNDATA* pscd )
{
    HRESULT hr;

    ZeroMemory(pscd, sizeof(*pscd)) ;

    if (!pidf)
        return E_INVALIDARG ;

    hr = CFSFolder_GetPathForItemW( this, pidf, pscd->wszFile );
    if (SUCCEEDED(hr))
    {
        //  Assign address of file name extension
        pscd->pwszExt = PathFindExtensionW( pscd->wszFile ) ;

        //  Assign file attributes
        pscd->dwFileAttributes = pidf->fs.wAttrs;

        // set the dwFlags member
        if ( this->_bUpdateExtendedCols )
        {
            pscd->dwFlags = SHCDF_UPDATEITEM;
            this->_bUpdateExtendedCols = FALSE;
        }
    }
    return hr;
}

HRESULT FS_HandleExtendedColumn(CFSFolder *this, LPCIDFOLDER pidf, UINT iColumn, SHELLDETAILS *pDetails)
{
    HRESULT hres;

    CFSFolder_LoadColumnHandlers( this );

    if (iColumn < this->_dwColCount)
    {
        COLUMNLISTENTRY cle;
        SHCOLUMNDATA    shcd;

        int iTry = 0;   // how many column handlers we've tried for the current col

        if (!CFSFolder_FindColHandler(this, iColumn, iTry, &cle))
            return E_NOTIMPL;

        hres = FS_InitShellColumnData( this, pidf, &shcd );
        if (SUCCEEDED(hres))
        {
            if (PathIsHighLatency(NULL, shcd.dwFileAttributes))
                hres = E_FAIL;
            else
            {
                // loop through all the column providers, breaking when one succeeds
                while (TRUE)
                {
                    VARIANT varData;

                    VariantInit( &varData );
                    hres = cle.pcp->lpVtbl->GetItemData(cle.pcp, &cle.shci.scid, &shcd, &varData);
                    // fall through on S_FALSE
                    if (S_OK == hres)
                    {
                        // 99/03/25 vtan: Special case VT_DATE from VariantChangeType().
                        // GetDetailsOf3() uses DosTimeToDateTimeString() for displaying
                        // the modified date. Extended columns should do the same for dates.
                        if (varData.vt == VT_DATE)
                        {
                            TCHAR szTemp[MAX_PATH];
                            USHORT wDosDate, wDosTime;

                            VariantTimeToDosDateTime(varData.date, &wDosDate, &wDosTime);
                            DosTimeToDateTimeString(wDosDate, wDosTime, szTemp, ARRAYSIZE(szTemp), pDetails->fmt & LVCFMT_DIRECTION_MASK);
                            hres = StringToStrRet(szTemp, &pDetails->str);
                        }
                        else
                        {
                            VARIANTARG vStr;
                            VariantInit(&vStr);
                            hres = VariantChangeType(&vStr, &varData, 0, VT_BSTR);

                            if (SUCCEEDED(hres))
                            {
                                hres = SHStrDupW(vStr.bstrVal, &pDetails->str.pOleStr);
                                pDetails->str.uType = STRRET_WSTR;
                                VariantClear(&vStr);
                            }
                            else
                            {
                                //  Perf: If we failed conversion, recover with a zero-length string;
                                //  otherwise, we'll end up calling all these providers over and over
                                //  again for the same bogus data.
                                TraceMsg(TF_WARNING, "Couldn't change type from %d to VT_BSTR(%d), hr=%x",varData.vt, VT_BSTR, hres);
                                pDetails->str.uType = STRRET_CSTR;
                                pDetails->str.cStr[0] = 0;
                                hres = S_FALSE ; 
                            }
                            VariantClear(&varData);
                        }
                        break;
                    }
                    else
                    {
                        // TraceMsg(TF_ALWAYS, "GetItemData failed with %x, cle.next=%d",hres,cle.next);
            
                        // if there are any other handlers for this column, give them a try
                        if (!CFSFolder_FindColHandler(this, iColumn, ++iTry, &cle))
                        {
                            pDetails->str.uType = STRRET_CSTR;
                            pDetails->str.cStr[0] = 0;
                            hres = S_FALSE ; // return success; otherwise we'll endlessly pester 
                                             // all column handlers for this column/item.
                            break;
                        }
                    }
                }
            }
        }
        else if ( NULL == pidf )
        {
            TraceMsg(TF_FSTREE, "using cached column info for %d (%ws)", iColumn, cle.shci.wszTitle);

            pDetails->fmt = cle.shci.fmt;
            pDetails->cxChar = cle.shci.cChars;
#ifdef UNICODE
            hres = SHStrDupW(cle.shci.wszTitle, &pDetails->str.pOleStr);
            pDetails->str.uType = STRRET_WSTR;
#else
            SHUnicodeToAnsi(cle.shci.wszTitle, pDetails->str.cStr, ARRAYSIZE(pDetails->str.cStr));
            pDetails->str.uType = STRRET_CSTR;
#endif
        }
    }
    else
        hres = E_NOTIMPL; // the bogus return value defview expects...

    return hres;
}

HRESULT FS_CompareExtendedDates (CFSFolder *this, LPARAM lParam, LPCIDFOLDER pidf1, LPCIDFOLDER pidf2)

{
    int     iColumn;

    iColumn = ((DWORD)lParam & SHCIDS_COLUMNMASK) - ARRAYSIZE(c_fs_cols);
    if (EVAL(iColumn >= 0))
    {

        // 99/03/24 #295631 vtan: Make sure the comparison is on an extended column.
        // Find a column handler for this column. Once found, use it to get the native
        // data type stored for the column. If both the types are VT_DATE then convert
        // the variant to a SYSTEMTIME and then to a FILETIME for comparison.

        TINT(CFSFolder_LoadColumnHandlers(this));
        if ((DWORD)iColumn < this->_dwColCount)
        {
            HRESULT             hres1, hres2;
            int                 iTry=0;   // how many column handlers we've tried for the current col
            VARIANT             var1, var2;
            COLUMNLISTENTRY     cle;
            SHCOLUMNDATA        shcd1, shcd2;

            if (!CFSFolder_FindColHandler(this, iColumn, iTry, &cle))
                return(E_FAIL);
            if (FAILED(FS_InitShellColumnData(this, pidf1, &shcd1)))
                return(E_FAIL);
            if (FAILED(FS_InitShellColumnData(this, pidf2, &shcd2)))
                return(E_FAIL);
            do
            {
                VariantInit(&var1);
                VariantInit(&var2);
                hres1 = cle.pcp->lpVtbl->GetItemData(cle.pcp, &cle.shci.scid, &shcd1, &var1);
                hres2 = cle.pcp->lpVtbl->GetItemData(cle.pcp, &cle.shci.scid, &shcd2, &var2);
                if ((hres1 == S_OK) && (var1.vt == VT_DATE) && (hres2 == S_OK) && (var2.vt == VT_DATE))
                {
                    SYSTEMTIME  sysTime1, sysTime2;
                    FILETIME    fileTime1, fileTime2;

                    VariantTimeToSystemTime(var1.date, &sysTime1);
                    VariantTimeToSystemTime(var2.date, &sysTime2);
                    SystemTimeToFileTime(&sysTime1, &fileTime1);
                    SystemTimeToFileTime(&sysTime2, &fileTime2);
                    return(ResultFromShort(CompareFileTime(&fileTime1, &fileTime2)));
                }
                if (!CFSFolder_FindColHandler(this, iColumn, ++iTry, &cle))
                    return(E_FAIL);
            } while (TRUE);
        }
    }
    return(E_FAIL);
}

HRESULT FS_GetDetailsOf(CFSFolder *this, LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    HRESULT hres = S_OK;
    LPIDFOLDER pidf = (LPIDFOLDER)pidl;
    TCHAR szTemp[MAX_PATH];

    pDetails->str.uType = STRRET_CSTR;
    pDetails->str.cStr[0] = 0;

    if (iColumn >= ARRAYSIZE(c_fs_cols))
    {
        return FS_HandleExtendedColumn(this, pidf, iColumn - ARRAYSIZE(c_fs_cols), pDetails);
    }

    if (!pidf)
    {
        pDetails->fmt = c_fs_cols[iColumn].iFmt;
        pDetails->cxChar = c_fs_cols[iColumn].cchCol;
        return ResToStrRet(c_fs_cols[iColumn].ids, &pDetails->str);
    }

    switch (iColumn)
    {
    case FS_ICOL_NAME:
        hres = FS_NormalGetDisplayNameOf(pidf, &pDetails->str);
        break;

    case FS_ICOL_SIZE:
        if (FS_IsFolder(pidf))
            hres = E_FAIL;
        else
        {
            ULONGLONG cbSize;

            FS_GetSize(this->_pidl, pidf, &cbSize);

            StrFormatKBSize(cbSize, szTemp, ARRAYSIZE(szTemp));
            hres = StringToStrRet(szTemp, &pDetails->str);
        }
        break;

    case FS_ICOL_TYPE:
        FS_GetTypeName(pidf, szTemp, ARRAYSIZE(szTemp));
        hres = StringToStrRet(szTemp, &pDetails->str);
        break;

    case FS_ICOL_MODIFIED:
        DosTimeToDateTimeString(pidf->fs.dateModified, pidf->fs.timeModified, szTemp, ARRAYSIZE(szTemp), pDetails->fmt & LVCFMT_DIRECTION_MASK);
        hres = StringToStrRet(szTemp, &pDetails->str);
        break;

    case FS_ICOL_ATTRIB:
        BuildAttributeString(pidf->fs.wAttrs, szTemp, ARRAYSIZE(szTemp));
        hres = StringToStrRet(szTemp, &pDetails->str);
        break;
    }
    return hres;
}

// These next functions are for the shell OM script support

HRESULT FS_GetDetailsEx(CFSFolder *this, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr = E_FAIL;
    LPCIDFOLDER pidf = FS_IsValidID(pidl);
    if (pidf)
    {
        if (IsEqualSCID(*pscid, SCID_FINDDATA))
        {
            WIN32_FIND_DATAW wfd;
            hr = FindDataFromIDFolder(pidf, &wfd);

            if (SUCCEEDED(hr))
            {
                hr = InitVariantFromBuffer(pv, &wfd, sizeof(wfd));
            }
        }
        else if (IsEqualSCID(*pscid, SCID_DESCRIPTIONID))
        {
            SHDESCRIPTIONID did;
            switch(((SIL_GetType(pidl) & SHID_TYPEMASK) & ~(SHID_FS_UNICODE | SHID_FS_COMMONITEM)) | SHID_FS)
            {
                case SHID_FS_FILE:      did.dwDescriptionId = SHDID_FS_FILE;      break;
                case SHID_FS_DIRECTORY: did.dwDescriptionId = SHDID_FS_DIRECTORY; break;
                default:                did.dwDescriptionId = SHDID_FS_OTHER;     break;
            }
            FS_GetCLSID(pidf, &did.clsid);

            hr = InitVariantFromBuffer(pv, &did, sizeof(did));
        }
        else    // if (Column Handler)
        {
            int iCol = FindSCID(c_fs_cols, ARRAYSIZE(c_fs_cols), pscid);
            if (iCol >= 0)
            {
                switch (iCol)
                {
                case FS_ICOL_SIZE:
                    if (FS_IsFolder(pidf))
                        hr = E_FAIL;
                    else
                    {
                        FS_GetSize(this->_pidl, pidf, &pv->ullVal);
                        pv->vt = VT_UI8;
                        hr = S_OK;
                    }
                    break;

                case FS_ICOL_MODIFIED:
                    {
                        if (DosDateTimeToVariantTime(pidf->fs.dateModified, pidf->fs.timeModified, &pv->date))
                        {
                            pv->vt = VT_DATE;
                            hr = S_OK;
                        }
                    }
                    break;
                    
                default:
                    {
                        SHELLDETAILS sd;

                        // Note that GetDetailsOf expects a relative pidl, since it is passed the SF itself.
                        // The columnid includes the absolute pidl, though.z
                        hr = FS_GetDetailsOf(this, pidl, iCol, &sd);
                        if (SUCCEEDED(hr))
                        {
                            hr = InitVariantFromStrRet(&sd.str, pidl, pv);
                        }
                    }
                }
            }
            else
            {
                int i;
                CFSFolder_LoadColumnHandlers(this);
                for (i = 0; i < DSA_GetItemCount(this->_hdsaColHandlers); i++)
                {
                    COLUMNLISTENTRY *pcle = DSA_GetItemPtr(this->_hdsaColHandlers, i);

                    if (IsEqualSCID(*pscid, pcle->shci.scid))
                    {
                        SHCOLUMNDATA shcd;

                        hr = FS_InitShellColumnData(this, pidf, &shcd);
                        if (SUCCEEDED(hr))
                        {
                            VariantInit(pv);

                            hr = pcle->pcp->lpVtbl->GetItemData(pcle->pcp, pscid, &shcd, pv);
                            if (S_OK == hr)
                                break;
                        }
                        else
                            break;
                    }
                }
            }
        }
    }

    return hr;
}

//  99/02/05 #159157 vtan: Respect any previously stored settings
//  from NT4 to automatically show attributes column by default
//  when no overriding defaults have been given (#226140).

BOOL HasShowAttribColRegistryKey (void)
{
    DWORD dwValue, dwValueSize = sizeof(dwValue);
    return (SHGetValue(HKEY_CURRENT_USER,
                       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                       TEXT("ShowAttribCol"),
                       NULL, &dwValue, &dwValueSize) == ERROR_SUCCESS) &&
           (dwValue != 0);
}

STDMETHODIMP CFSFolder_GetDefaultColumn(IShellFolder2 *psf, DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFSFolder_GetDefaultColumnState(IShellFolder2 *psf, UINT iColumn, DWORD *pdwState)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    HRESULT hr = S_OK;

    *pdwState = 0;

    if (iColumn < ARRAYSIZE(c_fs_cols))
    {
        if (iColumn == FS_ICOL_MODIFIED)
            *pdwState |= SHCOLSTATE_TYPE_DATE;
        else if (iColumn == FS_ICOL_SIZE)
            *pdwState |= SHCOLSTATE_TYPE_INT;
        else
            *pdwState |= SHCOLSTATE_TYPE_STR;

        if ((iColumn != FS_ICOL_ATTRIB) || HasShowAttribColRegistryKey())
            *pdwState |= SHCOLSTATE_ONBYDEFAULT;
    }
    else
    {
        iColumn -= ARRAYSIZE(c_fs_cols);
        CFSFolder_LoadColumnHandlers( this );

        if (iColumn < this->_dwColCount)
        {
            COLUMNLISTENTRY cle;

            CFSFolder_FindColHandler(this, iColumn, 0, &cle);
            *pdwState |= (cle.shci.csFlags | SHCOLSTATE_EXTENDED | SHCOLSTATE_SLOW); 
        }
        else
            hr = E_INVALIDARG;
    }
    return hr;
}

STDMETHODIMP CFSFolder_GetDetailsEx(IShellFolder2 *psf, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    return FS_GetDetailsEx(this, pidl, pscid, pv);
}

STDMETHODIMP CFSFolder_GetDetailsOf(IShellFolder2 *psf, LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    return FS_GetDetailsOf(this, pidl, iColumn, pDetails);
}

STDMETHODIMP CFSFolder_MapColumnToSCID(IShellFolder2 *psf, UINT iColumn, SHCOLUMNID *pscid)
{
    CFSFolder *this = IToClass(CFSFolder, sf, psf);
    HRESULT hr = MapColumnToSCIDImpl(c_fs_cols, ARRAYSIZE(c_fs_cols), iColumn, pscid);
    if (hr != S_OK)
    {
        COLUMNLISTENTRY cle;
        CFSFolder_LoadColumnHandlers(this);

        iColumn -= ARRAYSIZE(c_fs_cols);

        if (CFSFolder_FindColHandler(this, iColumn, 0, &cle))
        {
            *pscid = cle.shci.scid;
            hr = S_OK;
        }
    }
    return hr;
}

IShellFolder2Vtbl c_FSFolderVtbl =
{
    CFSFolder_QueryInterface, CFSFolder_AddRef, CFSFolder_Release,
    CFSFolder_ParseDisplayName,
    CFSFolder_EnumObjects,
    CFSFolder_BindToObject,
    CFSFolder_BindToStorage,
    CFSFolder_CompareIDs,
    CFSFolder_CreateViewObject,
    CFSFolder_GetAttributesOf,
    CFSFolder_GetUIObjectOf,
    CFSFolder_GetDisplayNameOf,
    CFSFolder_SetNameOf,
    FindFileOrFolders_GetDefaultSearchGUID,
    CFSFolder_EnumSearches,
    CFSFolder_GetDefaultColumn,
    CFSFolder_GetDefaultColumnState,
    CFSFolder_GetDetailsEx,
    CFSFolder_GetDetailsOf,
    CFSFolder_MapColumnToSCID,
};

//
// N ways to get a clasid for an item
//
BOOL GetBindCLSID(IBindCtx *pbc, LPCIDFOLDER pidf, REFCLSID rclsidBind, CLSID *pclsid)
{
    DWORD dwClassFlags = SHGetClassFlags(pidf);
    if (dwClassFlags & SHCF_IS_DOCOBJECT)
    {
        *pclsid = CLSID_CDocObjectFolder;
    }
    else if (dwClassFlags & SHCF_IS_SHELLEXT)
    {
        FSGetCLSIDFromPidf(pidf, pclsid);   // .ZIP, .CAB, etc
    }
    else if (FS_GetCLSID(pidf, pclsid))
    {
        // *pclsid has the value

        // HACK: CLSID_Briefcase is use to identify the briefcase
        // but it's InProcServer is syncui.dll. we need to map that CLSID
        // to the object implemented in shell32 (CLSID_BriefcaseFolder)

        if (IsEqualCLSID(pclsid, &CLSID_Briefcase))
            *pclsid = CLSID_BriefcaseFolder;
    }
    else if (!IsEqualCLSID(&CLSID_NULL, rclsidBind))
    {
        *pclsid = *rclsidBind;  // briefcase forces all children this way
    }
    else
    {
        return FALSE;   // do normal binding
    }

    // TRUE -> special binding, FALSE -> normal file system binding
    return !SHSkipJunctionBinding(pbc, pclsid);
}


// handle multiple init interfaces as well as thunking from TCHAR

HRESULT _InitFolder(CFSFolder *this, IBindCtx *pbc, 
                    LPCITEMIDLIST pidlInit, LPITEMIDLIST pidlTarget,
                    LPCIDFOLDER pidf, IUnknown *punkFolder)
{
    HRESULT hres;
    IPersistFolder *ppf;
    IPersistFolder3 *ppf3;

    hres = punkFolder->lpVtbl->QueryInterface(punkFolder, &IID_IPersistFolder3, (void **)&ppf3);
    if (SUCCEEDED(hres))
    {
        PERSIST_FOLDER_TARGET_INFO pfti = {0};

        ASSERT(FSFindJunctionNext(pidf) == NULL);     // no extra goo please
            
        hres = CFSFolder_GetPathForItemW(this, pidf, pfti.szTargetParsingName);
        if ( SUCCEEDED(hres) )
        {
            pfti.pidlTargetFolder = (LPITEMIDLIST)pidlTarget;

            if (this->_pszNetProvider)
                SHTCharToUnicode(this->_pszNetProvider, pfti.szNetworkProvider, ARRAYSIZE(pfti.szNetworkProvider));

            pfti.dwAttributes = FS_FindLastID(pidf)->fs.wAttrs;
            pfti.csidl = -1;

            hres = ppf3->lpVtbl->InitializeEx(ppf3, pbc, pidlInit, pidlTarget ? &pfti : NULL);
        }

        ppf3->lpVtbl->Release(ppf3);
        return hres;
    }

    hres = punkFolder->lpVtbl->QueryInterface(punkFolder, &IID_IPersistFolder, &ppf);
    if (SUCCEEDED(hres))
    {
        hres = ppf->lpVtbl->Initialize(ppf, pidlInit);
        ppf->lpVtbl->Release(ppf);

        if (hres == E_NOTIMPL)  // map E_NOTIMPL into success, the folder does not care
            hres = S_OK;
    }
    return hres;
}

//
// Requires:
//  pidl points a file system object (either file or directory)
//
// Parameters:
//  rclsid -- known clsid of the folder we want to bind to.
//            Use CLSID_NULL if not known (typically the case).
//  pidl   -- Absolute IDList
//  riid   -- Required interface
//  ppv -- Points to the variable in which the interface
//            pointer should be returned.
//
HRESULT FS_Bind(CFSFolder *this, LPBC pbc, LPCIDFOLDER pidf, REFIID riid, void **ppv)
{
    LPITEMIDLIST pidlInit = NULL;
    LPITEMIDLIST pidlTarget = NULL;
    HRESULT hres;

    *ppv = NULL;

    hres = SHILCombine(this->_pidl, (LPITEMIDLIST)pidf, &pidlInit);

    if ( SUCCEEDED(hres) )
    {
        if ( this->_csidlTrack >= 0 )
        {
            LPITEMIDLIST pidl;
            // SHGetSpecialFolderlocation will return error if the target
            // doesn't exist (which is good, since that means there's
            // nothing to bind to).
            hres = SHGetSpecialFolderLocation(NULL, this->_csidlTrack, &pidl);
            if (SUCCEEDED(hres))
            {
                hres = SHILCombine(pidl, (LPITEMIDLIST)pidf, &pidlTarget);
                ILFree(pidl);
            }
        }
        else if ( this->_pidlTarget )
            hres = SHILCombine(this->_pidlTarget, (LPITEMIDLIST)pidf, &pidlTarget);
    }

    if (SUCCEEDED(hres))
    {
        CLSID clsid;
        LPCIDFOLDER pidfLast;

        ASSERT(FSFindJunctionNext(pidf) == NULL);     // no extra goo please

        pidfLast = FS_FindLastID(pidf);

        if (GetBindCLSID(pbc, pidfLast, &this->_clsidBind, &clsid))
        {
            hres = SHExtCoCreateInstance(NULL, &clsid, NULL, riid, ppv);
            if (SUCCEEDED(hres))
            {
                hres = _InitFolder(this, pbc, pidlInit, pidlTarget, pidf, (IUnknown *)*ppv);
                if (FAILED(hres))
                {
                    ((IUnknown *)*ppv)->lpVtbl->Release(*ppv);
                    *ppv = NULL;
                }
            }

            // We will try this second way if it's a file system folder and the name
            // extension handler didn't exist.
            if (FAILED(hres) && FS_IsFolder(pidfLast))
            {
                // the IShellFolder extension failed to load, so check if we
                // are allowed to default to the FS
                UINT dwFlags;

                if (_GetFolderFlags(this, pidf, &dwFlags) && (dwFlags & GFF_DEFAULT_TO_FS))
                {
                    goto CreateFSFolder;
                }
            }
        }
        else
        {
CreateFSFolder:
            hres = CFSFolder_CreateInstance(NULL, riid, ppv);
            if (SUCCEEDED(hres))
            {
                hres = _InitFolder(this, pbc, pidlInit, pidlTarget, pidf, (IUnknown *)*ppv);
                if (FAILED(hres))
                {
                    ((IUnknown *)*ppv)->lpVtbl->Release(*ppv);
                    *ppv = NULL;
                }
            }
        }
    }

    ILFree(pidlInit);
    ILFree(pidlTarget);

    ASSERT((SUCCEEDED(hres) && *ppv) || (FAILED(hres) && (NULL == *ppv)));   // Assert hres is consistent w/out param.
    return hres;
}

// returns:
//

HRESULT FS_GetJunctionForBind(LPCIDFOLDER pidf, LPIDFOLDER *ppidfBind, LPCITEMIDLIST *ppidlRight)
{
    *ppidfBind = NULL;

    *ppidlRight = FSFindJunctionNext(pidf);
    if (*ppidlRight)
    {
        *ppidfBind = FS_Clone(pidf);
        if (*ppidfBind)
        {
            // remove the part below the junction point
            _ILSkip(*ppidfBind, (ULONG)((ULONG_PTR)*ppidlRight - (ULONG_PTR)pidf))->mkid.cb = 0;
            return S_OK;
        }
        return E_OUTOFMEMORY;
    }
    return S_FALSE; // nothing interesting
}

//
//  This function returns the icon handle to be used to represent the specified
// file. The caller should destroy the icon eventually.
//
STDAPI_(HICON) SHGetFileIcon(HINSTANCE hinst, LPCTSTR pszPath, DWORD dwFileAttributes, UINT uFlags)
{
    SHFILEINFO sfi;
    SHGetFileInfo(pszPath, dwFileAttributes, &sfi, SIZEOF(sfi), uFlags | SHGFI_ICON);
    return sfi.hIcon;
}

// Return 1 on success and 0 on failure.
DWORD_PTR _GetFileInfoSections(LPITEMIDLIST pidl, SHFILEINFO *psfi, UINT uFlags)
{
    DWORD_PTR dwResult = 1;
    IShellFolder *psf;
    LPITEMIDLIST pidlLast;
    HRESULT hres = SHBindToIDListParent(pidl, &IID_IShellFolder, &psf, &pidlLast);
    if (SUCCEEDED(hres))
    {
        // get attributes for file
        if (uFlags & SHGFI_ATTRIBUTES)
        {
            // [New in IE 4.0] If SHGFI_ATTR_SPECIFIED is set, we use psfi->dwAttributes as is

            if (!(uFlags & SHGFI_ATTR_SPECIFIED))
                psfi->dwAttributes = 0xFFFFFFFF;      // get all of them

            if (FAILED(psf->lpVtbl->GetAttributesOf(psf, 1, &pidlLast, &psfi->dwAttributes)))
                psfi->dwAttributes = 0;
        }

        //
        // get icon location, place the icon path into szDisplayName
        //
        if (uFlags & SHGFI_ICONLOCATION)
        {
            IExtractIcon *pxi;

            if (SUCCEEDED(psf->lpVtbl->GetUIObjectOf(psf, NULL, 1, &pidlLast, &IID_IExtractIcon, NULL, &pxi)))
            {
                UINT wFlags;
                pxi->lpVtbl->GetIconLocation(pxi, 0, psfi->szDisplayName, ARRAYSIZE(psfi->szDisplayName),
                    &psfi->iIcon, &wFlags);

                pxi->lpVtbl->Release(pxi);

                // the returned location is not a filename we cant return it.
                // just give then nothing.
                if (wFlags & GIL_NOTFILENAME)
                {
                    // special case one of our shell32.dll icons......

                    if (psfi->szDisplayName[0] != TEXT('*'))
                        psfi->iIcon=0;

                    psfi->szDisplayName[0] = 0;
                }
            }
        }

        // get the icon for the file.
        if ((uFlags & SHGFI_SYSICONINDEX) || (uFlags & SHGFI_ICON))
        {
            if (g_himlIcons == NULL)
                FileIconInit( FALSE );

            if (uFlags & SHGFI_SYSICONINDEX)
                dwResult = (DWORD_PTR)((uFlags & SHGFI_SMALLICON) ? g_himlIconsSmall : g_himlIcons);

            if (uFlags & SHGFI_OPENICON)
                SHMapPIDLToSystemImageListIndex(psf, pidlLast, &psfi->iIcon);
            else
                psfi->iIcon = SHMapPIDLToSystemImageListIndex(psf, pidlLast, NULL);
        }

        if (uFlags & SHGFI_ICON)
        {
            HIMAGELIST himl;
            UINT flags = 0;
            int cx, cy;

            if (uFlags & SHGFI_SMALLICON)
            {
                himl = g_himlIconsSmall;
                cx = GetSystemMetrics(SM_CXSMICON);
                cy = GetSystemMetrics(SM_CYSMICON);
            }
            else
            {
                himl = g_himlIcons;
                cx = GetSystemMetrics(SM_CXICON);
                cy = GetSystemMetrics(SM_CYICON);
            }

            if (!(uFlags & SHGFI_ATTRIBUTES))
            {
                psfi->dwAttributes = SFGAO_LINK;    // get link only
                psf->lpVtbl->GetAttributesOf(psf, 1, &pidlLast, &psfi->dwAttributes);
            }

            //
            //  check for a overlay image thing (link overlay)
            //
            if ((psfi->dwAttributes & SFGAO_LINK) || (uFlags & SHGFI_LINKOVERLAY))
            {
                IShellIconOverlayManager *psiom;
                HRESULT hresT = GetIconOverlayManager(&psiom);
                if (SUCCEEDED(hresT))
                {
                    int iOverlayIndex = 0;
                    hresT = psiom->lpVtbl->GetReservedOverlayInfo(psiom, NULL, -1, &iOverlayIndex, SIOM_OVERLAYINDEX, SIOM_RESERVED_LINK);
                    if (SUCCEEDED(hresT))
                        flags |= INDEXTOOVERLAYMASK(iOverlayIndex);
                }
            }
            if (( uFlags & SHGFI_ADDOVERLAYS ) || ( uFlags & SHGFI_OVERLAYINDEX ))
            {
                IShellIconOverlay * pio = NULL;
                if ( SUCCEEDED( psf->lpVtbl->QueryInterface(psf, &IID_IShellIconOverlay, (LPVOID *) &pio )))
                {
                    int iOverlayIndex = 0;
                    if ( SUCCEEDED( pio->lpVtbl->GetOverlayIndex(pio, pidlLast, &iOverlayIndex)))
                    {
                        if ( uFlags & SHGFI_ADDOVERLAYS )
                        {
                            flags |= INDEXTOOVERLAYMASK(iOverlayIndex);
                        }
                        if ( uFlags & SHGFI_OVERLAYINDEX )
                        {
                            // use the upper 16 bits for the overlayindex
                            psfi->iIcon |= iOverlayIndex << 24;
                        }
                    }
                    pio->lpVtbl->Release(pio);
                }
            }
            
            
            //  check for selected state
            if (uFlags & SHGFI_SELECTED)
                flags |= ILD_BLEND50;

            psfi->hIcon = ImageList_GetIcon(himl, psfi->iIcon, flags);

            // if the caller does not want a "shell size" icon
            // convert the icon to the "system" icon size.
            if (psfi->hIcon && !(uFlags & SHGFI_SHELLICONSIZE))
                psfi->hIcon = CopyImage(psfi->hIcon, IMAGE_ICON, cx, cy, LR_COPYRETURNORG | LR_COPYDELETEORG);
        }

        // get display name for the path
        if (uFlags & SHGFI_DISPLAYNAME)
        {
            STRRET str;
            HRESULT hres = psf->lpVtbl->GetDisplayNameOf(psf, pidlLast, SHGDN_NORMAL, &str);
            if (SUCCEEDED(hres))
                StrRetToBuf(&str, pidlLast, psfi->szDisplayName, ARRAYSIZE(psfi->szDisplayName));
            else
            {
                DebugMsg(TF_WARNING, TEXT("_GetFileInfoSections(), hres:%x bad PIDL %s"), hres, DumpPidl(pidlLast));
                // dwResult = 0;
            }
        }

        if (uFlags & SHGFI_TYPENAME)
        {
            IShellFolder2 *psf2;
            if (SUCCEEDED(psf->lpVtbl->QueryInterface(psf, &IID_IShellFolder2, (void **)&psf2)))
            {
                VARIANT var;
                VariantInit(&var);
                if (SUCCEEDED(psf2->lpVtbl->GetDetailsEx(psf2, pidlLast, &SCID_TYPE, &var)))
                {
                    VariantToStr(&var, psfi->szTypeName, ARRAYSIZE(psfi->szTypeName));
                    VariantClear(&var);
                }
                psf2->lpVtbl->Release(psf2);
            }
        }

        psf->lpVtbl->Release(psf);
    }
    else
        dwResult = 0;

    return dwResult;
}


//===========================================================================
//
// SHGetFileInfo
//
//  This function returns shell info about a given pathname.
//  a app can get the following:
//
//      Icon (large or small)
//      Display Name
//      Name of File Type
//
//  this function replaces SHGetFileIcon
//
//===========================================================================

STDAPI_(DWORD_PTR) SHGetFileInfo(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO *psfi, UINT cbFileInfo, UINT uFlags)
{
    LPITEMIDLIST pidlFull;
    DWORD_PTR res = 1;
    TCHAR szPath[MAX_PATH];

    // this was never enforced in the past
    ASSERT(psfi ? cbFileInfo == SIZEOF(*psfi) : TRUE);

    // You can't use both SHGFI_ATTR_SPECIFIED and SHGFI_ICON.
    ASSERT(uFlags & SHGFI_ATTR_SPECIFIED ? !(uFlags & SHGFI_ICON) : TRUE);

    if (pszPath == NULL )
        return 0;

    if (uFlags == SHGFI_EXETYPE)
        return GetExeType(pszPath);     // funky way to get EXE type

    if (psfi == NULL)
        return 0;

    psfi->hIcon = 0;

    // Zip Pro 6.0 relies on the fact that if you don't ask for the icon,
    // the iIcon field doesn't change.
    //
//  psfi->iIcon = 0;

    psfi->szDisplayName[0] = 0;
    psfi->szTypeName[0] = 0;

    //  do some simmple check on the input path.
    if (!(uFlags & SHGFI_PIDL))
    {
        // If the caller wants us to give them the file attributes, we can't trust
        // the attributes they gave us in the following two situations.
        if (uFlags & SHGFI_ATTRIBUTES)
        {
            if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)))
            {
                DebugMsg(TF_FSTREE, TEXT("SHGetFileInfo cant use caller supplied file attribs for a sys/ro directory (possible junction)"));
                uFlags &= ~SHGFI_USEFILEATTRIBUTES;
            }
            else if (PathIsRoot(pszPath))
            {
                DebugMsg(TF_FSTREE, TEXT("SHGetFileInfo cant use caller supplied file attribs for a roots"));
                uFlags &= ~SHGFI_USEFILEATTRIBUTES;
            }
        }

        if (PathIsRelative(pszPath))
        {
            GetCurrentDirectory(ARRAYSIZE(szPath), szPath);
            PathCombine(szPath, szPath, pszPath);
            pszPath = szPath;
        }
    }

    if (uFlags & SHGFI_PIDL)
        pidlFull = (LPITEMIDLIST)pszPath;
    else if (uFlags & SHGFI_USEFILEATTRIBUTES)
    {
        WIN32_FIND_DATA fd = {0};
        fd.dwFileAttributes = dwFileAttributes;
        SHSimpleIDListFromFindData(pszPath, &fd, &pidlFull);
    }
    else
        pidlFull = ILCreateFromPath(pszPath);

    if (pidlFull)
    {
        if (uFlags & (
            SHGFI_DISPLAYNAME   |
            SHGFI_ATTRIBUTES    |
            SHGFI_SYSICONINDEX  |
            SHGFI_ICONLOCATION  |
            SHGFI_ICON          | 
            SHGFI_TYPENAME))
        {
            res = _GetFileInfoSections(pidlFull, psfi, uFlags);
        }

        if (!(uFlags & SHGFI_PIDL))
            ILFree(pidlFull);
    }
    else
        res = 0;

    return res;
}


#ifdef UNICODE
//===========================================================================
//
// SHGetFileInfoA Stub
//
//  This function calls SHGetFileInfoW and then converts the returned
//  information back to ANSI.
//
//===========================================================================
STDAPI_(DWORD_PTR) SHGetFileInfoA(LPCSTR pszPath, DWORD dwFileAttributes, SHFILEINFOA *psfi, UINT cbFileInfo, UINT uFlags)
{
    WCHAR szPathW[MAX_PATH];
    LPWSTR pszPathW;
    DWORD_PTR dwRet;

    if (uFlags & SHGFI_PIDL)
    {
        pszPathW = (LPWSTR)pszPath;     // Its a pidl, fake it as a WSTR
    }
    else
    {
        SHAnsiToUnicode(pszPath, szPathW, ARRAYSIZE(szPathW));
        pszPathW = szPathW;
    }
    if (psfi)
    {
        SHFILEINFOW sfiw;

        ASSERT(cbFileInfo == SIZEOF(*psfi));

        // Zip Pro 6.0 sets SHGFI_SMALLICON | SHGFI_OPENICON but forgets to
        // pass SHGFI_ICON or SHGFI_SYSICONINDEX, even though they really
        // wanted the sys icon index.
        //
        // In Windows 95, fields of the SHFILEINFOA structure that you didn't
        // query for were left unchanged.  They happened to have the icon for
        // a closed folder lying around there from a previous query, so they
        // got away with it by mistake.  They got the wrong icon, but it was
        // close enough that nobody really complained.
        //
        // So pre-initialize the sfiw's iIcon with the app's iIcon.  That
        // way, if it turns out the app didn't ask for the icon, he just
        // gets his old value back.
        //

        sfiw.iIcon = psfi->iIcon;
        sfiw.dwAttributes = psfi->dwAttributes;

        dwRet = SHGetFileInfoW(pszPathW, dwFileAttributes, &sfiw, SIZEOF(sfiw), uFlags);

        psfi->hIcon = sfiw.hIcon;
        psfi->iIcon = sfiw.iIcon;
        psfi->dwAttributes = sfiw.dwAttributes;
        SHUnicodeToAnsi(sfiw.szDisplayName, psfi->szDisplayName, ARRAYSIZE(psfi->szDisplayName));
        SHUnicodeToAnsi(sfiw.szTypeName, psfi->szTypeName, ARRAYSIZE(psfi->szTypeName));
    }
    else
    {
        dwRet = SHGetFileInfoW(pszPathW, dwFileAttributes, NULL, 0, uFlags);
    }
    return dwRet;
}
#else
STDAPI_(DWORD_PTR) SHGetFileInfoW(LPCWSTR pszPath, DWORD dwFileAttributes, SHFILEINFOW *psfi, UINT cbFileInfo, UINT uFlags)
{
    return 0;
}
#endif


//===========================================================================
//
// SHGetDataFromIDList
//
//  This function will extract information that is cached in the pidl such
//  in the information that was returned from a FindFirst file.  This function
//  is sortof a hack as t allow outside callers to be able to get at the infomation
//  without knowing how we store it in the pidl.
//  a app can get the following:
//===========================================================================

STDAPI ThunkFindDataWToA(WIN32_FIND_DATAW *pfd, WIN32_FIND_DATAA *pfda, int cb)
{
    if (cb < SIZEOF(WIN32_FIND_DATAA))
        return DISP_E_BUFFERTOOSMALL;

    memcpy(pfda, pfd, FIELD_OFFSET(WIN32_FIND_DATAA, cFileName));

    SHUnicodeToAnsi(pfd->cFileName, pfda->cFileName, ARRAYSIZE(pfda->cFileName));
    SHUnicodeToAnsi(pfd->cAlternateFileName, pfda->cAlternateFileName, ARRAYSIZE(pfda->cAlternateFileName));
    return S_OK;
}

STDAPI ThunkNetResourceWToA(LPNETRESOURCEW pnrw, LPNETRESOURCEA pnra, UINT cb)
{
    HRESULT hres;

    if (cb >= SIZEOF(NETRESOURCEA))
    {
        LPSTR psza, pszDest[4] = {NULL, NULL, NULL, NULL};

        CopyMemory(pnra, pnrw, FIELD_OFFSET(NETRESOURCE, lpLocalName));

        psza = (LPSTR)(pnra + 1);   // Point just past the structure
        if (cb > SIZEOF(NETRESOURCE))
        {
            LPWSTR pszSource[4];
            UINT i, cchRemaining = cb - SIZEOF(NETRESOURCE);

            pszSource[0] = pnrw->lpLocalName;
            pszSource[1] = pnrw->lpRemoteName;
            pszSource[2] = pnrw->lpComment;
            pszSource[3] = pnrw->lpProvider;

            for (i = 0; i < 4; i++)
            {
                if (pszSource[i])
                {
                    UINT cchItem;
                    pszDest[i] = psza;
                    cchItem = SHUnicodeToAnsi(pszSource[i], pszDest[i], cchRemaining);
                    cchRemaining -= cchItem;
                    psza += cchItem;
                }
            }

        }
        pnra->lpLocalName  = pszDest[0];
        pnra->lpRemoteName = pszDest[1];
        pnra->lpComment    = pszDest[2];
        pnra->lpProvider   = pszDest[3];
        hres = S_OK;
    }
    else
        hres = DISP_E_BUFFERTOOSMALL;
    return hres;
}

STDAPI NetResourceWVariantToBuffer(const VARIANT* pvar, void* pv, UINT cb)
{
    HRESULT hres;

    if (cb >= SIZEOF(NETRESOURCEW))
    {
        if (pvar && pvar->vt == (VT_ARRAY | VT_UI1))
        {
            int i;
            NETRESOURCEW* pnrw = (NETRESOURCEW*) pvar->parray->pvData;
            UINT cbOffsets[4] = { 0, 0, 0, 0 };
            UINT cbEnds[4] = { 0, 0, 0, 0 };
            LPWSTR pszPtrs[4] = { pnrw->lpLocalName, pnrw->lpRemoteName,
                                  pnrw->lpComment, pnrw->lpProvider };
            hres = S_OK;
            for (i = 0; i < ARRAYSIZE(pszPtrs); i++)
            {
                if (pszPtrs[i])
                {
                    cbOffsets[i] = (UINT) ((BYTE*) pszPtrs[i] - (BYTE*) pnrw);
                    cbEnds[i] = cbOffsets[i] + (sizeof(WCHAR) * (lstrlenW(pszPtrs[i]) + 1));
                
                    // If any of the strings start or end too far into the buffer, then fail:
                    if ((cbOffsets[i] >= cb) || (cbEnds[i] > cb))
                    {
                        hres = DISP_E_BUFFERTOOSMALL;
                        break;
                    }
                }
            }
            if (SUCCEEDED(hres))
            {
                hres = VariantToBuffer(pvar, pv, cb) ? S_OK : E_FAIL;
                pnrw = (NETRESOURCEW*) pv;
                if (SUCCEEDED(hres))
                {
                    // Fixup pointers in structure to point into the output buffer,
                    // instead of the variant buffer:
                    LPWSTR* ppszPtrs[4] = { &(pnrw->lpLocalName), &(pnrw->lpRemoteName),
                                            &(pnrw->lpComment), &(pnrw->lpProvider) };
                                
                    for (i = 0; i < ARRAYSIZE(ppszPtrs); i++)
                    {
                        if (*ppszPtrs[i])
                        {
                            *ppszPtrs[i] = (LPWSTR) ((BYTE*) pnrw + cbOffsets[i]);
                        }
                    }
                }
            }
        }
        else
        {
            hres = E_FAIL;
        }
    }
    else
    {
        hres = DISP_E_BUFFERTOOSMALL;
    }
    return hres;
}

STDAPI SHGetDataFromIDListW(IShellFolder *psf, LPCITEMIDLIST pidl, int nFormat, void *pv, int cb)
{
    HRESULT hres = E_NOTIMPL;
    IShellFolder2 *psf2;
    SHCOLUMNID* pscid;

    if (!pv || !psf || !pidl)
        return E_INVALIDARG;

    switch (nFormat)
    {
        case SHGDFIL_FINDDATA:
            if (cb < SIZEOF(WIN32_FIND_DATAW))
                return DISP_E_BUFFERTOOSMALL;
            else
                pscid = (SHCOLUMNID*)&SCID_FINDDATA;
            break;
        case SHGDFIL_NETRESOURCE:
            if (cb < SIZEOF(NETRESOURCEW))
                return  DISP_E_BUFFERTOOSMALL;
            else
                pscid = (SHCOLUMNID*)&SCID_NETRESOURCE;
            break;
        case SHGDFIL_DESCRIPTIONID:
            pscid = (SHCOLUMNID*)&SCID_DESCRIPTIONID;
            break;
        default:
            return E_INVALIDARG;
    }


    if (SUCCEEDED(psf->lpVtbl->QueryInterface(psf, &IID_IShellFolder2, (void **)&psf2)))
    {
        VARIANT var;
        VariantInit(&var);
        hres = psf2->lpVtbl->GetDetailsEx(psf2, pidl, pscid, &var);
        if (SUCCEEDED(hres))
        {
            if (SHGDFIL_NETRESOURCE == nFormat)
            {
                hres = NetResourceWVariantToBuffer(&var, pv, cb);
            }
            else
            {
                if (!VariantToBuffer(&var, pv, cb))
                    hres = E_FAIL;
            }
            VariantClear(&var);
        }
        else
        {
            TraceMsg(TF_WARNING, "Trying to retrieve find data from unknown PIDL %s", DumpPidl(pidl));
        }

        psf2->lpVtbl->Release(psf2);
    }

    return hres;
}

STDAPI SHGetDataFromIDListA(IShellFolder *psf, LPCITEMIDLIST pidl, int nFormat, void *pv, int cb)
{
    HRESULT hres;
    WIN32_FIND_DATAW fdw;
    NETRESOURCEW *pnrw = NULL;
    void *pvData = pv;
    int cbData = cb;

    if (nFormat == SHGDFIL_FINDDATA)
    {
        cbData = SIZEOF(fdw);
        pvData = &fdw;
    }
    else if (nFormat == SHGDFIL_NETRESOURCE)
    {
        cbData = cb;
        pvData = pnrw = (NETRESOURCEW *)LocalAlloc(LPTR, cbData);
        if (pnrw == NULL)
            return E_OUTOFMEMORY;
    }

    hres = SHGetDataFromIDListW(psf, pidl, nFormat, pvData, cbData);

    if (SUCCEEDED(hres))
    {
        if (nFormat == SHGDFIL_FINDDATA)
        {
            hres = ThunkFindDataWToA(&fdw, (WIN32_FIND_DATAA *)pv, cb);
        }
        else if (nFormat == SHGDFIL_NETRESOURCE)
        {
            hres = ThunkNetResourceWToA(pnrw, (NETRESOURCEA *)pv, cb);
        }
    }

    if (pnrw)
        LocalFree(pnrw);

    return hres;
}

//===========================================================================
// CFSFolder::IShellIcon : Members
//===========================================================================

STDMETHODIMP CFSFolder_Icon_QueryInterface(IShellIcon *psi, REFIID riid, void **ppvObj)
{
    CFSFolder *this = IToClass(CFSFolder, si, psi);
    return this->punkOuter->lpVtbl->QueryInterface(this->punkOuter, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CFSFolder_Icon_AddRef(IShellIcon *psi)
{
    CFSFolder *this = IToClass(CFSFolder, si, psi);
    return this->punkOuter->lpVtbl->AddRef(this->punkOuter);
}

STDMETHODIMP_(ULONG) CFSFolder_Icon_Release(IShellIcon *psi)
{
    CFSFolder *this = IToClass(CFSFolder, si, psi);
    return this->punkOuter->lpVtbl->Release(this->punkOuter);
}

//
// GetIconOf
//
STDMETHODIMP CFSFolder_Icon_GetIconOf(IShellIcon *psi, LPCITEMIDLIST pidl, UINT flags, int *piIndex)
{
    CFSFolder *this = IToClass(CFSFolder, si, psi);
    DWORD dwFlags;
    int iIcon = -1;
    LPCIDFOLDER pidf = FS_IsValidID(pidl);

    if (!pidf)
    {
        ASSERT(SIL_GetType(pidl) == SHID_ROOT_REGITEM); // regitems gives us these
        return S_FALSE;
    }

    // WARNING: don't include junctions (FS_IsFileFolder(pidf))
    // so junctions like briefcase get their own cusotm icon.
    //
    if (FS_IsFileFolder(pidf))
    {
        TCHAR szMountPoint[MAX_PATH];
        TCHAR szModule[MAX_PATH];

        iIcon = II_FOLDER;
        if (FS_GetMountingPointInfo(this, pidf, szMountPoint, ARRAYSIZE(szMountPoint)))
        {
            iIcon = GetMountedVolumeIcon(szMountPoint, szModule, ARRAYSIZE(szModule));

            *piIndex = Shell_GetCachedImageIndex(szModule[0] ? szModule : c_szShell32Dll, iIcon, 0);
            return S_OK;
        }
        else
        {
            if (!FS_IsSystemFolder(pidf) && (CFSFolder_GetCSIDL(this) == CSIDL_NORMAL))
            {
                if (flags & GIL_OPENICON)
                    iIcon = II_FOLDEROPEN;
                else
                    iIcon = II_FOLDER;

                *piIndex = Shell_GetCachedImageIndex(c_szShell32Dll, iIcon, 0);
                return S_OK;
            }
            iIcon = II_FOLDER;
            dwFlags = SHCF_ICON_PERINSTANCE;
        }
    }
    else
        dwFlags = SHGetClassFlags(pidf);


    // the icon is per-instance, try to look it up
    if (dwFlags & SHCF_ICON_PERINSTANCE)
    {
        DWORD uid = FS_GetUID(pidf);    // get a unique identifier for this file.
        HRESULT hres;
        TCHAR szTmp[MAX_PATH];
        IShellFolder *psf;

        if (uid == 0)
            return S_FALSE;

        //
        // look for entry in the icon cache.
        //
        FS_CopyName(pidf, szTmp, ARRAYSIZE(szTmp));
        *piIndex = LookupIconIndex(szTmp, uid, flags | GIL_NOTFILENAME);

        if (*piIndex != -1)
            return S_OK;

        //
        //  async extract (GIL_ASYNC) support
        //
        //  we cant find the icon in the icon cache, we need to do real work
        //  to get the icon.  if the caller specified GIL_ASYNC
        //  dont do the work, return E_PENDING forcing the caller to call
        //  back later to get the real icon.
        //
        //  when returing E_PENDING we must fill in a default icon index
        //
        if (flags & GIL_ASYNC)
        {
            //
            // come up with a default icon and return E_PENDING
            //
            if (FS_IsFolder(pidf))
                iIcon = II_FOLDER;
            else if (!(dwFlags & SHCF_HAS_ICONHANDLER) && PathIsExe(szTmp))
                iIcon = II_APPLICATION;
            else
                iIcon = II_DOCNOASSOC;

            *piIndex = Shell_GetCachedImageIndex(c_szShell32Dll, iIcon, 0);

            return E_PENDING;   // we will be called back later for the real one
        }

        //
        // If this is a folder, see if this folder has Per-Instance folder icon
        // we do this here because it's too expensive to open a desktop.ini
        // file and see what's in there. Most of the cases we will just hit
        // the above cases
        //
        if (FS_IsSystemFolder(pidf))
        {
            if (!_GetFolderIconPath(this, pidf, NULL, 0, NULL))
            {
                // Note: the iIcon value has already been computed at the start of this funciton
                ASSERT(iIcon != -1);
                *piIndex = Shell_GetCachedImageIndex(c_szShell32Dll, iIcon, 0);
                return S_OK;
            }
        }

        //
        // look up icon using IExtractIcon, this will load handler iff needed
        // by calling ::GetUIObjectOf
        //
        hres = psi->lpVtbl->QueryInterface(psi, &IID_IShellFolder, &psf);
        if (SUCCEEDED(hres))
        {
            hres = SHGetIconFromPIDL(psf, NULL, (LPCITEMIDLIST)pidf, flags, piIndex);
            psf->lpVtbl->Release(psf);
        }

        //
        // remember this perinstance icon in the cache so we dont
        // need to load the handler again.
        //
        // SHGetIconFromPIDL will always return a valid image index
        // (it may default to a standard one) but it will fail
        // if the file cant be accessed or some other sort of error.
        // we dont want to cache in this case.
        //
        if (SUCCEEDED(hres) && (dwFlags & SHCF_HAS_ICONHANDLER))
        {
            AddToIconTable(szTmp, uid, flags | GIL_NOTFILENAME, *piIndex);
        }

        return *piIndex == -1 ? S_FALSE : S_OK;
    }

    // icon is per-class dwFlags has the image index
    *piIndex = (dwFlags & SHCF_ICON_INDEX);
    return S_OK;
}

IShellIconVtbl c_FSFolderIconVtbl =
{
    CFSFolder_Icon_QueryInterface, CFSFolder_Icon_AddRef, CFSFolder_Icon_Release,
    CFSFolder_Icon_GetIconOf,
};



//===========================================================================
// CFSFolder::IShellIconOverlay : Members
//===========================================================================
HANDLE g_hOverlayMgrCounter = NULL;   // Global count of Overlay Manager changes.
int g_lOverlayMgrPerProcessCount = 0; // Per process count of Overlay Manager changes.

//
// Use this function to obtain address of the singleton icon overlay manager.
// If the function succeeds, caller is responsible for calling Release() through
// the returned interface pointer.
// The function ensures that the manager is initialized and up to date.
//
HRESULT GetIconOverlayManager(IShellIconOverlayManager **ppsiom)
{
    HRESULT hres = E_FAIL;

    if (IconOverlayManagerInit())
    { 
        //
        // Is a critsec for g_psiom required here you ask?
        //
        // No.  The first call to IconOverlayInit in any process creates
        // the overlay manager object and initializes g_psiom.  This creation
        // contributes 1 to the object's ref count.  Subsequent calls to
        // GetIconOverlayManager add to the ref count and the caller is
        // responsible for decrementing the count through Release().
        // The original ref count of 1 is not removed until 
        // IconOverlayManagerTerminate is called which happens only
        // during PROCESS_DETACH.  Therefore, the manager referenced by g_psiom
        // in this code block will always be valid and a critsec is not
        // required.
        //

        //
        // ID for the global overlay manager counter.
        //
        static const GUID GUID_Counter = { /* 090851a5-eb96-11d2-8be4-00c04fa31a66 */
                                           0x090851a5,
                                           0xeb96,
                                           0x11d2,
                                           {0x8b, 0xe4, 0x00, 0xc0, 0x4f, 0xa3, 0x1a, 0x66}
                                         };
        long lGlobalCount;
        HANDLE hCounter;
    
        g_psiom->lpVtbl->AddRef(g_psiom);
    
        hCounter     = SHGetCachedGlobalCounter(&g_hOverlayMgrCounter, &GUID_Counter);
        lGlobalCount = SHGlobalCounterGetValue(hCounter);

        if (lGlobalCount != g_lOverlayMgrPerProcessCount)
        {
            //
            // Per-process counter is out of sync with the global counter.
            // This means someone called SHLoadNonloadedIconOverlayIdentifiers
            // so we must load any non-loaded identifiers from the registry.
            //
            g_psiom->lpVtbl->LoadNonloadedOverlayIdentifiers(g_psiom);
            g_lOverlayMgrPerProcessCount = lGlobalCount;
        }
        *ppsiom = g_psiom;
        hres = S_OK;
    }
    return hres;
}


STDMETHODIMP CFSFolder_IconOverlay_QueryInterface(IShellIconOverlay *psio, REFIID riid, void **ppvObj)
{
    CFSFolder *this = IToClass(CFSFolder, sio, psio);
    return this->punkOuter->lpVtbl->QueryInterface(this->punkOuter, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CFSFolder_IconOverlay_AddRef(IShellIconOverlay *psio)
{
    CFSFolder *this = IToClass(CFSFolder, sio, psio);
    return this->punkOuter->lpVtbl->AddRef(this->punkOuter);
}

STDMETHODIMP_(ULONG) CFSFolder_IconOverlay_Release(IShellIconOverlay *psio)
{
    CFSFolder *this = IToClass(CFSFolder, sio, psio);
    return this->punkOuter->lpVtbl->Release(this->punkOuter);
}


BOOL IconOverlayManagerInit()
{
    if (!g_psiom)
    {
        IShellIconOverlayManager* psiom;
        if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_CFSIconOverlayManager, NULL, &IID_IShellIconOverlayManager, (void **)&psiom)))
        {
            if (SHInterlockedCompareExchange((void **)&g_psiom, psiom, 0))
                psiom->lpVtbl->Release(psiom);
#ifdef DEBUG
            else
            {
                GetAndRegisterLeakDetection();
                // g_psiom is freed at process detach time, after we dumped the mem leak stuff
                // so to avoid fake alarm we remove it from the mem list (reljai)
                if (g_fInitTable)
                    LeakDetFunctionTable.pfnremove_from_memlist(g_psiom);
            }
#endif 
        }
    }
    return BOOLFROMPTR(g_psiom);
}

void IconOverlayManagerTerminate()
{
    IShellIconOverlayManager * psiom;

    ASSERTDLLENTRY;      // does not require a critical section

    psiom = (IShellIconOverlayManager *)InterlockedExchangePointer((void **)&g_psiom, 0);
    if (psiom)
        psiom->lpVtbl->Release(psiom);

    if (NULL != g_hOverlayMgrCounter)
    {
        CloseHandle(g_hOverlayMgrCounter);
        g_hOverlayMgrCounter = NULL;
    }
}


STDAPI SHLoadNonloadedIconOverlayIdentifiers(void)
{
    //
    // This will cause the next call GetIconOverlayManager() call in each process
    // to load any non-loaded icon overlay identifiers.
    //
    if (g_hOverlayMgrCounter)
        SHGlobalCounterIncrement(g_hOverlayMgrCounter);

    return S_OK;
}


STDMETHODIMP CFSFolder_IconOverlay_GetOverlayInfo(IShellIconOverlay *psio, LPCITEMIDLIST pidl, int * pIndex, DWORD dwFlags)
{
    HRESULT hres = E_FAIL;
    CFSFolder *this = IToClass(CFSFolder, sio, psio);
    LPCIDFOLDER pidf = FS_IsValidID(pidl);

    if (!pidf)
    {
        ASSERT(SIL_GetType(pidl) != SHID_ROOT_REGITEM); // CRegFolder should have handled it
        return S_FALSE;
    }

    ASSERT(pidl == ILFindLastID(pidl));

    *pIndex = 0;

    if (IconOverlayManagerInit())
    {
        int iReservedID = -1;
        WCHAR wszPath[MAX_PATH];
        DWORD dwAttrib = pidf->fs.wAttrs;

        hres = CFSFolder_GetPathForItemW(this, pidf, wszPath);
        if (SUCCEEDED(hres))
        {
            IShellIconOverlayManager *psiom;
            // The order of the "if" statements here is significant

            if (FS_IsFile(pidf) && (SHGetClassFlags(pidf) & SHCF_IS_LINK))
                iReservedID = SIOM_RESERVED_LINK;
            else
            {
                USES_CONVERSION;
                LPCTSTR szPath = W2CT(wszPath);

                if (FS_IsFolder(pidf) && (IsShared(szPath, FALSE)))
                    iReservedID = SIOM_RESERVED_SHARED;
                else if (PathIsHighLatency(szPath, dwAttrib))
                    iReservedID = SIOM_RESERVED_SLOWFILE;
            }

            if (SUCCEEDED(hres = GetIconOverlayManager(&psiom)))
            {
                if (iReservedID != -1)
                    hres = psiom->lpVtbl->GetReservedOverlayInfo(psiom, wszPath, dwAttrib, pIndex, dwFlags, iReservedID);
                else
                    hres = psiom->lpVtbl->GetFileOverlayInfo(psiom, wszPath, dwAttrib, pIndex, dwFlags);

                psiom->lpVtbl->Release(psiom);
            }
        }
    }
    return hres;
}

//
// GetOverlayIndex
//

STDMETHODIMP CFSFolder_IconOverlay_GetOverlayIndex(IShellIconOverlay *psio, LPCITEMIDLIST pidl, int * pIndex)
{
    HRESULT hres = E_INVALIDARG;
    ASSERT(pIndex);
    if (pIndex)
        hres = (*pIndex == OI_ASYNC) ? E_PENDING :
               CFSFolder_IconOverlay_GetOverlayInfo(psio, pidl, pIndex, SIOM_OVERLAYINDEX);

    return hres;
}

STDMETHODIMP CFSFolder_IconOverlay_GetOverlayIconIndex(IShellIconOverlay *psio, LPCITEMIDLIST pidl, int * pIconIndex)
{
    return CFSFolder_IconOverlay_GetOverlayInfo(psio, pidl, pIconIndex, SIOM_ICONINDEX);
}

IShellIconOverlayVtbl c_FSFolderIconOverlayVtbl =
{
    CFSFolder_IconOverlay_QueryInterface, CFSFolder_IconOverlay_AddRef, CFSFolder_IconOverlay_Release,
    CFSFolder_IconOverlay_GetOverlayIndex,
    CFSFolder_IconOverlay_GetOverlayIconIndex,
};


// CFSFolder : IPersist, IPersistFolder, IPersistFolder2, IPersistFolderAlias Members

STDMETHODIMP CFSFolder_PF_QueryInterface(IPersistFolder3 *ppf, REFIID riid, void **ppvObj)
{
    CFSFolder *this = IToClass(CFSFolder, pf, ppf);
    return this->punkOuter->lpVtbl->QueryInterface(this->punkOuter, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CFSFolder_PF_AddRef(IPersistFolder3 *ppf)
{
    CFSFolder *this = IToClass(CFSFolder, pf, ppf);
    return this->punkOuter->lpVtbl->AddRef(this->punkOuter);
}

STDMETHODIMP_(ULONG) CFSFolder_PF_Release(IPersistFolder3 *ppf)
{
    CFSFolder *this = IToClass(CFSFolder, pf, ppf);
    return this->punkOuter->lpVtbl->Release(this->punkOuter);
}

STDMETHODIMP CFSFolder_PF_GetClassID(IPersistFolder3 *ppf, CLSID *pclsid)
{
    *pclsid = CLSID_ShellFSFolder;
    return NOERROR;
}

STDMETHODIMP CFSFolder_PF_Initialize(IPersistFolder3 *ppf, LPCITEMIDLIST pidl)
{
    CFSFolder *this = IToClass(CFSFolder, pf, ppf);
    CFSFolder_Reset(this);
    return SHILClone(pidl, &this->_pidl);
}

STDMETHODIMP CFSFolder_PF_GetCurFolder(IPersistFolder3 *ppf, LPITEMIDLIST *ppidl)
{
    CFSFolder *this = IToClass(CFSFolder, pf, ppf);
    return GetCurFolderImpl(this->_pidl, ppidl);
}

LPTSTR StrDupUnicode(const WCHAR *pwsz)
{
    if (*pwsz)
    {
        USES_CONVERSION;
        return StrDup(W2CT(pwsz));
    }
    return NULL;
}

STDMETHODIMP CFSFolder_PF_InitializeEx(IPersistFolder3 *ppf, IBindCtx *pbc, 
                                       LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *pfti)
{
    CFSFolder *this = IToClass(CFSFolder, pf, ppf);
    HRESULT hres = CFSFolder_PF_Initialize(ppf, pidlRoot);
    if (SUCCEEDED(hres))
    {
        if (pfti)
        {
            if ( !pfti->pidlTargetFolder &&
                    !pfti->szTargetParsingName[0] &&
                        (pfti->csidl == -1) )
            {
                return E_INVALIDARG;
            }

            this->_dwAttributes = pfti->dwAttributes;

            if (pfti->csidl != -1 && (pfti->csidl & CSIDL_FLAG_PFTI_TRACKTARGET))
            {
                //
                //  For tracking target, all other fields must be null.
                //
                if (pfti->pidlTargetFolder ||
                    pfti->szTargetParsingName[0] ||
                    pfti->szNetworkProvider[0])
                    return E_INVALIDARG;

                this->_csidlTrack = pfti->csidl & (~CSIDL_FLAG_MASK | CSIDL_FLAG_CREATE);

            }
            else
            {
                this->_pidlTarget = ILClone(pfti->pidlTargetFolder);
                this->_pszPath = StrDupUnicode(pfti->szTargetParsingName);
                this->_pszNetProvider = StrDupUnicode(pfti->szNetworkProvider);
                if (pfti->csidl != -1)
                    this->_csidl = pfti->csidl & (~CSIDL_FLAG_MASK | CSIDL_FLAG_CREATE);

            }
        }
    }
    return hres;
}

STDMETHODIMP CFSFolder_PF_GetFolderInfo(IPersistFolder3 *ppf, PERSIST_FOLDER_TARGET_INFO *pfti)
{
    HRESULT hres = S_OK;
    CFSFolder *this = IToClass(CFSFolder, pf, ppf);

    ZeroMemory(pfti, sizeof(*pfti)); 

    CFSFolder_GetPathForItemW(this, NULL, pfti->szTargetParsingName);
    if (this->_pidlTarget)
        hres = SHILClone(this->_pidlTarget, &pfti->pidlTargetFolder);
    if (this->_pszNetProvider)
        SHTCharToUnicode(this->_pszNetProvider, pfti->szNetworkProvider, ARRAYSIZE(pfti->szNetworkProvider));

    pfti->dwAttributes = this->_dwAttributes;
    if (this->_csidlTrack >= 0)
        pfti->csidl = this->_csidlTrack | CSIDL_FLAG_PFTI_TRACKTARGET;
    else
        pfti->csidl = CFSFolder_GetCSIDL(this);

    return hres;
}


IPersistFolder3Vtbl c_CFSFolderPFVtbl =
{
    CFSFolder_PF_QueryInterface, CFSFolder_PF_AddRef, CFSFolder_PF_Release,
    CFSFolder_PF_GetClassID,
    CFSFolder_PF_Initialize,
    CFSFolder_PF_GetCurFolder,
    CFSFolder_PF_InitializeEx,
    CFSFolder_PF_GetFolderInfo,
};

//===========================================================================
// CFSFolder : Constructor
//===========================================================================

STDAPI CFSFolder_CreateFolder(IUnknown *punkOuter, LPCITEMIDLIST pidl, 
                              const PERSIST_FOLDER_TARGET_INFO *pfti, REFIID riid, void **ppv)
{
    HRESULT hres = E_OUTOFMEMORY;
    CFSFolder *this;

    *ppv = NULL;

    this = LocalAlloc(LPTR, SIZEOF(CFSFolder));
    if (this)
    {
        this->iunk.lpVtbl = &c_FSFolderUnkVtbl;
        this->sf.lpVtbl = &c_FSFolderVtbl;
        this->si.lpVtbl = &c_FSFolderIconVtbl;
        this->sio.lpVtbl = & c_FSFolderIconOverlayVtbl;
        this->pf.lpVtbl = &c_CFSFolderPFVtbl;
        this->punkOuter = punkOuter ? punkOuter : &this->iunk;
        this->cRef = 1;
        this->_csidl = -1;
        this->_iFolderIcon = -1;
        this->_dwAttributes = -1;
        this->_csidlTrack = -1;

        hres = CFSFolder_PF_InitializeEx(&this->pf, NULL, pidl, pfti);
        if (SUCCEEDED(hres))
            hres = this->iunk.lpVtbl->QueryInterface(&this->iunk, riid, ppv);
        this->iunk.lpVtbl->Release(&this->iunk);
    }

    return hres;
}

//
// COM object creation entry point for CLSID_ShellFSFolder
//
STDAPI CFSFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return CFSFolder_CreateFolder(punkOuter, &c_idlDesktop, NULL, riid, ppv);
}

HRESULT GetToolTipForItem(CFSFolder *this, LPCIDFOLDER pidf, REFIID riid, void **ppv)
{
    IQueryAssociations *pqa;
    HRESULT hr = FS_AssocCreate(pidf, &IID_IQueryAssociations, (void **)&pqa);
    if (SUCCEEDED(hr))
    {
        WCHAR szText[INFOTIPSIZE];

        hr = pqa->lpVtbl->GetString(pqa, 0, ASSOCSTR_INFOTIP, NULL,
                szText, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szText)));
        if (SUCCEEDED(hr))
            hr = CreateInfoTipFromItem(&this->sf, (LPCITEMIDLIST)pidf, szText, riid, ppv);
        pqa->lpVtbl->Release(pqa);
    }

    return hr;
}

#ifndef WINNT
// HACK so that we work with the OSR2 SHELL.DLL thunk scripts. We don't want to
// change SHELL.DLL when we install on OSR2 so that we can do no reboot.
// This is only called by the 16 bit thunk layer.
#undef SHGetFileInfo
STDAPI_(DWORD) SHGetFileInfo(LPCTSTR pszPath, DWORD dwFileAttributes, void *psfi, UINT cbFileInfo, UINT uFlags)
{
  return SHGetFileInfoA(pszPath, dwFileAttributes, psfi, cbFileInfo, uFlags);
}
#endif

// global hook in the SHChangeNotify() dispatcher. note we get all change notifies
// here so be careful!
//
// this is also called by defviewx.c (with SHCNE_UPDATEITEM)
// to remove a PIDL from the icon cache.

void Icon_FSEvent(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra)
{
    LPCIDFOLDER pidf;

    switch (lEvent)
    {
    case SHCNE_ASSOCCHANGED:
    {
        HWND hwnd = GetDesktopWindow();
        FlushFileClass();   // flush them all
        SHSetValue(HKEY_CURRENT_USER, STRREG_DISCARDABLE STRREG_POSTSETUP, TEXT("OpenAsList"), REG_SZ, "", 0);        
        if (IsWindow(hwnd) )
            PostMessage(hwnd, DTM_SETUPAPPRAN, 0, 0);
    }
        break;

    case SHCNE_UPDATEITEM:
        pidf = FS_IsValidID(ILFindLastID(pidl));
        if (pidf)
        {
            TCHAR szName[MAX_PATH];
            FS_CopyName(pidf, szName, ARRAYSIZE(szName));
            TraceMsg(TF_IMAGE, "IconCache: flush %s", szName);
            
            RemoveFromIconTable(szName);
        }
        break;
    }
}
