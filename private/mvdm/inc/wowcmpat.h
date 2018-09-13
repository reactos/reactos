/*++ BUILD Version: 0001
 *
 *  MVDM v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWCMPAT.H
 *  WOW compatibility flags
 *
 *  History:
 *  11-June-1993 Neil Sandlin (neilsa)
 *  Created.
--*/

////////// NOTE ////////// NOTE ////////// NOTE ////////// NOTE ////////// NOTE
//
// N   N       t
// NN  N  oo  ttt  ee
// N N N o  o  t  eeee  o
// N  NN o  o  t  e        Make sure you update both the string table entry and
// N   N  oo   tt  eee  o  the #define constant if you modify this file!!!
//
////////// NOTE ////////// NOTE ////////// NOTE ////////// NOTE ////////// NOTE


// These string tables only get included by mvdm\vdmexts\wow.c
// See NOTE above about adding anything to these tables!!!!
#ifdef _VDMEXTS_CFLAGS   // this is defined in mvdm\vdmexts\wow.c


// Original WOW compatibility flags.
// Kept in CURRENTPTD->dwWOWCompatFlags.

#ifdef _VDMEXTS_CF
{"WOWCF_GRAINYTICS",             0x80000000},
{"WOWCF_FAKEJOURNALRECORDHOOK",  0x40000000},
{"WOWCF_EDITCTRLWNDWORDS",       0x20000000},
{"WOWCF_SYNCHRONOUSDOSAPP",      0x10000000},

{"WOWCF_NOTDOSSPAWNABLE",        0x08000000},
{"WOWCF_RESETPAPER29ANDABOVE",   0x04000000},
{"WOWCF_4PLANECONVERSION",       0x02000000},
{"WOWCF_MGX_ESCAPES",            0x01000000},

{"WOWCF_CREATEBOGUSHWND",        0x00800000},
{"WOWCF_SANITIZEDOTWRSFILES",    0x00400000},
{"WOWCF_SIMPLEREGION",           0x00200000},
{"WOWCF_NOWAITFORINPUTIDLE",     0x00100000},

{"WOWCF_DSBASEDSTRINGPOINTERS",  0x00080000},
{"WOWCF_LIMIT_MEM_FREE_SPACE",   0x00040000},
{"WOWCF_DONTRELEASECACHEDDC",    0x00020000},
{"WOWCF_FORCETWIPSESCAPE",       0x00010000},

{"WOWCF_LB_NONNULLLPARAM",       0x00008000},
{"WOWCF_FORCENOPOSTSCRIPT",      0x00004000},
{"WOWCF_SETNULLMESSAGE",         0x00002000},
{"WOWCF_GWLINDEX2TO4",           0x00001000},

{"WOWCF_NEEDSTARTPAGE",          0x00000800},
{"WOWCF_NEEDIGNORESTARTPAGE",    0x00000400},
{"WOWCF_NOPC_RECTANGLE",         0x00000200},
{"WOWCF_NOFIRSTSAVE",            0x00000100},

{"WOWCF_ADD_MSTT",               0x00000080},
{"WOWCF_UNLOADNETFONTS",         0x00000040},
{"WOWCF_GETDUMMYDC",             0x00000020},
{"WOWCF_DBASEHANDLEBUG",         0x00000010},

{"WOWCF_NOCBDIRTHUNK",           0x00000008},
{"WOWCF_WMMDIACTIVATEBUG",       0x00000004},
{"WOWCF_UNIQUEHDCHWND",          0x00000002},
{"WOWCF_GWLCLRTOPMOST",          0x00000001},
#endif _VDMEXTS_CF




// Extra WOW compatibility flags bit definitions (WOWCFEX_).
// Kept in CURRENTPTD->dwWOWCompatFlagsEx.
#ifdef _VDMEXTS_CFEX
{"WOWCFEX_SENDPOSTEDMSG",        0x80000000},
{"WOWCFEX_BOGUSPOINTER",         0x40000000},
{"WOWCFEX_GETVERSIONHACK",       0x20000000},
{"WOWCFEX_FIXDCFONT4MENUSIZE",   0x10000000},

{"WOWCFEX_RESTOREEXPLORER",      0x08000000},
{"WOWCFEX_LONGWINEXECTAIL",      0x04000000},
{"WOWCFEX_FORCEINCDPMI",         0x02000000},
{"WOWCFEX_SETCAPSTACK",          0x01000000},

{"WOWCFEX_NODIBSHERE",           0x00800000},
{"WOWCFEX_PIXELMETRICS",         0x00400000},
{"WOWCFEX_DEFWNDPROCNCCALCSIZE", 0x00200000},
{"WOWCFEX_DIBDRVIMAGESIZEZERO",  0x00100000},

{"WOWCFEX_GLOBALDELETEATOM",     0x00080000},
{"WOWCFEX_IGNORECLIENTSHUTDOWN", 0x00040000},
{"WOWCFEX_ZAPGPPSDEFBLANKS",     0x00020000},
{"WOWCFEX_FAKECLASSINFOFAIL",    0x00010000},

{"WOWCFEX_SAMETASKFILESHARE",    0x00008000},
{"WOWCFEX_SAYITSNOTTHERE",       0x00004000},
{"WOWCFEX_BROKENFLATPOINTER",    0x00002000},
{"WOWCFEX_USEMCIAVI16",          0x00001000},

{"WOWCFEX_SAYNO2DRAWPATTERNRECT",0x00000800},
{"WOWCFEX_FAKENOTAWINDOW",       0x00000400},
{"WOWCFEX_NODIRECTHDPOPUP",      0x00000200},
{"WOWCFEX_ALLOWLFNDIALOGS",      0x00000100},

{"WOWCFEX_THUNKLBSELITEMRANGEEX",0x00000080},

{"WOWCFEX_FORMFEEDHACK",         0x00000001},
#endif _VDMEXTS_CFEX




// Win3.1/Win95/User32 compatibility bits (GACF_).
// Kept in CURRENTPTD->dwCompatFlags.
#ifdef _VDMEXTS_CF31
{"GACF_IGNORENODISCARD",        0x00000001},
{"GACF_FORCETEXTBAND",          0x00000002},
{"GACF_USEPRINTINGESCAPES"
 " aka GACF_ONELANDGRXBAND",    0x00000004},      // re-use GACF_ONELANDGRXBAND
{"GACF_IGNORETOPMOST",          0x00000008},
{"GACF_CALLTTDEVICE",           0x00000010},
{"GACF_MULTIPLEBANDS",          0x00000020},
{"GACF_ALWAYSSENDNCPAINT",      0x00000040},
{"GACF_EDITSETTEXTMUNGE",       0x00000080},
{"GACF_MOREEXTRAWNDWORDS",      0x00000100},
{"GACF_TTIGNORERASTERDUPE",     0x00000200},
{"GACF_HACKWINFLAGS",           0x00000400},
{"GACF_DELAYHWHNDSHAKECHK",     0x00000800},
{"GACF_ENUMHELVNTMSRMN",        0x00001000},
{"GACF_ENUMTTNOTDEVICE",        0x00002000},
{"GACF_SUBTRACTCLIPSIBS",       0x00004000},
{"GACF_FORCERASTERMODE"
    " aka GACF_FORCETTGRAPHICS",0x00008000},      // re-use GACF_FORCETTGRAPHICS
{"GACF_NOHRGN1",                0x00010000},
{"GACF_NCCALCSIZEONMOVE",       0x00020000},
{"GACF_SENDMENUDBLCLK",         0x00040000},
{"GACF_30AVGWIDTH",             0x00080000},
{"GACF_GETDEVCAPSNUMLIE",       0x00100000},

{"GACF_WINVER31",               0x00200000},      //
{"GACF_INCREASESTACK"
 " aka GACF_HEAPSLACK",         0x00400000},      //
{"GACF_FORCEWIN31DEVMODESIZE"
 " aka GACF_PEEKMESSAGEIDLE",   0x00800000},      // (replaces PEEKMESSAGEIDLE)
{"GACF_DISABLEFONTASSOC"
 " aka GACF_JAPANESCAPEMENT",   0x01000000},      // Used in FE only aka GACF_JAPANESCAPEMENT
{"GACF_IGNOREFAULTS",           0x02000000},      //
{"GACF_NOEMFSPOOLING",          0x04000000},      //
{"GACF_RANDOM3XUI",             0x08000000},      //
{"GACF_DONTJOURNALATTACH",      0x10000000},      //
{"GACF_NOBRUSHCACHE"
 " aka GACF_DISABLEDBCSPROPTT", 0x20000000},      // re-use GACF_DISABLEDBCSPROPTT
#endif _VDMEXTS_CF31




#ifdef FE_SB

// Extra WOW compatibility flags for DBCS.
// Kept in CURRENTPTD->dwWOWCompatFlags2.
#ifdef _VDMEXTS_CF_IME
{"WOWCF_AMIPRO_PM4J_IME",         0x00000001},
{"WOWCF_FORCEREGQRYLEN",          0x00000002},
{"WOWCF_AUDITNOTEPAD",            0x00000004},
{"WOWCF_USEUPPER",                0x00000008},

{"WOWCF_ASWHELPER",               0x00000010},
{"WOWCF_PPT4J_IME_GETVERSION",    0x00000020},
{"WOWCF_WORDJ_IME_GETVERSION",    0x00000040},
{"WOWCF_FLW2_PRINTING_PS",        0x00000080},

{"WOWCF_ARIRANG20_PRNDLG",        0x00000100},
{"WOWCF_BCW45J_COMMDLG",          0x00000200},
{"WOWCF_DIRECTOR_START",          0x00000400},
{"WOWCF_QPW_FIXINVALIDWINHANDLE", 0x00000800},

{"WOW_ICHITARO_ITALIC",           0x00001000},
#endif _VDMEXTS_CF_IME

#endif  // FE_SB


#else  // ! _VDMEXTS_CFLAGS
       // -- end the string tables included by mvdm\vdmexts\wow.c



//
// This part gets included by everything else
//


// Original WOW compatibility flags
// Kept in CURRENTPTD->dwWOWCompatFlags.
#define WOWCF_GRAINYTICS              0x80000000   // For apps that don't trust small GetTickCount deltas
#define WOWCF_FAKEJOURNALRECORDHOOK   0x40000000   // Used for MS Mail's MAILSPL
#define WOWCF_EDITCTRLWNDWORDS        0x20000000   // Used for Clip-Art Window Shopper SHOPPER
#define WOWCF_SYNCHRONOUSDOSAPP       0x10000000   // Used for BeyondMail installation
#define WOWCF_NOTDOSSPAWNABLE         0x08000000   // For apps that can't be spawned by dos as wowapps
#define WOWCF_RESETPAPER29ANDABOVE    0x04000000   // Used for WordPerfect DC_PAPERS
#define WOWCF_4PLANECONVERSION        0x02000000   // Used for PhotoShop 4pl-1bpp to 1pl-4bpp
#define WOWCF_MGX_ESCAPES             0x01000000   // Used for MicroGraphax Escapes
#define WOWCF_CREATEBOGUSHWND         0x00800000   // Explorapedia People has problems with handle recycling -- see bug #189004
#define WOWCF_SANITIZEDOTWRSFILES     0x00400000   // For WordPerfect printing on CONNECTED printers
#define WOWCF_SIMPLEREGION            0x00200000   // used to force simple region from GetClipBox
#define WOWCF_NOWAITFORINPUTIDLE      0x00100000   // InstallShield setup toolkit 3.00.077?.0 - 3.00.099.0 deadlock without this
#define WOWCF_DSBASEDSTRINGPOINTERS   0x00080000   // used for winworks2.0a so that it gets DS based string pointers
#define WOWCF_LIMIT_MEM_FREE_SPACE    0x00040000   // For apps that can't handle huge values returned by GetFreeSpace() (Automap Streets)
#define WOWCF_DONTRELEASECACHEDDC     0x00020000   // improv chart tool uses a released dc to get text extents, the dc is still usable on win3.1

// note: this is no longer set in the registry for PM5APP but users of older
//       versions of PM5 may need to set it manually - a-craigj
#define WOWCF_FORCETWIPSESCAPE        0x00010000   // PM5, force twips in Escape() of DOWNLOADFACE, GETFACENAME
#define WOWCF_LB_NONNULLLPARAM        0x00008000   // SuperProject: sets lParam of LB_GETTEXLEN message
#define WOWCF_FORCENOPOSTSCRIPT       0x00004000   // GetTechnology wont say PostScript.
#define WOWCF_SETNULLMESSAGE          0x00002000   // Winproj Tutorial: sets lpmsg->message = 0 in peekmessage
#define WOWCF_GWLINDEX2TO4            0x00001000   // PowerBuild30 uses index 2 on [S/G]etWindowLong for LISTBOXs, change it to 4 for NT. This is because, it is 16 bits on Win 31. and 32 bits on NT.
#define WOWCF_NEEDSTARTPAGE           0x00000800   // PhotoShop needs it
#define WOWCF_NEEDIGNORESTARTPAGE     0x00000400   // XPress needs it
#define WOWCF_NOPC_RECTANGLE          0x00000200   // QP draws bad if GetDeviceCaps(POLYGONALCAPS) sets PC_RECTANGLE
#define WOWCF_NOFIRSTSAVE             0x00000100   // Wordperfect needs it for meta files
#define WOWCF_ADD_MSTT                0x00000080   // FH4.0 needs to print on PS drivers
#define WOWCF_UNLOADNETFONTS          0x00000040   // Need to track an unload font loaded over net
#define WOWCF_GETDUMMYDC              0x00000020   // Corel Draw passes a NULL hDC to EnumMetaFile, we'll create a dummy to keep GDI32 happy.
#define WOWCF_DBASEHANDLEBUG          0x00000010   // Borland dBase handle bug
#define WOWCF_NOCBDIRTHUNK            0x00000008   // don't thunk CB_DIR lParam when sent to a subclassed window in PagePlus 3.0
#define WOWCF_WMMDIACTIVATEBUG        0x00000004   // Corel Chart doesn't pass correct params for WM_MDIACTIVATE (see ThunkWMMsg16())
#define WOWCF_UNIQUEHDCHWND           0x00000002   // For apps that assume that an hDC != hWnd
#define WOWCF_GWLCLRTOPMOST           0x00000001   // Lotus Approach needs the WS_EX_TOPMOST bit cleared on GWL of NETDDE AGENT window



// Extra WOW compatibility flags bit definitions (WOWCFEX_).  These flags
// are kept in CURRENTPTD->dwWOWCompatFlagsEx.
//
#define WOWCFEX_SENDPOSTEDMSG         0x80000000   // Lotus MM Reader.exe has message synchronization problem -- used to convert PostMessage() calls to SendMessage()
#define WOWCFEX_BOGUSPOINTER          0x40000000   // QuarkExpress v3.31 passes a hard coded 7FFF:0000 as the pointer to a RECT struct in an EM_GETRECT message
#define WOWCFEX_GETVERSIONHACK        0x20000000   // Set for programs we *may* wish to return 3.95 from GetVersion for.  WK32WowShouldWeSayWin95 restricts this further.
#define WOWCFEX_FIXDCFONT4MENUSIZE    0x10000000   // WP tutorial assumes that the font used to draw the menus is the same as the font selected into the hDc for the desktop window (hwnd == 0). This hack forces the use of the correct hDC.
#define WOWCFEX_RESTOREEXPLORER       0x08000000   // Symantec Q&A Install "restores" shell= by restoring saved copy of system.ini, fix it to explorer.exe
#define WOWCFEX_LONGWINEXECTAIL       0x04000000   // Intergraph Transcend setup uses too-long command tail with WinExec, don't fail if this flag is set.
#define WOWCFEX_FORCEINCDPMI          0x02000000   // Power Builder 4.0 needs to see DPMI alloc's with ever increasing linear address's.
#define WOWCFEX_SETCAPSTACK           0x01000000   // MS Works has unintialized variable. Hack stack to work around it.
#define WOWCFEX_NODIBSHERE            0x00800000   // PhotoShop 2.5 has bug getting DIB's from clipboard
#define WOWCFEX_PIXELMETRICS          0x00400000   // Freelance Tutorial, BorderWidth: winini metrics should be returned as pixels, not TWIPS
#define WOWCFEX_DEFWNDPROCNCCALCSIZE  0x00200000   // Pass WM_NCCALCSIZE to DefWindowProc for Mavis Beacon so USER 32 will set corect window flags.
#define WOWCFEX_DIBDRVIMAGESIZEZERO   0x00100000   // Return memory DC for dib.drv biSizeImage == 0  - Director 4.01
#define WOWCFEX_GLOBALDELETEATOM      0x00080000   // For Envoy viewer that ships with Word perfect office
#define WOWCFEX_IGNORECLIENTSHUTDOWN  0x00040000   // TurboCAD picks up saved 32-bit FS (x3b) and passes it as msg to DefFrameProc
#define WOWCFEX_ZAPGPPSDEFBLANKS      0x00020000   // Peachtree Accounting depends on GetPrivateProfileString zapping trailing blanks in caller's lpszDefault.
#define WOWCFEX_FAKECLASSINFOFAIL     0x00010000   // A bug in PageMaker 50a depends on the GetClassInfo failing in Win3.1 where it succeeds on NT
#define WOWCFEX_SAMETASKFILESHARE     0x00008000   // Broderbund Living Books install opens "install.txt" DENY ALL, and then tries to open it again
#define WOWCFEX_SAYITSNOTTHERE        0x00004000   // CrossTalk 2.2 hangs if it finds Printer/Device entry in xtalk.ini
#define WOWCFEX_BROKENFLATPOINTER     0x00002000   // Adobe Premiere 4.0 has a bug in its aliasing code which can touch unallocated memory
#define WOWCFEX_USEMCIAVI16           0x00001000   // Use 16-bit mciavi.drv for max compatibility
#define WOWCFEX_SAYNO2DRAWPATTERNRECT 0x00000800   // Many apps either don't handle DRP correctly or can't handle the 32-bit ones.
#define WOWCFEX_FAKENOTAWINDOW        0x00000400   // bug #235916 fail IsWindow calls for apps that get burned by handle recycling
#define WOWCFEX_NODIRECTHDPOPUP       0x00000200   // Indicates that we should not do a direct hardware popup for the app
#define WOWCFEX_ALLOWLFNDIALOGS       0x00000100   // Indicates that GetOpenFilename should support LFN
#define WOWCFEX_THUNKLBSELITEMRANGEEX 0x00000080   // Indicates that we should thunk the LB_SELITEMRANGEEX message (LB_ADDSTRING+3 in wow land)
// Note: This was put at 0x00000001 because it was back ported to 3.51 SP5
#define WOWCFEX_FORMFEEDHACK          0x00000001   // For apps that send a final form feed char to printer via Escape(PASSTHROUGH)





#ifdef FE_SB

// Extra WOW compatibility flags for DBCS  These flags
// are kept in CURRENTPTD->dwWOWCompatFlags2.
//

#define WOWCF_AMIPRO_PM4J_IME         0x00000001   // AMIPRO, set sizeof(DEVMODE of Win3.1) into dmSize , ExtDeviceMode; selectively ignore IME_SETCONVERSIONWINDOW MCW_DEFAULT
                                                   // PM4J, don't pass MCW_DEFAULT to prevent display timing problem
#define WOWCF_FORCEREGQRYLEN          0x00000002   // Lotus 123, set 80 into *lpcb , RegQueryValue
#define WOWCF_AUDITNOTEPAD            0x00000004   // Lotus Freelance Instration program, audit to exit notepad - read.me
#define WOWCF_USEUPPER                0x00000008   // Used for WinWrite "Key name"
#define WOWCF_ASWHELPER               0x00000010   // AutherWare Start, call SetMenu when called AppendMenu( MF_POPUP ) : MSKKBUG 3203
#define WOWCF_PPT4J_IME_GETVERSION    0x00000020   // PPT4J has a bug, expects the ime version to be 3.1 not greater.
// WARNING: For DaytonaJ RC1 Only.  Steal the following US bit for WinwordJ's TrueInLine hangup
#define WOWCF_WORDJ_IME_GETVERSION    0x00000040   // Used for Telling Winword we have OldVersion IME
#define WOWCF_FLW2_PRINTING_PS        0x00000080   // Lotus Freelance printing with PostScript.
                                                   // Between ESCAPE( BEGIN_PATH, CLIP_TO_PATH, END_PATH ), writing POLYGON with NULL_BRUSH
#define WOWCF_ARIRANG20_PRNDLG        0x00000100   // ARiRang word processor print dialg and print setup dialog problem : Korea
#define WOWCF_BCW45J_COMMDLG          0x00000200   // Boland C++ 4.5J, does not open common dialog : Japan
#define WOWCF_DIRECTOR_START          0x00000400   // Director 4.0J, does not start : Japan
#define WOWCF_QPW_FIXINVALIDWINHANDLE 0x00000800   // Quattro Pro Window use null window handle when it call Hanja conversion : Korea
#define WOW_ICHITARO_ITALIC           0x00001000   // map System Mincho to MS Mincho instead of Ms P Mincho
#endif // FE_SB





// Win3.1/Win95/User32 compatibility bits (GACF_).  These flags
// are kept in CURRENTPTD->dwCompatFlags, the 16-bit TDB,
// and over in user.
//
#ifndef _WINGDIP_         // these are defined by wingdip.h as well.


#define GACF_IGNORENODISCARD        0x00000001
#define GACF_FORCETEXTBAND          0x00000002
#define GACF_USEPRINTINGESCAPES     0x00000004      // re-use GACF_ONELANDGRXBAND
#define GACF_IGNORETOPMOST          0x00000008
#define GACF_CALLTTDEVICE           0x00000010
#define GACF_MULTIPLEBANDS          0x00000020
#define GACF_ALWAYSSENDNCPAINT      0x00000040
#define GACF_EDITSETTEXTMUNGE       0x00000080
#define GACF_MOREEXTRAWNDWORDS      0x00000100
#define GACF_TTIGNORERASTERDUPE     0x00000200
#define GACF_HACKWINFLAGS           0x00000400
#define GACF_DELAYHWHNDSHAKECHK     0x00000800
#define GACF_ENUMHELVNTMSRMN        0x00001000
#define GACF_ENUMTTNOTDEVICE        0x00002000
#define GACF_SUBTRACTCLIPSIBS       0x00004000
#define GACF_FORCERASTERMODE        0x00008000      // re-use GACF_FORCETTGRAPHICS
#define GACF_NOHRGN1                0x00010000
#define GACF_NCCALCSIZEONMOVE       0x00020000
#define GACF_SENDMENUDBLCLK         0x00040000
#define GACF_30AVGWIDTH             0x00080000
#define GACF_GETDEVCAPSNUMLIE       0x00100000

#define GACF_WINVER31               0x00200000      //
#define GACF_INCREASESTACK          0x00400000      //
#define GACF_FORCEWIN31DEVMODESIZE  0x00800000      // (replaces PEEKMESSAGEIDLE)
#define GACF_DISABLEFONTASSOC       0x01000000      // Used in FE only aka GACF_JAPANESCAPEMENT
#define GACF_IGNOREFAULTS           0x02000000      //
#define GACF_NOEMFSPOOLING          0x04000000      //
#define GACF_RANDOM3XUI             0x08000000      //
#define GACF_DONTJOURNALATTACH      0x10000000      //
#define GACF_NOBRUSHCACHE           0x20000000      // re-use GACF_DISABLEDBCSPROPTT
#define GACF_MIRRORREGFONTS         0x40000000      //


#endif // _WINGDIP_


#endif // ! _VDMEXTS_CFLAGS
