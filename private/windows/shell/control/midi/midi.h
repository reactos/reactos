/*
 * MIDI.H
 *
 * Copyright (C) 1990 Microsoft Corporation.
 *
 * Include file for MIDI control panel.
 */

#include <multimed.h>
#define SZCODE  char _based(_segname("_CODE"))

/* we were stupid and let an intern code the mapper */
#define STUPID

#if DBG
#if defined(WIN16)
extern VOID FAR PASCAL OutputDebugStr(LPSTR szString); // in COMM.ASM
#endif //WIN16
#define MDOUT(x) (OutputDebugStr("MIDI.CPL: "), OutputDebugStr(x), OutputDebugStr("\r\n"), 0)
#define MDOUTX(x) (OutputDebugStr(x), 0)
#else
#define MDOUT(x)     0
#define MDOUTX(x)    0
#endif

/********************************************************
 *                                                      *
 *      D A T A   T Y P E S                             *
 *                                                      *
 ********************************************************/

typedef struct tag_midiboxdata {
        DLGPROC lpfnBox;                // Dialog box function.
        int     idBox;                  // Resource ID of dialog box.
} MBDATA, FAR *LPMBDATA;

/********************************************************
 *                                                      *
 *      P R O T O T Y P E S                             *
 *                                                      *
 ********************************************************/

BOOL    FAR PASCAL _loadds      EnumFunc (LPSTR, LPSTR, UINT, HWND, LPSTR);
BOOL    FAR PASCAL              InvalidPortMsgBox (HWND);
BOOL    FAR PASCAL _loadds      KeyBox (HWND, UINT, WPARAM, LPARAM);
VOID    FAR PASCAL              Modify (BOOL);
BOOL    FAR PASCAL _loadds      PatchBox (HWND, UINT, WPARAM, LPARAM);
BOOL    FAR PASCAL _loadds      PropBox (HWND, UINT, WPARAM, LPARAM);
int     FAR PASCAL              QuerySave (VOID);
BOOL    FAR PASCAL _loadds      SetupBox (HWND, UINT, WPARAM, LPARAM);

#ifdef STUPID
//int     FAR PASCAL DosDelete(LPSTR);
BOOL    FAR PASCAL DupMapCfg(LPSTR,LPSTR);
BOOL    FAR PASCAL UpdateMapCfg(LPSTR,LPSTR);
#define ISSPACE(x)      ((x)==' '||(x)=='\t')
int FAR PASCAL          ComboLookup(HWND, LPSTR);
#define MAXPATHLEN      157             // 144 + "\12345678.123"

#endif

#ifdef ALLOW_SIZING
VOID    FAR PASCAL      SizeBox (VOID);
VOID    FAR PASCAL      WriteBoxRect (VOID);
#endif // ALLOW_SIZING

/********************************************************
 *                                                      *
 *      C O M M O N   D E F I N I T I O N S             *
 *                                                      *
 ********************************************************/

#define VERTMARGIN              4               // dialog units
#define HORZMARGIN              6

#define SEL_SETUP               0               // rgSelect array indices
#define SEL_PATCH               1
#define SEL_KEY                 2

#define ID_MAINBOX              700             // dialog box id's
#define ID_KEYBOX               701
#define ID_PROPBOX              702
#define DLG_SETUPEDIT           800
#define DLG_PATCHEDIT           801
#define DLG_KEYEDIT             802
#define IDH_DLG_MIDI_NEW        803
#define ID_MAINSETUPCOMBO       100             // main dialog box ctrl id's
#define ID_MAINPATCHCOMBO       101
#define ID_MAINKEYCOMBO         102
#define ID_MAINCOMBO            103
#define ID_MAINSETUP            104
#define ID_MAINPATCH            105
#define ID_MAINKEY              106
#define ID_MAINDELETE           107
#define ID_MAINEDIT             108
#define ID_MAINNEW              109
#define ID_MAINDESC             110
#define ID_MAINNAME             111

#define ID_MAINFIRSTRADIO       ID_MAINSETUP
#define ID_MAINLASTRADIO        ID_MAINKEY

#define ID_PROPNAME             100             // propbox ctrl id's
#define ID_PROPDESC             101

#define EN_ACTIVATE             0x1000          // custom messages
#define CBN_ACTIVATE            0x1000

#define WM_MY_INITDIALOG        0x1000
#define WM_MY_ENDDIALOG         0x1001
#define WM_MY_FREEDATA          0x1002

/********************************************************
 *                                                      *
 *      S E T U P   D E F I N I T I O N S               *
 *                                                      *
 ********************************************************/

#define NUM_CHANNELS            16

#define ID_SETUPGHOSTEDITFIRST  108     // ghost ctrl: first ctrl with tabstop
#define ID_SETUPGHOSTEDITLAST   109     // ghost ctrl: last ctrl with tabstop
#define ID_SETUPPORTLIST        110     // listbox for invalid port names
#define ID_SETUPEDIT            111     // channel number edit control id
#define ID_SETUPARROW           112     // channel number edit control id
#define ID_SETUPPORTCOMBO       113     // port name combo box control id
#define ID_SETUPPATCHCOMBO      114     // patch name combo box control id
#define ID_SETUPDESTMNEM        115
#define ID_SETUPPORTMNEM        116
#define ID_SETUPPATCHMNEM       117
#define ID_SETUPCHECK           118     // first 3-state btn (there are 16)
// don't define anything after this //

/********************************************************
 *                                                      *
 *      P A T C H M A P   D E F I N I T I O N S         *
 *                                                      *
 ********************************************************/

#define ID_PATCHGHOSTEDITFIRST  108     // ghost ctrl: first ctrl with tabstop
#define ID_PATCHGHOSTEDITLAST   109     // ghost ctrl: last ctrl with tabstop
#define ID_PATCHNUMARROW        110     // patch number arrow control id
#define ID_PATCHNUMEDIT         111     // patch number edit control id
#define ID_PATCHVOLARROW        112     // volume percent arrow control id
#define ID_PATCHVOLEDIT         113     // volume percent edit control id
#define ID_PATCHCOMBO           114     // keymap combo box control id
#define ID_PATCHSCROLL          115     // scroll bar control id
#define ID_PATCHBASED           116
#define ID_PATCHDESTMNEM        117
#define ID_PATCHVOLMNEM         118
#define ID_PATCHKEYMNEM         119

/********************************************************
 *                                                      *
 *      K E Y M A P   D E F I N I T I O N S             *
 *                                                      *
 ********************************************************/

#define ID_KEYGHOSTEDITFIRST    108     // ghost ctrl: first ctrl with tabstop
#define ID_KEYGHOSTEDITLAST     109     // ghost ctrl: last ctrl with tabstop
#define ID_KEYEDIT              110     // key number edit control id
#define ID_KEYARROW             111     // key number arrow control id
#define ID_KEYSCROLL            112     // scroll bar control id
#define ID_KEYDESTMNEM          113

/********************************************************
 *                                                      *
 *      S T R I N G   D E F I N I T I O N S             *
 *                                                      *
 ********************************************************/

#define IDS_MIDIMAPPER          500
#define IDS_VANILLANAME         502
#define IDS_VANILLADESC         503

#define IDS_SETUPS              1
#define IDS_PATCHES             2
#define IDS_KEYS                3

#define IDS_TITLE               102

/*      The first three of these are singular.  The next three are plural.
**      These values must retain their relative values.
*/

#define IDS_SETUP               103
#define IDS_PATCH               104
#define IDS_KEY                 105
#define IDS_SETUPPLURAL         106
#define IDS_PATCHPLURAL         107
#define IDS_KEYPLURAL           108

#define IDS_NEW                 109
#define IDS_SAVE                110
#define IDS_DELETE              111
#define IDS_CLOSE               112
#define IDS_NONE                113
#define IDS_ERROR               114
#define IDS_MIDIMAPCFG          115
#define IDS_HELPFILE            IDS_CONTROL_HLP
#define IDS_NOENTRIES           117
#define IDS_CAPTION             118

#define IDS_PORTNAME            120
#define IDS_PATCHNAME           121
#define IDS_SOURCE              122
#define IDS_DEST                123
#define IDS_CHANNEL             124
#define IDS_ACTIVE              125
#define IDS_CREATE_QUESTION     126
#define IDS_NEW_QUESTION        127
#define IDS_CHANGE_QUESTION     128
#define IDS_SAVE_CHANGES        129
#define IDS_RESERVED            130
#define IDS_SOURCEKEY           131
#define IDS_SOURCEKEYNAME       132
#define IDS_PATCHNUMBER         133
#define IDS_SOURCEPATCH         134
#define IDS_SOURCEPATCHNAME     135
#define IDS_INVALIDDESTINATION  136
#define IDS_USERERROR           137
#define IDS_SOURCEMNUMONIC      138
#define IDS_SOURCECHANNEL       139

//#define       IDS_NOCHANGEINUSE       140
//#define       IDS_NOEDITINUSE         141
#define IDS_NODELISCURRENT      142
#define IDS_NODELISREFERENCED   143
#define IDS_VERIFYDELETE        144
#define IDS_INVALIDPORT         145
#define IDS_DUPLICATE           146
#define IDS_READONLYMODE        147
#define IDS_ACTIVETITLE         148
#define IDS_NEW_KEY             149
#define IDS_NEW_SETUP           150
#define IDS_NEW_PATCH           151

#define IDS_PATCHMAP_BASE       152     // there can be 128 patches

#define IDS_KEYMAP_BASE         300     // there can be 128 keys

// This is crap coding just designed to make debugging harder.
//
// Errors are in the .rc file with names like IDS_MMAPERR_INVALIDSETUP
// Grepping on that id to see where it's used gives zilch.
// It's defined here as 1004.  Grepping on 1004 gives zilch.
//
// Instead there's a new name like MMAPERR_INVALIDSETUP defined in midimap.h as 4.
// (Grepping on 4 of course gives HUNDREDS of spurious hits)
// and a routine in midi.c called VShow_Error which adds IDS_MMAPERR_BASE
// (i.e. 1000) to the error code (4) and puts out the text for that number.
// Really clever stuff heh?

#define IDS_MMAPERR_BASE                1000    // With about a dozen of them.

#define IDS_MMAPERR_SUCCESS             1000
//#define       IDS_MMAPERR_FILENOTFOUND        1001
//#define       IDS_MMAPERR_INVALIDFLAG         1002
//#define       IDS_MMAPERR_INVALIDPORT         1003
#define IDS_MMAPERR_INVALIDSETUP        1004
#define IDS_MMAPERR_INVALIDPATCH        1005
#define IDS_MMAPERR_INVALIDKEY          1006
#define IDS_MMAPERR_MEMORY              1007
#define IDS_MMAPERR_READ                1008
#define IDS_MMAPERR_WRITE               1009
#define IDS_MMAPERR_OPEN                1010
#define IDS_MMAPERR_OPEN_READONLY       1011
#define IDS_MMAPERR_CREATE              1012
//#define       IDS_MMAPERR_UNDEFINED           1013
#define IDS_MMAPERR_FULL                1014

#define IDS_FCERR_ERROR                 1015
#define IDS_FCERR_WARN                  1016
#define IDS_FCERR_SUCCESS               1017
#define IDS_FCERR_NOSRC                 1018
#define IDS_FCERR_NODEST                1019
#define IDS_FCERR_DISKFULL              1020
#define IDS_FCERR_LOMEM                 1021
#define IDS_FCERR_WRITE                 1022
#define IDS_FCERR_DISK                  1023
#define IDS_FCERR_READONLY              1024

VOID FAR PASCAL VShowError(
        HWND    hwnd,
        MMAPERR mmaperr);

extern UINT NEAR uHelpMessage;
extern DWORD NEAR dwContext;
