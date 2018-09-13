/*++ BUILD Version: 0002    - Increment this if a change has global effects

/****************************************************************************/
/*                                                                          */
/*  PROGMAN.H -                                                             */
/*                                                                          */
/*      Include for the Windows Program Manager                             */
/*                                                                          */
/****************************************************************************/

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <winuserp.h>

#ifndef RC_INVOKED
#include "port1632.h"
#undef RDW_VALIDMASK
#endif

#include <pmvdm.h>

#include "pmhelp.h"
#include "shellapi.h"
#include "shlapip.h"

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Typedefs                                                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

typedef struct tagITEM {
    struct tagITEM *pNext;              /* link to next item */
    int             iItem;              /* index in group */
    DWORD           dwDDEId;            /* id used for Apps querying Progman */
                                        /* for its properties via DDE */
    RECT            rcIcon;             /* icon rectangle */
    HICON           hIcon;              /* the actual icon */
    RECT            rcTitle;            /* title rectangle */
} ITEM, *PITEM;

typedef struct tagGROUP {
    struct tagGROUP *pNext;               /* link to next group            */
    HWND            hwnd;                 /* hwnd of group window          */
    HANDLE          hGroup;               /* global handle of group object */
    PITEM           pItems;               /* pointer to first item         */
    LPTSTR          lpKey;                /* name of group key             */
    WORD            wIndex;               /* index in PROGMAN.INI of group */
    BOOL            fRO;                  /* group file is readonly        */
    BOOL            fCommon;              /* group is a common group vs a personal group */
    FILETIME        ftLastWriteTime;
    HBITMAP         hbm;                  /* bitmap 'o icons               */
    WORD            fLoaded;
    PSECURITY_DESCRIPTOR pSecDesc;
} GROUP, *PGROUP;

/*
 * .GRP File format structures -
 */
typedef struct tagGROUPDEF {
    DWORD   dwMagic;        /* magical bytes 'PMCC' */
    DWORD   cbGroup;        /* length of group segment */
    RECT    rcNormal;       /* rectangle of normal window */
    POINT   ptMin;          /* point of icon */
    WORD    wCheckSum;      /* adjust this for zero sum of file */
    WORD    nCmdShow;       /* min, max, or normal state */
    DWORD   pName;          /* name of group */
                            /* these four change interpretation */
    WORD    cxIcon;         /* width of icons */
    WORD    cyIcon;         /* hieght of icons */
    WORD    wIconFormat;    /* planes and BPP in icons */
    WORD    wReserved;      /* This word is no longer used. */

    WORD    cItems;         /* number of items in group */
    WORD    Reserved1;
    DWORD   Reserved2;
    DWORD   rgiItems[1];    /* array of ITEMDEF offsets */
} GROUPDEF, *PGROUPDEF;
typedef GROUPDEF *LPGROUPDEF;

typedef struct tagITEMDEF {
    POINT   pt;             /* location of item icon in group */
    WORD    iIcon;          /* id of item icon */
    WORD    wIconVer;       /* icon version */
    WORD    cbIconRes;      /* size of icon resource */
    WORD    wIconIndex;     /* index of the item icon (not the same as the id) */
    DWORD   pIconRes;       /* offset of icon resource */
    DWORD   pName;          /* offset of name string */
    DWORD   pCommand;       /* offset of command string */
    DWORD   pIconPath;      /* offset of icon path */
} ITEMDEF, *PITEMDEF;
typedef ITEMDEF *LPITEMDEF;


/* the pointers in the above structures are short pointers relative to the
 * beginning of the segments.  This macro converts the short pointer into
 * a long pointer including the proper segment/selector value.        It assumes
 * that its argument is an lvalue somewhere in a group segment, for example,
 * PTR(lpgd->pName) returns a pointer to the group name, but k=lpgd->pName;
 * PTR(k) is obviously wrong as it will use either SS or DS for its segment,
 * depending on the storage class of k.
 */
#define PTR(base, offset) (LPBYTE)((PBYTE)base + offset)

/* PTR2 is used for those cases where a variable already contains an offset
 * (The "case that doesn't work", above)
 */
#define PTR2(lp,offset) ((LPBYTE)MAKELONG(offset,HIWORD(lp)))

/* this macro is used to retrieve the i-th item in the group segment.  Note
 * that this pointer will NOT be NULL for an unused slot.
 */
#define ITEM(lpgd,i) ((LPITEMDEF)PTR(lpgd, lpgd->rgiItems[i]))

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Tag Stuff                                                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

typedef struct _tag
  {
    WORD wID;                   // tag identifier
    WORD dummy1;                // need this for alignment!
    int wItem;                  // (unde the covers 32 bit point!)item the tag belongs to
    WORD cb;                    // size of record, including id and count
    WORD dummy2;                // need this for alignment!
    BYTE rgb[1];
  } PMTAG, FAR * LPPMTAG;

#define PMTAG_MAGIC GROUP_MAGIC

    /* range 8000 - 80FF > global
     * range 8100 - 81FF > per item
     * all others reserved
     */

#define ID_MAINTAIN             0x8000
    /* bit used to indicate a tag that should be kept even if the writer
     * doesn't recognize it.
     */

#define ID_MAGIC                0x8000
    /* data: the string 'TAGS'
     */

#define ID_WRITERVERSION        0x8001
    /* data: string in the form [9]9.99[Z].99
     */

#define ID_APPLICATIONDIR       0x8101
    /* data: ASCIZ string of directory where application may be
     * located.
     * this is defined as application dir rather than default dir
     * since the default dir is explicit in the 3.0 command line and
     * must stay there.  The true "new information" is the application
     * directory.  If not present, search the path.
     */

#define ID_HOTKEY               0x8102
    /* data: WORD hotkey index
     */

#define ID_MINIMIZE             0x8103
    /* data none
     */

#define ID_NEWVDM               0x8104
    /* data none
     */

#define ID_LASTTAG              0xFFFF
    /* the last tag in the file
     */


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  For Icon Extraction                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

typedef struct _MyIconInfo {
    HICON hIcon;
    INT iIconId;
} MYICONINFO, *LPMYICONINFO;

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Function Templates                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/


BOOL  FAR PASCAL IsRemoteDrive(int);
BOOL  FAR PASCAL IsRemovableDrive(int);

int   FAR PASCAL MyMessageBox(HWND, WORD, WORD , LPTSTR , WORD);
BOOL  FAR PASCAL AppInit(HANDLE, LPTSTR , int);
void  FAR PASCAL BuildDescription(LPTSTR, LPTSTR);
WORD  FAR PASCAL ExecProgram(LPTSTR, LPTSTR, LPTSTR, BOOL, DWORD, WORD, BOOL);
void  FAR PASCAL ExecItem(PGROUP,PITEM,BOOL,BOOL);
WORD  FAR PASCAL SelectionType(void);
BOOL  APIENTRY ProgmanCommandProc(HWND, WPARAM, LPARAM);
void  FAR PASCAL WriteINIFile(void);
void  FAR PASCAL ArrangeItems(HWND);
void  FAR PASCAL WriteGroupsSection(void);
LRESULT APIENTRY DDEMsgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY AppIconDDEMsgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY AppDescriptionDDEMsgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY AppWorkingDirDDEMsgProc(HWND, UINT, WPARAM, LPARAM);

BOOL FAR PASCAL IsGroupReadOnly(LPTSTR szGroupKey, BOOL bCommonGroup);
LPGROUPDEF FAR PASCAL LockGroup(HWND);
void  FAR PASCAL UnlockGroup(HWND);
LPITEMDEF FAR PASCAL LockItem(PGROUP, PITEM);
HICON FAR PASCAL GetItemIcon(HWND, PITEM);
HWND  FAR PASCAL LoadGroupWindow(LPTSTR, WORD, BOOL);
PITEM FAR PASCAL CreateNewItem(HWND,LPTSTR,LPTSTR,LPTSTR,LPTSTR,WORD,BOOL,WORD,WORD,HICON,LPPOINT,DWORD);
HWND  FAR PASCAL CreateNewGroup(LPTSTR, BOOL);
void  FAR PASCAL DeleteItem(PGROUP, PITEM);
void  FAR PASCAL DeleteGroup(HWND);
void  FAR PASCAL ChangeGroupTitle(HWND, LPTSTR, BOOL);
void  FAR PASCAL CreateItemIcons(HWND);
void  FAR PASCAL GetItemText(PGROUP,PITEM,LPTSTR,int);
void  FAR PASCAL InvalidateIcon(PGROUP,PITEM);
void  FAR PASCAL ComputeIconPosition(PGROUP,POINT,LPRECT,LPRECT,LPTSTR);
void  FAR PASCAL CalcGroupScrolls(HWND);
BOOL  FAR PASCAL GroupCheck(PGROUP);
void  FAR PASCAL UnloadGroupWindow(HWND);
void  FAR PASCAL NukeIconBitmap(PGROUP pGroup);

WORD  FAR PASCAL GroupFlag(PGROUP, PITEM, WORD);
WORD  FAR PASCAL GetGroupTag(PGROUP, PITEM, WORD, LPTSTR, WORD);

VOID FAR PASCAL LoadAllGroups(VOID);

PITEM FAR PASCAL DuplicateItem(PGROUP,PITEM,PGROUP,LPPOINT);

void  FAR PASCAL GetItemCommand(PGROUP,PITEM,LPTSTR,LPTSTR);

VOID  APIENTRY RegisterDDEClasses(HANDLE);
INT MyDwordAlign(INT);

LRESULT APIENTRY GroupWndProc(HWND , UINT, WPARAM, LPARAM);
LRESULT APIENTRY ProgmanWndProc(HWND , UINT , WPARAM,  LPARAM );
WORD  FAR PASCAL MyDialogBox(WORD, HWND , DLGPROC );

INT_PTR APIENTRY ChooserDlgProc(HWND , UINT , WPARAM , LPARAM );
INT_PTR APIENTRY BrowseDlgProc(HWND, UINT , WPARAM , LPARAM );
INT_PTR APIENTRY RunDlgProc(HWND , UINT , WPARAM , LPARAM );
INT_PTR APIENTRY ExitDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY IconDlgProc(HWND , UINT , WPARAM , LPARAM );
INT_PTR APIENTRY NewItemDlgProc(HWND , UINT , WPARAM , LPARAM );
INT_PTR APIENTRY NewGroupDlgProc(HWND , UINT , WPARAM , LPARAM );
INT_PTR APIENTRY MoveItemDlgProc(HWND , UINT , WPARAM , LPARAM );
INT_PTR APIENTRY CopyItemDlgProc(HWND , UINT , WPARAM , LPARAM );
INT_PTR APIENTRY EditItemDlgProc(HWND , UINT , WPARAM , LPARAM );
INT_PTR APIENTRY EditGroupDlgProc(HWND, UINT , WPARAM , LPARAM );
INT_PTR APIENTRY AboutDlgProc(HWND , UINT , WPARAM , LPARAM );
INT_PTR APIENTRY HotKeyDlgProc(HWND , UINT , WPARAM , LPARAM );
BOOL  APIENTRY ShutdownDialog(HANDLE, HWND);
INT_PTR APIENTRY NewLogoffDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY UpdateGroupsDlgProc(HWND, UINT, WPARAM, LPARAM);
VOID  APIENTRY HandleDosApps
    (
    LPTSTR sz  // Full path sans arguments.
    );
DWORD  APIENTRY ValidatePath
    (
    HWND hDlg,
    LPTSTR szPath,        // Path to item
    LPTSTR szExePath,     // Path to associated exe.
    LPTSTR szDir          // Path to working directory.
    );
VOID APIENTRY GetDirectoryFromPath
    (
    LPTSTR szFilePath,    // Full path to a file.
    LPTSTR szDir          // Directory returned in here, the buffer is assumed
                        // to be as big as szFilePath.
    );
VOID APIENTRY GetFilenameFromPath
    (
    LPTSTR szPath,
    LPTSTR szFilename
    );
void APIENTRY TagExtension
    (
    LPTSTR szPath,
    UINT cbPath
    );
void APIENTRY StripArgs
    (
    LPTSTR szCmmdLine     // A command line.
    );
BOOL APIENTRY ValidPathDrive
    (
    LPTSTR lpstr
    );

LRESULT APIENTRY MessageFilter(int , WPARAM , LPARAM) ;

BOOL APIENTRY SaveGroup(HWND, BOOL);
BOOL SaveGroupsContent(BOOL);
BOOL IsReadOnly(LPTSTR);
VOID FAR PASCAL StartupGroup(HWND hwnd);
VOID APIENTRY PMHelp(HWND);
VOID FAR PASCAL RemoveLeadingSpaces(LPTSTR sz);
VOID FAR PASCAL BringItemToTop(PGROUP pGroup, PITEM pItem, BOOL fUpdate);
VOID APIENTRY SaveRecentFileList (HWND hwnd, LPTSTR szCurrentFile, WORD idControl);

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Defines                                                                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#define CharSizeOf(x)   (sizeof(x) / sizeof(*x))
#define ByteCountOf(x)  ((x) * sizeof(TCHAR))

#define CITEMSMAX 50
//
// CreateNewItem flags
//
#define CI_ACTIVATE            0x1
#define CI_SET_DOS_FULLSCRN    0x2
#define CI_NO_ASSOCIATION      0x4
#define CI_SEPARATE_VDM        0x8

/* magic number for .GRP file validation
 */
#define GROUP_MAGIC             0x43434D50L  /* 'PMCC' */
#define GROUP_UNICODE           0x43554D50L  /* 'PMUC' */

#define MAXTITLELEN             50        /* Length of MessageBox titles */
#define MAXMESSAGELEN           512        /* Length of MessageBox messages */

#define MAXITEMNAMELEN          40
//#define MAXITEMPATHLEN          64+16+48 /* Path + 8.3 + Drive(colon) + arguments */
#define MAXITEMPATHLEN          MAX_PATH - 1 // -1 for backward compatibility
                                             // with shell32.dll
#define MAXGROUPNAMELEN         30

#define NSLOTS                  16        /* initial number of items entries */

#define PATH_INVALID            0
#define PATH_INVALID_OK         1
#define PATH_VALID              2

#define CCHGROUP                5 // length of the string "Group"
#define CCHCOMMONGROUP          6 // length of the string "CGroup"

#define CGROUPSMAX              40      // The max number of groups allowed.

#define TYPE_ITEM               0
#define TYPE_PERSGROUP          1
#define TYPE_COMMONGROUP        2

/* Resource Numbers */
#define PROGMANICON             1
#define DOSAPPICON              2
#define WORDICON                3
#define SHEETICON               4
#define DATAICON                5
#define COMMICON                6
#define ITEMICON                7
#define PERSGROUPICON           8
#define COMMGROUPICON           9
#define WINDOWSICON             10                /* Should be large ? */
#define MSDOSICON               11
#define PMACCELS                1004
#define PROGMANMENU             1005
#define GROUPICON               146

#define DOSAPPICONINDEX         1
#define ITEMICONINDEX           6

#define ITEMLISTBOX             1

#define GWLP_PGROUP             0       // Used in the Group window frames
#define GWL_EXITING             0       // Used in the main window frame

#define DRAG_SWP                1
#define DRAG_COPY               2

/* DDE Messaging Stuff */
#define ACK_POS                 0x8000
#define ACK_NEG                 0x0000

// message sent to indicate another instance has been exec'd
#define WM_EXECINSTANCE         (WM_USER+100)

// message sent to reload a group
#define WM_RELOADGROUP          (WM_USER+101)
// message sent to delete a group
#define WM_UNLOADGROUP	    	(WM_USER+102)

// Lock errors
#define LOCK_LOWMEM             1
#define LOCK_FILECHANGED        2

// Binary type defins
#define BINARY_TYPE_DEFAULT     1
#define BINARY_TIMEOUT_DEFAULT  500

// Recent file list defines
#define MYCBN_SELCHANGE     (WM_USER+5)

#define INIT_MAX_FILES 4
#define FILES_KEY  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager\\Recent File List"
#define MAXFILES_ENTRY L"Max Files"
#define FILE_ENTRY L"File%lu"


/* Menu Command Defines */
#define IDM_FILE                0
#define IDM_NEW                 101
#define IDM_OPEN                102
#define IDM_MOVE                103
#define IDM_COPY                104
#define IDM_DELETE              105
#define IDM_PROPS               106
#define IDM_RUN                 107
#define IDM_EXIT                108
#define IDM_SAVE                109
#define IDM_SHUTDOWN            110
#define IDM_OPTIONS             1
#define IDM_AUTOARRANGE         201
#define IDM_MINONRUN            202
#define IDM_HOTKEY              203
#define IDM_SAVESETTINGS        204
#define IDM_SAVENOW             205
#define IDM_ANSIGROUPS          206
#define IDM_WINDOW              2
#define IDM_CASCADE             301
#define IDM_TILE                302
#define IDM_ARRANGEICONS        303

#define IDM_CHILDSTART          310

#define IDM_HELP                3
#define IDM_HELPINDEX           401
#define IDM_HELPHELP            402
#define IDM_ABOUT               403
#define IDM_HELPSEARCH          404


/* StringTable Defines */
#define IDS_APPTITLE            1
#define IDS_PMCLASS             2
#define IDS_GROUPCLASS          3
#define IDS_ITEMCLASS           4
#define IDS_DATAFILE            5
#define IDS_SETTINGS            7
#define IDS_CONFIRMDELTITLE     40        /* Must be > 32 */
#define IDS_CONFIRMDELITEMMSG   41
#define IDS_CONFIRMDELGROUPMSG  42
#define IDS_NOICONSTITLE        50
#define IDS_NOICONSMSG          51
#define IDS_BADPATHTITLE        52
#define IDS_BADPATHMSG          53
#define IDS_NETPATHTITLE        54
#define IDS_REMOVEPATHTITLE     55
#define IDS_PATHWARNING         56
#define IDS_EXECERRTITLE        60
#define IDS_UNKNOWNMSG          61
#define IDS_NOMEMORYMSG         62
#define IDS_FILENOTFOUNDMSG     63
#define IDS_MANYOPENFILESMSG    64
#define IDS_NOASSOCMSG          65
#define IDS_MULTIPLEDSMSG       66
#define IDS_ASSOCINCOMPLETE     67
#define IDS_COPYDLGTITLE        70
#define IDS_COPYDLGTITLE1       71
#define IDS_GROUPS              72
#define IDS_NOGRPFILE           73
#define IDS_LOWMEM              74
#define IDS_BADFILE             75
#define IDS_CANTWRITEGRP        76
#define IDS_GROUPFILEERR        77
#define IDS_GRPISRO             78
#define IDS_EXTRACTERROR        79
#define IDS_EEGROUPRO           80
#define IDS_CANTWRITEGRPS       81
#define IDS_OOMEXITTITLE        110
#define IDS_OOMEXITMSG          111
#define IDS_GROUPRO             112
#define IDS_CANTRENAMETITLE     113
#define IDS_CANTRENAMEMSG       114
#define IDS_TOOMANYITEMS        115
#define IDS_OS2APPMSG           116
#define IDS_NEWWINDOWSMSG       117
#define IDS_PMODEONLYMSG        118
#define IDS_ALREADYLOADED       119
#define IDS_STARTUP             120
#define IDS_PLUS                121
#define IDS_GRPHASCHANGED       122
#define IDS_NONE                123
#define IDS_EXIT                124
#define IDS_DUPHOTKEYTTL        125
#define IDS_DUPHOTKEYMSG        126
#define IDS_BROWSE              127
#define IDS_NEWITEMPROGS        128
#define IDS_PROPERTIESPROGS     129
#define IDS_CHNGICONPROGS       130
#define IDS_TOOMANYGROUPS       131
#define IDS_ACCESSDENIED        133
#define IDS_DDEFAIL             134
#define IDS_LOWMEMONINIT        135
#define IDS_PIFINIFILE          136
#define IDS_PIFSECTION          137
#define IDS_EXECSETUP           138
#define IDS_WINHELPERR          139
#define IDS_PIFADDINFO          140

#define IDS_BADPATHMSG2	        141
#define IDS_BADPATHMSG3	        142
#define IDS_LOWMEMONEXIT        143
#define IDS_WININIERR           144
#define IDS_STARTUPERR          145
#define IDS_CMDLINEERR          146
#define IDS_ITEMINGROUP         147
#define IDS_LOWMEMONEXTRACT     148
#define IDS_DEFICONSFILE        149
#define IDS_COMPRESSEDEXE       150
#define IDS_INVALIDDLL          151
#define IDS_SHAREERROR          152
#define IDS_CANTWRITEGRPDISK	153
#define IDS_COMMDLGLOADERR      154
#define IDS_NOICONSMSG1	    	155
#define IDS_NOGRPFILE2          156

#define IDS_REGISTRYERROR       164
#define IDS_ERRORDELETEGROUP    165
#define IDS_LOGOFFERROR         166

#define IDS_COMMONGRPSUFFIX     167
#define IDS_COMMONGROUPPROP     168
#define IDS_COMMONGROUPERR      169
#define IDS_NOCOMMONGRPS        170

#define IDS_NO_PERMISSION_SHUTDOWN 171
#define IDS_SHUTDOWN_MESSAGE    172


#define IDS_DEFAULTSTARTUP      173
#define IDS_TOOMANYCOMMONGROUPS 174

#define IDS_LOGOFF              175
#define IDS_SHUTDOWN            176

#define IDS_MSGBOXSTR1          177
#define IDS_MSGBOXSTR2          178

#define IDS_INSUFFICIENTQUOTA   179

#define IDS_ANSIGROUPSMENU      180

//#ifdef JAPAN
//#define IDS_BADPORTPATHTITLE    1102
//#define IDS_BADPORTPATHMSG      1103
//#endif //JAPAN

#include "pmdlg.h"
#include "pmreg.h"
#include "notify.h"

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global Externs                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

extern BOOL            UserIsAdmin;
extern BOOL            AccessToCommonGroups;
extern BOOL            bLoadIt;
extern BOOL            bMinOnRun;
extern BOOL            bArranging;
extern BOOL            bAutoArrange;
extern BOOL            bAutoArranging;
extern BOOL            bExitWindows;
extern BOOL            bSaveSettings;
extern BOOL            bIconTitleWrap;
extern BOOL            bScrolling;
extern BOOL            bLoadEvil;
extern BOOL            bMove;
extern BOOL            bInDDE;
extern BOOL            fInExec;
extern BOOL            fNoRun;
extern BOOL            fNoClose;
extern BOOL            fNoSave;
extern BOOL            fNoFileMenu;
extern BOOL            fLowMemErrYet;
extern BOOL            fExiting;
extern BOOL            fErrorOnExtract;
extern BOOL	           bFrameSysMenu;

extern TCHAR            szNULL[];
extern TCHAR            szProgman[];
extern TCHAR            szTitle[];
//
// Program Manager's Settings keys
//
extern TCHAR            szWindow[];
extern TCHAR            szOrder[];
extern TCHAR            szAnsiOrder[];
extern TCHAR            szStartup[];
extern TCHAR            szAutoArrange[];
extern TCHAR            szSaveSettings[];
extern TCHAR            szMinOnRun[];
extern TCHAR            szFocusOnCommonGroup[];

extern TCHAR            szMessage[MAXMESSAGELEN+1];
extern TCHAR            szNameField[MAXITEMPATHLEN+1];
extern TCHAR            szPathField[MAXITEMPATHLEN+1];
extern TCHAR            szIconPath[MAXITEMPATHLEN+1];
extern TCHAR            szDirField[];
extern TCHAR            szOriginalDirectory[];
extern TCHAR            szWindowsDirectory[];
extern TCHAR            szOOMExitMsg[64];
extern TCHAR            szOOMExitTitle[32];

extern HANDLE          hAccel;
extern HINSTANCE       hAppInstance;
extern HANDLE          hCommdlg;

extern HICON           hDlgIcon;
extern HICON           hItemIcon;
extern HICON           hProgmanIcon;
extern HICON           hGroupIcon;
extern HICON           hCommonGrpIcon;
extern HICON           hIconGlobal;


extern HWND            hwndProgman;
extern HWND            hwndMDIClient;

extern HBRUSH          hbrWorkspace;

extern int            nGroups;
extern int            dyBorder;
extern int            iDlgIconId;
extern int            iDlgIconIndex;
extern int            cxIcon;
extern int            cyIcon;
extern int            cxIconSpace;
extern int            cyIconSpace;
extern int            cxArrange;
extern int            cyArrange;
extern int            cxOffset;
extern int            cyOffset;

extern WORD         wPendingHotKey;

extern DWORD        dwDDEAppId;
extern DWORD        dwEditLevel;
extern WORD         wLockError;
extern UINT         uiActivateShellWindowMessage;
extern UINT         uiConsoleWindowMessage;
extern UINT         uiSaveSettingsMessage; // for User Profile Editor: upedit.exe

extern PGROUP       pFirstGroup;
extern PGROUP       pCurrentGroup;
extern PGROUP       pActiveGroup;
extern PGROUP       *pLastGroup;
extern PGROUP       pExecingGroup;

extern PITEM        pExecingItem;

extern RECT         rcDrag;
extern HWND         hwndDrag;

extern WORD         wNewSelection;

extern HFONT        hFontTitle;

extern UINT         uiHelpMessage;                // stuff for help
extern UINT         uiBrowseMessage;              // stuff for help
extern WORD         wMenuID;
extern HANDLE       hSaveMenuHandle;                /*Save hMenu into one variable*/
extern WORD         wSaveFlags;                                /*Save flags into another*/
extern HANDLE       hSaveMenuHandleAroundSendMessage;/*Save hMenu into one variable*/
extern WORD         wSaveFlagsAroundSendMessage;     /*Save flags into another*/
extern WORD         wSaveMenuIDAroundSendMessage;

extern DWORD        dwContext;
extern HHOOK        hhkMsgFilter;
extern TCHAR        szProgmanHelp[];

extern BOOL         bUseANSIGroups;

extern PSECURITY_ATTRIBUTES pSecurityAttributes;
extern PSECURITY_ATTRIBUTES pAdminSecAttr;

extern BOOL bDisableDDE;
