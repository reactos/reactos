/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGRESID.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  Resource identifiers for the Registry Editor.
*
*******************************************************************************/

#ifndef _INC_REGRESID
#define _INC_REGRESID

#define HEXEDIT_CLASSNAME               TEXT("HEX")

//
//
//

#define IDD_REGEXPORT                   100
#define IDD_REGPRINT                    108

#define IDC_FIRSTREGCOMMDLGID           1280
#define IDC_RANGEALL                    1280
#define IDC_RANGESELECTEDPATH           1281
#define IDC_SELECTEDPATH                1282
#define IDC_EXPORTRANGE                 1283
#define IDC_LASTREGCOMMDLGID            1283

//
//
//

#define IDD_EDITSTRINGVALUE             102
#define IDD_EDITBINARYVALUE             103
#define IDD_EDITDWORDVALUE              111

#define IDC_VALUENAME                   1000
#define IDC_VALUEDATA                   1001
#define IDC_HEXADECIMAL                 1002
#define IDC_DECIMAL                     1003

//
//
//

#define IDD_REGCONNECT                  104

#define IDC_REMOTENAME                  1100
#define IDC_BROWSE                      1101

//
//
//

#define IDD_REGPRINTABORT               105

//
//  Dialog box for the Edit-> Find... menu option.
//

#define IDD_REGFIND                     106

#define IDC_FINDWHAT                    1150
#define IDC_WHOLEWORDONLY               1151
//  NOTE: The flags IDC_FOR* must be consecutive.
#define IDC_FORKEYS                     1152
#define IDC_FORVALUES                   1153
#define IDC_FORDATA                     1154
#define IDC_GROUPBOX                    1160

#define IDD_REGDISCONNECT               107
#define IDC_REMOTELIST                  1175

//
//  Dialog box for the find abort.
//
#define IDD_REGFINDABORT                109

//
// Dialog boxes for adding/removing a favorite
//
#define IDD_ADDFAVORITE			112
#define IDC_FAVORITENAME		1190
#define IDD_REMFAVORITE			113
#define IDC_FAVORITES			1191

//
//  Dialog box for Import Registry File progress display.
//

#define IDD_REGPROGRESS                 110

#define IDC_FILENAME                    100
#define IDC_PROGRESSBAR                 101

//
//  Menu resource identifiers.
//

#define IDM_REGEDIT                     103
#define IDM_KEY_CONTEXT                 104
#define IDM_VALUE_CONTEXT               105
#define IDM_VALUELIST_NOITEM_CONTEXT    106
#define IDM_COMPUTER_CONTEXT            107

//
//  HexEdit context menu identifier and items.  The IDKEY_* identifier
//  correspond to the WM_CHAR message that it corresponds to.  For example,
//  IDKEY_COPY would send a control-c to the HexEdit_OnChar routine.
//

#define IDM_HEXEDIT_CONTEXT             108

#define IDKEY_COPY                      3
#define IDKEY_PASTE                     22
#define IDKEY_CUT                       24
#define ID_SELECTALL                    0x0400

//
//  Popup menu item identifiers.  Used to determine the context menu help
//  string.
//

#define ID_FIRSTMENUPOPUPITEM           0x0200
#define ID_LASTMENUPOPUPITEM            0x027F

#define IDMP_REGISTRY                   0x0200
#define IDMP_EDIT                       0x0201
#define IDMP_VIEW                       0x0202
#define IDMP_HELP                       0x0203
#define IDMP_NEW                        0x0204
#define IDMP_FAVORITES			0x0205

//
//  Main menu items.  If any of these items are selected from a context menu,
//  they will be automatically routed to the main window's command handler.
//

#define ID_FIRSTMAINMENUITEM            0x0280
#define ID_LASTMAINMENUITEM             0x02FF

//  Following are really keyboard accelerators.
#define ID_CYCLEFOCUS                   (ID_FIRSTMAINMENUITEM + 0x0000)

//  IMPORTANT:  Do not change the position of this identifier.  If Regedit is
//  already running and Regedit is then invoked through its commandline
//  interface, then the second instance will send a WM_COMMAND message with this
//  identifier to force a refresh.
#define ID_REFRESH                      (ID_FIRSTMAINMENUITEM + 0x0008)

#define ID_CONNECT                      (ID_FIRSTMAINMENUITEM + 0x0011)
#define ID_IMPORTREGFILE                (ID_FIRSTMAINMENUITEM + 0x0012)
#define ID_EXPORTREGFILE                (ID_FIRSTMAINMENUITEM + 0x0013)
#define ID_PRINT                        (ID_FIRSTMAINMENUITEM + 0x0014)
#define ID_EXIT                         (ID_FIRSTMAINMENUITEM + 0x0015)
#define ID_FIND                         (ID_FIRSTMAINMENUITEM + 0x0016)
#define ID_NEWKEY                       (ID_FIRSTMAINMENUITEM + 0x0017)
#define ID_NEWSTRINGVALUE               (ID_FIRSTMAINMENUITEM + 0x0018)
#define ID_NEWBINARYVALUE               (ID_FIRSTMAINMENUITEM + 0x0019)
#define ID_EXECCALC                     (ID_FIRSTMAINMENUITEM + 0x001A)
#define ID_ABOUT                        (ID_FIRSTMAINMENUITEM + 0x001B)
#define ID_STATUSBAR                    (ID_FIRSTMAINMENUITEM + 0x001C)
#define ID_SPLIT                        (ID_FIRSTMAINMENUITEM + 0x001E)
#define ID_FINDNEXT                     (ID_FIRSTMAINMENUITEM + 0x001F)
#define ID_HELPTOPICS                   (ID_FIRSTMAINMENUITEM + 0x0020)
#define ID_NETSEPARATOR                 (ID_FIRSTMAINMENUITEM + 0x0021)
#define ID_NEWDWORDVALUE                (ID_FIRSTMAINMENUITEM + 0x0022)
#define ID_COPYKEYNAME			(ID_FIRSTMAINMENUITEM + 0x0023)

//
//  Dual menu items.  The routing of these items depends on whether it was
//  selected from the main menu or from a context menu.
//

#define ID_FIRSTDUALMENUITEM            0x0300
#define ID_LASTDUALMENUITEM             0x037F

#define ID_DISCONNECT                   (ID_FIRSTDUALMENUITEM + 0x0000)

//
//  Context menu items.  If any of these items are selected from the main menu,
//  they will be automatically routed to the focus pane's command handler.
//

#define ID_FIRSTCONTEXTMENUITEM         0x0380
#define ID_LASTCONTEXTMENUITEM          0x03FF

//  Following are really keyboard accelerators.
#define ID_CONTEXTMENU                  (ID_FIRSTCONTEXTMENUITEM + 0x0000)

#define ID_MODIFY                       (ID_FIRSTCONTEXTMENUITEM + 0x0010)
#define ID_DELETE                       (ID_FIRSTCONTEXTMENUITEM + 0x0011)
#define ID_RENAME                       (ID_FIRSTCONTEXTMENUITEM + 0x0012)
#define ID_TOGGLE                       (ID_FIRSTCONTEXTMENUITEM + 0x0013)
#define ID_SENDTOPRINTER                (ID_FIRSTCONTEXTMENUITEM + 0x0014)

//
//  The following are new features added by BruceGr
//
#define ID_FIRSTNEWIDENTIFIER		0x0500
#define ID_REMOVEFAVORITE		(ID_FIRSTNEWIDENTIFIER + 0x0000)
#define ID_ADDTOFAVORITES		(ID_FIRSTNEWIDENTIFIER + 0x0001)

//
//  String resource identifiers.
//

#define IDS_REGEDIT                     16
#define IDS_NAMECOLUMNLABEL             17
#define IDS_DATACOLUMNLABEL             18
#define IDS_COMPUTER                    19
#define IDS_DEFAULTVALUE                20
//  #define IDS_EMPTYSTRING                 21
#define IDS_EMPTYBINARY                 22
#define IDS_NEWKEYNAMETEMPLATE          23
#define IDS_NEWVALUENAMETEMPLATE        24
#define IDS_COLLAPSE                    25
#define IDS_MODIFY                      26
#define IDS_VALUENOTSET                 27
#define IDS_HELPFILENAME                28
#define IDS_DWORDDATAFORMATSPEC         29
#define IDS_INVALIDDWORDDATA            30
#define IDS_TYPECOLUMNLABEL             31

#define IDS_IMPORTREGFILETITLE          32
#define IDS_EXPORTREGFILETITLE          33
#define IDS_REGIMPORTFILEFILTER         34
#define IDS_REGFILEDEFEXT               35
#define IDS_CONFIRMIMPFILE              36
#define IDS_REGEXPORTFILEFILTER         37

#define IDS_REGEDITDISABLED             40
#define IDS_SEARCHEDTOEND               41
#define IDS_COMPUTERBROWSETITLE         42
#define IDS_NOFILESPECIFIED             43

#define IDS_CONFIRMDELKEYTEXT           48
#define IDS_CONFIRMDELKEYTITLE          49
#define IDS_CONFIRMDELVALMULTITEXT      50
#define IDS_CONFIRMDELVALTITLE          51
#define IDS_CONFIRMDELVALTEXT           52

#define IDS_RENAMEKEYERRORTITLE         64
#define IDS_RENAMEPREFIX                65              //  Reserved
#define IDS_RENAMEKEYOTHERERROR         66
#define IDS_RENAMEKEYTOOLONG            67
#define IDS_RENAMEKEYEXISTS             68
#define IDS_RENAMEKEYBADCHARS           69
#define IDS_RENAMEKEYEMPTY              70

#define IDS_RENAMEVALERRORTITLE         72
#define IDS_RENAMEVALOTHERERROR         73
#define IDS_RENAMEVALEXISTS             74
#define IDS_RENAMEVALEMPTY              75

#define IDS_DELETEKEYERRORTITLE         80
#define IDS_DELETEPREFIX                81              //  Reserved
#define IDS_DELETEKEYDELETEFAILED       82

#define IDS_DELETEVALERRORTITLE         88
#define IDS_DELETEVALDELETEFAILED       89

#define IDS_OPENKEYERRORTITLE           96
#define IDS_OPENKEYCANNOTOPEN           97

#define IDS_EDITVALERRORTITLE           112
#define IDS_EDITPREFIX                  113             //  Reserved
#define IDS_EDITVALCANNOTREAD           114
#define IDS_EDITVALCANNOTWRITE          115

#define IDS_IMPFILEERRSUCCESS           128
#define IDS_IMPFILEERRFILEOPEN          129
#define IDS_IMPFILEERRFILEREAD          130
#define IDS_IMPFILEERRREGOPEN           131
#define IDS_IMPFILEERRREGSET            132
#define IDS_IMPFILEERRFORMATBAD         133
#define IDS_IMPFILEERRVERBAD            134

#define IDS_EXPFILEERRSUCCESS           136
#define IDS_EXPFILEERRBADREGPATH        137
#define IDS_EXPFILEERRFILEOPEN          138
#define IDS_EXPFILEERRREGOPEN           139
#define IDS_EXPFILEERRREGENUM           140
#define IDS_EXPFILEERRFILEWRITE         141

#define IDS_PRINTERRNOMEMORY            144
#define IDS_PRINTERRPRINTER             145

#define IDS_ERRINVALIDREGPATH           148

#define IDS_CONNECTERRORTITLE           152
#define IDS_CONNECTNOTLOCAL             153
#define IDS_CONNECTBADNAME              154
#define IDS_CONNECTROOTFAILED           155
#define IDS_CONNECTACCESSDENIED         156

#define IDS_NEWKEYERRORTITLE            160
#define IDS_NEWKEYPARENTOPENFAILED      161
#define IDS_NEWKEYCANNOTCREATE          162
#define IDS_NEWKEYNOUNIQUE              163

#define IDS_NEWVALUEERRORTITLE          168
#define IDS_NEWVALUECANNOTCREATE        169
#define IDS_NEWVALUENOUNIQUE            170

#define IDS_FAVORITEEXISTS		171
#define	IDS_FAVORITEERROR		172
#define IDS_FAVORITE			173

//  The range IDS_FIRSTMENUPOPUPITEM through IDS_LASTMENUPOPUPITEM is reserved
//  for context menu help.  This must match up with ID_FIRSTMENUPOPUPITEM
//  through ID_LASTMENUPOPUPITEM.
#define IDS_FIRSTMENUPOPUPITEM          ID_FIRSTMENUPOPUPITEM
#define IDS_LASTMENUPOPUPITEM           ID_LASTMENUPOPUPITEM

//  The range IDS_FIRSTMAINMENUITEM through IDS_LASTMAINMENUITEM is reserved for
//  context menu help.  This must match up with ID_FIRSTMAINMENUITEM through
//  ID_LASTMAINMENUITEM.

#define IDS_FIRSTMAINMENUITEM           ID_FIRSTMAINMENUITEM
#define IDS_LASTMAINMENUITEM            ID_LASTMAINMENUITEM

//  The range IDS_FIRSTCONTEXTMENUITEM through IDS_LASTCONTEXTMENUITEM is
//  reserved for context menu help.  This must match up with
//  ID_FIRSTCONTEXTMENUITEM through ID_LASTCONTEXTMENUITEM.

#define IDS_FIRSTCONTEXTMENUITEM        ID_FIRSTCONTEXTMENUITEM
#define IDS_LASTCONTEXTMENUITEM         ID_LASTCONTEXTMENUITEM

//  The range IDS_FIRSTDUALMENUITEM through IDS_LASTDUALMENUITEM is reserved for
//  context menu help.  This must match up with ID_FIRSTDUALMENUITEM through
//  ID_LASTDUALMENUITEM.
#define IDS_FIRSTDUALMENUITEM           ID_FIRSTDUALMENUITEM
#define IDS_LASTDUALMENUITEM            ID_LASTDUALMENUITEM

//
//  Icon resource identifiers.
//

#define IDI_REGEDIT                     100
#define IDI_REGEDDOC                    101
#define IDI_REGFIND                     102

#define IDI_FIRSTIMAGE                  201
//  #define IDI_DIAMOND                     200
#define IDI_COMPUTER                    201
#define IDI_REMOTE                      202
#define IDI_FOLDER                      203
#define IDI_FOLDEROPEN                  204
#define IDI_STRING                      205
#define IDI_BINARY                      206
#define IDI_LASTIMAGE                   IDI_BINARY

//
//  Cursor resource identifiers.
//

#define IDC_SPLIT                       100

//
//  Accelerator resource identifiers.
//

#define IDACCEL_REGEDIT                 100

#endif // _INC_REGRESID
