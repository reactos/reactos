/*==========================================================================*/
//
//  class.c
//
//  Copyright (C) 1993-1994 Microsoft Corporation.  All Rights Reserved.
//  Mod Log:   Modified by Shawn Brown (10/95)
//                - Ported to NT (Unicode, etc.)
/*==========================================================================*/

#include "mmcpl.h"
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <cpl.h> 
#define NOSTATUSBAR
#include <commctrl.h>
#include <prsht.h>
#include <string.h>
#include <memory.h>
#include <idf.h>
#include <regstr.h>
#include "utils.h"

#include "midi.h"
#include "mmdebug.h"
#if defined DEBUG || defined DEBUG_RETAIL
 extern TCHAR szNestLevel[];
#endif

#include "medhelp.h"

#ifndef TVIS_ALL
#define TVIS_ALL 0xFF7F
#endif


static CONST TCHAR cszIdfWildcard[]    = TEXT ("*.idf");
static CONST TCHAR cszIdf[]            = TEXT (".idf");
static CONST TCHAR cszSetupKey[]       = REGSTR_PATH_SETUP REGSTR_KEY_SETUP;
static CONST TCHAR cszMachineDir[]     = REGSTR_VAL_WINDIR;
static CONST TCHAR cszConfigDir[]      = TEXT ("config\\");

extern CONST TCHAR cszMidiSlash[];
extern CONST TCHAR cszFriendlyName[];
extern CONST TCHAR cszDescription[];
extern CONST TCHAR cszSlashInstruments[];
extern CONST TCHAR cszExternal[];
extern CONST TCHAR cszDefinition[];
extern CONST TCHAR cszPort[];
extern CONST TCHAR cszDriversRoot[];
extern CONST TCHAR cszSchemeRoot[];
extern CONST TCHAR cszMidiMapRoot[];
extern CONST TCHAR cszDriversRoot[];
extern CONST TCHAR csz02d[];
extern CONST TCHAR cszSlash[];
extern CONST TCHAR cszEmpty[];

extern int lstrnicmp (LPTSTR pszA, LPTSTR pszB, size_t cch);


typedef struct _midi_class {
    LPPROPSHEETPAGE ppsp;
    HKEY            hkMidi;
    BOOL            bDetails;
    BOOL            bRemote;  // device connected via midi cable
    UINT            bChanges;
    UINT            ixDevice; // registry enum index of driver key
    BYTE            nPort;
    BYTE            bFill[3];
    BOOL            bFillingList;
  #ifdef USE_IDF_ICONS
    HIMAGELIST      hIDFImageList;
  #endif
    LPTSTR          pszKey;
    TCHAR           szFullKey[MAX_PATH];
    TCHAR           szAlias[MAX_PATH];
    TCHAR           szFile[MAX_PATH*2];
    } MCLASS, FAR * PMCLASS;

#define MCL_ALIAS_CHANGED 1
#define MCL_TREE_CHANGED  2
#define MCL_IDF_CHANGED   4
#define MCL_PORT_CHANGED  8


/*+
 * Determines if a given string has a given prefix and if
 * the next character in the string is a given charater.
 *
 * if so, it returns a pointer to the first character in the
 * string after the prefix.
 *
 * this is useful for parsing off the file in  file<Instrument>
 * or parts of registry paths.
 *
 * note that we do NOT consider a string to be a prefix of itself.
 * psz MUST be longer than than pszPrefix or this function returns NULL.
 *
 *-=================================================================*/

STATICFN LPTSTR WINAPI IsPrefix (
    LPTSTR pszPrefix,
    LPTSTR psz,
    TCHAR  chTerm)
{
    UINT  cb  = lstrlen(pszPrefix);
    UINT  cb2 = lstrlen(psz);
    TCHAR ch;

    if (cb2 < cb)
        return NULL;

    ch = psz[cb];
    if (ch != chTerm)
        return NULL;

    psz[cb] = 0;
    if (lstrcmpi(pszPrefix, psz))
    {
        psz[cb] = ch;
        return NULL;
    }

    psz[cb] = ch;
    return psz + cb;
}


/*+ IsFullPath
 *
 * returns true if the filename passed in is a fully qualified
 * pathname. returns false if it is a relative path
 *
 * unc paths are treated as fully qualified always
 *
 *-=================================================================*/

BOOL IsFullPath (
    LPTSTR pszFile)
{
    // fully qualified paths either begin with a backslash
    // or with a drive letter, colon, then backslash
    //
    if ((pszFile[0] == TEXT('\\')) ||
        (pszFile[1] == TEXT(':') && pszFile[2] == TEXT('\\')))
        return TRUE;

    return FALSE;
}


/*+ GetIDFDirectory
 *
 *-=================================================================*/

BOOL GetIDFDirectory (
    LPTSTR pszDir,
    UINT   cchDir)
{
    HKEY  hKey;
    UINT  cbSize;

    *pszDir = 0;

#if(_WIN32_WINNT >= 0x0400)
    if (!GetSystemDirectory (pszDir, cchDir))
        return FALSE;
#else
    if (!RegOpenKey (HKEY_LOCAL_MACHINE, cszSetupKey, &hKey))
    {
        cbSize = cchDir * sizeof(TCHAR);
        RegQueryValueEx (hKey, 
                         cszMachineDir, 
                         NULL, 
                         NULL, 
                         (LPBYTE)pszDir, 
                         &cbSize);
        RegCloseKey (hKey);

        cchDir = cbSize/sizeof(TCHAR);

        if (!cchDir--)
            return FALSE;
    }
    else if (!GetWindowsDirectory (pszDir, cchDir))
        return FALSE;
#endif

    cchDir = lstrlen (pszDir);
    if (pszDir[cchDir -1] != TEXT('\\'))
        pszDir[cchDir++] = TEXT('\\');
    lstrcpy (pszDir + cchDir, cszConfigDir);

#ifdef DEBUG
    AuxDebugEx (4, DEBUGLINE TEXT("IDFDir='%s'\r\n"), pszDir);
#endif
    return TRUE;
}


/*+ GetIDFFileName
 *
 *-=================================================================*/

BOOL GetIDFFileName (
    HWND    hWnd,
    LPTSTR  lpszFile,
    UINT    cchFile)
    {
    OPENFILENAME ofn;
    TCHAR        szFilter[MAX_PATH];
    UINT         cch;

    assert (hWnd);

    // load filter string from resource and convert '#' characters
    // into NULLs
    //
    LoadString (ghInstance, IDS_IDFFILES, szFilter, NUMELMS(szFilter));
    cch = lstrlen(szFilter);
    assert2 (cch, TEXT ("IDFFILES resource is empty!"));
    while (cch--)
    {
        if (TEXT('#') == szFilter[cch])
            szFilter[cch] = 0;
    }

    ZeroMemory (&ofn, sizeof(ofn));
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = hWnd;
    ofn.hInstance    = ghInstance;
    ofn.lpstrFilter  = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile    = lpszFile;
    ofn.nMaxFile     = cchFile;
    ofn.Flags        = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt  = cszIdf;

    return GetOpenFileName (&ofn);
    }


/*+ InstallNewIDF
 *
 *-=================================================================*/

BOOL WINAPI InstallNewIDF (
    HWND hWnd)
{
    TCHAR szWinPath[MAX_PATH];
    TCHAR szNewIDF[MAX_PATH];
    UINT  cch;
    UINT  oBasename;

    // prompt for an IDF file
    //
    szNewIDF[0] = 0;
    if ( ! GetIDFFileName (hWnd, szNewIDF, NUMELMS(szNewIDF)))
        return FALSE;

    // set oBasename to pointer to the first character of the
    // basename of the new idf file
    //
    oBasename = lstrlen (szNewIDF);
    if (!oBasename)
        return FALSE;
    while (oBasename && (TEXT('\\') != szNewIDF[oBasename-1]))
        --oBasename;

    // build the new filename from windows directory and idf basename
    //
    GetIDFDirectory (szWinPath, NUMELMS(szWinPath));
    cch = lstrlen (szWinPath);
    if (cch && szWinPath[cch-1] != TEXT('\\'))
        szWinPath[cch++] = TEXT('\\');
    lstrcpyn (szWinPath + cch, szNewIDF + oBasename, NUMELMS(szWinPath)-cch);
    oBasename = cch;

#ifdef DEBUG
    AuxDebugEx (5, DEBUGLINE TEXT("install IDF to '%s'\r\n"), szWinPath);
#endif
    // now force .idf as the extension for new file
    //
    for (cch = lstrlen (szWinPath); cch && szWinPath[cch] != TEXT('.'); --cch)
        if (TEXT('\\') == szWinPath[cch])
        {
            cch = lstrlen(szWinPath);
            break;
        }
    lstrcpy (szWinPath + cch, cszIdf);

    // quit now if we are trying to copy a file to itself
    //
    if (IsSzEqual(szWinPath, szNewIDF))
        return FALSE;

    // copy the file, but fail if destination already exists
    //
#ifdef DEBUG
    AuxDebugEx (5, DEBUGLINE TEXT("Copying %s to %s\r\n"), szNewIDF, szWinPath);
#endif
    if (CopyFile (szNewIDF, szWinPath, TRUE))
        return TRUE;
    //
    // if copy fails, query to overwrite because destination
    // already exists.
    //
    else
    {
        TCHAR szQuery[255];
        TCHAR sz[255];

#ifdef DEBUG
        AuxDebugEx (1, DEBUGLINE TEXT ("InstallIDF -CopyFile failed w/ %d\r\n"),
                    GetLastError());
#endif

        LoadString (ghInstance, IDS_QUERY_OVERIDF, sz, NUMELMS(sz));
        wsprintf (szQuery, sz, szWinPath + oBasename);

        LoadString (ghInstance, IDS_IDF_CAPTION, sz, NUMELMS(sz));

        if (MessageBox (hWnd, szQuery, sz, MB_YESNO | MB_ICONQUESTION) == IDYES)
            return CopyFile (szNewIDF, szWinPath, FALSE);
    }
    return FALSE;
}

/*+
 *
 * BUGBUG:  Please remove the #ifdef UNICODE sections 
 *          when mmioOpen gets UNICODE enabled !!!
 *-=================================================================*/

typedef BOOL (WINAPI * FNIDFENUM)(LPVOID        pvArg,
                                  UINT          nEnum,
                                  LPIDFHEADER   pHdr,
                                  LPIDFINSTINFO pInst);

UINT WINAPI idfEnumInstruments (
    LPTSTR     lpszFile,
    FNIDFENUM  fnEnum,
    LPVOID     lpvArg)
{
    MMCKINFO    chkIDFX;         // Grandparent chunk
    MMCKINFO    chkMMAP;         // Parent chunk
    HMMIO       hmmio;           // Handle to the file.
    UINT        nInstruments;

#ifdef UNICODE
    TCHAR        szFile[MAX_PATH];
    UINT        cchLen;
#endif

#ifdef DEBUG
    AuxDebugEx (3, DEBUGLINE TEXT("idfEnumInstruments('%s',%08X,%08X)\r\n"),
                lpszFile, fnEnum, lpvArg);
#endif

#ifdef UNICODE  
/*
    cchLen = lstrlen (lpszFile);
    if (cchLen >= NUMELMS(szFile))
    {
        assert4(0, TEXT("Text buffer (size = %d) not big enough (needed = %d)"), NUMELMS(szFile), cchLen);
        return 0;
    }
    wcstombs(szFile, lpszFile, cchLen );
*/
    wcscpy(szFile,lpszFile);
#endif


    // Open the file for reading.
#ifdef UNICODE
    hmmio = mmioOpen(szFile, NULL, MMIO_READ);
#else
    hmmio = mmioOpen(lpszFile, NULL, MMIO_READ);
#endif
    if ( ! hmmio)
    {
// What were they thinking??  You can't assert this.
//      assert3(0, TEXT("Cant open IDF file %s"), lpszFile ? lpszFile : TEXT("<null>"));
        return 0;
    }

    // the whole IDF instrument stuff is wrapped in an 'IDF ' RIFF chunk
    //
    chkIDFX.fccType = MAKEFOURCC('I','D','F',' ');
    if (mmioDescend(hmmio, &chkIDFX, NULL, MMIO_FINDRIFF))
    {
#ifdef DEBUG
        AuxDebugEx (0, DEBUGLINE TEXT ("idfEnum: '%s' is not a valid IDF File\r\n"), lpszFile);
#endif
        mmioClose(hmmio, 0);
        return 0;
    }

    // Count the number of instruments by counting
    // the number of "MMAP"'s in the file.
    //
    nInstruments = 0;
    chkMMAP.fccType = MAKEFOURCC('M','M','A','P');
    while ( ! mmioDescend(hmmio, &chkMMAP, &chkIDFX, MMIO_FINDLIST))
    {
        union {
            IDFHEADER idf;
            TCHAR      sz[MAX_ALIAS + sizeof(IDFHEADER)];
            } hdr;
        union {
            IDFINSTINFO iii;
            BYTE        ab[MAX_ALIAS * 8 + sizeof(IDFINSTINFO)];
            } inst;
        MMCKINFO chk;
        DWORD    cb;

#ifdef DEBUG
        AuxDebugEx (15, DEBUGLINE TEXT ("MMAP[%d] id=%08X siz=%08x\r\n"),
                    nInstruments, chkMMAP.ckid, chkMMAP.cksize);
#endif

        // read the hdr chunk
        //
        chk.ckid = MAKEFOURCC('h','d','r',' ');
        if (mmioDescend(hmmio, &chk, &chkMMAP, MMIO_FINDCHUNK))
            break;

#ifdef DEBUG
        AuxDebugEx (15, DEBUGLINE TEXT("  hdr.id=%08X hdr.siz=%08x\r\n"),
                    chk.ckid, chk.cksize);
#endif
        assert (chk.cksize > 0 && chk.cksize < 0x0080000);

        //AuxDebugDump (6, &chk, sizeof(chk));

        cb = min(chk.cksize, sizeof(hdr));
        if ((DWORD)mmioRead (hmmio, (LPVOID)&hdr, cb) != cb)
           break;

        //AuxDebugDump (6, &chk, sizeof(chk));

        hdr.sz[NUMELMS(hdr.sz)-1] = 0;
        mmioAscend (hmmio, &chk, 0);
#ifdef DEBUG
        AuxDebugEx (15, DEBUGLINE TEXT("hdr = '%s'\r\n"), hdr.idf.abInstID);
#endif

        //AuxDebugDump (6, &chk, sizeof(chk));

        // read the inst chunk and locate the product name
        // field.
        //
        chk.ckid = MAKEFOURCC('i','n','s','t');
        if (mmioDescend(hmmio, &chk, &chkMMAP, MMIO_FINDCHUNK))
        {
#ifdef DEBUG
            AuxDebug (TEXT ("mmioDescend failed for 'inst' chunk"));
#endif
        }

#ifdef DEBUG
        AuxDebugEx (15, DEBUGLINE TEXT("  inst.id=%08X inst.siz=%08x\r\n"),
                    chk.ckid, chk.cksize);
#endif
        assert (chk.cksize > 0 && chk.cksize < 0x0080000);
        cb = min(chk.cksize, sizeof(inst));
        if ((DWORD)mmioRead (hmmio, (LPVOID)&inst, cb) != cb)
        {
#ifdef DEBUG
            AuxDebug ( TEXT ("mmioRead failed for 'inst' chunk"));
#endif
        }

        inst.ab[NUMELMS(inst.ab)-1] = 0;
        mmioAscend (hmmio, &chk, 0);
#ifdef DEBUG
        AuxDebugEx (15, DEBUGLINE TEXT ("inst.mfg = '%s'\r\n"), inst.iii.abData);
        AuxDebugEx (15, TEXT ("\t.prod = '%s'\r\n"), inst.iii.abData
                                                   + inst.iii.cbManufactASCII
                                                   + inst.iii.cbManufactUNICODE);
#endif
        // call the enum callback for this instrument
        //
        if ( ! fnEnum (lpvArg, nInstruments, &hdr.idf, &inst.iii))
            break;

        ++nInstruments;
        assert (nInstruments < 20);

        // ascend and loop back to look for the next instrument
        //
        if (mmioAscend(hmmio, &chkMMAP, 0))
            break;
    }

    mmioClose(hmmio, 0);
    return nInstruments;
}


/*+ LoadTypesIntoTree
 *
 *-=================================================================*/

struct types_enum_data {
    HANDLE            hWndT;
    TV_INSERTSTRUCT * pti;
    LPTSTR            pszInstr;
    HTREEITEM         htiSel;
    };

STATICFN BOOL WINAPI fnTypesEnum (
    LPVOID        lpv,
    UINT          nEnum,
    LPIDFHEADER   pHdr,
    LPIDFINSTINFO pInst)
{
    struct types_enum_data * pted = lpv;
    HTREEITEM hti;

    assert (pted);

#ifdef DEBUG
    AuxDebugEx (7, DEBUGLINE TEXT ("enum[%d] '%s' instr=%x\r\n"),
                nEnum, pHdr->abInstID, pted->pszInstr);
#endif

    MultiByteToWideChar(GetACP(), 0,
                        pHdr->abInstID, -1,
                        pted->pti->item.pszText, sizeof(pted->pti->item.pszText));

    hti = TreeView_InsertItem (pted->hWndT, pted->pti);

    // this item is the 'selected' one, if it is the first
    // item or if it matches the name
    //
    if ((nEnum == 0) ||
        (pted->pszInstr && pted->pszInstr[0] &&
         IsPrefix(pted->pti->item.pszText, pted->pszInstr + sizeof(TCHAR), TEXT('>'))))
    {
        pted->htiSel = hti;
#ifdef DEBUG
        AuxDebugEx (7, DEBUGLINE TEXT("\t'%s' hti %08X is select\r\n"),
                    pted->pszInstr ? pted->pszInstr : TEXT (""), hti);
#endif
    }

    // return true to continue enumeration
    //
    return TRUE;
}

STATICFN void SetTypesEdit (
    HWND    hWnd,
    UINT    uId,
    PMCLASS pmcl)
{
    SetDlgItemText (hWnd, uId, pmcl->szFile);
}

STATICFN void LoadTypesIntoTree (
    HWND     hWnd,
    UINT     uId,
    PMCLASS  pmcl)
{
    HWND  hWndT;
    UINT  cchBase;
    TCHAR szPath[MAX_PATH];
    TCHAR szDefaultIDF[MAX_PATH];
    int   ix;
    WIN32_FIND_DATA ffd;
    HANDLE          hFind;
   #ifdef USE_IDF_ICONS
    HIMAGELIST      hImageList;
   #endif
    HTREEITEM       htiSelect = NULL; // item to select

    hWndT = GetDlgItem (hWnd, uId);
    if (!hWndT)
        return;

#ifdef DEBUG
    AuxDebugEx (5, DEBUGLINE TEXT ("LoadTypesIntoTree( ,%x, )\r\n"), uId);
#endif

    LoadString (ghInstance, IDS_GENERAL, szDefaultIDF, NUMELMS(szDefaultIDF));

   #ifdef USE_IDF_ICONS

    // if we have not already loaded an image list for the IDF types
    // do so now.
    //
    if (!(hImageList = pmcl->hIDFImageList))
    {
        static LPCTSTR aid[] = {
            MAKEINTRESOURCE(IDI_IDFICON),
            MAKEINTRESOURCE(IDI_BLANK),
            };
        int cx = GetSystemMetrics(SM_CXSMICON);
        int cy = GetSystemMetrics(SM_CYSMICON);

        pmcl->hIDFImageList =
        hImageList = ImageList_Create (cx, cy, TRUE, NUMELMS(aid), 2);

        if (hImageList)
        {
            UINT  ii;

            for (ii = 0; ii < NUMELMS(aid); ++ii)
            {
                HICON hIcon = LoadImage (ghInstance, aid[ii], IMAGE_ICON,
                                         cx, cy, LR_DEFAULTCOLOR);
                if (hIcon)
                    ImageList_AddIcon (hImageList, hIcon);
            }
        }
    }

   #endif

    pmcl->bFillingList = TRUE;

    //SetWindowRedraw (hWndT, FALSE);
#ifdef DEBUG
    AuxDebugEx (6, DEBUGLINE TEXT ("tv_deleteAllItems(%08X)\r\n"), hWndT);
#endif
    TreeView_DeleteAllItems(hWndT);
#ifdef DEBUG
    AuxDebugEx (6, DEBUGLINE TEXT ("tv_deleteAllItems(%08X) ends\r\n"), hWndT);
#endif
   #ifdef USE_IDF_ICONS
    TreeView_SetImageList (hWndT, hImageList, TVSIL_NORMAL);
   #endif
    htiSelect = NULL;

    pmcl->bFillingList = FALSE;

    GetIDFDirectory (szPath, NUMELMS(szPath));
    cchBase = lstrlen (szPath);
    if (cchBase && szPath[cchBase-1] != TEXT('\\'))
        szPath[cchBase++] = TEXT('\\');
    lstrcpyn (szPath + cchBase, cszIdfWildcard, NUMELMS(szPath)-cchBase);

#ifdef DEBUG
    AuxDebugEx (3, DEBUGLINE TEXT ("scanning for idfs at '%s'\r\n"), szPath);
#endif

    ix = 0;

    hFind = FindFirstFile (szPath, &ffd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
       TV_INSERTSTRUCT ti;
       struct types_enum_data ted = {hWndT, &ti, NULL, NULL};
       ZeroMemory (&ti, sizeof(ti));

       do
       {
           UINT   nInstr;
           UINT   cch;

           // patch off the extension before we add
           // this name to the list
           //
           cch = lstrlen(ffd.cFileName);
           while (cch)
              if (ffd.cFileName[--cch] == TEXT('.'))
              {
                 ffd.cFileName[cch] = 0;
                 break;
              }

           ti.hParent      = TVI_ROOT;
           ti.hInsertAfter = TVI_SORT;
          #ifdef USE_IDF_ICONS
           ti.item.mask      = TVIF_TEXT | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
          #else
           ti.item.mask      = TVIF_TEXT | TVIF_STATE;
          #endif

           // BUGBUG: the TV_ITEM structure may not be unicode enabled ?!?
           ti.item.pszText   = ffd.cFileName;
           ti.item.state     = 0;
           ti.item.stateMask = TVIS_ALL;

#ifdef DEBUG
           AuxDebugEx (7, DEBUGLINE TEXT ("adding '%s' to types tree\r\n"), ti.item.pszText);
#endif

           ti.hParent = TreeView_InsertItem (hWndT, &ti);
           if ( ! ti.hParent)
              break;
           ti.hInsertAfter = TVI_LAST;

           // put the extension back
           //
           if (cch > 0)
              ffd.cFileName[cch] = TEXT('.');

           // check to see if this file is a match for the
           // current definition file.
           //
#ifdef DEBUG
           AuxDebugEx (7, DEBUGLINE TEXT ("comparing '%s' with '%s'\r\n"),
                       ffd.cFileName, pmcl->szFile);
#endif
           ted.pszInstr = IsPrefix (ffd.cFileName, pmcl->szFile, TEXT('<'));
#ifdef DEBUG
           AuxDebugEx (7, DEBUGLINE TEXT ("\tpszInstr = '%s'\r\n"), ted.pszInstr ? ted.pszInstr : TEXT ("NULL"));
#endif

           // add instruments as subkeys to this file
           // this also has the side effect of setting ted.htiSel
           // when the instrument name matches
           //
           lstrcpy (szPath + cchBase, ffd.cFileName);
           nInstr = idfEnumInstruments (szPath, fnTypesEnum, &ted);

           // if this idf has no instruments. ignore it.
           // if it has more than one instrument, expand the list
           // so that instruments are visible
           //
           if (0 == nInstr)
               TreeView_DeleteItem (hWndT, ti.hParent);
           else if (nInstr > 1)
               TreeView_Expand (hWndT, ti.hParent, TVE_EXPAND);
           else
               ted.htiSel = ti.hParent;

           // if we have a match on filename, then we need to select
           // either the parent or one of the children
           //
           if (ted.pszInstr ||
               IsSzEqual(ffd.cFileName,pmcl->szFile) ||
               IsSzEqual(ffd.cFileName,szDefaultIDF))
           {
#ifdef DEBUG
               AuxDebugEx (7, DEBUGLINE TEXT ("will be selecting %08X '%s'\r\n"),
                           ted.htiSel, ffd.cFileName);
#endif
               htiSelect = ted.htiSel;
           }

        } while (FindNextFile (hFind, &ffd));

        FindClose (hFind);
    }

    if (htiSelect)
    {
        pmcl->bFillingList = TRUE;
#ifdef DEBUG
        AuxDebugEx (7, DEBUGLINE TEXT ("selecting %08X\r\n"), htiSelect);
#endif
        TreeView_SelectItem (hWndT, htiSelect);
#ifdef DEBUG
        AuxDebugEx (7, DEBUGLINE TEXT ("FirstVisible %08X\r\n"), htiSelect);
#endif
        TreeView_SelectSetFirstVisible (hWndT, htiSelect);
        pmcl->bFillingList = FALSE;
    }

#ifdef DEBUG
    AuxDebugEx (5, DEBUGLINE TEXT ("LoadTypesIntoTree( ,%d, ) ends\r\n"), uId);
#endif
    //SetWindowRedraw (hWndT, TRUE);
}

/*+
 *
 *-=================================================================*/

STATICFN void WINAPI HandleTypesSelChange (
    PMCLASS pmcl,
    LPNMHDR lpnm)
{
    LPNM_TREEVIEW pntv = (LPVOID)lpnm;
    LPTV_ITEM     pti  = &pntv->itemNew;
    HTREEITEM     htiParent;
    TV_ITEM       ti;

    assert (pmcl->bDetails);

    // setup ti to get text & # of children
    // from the IDF filename entry.
    //
    ti.mask       = TVIF_TEXT;
    ti.pszText    = pmcl->szFile;
    ti.cchTextMax = NUMELMS(pmcl->szFile);
    ti.hItem      = pti->hItem;

#ifdef DEBUG
    AuxDebugEx (6, DEBUGLINE TEXT ("Type Change pti=%08X hItem=%08X\r\n"), pti, pti->hItem);
#endif

    // if this entry has a parent, it must be a IDF
    // instrument name.  if so, then we want to read
    // from its parent first.
    //
    htiParent = TreeView_GetParent (lpnm->hwndFrom, pti->hItem);
    if (htiParent)
        ti.hItem = htiParent;

    TreeView_GetItem (lpnm->hwndFrom, &ti);
    lstrcat (pmcl->szFile, cszIdf);

#ifdef DEBUG
    AuxDebugEx (6, DEBUGLINE TEXT ("mask=%08x htiParent=%08X %08x nChild=%d '%s'\r\n"),
                ti.mask, htiParent, ti.hItem, ti.cChildren, ti.pszText);
#endif

    // if the selection had a parent, and we are not it's first child
    // then we need to append child (delimited by <>) after parent
    //
    if (htiParent &&
        (TreeView_GetChild(lpnm->hwndFrom, htiParent) != pti->hItem))
    {
        static CONST TCHAR cszAngle[] = TEXT(">");
        UINT cch = lstrlen(pmcl->szFile);

        pmcl->szFile[cch++] = TEXT('<');

        ti.mask       = TVIF_TEXT;
        ti.pszText    = pmcl->szFile + cch;
        ti.cchTextMax = NUMELMS(pmcl->szFile) - cch;
        ti.hItem      = pti->hItem;

        TreeView_GetItem (lpnm->hwndFrom, &ti);
        lstrcat (pmcl->szFile, cszAngle);
#ifdef DEBUG
        AuxDebugEx (6, DEBUGLINE TEXT ("appending child %08X; '%s'\r\n"), pti->hItem, pmcl->szFile);
#endif
    }

    pmcl->bChanges |= MCL_IDF_CHANGED;
}


/*+
 *
 *-=================================================================*/

STATICFN void LoadDevicesIntoList (
    HWND     hWnd,
    UINT     uId,
    PMCLASS  pmcl,
    BOOL     bList)
{
    HWND   hWndT;
    TCHAR  sz[MAX_ALIAS];
    DWORD  cch = sizeof(sz) / sizeof(TCHAR);
    UINT   ii;
    BOOL   bAdded = FALSE;

    hWndT = GetDlgItem (hWnd, uId);
    if (!hWndT)
        return;

    SetWindowRedraw (hWndT, FALSE);
    if (bList)
        ListBox_ResetContent(hWndT);
    else
        ComboBox_ResetContent(hWndT);

    if (!pmcl->hkMidi &&
        RegCreateKey (HKEY_LOCAL_MACHINE, cszDriversRoot, &pmcl->hkMidi))
        return;

    for (cch = sizeof(sz)/sizeof(TCHAR), ii = 0; ! RegEnumKey (pmcl->hkMidi, ii, sz, cch); ++ii)
    {
        TCHAR  szAlias[MAX_ALIAS];
        int    ix;
        BOOL   bExtern;
        BOOL   bActive;

        // read in the friendly name for this driver
        //
        if (GetAlias (pmcl->hkMidi, sz, szAlias, sizeof(szAlias)/sizeof(TCHAR), &bExtern, &bActive))
            continue;

        if (IsPrefix (sz, pmcl->pszKey, TEXT('\\')))
            pmcl->ixDevice = ii;

        // ignore if this is not an external device or if it is disabled
        //
        if ( ! bExtern || ! bActive)
            continue;

        // otherwise, add the driver name to the combobox/list
        //
        if (bList)
        {
            ix = ListBox_AddString (hWndT, szAlias);
            if (ix >= 0)
            {
                ListBox_SetItemData (hWndT, ix, ii);
                bAdded = TRUE;
            }
        }
        else
        {
            ix = ComboBox_AddString (hWndT, szAlias);
            if (ix >= 0)
            {
                ComboBox_SetItemData (hWndT, ix, ii);
                bAdded = TRUE;
            }
        }
    }

    SetWindowRedraw (hWndT, TRUE);
    EnableWindow (hWndT, bAdded);
    if (ii > 0)
        InvalidateRect (hWndT, NULL, TRUE);

    // iterate back through the items and select the one
    // that has item data that corresponds to driver that
    // owns the current device
    //
    if (bList)
    {
        UINT jj;

        for (jj = 0; jj < ii; ++jj)
        {
            if ((UINT)ListBox_GetItemData (hWndT, jj) == pmcl->ixDevice)
            {
                ListBox_SetCurSel (hWndT, jj);
                break;
            }
        }
        if (jj >= ii)
            ListBox_SetCurSel (hWndT, 0);
    }
    else
    {
        UINT jj;

        for (jj = 0; jj < ii ; ++jj)
        {
            if ((UINT)ComboBox_GetItemData (hWndT, jj) == pmcl->ixDevice)
            {
                ComboBox_SetCurSel (hWndT, jj);
                break;
            }
        }
        if (jj >= ii)
            ComboBox_SetCurSel (hWndT, 0);
    }
}


/*+ LoadClass
 *
 *-=================================================================*/

STATICFN BOOL WINAPI LoadClass (
    HWND    hWnd,
    PMCLASS pmcl)
{
    HKEY  hKeyA = NULL;
    BOOL  bRet = FALSE;
    UINT  cbSize;
    UINT  cch;
    DWORD dw;

    if (!pmcl->hkMidi &&
        RegCreateKey (HKEY_LOCAL_MACHINE, cszDriversRoot, &pmcl->hkMidi))
        goto cleanup;

    if (RegOpenKey (pmcl->hkMidi, pmcl->pszKey, &hKeyA))
        goto cleanup;

    // read data from this key
    //
    cbSize = sizeof(pmcl->szFile);
    RegQueryValueEx (hKeyA, cszDefinition, NULL, &dw, (LPBYTE)pmcl->szFile, &cbSize);

    // strip off leading directory (if there is one).
    //
    cch = lstrlen(pmcl->szFile);
    while (cch && (pmcl->szFile[cch-1] != TEXT('\\')))
        --cch;
    if (cch)
    {
        TCHAR szFile[MAX_PATH];
        lstrcpy (szFile, pmcl->szFile + cch);
        lstrcpy (pmcl->szFile, szFile);
    }

    // get scheme alias
    //
    cbSize = sizeof(pmcl->szAlias);
    RegQueryValueEx (hKeyA, cszFriendlyName, NULL, &dw, (LPBYTE)pmcl->szAlias, &cbSize);

    //
    //
    pmcl->nPort = 0;
    cbSize = sizeof(pmcl->nPort);
    RegQueryValueEx (hKeyA, cszPort, NULL, &dw, (LPVOID)&pmcl->nPort, &cbSize);

    pmcl->bChanges = 0;
    bRet = TRUE;

  cleanup:
    if (hKeyA)
       RegCloseKey (hKeyA);

    return bRet;
}


/*+ RebuildSchemes
 *
 * correct key references in the midi schemes when an instrument
 * is moved from one external midi port to another
 *
 *-=================================================================*/

STATICFN BOOL WINAPI RebuildSchemes (
    LPTSTR pszOldKey,
    LPTSTR pszNewKey)
{
    HKEY  hkSchemes;
    UINT  ii;
    TCHAR sz[MAX_ALIAS];
    UINT  cchNew;

    cchNew = 0;
    if (pszNewKey)
        cchNew = lstrlen(pszNewKey) + 1;

#ifdef DEBUG
    AuxDebugEx (3, DEBUGLINE TEXT ("RebuildSchemes('%s','%s')\r\n"),
                pszOldKey, pszNewKey ? pszNewKey : TEXT ("NULL"));
#endif

    if (RegCreateKey (HKEY_LOCAL_MACHINE, cszSchemeRoot, &hkSchemes))
        return FALSE;


    for (ii = 0; ! RegEnumKey (hkSchemes, ii, sz, sizeof(sz)/sizeof(TCHAR)); ++ii)
    {
        HKEY  hKeyA;
        UINT  jj;

        if (RegOpenKey (hkSchemes, sz, &hKeyA))
            continue;

        for (jj = 0; ! RegEnumKey (hKeyA, jj, sz, sizeof(sz)/sizeof(TCHAR)); ++jj)
        {
            UINT  cb;
            TCHAR szKey[MAX_PATH];

            cb = sizeof(szKey);
            if (RegQueryValue (hKeyA, sz, szKey, &cb))
                continue;

            if (IsSzEqual(pszOldKey, szKey))
            {
                if (cchNew)
                    RegSetValue (hKeyA, sz, REG_SZ, pszNewKey, cchNew);
                else
                    RegDeleteKey (hKeyA, sz);

#ifdef DEBUG
                AuxDebugEx (4, DEBUGLINE TEXT ("RebuildSchemes - fixing %d\\%d\r\n"), ii, jj);
#endif
            }
        }
    }

    return TRUE;
}


/*+ OpenInstrumentKey
 *
 *-=================================================================*/

STATICFN HKEY WINAPI OpenInstrumentKey (
    HWND    hWnd,
    PMCLASS pmcl,
    BOOL    bCreate)   // create an new key (do not remove or rebuild existing)
{
    TCHAR  szKey[MAX_ALIAS];
    HKEY   hkInst;
    HKEY   hKeyA = NULL;

#ifdef DEBUG
    AuxDebugEx (5, DEBUGLINE TEXT ("OpenInstrumentKey(%X,%08X,%d) szKey=%s\r\n"),
                hWnd, pmcl, bCreate, pmcl->pszKey ? pmcl->pszKey : TEXT ("NULL"));
#endif

    hkInst = NULL;

    if (!pmcl->hkMidi &&
        RegCreateKey (HKEY_LOCAL_MACHINE, cszDriversRoot, &pmcl->hkMidi))
        goto cleanup;

    if (RegEnumKey (pmcl->hkMidi, pmcl->ixDevice, szKey, sizeof(szKey)/sizeof(TCHAR)))
    {
        assert3(0, TEXT ("Failed to enum Midi device %d"), pmcl->ixDevice);
        goto cleanup;
    }
#ifdef DEBUG
    AuxDebugEx (6, DEBUGLINE TEXT ("ixDevice = %d, Key is %s\r\n"),
                pmcl->ixDevice, szKey);
#endif

    // if this is a driver key, or if we are not creating and
    // the instrument has not changed parentage, we can just
    // open the existing key and update its content
    //
    if (!pmcl->bRemote ||
        (!bCreate && IsPrefix (szKey, pmcl->pszKey, TEXT('\\'))))
    {
        if (RegOpenKey (pmcl->hkMidi, pmcl->pszKey, &hKeyA))
            goto cleanup;
#ifdef DEBUG
        AuxDebugEx (6, DEBUGLINE TEXT ("opened key %s\r\n"), pmcl->pszKey);
#endif
    }
    else
    {
        UINT  kk;
        TCHAR szEnum[10];

        pmcl->bChanges |= MCL_TREE_CHANGED;

        lstrcat (szKey, cszSlashInstruments);
        if (RegCreateKey (pmcl->hkMidi, szKey, &hkInst))
            goto cleanup;

        // find an unused keyname
        //
        for (kk = 0; kk < 128; ++kk)
        {
           wsprintf (szEnum, csz02d, kk);
           if (RegOpenKey (hkInst, szEnum, &hKeyA))
               break;
           RegCloseKey (hKeyA);
        }
        lstrcat (szKey, cszSlash);
        lstrcat (szKey, szEnum);

        // create a key with that name
        //
        if (RegCreateKey (hkInst, szEnum, &hKeyA))
            goto cleanup;

#ifdef DEBUG
        AuxDebugEx (6, DEBUGLINE TEXT ("created key %s\r\n"), szKey);
#endif

        // we are moving an instrument from one
        // external midi port to another
        //
        if (!bCreate)
        {
#ifdef DEBUG
            AuxDebugEx (3, DEBUGLINE TEXT ("Deleting key midi\\%s\r\n"), pmcl->pszKey);
#endif
            RegDeleteKey (pmcl->hkMidi, pmcl->pszKey);
            RebuildSchemes (pmcl->pszKey, szKey);
        }

        lstrcpy (pmcl->pszKey, szKey);
    }


  cleanup:
    if (hkInst)
       RegCloseKey (hkInst);

    return hKeyA;
}

/*+ SaveDetails
 *
 *-=================================================================*/

STATICFN UINT WINAPI SaveDetails (
    HWND    hWnd,
    PMCLASS pmcl,
    BOOL    bCreate)
{
    HWND   hWndT;
    HKEY   hKeyA;
    UINT   bChanges;
    UINT   cbSize;

    // this should only be called on shutdown
    // of details page (or on exit of wizard)
    //
    assert (pmcl->bDetails);

    hKeyA = OpenInstrumentKey (hWnd, pmcl, bCreate);
    if ( ! hKeyA)
        return FALSE;

    hWndT = GetDlgItem (hWnd, IDE_ALIAS);
    if (hWndT)
    {
        TCHAR sz[NUMELMS(pmcl->szAlias)];
        GetWindowText (hWndT, sz, NUMELMS(sz));
        if ( ! IsSzEqual(sz, pmcl->szAlias))
        {
            lstrcpy (pmcl->szAlias, sz);
            pmcl->bChanges |= MCL_ALIAS_CHANGED;
        }
    }

#ifdef DEBUG
    AuxDebugEx (2, DEBUGLINE TEXT ("--------SaveInstrument---------\r\n"));
    AuxDebugEx (2, TEXT ("\tChanges=%x\r\n"), pmcl->bChanges);
    AuxDebugEx (2, TEXT ("\tFriendly='%s'\r\n"), pmcl->szAlias);
    AuxDebugEx (2, TEXT ("\tDefinition='%s'\r\n"), pmcl->szFile);
#endif

    // save value data from this key
    //
    cbSize = (lstrlen(pmcl->szFile)+1) * sizeof(TCHAR);
    RegSetValueEx (hKeyA, cszDefinition, 0, REG_SZ, (LPBYTE)pmcl->szFile,
                   cbSize);

    cbSize = (lstrlen(pmcl->szAlias)+1) * sizeof(TCHAR);
    RegSetValueEx (hKeyA, cszFriendlyName, 0, REG_SZ,
                   (LPBYTE)pmcl->szAlias, cbSize);

    RegSetValueEx (hKeyA, cszPort, 0, REG_BINARY, (LPVOID)&pmcl->nPort, 1);

    RegCloseKey (hKeyA);

    bChanges = pmcl->bChanges;
    pmcl->bChanges = 0;

    // return 'changed' flag
    //
    return bChanges;
}


/*+ ParseAngleBrackets
 *
 *  replace '<>' delimiters with 0s and return a pointer
 *  to the delimited string. This function does nothing if
 *  the string does not end in a '>' delimiter
 *
 *-=================================================================*/

static LPTSTR __inline WINAPI ParseAngleBrackets (
    LPTSTR pszArg)
{
    LPTSTR psz = pszArg + lstrlen(pszArg);

    while (--psz > pszArg)
    {
        if (*psz == TEXT('>'))
        {
            *psz = 0;
            while (--psz >= pszArg)
            {
                if (*psz == TEXT('<'))
                {
                    *psz = 0;
                    return psz+1;
                }
            }
        }
    }

    return NULL;
}


/*+ fnFindDevice
 *
 *-=================================================================*/

struct _find_data {
    HWND   hWnd;
    UINT   idMfg;
    UINT   idProd;
    LPTSTR pszInstr;
    };

STATICFN BOOL WINAPI fnFindDevice (
    LPVOID        lpv,
    UINT          nEnum,
    LPIDFHEADER   pHdr,
    LPIDFINSTINFO pInst)
{
    struct _find_data * pfd = lpv;
    TCHAR szTemp[MAX_PATH];

    assert (pfd);

    MultiByteToWideChar(GetACP(), 0,
                        pHdr->abInstID, -1,
                        szTemp, sizeof(szTemp)/sizeof(TCHAR));

    if (!pfd->pszInstr ||
        IsSzEqual (pfd->pszInstr, szTemp))
    {
        if (SetDlgItemText (pfd->hWnd, pfd->idMfg, (TCHAR*)(pInst->abData+pInst->cbManufactASCII )))
            pfd->idMfg = 0;

        if (SetDlgItemText (pfd->hWnd, pfd->idProd,
                            (TCHAR*)(pInst->abData
                            + pInst->cbManufactASCII + pInst->cbManufactUNICODE)))
            pfd->idProd = 0;

        // we can stop enumerating now
        //
        return FALSE;
    }

    // return true to consider ennumeration
    //
    return TRUE;
}


/*+ ActivateInstrumentPage
 *
 *-=================================================================*/

STATICFN void WINAPI ActivateInstrumentPage (
    HWND    hWnd,
    PMCLASS pmcl)
{
    pmcl->bDetails = FALSE;
    if (GetDlgItem (hWnd, IDC_TYPES))
    {
        pmcl->bDetails = TRUE;
        LoadTypesIntoTree (hWnd, IDC_TYPES, pmcl);
        SetTypesEdit (hWnd, IDE_TYPES, pmcl);

        LoadDevicesIntoList (hWnd, IDC_DEVICES, pmcl, FALSE);

        if ( ! pmcl->bRemote)
        {
            HWND hWndT = GetDlgItem (hWnd, IDC_DEVICES);

            if (hWndT)
                EnableWindow (hWndT, FALSE);
        }
    }
    else
    {
        struct _find_data fd;
        TCHAR  szFile[NUMELMS(pmcl->szFile)];

        if ( ! IsFullPath (pmcl->szFile))
        {
           UINT  cch;

           GetIDFDirectory (szFile, NUMELMS(szFile));
           cch = lstrlen (szFile);
           if (cch && szFile[cch-1] != TEXT('\\'))
               szFile[cch++] = TEXT('\\');
           lstrcpyn (szFile + cch, pmcl->szFile, NUMELMS(szFile)-cch);
        }
        else
           lstrcpy (szFile, pmcl->szFile);

        fd.hWnd = hWnd;
        fd.idMfg = IDC_MANUFACTURER;
        fd.idProd = IDC_DEVICE_TYPE;
        fd.pszInstr = ParseAngleBrackets(szFile);

        idfEnumInstruments (szFile, fnFindDevice, &fd);

        if (fd.idMfg)
            SetDlgItemText (hWnd, fd.idMfg, cszEmpty);
        if (fd.idProd)
        {
            LoadString (ghInstance, IDS_UNSPECIFIED, szFile, NUMELMS(szFile));
            SetDlgItemText (hWnd, fd.idProd, szFile);
        }
    }
}


/*+ IsInstrumentKey
 *
 * return TRUE if the keyname passed refers to an instrument key
 * rather than a device key.  device keys usually end in '>',
 * while instrument keys will always be of the form
 * <dev>\Instruments\<enum>  where <dev> and <enum> can be arbitrary
 * strings.
 *
 *-=================================================================*/

STATICFN BOOL WINAPI IsInstrumentKey (
    LPTSTR pszKey)
{
    UINT cch = lstrlen(pszKey);
    if (!cch)
        return FALSE;

    if (pszKey[cch-1] == TEXT('>'))
        return FALSE;

    while (--cch)
        if (pszKey[cch] == TEXT('\\'))
            return TRUE;

    return FALSE;
}


/*+ InitInstrumentProps
 *
 *-=================================================================*/

STATICFN BOOL WINAPI InitInstrumentProps (
    HWND    hWnd,
    PMCLASS pmcl)
{
    LPPROPSHEETPAGE ppsp = pmcl->ppsp;
    PMPSARGS        pmpsa;

    assert (ppsp && ppsp->dwSize == sizeof(*ppsp));
    if (!ppsp)
        return FALSE; // EndDialog (hWnd, FALSE);

    pmcl->bRemote = FALSE;

    pmpsa = (LPVOID)ppsp->lParam;
    if (pmpsa && pmpsa->lpfnMMExtPSCallback)
    {
       pmpsa->lpfnMMExtPSCallback (MM_EPS_GETNODEDESC,
                                   (DWORD_PTR)pmcl->szAlias,
                                   sizeof(pmcl->szAlias),
                                   (DWORD_PTR)pmpsa->lParam);
#ifdef DEBUG
       AuxDebugEx (3, TEXT ("\tgot szAlias='%s'\r\n"), pmcl->szAlias);
#endif
       pmpsa->lpfnMMExtPSCallback (MM_EPS_GETNODEID,
                                   (DWORD_PTR)pmcl->szFullKey,
                                   sizeof(pmcl->szFullKey),
                                   (DWORD_PTR)pmpsa->lParam);
#ifdef DEBUG
       AuxDebugEx (3, TEXT ("\tgot szFullKey='%s'\r\n"), pmcl->szFullKey);
#endif
       // skip over the midi\ part of the key if we have been
       // passed that.  we want the driver name to be the first
       // part of the key
       //
       pmcl->pszKey = pmcl->szFullKey;
       if (!lstrnicmp (pmcl->pszKey,
                       (LPTSTR)cszMidiSlash,
                       lstrlen(cszMidiSlash)))
       {
           pmcl->pszKey += lstrlen(cszMidiSlash);
       }

       // If this is an instrument key, set bRemote to true
       //
       if (IsInstrumentKey(pmcl->pszKey))
          pmcl->bRemote = TRUE;
    }
    else
       LoadString (ghInstance, IDS_UNSPECIFIED,
                   pmcl->szAlias, NUMELMS(pmcl->szAlias));

    SetDlgItemText (hWnd, IDE_ALIAS, pmcl->szAlias);
    Static_SetIcon(GetDlgItem (hWnd, IDC_CLASS_ICON),
                   LoadIcon (ghInstance, MAKEINTRESOURCE(IDI_INSTRUMENT)));

    LoadClass (hWnd, pmcl);

    //ActivateInstrumentPage(hWnd, pmcl);

    return TRUE;
}


/*+ NotifyMapper
 *
 *-=================================================================*/

STATICFN void WINAPI NotifyMapper (
    PMCLASS pmcl,
    UINT    bChanges,
    HWND    hWnd)
{
    // tell midi mapper about tree changes, IDF changes and port changes
    //
    if (bChanges & (MCL_TREE_CHANGED | MCL_IDF_CHANGED | MCL_PORT_CHANGED))
    {
        KickMapper (hWnd);
    }
}


/*+
 *
 *-=================================================================*/

STATICFN BOOL WINAPI RemoveInstrument (
    HWND    hWnd,
    PMCLASS pmcl)
{
    RegDeleteKey (pmcl->hkMidi, pmcl->pszKey);
    RebuildSchemes (pmcl->pszKey, NULL);
    return TRUE;
}

BOOL WINAPI RemoveInstrumentByKeyName (
    LPCTSTR pszKey)
{
    MCLASS  mcl;
    BOOL    rc = FALSE;

    memset ((TCHAR *)&mcl, 0x00, sizeof(mcl));
    mcl.pszKey = (LPTSTR)pszKey;

    if (!lstrnicmp (mcl.pszKey,
                    (LPTSTR)cszMidiSlash,
                    lstrlen(cszMidiSlash)))
    {
        mcl.pszKey += lstrlen(cszMidiSlash);
    }

    if (LoadClass (NULL, &mcl))
    {
        rc = RemoveInstrument (NULL, &mcl);

        if (mcl.hkMidi)
            RegCloseKey (mcl.hkMidi);
    }

    return rc;
}


/*+ MidiInstrumentCommands
 *
 *-=================================================================*/

BOOL WINAPI MidiInstrumentCommands (
    HWND        hWnd,
    UINT_PTR    uId,
    LPNMHDR     lpnm)
{
    PMCLASS pmcl = GetDlgData(hWnd);

#ifdef DEBUG
    AuxDebugEx (4, DEBUGLINE TEXT ("InstrumentCommands(..%d..) %d(%xx)\r\n"),
                uId, lpnm->code, lpnm->code);
#endif

    if (!pmcl)
        return FALSE;
    
    switch (uId)
    {
        case IDE_ALIAS:
            if (lpnm->code == EN_CHANGE)
                PropSheet_Changed(GetParent(hWnd), hWnd);
            break;

        case IDB_REMOVE:
            if (RemoveInstrument (hWnd, pmcl))
            {
                PMPSARGS  pmpsa = (LPVOID)pmcl->ppsp->lParam;
                if (pmpsa && pmpsa->lpfnMMExtPSCallback)
                    pmpsa->lpfnMMExtPSCallback (MM_EPS_TREECHANGE, 0, 0, (DWORD_PTR)pmpsa->lParam);

                NotifyMapper (pmcl, MCL_TREE_CHANGED, hWnd);
                SetDlgData(hWnd, NULL);
                if (pmcl->hkMidi)
                    RegCloseKey (pmcl->hkMidi), pmcl->hkMidi = NULL;

                LocalFree ((HLOCAL)(UINT_PTR)(DWORD_PTR)pmcl);
                PropSheet_PressButton(GetParent(hWnd), PSBTN_CANCEL);
            }
            break;

        case IDB_NEWTYPE:
            InstallNewIDF (hWnd);
            LoadTypesIntoTree (hWnd, IDC_TYPES, pmcl);
            SetTypesEdit (hWnd, IDE_TYPES, pmcl);
            break;

        case IDC_TYPES:
            if ((lpnm->code == TVN_SELCHANGED) && !pmcl->bFillingList)
            {
                HandleTypesSelChange (pmcl, lpnm);
                SetTypesEdit (hWnd, IDE_TYPES, pmcl);
                PropSheet_Changed(GetParent(hWnd), hWnd);
#ifdef DEBUG
                AuxDebugEx (5, DEBUGLINE TEXT ("file='%s'\r\n"), pmcl->szFile);
#endif
            }
            break;

        case IDC_DEVICES:
            if (lpnm->code == CBN_SELCHANGE)
            {
                int ix = ComboBox_GetCurSel (lpnm->hwndFrom);
                pmcl->ixDevice = (UINT) ((ix >= 0) ? ComboBox_GetItemData (lpnm->hwndFrom, ix) : -1);
                PropSheet_Changed(GetParent(hWnd), hWnd);
#ifdef DEBUG
                AuxDebugEx (4, DEBUGLINE TEXT ("IDC_DEVICES.selChange(%d) %d\r\n"), ix, pmcl->ixDevice);
#endif
            }
            break;

        // we get these only if invoked as a dialog, not as a property
        // sheet
        //
        case IDOK:
            {
            UINT bChanges = SaveDetails (hWnd, pmcl, FALSE);
            NotifyMapper (pmcl, bChanges, hWnd);
            }
            // fall through
        case IDCANCEL:
            EndDialog (hWnd, uId);
            break;

        case 0:
        {
            LONG lRet = FALSE;

            switch (lpnm->code)
            {
                case PSN_APPLY:
#ifdef DEBUG
                    AuxDebugEx (4, DEBUGLINE TEXT ("ID_APPLY\r\n"));
#endif
                    if (pmcl->bDetails)
                    {
                        UINT bChanges = SaveDetails (hWnd, pmcl, FALSE);

                        NotifyMapper (pmcl, bChanges, hWnd);

                        // tell mmsys.cpl about tree & alias changes
                        //
                        if (bChanges & (MCL_TREE_CHANGED | MCL_ALIAS_CHANGED))
                        {
                            PMPSARGS  pmpsa = (LPVOID)pmcl->ppsp->lParam;
                            if (pmpsa && pmpsa->lpfnMMExtPSCallback)
                                pmpsa->lpfnMMExtPSCallback (MM_EPS_TREECHANGE, 0, 0, (DWORD_PTR)pmpsa->lParam);
                        }

                        // we do this because the SysTreeView for IDF files
                        // forgets its selection when APPLY is pressed. go figure
                        //
#ifdef DEBUG
                        AuxDebugEx (7,  DEBUGLINE TEXT ("PSN_APPLY: re-doing selection '%s'\r\n"), pmcl->szFile);
                        //ActivateInstrumentPage (hWnd, pmcl);
                        AuxDebugEx (7,  DEBUGLINE TEXT ("PSN_APPLY: done re-doing selection '%s'\r\n"), pmcl->szFile);
#endif
                    }
                    break;

                case PSN_KILLACTIVE:
                    break;

                case PSN_SETACTIVE:
#ifdef DEBUG
                    AuxDebugEx (4, DEBUGLINE TEXT ("PSN_SETACTIVE\r\n"));
#endif
                    ActivateInstrumentPage (hWnd, pmcl);
#ifdef DEBUG
                    AuxDebugEx (4, DEBUGLINE TEXT ("PSN_SETACTIVE ends\r\n"));
#endif
                    break;
            }
            SetWindowLongPtr (hWnd, DWLP_MSGRESULT, (LONG_PTR)lRet);
            break;
        }
    }

    return FALSE;
}

const static DWORD aKeyWordIds[] = {  // Context Help IDs
    IDC_CLASS_ICON, IDH_MMCPL_DEVPROP_DETAILS_INSTRUMENT,
    IDE_ALIAS,      IDH_MMCPL_DEVPROP_DETAILS_INSTRUMENT,
    IDC_DEVICES,    IDH_MMCPL_DEVPROP_DETAILS_MIDI_PORT,
    IDC_TYPES,      IDH_MMCPL_DEVPROP_DETAILS_INS_DEF,
    IDB_NEWTYPE,    IDH_MMCPL_DEVPROP_DETAILS_BROWSE,

    0, 0
};

/*+ MidiInstrumentDlgProc
 *
 *-=================================================================*/

INT_PTR CALLBACK MidiInstrumentDlgProc (
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
   #if defined DEBUG || defined DEBUG_RETAIL
    TCHAR chNest = szNestLevel[0]++;
   #endif

    switch (uMsg)
    {
        case WM_COMMAND:
            {
            NMHDR nmh;
            nmh.hwndFrom = GET_WM_COMMAND_HWND(wParam, lParam);
            nmh.idFrom   = GET_WM_COMMAND_ID(wParam, lParam);
            nmh.code     = GET_WM_COMMAND_CMD(wParam, lParam);

            MidiInstrumentCommands(hWnd, nmh.idFrom, &nmh);
            }
            break;
        
        case WM_NOTIFY:
#ifdef DEBUG
            AuxDebugEx (3, DEBUGLINE TEXT ("WM_NOTIFY(%x,%x,%x)\r\n"), hWnd, wParam, lParam);
#endif

           #if defined DEBUG || defined DEBUG_RETAIL
            ++szNestLevel[0];
           #endif

            MidiInstrumentCommands(hWnd, wParam, (LPVOID)lParam);

           #if defined DEBUG || defined DEBUG_RETAIL
            --szNestLevel[0];
           #endif
            break;
        
        case WM_INITDIALOG:
        {
            PMCLASS         pmcl;

            pmcl = (LPVOID)LocalAlloc(LPTR, sizeof(*pmcl));
            if (!pmcl)
                EndDialog(hWnd, FALSE);

            pmcl->ppsp = (LPVOID)lParam;
            SetDlgData (hWnd, pmcl);

#ifdef DEBUG
            AuxDebugEx (5, DEBUGLINE TEXT ("midiInstrument.WM_INITDLG ppsp=%08X\r\n"));
#endif
            //AuxDebugDump (8, pmcl->ppsp, sizeof(*(pmcl->ppsp)));

            InitInstrumentProps (hWnd, pmcl);
            break;
        }

        case WM_DESTROY:
        {
            PMCLASS pmcl = GetDlgData(hWnd);

            if (pmcl)
            {
                if (pmcl->hkMidi)
                    RegCloseKey (pmcl->hkMidi), pmcl->hkMidi = NULL;

               #ifdef USE_IDF_ICONS

                if (pmcl->hIDFImageList)
                {
                    HWND hWndT = GetDlgItem (hWnd, IDC_TYPES);
                    if (hWndT)
                        TreeView_SetImageList (hWndT, NULL, TVSIL_NORMAL);

                    ImageList_Destroy (pmcl->hIDFImageList);
                    pmcl->hIDFImageList = NULL;
                }

               #endif

                LocalFree ((HLOCAL)(UINT_PTR)(DWORD_PTR)pmcl);
            }

            break;
        }

        case WM_CONTEXTMENU:
            WinHelp ((HWND) wParam, NULL, HELP_CONTEXTMENU,
                    (UINT_PTR) (LPTSTR) aKeyWordIds);
            break;

        case WM_HELP:
        {
            LPHELPINFO lphi = (LPVOID) lParam;
            WinHelp (lphi->hItemHandle, NULL, HELP_WM_HELP,
                    (UINT_PTR) (LPTSTR) aKeyWordIds);
            break;
        }
    }

   #if defined DEBUG || defined DEBUG_RETAIL
    szNestLevel[0] = chNest;
   #endif
    return FALSE;
}


/// --------------------- Wizard stuff ----------------------

static LPTSTR aidWiz[] = {
    MAKEINTRESOURCE(IDD_MIDIWIZ02),
    MAKEINTRESOURCE(IDD_MIDIWIZ03),
    MAKEINTRESOURCE(IDD_MIDIWIZ04)
    };

#define WIZ_TEMPLATE_DEVICE  aidWiz[0]
#define WIZ_TEMPLATE_IDF     aidWiz[1]
#define WIZ_TEMPLATE_ALIAS   aidWiz[2]

typedef struct _wizdata {
    LPPROPSHEETPAGE ppspActive;
    HBITMAP         hBmp;
    MCLASS          mcl;
    PMCMIDI         pmcm;
    HPROPSHEETPAGE  ahpsp[NUMELMS(aidWiz)];
    } WIZDATA, * PWIZDATA;

/*+ FindInstrument
 *
 *-=================================================================*/

STATICFN PINSTRUM WINAPI FindInstrument (
    PMCMIDI  pmcm,
    LPTSTR   pszFriendly)
{
    UINT  ii;

    for (ii = 0; ii < pmcm->nInstr; ++ii)
    {
        assert (pmcm->api[ii]);
        if (IsSzEqual(pszFriendly, pmcm->api[ii]->szFriendly))
            return pmcm->api[ii];
    }

    return NULL;
}

/*+ UniqueFriendlyName
 *
 *-=================================================================*/

STATICFN BOOL WINAPI fnFirstInstr (
    LPVOID        lpv,
    UINT          nEnum,
    LPIDFHEADER   pHdr,
    LPIDFINSTINFO pInst)
{
    LPTSTR pszInstr = lpv;

    assert (pszInstr);

    MultiByteToWideChar(GetACP(), 0,
                        pHdr->abInstID, -1,
                        pszInstr, MAX_ALIAS);

    return FALSE;
}

STATICFN BOOL WINAPI UniqueFriendlyName (
    PMCMIDI pmcm,
    PMCLASS pmcl,
    LPTSTR  pszAlias,
    UINT    cchAlias)
{
    TCHAR  szFile[MAX_PATH * 2];
    LPTSTR pszInstr;
    UINT   cch;
    UINT   ii;

    GetIDFDirectory (szFile, sizeof(szFile)/sizeof(TCHAR));
    cch = lstrlen(szFile);
    if (cch && szFile[cch-1] != TEXT('\\'))
        szFile[cch++] = TEXT('\\');
    lstrcpy (szFile + cch, pmcl->szFile);
    pszInstr = ParseAngleBrackets (szFile);
    if ( ! pszInstr)
    {
        pszInstr = szFile + lstrlen(szFile) + 1;
        idfEnumInstruments (szFile, fnFirstInstr, pszInstr);
    }

    // if no instrument name from the IDF file, get a default
    // from our resources
    //
    if ( ! lstrlen (pszInstr))
    {
        LoadString (ghInstance, IDS_DEF_INSTRNAME, pszInstr, MAX_ALIAS);
        return FALSE;
    }

    // make the instrument name the same as the alias, and prepare
    // to append a number if the alias turns out not to be unique
    //
    lstrcpyn (pszAlias, pszInstr, cchAlias);
    cch = lstrlen (pszAlias);
    cch = min (cch, (UINT)MAX_ALIAS-3);
    ii = 1;

    // loop while we are trying to use an instrument name
    // that has already been used
    //
    while (FindInstrument (pmcm, pszAlias))
    {
        static CONST TCHAR cszSpaceD[] = TEXT (" %d");

        wsprintf (pszAlias + cch, cszSpaceD, ++ii);
        if (ii > NUMELMS(pmcm->api))
        {
            assert2(0, TEXT ("infinite loop in UniqueFriendlyName!"));
            break;
        }
    }

    return TRUE;
}


/*+ MidiWizardCommands
 *
 *-=================================================================*/

BOOL WINAPI MidiWizardCommands (
    HWND        hWnd,
    UINT_PTR    uId,
    LPNMHDR     lpnm)
{
    PWIZDATA         pwd;
    LPPROPSHEETPAGE  ppsp = GetDlgData(hWnd);
    LONG             lRet = TRUE;

#ifdef DEBUG
    AuxDebugEx (4, DEBUGLINE TEXT ("WizardCmd ppsp=%08X code=%d(0x%X)\r\n"),
                ppsp, lpnm->code, lpnm->code);
#endif

    pwd = NULL;
    if (ppsp)
        pwd = (LPVOID)ppsp->lParam;
    assert (pwd);

    switch (uId)
    {
        case IDC_TYPES:
            if ((lpnm->code == TVN_SELCHANGED) && !pwd->mcl.bFillingList)
            {
                HandleTypesSelChange (&pwd->mcl, lpnm);
                UniqueFriendlyName (pwd->pmcm, &pwd->mcl, pwd->mcl.szAlias, NUMELMS(pwd->mcl.szAlias));
#ifdef DEBUG
                AuxDebugEx (5, DEBUGLINE TEXT ("file='%s'\r\n"), pwd->mcl.szFile);
#endif
            }
            break;

        case IDB_NEWTYPE:
            InstallNewIDF (hWnd);
            LoadTypesIntoTree (hWnd, IDC_TYPES, &pwd->mcl);
            break;

        //case IDC_DEVICES:
        //    break;
        //case IDE_ALIAS:
        //    break;

        case 0:
        {
            switch (lpnm->code)
            {
                case PSN_HELP:
                    break;

                case PSN_KILLACTIVE:
#ifdef DEBUG
                    AuxDebugEx (4, DEBUGLINE TEXT ("PSN_KILLACTIVE\r\n"));
#endif
                    break;

                case PSN_SETACTIVE:
                {
                    DWORD dwWizBtn;

#ifdef DEBUG
                    AuxDebugEx (4, DEBUGLINE TEXT ("PSN_SETACTIVE\r\n"));
#endif
                    if (pwd)
                        pwd->ppspActive = ppsp;

                    if (ppsp->pszTemplate == WIZ_TEMPLATE_DEVICE) // midi device
                        LoadDevicesIntoList (hWnd, IDC_DEVICES, &pwd->mcl, TRUE);
                    else if (ppsp->pszTemplate == WIZ_TEMPLATE_IDF) // idf file
                        LoadTypesIntoTree (hWnd, IDC_TYPES, &pwd->mcl);
                    else if (ppsp->pszTemplate == WIZ_TEMPLATE_ALIAS) // alias
                        SetDlgItemText (hWnd, IDE_ALIAS, pwd->mcl.szAlias);

                    dwWizBtn = PSWIZB_NEXT | PSWIZB_BACK;
                    if (ppsp->pszTemplate == aidWiz[NUMELMS(aidWiz)-1])
                        dwWizBtn = PSWIZB_FINISH | PSWIZB_BACK;
                    else if (ppsp->pszTemplate == aidWiz[0])
                        dwWizBtn = PSWIZB_NEXT;

                    PropSheet_SetWizButtons (GetParent(hWnd), dwWizBtn);
                }
                    break;

                case PSN_WIZNEXT:
#ifdef DEBUG
                    AuxDebugEx (4, DEBUGLINE TEXT ("PSN_WIZNEXT\r\n"));
#endif

                    if (ppsp->pszTemplate == WIZ_TEMPLATE_DEVICE) // midi device
                    {
                        HWND   hWndT;
                        int    ix;

                        pwd->mcl.ixDevice = (UINT)-1;
                        pwd->mcl.nPort = 0;

                        hWndT = GetDlgItem (hWnd, IDC_DEVICES);
                        if (hWndT)
                        {
                            ix = ListBox_GetCurSel (hWndT);
                            if (ix >= 0)
                                pwd->mcl.ixDevice = (UINT) ListBox_GetItemData (hWndT, ix);
                        }

                        if (pwd->mcl.ixDevice == (UINT)-1)
                            SetWindowLongPtr (hWnd, DWLP_MSGRESULT, (LONG_PTR)-1);
                    }
                    else if (ppsp->pszTemplate == WIZ_TEMPLATE_IDF) // idf file
                    {
                        if ( ! pwd->mcl.szAlias[0])
                        {
                            LoadString (ghInstance, IDS_DEF_INSTRNAME,
                                        pwd->mcl.szAlias,
                                        NUMELMS(pwd->mcl.szAlias));
                        }
                    }
                    else if (ppsp->pszTemplate == WIZ_TEMPLATE_IDF) // alias
                    {
                        GetDlgItemText (hWnd, IDE_ALIAS, pwd->mcl.szAlias,
                                        NUMELMS(pwd->mcl.szAlias));
                    }
                    break;

                case PSN_WIZBACK:
#ifdef DEBUG
                    AuxDebugEx (4, DEBUGLINE TEXT ("PSN_WIZBACK\r\n"));
#endif
                    break;

                case PSN_WIZFINISH:
#ifdef DEBUG
                    AuxDebugEx (4, DEBUGLINE TEXT ("PSN_WIZFINISH\r\n"));
#endif
                    //if (!save success)
                       lRet = FALSE;
                    //SetWindowLong (hWnd, DWL_MSGRESULT, lRet);
                    SaveDetails (hWnd, &pwd->mcl, TRUE);
                    break;

                default:
                    lRet = FALSE;
            }
        }
            break;
    }

    return lRet;
}


/*+ MidiWizardDlgProc
 *
 *-=================================================================*/

INT_PTR CALLBACK MidiWizardDlgProc (
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL bRet = TRUE;
   #if defined DEBUG || defined DEBUG_RETAIL
    TCHAR chNest = szNestLevel[0]++;
   #endif

    switch (uMsg)
    {
        case WM_COMMAND:
            {
            NMHDR nmh;
            nmh.hwndFrom = GET_WM_COMMAND_HWND(wParam, lParam);
            nmh.idFrom   = GET_WM_COMMAND_ID(wParam, lParam);
            nmh.code     = GET_WM_COMMAND_CMD(wParam, lParam);

            bRet = MidiWizardCommands(hWnd, nmh.idFrom, &nmh);
            }
            break;
        
        case WM_NOTIFY:
#ifdef DEBUG
            AuxDebugEx (6, DEBUGLINE TEXT ("WM_NOTIFY(%x,%x,%x)\r\n"), hWnd, wParam, lParam);
#endif
            bRet = MidiWizardCommands(hWnd, wParam, (LPVOID)lParam);
            break;
        
        case WM_INITDIALOG:
        {
            PWIZDATA         pwd;
            LPPROPSHEETPAGE  ppsp = (LPVOID)lParam;

            SetDlgData (hWnd, lParam);

            pwd = (LPVOID)ppsp->lParam;

            SendDlgItemMessage(hWnd, IDC_WIZBMP, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)pwd->hBmp);

#ifdef DEBUG
            AuxDebugEx (5, DEBUGLINE TEXT ("MidiWizard.WM_INITDLG ppsp=%08X\r\n"), ppsp);
#endif
        }
            break;

        case WM_DESTROY:
        {
            PWIZDATA         pwd;
            LPPROPSHEETPAGE  ppsp = GetDlgData(hWnd);

            if (ppsp && (pwd = (LPVOID)ppsp->lParam) != NULL)
            {
                if (pwd->mcl.hkMidi)
                    RegCloseKey (pwd->mcl.hkMidi), pwd->mcl.hkMidi = NULL;

               #ifdef USE_IDF_ICONS

                if (pwd->mcl.hIDFImageList)
                {
                    HWND hWndT = GetDlgItem (hWnd, IDC_TYPES);
                    if (hWndT)
                        TreeView_SetImageList (hWndT, NULL, TVSIL_NORMAL);

                    ImageList_Destroy (pwd->mcl.hIDFImageList);
                    pwd->mcl.hIDFImageList = NULL;
                }

               #endif

            }
        }
            break;

        default:
            bRet = FALSE;
            break;
    }

   #if defined DEBUG || defined DEBUG_RETAIL
    szNestLevel[0] = chNest;
   #endif
    return bRet;
}


INT CALLBACK
iSetupDlgCallback(
    IN HWND             hwndDlg,
    IN UINT             uMsg,
    IN LPARAM           lParam
    )
/*++

Routine Description:

    Call back used to remove the "?" from the wizard page.

Arguments:

    hwndDlg - Handle to the property sheet dialog box.

    uMsg - Identifies the message being received. This parameter
            is one of the following values:

            PSCB_INITIALIZED - Indicates that the property sheet is
            being initialized. The lParam value is zero for this message.

            PSCB_PRECREATE      Indicates that the property sheet is about
            to be created. The hwndDlg parameter is NULL and the lParam
            parameter is a pointer to a dialog template in memory. This
            template is in the form of a DLGTEMPLATE structure followed
            by one or more DLGITEMTEMPLATE structures.

    lParam - Specifies additional information about the message. The
            meaning of this value depends on the uMsg parameter.

Return Value:

    The function returns zero.

--*/
{
    switch( uMsg )
    {
        case PSCB_INITIALIZED:
            break;

        case PSCB_PRECREATE:
            if( lParam ){
                DLGTEMPLATE *pDlgTemplate = (DLGTEMPLATE *)lParam;
                pDlgTemplate->style &= ~DS_CONTEXTHELP;
            }
            break;
    }

    return FALSE;
}

/*+ MidiInstrumentsWizard
 *
 *-=================================================================*/

INT_PTR MidiInstrumentsWizard (
    HWND    hWnd,
    PMCMIDI pmcm,       // optional
    LPTSTR  pszDriverKey) // optional
{
    WIZDATA         wd;
    PROPSHEETHEADER psh;
    PROPSHEETPAGE   psp;
    UINT            ii;
    INT_PTR         iRet = -1;
    LPTSTR          psz;

    ZeroMemory (&wd, sizeof(wd));
    wd.mcl.bDetails = TRUE;
    wd.mcl.bRemote = TRUE;
    wd.mcl.ixDevice = 0;
    LoadString (ghInstance, IDS_DEF_DEFINITION, wd.mcl.szFile,
                NUMELMS(wd.mcl.szFile));

    // set the default driver key to what was passed.
    // If someone passed us a path, rather than a driver key
    // null out the '\\' characters so that we see only the
    // leading driver part of the key.
    //
    wd.mcl.pszKey = wd.mcl.szFullKey;
    if (pszDriverKey)
       lstrcpy (wd.mcl.szFullKey, pszDriverKey);
    if (!lstrnicmp (wd.mcl.pszKey, (LPTSTR)cszMidiSlash, lstrlen(cszMidiSlash)))
        wd.mcl.pszKey += lstrlen(cszMidiSlash);
    psz = wd.mcl.pszKey;
    while (*psz)
    {
        if (*psz == TEXT('\\'))
            *psz = 0;
        ++psz;
    }

    // load all current instrument names from the registry
    //
    if (!(wd.pmcm = pmcm))
    {
        wd.pmcm = (LPVOID) LocalAlloc (LPTR, sizeof(MCMIDI));
        LoadInstruments (wd.pmcm, FALSE);
    }

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = ghInstance;
    psp.pfnDlgProc = MidiWizardDlgProc;
    psp.lParam = (LPARAM)&wd;

    for (psh.nPages = 0, ii = 0; ii < NUMELMS(aidWiz); ++ii)
    {
        HPROPSHEETPAGE hpsp;

        psp.pszTemplate = aidWiz[ii];
        wd.ahpsp[psh.nPages] = hpsp = CreatePropertySheetPage(&psp);
        if (hpsp)
            ++psh.nPages;
    }

    if ( ! psh.nPages)
        return -1;

    wd.hBmp = LoadBitmap(ghInstance, MAKEINTRESOURCE(IDB_WIZBMP));
#ifdef DEBUG
    AuxDebugEx (3, DEBUGLINE TEXT ("Wizard bitmap = %08X\r\n"));
#endif

    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE | PSH_WIZARD_LITE | PSH_USECALLBACK;
    psh.hwndParent = hWnd;
    psh.hInstance = ghInstance;
    psh.pszCaption = MAKEINTRESOURCE(IDS_WIZNAME);
    psh.nPages = NUMELMS(aidWiz);
    psh.nStartPage = 0;
    psh.phpage = wd.ahpsp;
    psh.pfnCallback = iSetupDlgCallback;

    iRet = PropertySheet (&psh);

    // free dynamically allocated stuff.
    //
    if (wd.hBmp)
       DeleteObject (wd.hBmp);

    // if no MCMIDI was passed, we dynamically loaded one,
    // so now we need to free it.
    //
    if ( ! pmcm)
    {
        if (wd.pmcm->hkMidi)
            RegCloseKey (wd.pmcm->hkMidi);
        FreeInstruments (wd.pmcm);
        LocalFree ((HLOCAL)(UINT_PTR)(DWORD_PTR)wd.pmcm);
    }

    return iRet;
}
