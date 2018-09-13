/****************************** Module Header ******************************\
*
* Module Name: extract.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Icon Extraction Routines
*
* History:
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include "newexe.h"

/****************************************************************************
 ****************************************************************************/

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#define ICON_MAGIC      0
#define ICO_MAGIC1      1
#define CUR_MAGIC1      2
#define BMP_MAGIC       ((WORD)'B'+((WORD)'M'<<8))
#define ANI_MAGIC       ((WORD)'R'+((WORD)'I'<<8))
#define ANI_MAGIC1      ((WORD)'F'+((WORD)'F'<<8))
#define ANI_MAGIC4      ((WORD)'A'+((WORD)'C'<<8))
#define ANI_MAGIC5      ((WORD)'O'+((WORD)'N'<<8))
#define MZMAGIC         ((WORD)'M'+((WORD)'Z'<<8))
#define PEMAGIC         ((WORD)'P'+((WORD)'E'<<8))
#define LEMAGIC         ((WORD)'L'+((WORD)'E'<<8))

typedef struct new_exe          NEWEXE,      *LPNEWEXE;
typedef struct exe_hdr          EXEHDR,      *LPEXEHDR;
typedef struct rsrc_nameinfo    RESNAMEINFO, *LPRESNAMEINFO;
typedef struct rsrc_typeinfo    RESTYPEINFO, *LPRESTYPEINFO;
typedef struct rsrc_typeinfo    UNALIGNED    *ULPRESTYPEINFO;
typedef struct new_rsrc         RESTABLE,    *LPRESTABLE;

#define RESOURCE_VA(x)        ((x)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress)
#define RESOURCE_SIZE(x)      ((x)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size)
#define NUMBER_OF_SECTIONS(x) ((x)->FileHeader.NumberOfSections)

#define FCC(c0,c1,c2,c3) ((DWORD)(c0)|((DWORD)(c1)<<8)|((DWORD)(c2)<<16)|((DWORD)(c3)<<24))

#define COM_FILE    FCC('.', 'c', 'o', 'm')
#define BAT_FILE    FCC('.', 'b', 'a', 't')
#define CMD_FILE    FCC('.', 'c', 'm', 'd')
#define PIF_FILE    FCC('.', 'p', 'i', 'f')
#define LNK_FILE    FCC('.', 'l', 'n', 'k')
#define ICO_FILE    FCC('.', 'i', 'c', 'o')
#define EXE_FILE    FCC('.', 'e', 'x', 'e')


#define WIN32VER30  0x00030000  // for CreateIconFromResource()

#define GET_COUNT   424242


/***************************************************************************\
* PathIsUNC
*
* Inline function to check for a double-backslash at the
* beginning of a string
*
\***************************************************************************/

__inline BOOL PathIsUNC(
    LPWSTR psz)
{
    return (psz[0] == L'\\' && psz[1] == L'\\');
}

/***************************************************************************\
* ReadAByte
*
* This is used to touch memory to assure that if we page-fault, it is
* outside win16lock.  Most icons aren't more than two pages.
*
\***************************************************************************/

BOOL ReadAByte(
    LPCVOID pMem)
{
    return ((*(PBYTE)pMem) == 0);
}

/***************************************************************************\
* RVAtoP
*
*
\***************************************************************************/

LPVOID RVAtoP(
    LPVOID pBase,
    DWORD  rva)
{
    LPEXEHDR             pmz;
    IMAGE_NT_HEADERS     *ppe;
    IMAGE_SECTION_HEADER *pSection; // section table
    int                  i;
    DWORD                size;

    pmz = (LPEXEHDR)pBase;
    ppe = (IMAGE_NT_HEADERS*)((BYTE*)pBase + pmz->e_lfanew);

    /*
     * Scan the section table looking for the RVA
     */
    pSection = IMAGE_FIRST_SECTION(ppe);

    for (i = 0; i < NUMBER_OF_SECTIONS(ppe); i++) {

        size = pSection[i].Misc.VirtualSize ?
               pSection[i].Misc.VirtualSize : pSection[i].SizeOfRawData;

        if (rva >= pSection[i].VirtualAddress &&
            rva <  pSection[i].VirtualAddress + size) {

            return (LPBYTE)pBase + pSection[i].PointerToRawData + (rva - pSection[i].VirtualAddress);
        }
    }

    return NULL;
}

/***************************************************************************\
* GetResourceTablePE
*
*
\***************************************************************************/

LPVOID GetResourceTablePE(
    LPVOID pBase)
{
    LPEXEHDR         pmz;
    IMAGE_NT_HEADERS *ppe;

    pmz = (LPEXEHDR)pBase;
    ppe = (IMAGE_NT_HEADERS*)((BYTE*)pBase + pmz->e_lfanew);

    if (pmz->e_magic != MZMAGIC)
        return 0;

    if (ppe->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    if (ppe->FileHeader.SizeOfOptionalHeader < IMAGE_SIZEOF_NT_OPTIONAL_HEADER)
        return 0;

    return RVAtoP(pBase, RESOURCE_VA(ppe));
}

/****************************************************************************
* FindResourcePE
*
*   given a PE resource directory will find a resource in it.
*
*   if iResIndex < 0 we will search for the specific index
*   if iResIndex >= 0 we will return the Nth index
*   if iResIndex == GET_COUNT the count of resources will be returned
*
\*****************************************************************************/

LPVOID FindResourcePE(
    LPVOID pBase,
    LPVOID prt,
    int    iResIndex,
    int    ResType,
    DWORD  *pcb)
{
    int                            i;
    int                            cnt;
    IMAGE_RESOURCE_DIRECTORY       *pdir;
    IMAGE_RESOURCE_DIRECTORY_ENTRY *pres;
    IMAGE_RESOURCE_DATA_ENTRY      *pent;

    pdir = (IMAGE_RESOURCE_DIRECTORY *)prt;

    /*
     * First find the type always a ID so ignore strings totaly
     */
    cnt  = pdir->NumberOfIdEntries + pdir->NumberOfNamedEntries;
    pres = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(pdir+1);

    for (i = 0; i < cnt; i++) {

        if (pres[i].Name == (DWORD)ResType)
            break;
    }

    if (i==cnt)             // did not find the type
        return 0;

    /*
     * Now go find the actual resource  either by id (iResIndex < 0) or
     * by ordinal (iResIndex >= 0)
     */
    pdir = (IMAGE_RESOURCE_DIRECTORY*)((LPBYTE)prt +
        (pres[i].OffsetToData & ~IMAGE_RESOURCE_DATA_IS_DIRECTORY));

    cnt  = pdir->NumberOfIdEntries + pdir->NumberOfNamedEntries;
    pres = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(pdir+1);

    /*
     * If we just want size, do it.
     */
    if (iResIndex == GET_COUNT)
        return (LPVOID)UIntToPtr( cnt );

    /*
     * if we are to search for a specific id do it.
     */
    if (iResIndex < 0) {

        for (i = 0; i < cnt; i++)
            if (pres[i].Name == (DWORD)(-iResIndex))
                break;
    } else {
        i = iResIndex;
    }

    /*
     * is the index in range?
     */
    if (i >= cnt)
        return 0;

    /*
     * if we get this far the resource has a language part, ick!
     * !!!for now just punt and return the first one.
     * !!!BUGBUG we dont handle multi-language icons
     */
    if (pres[i].OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY) {

        pdir = (IMAGE_RESOURCE_DIRECTORY*)((LPBYTE)prt +
                (pres[i].OffsetToData & ~IMAGE_RESOURCE_DATA_IS_DIRECTORY));
        pres = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(pdir+1);
        i = 0;  // choose first one
    }

    /*
     * Nested way to deep for me!
     */
    if (pres[i].OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY)
        return 0;

    pent = (IMAGE_RESOURCE_DATA_ENTRY*)((LPBYTE)prt + pres[i].OffsetToData);

    /*
     * all OffsetToData fields except the final one are relative to
     * the start of the section.  the final one is a virtual address
     * we need to go back to the header and get the virtual address
     * of the resource section to do this right.
     */
    *pcb = pent->Size;
    return RVAtoP(pBase, pent->OffsetToData);
}

/***************************************************************************\
* GetResourceTableNE
*
*
\***************************************************************************/

LPVOID GetResourceTableNE(
    LPVOID pBase)
{
    LPNEWEXE pne;
    LPEXEHDR pmz;

    pmz = (LPEXEHDR)pBase;
    pne = (LPNEWEXE)((LPBYTE)pBase + pmz->e_lfanew);

    if (pmz->e_magic != MZMAGIC)
        return 0;

    if (pne->ne_magic != NEMAGIC)           // must be a NEWEXE
        return 0;

    if (pne->ne_exetyp != NE_WINDOWS &&     // must be a Win DLL/EXE/386
        pne->ne_exetyp != NE_DEV386)
        return 0;

    if (pne->ne_expver < 0x0300)            // must be 3.0 or greater
        return 0;

    if (pne->ne_rsrctab == pne->ne_restab)  // no resources
        return 0;

    return (LPBYTE)pne + pne->ne_rsrctab;   // return resource table pointer
}

/***************************************************************************\
* FindResourceNE
*
* This returns a pointer to the rsrc_nameinfo of the resource with the
* given index and type, if it is found, otherwise it returns NULL.
*
* if iResIndex is < 0, then it is assumed to be a ID and the res table
* will be searched for a matching id.
*
* if iResIndex is >= 0, then it is assumed to be a index and the Nth
* resorce of the specifed type will be returned.
*
* if iResIndex == GET_COUNT the count of resources will be returned
*
\***************************************************************************/

LPVOID FindResourceNE(
    LPVOID lpBase,
    LPVOID prt,
    int    iResIndex,
    int    iResType,
    DWORD  *pcb)
{
    LPRESTABLE     lpResTable;
    ULPRESTYPEINFO ulpResTypeInfo;
    LPRESNAMEINFO  lpResNameInfo;  // 16 bit alignment ok - had ushorts only
    int            i;

    lpResTable = (LPRESTABLE)prt;
//ulpResTypeInfo = (ULPRESTYPEINFO)(LPWBYTE)&lpResTable->rs_typeinfo;
    ulpResTypeInfo = (ULPRESTYPEINFO)((LPBYTE)lpResTable + 2);

    while (ulpResTypeInfo->rt_id) {

        if (ulpResTypeInfo->rt_id == (iResType | RSORDID)) {

            lpResNameInfo = (LPRESNAMEINFO)(ulpResTypeInfo + 1);

            if (iResIndex == GET_COUNT)
                return (LPVOID)ulpResTypeInfo->rt_nres;

            if (iResIndex < 0) {

                for (i=0; i < (int)ulpResTypeInfo->rt_nres; i++) {

                    if (lpResNameInfo[i].rn_id == ((-iResIndex) | RSORDID))
                        break;
                }

                iResIndex = i;
            }

            if (iResIndex >= (int)ulpResTypeInfo->rt_nres)
                return NULL;

            *pcb = ((DWORD)lpResNameInfo[iResIndex].rn_length) << lpResTable->rs_align;
            return (LPBYTE)lpBase + ((long)lpResNameInfo[iResIndex].rn_offset << lpResTable->rs_align);
        }

        ulpResTypeInfo =
               (ULPRESTYPEINFO)((LPRESNAMEINFO)(ulpResTypeInfo + 1) +
                ulpResTypeInfo->rt_nres);
    }

    *pcb = 0;
    return NULL;
}

/***************************************************************************\
* ExtractIconFromICO
*
*
\***************************************************************************/

UINT ExtractIconFromICO(
    LPTSTR szFile,
    int    nIconIndex,
    int    cxIcon,
    int    cyIcon,
    HICON  *phicon,
    UINT   flags)
{
    HICON hicon;

    if (nIconIndex >= 1)
        return 0;

    flags |= LR_LOADFROMFILE;

again:

    hicon = LoadImage(NULL,
                      szFile,
                      IMAGE_ICON,
                      LOWORD(cxIcon),
                      LOWORD(cyIcon),
                      flags);

    if (hicon == NULL)
        return 0;

    /*
     * Do we just want a count?
     */
    if (phicon == NULL)
        DestroyCursor((HCURSOR)hicon);
    else
        *phicon = hicon;

    /*
     * Check for large/small icon extract
     */
    if (HIWORD(cxIcon)) {

        cxIcon = HIWORD(cxIcon);
        cyIcon = HIWORD(cyIcon);
        phicon++;

        goto again;
    }

    return 1;
}

/***************************************************************************\
* ExtractIconFromBMP
*
*
\***************************************************************************/

#define ROP_DSna 0x00220326

UINT ExtractIconFromBMP(
    LPTSTR szFile,
    int    nIconIndex,
    int    cxIcon,
    int    cyIcon,
    HICON  *phicon,
    UINT   flags)
{
    HICON    hicon;
    HBITMAP  hbm;
    HBITMAP  hbmMask;
    HDC      hdc;
    HDC      hdcMask;
    ICONINFO ii;

    if (nIconIndex >= 1)
        return 0;

    /*
     * BUGUS: don't use LR_CREATEDIBSECTION.  USER can't make an icon out
     * of a DibSection.
     */
    flags |= LR_LOADFROMFILE;

again:

    hbm = (HBITMAP)LoadImage(NULL,
                             szFile,
                             IMAGE_BITMAP,
                             LOWORD(cxIcon),
                             LOWORD(cyIcon),
                             flags);

    if (hbm == NULL)
        return 0;

    /*
     *  do we just want a count?
     */
    if (phicon == NULL) {
        DeleteObject(hbm);
        return 1;
    }

    hbmMask = CreateBitmap(LOWORD(cxIcon), LOWORD(cyIcon), 1, 1, NULL);

    hdc = CreateCompatibleDC(NULL);
    SelectObject(hdc, hbm);

    hdcMask = CreateCompatibleDC(NULL);
    SelectObject(hdcMask, hbmMask);

    SetBkColor(hdc, GetPixel(hdc, 0, 0));

    BitBlt(hdcMask, 0, 0, LOWORD(cxIcon), LOWORD(cyIcon), hdc, 0, 0, SRCCOPY);
    BitBlt(hdc, 0, 0, LOWORD(cxIcon), LOWORD(cyIcon), hdcMask, 0, 0, ROP_DSna);

    ii.fIcon    = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmColor = hbm;
    ii.hbmMask  = hbmMask;
    hicon = CreateIconIndirect(&ii);

    DeleteObject(hdc);
    DeleteObject(hbm);
    DeleteObject(hdcMask);
    DeleteObject(hbmMask);

    *phicon = hicon;

    /*
     * Check for large/small icon extract
     */
    if (HIWORD(cxIcon)) {
        cxIcon = HIWORD(cxIcon);
        cyIcon = HIWORD(cyIcon);
        phicon++;

        goto again;
    }

    return 1;
}

/***************************************************************************\
* ExtractIconFromEXE
*
*
\***************************************************************************/

UINT ExtractIconFromEXE(
    HANDLE hFile,
    int    nIconIndex,
    int    cxIconSize,
    int    cyIconSize,
    HICON  *phicon,
    UINT   *piconid,
    UINT   nIcons,
    UINT   flags)
{
    HANDLE           hFileMap = INVALID_HANDLE_VALUE;
    LPVOID           lpFile = NULL;
    EXEHDR           *pmz;
    NEWEXE UNALIGNED *pne;
    LPVOID           pBase;
    LPVOID           pres = NULL;
    UINT             result = 0;
    LONG             FileLength;
    DWORD            cbSize;
    int              cxIcon;
    int              cyIcon;

    LPVOID (*FindResourceX)(LPVOID pBase,
                            LPVOID prt,
                            int    iResIndex,
                            int    iResType,
                            DWORD  *pcb);

    FileLength = (LONG)GetFileSize(hFile, NULL);

    hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hFileMap == NULL)
        goto exit;

    lpFile = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
    if (lpFile == NULL)
        goto exit;

    pBase = (LPVOID)lpFile;
    pmz = (struct exe_hdr *)pBase;

    _try {

        if (pmz->e_magic != MZMAGIC)
            goto exit;

        if (pmz->e_lfanew <= 0)             // not a new exe
            goto exit;

        if (pmz->e_lfanew >= FileLength)    // not a new exe
            goto exit;

        pne = (NEWEXE UNALIGNED *)((BYTE*)pmz + pmz->e_lfanew);

        switch (pne->ne_magic) {
        case NEMAGIC:
            pres = GetResourceTableNE(pBase);
            FindResourceX = FindResourceNE;
            break;

        case PEMAGIC:
            pres = GetResourceTablePE(pBase);
            FindResourceX = FindResourcePE;
            break;
        }

        /*
         * cant find the resource table, fail
         */
        if (pres == NULL)
            goto exit;

        /*
         * do we just want a count?
         */
        if (phicon == NULL) {
            result = PtrToUlong(FindResourceX(pBase,
                                             pres,
                                             GET_COUNT,
                                             (LONG_PTR)RT_GROUP_ICON,
                                             &cbSize));
            goto exit;
        }

        while (result < nIcons) {

            LPVOID lpIconDir;
            LPVOID lpIcon;
            int    idIcon;

            cxIcon = cxIconSize;
            cyIcon = cyIconSize;

            /*
             *  find the icon dir for this icon.
             */
            lpIconDir = FindResourceX(pBase,
                                      pres,
                                      nIconIndex,
                                      (LONG_PTR)RT_GROUP_ICON,
                                      &cbSize);

            if (lpIconDir == NULL)
                goto exit;

            if ((((LPNEWHEADER)lpIconDir)->Reserved != 0) ||
                (((LPNEWHEADER)lpIconDir)->ResType != FT_ICON)) {

                goto exit;
            }
again:
            idIcon = LookupIconIdFromDirectoryEx((LPBYTE)lpIconDir,
                                                 TRUE,
                                                 LOWORD(cxIcon),
                                                 LOWORD(cyIcon),
                                                 flags);
            lpIcon = FindResourceX(pBase,
                                   pres,
                                   -idIcon,
                                   (LONG_PTR)RT_ICON,
                                   &cbSize);

            if (lpIcon == NULL)
                goto exit;

            if ((((UPBITMAPINFOHEADER)lpIcon)->biSize != sizeof(BITMAPINFOHEADER)) &&
                (((UPBITMAPINFOHEADER)lpIcon)->biSize != sizeof(BITMAPCOREHEADER))) {

                goto exit;
            }

#ifndef WINNT
            /* touch this memory before calling USER
             * so if we page fault we will do it outside of the Win16Lock
             * most icons aren't more than 2 pages
             */
            ReadAByte(((BYTE *)lpIcon) + cbSize - 1);
#endif

            if (piconid)
                piconid[result] = idIcon;

            phicon[result++] = CreateIconFromResourceEx((LPBYTE)lpIcon,
                                                        cbSize,
                                                        TRUE,
                                                        WIN32VER30,
                                                        LOWORD(cxIcon),
                                                        LOWORD(cyIcon),
                                                        flags);

            /*
             * check for large/small icon extract
             */
            if (HIWORD(cxIcon)) {

                cxIcon = HIWORD(cxIcon);
                cyIcon = HIWORD(cyIcon);

                goto again;
            }

            nIconIndex++;       // next icon index
        }

    } _except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        result = 0;
    }

exit:

    if (lpFile)
        UnmapViewOfFile(lpFile);

    if (hFileMap != INVALID_HANDLE_VALUE)
        CloseHandle(hFileMap);

    return result;
}

/***************************************************************************\
* PathFindExtension
*
*
\***************************************************************************/

LPWSTR PathFindExtension(
    LPWSTR pszPath)
{
    LPWSTR pszDot;

    for (pszDot = NULL; *pszPath; pszPath = CharNext(pszPath)) {

        switch (*pszPath) {
        case L'.':
            pszDot = pszPath;    // remember the last dot
            break;

        case L'\\':
        case L' ':               // extensions can't have spaces
            pszDot = NULL;       // forget last dot, it was in a directory
            break;
        }
    }

    /*
     * if we found the extension, return ptr to the dot, else
     * ptr to end of the string (NULL extension) (cast->non const)
     */
    return pszDot ? (LPWSTR)pszDot : (LPWSTR)pszPath;
}

/***************************************************************************\
* PrivateExtractIconExA
*
* Ansi version of PrivateExtractIconExW
*
\***************************************************************************/

WINUSERAPI UINT PrivateExtractIconExA(
    LPCSTR szFileName,
    int    nIconIndex,
    HICON  *phiconLarge,
    HICON  *phiconSmall,
    UINT   nIcons)
{
    LPWSTR szFileNameW;
    UINT    uRet;

    if (!MBToWCS(szFileName, -1, &szFileNameW, -1, TRUE))
        return 0;

    uRet = PrivateExtractIconExW(szFileNameW,
                                 nIconIndex,
                                 phiconLarge,
                                 phiconSmall,
                                 nIcons);

    UserLocalFree(szFileNameW);

    return uRet;
}

/***************************************************************************\
* HasExtension
*
*
\***************************************************************************/

DWORD HasExtension(
    LPWSTR pszPath)
{
    LPWSTR p = PathFindExtension(pszPath);

    /*
     * BUGBUG - BobDay - Shouldn't this limit the length to 4 characters?
     * "Fister.Bather" would return .BAT
     *
     * BUGBUG - BobDay - We could make this EXTKEY based like the extension
     * matching stuff elsewhere.  EXTKEY is a QWORD value so UNICODE would
     * fit.
     */
    if (*p == L'.') {

        WCHAR szExt[5];

        lstrcpynW(szExt, p, 5);

        if (lstrcmpiW(szExt,TEXT(".com")) == 0) return COM_FILE;
        if (lstrcmpiW(szExt,TEXT(".bat")) == 0) return BAT_FILE;
        if (lstrcmpiW(szExt,TEXT(".cmd")) == 0) return CMD_FILE;
        if (lstrcmpiW(szExt,TEXT(".pif")) == 0) return PIF_FILE;
        if (lstrcmpiW(szExt,TEXT(".lnk")) == 0) return LNK_FILE;
        if (lstrcmpiW(szExt,TEXT(".ico")) == 0) return ICO_FILE;
        if (lstrcmpiW(szExt,TEXT(".exe")) == 0) return EXE_FILE;
    }

    return 0;
}

/***************************************************************************\
* PrivateExtractIconsW
*
* Extracts 1 or more icons from a file.
*
* input:
*     szFileName          - EXE/DLL/ICO/CUR/ANI file to extract from
*     nIconIndex          - what icon to extract
*                             0 = first icon, 1=second icon, etc.
*                            -N = icon with id==N
*     cxIcon              - icon size wanted (if HIWORD != 0 two sizes...)
*     cyIcon              - icon size wanted (if HIWORD != 0 two sizes...)
*                           0,0 means extract at natural size.
*     phicon              - place to return extracted icon(s)
*     nIcons              - number of icons to extract.
*     flags               - LoadImage LR_* flags
*
* returns:
*     if picon is NULL, number of icons in the file is returned.
*
* notes:
*     handles extraction from PE (Win32), NE (Win16), ICO (Icon),
*     CUR (Cursor), ANI (Animated Cursor), and BMP (Bitmap) files.
*     only Win16 3.x files are supported (not 2.x)
*
*     cx/cyIcon are the size of the icon to extract, two sizes
*     can be extracted by putting size 1 in the loword and size 2 in the
*     hiword, ie MAKELONG(24, 48) would extract 24 and 48 size icons.
*     yea this is a stupid hack it is done so IExtractIcon::Extract
*     can be called by outside people with custom large/small icon
*     sizes that are not what the shell uses internaly.
*
\***************************************************************************/

WINUSERAPI UINT WINAPI PrivateExtractIconsW(
    LPCWSTR szFileName,
    int     nIconIndex,
    int     cxIcon,
    int     cyIcon,
    HICON   *phicon,
    UINT    *piconid,
    UINT    nIcons,
    UINT    flags)
{
    HANDLE   hFile = (HANDLE)INVALID_HANDLE_VALUE;
    UINT     result = 0;
    WORD     magic[6];
    WCHAR    achFileName[MAX_PATH];
    FILETIME ftAccess;
    WCHAR    szExpFileName[MAX_PATH];
    DWORD    dwBytesRead;

    /*
     * Set failure defaults.
     */
    if (phicon)
        *phicon = NULL;

    /*
     * Check for special extensions, and fail quick
     */
    switch (HasExtension((LPWSTR)szFileName)) {
    case COM_FILE:
    case BAT_FILE:
    case CMD_FILE:
    case PIF_FILE:
    case LNK_FILE:
        goto exit;

    default:
        break;
    }

    /*
     * Try expanding environment variables in the file name we're passed.
     */
    ExpandEnvironmentStrings(szFileName, szExpFileName, MAX_PATH);
    szExpFileName[ MAX_PATH-1 ] = (WCHAR)0;

    /*
     * Open the file - First check to see if it is a UNC path.  If it
     * is make sure that we have access to the path...
     */
    if (PathIsUNC(szExpFileName)) {

        lstrcpynW(achFileName, szExpFileName, ARRAYSIZE(achFileName));     // BUGBUG sizeof

    } else {

        if (SearchPath(NULL,
                       szExpFileName,
                       NULL,
                       ARRAYSIZE(achFileName),
                       achFileName, NULL) == 0) {

            goto error_file;
        }
    }

    hFile = CreateFile(achFileName,
                       GENERIC_READ|FILE_WRITE_ATTRIBUTES,
                       FILE_SHARE_WRITE | FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                       0);

    if (hFile == INVALID_HANDLE_VALUE) {

        hFile = CreateFile(achFileName, GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                           0);

        if (hFile == INVALID_HANDLE_VALUE)
            goto error_file;

    } else {

        /*
         * Restore the Access Date
         */
        if (GetFileTime(hFile, NULL, &ftAccess, NULL))
            SetFileTime(hFile, NULL, &ftAccess, NULL);
    }


    ReadFile(hFile, &magic, sizeof(magic), &dwBytesRead, NULL);
    if (dwBytesRead != sizeof(magic))
        goto exit;

    if (piconid)
        *piconid = (UINT)-1;    // Fill in "don't know" value

    switch (magic[0]) {
    case MZMAGIC:
        result = ExtractIconFromEXE(hFile,
                                    nIconIndex,
                                    cxIcon,
                                    cyIcon,
                                    phicon,
                                    piconid,
                                    nIcons,
                                    flags);
        break;

    case ANI_MAGIC:    // possible .ani cursor

        /*
         * Ani cursors are easy they are RIFF files of type 'ACON'
         */
        if (magic[1] == ANI_MAGIC1 && magic[4] == ANI_MAGIC4 &&
            magic[5] == ANI_MAGIC5) {

            result = ExtractIconFromICO(achFileName,
                                        nIconIndex,
                                        cxIcon,
                                        cyIcon,
                                        phicon,
                                        flags);
        }
        break;

    case BMP_MAGIC:    // possible bitmap
        result = ExtractIconFromBMP(achFileName,
                                    nIconIndex,
                                    cxIcon,
                                    cyIcon,
                                    phicon,
                                    flags);
        break;

    case ICON_MAGIC:   // possible .ico or .cur

        /*
         * Icons and cursors look like this
         *
         * iReserved       - always zero
         * iResourceType   - 1 for icons 2 cor cursor.
         * cresIcons       - number of resolutions in this file
         *
         * We only allow 1 <= cresIcons <= 10
         */
        if ((magic[1] == ICO_MAGIC1 || magic[1] == CUR_MAGIC1) &&
            magic[2] >= 1 && magic[2] <= 10) {

            result = ExtractIconFromICO(achFileName,
                                        nIconIndex,
                                        cxIcon,
                                        cyIcon,
                                        phicon,
                                        flags);
        }
        break;
    }

exit:

    if (hFile!=INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return result;

    /*
     *  if we cant open the file, return a code saying we cant open the file
     *  if phicon==NULL return the count of icons in the file 0
     */

error_file:

    result = (phicon ? (UINT)-1 : 0);

    goto exit;
}

/***************************************************************************\
* PrivateExtractIconsA
*
*
\***************************************************************************/

WINUSERAPI UINT WINAPI PrivateExtractIconsA(
    LPCSTR szFileName,
    int     nIconIndex,
    int     cxIcon,
    int     cyIcon,
    HICON   *phicon,
    UINT    *piconid,
    UINT    nIcons,
    UINT    flags)
{
    LPWSTR szFileNameW;
    UINT uRet;

    if (!MBToWCS(szFileName, -1, &szFileNameW, -1, TRUE))
        return 0;

    uRet = PrivateExtractIconsW(szFileNameW,
                                nIconIndex,
                                cxIcon,
                                cyIcon,
                                phicon,
                                piconid,
                                nIcons,
                                flags);

    UserLocalFree(szFileNameW);

    return uRet;
}

/***************************************************************************\
* PrivateExtractIconExW
*
* extracts 1 or more icons from a file.
*
* input:
*     szFileName          - EXE/DLL/ICO file to extract from
*     nIconIndex          - what icon to extract
*                             0 = first icon, 1=second icon, etc.
*                            -N = icon with id==N
*     phiconLarge         - place to return extracted icon(s)
*     phiconSmall         - place to return extracted icon(s) (small size)
*     nIcons              - number of icons to extract.
*
* returns:
*     number of icons extracted, or the count of icons if phiconLarge==NULL
*
* notes:
*     handles extraction from PE (Win32), NE (Win16), and ICO (Icon) files.
*     only Win16 3.x files are supported (not 2.x)
*
\***************************************************************************/

WINUSERAPI UINT PrivateExtractIconExW(
    LPCWSTR szFileName,
    int     nIconIndex,
    HICON   *phiconLarge,
    HICON   *phiconSmall,
    UINT    nIcons)
{
    UINT result = 0;

    if ((nIconIndex == -1) || ((phiconLarge == NULL) && (phiconSmall == NULL)))
        return PrivateExtractIconsW(szFileName, 0, 0, 0, NULL, NULL, 0, 0);

    if (phiconLarge && phiconSmall && (nIcons == 1)) {

        HICON ahicon[2];

        ahicon[0] = NULL;
        ahicon[1] = NULL;

        result = PrivateExtractIconsW(szFileName,
                                      nIconIndex,
                                      MAKELONG(GetSystemMetrics(SM_CXICON),
                                               GetSystemMetrics(SM_CXSMICON)),
                                      MAKELONG(GetSystemMetrics(SM_CYICON),
                                               GetSystemMetrics(SM_CYSMICON)),
                                      ahicon,
                                      NULL,
                                      2,
                                      0);

        *phiconLarge = ahicon[0];
        *phiconSmall = ahicon[1];

    } else {

        if (phiconLarge)
            result = PrivateExtractIconsW(szFileName,
                                          nIconIndex,
                                          GetSystemMetrics(SM_CXICON),
                                          GetSystemMetrics(SM_CYICON),
                                          phiconLarge,
                                          NULL,
                                          nIcons,
                                          0);

        if (phiconSmall)
            result = PrivateExtractIconsW(szFileName,
                                          nIconIndex,
                                          GetSystemMetrics(SM_CXSMICON),
                                          GetSystemMetrics(SM_CYSMICON),
                                          phiconSmall,
                                          NULL,
                                          nIcons,
                                          0);
    }

    return result;
}
