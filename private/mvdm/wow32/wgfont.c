/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGFONT.C
 *  WOW32 16-bit GDI API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop
#include "wingdip.h"

MODNAME(wgfont.c);

extern int RemoveFontResourceTracking(LPCSTR psz, UINT id);
extern int AddFontResourceTracking(LPCSTR psz, UINT id);


// for Quickbooks v4 & v5 OCR font support
void LoadOCRFont(void);
char szOCRA[]      = "OCR-A";
char szFonts[]     = "\\FONTS";
char szSystem[]    = "\\SYSTEM";
char szOCRDotTTF[] = "\\OCR-A.TTF";
BOOL gfOCRFontLoaded = FALSE;


// a.k.a. WOWAddFontResource
ULONG FASTCALL WG32AddFontResource(PVDMFRAME pFrame)
{
    ULONG    ul;
    PSZ      psz1;
    register PADDFONTRESOURCE16 parg16;

    GETARGPTR(pFrame, sizeof(ADDFONTRESOURCE16), parg16);
    GETPSZPTR(parg16->f1, psz1);

    // note: we will never get an hModule in the low word here.
    //       the 16-bit side resolves hModules to an lpsz before calling us

    if( CURRENTPTD()->dwWOWCompatFlags & WOWCF_UNLOADNETFONTS )
    {
        ul = GETINT16(AddFontResourceTracking(psz1,(UINT)CURRENTPTD()));
    }
    else
    {
        ul = GETINT16(AddFontResourceA(psz1));
    }

    FREEPSZPTR(psz1);
    FREEARGPTR(parg16);

    RETURN(ul);
}


#define PITCH_MASK  ( FIXED_PITCH | VARIABLE_PITCH )

ULONG FASTCALL WG32CreateFont(PVDMFRAME pFrame)
{
    ULONG    ul;
    PSZ      psz14;
    register PCREATEFONT16 parg16;
    INT      iWidth;
    char     achCapString[LF_FACESIZE];
    BYTE     lfCharSet;
    BYTE     lfPitchAndFamily;
#ifdef FE_SB
    BOOL     bUseAlternateFace = FALSE;
#endif    

    GETARGPTR(pFrame, sizeof(CREATEFONT16), parg16);
    GETPSZPTR(parg16->f14, psz14);

    // take careof compatiblity flags:
    //   if a specific width is specified and GACF_30AVGWIDTH compatiblity
    //   flag is set, scaledown the width by 7/8.
    //

    iWidth = INT32(parg16->f2);
    if (iWidth != 0 &&
           (W32GetAppCompatFlags((HAND16)NULL) & GACF_30AVGWIDTH)) {
        iWidth = (iWidth * 7) / 8;
    }

    lfCharSet        = BYTE32(parg16->f9);
    lfPitchAndFamily = BYTE32(parg16->f13);

#ifdef FE_SB
    if (psz14 && *psz14)
#else // !FE_SB
    if (psz14)
#endif // !FE_SB
    {
        // Capitalize the string for faster compares.

        WOW32_strncpy(achCapString, psz14, LF_FACESIZE);
        WOW32_strupr(achCapString);

        // Here we are going to implement a bunch of Win 3.1 hacks rather
        // than contaminate the 32-bit engine.  These same hacks can be found
        // in WOW (in the CreateFont/CreateFontIndirect code).
        //
        // These hacks are keyed off the facename in the LOGFONT.  String
        // comparisons have been unrolled for maximal performance.

        // Win 3.1 facename-based hack.  Some apps, like
        // Publisher, create a "Helv" font but have the lfPitchAndFamily
        // set to specify FIXED_PITCH.  To work around this, we will patch
        // the pitch field for a "Helv" font to be variable.

        if ( !WOW32_strcmp(achCapString, szHelv) )
        {
            lfPitchAndFamily |= ( (lfPitchAndFamily & ~PITCH_MASK) | VARIABLE_PITCH );
        }
        else
        {
            // Win 3.1 hack for Legacy 2.0.  When a printer does not enumerate
            // a "Tms Rmn" font, the app enumerates and gets the LOGFONT for
            // "Script" and then create a font with the name "Tms Rmn" but with
            // the lfCharSet and lfPitchAndFamily taken from the LOGFONT for
            // "Script".  Here we will over the lfCharSet to be ANSI_CHARSET.

            if ( !WOW32_strcmp(achCapString, szTmsRmn) )
            {
                lfCharSet = ANSI_CHARSET;
            }
            else
            {
                // If the lfFaceName is "Symbol", "Zapf Dingbats", or "ZapfDingbats",
                // enforce lfCharSet to be SYMBOL_CHARSET.  Some apps (like Excel) ask
                // for a "Symbol" font but have the char set set to ANSI.  PowerPoint
                // has the same problem with "Zapf Dingbats".

                if ( !WOW32_strcmp(achCapString, szSymbol) ||
                     !WOW32_strcmp(achCapString, szZapfDingbats) ||
                     !WOW32_strcmp(achCapString, szZapf_Dingbats) )
                {
                    lfCharSet = SYMBOL_CHARSET;
                }
            }
        }

        // Win3.1(Win95) hack for Mavis Beacon Teaches Typing 3.0
        // The app uses a fixed width of 34*13 for the typing screen.
        // NT returns 14 from GetTextExtent for Mavis Beacon Courier FP font (width of 14)
        // while Win95 returns 13, thus long strings won't fit in the typing screen on NT.
        // Force the width to 13.

        if ( iWidth==14 && (INT32(parg16->f1)== 20) && !WOW32_strcmp(achCapString, szMavisCourier))
        {
           iWidth = 13;
        }

#ifdef FE_SB
       // WOW_ICHITARO_ITALIC
       // Ichitaro asks for System Mincho because WIFE fonts aren't installed
       // we give it a proportional font which is can't handle.  If we see
       // this face name we will replace it with Ms Mincho

        if (GetSystemDefaultLangID() == 0x411 &&
            CURRENTPTD()->dwWOWCompatFlags2 & WOW_ICHITARO_ITALIC ) 
        {
            if(!WOW32_strcmp(achCapString, szSystemMincho))
            {
                strcpy(achCapString, szMsMincho);
                bUseAlternateFace = TRUE;
            }
        }
#endif // FE_SB

    }

#ifdef FE_SB
    ul = GETHFONT16(CreateFont(INT32(parg16->f1),
                               iWidth,
                               INT32(parg16->f3),
                               INT32(parg16->f4),
                               INT32(parg16->f5),
                               BYTE32(parg16->f6),
                               BYTE32(parg16->f7),
                               BYTE32(parg16->f8),
                               lfCharSet,
                               BYTE32(parg16->f10),
                               BYTE32(parg16->f11),
                               BYTE32(parg16->f12),
                               lfPitchAndFamily,
                               (bUseAlternateFace ? achCapString : psz14)
                               ));
#else
    ul = GETHFONT16(CreateFont(INT32(parg16->f1),
                               iWidth,
                               INT32(parg16->f3),
                               INT32(parg16->f4),
                               INT32(parg16->f5),
                               BYTE32(parg16->f6),
                               BYTE32(parg16->f7),
                               BYTE32(parg16->f8),
                               lfCharSet,
                               BYTE32(parg16->f10),
                               BYTE32(parg16->f11),
                               BYTE32(parg16->f12),
                               lfPitchAndFamily,
                               psz14));
#endif



    FREEPSZPTR(psz14);
    FREEARGPTR(parg16);

    RETURN(ul);
}


ULONG FASTCALL WG32CreateFontIndirect(PVDMFRAME pFrame)
{
    ULONG    ul;
    LOGFONT  logfont;
    register PCREATEFONTINDIRECT16 parg16;
    char     achCapString[LF_FACESIZE];

    GETARGPTR(pFrame, sizeof(CREATEFONTINDIRECT16), parg16);
    GETLOGFONT16(parg16->f1, &logfont);

    // Capitalize the string for faster compares.

    WOW32_strncpy(achCapString, logfont.lfFaceName, LF_FACESIZE);
    CharUpperBuff(achCapString, LF_FACESIZE);

    // Here we are going to implement a bunch of Win 3.1 hacks rather
    // than contaminate the 32-bit engine.  These same hacks can be found
    // in WOW (in the CreateFont/CreateFontIndirect code).
    //
    // These hacks are keyed off the facename in the LOGFONT.  String
    // comparisons have been unrolled for maximal performance.

    // Win 3.1 facename-based hack.  Some apps, like
    // Publisher, create a "Helv" font but have the lfPitchAndFamily
    // set to specify FIXED_PITCH.  To work around this, we will patch
    // the pitch field for a "Helv" font to be variable.

    if ( !WOW32_strcmp(achCapString, szHelv) )
    {
        logfont.lfPitchAndFamily |= ( (logfont.lfPitchAndFamily & ~PITCH_MASK) | VARIABLE_PITCH );
#ifdef FE_SB
        //
        // FE Win 3.1 facename-based hack.  Some FE apps
        // create a "Helv" font but have the lfCharSet
        // set to DBCS charset (ex. SHIFTJIS_CHARSET).
        // To work around this, we will wipe out the
        // lfFaceName[0] with '\0' and let GDI picks a
        // DBCS font for us.
        //
        if (IS_ANY_DBCS_CHARSET(logfont.lfCharSet))
            logfont.lfFaceName[0]='\0';
#endif // FE_SB
    }
    else
    {
        // Win 3.1 hack for Legacy 2.0.  When a printer does not enumerate
        // a "Tms Rmn" font, the app enumerates and gets the LOGFONT for
        // "Script" and then create a font with the name "Tms Rmn" but with
        // the lfCharSet and lfPitchAndFamily taken from the LOGFONT for
        // "Script".  Here we will over the lfCharSet to be ANSI_CHARSET.

        if ( !WOW32_strcmp(achCapString, szTmsRmn) )
        {
            logfont.lfCharSet = ANSI_CHARSET;
        }
        
        // for Quickbooks v4 & v5 OCR font support (see LoadOCRFont for details)
        else if ( !WOW32_strcmp(achCapString, szOCRA) )
        {

            // Further localize this hack to QuickBooks.  Most other apps won't
            // know about this quirk in this particular font.
            if(logfont.lfCharSet == SYMBOL_CHARSET) {
                logfont.lfCharSet = DEFAULT_CHARSET;

                if(!gfOCRFontLoaded) {
                    LoadOCRFont();
                }
            }
        }
        else
        {
            // If the lfFaceName is "Symbol", "Zapf Dingbats", or "ZapfDingbats",
            // enforce lfCharSet to be SYMBOL_CHARSET.  Some apps (like Excel) ask
            // for a "Symbol" font but have the char set set to ANSI.  PowerPoint
            // has the same problem with "Zapf Dingbats".

            if ( !WOW32_strcmp(achCapString, szSymbol) ||
                 !WOW32_strcmp(achCapString, szZapfDingbats) ||
                 !WOW32_strcmp(achCapString, szZapf_Dingbats) )
            {
                logfont.lfCharSet = SYMBOL_CHARSET;
            }

#ifdef FE_SB
       // WOW_ICHITARO_ITALIC
       // Ichitaro asks for System Mincho because WIFE fonts aren't installed
       // we give it a proportional font which is can't handle.  If we see
       // this face name we will replace it with Ms Mincho

        if (GetSystemDefaultLangID() == 0x411 &&
            CURRENTPTD()->dwWOWCompatFlags2 & WOW_ICHITARO_ITALIC ) 
        {
            if(!WOW32_strcmp(achCapString, szSystemMincho))
            {
                strcpy(logfont.lfFaceName, szMsMincho);
            }
        }
#endif // FE_SB
        }
    }

    ul = GETHFONT16(CreateFontIndirect(&logfont));

    FREEARGPTR(parg16);

    RETURN(ul);
}


LPSTR lpMSSansSerif = "MS Sans Serif";
LPSTR lpMSSerif     = "MS Serif";

INT W32EnumFontFunc(LPENUMLOGFONT pEnumLogFont,
                    LPNEWTEXTMETRIC pNewTextMetric, INT nFontType, PFNTDATA pFntData)
{
    INT    iReturn;
    PARM16 Parm16;
    LPSTR  lpFaceNameT = NULL;

    WOW32ASSERT(pFntData);

    // take care of compatibility flags:
    //  ORin DEVICE_FONTTYPE bit if the fonttype is truetype and  the
    //  Compataibility flag GACF_CALLTTDEVICE is set.
    //

    if (nFontType & TRUETYPE_FONTTYPE) {
        if (W32GetAppCompatFlags((HAND16)NULL) & GACF_CALLTTDEVICE) {
            nFontType |= DEVICE_FONTTYPE;
        }
    }

    // take care of compatibility flags:
    //   replace Ms Sans Serif with Helv and
    //   replace Ms Serif      with Tms Rmn
    //
    // only if the facename is NULL and the compat flag GACF_ENUMHELVNTMSRMN
    // is set.

    if (pFntData->vpFaceName == (VPVOID)NULL) {
        if (W32GetAppCompatFlags((HAND16)NULL) & GACF_ENUMHELVNTMSRMN) {
            if (!WOW32_strcmp(pEnumLogFont->elfLogFont.lfFaceName, lpMSSansSerif)) {
                strcpy(pEnumLogFont->elfLogFont.lfFaceName, "Helv");
                lpFaceNameT = lpMSSansSerif;
            }
            else if (!WOW32_strcmp(pEnumLogFont->elfLogFont.lfFaceName, lpMSSerif)) {
                strcpy(pEnumLogFont->elfLogFont.lfFaceName, "Tms Rmn");
                lpFaceNameT = lpMSSerif;
            }
        }
    }

CallAgain:

    // be sure allocation size matches stackfree16() size below
    pFntData->vpLogFont    = stackalloc16(sizeof(ENUMLOGFONT16)+sizeof(NEWTEXTMETRIC16));

    pFntData->vpTextMetric = (VPVOID)((LPSTR)pFntData->vpLogFont + sizeof(ENUMLOGFONT16));

    PUTENUMLOGFONT16(pFntData->vpLogFont, pEnumLogFont);
    PUTNEWTEXTMETRIC16(pFntData->vpTextMetric, pNewTextMetric);

    STOREDWORD(Parm16.EnumFontProc.vpLogFont, pFntData->vpLogFont);
    STOREDWORD(Parm16.EnumFontProc.vpTextMetric, pFntData->vpTextMetric);
    STOREDWORD(Parm16.EnumFontProc.vpData,pFntData->dwUserFntParam);

    Parm16.EnumFontProc.nFontType = (SHORT)nFontType;

    CallBack16(RET_ENUMFONTPROC, &Parm16, pFntData->vpfnEnumFntProc, (PVPVOID)&iReturn);

    if(pFntData->vpLogFont) {
        stackfree16(pFntData->vpLogFont,
                    (sizeof(ENUMLOGFONT16) + sizeof(NEWTEXTMETRIC16)));
    }

    if (((SHORT)iReturn) && lpFaceNameT) {
        // if the callback returned true, now call with the actual facename
        // Just to be sure, we again copy all the data for callback. This will
        // take care of any apps which modify the passed in structures.

        strcpy(pEnumLogFont->elfLogFont.lfFaceName, lpFaceNameT);
        lpFaceNameT = (LPSTR)NULL;
        goto CallAgain;
    }
    return (SHORT)iReturn;
}


ULONG  W32EnumFontHandler( PVDMFRAME pFrame, BOOL fEnumFontFamilies )
{
    ULONG    ul = 0;
    PSZ      psz2;
    FNTDATA  FntData;
    register PENUMFONTS16 parg16;

    GETARGPTR(pFrame, sizeof(ENUMFONTS16), parg16);
    GETPSZPTR(parg16->f2, psz2);

    FntData.vpfnEnumFntProc = DWORD32(parg16->f3);
    FntData.dwUserFntParam  = DWORD32(parg16->f4);
    FntData.vpFaceName   = DWORD32(parg16->f2);


    if ( fEnumFontFamilies ) {
        ul = GETINT16(EnumFontFamilies(HDC32(parg16->f1),
                                       psz2,
                                       (FONTENUMPROC)W32EnumFontFunc,
                                       (LPARAM)&FntData));
    } else {
        ul = GETINT16(EnumFonts(HDC32(parg16->f1),
                                psz2,
                                (FONTENUMPROC)W32EnumFontFunc,
                                (LPARAM)&FntData));
    }



    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);

    RETURN(ul);
}



ULONG FASTCALL WG32EnumFonts(PVDMFRAME pFrame)
{
    return( W32EnumFontHandler( pFrame, FALSE ) );
}



ULONG FASTCALL WG32GetAspectRatioFilter(PVDMFRAME pFrame)
{
    ULONG    ul = 0;
    SIZE     size2;
    register PGETASPECTRATIOFILTER16 parg16;

    GETARGPTR(pFrame, sizeof(GETASPECTRATIOFILTER16), parg16);

    if (GETDWORD16(GetAspectRatioFilterEx(HDC32(parg16->f1), &size2))) {
        ul = (WORD)size2.cx | (size2.cy << 16);
    }

    FREEARGPTR(parg16);

    RETURN(ul);
}


ULONG FASTCALL WG32GetCharWidth(PVDMFRAME pFrame)
{
    ULONG    ul = 0L;
    INT      ci;
    PINT     pi4;
    register PGETCHARWIDTH16 parg16;
    INT      BufferT[256];

    GETARGPTR(pFrame, sizeof(GETCHARWIDTH16), parg16);

    ci = WORD32(parg16->wLastChar) - WORD32(parg16->wFirstChar) + 1;
    pi4 = STACKORHEAPALLOC(ci * sizeof(INT), sizeof(BufferT), BufferT);

    if (pi4) {
        ULONG ulLast = WORD32(parg16->wLastChar);
#ifdef FE_SB
        /*
         * If ulLast sets DBCS code (0x82xx), then below code is illigal.
         */
        if (ulLast > 0xff && !(IsDBCSLeadByte(HIBYTE(ulLast))))
#else // !FE_SB
        if (ulLast > 0xff)
#endif // !FE_SB
            ulLast = 0xff;

        ul = GETBOOL16(GetCharWidth(HDC32(parg16->hDC),
                                    WORD32(parg16->wFirstChar),
                                    ulLast,
                                    pi4));

        PUTINTARRAY16(parg16->lpIntBuffer, ci, pi4);
        STACKORHEAPFREE(pi4, BufferT);

    }

    FREEARGPTR(parg16);

    RETURN(ul);
}


// a.k.a. WOWRemoveFontResource
ULONG FASTCALL WG32RemoveFontResource(PVDMFRAME pFrame)
{
    ULONG    ul;
    PSZ      psz1;
    register PREMOVEFONTRESOURCE16 parg16;

    GETARGPTR(pFrame, sizeof(REMOVEFONTRESOURCE16), parg16);

    GETPSZPTR(parg16->f1, psz1);

    // note: we will never get an hModule in the low word here.
    //       the 16-bit side resolves hModules to an lpsz before calling us


    if( CURRENTPTD()->dwWOWCompatFlags & WOWCF_UNLOADNETFONTS )
    {
        ul = GETBOOL16(RemoveFontResourceTracking(psz1,(UINT)CURRENTPTD()));
    }
    else
    {
        ul = GETBOOL16(RemoveFontResource(psz1));
    }

    FREEPSZPTR(psz1);

    FREEARGPTR(parg16);

    RETURN(ul);
}


/* WG32GetCurLogFont
 *
 * This thunk implements the undocumented Win3.0 and Win3.1 API
 * GetCurLogFont (GDI.411). Symantec QA4.0 uses it.
 *
 * HFONT GetCurLogFont (HDC)
 * HDC   hDC;        // Device Context
 *
 * This function returns the current Logical font selected for the
 * specified device context.
 *
 * To implement this undocumented API we will use the NT undocumented API
 * GetHFONT.
 *
 * SudeepB 08-Mar-1996
 *
 */

extern HFONT APIENTRY GetHFONT (HDC hdc);

ULONG FASTCALL WG32GetCurLogFont(PVDMFRAME pFrame)
{

    ULONG    ul;
    register PGETCURLOGFONT16 parg16;

    GETARGPTR(pFrame, sizeof(GETCURLOGFONT16), parg16);

    ul = GETHFONT16 (GetHFONT(HDC32 (parg16->hDC)));

    FREEARGPTR(parg16);

    return (ul);
}



//
//  This allows Quickbooks v4 & v5 to use their OCR-A.TTF font right after they
//  install.  At the end of installation on both versions, you are asked if you
//  want to "restart" windows. If you click OK it logs you off of NT5, but does
//  *not* reboot the system -- which the app is counting on to cause the OCR-A 
//  font to be loaded. The result on W2K is that whenever the app uses the OCR-A
//  font, it will get mapped to wingdings instead.
//
//  This is further complicated by the fact that the font file OCR-A.TTF doesn't
//  specify the charset in the header.  On Win3.1, Win95, & pre-NT5, unspecified
//  charset's got mapped to the SYMBOL_CHARSET - therefore, Quickbooks specifies
//  SYMBOL_CHARSET in its LOGFONT struct to accomodate this.  (OCR-A apparently
//  is licenced from Monotype Typography, Ltd. which presumably is why Intuit
//  didn't fix the header issue in the font file).
//
//  This changed on Win98 and W2K, unspecified charset's now get mapped to the
//  DEFAULT_CHARSET.  This was done so these fonts will always map to a default 
//  localized font that will always be readable. Hence, the hack where we change
//  the charset from SYMBOL_CHARSET to DEFAULT_CHARSET in the LOGFONT struct.
//
//  On v4, the install program copies OCR-A.FOT & OCR-A.TTF to the SYSTEM dir.  
//  Once you "restart" (not reboot) the system & log back on, the OCR-A font is
//  added to the registry (as OCR-A.FOT) but the font files are still in the 
//  SYSTEM dir.  Rebooting causes the fonts files to be moved to the FONTS dir,
//  the registry entry is changed to OCR-A.TTF. (done by the "Font Sweeper")
//
//  On v5, the install program copies the .ttf & .fot files to the FONTS dir
//  but again, counts on the reboot to cause the fonts to be loaded.  It puts 
//  correct registry entry (OCR-A.TTF) in the registry fonts section.
//
//  The result of all this is:
//  For either version of the app, without the charset hack, you will always get
//  a wingding font instead of OCR-A.  With the charset hack, you will get a
//  readable font, such as Arial, until you reboot -- after which you will get
//  OCR-A for v5 but Arial for v4. With this function (in conjunction with the 
//  charset hack) both version will always get OCR-A with or without rebooting.
//
//  This function explicitly loads the OCR-A from the font files located in 
//  either the FONTS dir or the SYSTEM dir.
//
void LoadOCRFont(void)
{
    char  szFontPath[MAX_PATH];
    DWORD dw;
    int   cb;

    // get equivalent of "c:\windows" for this system
    dw = GetWindowsDirectory(szFontPath, MAX_PATH);

    // we're going to add a maximum of 18 chars "\SYSTEM\OCR-A.TTF"
    if(dw && ((MAX_PATH - 18) > dw)) {

        // build "c:\windows\FONTS\OCR-A.TTF"  (QuickBooks v5)
        strcat(szFontPath, szFonts);
        strcat(szFontPath, szOCRDotTTF); 

        // If font file doesn't exist in FONTS dir, this must be QuickBooks v4
        // The FR_PRIVATE flag means that the font will be unloaded when the vdm
        // process goes away.  The FR_NO_ENUM flag means that this instance of
        // the font can't be enumerated by other processes (it might go away
        // while the other processes are trying to use it).
        cb = AddFontResourceEx(szFontPath, FR_PRIVATE | FR_NOT_ENUM, NULL);
        if(!cb) {
                 
            // reset path to "c:\windows"
            szFontPath[dw] = '\0';

            // build "c:\windows\SYSTEM\OCR-A.TTF"
            strcat(szFontPath, szSystem);
            strcat(szFontPath, szOCRDotTTF); 
            
            cb = AddFontResourceEx(szFontPath, FR_PRIVATE | FR_NOT_ENUM, NULL);

            // if it wasn't loaded from the SYSTEM dir either, punt
        }

        if(cb) {

            // specify that the font is already loaded for the life of this VDM
            gfOCRFontLoaded = TRUE;
        }
    }
}        
