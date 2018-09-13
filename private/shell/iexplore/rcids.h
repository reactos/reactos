//---------------------------------------------------------------------------
// Defines for the rc file.
//---------------------------------------------------------------------------

//
// IE 2.0 icon ids
//
// NOTE for IE 2.0 compatibilty these icons MUST be in this order
//
#define RES_ICO_FRAME           32528
#define RES_ICO_HTML            32529
#define RES_ICO_EXTRA_1         32530
#define RES_ICO_EXTRA_2         32531
#define RES_ICO_EXTRA_3         32532
#define RES_ICO_EXTRA_4         32533
#define IDI_APPEARANCE          32534
#define IDI_ADVANCED            32535
#undef IDI_HOMEPAGE
#define IDI_HOMEPAGE            32536
#define IDI_GOTOURL             32537
#define IDI_FINDTEXT            32538
#define IDI_UNKNOWN_FILETYPE    32539
#define RES_ICO_JPEG            32540
#define RES_ICO_GIF             32541
#define IDI_INTERNET            32542
#define RES_ICON_FOLDER_OPEN    32543
#define RES_ICON_FOLDER_CLOSED  32544
#define RES_ICON_URL_FILE       32545
#define IDI_SECURITY            32546
#define RES_ICO_NOICON          32547
#define RES_ICO_FINDING         32548
#define RES_ICO_CONNECTING      32549
#define RES_ICO_ACCESSING       32550
#define RES_ICO_RECEIVING       32551
#define IDI_NEWS                32552
#define IDI_VRML                32553
#define IDI_MHTMLFILE           32554


// String IDS which are actually used:
#define IDS_COMPATMODEWARNING        700
#define IDS_COMPATMODEWARNINGTITLE   701


// Commmand ID
#define FCIDM_FIRST             FCIDM_GLOBALFIRST
#define FCIDM_LAST              FCIDM_BROWSERLAST

//---------------------------------------------------------------------------
#define FCIDM_BROWSER_FILE      (FCIDM_BROWSERFIRST+0x0020)
#define FCIDM_FILECLOSE         (FCIDM_BROWSER_FILE+0x0001)
#define FCIDM_PREVIOUSFOLDER    (FCIDM_BROWSER_FILE+0x0002)
#define FCIDM_ENTER		(FCIDM_BROWSER_FILE+0x0003)

// these aren't real menu commands, but they map to accelerators or other things
#define FCIDM_NEXTCTL           (FCIDM_BROWSER_FILE+0x0010)
#define FCIDM_DROPDRIVLIST      (FCIDM_BROWSER_FILE+0x0011)

//---------------------------------------------------------------------------
#define FCIDM_VIEWTOOLBAR     (FCIDM_BROWSERFIRST + 0x0010)
#define FCIDM_VIEWSTATUSBAR   (FCIDM_BROWSERFIRST + 0x0011)
#define FCIDM_VIEWOPTIONS     (FCIDM_BROWSERFIRST + 0x0012)

//---------------------------------------------------------------------------
#define FCIDM_BROWSER_HELP      (FCIDM_BROWSERFIRST+0x0100)

#define FCIDM_HELPSEARCH        (FCIDM_BROWSER_HELP+0x0001)
#define FCIDM_HELPABOUT         (FCIDM_BROWSER_HELP+0x0002)

//----------------------------------------------------------------
#define FCIDM_BROWSER_EXPLORE   (FCIDM_BROWSERFIRST + 0x0110)
#define FCIDM_NAVIGATEBACK      (FCIDM_BROWSER_EXPLORE+0x0001)
#define FCIDM_NAVIGATEFORWARD   (FCIDM_BROWSER_EXPLORE+0x0002)
#define FCIDM_BROWSEROPTIONS    (FCIDM_BROWSER_EXPLORE+0x0003)
#define FCIDM_RECENTMENU        (FCIDM_BROWSER_EXPLORE+0x0010)
#define FCIDM_RECENTFIRST       (FCIDM_BROWSER_EXPLORE+0x0011)
#define FCIDM_RECENTLAST        (FCIDM_BROWSER_EXPLORE+0x0050)
#define FCIDM_FAVORITES         (FCIDM_BROWSER_EXPLORE+0x0052)
#define FCIDM_ADDTOFAVORITES    (FCIDM_BROWSER_EXPLORE+0x0053)
#define FCIDM_FAVORITEFIRST     (FCIDM_BROWSER_EXPLORE+0x0055)
#define FCIDM_FAVORITELAST      (FCIDM_BROWSER_EXPLORE+0x0100)
#define FCIDM_FAVORITE_ITEM     (FCIDM_FAVORITEFIRST + 0)
#define FCIDM_FAVORITECMDFIRST  (FCIDM_FAVORITES)
#define FCIDM_FAVORITECMDLAST   (FCIDM_FAVORITELAST)

#define MH_POPUPS	700
#define MH_ITEMS	(800-FCIDM_FIRST)
#define MH_TTBASE               (MH_ITEMS - (FCIDM_LAST - FCIDM_FIRST))
#define IDS_TT_PREVIOUSFOLDER   (MH_TTBASE+FCIDM_PREVIOUSFOLDER)
#define IDS_TT_NAVIGATEBACK             (MH_TTBASE + FCIDM_NAVIGATEBACK)
#define IDS_TT_NAVIGATEFORWARD          (MH_TTBASE + FCIDM_NAVIGATEFORWARD)
#define IDS_TT_FAVORITES             (MH_TTBASE + FCIDM_FAVORITES)
#define IDS_TT_ADDTOFAVORITES          (MH_TTBASE + FCIDM_ADDTOFAVORITES)

// Define string ids that go into resource file
#define IDS_MH_DRIVELIST        (MH_ITEMS+FCIDM_DRIVELIST)
#define IDS_MH_MENU_FILE        (MH_ITEMS+FCIDM_MENU_FILE)
#define IDS_MH_MENU_EXPLORE     (MH_ITEMS+FCIDM_MENU_EXPLORE)
#define IDS_MH_MENU_HELP        (MH_ITEMS+FCIDM_MENU_HELP)
#define IDS_MH_FILECLOSE        (MH_ITEMS+FCIDM_FILECLOSE)
#define IDS_MH_PREVIOUSFOLDER   (MH_ITEMS+FCIDM_PREVIOUSFOLDER)
#define IDS_MH_HELPSEARCH       (MH_ITEMS+FCIDM_HELPSEARCH)
#define IDS_MH_HELPABOUT        (MH_ITEMS+FCIDM_HELPABOUT)
#define IDS_MH_NAVIGATEBACK	(MH_ITEMS+FCIDM_NAVIGATEBACK)
#define IDS_MH_NAVIGATEBACK     (MH_ITEMS+FCIDM_NAVIGATEBACK)
#define IDS_MH_NAVIGATEFORWARD  (MH_ITEMS+FCIDM_NAVIGATEFORWARD)
#define IDS_MH_RECENTMENU       (MH_ITEMS+FCIDM_RECENTMENU)
#define IDS_MH_MENU_FAVORITES   (MH_ITEMS+FCIDM_MENU_FAVORITES)
#define IDS_MH_FAVORITES        (MH_ITEMS+FCIDM_FAVORITES)
#define IDS_MH_ADDTOFAVORITES   (MH_ITEMS+FCIDM_ADDTOFAVORITES)

#define IDS_NAVIGATEBACKTO  720
#define IDS_NAVIGATEFORWARDTO 721

#define IDS_OPTIONS	722
#define IDS_TITLE	723
#define IDS_ERROR_GOTO	724

#define IDS_NONE        725
#define IDS_NAME        726     // Used for NAME member function for fram programmability

// Accelerator ID
#define ACCEL_MERGE	0x100

// Menu ID
#define MENU_TEMPLATE	0x100
#define MENU_FAVORITES  0x101

