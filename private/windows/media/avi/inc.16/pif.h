/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1991-1994
 *  All Rights Reserved.
 *
 *
 *  PIF.H
 *  DOS Program Information File structures, constants, etc.
 */


#ifndef _INC_PIF
#define _INC_PIF

#define PIFNAMESIZE     30
#define PIFSTARTLOCSIZE 63
#define PIFDEFPATHSIZE  64
#define PIFPARAMSSIZE   64
#define PIFSHPROGSIZE   64
#define PIFSHDATASIZE   64
#define PIFDEFFILESIZE  80

#ifndef LF_FACESIZE
#define LF_FACESIZE     32
#endif

#define LARGEST_GROUP   sizeof(STDPIF)

#define OPENPROPS_NONE          0x0000
#define OPENPROPS_RAWIO         0x0001

#define GETPROPS_NONE           0x0000
#define GETPROPS_RAWIO          0x0001
#define GETPROPS_EXTENDED       0x0004

#define SETPROPS_NONE           0x0000
#define SETPROPS_RAWIO          0x0001
#define SETPROPS_CACHE          0x0002
#define SETPROPS_EXTENDED       0x0004

#define FLUSHPROPS_NONE         0x0000
#define FLUSHPROPS_DISCARD      0x0001

#define CLOSEPROPS_NONE         0x0000
#define CLOSEPROPS_DISCARD      0x0001

#define LOADPROPLIB_DEFER       0x0001


/* XLATOFF */
#ifndef FAR
#define FAR
#endif
/* XLATON */

#ifdef  RECT
#define _INC_WINDOWS
#endif

#ifndef _INC_WINDOWS

/* ASM
RECT    struc
        rcLeft      dw  ?
        rcTop       dw  ?
        rcRight     dw  ?
        rcBottom    dw  ?
RECT    ends
*/

/* XLATOFF */
typedef struct tagRECT {
    int left;
    int top;
    int right;
    int bottom;
} RECT;
typedef RECT *PRECT;
typedef RECT FAR *LPRECT;
/* XLATON */

#endif


/*
 *  Property groups, used by PIFMGR.DLL and VxD interfaces
 *
 *  The structures for each of the pre-defined, ordinal-based groups
 *  is a logical view of data in the associated PIF file, if any -- not a
 *  physical view.
 */

#define GROUP_PRG               1           // program group

#define PRG_DEFAULT		(PRG_CLOSEONEXIT | PRG_AUTOWINEXEC)
#define PRG_CLOSEONEXIT         0x0001      // MSflags & EXITMASK
//#define PRG_RESERVED          0x0002      // Reserved
#define PRG_AUTOWINEXEC 	0x0004	    // !(PfW386Flags & fDisAutoWinExec)

#define PRGINIT_DEFAULT         0
#define PRGINIT_MINIMIZED       0x0001      // PfW386Flags & fMinimized  (NEW)
#define PRGINIT_MAXIMIZED       0x0002      // PfW386Flags & fMaximized  (NEW)
#define PRGINIT_USEPIFICON      0x0004      // PfW386Flags & fUsePIFIcon (NEW)
#define PRGINIT_REALMODE        0x0008      // PfW386Flags & fRealMode   (NEW)
#define PRGINIT_NOPIF           0x1000      // (NEW -- informational only)
#define PRGINIT_DEFAULTPIF      0x2000      // (NEW -- informational only)

#define ICONFILE_DEFAULT        "PIFMGR.DLL"
#define ICONINDEX_DEFAULT       0

typedef struct PROPPRG {                    /* prg */
    WORD    flPrg;                          // see PRG_ flags
    WORD    flPrgInit;                      // see PRGINIT_ flags
    char    achTitle[PIFNAMESIZE];          // name[30]
    char    achCmdLine[PIFSTARTLOCSIZE];    // startfile[63] + params[64]
    char    achWorkDir[PIFDEFPATHSIZE];     // defpath[64]
    WORD    wHotKey;			    // PfHotKeyScan thru PfHotKeyVal
    BYTE    rgbReserved[6];		    // (Reserved)
    char    achIconFile[PIFDEFFILESIZE];    // (NEW)
    WORD    wIconIndex;                     // (NEW)
    char    achPIFFile[PIFDEFFILESIZE];     // (NEW)
} PROPPRG;
typedef PROPPRG *PPROPPRG;
typedef PROPPRG FAR *LPPROPPRG;
typedef const PROPPRG FAR *LPCPROPPRG;


#define GROUP_TSK               2           // multi-tasking group

#define TSK_DEFAULT             (TSK_BACKGROUND)
#define TSK_ALLOWCLOSE          0x0001      // PfW386Flags & fEnableClose (SAME BIT)
#define TSK_BACKGROUND          0x0002      // PfW386Flags & fBackground  (SAME BIT)
#define TSK_EXCLUSIVE           0x0004      // PfW386Flags & fExclusive   (SAME BIT)
#define TSK_FAKEBOOST           0x0008      // (NEW -- informational only)
#define TSK_NOWARNTERMINATE	0x0010	    // Don't warn before terminating (NEW)
#define TSK_NOSCREENSAVER	0x0020	    // Do not activate screen saver (NEW)

#define TSKINIT_DEFAULT         0

#define TSKFGND_DEFAULT         75          // normal fgnd % (NEW)
#define TSKBGND_DEFAULT         25          // normal bgnd % (NEW)

#define TSKFGND_OLD_DEFAULT     100         // normal fgnd setting
#define TSKBGND_OLD_DEFAULT     50          // normal bgnd setting

#define TSKBOOSTTIME_MIN        0           // in # milliseconds
#define TSKBOOSTTIME_DEFAULT    1           // in # milliseconds
#define TSKBOOSTTIME_MAX        5000        // in # milliseconds
#define TSKIDLEDELAY_MIN        0           // in # milliseconds
#define TSKIDLEDELAY_DEFAULT    500         // in # milliseconds
#define TSKIDLEDELAY_MAX        5000        // in # milliseconds
#define TSKIDLESENS_DEFAULT     50          // % (min-max == 0-100)

typedef struct PROPTSK {                    /* tsk */
    WORD    flTsk;                          // see TSK_ flags
    WORD    flTskInit;                      // see TSKINIT_ flags
    short   iFgndBoost;                     // PfFPriority (NEW, converted to boost)
    short   iBgndBoost;                     // PfBPriority (NEW, converted to boost)
    WORD    msKeyBoostTime;                 // ([386Enh]:KeyBoostTime)
    WORD    msKeyIdleDelay;                 // ([386Enh]:KeyIdleDelay)
    WORD    wIdleSensitivity;               // PfW386Flags & fPollingDetect (NEW, %)
} PROPTSK;
typedef PROPTSK *PPROPTSK;
typedef PROPTSK FAR *LPPROPTSK;


#define GROUP_VID               3           // video group

#define VID_DEFAULT             (VID_TEXTEMULATE | VID_FULLSCREENGRFX | VID_AUTOSUSPEND)
#define VID_TEXTEMULATE         0x0001      // PfW386Flags2 & fVidTxtEmulate  (SAME BIT)
#define VID_TEXTTRAP            0x0002      // PfW386Flags2 & fVidNoTrpTxt    (INVERTED BIT)
#define VID_LOGRFXTRAP          0x0004      // PfW386Flags2 & fVidNoTrpLRGrfx (INVERTED BIT)
#define VID_HIGRFXTRAP          0x0008      // PfW386Flags2 & fVidNoTrpHRGrfx (INVERTED BIT)
#define VID_RETAINMEMORY        0x0080      // PfW386Flags2 & fVidRetainAllo  (SAME BIT)
#define VID_FULLSCREEN          0x0100      // PfW386Flags  & fFullScreen
#define VID_FULLSCREENGRFX      0x0200      // PfW386Flags2 & fFullScreenGrfx (NEW)

#define VIDINIT_DEFAULT         0

#define VIDMODE_DEFAULT         VIDMODE_TEXT
#define VIDMODE_TEXT            1           // PfW386Flags2 & fVidTextMd
#define VIDMODE_LOGRFX          2           // PfW386Flags2 & fVidLowRsGrfxMd
#define VIDMODE_HIGRFX          3           // PfW386Flags2 & fVidHghRsGrfxMd

#define VIDSCROLLFREQ_MIN       1           // in # lines
#define VIDSCROLLFREQ_DEFAULT   2           // in # lines
#define VIDSCROLLFREQ_MAX       25          // in # lines

#define VIDUPDATEFREQ_MIN       10          // in # milliseconds
#define VIDUPDATEFREQ_DEFAULT   50          // in # milliseconds
#define VIDUPDATEFREQ_MAX       5000        // in # milliseconds

#define VIDSCREENLINES_MIN      0           // in # lines (0 = use VDD value)
#define VIDSCREENLINES_DEFAULT  0           // in # lines

typedef struct PROPVID {                    /* vid */
    WORD    flVid;                          // see VID_ flags
    WORD    flVidInit;                      // see VIDINIT_ flags
    WORD    iVidMode;                       // see VIDMODE_ ordinals
    WORD    cScrollFreq;                    // ([386Enh]:ScrollFrequency)
    WORD    msUpdateFreq;                   // ([386Enh]:WindowUpdateTime)
    WORD    cScreenLines;                   // ([NonWindowsApp]:ScreenLines)
    BYTE    abTextColorRemap[16];           // (NEW)
} PROPVID;
typedef PROPVID *PPROPVID;
typedef PROPVID FAR *LPPROPVID;


#define GROUP_MEM               4           // memory group

#define MEM_DEFAULT             0

#define MEMINIT_DEFAULT         0
#define MEMINIT_NOHMA           0x0001      // PfW386Flags & fNoHMA
#define MEMINIT_LOWLOCKED       0x0002      // PfW386Flags & fVMLocked
#define MEMINIT_EMSLOCKED       0x0004      // PfW386Flags & fEMSLocked
#define MEMINIT_XMSLOCKED       0x0008      // PfW386Flags & fXMSLocked
#define MEMINIT_GLOBALPROTECT   0x0010      // PfW386Flags & fGlobalProtect (NEW)
#define MEMINIT_STRAYPTRDETECT  0x0020      // PfW386Flags & fStrayPtrDetect(NEW)

#define MEMLOW_MIN_MIN          0xFFFF      // in KB
#define MEMLOW_MIN_DEFAULT      0           // in KB
#define MEMLOW_MIN_MAX          640         // in KB

#define MEMLOW_MAX_MIN          0xFFFF      // in KB
#define MEMLOW_MAX_DEFAULT      0xFFFF      // in KB
#define MEMLOW_MAX_MAX          640         // in KB

#define MEMEMS_MIN_MIN          0           // in KB
#define MEMEMS_MIN_DEFAULT      0           // in KB
#define MEMEMS_MIN_MAX          16384       // in KB

#define MEMEMS_MAX_MIN          0xFFFF      // in KB (-1 means "no limit")
#define MEMEMS_MAX_DEFAULT      1024        // in KB
#define MEMEMS_MAX_MAX          16384       // in KB

#define MEMXMS_MIN_MIN          0           // in KB
#define MEMXMS_MIN_DEFAULT      0           // in KB
#define MEMXMS_MIN_MAX          16384       // in KB

#define MEMXMS_MAX_MIN          0xFFFF      // in KB (-1 means "no limit")
#define MEMXMS_MAX_DEFAULT      1024        // in KB
#define MEMXMS_MAX_MAX          16384       // in KB

typedef struct PROPMEM {                    /* mem */
    WORD    flMem;                          // see MEM_ flags
    WORD    flMemInit;                      // see MEMINIT_ flags
    WORD    wMinLow;                        // PfW386minmem
    WORD    wMaxLow;                        // PfW386maxmem
    WORD    wMinEMS;                        // PfMinEMMK
    WORD    wMaxEMS;                        // PfMaxEMMK
    WORD    wMinXMS;                        // PfMinXmsK
    WORD    wMaxXMS;                        // PfMaxXmsK
} PROPMEM;
typedef PROPMEM *PPROPMEM;
typedef PROPMEM FAR *LPPROPMEM;


#define GROUP_KBD               5           // keyboard group

#define KBD_DEFAULT             (KBD_FASTPASTE)
#define KBD_FASTPASTE           0x0001      // PfW386Flags & fINT16Paste
#define KBD_NOALTTAB            0x0020      // PfW386Flags & fALTTABdis   (SAME BIT)
#define KBD_NOALTESC            0x0040      // PfW386Flags & fALTESCdis   (SAME BIT)
#define KBD_NOALTSPACE          0x0080      // PfW386Flags & fALTSPACEdis (SAME BIT)
#define KBD_NOALTENTER          0x0100      // PfW386Flags & fALTENTERdis (SAME BIT)
#define KBD_NOALTPRTSC          0x0200      // PfW386Flags & fALTPRTSCdis (SAME BIT)
#define KBD_NOPRTSC             0x0400      // PfW386Flags & fPRTSCdis    (SAME BIT)
#define KBD_NOCTRLESC           0x0800      // PfW386Flags & fCTRLESCdis  (SAME BIT)

#define KBDINIT_DEFAULT         0

#define KBDALTDELAY_MIN             1
#define KBDALTDELAY_DEFAULT         5
#define KBDALTDELAY_MAX             5000

#define KBDALTPASTEDELAY_MIN        1
#define KBDALTPASTEDELAY_DEFAULT    25
#define KBDALTPASTEDELAY_MAX        5000

#define KBDPASTEDELAY_MIN           1
#define KBDPASTEDELAY_DEFAULT       3
#define KBDPASTEDELAY_MAX           5000

#define KBDPASTEFULLDELAY_MIN       1
#define KBDPASTEFULLDELAY_DEFAULT   200
#define KBDPASTEFULLDELAY_MAX       5000

#define KBDPASTETIMEOUT_MIN         1
#define KBDPASTETIMEOUT_DEFAULT     1000
#define KBDPASTETIMEOUT_MAX         5000

#define KBDPASTESKIP_MIN            1
#define KBDPASTESKIP_DEFAULT        2
#define KBDPASTESKIP_MAX            100

#define KBDPASTECRSKIP_MIN          1
#define KBDPASTECRSKIP_DEFAULT      10
#define KBDPASTECRSKIP_MAX          100

typedef struct PROPKBD {                    /* kbd */
    WORD    flKbd;                          // see KBD_ flags
    WORD    flKbdInit;                      // see KBDINIT_ flags
    WORD    msAltDelay;                     // ([386Enh]:AltKeyDelay)
    WORD    msAltPasteDelay;                // ([386Enh]:AltPasteDelay)
    WORD    msPasteDelay;                   // ([386Enh]:KeyPasteDelay)
    WORD    msPasteFullDelay;               // ([386Enh]:KeyBufferDelay)
    WORD    msPasteTimeout;                 // ([386Enh]:KeyPasteTimeOut)
    WORD    cPasteSkip;                     // ([386Enh]:KeyPasteSkipCount)
    WORD    cPasteCRSkip;                   // ([386Enh]:KeyPasteCRSkipCount)
} PROPKBD;
typedef PROPKBD *PPROPKBD;
typedef PROPKBD FAR *LPPROPKBD;


#define GROUP_MSE               6           // mouse group

/* No VxD currently pays attention to PROPMSE. VMDOSAPP should know how to
 * handle all cases resulting from a change in these flags.
 *
 * Note that MSE_WINDOWENABLE corresponds to the Windows NT "QuickEdit"
 * property, except backwards.
 */

#define MSE_DEFAULT             (MSE_WINDOWENABLE)
#define MSE_WINDOWENABLE        0x0001      // ([NonWindowsApp]:MouseInDosBox)
#define MSE_EXCLUSIVE		0x0002	    // (NEW)

#define MSEINIT_DEFAULT         0           // default flags

typedef struct PROPMSE {                    /* mse */
    WORD    flMse;                          // see MSE_ flags
    WORD    flMseInit;                      // see MSEINIT_ flags
} PROPMSE;
typedef PROPMSE *PPROPMSE;
typedef PROPMSE FAR *LPPROPMSE;


#define GROUP_TMR               7           // timer group

#define TMR_DEFAULT             0
#define TMR_TRAPTMRPORTS        0x0001      // (NEW)
#define TMR_FULLBGNDTICKS       0x0002      // (NEW)
#define TMR_BURSTMODE           0x0004      // (NEW)
#define TMR_PATCHEOI            0x0008      // (NEW)

#define TMRINIT_DEFAULT         0

#define TMRBURSTDELAY_MIN       0
#define TMRBURSTDELAY_DEFAULT   0
#define TMRBURSTDELAY_MAX       100

typedef struct PROPTMR {                    /* tmr */
    WORD    flTmr;                          // see TMR_ flags
    WORD    flTmrInit;                      // see TMRINIT_ flags
    WORD    wBurstDelay;
} PROPTMR;
typedef PROPTMR *PPROPTMR;
typedef PROPTMR FAR *LPPROPTMR;

// Extended TIMER data

typedef struct PROPTMREXT {                 /* tmrext */
    PROPTMR tmrData;                        //
    WORD    msIntFreq;                      // current interrupt frequency (in ms.)
    WORD    wExecPercent;                   // current *actual* execution percentage
} PROPTMREXT;
typedef PROPTMREXT *PPROPTMREXT;
typedef PROPTMREXT FAR *LPPROPTMREXT;


#define GROUP_FNT               11          // font group

#define FNT_DEFAULT             (FNT_BOTHFONTS)
#define FNT_RASTERFONTS         0x0004      // allow raster fonts in dialog
#define FNT_TTFONTS             0x0008      // allow truetype fonts in dialog
#define FNT_BOTHFONTS           (FNT_RASTERFONTS | FNT_TTFONTS)
#define FNT_AUTOSIZE            0x0010      // enable auto-sizing
#define FNT_RASTER              0x0400      // specified font is raster
#define FNT_TT                  0x0800      // specified font is truetype

#define FNT_FONTMASK            (FNT_BOTHFONTS)
#define FNT_FONTMASKBITS        2           // # of bits shifted left

#define FNTINIT_DEFAULT         0
#define FNTINIT_NORESTORE       0x0001      // don't restore on restart

typedef struct PROPFNT {                    /* fnt */
    WORD    flFnt;                          // see FNT_ flags
    WORD    flFntInit;                      // see FNTINIT_ flags
    WORD    cxFont;                         // width of desired font
    WORD    cyFont;                         // height of desired font
    WORD    cxFontActual;                   // actual width of desired font
    WORD    cyFontActual;                   // actual height of desired font
    char    achRasterFaceName[LF_FACESIZE]; // name to use for raster font
    char    achTTFaceName[LF_FACESIZE];     // name to use for tt font
#ifdef JAPAN //CodePage Support
    WORD    wCharSet;                       // Character Set
#endif
} PROPFNT;
typedef PROPFNT *PPROPFNT;
typedef PROPFNT FAR *LPPROPFNT;

#define GROUP_WIN               12          // window group

#define WIN_DEFAULT             (WIN_SAVESETTINGS)
#define WIN_SAVESETTINGS        0x0001      // save settings on exit (default)
#define WIN_TOOLBAR             0x0002      // enable toolbar

#define WININIT_DEFAULT         0
#define WININIT_NORESTORE       0x0001      // don't restore on restart

typedef struct PROPWIN {                    /* win */
    WORD    flWin;                          // see WIN_ flags
    WORD    flWinInit;                      // see WININIT flags
    WORD    cxCells;                        // width in cells
    WORD    cyCells;                        // height in cells
    WORD    cxClient;                       // width of client window
    WORD    cyClient;                       // height of client window
    WORD    cxWindow;                       // width of entire window
    WORD    cyWindow;                       // height of entire window
#ifdef WPF_SETMINPOSITION                   // if windows.h is included
    WINDOWPLACEMENT wp;                     // then use WINDOWPLACEMENT type
#else                                       // else define equivalent structure
    WORD    wLength;
    WORD    wShowFlags;
    WORD    wShowCmd;
    WORD    xMinimize;
    WORD    yMinimize;
    WORD    xMaximize;
    WORD    yMaximize;
    RECT    rcNormal;
#endif
} PROPWIN;
typedef PROPWIN *PPROPWIN;
typedef PROPWIN FAR *LPPROPWIN;

#define GROUP_ENV               13          // environment/startup group

#define ENV_DEFAULT             0

#define ENVINIT_DEFAULT         (ENVINIT_INSTRUCTIONS)
#define ENVINIT_INSTRUCTIONS    0x0001      // ([386Enh]:DOSPromptExitInstruc)

#define ENVSIZE_MIN             0
#define ENVSIZE_DEFAULT         0
#define ENVSIZE_MAX             4096

typedef struct PROPENV {                    /* env */
    WORD    flEnv;                          // see ENV_ flags
    WORD    flEnvInit;                      // see ENVINIT_ flags
    char    achBatchFile[PIFDEFFILESIZE];   // (NEW)
    WORD    cbEnvironment;                  // ([386Enh]:CommandEnvSize)
#ifdef JAPAN //CodePage Support
    WORD    wCodePage;                      // Current CodePage
    WORD    wInitCodePage;                  // Initial CodePage
#endif
} PROPENV;
typedef PROPENV *PPROPENV;
typedef PROPENV FAR *LPPROPENV;


#define MAX_GROUP               0x0FF
#define MAX_VALID_GROUP         GROUP_ENV

/*
 * Additional group ordinal bits that can be passed to VxD property hooks
 */
#define EXT_GROUP_QUERY         0x100
#define EXT_GROUP_UPDATE        0x200


/*
 *  PIF "file" structures, used by .PIFs
 */

#define PIFEXTSIGSIZE   16                  // Length of extension signatures
#define MAX_GROUP_NAME  PIFEXTSIGSIZE       //
#define STDHDRSIG       "MICROSOFT PIFEX"   // extsig value for stdpifext
#define LASTHDRPTR      0xFFFF              // This value in the
                                            //  extnxthdrfloff field indicates
                                            //   there are no more extensions.
#define W286HDRSIG30    "WINDOWS 286 3.0"
#define W386HDRSIG30    "WINDOWS 386 3.0"
#define WENHHDRSIG40    "WINDOWS PIF.402"   // (NEW)
#define WENHICOSIG001   "WINDOWS ICO.001"   // (NEW)


typedef struct PIFEXTHDR {                  /* peh */
    char    extsig[PIFEXTSIGSIZE];
    WORD    extnxthdrfloff;
    WORD    extfileoffset;
    WORD    extsizebytes;
} PIFEXTHDR;
typedef PIFEXTHDR *PPIFEXTHDR;
typedef PIFEXTHDR FAR *LPPIFEXTHDR;


/* Flags for MSflags
 */

#define fResident       0x01    // Directly Modifies: Memory
#define fGraphics       0x02    // Screen Exchange: Graphics/Text
#define fNoSwitch       0x04    // Program Switch: Prevent
#define fNoGrab         0x08    // Screen Exchange: None
#define fDestroy        0x10    // Close Window on exit
#define fCOM2           0x40    // Directly Modifies: COM2
#define fCOM1           0x80    // Directly Modifies: COM1

#define MEMMASK         fResident
#define GRAPHMASK       fGraphics
#define TEXTMASK        ((BYTE)(~GRAPHMASK))
#define PSMASK          fNoSwitch
#define SGMASK          fNoGrab
#define EXITMASK        fDestroy
#define COM2MASK        fCOM2
#define COM1MASK        fCOM1

/* Flags for behavior
 */
#define fScreen         0x80    // Directly Modifies: Screen
#define fForeground     0x40    // Set same as fScreen (alias)
#define f8087           0x20    // No PIFEDIT control
#define fKeyboard       0x10    // Directly Modifies: Keyboard

#define SCRMASK         (fScreen + fForeground)
#define MASK8087        f8087
#define KEYMASK         fKeyboard

/* Flags for sysflags
 */

#define SWAPMASK        0x20
#define PARMMASK        0x40

/*
 * All strings in the STDPIF are in the OEM character set.
 */
typedef struct STDPIF {                     /* std */ //Examples
    BYTE    unknown;                        // 0x00     0x00
    BYTE    id;                             // 0x01     0x78
    char    appname[PIFNAMESIZE];           // 0x02     'MS-DOS Prompt'
    WORD    maxmem;                         // 0x20     0x0200 (512Kb)
    WORD    minmem;                         // 0x22     0x0080 (128Kb)
    char    startfile[PIFSTARTLOCSIZE];     // 0x24     "COMMAND.COM"
    BYTE    MSflags;                        // 0x63     0x10
    BYTE    reserved;                       // 0x64     0x00
    char    defpath[PIFDEFPATHSIZE];        // 0x65     "\"
    char    params[PIFPARAMSSIZE];          // 0xA5     ""
    BYTE    screen;                         // 0xE5     0x00
    BYTE    cPages;                         // 0xE6     0x01 (ALWAYS!)
    BYTE    lowVector;                      // 0xE7     0x00 (ALWAYS!)
    BYTE    highVector;                     // 0xE8     0xFF (ALWAYS!)
    BYTE    rows;                           // 0xE9     0x19 (Not used)
    BYTE    cols;                           // 0xEA     0x50 (Not used)
    BYTE    rowoff;                         // 0xEB     0x00 (Not used)
    BYTE    coloff;                         // 0xEC     0x00 (Not used)
    WORD    sysmem;                         // 0xED   0x0007 (Not used; 7=>Text, 23=>Grfx/Mult Text)
    char    shprog[PIFSHPROGSIZE];          // 0xEF     0's  (Not used)
    char    shdata[PIFSHDATASIZE];          // 0x12F    0's  (Not used)
    BYTE    behavior;                       // 0x16F    0x00
    BYTE    sysflags;                       // 0x170    0x00 (Not used)
} STDPIF;
typedef STDPIF *PSTDPIF;
typedef STDPIF FAR *LPSTDPIF;


/* Flags for PfW286Flags
 */

#define fALTTABdis286   0x0001
#define fALTESCdis286   0x0002
#define fALTPRTSCdis286 0x0004
#define fPRTSCdis286    0x0008
#define fCTRLESCdis286  0x0010
#define fNoSaveVid286   0x0020              // New for 3.10
#define fCOM3_286       0x4000
#define fCOM4_286       0x8000

typedef struct W286PIF30 {                  /* 286 */ //Examples
    WORD    PfMaxXmsK;                      // 0x19D    0x0000
    WORD    PfMinXmsK;                      // 0x19F    0x0000
    WORD    PfW286Flags;                    // 0x1A1    0x0000
} W286PIF30;
typedef W286PIF30 *PW286PIF30;
typedef W286PIF30 FAR *LPW286PIF30;


/* Flags for PfW386Flags
 */

#define fEnableClose    0x00000001          //
#define fEnableCloseBit             0       //
#define fBackground     0x00000002          //
#define fBackgroundBit              1       //
#define fExclusive      0x00000004          //
#define fExclusiveBit               2       //
#define fFullScreen     0x00000008          //
#define fFullScreenBit              3       //
#define fALTTABdis      0x00000020          //
#define fALTTABdisBit               5       //
#define fALTESCdis      0x00000040          //
#define fALTESCdisBit               6       //
#define fALTSPACEdis    0x00000080          //
#define fALTSPACEdisBit             7       //
#define fALTENTERdis    0x00000100          //
#define fALTENTERdisBit             8       //
#define fALTPRTSCdis    0x00000200          //
#define fALTPRTSCdisBit             9       //
#define fPRTSCdis       0x00000400          //
#define fPRTSCdisBit                10      //
#define fCTRLESCdis     0x00000800          //
#define fCTRLESCdisBit              11      //
#define fPollingDetect  0x00001000          //
#define fPollingDetectBit           12      //
#define fNoHMA          0x00002000          //
#define fNoHMABit                   13      //
#define fHasHotKey      0x00004000          //
#define fHasHotKeyBit               14      //
#define fEMSLocked      0x00008000          //
#define fEMSLockedBit               15      //
#define fXMSLocked      0x00010000          //
#define fXMSLockedBit               16      //
#define fINT16Paste     0x00020000          //
#define fINT16PasteBit              17      //
#define fVMLocked       0x00040000          //
#define fVMLockedBit                18      //
#define fGlobalProtect  0x00080000          //  New for 4.00
#define fGlobalProtectBit           19      //  New for 4.00
#define fMinimized      0x00100000          //  New for 4.00
#define fMinimizedBit               20      //  New for 4.00
#define fMaximized      0x00200000          //  New for 4.00
#define fMaximizedBit               21      //  New for 4.00
#define fRealMode       0x00800000          //  New for 4.00
#define fRealModeBit                23      //  New for 4.00
#define fDisAutoWinExec 0x01000000          //  New for 4.00
#define fDisAutoWinExecBit	    24	    //	New for 4.00
#define fStrayPtrDetect 0x02000000          //  New for 4.00
#define fStrayPtrDetectBit          25      //  New for 4.00

/* Flags for PfW386Flags2
 *
 *  NOTE THAT THE LOW 16 BITS OF THIS DWORD ARE VDD RELATED
 *  NOTE THAT ALL OF THE LOW 16 BITS ARE RESERVED FOR VIDEO BITS
 *
 *  You cannot monkey with these bits locations without breaking
 *  all VDDs as well as all old PIFs. SO DON'T MESS WITH THEM.
 */

#define fVDDMask        0x0000FFFF          //
#define fVDDMinBit                  0       //
#define fVDDMaxBit                  15      //

#define fVidTxtEmulate  0x00000001          //
#define fVidTxtEmulateBit           0       //
#define fVidNoTrpTxt    0x00000002          //
#define fVidNoTrpTxtBit             1       //
#define fVidNoTrpLRGrfx 0x00000004          //
#define fVidNoTrpLRGrfxBit          2       //
#define fVidNoTrpHRGrfx 0x00000008          //
#define fVidNoTrpHRGrfxBit          3       //
#define fVidTextMd      0x00000010          //
#define fVidTextMdBit               4       //
#define fVidLowRsGrfxMd 0x00000020          //
#define fVidLowRsGrfxMdBit          5       //
#define fVidHghRsGrfxMd 0x00000040          //
#define fVidHghRsGrfxMdBit          6       //
#define fVidRetainAllo  0x00000080          //
#define fVidRetainAlloBit           7       //

/*
 * This mask is used to isolate status bits shared with VM_Descriptor
 */

#define PifDescMask \
(fALTTABdis   + fALTESCdis    + fALTSPACEdis + \
 fALTENTERdis + fALTPRTSCdis  + fPRTSCdis + \
 fCTRLESCdis  + fPollingDetect+ fNoHMA + \
 fHasHotKey   + fEMSLocked    + fXMSLocked + \
 fINT16Paste  + fVMLocked)


typedef struct W386PIF30 {                  /* 386 */ //Examples
    // These new maxmem/minmem fields allow values
    // that will not conflict with the 286-specific values
    WORD    PfW386maxmem;                   // 0x1B9    0xFFFF (-1)
    WORD    PfW386minmem;                   // 0x1BB    0xFFFF (-1)
    WORD    PfFPriority;                    // 0x1BD    0x0064 (100)
    WORD    PfBPriority;                    // 0x1BF    0x0032 (50)
    WORD    PfMaxEMMK;                      // 0x1C1    0x0000 (0)
    WORD    PfMinEMMK;                      // 0x1C3    0x0000 (0)
    WORD    PfMaxXmsK;                      // 0x1C5    0x0800 (2048)
    WORD    PfMinXmsK;                      // 0x1C7    0x0000 (0)
    DWORD   PfW386Flags;                    // 0x1C9    0x00021003
    DWORD   PfW386Flags2;                   // 0x1CD    0x0000001F
    WORD    PfHotKeyScan;                   // 0x1D1    Scan code in lower byte
    WORD    PfHotKeyShVal;                  // 0x1D3    Shift state
    WORD    PfHotKeyShMsk;                  // 0x1D5    Mask for shift states interested in
    BYTE    PfHotKeyVal;                    // 0x1D7    Enhanced flags
    BYTE    PfHotKeyPad[9];                 // 0x1D8    Pad Hot key section to 16 bytes
    char    PfW386params[PIFPARAMSSIZE];    // 0x1E1
} W386PIF30;
typedef W386PIF30 *PW386PIF30;
typedef W386PIF30 FAR *LPW386PIF30;


/* AssociateProperties associations
 */

#define HVM_ASSOCIATION         1
#define HWND_ASSOCIATION        2


/* SHEETTYPEs for AddPropertySheet/EnumPropertySheets
 */

#define SHEETTYPE_SIMPLE    0
#define SHEETTYPE_ADVANCED  1


/*  External function ordinals and prototypes
 */

#define ORD_OPENPROPERTIES      2
#define ORD_GETPROPERTIES       3
#define ORD_SETPROPERTIES       4
#define ORD_EDITPROPERTIES      5
#define ORD_FLUSHPROPERTIES     6
#define ORD_ENUMPROPERTIES      7
#define ORD_ASSOCIATEPROPERTIES 8
#define ORD_CLOSEPROPERTIES     9
#define ORD_LOADPROPERTYLIB     10
#define ORD_ENUMPROPERTYLIBS    11
#define ORD_FREEPROPERTYLIB     12
#define ORD_ADDPROPERTYSHEET    13
#define ORD_REMOVEPROPERTYSHEET 14
#define ORD_LOADPROPERTYSHEETS  15
#define ORD_ENUMPROPERTYSHEETS  16
#define ORD_FREEPROPERTYSHEETS  17


/* XLATOFF */
#ifdef WINAPI

int  WINAPI OpenProperties(LPCSTR lpszApp, int hInf, int flOpt);
int  WINAPI GetProperties(int hProps, LPCSTR lpszGroup, LPVOID lpProps, int cbProps, int flOpt);
int  WINAPI SetProperties(int hProps, LPCSTR lpszGroup, CONST VOID FAR *lpProps, int cbProps, int flOpt);
int  WINAPI EditProperties(int hProps, LPCSTR lpszTitle, UINT uStartPage, HWND hwnd, UINT uMsgPost);
int  WINAPI FlushProperties(int hProps, int flOpt);
int  WINAPI EnumProperties(int hProps);
long WINAPI AssociateProperties(int hProps, int iAssociate, long lData);
int  WINAPI CloseProperties(int hProps, int flOpt);

#ifdef  PIF_PROPERTY_SHEETS
int  WINAPI LoadPropertyLib(LPCSTR lpszDLL, int fLoad);
int  WINAPI EnumPropertyLibs(int iLib, LPHANDLE lphDLL, LPSTR lpszDLL, int cbszDLL);
BOOL WINAPI FreePropertyLib(int hLib);
int  WINAPI AddPropertySheet(const PROPSHEETPAGE FAR *lppsi, int iType);
BOOL WINAPI RemovePropertySheet(int hSheet);
int  WINAPI LoadPropertySheets(int hProps, int flags);
int  WINAPI EnumPropertySheets(int hProps, int iType, int iSheet, LPPROPSHEETPAGE lppsi);
int  WINAPI FreePropertySheets(int hProps, int flags);
#endif

#endif
/* XLATON */

#endif // _INC_PIF
