/****************************************************************************/
/*                                                                          */
/*  CONVGRP.C -                                                             */
/*                                                                          */
/*      Conversion from Windows NT 1.0 program group format with ANSI       */
/*      strings to Windows NT 1.0a group format with UNICODE strings.       */
/*                                                                          */
/*  Created: 09-10-93   Johanne Caron                                       */
/*                                                                          */
/****************************************************************************/
#include "progman.h"
#include "convgrp.h"

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CreateNewGroupFromAnsiGroup() -                                                      */
/*                                                                          */
/*  This function creates a new, empty group.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

HANDLE CreateNewGroupFromAnsiGroup(LPGROUPDEF_A lpGroupORI)
{
    HANDLE      hT;
    LPGROUPDEF  lpgd;
    int         i;
    int         cb;
    int         cItems;          // number of items in 16bit group
    LPSTR       pGroupName;      // 32bit group name
    LPTSTR      pGroupNameUNI = NULL;   // 32bit UNICODE group name
    INT         wGroupNameLen;   // length of pGroupName DWORD aligned.
    INT		cchWideChar = 0; //character count of resultant unicode string
    INT		cchMultiByte = 0;

    pGroupName = (LPSTR)PTR(lpGroupORI, lpGroupORI->pName);

    //
    // convert pGroupName to unicode here
    //
    cchMultiByte=MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,pGroupName,
            -1,pGroupNameUNI,cchWideChar) ;

    pGroupNameUNI = LocalAlloc(LPTR,(++cchMultiByte)*sizeof(TCHAR)) ;

    MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,pGroupName,
            -1,pGroupNameUNI,cchMultiByte) ;


    wGroupNameLen = MyDwordAlign(sizeof(TCHAR)*(lstrlen(pGroupNameUNI) + 1));
    cItems = lpGroupORI->cItems;
    cb = sizeof(GROUPDEF) + (cItems * sizeof(DWORD)) +  wGroupNameLen;

    //
    // In CreateNewGroup before GlobalAlloc.
    //
    hT = GlobalAlloc(GHND, (DWORD)cb);
    if (!hT) {
        goto Exit;
    }

    lpgd = (LPGROUPDEF)GlobalLock(hT);

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
    lpgd->pName = sizeof(GROUPDEF) + cItems * sizeof(DWORD);

    lpgd->Reserved1 = (WORD)-1;
    lpgd->Reserved2 = (DWORD)-1;

    lpgd->cItems = (WORD)cItems;

    for (i = 0; i < cItems; i++) {
        lpgd->rgiItems[i] = 0;
    }

    lstrcpy((LPTSTR)((LPSTR)lpgd + sizeof(GROUPDEF) + cItems * sizeof(DWORD)),
            pGroupNameUNI); // lhb tracks
    LocalFree(pGroupNameUNI);

    GlobalUnlock(hT);
    return(hT);

Exit:
    return NULL;
}

DWORD AddThing_A(HANDLE hGroup, LPSTR lpStuff, WORD cbStuff)
{
    LPTSTR      lpStuffUNI = NULL;
    BOOL	bAlloc = FALSE;
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

        lpStuffUNI = LocalAlloc(LPTR,(++cchMultiByte)*sizeof(TCHAR)) ;

        MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,lpStuff,
            -1,lpStuffUNI,cchMultiByte) ;

        cbStuff = (WORD)sizeof(TCHAR)*(1 + lstrlen(lpStuffUNI)); // lhb tracks
    } else {
        lpStuffUNI = (LPTSTR)lpStuff;
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

int ConvertToUnicodeGroup(LPGROUPDEF_A lpGroupORI, LPHANDLE lphNewGroup)
{
    HANDLE hNewGroup;
    LPGROUPDEF lpgd;
    LPITEMDEF lpid;
    LPBYTE lpid_A;
    LPPMTAG lptag_A;
    LPSTR lpTagValue;
    WORD wTagId;
    LPSTR lpT;	
    DWORD offset;
    int cb;
    int i;
    INT cchMultiByte;
    INT cchWideChar;
    LPTSTR lpTagValueUNI;
    BOOL bAlloc = FALSE;

    hNewGroup = CreateNewGroupFromAnsiGroup(lpGroupORI);
    if (!hNewGroup) {
        return(0);
    }

    //
    // Add all items to the new formatted group.
    //
    for (i = 0; (i < (int)lpGroupORI->cItems) && (i < CITEMSMAX); i++) {

      //
      // Get the pointer to the 16bit item
      //
      lpid_A = (LPBYTE)ITEM(lpGroupORI, i);
      if (lpGroupORI->rgiItems[i]) {

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
        lpid->pt.x = ((LPITEMDEF_A)lpid_A)->pt.x;
        lpid->pt.y = ((LPITEMDEF_A)lpid_A)->pt.y;

        //
        // Add the item's Name.
        //
        GlobalUnlock(hNewGroup);
        lpT = (LPSTR)PTR(lpGroupORI,((LPITEMDEF_A)lpid_A)->pName);

        offset = AddThing_A(hNewGroup, lpT, 0);
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
        lpT = (LPSTR)PTR(lpGroupORI, ((LPITEMDEF_A)lpid_A)->pCommand);
        offset = AddThing_A(hNewGroup, lpT, 0);
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
        lpT = (LPSTR)PTR(lpGroupORI, ((LPITEMDEF_A)lpid_A)->pIconPath);
        offset = AddThing_A(hNewGroup, lpT, 0);
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
        lpid->iIcon    = ((LPITEMDEF_A)lpid_A)->idIcon;
	    lpid->cbIconRes = ((LPITEMDEF_A)lpid_A)->cbIconRes;
	    lpid->wIconVer  = ((LPITEMDEF_A)lpid_A)->wIconVer;
        GlobalUnlock(hNewGroup);

        lpT = (LPBYTE)PTR(lpGroupORI, ((LPITEMDEF_A)lpid_A)->pIconRes);
        offset = AddThing_A(hNewGroup, (LPSTR)lpT, lpid->cbIconRes);
        if (!offset) {
            KdPrint(("ConvGrp: AddThing pIconRes failed for item %d \n", i));
            goto PuntCreation;
        }
        lpgd = (LPGROUPDEF)GlobalLock(hNewGroup);
        lpid = ITEM(lpgd, i);
        lpid->pIconRes = offset;

        GlobalUnlock(hNewGroup);

      }
    }

    /*
     * Copy all the tags to the new group format.
     */
    lptag_A = (LPPMTAG)((LPSTR)lpGroupORI + lpGroupORI->cbGroup); // lhb tracks

    if (lptag_A->wID == ID_MAGIC &&
        lptag_A->wItem == (int)0xFFFF &&
        *(LONG FAR *)lptag_A->rgb == PMTAG_MAGIC) {

        //
        // This is the first tag id, goto start of item tags.
        //
        (LPBYTE)lptag_A += lptag_A->cb;

        while (lptag_A->wID != ID_LASTTAG) {

            wTagId = lptag_A->wID;
            cb = lptag_A->cb  - (3 * sizeof(DWORD)); // cb - sizeof tag

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
                                        -1,lpTagValueUNI,cchWideChar) ;

                    lpTagValueUNI = LocalAlloc(LPTR,(++cchMultiByte)*sizeof(TCHAR)) ;

                    MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,lpTagValue,
                                        -1,lpTagValueUNI,cchMultiByte) ;
                    cb = sizeof(TCHAR)*(lstrlen(lpTagValueUNI) + 1); // lhb tracks
                }
                else {
                    lpTagValueUNI = (LPTSTR)lpTagValue;
                }
            }

            if (! AddTag( hNewGroup,
                          lptag_A->wItem,   // wItem
                          wTagId,              // wID
                          lpTagValueUNI,          // rgb : tag value
                          cb
                        )) {

                KdPrint(("ConvGrp: AddTag wItem=%d, wID=%d failed \n",
                              lptag_A->wItem ,
                              lptag_A->wID));
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
