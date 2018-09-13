/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WDIB.C
 *  DIB.DRV support
 *
 *  History:
 *  28-Apr-1994 Sudeep Bharati
 *  Created.
 *
--*/


#include "precomp.h"
#pragma hdrstop
#include "wowgdip.h"
#include "wdib.h"
#include "memapi.h"

MODNAME(wdib.c);

#define CJSCAN(width,planes,bits) ((((width)*(planes)*(bits)+31) & ~31) / 8)
#define ABS(X) (((X) < 0 ) ? -(X) : (X))

BOOL W32CheckDibColorIndices(LPBITMAPINFOHEADER lpbmi);

// VGA colors
RGBQUAD rgbVGA[] = {
//    Blue  Green   Red
      0x00, 0x00, 0x00, 0,    // 0  ; black
      0x00, 0x00, 0x80, 0,    // 1  ; dark red
      0x00, 0x80, 0x00, 0,    // 2  ; dark green
      0x00, 0x80, 0x80, 0,    // 3  ; mustard
      0x80, 0x00, 0x00, 0,    // 4  ; dark blue
      0x80, 0x00, 0x80, 0,    // 5  ; purple
      0x80, 0x80, 0x00, 0,    // 6  ; dark turquoise
      0xc0, 0xc0, 0xc0, 0,    // 7  ; gray
      0x80, 0x80, 0x80, 0,    // 8  ; dark gray
      0x00, 0x00, 0xff, 0,    // 9  ; red
      0x00, 0xff, 0x00, 0,    // a  ; green
      0x00, 0xff, 0xff, 0,    // b  ; yellow
      0xff, 0x00, 0x00, 0,    // c  ; blue
      0xff, 0x00, 0xff, 0,    // d  ; magenta
      0xff, 0xff, 0x00, 0,    // e  ; cyan
      0xff, 0xff, 0xff, 0     // f  ; white
};

RGBQUAD rgb4[] = {
      0xc0, 0xdc, 0xc0, 0,    // 8
      0xf0, 0xca, 0xa6, 0,    // 9
      0xf0, 0xfb, 0xff, 0,    // 246
      0xa4, 0xa0, 0xa0, 0     // 247
};



PDIBINFO pDibInfoHead = NULL;
PDIBSECTIONINFO pDibSectionInfoHead = NULL;

HDC W32HandleDibDrv (PVPVOID vpbmi16)
{
    HDC             hdcMem = NULL;
    HBITMAP         hbm = NULL;
    PVOID           pvBits, pvIntelBits;
    STACKBMI32      bmi32;
    LPBITMAPINFO    lpbmi32;
    DWORD           dwClrUsed,nSize,nAlignmentSpace;
    PBITMAPINFOHEADER16 pbmi16;
    INT             nbmiSize,nBytesWritten;
    HANDLE          hfile=NULL,hsec=NULL;
    ULONG           RetVal,OriginalSelLimit,SelectorLimit,OriginalFlags;
    PARM16          Parm16;
    CHAR            pchTempFile[MAX_PATH];
    BOOL            bRet = FALSE;
    PVPVOID         vpBase16 = (PVPVOID) ((ULONG) vpbmi16 & 0xffff0000);

    if ((hdcMem = W32FindAndLockDibInfo((USHORT)HIWORD(vpbmi16))) != (HDC)NULL) {
        return hdcMem;
    }

    // First create a memory device context compatible to
    // the app's current screen
    if ((hdcMem = CreateCompatibleDC (NULL)) == NULL) {
        LOGDEBUG(LOG_ALWAYS,("\nWOW::W32HandleDibDrv CreateCompatibleDC failed\n"));
        return NULL;
    }

    // Copy bmi16 to bmi32. DIB.DRV only supports DIB_RGB_COLORS
    lpbmi32 = CopyBMI16ToBMI32(
                     vpbmi16,
                     (LPBITMAPINFO)&bmi32,
                     (WORD) DIB_RGB_COLORS);

    // this hack for Director 4.0 does essentially what WFW does
    // if this bitmap is 0 sized, just return an hDC for something simple
    if(bmi32.bmiHeader.biSizeImage == 0 &&
       (CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_DIBDRVIMAGESIZEZERO)) {
        LOGDEBUG(LOG_ALWAYS,("\nWOW::W32HandleDibDrv:Zero biSizeImage, returning memory DC!\n"));
        return hdcMem;
    }

    try {

        // Copy the wholething into a temp file. First get a temp file name
        if ((nSize = GetTempPath (MAX_PATH, pchTempFile)) == 0 ||
             nSize >= MAX_PATH)
            goto hdd_err;

        if (GetTempFileName (pchTempFile,
                             "DIB",
                             0,
                             pchTempFile) == 0)
            goto hdd_err;

        if ((hfile = CreateFile (pchTempFile,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_WRITE,
                                NULL,
                                CREATE_ALWAYS,
                                (FILE_ATTRIBUTE_NORMAL |
                                 FILE_ATTRIBUTE_TEMPORARY |
                                 FILE_FLAG_DELETE_ON_CLOSE),
                                NULL)) == INVALID_HANDLE_VALUE) {
            LOGDEBUG(LOG_ALWAYS,("\nWOW::W32HandleDibDrv CreateFile failed\n"));
            goto hdd_err;
        }

        // call back to get the size of the global object
        // associated with vpbmi16
        Parm16.WndProc.wParam = HIWORD(vpbmi16);

        CallBack16(RET_GETDIBSIZE,
                   &Parm16,
                   0,
                   (PVPVOID)&SelectorLimit);

        Parm16.WndProc.wParam = HIWORD(vpbmi16);

        if (SelectorLimit == 0xffffffff || SelectorLimit == 0) {
            LOGDEBUG(LOG_ALWAYS,("\nWOW::W32HandleDibDrv Invalid Selector %x\n",HIWORD(vpbmi16)));
            goto hdd_err;
        }

        SelectorLimit++;

        OriginalSelLimit = SelectorLimit;

        CallBack16(RET_GETDIBFLAGS,
                   &Parm16,
                   0,
                   (PVPVOID)&OriginalFlags);

        if (OriginalFlags == 0x4) { //GA_DGROUP
            LOGDEBUG(LOG_ALWAYS,("\nWOW::W32HandleDibDrv GA_DGROUP Not Handled\n"));
            goto hdd_err;
        }

        GETVDMPTR(vpBase16, SelectorLimit, pbmi16);

        nbmiSize = GetBMI16Size(vpbmi16, (WORD) DIB_RGB_COLORS, &dwClrUsed);

        // Under NT CreateDIBSection will fail if the offset to the bits
        // is not dword aligned. So we may have to add some space at the top
        // of the section to get the offset correctly aligned.

        nAlignmentSpace = (nbmiSize+LOWORD(vpbmi16)) % 4;

        if (nAlignmentSpace) {
            if (WriteFile (hfile,
                           pbmi16,
                           nAlignmentSpace,
                           &nBytesWritten,
                           NULL) == FALSE ||
                           nBytesWritten != (INT) nAlignmentSpace)
            goto hdd_err;
        }

        //
        // detect a clinical case of bitedit screwing around dib.drv
        //
        // code below is using dib macros declared in wdib.h
        // namely:
        //      DibNumColors - yields max number of colors in dib
        //      DibColors    - yields pointer to a dib color table
        //
        // Function W32CheckDibColorIndices checks to see if DIB color
        // table looks like a number (defined usually by biClrImportant)
        // of WORD indices in a sequential order (0, 1, 2, ...)
        // if this is the case, app is trying to use undocumented feature
        // of DIB.DRV that turns color matching off in this case.
        // Since we cannot enforce that rule, we approximate it by filling
        // color table by a number of known (and always same) entries
        // When blitting occurs, no color matching will be performed (when
        // both target and destination are of this very nature).
        // For no reason at all we fill color table with vga colors.
        // Sequential indices could have worked just as well.
        //
        // Modifications are made to memory pointed to by lpbmi32

        if (W32CheckDibColorIndices((LPBITMAPINFOHEADER)lpbmi32)) {
            BYTE i;
            INT nColors;
            LPBITMAPINFOHEADER lpbmi = (LPBITMAPINFOHEADER)lpbmi32;
            LPRGBQUAD lprgbq = (LPRGBQUAD)DibColors(lpbmi);

            nColors = DibNumColors(lpbmi);
            lpbmi->biClrImportant = nColors;

            switch (lpbmi->biBitCount) {
                case 1:
                    lprgbq[0] = rgbVGA[0];
                    lprgbq[1] = rgbVGA[0x0f];
                    break;

                case 4:
                    RtlCopyMemory(lprgbq, rgbVGA, sizeof(rgbVGA));
                    break;

                case 8:
                    RtlCopyMemory(lprgbq,     rgbVGA,   8*sizeof(RGBQUAD));
                    RtlCopyMemory(lprgbq+248, rgbVGA+8, 8*sizeof(RGBQUAD));
                    RtlCopyMemory(lprgbq+8,   rgb4,   2*sizeof(RGBQUAD));
                    RtlCopyMemory(lprgbq+246, rgb4+2, 2*sizeof(RGBQUAD));
                    for (i = 10; i < 246; ++i) {
                        lprgbq[i].rgbBlue = i;
                        lprgbq[i].rgbGreen= 0;
                        lprgbq[i].rgbRed  = 0;
                        lprgbq[i].rgbReserved = 0;
                    }
                    break;

                default: // this should never happen
                    break;
            }
        }

        if (WriteFile (hfile,
                       pbmi16,
                       SelectorLimit,
                       &nBytesWritten,
                       NULL) == FALSE || nBytesWritten != (INT) SelectorLimit)
            goto hdd_err;

        if (SelectorLimit < 64*1024) {
            if (SetFilePointer (hfile,
                                64*1024+nAlignmentSpace,
                                NULL,
                                FILE_BEGIN) == -1)
                goto hdd_err;

            if (SetEndOfFile (hfile) == FALSE)
                goto hdd_err;

            SelectorLimit = 64*1024;
        }

        if ((hsec = CreateFileMapping (hfile,
                                       NULL,
                                       PAGE_READWRITE | SEC_COMMIT,
                                       0,
                                       SelectorLimit+nAlignmentSpace,
                                       NULL)) == NULL) {
            LOGDEBUG(LOG_ALWAYS,("\nWOW::W32HandleDibDrv CreateFileMapping Failed\n"));
            goto hdd_err;
        }

        // Now create the DIB section
        if ((hbm = CreateDIBSection (hdcMem,
                                lpbmi32,
                                DIB_RGB_COLORS,
                                &pvBits,
                                hsec,
                                nAlignmentSpace + LOWORD(vpbmi16) + nbmiSize
                                )) == NULL) {
            LOGDEBUG(LOG_ALWAYS,("\nWOW::W32HandleDibDrv CreateDibSection Failed\n"));
            goto hdd_err;
        }

        FREEVDMPTR(pbmi16);

        if((pvBits = MapViewOfFile(hsec,
                         FILE_MAP_WRITE,
                         0,
                         0,
                         SelectorLimit+nAlignmentSpace)) == NULL) {
            LOGDEBUG(LOG_ALWAYS,("\nWOW::W32HandleDibDrv MapViewOfFile Failed\n"));
            goto hdd_err;
        }

        pvBits = (PVOID) ((ULONG)pvBits + nAlignmentSpace);

        SelectObject (hdcMem, hbm);

        GdiSetBatchLimit(1);

#ifndef i386
        if (!NT_SUCCESS(VdmAddVirtualMemory((ULONG)pvBits,
                                            (ULONG)SelectorLimit,
                                            (PULONG)&pvIntelBits))) {
            LOGDEBUG(LOG_ALWAYS,("\nWOW::W32HandleDibDrv VdmAddVirtualMemory failed\n"));
            goto hdd_err;
        }

        // On risc platforms, the intel base + the intel linear address
        // of the DIB section is not equal to the DIB section's process
        // address. This is because of the VdmAddVirtualMemory call
        // above. So here we zap the correct address into the flataddress
        // array.
        if (!VdmAddDescriptorMapping(HIWORD(vpbmi16),
                                    (USHORT) ((SelectorLimit+65535)/65536),
                                    (ULONG) pvIntelBits,
                                    (ULONG) pvBits)) {
            LOGDEBUG(LOG_ALWAYS,("\nWOW::W32HandleDibDrv VdmAddDescriptorMapping failed\n"));
            goto hdd_err;
        }

#else
        pvIntelBits = pvBits;
#endif

        // Finally set the selectors to the new DIB
        Parm16.WndProc.wParam = HIWORD(vpbmi16);
        Parm16.WndProc.lParam = (LONG)pvIntelBits;
        Parm16.WndProc.wMsg   = 0x10; // GA_NOCOMPACT
        Parm16.WndProc.hwnd   = 1;    // set so it's not randomly 0

        CallBack16(RET_SETDIBSEL,
                   &Parm16,
                   0,
                   (PVPVOID)&RetVal);

        if (!RetVal) {
            LOGDEBUG(LOG_ALWAYS,("\nWOW::W32HandleDibDrv Callback set_sel_for_dib failed\n"));
            goto hdd_err;
        }


        // Store all the relevant information so that DeleteDC could
        // free all the resources later.
        if (W32AddDibInfo(hdcMem,
                          hfile,
                          hsec,
                          nAlignmentSpace,
                          pvBits,
                          pvIntelBits,
                          hbm,
                          OriginalSelLimit,
                          (USHORT)OriginalFlags,
                          (USHORT)((HIWORD(vpbmi16)))) == FALSE)
            goto hdd_err;


        // Finally spit out the dump for debugging
        LOGDEBUG(6,("\t\tWOW::W32HandleDibDrv hdc=%04x nAlignment=%04x\n\t\tNewDib=%x OldDib=%04x:%04x DibSize=%x DibFlags=%x\n",hdcMem,nAlignmentSpace,pvBits,HIWORD(vpbmi16),LOWORD(vpbmi16),OriginalSelLimit,(USHORT)OriginalFlags));

        bRet = TRUE;
hdd_err:;
    }
    finally {
        if (!bRet) {

            if (hdcMem) {
                DeleteDC (hdcMem);
                hdcMem = NULL;
            }
            if (hfile)
                CloseHandle (hfile);

            if (hsec)
                CloseHandle (hsec);

            if (hbm)
                CloseHandle (hbm);
        }
    }
    return hdcMem;
}


BOOL W32AddDibInfo (
    HDC hdcMem,
    HANDLE hfile,
    HANDLE hsec,
    ULONG  nalignment,
    PVOID  newdib,
    PVOID  newIntelDib,
    HBITMAP hbm,
    ULONG dibsize,
    USHORT originaldibflags,
    USHORT originaldibsel
    )
{
    PDIBINFO pdi;

    if ((pdi = malloc_w (sizeof (DIBINFO))) == NULL)
        return FALSE;

    pdi->di_hdc     = hdcMem;
    pdi->di_hfile   = hfile;
    pdi->di_hsec    = hsec;
    pdi->di_nalignment    = nalignment;
    pdi->di_newdib  = newdib;
    pdi->di_newIntelDib = newIntelDib;
    pdi->di_hbm     = hbm;
    pdi->di_dibsize = dibsize;
    pdi->di_originaldibsel = originaldibsel;
    pdi->di_originaldibflags = originaldibflags;
    pdi->di_next    = pDibInfoHead;
    pdi->di_lockcount = 1;
    pDibInfoHead    = pdi;

    return TRUE;
}

BOOL W32FreeDibInfoHandle(PDIBINFO pdi, PDIBINFO pdiLast)
{
    if (W32RestoreOldDib (pdi) == 0) {
        LOGDEBUG(LOG_ALWAYS,("\nWOW::W32RestoreDib failed\n"));
        return FALSE;
    }
#ifndef i386
    VdmRemoveVirtualMemory((ULONG)pdi->di_newIntelDib);
#endif
    UnmapViewOfFile ((LPVOID)((ULONG)pdi->di_newdib - pdi->di_nalignment));

    DeleteObject (pdi->di_hbm);
    CloseHandle (pdi->di_hsec);
    CloseHandle (pdi->di_hfile);

    DeleteDC(pdi->di_hdc);
    W32FreeDibInfo (pdi, pdiLast);

    return TRUE;
}


BOOL    W32CheckAndFreeDibInfo (HDC hdc)
{
    PDIBINFO pdi = pDibInfoHead,pdiLast=NULL;

    while (pdi) {
        if (pdi->di_hdc == hdc){

            if (--pdi->di_lockcount) {
                //
                // This must be a releasedc within a nested call to createdc.
                // Just return, as this should be released again later.
                //
                LOGDEBUG(LOG_ALWAYS, ("\nW32CheckAndFreeDibInfo: lockcount!=0\n"));
                return TRUE;
            }

            return W32FreeDibInfoHandle(pdi, pdiLast);
        }
        pdiLast = pdi;
        pdi = pdi->di_next;
    }

    return FALSE;
}

VOID W32FreeDibInfo (PDIBINFO pdiCur, PDIBINFO pdiLast)
{
    if (pdiLast == NULL)
        pDibInfoHead = pdiCur->di_next;
    else
        pdiLast->di_next = pdiCur->di_next;

    free_w (pdiCur);
}

ULONG W32RestoreOldDib (PDIBINFO pdi)
{
    PARM16          Parm16;
    ULONG           retval;

    // callback to allocate memory and copy the dib from dib section

    Parm16.WndProc.wParam = pdi->di_originaldibsel;
    Parm16.WndProc.lParam = (LONG) (pdi->di_newdib);
    Parm16.WndProc.wMsg = pdi->di_originaldibflags;

    CallBack16(RET_FREEDIBSEL,
               &Parm16,
               0,
               (PVPVOID)&retval);

    return retval;
}


HDC W32FindAndLockDibInfo (USHORT sel)
{
    PDIBINFO pdi = pDibInfoHead;

    while (pdi) {

        if (pdi->di_originaldibsel == sel){
            pdi->di_lockcount++;
            return (pdi->di_hdc);

        }
        pdi = pdi->di_next;

    }
    return (HDC) NULL;
}

//
//  This function is called from krnl386 if GlobalReAlloc or GlobalFree is
//  trying to operate on memory which we suspect is dib-mapped. It finds
//  dib by original selector and restores it, thus allowing respective function
//  to succeede. Bitedit is the app that does globalrealloc before DeleteDC
//
//

ULONG FASTCALL WK32FindAndReleaseDib(PVDMFRAME pvf)
{
    USHORT sel;
    PFINDANDRELEASEDIB16 parg;
    PDIBINFO pdi;
    PDIBINFO pdiLast = NULL;

    // get the argument pointer, see wowkrnl.h
    GETARGPTR(pvf, sizeof(*parg), parg);

    // get selector from the handle
    sel = parg->hdib | (USHORT)0x01; // "convert to sel"

    // find this said sel in the dibinfo
    pdi = pDibInfoHead;
    while (pdi) {
        if (pdi->di_originaldibsel == sel) {

            // found ! this is what we are releasing or reallocating
            LOGDEBUG(LOG_ALWAYS, ("\nWOW: In FindAndReleaseDIB function %d\n", (DWORD)parg->wFunId));

            // see if we need to nuke...
            if (--pdi->di_lockcount) {
                // the problem with lock count...
                LOGDEBUG(LOG_ALWAYS, ("\nWOW: FindAndReleaseDib failed (lock count!)\n"));
                return FALSE;
            }

            return W32FreeDibInfoHandle(pdi, pdiLast);
        }

        pdiLast = pdi;
        pdi = pdi->di_next;
    }

    return FALSE;
}


BOOL W32CheckDibColorIndices(LPBITMAPINFOHEADER lpbmi)
{
    WORD i, nColors;
    LPWORD lpw = (LPWORD)DibColors(lpbmi);

    nColors = DibNumColors(lpbmi);
    if (lpbmi->biClrImportant) {
        nColors = min(nColors, (WORD)lpbmi->biClrImportant);
    }

    for (i = 0; i < nColors; ++i) {
        if (*lpw++ != i) {
            return FALSE;
        }
    }

    LOGDEBUG(LOG_ALWAYS, ("\nUndocumented Dib.Drv behaviour used\n"));

    return TRUE;
}

/******************************Public*Routine******************************\
* DIBSection specific calls
*
* History:
*  04-May-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG cjBitmapBitsSize(CONST BITMAPINFO *pbmi)
{
// Check for PM-style DIB

    if (pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        LPBITMAPCOREINFO pbmci;
        pbmci = (LPBITMAPCOREINFO)pbmi;
        return(CJSCAN(pbmci->bmciHeader.bcWidth,pbmci->bmciHeader.bcPlanes,
                      pbmci->bmciHeader.bcBitCount) *
                      pbmci->bmciHeader.bcHeight);
    }

// not a core header

    if ((pbmi->bmiHeader.biCompression == BI_RGB) ||
        (pbmi->bmiHeader.biCompression == BI_BITFIELDS))
    {
        return(CJSCAN(pbmi->bmiHeader.biWidth,pbmi->bmiHeader.biPlanes,
                      pbmi->bmiHeader.biBitCount) *
               ABS(pbmi->bmiHeader.biHeight));
    }
    else
    {
        return(pbmi->bmiHeader.biSizeImage);
    }
}

ULONG FASTCALL WG32CreateDIBSection(PVDMFRAME pFrame)
{
    ULONG              ul = 0;
    STACKBMI32         bmi32;
    LPBITMAPINFO       lpbmi32;
    HBITMAP            hbm32;
    PVOID              pv16, pvBits, pvIntelBits;
    PVPVOID            vpbmi16;
    PVOID              pvBits32;
    DWORD              dwArg16;

    register PCREATEDIBSECTION16 parg16;

    GETARGPTR(pFrame, sizeof(CREATEDIBSECTION16), parg16);

    // this is performance hack so we don't generate extra code
    dwArg16 = FETCHDWORD(parg16->f4); // do it once here
    pv16 = (PVOID)GetPModeVDMPointer(dwArg16, sizeof(DWORD)); // aligned here!

    WOW32ASSERTMSG(((parg16->f5 == 0) && (parg16->f6 == 0)),
                   ("WOW:WG32CreateDIBSection, hSection/dwOffset non-null\n"));

    vpbmi16 = (PVPVOID)FETCHDWORD(parg16->f2);
    lpbmi32 = CopyBMI16ToBMI32(vpbmi16,
                               (LPBITMAPINFO)&bmi32,
                               FETCHWORD(parg16->f3));

    hbm32 = CreateDIBSection(HDC32(parg16->f1),
                             lpbmi32,
                             WORD32(parg16->f3),
                             &pvBits,
                             NULL,
                             0);

    if (hbm32 != 0)
    {
        PARM16          Parm16;
        PDIBSECTIONINFO pdi;
        ULONG           SelectorLimit;

        SelectorLimit = (ULONG)cjBitmapBitsSize(lpbmi32);
#ifndef i386
        if (!NT_SUCCESS(VdmAddVirtualMemory((ULONG)pvBits,
                                            SelectorLimit,
                                            (PULONG)&pvIntelBits))) {
            LOGDEBUG(LOG_ALWAYS,("\nWOW::WG32CreateDibSection VdmAddVirtualMemory failed\n"));
            goto cds_err;
        }

#else
        pvIntelBits = pvBits;
#endif

        // Create a selector array for the bits backed by pvIntelBits

        Parm16.WndProc.wParam = (WORD)-1;           // -1 => allocate selectors
        Parm16.WndProc.lParam = (LONG) pvIntelBits; // backing pointer
        Parm16.WndProc.wMsg = 0x10;                 // GA_NOCOMPACT
        Parm16.WndProc.hwnd = (WORD)((SelectorLimit+65535)/65536);// selector count

        CallBack16(RET_SETDIBSEL,
                   &Parm16,
                   0,
                   (PVPVOID)&pvBits32);

        // 16:16 pointer is still valid as call above makes no difference
        if (pv16 != NULL) {
            *(UNALIGNED PVOID*)pv16 = pvBits32;
        }

        if (pvBits32 == NULL) {
            LOGDEBUG(LOG_ALWAYS,("\nWOW::WG32CreateDibSection, Callback set_sel_for_dib failed\n"));
            goto cds_err;
        }

#ifndef i386
        // okay, that was successful - map the descriptors properly

        if (!VdmAddDescriptorMapping(HIWORD(pvBits32),
                                    (USHORT) ((SelectorLimit+65535)/65536),
                                    (ULONG) pvIntelBits,
                                    (ULONG) pvBits)) {
            LOGDEBUG(LOG_ALWAYS,("\nWOW::WG32CreateDibSection VdmAddDescriptorMapping failed\n"));
            goto cds_err;
        }
#endif

        LOGDEBUG(LOG_ALWAYS, ("\nWOW:CreateDIBSection: [16:16 %x] [Intel %x] [Flat %x]\n",
                             pvBits32, pvIntelBits, pvBits));

        ul = GETHBITMAP16(hbm32);

        // Add it to the list used for cleanup at DeleteObject time.

        if ((pdi = malloc_w (sizeof (DIBSECTIONINFO))) != NULL) {
            pdi->di_hbm         = hbm32;
            pdi->di_pv16        = pvBits32;
#ifndef i386
            pdi->di_newIntelDib = pvIntelBits;
#endif
            pdi->di_next        = pDibSectionInfoHead;
            pDibSectionInfoHead = pdi;

            // need to turn batching off since a DIBSECTION means the app can
            // also draw on the bitmap and we need synchronization.

            GdiSetBatchLimit(1);

            goto cds_ok;
        }
        else {
            // Failure, free the selector array

            Parm16.WndProc.wParam = (WORD)-1;            // -1 => allocate/free
            Parm16.WndProc.lParam = (LONG) pvBits32; // pointer
            Parm16.WndProc.wMsg = 0x10; // GA_NOCOMPACT
            Parm16.WndProc.hwnd = 0;                     // 0 => free

            CallBack16(RET_SETDIBSEL,
                       &Parm16,
                       0,
                       (PVPVOID)&ul);
#ifndef i386
            VdmRemoveVirtualMemory((ULONG)pvIntelBits);
#endif

        }
    }
    else {
        LOGDEBUG(LOG_ALWAYS,("\nWOW::WG32CreateDibSection, CreateDibSection Failed\n"));
    }

cds_err:

    if (hbm32 != 0) {
        DeleteObject(hbm32);
    }
    LOGDEBUG(LOG_ALWAYS,("\nWOW::WG32CreateDibSection returning failure\n"));
    ul = 0;

cds_ok:
    WOW32APIWARN(ul, "CreateDIBSection");

    FREEMISCPTR(pv16);
    FREEARGPTR(parg16);

    return(ul);
}

ULONG FASTCALL WG32GetDIBColorTable(PVDMFRAME pFrame)
{
    ULONG              ul = 0;
    RGBQUAD *          prgb;

    register PGETDIBCOLORTABLE16 parg16;

    GETARGPTR(pFrame, sizeof(GETDIBCOLORTABLE16), parg16);
    GETMISCPTR(parg16->f4,prgb);

    ul = (ULONG)GetDIBColorTable(HDC32(parg16->f1),
                                 parg16->f2,
                                 parg16->f3,
                                 prgb);

    WOW32APIWARN(ul, "GetDIBColorTable");

    if (ul)
        FLUSHVDMPTR(parg16->f4,sizeof(RGBQUAD) * ul,prgb);

    FREEMISCPTR(prgb);
    FREEARGPTR(parg16);

    return(ul);
}

ULONG FASTCALL WG32SetDIBColorTable(PVDMFRAME pFrame)
{
    ULONG              ul = 0;
    RGBQUAD *          prgb;

    register PSETDIBCOLORTABLE16 parg16;

    GETARGPTR(pFrame, sizeof(SETDIBCOLORTABLE16), parg16);
    GETMISCPTR(parg16->f4,prgb);

    ul = (ULONG)SetDIBColorTable(HDC32(parg16->f1),
                                 parg16->f2,
                                 parg16->f3,
                                 prgb);

    WOW32APIWARN(ul, "SetDIBColorTable");

    FREEMISCPTR(prgb);
    FREEARGPTR(parg16);

    return(ul);
}


// DIBSection routines

BOOL W32CheckAndFreeDibSectionInfo (HBITMAP hbm)
{
    PDIBSECTIONINFO pdi = pDibSectionInfoHead,pdiLast=NULL;

    while (pdi) {
        if (pdi->di_hbm == hbm){

            PARM16 Parm16;
            ULONG  ulRet;

            // need to free the selector array for the memory

            Parm16.WndProc.wParam = (WORD)-1;            // selector, -1 == allocate/free
            Parm16.WndProc.lParam = (LONG) pdi->di_pv16; // pointer
            Parm16.WndProc.wMsg = 0x10; // GA_NOCOMPACT
            Parm16.WndProc.hwnd = 0;                     // selector count, 0 == free

            CallBack16(RET_SETDIBSEL,
                       &Parm16,
                       0,
                       (PVPVOID)&ulRet);
#ifndef i386
            VdmRemoveVirtualMemory((ULONG)pdi->di_newIntelDib);
#endif

            if (pdiLast == NULL)
                pDibSectionInfoHead = pdi->di_next;
            else
                pdiLast->di_next = pdi->di_next;

            // now delete the object

            DeleteObject (pdi->di_hbm);

            free_w(pdi);

            return TRUE;
        }
        pdiLast = pdi;
        pdi = pdi->di_next;
    }
    return FALSE;
}
