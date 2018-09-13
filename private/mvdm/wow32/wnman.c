/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WNMAN.C
 *  WOW32 16-bit Winnls API support (manually-coded thunks)
 *
 *  History:
 *  Created 19-Feb-1992 by Junichi Okubo (junichio)
 *  Changed 30-Jun-1992 by Hiroyuki Hanaoka (hiroh)
 *  Changed 05-Nov-1992 by Kazuyuki Kato (v-kazuyk)
 *
--*/
#include "precomp.h"
#pragma hdrstop

#ifdef FE_IME

#include "ime.h"
#include "imep.h"
#include "winnls32.h"
#include "wcall16.h"        // use GlobalLock16

#include "wownls.h"
#include "wnman.h"

MODNAME(wnman.c);

struct  _wow32imedebug {
    LPSZ    subfunction;
} wow32imedebug[]={
    {"undefined IME function"},     //0x00
    {"undefined IME function"},     //0x01
    {"undefined IME function"},     //0x02
    {"IME_GETIMECAPS"},             //0x03
    {"IME_SETOPEN"},                //0x04
    {"IME_GETOPEN"},                //0x05
    {"IME_ENABLEDOSIME"},           //0x06
    {"IME_GETVERSION"},             //0x07
    {"IME_SETCONVERSIONWINDOW"},    //0x08
    {"undefined IME function"},     //0x09
    {"undefined IME function"},     //0x0a
    {"undefined IME function"},     //0x0b
    {"undefined IME function"},     //0x0c
    {"undefined IME function"},     //0x0d
    {"undefined IME function"},     //0x0e
    {"undefined IME function"},     //0x0f
    {"IME_SETCONVERSIONMODE, (undefined IME function - KOREA)"}, //0x10
    {"IME_GETCONVERSIONMODE, (IME_GET_MODE - KOREA)"},           //0x11
    {"IME_SETCONVERSIONFONT, (IME_SET_MODE - KOREA)"},           //0x12
    {"IME_SENDVKEY"},               //0x13
    {"IME_DESTROYIME"},             //0x14
    {"IME_PRIVATE"},                //0x15
    {"IME_WINDOWUPDATE"},           //0x16
    {"IME_SELECT"},                 //0x17
    {"IME_ENTERWORDREGISTERMODE"},  //0x18
    {"IME_SETCONVERSIONFONTEX"},    //0x19
    {"undefined IME function"},     //0x1a
    {"undefined IME function"},     //0x1b
    {"undefined IME function"},     //0x1c
    {"undefined IME function"},     //0x1d
    {"undefined IME function"},     //0x1e
    {"undefined IME function"},     //0x1f
    {"IME_CODECONVERT"},            //0x20
    {"IME_CONVERTLIST"},            //0x21
    {"undefined IME function"},     //0x22
    {"undefined IME function"},     //0x23
    {"undefined IME function"},     //0x24
    {"undefined IME function"},     //0x25
    {"undefined IME function"},     //0x26
    {"undefined IME function"},     //0x27
    {"undefined IME function"},     //0x28
    {"undefined IME function"},     //0x29
    {"undefined IME function"},     //0x2a
    {"undefined IME function"},     //0x2b
    {"undefined IME function"},     //0x2c
    {"undefined IME function"},     //0x2d
    {"undefined IME function"},     //0x2e
    {"undefined IME function"},     //0x2f
    {"IME_AUTOMATA"},               //0x30
    {"IME_HANJAMODE"},              //0x31
    {"undefined IME function"},     //0x32
    {"undefined IME function"},     //0x33
    {"undefined IME function"},     //0x34
    {"undefined IME function"},     //0x35
    {"undefined IME function"},     //0x36
    {"undefined IME function"},     //0x37
    {"undefined IME function"},     //0x38
    {"undefined IME function"},     //0x39
    {"undefined IME function"},     //0x3a
    {"undefined IME function"},     //0x3b
    {"undefined IME function"},     //0x3c
    {"undefined IME function"},     //0x3d
    {"undefined IME function"},     //0x3e
    {"undefined IME function"},     //0x3f
    {"IME_GETLEVEL"},               //0x40
    {"IME_SETLEVEL"},               //0x41
    {"IME_GETMNTABLE"}              //0x42
};

INT wow32imedebugMax=0x43;
HAND16  hFnt16;     // 16 bit Font handle;

#define IME_MOVEIMEWINDOW IME_SETCONVERSIONWINDOW

ULONG FASTCALL  WN32SendIMEMessage(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    PSENDIMEMESSAGE16 parg16;
    IMESTRUCT * imestruct32;
    register PIMESTRUCT16 ptag16;
    HANDLE hIME32;
    INT cb;
    VPVOID  vp;
    HANDLE  hlParam1 = NULL;    // IME_ENTERWORDREGISTERMODE
    HANDLE  hlParam2 = NULL;
    HANDLE hLFNT32;     // IMW_SETCONVERSIONFONT(EX)

    GETARGPTR(pFrame, sizeof(SENDIMEMESSAGE16), parg16);
    vp = GlobalLock16(FETCHWORD(parg16->lParam), NULL);
    GETMISCPTR(vp, ptag16); // Get IME struct16 ptr

    hIME32 = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, sizeof(IMESTRUCT));
    imestruct32 = GlobalLock(hIME32);

    if (ptag16 == NULL) {
        LOGDEBUG(1,("   WINNLS:(Jun)ptag16==NULL!!"));
        goto eee;
    }

    switch (ptag16 -> fnc) {
    case IME_HANJAMODE:
        // Korea specific function
        if (GetSystemDefaultLangID() != 0x412)
            goto eee;

        STOREDWORD(imestruct32->fnc, ptag16->fnc);
        STOREDWORD(imestruct32->wParam, 0);

        // The 4th word of imestruct32 must contains ptag16->dchSource.
        // msime95 will find ptag16->dchSource on the 4th word.
        *((LPSTR)(imestruct32) + sizeof(ptag16->fnc) +
                              sizeof(ptag16->wParam) +
                              sizeof(ptag16->wCount) )
                = (CHAR)ptag16->dchSource;

        *((LPSTR)(imestruct32) + ptag16->dchSource)
                = *(LPSTR)((LPSTR)(ptag16) + (ptag16)->dchSource);

        *((LPSTR)(imestruct32) + ptag16->dchSource + 1)
                = *(LPSTR)((LPSTR)(ptag16) + (ptag16)->dchSource + 1);

        // Quattro Pro Window use null window handle when it call Hanja conversion.
        if (CURRENTPTD()->dwWOWCompatFlags2 & WOWCF_QPW_FIXINVALIDWINHANDLE)
             parg16->hwnd = GETHWND16(GetFocus());
        break;

    case IME_CONVERTLIST:
    case IME_AUTOMATA:
    case IME_CODECONVERT:
    case IME_SETLEVEL:
    case IME_GETLEVEL:
    case IME_GETMNTABLE:
        // Korea specific function
        if (GetSystemDefaultLangID() != 0x412)
            goto eee;
        goto standard;

    case IME_SETCONVERSIONWINDOW:   // (IME_MOVECONVERTWINDOW)
                                    //  IME_MOVEIMEWINDOW for Korea
        if (GetSystemDefaultLangID() != 0x412 &&
            CURRENTPTD()->dwWOWCompatFlags2 & WOWCF_AMIPRO_PM4J_IME) {
            // Don't pass the MCW_DEFAULT.
            // because, Conversion window will be flushed when
            // default conversion window and AMIPRO's window have overlap.
            //
            // Also, for PM4J, when the codebox is moved Pagemaker
            // thinks it needs to be displayed at default. Prevent
            // endless loop of default screen|window displays
            //
            if (ptag16->wParam == MCW_DEFAULT) {
                ul = FALSE;
                goto eee;
            }
        }

    case IME_GETOPEN:
    case IME_SETOPEN:
    case IME_GETIMECAPS:        // (IME_QUERY)
    case IME_SETCONVERSIONMODE:     // (IME_SET_MODE)
    case IME_GETCONVERSIONMODE:     // (IME_GET_MODE)
    case IME_SENDVKEY:          // (IME_SENDKEY)
    case IME_DESTROYIME:        // (IME_DESTROY)
    case IME_WINDOWUPDATE:
    case IME_SELECT:
    case IME_GETVERSION:
standard:
        STOREDWORD(imestruct32->fnc, ptag16->fnc);
        STOREDWORD(imestruct32->wParam, ptag16->wParam);
        STOREDWORD(imestruct32->wCount, ptag16->wCount);
        STOREDWORD(imestruct32->dchSource, ptag16->dchSource);
        STOREDWORD(imestruct32->dchDest, ptag16->dchDest);
        /*** STOREWORD -> STOREDWORD v-kazyk ***/
        STOREDWORD(imestruct32->lParam1, ptag16->lParam1);
        STOREDWORD(imestruct32->lParam2, ptag16->lParam2);
        STOREDWORD(imestruct32->lParam3, ptag16->lParam3);
        break;

    case IME_ENTERWORDREGISTERMODE: // (IME_WORDREGISTER)
        {
        LPBYTE  lpMem16;
        LPBYTE  lpMem32;

        STOREDWORD(imestruct32->fnc, ptag16->fnc);
        if (ptag16->lParam1) {
            vp = GlobalLock16(FETCHWORD(ptag16->lParam1), &cb);
            hlParam1 = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, cb + 1);
            lpMem32 = GlobalLock(hlParam1);
            GETMISCPTR(vp, lpMem16);
            RtlCopyMemory(lpMem32, lpMem16, cb);
            GlobalUnlock(hlParam1);
            GlobalUnlock16(FETCHWORD(ptag16->lParam1));
        }
        if (ptag16->lParam2) {
            vp = GlobalLock16(FETCHWORD(ptag16->lParam2), &cb);
            hlParam2 = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, cb + 1);
            lpMem32 = GlobalLock(hlParam2);
            GETMISCPTR(vp, lpMem16);
            RtlCopyMemory(lpMem32, lpMem16, cb);
            GlobalUnlock(hlParam2);
            GlobalUnlock16(FETCHWORD(ptag16->lParam2));
        }
        STOREDWORD(imestruct32->lParam1, hlParam1);
        STOREDWORD(imestruct32->lParam2, hlParam2);
        STOREDWORD(imestruct32->lParam3, ptag16->lParam3);
        }
        break;

    case IME_SETCONVERSIONFONT:     // (IME_SET_MODE - Korea)
        {
        LOGFONT * logfont32;

        if (GetSystemDefaultLangID() == 0x412) {
            // Hunguel WOW should do anything for IME_SET_MODE function
            goto eee;
        }

        STOREDWORD(imestruct32->fnc, IME_SETCONVERSIONFONTEX);
        if ( ptag16->wParam ) {
            hLFNT32 = GlobalAlloc( GMEM_SHARE | GMEM_MOVEABLE, sizeof(LOGFONT));
            logfont32 = GlobalLock(hLFNT32);
            GetObject(HOBJ32(ptag16->wParam), sizeof(LOGFONT), logfont32);
            GlobalUnlock(hLFNT32);
        }
        else {
            hLFNT32 = NULL;
        }
        STOREDWORD(imestruct32->lParam1, hLFNT32);
        }
        break;

    case IME_SETCONVERSIONFONTEX:
        {
        LOGFONT * logfont32;

        STOREDWORD(imestruct32->fnc, IME_SETCONVERSIONFONTEX);
        if (!ptag16->lParam1) {
            imestruct32->lParam1 = (ULONG)NULL;
            break;
        }

        // HANDLE of LOGFONT check
        // If lParam1 is Invalid Handle, hLFNT32 is NULL
        if (FETCHWORD(ptag16->lParam1) &&
            (vp = GlobalLock16(FETCHWORD(ptag16->lParam1), NULL))) {
            hLFNT32 = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, sizeof(LOGFONT));
            logfont32 = GlobalLock(hLFNT32);
            // GETMISCPTR(vp, logfont16);
            GETLOGFONT16(vp, logfont32);
            GlobalUnlock16(FETCHWORD(ptag16->lParam1));
            GlobalUnlock(hLFNT32);
        }
        else {
            hLFNT32 = NULL;
        }
        STOREDWORD(imestruct32->lParam1, hLFNT32);
        }
        break;

    case IME_PRIVATE:
        LOGDEBUG(0,("    ERROR:SendIMEMessage IME_PRIVATE NOT IMPLEMENTED\n"));
        goto eee;

    case IME_ENABLEDOSIME:      // (IME_ENABLE)
    default:

        LOGDEBUG(0,("    ERROR:SendIMEMessage unexpected subfunction\n"));
        LOGDEBUG(1,("    WINNLS:SENDIMEMESSAGE %s\n",
        wow32imedebug[ptag16->fnc]));
        goto eee;
    }

    LOGDEBUG(1,("    WINNLS:SENDIMEMESSAGE %s\n",
    wow32imedebug[ptag16->fnc]));

    if (ptag16 -> fnc != IME_SETCONVERSIONWINDOW) {
        LOGDEBUG(1,("WINNLS: fnc == %x wParam == %x wCount == %x\n",
        imestruct32->fnc, imestruct32->wParam, imestruct32->wCount ));
        LOGDEBUG(1,("WINNLS: dchDest == %x dchSource == %x\n",
        imestruct32->dchDest, imestruct32->dchSource));
        LOGDEBUG(1,("WINNLS: lParam1 == %x  lParam2 == %x  lParam3 == %x\n",
        imestruct32->lParam1, imestruct32->lParam2, imestruct32->lParam3));
        LOGDEBUG(1,("WINNLS: hwnd == %x %x\n",
        parg16->hwnd,HWND32(parg16->hwnd)));
    }

    GlobalUnlock(hIME32);

    // For win31 compatibility, since win31 didn't check the first
    // parm, check it here and fill in a dummy (WOW) hwnd if its bogus
    // so that NT doesn't reject the call

    ul = SendIMEMessageEx(
        ((parg16->hwnd) ? HWND32(parg16->hwnd) : (HWND)0xffff0000),
        (LPARAM)hIME32);

    LOGDEBUG(1,("WINNLS: Ret == %x\n", ul ));

    imestruct32 = GlobalLock(hIME32);

    LOGDEBUG(1,("WINNLS: wParam == %x\n\n", imestruct32->wParam ));

    STOREWORD(ptag16->wParam, ul);

    switch (ptag16->fnc) {
    case IME_GETOPEN:
        STOREWORD(ptag16->wCount, imestruct32->wCount);
        break;

    case IME_ENTERWORDREGISTERMODE: // (IME_WORDREGISTER)
        if (hlParam1)
            GlobalFree(hlParam1);
        if (hlParam2)
            GlobalFree(hlParam2);
        break;

    case IME_SETCONVERSIONFONT:     // (IME_SETFONT)
    {
        HAND16  hTmp;
        hTmp = ptag16->wParam;
        ptag16->wParam = hFnt16;
        hFnt16 = hTmp;
        if ( hLFNT32 )
            GlobalFree(hLFNT32);
    }
    break;

    case IME_SETCONVERSIONFONTEX:
        if ( hLFNT32 )
            GlobalFree(hLFNT32);
        break;

    case IME_GETVERSION:
        // PowerPoint4J must have version returned as 3.1
        // Or else it thinks that the ime doesn't support IR_UNDETERMINE
        if (CURRENTPTD()->dwWOWCompatFlags2 & WOWCF_PPT4J_IME_GETVERSION) {
            STOREWORD(ptag16->wParam, 0x0A03);
        }
        // WARNING: For DaytonaJ RC1 only!!!
        // Tell Winword6J that the IME doesn't support TrueInline (undetermine msgs)
        // So, that WinWord6J doesn't hang up doing the input loop processing of it.
        else if (CURRENTPTD()->dwWOWCompatFlags2 & WOWCF_WORDJ_IME_GETVERSION) {
            STOREWORD(ptag16->wParam, 0x0003);
        }
        break;

    default:
        break;
    }
eee:
    GlobalUnlock(hIME32);
    GlobalFree(hIME32);
    GlobalUnlock16(FETCHWORD(parg16->lParam));
    FREEVDMPTR(ptag16);
    FREEARGPTR(parg16);

    return(ul);
}


ULONG FASTCALL  WN32SendIMEMessageEx(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    PSENDIMEMESSAGE16 parg16;
    IMESTRUCT * imestruct32;
    register PIMESTRUCT16 ptag16;
    HANDLE hIME32;
    INT cb;
    VPVOID  vp;
    HANDLE  hlParam1 = NULL;        // IME_ENTERWORDREGISTERMODE
    HANDLE  hlParam2 = NULL;
    HANDLE hLFNT32;                 // IME_SETCONVERSIONFONT(EX)

    GETARGPTR(pFrame, sizeof(SENDIMEMESSAGE16), parg16);
    vp = GlobalLock16(FETCHWORD(parg16->lParam), NULL);
    GETMISCPTR(vp, ptag16);

    hIME32 = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, sizeof(IMESTRUCT));
    imestruct32 = GlobalLock(hIME32);

    if (ptag16 == NULL) {
        LOGDEBUG(1,("   WINNLS:(Jun)ptag16==NULL!!"));
        goto eee;
    }
    switch (ptag16->fnc) {
    case IME_HANJAMODE:
        // Korea specific function
        if (GetSystemDefaultLangID() != 0x412)
            goto eee;

        STOREDWORD(imestruct32->fnc, ptag16->fnc);
        STOREDWORD(imestruct32->wParam, 0);

        // The 4th word of imestruct32 must contains ptag16->dchSource.
        // msime95 will find ptag16->dchSource on the 4th word.
        *((LPSTR)(imestruct32) + sizeof(ptag16->fnc) +
                                 sizeof(ptag16->wParam) +
                                 sizeof(ptag16->wCount) )
                = (CHAR)ptag16->dchSource;

        *((LPSTR)(imestruct32) + ptag16->dchSource)
                = *(LPSTR)((LPSTR)(ptag16) + (ptag16)->dchSource);

        *((LPSTR)(imestruct32) + ptag16->dchSource + 1)
                = *(LPSTR)((LPSTR)(ptag16) + (ptag16)->dchSource + 1);
        break;

    case IME_CONVERTLIST:
    case IME_AUTOMATA:
    case IME_CODECONVERT:
    case IME_SETLEVEL:
    case IME_GETLEVEL:
    case IME_GETMNTABLE:
        // Korea specific function
        if (GetSystemDefaultLangID() != 0x412)
            goto eee;
        goto standard;

    case IME_SETCONVERSIONWINDOW:   // (IME_MOVECONVERTWINDOW)
                                    //  IME_MOVEIMEWINDOW for Korea
        if (GetSystemDefaultLangID() != 0x412 &&
            CURRENTPTD()->dwWOWCompatFlags2 & WOWCF_AMIPRO_PM4J_IME) {
            // Don't pass the MCW_DEFAULT.
            // because, Conversion window will be flushed when
            // default conversion window and AMIPRO's window have overlap.
            //
            // Also, for PM4J, when the codebox is moved Pagemaker
            // thinks it needs to be displayed at default. Prevent
            // endless loop of default screen|window displays
            //
            if (ptag16->wParam == MCW_DEFAULT) {
                ul = FALSE;
                goto eee;
            }
        }

    case IME_GETOPEN:
    case IME_SETOPEN:
    case IME_GETIMECAPS:        // (IME_QUERY)
    case IME_SETCONVERSIONMODE:     // (IME_SET_MODE)
    case IME_GETCONVERSIONMODE:     // (IME_GET_MODE)
    case IME_SENDVKEY:          // (IME_SENDKEY)
    case IME_DESTROYIME:        // (IME_DESTROY)
    case IME_WINDOWUPDATE:
    case IME_SELECT:
    case IME_GETVERSION:        // Win3.1
standard:
        STOREDWORD(imestruct32->fnc, ptag16->fnc);
        STOREDWORD(imestruct32->wParam, ptag16->wParam);
        STOREDWORD(imestruct32->wCount, ptag16->wCount);
        STOREDWORD(imestruct32->dchSource, ptag16->dchSource);
        STOREDWORD(imestruct32->dchDest, ptag16->dchDest);
        STOREDWORD(imestruct32->lParam1, ptag16->lParam1);
        STOREDWORD(imestruct32->lParam2, ptag16->lParam2);
        STOREDWORD(imestruct32->lParam3, ptag16->lParam3);
        break;

    case IME_ENTERWORDREGISTERMODE: // (IME_WORDREGISTER)
    {
        LPBYTE  lpMem16;
        LPBYTE  lpMem32;

        STOREDWORD(imestruct32->fnc, ptag16->fnc);
        if (ptag16->lParam1) {
            vp = GlobalLock16(FETCHWORD(ptag16->lParam1), &cb);
            hlParam1 = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, cb + 1);
            lpMem32 = GlobalLock(hlParam1);
            GETMISCPTR(vp, lpMem16);
            RtlCopyMemory(lpMem32, lpMem16, cb);
            GlobalUnlock(hlParam1);
            GlobalUnlock16(FETCHWORD(ptag16->lParam1));
        }
        if (ptag16->lParam2) {
            vp = GlobalLock16(FETCHWORD(ptag16->lParam2), &cb);
            hlParam2 = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, cb + 1);
            lpMem32 = GlobalLock(hlParam2);
            GETMISCPTR(vp, lpMem16);
            RtlCopyMemory(lpMem32, lpMem16, cb);
            GlobalUnlock(hlParam2);
            GlobalUnlock16(FETCHWORD(ptag16->lParam2));
        }
        imestruct32->lParam1 = (LPARAM)hlParam1;
        imestruct32->lParam2 = (LPARAM)hlParam2;
        STOREDWORD(imestruct32->lParam3, ptag16->lParam3);
    }
    break;

    case IME_SETCONVERSIONFONT:     // (IME_SET_MODE - Korea)
        {
        LOGFONT   * logfont32;

        if (GetSystemDefaultLangID() == 0x412) {
            // Hunguel WOW should do anything for IME_SET_MODE function
            goto eee;
        }

        STOREDWORD(imestruct32->fnc, IME_SETCONVERSIONFONTEX);
        if ( ptag16->wParam ) {
            hLFNT32 = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, sizeof(LOGFONT));
            logfont32 = GlobalLock(hLFNT32);
            GetObject(HOBJ32(ptag16->wParam), sizeof(LOGFONT), logfont32);
            GlobalUnlock(hLFNT32);
        }
        else {
            hLFNT32 = NULL;
        }
        imestruct32->lParam1 = (LPARAM)hLFNT32;
        }
        break;

    case IME_SETCONVERSIONFONTEX:   // Win3.1
    {
        LOGFONT   * logfont32;

        STOREDWORD(imestruct32->fnc, IME_SETCONVERSIONFONTEX);

        if (!(ptag16->lParam1)) {
            imestruct32->lParam1 = (LPARAM)NULL;
            break;
        }
        hLFNT32 = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, sizeof(LOGFONT));
        logfont32 = GlobalLock(hLFNT32);
        vp = GlobalLock16(FETCHWORD(ptag16->lParam1), NULL);
        // GETMISCPTR(vp, logfont16);
        GETLOGFONT16(vp, logfont32);
        GlobalUnlock16(FETCHWORD(ptag16->lParam1));
        GlobalUnlock(hLFNT32);
        imestruct32->lParam1 = (LPARAM)hLFNT32;
    }
    break;

    case IME_PRIVATE:
        LOGDEBUG(0,("    ERROR:SendIMEMessageEx IME_PRIVATE NOT YET IMPLEMENTED\n"));
        goto eee;

    case IME_ENABLEDOSIME:      // (IME_ENABLE)
    default:
        LOGDEBUG(0,("    ERROR:SendIMEMessageEx unexpected subfunction\n"));
        LOGDEBUG(1,("    WINNLS:SENDIMEMESSAGEEX %s\n",
        wow32imedebug[ptag16->fnc]));
        goto eee;
    }

    LOGDEBUG(1,("    WINNLS:SENDIMEMESSAGEEX %s\n",
        wow32imedebug[ptag16->fnc]));
    LOGDEBUG(1,("    IMESTRUCT16 Size = %d\n",
        sizeof(IMESTRUCT16)));
    LOGDEBUG(1,("WINNLS: IMESTRUCT.fnc == %x wParam == %x\n",
        imestruct32->fnc,imestruct32->wParam));
    LOGDEBUG(1,("WINNLS: IMESTRUCT.wCount == %x dchSource == %x\n",
        imestruct32->wCount,imestruct32->dchSource));
    LOGDEBUG(1,("WINNLS: IMESTRUCT.dchDest == %x lParam1 == %x\n",
        imestruct32->dchDest,imestruct32->lParam1));
    LOGDEBUG(1,("WINNLS: IMESTRUCT.lParam2 == %x lParam3 == %x\n",
        imestruct32->lParam2,imestruct32->lParam3));
    LOGDEBUG(1,("WINNLS: hwnd == %x %x\n",
        parg16->hwnd,HWND32(parg16->hwnd)));

    GlobalUnlock(hIME32);

    // For win31 compatibility, since win31 didn't check the first
    // parm, check it here and fill in a dummy (WOW) hwnd if its bogus
    // so that NT doesn't reject the call

    ul = SendIMEMessageEx(
        ((parg16->hwnd) ? HWND32(parg16->hwnd) : (HWND)0xffff0000),
        (LPARAM)hIME32);

    LOGDEBUG(1,("WINNLS: Ret == %x\n", ul ));

    imestruct32=GlobalLock(hIME32);

    LOGDEBUG(1,("WINNLS: wParam == %x\n\n", imestruct32->wParam ));

    STOREWORD(ptag16->wParam, imestruct32->wParam);

    switch (ptag16->fnc) {
    case IME_GETOPEN:
        STOREWORD(ptag16->wCount, imestruct32->wCount);
        break;
    case IME_ENTERWORDREGISTERMODE: // (IME_WORDREGISTER)
        if (hlParam1)
            GlobalFree(hlParam1);
        if (hlParam2)
            GlobalFree(hlParam2);
        break;
    case IME_SETCONVERSIONFONT:     // (IME_SETFONT)
    {
        HAND16  hTmp;
        hTmp = ptag16->wParam;
        ul = (hFnt16);
        hFnt16 = hTmp;
        ul = TRUE;
        // kksuszuka #1765 v-hidekk
        if(hLFNT32)
            GlobalFree(hLFNT32);
    }
    break;
    case IME_SETCONVERSIONFONTEX:
        if(ptag16->lParam1)
            GlobalFree(hLFNT32);
        break;
    case IME_GETVERSION:
        // PowerPoint4J must have version returned as 3.1
        // Or else it thinks that the ime doesn't support IR_UNDETERMINE
        if (CURRENTPTD()->dwWOWCompatFlags2 & WOWCF_PPT4J_IME_GETVERSION) {
            STOREWORD(ptag16->wParam, 0x0A03);
        }
        // WARNING: For DaytonaJ RC1 only!!!
        // Tell Winword6J that the IME doesn't support TrueInline (undetermine msgs)
        // So, that WinWord6J doesn't hang up doing the input loop processing of it.
        else if (CURRENTPTD()->dwWOWCompatFlags2 & WOWCF_WORDJ_IME_GETVERSION) {
            STOREWORD(ptag16->wParam, 0x0003);
        }
        break;

    default:
        break;
    }
eee:
    GlobalUnlock(hIME32);
    GlobalFree(hIME32);
    GlobalUnlock16(FETCHWORD(parg16->lParam));
    FREEVDMPTR(ptag16);
    FREEARGPTR(parg16);

    return(ul);
}


ULONG FASTCALL  WN32WINNLSGetIMEHotkey(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWINNLSGETIMEHOTKEY16 parg16;

    GETARGPTR(pFrame, sizeof(WINNLSGETIMEHOTKEY16), parg16);

    LOGDEBUG(1,("    WINNLS:GetIMEHotkey %x \n",
        parg16->hwnd));

    ul = GETWORD16(WINNLSGetIMEHotkey(
    HWND32(parg16->hwnd)
    ));
    FREEARGPTR(parg16);

    RETURN(ul);
}


ULONG FASTCALL  WN32WINNLSEnableIME(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWINNLSENABLEIME16 parg16;

    GETARGPTR(pFrame, sizeof(WINNLSENABLEIME16), parg16);

    // The spec says the first parameter should always be NULL.
    // Windows 3.1 ignores the first parameter and lets the call proceed.
    // Windows NT ignores the call if the first paramater is non-null
    // For compatibility purposes, pass NULL to user32 so that the call
    // will proceed as in win3.1
    //

    ul = GETBOOL16(WINNLSEnableIME( NULL, WORD32(parg16->fEnabled) ));

    LOGDEBUG(1,("    WINNLS:EnableIME %x %x %x\n",
        parg16->hwnd, parg16->fEnabled, ul ));

    FREEARGPTR(parg16);

    RETURN(ul);
}


ULONG FASTCALL  WN32WINNLSGetEnableStatus(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWINNLSGETENABLESTATUS16 parg16;

    GETARGPTR(pFrame, sizeof(WINNLSGETENABLESTATUS16), parg16);

    LOGDEBUG(1,("    WINNLS:GetEnableStatus %x \n",
        parg16->hwnd));

    // Call the user32 with a NULL pwnd for the same reason as
    // in WINNLSEnableIME above.
    //
    ul = GETWORD16(WINNLSGetEnableStatus( NULL ));

    FREEARGPTR(parg16);

    RETURN(ul);
}


ULONG FASTCALL  WN32IMPQueryIME(PVDMFRAME pFrame)
{
    ULONG ul;
    PIMPQUERYIME16 parg16;
    register PIMEPRO16 pime16;
    PIMEPRO  pimepro32;
    HANDLE hIME32;

    GETARGPTR(pFrame, sizeof(IMPQUERYIME16), parg16);
    GETVDMPTR(parg16->lpIMEPro,sizeof(IMEPRO16), pime16);
    if(pime16==NULL){
    LOGDEBUG(1,("   WINNLS:(Jun)pime16==NULL!!"));
        goto fff;

    }
    LOGDEBUG(1,("    WINNLS:IMPQueryIME called\n"));
    hIME32=GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, sizeof(IMEPRO));
    pimepro32=GlobalLock(hIME32);
    if (pime16->szName[0])
        GETIMEPRO16(pimepro32,pime16);
    else
        pimepro32->szName[0]=pime16->szName[0];

    ul=IMPQueryIME(pimepro32);

    SETIMEPRO16(pime16,pimepro32);

    GlobalUnlock(hIME32);
    GlobalFree(hIME32);
fff:
    FREEVDMPTR(pime16);
    FREEARGPTR(parg16);
    return (ul);
}


ULONG FASTCALL  WN32IMPGetIME(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    PIMPGETIME16 parg16;
    register PIMEPRO16 pime16;
    PIMEPRO  pimepro32;
    HANDLE hIME32;

    GETARGPTR(pFrame, sizeof(IMPGETIME16), parg16);
    GETVDMPTR(parg16->lpIMEPro,sizeof(IMEPRO16), pime16);
    if(pime16==NULL){
    LOGDEBUG(1,("   WINNLS:(Jun)pime16==NULL!!"));
        goto fff;

    }
    LOGDEBUG(1,("    WINNLS:IMPGetIME called\n"));
    hIME32=GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, sizeof(IMEPRO));
    pimepro32=GlobalLock(hIME32);

    // not use app,s handle IMPGetIME(HWND32(parg16->hwnd), pimepro32);
    ul=IMPGetIME(NULL, pimepro32);

    SETIMEPRO16(pime16, pimepro32);

    GlobalUnlock(hIME32);
    GlobalFree(hIME32);
fff:
    FREEVDMPTR(pime16);
    FREEARGPTR(parg16);
    return (ul);
}


ULONG FASTCALL  WN32IMPSetIME(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    PIMPSETIME16 parg16;
    register PIMEPRO16 pime16;
    PIMEPRO  pimepro32;
    HANDLE hIME32;
    INT i;

    GETARGPTR(pFrame, sizeof(IMPSETIME16), parg16);
    GETVDMPTR(parg16->lpIMEPro,sizeof(IMEPRO16), pime16);
    if(pime16==NULL){
    LOGDEBUG(1,("   WINNLS:(Jun)pime16==NULL!!"));
        goto fff;

    }
    LOGDEBUG(1,("    WINNLS:IMPSetIME called\n"));
    hIME32=GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, sizeof(IMEPRO));
    pimepro32=GlobalLock(hIME32);
    if (pime16->szName[0]) {
    for(i=0; i < (sizeof(pimepro32->szName) /
                 sizeof(pimepro32->szName[0])); i++)
    pimepro32->szName[i]=pime16->szName[i];
    }
    else
    pimepro32->szName[0]=pime16->szName[0];

    // not use app,s handle IMPSetIME(HWND32(parg16->hwnd), pimepro32);
    ul = IMPSetIME(NULL, pimepro32);

    GlobalUnlock(hIME32);
    GlobalFree(hIME32);
fff:
    FREEVDMPTR(pime16);
    FREEARGPTR(parg16);
    return (ul);
}


VOID GETIMEPRO16(PIMEPRO pimepro32,PIMEPRO16 pime16)
{
    INT i;

    pimepro32->hWnd = HWND32(pime16->hWnd);
    STOREWORD(pimepro32->InstDate.year, pime16->InstDate.year);
    STOREWORD(pimepro32->InstDate.month, pime16->InstDate.month);
    STOREWORD(pimepro32->InstDate.day, pime16->InstDate.day);
    STOREWORD(pimepro32->InstDate.hour, pime16->InstDate.hour);
    STOREWORD(pimepro32->InstDate.min, pime16->InstDate.min);
    STOREWORD(pimepro32->InstDate.sec, pime16->InstDate.sec);
    STOREWORD(pimepro32->wVersion, pime16->wVersion);

    for(i=0;i<(sizeof(pimepro32->szDescription)/
                 sizeof(pimepro32->szDescription[0]));i++)
    pimepro32->szDescription[i]=pime16->szDescription[i];

    for(i=0;i<(sizeof(pimepro32->szName)/
                 sizeof(pimepro32->szName[0]));i++)
    pimepro32->szName[i]=pime16->szName[i];

    for(i=0;i<(sizeof(pimepro32->szOptions)/
                 sizeof(pimepro32->szOptions[0]));i++)
    pimepro32->szOptions[i]=pime16->szOptions[i];

}


VOID SETIMEPRO16(PIMEPRO16 pime16, PIMEPRO pimepro32)
{
    INT i;

    pime16->hWnd = GETHWND16(pimepro32->hWnd);
    STOREWORD(pime16->InstDate.year,pimepro32->InstDate.year);
    STOREWORD(pime16->InstDate.month,pimepro32->InstDate.month);
    STOREWORD(pime16->InstDate.day,pimepro32->InstDate.day);
    STOREWORD(pime16->InstDate.hour,pimepro32->InstDate.hour);
    STOREWORD(pime16->InstDate.min,pimepro32->InstDate.min);
    STOREWORD(pime16->InstDate.sec,pimepro32->InstDate.sec);
    STOREWORD(pime16->wVersion,pimepro32->wVersion);

    for(i=0;i<(sizeof(pimepro32->szDescription)/
                 sizeof(pimepro32->szDescription[0]));i++)
    pime16->szDescription[i]=pimepro32->szDescription[i];

    for(i=0;i<(sizeof(pimepro32->szName)/
                 sizeof(pimepro32->szName[0]));i++)
    pime16->szName[i]=pimepro32->szName[i];

    for(i=0;i<(sizeof(pimepro32->szOptions)/
                 sizeof(pimepro32->szOptions[0]));i++)
    pime16->szOptions[i]=pimepro32->szOptions[i];

}


//
// Notify IMM32 wow task exit so that
// it can perform any clean up.
//
VOID WN32WINNLSSImeNotifyTaskExit()
{
#if 0
    HANDLE hIME32;
    IMESTRUCT * imestruct32;

    hIME32 = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, sizeof(IMESTRUCT));
    if ( hIME32 == NULL )
        return;

    if ( (imestruct32 = GlobalLock(hIME32) ) != NULL ) {
        imestruct32->fnc = IME_NOTIFYWOWTASKEXIT;
        GlobalUnlock(hIME32);
        SendIMEMessageEx( NULL, (LPARAM)hIME32 );
    }
    GlobalFree(hIME32);
#endif
}
#endif // FE_IME
