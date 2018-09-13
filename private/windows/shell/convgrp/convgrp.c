/****************************************************************************/
/*                                                                          */
/*  CONVGRP.H -                                                             */
/*                                                                          */
/*      Conversion from Win3.1 16 bit .grp file to NT 32bit .grp files for  */
/*      the Program Manager                                                 */
/*                                                                          */
/*  Created: 10-15-92   Johanne Caron                                       */
/*                                                                          */
/****************************************************************************/
#include "convgrp.h"
#include <shellapi.h>
#include <shlapip.h>

#if DBG
void DbgPrint(char *, ...);
#define KdPrint(_x_) DbgPrint _x_
#else
#define KdPrint(_x_)
#endif

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  MyDwordAlign() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

WORD MyDwordAlign(int wStrLen)
{
    return ((WORD)((wStrLen + 3) & ~3));
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SizeofGroup() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

DWORD SizeofGroup(LPGROUPDEF lpgd)
{
    LPPMTAG lptag;
    WORD cbSeg;
    WORD cb;

    cbSeg = (WORD)GlobalSize(lpgd);

    lptag = (LPPMTAG)((LPSTR)lpgd+lpgd->cbGroup);

    if ((WORD)((PCHAR)lptag - (PCHAR)lpgd +MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb))+4) <= cbSeg
        && lptag->wID == ID_MAGIC
        && lptag->wItem == (int)0xFFFF
        && lptag->cb == (WORD)(MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)) + 4)
        && *(PLONG)lptag->rgb == PMTAG_MAGIC)
      {
        while ((cb = (WORD)((PCHAR)lptag - (PCHAR)lpgd + MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)))) <= cbSeg)
          {
            if (lptag->wID == ID_LASTTAG)
                return (DWORD)cb;
            (LPSTR)lptag += lptag->cb;
          }
      }
    return lpgd->cbGroup;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddUpGroupFile() -                                                      */
/*                                                                          */
/* Calculates the group file's checksum.                                    */
/*--------------------------------------------------------------------------*/

WORD AddUpGroupFile(LPGROUPDEF lpgd)
{
    LPINT lpW;
    LPINT save_lpW;
    DWORD wSum = 0;
    DWORD cbFile;

    cbFile = SizeofGroup(lpgd);

    for (save_lpW = lpW = (LPINT)lpgd, cbFile >>= 2; cbFile; cbFile--, lpW++)
        wSum += *lpW;

    return (WORD)((DWORD_PTR)lpW - (DWORD_PTR)save_lpW);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddThing() -                                                            */
/*                                                                          */
/* in:                                                                      */
/*	hGroup	group handle, must not be discardable                           */
/*	lpStuff	pointer to data or NULL to init data to zero                    */
/*	cbStuff	count of item (may be 0) if lpStuff is a string                 */
/*                                                                          */
/* Adds an object to the group segment and returns its offset.	Will        */
/* reallocate the segment if necessary.                                     */
/*                                                                          */
/* Handle passed in must not be discardable                                 */
/*                                                                          */
/* returns:                                                                 */
/*	0	failure                                                             */
/*	> 0	offset to thing in the segment                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

WORD AddThing(HANDLE hGroup, LPSTR lpStuff, WORD cbStuff)
{
    WORD        cb;
    LPGROUPDEF  lpgd;
    WORD        offset;
    LPSTR       lpT;
    WORD        cbStuffSize;
    WORD        cbGroupSize;
    WORD        myOffset;

    if (cbStuff == 0xFFFF) {
        return 0xFFFF;
    }

    if (!cbStuff) {
        cbStuff = (WORD)(1 + lstrlen(lpStuff));
    }

    cbStuffSize = (WORD)MyDwordAlign((int)cbStuff);

    lpgd = (LPGROUPDEF)GlobalLock(hGroup);
    cb = (WORD)SizeofGroup(lpgd);
    cbGroupSize = (WORD)MyDwordAlign((int)cb);

    offset = lpgd->cbGroup;
    myOffset = (WORD)MyDwordAlign((int)offset);

    GlobalUnlock(hGroup);

    if (!GlobalReAlloc(hGroup,(DWORD)(cbGroupSize + cbStuffSize), GMEM_MOVEABLE))
        return 0;

    lpgd = (LPGROUPDEF)GlobalLock(hGroup);

    /*
     * Slide the tags up
     */
    memmove((LPSTR)lpgd + myOffset + cbStuffSize, (LPSTR)lpgd + myOffset,
            (WORD)(cbGroupSize - myOffset));
    lpgd->cbGroup += cbStuffSize;

    lpT = (LPSTR)((LPSTR)lpgd + myOffset);
    if (lpStuff) {
        memmove(lpT, lpStuff, cbStuff);

    } else {
        /*
         * Zero it
         */
        while (cbStuffSize--) {
            *lpT++ = 0;
        }
    }

    GlobalUnlock(hGroup);

    return myOffset;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FindTag() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LPPMTAG FindTag(LPGROUPDEF lpgd, int item, WORD id)
{
    LPPMTAG lptag;
    int cbSeg;
    int cb;

    cbSeg = (int)GlobalSize(lpgd);

    lptag = (LPPMTAG)((LPSTR)lpgd+lpgd->cbGroup);

    if ((PCHAR)lptag - (PCHAR)lpgd + MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)) + 4 <= cbSeg
        && lptag->wID == ID_MAGIC
        && lptag->wItem == (int)0xFFFF
        && lptag->cb == (WORD)(MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)) +4)
        && *(LONG FAR *)lptag->rgb == PMTAG_MAGIC) {

        while ((cb = (int)((PCHAR)lptag - (PCHAR)lpgd) + MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb))) <= cbSeg)
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
/*  AddTag() -                                                              */
/*                                                                          */
/* in:                                                                      */
/*	h	group handle, must not be discardable!                              */
/*                                                                          */
/* returns:                                                                 */
/*  0	failure                                                             */
/*	1	success                                                             */
/*--------------------------------------------------------------------------*/
INT AddTag(HANDLE h, int item, WORD id, LPSTR lpbuf, int cb)
{
    LPPMTAG lptag;
    WORD fAddFirst;
    LPGROUPDEF lpgd;
    int cbNew;
    int cbMyLen;
    LPGROUPDEF lpgdOld;

    if (!cb && lpbuf) {
        cb = lstrlen(lpbuf) + 1;
    }
    cbMyLen = MyDwordAlign(cb);

    if (!lpbuf) {
        cb = 0;
        cbMyLen = 0;
    }

    lpgd = (LPGROUPDEF)GlobalLock(h);

    lptag = FindTag(lpgd, (int)0xFFFF, (WORD)ID_LASTTAG);

    if (!lptag) {
        /*
         * In this case, there are no tags at all, and we have to add
         * the first tag, the interesting tag, and the last tag
         */
        cbNew = 3 * (MyDwordAlign(sizeof(PMTAG)) - MyDwordAlign(sizeof(lptag->rgb))) + 4 + cbMyLen;

        fAddFirst = TRUE;
        lptag = (LPPMTAG)((LPSTR)lpgd + lpgd->cbGroup);

    } else {
        /*
         * In this case, only the interesting tag needs to be added
         * but we count in the last because the delta is from lptag
         */
        cbNew = 2 * (MyDwordAlign(sizeof(PMTAG)) - MyDwordAlign(sizeof(lptag->rgb))) + cbMyLen;
        fAddFirst = FALSE;
    }


    cbNew += (int)((PCHAR)lptag -(PCHAR)lpgd);
    lpgdOld = lpgd;
    GlobalUnlock(h);
    if (!GlobalReAlloc(h, (DWORD)cbNew, GMEM_MOVEABLE)) {
        return 0;
    }

    lpgd = (LPGROUPDEF)GlobalLock(h);
    lptag = (LPPMTAG)((LPSTR)lpgd + ((LPSTR)lptag - (LPSTR)lpgdOld));
    if (fAddFirst) {
        /*
         * Add the first tag
         */
        lptag->wID = ID_MAGIC;
        lptag->wItem = (int)0xFFFF;
        *(LONG FAR *)lptag->rgb = PMTAG_MAGIC;
        lptag->cb = (WORD)(MyDwordAlign(sizeof(PMTAG)) - MyDwordAlign(sizeof(lptag->rgb)) + 4);
        (LPSTR)lptag += lptag->cb;
    }

    /*
     * Add the tag
     */
    lptag->wID = id;
    lptag->wItem = item;
    lptag->cb = (WORD)(MyDwordAlign(sizeof(PMTAG)) - MyDwordAlign(sizeof(lptag->rgb)) + cbMyLen);
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
/*  AddItemIconResource() -                                                 */
/*                                                                          */
/*  Adds the icon resource to the group item. Returns TRUE if the icon      */
/*  resource was extracted ok and it was added ok.                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL AddItemIconResource(HANDLE hNewGroup, LPITEMDEF lpid, WORD iItem, LPSTR lpIconPath)
{
    LPGROUPDEF lpgd;
    LPBYTE lpIconRes = NULL;
    HANDLE hIconRes;
    HANDLE hModule;
    HICON hIcon;
    CHAR szIconExe[MAX_PATH];
    WORD id;
    WORD offset;
    DWORD OldErrorMode;

    lpid->cbIconRes = 0;

    id = lpid->indexIcon;
    lstrcpy(szIconExe, lpIconPath);
    CharLower(szIconExe);
    if (id > 7 && strstr(szIconExe, "progman")) {
        //
        // There's one more icon in the NT progman.exe than in the Win3.1
        // progman.exe and it's inserted at the 8th icon position. So if
        // the icon index is 9 in Win3.1 then it will be the 10th icon in
        // NT progman.exe, etc
        //
        id++;
    }

    hIcon = ExtractAssociatedIcon(hInst, szIconExe, &id);
    if (!hIcon) {
        goto Failed;
    }
    DestroyIcon(hIcon);
    lpid->idIcon = id;

    OldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    if (hModule = LoadLibrary(szIconExe)) {

        SetErrorMode(OldErrorMode);

            //
            // It's a 32bit .exe
            //
        hIconRes = FindResource(hModule, MAKEINTRESOURCE(id), MAKEINTRESOURCE(RT_ICON));
        if (hIconRes) {
            lpid->wIconVer = 3;  // resource version is windows 3.x
            lpid->cbIconRes = (WORD)SizeofResource(hModule, hIconRes);
            if (hIconRes = LoadResource(hModule, hIconRes))
                lpIconRes = LockResource(hIconRes);
        }
    }
    else {

        SetErrorMode(OldErrorMode);

            //
            // It's a 16bit .exe
            //
        if (lpid->wIconVer = ExtractIconResInfo(hInst,
                                                szIconExe,
                                                lpid->indexIcon,
                                                &lpid->cbIconRes,
                                                &hIconRes)){

            lpIconRes = GlobalLock(hIconRes);
        }
    }



    //
    // Add the item's Icon resource.
    //
    if (!lpid->cbIconRes) {
        goto Failed;
    }
    offset = AddThing(hNewGroup, lpIconRes, lpid->cbIconRes);

    GlobalUnlock(hIconRes);
    GlobalFree(hIconRes);

    lpgd = (LPGROUPDEF)GlobalLock(hNewGroup);
    lpid = ITEM(lpgd, iItem);
    if (!offset) {
        GlobalUnlock(hNewGroup);
        KdPrint(("ConvGrp: AddThing lpIconRes failed for item %d \n", iItem));
        goto Failed;
    }
    lpid->pIconRes = offset;

    GlobalUnlock(hNewGroup);
    return(TRUE);

Failed:
    KdPrint(("ConvGrp: AddItemIconResource failed to extract icon for item %d \n", iItem));
    return(FALSE);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CreateNewGroup() -                                                      */
/*                                                                          */
/*  This function creates a new, empty group.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

HANDLE CreateNewGroup(LPGROUPDEF16 lpGroup16)
{
    HANDLE      hT;
    LPGROUPDEF  lpgd;
    int         i;
    int         cb;
    int         cItems;          // number of items in 16bit group
    LPSTR       pGroupName;      // 16bit group name
    INT         wGroupNameLen;   //length of pGroupName DWORD aligned.

    pGroupName = PTR(lpGroup16, lpGroup16->pName);
    wGroupNameLen = MyDwordAlign(lstrlen(pGroupName) + 1);
    cItems = lpGroup16->cItems;
    cb = sizeof(GROUPDEF) + (cItems * sizeof(WORD)) +  wGroupNameLen;

    //
    // In CreateNewGroup before GlobalAlloc.
    //
    hT = GlobalAlloc(GHND, (DWORD)cb);
    if (!hT) {
        return NULL;
    }

    lpgd = (LPGROUPDEF)GlobalLock(hT);

    //
    // use the 16bit .grp file settings for what we can.
    //
    lpgd->nCmdShow = lpGroup16->nCmdShow;
    lpgd->wIconFormat = lpGroup16->wIconFormat;
    lpgd->cxIcon = lpGroup16->cxIcon;
    lpgd->cyIcon = lpGroup16->cyIcon;
    lpgd->ptMin.x = (INT)lpGroup16->ptMin.x;
    lpgd->ptMin.y = (INT)lpGroup16->ptMin.y;
    SetRect(&(lpgd->rcNormal),
            (INT)lpGroup16->rcNormal.Left,
            (INT)lpGroup16->rcNormal.Top,
            (INT)lpGroup16->rcNormal.Right,
            (INT)lpGroup16->rcNormal.Bottom);


    lpgd->dwMagic = GROUP_MAGIC;
    lpgd->wCheckSum = 0;           /* adjusted later... */
    lpgd->cbGroup = (WORD)cb;
    lpgd->pName = sizeof(GROUPDEF) + cItems * sizeof(WORD);

    lpgd->cItems = (WORD)cItems;

    for (i = 0; i < cItems; i++) {
        lpgd->rgiItems[i] = 0;
    }

    lstrcpy((LPSTR)lpgd + sizeof(GROUPDEF) + cItems * sizeof(WORD),
            pGroupName);

    lpgd->wCheckSum = -(AddUpGroupFile(lpgd));

    GlobalUnlock(hT);
    return(hT);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Create32bitGroupFormat() -                                              */
/*
/*  returns the size of the new 32bit group.
/*                                                                          */
/*--------------------------------------------------------------------------*/

int Create32bitGroupFormat(LPGROUPDEF16 lpGroup16,
                           int cbGroup16Size,
                           LPHANDLE lphNewGroup)
{
    HANDLE hNewGroup;
    LPGROUPDEF lpgd;
    LPITEMDEF lpid;
    LPBYTE lpid16;
    LPBYTE lptag16;
    LPSTR lpTagValue;
    WORD wTagId;
    LPSTR lpT;
    WORD offset;
    int cb;
    int i;

    hNewGroup = CreateNewGroup(lpGroup16);
    if (!hNewGroup) {
        return(0);
    }

    //
    // Add all items to the new formatted group.
    //
    for (i = 0; (i < (int)lpGroup16->cItems) && (i < CITEMSMAX); i++) {

        //
        // Get the pointer to the 16bit item
        //
        lpid16 = ITEM16(lpGroup16, i);

        //
        // Create the item.
        //
        offset = AddThing(hNewGroup, NULL, sizeof(ITEMDEF));
        if (!offset) {
            KdPrint(("ConvGrp: Addthing ITEMDEF failed for item %d \n", i));
            goto QuitThis;
        }

        lpgd = (LPGROUPDEF)GlobalLock(hNewGroup);

        lpgd->rgiItems[i] = offset;
        lpid = ITEM(lpgd, i);

        //
        // Set the item's position.
        //
        lpid->pt.x = *(LPWORD)lpid16;
        lpid->pt.y = *((LPWORD)lpid16 + 1);

        //
        // Add the item's Name.
        //
        GlobalUnlock(hNewGroup);
        lpT = PTR(lpGroup16, *((LPWORD)lpid16 + 9));
        offset = AddThing(hNewGroup, lpT, 0);
        if (!offset) {
            KdPrint(("ConvGrp: Addthing pName failed for item %d \n", i));
            goto PuntCreation;
        }
        lpgd = (LPGROUPDEF)GlobalLock(hNewGroup);
        lpid = ITEM(lpgd, i);
        lpid->pName = offset;

        //
        // Add the item's Command line.
        //
        GlobalUnlock(hNewGroup);
        lpT = PTR(lpGroup16, *((LPWORD)lpid16 + 10));
        offset = AddThing(hNewGroup, lpT, 0);
        if (!offset) {
            KdPrint(("ConvGrp: Addthing pCommand failed for item %d \n", i));
            goto PuntCreation;
        }
        lpgd = (LPGROUPDEF)GlobalLock(hNewGroup);
        lpid = ITEM(lpgd, i);
        lpid->pCommand = offset;

        //
        // Add the item's Icon path.
        //
        GlobalUnlock(hNewGroup);
        lpT = PTR(lpGroup16, *((LPWORD)lpid16 + 11));
        offset = AddThing(hNewGroup, lpT, 0);
        if (!offset) {
            KdPrint(("ConvGrp: Addthing pIconPath failed for item %d \n", i));
            goto PuntCreation;
        }
        lpgd = (LPGROUPDEF)GlobalLock(hNewGroup);
        lpid = ITEM(lpgd, i);
        lpid->pIconPath = offset;

        //
        // Get the item's icon resource using the Icon path and the icon index.
        // And add the item's Icon resource.
        //
        GlobalUnlock(hNewGroup);
        lpid->indexIcon = *((LPWORD)lpid16 + 2);
        if (!AddItemIconResource(hNewGroup, lpid, (WORD)i, lpT)) {
            KdPrint(("ConvGrp: AddItemIconResource failed for item %d \n", i));
            goto PuntCreation;
        }

    }

    /*
     * Copy all the tags to the new group format.
     */
    lptag16 = (LPSTR)lpGroup16 + lpGroup16->cbGroup;

    if (*(UNALIGNED WORD *)lptag16 == ID_MAGIC &&
        *((UNALIGNED WORD *)(lptag16+1)) == 0xFFFF &&
        *(UNALIGNED DWORD *)((UNALIGNED WORD *)(lptag16+3)) == PMTAG_MAGIC) {

        //
        // This is the first tag id, goto start of item tags.
        //
        lptag16 += *((LPWORD)lptag16+2);

        while (*(LPWORD)lptag16 != ID_LASTTAG) {

            wTagId = *(LPWORD)lptag16;
            if (wTagId == ID_MINIMIZE) {
                lpTagValue = NULL;
            }
            else {
                lpTagValue = (LPSTR)((LPWORD)lptag16 + 3);
            }

            if (! AddTag( hNewGroup,
                          (int)*((LPWORD)lptag16 + 1),   // wItem
                          wTagId,                        // wID
                          lpTagValue,                    // rgb : tag value
                          *((LPWORD)lptag16 + 2) - (3 * sizeof(WORD))  // cb - sizeof tag
                        )) {

                KdPrint(("ConvGrp: AddTag wItem=%d, wID=%d failed \n",
                              *((LPWORD)lptag16 + 1),
                              *(LPWORD)lptag16));
            }

            lptag16 += *((LPWORD)lptag16 + 2);      //  go to next tag
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


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ReadGroup() -                                                           */
/*                                                                          */
/*  Read in the 16bit group file.                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

int ReadGroup(LPSTR pszPath, LPHANDLE lphGroup)

{
    HANDLE     hGroup;
    LPBYTE     lpgd;
    int        cbGroup;
    int        fh;
    PSTR       psz;


    //
    // Find and open the group file.
    //
    fh = _open(pszPath, O_RDONLY | O_BINARY);
    if (fh == -1) {
        KdPrint(("ConvGrp: Could NOT open file %s \n", pszPath));
        fprintf(stderr, "ConvGrp: Could NOT open file %s \n", pszPath);
        goto LGError1;
    }

    //
    // Find the size of the file by seeking to the end.
    //
    cbGroup = (WORD)_lseek(fh, 0L, SEEK_END);
    if (cbGroup < sizeof(GROUPDEF)) {
        KdPrint(("ConvGrp: bad group file - %s\n", pszPath));
        fprintf(stderr, "ConvGrp: bad group file - %s\n", pszPath);
        goto LGError2;
    }

    _lseek(fh, 0L, SEEK_SET);

    //
    // Allocate some memory for the thing.
    //
    if (!(hGroup = GlobalAlloc(GMEM_MOVEABLE|GMEM_DISCARDABLE, (DWORD)cbGroup))) {
        KdPrint(("ConvGrp: Alloc failed for input file %s\n", pszPath));
        psz = NULL;
        goto LGError2;
    }

    lpgd = (LPBYTE)GlobalLock(hGroup);

    //
    // Read the whole group file into memory.
    //
    if (_read(fh, (PSTR)lpgd, cbGroup) != cbGroup) {
        fprintf(stderr, "ConvGrp: Could NOT read file %s\n", pszPath);
        goto LGError3;
    }

    //
    // Validate the group file by checking the magic bytes and the checksum.
    //
    if (*((LPWORD)lpgd + 3) > (WORD)cbGroup) {
        fprintf(stderr, "ConvGrp: Invalid group file - %s\n", pszPath);
        goto LGError3;
    }

    if (*(LPDWORD)lpgd != GROUP_MAGIC) {
        fprintf(stderr, "ConvGrp: Invalid group file - %s\n", pszPath);
        goto LGError3;
    }

    //
    // Test if this is an NT .grp file
    //

    if ( (((LPGROUPDEF)lpgd)->rcNormal.left == (INT)(SHORT)((LPGROUPDEF)lpgd)->rcNormal.left) &&
         (((LPGROUPDEF)lpgd)->rcNormal.right == (INT)(SHORT)((LPGROUPDEF)lpgd)->rcNormal.right) &&
         (((LPGROUPDEF)lpgd)->rcNormal.top == (INT)(SHORT)((LPGROUPDEF)lpgd)->rcNormal.top) &&
         (((LPGROUPDEF)lpgd)->rcNormal.bottom == (INT)(SHORT)((LPGROUPDEF)lpgd)->rcNormal.bottom) ){

        //
        // it's an NT .grp file, not valid for conversion
        //
        fprintf(stderr, "ConvGrp: Invalid group file - %s\n", pszPath);
        goto LGError3;
    }

    _close(fh);


    GlobalUnlock(hGroup);
    *lphGroup = hGroup;
    return(cbGroup);

LGError3:
    GlobalUnlock(hGroup);
    GlobalDiscard(hGroup);

LGError2:
    _close(fh);

LGError1:
    *lphGroup = NULL;
    return(0);
}

#define S_IREAD     0000400         /* read permission, owner */
#define S_IWRITE    0000200         /* write permission, owner */

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Write32bitGroup() -                                                           */
/*                                                                          */
/*  Write out the 32bit group file.                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL Write32bitGroup(LPGROUPDEF lpgd, int cbGroup, LPSTR pszPath)
{
    int fh;
    DWORD Error = 0;


    fh = _open(pszPath, O_CREAT | O_WRONLY | O_BINARY, S_IREAD | S_IWRITE);
    if (fh == -1) {
        Error = GetLastError();
        fprintf(stderr, "ConvGrp: Could NOT open output file - %s\n", pszPath);
        goto Exit1;
    }

    if (_write(fh, (PSTR)lpgd, cbGroup) != (int)cbGroup) {
        Error = GetLastError();
        fprintf(stderr, "ConvGrp: Could NOT write to output file - %s\n", pszPath);
    }

    _write(fh, NULL, 0);        // truncate if getting smaller

    _close(fh);

Exit1:

    if (Error) {
        KdPrint(("         Error = %d \n", Error));
    }
    return (Error == 0);
}


int __cdecl main(
    int argc,
    char *argv[],
    char *envp[])
{
    HANDLE h16bitGroup;
    HANDLE h32bitGroup;
    LPGROUPDEF16 lp16bitGroup;
    LPGROUPDEF lp32bitGroup;
    LPSTR lp16bitGroupFile;
    LPSTR lp32bitGroupFile;
    int cbGroup;
    BOOL bRet = FALSE;

    hInst = GetModuleHandle(NULL);

    //
    // We need the name of the 16bit .grp file and the name of the
    // 32bit .grp file. The first being the 16bit .grp file
    if (argc != 3) {
       fprintf(stderr, "ConvGrp: Invalid number of paramters, should have 2 filenames\n");
       fprintf(stderr, "\nusage: convgrp <Win3.1 .grp filename> <NT .grp filename>\n");
       return(FALSE);
    }

    //
    // The first argument is the name of the 16bit group file,
    // the second argument is the filename for the 32bit group file.
    //
    //
    lp16bitGroupFile = argv[1];
    lp32bitGroupFile = argv[2];

    cbGroup = ReadGroup(lp16bitGroupFile, &h16bitGroup);
    if (!cbGroup) {
        return(FALSE);
    }

    if (!(lp16bitGroup = (LPGROUPDEF16)GlobalLock(h16bitGroup))) {
        KdPrint(("ConvGrp: GlobalLock failed on %s\n", "h16bitGroup"));
        goto Exit;;
    }

    cbGroup = Create32bitGroupFormat(lp16bitGroup, cbGroup, &h32bitGroup);

    if (cbGroup) {
        lp32bitGroup = (LPGROUPDEF)GlobalLock(h32bitGroup);
        bRet = Write32bitGroup(lp32bitGroup, cbGroup, lp32bitGroupFile);
        GlobalUnlock(h32bitGroup);
        GlobalFree(h32bitGroup);
    }

Exit:

    if (h16bitGroup) {
        GlobalFree(h16bitGroup);
    }

    if (bRet) {
       fprintf(stderr, "ConvGrp: group successfully converted\n");
    }
    return(bRet);
}
