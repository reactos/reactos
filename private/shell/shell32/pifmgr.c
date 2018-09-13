/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1991
 *  All Rights Reserved.
 *
 *
 *  PIFMGR.C
 *  Main module for PIFMGR.DLL
 *
 *  History:
 *  Created 31-Jul-1992 3:30pm by Jeff Parsons
 *
 *  Exported Program Information File (PIF) Manager services:
 *
 *      PifMgr_OpenProperties()
 *          Give it the name of an DOS application (com, exe, or bat),
 *          and it will open the PIF associated with that application
 *          and return a "handle" to the app's "properties".  Use this
 *          handle when calling any of the other "properties" services (ie,
 *          Get, Set, and Close).
 *
 *          If no PIF exists, it will still allocate a PIF data block
 *          in memory and initialize it, either with data from _DEFAULT.PIF
 *          or its internal defaults.  It will also construct the PIF name
 *          it was looking for but couldn')t find and save that in its internal
 *          PIF data structure, so that if PifMgr_SetProperties is ever called, the
 *          data can be saved to disk.
 *
 *      PifMgr_GetProperties()
 *          Returns the specified block of data from the associated PIF.
 *          If it is a "named" block, it must be the name of a linked
 *          extension inside the PIF, which can be any predefined name
 *          (eg, "WINDOWS 386 3.0") or the name of your own block.  You can
 *          create your own named data blocks using the PifMgr_SetProperties()
 *          service.  "Named" data can also be thought of as "raw" data,
 *          because it is returned to the caller as-is -- without translation.
 *
 *          The size of a named block can be determined by calling
 *          PifMgr_GetProperties with a size of zero; no data is copied, but the size
 *          of the requested block is returned (0 if not found).
 *
 *          All named blocks can be enumerated by passing NULL for the name,
 *          a pointer to a 16-byte buffer for the requested block name, and a
 *          0-based block index in the size parameter.  The size returned
 *          is the size of the block (0 if none).
 *
 *          If an unnamed property block is requested (ie, the selector of
 *          the name parameter is NULL, and the offset is a property group
 *          ordinal), then the associated structure is returned.  For example,
 *          PifMgr_GetProperties(GROUP_TSK) returns a predefined structure (see
 *          PROPTSK in PIF.H) containing all the tasking-related information,
 *          in a format that is PIF-independent.  This is a valuable service,
 *          because it relieves callers from having to cope with PIFs
 *          containing a wide variety of sections (known as PIF extensions),
 *          only one of which is required.  Think of this as "cooked" data.
 *
 *          A third variation is raw read/write of the entire PIF data block,
 *          if lpszGroup is NULL.  This must be used with extreme caution, and
 *          will only be allowed if the properties were opened with the
 *          OPENPROPS_RAWIO flag specified.
 *
 *      PifMgr_SetProperties()
 *          This is pretty much the opposite of PifMgr_GetProperties, except that it
 *          also takes a flags parameter that can specify that the changes
 *          be made immediately, or deferred to PifMgr_CloseProperties.
 *
 *      PifMgr_CloseProperties()
 *          Flushes any dirty PIF data in memory, and frees the local heap
 *          storage.
 *
 */

#include "shellprv.h"
#pragma hdrstop

/* Global R/W DLL data
 */

PPROPLINK g_pplHead;              // pointer to first prop entry
HANDLE    g_offHighestPropLink;     // highest offset of a prop thus far recorded

TCHAR g_szNone[16];                // initialized by LibMainP,
TCHAR g_szAuto[16];                // and 16 chars to allow for localization

char g_szMSDOSSTSFile[] = "C:\\MSDOSSYS.STS";

TCHAR g_szConfigFile[] = TEXT("C:") CONFIGFILE;
TCHAR g_szAutoexecFile[] = TEXT("C:") AUTOEXECFILE;

TCHAR g_szMConfigFile[] = TEXT("C:") MCONFIGFILE;
TCHAR g_szMAutoexecFile[] = TEXT("C:") MAUTOEXECFILE;

TCHAR g_szWConfigFile[] = TEXT("C:") WCONFIGFILE;
TCHAR g_szWAutoexecFile[] = TEXT("C:") WAUTOEXECFILE;

#ifdef DBCS

char ImeBatchFile[] = "DOSIME\0";
#endif

#ifdef WINNT
#define NT_CONFIG_FILE "%SystemRoot%\\SYSTEM32\\CONFIG.NT"
#define NT_AUTOEXEC_FILE "%SystemRoot%\\SYSTEM32\\AUTOEXEC.NT"
#endif

#define LPPH_OFF(off) ((LPBYTE)lpph + off)
#define LPPIF_FIELDOFF(off) ((LPBYTE)ppl->lpPIFData + FIELD_OFFSET(PIFDATA,off))
#define LPPIF_OFF(off) ((LPBYTE)ppl->lpPIFData + off)

//
//  g_szDefaultPIF can be in one of three states:
//
//  1.  "_DEFAULT.PIF", which means that we have never needed to search
//          for a _default.pif yet.  The next time we need to locate
//          _default.pif, we must perform a full search.  On success,
//          move to state 2.  On failure, move to state 3.
//
//  2.  A fully-qualified path to _default.pif, which means that we have
//          searched for a _default.pif and found it in the specified
//          location.  The next time we need to locate _default.pif, we
//          will look here.  If found, remain in state 2, else move to
//          state 3.
//
//  3.  The null string, which means that we searched for a _default.pif
//          and didn't find one.  The next time we need to locate
//          _default.pif, we just fail without even looking on the disk.
//          (This is the common case for a clean install.)
//
//      Note that all the cases are "sticky"; once you reach a state, you
//      can never move back to a previous state.  This sacrifices flexibility
//      for performance.
//
//      The macro fTryDefaultPif() returns nonzero if we are in cases
//      1 or 2.
//
//      The macro fDefaultPifFound() returns nonzero if we are in case 2.
//
//  WARNING!  WARNING!  WARNING!  WARNING!
//
//      Evil hack relies on the fact that the three states can be
//      distinguished by the first character of g_szDefaultPIF, which
//      in turn relies on the fact that `_' cannot be the first character
//      of a fully-qualified path.  (It is not a valid drive letter,
//      and it cannot start a UNC.)
//
//

#define INIT_INIDATA                0x01
#define INIT_PIFDIR                 0x02

CHAR     fbInit = 0;                    // see INIT_* flags
INT      iPIFName = (12*SIZEOF(TCHAR)); // strlen(g_szPIFDir)
INT      iWinName = (12*SIZEOF(TCHAR)); // strlen(g_szPIFDir)
TCHAR    g_szPIFDir[MAXPATHNAME]     = TEXT("\\WINDOWS\\PIF");
TCHAR    g_szDefaultPIF[MAXPATHNAME] = TEXT("_DEFAULT.PIF");

#define fTryDefaultPif()            g_szDefaultPIF[0]
#define fDefaultPifFound()          (g_szDefaultPIF[0] != TEXT('_'))

//
// szComspec starts out life as "COMSPEC=" but eventually becomes the
// name of the COMSPEC program, usually "COMMAND.COM".
//
TCHAR   szComspec[8+1+3+1]        = TEXT("COMSPEC=");
#define cchComspec  (8*SIZEOF(TCHAR))     /* strlen("COMSPEC=") */

/* Global R/O DLL data
 */

extern const TCHAR c_szNULL[];              // A string so nice...

const TCHAR szZero[]            = TEXT("0");

const int acbData[] = {
                SIZEOF(PROPPRG),
                SIZEOF(PROPTSK),
                SIZEOF(PROPVID),
                SIZEOF(PROPMEM),
                SIZEOF(PROPKBD),
                SIZEOF(PROPMSE),
                SIZEOF(PROPSND),
                SIZEOF(PROPFNT),
                SIZEOF(PROPWIN),
                SIZEOF(PROPENV),
#ifdef WINNT
                SIZEOF(PROPNT31),
#endif
#ifdef UNICODE
                SIZEOF(PROPNT40),
#endif
};

/*
 * The casts are used because we intentionally mis-prototyped the GetXxxData
 * and SetXxxData functions to receive their third argument as a LPXXX instead
 * of a LPVOID.
 */

const DATAGETFN afnGetData[] = {
                (DATAGETFN)GetPrgData,
                (DATAGETFN)GetTskData,
                (DATAGETFN)GetVidData,
                (DATAGETFN)GetMemData,
                (DATAGETFN)GetKbdData,
                (DATAGETFN)GetMseData,
                (DATAGETFN)GetSndData,
                (DATAGETFN)GetFntData,
                (DATAGETFN)GetWinData,
                (DATAGETFN)GetEnvData,
#ifdef WINNT
                (DATAGETFN)GetNt31Data,
#endif
#ifdef UNICODE
                (DATAGETFN)GetNt40Data,
#endif
};

const DATASETFN afnSetData[] = {
                (DATASETFN)SetPrgData,
                (DATASETFN)SetTskData,
                (DATASETFN)SetVidData,
                (DATASETFN)SetMemData,
                (DATASETFN)SetKbdData,
                (DATASETFN)SetMseData,
                (DATASETFN)SetSndData,
                (DATASETFN)SetFntData,
                (DATASETFN)SetWinData,
                (DATASETFN)SetEnvData,
#ifdef WINNT
                (DATASETFN)SetNt31Data,
#endif
#ifdef UNICODE
                (DATASETFN)SetNt40Data,
#endif
};


// WIN.INI things of interest
// Note: some of these NEED to be ANSI strings, and other TCHAR
// strings.  Please do not arbitrarily change the type casts of
// these strings!!!! (RickTu)


const TCHAR szMemory[]              = TEXT("MEMORY");
const TCHAR szComp[]                = TEXT("COMPATIBLE");

#ifndef WINNT
const TCHAR szWinCom[]              = TEXT("\\WIN.COM /W");
const TCHAR szWinCommand[]          = TEXT("\r\n");
const TCHAR szExitWinCommand[]      = TEXT("X\r\n");
const TCHAR szQuickWinCommand[]     = TEXT("Q\r\n");
const TCHAR szExitQuickWinCommand[] = TEXT("XQ\r\n");
#endif
CHAR szSingle[]                     = "DOS=SINGLE\r\n";
CHAR szCRLF[]                       = "\r\n";
CHAR szEcho[]                       = "ECHO ";
CHAR szPause[]                      = "\r\nPAUSE\r\n";
CHAR szCall[]                       = "CALL ";
CHAR szCD[]                         = "CD ";
CHAR szWin[]                        = "WIN";

// SYSTEM.INI things of interest

const TCHAR szSystemINI[]           = TEXT("SYSTEM.INI");
const TCHAR sz386EnhSection[]       = TEXT("386Enh");
const TCHAR szWOAFontKey[]          = TEXT("WOAFont");
const TCHAR szWOADBCSFontKey[]      = TEXT("WOADBCSFont");
const TCHAR szNonWinSection[]       = TEXT("NonWindowsApp");
const TCHAR szTTInitialSizes[]      = TEXT("TTInitialSizes");
#ifdef  CUSTOMIZABLE_HEURISTICS
const TCHAR szTTHeuristics[]        = TEXT("TTHeuristics");
const TCHAR szTTNonAspectMin[]      = TEXT("TTNonAspectMin");
#endif
TCHAR szTTCacheSection[2][32] = {TEXT("TTFontDimenCache"), TEXT("TTFontDimenCacheDBCS")};

//
// These are because they are accessed only when we need to create
// a new PIF file or convert a 3.1 PIF file into a 4.0 PIF file.
//
const TCHAR szDOSAPPINI[]           = TEXT("DOSAPP.INI");
const TCHAR szDOSAPPSection[]       = TEXT("DOS Applications");
const TCHAR szDOSAPPDefault[]       = TEXT("Default");

const TCHAR szDisplay[]             = TEXT("DISPLAY");
const TCHAR szDefIconFile[]         = ICONFILE_DEFAULT;

const TCHAR szDotExe[]              = TEXT(".EXE");
const TCHAR szDotCom[]              = TEXT(".COM");
const TCHAR szDotBat[]              = TEXT(".BAT");
const TCHAR szDotPif[]              = TEXT(".PIF");
const TCHAR szDotCmd[]              = TEXT(".CMD");
const TCHAR * apszAppType[] =  {
    szDotExe, szDotCom, szDotBat, szDotCmd, szDotPif
};

CHAR szSTDHDRSIG[]                  = STDHDRSIG;
CHAR szW286HDRSIG30[]               = W286HDRSIG30;
CHAR szW386HDRSIG30[]               = W386HDRSIG30;
CHAR szWENHHDRSIG40[]               = WENHHDRSIG40;
#ifdef WINNT
CHAR szWNTHDRSIG31[]                = WNTHDRSIG31;
#endif
#ifdef UNICODE
CHAR szWNTHDRSIG40[]                = WNTHDRSIG40;
#endif

CHAR szCONFIGHDRSIG40[]             = CONFIGHDRSIG40;
CHAR szAUTOEXECHDRSIG40[]           = AUTOEXECHDRSIG40;
const TCHAR szRunOnceKey[]          = REGSTR_PATH_RUNONCE;

const TCHAR szPIFConvert[]          = TEXT("PIFConvert");
const TCHAR szPIFConvertExe[]       = TEXT("RUNDLL.EXE PIFMGR.DLL,ProcessStartupProperties");
const TCHAR szPIFConvertKey[]       = REGSTR_PATH_PIFCONVERT;
const TCHAR szMSDOSMode[]           = REGSTR_VAL_MSDOSMODE;
const TCHAR szMSDOSModeDiscard[]    = REGSTR_VAL_MSDOSMODEDISCARD;


// wsprintf formatting strings
const TCHAR szDotPercent03d[]       = TEXT(".%03d");

// miscellaneous hack-o-ramas
const TCHAR szPP4[]                 = TEXT("PP4");      // MS Powerpoint 4.0

PROPTSK tskDefault          ={TSK_DEFAULT,
                              TSKINIT_DEFAULT,
                              TSKFGNDBOOST_DEFAULT,
                              TSKBGNDBOOST_DEFAULT,
                              0,
                              0,
                              TSKIDLESENS_DEFAULT,
};

PROPVID vidDefault          ={VID_DEFAULT,
                              VIDINIT_DEFAULT,
                              0,
                              0,
                              0,
};

PROPMEM memDefault          ={MEM_DEFAULT,
                              MEMINIT_DEFAULT,
                              MEMLOW_DEFAULT,   // BUGBUG: ignore stdpifdata.minmem?
                              MEMLOW_MAX,       // BUGBUG: ignore stdpifdata.maxmem?
                              MEMEMS_DEFAULT,
                              MEMEMS_MAX,
                              MEMXMS_DEFAULT,
                              MEMXMS_MAX,
};

PROPKBD kbdDefault          ={KBD_DEFAULT,
                              KBDINIT_DEFAULT,
                              KBDALTDELAY_DEFAULT,
                              KBDALTPASTEDELAY_DEFAULT,
                              KBDPASTEDELAY_DEFAULT,
                              KBDPASTEFULLDELAY_DEFAULT,
                              KBDPASTETIMEOUT_DEFAULT,
                              KBDPASTESKIP_DEFAULT,
                              KBDPASTECRSKIP_DEFAULT,
};

PROPMSE mseDefault          ={MSE_DEFAULT,
                              MSEINIT_DEFAULT,
};

PROPENV envDefault          ={ENV_DEFAULT,
                              ENVINIT_DEFAULT,
                              "",
                              ENVSIZE_DEFAULT,
                              ENVDPMI_DEFAULT,
};

WORD    flWinDefault        = WIN_DEFAULT;

/*
 * Default face name to use for Raster fonts.  Currently, this is
 * just a hard-coded value (ie, not maintained in any INI file).
 */
CHAR    szRasterFaceName[LF_FACESIZE] = "Terminal";


/*
 * Default face name to use for TrueType fonts.  It must be a monospace
 * font, and it must be a font that everyone is guaranteed to have.  Currently,
 * this can be changed by setting TTFont in [NonWindowsApp] in SYSTEM.INI.
 */
#ifdef WINNT
// now this is initialized with string resource. The 2nd element will get
// the native typeface for the bilingual dos prompt
CHAR    szTTFaceName[2][LF_FACESIZE] = {"Lucida Console", "Courier New"};
#endif // WINNT

const TCHAR szAltKeyDelay        [] = TEXT("AltKeyDelay");
const TCHAR szAltPasteDelay      [] = TEXT("AltPasteDelay");
const TCHAR szKeyPasteDelay      [] = TEXT("KeyPasteDelay");
const TCHAR szKeyBufferDelay     [] = TEXT("KeyBufferDelay");
const TCHAR szKeyPasteTimeout    [] = TEXT("KeyPasteTimeout");
const TCHAR szKeyPasteSkipCount  [] = TEXT("KeyPasteSkipCount");
const TCHAR szKeyPasteCRSkipCount[] = TEXT("KeyPasteCRSkipCount");
const TCHAR szMouseInDosBox      [] = TEXT("MouseInDosBox");
const TCHAR szDisablePositionSave[] = TEXT("DisablePositionSave");
const TCHAR szDOSPromptExitInst  [] = TEXT("DOSPromptExitInstruc");
const TCHAR szCommandEnvSize     [] = TEXT("CommandEnvSize");
const TCHAR szScreenLines        [] = TEXT("ScreenLines");

const INIDATA aINIData[] = {
    {sz386EnhSection,   szAltKeyDelay,          &kbdDefault.msAltDelay,      INIDATA_FIXEDPOINT},
    {sz386EnhSection,   szAltPasteDelay,        &kbdDefault.msAltPasteDelay, INIDATA_FIXEDPOINT},
    {sz386EnhSection,   szKeyPasteDelay,        &kbdDefault.msPasteDelay,    INIDATA_FIXEDPOINT},
    {sz386EnhSection,   szKeyBufferDelay,       &kbdDefault.msPasteFullDelay,INIDATA_FIXEDPOINT},
    {sz386EnhSection,   szKeyPasteTimeout,      &kbdDefault.msPasteTimeout,  INIDATA_FIXEDPOINT},
    {sz386EnhSection,   szKeyPasteSkipCount,    &kbdDefault.cPasteSkip,      INIDATA_DECINT},
    {sz386EnhSection,   szKeyPasteCRSkipCount,  &kbdDefault.cPasteCRSkip,    INIDATA_DECINT},
    {szNonWinSection,   szMouseInDosBox,        &mseDefault.flMse,           INIDATA_BOOLEAN,  MSE_WINDOWENABLE},
    {szNonWinSection,   szDisablePositionSave,  &flWinDefault,               INIDATA_BOOLEAN | INIDATA_INVERT,  WIN_SAVESETTINGS},
#ifdef ENVINIT_INSTRUCTIONS
    {sz386EnhSection,   szDOSPromptExitInst,    &envDefault.flEnvInit,       INIDATA_BOOLEAN,  ENVINIT_INSTRUCTIONS},
#endif
    {szNonWinSection,   szCommandEnvSize,       &envDefault.cbEnvironment,   INIDATA_DECINT},
    {szNonWinSection,   szScreenLines,          &vidDefault.cScreenLines,    INIDATA_DECINT},
};

/**************************************************************************
 *
 *  OVERVIEW OF INI FILE USAGE
 *
 *
 *  SYSTEM.INI
 *
 *  [386Enh]
 *
 *  WOAFont=<fon filename>
 *
 *  Status:     Public
 *  Default:    dosapp.fon
 *  Purpose:
 *
 *      This setting allows the user to specify which Terminal font
 *      file should be loaded when DOS box is started.
 *
 *  To change:
 *
 *      Use Notepad to edit the SYSTEM.INI file.
 *
 *
 *  [NonWindowsApp]
 *
 *  DisablePositionSave=<Boolean>
 *
 *  Status:     Public
 *  Default:    0 (FALSE)
 *  Purpose:
 *
 *      When FALSE, the position and font used in a non-Windows
 *      application is saved in the application's PIF file when
 *      you exit the application.  When TRUE, the position, fonts, and
 *      toolbar state of a non-Windows application whose settings
 *      have not been previously saved in the DOSAPP.INI file will
 *      not be saved.
 *
 *      If enabled, the setting can be overridden for each
 *      non-Windows application by selecting the Save Settings On
 *      Exit check box in the Font dialog box.
 *
 *  To change:
 *
 *      Use Notepad to edit the SYSTEM.INI file.
 *
 *  Compatibility notes:
 *
 *      In Windows 3.x, the "position save" (and font) information was
 *      saved in DOSAPP.INI, and although we will still read DOSAPP.INI
 *      in the absence of any information in the PIF file, we only *write*
 *      settings back to the PIF file.  DOSAPP.INI should be considered
 *      obsolete.
 *
 *
 *  TTFont=<fontname>
 *
 *  Status:     ?
 *  Default:    Courier New     // BUGBUG -- this should be a TT OEM font
 *  Purpose:
 *
 *      This setting allows the user to specify which TrueType font
 *      will be used in a DOS box.  It must be an OEM font.
 *
 *  To change:
 *
 *      Use Notepad to edit the SYSTEM.INI file.
 *
 *
 *  TTInitialSizes=<i1 i2 i3 i4 ... i16>
 *
 *  Status:     ?
 *  Default:    4 5 6 7 8 9 10 11 12 14 16 18 20 22 36 72
 *  Purpose:
 *
 *      This setting allows the user to specify which font sizes
 *      WinOldAp initially builds for the TrueType fonts in a DOS
 *      application window.
 *
 *      At most 16 font sizes can be requested.
 *
 *      Note that this INI entry is consulted only the first time
 *      Windows is restarted after changing video drivers or fonts.
 *
 *  To change:
 *
 *      Use Notepad to edit the SYSTEM.INI file.
 *
 *
 *  TTHeuristics=<i1 i2 i3 i4 i5 i6 i7 i8 i9>
 *
 *  Status:     Public
 *  Default:    5000 1000 0 1000 5000 1000 0 1000 1
 *  Purpose:
 *
 *      These integers control the way Windows chooses the font to
 *      display for DOS applications running inside a window if you
 *      have chosen "Auto" as the font size.
 *
 *      The parameters are named as follows:
 *
 *          i1=XOvershootInitial
 *          i2=XOvershootScale
 *          i3=XShortfallInitial
 *          i4=XShortfallScale
 *          i5=YOvershootInitial
 *          i6=YOvershootScale
 *          i7=YShortfallInitial
 *          i8=YShortfallScale
 *          i9=TrueTypePenalty
 *
 *      Each penalty value may not exceed 5000.
 *
 *      When Windows needs to select a font for use in a DOS
 *      application's window, it goes through the list of font
 *      sizes available and computes the "penalty" associated
 *      with using that font.  Windows then selects the font with
 *      the smallest penalty.
 *
 *      The horizontal penalty is computed as follows:
 *
 *          Let dxActual = <actual window width>
 *          Let dxDesired = <font width> * <characters per line>
 *
 *          If dxActual = dxDesired:
 *              xPenalty = 0
 *          If dxActual < dxDesired:
 *              Let Ratio = 1 - dxDesired / dxActual
 *              xPenalty = XOvershootInitial + Ratio * XOvershootScale
 *          If dxActual > dxDesired:
 *              Let Ratio = 1 - dxActual / dxDesired
 *              xPenalty = XShortfallInitial + Ratio * XShortfallScale
 *
 *      The vertical penalty is computed similarly.
 *
 *      Note that the Ratio is always a fraction between 0 and 1.
 *
 *      The penalty associated with a font is the sum of the vertical
 *      and horizontal penalties, plus the TrueTypePenalty if the font
 *      is a TrueType font.
 *
 *      The default value of 1 for the TrueTypePenalty means that,
 *      all other things being equal, Windows will select a raster
 *      font in preference to a TrueType font.  You can set this
 *      value to -1 if you wish the opposite preference.
 *
 *  To change:
 *
 *      Use Notepad to edit the SYSTEM.INI file.
 *
 *  Internals:
 *
 *      Even though floating point appears in the computations,
 *      everything is really done in integer arithmetic.
 *
 *      Pixels are NEVER MENTIONED anywhere in the penalty computations.
 *      (All pixel values are divided by other pixel values, so that
 *      we get a dimensionless number as a result.)
 *      This keeps us independent of the display resolution as well
 *      as the display aspect ratio.
 *
 *      Since the stretch and shrink are taken as fractions of the
 *      larger dimension, this keeps us from penalizing large
 *      differences by too much.  This is important because there
 *      isn't much visible difference between being ten times too
 *      big and being eleven times too big, but there is a big
 *      difference between being just right and being twice as big.
 *
 *      We must be careful not to let the maximum possible penalty
 *      exceed 32767.  This is done by making sure that each
 *      dimension cannot produce a penalty of greater than 10000
 *      (5000+5000), and that the TrueTypePenalty is at most 5000.
 *      This makes the maximum possible penalty 25000.
 *      This range checking is done by FontSelInit.
 *
 *
 *  TTNonAspectMin=<x y>
 *
 *  Status:     Public
 *  Default:    3 3
 *  Purpose:
 *
 *      These integers control the minimum width and height font that
 *      Windows will attempt to create automatically in response to a
 *      resize operation when TrueType fonts in DOS boxes are enabled
 *      and the "Auto" font size is selected.
 *
 *      These values prevent Windows from creating visually useless
 *      fonts like 10 x 1 or 1 x 10.  The default values prevent Windows
 *      from trying to create X x Y fonts if X < 3 or Y < 3.
 *
 *      TTNonAspectMin is not consulted if the font is being created at
 *      its default aspect ratio.  In other words, Windows will create,
 *      for example, a 1 x 3 font, if 1 x 3 is the standard aspect ratio
 *      for a 3-pixel-high font.
 *
 *      To permit all aspect ratios, set the values to "0 0".
 *
 *      To forbid all aspect ratios except for the standard aspect ratio,
 *      set the values to "-1 -1".
 *
 *  [TTFontDimenCache]
 *
 *  dxWidthRequested dyHeightRequested=dxWidthActual dyWidthActual
 *
 *  Status:     Private
 *  Default:    Null
 *  Purpose:
 *
 *      The [FontDimenCache] section contains information about
 *      TrueType font sizes that have been created.  Each entry
 *      has as the keyname the width and height that were passed
 *      to CreateFont and has as the value the width and height of
 *      the font that was actually created.
 *
 *  Internals:
 *
 *      Inspected by AddTrueTypeFontsToFontList.
 *      Set by AddOneNewTrueTypeFontToFontList.
 *
 *
 **************************************************************************
 *
 *  DOSAPP.INI (obsolete, supported on a read-only basis)
 *
 *  [Dos Applications]
 *
 *  C:\FULL\PATH\TO\EXE\COM\BAT\OR.PIF=<wFlags wFontWidth wFontHeight
 *          wWinWidth wWinHeight length flags showCmd ptMinPositionX
 *          ptMinPositionY ptMaxPositionX ptMaxPositionY
 *          rcNormalLeft rcNormalTop rcNormalRight rcNormalBottom>
 *
 *  Status:     Private
 *  Purpose:
 *
 *      These values are used to restore a DOS application's window
 *      to the state it was in when the DOS app last exited normally.
 *
 *      The values are taken directly from the INIINFO structure, qv.
 *
 *      The values of ptMinPositionX and ptMinPositionY are always -1,
 *      since we do not try to preserve the icon position.
 *
 *      If wFontHeight has the high bit set, then the font that
 *      should be used is a TrueType font.
 *
 *      If wFontWidth = 1 and wFontHeight = -1, then
 *      Auto-font-selection is active.
 *
 *  Compatibility notes:
 *
 *      In Windows 3.x, the "position save" (and font) information was
 *      saved in DOSAPP.INI, and although we will still read DOSAPP.INI
 *      in the absence of any information in the PIF file, we only *write*
 *      settings back to the PIF file.  DOSAPP.INI should be considered
 *      obsolete.
 *
 *
 **************************************************************************
 *
 * THE NEXT INI VAR IS NOT IMPLEMENTED BUT SHOULD BE
 *
 **************************************************************************
 *
 *  SYSTEM.INI
 *
 *  [NonWindowsApp]
 *
 *  TTFontTolerance=<i>
 *
 *  Status:     Public
 *  Default:    200
 *  Purpose:
 *
 *      This setting indicates how large a penalty (see TTHeuristics)
 *      Windows should tolerate before trying to synthesize new font
 *      sizes from TrueType fonts.
 *
 *      Decreasing this value will result in a tighter fit of the
 *      Windows-selected font to the actual window size, but at a
 *      cost in speed and memory.
 *
 *  To change:
 *
 *      Use Notepad to edit the SYSTEM.INI file.
 *
 *
 *  Internals:
 *
 *      Inspected by ChooseBestFont, if implemented.
 *
 **************************************************************************/



void PifMgrDLL_Init()
{
    static BOOL fInit = FALSE;
    if (!fInit)
    {
        LoadString(g_hinst, IDS_PIF_NONE, g_szNone, ARRAYSIZE(g_szNone));
        LoadString(g_hinst, IDS_AUTONORMAL, g_szAuto, ARRAYSIZE(g_szAuto));
        LoadGlobalFontData();
        fInit = TRUE;
    }
}

void PifMgrDLL_Term()
{
    FreeGlobalFontData();
}


/** GetPIFDir - Form default PIF directory name + name of given file
 *
 * INPUT
 *  None
 *
 * OUTPUT
 *  None
 */

void GetPIFDir(LPTSTR pszName)
{
    int i;
    static const TCHAR szBackslashPIF[] = TEXT("\\PIF");
    FunctionName(GetPIFDir);

    if (!(fbInit & INIT_PIFDIR)) {

        // Set up g_szPIFDir, less space for a filename, less space for \PIF

        i = ARRAYSIZE(g_szPIFDir)-lstrlen(pszName)-ARRAYSIZE(szBackslashPIF);
        if (i <= 0)                         // sanity check
            return;

        GetWindowsDirectory(g_szPIFDir, i);
        iPIFName = lstrlen(g_szPIFDir);
        if (lstrrchr(g_szPIFDir, TEXT('\\')) == &g_szPIFDir[iPIFName-1])
            iPIFName--;
        iWinName = iPIFName;

        lstrcpy(g_szPIFDir+iPIFName, szBackslashPIF);
        iPIFName += ARRAYSIZE(szBackslashPIF)-1;

        i = (int)GetFileAttributes(g_szPIFDir);

        if (i == -1) {

            // It didn't exist, so try to create it (returns TRUE if success)

            i = CreateDirectory(g_szPIFDir, NULL);
            if (i)
                SetFileAttributes(g_szPIFDir, FILE_ATTRIBUTE_HIDDEN);
        }
        else if (i & FILE_ATTRIBUTE_DIRECTORY)
            i = TRUE;                       // directory already exists, cool!
        else
            i = FALSE;                      // some sort of file is in the way...

        if (i) {
            g_szPIFDir[iPIFName++] = TEXT('\\');    // append the slash we'll need
                                            // to separate future filenames (the
                                            // space after is already zero-init'ed)
        }
        else                                // we'll just have to use the Windows dir
            iPIFName -= ARRAYSIZE(szBackslashPIF)-2;

        fbInit |= INIT_PIFDIR;
    }

    // Now initialize g_szPIFDir with the name of the file we're processing

    if (pszName)
        lstrcpyn(g_szPIFDir+iPIFName, pszName, ARRAYSIZE(g_szPIFDir)-iPIFName);
}


/** GetEnvVar - Locate a environment variable
 *
 * This code has been swiped from so many places, I lost track...
 *
 * INPUT
 *  pszName -> variable name, case-sensitive, terminated by equals sign
 *  cchName  =  length of variable name, including terminating equals sign
 *
 * OUTPUT
 *  Returns far pointer to environment variable.  If variable was not found,
 *  returns far pointer to null string.  (*NOT* a null pointer!)
 *
 */

// disable "inline assembler precludes global optimizations" warning
#pragma warning(disable:4704)

LPCTSTR GetEnvVar(LPCTSTR pszName, int cchName)
{
#ifndef WINNT
    FunctionName(GetEnvVar);

    GetDOSEnvironment();                /* DX:AX -> environment */
_asm {
        mov     es, dx                  ; ES -> environment
        xor     di, di                  ; ES:DI -> environment
        xor     ax, ax                  ; AX = 0 for scasb
        jmp     envTest

envLoop:
        mov     cx, cchName             ; CX = cch
        mov     si, pszName             ; DS:SI -> variable
        repe    cmpsb ; ds:[si], es:[di]
        jz      envFound                ; ES:DI -> variable contents
        mov     cx, -1                  ; Skip to next environment variable
        repnz   scasb ; es:[di]         ; AL = 0 remember

envTest:
        cmp     es:[di], al             ; AL = 0, remember
        jnz     envLoop
envFound:
        xchg    ax, di                  ; Size not speed
                                        ; dx already equals es
    }
    if (0) return 0;                    /* Keep compiler happy */
#else
    DWORD dwRet;
    TCHAR szValue[ 1024 ];   // BUGBUG: not sure what biggest value could be
    LPTSTR lpszReturn = (LPTSTR)c_szNULL;

    dwRet = GetEnvironmentVariable( pszName, szValue, ARRAYSIZE(szValue) );
    if ((dwRet<1024) && (dwRet!=0))
    {
        // +1 is for terminating NULL
        lpszReturn = LocalAlloc( LPTR, (dwRet+1)*SIZEOF(TCHAR));
        if (lpszReturn)
        {
            lstrcpy( lpszReturn, szValue );
        }
        else
        {
            lpszReturn = (LPTSTR)c_szNULL;
        }
    }

    return lpszReturn;
#endif
}

#pragma warning(default:4704)

/** GetINIData - Read WIN.INI/SYSTEM.INI/DOSAPP.INI for default settings
 *
 * INPUT
 *  Nothing
 *
 * OUTPUT
 *  Nothing; global defaults (re)set
 *
 * NOTES
 *  We only do this work now if GetPIFData couldn't open a PIF file, or
 *  could but it contained no enhanced section.  And we never do it more than
 *  once per fresh load of this DLL.
 */

void GetINIData()
{
    int t;
    const INIDATA *pid;
    LPCTSTR lpsz;
    TCHAR szTemp[MAX_PATH];
    FunctionName(GetINIData);

    if (fbInit & INIT_INIDATA)          // if already done
        return;                         // then go away

    for (pid=aINIData; pid-aINIData < ARRAYSIZE(aINIData); pid++) {

        t = *(INT UNALIGNED *)pid->pValue;
        if (pid->iFlags & (INIDATA_DECINT | INIDATA_BOOLEAN)) {

            if (pid->iFlags & INIDATA_BOOLEAN) {
                t &= pid->iMask;
                if (pid->iFlags & INIDATA_INVERT)
                    t ^= pid->iMask;
            }
            t = GetPrivateProfileInt(pid->pszSection,
                                     pid->pszKey,
                                     t,
                                     szSystemINI);
            if (pid->iFlags & INIDATA_BOOLEAN) {
                if (t)
                    t = pid->iMask;
                if (pid->iFlags & INIDATA_INVERT)
                    t ^= pid->iMask;
                t |= *(INT UNALIGNED *)pid->pValue & ~pid->iMask;
            }
            *(INT UNALIGNED *)pid->pValue = t;
        }
        else
        if (pid->iFlags & INIDATA_FIXEDPOINT) {
            wsprintf(szTemp, szDotPercent03d, t);
            GetPrivateProfileString(pid->pszSection,
                                    pid->pszKey,
                                    szTemp,
                                    szTemp,
                                    ARRAYSIZE(szTemp),
                                    szSystemINI);
            *(INT UNALIGNED *)pid->pValue = _fatoi(szTemp+1);
        }
        else
            ASSERTFAIL();
    }

    //
    // Locate COMSPEC once and for all.
    //
    lpsz = lstrrchr( GetEnvVar(szComspec, cchComspec), TEXT('\\'));
    if (lpsz) {
        lstrcpyn(szComspec, lpsz+1, ARRAYSIZE(szComspec));
    }

    fbInit |= INIT_INIDATA;
}

/** InitProperties - initialize new property structure
 *
 * INPUT
 *  ppl -> property
 *  fLocked == TRUE to return data locked, FALSE unlocked
 *
 * OUTPUT
 *  Nothing (if successful, ppl->hPIFData will become non-zero)
 */

void InitProperties(PPROPLINK ppl, BOOL fLocked)
{
    LPSTDPIF lpstd;
    LPW386PIF30 lp386 = NULL;
#ifdef UNICODE
    CHAR achPathName[ARRAYSIZE(ppl->szPathName)];
#endif
    BYTE behavior = 0;
    FunctionName(InitProperties);

    GetINIData();       // make sure we have all the right defaults

    if (ResizePIFData(ppl, SIZEOF(STDPIF)) != -1) {

        // We're no longer called *only* after a fresh ZERO'd HeapAlloc
        // by ResizePIFData.  We could be getting called because PifMgr_OpenProperties
        // was told to punt on an ambiguous PIF and create new settings.
        // Hence, we always zero-init the buffer ourselves now.

        BZero(ppl->lpPIFData, ppl->cbPIFData);

        lpstd = (LPSTDPIF)ppl->lpPIFData;
        lpstd->id = 0x78;
#ifdef UNICODE
        PifMgr_WCtoMBPath( ppl->szPathName, achPathName, ARRAYSIZE(achPathName) );
        lstrcpyncharA(lpstd->appname, achPathName+ppl->iFileName, ARRAYSIZE(lpstd->appname), '.');
#else
        lstrcpyncharA(lpstd->appname, ppl->szPathName+ppl->iFileName, ARRAYSIZE(lpstd->appname), '.');
#endif
        CharToOemA(lpstd->appname, lpstd->appname);

        // NOTE: When 3.x Setup creates PIF files, it sets maxmem to 640;
        // that's typically what memDefault.wMaxLow will be too....

        lpstd->minmem = memDefault.wMinLow;
        lpstd->maxmem = (WORD) GetProfileInt(apszAppType[APPTYPE_PIF]+1, szMemory, memDefault.wMaxLow);
#ifdef UNICODE
        lstrcpynA(lpstd->startfile, achPathName, ARRAYSIZE(lpstd->startfile));
#else
        lstrcpynA(lpstd->startfile, ppl->szPathName, ARRAYSIZE(lpstd->startfile));
#endif
        CharToOemA(lpstd->startfile, lpstd->startfile);

        //
        // New for 4.0:  fDestroy (close on exit) is disabled by default
        // for most apps, but is enabled by default for COMSPEC.
        //
        lpstd->MSflags = 0;
        if (!lstrcmpi(ppl->szPathName+ppl->iFileName, szComspec)) {
            lpstd->MSflags = fDestroy;
        }

        // Initialize various goofy non-zero stuff just to make it
        // look like a backward-compatible PIF file -- not that we use
        // or particularly care about any of it

        // NOTE: When 3.x Setup creates PIF files, it sets screen to 0x7F

        lpstd->cPages = 1;
        lpstd->highVector = 0xFF;
        lpstd->rows = 25;
        lpstd->cols = 80;
        lpstd->sysmem = 0x0007;

        // fFullScreen is no longer default, so only if an explicit
        // COMPATIBLE=FALSE exists in the PIF section of WIN.INI will
        // we set fScreen in behavior and fFullScreen in PfW386Flags
        // Similarly, fDestroy is no longer default, but we'll go
        // back to the old way if the switch tells us to.

        if (!GetProfileInt(apszAppType[APPTYPE_PIF]+1, szComp, TRUE)) {
            lpstd->behavior = behavior = fScreen;
            lpstd->MSflags = fDestroy;
        }

        if (ppl->ckbMem != -1 && ppl->ckbMem != 1)
            lpstd->minmem = lpstd->maxmem = (WORD) ppl->ckbMem;


        if (AddGroupData(ppl, szW386HDRSIG30, NULL, SIZEOF(W386PIF30))) {
            if (NULL != (lp386 = GetGroupData(ppl, szW386HDRSIG30, NULL, NULL))) {
                lp386->PfW386minmem = lpstd->minmem;
                lp386->PfW386maxmem = lpstd->maxmem;
                lp386->PfFPriority = TSKFGND_OLD_DEFAULT;
                lp386->PfBPriority = TSKBGND_OLD_DEFAULT;
                lp386->PfMinEMMK = memDefault.wMinEMS;
                lp386->PfMaxEMMK = memDefault.wMaxEMS;
                lp386->PfMinXmsK = memDefault.wMinXMS;
                lp386->PfMaxXmsK = memDefault.wMaxXMS;
                lp386->PfW386Flags = fBackground + fPollingDetect + fINT16Paste;
                if (behavior & fScreen)
                    lp386->PfW386Flags |= fFullScreen;
                lp386->PfW386Flags2 = fVidTxtEmulate + fVidNoTrpTxt + fVidNoTrpLRGrfx + fVidNoTrpHRGrfx + fVidTextMd;
            }
        }
        VERIFYTRUE(AddEnhancedData(ppl, lp386));
#ifdef WINNT
        if (AddGroupData(ppl, szWNTHDRSIG31, NULL, SIZEOF(WNTPIF31))) {
            LPWNTPIF31 lpnt31;

            if (NULL != (lpnt31 = GetGroupData(ppl, szWNTHDRSIG31, NULL, NULL))) {
                lstrcpyA( lpnt31->nt31Prop.achConfigFile, NT_CONFIG_FILE );
                lstrcpyA( lpnt31->nt31Prop.achAutoexecFile, NT_AUTOEXEC_FILE );
            }
        }
#endif
#ifdef UNICODE
        VERIFYTRUE(AddGroupData(ppl, szWNTHDRSIG40, NULL, SIZEOF(WNTPIF40)));
#endif

        // Can't be dirty anymore, 'cause we just set everything to defaults

        ppl->flProp &= ~PROP_DIRTY;

        if (!fLocked)
            ppl->cLocks--;
    }
    else
        ASSERTFAIL();
}


/** OpenPIFFile - Wrapper around CreateFile for opening PIF files
 *
 *  The wrapper handles the following things:
 *
 *      Passing the proper access and sharing flags to CreateFile.
 *      Setting pof->nErrCode = 0 on success.
 *      Converting ERROR_PATH_NOT_FOUND to ERROR_FILE_NOT_FOUND.
 *
 * INPUT
 *
 *  pszFile -> name of file to attempt to open
 *  pof -> PIFOFSTRUCT to fill in
 *
 *  OUTPUT
 *
 *  Same return code as CreateFile.
 *
 */

HANDLE OpenPIFFile(LPCTSTR pszFile, LPPIFOFSTRUCT pof)
{
    HANDLE hf;
    TCHAR pszFullFile[ MAX_PATH ];
    LPTSTR pszTheFile;
    DWORD dwRet;

    //
    // CreateFile does not search the path, so do that first, then
    // give CreateFile a fully qualified file name to open...
    //

    dwRet = SearchPath( NULL,
                        pszFile,
                        NULL,
                        ARRAYSIZE(pszFullFile),
                        pszFullFile,
                        &pszTheFile
                       );

    if ((dwRet==0) || (dwRet > ARRAYSIZE(pszFullFile)))
    {
        pszTheFile = (LPTSTR)pszFile;
    }
    else
    {
        pszTheFile = pszFullFile;
    }

    hf = CreateFile( pszTheFile,
                     GENERIC_READ,
                     FILE_SHARE_READ,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL );


    if (hf == INVALID_HANDLE_VALUE)
    {
        pof->nErrCode = GetLastError();
        if (pof->nErrCode == ERROR_PATH_NOT_FOUND)
            pof->nErrCode = ERROR_FILE_NOT_FOUND;
    }
    else
    {
        LPTSTR lpDummy;

        //
        //  NOTE:  Special hack for creating shortcuts.  If the PIF file
        //  that we find is 0 bytes long, pretend we did not find one at all.
        //  This is because appwiz renames a 0 length file from "New shortcut.lnk"
        //  to "appname.pif" and we end up finding it.  We'll ignore this file.
        //

        if (SetFilePointer( hf, 0, NULL, FILE_END) == 0)
        {
            CloseHandle( hf );
            hf = INVALID_HANDLE_VALUE;
            pof->nErrCode = ERROR_FILE_NOT_FOUND;
        }
        else
        {
            LPCTSTR pszNewFile;
            TCHAR szTemp[ ARRAYSIZE(pof->szPathName) ];

            SetFilePointer( hf, 0, NULL, FILE_BEGIN );
            pof->nErrCode = ERROR_SUCCESS;

            // In some cases, people pass in two pointers to the same
            // buffer.  This will hose GetFullPathName, so if they
            // are the same, then make a copy before calling GetFullPathName.
            if (pszTheFile==pof->szPathName) {
                FillMemory( szTemp, SIZEOF(szTemp), 0 );
                lstrcpy( szTemp, pszTheFile );
                pszNewFile = szTemp;
            }
            else
            {
                pszNewFile = pszTheFile;
            }
            GetFullPathName( pszNewFile, ARRAYSIZE(pof->szPathName),
                             pof->szPathName, &lpDummy );
        }
    }

    return hf;
}


/** PifMgr_OpenProperties - return handle to property info for application
 *
 * INPUT
 *  lpszApp -> name of application
 *  lpszPIF -> name of PIF file to use/create
 *  hInf = Inf handle, 0 if none, -1 to inhibit Inf processing
 *  flOpt = OPENPROPS_RAWIO to allow raw file updates; otherwise, 0
 *
 * OUTPUT
 *  handle to properties, FALSE if could not be opened, or out of memory
 *
 * REMARKS
 *  This should not be thought of as a function that opens a file somewhere
 *  on the disk (although that's usually the effect), but rather as an
 *  property structure allocator that is optionally initialized by disk data
 *  (currently, the file does not even remain open after this call).  So the
 *  main reason for failure in this function will be either a low memory
 *  condition *or* inability to open a specific PIF file.
 *
 *  The rules for PIF file searching are as follows:
 *
 *      If not a .PIF file:
 *          Search in current directory.
 *      Endif.
 *
 *      If path components were specified:
 *          Search in specified directory.
 *      Endif.
 *
 *      Search in PIF directory.
 *      Search the path.
 *
 *  Note that this differs from the Windows 3.1 PIF search algorithm, which
 *  was...
 *
 *      Search current directory.
 *      Search the path.
 *      Search in application directory.
 *
 *  This was a really bogus search order.  Fortunately, it seems that
 *  very few people relied on it.
 *
 *  Things to watch out for when dorking the PIF file search order:
 *
 *      Make sure editing PIF properties from the shell works.  (I.e.,
 *      if a full path to a PIF is given, then use it; don't search.)
 *
 *  Extra special thing to watch out for when dorking the PIF file
 *  search order:
 *
 *      MS Delta execs its child process as follows:
 *
 *          CreatePif("C:\DELTA\DELTABAT.PIF");
 *          SetCurrentDirectory("C:\RANDOM\PLACE");
 *          WinExec("C:\TMP\DELTABAT.BAT", SW_HIDE);
 *
 *      It expects the PIF search to pick up C:\DELTA\DELTABAT.PIF
 *      from the path, even though the WinExec supplied a full path.
 *
 */

HANDLE WINAPI PifMgr_OpenProperties(LPCTSTR lpszApp, LPCTSTR lpszPIF, UINT hInf, UINT flOpt)
{
    PPROPLINK ppl;
    LPTSTR pszExt;
    BOOL fError = FALSE;
    BOOL fFixedDisk = FALSE;
    BOOL fSearchInf = FALSE;
    BOOL fExplicitPIF = FALSE;
    PROPPRG prg;
#ifdef UNICODE
    PROPNT40 nt40;
#endif
    LPTSTR pszName, pszFullName;
#ifdef DBCS
    PROPENV env;
#endif
    FunctionName(PifMgr_OpenProperties);
    // Allocate new prop

    if (!(ppl = (PPROPLINK)LocalAlloc(LPTR, SIZEOF(PROPLINK))))
        return 0;

    if (!(pszFullName = (LPTSTR)LocalAlloc(LPTR, MAXPATHNAME*SIZEOF(TCHAR)))) {
        EVAL(LocalFree(ppl) == NULL);
        return 0;
    }

    if ((HANDLE)ppl > g_offHighestPropLink) {
        g_offHighestPropLink = (HANDLE)ppl;

    }

    // Initialize the new prop

    ppl->ppl = ppl;
    ppl->ckbMem = -1;
    ppl->iSig = PROP_SIG;
    ppl->hPIF = INVALID_HANDLE_VALUE;
    if (flOpt & OPENPROPS_RAWIO)
        ppl->flProp |= PROP_RAWIO;

    #if (PRGINIT_INHIBITPIF != PROP_INHIBITPIF)
    #error PRGINIT_INIHIBITPIF and PROP_INHIBITPIF out of sync!
    #endif

    ppl->flProp |= (flOpt & PROP_INHIBITPIF);

    // Link into the global list

    if (NULL != (ppl->pplNext = g_pplHead))
        g_pplHead->pplPrev = ppl;
    g_pplHead = ppl;

    // Copy app name to both temp and perm buffers, and record location
    // of base filename, and extension if any, within the buffer

    lstrcpyn(pszFullName, lpszApp, MAXPATHNAME-4);
    lstrcpyn(ppl->szPathName, pszFullName, ARRAYSIZE(ppl->szPathName));

    if (NULL != (pszName = lstrrchr(pszFullName, TEXT('\\'))) ||
        NULL != (pszName = lstrrchr(pszFullName, TEXT(':'))))
        pszName++;
    else
        pszName = pszFullName;

    if (!(pszExt = lstrrchr(pszName, TEXT('.'))))
        pszExt = pszFullName + lstrlen(pszFullName);

    ppl->iFileName = (UINT) (pszName - pszFullName);
    ppl->iFileExt = (UINT) (pszExt - pszFullName);

    // Check the application's file extension

    if (!*pszExt) {
        lstrcat(pszFullName, apszAppType[APPTYPE_PIF]);
    }
    else if (!lstrcmpi(pszExt, apszAppType[APPTYPE_EXE]) ||
             !lstrcmpi(pszExt, apszAppType[APPTYPE_COM]) ||
             !lstrcmpi(pszExt, apszAppType[APPTYPE_BAT])) {
//             !lstrcmpi(pszExt, apszAppType[APPTYPE_CMD])) {
        lstrcpy(pszExt, apszAppType[APPTYPE_PIF]);
    }
    else if (!lstrcmpi(pszExt, apszAppType[APPTYPE_PIF]))
        fExplicitPIF = TRUE;
    else {
        // Let's disallow random file extensions, since WinOldAp never
        // allowed them either
        goto Error;
    }

    // INFONLY means the caller just wants to search the INF, so ignore
    // any WIN.INI garbage and any PIFs laying around.  We still look for
    // _DEFAULT.PIF, since that code takes care of other important
    // initialization that needs to happen when no PIF was found at all.

    if (flOpt & OPENPROPS_INFONLY)
        goto FindDefault;

    // Backward compatibility requires that if the app is not a PIF,
    // then we must check the PIF section of WIN.INI for an entry matching
    // the base name of the app.  If the entry exists, then we have to skip
    // the PIF search, and pass the value of the entry to InitProperties,
    // which it uses to establish default memory requirements
    //
    // Also note that if IGNOREPIF is set, then ofPIF.szPathName is nothing
    // more than the name of the app that was given to PifMgr_OpenProperties;  this
    // may give us the opportunity to do something more intelligent later...

    if (!fExplicitPIF) {
        ppl->ckbMem = GetProfileInt(apszAppType[APPTYPE_PIF]+1, ppl->szPathName+ppl->iFileName, -1);
        if (ppl->ckbMem != -1) {
            ppl->flProp |= PROP_IGNOREPIF | PROP_SKIPPIF;
            lstrcpyn(ppl->ofPIF.szPathName, lpszApp, ARRAYSIZE(ppl->ofPIF.szPathName));
            goto IgnorePIF;     // entry exists, skip PIF file search
        }
    }

    //
    // Initialize default error return code.  Once we get a successful
    // open, it will be set to zero.
    //
    ppl->flProp |= PROP_NOCREATEPIF;
    ppl->ofPIF.nErrCode = ERROR_FILE_NOT_FOUND;

    //
    // We must search in the current directory if not given a path to a PIF.
    // We need to prefix `.\' to the filename so that OpenFile will not do
    // a path search.
    //
    if (!fExplicitPIF || pszName == pszFullName) {
        //
        // This relies on a feature of OpenFile, that it copies the input
        // buffer to a private buffer before stomping the output buffer,
        // thus permitting precisely the stunt we are pulling here, namely,
        // passing an input buffer equal to the output buffer.
        //
#ifdef UNICODE
        *(LPDWORD)(ppl->ofPIF.szPathName) = 0x005C002E; /*dot backslash prefix */
#else
        *(PWORD)(ppl->ofPIF.szPathName) = 0x5C2E;   /* dot backslash prefix */
#endif
        lstrcpyn( &ppl->ofPIF.szPathName[2],
                  pszName,
                  ARRAYSIZE(ppl->ofPIF.szPathName) - 2);
        ppl->hPIF = OpenPIFFile(ppl->ofPIF.szPathName, &ppl->ofPIF);
    }

    //
    // If we were given a path component, then look in that directory.
    // (The fact that we have a backslash or drive letter will suppress
    // the path search.)
    //
    if (pszName != pszFullName && ppl->ofPIF.nErrCode == ERROR_FILE_NOT_FOUND) {

        ppl->hPIF = OpenPIFFile(pszFullName, &ppl->ofPIF);

        // If we didn't find a PIF there, we'd probably still like to create
        // one there if the media is a fixed disk.  Network shares, CD-ROM
        // drives, and floppies are not good targets for PIF files in general.
        //
        // So, if the media is a fixed disk, set the fFixedDisk flag so that
        // we'll leave pszFullName alone.

        if (ppl->hPIF == INVALID_HANDLE_VALUE && pszFullName[1] == TEXT(':')) {
            TCHAR szTemp[4];

            lstrcpyn( szTemp, pszFullName, 4 );

            szTemp[3] = (TCHAR)0;

            if (GetDriveType(szTemp) == DRIVE_FIXED)
                    fFixedDisk++;
        }
    }

    // BUGBUG: replace this PIF dir search with a registry search -JTP
    //
    // Failing that, let's look in the PIF directory.  Again, since we're
    // supplying a full pathname, OpenFile won't try to search the PATH again.

    if (ppl->ofPIF.nErrCode == ERROR_FILE_NOT_FOUND) {
        GetPIFDir(pszName);
        ppl->hPIF = OpenPIFFile(g_szPIFDir, &ppl->ofPIF);
        if (ppl->hPIF != INVALID_HANDLE_VALUE)
            ppl->flProp |= PROP_PIFDIR;
    }

    // If we're still in trouble, our last chance is to do a path
    // search.  This is an unconditional search, thanks to the
    // wonders of MS-Delta.

    if (ppl->ofPIF.nErrCode == ERROR_FILE_NOT_FOUND) {
        ppl->hPIF = OpenPIFFile(pszName, &ppl->ofPIF);
    }

    if (ppl->hPIF == INVALID_HANDLE_VALUE) {

        if (ppl->ofPIF.nErrCode != ERROR_FILE_NOT_FOUND || fExplicitPIF) {

            // Hmmm, file *may* exist, but it cannot be opened;  if it's a
            // strange error, or we were specifically told to open that file,
            // then return error

            goto Error;
        }

    FindDefault:

        fSearchInf = TRUE;
        ppl->flProp &= ~PROP_NOCREATEPIF;

        // Any files we find now are NOT really what we wanted, so save
        // the name we'd like to use in the future, in case we need to save
        // updated properties later.
        //
        // We must save the name now because we might stomp g_szPIFDir while
        // searching for the _default.pif.  Furthermore, we must save it in
        // the buffer we HeapAlloc'ed (pszFullName) temporarily, because
        // the following calls to OpenPIFFile can still stomp on szPathName
        // in our OpenFile structure (ofPIF.szPathName).

        GetPIFDir(pszName);
        if (!fFixedDisk)                        // save desired name in
            lstrcpy(pszFullName, g_szPIFDir);     // temp buffer (pszFullName)

        //
        // Try to locate the _default.pif.
        //

        if (fTryDefaultPif()) {

            if (!fDefaultPifFound()) {          // Must search for it

                // First try PIFDir

                lstrcpy(g_szPIFDir+iPIFName, g_szDefaultPIF);
                ppl->hPIF = OpenPIFFile(g_szPIFDir, &ppl->ofPIF);

                if (ppl->ofPIF.nErrCode == ERROR_FILE_NOT_FOUND) { // try PATH
                    ppl->hPIF = OpenPIFFile(g_szDefaultPIF, &ppl->ofPIF);
                }

            } else {                            // Look in cached path

                // We've already found it once, so just open it

                ppl->hPIF = OpenPIFFile(g_szDefaultPIF, &ppl->ofPIF);
            }
        }

        if (ppl->hPIF != INVALID_HANDLE_VALUE) {

            ppl->flProp |= PROP_DEFAULTPIF;

            // Save the fully-qualified pathname of the default PIF file,
            // so that subsequent OpenFile() calls will be faster (note that
            // we don't specify OF_SEARCH on that particular call)

            lstrcpy(g_szDefaultPIF, ppl->ofPIF.szPathName);
        }
        else {

            // Not only could we not open any sort of PIF, we also need to
            // tell GetPIFData to not bother trying to open the file itself
            // (since it is unlikely someone created one in this short time)

            ppl->flProp |= PROP_NOPIF | PROP_SKIPPIF;

            if (ppl->ofPIF.nErrCode == ERROR_FILE_NOT_FOUND)
                g_szDefaultPIF[0] = 0;            // Invalidate cache.
        }

        // NOW we can set ppl->ofPIF.szPathName to the filename we REALLY
        // wanted, since we're done with all the calls to OpenPIFFile.

        lstrcpy(ppl->ofPIF.szPathName, pszFullName);
    }

    // Initialize the properties by PIF if we have one, by hand if not

  IgnorePIF:

    // We don't need to check the return code from GetPIFData() here,
    // because we validate hPIFData below anyway.  Please also note that
    // this GetPIFData call uses the handle we supplied (if any), and closes
    // it for us when it's done.  Furthermore, if we didn't supply a handle,
    // then we should have set PROP_SKIPPIF, so that GetPIFData won't try to
    // open anything (since we just tried!)

    GetPIFData(ppl, FALSE);

    // Now that the original file from which we obtained settings (if any) is
    // closed, we need to see if the caller wants us to create a new PIF file
    // using a specific name.  If so, force it to be created now.

    if (lpszPIF) {
        lstrcpy(ppl->ofPIF.szPathName, lpszPIF);
        ppl->flProp |= PROP_DIRTY;
        ppl->flProp &= ~PROP_NOCREATEPIF;
        fError = !FlushPIFData(ppl, FALSE);
    }

    // Apply INF data to the PIF data we just retrieved, as appropriate,
    // as long as it's an app file and not a PIF file (and if, in the case of
    // creating a specific PIF, we were actually able to create one).

    if (!fError && !fExplicitPIF && (hInf != -1)) {

        if (PifMgr_GetProperties(ppl, MAKELP(0,GROUP_PRG),
                            &prg, SIZEOF(prg), GETPROPS_NONE)) {

            // In the PRGINIT_AMBIGUOUSPIF case, GetAppsInfData must
            // again look for a matching entry;  however, if the entry it
            // finds is the same as what we've already got (based on Other
            // File), then it will leave the PIF data alone (ie, it doesn't
            // reinitialize it, it doesn't call AppWiz to silently
            // reconfigure it, etc).

            if (fSearchInf || (prg.flPrgInit & PRGINIT_AMBIGUOUSPIF)) {

#ifdef UNICODE
                if (PifMgr_GetProperties(ppl, MAKELP(0,GROUP_NT40),
                                &nt40, SIZEOF(nt40), GETPROPS_NONE)) {

                if (!GetAppsInfData(ppl, &prg, &nt40, (HINF)IntToPtr( hInf ), lpszApp, fFixedDisk, flOpt)) {
#else
                if (!GetAppsInfData(ppl, &prg, (HINF)IntToPtr( hInf ), lpszApp, fFixedDisk, flOpt)) {
#endif

                    // When GetAppsInfData fails and the PIF is ambiguous, then
                    // we need to restart the PIF search process at the point where
                    // it searches for _DEFAULT.PIF, so that the ambiguous PIF is
                    // effectively ignored now.

                    // Also, we avoid the ugly possibility of getting to this
                    // point again and infinitely jumping back FindDefault, by
                    // only jumping if fSearchInf was FALSE.  FindDefault sets
                    // it to TRUE.

                    if (!fSearchInf && (prg.flPrgInit & PRGINIT_AMBIGUOUSPIF)) {
                        goto FindDefault;
                    }
#ifdef DBCS
                    if (GetSystemDefaultLangID() == 0x0411) {
                        lstrcpyA(env.achBatchFile, ImeBatchFile);
                        PifMgr_SetProperties(ppl, MAKELP(0,GROUP_ENV),
                                             &env, SIZEOF(env), SETPROPS_NONE);
                    }
#endif
                }
#ifdef UNICODE
                }
#endif
            }
        }
    }

  Error:
    LocalFree(pszFullName);

    if (fError || !ppl->lpPIFData) {
        PifMgr_CloseProperties(ppl, 0);
        return 0;
    }

    // We should never leave PIFMGR with outstanding locks

    ASSERTTRUE(!ppl->cLocks);

    return ppl;
}


/** PifMgr_GetProperties - get property info by name
 *
 * INPUT
 *  hProps = handle to properties
 *  lpszGroup -> property group; may be one of the following:
 *      "WINDOWS 286 3.0"
 *      "WINDOWS 386 3.0"
 *      "WINDOWS VMM 4.0"
 *      "WINDOWS NT  3.1"
 *      "WINDOWS NT  4.0"
 *    or any other group name that is the name of a valid PIF extension;
 *    if NULL, then cbProps is a 0-based index of a named group, and lpProps
 *    must point to a 16-byte buffer to receive the name of the group (this
 *    enables the caller to enumerate the names of all the named groups)
 *  lpProps -> property group record to receive the data
 *  cbProps = size of property group record to get; if cbProps is zero
 *    and a named group is requested, lpProps is ignored, no data is copied,
 *    and the size of the group record is returned (this enables the caller
 *    to determine the size of a named group)
 *  flOpt = GETPROPS_RAWIO to perform raw file read (lpszGroup ignored)
 *
 *  Alternatively, if the high word (selector) of lpszGroup is 0, the low
 *  word must be a group ordinal (eg, GROUP_PRG, GROUP_TSK, etc)
 *
 * OUTPUT
 *  If the group is not found, or an error occurs, 0 is returned.
 *  Otherwise, the size of the group info transferred in bytes is returned.
 */

int WINAPI PifMgr_GetProperties(HANDLE hProps, LPCSTR lpszGroup, void *lpProps, int cbProps, UINT flOpt)
{
    int cb, i;
    void *lp;
    LPW386PIF30 lp386;
    LPWENHPIF40 lpenh;
#ifdef UNICODE
    LPWNTPIF40 lpnt40;
#endif
#ifdef WINNT
    LPWNTPIF31 lpnt31;
#endif

    PPROPLINK ppl;
    FunctionName(PifMgr_GetProperties);

    cb = 0;

    if (!(ppl = ValidPropHandle(hProps)))
        return cb;

    // We should never enter PIFMGR with outstanding locks (we also call
    // here from *inside* PIFMGR, but none of those cases should require a
    // lock either)

    ASSERTTRUE(!ppl->cLocks);

    ppl->cLocks++;

    if (flOpt & GETPROPS_RAWIO) {
        if (ppl->flProp & PROP_RAWIO) {
            cb = min(ppl->cbPIFData, cbProps);
            hmemcpy(lpProps, ppl->lpPIFData, cb);
        }
        ppl->cLocks--;
        return cb;
    }

    if (!lpszGroup) {
        if (lpProps) {
            lp = GetGroupData(ppl, NULL, &cbProps, NULL);
            if (lp) {
                cb = cbProps;
                hmemcpy(lpProps, lp, PIFEXTSIGSIZE);
            }
        }
    }
    else if (IS_INTRESOURCE(lpszGroup) && lpProps) {

        // Special case: if GROUP_ICON, then do a nested call to
        // PifMgr_GetProperties to get GROUP_PRG data, then feed it to load
        // LoadPIFIcon, and finally return the hIcon, if any, to the user.

        if (LOWORD((DWORD_PTR) lpszGroup) == GROUP_ICON) {
#ifdef UNICODE
            PPROPPRG pprg;
            PPROPNT40 pnt40 = (void *)LocalAlloc(LPTR, SIZEOF(PROPNT40));
            if ( pnt40 ) {
                pprg = (void *)LocalAlloc(LPTR, SIZEOF(PROPPRG));
                if (pprg) {
                    if ( PifMgr_GetProperties(ppl, MAKELP(0,GROUP_PRG), pprg, SIZEOF(PROPPRG), GETPROPS_NONE)
                          && PifMgr_GetProperties(ppl, MAKELP(0,GROUP_NT40), pnt40, SIZEOF(PROPNT40), GETPROPS_NONE) ) {
                        *(HICON *)lpProps = LoadPIFIcon(pprg, pnt40);
                        cb = 2;
                    }
                    EVAL(LocalFree(pprg) == NULL);
                }
                EVAL(LocalFree(pnt40) == NULL);
            }
#else
            PPROPPRG pprg;

            pprg = (void *)LocalAlloc(LPTR, SIZEOF(PROPPRG));
            if (pprg) {
                if ( PifMgr_GetProperties(ppl, MAKELP(0,GROUP_PRG), pprg, SIZEOF(PROPPRG), GETPROPS_NONE) ) {
                    *(HICON *)lpProps = LoadPIFIcon(pprg);
                    cb = 2;
                }
                EVAL(LocalFree(pprg) == NULL);
            }

#endif
        }
        else {
            lp386 = GetGroupData(ppl, szW386HDRSIG30, NULL, NULL);
            lpenh = GetGroupData(ppl, szWENHHDRSIG40, NULL, NULL);
#ifdef UNICODE
            lpnt40 = GetGroupData(ppl, szWNTHDRSIG40, NULL, NULL);
#endif
#ifdef WINNT
            lpnt31  = GetGroupData(ppl, szWNTHDRSIG31, NULL, NULL);
#endif

            //
            // Fix anything from down-level PIF files.  Since this
            // is the first revision of the WENHPIF40 format, we
            // don't have anything to worry about (yet).
            //
            // Don't muck with PIF files from the future!
            //
// We don't have to remove this until product 2 actually -JTP
// #pragma message("Remove this after M8")
#if 1
            if (lpenh && lpenh->wInternalRevision != WENHPIF40_VERSION) {
#else
            if (lpenh && lpenh->wInternalRevision < WENHPIF40_VERSION) {
#endif
                lpenh->wInternalRevision = WENHPIF40_VERSION;
                ppl->flProp |= PROP_DIRTY;

                //
                //  Old (pre-M7) PIFs did not zero-initialize the reserved
                //  fields of PIF files, so zero them out now.
                //
                lpenh->tskProp.wReserved1 = 0;
                lpenh->tskProp.wReserved2 = 0;
                lpenh->tskProp.wReserved3 = 0;
                lpenh->tskProp.wReserved4 = 0;
                lpenh->vidProp.wReserved1 = 0;
                lpenh->vidProp.wReserved2 = 0;
                lpenh->vidProp.wReserved3 = 0;
                lpenh->envProp.wMaxDPMI = 0;

                // Turn off bits that have been deleted during the development
                // cycle.
                lpenh->envProp.flEnv = 0;
                lpenh->envProp.flEnvInit = 0;
                if (lp386)
                    lp386->PfW386Flags &= ~0x00400000;
            }
            // End of "Remove this after M8"

            // Zero the input buffer first, so that the Get* functions
            // need not initialize every byte to obtain consistent results

            BZero(lpProps, cbProps);

            // The GetData functions CANNOT rely on either lp386 or lpenh

            i = LOWORD((DWORD_PTR) lpszGroup)-1;
            if (i >= 0 && i < ARRAYSIZE(afnGetData) && cbProps >= acbData[i]) {
                void *aDataPtrs[NUM_DATA_PTRS];

                aDataPtrs[ LP386_INDEX ] = (LPVOID)lp386;
                aDataPtrs[ LPENH_INDEX ] = (LPVOID)lpenh;

#ifdef UNICODE
                aDataPtrs[ LPNT40_INDEX ] = (LPVOID)lpnt40;
#endif

#ifdef WINNT
                aDataPtrs[ LPNT31_INDEX ] = (LPVOID)lpnt31;
#endif

                cb = (afnGetData[i])(ppl, aDataPtrs, lpProps, cbProps, flOpt );
            }
        }
    }
    else if (NULL != (lp = GetGroupData(ppl, lpszGroup, &cb, NULL))) {
        if (lpProps && cbProps != 0) {
            cb = min(cb, cbProps);
            hmemcpy(lpProps, lp, cb);
        }
    }
    ppl->cLocks--;

#ifdef EXTENDED_DATA_SUPPORT

    // Note that for GETPROPS_EXTENDED, both the normal and extended
    // sections are returned, and that the return code reflects the success
    // or failure of reading the normal portion only.  We return both because
    // that's the most convenient thing to do for the caller.

    if (flOpt & GETPROPS_EXTENDED) {
        if (ppl->hVM) {
            WORD wGroup = EXT_GROUP_QUERY;
            if (!HIWORD(lpszGroup) && LOWORD(lpszGroup) <= MAX_GROUP)
                wGroup |= LOWORD(lpszGroup);
            GetSetExtendedData(ppl->hVM, wGroup, lpszGroup, lpProps);
        }
    }
#endif

    // We should never leave PIFMGR with outstanding locks (we also call
    // here from *inside* PIFMGR, but none of those cases should require a
    // lock either)

    ASSERTTRUE(!ppl->cLocks);

    return cb;
}


/** PifMgr_SetProperties - set property info by name
 *
 * INPUT
 *  hProps = handle to properties
 *  lpszGroup -> property group; may be one of the following:
 *      "WINDOWS 286 3.0"
 *      "WINDOWS 386 3.0"
 *      "WINDOWS PIF.400"
 *    or any other group name that is the name of a valid PIF extension
 *  lpProps -> property group record to copy the data from
 *  cbProps = size of property group record to set;  if cbProps is
 *    zero and lpszGroup is a group name, the group will be removed
 *  flOpt = SETPROPS_RAWIO to perform raw file write (lpszGroup ignored)
 *          SETPROPS_CACHE to cache changes until properties are closed
 *
 *  Alternatively, if the high word (selector) of lpszGroup is 0, the low
 *  word must be a group ordinal (eg, GROUP_PRG, GROUP_TSK, etc)
 *
 * OUTPUT
 *  If the group is not found, or an error occurs, 0 is returned.
 *  Otherwise, the size of the group info transferred in bytes is returned.
 */

int WINAPI PifMgr_SetProperties(HANDLE hProps, LPCSTR lpszGroup, void *lpProps, int cbProps, UINT flOpt)
{
    void *p = NULL;
    void *lp = NULL;
    LPW386PIF30 lp386;
    LPWENHPIF40 lpenh;
#ifdef UNICODE
    LPWNTPIF40 lpnt40;
#endif
#ifdef WINNT
    LPWNTPIF31 lpnt31;
#endif
    int i, cb = 0;
    PPROPLINK ppl;

    FunctionName(PifMgr_SetProperties);

    if (!lpProps)
        return 0;

    ppl = ValidPropHandle(hProps);
    if (!ppl)
        return 0;

    // We should never enter PIFMGR with outstanding locks (we also call
    // here from *inside* PIFMGR, but none of those cases should require a
    // lock either)

    ASSERTTRUE(!ppl->cLocks);

    if (flOpt & SETPROPS_RAWIO) {
        if (ppl->flProp & PROP_RAWIO) {
            ppl->cLocks++;
            cb = min(ppl->cbPIFData, cbProps);
            if (IsBufferDifferent(ppl->lpPIFData, lpProps, cb)) {
                hmemcpy(ppl->lpPIFData, lpProps, cb);
                ppl->flProp |= PROP_DIRTY;
            }
            if (cb < ppl->cbPIFData)
                ppl->flProp |= PROP_DIRTY | PROP_TRUNCATE;
            ppl->cbPIFData = cb;
            ppl->cLocks--;
        }
        return cb;
    }

#ifdef EXTENDED_DATA_SUPPORT

    // Note that, unlike GETPROPS_EXTENDED, SETPROPS_EXTENDED only updates
    // the extended section, and that the return code reflects the existence
    // of a VM only.  This is because there's a performance hit associated
    // with setting the normal portion, and because the caller generally only
    // wants to set one or the other.

    if (flOpt & SETPROPS_EXTENDED) {
        if (ppl->hVM) {
            WORD wGroup = EXT_GROUP_UPDATE;
            cb = cbProps;
            if (!HIWORD(lpszGroup) && LOWORD(lpszGroup) <= MAX_GROUP)
                wGroup |= LOWORD(lpszGroup);
            GetSetExtendedData(ppl->hVM, wGroup, lpszGroup, lpProps);
        }
        return cb;
    }
#endif

    // For named groups, if the group does NOT exist, or DOES but is
    // a different size, then we have to remove the old data, if any, and
    // then add the new.

    if (!IS_INTRESOURCE(lpszGroup)) {

        cb = PifMgr_GetProperties(hProps, lpszGroup, NULL, 0, GETPROPS_NONE);

        if (cb == 0 || cb != cbProps) {
            if (cb) {
                RemoveGroupData(ppl, lpszGroup);
                cb = 0;
            }
            if (cbProps) {
                if (AddGroupData(ppl, lpszGroup, lpProps, cbProps))
                    cb = cbProps;
            }
            goto done;
        }
    }

    if (cbProps) {
        if (!lpszGroup)
            return cb;

        p = (void *)LocalAlloc(LPTR, cbProps);
        if (!p)
            return cb;
    }

    cb = PifMgr_GetProperties(hProps, lpszGroup, p, cbProps, GETPROPS_NONE);

    // If the group to set DOES exist, and if the data given is
    // different, copy into the appropriate group(s) in the PIF data

    if (cb != 0) {
        cbProps = min(cb, cbProps);
        if (IsBufferDifferent(p, lpProps, cbProps)) {
            cb = 0;
            ppl->cLocks++;
            i = LOWORD((DWORD_PTR) lpszGroup)-1;
            if (!IS_INTRESOURCE(lpszGroup)) {
                lp = GetGroupData(ppl, lpszGroup, NULL, NULL);
                if (lp) {
                    cb = cbProps;
                    hmemcpy(lp, lpProps, cbProps);
                    ppl->flProp |= PROP_DIRTY;
                }
            }
            else if (i >= 0 && i < ARRAYSIZE(afnSetData) && cbProps >= acbData[i]) {

                // Insure that both 386 and enhanced sections of PIF
                // file are present.  There are some exceptions:  all
                // groups from GROUP_MSE on up do not use the 386 section,
                // and GROUP_MEM does not need the enh section....

                lp386 = GetGroupData(ppl, szW386HDRSIG30, NULL, NULL);
                if (i < GROUP_MSE-1 && !lp386) {
                    if (AddGroupData(ppl, szW386HDRSIG30, NULL, SIZEOF(W386PIF30))) {
                        lp386 = GetGroupData(ppl, szW386HDRSIG30, NULL, NULL);
                        if (!lp386) {
                            ASSERTFAIL();
                            cbProps = 0;    // indicate error
                        }
                    }
                }
                if (cbProps) {
                    lpenh = GetGroupData(ppl, szWENHHDRSIG40, NULL, NULL);
                    if (i != GROUP_MEM-1 && !lpenh) {
                        if (!(lpenh = AddEnhancedData(ppl, lp386))) {
                            ASSERTFAIL();
                            cbProps = 0;    // indicate error
                        }
                    }
#ifdef UNICODE
                    lpnt40 = GetGroupData(ppl, szWNTHDRSIG40, NULL, NULL);
                    if (!lpnt40)
                    {
                        if (AddGroupData(ppl, szWNTHDRSIG40, NULL, SIZEOF(WNTPIF40)))
                        {
                            lpnt40 = GetGroupData(ppl, szWNTHDRSIG40, NULL, NULL);
                        }
                    }
                    ASSERT(lpnt40);
#endif

#ifdef WINNT
                    lpnt31 = GetGroupData(ppl, szWNTHDRSIG31, NULL, NULL);
                    if (!lpnt31)
                    {
                        if (AddGroupData(ppl, szWNTHDRSIG31, NULL, SIZEOF(WNTPIF31)))
                        {
                            if (NULL != (lpnt31 = GetGroupData(ppl, szWNTHDRSIG31, NULL, NULL))) {
                                lstrcpyA( lpnt31->nt31Prop.achConfigFile, NT_CONFIG_FILE );
                                lstrcpyA( lpnt31->nt31Prop.achAutoexecFile, NT_AUTOEXEC_FILE );
                            }
                        }
                    }
                    ASSERT(lpnt31);
#endif
                }
                if (cbProps)
                {

                    void *aDataPtrs[NUM_DATA_PTRS];

                    //
                    // We need to re-establish the pointers because any of
                    // the AddGroupData's could have moved the block (via
                    // a HeapReAlloc call), so do that now...
                    //

                    lp386 = GetGroupData( ppl, szW386HDRSIG30, NULL, NULL );
                    lpenh = GetGroupData( ppl, szWENHHDRSIG40, NULL, NULL );
#ifdef UNICODE
                    lpnt40 = GetGroupData( ppl, szWNTHDRSIG40, NULL, NULL );
#endif
#ifdef WINNT
                    lpnt31 = GetGroupData( ppl, szWNTHDRSIG31, NULL, NULL );
#endif

                    aDataPtrs[ LP386_INDEX ] = (LPVOID)lp386;
                    aDataPtrs[ LPENH_INDEX ] = (LPVOID)lpenh;
#ifdef UNICODE
                    aDataPtrs[ LPNT40_INDEX ] = (LPVOID)lpnt40;
#endif

#ifdef WINNT
                    aDataPtrs[ LPNT31_INDEX ] = (LPVOID)lpnt31;
#endif

                    cb = (afnSetData[i])(ppl, aDataPtrs, lpProps, cbProps, flOpt );
                }
            }
            ppl->cLocks--;
        }
    }
    EVAL(LocalFree(p) == NULL);

  done:
    if (!(flOpt & SETPROPS_CACHE))
        if (!FlushPIFData(ppl, FALSE))
            cb = 0;

    // We should never leave PIFMGR with outstanding locks (we also call
    // here from *inside* PIFMGR, but none of those cases should require a
    // lock either)

    ASSERTTRUE(!ppl->cLocks);

    return cb;
}


/** FlushProperties - flush (or discard) any cached property info
 *
 * INPUT
 *  hProps = handle to properties
 *  flOpt = FLUSHPROPS_DISCARD to abandon cached PIF data, otherwise flush it
 *
 * OUTPUT
 *  TRUE if successful, FALSE otherwise
 */

int WINAPI FlushProperties(HANDLE hProps, UINT flOpt)
{
    PPROPLINK ppl;
    FunctionName(FlushProperties);

    if (!(ppl = ValidPropHandle(hProps)))
        return FALSE;

    return FlushPIFData(ppl, (flOpt & FLUSHPROPS_DISCARD));
}


/** EnumProperties - enumerate open properties
 *
 * INPUT
 *  hProps = handle to previous properties (NULL to start)
 *
 * OUTPUT
 *  next property handle, 0 if none
 */

HANDLE WINAPI EnumProperties(HANDLE hProps)
{
    PPROPLINK ppl;
    FunctionName(EnumProperties);

    if (!hProps)
        return g_pplHead;

    if (!(ppl = ValidPropHandle(hProps)))
        return NULL;

    return ppl->pplNext;
}


/** AssociateProperties - associate data with property info
 *
 * INPUT
 *  hProps = handle to properties
 *  iAssociate = association index (eg, HVM_ASSOCIATION)
 *  lData = new associated data
 *
 * OUTPUT
 *  previously associated data for the index, 0 if none (or error)
 *
 * NOTES
 *  If iAssociate is a negative association index, then the current
 *  associated value is returned and not modified (ie, lData is ignored)
 */

LONG_PTR WINAPI AssociateProperties(HANDLE hProps, int iAssociate, LONG_PTR lData)
{
    LONG_PTR l;
    int iIndex;
    PPROPLINK ppl;
    FunctionName(AssociateProperties);

    if (!(ppl = ValidPropHandle(hProps)))
        return FALSE;

    iIndex = iAssociate;
    if (iIndex < 0)
        iIndex *= -1;

    switch(iIndex) {
    case HVM_ASSOCIATION:
        l = ppl->hVM;
        break;
    case HWND_ASSOCIATION:
        l = (LONG_PTR)ppl->hwndTty;
        break;
    case LPARGS_ASSOCIATION:
        l = (LONG_PTR)ppl->lpArgs;
        break;
    default:
        return FALSE;
    }

    switch(iAssociate) {
    case HVM_ASSOCIATION:
        ppl->hVM = (DWORD) lData;
        break;
    case HWND_ASSOCIATION:
        ppl->hwndTty = (HWND)lData;
        break;
    case LPARGS_ASSOCIATION:
        ppl->lpArgs = (LPTSTR)lData;
        break;
    }
    return l;
}


void SetBootDrive(LPTSTR pszFile)
{
    TCHAR szPath[10];
    DWORD cbPath = sizeof(szPath);

    szPath[0] = 0;
    SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup"), TEXT("BootDir"), NULL, szPath, &cbPath);

    pszFile[0] = szPath[0] ? szPath[0] : TEXT('C');
}

#define RENAME_DELETENEW    0x0001
#define RENAME_CREATEOLD    0x0002
#define RENAME_DELETEOLD    0x0004

/** RenameStartupFiles - rename startup files (for real-mode only)
 *
 * INPUT
 *  pszOld -> old filename (eg, C:CONFIG.SYS)
 *  pszNew -> new filename (eg, C:CONFIG.WOS)
 *  flRename == see RENAME_* above
 *
 * OUTPUT
 *  None
 */

void RenameStartupFiles(LPTSTR pszOld, LPTSTR pszNew, int flRename)
{
    SetBootDrive(pszOld);
    SetBootDrive(pszNew);

    // For RENAME_DELETENEW:  make sure there's no way an existing new filename
    // could block the rename process, not even a R/H/S file!

    if (flRename & RENAME_DELETENEW) {
        SetFileAttributes(pszNew, 0);
        DeleteFile(pszNew);
    }

    // For RENAME_CREATEOLD:  if the old filename doesn't exist, then we
    // want to create a hidden 0-length copy of the new filename, as a way
    // of telling ourselves later that the old file didn't exist.  Then,
    // when it's later (see RENAME_DELETEOLD), if the file we're to rename
    // is a hidden 0-length file, just delete it.

    if (flRename & RENAME_CREATEOLD) {

        int i;
        HANDLE hFile;

        i = (int)GetFileAttributes(pszOld);
        if (i == -1) {
            hFile = CreateFile( pszNew,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_HIDDEN,
                                NULL
                               );
            if (hFile != INVALID_HANDLE_VALUE)
                CloseHandle(hFile);
            return;
        }
    }

    // For RENAME_DELETEOLD:  if the old file is a hidden 0-length
    // dummy file that we presumably created ourselves, then simply delete it.

    if (flRename & RENAME_DELETEOLD) {

        WIN32_FIND_DATA dFind;
        HANDLE hFind = FindFirstFile(pszOld, &dFind);
        if (hFind != INVALID_HFINDFILE) {
            FindClose(hFind);
            if ((dFind.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && dFind.nFileSizeLow == 0) {
                DeleteFile(pszOld);
                return;
            }
        }
    }
    MoveFile(pszOld, pszNew);
}


/** CheckForceReboot - return TRUE if exit to real-mode has been disabled
 *
 *  This function largely copied from win\core\user\inexit.c -JTP
 *
 *  The key being examined is:
 *
 *      HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Control\\Shutdown
 *
 *  The value-name is:
 *
 *      ForceReboot
 *
 *  The value to force a reboot is:
 *
 *      "1"
 *
 * INPUT
 *  None
 *
 * OUTPUT
 *  TRUE if reboot must be forced, FALSE if not
 */

BOOL CheckForceReboot()
{
    // Check to see if we have to force a reboot of the system. One reason is
    // so that the double space utilities can tell us the system must be reboot
    // since we can't exit to real mode double space...

    HKEY     hkey;
    static TCHAR szShutdown[] = REGSTR_PATH_SHUTDOWN;
    BOOL     fForce=FALSE;
    TCHAR    sz[12];
    DWORD    dwType;

    if (!RegOpenKey(HKEY_LOCAL_MACHINE, szShutdown, &hkey))
    {

        LONG cb = SIZEOF(sz);
        static TCHAR szForceReboot[] = REGSTR_VAL_FORCEREBOOT;

        if (!SHQueryValueEx(hkey, szForceReboot, NULL, &dwType, (LPBYTE)sz, &cb))
        {
            if (*sz==TEXT('1'))
                fForce = TRUE;
        }

        RegCloseKey(hkey);
    }

    return(fForce);
}


/** CreateStartupProperties - create startup files (for real-mode only)
 *
 * INPUT
 *  hProps = handle to properties
 *  flOpt = CREATEPROPS_NONE (all bits reserved)
 *
 * OUTPUT
 *  TRUE (1) if any files were created, FALSE (0) if there was an error
 *  creating 1 or more of the necessary files, and -1 there was no error and
 *  no files needed to be created.
 */

int WINAPI CreateStartupProperties(HANDLE hProps, UINT flOpt)
{
#ifndef WINNT
    HFILE hf;
    HKEY hKey;
#endif
    BOOL fForceReboot;
    int fSuccess = FALSE;
    PPROPLINK ppl;
    FunctionName(CreateStartupProperties);

    if (NULL != (ppl = ValidPropHandle(hProps))) {

        InitRealModeFlag(ppl);

        if (ppl->flProp & PROP_REALMODE) {

            // WriteAdvPrgData only returns the exact value TRUE (1) if it
            // actually created a file.  If there was no need to create
            // a particular file, and therefore no error, it returns -1.

            fForceReboot = CheckForceReboot();
            fSuccess = WriteAdvPrgData(ppl, MAX_CONFIG_SIZE, szCONFIGHDRSIG40, g_szMConfigFile, g_szConfigFile, fForceReboot, fForceReboot);
            fSuccess &= WriteAdvPrgData(ppl, MAX_AUTOEXEC_SIZE, szAUTOEXECHDRSIG40, g_szMAutoexecFile, g_szAutoexecFile, fSuccess, fForceReboot);

#ifndef WINNT
            // If we created 1 or more files, then we need to also update
            // the RunOnce section in the registry to run PIFCONV.EXE, who
            // will then look for his REGSTR_VAL_MSDOSMODE value under his
            // REGSTR_PATH_PIFCONVERT key, which should point to a PIF that
            // the startup files in the root can be read back into.

            if (fSuccess == 1) {

                // Rename the startup files to their new names
                RenameStartupFiles(g_szConfigFile, g_szWConfigFile, RENAME_DELETENEW | RENAME_CREATEOLD);
                RenameStartupFiles(g_szMConfigFile, g_szConfigFile, 0);
                RenameStartupFiles(g_szAutoexecFile, g_szWAutoexecFile, RENAME_DELETENEW | RENAME_CREATEOLD);
                RenameStartupFiles(g_szMAutoexecFile, g_szAutoexecFile, 0);

                // Create a 0-length normal-attribute file on the host drive
                // so that IO.SYS can determine that we're in MSDOS-mode before
                // any compression drivers are loaded.

                if (SetBootHostDrive(g_szMSDOSSTSFile)) {
                    hf = _lcreat(g_szMSDOSSTSFile, 0);
                    if (hf != HFILE_ERROR) {
                        _lclose(hf);
                    }
                }

                // Add the PIFCONV info to the registry
                if (RegCreateKey(HKEY_LOCAL_MACHINE, szPIFConvertKey, &hKey) == ERROR_SUCCESS) {

                    // Set the MSDOSMode value
                    RegSetValueEx(hKey, szMSDOSMode, 0L, REG_SZ, (LPBYTE)ppl->ofPIF.szPathName, ARRAYSIZE(ppl->ofPIF.szPathName));

                    // Set the Discard value
                    RegSetValueEx(hKey, szMSDOSModeDiscard, 0L, REG_BINARY, (LPBYTE)&fForceReboot, SIZEOF(fForceReboot));

                    RegCloseKey(hKey);

                    fSuccess = 2;
                }
            }

            // If step 1 succeeded, then move on to step 2

            if (fSuccess == 2) {

                // Add the PIFCONV applet to the RunOnce Key
                if (RegCreateKey(HKEY_LOCAL_MACHINE, szRunOnceKey, &hKey) == ERROR_SUCCESS) {

                    // Set the command line to run RUNDLL, which
                    // will in turn invoke PIFMGR!ProcessStartupProperties
                    RegSetValueEx(hKey, szPIFConvert, 0L, REG_SZ, (LPBYTE)szPIFConvertExe, ARRAYSIZE(szPIFConvertExe));

                    RegCloseKey(hKey);
                }
            }
#endif
        }
    }
    return fSuccess;
}


/** DoLargeEnvironmentSubstA - do large environment substitution
 *
 *  NOTE: the below comment is no longer true.  DoEnvironmentSubst
 *        calls ExpandEnvironmentStrings which does not have this restriction!
 *
 *  This is a wrapper around shell's DoEnvironmentSubst that can handle
 *  larger buffers.  Although not well-documented, DoEnvironmentSubst will
 *  fail if the input buffer simply contains more than 256 chars, hence
 *  it is not entirely suitable for our purposes.
 *
 *  The basic idea is:  find CR/LF-terminated lines, copy them to szTmp
 *  with a null terminator instead of the CR/LF, do environment substitution
 *  on szTmp, move the data in psz up by the number of bytes szTmp grew (if
 *  any), and then copy the expanded string back into psz.  Repeat until no
 *  more lines.
 */

int DoLargeEnvironmentSubstA(LPSTR psz, int cchsz, BOOL fStripWIN)
{
    int cch, cchNew, cchDiff;
    DWORD dwResult;
    CHAR szTmp[256];
    CHAR *pszCRLF, *pszCRLFNext;

    pszCRLF = psz;
    while (NULL != (pszCRLFNext = (PBYTE)(UINT_PTR)(ULONG_PTR)_fstrstr(pszCRLF, szCRLF))) {

        cch = (int)(pszCRLFNext - pszCRLF);
        pszCRLFNext += ARRAYSIZE(szCRLF)-1;

        if (cch >= ARRAYSIZE(szTmp))
            goto Next;

        hmemcpy(szTmp, pszCRLF, cch);
        szTmp[cch] = '\0';

        // fStripWIN means: strip WIN wherever you find it and do NOTHING else

        if (fStripWIN) {
            if (lstrcmpiA(szTmp, szWin) == 0) {
                cchNew = 0;
                goto Strip;
            }
            goto Next;
        }

        cchNew = cch;
        dwResult = DoEnvironmentSubstA(szTmp, ARRAYSIZE(szTmp));
        if (HIWORD(dwResult))
            cchNew = LOWORD(dwResult);

    Strip:
        cchDiff = cchNew - cch;            // cchDiff == # chars needed
        if (cchDiff) {

            if (cchDiff > cchsz - (lstrlenA(psz)+1))
                goto Next;              // not enough room left

            cch = lstrlenA(pszCRLFNext)+1;// cch == # chars required to copy

            hmemcpy(pszCRLFNext+(cchDiff*SIZEOF(CHAR)), pszCRLFNext, cch*SIZEOF(CHAR));
            pszCRLFNext += cchDiff;
        }

        hmemcpy(pszCRLF, szTmp, cchNew*SIZEOF(CHAR));
        hmemcpy(pszCRLF+cchNew, szCRLF, (ARRAYSIZE(szCRLF)-1)*SIZEOF(CHAR));

      Next:
        pszCRLF = pszCRLFNext;
    }
    return lstrlenA(psz);
}


#ifndef WINNT
/** DeleteStartupProperties - delete startup files (for real-mode only)
 *
 * INPUT
 *  hProps = handle to properties
 *  flOpt = DELETEPROPS_NONE, or DELETEPROPS_DISCARD to avoid re-merging the
 *          contents back into the PIF, or DELETEPROPS_ABORT to clean up files
 *          left from aborted startup;  all other bits reserved
 *
 * OUTPUT
 *  TRUE if successful, FALSE otherwise
 */

int WINAPI DeleteStartupProperties(HANDLE hProps, UINT flOpt)
{
    HKEY hKey;
    int fSuccess = FALSE;
    PPROPLINK ppl;
    FunctionName(DeleteStartupProperties);

    if (ppl = ValidPropHandle(hProps)) {

        fSuccess++;

        if (flOpt & DELETEPROPS_ABORT) {

            // Rename the original startup files back;  we only do this
            // in the case where WINOLDAP is calling to discard the new
            // startup files (ie, the restart was aborted), because normally
            // WIN.COM /W (or IO.SYS, in a pinch) will have renamed everything
            // by the time we get back into Windows again.

            // Delete any cookie file on the host drive, too.

            if (SetBootHostDrive(g_szMSDOSSTSFile)) {
                DeleteFile(g_szMSDOSSTSFile);
            }

            RenameStartupFiles(g_szWConfigFile, g_szConfigFile, RENAME_DELETEOLD | RENAME_DELETENEW);
            RenameStartupFiles(g_szWAutoexecFile, g_szAutoexecFile, RENAME_DELETEOLD | RENAME_DELETENEW);

            if (RegOpenKey(HKEY_LOCAL_MACHINE, szRunOnceKey, &hKey) == ERROR_SUCCESS) {

                RegDeleteValue(hKey, szPIFConvert);

                RegCloseKey(hKey);
            }
        }
        else {
            ReadAdvPrgData(ppl, MAX_CONFIG_SIZE, szCONFIGHDRSIG40, g_szMConfigFile, flOpt);
            ReadAdvPrgData(ppl, MAX_AUTOEXEC_SIZE, szAUTOEXECHDRSIG40, g_szMAutoexecFile, flOpt);
        }
    }
    return fSuccess;
}
#endif


void ReadAdvPrgData(PPROPLINK ppl, int cbMax, LPCSTR lpszName, LPTSTR pszFile, UINT flOpt)
{
    HANDLE hFile;
    int i;
    DWORD cbData = 0, cb;
    CHAR  szTmp[256];
    PBYTE pb, pbLast;
    PBYTE pbData = NULL, pbDataTmp, pbDataOrig;

    SetBootDrive(pszFile);

    if (flOpt & DELETEPROPS_DISCARD) {
        DeleteFile(pszFile);
        return;
    }

    hFile = CreateFile( pszFile,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

    if (hFile != INVALID_HANDLE_VALUE) {

        pbData = (PBYTE)LocalAlloc(LPTR, cbMax);

        if (pbData) {

            pbDataOrig = pbData;
            ReadFile( hFile, (PVOID)pbData, (DWORD)cbMax-1, &cbData, NULL);

            // Guess what, this routine isn't so general purpose after all:
            // in the autoexec case, we must find the signature that precedes
            // the command-line that we added to the file.  For expediency,
            // we simply delete it and everything after it.

            if (pszFile == g_szMConfigFile) {

                // If "DOS=SINGLE" is present, skip over it

                if (pbData == (PBYTE)(UINT_PTR)(ULONG_PTR)_fstrstr(pbData, szSingle)) {
                    cb = lstrlenA(szSingle);
                    pbData += cb;
                    cbData -= cb;
                }
            }

            if (pszFile == g_szMAutoexecFile) {

                // If "@echo off" is present, skip over it

                if (LoadStringA(g_hinst, IDS_AUTOEXECTOP, szTmp, ARRAYSIZE(szTmp))) {

                    if (pbData == (PBYTE)(UINT_PTR)(ULONG_PTR)_fstrstr(pbData, szTmp)) {
                        cb = lstrlenA(szTmp);
                        pbData += cb;
                        cbData -= cb;
                    }
                }

                // Try to find our signature at the end of the file now

                if (LoadStringA(g_hinst, IDS_AUTOEXECBOTTOM, szTmp, ARRAYSIZE(szTmp))) {

                    pb = pbData;
                    pbLast = NULL;
                    while (NULL != (pb = (PBYTE)(UINT_PTR)(ULONG_PTR)_fstrstr(pb, szTmp))) {
                        pbLast = pb++;
                    }

                    // If there was a (last) occurrence of the signature
                    // in the file, then it's starting point is where we cut
                    // off the data we're about to rewrite.

                    if (pbLast)
                        cbData = (DWORD) (pbLast - pbData);
                }
            }
        }
        CloseHandle(hFile);

        DeleteFile(pszFile);
    }

    // Before we simply blast the data back out to the PIF, we check to
    // see if the data we're about to write is the same as what's already
    // there after environment string substitution.  If so, we leave the
    // data alone, so that environment strings are left in their pre-expanded
    // form whenever possible (and with a minimum of work on our part).

    i = -1;
    if (NULL != (pbDataTmp = (PBYTE)LocalAlloc(LPTR, cbMax))) {
        if (PifMgr_GetProperties(ppl, lpszName, pbDataTmp, cbMax-1, GETPROPS_NONE)) {
            DoLargeEnvironmentSubstA(pbDataTmp, cbMax, FALSE);
            i = lstrcmpA(pbDataTmp, pbData);
        }
        EVAL(LocalFree(pbDataTmp) == NULL);
    }

    // Note that if the file no longer exists, or does but is zero bytes
    // long, this call effectively removes the data from the PIF.  Otherwise,
    // the PIF gets refreshed with the contents of the startup file, which
    // we deleted immediately after sucking its contents out.

    if (i != 0)
        PifMgr_SetProperties(ppl, lpszName, pbData, cbData, SETPROPS_NONE);

    if (pbDataOrig)
        EVAL(LocalFree(pbDataOrig) == NULL);
}


BOOL WriteAdvPrgData(PPROPLINK ppl, int cbMax, LPCSTR lpszName, LPTSTR pszFile, LPTSTR pszOrigFile, BOOL fCreateAnyway, BOOL fForceReboot)
{
    HANDLE hFile;
    int cbData, cb, i, j;
    PBYTE pbData;
    PROPPRG prg;
    PROPENV env;
    DWORD   dwBytes;
    BOOL fPause = FALSE;
    BOOL fSuccess = FALSE;
#ifndef WINNT
    LPCTSTR lpsz;
#endif
    CHAR szTmp[MAXPATHNAME];

    cbData = PifMgr_GetProperties(ppl, lpszName, NULL, 0, GETPROPS_NONE);

    if (!cbData) {              // if there's no autoexec data
        if (fCreateAnyway != 1) // and no previous file was created
            return -1;          // then there's no need to create this file
    }

    // If fCreateAnyway is -1, then we know are being called for AUTOEXEC
    // *and* that no file was created for CONFIG.  Since we are committed to
    // creating an AUTOEXEC now, we therefore *force* the CONFIG file to
    // be created as well.  That way, the user won't end up with the default
    // CONFIG (he may have just not have needed/cared about device drivers, but
    // still wanted to run in real-mode).

    if (fCreateAnyway == -1) {
        if (!WriteAdvPrgData(ppl, MAX_CONFIG_SIZE, szCONFIGHDRSIG40, g_szMConfigFile, g_szConfigFile, TRUE, fForceReboot))
            return FALSE;
    }

    pbData = (PBYTE)LocalAlloc(LPTR, cbMax);
    if (pbData) {
        if (cbData) {
            if (cbData >= cbMax)
                cbData = cbMax-1;
            PifMgr_GetProperties(ppl, lpszName, pbData, cbData, GETPROPS_NONE);

        } else if (fForceReboot) {

            SetBootDrive(pszOrigFile);

            hFile = CreateFile( pszOrigFile,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                               );

            if (hFile != INVALID_HANDLE_VALUE) {

                cbData = (int)SetFilePointer( hFile, 0, NULL, FILE_END );
                if (cbData) {
                    if (cbData >= cbMax)
                        cbData = cbMax-1;
                    SetFilePointer( hFile, 0, NULL, FILE_BEGIN );
                    if (ReadFile( hFile, pbData, cbData, &dwBytes, NULL)) {
                        if (dwBytes != (UINT)cbData) {
                            cbData = 0;
                        } else {
                            cbData = (int)dwBytes;
                        }

                    }
                }

                CloseHandle(hFile);
            }
        }
    }
    else
        cbData = 0;

    SetBootDrive( pszFile );

    hFile = CreateFile( pszFile,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

    if (hFile!=INVALID_HANDLE_VALUE) {

        // Do environment string substitution on the buffer before writing
        // it out.  Treat it like the huge null-terminated string that it is.

        cbData = DoLargeEnvironmentSubstA(pbData, cbMax, fForceReboot);

        // Do the simple thing:  ALWAYS write out the "@echo off" line;
        // if someone really wants to see the batch file execute, let them
        // put an explicit "echo on" in their autoexec edit control;  since it
        // comes after ours, it will override as appropriate.
        //
        // This rule also simplifies conversion on the way back:  if the
        // first line is "@echo off", we delete it.

        if (pszFile == g_szMConfigFile) {
            WriteFile( hFile, szSingle, lstrlenA(szSingle), &dwBytes, NULL );
        }

        if (pszFile == g_szMAutoexecFile) {
            szTmp[0] = '\0';
            LoadStringA(g_hinst, IDS_AUTOEXECTOP, szTmp, ARRAYSIZE(szTmp));
            WriteFile( hFile, szTmp, lstrlenA(szTmp), &dwBytes, NULL );
        }

        if (WriteFile(hFile, pbData, cbData, &dwBytes, NULL) && (dwBytes==(DWORD)cbData))
            fSuccess++;         // return TRUE to indicate file created

        // Make sure the user-defined gob-o-goo is CR/LF-terminated;
        // if not, spit one out.

        if (cbData != 0)
            if (cbData < 2 || lstrcmpA(pbData+cbData-(ARRAYSIZE(szCRLF)-1), szCRLF) != 0)
                WriteFile( hFile, szCRLF, SIZEOF(szCRLF)-1, &dwBytes, NULL);

        // Guess what, this routine isn't so general purpose after all:
        // in the autoexec case, we must append the command-line to
        // the file, followed by C:\WINDOWS\WIN.COM /W to restore everything

        if (pszFile == g_szMAutoexecFile) {

            if (PifMgr_GetProperties(ppl, MAKELP(0,GROUP_PRG),
                                &prg, SIZEOF(prg), GETPROPS_NONE)) {

                szTmp[0] = '\0';
                LoadStringA(g_hinst, IDS_AUTOEXECBOTTOM, szTmp, ARRAYSIZE(szTmp));
                WriteFile(hFile, szTmp, lstrlenA(szTmp), &dwBytes, NULL);

                // If the program is on drive A, then it is time emit:
                //
                //  ECHO Please insert the diskette for Mickey's ABCs in drive A now
                //  PAUSE
                //
                // since WinOldAp will have asked the user to remove it.

                if ((ppl->szPathName[0] == TEXT('a') ||
                     ppl->szPathName[0] == TEXT('A')) && ppl->szPathName[1] == TEXT(':')) {

                    CHAR szTmpFmt[128];

                    fPause++;
                    WriteFile(hFile, szEcho, SIZEOF(szEcho)-1, &dwBytes, NULL);

                    szTmpFmt[0] = '\0';
                    LoadStringA(g_hinst, IDS_DISKINSERT, szTmpFmt, ARRAYSIZE(szTmpFmt));

                    // The DISKINSERT string should have one %s for the title...

                    wsprintfA(szTmp, szTmpFmt, prg.achTitle);

                    WriteFile(hFile, szTmp, lstrlenA(szTmp), &dwBytes, NULL);
                    WriteFile(hFile, szPause, SIZEOF(szPause)-1, &dwBytes, NULL);
                }

                if (PifMgr_GetProperties(ppl, MAKELP(0,GROUP_ENV),
                                    &env, SIZEOF(env), GETPROPS_NONE)) {

                    if (env.achBatchFile[0]) {

                        WriteFile(hFile, szCall, SIZEOF(szCall)-1, &dwBytes, NULL);

                        // env.achBatchFile could be a long filename and/or
                        // quoted (since it could be followed by args).  Take
                        // care of it now.

                        i = lstrunquotefnameA(szTmp, env.achBatchFile, ARRAYSIZE(szTmp), TRUE);

                        j = lstrskipfnameA(env.achBatchFile);

                        if (env.achBatchFile[j])
                            lstrcpynA(szTmp+i, env.achBatchFile+j, ARRAYSIZE(szTmp)-i);

                        WriteFile(hFile, szTmp, lstrlenA(szTmp), &dwBytes, NULL);
                        WriteFile(hFile, szCRLF, SIZEOF(szCRLF)-1, &dwBytes, NULL);
                    }
                }
#ifdef DBCS
//                else if (!GetProperties(ppl, lpszName, NULL, 0, GETPROPS_NONE))
//                    _lwrite(hFile, szDOSIME, lstrlen(szDOSIME));
#endif

                if (!prg.achWorkDir[0]) {

                    // If there's no hard-coded working directory,
                    // create one.  It's ok to party on prg.achWorkDir because
                    // we're not going to call PifMgr_SetProperties and inadvertently
                    // change it.

#ifdef UNICODE
                    InitWorkDir(ppl, &prg, NULL);
#else
                    InitWorkDir(ppl, &prg );
#endif
                }

                if (prg.achWorkDir[0]) {

                    cb = lstrunquotefnameA(szTmp, prg.achWorkDir, ARRAYSIZE(szTmp), TRUE);

                    if (szTmp[1] == ':') {
                        WriteFile(hFile, szTmp, 2, &dwBytes, NULL);
                        WriteFile(hFile, szCRLF, ARRAYSIZE(szCRLF)-1, &dwBytes, NULL);
                    }
                    WriteFile(hFile, szCD, SIZEOF(szCD)-1, &dwBytes, NULL);
                    WriteFile(hFile, szTmp, cb, &dwBytes, NULL);
                    WriteFile(hFile, szCRLF, SIZEOF(szCRLF)-1, &dwBytes, NULL);
                }

                // We now always CALL the cmd-line, in case it's a batch file.
                // If it isn't, no biggie, because command.com is smart enough
                // to ignore it.

                WriteFile(hFile, szCall, SIZEOF(szCall)-1, &dwBytes, NULL);

                // If the properties we have are ones *we* created in the
                // PIF dir, and it looks like the application name passed to
                // PifMgr_OpenProperties is a "qualified" pathname, then we'll use
                // that instead of the path stored in the PIF, since the app
                // might have moved since *we* created the PIF.

                if ((ppl->flProp & PROP_PIFDIR) &&
                    (ppl->szPathName[0] == TEXT('\\') ||
                     ppl->szPathName[0] && ppl->szPathName[1] == TEXT(':'))) {

                    // We're making this call not because ppl->szPathName
                    // is quoted (it shouldn't be), but because it could still
                    // be a long filename, so translate it.

#ifdef UNICODE
                    CHAR achPathName[ ARRAYSIZE(ppl->szPathName) ];
                    PifMgr_WCtoMBPath( ppl->szPathName, achPathName, ARRAYSIZE(achPathName) );
                    i = lstrunquotefnameA(szTmp, achPathName, ARRAYSIZE(szTmp), -1);
#else
                    i = lstrunquotefnameA(szTmp, ppl->szPathName, ARRAYSIZE(szTmp), -1);
#endif
                }
                else {

                    // As for the thing stored in the PIF, now *that* could
                    // both long and quoted.  Take care of it now.

                    i = lstrunquotefnameA(szTmp, prg.achCmdLine, ARRAYSIZE(szTmp), TRUE);
                }

                // BUGBUG: There is an obscure case where the preceding call
                // to lstrunquotefname can fail, returning i == 0, even though
                // WinOldAp already verified the app exists.  That case is when
                // the app name is NOT fully-qualified, so it exists somewhere
                // on the PATH, but NOT in the current directory, and the app
                // name is an LFN.  Because lstrunquotefname must call INT 21h,
                // function 7160h, subfunction 1 (NAMTRN_DO83QUERY), IFSMGR must
                // find the equivalent 8.3 name for the LFN app name, but all
                // IFSMGR has to work with is the current directory, and that's
                // not where the app is, so the INT 21h fails, returning error
                // #3 (ERROR_PATH_NOT_FOUND).
                //
                // To solve, we could do another PATH search for the app file,
                // via OpenFileEx;  if found, it would return the fully-qualifed
                // long-file-name of the app, in the OEM char set, which we
                // could then perform NameTrans on, to get full 8.3.  Yuck! -JP


                // If there are any arguments in the PIF, find them
                // and append them, giving PREFERENCE to any args that WINOLDAP
                // specifically associated with this instance of the app.

                if (ppl->lpArgs) {      // any associated args?
                    j = lstrlen(ppl->lpArgs);
                    if (j) {
                        szTmp[i++] = ' ';
                        if (ppl->lpArgs[j-1] == TEXT('\r'))
                            j--;
#ifdef UNICODE
                        {
                            CHAR achArgs[ ARRAYSIZE(ppl->lpArgs) ];

                            WideCharToMultiByte( CP_ACP, 0,
                                                 ppl->lpArgs, -1,
                                                 achArgs, ARRAYSIZE(achArgs),
                                                 NULL, NULL );
                            lstrcpynA(szTmp+i, achArgs, min(SIZEOF(szTmp)-i,(unsigned)j+1));
                        }
#else
                        lstrcpynA(szTmp+i, ppl->lpArgs, min(SIZEOF(szTmp)-i,(unsigned)j+1));
#endif
                    }
                } else {
                    j = lstrskipfnameA(prg.achCmdLine);

                    if (prg.achCmdLine[j])
                        lstrcpynA(szTmp+i, prg.achCmdLine+j, ARRAYSIZE(szTmp)-i);
                }

                WriteFile(hFile, szTmp, lstrlenA(szTmp), &dwBytes, NULL);
                WriteFile(hFile, szCRLF, SIZEOF(szCRLF)-1, &dwBytes, NULL);

                // If we paused above to ask for a disk, we should pause
                // again to ask them to remove the disk.  We won't actually
                // emit a PAUSE though unless close-on-exit is set, since
                // WIN.COM should already pause for them when the cursor isn't
                // at 0,0.

                if (fPause) {
                    WriteFile(hFile, szEcho, SIZEOF(szEcho)-1, &dwBytes, NULL);

                    szTmp[0] = TEXT('\0');
                    LoadStringA(g_hinst, IDS_DISKREMOVE, szTmp, ARRAYSIZE(szTmp));

                    WriteFile(hFile, szTmp, lstrlenA(szTmp), &dwBytes, NULL);

                    if (prg.flPrg & PRG_CLOSEONEXIT)
                        WriteFile(hFile, szPause, SIZEOF(szPause)-1, &dwBytes, NULL);
                    else
                        WriteFile(hFile, szCRLF, SIZEOF(szCRLF)-1, &dwBytes, NULL);
                }

#ifndef WINNT   // BUGBUG -- do we need this for NT?

                // Now emit the WIN.COM line that will get us back to Windows...

                GetPIFDir(NULL);
                lstrcpy(g_szPIFDir+iWinName, szWinCom);

                if (!(prg.flPrgInit & PRGINIT_QUICKSTART)) {
                    if (!(prg.flPrg & PRG_CLOSEONEXIT))
                        lpsz = szWinCommand;
                    else
                        lpsz = szExitWinCommand;
                } else {
                    if (!(prg.flPrg & PRG_CLOSEONEXIT))
                        lpsz = szQuickWinCommand;
                    else
                        lpsz = szExitQuickWinCommand;
                }
                lstrcat(g_szPIFDir, lpsz);

                // The preceding lstrcpy toasts g_szPIFDir, so remember that
                // we need to re-init in the event that we cannot start this
                // application after all.

                fbInit &= ~INIT_PIFDIR;

                WriteFile(hFile, g_szPIFDir, lstrlen(g_szPIFDir), &dwBytes, NULL);
#endif
            }
        }
        CloseHandle(hFile);
    }
    if (pbData)
        EVAL(LocalFree(pbData) == NULL);

    return fSuccess;
}


/** PifMgr_CloseProperties - close property info for application
 *
 * INPUT
 *  hProps = handle to properties
 *  flOpt = CLOSEPROPS_DISCARD to abandon cached PIF data, otherwise save it
 *
 * OUTPUT
 *  NULL if successful, otherwise hProps is returned as given
 */

HANDLE WINAPI PifMgr_CloseProperties(HANDLE hProps, UINT flOpt)
{
    PPROPLINK ppl;
    FunctionName(PifMgr_CloseProperties);

    if (!(ppl = ValidPropHandle(hProps)))
        return hProps;

    // When discarding on a close, set the SKIPPIF flag, so that the
    // flush code won't say "oh, not only should I throw away my current
    // set of data, but I should read in clean data" -- new data is no use
    // since the caller is closing.

    if (flOpt & CLOSEPROPS_DISCARD)
        ppl->flProp |= PROP_SKIPPIF;

    if (ppl->flProp & PROP_DIRTY) {     // this redundant check added
                                        // to avoid making FlushPIFData PRELOAD -JTP

        // Note that we avoid calling FlushPIFData if INHIBITPIF is set,
        // since FlushPIFData will just return a fake TRUE result anyway.
        // But we don't want to be fooled, we want to make sure the block
        // gets unlocked now.

        if ((ppl->flProp & PROP_INHIBITPIF) || !FlushPIFData(ppl, (flOpt & CLOSEPROPS_DISCARD))) {

            // If FlushPIFData failed, then if we still have an outstanding
            // dirty lock, force the data to become unlocked, by clearing the
            // dirty flag in the middle of a pair otherwise pointless lock/unlock
            // calls (because that's the nice, clean way to do it!)

            if (ppl->flProp & PROP_DIRTYLOCK) {
                ppl->cLocks++;
                ppl->flProp &= ~PROP_DIRTY;
                ppl->cLocks--;
            }
        }
    }

    if (ppl->lpPIFData) {
        LocalFree(ppl->lpPIFData);
        ppl->lpPIFData = NULL;
    }

    if (ppl->hPIF != INVALID_HANDLE_VALUE)
        CloseHandle(ppl->hPIF);

    // Unlink from the global list

    if (ppl->pplPrev)
        ppl->pplPrev->pplNext = ppl->pplNext;
    else
        g_pplHead = ppl->pplNext;

    if (ppl->pplNext)
        ppl->pplNext->pplPrev = ppl->pplPrev;

    LocalFree(ppl);
    return NULL;
}


/** ValidPropHandle - verify handle
 *
 * INPUT
 *  hProps = handle to properties
 *
 * OUTPUT
 *  pointer to prop, NULL otherwise
 */

PPROPLINK ValidPropHandle(HANDLE hProps)
{
    FunctionName(ValidPropHandle);
    if (!hProps ||
        (HANDLE)hProps > g_offHighestPropLink ||
        ((PPROPLINK)hProps)->iSig != PROP_SIG) {
        ASSERTFAIL();
        return NULL;
    }
    return (PPROPLINK)hProps;
}


/** ResizePIFData - verify handle and resize PIF data
 *
 * INPUT
 *  ppl -> property
 *  cbResize = bytes to resize PIF data by
 *
 * OUTPUT
 *  previous size of PIF data if successful, -1 if not
 *
 *  on success, the PIF data is returned LOCKED, so successful
 *  ResizePIFData calls should be matched with UnlockPIFData calls.
 */

int ResizePIFData(PPROPLINK ppl, INT cbResize)
{
    INT cbOld, cbNew;
    void *lpNew;
    BOOL fInitStdHdr = FALSE;
    FunctionName(ResizePIFData);

    ASSERTTRUE(cbResize != 0);

    // Cope with empty or old PIF files

    cbOld = ppl->cbPIFData;
    cbNew = ppl->cbPIFData + cbResize;

    if ((cbNew < cbOld) == (cbResize > 0))
        return -1;      // underflow/overflow

    if (!ppl->lpPIFData && cbOld == 0) {
        if (cbNew >= SIZEOF(STDPIF) + SIZEOF(PIFEXTHDR))
            fInitStdHdr = TRUE;
        lpNew = LocalAlloc(LPTR, cbNew);
    }
    else
    {

        if (cbOld == SIZEOF(STDPIF))
        {
            fInitStdHdr = TRUE;
            cbOld += SIZEOF(PIFEXTHDR);
            cbNew += SIZEOF(PIFEXTHDR);
        }

        lpNew = LocalReAlloc( ppl->lpPIFData, cbNew, LMEM_MOVEABLE|LMEM_ZEROINIT);

    }

    if (lpNew) {
        ppl->cbPIFData = cbNew;
        ppl->lpPIFData = (LPPIFDATA)lpNew;
        ppl->cLocks++;
        if (fInitStdHdr) {
            lstrcpyA(ppl->lpPIFData->stdpifext.extsig, szSTDHDRSIG);
            ppl->lpPIFData->stdpifext.extnxthdrfloff = LASTHDRPTR;
            ppl->lpPIFData->stdpifext.extfileoffset = 0x0000;
            ppl->lpPIFData->stdpifext.extsizebytes = SIZEOF(STDPIF);
        }
        return cbOld;
    }
    return -1;
}



/** GetPIFData - read PIF data back from PIF
 *
 * INPUT
 *  ppl -> property
 *  fLocked == TRUE to return data locked, FALSE unlocked
 *
 * OUTPUT
 *  TRUE if succeeded, FALSE if not
 */

BOOL GetPIFData(PPROPLINK ppl, BOOL fLocked)
{
    DWORD dwOff;
    LPTSTR pszOpen;
    BOOL fSuccess = FALSE;
    FunctionName(GetPIFData);

    // Since we're going to (re)load the property data now, reset
    // the current size, so that ResizePIFData will resize it from zero

    ppl->cbPIFData = 0;

    // If SKIPPIF is set (eg, by PifMgr_OpenProperties), then don't
    // try to open anything (since PifMgr_OpenProperties already tried!),

    if (ppl->hPIF == INVALID_HANDLE_VALUE && !(ppl->flProp & PROP_SKIPPIF)) {
        pszOpen = g_szDefaultPIF;
        if (!(ppl->flProp & PROP_DEFAULTPIF))
            pszOpen = ppl->ofPIF.szPathName;
        ppl->hPIF = CreateFile( pszOpen,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );
    }
    if (ppl->hPIF == INVALID_HANDLE_VALUE) {

        // The following warning is disabled because the presence of
        // the dialog box got WINOLDAP stuck in an infinite message loop -JTP

        InitProperties(ppl, fLocked);
        goto Exit;
    }
    dwOff = SetFilePointer(ppl->hPIF, 0, NULL, FILE_END);
    if (dwOff >= SIZEOF(STDPIF)) {

        ppl->flProp |= PROP_REGEN;

        if (ResizePIFData(ppl, dwOff) != -1) {

            SetFilePointer(ppl->hPIF, 0, NULL, FILE_BEGIN);
            ReadFile( ppl->hPIF, ppl->lpPIFData,
                      ppl->cbPIFData, &ppl->cbPIFData, NULL );

            // Can't be dirty anymore, 'cause we just read the PIF back in

            ppl->flProp &= ~PROP_DIRTY;

            if (ppl->flProp & PROP_DEFAULTPIF) {
#ifdef UNICODE

                // BUGBUG: Do we need to do PifMgr_WCtoMBPath on just the
                //         filename?
                WideCharToMultiByte( CP_ACP, 0,
                                     ppl->szPathName+ppl->iFileName,
                                     -1,
                                     ppl->lpPIFData->stdpifdata.appname,
                                     ARRAYSIZE(ppl->lpPIFData->stdpifdata.appname),
                                     NULL, NULL
                                    );

                PifMgr_WCtoMBPath( ppl->szPathName,
                                   ppl->lpPIFData->stdpifdata.startfile,
                                   ARRAYSIZE(ppl->lpPIFData->stdpifdata.startfile)
                                  );
#else
                lstrcpyA(ppl->lpPIFData->stdpifdata.appname, ppl->szPathName+ppl->iFileName);
                lstrcpynA(ppl->lpPIFData->stdpifdata.startfile, ppl->szPathName, ARRAYSIZE(ppl->lpPIFData->stdpifdata.startfile));
#endif

                // I don't think this is generally worth dirtying the
                // property info for, because otherwise every app that used
                // _DEFAULT.PIF initially would get its own PIF file created
                // later;  PIF file creation should only take place when
                // substantive changes have been made

                // ppl->flProp |= PROP_DIRTY;
            }

            // If we're not dealing with an enhanced PIF, then we
            // go to the various INI files to retrieve DOS app defaults

            if (!GetGroupData(ppl, szWENHHDRSIG40, NULL, NULL)) {
                GetINIData();
            }

#ifdef UNICODE

            // If we're not dealing with a new NT/UNICODE PIF, then
            // we add a new section so it's ALWAYS there when we're
            // UNICODE enabled.

            if (!GetGroupData(ppl, szWNTHDRSIG40, NULL, NULL)) {
                VERIFYTRUE(AddGroupData(ppl, szWNTHDRSIG40, NULL, SIZEOF(WNTPIF40)));
            }
#endif

#ifdef WINNT
            // If we're not dealing with a NT PIF, then
            // we add the NT sections so it's ALWAYS there when we're
            // running on NT.

            if (!GetGroupData(ppl, szWNTHDRSIG31, NULL, NULL)) {
                LPWNTPIF31 lpnt31;

                VERIFYTRUE(AddGroupData(ppl, szWNTHDRSIG31, NULL, SIZEOF(WNTPIF31)));
                if (NULL != (lpnt31 = GetGroupData(ppl, szWNTHDRSIG31, NULL, NULL))) {
                    lstrcpyA( lpnt31->nt31Prop.achConfigFile, NT_CONFIG_FILE );
                    lstrcpyA( lpnt31->nt31Prop.achAutoexecFile, NT_AUTOEXEC_FILE );
                }
            }
#endif

            if (!fLocked)
                ppl->cLocks--;  // UnlockPIFData(ppl);
            fSuccess++;
        }
        else
            ASSERTFAIL();

        ppl->flProp &= ~PROP_REGEN;
    }
    CloseHandle(ppl->hPIF);
    ppl->hPIF = INVALID_HANDLE_VALUE;

    // As long as IGNOREPIF isn't set, clear SKIPPIF, because even if we
    // already knew the PIF didn't exist on *this* call, one may be created
    // (by someone else) by the next time we're called

  Exit:
    if (!(ppl->flProp & PROP_IGNOREPIF))
        ppl->flProp &= ~PROP_SKIPPIF;
    return fSuccess;
}


/** FlushPIFData - write dirty PIF data back to PIF
 *
 * INPUT
 *  ppl -> property
 *  fDiscard == TRUE to discard dirty data, FALSE to keep it
 *
 * OUTPUT
 *  TRUE if succeeded, FALSE if not
 *
 * NOTES
 *  We must first check the PROPLINK and see if the DONTWRITE bit has
 *  been set, in which case we have to fail the flush.  Once DONTWRITE is
 *  set in a PROPLINK, it will never be cleared, unless the caller
 *  specifies fDiscard == TRUE to reload the data.  This is BY DESIGN (ie,
 *  a UI compromise).  How does DONTWRITE get set?  By someone else
 *  having previously (and successfully) done a flush to the same PIF; at
 *  that point in time, we will look for all other properties that refer to
 *  the same file, and set their DONTWRITE bit.  What about PROPLINKs that
 *  are created later?  They're ok, they don't get DONTWRITE set until
 *  the above sequence takes place during their lifetime.
 */

BOOL FlushPIFData(PPROPLINK ppl, BOOL fDiscard)
{
    UINT u;
    BOOL fSuccess = FALSE;
    FunctionName(FlushPIFData);

    // If nothing dirty, nothing to do

    if (!(ppl->flProp & PROP_DIRTY) || (ppl->flProp & PROP_INHIBITPIF))
        return TRUE;            // ie, success

    // If discarding, then clear PROP_DIRTY and reload the data

    if (fDiscard) {
        ppl->flProp &= ~(PROP_DIRTY | PROP_DONTWRITE);
        return GetPIFData(ppl, FALSE);
    }

    if (ppl->flProp & PROP_DONTWRITE)
        return fSuccess;        // ie, FALSE (error)

    if (!ppl->lpPIFData)
        return fSuccess;        // ie, FALSE (error)

    ppl->cLocks++;

    // If we created properties without opening a file, it may have
    // been because normal PIF search processing was overridden by the
    // presence of a WIN.INI entry;  if that entry is still there,
    // then our data is not in sync with any existing file, nor is there
    // any point in creating a new file as long as that entry exists.  We
    // need to consider prompting the user as to whether he really wants
    // that WIN.INI entry, so that it's clear what the heck is going on

    if (ppl->flProp & PROP_IGNOREPIF) {

        HANDLE hProps;

        ppl->ckbMem = GetProfileInt(apszAppType[APPTYPE_PIF]+1, ppl->szPathName+ppl->iFileName, -1);
        if (ppl->ckbMem != -1)
            goto Exit;

        // The WIN.INI entry apparently went away, so let's re-attempt to
        // open the properties that we should have obtained in the first
        // place.  Assuming success, we will copy our entire block on top of
        // them (thereby flushing it), and also copy their PIF name to our
        // PIF name and their PIF flags to our PIF flags, so that future
        // flushes are of the more normal variety

        hProps = PifMgr_OpenProperties(ppl->ofPIF.szPathName, NULL, 0, OPENPROPS_RAWIO);
        if (hProps) {
            ppl->flProp &= ~(PROP_IGNOREPIF | PROP_SKIPPIF);
            ppl->flProp |= ((PPROPLINK)hProps)->flProp & (PROP_IGNOREPIF | PROP_SKIPPIF);
            lstrcpy(ppl->ofPIF.szPathName, ((PPROPLINK)hProps)->ofPIF.szPathName);
            if (PifMgr_SetProperties(hProps, NULL, ppl->lpPIFData, ppl->cbPIFData, SETPROPS_RAWIO) == ppl->cbPIFData) {
                fSuccess++;
                ppl->flProp &= ~(PROP_DIRTY | PROP_TRUNCATE);
            }
            PifMgr_CloseProperties(hProps, CLOSEPROPS_NONE);
        }
        goto Exit;
    }

    // Disable annoying critical error popups (NO MORE GOTOS PAST HERE PLEASE)

    u = SetErrorMode(SEM_FAILCRITICALERRORS);

    ppl->hPIF = CreateFile( ppl->ofPIF.szPathName,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

    // If we couldn't open the file, then the presumption is that the
    // app didn't have a PIF (or did but someone but someone deleted it),
    // and so we use the name we constructed during PifMgr_OpenProperties in case
    // they ever opted to save new settings (which they obviously have done!)

    // 28-Feb-95: If the PIF did exist at one time (meaning NOCREATPIF is
    // set), then don't recreate it;  somebody's trying to delete their own
    // PIF, so let them. -JTP

    if ((ppl->hPIF != INVALID_HANDLE_VALUE) && (GetLastError()!=ERROR_FILE_EXISTS)) {

        if (!(ppl->flProp & PROP_NOCREATEPIF))
            SetFilePointer( ppl->hPIF, 0, NULL, FILE_BEGIN );

        // If the create succeeded, we're no longer using the default PIF

        if (ppl->hPIF != INVALID_HANDLE_VALUE) {

            ppl->flProp |= PROP_NOCREATEPIF;

            ppl->flProp &= ~(PROP_TRUNCATE | PROP_NOPIF | PROP_DEFAULTPIF);
        }
    }

    // If either the open or the create succeeded, write the PIF data out now

    if (ppl->hPIF != INVALID_HANDLE_VALUE) {

        PPROPLINK pplEnum;
        DWORD dwDummy;

        WriteFile( ppl->hPIF, (LPCVOID)ppl->lpPIFData,
                   ppl->cbPIFData, &dwDummy, NULL );
        if (ppl->flProp & PROP_TRUNCATE)
            WriteFile(ppl->hPIF, (LPCVOID)ppl->lpPIFData, 0, &dwDummy, NULL );
        CloseHandle(ppl->hPIF);
        ppl->hPIF = INVALID_HANDLE_VALUE;
        ppl->flProp &= ~(PROP_DIRTY | PROP_TRUNCATE);
        fSuccess++;

        // Here's where we want to check for other active PROPLINKs using the
        // same PIF.  For each one found, set its DONTWRITE bit.

        pplEnum = NULL;
        while (NULL != (pplEnum = (PPROPLINK)EnumProperties(pplEnum))) {
            if (lstrcmpi(ppl->ofPIF.szPathName, pplEnum->ofPIF.szPathName) == 0) {
                if (pplEnum != ppl)
                    pplEnum->flProp |= PROP_DONTWRITE;
            }
        }
    }

    // Re-enable annoying critical error popups

    SetErrorMode(u);

  Exit:
    ppl->cLocks--;
    return fSuccess;
}


/** AddEnhancedData - create enhanced section(s) of PIF data
 *
 * INPUT
 *  ppl -> property
 *
 * OUTPUT
 *  lpenh or NULL
 */

LPWENHPIF40 AddEnhancedData(PPROPLINK ppl, LPW386PIF30 lp386)
{
    PROPPRG prg;
    PROPTSK tsk;
    PROPVID vid;
    PROPKBD kbd;
    PROPMSE mse;
    PROPFNT fnt;
    PROPWIN win;
    PROPENV env;
    void *aDataPtrs[NUM_DATA_PTRS];
    LPWENHPIF40 lpenh = NULL;

    FunctionName(AddEnhancedData);

    // Get copies of pre-enhanced and/or default settings first,
    // and do them all *before* doing the AddGroupData, because the
    // functions' behavior will change once the enhanced section is added;

    // in addition, zero those strucs that contain strings, since lstrcpy()
    // may initialize a minimum of 1 byte, leaving garbage in the rest.

    BZero(&prg, SIZEOF(prg));
    BZero(&fnt, SIZEOF(fnt));
    BZero(&win, SIZEOF(win));
    BZero(&env, SIZEOF(env));
    BZero(aDataPtrs, SIZEOF(aDataPtrs));

    aDataPtrs[ LP386_INDEX ] = (LPVOID)lp386;
    GetPrgData(ppl, aDataPtrs, &prg, SIZEOF(prg), GETPROPS_NONE);
    GetTskData(ppl, aDataPtrs, &tsk, SIZEOF(tsk), GETPROPS_NONE);
    GetVidData(ppl, aDataPtrs, &vid, SIZEOF(vid), GETPROPS_NONE);
    GetKbdData(ppl, aDataPtrs, &kbd, SIZEOF(kbd), GETPROPS_NONE);
    GetMseData(ppl, aDataPtrs, &mse, SIZEOF(mse), GETPROPS_NONE);
    GetFntData(ppl, aDataPtrs, &fnt, SIZEOF(fnt), GETPROPS_NONE);
    GetWinData(ppl, aDataPtrs, &win, SIZEOF(win), GETPROPS_NONE);
    GetEnvData(ppl, aDataPtrs, &env, SIZEOF(env), GETPROPS_NONE);


    if (AddGroupData(ppl, szWENHHDRSIG40, NULL, SIZEOF(WENHPIF40))) {

        if (NULL != (lpenh = GetGroupData(ppl, szWENHHDRSIG40, NULL, NULL))) {

            lpenh->dwEnhModeFlagsProp = prg.dwEnhModeFlags;
            lpenh->dwRealModeFlagsProp = prg.dwRealModeFlags;
            lstrcpyA(lpenh->achOtherFileProp, prg.achOtherFile);
            lstrcpyA(lpenh->achIconFileProp, prg.achIconFile);
            lpenh->wIconIndexProp = prg.wIconIndex;
            lpenh->tskProp = tsk;
            lpenh->vidProp = vid;
            lpenh->kbdProp = kbd;
            lpenh->mseProp = mse;
            lpenh->fntProp = fnt;
            lpenh->winProp = win;
            lpenh->envProp = env;
            lpenh->wInternalRevision = WENHPIF40_VERSION;
        }
    }
    return lpenh;
}


/** AddGroupData - add NEW property group to PIF data
 *
 * INPUT
 *  ppl -> property
 *  lpszGroup -> name of new group
 *  lpGroup -> new group record (if NULL, then group data is zero-filled)
 *  cbGroup == size of new group record
 *
 * OUTPUT
 *  TRUE if successful, FALSE if not
 */

BOOL AddGroupData(PPROPLINK ppl, LPCSTR lpszGroup, LPCVOID lpGroup, int cbGroup)
{
    INT cbOld;
    LPPIFEXTHDR lpph;
    FunctionName(AddGroupData);

    if ((cbOld = ResizePIFData(ppl, cbGroup+SIZEOF(PIFEXTHDR))) != -1) {

        lpph = (LPPIFEXTHDR)LPPIF_FIELDOFF(stdpifext);

        while ((DWORD_PTR)lpph <= (DWORD_PTR)LPPIF_OFF(cbOld - SIZEOF(PIFEXTHDR)) &&
               (DWORD_PTR)lpph >= (DWORD_PTR)LPPIF_FIELDOFF(stdpifext)) {
            if (lpph->extnxthdrfloff == LASTHDRPTR) {
                lpph->extnxthdrfloff = (WORD) cbOld;
                lpph = (LPPIFEXTHDR)LPPIF_OFF(cbOld);
                lstrcpynA(lpph->extsig, lpszGroup, SIZEOF(lpph->extsig));
                lpph->extnxthdrfloff = LASTHDRPTR;
                lpph->extfileoffset = (INT)(cbOld + SIZEOF(PIFEXTHDR));
                lpph->extsizebytes = (WORD) cbGroup;
                if (lpGroup) {
                    hmemcpy((LPBYTE)LPPH_OFF(SIZEOF(PIFEXTHDR)), lpGroup, cbGroup);
                    ppl->flProp |= PROP_DIRTY;
                }
                break;
            }
            lpph = (LPPIFEXTHDR)LPPIF_OFF(lpph->extnxthdrfloff);
        }
        ppl->cLocks--;
        return TRUE;
    }
    ASSERTFAIL();
    return FALSE;
}


/** RemoveGroupData - remove EXISTING property group from PIF data
 *
 * INPUT
 *  ppl -> property
 *  lpszGroup -> name of group
 *
 * OUTPUT
 *  TRUE if successful, FALSE if not
 */

BOOL RemoveGroupData(PPROPLINK ppl, LPCSTR lpszGroup)
{
    INT cbGroup, fSuccess;
    LPBYTE lpGroup;
    WORD extnxthdrfloff;
    LPPIFEXTHDR lpph, lpphGroup;
    FunctionName(RemoveGroupData);

    ppl->cLocks++;

    fSuccess = FALSE;
    if (NULL != (lpGroup = GetGroupData(ppl, lpszGroup, &cbGroup, &lpphGroup))) {

        // Removing groups is a bit tedious, so here goes....

        // First, we will walk all the headers, attempting to find the
        // one that points to the one we're about to remove, and point it
        // to the next one, and at the same time adjust all file offsets that
        // equal or exceed the offsets of either the outgoing data or its
        // header.

        lpph = (LPPIFEXTHDR)LPPIF_FIELDOFF(stdpifext);

        while ((DWORD_PTR)lpph <= (DWORD_PTR)LPPIF_OFF(ppl->cbPIFData - SIZEOF(PIFEXTHDR)) &&
               (DWORD_PTR)lpph >= (DWORD_PTR)LPPIF_FIELDOFF(stdpifext)) {

            extnxthdrfloff = lpph->extnxthdrfloff;

            if ((DWORD_PTR)LPPH_OFF(lpph->extfileoffset) >= (DWORD_PTR)lpGroup)
                lpph->extfileoffset -= (WORD) cbGroup;

            if (lpphGroup) {
                if ((DWORD_PTR)LPPH_OFF(lpph->extfileoffset) >= (DWORD_PTR)lpphGroup)
                    lpph->extfileoffset -= SIZEOF(PIFEXTHDR);
                if ((DWORD_PTR)LPPH_OFF(lpph->extnxthdrfloff) == (DWORD_PTR)lpphGroup)
                    extnxthdrfloff = lpph->extnxthdrfloff = lpphGroup->extnxthdrfloff;
            }
            if (extnxthdrfloff == LASTHDRPTR)
                break;

            if ((DWORD_PTR)LPPH_OFF(lpph->extnxthdrfloff) >= (DWORD_PTR)lpGroup)
                lpph->extnxthdrfloff -= (WORD) cbGroup;

            if (lpphGroup)
                if ((DWORD_PTR)LPPH_OFF(lpph->extnxthdrfloff) >= (DWORD_PTR)lpphGroup)
                    lpph->extnxthdrfloff -= SIZEOF(PIFEXTHDR);

            lpph = (LPPIFEXTHDR)LPPIF_OFF(extnxthdrfloff);
        }

        // Next, move everything up over the data, then adjust lpph as
        // needed and move everything up over the header (this must be done
        // in two discrete steps, because we shouldn't assume anything
        // about the data's location relative to its header).

        hmemcpy(lpGroup, (LPBYTE)lpGroup+cbGroup,
                (DWORD_PTR)LPPIF_OFF(ppl->cbPIFData) - (DWORD_PTR)((LPBYTE)lpGroup+cbGroup));

        if (lpphGroup) {

            if ((DWORD_PTR)lpphGroup >= (DWORD_PTR)((LPBYTE)lpGroup+cbGroup))
                lpphGroup -= cbGroup;

            hmemcpy(lpphGroup, lpphGroup+1,
                    (DWORD_PTR)LPPIF_OFF(ppl->cbPIFData) - (DWORD_PTR)((LPBYTE)lpphGroup+1+cbGroup));

            cbGroup += SIZEOF(PIFEXTHDR);
        }
        ResizePIFData(ppl, -cbGroup);
        ppl->flProp |= PROP_DIRTY | PROP_TRUNCATE;
        ppl->cLocks--;
    }
    ppl->cLocks--;
    return fSuccess;
}


/** GetGroupData - get ptr to property group (by name)
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lpszGroup -> property group; may be one of the following:
 *      "WINDOWS 286 3.0"
 *      "WINDOWS 386 3.0"
 *      "WINDOWS PIF.400"
 *    or any other group name that is the name of a valid PIF extension.
 *  lpcbGroup -> where to return size of group data (NULL if not)
 *  lplpph -> where to return ptr to pif extension header, if any (NULL if not)
 *
 * OUTPUT
 *  Returns ptr to property group info, NULL if not found
 */

void *GetGroupData(PPROPLINK ppl, LPCSTR lpszGroup,
                    LPINT lpcbGroup, LPPIFEXTHDR *lplpph)
{
    BOOL fFixMe;
    LPPIFEXTHDR lpph;
    FunctionName(GetGroupData);

    if (!ppl->lpPIFData)
        return NULL;

    lpph = (LPPIFEXTHDR)LPPIF_FIELDOFF(stdpifext);

    while ((DWORD_PTR)lpph <= (DWORD_PTR)LPPIF_OFF(ppl->cbPIFData-SIZEOF(PIFEXTHDR)) &&
           (DWORD_PTR)lpph >= (DWORD_PTR)LPPIF_FIELDOFF(stdpifext)) {

        // PIFEDIT 3.x can trash the first byte of our extended portion
        // (generally with a zero), so try to recover by stuffing the first
        // character of the group we're looking for into the signature;
        // if the rest of the signature matches, great, if it doesn't, then
        // re-zero it.

        ASSERTTRUE(lpph->extsig[0]);

        if (!lpszGroup) {
            if (!(*lpcbGroup)--) {
                if (lplpph)
                    *lplpph = lpph;
                *lpcbGroup = lpph->extsizebytes;
                return lpph;
            }
        }
        else {
            if (FALSE != (fFixMe = !lpph->extsig[0]))      // attempt to fix
                lpph->extsig[0] = *lpszGroup;
            if (lstrcmpiA(lpph->extsig, lpszGroup) == 0) {
                if (lplpph)
                    *lplpph = lpph;
                if (lpcbGroup)
                    *lpcbGroup = lpph->extsizebytes;
                if (lpph->extfileoffset >= (WORD)ppl->cbPIFData) {
                    ASSERTFAIL();
                    return NULL;
                }
                return (LPBYTE)LPPIF_OFF(lpph->extfileoffset);
            }
            if (fFixMe)                         // fix failed (this time anyway)
                lpph->extsig[0] = 0;
        }
        if (lpph->extnxthdrfloff == LASTHDRPTR)
            break;
        lpph = (LPPIFEXTHDR)LPPIF_OFF(lpph->extnxthdrfloff);
    }

    // If we didn't get anywhere, check if this is a "really old" PIF;
    // ie, one without any headers;  if so, then if all they were asking for
    // was the old stuff, return it

    if (ppl->cbPIFData == SIZEOF(STDPIF) && lpszGroup) {
        if (lstrcmpiA(szSTDHDRSIG, lpszGroup) == 0) {
            if (lplpph)
                *lplpph = NULL;
            if (lpcbGroup)
                *lpcbGroup = SIZEOF(STDPIF);
            return ppl->lpPIFData;
        }
    }
    return NULL;
}

/** AppWizard - call the AppWizard CPL (appwiz.cpl)
 */



TCHAR c_szAPPWIZ[]    = TEXT("appwiz.cpl");
CHAR  c_szAppWizard[] = "AppWizard";

typedef DWORD (WINAPI *LPAPPWIZARD)(HWND hwnd, HANDLE i, UINT ui);

UINT WINAPI AppWizard(HWND hwnd, HANDLE hProps, UINT action)
{
    DWORD err = 42;
    LPAPPWIZARD XAppWizard;
    HINSTANCE hAppWizard;

    hAppWizard = LoadLibrary(c_szAPPWIZ);

    if (hAppWizard)
    {
        if (NULL != (XAppWizard = (LPAPPWIZARD)GetProcAddress(hAppWizard, c_szAppWizard)))
        {
            err = XAppWizard( hwnd, hProps, action );
        }
        FreeLibrary((HINSTANCE)hAppWizard);
    }

    return (UINT)err;
}
