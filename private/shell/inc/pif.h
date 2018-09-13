/*
 *  Microsoft  Confidential
 *  Copyright (C) Microsoft Corporation 1991-1995
 *  All Rights Reserved.
 *
 *
 *  PIF.H
 *  DOS Program Information File structures, constants, etc.
 */


#ifndef _INC_PIF
#define _INC_PIF

/* XLATOFF */
#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */
/* XLATON */

#define PIFNAMESIZE     30
#define PIFSTARTLOCSIZE 63
#define PIFDEFPATHSIZE  64
#define PIFPARAMSSIZE   64
#define PIFSHPROGSIZE   64
#define PIFSHDATASIZE   64
#define PIFDEFFILESIZE  80
#define PIFMAXFILEPATH  260

#ifndef LF_FACESIZE
#define LF_FACESIZE     32
#endif

#define LARGEST_GROUP   sizeof(PROPPRG)

#define OPENPROPS_NONE          0x0000
#define OPENPROPS_RAWIO         0x0001
#define OPENPROPS_INFONLY       0x0002
#define OPENPROPS_FORCEREALMODE 0x0004
#define OPENPROPS_INHIBITPIF    0x8000

#define GETPROPS_NONE           0x0000
#define GETPROPS_RAWIO          0x0001
#define GETPROPS_EXTENDED       0x0004          // ;Internal
#define GETPROPS_OEM            0x0008

#define SETPROPS_NONE           0x0000
#define SETPROPS_RAWIO          0x0001
#define SETPROPS_CACHE          0x0002
#define SETPROPS_EXTENDED       0x0004          // ;Internal
#define SETPROPS_OEM            0x0008

#define FLUSHPROPS_NONE         0x0000
#define FLUSHPROPS_DISCARD      0x0001

#define CLOSEPROPS_NONE         0x0000
#define CLOSEPROPS_DISCARD      0x0001

#define CREATEPROPS_NONE        0x0000

#define DELETEPROPS_NONE        0x0000
#define DELETEPROPS_DISCARD     0x0001
#define DELETEPROPS_ABORT       0x0002

#define LOADPROPLIB_DEFER       0x0001


/* XLATOFF */
#ifndef FAR
#define FAR
#endif
/* XLATON */

//#ifdef  RECT
//#define _INC_WINDOWS
//#endif

//#ifndef _INC_WINDOWS

/* ASM
RECT    struc
        rcLeft      dw  ?
        rcTop       dw  ?
        rcRight     dw  ?
        rcBottom    dw  ?
RECT    ends
*/

/* XLATOFF */
typedef struct tagPIFRECT {
    WORD left;
    WORD top;
    WORD right;
    WORD bottom;
} PIFRECT;
typedef PIFRECT *PPIFRECT;
typedef PIFRECT FAR *LPPIFRECT;
/* XLATON */

//#endif


/*
 *  Property groups, used by PIFMGR.DLL and VxD interfaces
 *
 *  The structures for each of the pre-defined, ordinal-based groups
 *  is a logical view of data in the associated PIF file, if any -- not a
 *  physical view.
 */

#define GROUP_PRG               1           // program group

#define PRG_DEFAULT             0
#define PRG_CLOSEONEXIT         0x0001      // MSflags & EXITMASK
#define PRG_NOSUGGESTMSDOS      0x0400      // see also: PfW386Flags & fNoSuggestMSDOS

#define PRGINIT_DEFAULT         0
#define PRGINIT_MINIMIZED       0x0001      // see also: PfW386Flags & fMinimized
#define PRGINIT_MAXIMIZED       0x0002      // see also: PfW386Flags & fMaximized
#define PRGINIT_WINLIE          0x0004      // see also: PfW386Flags & fWinLie
#define PRGINIT_REALMODE        0x0008      // see also: PfW386Flags & fRealMode
#define PRGINIT_REALMODESILENT  0x0100      // see also: PfW386Flags & fRealModeSilent
#define PRGINIT_QUICKSTART      0x0200      // see also: PfW386Flags & fQuickStart  /* ;Internal */
#define PRGINIT_AMBIGUOUSPIF    0x0400      // see also: PfW386Flags & fAmbiguousPIF
#define PRGINIT_NOPIF           0x1000      // no PIF found
#define PRGINIT_DEFAULTPIF      0x2000      // default PIF found
#define PRGINIT_INFSETTINGS     0x4000      // INF settings found
#define PRGINIT_INHIBITPIF      0x8000      // INF indicates that no PIF be created

/*
 *  Real mode option flags.  NOTE: this field is a dword.  The low word
 *  uses these flags to indicate required options.  The high word is used
 *  to specify "nice" but not required options.
 */
#define RMOPT_MOUSE             0x0001      // Real mode mouse
#define RMOPT_EMS               0x0002      // Expanded Memory
#define RMOPT_CDROM             0x0004      // CD-ROM support
#define RMOPT_NETWORK           0x0008      // Network support
#define RMOPT_DISKLOCK          0x0010      // disk locking required
#define RMOPT_PRIVATECFG        0x0020      // use private configuration (ie, CONFIG/AUTOEXEC)
#define RMOPT_VESA              0x0040      // VESA driver


#define ICONFILE_DEFAULT        TEXT("PIFMGR.DLL")
#define ICONINDEX_DEFAULT       0

typedef struct PROPPRG {                    /* prg */
    WORD    flPrg;                          // see PRG_ flags
    WORD    flPrgInit;                      // see PRGINIT_ flags
    CHAR    achTitle[PIFNAMESIZE];          // name[30]
    CHAR    achCmdLine[PIFSTARTLOCSIZE+PIFPARAMSSIZE+1];// startfile[63] + params[64]
    CHAR    achWorkDir[PIFDEFPATHSIZE];     // defpath[64]
    WORD    wHotKey;                        // PfHotKeyScan thru PfHotKeyVal
    CHAR    achIconFile[PIFDEFFILESIZE];    // name of file containing icon
    WORD    wIconIndex;                     // index of icon within file
    DWORD   dwEnhModeFlags;                 // reserved enh-mode flags
    DWORD   dwRealModeFlags;                // real-mode flags (see RMOPT_*)
    CHAR    achOtherFile[PIFDEFFILESIZE];   // name of "other" file in directory
    CHAR    achPIFFile[PIFMAXFILEPATH];     // name of PIF file
} PROPPRG;
typedef UNALIGNED PROPPRG *PPROPPRG;
typedef UNALIGNED PROPPRG FAR *LPPROPPRG;
typedef const UNALIGNED PROPPRG FAR *LPCPROPPRG;



#define GROUP_TSK               2           // tasking group

#define TSK_DEFAULT             (TSK_BACKGROUND)
#define TSK_ALLOWCLOSE          0x0001      // PfW386Flags & fEnableClose
#define TSK_BACKGROUND          0x0002      // PfW386Flags & fBackground
#define TSK_EXCLUSIVE           0x0004      // PfW386Flags & fExclusive             /* ;Internal */
#define TSK_NOWARNTERMINATE     0x0010      // Don't warn before closing
#define TSK_NOSCREENSAVER       0x0020      // Do not activate screen saver

#define TSKINIT_DEFAULT         0

#define TSKFGNDBOOST_DEFAULT    0           // fgnd boost                           /* ;Internal */
#define TSKBGNDBOOST_DEFAULT    0           // bgnd boost                           /* ;Internal */
                                                                                    /* ;Internal */
#define TSKFGND_OLD_DEFAULT     100         // normal fgnd setting                  /* ;Internal */
#define TSKBGND_OLD_DEFAULT     50          // normal bgnd setting                  /* ;Internal */
                                                                                    /* ;Internal */
#define TSKIDLESENS_DEFAULT     50          // % (min-max == 0-100)

typedef struct PROPTSK {                    /* tsk */
    WORD    flTsk;                          // see TSK_ flags
    WORD    flTskInit;                      // see TSKINIT_ flags
    WORD    wReserved1;                     // (reserved, must be zero)
    WORD    wReserved2;                     // (reserved, must be zero)
    WORD    wReserved3;                     // (reserved, must be zero)
    WORD    wReserved4;                     // (reserved, must be zero)
    WORD    wIdleSensitivity;               // %, also affects PfW386Flags & fPollingDetect
} PROPTSK;
typedef UNALIGNED PROPTSK *PPROPTSK;
typedef UNALIGNED PROPTSK FAR *LPPROPTSK;


#define GROUP_VID               3           // video group

#define VID_DEFAULT             (VID_TEXTEMULATE)
#define VID_TEXTEMULATE         0x0001      // PfW386Flags2 & fVidTxtEmulate
#define VID_RETAINMEMORY        0x0080      // PfW386Flags2 & fVidRetainAllo
#define VID_FULLSCREEN          0x0100      // PfW386Flags  & fFullScreen

#define VIDINIT_DEFAULT         0

#define VIDSCREENLINES_MIN      0           // in # lines (0 = use VDD value)
#define VIDSCREENLINES_DEFAULT  0           // in # lines
#define VIDSCREENLINES_MAX      50          // in # lines                           /* ;Internal */

typedef struct PROPVID {                    /* vid */
    WORD    flVid;                          // see VID_ flags
    WORD    flVidInit;                      // see VIDINIT_ flags
    WORD    wReserved1;                     // (reserved, must be zero)
    WORD    wReserved2;                     // (reserved, must be zero)
    WORD    wReserved3;                     // (reserved, must be zero)
    WORD    cScreenLines;                   // ([NonWindowsApp]:ScreenLines)
} PROPVID;
typedef UNALIGNED PROPVID *PPROPVID;
typedef UNALIGNED PROPVID FAR *LPPROPVID;


#define GROUP_MEM               4           // memory group

#define MEM_DEFAULT             0

#define MEMINIT_DEFAULT         0
#define MEMINIT_NOHMA           0x0001      // PfW386Flags & fNoHMA
#define MEMINIT_LOWLOCKED       0x0002      // PfW386Flags & fVMLocked
#define MEMINIT_EMSLOCKED       0x0004      // PfW386Flags & fEMSLocked
#define MEMINIT_XMSLOCKED       0x0008      // PfW386Flags & fXMSLocked
#define MEMINIT_GLOBALPROTECT   0x0010      // PfW386Flags & fGlobalProtect
#define MEMINIT_STRAYPTRDETECT  0x0020      // PfW386Flags & fStrayPtrDetect        /* ;Internal */
#define MEMINIT_LOCALUMBS       0x0040      // PfW386Flags & fLocalUMBs             /* ;Internal */

#define MEMLOW_MIN              0           // in KB
#define MEMLOW_DEFAULT          0           // in KB
#define MEMLOW_MAX              640         // in KB

#define MEMEMS_MIN              0           // in KB
#define MEMEMS_DEFAULT          0           // in KB
#define MEMEMS_MAX              0xFFFF      // in KB

#define MEMXMS_MIN              0           // in KB
#define MEMXMS_DEFAULT          0           // in KB
#define MEMXMS_MAX              0xFFFF      // in KB

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
typedef UNALIGNED PROPMEM *PPROPMEM;
typedef UNALIGNED PROPMEM FAR *LPPROPMEM;


#define GROUP_KBD               5           // keyboard group

#define KBD_DEFAULT             (KBD_FASTPASTE)
#define KBD_FASTPASTE           0x0001      // PfW386Flags & fINT16Paste
#define KBD_NOALTTAB            0x0020      // PfW386Flags & fALTTABdis
#define KBD_NOALTESC            0x0040      // PfW386Flags & fALTESCdis
#define KBD_NOALTSPACE          0x0080      // PfW386Flags & fALTSPACEdis
#define KBD_NOALTENTER          0x0100      // PfW386Flags & fALTENTERdis
#define KBD_NOALTPRTSC          0x0200      // PfW386Flags & fALTPRTSCdis
#define KBD_NOPRTSC             0x0400      // PfW386Flags & fPRTSCdis
#define KBD_NOCTRLESC           0x0800      // PfW386Flags & fCTRLESCdis

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
typedef UNALIGNED PROPKBD *PPROPKBD;
typedef UNALIGNED PROPKBD FAR *LPPROPKBD;


#define GROUP_MSE               6           // mouse group

/* No VxD currently pays attention to PROPMSE. VMDOSAPP should know how to
 * handle all cases resulting from a change in these flags.
 *
 * Note that MSE_WINDOWENABLE corresponds to the Windows NT "QuickEdit"
 * property, except backwards.
 */

#define MSE_DEFAULT             (MSE_WINDOWENABLE)
#define MSE_WINDOWENABLE        0x0001      // ([NonWindowsApp]:MouseInDosBox)
#define MSE_EXCLUSIVE           0x0002      //

#define MSEINIT_DEFAULT         0           // default flags

typedef struct PROPMSE {                    /* mse */
    WORD    flMse;                          // see MSE_ flags
    WORD    flMseInit;                      // see MSEINIT_ flags
} PROPMSE;
typedef UNALIGNED PROPMSE *PPROPMSE;
typedef UNALIGNED PROPMSE FAR *LPPROPMSE;


#define GROUP_SND               7           // sound group                  /* ;Internal */
                                                                            /* ;Internal */
#define SND_DEFAULT             (SND_SPEAKERENABLE)                         /* ;Internal */
#define SND_SPEAKERENABLE       0x0001      //                              /* ;Internal */
                                                                            /* ;Internal */
#define SNDINIT_DEFAULT         0                                           /* ;Internal */
                                                                            /* ;Internal */
typedef struct PROPSND {                    /* snd */                       /* ;Internal */
    WORD    flSnd;                          // see SND_ flags               /* ;Internal */
    WORD    flSndInit;                      // see SNDINIT_ flags           /* ;Internal */
} PROPSND;                                                                  /* ;Internal */
typedef UNALIGNED PROPSND *PPROPSND;                                        /* ;Internal */
typedef UNALIGNED PROPSND FAR *LPPROPSND;                                   /* ;Internal */
                                                                            /* ;Internal */
                                                                            /* ;Internal */
#define GROUP_FNT               8           // font group

#define FNT_DEFAULT             (FNT_BOTHFONTS | FNT_AUTOSIZE)
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
    CHAR    achRasterFaceName[LF_FACESIZE]; // name to use for raster font
    CHAR    achTTFaceName[LF_FACESIZE];     // name to use for tt font
    WORD    wCurrentCP;                     // Current Codepage
} PROPFNT;
typedef UNALIGNED PROPFNT *PPROPFNT;
typedef UNALIGNED PROPFNT FAR *LPPROPFNT;

#define GROUP_WIN               9          // window group

#define WIN_DEFAULT             (WIN_SAVESETTINGS | WIN_TOOLBAR)
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
    WORD    wLength;
    WORD    wShowFlags;
    WORD    wShowCmd;
    WORD    xMinimize;
    WORD    yMinimize;
    WORD    xMaximize;
    WORD    yMaximize;
    PIFRECT rcNormal;
} PROPWIN;
typedef UNALIGNED PROPWIN *PPROPWIN;
typedef UNALIGNED PROPWIN FAR *LPPROPWIN;

#define PIF_WP_SIZE             ((sizeof(WORD)*7) + sizeof(PIFRECT))

#define GROUP_ENV               10          // environment/startup group

#define ENV_DEFAULT             0

#define ENVINIT_DEFAULT         0

#define ENVSIZE_MIN             0
#define ENVSIZE_DEFAULT         0
#define ENVSIZE_MAX             32768

#define ENVDPMI_MIN             0           // in KB
#define ENVDPMI_DEFAULT         0           // in KB (0 = Auto)
#define ENVDPMI_MAX             0xFFFF      // in KB

typedef struct PROPENV {                    /* env */
    WORD    flEnv;                          // see ENV_ flags
    WORD    flEnvInit;                      // see ENVINIT_ flags
    CHAR    achBatchFile[PIFDEFFILESIZE];   //
    WORD    cbEnvironment;                  // ([386Enh]:CommandEnvSize)
    WORD    wMaxDPMI;                       // (NEW)
} PROPENV;
typedef UNALIGNED PROPENV *PPROPENV;
typedef UNALIGNED PROPENV FAR *LPPROPENV;

#ifdef WINNT

#define GROUP_NT31              11
#ifndef UNICODE
#define MAX_VALID_GROUP         GROUP_NT31
#endif

typedef struct PROPNT31 {
   DWORD dwWNTFlags;                                                        /* ;Internal */
   DWORD dwRes1;                                                            /* ;Internal */
   DWORD dwRes2;                                                            /* ;Internal */
   char  achConfigFile[PIFDEFPATHSIZE];                                     /* ;Internal */
   char  achAutoexecFile[PIFDEFPATHSIZE];                                   /* ;Internal */
} PROPNT31;
typedef UNALIGNED PROPNT31 *PPROPNT31;
typedef UNALIGNED PROPNT31 FAR *LPPROPNT31;
#define COMPAT_TIMERTIC 0x10                                                /* ;Internal */
#define NT31_COMPATTIMER COMPAT_TIMERTIC                                    /* ;Internal */
#endif

#ifdef UNICODE
#ifdef GROUP_NT31
#define GROUP_NT40              12
#else
#define GROUP_NT40              11
#endif
#define MAX_VALID_GROUP         GROUP_NT40


#define WNT_LET_SYS_POS         0x0001
#define WNT_CONSOLE_PROPS       0x0002

typedef struct PROPNT40 {                                   /* wnt */
   DWORD    flWnt;                                          // NT Specific PIF falgs

// UNICODE version of strings, and copy of ANSI to see if they've changed

   WCHAR    awchCmdLine[PIFSTARTLOCSIZE+PIFPARAMSSIZE+1];   // Command line
   CHAR     achSaveCmdLine[PIFSTARTLOCSIZE+PIFPARAMSSIZE+1];// Saved ANSI Command Line

   WCHAR    awchOtherFile[PIFDEFFILESIZE];                  // name of "other" file in directory
   CHAR     achSaveOtherFile[PIFDEFFILESIZE];               // Saved ANSI "other" file in directory

   WCHAR    awchPIFFile[PIFDEFFILESIZE];                    // name of PIF file
   CHAR     achSavePIFFile[PIFDEFFILESIZE];                 // Saved ANSI name of PIF file

   WCHAR    awchTitle[PIFNAMESIZE];                         // Title for cmd window
   CHAR     achSaveTitle[PIFNAMESIZE];                      // Saved ANSI Title for cmd window

   WCHAR    awchIconFile[PIFDEFFILESIZE];                   // Name of file containing icons
   CHAR     achSaveIconFile[PIFDEFFILESIZE];                // Saved ANSI Name of file containing icons

   WCHAR    awchWorkDir[PIFDEFPATHSIZE];                    // working directory
   CHAR     achSaveWorkDir[PIFDEFPATHSIZE];                 // Saved ANSI working directory

   WCHAR    awchBatchFile[PIFDEFFILESIZE];                  // batch file
   CHAR     achSaveBatchFile[PIFDEFFILESIZE];               // Saved ANSI batch file

// Console properties

   DWORD    dwForeColor;                                    // Console Text Foreground Color
   DWORD    dwBackColor;                                    // Console Text Background Color
   DWORD    dwPopupForeColor;                               // Console Popup Text Foreground Color
   DWORD    dwPopupBackColor;                               // Console Popup Text Background Color
   COORD    WinSize;                                        // Console Window Size
   COORD    BuffSize;                                       // Console Buffer Size
   POINT    WinPos;                                         // Console Window Position
   DWORD    dwCursorSize;                                   // Console Cursor Size
   DWORD    dwCmdHistBufSize;                               // Console Command Histroy Buffer Size
   DWORD    dwNumCmdHist;                                   // Number of Command Histories for Console

} PROPNT40;
typedef UNALIGNED PROPNT40 *PPROPNT40;
typedef UNALIGNED PROPNT40 FAR *LPPROPNT40;

#else

#ifndef WINNT
#define MAX_VALID_GROUP         GROUP_ENV
#endif

#endif // UNICODE

#define GROUP_ICON              (MAX_VALID_GROUP+1)
#define GROUP_MAX               0x0FF

                                                                                // ;Internal
                                                                                // ;Internal
/*                                                                              // ;Internal
 * Additional group ordinal bits that can be passed to VxD property hooks       // ;Internal
 */                                                                             // ;Internal
#define EXT_GROUP_QUERY         0x100                                           // ;Internal
#define EXT_GROUP_UPDATE        0x200                                           // ;Internal
                                                                                // ;Internal
                                                                                // ;Internal
/*
 *  PIF "file" structures, used by .PIFs
 */

#define PIFEXTSIGSIZE   16                  // Length of extension signatures
#define MAX_GROUP_NAME  PIFEXTSIGSIZE       //
#define STDHDRSIG       "MICROSOFT PIFEX"   // extsig value for stdpifext
#define LASTHDRPTR      0xFFFF              // This value in the
                                            //  extnxthdrfloff field indicates
                                            //   there are no more extensions.
#define W286HDRSIG30     "WINDOWS 286 3.0"
#define W386HDRSIG30     "WINDOWS 386 3.0"
#define WNTHDRSIG31      "WINDOWS NT  3.1"
#define WENHHDRSIG40     "WINDOWS VMM 4.0"  //
#define WNTHDRSIG40      "WINDOWS NT  4.0"

#define CONFIGHDRSIG40   "CONFIG  SYS 4.0"  //
#define AUTOEXECHDRSIG40 "AUTOEXECBAT 4.0"  //

#define MAX_CONFIG_SIZE     4096
#define MAX_AUTOEXEC_SIZE   4096

#define CONFIGFILE      TEXT("\\CONFIG.SYS")      // normal filenames
#define AUTOEXECFILE    TEXT("\\AUTOEXEC.BAT")

#define MCONFIGFILE     TEXT("\\CONFIG.APP")      // msdos-mode temp filenames
#define MAUTOEXECFILE   TEXT("\\AUTOEXEC.APP")

#define WCONFIGFILE     TEXT("\\CONFIG.WOS")      // windows-mode temp filenames
#define WAUTOEXECFILE   TEXT("\\AUTOEXEC.WOS")


typedef struct PIFEXTHDR {                  /* peh */
    CHAR    extsig[PIFEXTSIGSIZE];
    WORD    extnxthdrfloff;
    WORD    extfileoffset;
    WORD    extsizebytes;
} PIFEXTHDR;
typedef UNALIGNED PIFEXTHDR *PPIFEXTHDR;
typedef UNALIGNED PIFEXTHDR FAR *LPPIFEXTHDR;


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
    CHAR    appname[PIFNAMESIZE];           // 0x02     'MS-DOS Prompt'
    WORD    maxmem;                         // 0x20     0x0200 (512Kb)
    WORD    minmem;                         // 0x22     0x0080 (128Kb)
    CHAR    startfile[PIFSTARTLOCSIZE];     // 0x24     "COMMAND.COM"
    BYTE    MSflags;                        // 0x63     0x10
    BYTE    reserved;                       // 0x64     0x00
    CHAR    defpath[PIFDEFPATHSIZE];        // 0x65     "\"
    CHAR    params[PIFPARAMSSIZE];          // 0xA5     ""
    BYTE    screen;                         // 0xE5     0x00
    BYTE    cPages;                         // 0xE6     0x01 (ALWAYS!)
    BYTE    lowVector;                      // 0xE7     0x00 (ALWAYS!)
    BYTE    highVector;                     // 0xE8     0xFF (ALWAYS!)
    BYTE    rows;                           // 0xE9     0x19 (Not used)
    BYTE    cols;                           // 0xEA     0x50 (Not used)
    BYTE    rowoff;                         // 0xEB     0x00 (Not used)
    BYTE    coloff;                         // 0xEC     0x00 (Not used)
    WORD    sysmem;                         // 0xED   0x0007 (Not used; 7=>Text, 23=>Grfx/Mult Text)
    CHAR    shprog[PIFSHPROGSIZE];          // 0xEF     0's  (Not used)
    CHAR    shdata[PIFSHDATASIZE];          // 0x12F    0's  (Not used)
    BYTE    behavior;                       // 0x16F    0x00
    BYTE    sysflags;                       // 0x170    0x00 (Not used)
} STDPIF;
typedef UNALIGNED STDPIF *PSTDPIF;
typedef UNALIGNED STDPIF FAR *LPSTDPIF;


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
typedef UNALIGNED W286PIF30 *PW286PIF30;
typedef UNALIGNED W286PIF30 FAR *LPW286PIF30;


/* Flags for PfW386Flags
 */

#define fEnableClose    0x00000001          //
#define fEnableCloseBit             0       //
#define fBackground     0x00000002          //
#define fBackgroundBit              1       //
#define fExclusive      0x00000004          //                          /* ;Internal */
#define fExclusiveBit               2       //                          /* ;Internal */
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
//                      0x00400000          //  Not used                /* ;Internal */
//                                  22      //  Not used                /* ;Internal */
#define fRealMode       0x00800000          //  New for 4.00
#define fRealModeBit                23      //  New for 4.00
#define fWinLie         0x01000000          //  New for 4.00
#define fWinLieBit                  24      //  New for 4.00
#define fStrayPtrDetect 0x02000000          //  New for 4.00            /* ;Internal */
#define fStrayPtrDetectBit          25      //  New for 4.00            /* ;Internal */
#define fNoSuggestMSDOS 0x04000000          //  New for 4.00
#define fNoSuggestMSDOSBit          26      //  New for 4.00
#define fLocalUMBs      0x08000000          //  New for 4.00            /* ;Internal */
#define fLocalUMBsBit               27      //  New for 4.00            /* ;Internal */
#define fRealModeSilent 0x10000000          //  New for 4.00
#define fRealModeSilentBit          28      //  New for 4.00
#define fQuickStart     0x20000000          //  New for 4.00            /* ;Internal */
#define fQuickStartBit              29      //  New for 4.00            /* ;Internal */
#define fAmbiguousPIF   0x40000000          //  New for 4.00
#define fAmbiguousPIFBit            30      //  New for 4.00

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
#define fVidNoTrpTxt    0x00000002          // Obsolete
#define fVidNoTrpTxtBit             1       // Obsolete
#define fVidNoTrpLRGrfx 0x00000004          // Obsolete
#define fVidNoTrpLRGrfxBit          2       // Obsolete
#define fVidNoTrpHRGrfx 0x00000008          // Obsolete
#define fVidNoTrpHRGrfxBit          3       // Obsolete
#define fVidTextMd      0x00000010          // Obsolete
#define fVidTextMdBit               4       // Obsolete
#define fVidLowRsGrfxMd 0x00000020          // Obsolete
#define fVidLowRsGrfxMdBit          5       // Obsolete
#define fVidHghRsGrfxMd 0x00000040          // Obsolete
#define fVidHghRsGrfxMdBit          6       // Obsolete
#define fVidRetainAllo  0x00000080          //
#define fVidRetainAlloBit           7       //

/*                                                                             ;Internal
 * This mask is used to isolate status bits shared with VM_Descriptor          ;Internal
 */                                                                         /* ;Internal */
                                                                            /* ;Internal */
#define PifDescMask                                                         /* ;Internal */ \
(fALTTABdis   + fALTESCdis    + fALTSPACEdis +                              /* ;Internal */ \
 fALTENTERdis + fALTPRTSCdis  + fPRTSCdis +                                 /* ;Internal */ \
 fCTRLESCdis  + fPollingDetect+ fNoHMA +                                    /* ;Internal */ \
 fHasHotKey   + fEMSLocked    + fXMSLocked +                                /* ;Internal */ \
 fINT16Paste  + fVMLocked)                                                  /* ;Internal */
                                                                            /* ;Internal */
                                                                            /* ;Internal */
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
    CHAR    PfW386params[PIFPARAMSSIZE];    // 0x1E1
} W386PIF30;
typedef UNALIGNED W386PIF30 *PW386PIF30;
typedef UNALIGNED W386PIF30 FAR *LPW386PIF30;


typedef struct WENHPIF40 {                  /* enh */                       /* ;Internal */
    DWORD   dwEnhModeFlagsProp;             // PROPPRG data (subset)        /* ;Internal */
    DWORD   dwRealModeFlagsProp;            // PROPPRG data (subset)        /* ;Internal */
    CHAR    achOtherFileProp[PIFDEFFILESIZE];//PROPPRG data (subset)        /* ;Internal */
    CHAR    achIconFileProp[PIFDEFFILESIZE];// PROPPRG data (subset)        /* ;Internal */
    WORD    wIconIndexProp;                 // PROPPRG data (subset)        /* ;Internal */
    PROPTSK tskProp;                        // PROPTSK data                 /* ;Internal */
    PROPVID vidProp;                        // PROPVID data                 /* ;Internal */
    PROPKBD kbdProp;                        // PROPKBD data                 /* ;Internal */
    PROPMSE mseProp;                        // PROPMSE data                 /* ;Internal */
    PROPSND sndProp;                        // PROPSND data                 /* ;Internal */
    PROPFNT fntProp;                        // PROPFNT data                 /* ;Internal */
    PROPWIN winProp;                        // PROPWIN data                 /* ;Internal */
    PROPENV envProp;                        // PROPENV data                 /* ;Internal */
    WORD    wInternalRevision;              // Internal WENHPIF40 version   /* ;Internal */
} WENHPIF40;                                                                /* ;Internal */
typedef UNALIGNED WENHPIF40 *PWENHPIF40;                                    /* ;Internal */
typedef UNALIGNED WENHPIF40 FAR *LPWENHPIF40;                               /* ;Internal */

#ifdef WINNT
/* Windows NT extension format */
typedef struct WNTPIF31 {                                                   /* ;Internal */
   PROPNT31 nt31Prop;                                                       /* ;Internal */
   WORD     wInternalRevision;                                              /* ;Internal */
} WNTPIF31;                                                                 /* ;Internal */
typedef UNALIGNED WNTPIF31 *PWNTPIF31;                                      /* ;Internal */
typedef UNALIGNED WNTPIF31 FAR *LPWNTPIF31;                                 /* ;Internal */
#endif
                                                                            /* ;Internal */
#ifdef UNICODE
typedef struct WNTPIF40 {                   /* adv */                       /* ;Internal */
    PROPNT40 nt40Prop;                      // PROPWNT data                 /* ;Internal */
    WORD    wInternalRevision;              // Internal WNTPIF40 version    /* ;Internal */
} WNTPIF40;                                                                 /* ;Internal */
typedef UNALIGNED WNTPIF40 *PWNTPIF40;                                      /* ;Internal */
typedef UNALIGNED WNTPIF40 FAR *LPWNTPIF40;                                 /* ;Internal */
#endif
                                                                            /* ;Internal */
//                                                                          /* ;Internal */
// Whenever a previously reserved field or bit becomes used, increment      /* ;Internal */
// the internal revision so that we know to zero them out when we see a     /* ;Internal */
// down-level PIF file.                                                     /* ;Internal */
//                                                                          /* ;Internal */
#define WENHPIF40_VERSION       1           // Current internal version     /* ;Internal */
#define WNTPIF40_VERSION        1           // Current internal version     /* ;Internal */
#define WNTPIF31_VERSION        1           // Current internal version     /* ;Internal */

                                                                            /* ;Internal */
typedef struct PIFDATA {                    /* pd */  //Examples            /* ;Internal */
                                                                            /* ;Internal */
    STDPIF      stdpifdata;                 // 0x000                        /* ;Internal */
                                                                            /* ;Internal */
    PIFEXTHDR   stdpifext;                  // 0x171                        /* ;Internal */
//  struct {                                                                /* ;Internal */
//      CHAR    extsig[PIFEXTSIGSIZE];      // 0x171    "MICROSOFT PIFEX"   /* ;Internal */
//      WORD    extnxthdrfloff;             // 0x181    0x0187 (or 0xFFFF)  /* ;Internal */
//      WORD    extfileoffset;              // 0x183    0x0000              /* ;Internal */
//      WORD    extsizebytes;               // 0x185    0x0171              /* ;Internal */
//  };                                                                      /* ;Internal */
                                                                            /* ;Internal */
    PIFEXTHDR   w286hdr30;                  // 0x187                        /* ;Internal */
//  struct {                                                                /* ;Internal */
//      CHAR    extsig[PIFEXTSIGSIZE];      // 0x187    "WINDOWS 286 3.0"   /* ;Internal */
//      WORD    extnxthdrfloff;             // 0x197    0x01A3 (or 0xFFFF)  /* ;Internal */
//      WORD    extfileoffset;              // 0x199    0x019D              /* ;Internal */
//      WORD    extsizebytes;               // 0x19B    0x0006              /* ;Internal */
//  };                                                                      /* ;Internal */
    W286PIF30   w286ext30;                  // 0x19D                        /* ;Internal */
                                                                            /* ;Internal */
    PIFEXTHDR   w386hdr30;                  // 0x1A3                        /* ;Internal */
//  struct {                                                                /* ;Internal */
//      CHAR    extsig[PIFEXTSIGSIZE];      // 0x1A3    "WINDOWS 386 3.0"   /* ;Internal */
//      WORD    extnxthdrfloff;             // 0x1B3    0xFFFF (ENH=0x221)  /* ;Internal */
//      WORD    extfileoffset;              // 0x1B5    0x01B9              /* ;Internal */
//      WORD    extsizebytes;               // 0x1B7    0x0068              /* ;Internal */
//  };                                                                      /* ;Internal */
    W386PIF30   w386ext30;                  // 0x1B9                        /* ;Internal */
                                                                            /* ;Internal */
    PIFEXTHDR   wenhhdr40;                  // 0x221                        /* ;Internal */
//  struct {                                                                /* ;Internal */
//      CHAR    extsig[PIFEXTSIGSIZE];      // 0x221    "WINDOWS VMM 4.0"   /* ;Internal */
//      WORD    extnxthdrfloff;             // 0x231    0x????              /* ;Internal */
//      WORD    extfileoffset;              // 0x233    0x0237              /* ;Internal */
//      WORD    extsizebytes;               // 0x235    ???                 /* ;Internal */
//  };                                                                      /* ;Internal */
    WENHPIF40   wenhext40;                  // 0x237                        /* ;Internal */

#ifdef WINNT
    PIFEXTHDR   wnthdr31;                   // 0x000                        /* ;Internal */
//  struct {                                                                /* ;Internal */
//      CHAR    extsig[PIFEXTSIGSIZE];      // 0x000    "WINDOWS NT  3.1"   /* ;Internal */
//      WORD    extnxthdrfloff;             // 0x000    0xFFFF              /* ;Internal */
//      WORD    extfileoffset;              // 0x000    0x0237              /* ;Internal */
//      WORD    extsizebytes;               // 0x000    ???                 /* ;Internal */
//  };                                                                      /* ;Internal */
    WNTPIF31    wntpif31;                   // 0x000                        /* ;Internal */
#endif


#ifdef UNICODE
    PIFEXTHDR   wnthdr40;                   // 0x000                        /* ;Internal */
//  struct {                                                                /* ;Internal */
//      CHAR    extsig[PIFEXTSIGSIZE];      // 0x000    "WINDOWS NT  4.0"   /* ;Internal */
//      WORD    extnxthdrfloff;             // 0x000    0xFFFF              /* ;Internal */
//      WORD    extfileoffset;              // 0x000    0x0237              /* ;Internal */
//      WORD    extsizebytes;               // 0x000    ???                 /* ;Internal */
//  };                                                                      /* ;Internal */
    WNTPIF40    wntpif40;                   // 0x000                        /* ;Internal */
#endif
                                                                            /* ;Internal */
} PIFDATA;                                  // 0x221 if Windows 3.x PIF     /* ;Internal */
typedef UNALIGNED PIFDATA *PPIFDATA;                                        /* ;Internal */
typedef UNALIGNED PIFDATA FAR *LPPIFDATA;                                   /* ;Internal */
                                                                            /* ;Internal */
                                                                            /* ;Internal */                                                                            /* ;Internal */
/* AssociateProperties associations
 */

#define HVM_ASSOCIATION         1
#define HWND_ASSOCIATION        2
#define LPARGS_ASSOCIATION      3                                           /* ;Internal */


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
#define ORD_CREATESTARTUPPROPERTIES 20
#define ORD_DELETESTARTUPPROPERTIES 21

typedef UINT PIFWIZERR;

#define PIFWIZERR_SUCCESS           0
#define PIFWIZERR_GENERALFAILURE    1
#define PIFWIZERR_INVALIDPARAM      2
#define PIFWIZERR_UNSUPPORTEDOPT    3
#define PIFWIZERR_OUTOFMEM          4
#define PIFWIZERR_USERCANCELED      5

#define WIZACTION_UICONFIGPROP      0
#define WIZACTION_SILENTCONFIGPROP  1
#define WIZACTION_CREATEDEFCLEANCFG 2

/* XLATOFF */

#ifdef WINAPI
PIFWIZERR WINAPI AppWizard(HWND hwnd, HANDLE hProps, UINT action);

int  WINAPI OpenProperties(LPCTSTR lpszApp, LPCTSTR lpszPIF, UINT hInf, UINT flOpt);
int  WINAPI GetProperties(HANDLE hProps, LPCSTR lpszGroup, LPVOID lpProps, int cbProps, UINT flOpt);
int  WINAPI SetProperties(HANDLE hProps, LPCSTR lpszGroup, const VOID FAR *lpProps, int cbProps, UINT flOpt);
int  WINAPI EditProperties(HANDLE hProps, LPCTSTR lpszTitle, UINT uStartPage, HWND hwnd, UINT uMsgPost);
int  WINAPI FlushProperties(HANDLE hProps, UINT flOpt);
HANDLE  WINAPI EnumProperties(HANDLE hProps);
LONG_PTR WINAPI AssociateProperties(HANDLE hProps, int iAssociate, LONG_PTR lData);
int  WINAPI CloseProperties(HANDLE hProps, UINT flOpt);
int  WINAPI CreateStartupProperties(HANDLE hProps, UINT flOpt);
int  WINAPI DeleteStartupProperties(HANDLE hProps, UINT flOpt);
BOOL WINAPI PifPropGetPages(LPVOID lpv, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);


#ifdef  PIF_PROPERTY_SHEETS
HANDLE  WINAPI LoadPropertyLib(LPCTSTR lpszDLL, int fLoad);
HANDLE  WINAPI EnumPropertyLibs(HANDLE iLib, LPHANDLE lphDLL, LPSTR lpszDLL, int cbszDLL);
BOOL WINAPI FreePropertyLib(HANDLE hLib);
HANDLE  WINAPI AddPropertySheet(const PROPSHEETPAGE FAR *lppsi, int iType);
BOOL WINAPI RemovePropertySheet(HANDLE hSheet);
int  WINAPI LoadPropertySheets(HANDLE hProps, int flags);
INT_PTR  WINAPI EnumPropertySheets(HANDLE hProps, int iType, INT_PTR iSheet, LPPROPSHEETPAGE lppsi);
HANDLE  WINAPI FreePropertySheets(HANDLE hProps, int flags);
#endif  /* PIF_PROPERTY_SHEETS */

#endif  /* WINAPI */

#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

/* XLATON */

#endif // _INC_PIF
