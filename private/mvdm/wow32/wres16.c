/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WRES16.C
 *  WOW32 16-bit resource support
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

//
//  BUGBUG: moved macros from mvdm.h and wo32.h
//  as they are not what they appear to be.
//  Watch out these macros increment the pointer arguments!!!!
//  02-Feb-1994 Jonle
//
#define VALIDPUT(p)      ((UINT)p>65535)
#define PUTWORD(p,w)     {if (VALIDPUT(p)) *(PWORD)p=w; ((PWORD)p)++; }
#define PUTDWORD(p,d)    {if (VALIDPUT(p)) *(PDWORD)p=d;((PDWORD)p)++;}
#define PUTUDWORD(p,d)   {if (VALIDPUT(p)) *(DWORD UNALIGNED *)p=d;((DWORD UNALIGNED *)p)++;}
#define GETWORD(pb)      (*((UNALIGNED WORD *)pb)++)
#define GETDWORD(pb)     (*((UNALIGNED DWORD *)pb)++)

#define ADVGET(p,i)      {(UINT)p+=i;}
#define ADVPUT(p,i)      {(UINT)p+=i;}
#define ALIGNWORD(p)     {(UINT)p+=( ((UINT)p)&(sizeof(WORD)-1));}
#define ALIGNDWORD(p)    {(UINT)p+=(-((INT)p)&(sizeof(DWORD)-1));}


MODNAME(wres16.c);

PRES presFirst;     // pointer to first RES entry

#ifdef DEBUG

typedef struct _RTINFO {    /* rt */
    LPSTR lpType;       // predefined resource type
    PSZ   pszName;      // name of type
} RTINFO, *PRTINFO;

RTINFO artInfo[] = {
   {RT_CURSOR,      "CURSOR"},
   {RT_BITMAP,      "BITMAP"},
   {RT_ICON,        "ICON"},
   {RT_MENU,        "MENU"},
   {RT_DIALOG,      "DIALOG"},
   {RT_STRING,      "STRING"},
   {RT_FONTDIR,     "FONTDIR"},
   {RT_FONT,        "FONT"},
   {RT_ACCELERATOR, "ACCELERATOR"},
   {RT_RCDATA,      "RCDATA"},
   {RT_MESSAGETABLE,"MESSAGETABLE"},
   {RT_GROUP_CURSOR,"CURSOR DIRECTORY"},
   {RT_GROUP_ICON,  "ICON DIRECTORY"},
};

PSZ GetResourceType(LPSZ lpszType)
{
    INT i;
    register PRTINFO prt;

    if (HIWORD(lpszType) != 0)
    return lpszType;
    for (prt=artInfo,i=NUMEL(artInfo); i>0; i--,prt++)
    if (prt->lpType == lpszType)
        return prt->pszName;
    return "UNKNOWN";
}

#endif


/* Resource management functions
 */

PRES AddRes16(HMOD16 hmod16, WORD wExeVer, HRESI16 hresinfo16, LPSZ lpszType)
{
    register PRES pres;

    if (pres = malloc_w(sizeof(RES))) {

        // Initialize the structure
        pres->hmod16      = hmod16;
        pres->wExeVer     = wExeVer;
        pres->flState     = 0;
        pres->hresinfo16  = hresinfo16;
        pres->hresdata16  = 0;
        pres->lpszResType = lpszType;
        pres->pbResData   = NULL;

        // And then link it in
        pres->presNext    = presFirst;
        presFirst = pres;
        return pres;
    }
    return NULL;
}


VOID FreeRes16(PRES presFree)
{
    register PRES pres, presPrev;

    presPrev = (PRES)(&presFirst);
    while (pres = presPrev->presNext) {
        if (pres == presFree)
            break;
        presPrev = pres;
    }

    // Changed from WOW32ASSERT by cmjones  11/3/97
    // This might be a bogus warning in that USER32!SplFreeResource() calls
    // W32FreeResource() twice on certain types of resources.  The Warning
    // might be raised on the 2nd call after the resource was just freed.
    // The only known occurances are at the start of the Winstone '94 
    // Quattro Pro test.  This can safely be ignored if you see SplFreeResource
    // in the stack dump.
    WOW32WARNMSG((pres),("WOW::FreeRes16:Possible lost resource.\n"));

    if (pres) {
        presPrev->presNext = pres->presNext;
        if (pres->pbResData)
            UnlockResource16(pres);
        free_w(pres);
    }
}


VOID DestroyRes16(HMOD16 hmod16)
{
    register PRES pres, presPrev;

    presPrev = (PRES)(&presFirst);
    while (pres = presPrev->presNext) {
        if (pres->hmod16 == hmod16) {

            LOGDEBUG(5,("Freeing resource info for current terminating task\n"));

            // Now basically do a FreeRes16
            presPrev->presNext = pres->presNext;
            if (pres->pbResData)
                UnlockResource16(pres);
            free_w(pres);
        } else {
            presPrev = pres;
        }
    }
}


PRES FindResource16(HMOD16 hmod16, LPSZ lpszName, LPSZ lpszType)
{
    INT cb;
    PRES pres = NULL;
    VPVOID vp=0;
    PARM16 Parm16;
    VPSZ vpszName = 0, vpszType = 0;
    WORD wExpWinVer;

    if (HIWORD(lpszName) == 0) {
        vpszName = (VPSZ)lpszName;
        LOGDEBUG(5,("    Finding resource %lx, type %s(%lx)\n",
                 lpszName, GetResourceType(lpszType), lpszType));
    } else {
#ifdef FE_SB
        if (CURRENTPTD()->dwWOWCompatFlags2 & WOWCF_ARIRANG20_PRNDLG) {
       /*
        * In case of Korean Arirang2.0 word processor, it use wrong dialog ID
        * for Print or Print setup dialog. See dialog box ID on awpfont.dll.
        */
            if (!WOW32_strcmp(lpszName, "PRINTDLGTEMP"))
                vpszName = (VPSZ) 2;
            else if(!WOW32_strcmp(lpszName, "PRNSETUPDLGTEMP"))
                vpszName = (VPSZ) 1;
            else goto NOT_ARIRANG20;
        } else {  // original code
NOT_ARIRANG20:
#endif
        cb = strlen(lpszName)+1;
        if (vpszName = GlobalAllocLock16(GMEM_MOVEABLE, cb, NULL))
            putstr16(vpszName, lpszName, cb);
        LOGDEBUG(5,("    Finding resource \"%s\", type %s(%lx)\n",
                 lpszName, GetResourceType(lpszType), lpszType));
#ifdef FE_SB
        }
#endif
    }

    if (vpszName) {
        if (HIWORD(lpszType) == 0) {    // predefined resource
            vpszType = (VPSZ)lpszType;  // no doubt from MAKEINTRESOURCE
        } else {
            cb = strlen(lpszType)+1;
            if (vpszType = GlobalAllocLock16(GMEM_MOVEABLE, cb, NULL)) {
                putstr16(vpszType, lpszType, cb);
            }
        }
        if (vpszType) {
            Parm16.WndProc.wParam = hmod16;
            Parm16.WndProc.lParam = vpszName;
            Parm16.WndProc.wMsg = LOWORD(vpszType);
            Parm16.WndProc.hwnd = HIWORD(vpszType);
            CallBack16(RET_FINDRESOURCE, &Parm16, 0, &vp);
            wExpWinVer = LOWORD(Parm16.WndProc.lParam);
            if (HIWORD(vpszType))
                GlobalUnlockFree16(vpszType);
        }
        if (HIWORD(vpszName))
            GlobalUnlockFree16(vpszName);
    }

    if ((HRESI16)vp) {
        pres = AddRes16(hmod16,wExpWinVer,(HRESI16)vp, lpszType);
    }
    return pres;
}


PRES LoadResource16(HMOD16 hmod16, PRES pres)
{
    VPVOID vp=0;
    PARM16 Parm16;

    DBG_UNREFERENCED_PARAMETER(hmod16);
    WOW32ASSERT(pres && hmod16 == pres->hmod16);

    Parm16.WndProc.wParam = pres->hmod16;
    Parm16.WndProc.lParam = pres->hresinfo16;

    CallBack16(RET_LOADRESOURCE, &Parm16, 0, &vp);

    if (pres->hresdata16 = (HRESD16)vp)
        return pres;

    // BUGBUG -- On a LoadResource failure, WIN32 is not required to do a
    // corresponding FreeResource, so our RES structure will hang around until
    // task termination clean-up (which may be OK) -JTP
    return NULL;
}


BOOL FreeResource16(PRES pres)
{
    VPVOID vp=0;
    PARM16 Parm16;

    WOW32ASSERT(pres);

    Parm16.WndProc.wParam = pres->hresdata16;
    CallBack16(RET_FREERESOURCE, &Parm16, 0, &vp);

    FreeRes16(pres);

    return (BOOL)vp;
}


LPBYTE LockResource16(register PRES pres)
{
    DWORD cb, cb16;
    VPVOID vp=0;
    PARM16 Parm16;
    WOW32ASSERT(pres);

    Parm16.WndProc.wParam = pres->hresdata16;
    CallBack16(RET_LOCKRESOURCE, &Parm16, 0, &vp);

    if (vp) {

        // Get size of 16-bit resource
        cb16 = Parm16.WndProc.lParam;

        LOGDEBUG(5,("    Locking/converting resource type %s(%lx)\n",
             GetResourceType(pres->lpszResType), pres->lpszResType));

        // Handle known resource types here
        if (pres->lpszResType) {

            switch((INT)pres->lpszResType) {


            case (INT)RT_MENU:
            //    cb = ConvertMenu16(pres->wExeVer, NULL, vp, cb, cb16);
                cb = cb16 * sizeof(WCHAR);    // see SizeofResource16
                if (cb && (pres->pbResData = malloc_w(cb)))
                    ConvertMenu16(pres->wExeVer, pres->pbResData, vp, cb, cb16);
                return pres->pbResData;

            case (INT)RT_DIALOG:
             //   cb = ConvertDialog16(NULL, vp, cb, cb16);
                cb = cb16 * sizeof(WCHAR);    // see SizeofResource16
                if (cb && (pres->pbResData = malloc_w(cb)))
                    ConvertDialog16(pres->pbResData, vp, cb, cb16);
                return pres->pbResData;

            case (INT)RT_ACCELERATOR:
                WOW32ASSERT(FALSE); // never should we come here.
                return NULL;

//            case (INT)RT_GROUP_CURSOR:
//            case (INT)RT_GROUP_ICON:
//            GETOPTPTR(vp, 0, lp);
//            return lp;
            }
        }

        // If we're still here, get desperate and return a simple 32-bit alias
        GETVDMPTR(vp, cb16, pres->pbResData);
        pres->flState |= RES_ALIASPTR;
        return pres->pbResData;
    }
    // If we're still here, nothing worked
    return NULL;
}


BOOL UnlockResource16(PRES pres)
{
    VPVOID vp=0;
    PARM16 Parm16;

    WOW32ASSERT(pres);

    Parm16.WndProc.wParam = pres->hresdata16;
    CallBack16(RET_UNLOCKRESOURCE, &Parm16, 0, &vp);

    if (pres->pbResData && !(pres->flState & RES_ALIASPTR))
        free_w(pres->pbResData);
    pres->pbResData = NULL;
    pres->flState &= ~RES_ALIASPTR;

    return (BOOL)vp;
}


DWORD SizeofResource16(HMOD16 hmod16, PRES pres)
{
    VPVOID vp=0;
    DWORD cbData;
    PARM16 Parm16;

    DBG_UNREFERENCED_PARAMETER(hmod16);

    WOW32ASSERT(pres && hmod16 == pres->hmod16);

    Parm16.WndProc.wParam = pres->hmod16;
    Parm16.WndProc.lParam = pres->hresinfo16;

    CallBack16(RET_SIZEOFRESOURCE, &Parm16, 0, &vp);

    cbData = (DWORD)vp;

    /*
     * Adjust the size of the resource if they are different
     * between NT and Windows
     */
    // Handle known resource types here
    if (pres->lpszResType) {

        switch((INT)pres->lpszResType) {

        case (INT)RT_MENU:
        case (INT)RT_DIALOG:

// If we need an exact count then we would have to enable this code
// but currently the count is only used in USER to alloc enough space
// in the client\server transition windows.
// WARNING - if this code is re-enabled you must also change LockResource16
//                CallBack16(RET_LOADRESOURCE, &Parm16, 0, &vpResLoad);
//                CallBack16(RET_LOCKRESOURCE, vpResLoad, 0, &vp);
//                if ((INT)pres->lpszResType == RT_MENU)
//                    cbData = (DWORD)ConvertMenu16(pres->wExeVer, NULL, vp, cbData);
//                else
//                    cbData = (DWORD)ConvertDialog16(NULL, vp, cbData);
//                CallBack16(RET_UNLOCKRESOURCE, &Parm16, 0, &vp);

            cbData = (DWORD)((DWORD)vp * sizeof(WCHAR));
            break;

        case (INT)RT_STRING:
            cbData = (DWORD)((DWORD)vp * sizeof(WCHAR));
            break;
        }
    }

    return cbData;
}

/*
 * ConvertMenu16
 *
 * If pmenu32 is NULL then its just a size query
 *
 * Returns the number of bytes in the CONVERTED menu
 */

DWORD ConvertMenu16(WORD wExeVer, PBYTE pmenu32, VPBYTE vpmenu16, DWORD cb, DWORD cb16)
{
    WORD wVer, wOffset;
    PBYTE pmenu16, pmenu16Save;
    PBYTE pmenu32T = pmenu32;

    pmenu16 = GETVDMPTR(vpmenu16, cb16, pmenu16Save);
    wVer = 0;
    if (wExeVer >= 0x300)
        wVer = GETWORD(pmenu16);
    PUTWORD(pmenu32, wVer);         // transfer versionNumber
    wOffset = 0;
    if (wExeVer >= 0x300)
        wOffset = GETWORD(pmenu16);
    PUTWORD(pmenu32, wOffset);      // transfer offset
    ADVGET(pmenu16, wOffset);       // and advance by offset
    ADVPUT(pmenu32, wOffset);
    ALIGNWORD(pmenu32);             // this is the DIFFERENCE for WIN32
    cb = pmenu32 - pmenu32T;        // pmenu32 will == 4 for size queries
    cb += ConvertMenuItems16(wExeVer, &pmenu32, &pmenu16, vpmenu16+(pmenu16 - pmenu16Save));

    FREEVDMPTR(pmenu16Save);
    RETURN(cb);
}



/*
 * ConvertMenuItems16
 *
 * Returns the number of bytes in the CONVERTED menu
 * Note: This can be called with ppmenu32==4 which means the caller is looking
 *       for the size to allocate for the 32-bit menu structure.
 */

DWORD ConvertMenuItems16(WORD wExeVer, PPBYTE ppmenu32, PPBYTE ppmenu16, VPBYTE vpmenu16)
{
    INT cbAnsi;
    DWORD cbTotal = 0;
    UINT cbUni;
    WORD wOption, wID;
    PBYTE pmenu32 = *ppmenu32;
    PBYTE pmenu16 = *ppmenu16;
    PBYTE pmenu16T = pmenu16;
    PBYTE pmenu32T = pmenu32;

    do {
        if (wExeVer < 0x300)
            wOption = GETBYTE(pmenu16);
        else
            wOption = GETWORD(pmenu16);
        PUTWORD(pmenu32, wOption);           // transfer mtOption
        if (!(wOption & MF_POPUP)) {
            wID = GETWORD(pmenu16);
            PUTWORD(pmenu32, wID);           // transfer mtID
        }
        cbAnsi = strlen(pmenu16)+1;

        // If this is an ownerdraw menu don't copy the ANSI memu string to
        // Unicode. Put a 16:16 pointer into the 32-bit resource which
        // points to menu string instead.  User will place this pointer in 
        // MEASUREITEMSTRUCT->itemData before sending WM_MEASUREITEM.  If it's a
        // NULL string User will place a NULL in MEASUREITEMSTRUCT->itemData.
        // Chess Master and Mavis Beacon Teaches Typing depend on this.
        if ((wOption & MFT_OWNERDRAW) && *pmenu16) {
            if (VALIDPUT(pmenu32)) {
                *(DWORD UNALIGNED *)pmenu32 = vpmenu16 + (pmenu16 - pmenu16T);
            }
            cbUni = sizeof(DWORD);
        }
        else {
            if (VALIDPUT(pmenu32)) {
                RtlMultiByteToUnicodeN((LPWSTR)pmenu32, MAXULONG, (PULONG)&cbUni, pmenu16, cbAnsi);

            } 
            else {
                cbUni = cbAnsi * sizeof(WCHAR);
            }
        }

        ADVGET(pmenu16, cbAnsi);
        ADVPUT(pmenu32, cbUni);
        ALIGNWORD(pmenu32);         // this is the DIFFERENCE for WIN32
        if (wOption & MF_POPUP)
            cbTotal += ConvertMenuItems16(wExeVer, &pmenu32, &pmenu16, vpmenu16+(pmenu16 - pmenu16T));


    } while (!(wOption & MF_END));

    *ppmenu32 = pmenu32;
    *ppmenu16 = pmenu16;

    return (pmenu32 - pmenu32T);
}


DWORD ConvertDialog16(PBYTE pdlg32, VPBYTE vpdlg16, DWORD cb, DWORD cb16)
{
    BYTE b;
    WORD w;
    DWORD dwStyle;
    INT i, cItems;
    UINT cbAnsi;
    UINT cbUni;
    PBYTE pdlg16, pdlg16Save;
    PBYTE pdlg32T = pdlg32;

    pdlg16 = GETVDMPTR(vpdlg16, cb16, pdlg16Save);
    dwStyle = GETDWORD(pdlg16);
    PUTDWORD(pdlg32, dwStyle);          // transfer style
    PUTDWORD(pdlg32, 0);                // Add NEW extended style

    cItems = GETBYTE(pdlg16);
    PUTWORD(pdlg32, (WORD)cItems);      // stretch count to WORD for WIN32
    for (i=0; i<4; i++) {
        w = GETWORD(pdlg16);
        PUTWORD(pdlg32, w);             // transfer x & y, then cx & cy
    }

    //
    // the next three fields are all strings (possibly null)
    //       menuname, classname, captiontext
    // the Menu string can be encoded as  ff nn mm    which
    // means that the menu id is ordinal mmnn
    //

    for (i=0; i<3; i++) {
        if (i==0 && *pdlg16 == 0xFF) {  // special encoding of szMenuName
            GETBYTE(pdlg16);            // advance past the ff byte
            PUTWORD(pdlg32, 0xffff);    // copy the f word
            w = GETWORD(pdlg16);        // get the menu ordinal
            PUTWORD(pdlg32, w);         // transfer it
        } else {    // ordinary string
            cbAnsi = strlen(pdlg16)+1;
            if (VALIDPUT(pdlg32)) {
                RtlMultiByteToUnicodeN((LPWSTR)pdlg32, MAXULONG, (PULONG)&cbUni, pdlg16, cbAnsi);
            } else {
                cbUni = cbAnsi * sizeof(WCHAR);
            }
            ADVGET(pdlg16, cbAnsi);
            ADVPUT(pdlg32, cbUni);
            ALIGNWORD(pdlg32);          // fix next field alignment for WIN32
        }
    }

    if (dwStyle & DS_SETFONT) {
        w = GETWORD(pdlg16);
        PUTWORD(pdlg32, w);             // transfer cPoints
        cbAnsi = strlen(pdlg16)+1;      // then szTypeFace
        if (VALIDPUT(pdlg32)) {
            RtlMultiByteToUnicodeN((LPWSTR)pdlg32, MAXULONG, (PULONG)&cbUni, pdlg16, cbAnsi);
#ifdef FE_SB
            // orignal source should be fixed.
            // We can't convert right Unicode string from Broken FaceName
            // string.
            // Converted Unicode String should be NULL terminated.
            // 1994.11.12 V-HIDEKK
            if( cbUni && ((LPWSTR)pdlg32)[cbUni/sizeof(WCHAR)-1] ){
                LOGDEBUG(0,("    ConvertDialog16: WARNING: BAD FaceName String\n      End of Unicode String (%04x)\n", ((LPWSTR)pdlg32)[cbUni/2-1]));
                ((LPWSTR)pdlg32)[cbUni/sizeof(WCHAR)-1] = 0;
            }
#endif // FE_SB
        } else {
            cbUni = cbAnsi * sizeof(WCHAR);
        }
        ADVGET(pdlg16, cbAnsi);
        ADVPUT(pdlg32, cbUni);

    }
    while (cItems--) {
        ALIGNDWORD(pdlg32);         // items start on DWORD boundaries
        PUTDWORD(pdlg32, FETCHDWORD(*(PDWORD)(pdlg16+sizeof(WORD)*5)));
        PUTDWORD(pdlg32, 0);        // Add NEW extended style

        for (i=0; i<5; i++) {
            w = GETWORD(pdlg16);
            PUTWORD(pdlg32, w);     // transfer x & y, then cx & cy, then id
        }

        ADVGET(pdlg16, sizeof(DWORD));  // skip style, which we already copied

        //
        // get the class name   could be string or encoded value
        // win16 encoding scheme: class is 1 byte with bit 0x80 set,
        //     this byte == predefined class
        // win32 encoding: a word of ffff, followed by class (word)
        //

        if (*pdlg16 & 0x80) {
            PUTWORD(pdlg32, 0xFFFF); // NEW encoding marker 0xFFFF
            b = GETBYTE(pdlg16);     // special encoding for predefined class
            PUTWORD(pdlg32, (WORD)b);
        } else {
            cbAnsi = strlen(pdlg16)+1;
            if (VALIDPUT(pdlg32)) {  // transfer szClass
                RtlMultiByteToUnicodeN((LPWSTR)pdlg32, MAXULONG, (PULONG)&cbUni, pdlg16, cbAnsi);
            } else {
                cbUni = cbAnsi * sizeof(WCHAR);
            }
            ADVGET(pdlg16, cbAnsi);
            ADVPUT(pdlg32, cbUni);
        }
        ALIGNWORD(pdlg32);           // fix next field alignment for WIN32

        //
        // transfer the item text
        //

        if (*pdlg16 == 0xFF) {       // special encoding
            GETBYTE(pdlg16);
            PUTWORD(pdlg32, 0xFFFF);
            w = GETWORD(pdlg16);
            PUTWORD(pdlg32, w);
        } else {
            cbAnsi = strlen(pdlg16)+1;
            if (VALIDPUT(pdlg32)) {  // otherwise, just transfer szText
                RtlMultiByteToUnicodeN((LPWSTR)pdlg32, MAXULONG, (PULONG)&cbUni, pdlg16, cbAnsi);
            } else {
                cbUni = cbAnsi * sizeof(WCHAR);
            }
            ADVGET(pdlg16, cbAnsi);
            ADVPUT(pdlg32, cbUni);
        }
        ALIGNWORD(pdlg32);           // fix next field alignment for WIN32

        //
        // transfer the create params
        //

        b = GETBYTE(pdlg16);

        //
        // If the template has create params, we're going to get tricky.
        // When USER sends the WM_CREATE message to a control with
        // createparams, lParam points to the CREATESTRUCT, which
        // contains lpCreateParams.  lpCreateParams needs to point
        // to the createparams in the DLGTEMPLATE.  In order to
        // accomplish this, we store a 16:16 pointer to the 16-bit
        // DLGTEMPLATE's createparams in the 32-bit DLGTEMPLATE's
        // createparams.  In other words, whenever the count of
        // bytes of createparams is nonzero (b != 0), we put 4
        // bytes of createparams in the 32-bit DLGTEMPLATE that
        // happen to be a 16:16 pointer to the createparams in
        // the 16-bit DLGTEMPLATE.
        //
        // The other half of this magic is accomplished in USERSRV's
        // xxxServerCreateDialog, which special-cases the creation
        // of controls in a WOW dialog box.  USERSRV will pass the
        // DWORD pointed to by lpCreateParams instead of lpCreateParams
        // to CreateWindow.  This DWORD is the 16:16 pointer to the
        // 16-bit DLGTEMPLATE's createparams.
        //
        // DaveHart 14-Mar-93
        //

        if (b != 0) {

            // store 32-bit createparams size (room for 16:16 ptr)

            PUTWORD(pdlg32, sizeof(pdlg16));
            //ALIGNDWORD(pdlg32);

            // store 16:16 pointer in 32-bit createparams

            PUTUDWORD(pdlg32, (DWORD)vpdlg16 + (DWORD)(pdlg16 - pdlg16Save));

            // point pdlg16 past createparams

            ADVGET(pdlg16, b);

        } else {

            // there are no createparams, store size of zero.

            PUTWORD(pdlg32, 0);
            //ALIGNDWORD(pdlg32);
        }

    }
    FREEVDMPTR(pdlg16Save);
    RETURN(pdlg32 - pdlg32T);
}
