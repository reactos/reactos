#include <pifdump.h>
#pragma hdrstop


#define STRING(x) x[0]?x:"(empty string in .pif file)"


int __cdecl main( int argc, char *argv[])
{
    HFILE hFile;
    STDPIF stdpif;
    PIFEXTHDR pifexthdr;
    LONG cbRead;
    CHAR szTemp[ 512 ];
    memset( &stdpif, 0, sizeof(stdpif) );

    if (argc!=2)
    {
        printf("usage: pifdump filename.pif\n" );
        return(1);
    }


    hFile =  _lopen( argv[1], OF_READ );
    if (hFile == HFILE_ERROR)
    {
        printf( "pifdump: unable to open file %s, err %d\n",
                argv[1],
                GetLastError()
               );
        return(1);
    }

    _lread( hFile, &stdpif, sizeof(stdpif) );


    printf( "\n===== Dump of PIF file (%s) =====\n\n", argv[1] );

    printf( "[Standard PIF Data]\n" );
    printf( "    unknown    = 0x%02X\n", stdpif.unknown );
    printf( "    id         = 0x%02X\n", stdpif.id );
    printf( "    appname    = %s\n",     STRING(stdpif.appname) );
    printf( "    maxmem     = 0x%04X\n", stdpif.maxmem );
    printf( "    minmem     = 0x%04X\n", stdpif.minmem );
    printf( "    startfile  = %s\n",     STRING(stdpif.startfile) );
    printf( "    MSflags    = 0x%02X\n", stdpif.MSflags );
    if (stdpif.MSflags & fResident)
        printf( "        [fResident is set ==> directly modifies memory]\n", fResident );
    if (stdpif.MSflags & fGraphics)
        printf( "        [fGraphics is set ==> screen exchange: graphics/text]\n", fGraphics );
    if (stdpif.MSflags & fNoSwitch)
        printf( "        [fNoSwitch is set ==> program switch: prevent]\n", fNoSwitch );
    if (stdpif.MSflags & fNoGrab)
        printf( "        [fNoGrab   is set ==> screen exchange: none]\n", fNoGrab );
    if (stdpif.MSflags & fDestroy)
        printf( "        [fDestroy  is set ==> close window on exit]\n", fDestroy );
    if (stdpif.MSflags & fCOM1)
        printf( "        [fCOM1     is set ==> directly modifies: COM1]\n", fCOM1 );
    if (stdpif.MSflags & fCOM2)
        printf( "        [fCOM2     is set ==> directly modifies: COM2]\n", fCOM2 );
    printf( "    reserved   = 0x%02X\n", stdpif.reserved );
    printf( "    defpath    = %s\n",     STRING(stdpif.defpath) );
    printf( "    params     = %s\n",     STRING(stdpif.params) );
    printf( "    cPages     = 0x%02X   (should always be 0x01!)\n", stdpif.cPages );
    printf( "    lowVector  = 0x%02X   (should always be 0x00!)\n", stdpif.lowVector );
    printf( "    highVector = 0x%02X   (should always be 0xFF!)\n", stdpif.highVector );
    printf( "    rows       = 0x%02X   (not used)\n", stdpif.rows );
    printf( "    cols       = 0x%02X   (not used)\n", stdpif.cols );
    printf( "    rowoff     = 0x%02X   (not used)\n", stdpif.rowoff );
    printf( "    coloff     = 0x%02X   (not used)\n", stdpif.coloff );
    printf( "    sysmem     = 0x%04X (not used)\n", stdpif.sysmem );
    printf( "    behavior   = 0x%02X\n", stdpif.behavior);
    if (stdpif.behavior & fScreen)
        printf( "      [fScreen     (0x%02X) is set ==> directly modifies screen]\n", fScreen );
    if (stdpif.behavior & fForeground)
        printf( "      [fForeground (0x%02X) is set ==> Set same as fScreen (alias)]\n", fForeground );
    if (stdpif.behavior & f8087)
        printf( "      [f8087       (0x%02X) is set ==> No PIFEDIT control]\n", f8087 );
    if (stdpif.behavior & fKeyboard)
        printf( "      [fKeyboard   (0x%02X) is set ==> directly modifies keyboard]\n", fKeyboard );
    printf( "    sysflags   = 0x%02X\n", stdpif.sysflags);


    // Read in extended header sections
    while( (cbRead = _lread( hFile, &pifexthdr, sizeof( pifexthdr )))!=0 )
    {
        printf( "\n[%s]\n", pifexthdr.extsig );

        if (strcmp(pifexthdr.extsig, STDHDRSIG)==0) {


            if (pifexthdr.extnxthdrfloff==0xFFFF) {
                printf( "    No more entries\n" );
                goto OutOfHere;
            }
            else
                printf( "    No information in \"%s\" section (as expected)\n\n", STDHDRSIG );

        }

        if (strcmp(pifexthdr.extsig, W286HDRSIG30)==0) {

            W286PIF30   w286ext30;


            _lread( hFile, &w286ext30, sizeof( w286ext30 ) );
            printf( "    PfMaxXmsK   = 0x%04X\n", w286ext30.PfMaxXmsK );
            printf( "    PfMinXmsK   = 0x%04X\n", w286ext30.PfMinXmsK );
            printf( "    PfW286Flags = 0x%04X\n", w286ext30.PfW286Flags );
            if (w286ext30.PfW286Flags & fALTTABdis286)
                printf( "        fALTTABdis286   is set.\n" );
            if (w286ext30.PfW286Flags & fALTESCdis286)
                printf( "        fALTESCdis286   is set.\n" );
            if (w286ext30.PfW286Flags & fALTESCdis286)
                printf( "        fALTPRTSCdis286 is set.\n" );
            if (w286ext30.PfW286Flags & fPRTSCdis286)
                printf( "        fPRTSCdis286    is set.\n" );
            if (w286ext30.PfW286Flags & fCTRLESCdis286)
                printf( "        fCTRLESCdis286  is set.\n" );
            if (w286ext30.PfW286Flags & fNoSaveVid286)
                printf( "        fNoSaveVid286   is set.\n" );
            if (w286ext30.PfW286Flags & fCOM3_286)
                printf( "        fCOM3_286       is set.\n" );
            if (w286ext30.PfW286Flags & fCOM4_286)
                printf( "        fCOM4_286       is set.\n" );
            printf( "\n" );

        }

        if (strcmp(pifexthdr.extsig, W386HDRSIG30)==0) {
            W386PIF30   w386ext30;

            _lread( hFile, &w386ext30, sizeof( w386ext30 ) );
            printf( "    PfW386maxmem  = 0x%02X (%d)\n", w386ext30.PfW386maxmem, w386ext30.PfW386maxmem );
            printf( "    PfW386minmem  = 0x%02X (%d)\n", w386ext30.PfW386minmem, w386ext30.PfW386minmem );
            printf( "    PfFPriority   = 0x%02X (%d)\n", w386ext30.PfFPriority,  w386ext30.PfFPriority );
            printf( "    PfBPriority   = 0x%02X (%d)\n", w386ext30.PfBPriority,  w386ext30.PfBPriority );
            printf( "    PfMaxEMMK     = 0x%02X (%d)\n", w386ext30.PfMaxEMMK,    w386ext30.PfMaxEMMK );
            printf( "    PfMinEMMK     = 0x%02X (%d)\n", w386ext30.PfMinEMMK,    w386ext30.PfMinEMMK );
            printf( "    PfMaxXmsK     = 0x%02X (%d)\n", w386ext30.PfMaxXmsK,    w386ext30.PfMaxXmsK );
            printf( "    PfMinXmsK     = 0x%02X (%d)\n", w386ext30.PfMinXmsK,    w386ext30.PfMinXmsK );
            printf( "    PfW386Flags   = 0x%08X\n", w386ext30.PfW386Flags );
            if (w386ext30.PfW386Flags & fEnableClose)
                printf( "        fEnableClose    is set.\n" );
            if (w386ext30.PfW386Flags & fBackground)
                printf( "        fBackground     is set.\n" );
            if (w386ext30.PfW386Flags & fExclusive)
                printf( "        fExclusive      is set.\n" );
            if (w386ext30.PfW386Flags & fFullScreen)
                printf( "        fFullScreen     is set.\n" );
            if (w386ext30.PfW386Flags & fALTTABdis)
                printf( "        fALTTABdis      is set.\n" );
            if (w386ext30.PfW386Flags & fALTESCdis)
                printf( "        fALTESCdis      is set.\n" );
            if (w386ext30.PfW386Flags & fALTSPACEdis)
                printf( "        fALTSPACEdis    is set.\n" );
            if (w386ext30.PfW386Flags & fALTENTERdis)
                printf( "        fALTENTERdis    is set.\n" );
            if (w386ext30.PfW386Flags & fALTPRTSCdis)
                printf( "        fALTPRTSCdis    is set.\n" );
            if (w386ext30.PfW386Flags & fPRTSCdis)
                printf( "        fPRTSCdis       is set.\n" );
            if (w386ext30.PfW386Flags & fCTRLESCdis)
                printf( "        fCTRLESCdis     is set.\n" );
            if (w386ext30.PfW386Flags & fPollingDetect)
                printf( "        fPollingDetect  is set.\n" );
            if (w386ext30.PfW386Flags & fNoHMA)
                printf( "        fNoHMA          is set.\n" );
            if (w386ext30.PfW386Flags & fHasHotKey)
                printf( "        fHasHotKey      is set.\n" );
            if (w386ext30.PfW386Flags & fEMSLocked)
                printf( "        fEMSLocked      is set.\n" );
            if (w386ext30.PfW386Flags & fXMSLocked)
                printf( "        fXMSLocked      is set.\n" );
            if (w386ext30.PfW386Flags & fINT16Paste)
                printf( "        fINT16Paste     is set.\n" );
            if (w386ext30.PfW386Flags & fVMLocked)
                printf( "        fVMLocked       is set.\n" );
            if (w386ext30.PfW386Flags & fGlobalProtect)
                printf( "        fGlobalProtect  is set.\n" );
            if (w386ext30.PfW386Flags & fMinimized)
                printf( "        fMinimized      is set.\n" );
            if (w386ext30.PfW386Flags & fMaximized)
                printf( "        fMaximized      is set.\n" );
            if (w386ext30.PfW386Flags & fRealMode)
                printf( "        fRealMode       is set.\n" );
            if (w386ext30.PfW386Flags & fWinLie)
                printf( "        fWinLie         is set.\n" );
            if (w386ext30.PfW386Flags & fStrayPtrDetect)
                printf( "        fStrayPtrDetect is set.\n" );
            if (w386ext30.PfW386Flags & fNoSuggestMSDOS)
                printf( "        fNoSuggestMSDOS is set.\n" );
            if (w386ext30.PfW386Flags & fLocalUMBs)
                printf( "        fLocalUMBs      is set.\n" );
            if (w386ext30.PfW386Flags & fRealModeSilent)
                printf( "        fRealModeSilent is set.\n" );
            if (w386ext30.PfW386Flags & fQuickStart)
                printf( "        fQuickStart     is set.\n" );
            if (w386ext30.PfW386Flags & fAmbiguousPIF)
                printf( "        fAmbiguousPIF   is set.\n" );
            printf( "    PfW386Flags2  = 0x%08X\n", w386ext30.PfW386Flags2 );
            if (w386ext30.PfW386Flags2 & fVidTxtEmulate)
                printf( "        fVidTxtEmulate  is set.\n" );
            if (w386ext30.PfW386Flags2 & fVidNoTrpTxt)
                printf( "        fVidNoTrpTxt    is set.\n" );
            if (w386ext30.PfW386Flags2 & fVidNoTrpLRGrfx)
                printf( "        fVidNoTrpLRGrfx is set.\n" );
            if (w386ext30.PfW386Flags2 & fVidNoTrpHRGrfx)
                printf( "        fVidNoTrpHRGrfx is set.\n" );
            if (w386ext30.PfW386Flags2 & fVidTextMd)
                printf( "        fVidTextMd      is set.\n" );
            if (w386ext30.PfW386Flags2 & fVidLowRsGrfxMd)
                printf( "        fVidLowRsGrfxd  is set.\n" );
            if (w386ext30.PfW386Flags2 & fVidHghRsGrfxMd)
                printf( "        fVidHghRsGrfxd  is set.\n" );
            if (w386ext30.PfW386Flags2 & fVidRetainAllo)
                printf( "        fVidRetainAllo  is set.\n" );
            printf( "    PfHotKeyScan  = 0x%04X\n", w386ext30.PfHotKeyScan );
            printf( "    PfHotKeyShVal = 0x%04X\n", w386ext30.PfHotKeyShVal );
            printf( "    PfHotKeyShMsk = 0x%04X\n", w386ext30.PfHotKeyShMsk );
            printf( "    PfHotKeyVal   = 0x%02X\n", w386ext30.PfHotKeyVal );
            printf( "    PfW386params  = %s\n",     STRING(w386ext30.PfW386params) );
            printf( "\n" );

        }

        if (strcmp(pifexthdr.extsig, WNTHDRSIG31)==0) {
            WNTPIF31    wntpif31;

            _lread( hFile, &wntpif31, sizeof( wntpif31 ) );
            printf( "    ==> Internal Revision %d <==\n", wntpif31.wInternalRevision );
            printf( "    dwWNTFlags      = 0x%08X\n", wntpif31.nt31Prop.dwWNTFlags );
            if (wntpif31.nt31Prop.dwWNTFlags & COMPAT_TIMERTIC)
                printf( "        COMPAT_TIMERTIC is set.\n" );
            printf( "    dwRes1          = 0x%08X\n", wntpif31.nt31Prop.dwRes1 );
            printf( "    dwRes2          = 0x%08X\n", wntpif31.nt31Prop.dwRes2 );
            printf( "    achConfigFile   = %s\n", STRING(wntpif31.nt31Prop.achConfigFile) );
            printf( "    achAuotexecFile = %s\n", STRING(wntpif31.nt31Prop.achAutoexecFile) );
        }

        if (strcmp(pifexthdr.extsig, WENHHDRSIG40)==0) {
            WENHPIF40    wenhpif40;

            _lread( hFile, &wenhpif40, sizeof( wenhpif40 ) );
            printf( "    ==> Internal Revision %d <==\n", wenhpif40.wInternalRevision );
            printf( "    dwEnhModeFlagsProp  = 0x%08X\n", wenhpif40.dwEnhModeFlagsProp );
            printf( "    dwRealModeFlagsProp = 0x%08X\n", wenhpif40.dwRealModeFlagsProp );
            printf( "    achOtherFileProp    = %s\n",     STRING(wenhpif40.achOtherFileProp) );
            printf( "    achIconFileProp     = %s\n",     STRING(wenhpif40.achIconFileProp) );
            printf( "    wIconInxexProp      = 0x%04X\n", wenhpif40.wIconIndexProp );

            printf( "    PROPTSK:\n" );
            printf( "        flTsk            = 0x%04X\n", wenhpif40.tskProp.flTsk );
            if (wenhpif40.tskProp.flTsk & TSK_ALLOWCLOSE)
                printf( "            TSK_ALLOWCLOSE      is set.\n" );
            if (wenhpif40.tskProp.flTsk & TSK_BACKGROUND)
                printf( "            TSK_BACKGROUND      is set.\n" );
            if (wenhpif40.tskProp.flTsk & TSK_EXCLUSIVE)
                printf( "            TSK_EXCLUSIVE       is set.\n" );
            if (wenhpif40.tskProp.flTsk & TSK_NOWARNTERMINATE)
                printf( "            TSK_NOWARNTERMINATE is set.\n" );
            if (wenhpif40.tskProp.flTsk & TSK_NOSCREENSAVER)
                printf( "            TSK_NOSCREENSAVER   is set.\n" );
            printf( "        flTskInit        = 0x%04X\n", wenhpif40.tskProp.flTskInit );
            printf( "        wReserved1       = 0x%04X\n", wenhpif40.tskProp.wReserved1 );
            printf( "        wReserved2       = 0x%04X\n", wenhpif40.tskProp.wReserved2 );
            printf( "        wReserved3       = 0x%04X\n", wenhpif40.tskProp.wReserved3 );
            printf( "        wReserved4       = 0x%04X\n", wenhpif40.tskProp.wReserved4 );
            printf( "        wIdleSensitivity = 0x%04X\n", wenhpif40.tskProp.wIdleSensitivity );

            printf( "    PROPVID:\n" );
            printf( "        flVid        = 0x%04X\n", wenhpif40.vidProp.flVid );
            if (wenhpif40.vidProp.flVid & VID_TEXTEMULATE)
                printf( "            VID_TEXTEMULATE  is set.\n" );
            if (wenhpif40.vidProp.flVid & VID_RETAINMEMORY)
                printf( "            VID_RETAINMEMORY is set.\n" );
            if (wenhpif40.vidProp.flVid & VID_FULLSCREEN)
                printf( "            VID_FULLSCREEN   is set.\n" );
            printf( "        flVidInit    = 0x%04X\n", wenhpif40.vidProp.flVidInit );
            printf( "        wReserved1   = 0x%04X\n", wenhpif40.vidProp.wReserved1 );
            printf( "        wReserved2   = 0x%04X\n", wenhpif40.vidProp.wReserved2 );
            printf( "        wReserved3   = 0x%04X\n", wenhpif40.vidProp.wReserved3 );
            printf( "        cScreenLines = 0x%04X\n", wenhpif40.vidProp.cScreenLines );

            printf( "    PROPKBD:\n" );
            printf( "        flKbd            = 0x%04X\n", wenhpif40.kbdProp.flKbd );
            if (wenhpif40.kbdProp.flKbd & KBD_FASTPASTE)
                printf( "            KBD_FASTPASTE  is set.\n" );
            if (wenhpif40.kbdProp.flKbd & KBD_NOALTTAB)
                printf( "            KBD_NOALTTAB   is set.\n" );
            if (wenhpif40.kbdProp.flKbd & KBD_NOALTESC)
                printf( "            KBD_NOALTESC   is set.\n" );
            if (wenhpif40.kbdProp.flKbd & KBD_NOALTSPACE)
                printf( "            KBD_NOALTSPACE is set.\n" );
            if (wenhpif40.kbdProp.flKbd & KBD_NOALTENTER)
                printf( "            KBD_NOALTENTER is set.\n" );
            if (wenhpif40.kbdProp.flKbd & KBD_NOALTPRTSC)
                printf( "            KBD_NOALTPRTSC is set.\n" );
            if (wenhpif40.kbdProp.flKbd & KBD_NOPRTSC)
                printf( "            KBD_NOPRTSC    is set.\n" );
            if (wenhpif40.kbdProp.flKbd & KBD_NOCTRLESC)
                printf( "            KBD_NOCTRLESC   is set.\n" );
            printf( "        flKbdInit        = 0x%04X\n", wenhpif40.kbdProp.flKbdInit );
            printf( "        msAltDelay       = 0x%04X\n", wenhpif40.kbdProp.msAltDelay );
            printf( "        msAltPasteDelay  = 0x%04X\n", wenhpif40.kbdProp.msAltPasteDelay );
            printf( "        msPasteDelay     = 0x%04X\n", wenhpif40.kbdProp.msPasteDelay );
            printf( "        msPasteFullDelay = 0x%04X\n", wenhpif40.kbdProp.msPasteFullDelay );
            printf( "        msPasteTimeout   = 0x%04X\n", wenhpif40.kbdProp.msPasteTimeout );
            printf( "        cPasteSkip       = 0x%04X\n", wenhpif40.kbdProp.cPasteSkip );
            printf( "        cPasteCRSkip     = 0x%04X\n", wenhpif40.kbdProp.cPasteCRSkip );

            printf( "    PROPMSE:\n" );
            printf( "        flMse     = 0x%04X\n", wenhpif40.mseProp.flMse );
            if (wenhpif40.mseProp.flMse & MSE_WINDOWENABLE)
                printf( "            MSE_WINDOWENABLE is set.\n" );
            if (wenhpif40.mseProp.flMse & MSE_EXCLUSIVE)
                printf( "            MSE_EXCLUSIVE    is set.\n" );
            printf( "        flMseInit = 0x%04X\n", wenhpif40.mseProp.flMseInit );

            printf( "    PROPSND:\n" );
            printf( "        flSnd     = 0x%04X\n", wenhpif40.sndProp.flSnd );
            if (wenhpif40.sndProp.flSnd & SND_SPEAKERENABLE)
                printf( "            SND_SPEAKERENABLE is set.\n" );
            printf( "        flSndInit = 0x%04X\n", wenhpif40.sndProp.flSndInit );

            printf( "    PROPFNT:\n" );
            printf( "        flFnt             = 0x%04X\n", wenhpif40.fntProp.flFnt );
            if (wenhpif40.fntProp.flFnt & FNT_RASTERFONTS)
                printf( "            FNT_RASTERFONTS is set.\n" );
            if (wenhpif40.fntProp.flFnt & FNT_TTFONTS)
                printf( "            FNT_TTFONTS     is set.\n" );
            if (wenhpif40.fntProp.flFnt & FNT_AUTOSIZE)
                printf( "            FNT_AUTOSIZE    is set.\n" );
            if (wenhpif40.fntProp.flFnt & FNT_RASTER)
                printf( "            FNT_RASTER      is set.\n" );
            if (wenhpif40.fntProp.flFnt & FNT_TT)
                printf( "            FNT_TT          is set.\n" );
            printf( "        flFntInit         = 0x%04X\n", wenhpif40.fntProp.flFntInit );
            printf( "        cxFont            = 0x%04X (%d)\n", wenhpif40.fntProp.cxFont, wenhpif40.fntProp.cxFont );
            printf( "        cyFont            = 0x%04X (%d)\n", wenhpif40.fntProp.cyFont, wenhpif40.fntProp.cyFont );
            printf( "        cxFontActual      = 0x%04X (%d)\n", wenhpif40.fntProp.cxFontActual, wenhpif40.fntProp.cxFontActual );
            printf( "        cyFontActual      = 0x%04X (%d)\n", wenhpif40.fntProp.cyFontActual, wenhpif40.fntProp.cyFontActual );
            printf( "        achRasterFaceName = %s\n",     STRING(wenhpif40.fntProp.achRasterFaceName) );
            printf( "        achTTFaceName     = %s\n",     STRING(wenhpif40.fntProp.achRasterFaceName) );
            printf( "        wCurrentCP        = 0x%04X\n", wenhpif40.fntProp.wCurrentCP );

            printf( "    PROPWIN:\n" );
            printf( "        flWin      = 0x%04X\n", wenhpif40.winProp.flWin );
            if (wenhpif40.winProp.flWin & WIN_SAVESETTINGS)
                printf( "            WIN_SAVESETTINGS is set.\n" );
            if (wenhpif40.winProp.flWin & WIN_TOOLBAR)
                printf( "            WIN_TOOLBAR      is set.\n" );
            printf( "        flWinInit  = 0x%04X\n",      wenhpif40.winProp.flWinInit );
            printf( "        cxCells    = 0x%04X (%d)\n", wenhpif40.winProp.cxCells,  wenhpif40.winProp.cxCells );
            printf( "        cyCells    = 0x%04X (%d)\n", wenhpif40.winProp.cyCells,  wenhpif40.winProp.cyCells );
            printf( "        cxClient   = 0x%04X (%d)\n", wenhpif40.winProp.cxClient, wenhpif40.winProp.cxClient );
            printf( "        cyClient   = 0x%04X (%d)\n", wenhpif40.winProp.cyClient, wenhpif40.winProp.cyClient );
            printf( "        cxWindow   = 0x%04X (%d)\n", wenhpif40.winProp.cxWindow, wenhpif40.winProp.cxWindow );
            printf( "        cyWindow   = 0x%04X (%d)\n", wenhpif40.winProp.cyWindow, wenhpif40.winProp.cyWindow );
            printf( "        wLength    = 0x%04X\n",      wenhpif40.winProp.wLength );
            printf( "        wShowFlags = 0x%04X\n",      wenhpif40.winProp.wShowFlags );
            printf( "        wShowCmd   = 0x%04X\n",      wenhpif40.winProp.wShowCmd );
            printf( "        xMinimize  = 0x%04X (%d)\n", wenhpif40.winProp.xMinimize, wenhpif40.winProp.xMinimize );
            printf( "        yMinimize  = 0x%04X (%d)\n", wenhpif40.winProp.yMinimize, wenhpif40.winProp.yMinimize );
            printf( "        xMaximize  = 0x%04X (%d)\n", wenhpif40.winProp.xMaximize, wenhpif40.winProp.xMaximize );
            printf( "        yMaximize  = 0x%04X (%d)\n", wenhpif40.winProp.yMaximize, wenhpif40.winProp.yMaximize );
            printf( "        rcNormal   = (0x%04X, 0x%04X, 0x%04X, 0x%04X)\n",
                    wenhpif40.winProp.rcNormal.left, wenhpif40.winProp.rcNormal.top,
                    wenhpif40.winProp.rcNormal.right,wenhpif40.winProp.rcNormal.bottom
                   );

            printf( "    PROPENV:\n" );
            printf( "        flEnv         = 0x%04X\n", wenhpif40.envProp.flEnv );
            printf( "        flEnvInit     = 0x%04X\n", wenhpif40.envProp.flEnvInit );
            printf( "        achBatchFile  = %s\n",     STRING(wenhpif40.envProp.achBatchFile) );
            printf( "        cbEnvironemnt = 0x%04X\n", wenhpif40.envProp.cbEnvironment );
            printf( "        wMaxDPMI      = 0x%04X\n", wenhpif40.envProp.wMaxDPMI );

        }

        if (strcmp(pifexthdr.extsig, WNTHDRSIG40)==0) {

            WNTPIF40   wntpif40;

            _lread( hFile, &wntpif40, sizeof( wntpif40 ) );
            printf( "    ==> Internal Revision %d <==\n", wntpif40.wInternalRevision );
            printf( "    flWnt            = 0x%08X\n", wntpif40.nt40Prop.flWnt );
            if (wntpif40.nt40Prop.flWnt & WNT_LET_SYS_POS)
                printf( "            WNT_LET_SYS_POS   is set.\n" );
            if (wntpif40.nt40Prop.flWnt & WNT_CONSOLE_PROPS)
                printf( "            WNT_CONSOLE_PROPS is set.\n" );

            WideCharToMultiByte( CP_ACP, 0, wntpif40.nt40Prop.awchCmdLine, -1, szTemp, 512, NULL, NULL );
            printf( "    awchCmdLine      = %s\n", STRING(szTemp) );
            printf( "    achSaveCmdLine   = %s\n", STRING(wntpif40.nt40Prop.achSaveCmdLine) );

            WideCharToMultiByte( CP_ACP, 0, wntpif40.nt40Prop.awchOtherFile, -1, szTemp, 512, NULL, NULL );
            printf( "    awchOtherFile    = %s\n", STRING(szTemp) );
            printf( "    achSaveOtherFile = %s\n", STRING(wntpif40.nt40Prop.achSaveOtherFile) );

            WideCharToMultiByte( CP_ACP, 0, wntpif40.nt40Prop.awchPIFFile, -1, szTemp, 512, NULL, NULL );
            printf( "    awchPIFFile      = %s\n", STRING(szTemp) );
            printf( "    achSavePIFFile   = %s\n", STRING(wntpif40.nt40Prop.achSavePIFFile) );

            WideCharToMultiByte( CP_ACP, 0, wntpif40.nt40Prop.awchTitle, -1, szTemp, 512, NULL, NULL );
            printf( "    awchTitle        = %s\n", STRING(szTemp) );
            printf( "    achSaveTitle     = %s\n", STRING(wntpif40.nt40Prop.achSaveTitle) );

            WideCharToMultiByte( CP_ACP, 0, wntpif40.nt40Prop.awchIconFile, -1, szTemp, 512, NULL, NULL );
            printf( "    awchIconFIle     = %s\n", STRING(szTemp) );
            printf( "    achSaveIconFile  = %s\n", STRING(wntpif40.nt40Prop.achSaveIconFile) );

            WideCharToMultiByte( CP_ACP, 0, wntpif40.nt40Prop.awchWorkDir, -1, szTemp, 512, NULL, NULL );
            printf( "    awchWorkDir      = %s\n", STRING(szTemp) );
            printf( "    achSaveWorkDir   = %s\n", STRING(wntpif40.nt40Prop.achSaveWorkDir) );

            WideCharToMultiByte( CP_ACP, 0, wntpif40.nt40Prop.awchBatchFile, -1, szTemp, 512, NULL, NULL );
            printf( "    awchBatchFile    = %s\n", STRING(szTemp) );
            printf( "    achSaveBatchFile = %s\n", STRING(wntpif40.nt40Prop.achSaveBatchFile) );

            printf( "    dwForeColor      = 0x%08X\n", wntpif40.nt40Prop.dwForeColor );
            printf( "    dwBackColor      = 0x%08X\n", wntpif40.nt40Prop.dwBackColor );
            printf( "    dwPopupForeColor = 0x%08X\n", wntpif40.nt40Prop.dwPopupForeColor );
            printf( "    dwPopupBackColor = 0x%08X\n", wntpif40.nt40Prop.dwPopupBackColor );
            printf( "    WinSize          = (0x%04X, 0x%04X)\n", wntpif40.nt40Prop.WinSize.X,  wntpif40.nt40Prop.WinSize.Y );
            printf( "    BuffSize         = (0x%04X, 0x%04X)\n", wntpif40.nt40Prop.BuffSize.X, wntpif40.nt40Prop.BuffSize.Y );
            printf( "    WinPos           = (0x%08X, 0x%08X)\n", wntpif40.nt40Prop.WinPos.x,   wntpif40.nt40Prop.WinPos.y );
            printf( "    dwCursorSize     = 0x%08X\n", wntpif40.nt40Prop.dwCursorSize );
            printf( "    dwCmdHistBufSize = 0x%08X\n", wntpif40.nt40Prop.dwCmdHistBufSize );
            printf( "    dwNumCmdHist     = 0x%08X\n", wntpif40.nt40Prop.dwNumCmdHist );

        }

        _llseek( hFile, pifexthdr.extnxthdrfloff, FILE_BEGIN );

    }

OutOfHere:
    printf( "\n" );

    return(0);
}

