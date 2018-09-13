//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#include "grpconv.h"
#include "util.h"
#include "rcids.h"
#include "group.h"
#include "gcinst.h"
#include <port32.h>
#include <regstr.h>
#define INITGUID
#include <initguid.h>
#pragma data_seg(DATASEG_READONLY)
#include <coguid.h>
#include <oleguid.h>
#pragma data_seg()

#ifdef DEBUG
extern UINT GC_TRACE;
#endif

//---------------------------------------------------------------------------
// Exported.
const TCHAR c_szMapGroups[] = TEXT("MapGroups");
#ifndef WINNT
const TCHAR c_szDelGroups[] = TEXT("DelGroups");
#endif

//---------------------------------------------------------------------------
// Global to this file only;
static const TCHAR c_szGrpConv[] = TEXT("Grpconv");
static const TCHAR c_szLastModDateTime[] = TEXT("LastModDateTime");
static const TCHAR c_szRegistry[] = TEXT("Registry");
static const TCHAR c_szDefaultUser[] = TEXT("DefaultUser");
static const TCHAR c_szGrpConvData[] = TEXT("compat.csv");
static const TCHAR c_szProgmanStartup[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager\\Settings\\Startup");
static const TCHAR c_szDotPif[] = TEXT(".pif");

// New group stuff
HDSA hdsaPMItems;       // current group
HDSA g_hdsaAppList;

HKEY hkeyGroups = NULL;
BOOL g_fDoingCommonGroups = FALSE;


#pragma pack(1)

typedef struct tagRECTS
{
    short left;
    short top;
    short right;
    short bottom;
} RECTS;

typedef struct
    {
    LPTSTR lpszDesc;
    LPTSTR lpszCL;
    LPTSTR lpszWD;
    LPTSTR lpszIconPath;
    WORD wiIcon;
    WORD wHotKey;
    int nShowCmd;
#ifdef WINNT
    BOOL bSepVdm;
#endif
    }   PMITEM, *PPMITEM, *LPPMITEM;

// Old Progman stuff.
#define GROUP_MAGIC    0x43434D50L  // 'PMCC'
#define GROUP_UNICODE  0x43554D50L  // 'PMUC'

/*
 * Win 3.1 .GRP file formats (ITEMDEF for items, GROUPDEF for groups)
 */
typedef struct
    {
    POINTS        pt;
    WORD          iIcon;
    WORD          cbHeader;
    WORD          cbANDPlane;
    WORD          cbXORPlane;
    WORD          pHeader;
    WORD          pANDPlane;
    WORD          pXORPlane;
    WORD          pName;
    WORD          pCommand;
    WORD          pIconPath;
    } ITEMDEF, *PITEMDEF, *LPITEMDEF;

typedef struct
    {
    DWORD     dwMagic;
    WORD      wCheckSum;
    WORD      cbGroup;
    WORD      nCmdShow;
    RECTS     rcNormal;
    POINTS    ptMin;
    WORD      pName;
    WORD      cxIcon;
    WORD      cyIcon;
    WORD      wIconFormat;
    WORD      wReserved;
    WORD      cItems;
    } GROUPDEF, *PGROUPDEF, *LPGROUPDEF;

typedef struct
    {
    WORD wID;
    WORD wItem;
    WORD cb;
    } PMTAG, *PPMTAG, *LPPMTAG;

// Thank God the tag stuff never really caught on.
#define TAG_MAGIC GROUP_MAGIC
#define ID_MAINTAIN                 0x8000
#define ID_MAGIC                    0x8000
#define ID_WRITERVERSION        0x8001
#define ID_APPLICATIONDIR       0x8101
#define ID_HOTKEY                   0x8102
#define ID_MINIMIZE                 0x8103
#ifdef WINNT
#define ID_NEWVDM                   0x8104
#endif
#define ID_LASTTAG                  0xFFFF

/*
 * NT 3.1 Ansi .GRP File format structures
 */
typedef struct tagGROUPDEF_A {
    DWORD   dwMagic;        /* magical bytes 'PMCC' */
    WORD    wCheckSum;      /* adjust this for zero sum of file */
    WORD    cbGroup;        /* length of group segment */
    RECT    rcNormal;       /* rectangle of normal window */
    POINT   ptMin;          /* point of icon */
    WORD    nCmdShow;       /* min, max, or normal state */
    WORD    pName;          /* name of group */
                            /* these four change interpretation */
    WORD    cxIcon;         /* width of icons */
    WORD    cyIcon;         /* hieght of icons */
    WORD    wIconFormat;    /* planes and BPP in icons */
    WORD    wReserved;      /* This word is no longer used. */

    WORD    cItems;         /* number of items in group */
    WORD    rgiItems[1];    /* array of ITEMDEF offsets */
} NT_GROUPDEF_A, *PNT_GROUPDEF_A;
typedef NT_GROUPDEF_A *LPNT_GROUPDEF_A;

typedef struct tagITEMDEF_A {
    POINT   pt;             /* location of item icon in group */
    WORD    idIcon;         /* id of item icon */
    WORD    wIconVer;       /* icon version */
    WORD    cbIconRes;      /* size of icon resource */
    WORD    indexIcon;      /* index of item icon */
    WORD    dummy2;         /* - not used anymore */
    WORD    pIconRes;       /* offset of icon resource */
    WORD    dummy3;         /* - not used anymore */
    WORD    pName;          /* offset of name string */
    WORD    pCommand;       /* offset of command string */
    WORD    pIconPath;      /* offset of icon path */
} NT_ITEMDEF_A, *PNT_ITEMDEF_A;
typedef NT_ITEMDEF_A *LPNT_ITEMDEF_A;

/*
 * NT 3.1a Unicode .GRP File format structures
 */
typedef struct tagGROUPDEF {
    DWORD   dwMagic;        /* magical bytes 'PMCC' */
    DWORD   cbGroup;        /* length of group segment */
    RECT    rcNormal;       /* rectangle of normal window */
    POINT   ptMin;          /* point of icon */
    WORD    wCheckSum;      /* adjust this for zero sum of file */
    WORD    nCmdShow;       /* min, max, or normal state */
    DWORD   pName;          /* name of group */
                            /* these four change interpretation */
    WORD    cxIcon;         /* width of icons */
    WORD    cyIcon;         /* hieght of icons */
    WORD    wIconFormat;    /* planes and BPP in icons */
    WORD    wReserved;      /* This word is no longer used. */

    WORD    cItems;         /* number of items in group */
    WORD    Reserved1;
    DWORD   Reserved2;
    DWORD   rgiItems[1];    /* array of ITEMDEF offsets */
} NT_GROUPDEF, *PNT_GROUPDEF;
typedef NT_GROUPDEF *LPNT_GROUPDEF;

typedef struct tagITEMDEF {
    POINT   pt;             /* location of item icon in group */
    WORD    iIcon;          /* id of item icon */
    WORD    wIconVer;       /* icon version */
    WORD    cbIconRes;      /* size of icon resource */
    WORD    wIconIndex;     /* index of the item icon (not the same as the id) */
    DWORD   pIconRes;       /* offset of icon resource */
    DWORD   pName;          /* offset of name string */
    DWORD   pCommand;       /* offset of command string */
    DWORD   pIconPath;      /* offset of icon path */
} NT_ITEMDEF, *PNT_ITEMDEF;
typedef NT_ITEMDEF *LPNT_ITEMDEF;

typedef struct _tag
  {
    WORD wID;                   // tag identifier
    WORD dummy1;                // need this for alignment!
    int wItem;                  // (unde the covers 32 bit point!)item the tag belongs to
    WORD cb;                    // size of record, including id and count
    WORD dummy2;                // need this for alignment!
    BYTE rgb[1];
  } NT_PMTAG, * LPNT_PMTAG;

/* the pointers in the above structures are short pointers relative to the
 * beginning of the segments.  This macro converts the short pointer into
 * a long pointer including the proper segment/selector value.        It assumes
 * that its argument is an lvalue somewhere in a group segment, for example,
 * PTR(lpgd->pName) returns a pointer to the group name, but k=lpgd->pName;
 * PTR(k) is obviously wrong as it will use either SS or DS for its segment,
 * depending on the storage class of k.
 */
#define PTR(base, offset) (LPBYTE)((PBYTE)base + offset)

/* PTR2 is used for those cases where a variable already contains an offset
 * (The "case that doesn't work", above)
 */
#define PTR2(lp,offset) ((LPBYTE)MAKELONG(offset,HIWORD(lp)))

/* this macro is used to retrieve the i-th item in the group segment.  Note
 * that this pointer will NOT be NULL for an unused slot.
 */
#define ITEM(lpgd,i) ((LPNT_ITEMDEF)PTR(lpgd, lpgd->rgiItems[i]))

/* Keeping things starting on aligned boundaries allows faster access on
 * most platforms.
 */
#define MyDwordAlign(size)  (((size) + 3) & ~3)

#pragma pack()


#define CFree(a)    if(a) Free(a)

//---------------------------------------------------------------------------
#define Stream_Write(ps, pv, cb)    SUCCEEDED((ps)->lpVtbl->Write(ps, pv, cb, NULL))
#define Stream_Close(ps)            (void)(ps)->lpVtbl->Release(ps)

#define VOF_BAD     0
#define VOF_WIN31   1
#define VOF_WINNT   2

int ConvertToUnicodeGroup(LPNT_GROUPDEF_A lpGroupORI, LPHANDLE lphNewGroup);


//---------------------------------------------------------------------------
// Init the group stuff
BOOL ItemList_Create(LPCTSTR lpszGroup)
{
    if (!hdsaPMItems)
            hdsaPMItems = DSA_Create(SIZEOF(PMITEM), 16);

        if (hdsaPMItems)
                return TRUE;

        DebugMsg(DM_ERROR, TEXT("cg.gi: Unable to init."));
        return FALSE;
}

//---------------------------------------------------------------------------
// Tidyup.
void ItemList_Destroy(void)
{
        int i;
        int cItems;
        LPPMITEM lppmitem;

        // Clean up the items.
        cItems = DSA_GetItemCount(hdsaPMItems);
        for(i=0; i < cItems; i++)
        {
                lppmitem = DSA_GetItemPtr(hdsaPMItems, 0);
                // Nuke the strings.
                CFree(lppmitem->lpszDesc);
                CFree(lppmitem->lpszCL);
                CFree(lppmitem->lpszWD);
                CFree(lppmitem->lpszIconPath);
                // Nuke the structure.
                DSA_DeleteItem(hdsaPMItems, 0);
        }
        DSA_Destroy(hdsaPMItems);
        hdsaPMItems = NULL;
}

//---------------------------------------------------------------------------
// Returns TRUE if the file smells like an old PM group, the title of the
// group is returned in lpszTitle which must be at least 32 chars big.
// REVIEW - Is it worth checking the checksum?
UINT Group_ValidOldFormat(LPCTSTR lpszOldGroup, LPTSTR lpszTitle)
    {
#ifdef UNICODE
    HANDLE fh;
    DWORD  dwBytesRead;
#else
    HFILE  fh;
#endif
    UINT nCode;
    GROUPDEF grpdef;

    // Find and open the group file.
#ifdef UNICODE
    fh = CreateFile(
             lpszOldGroup,
             GENERIC_READ,
             FILE_SHARE_READ,
             NULL,
             OPEN_EXISTING,
             0,
             NULL
             );
    if (fh != INVALID_HANDLE_VALUE)
#else
    fh = _lopen(lpszOldGroup, OF_READ | OF_SHARE_DENY_NONE);
    if (fh != HFILE_ERROR)
#endif
        {
        // Get the definition.
#ifdef UNICODE
        ReadFile(fh, &grpdef, SIZEOF(grpdef), &dwBytesRead, NULL);
#else
        _lread(fh, &grpdef, SIZEOF(grpdef));
#endif

        // Does it have the right magic bytes?.
        switch( grpdef.dwMagic )
            {
            case GROUP_UNICODE:
                {
                    NT_GROUPDEF nt_grpdef;

#ifdef UNICODE
                    SetFilePointer(fh, 0, NULL, FILE_BEGIN);
                    ReadFile(fh, &nt_grpdef, SIZEOF(nt_grpdef), &dwBytesRead, NULL);
#else
                    _llseek(fh, 0, 0);      // Back to the start
                    _lread(fh, &nt_grpdef, SIZEOF(nt_grpdef));
#endif

                    // Yep, Get it's size..
                    // Is it at least as big as the header says it is?
#ifdef UNICODE
                    if ( nt_grpdef.cbGroup <= (DWORD)SetFilePointer(fh, 0L, NULL,  FILE_END))
#else
                    if ( nt_grpdef.cbGroup <= (DWORD)_llseek(fh, 0L, 2))
#endif
                    {
                        WCHAR wchGroupName[MAXGROUPNAMELEN+1];

                        // Yep, probably valid.
                        // Get its title.
#ifdef UNICODE
                        SetFilePointer(fh, nt_grpdef.pName, 0, FILE_BEGIN);
                        ReadFile(fh, wchGroupName, SIZEOF(wchGroupName), &dwBytesRead, NULL);
                        lstrcpy(lpszTitle, wchGroupName);
#else
                        _llseek(fh, nt_grpdef.pName, 0);
                        _lread(fh,wchGroupName, SIZEOF(wchGroupName));
                        WideCharToMultiByte (CP_ACP, 0, wchGroupName, -1,
                                         lpszTitle, MAXGROUPNAMELEN+1, NULL, NULL);
#endif
                        nCode = VOF_WINNT;
                    }
                    else
                    {
                        // No. Too small.
                        DebugMsg(DM_TRACE, TEXT("gc.gvof: File has invalid size."));
                        nCode = VOF_BAD;
                    }
                }
                break;
            case GROUP_MAGIC:
                {
                CHAR chGroupName[MAXGROUPNAMELEN+1];
                // Yep, Get it's size..
                // Is it at least as big as the header says it is?
#ifdef UNICODE
                if (grpdef.cbGroup <= (WORD) SetFilePointer(fh, 0L, NULL, FILE_END))
#else
                if (grpdef.cbGroup <= (WORD) _llseek(fh, 0L, 2))
#endif
                    {
                    // Check to make sure there is a name embedded in the
                    // .grp file.  If not, just use the filename
                    if (grpdef.pName==0)
                        {
                        LPTSTR lpszFile, lpszExt, lpszDest = lpszTitle;

                        lpszFile = PathFindFileName( lpszOldGroup );
                        lpszExt  = PathFindExtension( lpszOldGroup );
                        for( ;
                             lpszFile && lpszExt && (lpszFile != lpszExt);
                             *lpszDest++ = *lpszFile++
                            );
                        *lpszDest = TEXT('\0');

                        }
                    else
                        {

                        // Yep, probably valid.
                        // Get it's title.
#ifdef UNICODE
                        SetFilePointer(fh, grpdef.pName, NULL, FILE_BEGIN);
                        ReadFile(fh, chGroupName, MAXGROUPNAMELEN+1, &dwBytesRead, NULL);
                        MultiByteToWideChar(
                            CP_ACP,
                            MB_PRECOMPOSED,
                            chGroupName,
                            -1,
                            lpszTitle,
                            MAXGROUPNAMELEN+1
                            ) ;
#else
                        _llseek(fh, grpdef.pName, 0);
                        _lread(fh, lpszTitle, MAXGROUPNAMELEN+1);
#endif
                        }

                    nCode = VOF_WIN31;
                    }
                else
                    {
                    // No. Too small.
                    DebugMsg(DM_TRACE, TEXT("gc.gvof: File has invalid size."));
                    nCode = VOF_BAD;
                    }
                break;
                }

            default:
                // No, the magic bytes are wrong.
                DebugMsg(DM_TRACE, TEXT("gc.gvof: File has invalid magic bytes."));
                nCode = VOF_BAD;
                break;
            }
#ifdef UNICODE
        CloseHandle(fh);
#else
        _lclose(fh);
#endif
        }
    else
        {
        // No. Can't even read the file.
        DebugMsg(DM_TRACE, TEXT("gc.gvof: File is unreadble."));
        nCode = VOF_BAD;
        }

    return nCode;
    }

//---------------------------------------------------------------------------
// BUGBUG:: there is a similar function in shelldll\path.c and in cabinet\dde.c
BOOL _IsValidFileNameChar(TBYTE ch, UINT flags)
{
    switch (ch) {
    case TEXT('\\'):      // path separator
        return flags & PRICF_ALLOWSLASH;
    case TEXT(';'):       // terminator
    case TEXT(','):       // terminator
    case TEXT('|'):       // pipe
    case TEXT('>'):       // redir
    case TEXT('<'):       // redir
    case TEXT('"'):       // quote
    case TEXT('?'):       // wc           we only do wilds here because they're
    case TEXT('*'):       // wc           legal for qualifypath
    case TEXT(':'):       // drive colon
    case TEXT('/'):       // path sep
        return FALSE;
    }

    // Can not be a control char...
    return ch >= TEXT(' ');
}


void PathRemoveIllegalChars(LPTSTR pszPath, int iGroupName, UINT flags)
{
    LPTSTR pszT = pszPath + iGroupName;

    // Map all of the strange characters out of the name for both LFn and not
    // machines
    while (*pszT)
    {
        if (!_IsValidFileNameChar(*pszT, flags))
            *pszT = TEXT('_');        // Don't Allow invalid chars in names
        pszT = CharNext(pszT);
    }
}

//---------------------------------------------------------------------------
// We want certain groups to end up in a new location eg Games is now
// Applications\Games.
void MapGroupTitle(LPCTSTR lpszOld, LPTSTR lpszNew, UINT cchNew)
{
    // Is there a mapping?
    if (!Reg_GetString(g_hkeyGrpConv, c_szMapGroups, lpszOld, lpszNew, cchNew*sizeof(TCHAR)))
    {
        // Nope, just use the given name.
        lstrcpyn(lpszNew, lpszOld, cchNew);
    }
    DebugMsg(DM_TRACE, TEXT("gc.mgr: From %s to %s"), lpszOld, lpszNew);
}

#ifndef WINNT
BOOL Group_DeleteIfRequired(LPCTSTR lpszOldGrpTitle, LPCTSTR lpszOldGrpFile)
{
    BOOL  fRet;
    HKEY  hkeyNew;
    TCHAR szIniFile[MAX_PATH], szFile[MAX_PATH];

    if (Reg_GetString(g_hkeyGrpConv, c_szDelGroups, lpszOldGrpTitle, NULL, 0))
    {
        Win32DeleteFile(lpszOldGrpFile);

        DebugMsg(DM_TRACE, TEXT("gc.mgr: old group %s has been deleted"), lpszOldGrpTitle);

        // Remove old Group entry from registry..
        //
        if (RegOpenKey(g_hkeyGrpConv, c_szGroups, &hkeyNew) == ERROR_SUCCESS)
        {
            if (RegDeleteValue(hkeyNew, lpszOldGrpFile) == ERROR_SUCCESS)
            {
                fRet = TRUE;
            }
            RegCloseKey(hkeyNew);
        }

        // Remove old Group entry from progman.ini
        if (FindProgmanIni(szIniFile))
        {
            UINT uSize;
            LPTSTR pSection, pKey;

            for (uSize = 1024; uSize < 1024 * 8; uSize += 1024)
            {
                pSection = (PSTR)LocalAlloc(LPTR, uSize);
                if (!pSection)
                    break;

                if ((UINT)GetPrivateProfileString(c_szGroups, NULL, c_szNULL, pSection, uSize, szIniFile) < uSize - 5)
                    break;

                LocalFree((HLOCAL)pSection);
                pSection = NULL;
                fRet = FALSE;
            }

            if (pSection)
            {
                for (pKey = pSection; *pKey; pKey += lstrlen(pKey) + 1)
                {
                    GetPrivateProfileString(c_szGroups, pKey, c_szNULL, szFile, ARRAYSIZE(szFile), szIniFile);

                    if (lstrcmpi(lpszOldGrpFile, szFile) == 0)
                    {
                        WritePrivateProfileString(c_szGroups, pKey, NULL, szIniFile);
                        break;
                    }
                }
                LocalFree((HLOCAL)pSection);
                pSection = NULL;
            }
        }
    }
    return fRet;
}
#endif // !WINNT

#undef PathRemoveExtension
//---------------------------------------------------------------------------
void PathRemoveExtension(LPTSTR pszPath)
{
    LPTSTR pExt = PathFindExtension(pszPath);
    if (*pExt)
    {
        Assert(*pExt == TEXT('.'));
        *pExt = 0;    // null out the "."
    }
}

//---------------------------------------------------------------------------
// Given a path to an old group, create and return a path to where the new
// group will be.
BOOL Group_GenerateNewGroupPath(HWND hwnd, LPCTSTR lpszOldGrpTitle,
    LPTSTR lpszNewGrpPath, LPCTSTR pszOldGrpPath)
{
    int iLen;
    TCHAR szGrpTitle[MAX_PATH];
    TCHAR szOldGrpTitle[32];


    // Get the location for all the special shell folders.
    if (g_fDoingCommonGroups)
        SHGetSpecialFolderPath(hwnd, lpszNewGrpPath, CSIDL_COMMON_PROGRAMS, TRUE);
    else
        SHGetSpecialFolderPath(hwnd, lpszNewGrpPath, CSIDL_PROGRAMS, TRUE);


    if (IsLFNDrive(lpszNewGrpPath))
    {
        // Fix it a bit.
        lstrcpyn(szOldGrpTitle, lpszOldGrpTitle, ARRAYSIZE(szOldGrpTitle));
        PathRemoveIllegalChars(szOldGrpTitle, 0, PRICF_NORMAL);
        // Munge the names so that things move to the new locations.
        MapGroupTitle(szOldGrpTitle, szGrpTitle, ARRAYSIZE(szGrpTitle));
        // Stick on the new group name.
        PathAddBackslash(lpszNewGrpPath);
        iLen = lstrlen(lpszNewGrpPath);
        // NB Don't use PathAppend() - very bad if there's a colons in the title.
        lstrcpyn(lpszNewGrpPath+iLen, szGrpTitle, MAX_PATH-iLen);
        PathRemoveIllegalChars(lpszNewGrpPath, iLen, PRICF_ALLOWSLASH);
    }
    else
    {
        // Just use the old group file name - this will make sure the group
        // names remain unique.
        PathAppend(lpszNewGrpPath, PathFindFileName(pszOldGrpPath));
        PathRemoveExtension(lpszNewGrpPath);
    }

    if (!PathFileExists(lpszNewGrpPath))
    {
        // Folder doesn't exist.
        // return Win32CreateDirectory(lpszNewGrpPath, NULL);
        return (SHCreateDirectory(hwnd, lpszNewGrpPath) == 0);
    }

    // Folder already exists.
    return TRUE;
}

//---------------------------------------------------------------------------
// Returns true if the offsets given in the item def are valid-ish.
BOOL CheckItemDef(LPITEMDEF lpitemdef, WORD cbGroup)
    {
    if (lpitemdef->pHeader < cbGroup && lpitemdef->pANDPlane < cbGroup &&
        lpitemdef->pXORPlane < cbGroup && lpitemdef->pName < cbGroup &&
        lpitemdef->pCommand < cbGroup && lpitemdef->pIconPath < cbGroup &&
        lpitemdef->pHeader && lpitemdef->pXORPlane && lpitemdef->pCommand)
        return TRUE;
    else
        {
        return FALSE;
        }
    }

//---------------------------------------------------------------------------
// Returns true if the offsets given in the item def are valid-ish.
BOOL CheckItemDefNT(LPNT_ITEMDEF lpitemdef, DWORD cbGroup)
    {
    if (lpitemdef->pName < cbGroup &&
        lpitemdef->pCommand < cbGroup &&
        lpitemdef->pIconPath < cbGroup &&
        lpitemdef->pCommand)
        return TRUE;
    else
        {
        return FALSE;
        }
    }

//---------------------------------------------------------------------------
// Read the tags info from the given file handle from the given offset.
#ifdef UNICODE
void HandleTags(HANDLE fh, WORD oTags)
#else
void HandleTags(int fh, WORD oTags)
#endif
{
    LONG cbGroupReal;
    PMTAG pmtag;
    BOOL fTags = TRUE;
    TCHAR szText[MAX_PATH];
    BOOL fFirstTag = FALSE;
    LPPMITEM lppmitem;
    WORD wHotKey;
#ifdef UNICODE
    DWORD      dwBytesRead;
#endif

    DebugMsg(DM_TRACE, TEXT("cg.ht: Reading tags."));
#ifdef UNICODE
    cbGroupReal = SetFilePointer(fh, 0, NULL, FILE_END);
#else
    cbGroupReal = (WORD) _llseek(fh, 0L, 2);
#endif
    if (cbGroupReal <= (LONG) oTags)
    {
        // No tags in this file.
        return;
    }

    // Get to the tags section.
#ifdef UNICODE
    SetFilePointer(fh, oTags, NULL, FILE_BEGIN);
#else
    _llseek(fh, oTags, 0);
#endif
    while (fTags)
    {
#ifdef UNICODE
        if (!ReadFile(fh, &pmtag, SIZEOF(pmtag), &dwBytesRead, NULL) || dwBytesRead == 0) {
            fTags = FALSE;
            break;
        }
#else
        fTags = _lread(fh, &pmtag, SIZEOF(pmtag));
#endif
        switch (pmtag.wID)
        {
            case ID_MAGIC:
            {
//                DebugMsg(DM_TRACE, "gc.ht: First tag found.");
                fFirstTag = TRUE;
#ifdef UNICODE
                SetFilePointer(fh, pmtag.cb - SIZEOF(PMTAG), NULL, FILE_CURRENT);
#else
                _llseek(fh, pmtag.cb - SIZEOF(PMTAG), 1);
#endif
                break;
            }
            case ID_LASTTAG:
            {
//                DebugMsg(DM_TRACE, "gc.ht: Last tag found.");
                fTags = FALSE;
                break;
            }
            case ID_APPLICATIONDIR:
            {
//                DebugMsg(DM_TRACE, "gc.ht: App dir %s found for %d.", (LPSTR) szText, pmtag.wItem);
                fgets(szText, SIZEOF(szText), fh);
                lppmitem = DSA_GetItemPtr(hdsaPMItems, pmtag.wItem);
                if (lppmitem)
                {
                    Str_SetPtr(&lppmitem->lpszCL, szText);
                }
#ifdef DEBUG
                else
                {
                    DebugMsg(DM_ERROR, TEXT("gc.ht: Item is invalid."));
                }
#endif
                break;
            }
            case ID_HOTKEY:
            {
                // DebugMsg(DM_TRACE, "gc.ht: Hotkey found for %d.", pmtag.wItem);
#ifdef UNICODE
                ReadFile(fh, &wHotKey, SIZEOF(wHotKey), &dwBytesRead, NULL);
#else
                _lread(fh, &wHotKey, SIZEOF(wHotKey));
#endif
                lppmitem = DSA_GetItemPtr(hdsaPMItems, pmtag.wItem);
                if (lppmitem)
                {
                    lppmitem->wHotKey = wHotKey;
                }
#ifdef DEBUG
                else
                {
                    DebugMsg(DM_ERROR, TEXT("gc.ht: Item is invalid."));
                }
#endif
                break;
            }
            case ID_MINIMIZE:
            {
                // DebugMsg(DM_TRACE, "gc.ht: Minimise flag found for %d.", pmtag.wItem);
                lppmitem = DSA_GetItemPtr(hdsaPMItems, pmtag.wItem);
                if (lppmitem)
                {
                    lppmitem->nShowCmd = SW_SHOWMINNOACTIVE;
                }
#ifdef DEBUG
                else
                {
                    DebugMsg(DM_ERROR, TEXT("gc.ht: Item is invalid."));
                }
#endif
                // Skip to the next tag.
#ifdef UNICODE
                SetFilePointer(fh, pmtag.cb - SIZEOF(PMTAG), NULL, FILE_CURRENT);
#else
                _llseek(fh, pmtag.cb - SIZEOF(PMTAG), 1);
#endif
                break;
            }
#ifdef WINNT
            case ID_NEWVDM:
            {
                // DebugMsg(DM_TRACE, "gc.ht: Separate VDM flag found for %d.", pmtag.wItem );
                lppmitem = DSA_GetItemPtr(hdsaPMItems, pmtag.wItem);
                if (lppmitem)
                {
                    lppmitem->bSepVdm = TRUE;
                }
#ifdef DEBUG
                else
                {
                    DebugMsg(DM_ERROR, TEXT("gc.ht: Item is invalid."));
                }
#endif
                // Skip to the next tag.
#ifdef UNICODE
                SetFilePointer(fh, pmtag.cb - SIZEOF(PMTAG), NULL, FILE_CURRENT);
#else
                _llseek(fh, pmtag.cb - SIZEOF(PMTAG), 1);
#endif
                break;
            }
#endif
            default:
            {
                // We've found something we don't understand but we haven't
                // found the first tag yet - probably a bust file.
                if (!fFirstTag)
                {
                    DebugMsg(DM_TRACE, TEXT("gc.ht: No initial tag found - tags section is corrupt."));
                    fTags = FALSE;
                }
                else
                {
                    // Some unknown tag.
                    if (pmtag.cb < SIZEOF(PMTAG))
                    {
                        // Can't continue!
                        DebugMsg(DM_TRACE, TEXT("gc.ht: Tag has invalid size - ignoring remaining tags."));
                        fTags = FALSE;
                    }
                    else
                    {
                        // Just ignore its data and continue.
#ifdef UNICODE
                        SetFilePointer(fh, pmtag.cb - SIZEOF(PMTAG), NULL, FILE_CURRENT);
#else
                        _llseek(fh, pmtag.cb - SIZEOF(PMTAG), 1);
#endif
                    }
                }
                break;
            }
        }
    }
}

//---------------------------------------------------------------------------
// Read the tags info from the given file handle from the given offset.
#ifdef UNICODE
void HandleTagsNT(HANDLE fh, DWORD oTags)
#else
void HandleTagsNT(int fh, DWORD oTags)
#endif
{
    DWORD cbGroupReal;
    DWORD dwPosition;
    NT_PMTAG pmtag;
    BOOL fTags = TRUE;
    WCHAR wszTemp[MAX_PATH];
    TCHAR szText[MAX_PATH];
    BOOL fFirstTag = FALSE;
    LPPMITEM lppmitem;
    WORD wHotKey;
#ifdef UNICODE
    DWORD dwBytesRead;
#endif

    DebugMsg(DM_TRACE, TEXT("cg.ht: Reading tags."));
#ifdef UNICODE
    cbGroupReal = SetFilePointer(fh, 0, NULL, FILE_END);
#else
    cbGroupReal = _llseek(fh, 0L, 2);
#endif
    if (cbGroupReal <= oTags)
    {
        // No tags in this file.
        return;
    }

    // Get to the tags section.
    dwPosition = oTags;
    while (fTags)
    {
#ifdef UNICODE
        SetFilePointer(fh, dwPosition, NULL, FILE_BEGIN);
        if (!ReadFile(fh, &pmtag, SIZEOF(pmtag), &dwBytesRead, NULL) || dwBytesRead == 0) {
            fTags = FALSE;
            break;
        }

#else
        _llseek(fh,dwPosition,0);
        fTags = _lread(fh, &pmtag, SIZEOF(pmtag));
#endif
        switch (pmtag.wID)
        {
            case ID_MAGIC:
            {
//                DebugMsg(DM_TRACE, "gc.ht: First tag found.");
                fFirstTag = TRUE;
                dwPosition += pmtag.cb;
                break;
            }
            case ID_LASTTAG:
            {
//                DebugMsg(DM_TRACE, "gc.ht: Last tag found.");
                fTags = FALSE;
                break;
            }
            case ID_APPLICATIONDIR:
            {
#ifdef UNICODE
                SetFilePointer(fh, dwPosition+FIELD_OFFSET(NT_PMTAG,rgb[0]), NULL, FILE_BEGIN);
                ReadFile(fh, wszTemp, SIZEOF(wszTemp), &dwBytesRead, NULL);
                lstrcpy(szText, wszTemp);
#else
                _llseek(fh,dwPosition+FIELD_OFFSET(NT_PMTAG,rgb[0]),0);
                _lread(fh,wszTemp,SIZEOF(wszTemp));
                WideCharToMultiByte (CP_ACP, 0, wszTemp, -1,
                                 szText, ARRAYSIZE(szText), NULL, NULL);
#endif
//                DebugMsg(DM_TRACE, "gc.ht: App dir %s found for %d.", (LPSTR) szText, pmtag.wItem);
                lppmitem = DSA_GetItemPtr(hdsaPMItems, pmtag.wItem);
                if (lppmitem)
                {
                    Str_SetPtr(&lppmitem->lpszCL, szText);
                }
#ifdef DEBUG
                else
                {
                    DebugMsg(DM_ERROR, TEXT("gc.ht: Item is invalid."));
                }
#endif
                dwPosition += pmtag.cb;
                break;
            }
            case ID_HOTKEY:
            {
//                DebugMsg(DM_TRACE, "gc.ht: Hotkey found for %d.", pmtag.wItem);
#ifdef UNICODE
                ReadFile(fh, &wHotKey, SIZEOF(wHotKey), &dwBytesRead, NULL);
#else
                _lread(fh, &wHotKey, SIZEOF(wHotKey));
#endif
                lppmitem = DSA_GetItemPtr(hdsaPMItems, pmtag.wItem);
                if (lppmitem)
                {
                    lppmitem->wHotKey = wHotKey;
                }
#ifdef DEBUG
                else
                {
                    DebugMsg(DM_ERROR, TEXT("gc.ht: Item is invalid."));
                }
#endif
                dwPosition += pmtag.cb;
                break;
            }
            case ID_MINIMIZE:
            {
//                DebugMsg(DM_TRACE, "gc.ht: Minimise flag found for %d.", pmtag.wItem);
                lppmitem = DSA_GetItemPtr(hdsaPMItems, pmtag.wItem);
                if (lppmitem)
                {
                    lppmitem->nShowCmd = SW_SHOWMINNOACTIVE;
                }
#ifdef DEBUG
                else
                {
                    DebugMsg(DM_ERROR, TEXT("gc.ht: Item is invalid."));
                }
#endif
                // Skip to the next tag.
                dwPosition += pmtag.cb;
                break;
            }
#ifdef WINNT
            case ID_NEWVDM:
            {
                // DebugMsg(DM_TRACE, "gc.ht: Separate VDM flag found for %d.", pmtag.wItem );
                lppmitem = DSA_GetItemPtr(hdsaPMItems, pmtag.wItem);
                if (lppmitem)
                {
                    lppmitem->bSepVdm = TRUE;
                }
#ifdef DEBUG
                else
                {
                    DebugMsg(DM_ERROR, TEXT("gc.ht: Item is invalid."));
                }
#endif
                // Skip to the next tag.
                dwPosition += pmtag.cb;
                break;
            }
#endif
            default:
            {
                // We've found something we don't understand but we haven't
                // found the first tag yet - probably a bust file.
                if (!fFirstTag)
                {
                    DebugMsg(DM_TRACE, TEXT("gc.ht: No initial tag found - tags section is corrupt."));
                    fTags = FALSE;
                }
                else
                {
                    // Some unknown tag.
                    if (pmtag.cb < SIZEOF(PMTAG))
                    {
                        // Can't continue!
                        DebugMsg(DM_TRACE, TEXT("gc.ht: Tag has invalid size - ignoring remaining tags."));
                        fTags = FALSE;
                    }
                    else
                    {
                        // Just ignore its data and continue.
                        dwPosition += pmtag.cb;
                    }
                }
                break;
            }
        }
    }
}

//---------------------------------------------------------------------------
void DeleteBustedItems(void)
{
    int i, cItems;
    LPPMITEM ppmitem;


    cItems = DSA_GetItemCount(hdsaPMItems);
    for (i=0; i<cItems; i++)
    {
        ppmitem = DSA_GetItemPtr(hdsaPMItems, i);
        // Is the item broken?
        if (!ppmitem->lpszDesc || !(*ppmitem->lpszDesc))
        {
            // Yep, delete it.
            DSA_DeleteItem(hdsaPMItems, i);
            cItems--;
            i--;
        }
    }
}

//---------------------------------------------------------------------------
void ShortenDescriptions(void)
{
    int i, cItems;
    LPPMITEM ppmitem;

    cItems = DSA_GetItemCount(hdsaPMItems);
    for (i=0; i<cItems; i++)
    {
        ppmitem = DSA_GetItemPtr(hdsaPMItems, i);
        // Shorten the descriptions
        lstrcpyn(ppmitem->lpszDesc, ppmitem->lpszDesc, 9);
    }
}

//---------------------------------------------------------------------------
// Kinda like PathFindFileName() but handles things like c:\foo\ differently
// to match progmans code.
LPTSTR WINAPI _PathFindFileName(LPCTSTR pPath)
{
    LPCTSTR pT;

    for (pT = pPath; *pPath; pPath = CharNext(pPath)) {
        if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':')) && (pPath[1] != TEXT('\\')))
            pT = pPath + 1;
    }

    return (LPTSTR)pT;   // const -> non const
}

//---------------------------------------------------------------------------
// Take a 3.1 format WD and exe and convert them to the new style.
// NB Old style was WD+exename and exepath - new style is exepath+exename and
// WD.
void MungePaths(void)
{
    LPTSTR lpszFileName;         // Ptr to filename part (plus params).
    LPTSTR lpszParams;           // Ptr to first char of params.
    TCHAR szCL[MAX_PATH];
    TCHAR szWD[MAX_PATH];
    int i, cItems;
    LPPMITEM lppmitem;


    cItems = DSA_GetItemCount(hdsaPMItems);

    for (i=0; i<cItems; i++)
    {
        szCL[0] = TEXT('\0');
        szWD[0] = TEXT('\0');
        lppmitem = DSA_GetItemPtr(hdsaPMItems, i);

        // Get the current command line.
        Str_GetPtr(lppmitem->lpszCL, szCL, ARRAYSIZE(szCL));
        // Get the current working dir.
        Str_GetPtr(lppmitem->lpszWD, szWD, ARRAYSIZE(szWD));
#ifdef OLDWAY
        // Find the filename part...
        // Params will confuse PFFN.
        lpszParams = PathGetArgs(szWD);
        if (*lpszParams)
        {
            // Chop them off.
            // NB Previous char is a space by definition.
            *(lpszParams-1) = TEXT('\0');
            lpszFileName = _PathFindFileName(szWD);
            // Put them back
            *(lpszParams-1) = TEXT(' ');
        }
        else
        {
            // No params.
            lpszFileName = PathFindFileName(szWD);
        }
        // Copy this onto the exe path.
        lstrcat((LPTSTR) szCL, lpszFileName);
        // Remove it from the end of the WD.
        *lpszFileName = TEXT('\0');
        // For anything but things like c:\ remove the last slash.
        if (!PathIsRoot(szWD))
        {
            *(lpszFileName-1) = TEXT('\0');
        }
#else
        lpszFileName = szWD;

        if (*lpszFileName == TEXT('"'))
        {
            while (lpszFileName)
            {
                lpszFileName = StrChr(lpszFileName+1,TEXT('"'));
                if (!lpszFileName)
                {
                    //
                    // The directory is not in quotes and since the command
                    // path starts with a quote, there is no working directory.
                    //
                    lpszFileName = szWD;
                    break;
                }
                if (*(lpszFileName+1) == TEXT('\\'))
                {
                    //
                    // The working directory is in quotes.
                    //
                    lpszFileName++;
                    break;
                }
            }
        }
        else
        {
            //
            // if there's a working directory, it is not in quotes
            // Copy up until the last \ preceding any quote, space, or the end
            //
            LPTSTR lpEnd = lpszFileName;

            while (*lpszFileName && *lpszFileName != TEXT('"') && *lpszFileName != TEXT(' '))
            {
                if ((*lpszFileName == TEXT('\\') || *lpszFileName == TEXT(':')) && *(lpszFileName+1) != TEXT('\\'))
                    lpEnd = lpszFileName;
                lpszFileName = CharNext(lpszFileName);
            }
            lpszFileName = lpEnd;
        }
        //
        // If the split is at the beginning,
        // then there is no working dir
        //
        if (lpszFileName == szWD)
        {
            lstrcat(szCL, szWD);
            szWD[0] = TEXT('\0');
        }
        else
        {
            lstrcat(szCL, lpszFileName+1);
            *(lpszFileName+1) = TEXT('\0');        // Split it.

            //
            // Remove quotes from the working dir NOW.
            //
            if (szWD[0] == TEXT('"')) {
               LPTSTR lpTemp;

               for (lpTemp = szWD+1; *lpTemp && *lpTemp != TEXT('"'); lpTemp++)
                  *(lpTemp-1) = *lpTemp;

               if (*lpTemp == TEXT('"')) {
                  *(lpTemp-1) = TEXT('\0');
               }
            }

            // For anything but things like c:\ remove the last slash.
            if (!PathIsRoot(szWD))
            {
                *lpszFileName = TEXT('\0');
            }
        }
#endif

        // Replace the data.
        Str_SetPtr(&lppmitem->lpszCL, szCL);
        Str_SetPtr(&lppmitem->lpszWD, szWD);

        // DebugMsg(DM_TRACE, "gc.mp: Exe %s, WD %s", (LPSTR)szCL, (LPSTR)szWD);
    }
}


//---------------------------------------------------------------------------
// Set all the fields of the given pmitem to clear;
void PMItem_Clear(LPPMITEM lppmitem)
    {
    lppmitem->lpszDesc = NULL;
    lppmitem->lpszCL = NULL;
    lppmitem->lpszWD = NULL;
    lppmitem->lpszIconPath = NULL;
    lppmitem->wiIcon = 0;
    lppmitem->wHotKey = 0;
    lppmitem->nShowCmd = SW_SHOWNORMAL;
#ifdef WINNT
    lppmitem->bSepVdm = FALSE;
#endif
    }

//---------------------------------------------------------------------------
// Read the item data from the file and add it to the list.
// Returns TRUE if everything went perfectly.
#ifdef UNICODE
BOOL GetAllItemData(HANDLE fh, WORD cItems, WORD cbGroup, LPTSTR lpszOldGrpTitle, LPTSTR lpszNewGrpPath)
#else
BOOL GetAllItemData(HFILE fh, WORD cItems, WORD cbGroup, LPTSTR lpszOldGrpTitle, LPTSTR lpszNewGrpPath)
#endif
{
    UINT cbItemArray;
    WORD *rgItems;
    UINT i, iItem;
    TCHAR szDesc[CCHSZNORMAL];
    TCHAR szCL[CCHSZNORMAL];
    TCHAR szIconPath[CCHSZNORMAL];
    ITEMDEF itemdef;
    BOOL fOK = TRUE;
    UINT cbRead;
    PMITEM pmitem;
#ifdef UNICODE
    DWORD dwBytesRead;
#endif

    // Read in the old item table...
    iItem = 0;
    cbItemArray = cItems * SIZEOF(*rgItems);
    rgItems = (WORD *)LocalAlloc(LPTR, cbItemArray);
    if (!rgItems)
    {
        DebugMsg(DM_ERROR, TEXT("gc.gcnfo: Out of memory."));
        return FALSE;
    }
#ifdef UNICODE
    SetFilePointer(fh, SIZEOF(GROUPDEF), NULL, FILE_BEGIN);
    ReadFile(fh, rgItems, cbItemArray, &dwBytesRead, NULL);
#else
    _llseek(fh, SIZEOF(GROUPDEF), 0);
    _lread(fh, rgItems, cbItemArray);
#endif

    // Show progress in two stages, first reading then writing.
    Group_SetProgressNameAndRange(lpszNewGrpPath, (cItems*2)-1);

    // Read in the items.
    // NB Don't just skip busted items since the tag data contains
    // indices to items and that includes busted ones. Just use
    // an empty description to indicate that the link is invalid.
    for (i=0; i<cItems; i++)
    {
        Group_SetProgress(i);

        szDesc[0] = TEXT('\0');
        szCL[0] = TEXT('\0');
        szIconPath[0] = TEXT('\0');
        itemdef.iIcon = 0;

        if (rgItems[i] == 0)
        {
            DebugMsg(DM_TRACE, TEXT("gc.gcnfo: Old group file has empty item definition - skipping."));
            goto AddItem;
        }
        if (rgItems[i] > cbGroup)
        {
            DebugMsg(DM_TRACE, TEXT("gc.gcnfo: Old group file busted (item entry in invalid part of file) - skipping item."));
            fOK = FALSE;
            goto AddItem;
        }
#ifdef UNICODE
        SetFilePointer(fh, rgItems[i], NULL, FILE_BEGIN);
        ReadFile(fh, &itemdef, SIZEOF(itemdef), &cbRead, NULL);
#else
        _llseek(fh, rgItems[i], 0);
        cbRead = _lread(fh, &itemdef, SIZEOF(itemdef));
#endif
        if (cbRead != SIZEOF(itemdef))
        {
            DebugMsg(DM_TRACE, TEXT("gc.gcnfo: Old group file busted (invalid definition) - skipping item %d."), i);
            fOK = FALSE;
            goto AddItem;
        }
        if (!CheckItemDef(&itemdef, cbGroup))
        {
            DebugMsg(DM_TRACE, TEXT("gc.gcnfo: Old group file busted (invalid item field) - skipping item %d."), i);
            fOK = FALSE;
            goto AddItem;
        }
#ifdef UNICODE
        SetFilePointer(fh, itemdef.pName, NULL, FILE_BEGIN);
#else
        _llseek(fh, itemdef.pName, 0);
#endif
        fgets(szDesc, SIZEOF(szDesc), fh);
        if (!*szDesc)
        {
            DebugMsg(DM_TRACE, TEXT("gc.gcnfo: Old group file busted (empty name) - skipping item %d."), i);
            fOK = FALSE;
            goto AddItem;
        }
#ifdef UNICODE
        SetFilePointer(fh, itemdef.pCommand, NULL, FILE_BEGIN);
#else
        _llseek(fh, itemdef.pCommand, 0);
#endif
        fgets(szCL, SIZEOF(szCL), fh);

// We hit this case with links to c:\ (rare, very rare).
#if 0
        if (!*szCL)
        {
            DebugMsg(DM_TRACE, TEXT("gc.gcnfo: Old group file busted (empty command line) - skipping item %d."), i);
            // We use a null description to signal a problem with this item.
            szDesc[0] = TEXT('\0');
            fOK = FALSE;
            goto AddItem;
        }
#endif

        if (itemdef.pIconPath!=0xFFFF)
        {
#ifdef UNICODE
            SetFilePointer(fh, itemdef.pIconPath, NULL, FILE_BEGIN);
#else
            _llseek(fh, itemdef.pIconPath, 0);
#endif
            fgets(szIconPath, SIZEOF(szIconPath), fh);
        }
        else
        {
            szIconPath[ 0 ] = TEXT('\0');
        }

        if (!*szIconPath)
        {
            // NB Do nothing. Empty icon paths are legal - associated apps where the associated
            // app is missing will have an empty icon path.
        }

        // NB Forget about the icon data.

        // DebugMsg(DM_TRACE, "gc.gcnfo: Found item %s.", (LPSTR) szDesc);

        // Store away the data....
        // NB We load the old commands line into the working dir field because
        // only the leaf is the command, the rest is the WD. Once we've been
        // through the tags section we can sort out the mess.
AddItem:
        PMItem_Clear(&pmitem);

#ifdef DEBUG
        DebugMsg(GC_TRACE, TEXT("gc.gaid: Desc %s"), (LPTSTR) szDesc);
        DebugMsg(GC_TRACE, TEXT("    WD: %s"), (LPTSTR) szCL);
        DebugMsg(GC_TRACE, TEXT("    IP: %s(%d)"), (LPTSTR) szIconPath, itemdef.iIcon);
#endif

        // Don't store anything for items with invalid descriptions.
        if (*szDesc)
        {
            // Remove illegal chars.
            PathRemoveIllegalChars(szDesc, 0, PRICF_NORMAL);
            Str_SetPtr(&pmitem.lpszDesc, szDesc);
            Str_SetPtr(&pmitem.lpszWD, szCL);
            Str_SetPtr(&pmitem.lpszIconPath, szIconPath);
            pmitem.wiIcon = itemdef.iIcon;
        }

        DSA_InsertItem(hdsaPMItems, iItem, &pmitem);

        iItem++;
    }

    LocalFree((HLOCAL)rgItems);

    return fOK;
}

//-----------------------------------------------------------------------------
// Functions to try to find out which icon was appropriate given the NT icon
// identifier number (the identifier for the RT_ICON resource only).
//-----------------------------------------------------------------------------
typedef struct _enumstruct {
    UINT    iIndex;
    BOOL    fFound;
    WORD    wIconRTIconID;
} ENUMSTRUCT, *LPENUMSTRUCT;

BOOL EnumIconFunc(
    HMODULE hMod,
    LPCTSTR lpType,
    LPTSTR  lpName,
    LPARAM  lParam
) {
    HANDLE  h;
    PBYTE   p;
    int     id;
    LPENUMSTRUCT    lpes = (LPENUMSTRUCT)lParam;

    if (!lpName)
        return TRUE;

    h = FindResource(hMod, lpName, lpType);
    if (!h)
        return TRUE;

    h = LoadResource(hMod, h);
    p = LockResource(h);
    id = LookupIconIdFromDirectory(p, TRUE);
    UnlockResource(h);
    FreeResource(h);

    if (id == lpes->wIconRTIconID)
    {
        lpes->fFound = TRUE;
        return FALSE;
    }
    lpes->iIndex++;

    return TRUE;
}

WORD FindAppropriateIcon( LPTSTR lpszFileName, WORD wIconRTIconID )
{
    HINSTANCE hInst;
    TCHAR   szExe[MAX_PATH];
    WORD    wIcon = wIconRTIconID;
    ENUMSTRUCT  es;
    int olderror;

    hInst = FindExecutable(lpszFileName,NULL,szExe);
    if ( hInst <= (HINSTANCE)HINSTANCE_ERROR )
    {
        return 0;
    }

    olderror = SetErrorMode(SEM_FAILCRITICALERRORS);
    hInst = LoadLibraryEx(szExe,NULL, DONT_RESOLVE_DLL_REFERENCES);
    SetErrorMode(olderror);
    if ( hInst <= (HINSTANCE)HINSTANCE_ERROR )
    {
        return 0;
    }

    es.iIndex = 0;
    es.fFound = FALSE;
    es.wIconRTIconID = wIconRTIconID;

    EnumResourceNames( hInst, RT_GROUP_ICON, EnumIconFunc, (LPARAM)&es );

    FreeLibrary( hInst );

    if (es.fFound)
    {
        return (WORD)es.iIndex;
    }
    else
    {
        return 0;
    }
}

//---------------------------------------------------------------------------
// Read the item data from the file and add it to the list.
// Returns TRUE if everything went perfectly.
#ifdef UNICODE
BOOL GetAllItemDataNT(HANDLE fh, WORD cItems, DWORD cbGroup, LPTSTR lpszOldGrpTitle, LPTSTR lpszNewGrpPath)
#else
BOOL GetAllItemDataNT(HFILE fh, WORD cItems, DWORD cbGroup, LPTSTR lpszOldGrpTitle, LPTSTR lpszNewGrpPath)
#endif
{
    UINT cbItemArray;
    DWORD *rgItems;
    UINT i, iItem;
    WCHAR wszTemp[CCHSZNORMAL];
    TCHAR szDesc[CCHSZNORMAL];
    TCHAR szCL[CCHSZNORMAL];
    TCHAR szIconPath[CCHSZNORMAL];
    NT_ITEMDEF itemdef;
    BOOL fOK = TRUE;
#ifdef UNICODE
    DWORD cbRead;
#else
    UINT cbRead;
#endif
    PMITEM pmitem;

    // Read in the old item table...
    iItem = 0;
    cbItemArray = cItems * SIZEOF(*rgItems);
    rgItems = (DWORD *)LocalAlloc(LPTR, cbItemArray);
    if (!rgItems)
    {
        DebugMsg(DM_ERROR, TEXT("gc.gcnfo: Out of memory."));
        return FALSE;
    }
#ifdef UNICODE
    SetFilePointer(fh, FIELD_OFFSET(NT_GROUPDEF,rgiItems[0]), NULL, FILE_BEGIN);
    ReadFile(fh, rgItems, cbItemArray, &cbRead, NULL);
#else
    _llseek(fh, FIELD_OFFSET(NT_GROUPDEF,rgiItems[0]), 0);
    _lread(fh, rgItems, cbItemArray);
#endif

    // Show progress in two stages, first reading then writing.
    Group_SetProgressNameAndRange(lpszNewGrpPath, (cItems*2)-1);

    // Read in the items.
    // NB Don't just skip busted items since the tag data contains
    // indices to items and that includes busted ones. Just use
    // an empty description to indicate that the link is invalid.
    for (i=0; i<cItems; i++)
    {
        Group_SetProgress(i);

        szDesc[0] = TEXT('\0');
        szCL[0] = TEXT('\0');
        szIconPath[0] = TEXT('\0');
        itemdef.iIcon = 0;

        if (rgItems[i] == 0)
        {
            DebugMsg(DM_TRACE, TEXT("gc.gcnfo: Old group file has empty item definition - skipping."));
            goto AddItem;
        }
        if (rgItems[i] > cbGroup)
        {
            DebugMsg(DM_TRACE, TEXT("gc.gcnfo: Old group file busted (item entry in invalid part of file) - skipping item."));
            fOK = FALSE;
            goto AddItem;
        }
#ifdef UNICODE
        SetFilePointer(fh, rgItems[i], NULL, FILE_BEGIN);
        ReadFile(fh, &itemdef, SIZEOF(itemdef), &cbRead, NULL);
#else
        _llseek(fh, rgItems[i], 0);
        cbRead = _lread(fh, &itemdef, SIZEOF(itemdef));
#endif
        if (cbRead != SIZEOF(itemdef))
        {
            DebugMsg(DM_TRACE, TEXT("gc.gcnfo: Old group file busted (invalid definition) - skipping item %d."), i);
            fOK = FALSE;
            goto AddItem;
        }
        if (!CheckItemDefNT(&itemdef, cbGroup))
        {
            DebugMsg(DM_TRACE, TEXT("gc.gcnfo: Old group file busted (invalid item field) - skipping item %d."), i);
            fOK = FALSE;
            goto AddItem;
        }
#ifdef UNICODE
        SetFilePointer(fh, itemdef.pName, NULL, FILE_BEGIN);
        ReadFile(fh, wszTemp, SIZEOF(wszTemp), &cbRead, NULL);
#else
        _llseek(fh, itemdef.pName, 0);
        _lread(fh, wszTemp, SIZEOF(wszTemp)); // There will be a NUL somewhere
#endif
        if (!*wszTemp)
        {
            DebugMsg(DM_TRACE, TEXT("gc.gcnfo: Old group file busted (empty name) - skipping item %d."), i);
            fOK = FALSE;
            goto AddItem;
        }
#ifdef UNICODE
        lstrcpy(szDesc, wszTemp);
#else
        WideCharToMultiByte (CP_ACP, 0, wszTemp, -1,
                         szDesc, ARRAYSIZE(szDesc), NULL, NULL);
#endif

#ifdef UNICODE
        SetFilePointer(fh, itemdef.pCommand, NULL, FILE_BEGIN);
        ReadFile(fh, &wszTemp, SIZEOF(wszTemp), &cbRead, NULL);
#else
        _llseek(fh, itemdef.pCommand, 0);
        _lread(fh, wszTemp, SIZEOF(wszTemp));
#endif
        if (!*wszTemp)
        {
            DebugMsg(DM_TRACE, TEXT("gc.gcnfo: Old group file busted (empty command line) - skipping item %d."), i);
            // We use a null description to signal a problem with this item.
            szDesc[0] = TEXT('\0');
            fOK = FALSE;
            goto AddItem;
        }
#ifdef UNICODE
        lstrcpy(szCL, wszTemp);
#else
        WideCharToMultiByte (CP_ACP, 0, wszTemp, -1,
                         szCL, ARRAYSIZE(szCL), NULL, NULL);
#endif

#ifdef UNICODE
        SetFilePointer(fh, itemdef.pIconPath, NULL, FILE_BEGIN);
        ReadFile(fh, wszTemp, SIZEOF(wszTemp), &cbRead, NULL);
#else
        _llseek(fh, itemdef.pIconPath, 0);
        _lread(fh, wszTemp, SIZEOF(wszTemp));
#endif
        if (!*wszTemp)
        {
            // NB Do nothing. Empty icon paths are legal - associated apps where the associated
            // app is missing will have an empty icon path.
        }
#ifdef UNICODE
        lstrcpy(szIconPath, wszTemp);
#else
        WideCharToMultiByte (CP_ACP, 0, wszTemp, -1,
                         szIconPath, ARRAYSIZE(szIconPath), NULL, NULL);
#endif

        // NB Forget about the icon data.

        // DebugMsg(DM_TRACE, "gc.gcnfo: Found item %s.", (LPSTR) szDesc);

        // Store away the data....
        // NB We load the old commands line into the working dir field because
        // only the leaf is the command, the rest is the WD. Once we've been
        // through the tags section we can sort out the mess.
AddItem:
        PMItem_Clear(&pmitem);

#ifdef DEBUG
        DebugMsg(GC_TRACE, TEXT("gc.gaid: Desc %s"), (LPTSTR) szDesc);
        DebugMsg(GC_TRACE, TEXT("    WD: %s"), (LPTSTR) szCL);
        DebugMsg(GC_TRACE, TEXT("    IP: %s(%d)"), (LPTSTR) szIconPath, itemdef.iIcon);
#endif

        // Don't store anything for items with invalid descriptions.
        if (*szDesc)
        {
            WORD    wIconIndex;

            // Remove illegal chars.
            PathRemoveIllegalChars(szDesc, 0, PRICF_NORMAL);
            Str_SetPtr(&pmitem.lpszDesc, szDesc);
            Str_SetPtr(&pmitem.lpszWD, szCL);
            Str_SetPtr(&pmitem.lpszIconPath, szIconPath);

            wIconIndex = itemdef.wIconIndex;
            if ( wIconIndex == 0 )
            {
                WORD    wIcon;
                HICON   hIcon;

                if ( *szIconPath == TEXT('\0') )
                {
                    FindExecutable(szCL,NULL,szIconPath);
                }
                if ( *szIconPath != TEXT('\0') )
                {
                    wIconIndex = FindAppropriateIcon( szIconPath, itemdef.iIcon);
                }
            }
            pmitem.wiIcon = wIconIndex;
        }

        DSA_InsertItem(hdsaPMItems, iItem, &pmitem);

        iItem++;
    }

    LocalFree((HLOCAL)rgItems);

    return fOK;
}

//---------------------------------------------------------------------------
// Create the links in the given dest dir.
void CreateLinks(LPCTSTR lpszNewGrpPath, BOOL fStartup, INT cItemsStart)
{
    int i, cItems;
    TCHAR szLinkName[MAX_PATH];
    TCHAR szBuffer[MAX_PATH];
    // we make this 3*MAX_PATH so that DARWIN and LOGO3 callers can pass their extra information
    TCHAR szExpBuff[3*MAX_PATH];
    WCHAR wszPath[MAX_PATH];
    LPTSTR lpszArgs;
    LPCTSTR dirs[2];
    IShellLink *psl;
    LPTSTR pszExt;

    if (SUCCEEDED(ICoCreateInstance(&CLSID_ShellLink, &IID_IShellLink, &psl))) {
        IPersistFile *ppf;
        psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf);

        cItems = DSA_GetItemCount(hdsaPMItems);

        for (i = 0; i < cItems; i++) {
            LPPMITEM lppmitem = DSA_GetItemPtr(hdsaPMItems, i);

            // We show the progress in 2 halves.
            Group_SetProgress(cItemsStart+(i*cItemsStart/cItems));

            // command line and args.
            // if this command line points to net drives we should add
            // the UNC mapping to the link
            Str_GetPtr(lppmitem->lpszCL, szBuffer, ARRAYSIZE(szBuffer));

            // Spaces at the begining of the CL will confuse us.
            PathRemoveBlanks(szBuffer);

            lpszArgs = PathGetArgs(szBuffer);
            if (*lpszArgs)
                *(lpszArgs-1) = TEXT('\0');

            // NB Special case, remove all links to Progman[.exe] from the
            // Startup Group. A lot of people put it there to give it a hotkey.
            // We want to be able to delete it regardless of its name ie we
            // can't just use setup.ini to do the work.
            if (fStartup)
            {
                if ((lstrcmpi(c_szProgmanExe, PathFindFileName(szBuffer)) == 0) ||
                    (lstrcmpi(c_szProgman, PathFindFileName(szBuffer)) == 0))
                    continue;
            }

            psl->lpVtbl->SetArguments(psl, lpszArgs);

            //
            // Remove quotes from the command file name NOW.
            //
            if (szBuffer[0] == TEXT('"')) {
               LPTSTR lpTemp;

               for (lpTemp = szBuffer+1; *lpTemp && *lpTemp != TEXT('"'); lpTemp++)
                  *(lpTemp-1) = *lpTemp;

               if (*lpTemp == TEXT('"')) {
                  *(lpTemp-1) = TEXT('\0');
               }
            }

            // working directory
            // NB Progman assumed an empty WD meant use the windows
            // directory but we want to change this so to be
            // backwards compatable we'll fill in missing WD's here.
            if (!lppmitem->lpszWD || !*lppmitem->lpszWD)
            {
                // NB For links to pif's we don't fill in a default WD
                // so we'll pick it up from pif itself. This fixes a
                // problem upgrading some Compaq Deskpro's.
                pszExt = PathFindExtension(szBuffer);
                if (lstrcmpi(pszExt, c_szDotPif) == 0)
                {
                    psl->lpVtbl->SetWorkingDirectory(psl, c_szNULL);
                }
                else
                {
#ifdef WINNT
                    // Avoid setting to %windir%, under NT we want to change to the users home directory.
                    psl->lpVtbl->SetWorkingDirectory( psl, TEXT("%HOMEDRIVE%%HOMEPATH%") );
#else
                    // Not a pif. Set the WD to be that of the windows dir.
                    psl->lpVtbl->SetWorkingDirectory(psl, TEXT("%windir%"));
#endif
                }
            }
            else
            {
                psl->lpVtbl->SetWorkingDirectory(psl, lppmitem->lpszWD);
            }

            // icon location

            // REVIEW, do we want to unqualify the icon path if possible?  also,
            // if the icon path is the same as the command line we don't need it
            if (lppmitem->wiIcon != 0 || lstrcmpi(lppmitem->lpszIconPath, szBuffer) != 0)
            {
                // Remove args.
                lpszArgs = PathGetArgs(lppmitem->lpszIconPath);
                if (*lpszArgs)
                    *(lpszArgs-1) = TEXT('\0');
                psl->lpVtbl->SetIconLocation(psl, lppmitem->lpszIconPath, lppmitem->wiIcon);
            }
            else
            {
                psl->lpVtbl->SetIconLocation(psl, NULL, 0);
            }

            // hotkey
            psl->lpVtbl->SetHotkey(psl, lppmitem->wHotKey);

            // show command
            psl->lpVtbl->SetShowCmd(psl, lppmitem->nShowCmd);

            // Description. Currently pifmgr is the only guy
            // that cares about the description and they use
            // it to overide the default pif description.
            psl->lpVtbl->SetDescription(psl, lppmitem->lpszDesc);

            //
            //  NOTE it is very important to set filename *last*
            //  because if this is a group item to another link
            //  (either .lnk or .pif) we want the link properties
            //  to override the ones we just set.
            //
            // BUGBUG: qualify path to subject (szBuffer)

            dirs[0] = lppmitem->lpszWD;
            dirs[1] = NULL;

            // Try expanding szBuffer
            ExpandEnvironmentStrings( szBuffer, szExpBuff, MAX_PATH );
            szExpBuff[ MAX_PATH-1 ] = TEXT('\0');
            if (!PathResolve(szExpBuff, dirs, PRF_TRYPROGRAMEXTENSIONS))
            {
                // Just assume the expanded thing was a-ok...
                ExpandEnvironmentStrings(szBuffer, szExpBuff, MAX_PATH);
                szExpBuff[ MAX_PATH-1 ] = TEXT('\0');
            }

            // all we need to call is setpath, it takes care of creating the
            // pidl for us.
            psl->lpVtbl->SetPath( psl, szBuffer );
#ifdef WINNT
            {
                IShellLinkDataList* psldl;

                if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IShellLinkDataList, (LPVOID)&psldl)))
                {
                    DWORD dwFlags;
                    if (SUCCEEDED(psldl->lpVtbl->GetFlags(psldl, &dwFlags)))
                    {
                        if (lppmitem->bSepVdm)
                            dwFlags |= SLDF_RUN_IN_SEPARATE;
                        else
                            dwFlags &= (~SLDF_RUN_IN_SEPARATE);

                        psldl->lpVtbl->SetFlags(psldl, dwFlags);
                    }
                    psldl->lpVtbl->Release(psldl);
                }
            }
#endif

            // over write the link if it already exists

            PathCombine(szLinkName, lpszNewGrpPath, lppmitem->lpszDesc);
            lstrcat(szLinkName, TEXT(".lnk"));
            PathQualify(szLinkName);
            // OLE string.
            StrToOleStrN(wszPath, ARRAYSIZE(wszPath), szLinkName, -1);
            ppf->lpVtbl->Save(ppf, wszPath, TRUE);
        }
        ppf->lpVtbl->Release(ppf);
        psl->lpVtbl->Release(psl);
    }
}

//----------------------------------------------------------------------------
// Returns TRUE if the specified group title is that of the startup group.
BOOL StartupCmp(LPTSTR szGrp)
{
    static TCHAR szOldStartupGrp[MAX_PATH];
    TCHAR szNewStartupPath[MAX_PATH];

    if (!*szOldStartupGrp)
    {
        // Was it over-ridden in progman ini?
        GetPrivateProfileString(c_szSettings, c_szStartup, c_szNULL, szOldStartupGrp,
            ARRAYSIZE(szOldStartupGrp), c_szProgmanIni);
        if (!*szOldStartupGrp)
        {
            LONG    lResult;
            DWORD   cbSize;

            // No, try reading it from the NT registry
            cbSize = MAX_PATH;
            lResult = RegQueryValue(HKEY_CURRENT_USER, c_szProgmanStartup, szOldStartupGrp, &cbSize );
            //
            // BUGBUG_JAPAN - Probably need a check for Kana Start
            //
            if ( lResult != ERROR_SUCCESS )
            {
                // No, use the default name.
                LoadString(g_hinst, IDS_STARTUP, szOldStartupGrp, ARRAYSIZE(szOldStartupGrp));
            }
        }

        if (*szOldStartupGrp)
        {
            // Yes, use the over-riding name by updating the registry.
            SHGetSpecialFolderPath(NULL, szNewStartupPath, CSIDL_PROGRAMS, FALSE);
            PathAddBackslash(szNewStartupPath);
            lstrcat(szNewStartupPath, szOldStartupGrp);
            DebugMsg(DM_TRACE, TEXT("gc.sc: Non-default Startup path is %s."), szNewStartupPath);
            Reg_SetString(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER_SHELLFOLDERS, c_szStartup, szNewStartupPath);
        }

    }

    // Does it match?
    if (*szOldStartupGrp && (lstrcmpi(szGrp, szOldStartupGrp) == 0))
        return TRUE;
    else
        return FALSE;
}

//---------------------------------------------------------------------------
BOOL CALLBACK IsDescUnique(LPCTSTR lpsz, UINT n)
{
    int i, cItems;
    LPPMITEM pitem;

    // DebugMsg(DM_TRACE, "gc.idu: Checking uniqueness of %s.", lpsz);

    cItems = DSA_GetItemCount(hdsaPMItems);
    for (i=0; i<cItems; i++)
    {
        // N is our guy, skip it.
        if ((UINT)i == n)
            continue;

        pitem = DSA_GetItemPtr(hdsaPMItems, i);
        Assert(pitem);
        if (pitem->lpszDesc && *pitem->lpszDesc && (lstrcmpi(pitem->lpszDesc, lpsz) == 0))
        {
            // DebugMsg(DM_TRACE, "gc.idu: Not Unique.");
            return FALSE;
        }
    }
    // Yep. can't find it, must be unique.
    // DebugMsg(DM_TRACE, "gc.idu: Unique.");
    return TRUE;
}

//---------------------------------------------------------------------------
// If there are two or more items with the same link name then change them so
// that they are unique.
void ResolveDuplicates(LPCTSTR pszNewGrpPath)
{
    LPPMITEM pitem;
    int i, cItems;
    TCHAR szNew[MAX_PATH];
    BOOL fLFN;
    UINT cchSpace;

    DebugMsg(DM_TRACE, TEXT("gc.rd: Fixing dups..."));

    // How much room is there for adding the #xx stuff?
    cchSpace = (ARRAYSIZE(szNew)-lstrlen(pszNewGrpPath))-2;

    // LFN's or no?
    fLFN = IsLFNDrive(pszNewGrpPath);
    if (!fLFN && cchSpace > 8)
        cchSpace = 8;

    // Fix dups
    cItems = DSA_GetItemCount(hdsaPMItems);
    for (i=0; i<(cItems-1); i++)
    {
        pitem = DSA_GetItemPtr(hdsaPMItems, i);
        Assert(pitem);
        YetAnotherMakeUniqueName(szNew, cchSpace, pitem->lpszDesc, IsDescUnique, i, fLFN);
        // Did we get a new name?
        if (lstrcmp(szNew, pitem->lpszDesc) != 0)
        {
            // Yep.
            DebugMsg(DM_TRACE, TEXT("gc.rd: %s to %s"), pitem->lpszDesc, szNew);
            Str_SetPtr(&pitem->lpszDesc, szNew);
        }
    }

    DebugMsg(DM_TRACE, TEXT("gc.rd: Done."));
}

//---------------------------------------------------------------------------
typedef struct
{
    LPTSTR pszName;
    LPTSTR pszPath;
    LPTSTR pszModule;
    LPTSTR pszVer;
} ALITEM;
typedef ALITEM *PALITEM;

//---------------------------------------------------------------------------
// Record the total list of apps in a DSA.
void AppList_WriteFile(void)
{
    int i, cItems;
    PALITEM palitem;
    TCHAR szBetaID[MAX_PATH];
    TCHAR szLine[4*MAX_PATH];
    HANDLE hFile;
    DWORD cbWritten;

    Assert(g_hdsaAppList);

    cItems = DSA_GetItemCount(g_hdsaAppList);
    if (cItems)
    {
        // Get the beta ID.
        szBetaID[0] = TEXT('\0');
        Reg_GetString(HKEY_LOCAL_MACHINE, c_szRegistry, c_szDefaultUser, szBetaID, SIZEOF(szBetaID));

        // Ick - Hard coded file name and in the current dir!
        hFile = CreateFile(c_szGrpConvData, GENERIC_WRITE, FILE_SHARE_READ, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            for (i=0; i < cItems; i++)
            {
                palitem = DSA_GetItemPtr(g_hdsaAppList, i);
                wsprintf(szLine, TEXT("%s,\"%s\",\"%s\",\"%s\",\"%s\",,,\r\n"), szBetaID, palitem->pszName,
                    palitem->pszPath, palitem->pszModule, palitem->pszVer);
                DebugMsg(DM_TRACE,TEXT("gc.al_wf: %s"), szLine);
                WriteFile(hFile, szLine, lstrlen(szLine)*SIZEOF(TCHAR), &cbWritten, NULL);
            }
            CloseHandle(hFile);
        }
        else
        {
            DebugMsg(DM_ERROR, TEXT("gc.al_wf: Can't write file."));
        }
    }
    else
    {
        DebugMsg(DM_TRACE, TEXT("gc.al_wf: Empty app list. Nothing to write."));
    }
}

//---------------------------------------------------------------------------
//#define DSA_AppendItem(hdsa, pitem)  DSA_InsertItem(hdsa, 0x7fff, pitem)

//---------------------------------------------------------------------------
static TCHAR const c_szTranslation[] = TEXT("\\VarFileInfo\\Translation");
static TCHAR const c_szStringFileInfo[] = TEXT("\\StringFileInfo\\");
static TCHAR const c_szEngLangCharSet[] = TEXT("040904e4");
static TCHAR const c_szSlash[] = TEXT("\\");
static TCHAR const c_szInternalName[] = TEXT("InternalName");
static TCHAR const c_szProductVersion[] = TEXT("ProductVersion");

//----------------------------------------------------------------------------
// Semi-decent wrappers around the not very good ver apis.
BOOL Ver_GetDefaultCharSet(const PVOID pBuf, LPTSTR pszLangCharSet, int cbLangCharSet)
{

    LPWORD pTransTable;
    DWORD cb;

    Assert(pszLangCharSet);
    Assert(cbLangCharSet > 8);

    if (VerQueryValue(pBuf, (LPTSTR)c_szTranslation, &pTransTable, &cb))
    {
        wsprintf(pszLangCharSet, TEXT("%04X%04X"), *pTransTable, *(pTransTable+1));
        return TRUE;
    }

    return FALSE;
}

//----------------------------------------------------------------------------
// Semi-decent wrappers around the not very good ver apis.
BOOL Ver_GetStringFileInfo(PVOID pBuf, LPCTSTR pszLangCharSet,
    LPCTSTR pszStringName, LPTSTR pszValue, int cbValue)
{
    TCHAR szSubBlock[MAX_PATH];
    LPTSTR pszBuf;
    DWORD cbBuf;

    lstrcpy(szSubBlock, c_szStringFileInfo);
    lstrcat(szSubBlock, pszLangCharSet);
    lstrcat(szSubBlock, c_szSlash);
    lstrcat(szSubBlock, pszStringName);

    if (VerQueryValue(pBuf, szSubBlock, &pszBuf, &cbBuf))
    {
        lstrcpyn(pszValue, pszBuf, cbValue);
        return TRUE;
    }
    return FALSE;
}

//---------------------------------------------------------------------------
void GetVersionInfo(LPTSTR pszPath, LPTSTR pszModule, int cbModule, LPTSTR pszVer, int cbVer)
{
    DWORD cbBuf;
    LPVOID pBuf;
    TCHAR szCharSet[MAX_PATH];
    DWORD dwWasteOfAnAuto;

    Assert(pszModule);
    Assert(pszVer);

    pszModule[0] = TEXT('\0');
    pszVer[0] = TEXT('\0');

    cbBuf = GetFileVersionInfoSize(pszPath, &dwWasteOfAnAuto);
    if (cbBuf)
    {
        pBuf = SHAlloc(cbBuf);
        if (pBuf)
        {
            if (GetFileVersionInfo(pszPath, 0, cbBuf, pBuf))
            {
                // Try the default language from the translation tables.
                if (Ver_GetDefaultCharSet(pBuf, szCharSet, ARRAYSIZE(szCharSet)))
                {
                    Ver_GetStringFileInfo(pBuf, szCharSet, c_szInternalName, pszModule, cbModule);
                    Ver_GetStringFileInfo(pBuf, szCharSet, c_szProductVersion, pszVer, cbVer);
                }
                else
                {
                    // Try the same language as us.
                    LoadString(g_hinst, IDS_DEFLANGCHARSET, szCharSet, ARRAYSIZE(szCharSet));
                    Ver_GetStringFileInfo(pBuf, szCharSet, c_szInternalName, pszModule, cbModule);
                    Ver_GetStringFileInfo(pBuf, szCharSet, c_szProductVersion, pszVer, cbVer);
                }

                // Last chance - try English.
                if (!*pszModule)
                    Ver_GetStringFileInfo(pBuf, c_szEngLangCharSet, c_szInternalName, pszModule, cbModule);
                if (!*pszVer)
                    Ver_GetStringFileInfo(pBuf, c_szEngLangCharSet, c_szProductVersion, pszVer, cbVer);
            }
            else
            {
                DebugMsg(DM_TRACE, TEXT("gc.gvi: Can't get version info."));
            }
            SHFree(pBuf);
        }
        else
        {
            DebugMsg(DM_TRACE, TEXT("gc.gvi: Can't allocate version info buffer."));
            }
    }
    else
    {
        DebugMsg(DM_TRACE, TEXT("gc.gvi: No version info."));
    }
}

//---------------------------------------------------------------------------
// Record the total list of apps in a DSA.
BOOL AppList_Create(void)
{
    Assert(!g_hdsaAppList);

    g_hdsaAppList = DSA_Create(SIZEOF(ALITEM), 0);

    if (g_hdsaAppList)
    {
        return TRUE;
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("gc.al_c: Can't create app list."));
        return FALSE;
    }
}

//---------------------------------------------------------------------------
// Record the total list of apps in a DSA.
void AppList_Destroy(void)
{
    int i, cItems;
    PALITEM palitem;

    Assert(g_hdsaAppList);

    cItems = DSA_GetItemCount(g_hdsaAppList);
    for (i=0; i < cItems; i++)
    {
        palitem = DSA_GetItemPtr(g_hdsaAppList, i);
        if (palitem->pszName)
            SHFree(palitem->pszName);
        if (palitem->pszPath)
            SHFree(palitem->pszPath);
        if (palitem->pszModule)
            SHFree(palitem->pszModule);
        if (palitem->pszVer)
            SHFree(palitem->pszVer);
    }

    DSA_Destroy(g_hdsaAppList);
    g_hdsaAppList = NULL;
}

//---------------------------------------------------------------------------
// Record the total list of apps in a DSA.
void AppList_Append(void)
{
    int i, cItems;
    // char szName[MAX_PATH];
    // char szPath[MAX_PATH];
    TCHAR szModule[MAX_PATH];
    TCHAR szVer[MAX_PATH];
    TCHAR szCL[MAX_PATH];
    LPTSTR lpszArgs;
    LPCTSTR dirs[2];
    ALITEM alitem;

    Assert(g_hdsaAppList);

    cItems = DSA_GetItemCount(hdsaPMItems);
    for (i = 0; i < cItems; i++)
    {
        LPPMITEM lppmitem = DSA_GetItemPtr(hdsaPMItems, i);

        // We show the progress in 2 halves.
        Group_SetProgress(cItems+i);

        // Command line and args.
        Str_GetPtr(lppmitem->lpszCL, szCL, ARRAYSIZE(szCL));
        lpszArgs = PathGetArgs(szCL);
        if (*lpszArgs)
            *(lpszArgs-1) = TEXT('\0');
        dirs[0] = lppmitem->lpszWD;
        dirs[1] = NULL;
        PathResolve(szCL, dirs, PRF_TRYPROGRAMEXTENSIONS);

        // Version info.
        GetVersionInfo(szCL, szModule, ARRAYSIZE(szModule), szVer, ARRAYSIZE(szVer));

        alitem.pszName = NULL;
        alitem.pszPath = NULL;
        alitem.pszModule = NULL;
        alitem.pszVer = NULL;

        Str_SetPtr(&alitem.pszName, lppmitem->lpszDesc);
        Str_SetPtr(&alitem.pszPath, szCL);
        Str_SetPtr(&alitem.pszModule, szModule);
        Str_SetPtr(&alitem.pszVer, szVer);
        DSA_AppendItem(g_hdsaAppList, &alitem);
    }
    DebugMsg(DM_TRACE, TEXT("gc.al_a: %d items"), DSA_GetItemCount(g_hdsaAppList));
}

//---------------------------------------------------------------------------
// Reads an old format Progman Group files and creates a directory containing
// links that matches the group file.
BOOL Group_CreateNewFromOld(HWND hwnd, LPCTSTR lpszOldGrpPath, UINT options)
{
    GROUPDEF grpdef;
#ifdef UNICODE
    HANDLE fh;
    DWORD  dwBytesRead;
#else
    HFILE fh;
#endif
    TCHAR szNewGrpPath[MAX_PATH];
    TCHAR szOldGrpTitle[MAXGROUPNAMELEN + 1];
    // LPSTR lpszExt;
    BOOL fStatus = FALSE;
    SHELLEXECUTEINFO sei;
    BOOL fStartup = FALSE;

    if (!ItemList_Create(lpszOldGrpPath))
        return FALSE;

#ifdef UNICODE
    fh = CreateFile(
             lpszOldGrpPath,
             GENERIC_READ,
             FILE_SHARE_READ,
             NULL,
             OPEN_EXISTING,
             0,
             NULL
             );
    if (fh == INVALID_HANDLE_VALUE) {
#else
    fh = _lopen(lpszOldGrpPath, OF_READ | OF_SHARE_DENY_NONE);
    if (fh == HFILE_ERROR) {
#endif
        DebugMsg(DM_ERROR, TEXT("gc.gcnfo: Unable to open group."));
        goto ProcExit2;
    }

#ifdef UNICODE
    if ((!ReadFile(fh, &grpdef, SIZEOF(grpdef), &dwBytesRead, NULL)) ||
        (dwBytesRead != SIZEOF(grpdef))) {
#else
    if (_lread(fh, &grpdef, SIZEOF(grpdef)) != SIZEOF(grpdef)) {
#endif
        DebugMsg(DM_ERROR, TEXT("gc.gcnfo: header too small."));
        goto ProcExit;
    }

    if (grpdef.cItems > 50) {
        // NB This isn;t fatal so carry on.
        DebugMsg(DM_ERROR, TEXT("gc.gcnfo: Too many items."));
    }

    // Check to make sure there is a name embedded in the
    // .grp file.  If not, just use the filename
    if (grpdef.pName==0) {
        LPTSTR lpszFile, lpszExt, lpszDest = szOldGrpTitle;

        lpszFile = PathFindFileName( lpszOldGrpPath );
        lpszExt  = PathFindExtension( lpszOldGrpPath );
        for( ;
             lpszFile && lpszExt && (lpszFile != lpszExt);
             *lpszDest++ = *lpszFile++
            );
        *lpszDest = TEXT('\0');

    } else {

#ifdef UNICODE
        CHAR szAnsiTitle[ MAXGROUPNAMELEN + 1 ];

        SetFilePointer(fh, grpdef.pName, NULL, FILE_BEGIN);
        ReadFile(fh, szAnsiTitle, SIZEOF(szAnsiTitle), &dwBytesRead, NULL);
        MultiByteToWideChar( CP_ACP, 0, szAnsiTitle, -1, szOldGrpTitle, ARRAYSIZE(szOldGrpTitle) );
#else
        _llseek(fh, grpdef.pName, 0);
        _lread(fh, szOldGrpTitle, SIZEOF(szOldGrpTitle));
#endif

    }

    // Get the destination dir, use the title from the old group...

    // Special case the startup group.
    if (StartupCmp(szOldGrpTitle)) {
        fStartup = TRUE;
        if (g_fDoingCommonGroups) {
            SHGetSpecialFolderPath(hwnd, szNewGrpPath, CSIDL_COMMON_STARTUP, TRUE);
        } else {
            SHGetSpecialFolderPath(hwnd, szNewGrpPath, CSIDL_STARTUP, TRUE);
        }
    } else {
        if (!Group_GenerateNewGroupPath(hwnd, szOldGrpTitle, szNewGrpPath, lpszOldGrpPath)) {
            DebugMsg(DM_ERROR, TEXT("gc.gcnfo; Unable to create destination directory."));
            goto ProcExit;
        }
    }

    // PathQualify(szNewGrpPath);

    // ResolveDuplicateGroupNames(szNewGrpPath);

    // Go through every item in the old group and make it a link...

    if (!GetAllItemData(fh, grpdef.cItems, grpdef.cbGroup, szOldGrpTitle, szNewGrpPath)) {
        if (options & GC_REPORTERROR)
            MyMessageBox(hwnd, IDS_APPTITLE, IDS_BADOLDGROUP, NULL, MB_OK | MB_ICONEXCLAMATION);
    }

    // Deal with the tags section.
    HandleTags(fh, grpdef.cbGroup);

    // Now we've dealt with the tags we don't need to keep track of
    // busted items so delete them now. From here on we always have
    // valid items.
    DeleteBustedItems();

    // Shorten descs on non-lfn drives.
    if (!IsLFNDrive(szNewGrpPath))
        ShortenDescriptions();

    // Fixup the paths/WD stuff.
    MungePaths();

    // Fix dups.
    ResolveDuplicates(szNewGrpPath);

    // Do we just want a list of the apps or create some links?
    if (options & GC_BUILDLIST)
            AppList_Append();
    else
        CreateLinks(szNewGrpPath, fStartup, grpdef.cItems);

    // Get the cabinet to show the new group.
    if (options & GC_OPENGROUP)
    {
        sei.cbSize = SIZEOF(sei);
        sei.fMask = 0;
        sei.hwnd = hwnd;
        sei.lpVerb = NULL;
        sei.lpFile = szNewGrpPath;
        sei.lpParameters = NULL;
        sei.lpDirectory = NULL;
        sei.lpClass = NULL;
        sei.nShow = SW_SHOWNORMAL;
        sei.hInstApp = g_hinst;

        // ShellExecute(hwnd, NULL, szNewGrpPath, NULL, NULL, SW_SHOWNORMAL);
        ShellExecuteEx(&sei);
    }

    // Everything went OK.
    fStatus = TRUE;

ProcExit:
#ifdef UNICODE
    CloseHandle(fh);
#else
    _lclose(fh);
#endif
#ifndef WINNT
    // BUGBUG (reinerf)  - we only need to call Group_DeleteIfRequired
    // when we are on a Japanese language machine (win95J or win98J). We 
    // should have a runtime check for Japanese here.

    // Delete old group file when it is specified in special
    // registry entry. Bug#7259-win95d
    //
    if (fStatus == TRUE)
    {
        // delete it only if the conversion was successful.
        Group_DeleteIfRequired(szOldGrpTitle,lpszOldGrpPath);
    }
#endif // !WINNT
ProcExit2:
    ItemList_Destroy();
    return fStatus;
}

//---------------------------------------------------------------------------
// Reads an NT format Progman Group files and creates a directory containing
// links that matches the group file.
BOOL Group_CreateNewFromOldNT(HWND hwnd, LPCTSTR lpszOldGrpPath, UINT options)
{
    NT_GROUPDEF grpdef;
#ifdef UNICODE
    HANDLE fh;
    DWORD  dwBytesRead;
#else
    HFILE fh;
#endif
    TCHAR szNewGrpPath[MAX_PATH];
    WCHAR szOldGrpTitleUnicode[MAXGROUPNAMELEN + 1];
    TCHAR szOldGrpTitle[MAXGROUPNAMELEN + 1];
    // LPSTR lpszExt;
    BOOL fStatus = FALSE;
    SHELLEXECUTEINFO sei;
    BOOL fStartup = FALSE;

    if (!ItemList_Create(lpszOldGrpPath))
        return FALSE;


#ifdef UNICODE
    fh = CreateFile(
             lpszOldGrpPath,
             GENERIC_READ,
             FILE_SHARE_READ,
             NULL,
             OPEN_EXISTING,
             0,
             NULL
             );
    if (fh == INVALID_HANDLE_VALUE) {
#else
    fh = _lopen(lpszOldGrpPath, OF_READ | OF_SHARE_DENY_NONE);
    if (fh == HFILE_ERROR) {
#endif
        DebugMsg(DM_ERROR, TEXT("gc.gcnfont: Unable to open group."));
        goto ProcExit2;
    }

#ifdef UNICODE
    if (!ReadFile(fh, &grpdef, SIZEOF(grpdef), &dwBytesRead, NULL) ||
        dwBytesRead != SIZEOF(grpdef)) {
#else
    if (_lread(fh, &grpdef, SIZEOF(grpdef)) != SIZEOF(grpdef)) {
#endif
        DebugMsg(DM_ERROR, TEXT("gc.gcnfont: header too small."));
        goto ProcExit;
    }

    if (grpdef.cItems > 50) {
        // NB This isn;t fatal so carry on.
        DebugMsg(DM_ERROR, TEXT("gc.gcnfont: Too many items."));
    }

#ifdef UNICODE
    SetFilePointer(fh, grpdef.pName, NULL, FILE_BEGIN);
    ReadFile(fh, szOldGrpTitleUnicode, SIZEOF(szOldGrpTitleUnicode), &dwBytesRead, NULL);
#else
    _llseek(fh, grpdef.pName, 0);
    _lread(fh, szOldGrpTitleUnicode, SIZEOF(szOldGrpTitleUnicode));
#endif

#ifdef UNICODE
    lstrcpy(szOldGrpTitle, szOldGrpTitleUnicode);
#else
    WideCharToMultiByte (CP_ACP, 0, szOldGrpTitleUnicode, -1,
                         szOldGrpTitle, MAXGROUPNAMELEN+1, NULL, NULL);
#endif

    // Get the destination dir, use the title from the old group.
    // REVIEW UNDONE - until we get long filenames we'll use the old
    // groups' filename as the basis for the new group instead of it's
    // title.


    // Special case the startup group.
    if (StartupCmp(szOldGrpTitle)) {
        if (g_fDoingCommonGroups) {
            SHGetSpecialFolderPath(hwnd, szNewGrpPath, CSIDL_COMMON_STARTUP, TRUE);
        } else {
            SHGetSpecialFolderPath(hwnd, szNewGrpPath, CSIDL_STARTUP, TRUE);
        }
    } else {
        if (!Group_GenerateNewGroupPath(hwnd, szOldGrpTitle, szNewGrpPath, lpszOldGrpPath)) {
            DebugMsg(DM_ERROR, TEXT("gc.gcnfo; Unable to create destination directory."));
            goto ProcExit;
        }
    }

    // Go through every item in the old group and make it a link...
    if (!GetAllItemDataNT(fh, grpdef.cItems, grpdef.cbGroup, szOldGrpTitle, szNewGrpPath)) {
        if (options & GC_REPORTERROR)
            MyMessageBox(hwnd, IDS_APPTITLE, IDS_BADOLDGROUP, NULL, MB_OK | MB_ICONEXCLAMATION);
    }

    // Deal with the tags section.
    HandleTagsNT(fh, grpdef.cbGroup);

    // Now we've dealt with the tags we don't need to keep track of
    // busted items so delete them now. From here on we always have
    // valid items.
    DeleteBustedItems();

    // Shorten descs on non-lfn drives.
    if (!IsLFNDrive(szNewGrpPath))
        ShortenDescriptions();

    // Fixup the paths/WD stuff.
    MungePaths();

    // Fix dups.
    ResolveDuplicates(szNewGrpPath);

    // Do we just want a list of the apps or create some links?
    if (options & GC_BUILDLIST)
            AppList_Append();
    else
        CreateLinks(szNewGrpPath, fStartup, grpdef.cItems);

    // Get the cabinet to show the new group.
    if (options & GC_OPENGROUP)
    {
        sei.cbSize = SIZEOF(sei);
        sei.fMask = 0;
        sei.hwnd = hwnd;
        sei.lpVerb = NULL;
        sei.lpFile = szNewGrpPath;
        sei.lpParameters = NULL;
        sei.lpDirectory = NULL;
        sei.lpClass = NULL;
        sei.nShow = SW_SHOWNORMAL;
        sei.hInstApp = g_hinst;

        // ShellExecute(hwnd, NULL, szNewGrpPath, NULL, NULL, SW_SHOWNORMAL);
        ShellExecuteEx(&sei);
    }

    // Everything went OK.
    fStatus = TRUE;

ProcExit:
#ifdef UNICODE
    CloseHandle(fh);
#else
    _lclose(fh);
#endif
ProcExit2:
    ItemList_Destroy();
    return fStatus;
}

//---------------------------------------------------------------------------
// Record the last write date/time of the given group in the ini file.
void Group_WriteLastModDateTime(LPCTSTR lpszGroupFile,DWORD dwLowDateTime)
{
    Reg_SetStruct(g_hkeyGrpConv, c_szGroups, lpszGroupFile, &dwLowDateTime, SIZEOF(dwLowDateTime));
}

//---------------------------------------------------------------------------
// Read the last write date/time of the given group from the ini file.
DWORD Group_ReadLastModDateTime(LPCTSTR lpszGroupFile)
{
    DWORD dwDateTime = 0;

    Reg_GetStruct(g_hkeyGrpConv, c_szGroups, lpszGroupFile, &dwDateTime, SIZEOF(dwDateTime));

    return dwDateTime;
}

//---------------------------------------------------------------------------
// Convert the given group to the new format.
// Returns FALSE if something goes wrong.
// Returns true if the given group got converted or the user cancelled.
BOOL Group_Convert(HWND hwnd, LPCTSTR lpszOldGrpFile, UINT options)
    {
    TCHAR szGroupTitle[MAXGROUPNAMELEN + 1];          // PM Groups had a max title len of 30.
    BOOL fStatus;
    WIN32_FIND_DATA fd;
    HANDLE hff;
    UINT    nCode;
    UINT    iErrorId;


    Log(TEXT("Grp: %s"), lpszOldGrpFile);

    DebugMsg(DM_TRACE, TEXT("gc.gc: Converting group %s"), (LPTSTR) lpszOldGrpFile);

    // Does the group exist?
    if (PathFileExists(lpszOldGrpFile))
        {
        // Group exists - is it valid?

        nCode = Group_ValidOldFormat(lpszOldGrpFile, szGroupTitle);
        switch( nCode )
            {
            case VOF_WINNT:
            case VOF_WIN31:
                // Yes - ask for confirmation.
                if (!(options & GC_PROMPTBEFORECONVERT) ||
                    MyMessageBox(hwnd, IDS_APPTITLE, IDS_OKTOCONVERT, szGroupTitle, MB_YESNO) == IDYES)
                    {
                    // Everything went OK?
                    if ( nCode == VOF_WIN31 )
                        {
                        fStatus = Group_CreateNewFromOld(hwnd,lpszOldGrpFile,
                                                                      options);
                        }
                    else
                        {
                        fStatus = Group_CreateNewFromOldNT(hwnd,lpszOldGrpFile,
                                                                      options);
                        }
                    if ( fStatus )
                        {
                        iErrorId = 0;
                        }
                    else
                        {
                        // Nope - FU. Warn and exit.
                        iErrorId = IDS_CONVERTERROR;
                        }
                    }
                else
                    {
                    // User cancelled...
                    iErrorId = 0;
                    }
                break;

            default:
            case VOF_BAD:
                {
                // Nope, File is invalid.
                // Warn user.
                iErrorId = IDS_NOTGROUPFILE;
                }
                break;
            }
        }
    else
        {
        // Nope, File doesn't even exist.
        iErrorId = IDS_MISSINGFILE;
        }

    if ( iErrorId != 0 )
        {
        if (options & GC_REPORTERROR)
            {
            MyMessageBox(hwnd, IDS_APPTITLE, iErrorId,
                         lpszOldGrpFile, MB_OK|MB_ICONEXCLAMATION);
            }

        Log(TEXT("Grp: %s done."), lpszOldGrpFile);

        return FALSE;
        }
    else
        {
        DebugMsg(DM_TRACE, TEXT("gc.gc: Done."));

        Log(TEXT("Grp: %s done."), lpszOldGrpFile);

        return TRUE;
        }
    }

//---------------------------------------------------------------------------
// Checks the date/time stamp of the given group against the one in
// grpconv.ini
BOOL GroupHasBeenModified(LPCTSTR lpszGroupFile)
{
        WIN32_FIND_DATA fd;
        HANDLE hff;
        BOOL fModified;

        hff = FindFirstFile(lpszGroupFile, &fd);
        if (hff != INVALID_HANDLE_VALUE)
        {
                if (Group_ReadLastModDateTime(lpszGroupFile) != fd.ftLastWriteTime.dwLowDateTime)
                {
                        DebugMsg(DM_TRACE, TEXT("cg.ghbm: Group %s has been modified."), (LPTSTR)lpszGroupFile);
                        fModified = TRUE;
                }
                else
                {
                        DebugMsg(DM_TRACE, TEXT("cg.ghbm: Group %s has not been modified."), (LPTSTR)lpszGroupFile);
                        fModified = FALSE;
                }
                FindClose(hff);
                return fModified;
        }
        else
        {
                // Hmm, file doesn't exist, pretend it's up to date.
                return TRUE;
        }
}

//---------------------------------------------------------------------------
// Converts a group file from its NT registry into a real file on disk. Since
// the disk format for NT 1.0 files never existed and collided in its usage
// the GROUP_MAGIC file type, we will convert it from the registry, directly
// into a GROUP_UNICODE format file.  In this way we will always be able to
// distiguish the NT group files from the Win 3.1 group files.

BOOL MakeGroupFile( LPTSTR lpFileName, LPTSTR lpGroupName)
{
    LONG    lResult;
    DWORD   cbSize;
    HGLOBAL hBuffer;
    HGLOBAL hNew;
    LPBYTE  lpBuffer;
    BOOL    fOk;
    HANDLE  hFile;
    HKEY    hkey;
    DWORD   cbWrote;

    fOk = FALSE;

    lResult = RegOpenKeyEx(hkeyGroups, lpGroupName, 0,
                            KEY_READ, &hkey );
    if ( lResult != ERROR_SUCCESS )
    {
        return FALSE;
    }

    lResult = RegQueryValueEx( hkey, NULL, NULL, NULL, NULL, &cbSize);
    if ( lResult != ERROR_SUCCESS )
    {
        goto CleanupKey;
    }

    hBuffer = GlobalAlloc(GMEM_MOVEABLE,cbSize);
    if ( hBuffer == NULL )
    {
        goto CleanupKey;
    }
    lpBuffer = (LPBYTE)GlobalLock(hBuffer);
    if ( lpBuffer == NULL )
    {
        goto CleanupMem;
    }

    lResult = RegQueryValueEx( hkey, NULL, NULL, NULL,
                             lpBuffer, &cbSize );

    if ( lResult != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    if ( *(DWORD *)lpBuffer == GROUP_MAGIC )
    {
        HGLOBAL hNew;

        cbSize = ConvertToUnicodeGroup( (LPNT_GROUPDEF_A)lpBuffer, &hNew );

        GlobalUnlock( hBuffer );
        GlobalFree( hBuffer );
        hBuffer = hNew;
        lpBuffer = GlobalLock( hBuffer );
        if ( lpBuffer == NULL )
        {
            goto CleanupMem;
        }
    }

    hFile = CreateFile(lpFileName,GENERIC_WRITE,0,NULL,
                       CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        fOk = WriteFile(hFile,lpBuffer,cbSize,&cbWrote,NULL);

        CloseHandle(hFile);
    }

Cleanup:
    GlobalUnlock(hBuffer);

CleanupMem:
    GlobalFree(hBuffer);

CleanupKey:
    RegCloseKey( hkey );
    return fOk;
}

#define BIG_STEP 1024

//----------------------------------------------------------------------------
// Enumerate all the groups or just all the modified groups.
int Group_Enum(PFNGRPCALLBACK pfncb, BOOL fProgress,
    BOOL fModifiedOnly)
{
    TCHAR szIniFile[MAX_PATH], szFile[MAX_PATH];
    FILETIME  ft;
    UINT uSize;
    LPTSTR pSection, pKey;
    int cGroups = 0;
    HANDLE hFile;
    WIN32_FIND_DATA fd;

    if (!FindProgmanIni(szIniFile))
        return 0;

    for (uSize = BIG_STEP; uSize < BIG_STEP * 8; uSize += BIG_STEP)
    {
        pSection = (LPTSTR)LocalAlloc(LPTR, uSize);
        if (!pSection)
            return 0;
        if ((UINT)GetPrivateProfileString(c_szGroups, NULL, c_szNULL, pSection, uSize, szIniFile) < uSize - 5)
            break;
        LocalFree((HLOCAL)pSection);
        pSection = NULL;
    }

    if (!pSection)
        return 0;

    if (fProgress)
        Group_CreateProgressDlg();

    for (pKey = pSection; *pKey; pKey += lstrlen(pKey) + 1)
    {
        GetPrivateProfileString(c_szGroups, pKey, c_szNULL, szFile, ARRAYSIZE(szFile), szIniFile);
        if (szFile[0])
        {
            if (!fModifiedOnly || GroupHasBeenModified(szFile))
            {
                (*pfncb)(szFile);
                cGroups++;
                hFile = FindFirstFile (szFile, &fd);

                if (hFile != INVALID_HANDLE_VALUE) {
                    FindClose (hFile);
                    Group_WriteLastModDateTime(szFile, fd.ftLastWriteTime.dwLowDateTime);
                }
            }
        }
    }

    // Cabinet uses the date/time of progman.ini as a hint to speed things up
    // so set it here so we won't run automatically again.
    GetSystemTimeAsFileTime(&ft);
    Group_WriteLastModDateTime(szIniFile,ft.dwLowDateTime);

    LocalFree((HLOCAL)pSection);

    if (fProgress)
        Group_DestroyProgressDlg();

    return cGroups;
}



//----------------------------------------------------------------------------
// Enumerate all the NT groups or just all the modified groups.
int Group_EnumNT(PFNGRPCALLBACK pfncb, BOOL fProgress,
    BOOL fModifiedOnly, HKEY hKeyRoot, LPCTSTR lpKey)
{
    LONG      lResult;
    DWORD     dwSubKey = 0;
    TCHAR     szGroupName[MAXGROUPNAMELEN+1];
    TCHAR     szFileName[MAX_PATH];
    TCHAR     szTempFileDir[MAX_PATH];
    TCHAR     szTempFileName[MAX_PATH];
    DWORD     cchGroupNameLen;
    FILETIME  ft;
    BOOL      fOk;
    BOOL      fDialog = FALSE;
    BOOL      fProcess;
    int       cGroups = 0;


    //
    // Look for groups in the registry
    //

    lResult = RegOpenKeyEx(hKeyRoot, lpKey, 0,
                            KEY_READ, &hkeyGroups );
    if ( lResult != ERROR_SUCCESS )
    {
        return 0;
    }


    while ( TRUE )
    {
        cchGroupNameLen = ARRAYSIZE(szGroupName);
        lResult = RegEnumKeyEx( hkeyGroups, dwSubKey, szGroupName,
                                &cchGroupNameLen, NULL, NULL, NULL, &ft );
        szGroupName[MAXGROUPNAMELEN] = TEXT('\0');

        if ( lResult == ERROR_NO_MORE_ITEMS )
        {
            break;
        }
        if ( lResult == ERROR_SUCCESS )
        {
            GetWindowsDirectory(szFileName, ARRAYSIZE(szFileName));

            // Save this dir for use by GetTempFileName below
            lstrcpy(szTempFileDir, szFileName);

#ifdef WINNT
            GetEnvironmentVariable(TEXT("USERPROFILE"), szTempFileDir, MAX_PATH);
#endif
            lstrcat(szFileName,TEXT("\\"));
            lstrcat(szFileName,szGroupName);
            lstrcat(szFileName,TEXT(".grp"));

            //
            // If the key has been modified since we last processed it,
            // then time to process it again.
            //
            fProcess = FALSE;
            if (fModifiedOnly)
            {
                if ( Group_ReadLastModDateTime(szFileName) != ft.dwLowDateTime )
                {
                    fProcess = TRUE;
                }
            }
            else
            {
                fProcess = TRUE;
            }

            if (fProcess)
            {
                if (GetTempFileName(szTempFileDir,TEXT("grp"),0,szTempFileName) != 0)
                {
                    fOk = MakeGroupFile(szTempFileName,szGroupName);
                    if ( fOk )
                    {
                        if (fProgress && !fDialog)
                        {
                            Group_CreateProgressDlg();
                            fDialog = TRUE;
                        }
                        (*pfncb)(szTempFileName);
                        DeleteFile(szTempFileName);
                        Group_WriteLastModDateTime(szFileName,ft.dwLowDateTime);
                        cGroups++;
                    }
                }
            }
        }
        dwSubKey++;
    }

    RegCloseKey( hkeyGroups );
    hkeyGroups = NULL;

    if (fProgress && fDialog)
        Group_DestroyProgressDlg();

    return cGroups;
}







//---------------------------------------------------------------------------
// Find the progman ini from before an upgrade.
BOOL FindOldProgmanIni(LPTSTR pszPath)
{
    if (Reg_GetString(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, REGSTR_VAL_OLDWINDIR, pszPath, MAX_PATH*SIZEOF(TCHAR)))
    {
        PathAppend(pszPath, c_szProgmanIni);

        if (PathFileExists(pszPath))
        {
            return TRUE;
        }
        DebugMsg(DM_ERROR, TEXT("Can't find old progman.ini"));
        return FALSE;
    }
    return FALSE;
}

//----------------------------------------------------------------------------
// Enumerate all the old groups.
void Group_EnumOldGroups(PFNGRPCALLBACK pfncb, BOOL fProgress)
{
    TCHAR szIniFile[MAX_PATH], szFile[MAX_PATH];
    UINT uSize;
    LPTSTR pSection, pKey;

    if (!FindOldProgmanIni(szIniFile))
        return;

    for (uSize = BIG_STEP; uSize < BIG_STEP * 8; uSize += BIG_STEP)
    {
        pSection = (LPTSTR)LocalAlloc(LPTR, uSize);
        if (!pSection)
            return;
        if ((UINT)GetPrivateProfileString(c_szGroups, NULL, c_szNULL, pSection, uSize, szIniFile) < uSize - 5)
            break;
        LocalFree((HLOCAL)pSection);
        pSection = NULL;
    }

    if (!pSection)
        return;

    if (fProgress)
        Group_CreateProgressDlg();

    for (pKey = pSection; *pKey; pKey += lstrlen(pKey) + 1)
    {
        GetPrivateProfileString(c_szGroups, pKey, c_szNULL, szFile, ARRAYSIZE(szFile), szIniFile);
        if (szFile[0])
        {
            (*pfncb)(szFile);
        }
    }

    if (fProgress)
        Group_DestroyProgressDlg();

    LocalFree((HLOCAL)pSection);
}

//----------------------------------------------------------------------------
// Given a pidl for a link, extract the appropriate info and append it to
// the app list.
void AppList_AppendCurrentItem(LPITEMIDLIST pidlFolder, LPSHELLFOLDER psf,
    LPITEMIDLIST pidlItem, IShellLink *psl, IPersistFile *ppf)
{
    STRRET str;
    WCHAR wszPath[MAX_PATH];
    TCHAR szName[MAX_PATH];
    TCHAR sz[MAX_PATH];
    TCHAR szPath[MAX_PATH];
    TCHAR szModule[MAX_PATH];
    TCHAR szVer[MAX_PATH];
    ALITEM alitem;

    if (SUCCEEDED(psf->lpVtbl->GetDisplayNameOf(psf, pidlItem, SHGDN_NORMAL, &str)))
    {
        // Get the name.
        StrRetToStrN(szName, ARRAYSIZE(szName), &str, pidlItem);
        DebugMsg(DM_TRACE, TEXT("c.gi_gi: Link %s"), szName);

        // Get the path from the link...
        SHGetPathFromIDList(pidlFolder, sz);
        PathAppend(sz, szName);
        lstrcat(sz, TEXT(".lnk"));
        StrToOleStrN(wszPath, ARRAYSIZE(wszPath), sz, -1);
        ppf->lpVtbl->Load(ppf, wszPath, 0);
        // Copy all the data.
        szPath[0] = TEXT('\0');
        if (SUCCEEDED(psl->lpVtbl->GetPath(psl, szPath, ARRAYSIZE(szPath), NULL, SLGP_SHORTPATH)))
        {
            // Valid CL?
            if (szPath[0])
            {
                GetVersionInfo(szPath, szModule, ARRAYSIZE(szModule), szVer, sizeof(szVer));

                alitem.pszName = NULL;
                alitem.pszPath = NULL;
                alitem.pszModule = NULL;
                alitem.pszVer = NULL;

                Str_SetPtr(&alitem.pszName, szName);
                Str_SetPtr(&alitem.pszPath, szPath);
                Str_SetPtr(&alitem.pszModule, szModule);
                Str_SetPtr(&alitem.pszVer, szVer);
                DSA_AppendItem(g_hdsaAppList, &alitem);
            }
        }
    }
}

//----------------------------------------------------------------------------
HRESULT AppList_ShellFolderEnum(LPITEMIDLIST pidlFolder, LPSHELLFOLDER psf)
{
    HRESULT hres;
    LPENUMIDLIST penum;
    IShellLink *psl;
    LPITEMIDLIST pidlItem;
    UINT celt;
    IPersistFile *ppf;
    DWORD dwAttribs;
    LPSHELLFOLDER psfItem;
    LPITEMIDLIST pidlPath;

    DebugMsg(DM_TRACE, TEXT("gc.al_sfe: Enum..."));

    hres = psf->lpVtbl->EnumObjects(psf, (HWND)NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &penum);
    if (SUCCEEDED(hres))
    {
        hres = ICoCreateInstance(&CLSID_ShellLink, &IID_IShellLink, &psl);
        if (SUCCEEDED(hres))
        {
            psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf);
            while ((penum->lpVtbl->Next(penum, 1, &pidlItem, &celt) == NOERROR) && (celt == 1))
            {
                dwAttribs = SFGAO_LINK|SFGAO_FOLDER;
                if (SUCCEEDED(psf->lpVtbl->GetAttributesOf(psf, 1, &pidlItem, &dwAttribs)))
                {
                    // Is it a folder
                    if (dwAttribs & SFGAO_FOLDER)
                    {
                        // Recurse.
                        DebugMsg(DM_TRACE, TEXT("al_sfe: Folder."));
                        hres = psf->lpVtbl->BindToObject(psf, pidlItem, NULL, &IID_IShellFolder, &psfItem);
                        if (SUCCEEDED(hres))
                        {
                            pidlPath = ILCombine(pidlFolder, pidlItem);
                            if (pidlPath)
                            {
                                AppList_ShellFolderEnum(pidlPath, psfItem);
                                psfItem->lpVtbl->Release(psfItem);
                                ILFree(pidlPath);
                            }
                        }
                    }
                    else if (dwAttribs & SFGAO_LINK)
                    {
                        // Regular link, add it to the list.
                        DebugMsg(DM_TRACE, TEXT("al_sfe: Link."));
                        AppList_AppendCurrentItem(pidlFolder, psf, pidlItem, psl, ppf);
                    }
                }
                SHFree(pidlItem);
            }
            ppf->lpVtbl->Release(ppf);
            psl->lpVtbl->Release(psl);
        }
        penum->lpVtbl->Release(penum);
    }
    return hres;
}

//----------------------------------------------------------------------------
void Applist_SpecialFolderEnum(int nFolder)
{
    HRESULT hres;
    LPITEMIDLIST pidlGroup;
    LPSHELLFOLDER psf, psfDesktop;
    TCHAR sz[MAX_PATH];

    // Get the group info.
    if (SHGetSpecialFolderPath(NULL, sz, nFolder, FALSE))
    {
        pidlGroup = ILCreateFromPath(sz);
        if (pidlGroup)
            {
            if (SUCCEEDED(ICoCreateInstance(&CLSID_ShellDesktop, &IID_IShellFolder, &psfDesktop)))
            {
                hres = psfDesktop->lpVtbl->BindToObject(psfDesktop, pidlGroup, NULL, &IID_IShellFolder, &psf);
                if (SUCCEEDED(hres))
                {
                    hres = AppList_ShellFolderEnum(pidlGroup, psf);
                    psf->lpVtbl->Release(psf);
                }
                psfDesktop->lpVtbl->Release(psfDesktop);
            }
            else
            {
                DebugMsg(DM_ERROR, TEXT("OneTree: failed to bind to Desktop root"));
            }
            ILFree(pidlGroup);
            }
        else
        {
                DebugMsg(DM_ERROR, TEXT("gc.al_acs: Can't create IDList for path.."));
        }
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("gc.al_acs: Can't find programs folder."));
    }
}

BOOL StartMenuIsProgramsParent(void)
{
    LPITEMIDLIST pidlStart, pidlProgs;
    BOOL fParent = FALSE;

    if (SHGetSpecialFolderLocation(NULL, CSIDL_STARTMENU, &pidlStart))
    {
        if (SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidlProgs))
        {
            if (ILIsParent(pidlStart, pidlProgs, FALSE))
                fParent = TRUE;
            ILFree(pidlProgs);
        }
        ILFree(pidlStart);
    }

    return fParent;
}

//---------------------------------------------------------------------------
// Return the links in a group.
void AppList_AddCurrentStuff(void)
{

    DebugMsg(DM_TRACE, TEXT("gc.al_acs: Enumerating everything..."));

    DebugMsg(DM_TRACE, TEXT("gc.al_acs: Enumerating StartMenu..."));
    Applist_SpecialFolderEnum(CSIDL_STARTMENU);
    if (!StartMenuIsProgramsParent())
    {
        DebugMsg(DM_TRACE, TEXT("gc.al_acs: Enumerating Programs..."));
        Applist_SpecialFolderEnum(CSIDL_PROGRAMS);
    }
}

// On NT we plan on converting NT formated group files into folders and links
// therefore we need the ability of supporting all of the NT group file formats

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SIZEOFGroup() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/
DWORD SizeofGroup(LPNT_GROUPDEF lpgd)
{
    LPNT_PMTAG lptag;
    DWORD cbSeg;
    DWORD cb;

    cbSeg = (DWORD)GlobalSize(lpgd);

    // BUGBUG - The following needs to be verified
    lptag = (LPNT_PMTAG)((LPSTR)lpgd+lpgd->cbGroup);

    if ((DWORD)((PCHAR)lptag - (PCHAR)lpgd +MyDwordAlign(SIZEOF(NT_PMTAG))-MyDwordAlign(SIZEOF(lptag->rgb))+4) <= cbSeg
        && lptag->wID == ID_MAGIC
        && lptag->wItem == (int)0xFFFF
        && lptag->cb == (WORD)(MyDwordAlign(SIZEOF(NT_PMTAG))-MyDwordAlign(SIZEOF(lptag->rgb)) + 4)
        && *(PLONG)lptag->rgb == TAG_MAGIC)
      {
        while ((cb = (DWORD)((PCHAR)lptag - (PCHAR)lpgd + MyDwordAlign(SIZEOF(NT_PMTAG))-MyDwordAlign(SIZEOF(lptag->rgb)))) <= cbSeg)
          {
            if (lptag->wID == ID_LASTTAG)
                return cb;
            (LPSTR)lptag += lptag->cb;
          }
      }
    return lpgd->cbGroup;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FindTag() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LPNT_PMTAG FindTag(LPNT_GROUPDEF lpgd, int item, WORD id)
{
    LPNT_PMTAG lptag;
    DWORD cbSeg;
    DWORD cb;

    cbSeg = (DWORD)GlobalSize(lpgd);

    lptag = (LPNT_PMTAG)((LPSTR)lpgd+lpgd->cbGroup);

    if ((PCHAR)lptag - (PCHAR)lpgd + MyDwordAlign(SIZEOF(NT_PMTAG))-MyDwordAlign(SIZEOF(lptag->rgb)) + 4 <= cbSeg
        && lptag->wID == ID_MAGIC
        && lptag->wItem == (int)0xFFFF
        && lptag->cb == (WORD)(MyDwordAlign(SIZEOF(NT_PMTAG))-MyDwordAlign(SIZEOF(lptag->rgb)) +4)
        && *(LONG *)lptag->rgb == TAG_MAGIC) {

        while ((cb = (DWORD)((PCHAR)lptag - (PCHAR)lpgd + MyDwordAlign(SIZEOF(NT_PMTAG))-MyDwordAlign(SIZEOF(lptag->rgb)))) <= cbSeg)
        {
            if ((item == lptag->wItem)
                && (id == 0 || id == lptag->wID)) {
                return lptag;
            }

            if (lptag->wID == ID_LASTTAG)
                return NULL;

            (LPSTR)lptag += lptag->cb;
        }
    }
    return NULL;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DeleteTag() -                                                           */
/*                                                                          */
/* in:                                                                      */
/*      hGroup  group handle, can be discardable (alwayws shrink object)        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID DeleteTag(HANDLE hGroup, int item, WORD id)
{
    LPNT_PMTAG lptag;
    LPWSTR lp1, lp2;
    LPWSTR lpend;
    LPNT_GROUPDEF lpgd;

    lpgd = (LPNT_GROUPDEF) GlobalLock(hGroup);

    lptag = FindTag(lpgd,item,id);

    if (lptag == NULL) {
        GlobalUnlock(hGroup);
        return;
    }

    lp1 = (LPWSTR)lptag;

    lp2 = (LPWSTR)((LPSTR)lptag + lptag->cb);

    lpend = (LPWSTR)((LPSTR)lpgd + SizeofGroup(lpgd));

    while (lp2 < lpend) {
        *lp1++ = *lp2++;
    }

    /* always reallocing smaller
     */
    GlobalUnlock(hGroup);
    GlobalReAlloc(hGroup, (DWORD)((LPSTR)lp1 - (LPSTR)lpgd), 0);

    return;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddTag() -                                                              */
/*                                                                          */
/* in:                                                                      */
/*      h       group handle, must not be discardable!                              */
/*                                                                          */
/* returns:                                                                 */
/*  0   failure                                                             */
/*      1       success                                                             */
/*--------------------------------------------------------------------------*/
INT AddTag(HANDLE h, int item, WORD id, LPWSTR lpbuf, UINT cb)
{
    LPNT_PMTAG lptag;
    WORD fAddFirst;
    LPNT_GROUPDEF lpgd;
    int cbNew;
    int cbMyLen;
    LPNT_GROUPDEF lpgdOld;


    if (!cb && lpbuf) {
        cb = SIZEOF(WCHAR)*(lstrlenW(lpbuf) + 1);
    }
    cbMyLen = MyDwordAlign(cb);

    if (!lpbuf) {
        cb = 0;
        cbMyLen = 0;
    }

    /*
     * Remove the old version of the tag, if any.
     */
    DeleteTag(h, item, id);

    lpgd = (LPNT_GROUPDEF)GlobalLock(h);

    lptag = FindTag(lpgd, (int)0xFFFF, (WORD)ID_LASTTAG);

    if (!lptag) {
        /*
         * In this case, there are no tags at all, and we have to add
         * the first tag, the interesting tag, and the last tag
         */
        cbNew = 3 * (MyDwordAlign(SIZEOF(NT_PMTAG)) - MyDwordAlign(SIZEOF(lptag->rgb))) + 4 + cbMyLen;
        fAddFirst = TRUE;
        lptag = (LPNT_PMTAG)((LPSTR)lpgd + lpgd->cbGroup);

    } else {
        /*
         * In this case, only the interesting tag needs to be added
         * but we count in the last because the delta is from lptag
         */
        cbNew = 2 * (MyDwordAlign(SIZEOF(NT_PMTAG)) - MyDwordAlign(SIZEOF(lptag->rgb))) + cbMyLen;
        fAddFirst = FALSE;
    }

    /*
     * check for 64K limit
     */
    if ((DWORD_PTR)lptag + cbNew < (DWORD_PTR)lptag) {
        return 0;
    }

    cbNew += (DWORD)((PCHAR)lptag -(PCHAR)lpgd);
    lpgdOld = lpgd;
    GlobalUnlock(h);
    if (!GlobalReAlloc(h, (DWORD)cbNew, GMEM_MOVEABLE)) {
        return 0;
    }

    lpgd = (LPNT_GROUPDEF)GlobalLock(h);
    lptag = (LPNT_PMTAG)((LPSTR)lpgd + ((LPSTR)lptag - (LPSTR)lpgdOld));
    if (fAddFirst) {
        /*
         * Add the first tag
         */
        lptag->wID = ID_MAGIC;
        lptag->wItem = (int)0xFFFF;
        *(LONG *)lptag->rgb = TAG_MAGIC;
        lptag->cb = (WORD)(MyDwordAlign(SIZEOF(NT_PMTAG)) - MyDwordAlign(SIZEOF(lptag->rgb)) + 4);
        (LPSTR)lptag += lptag->cb;
    }

    /*
     * Add the tag
     */
    lptag->wID = id;
    lptag->wItem = item;
    lptag->cb = (WORD)(MyDwordAlign(SIZEOF(NT_PMTAG)) - MyDwordAlign(SIZEOF(lptag->rgb)) + cbMyLen);
    if (lpbuf) {
        memmove(lptag->rgb, lpbuf, (WORD)cb);
    }
    (LPSTR)lptag += lptag->cb;

    /*
     * Add the end tag
     */
    lptag->wID = ID_LASTTAG;
    lptag->wItem = (int)0xFFFF;
    lptag->cb = 0;

    GlobalUnlock(h);

    return 1;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CreateNewGroupFromAnsiGroup() -                                                      */
/*                                                                          */
/*  This function creates a new, empty group.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

HANDLE CreateNewGroupFromAnsiGroup(LPNT_GROUPDEF_A lpGroupORI)
{
    HANDLE      hT;
    LPNT_GROUPDEF lpgd;
    int         i;
    int         cb;
    int         cItems;          // number of items in 16bit group
    LPSTR       pGroupName;      // 32bit group name
    LPWSTR      pGroupNameUNI = NULL;   // 32bit UNICODE group name
    UINT        wGroupNameLen;   // length of pGroupName DWORD aligned.
    INT         cchWideChar = 0; //character count of resultant unicode string
    INT         cchMultiByte = 0;

    pGroupName = (LPSTR)PTR(lpGroupORI, lpGroupORI->pName);

    //
    // convert pGroupName to unicode here
    //
    cchMultiByte=MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,pGroupName,
            -1,pGroupNameUNI,cchWideChar) ;

    pGroupNameUNI = LocalAlloc(LPTR,(++cchMultiByte)*SIZEOF(WCHAR)) ;

    MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,pGroupName,
            -1,pGroupNameUNI,cchMultiByte) ;


    wGroupNameLen = MyDwordAlign(SIZEOF(WCHAR)*(lstrlenW(pGroupNameUNI) + 1));
    cItems = lpGroupORI->cItems;
    cb = SIZEOF(NT_GROUPDEF) + (cItems * SIZEOF(DWORD)) +  wGroupNameLen;

    //
    // In CreateNewGroup before GlobalAlloc.
    //
    hT = GlobalAlloc(GHND, (DWORD)cb);
    if (!hT) {
        goto Exit;
    }

    lpgd = (LPNT_GROUPDEF)GlobalLock(hT);

    //
    // use the NT 1.0 group settings for what we can.
    //
    lpgd->nCmdShow = lpGroupORI->nCmdShow;
    lpgd->wIconFormat = lpGroupORI->wIconFormat;
    lpgd->cxIcon = lpGroupORI->cxIcon;
    lpgd->cyIcon = lpGroupORI->cyIcon;
    lpgd->ptMin.x = (INT)lpGroupORI->ptMin.x;
    lpgd->ptMin.y = (INT)lpGroupORI->ptMin.y;
    CopyRect(&(lpgd->rcNormal),&(lpGroupORI->rcNormal));


    lpgd->dwMagic = GROUP_UNICODE;
    lpgd->cbGroup = (DWORD)cb;
    lpgd->pName = SIZEOF(NT_GROUPDEF) + cItems * SIZEOF(DWORD);

    lpgd->Reserved1 = (WORD)-1;
    lpgd->Reserved2 = (DWORD)-1;

    lpgd->cItems = (WORD)cItems;

    for (i = 0; i < cItems; i++) {
        lpgd->rgiItems[i] = 0;
    }

    lstrcpyW((LPWSTR)((LPBYTE)lpgd + SIZEOF(NT_GROUPDEF) + cItems * SIZEOF(DWORD)),
            pGroupNameUNI); // lhb tracks
    LocalFree((HLOCAL)pGroupNameUNI);

    GlobalUnlock(hT);
    return(hT);

Exit:
    return NULL;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddThing() -                                                            */
/*                                                                          */
/* in:                                                                      */
/*      hGroup  group handle, must not be discardable                       */
/*      lpStuff pointer to data or NULL to init data to zero                */
/*      cbStuff count of item (may be 0) if lpStuff is a string             */
/*                                                                          */
/* Adds an object to the group segment and returns its offset.  Will        */
/* reallocate the segment if necessary.                                     */
/*                                                                          */
/* Handle passed in must not be discardable                                 */
/*                                                                          */
/* returns:                                                                 */
/*      0       failure                                                     */
/*      > 0     offset to thing in the segment                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

DWORD AddThing(HANDLE hGroup, LPWSTR lpStuff, DWORD cbStuff)
{
    DWORD        cb;
    LPNT_GROUPDEF lpgd;
    DWORD        offset;
    LPWSTR       lpT;
    DWORD        cbStuffSize;
    DWORD        cbGroupSize;
    DWORD        myOffset;

    if (cbStuff == 0xFFFFFFFF) {
        return 0xFFFFFFFF;
    }

    if (!cbStuff) {
        cbStuff = SIZEOF(WCHAR)*(DWORD)(1 + lstrlenW(lpStuff));
    }

    cbStuffSize = MyDwordAlign((int)cbStuff);

    lpgd = (LPNT_GROUPDEF)GlobalLock(hGroup);
    cb = SizeofGroup(lpgd);
    cbGroupSize = MyDwordAlign((int)cb);

    offset = lpgd->cbGroup;
    myOffset = (DWORD)MyDwordAlign((int)offset);

    GlobalUnlock(hGroup);

    if (!GlobalReAlloc(hGroup,(DWORD)(cbGroupSize + cbStuffSize), GMEM_MOVEABLE))
        return 0;

    lpgd = (LPNT_GROUPDEF)GlobalLock(hGroup);

    /*
     * Slide the tags up
     */
    memmove((LPSTR)lpgd + myOffset + cbStuffSize, (LPSTR)lpgd + myOffset,
                            (cbGroupSize - myOffset));
    lpgd->cbGroup += cbStuffSize;

    lpT = (LPWSTR)((LPSTR)lpgd + myOffset);
    if (lpStuff) {
        memcpy(lpT, lpStuff, cbStuff);

    } else {
        /*
         * Zero it
         */
        while (cbStuffSize--) {
            *((LPBYTE)lpT)++ = 0;
        }
    }


    GlobalUnlock(hGroup);

    return myOffset;
}

DWORD AddThing_A(HANDLE hGroup, LPSTR lpStuff, WORD cbStuff)
{
    LPWSTR      lpStuffUNI = NULL;
    BOOL        bAlloc = FALSE;
    DWORD cb;

    if (cbStuff == 0xFFFF) {
        return 0xFFFF;
    }

    if (!cbStuff) {
            INT cchMultiByte;
            INT cchWideChar = 0;

        bAlloc = TRUE;
        cchMultiByte=MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,lpStuff,
            -1,lpStuffUNI,cchWideChar) ;

        lpStuffUNI = LocalAlloc(LPTR,(++cchMultiByte)*SIZEOF(WCHAR)) ;

        MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,lpStuff,
            -1,lpStuffUNI,cchMultiByte) ;

        cbStuff = (WORD)SIZEOF(WCHAR)*(1 + lstrlenW(lpStuffUNI)); // lhb tracks
    } else {
        lpStuffUNI = (LPWSTR)lpStuff;
    }

    cb = AddThing(hGroup, lpStuffUNI, cbStuff);

    if (bAlloc)
        LocalFree(lpStuffUNI);

    return(cb);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ConvertToUnicodeGroup() -                                               */
/*                                                                          */
/*  returns the size of the new unicode group.                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

int ConvertToUnicodeGroup(LPNT_GROUPDEF_A lpGroupORI, LPHANDLE lphNewGroup)
{
    HANDLE hNewGroup;
    LPNT_GROUPDEF lpgd;
    LPNT_ITEMDEF lpid;
    LPBYTE lpid_A;
    LPNT_PMTAG lptag_A;
    LPSTR lpTagValue;
    WORD wTagId;
    LPSTR lpT;
    DWORD offset;
    int cb;
    int i;
    INT cchMultiByte;
    INT cchWideChar;
    LPWSTR lpTagValueUNI;
    BOOL bAlloc = FALSE;

    hNewGroup = CreateNewGroupFromAnsiGroup(lpGroupORI);
    if (!hNewGroup) {
        return(0);
    }

    //
    // Add all items to the new formatted group.
    //
    for (i = 0; i < (int)lpGroupORI->cItems; i++) {

      //
      // Get the pointer to the 16bit item
      //
      lpid_A = (LPBYTE)ITEM(lpGroupORI, i);
      if (lpGroupORI->rgiItems[i]) {

        //
        // Create the item.
        //
        offset = AddThing(hNewGroup, NULL, SIZEOF(NT_ITEMDEF));
        if (!offset) {
            DebugMsg(DM_ERROR, TEXT("gc.ctug: AddThing NT_ITEMDEF failed"));
            goto QuitThis;
        }

        lpgd = (LPNT_GROUPDEF)GlobalLock(hNewGroup);

        lpgd->rgiItems[i] = offset;
        lpid = ITEM(lpgd, i);

        //
        // Set the item's position.
        //
        lpid->pt.x = ((LPNT_ITEMDEF_A)lpid_A)->pt.x;
        lpid->pt.y = ((LPNT_ITEMDEF_A)lpid_A)->pt.y;

        //
        // Add the item's Name.
        //
        GlobalUnlock(hNewGroup);
        lpT = (LPSTR)PTR(lpGroupORI,((LPNT_ITEMDEF_A)lpid_A)->pName);

        offset = AddThing_A(hNewGroup, lpT, 0);
        if (!offset) {
            DebugMsg(DM_ERROR, TEXT("gc.ctug: AddThing pName failed"));
            goto PuntCreation;
        }
        lpgd = (LPNT_GROUPDEF)GlobalLock(hNewGroup);
        lpid = ITEM(lpgd, i);
        lpid->pName = offset;

        //
        // Add the item's Command line.
        //
        GlobalUnlock(hNewGroup);
        lpT = (LPSTR)PTR(lpGroupORI, ((LPNT_ITEMDEF_A)lpid_A)->pCommand);
        offset = AddThing_A(hNewGroup, lpT, 0);
        if (!offset) {
            DebugMsg(DM_ERROR, TEXT("gc.ctug: AddThing pCommand failed"));
            goto PuntCreation;
        }
        lpgd = (LPNT_GROUPDEF)GlobalLock(hNewGroup);
        lpid = ITEM(lpgd, i);
        lpid->pCommand = offset;

        //
        // Add the item's Icon path.
        //
        GlobalUnlock(hNewGroup);
        lpT = (LPSTR)PTR(lpGroupORI, ((LPNT_ITEMDEF_A)lpid_A)->pIconPath);
        offset = AddThing_A(hNewGroup, lpT, 0);
        if (!offset) {
            DebugMsg(DM_ERROR, TEXT("gc.ctug: AddThing pIconPath failed"));
            goto PuntCreation;
        }
        lpgd = (LPNT_GROUPDEF)GlobalLock(hNewGroup);
        lpid = ITEM(lpgd, i);
        lpid->pIconPath = offset;

        //
        // Get the item's icon resource using the Icon path and the icon index.
        // And add the item's Icon resource.
        //
        lpid->iIcon    = ((LPNT_ITEMDEF_A)lpid_A)->idIcon;
            lpid->cbIconRes = ((LPNT_ITEMDEF_A)lpid_A)->cbIconRes;
            lpid->wIconVer  = ((LPNT_ITEMDEF_A)lpid_A)->wIconVer;
        GlobalUnlock(hNewGroup);

        lpT = (LPBYTE)PTR(lpGroupORI, ((LPNT_ITEMDEF_A)lpid_A)->pIconRes);
        offset = AddThing_A(hNewGroup, (LPSTR)lpT, lpid->cbIconRes);
        if (!offset) {
            DebugMsg(DM_ERROR, TEXT("gc.ctug: AddThing pIconRes failed"));
            goto PuntCreation;
        }
        lpgd = (LPNT_GROUPDEF)GlobalLock(hNewGroup);
        lpid = ITEM(lpgd, i);
        lpid->pIconRes = offset;

        GlobalUnlock(hNewGroup);

      }
    }

    /*
     * Copy all the tags to the new group format.
     */
    lptag_A = (LPNT_PMTAG)((LPSTR)lpGroupORI + lpGroupORI->cbGroup); // lhb tracks

    if (lptag_A->wID == ID_MAGIC &&
        lptag_A->wItem == (int)0xFFFF &&
        *(LONG *)lptag_A->rgb == TAG_MAGIC) {

        //
        // This is the first tag id, goto start of item tags.
        //
        (LPBYTE)lptag_A += lptag_A->cb;

        while (lptag_A->wID != ID_LASTTAG) {

            wTagId = lptag_A->wID;
            cb = lptag_A->cb  - (3 * SIZEOF(DWORD)); // cb - sizeof tag

            if (wTagId == ID_MINIMIZE) {
                lpTagValueUNI = NULL;
            }
            else {
                lpTagValue = lptag_A->rgb ;
                if (wTagId != ID_HOTKEY) {

                    bAlloc = TRUE;
                    cchWideChar = 0;
                    cchMultiByte=MultiByteToWideChar(CP_ACP,
                                         MB_PRECOMPOSED,lpTagValue,
                                        -1,NULL,cchWideChar) ;

                    lpTagValueUNI = LocalAlloc(LPTR,(++cchMultiByte)*SIZEOF(WCHAR)) ;

                    MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,lpTagValue,
                                        -1,lpTagValueUNI,cchMultiByte) ;
                    cb = SIZEOF(WCHAR)*(lstrlenW(lpTagValueUNI) + 1); // lhb tracks
                }
                else {
                    lpTagValueUNI = (LPWSTR)lpTagValue;
                }
            }

            if (! AddTag( hNewGroup,
                          lptag_A->wItem,   // wItem
                          wTagId,              // wID
                          lpTagValueUNI,          // rgb : tag value
                          cb
                        )) {

                DebugMsg(DM_ERROR, TEXT("gc.ctug: AddTag failed"));
            }

            if (bAlloc && lpTagValueUNI) {
                LocalFree(lpTagValueUNI);
                bAlloc = FALSE;
            }

            (LPBYTE)lptag_A += lptag_A->cb ;      //  go to next tag
        }
    }

    lpgd = GlobalLock(hNewGroup);
    cb = SizeofGroup(lpgd);
    GlobalUnlock(hNewGroup);
    *lphNewGroup = hNewGroup;
    return(cb);

PuntCreation:
QuitThis:
    if (hNewGroup) {
        GlobalFree(hNewGroup);
    }
    return(0);
}
