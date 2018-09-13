/***************************************************************************/
/* WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! */
/***************************************************************************/
/* As part of the shdocvw/browseui split, parts of this file are moving to */
/* shell32 (#ifdef POSTPOSTSPLIT).  Make sure you make your delta to the   */
/* shell32 version if you don't want your changes to be lost!              */
/***************************************************************************/

// Resource IDs for SHDOCVW
//
// Cursor IDs

//  #define IDC_OFFLINE_HAND        103  This has been moved to shlobj.w so
// that ISVs can access it from outside
// Hence you will find it in shlobj.h


// BUGBUG:CHEE
// these are duplicated with explorer.exe
//
// (no they aren't -raymondc)
//
#define IDS_CHANNEL_UNAVAILABLE 832
#define IDS_BETAEXPIRED         835
#define IDS_FAV_UNABLETOCREATE  833

#define IDS_OPEN                840
#define IDS_SYNCHRONIZE         841
#define IDS_MAKE_OFFLINE        842
#define IDS_VALUE_UNKNOWN       844

#define IDS_DESKTOP             901

// global ids
#define IDC_STATIC                      -1

#define IDC_GROUPBOX                    300

#define IDC_KBSTART                     305

//
// ENDBUGBUG:CHEE
//
//
// Icons in other modules

#define IDI_URL_WEBDOC                  102     // in url.dll
#define IDI_URL_SPLAT                   106     // in url.dll


// Icon IDs (the order of these IDs should be preserved
//  across releases)
//
//
// *** READ THIS BEFORE MODIFYING ICONS ***
// ***
// *** The order of icons is important, as sometimes they
// *** are referenced by INDEX instead of by ID.  This
// *** means that technically, to maintain backward compat
// *** all icons, onced shipped, must stay there and stay
// *** in the same order.  Specifically, icon IDI_CHANNELSAPP (118)
// *** must be at index 18 as ie4 channels.scf references id.
// ***
// *** IE5 already ripped out a bunch of icons, and I'm putting
// *** back just enough to fix this bug.
// ***
//
// #define IDI_HOMEPAGE                 100 // Index:   0   // defined in inc\shdocvw.h
#define IDI_101                         101 //          1
#define IDI_RUNDLG                      102 //          2
#define IDI_SSL                         103 //          3
#define IDI_104                         104 //          4
//#define IDI_OFFLINE                   105 //          5   // defined in inc\shdocvw.h
#define IDI_106                         106 // ICO_CHANNELS in ie4
#define IDI_107                         107 // ICO_FAVORITES in ie4
#define IDI_108                         108 // ICO_SEARCH in ie4
#define IDI_109                         109 // ICO_HISTORY in ie4
#define IDI_STATE_FIRST                 110
//#define IDI_STATE_NORMAL              110 //          10  // defined in inc\shdocvw.h
#define IDI_STATE_FINDINGRESOURCE       111 //          11
#define IDI_STATE_SENDINGREQUEST        112 //          12
#define IDI_STATE_DOWNLOADINGDATA       113 //          13
#define IDI_STATE_LAST                  113
#define IDI_115                         115 // IDI_SUBSCRIPTION in ie4
#define IDI_PRINTER                     116 //          16
#define IDI_117                         117 // IDI_BACK_NONE in ie4
#define IDI_CHANNELSAPP                 118 //          18
#define IDI_154                         154 // IDI_SYSFILE in ie4
#define IDI_FRAME                       190 // IE4 shipped at 101
#define IDI_FAVORITE                    191 // IE4 shipped at 104
#define IDI_200                         200 // ICO_SHOWALL in ie4
#define IDI_201                         201 // ICO_HIDEHIDDEN in ie4
#define IDI_202                         202 // ICO_HIDESYSTEM in ie4
#define IDI_203                         203 // ICO_MULTWIN in ie4
//#define ICO_TREEUP                    204 //              // defined in inc\shdocvw.h
//#define ICO_GLEAM                     205 //              // defined in inc\shdocvw.h
#ifndef POSTPOSTSPLIT
#define IDI_NEW_FOLDER                  206
#endif
#define IDI_207                         207 // IDI_FOLDER in ie4
#define IDI_208                         208 // IDI_FOLDERVIEW in ie4
#define IDI_209                         209 // IDI_HTTFILE in ie4
#define IDI_LOCK                        0x31E0
#define IDI_UNLOCK                      0x31E1
#define IDI_USAGE_ICON                  0x31E2
#define IDI_SUGENERIC                   0x3330
#define IDI_REMOTEFLD                   20780
#define IDI_HISTORYDELETE               20782
#define IDI_HISTWEEK                    20783
#define IDI_HISTOPEN                    20784
#define IDI_HISTFOLDER                  20785
#define IDI_HISTURL                     20786
// ***
// *** IE4 shipped all the above icons, so if you want
// *** to add new ones, you should must add them
// *** after id 20786
// ***

#define IDI_FORTEZZA                    20788
#define IDI_STATE_SCRIPTERROR           20789


// other stuff
#define HSFBASE                         20480   //  0x5000
#ifdef _HSFOLDER
#define POPUP_CACHECONTEXT_URL          20680
#define POPUP_CONTEXT_URL_VERBSONLY     20681
#define POPUP_HISTORYCONTEXT_URL        20682
#define MENU_HISTORY                    20683
#define MENU_CACHE                      20684


#define IDS_BYTES                       (HSFBASE+515)
#define IDS_ORDERKB                     (HSFBASE+520)
#define IDS_ORDERMB                     (HSFBASE+521)
#define IDS_ORDERGB                     (HSFBASE+522)
#define IDS_ORDERTB                     (HSFBASE+523)

#define IDS_HOSTNAME_COL                (HSFBASE+345)
#define IDS_TIMEPERIOD_COL              (HSFBASE+346)
#define IDS_NAME_COL                    (HSFBASE+347)
#define IDS_ORIGINAL_COL                (HSFBASE+348)
#define IDS_STATUS_COL                  (HSFBASE+349)
#define IDS_SIZE_COL                    (HSFBASE+350)
#define IDS_TYPE_COL                    (HSFBASE+351)
#define IDS_MODIFIED_COL                (HSFBASE+352)
#define IDS_EXPIRES_COL                 (HSFBASE+353)
#define IDS_ACCESSED_COL                (HSFBASE+354)
#define IDS_LASTSYNCED_COL              (HSFBASE+355)
#define IDS_HSFNONE                     (HSFBASE+356)

#define IDS_CACHETYPE                   (HSFBASE+357)

#define IDS_LASTVISITED_COL             (HSFBASE+358)
#define IDS_NUMVISITS_COL               (HSFBASE+359)
#define IDS_WHATSNEW_COL                (HSFBASE+360)
#define IDS_DESCRIPTION_COL             (HSFBASE+361)
#define IDS_AUTHOR_COL                  (HSFBASE+362)
#define IDS_TITLE_COL                   (HSFBASE+363)
#define IDS_LASTUPDATED_COL             (HSFBASE+364)
#define IDS_SHORTNAME_COL               (HSFBASE+365)
#define IDS_NOTNETHOST                  (HSFBASE+366)
#define IDS_TODAY                       (HSFBASE+367)
#define IDS_FROMTO                      (HSFBASE+368)
#define IDS_WEEKOF                      (HSFBASE+369)
#define IDS_SITETOOLTIP                 (HSFBASE+370)
#define IDS_DAYTOOLTIP                  (HSFBASE+371)
#define IDS_WEEKTOOLTIP                 (HSFBASE+372)
#define IDS_MISCTOOLTIP                 (HSFBASE+373)
#define IDS_TODAYTOOLTIP                (HSFBASE+374)
#define IDS_WEEKSAGO                    (HSFBASE+375)
#define IDS_LASTWEEK                    (HSFBASE+376)
#define IDS_FILE_TYPE                   (HSFBASE+377)
#define IDS_HISTHOST_FMT                (HSFBASE+378)

#define IDM_SORTBYTITLE                 10
#define IDM_SORTBYADDRESS               11
#define IDM_SORTBYVISITED               12
#define IDM_SORTBYUPDATED               13

#define IDM_SORTBYNAME                  20
#define IDM_SORTBYADDRESS2              21
#define IDM_SORTBYSIZE                  22
#define IDM_SORTBYEXPIRES2              23
#define IDM_SORTBYMODIFIED              24
#define IDM_SORTBYACCESSED              25
#define IDM_SORTBYCHECKED               26

#define IDM_MOREINFO                    30

#define IDS_MH_FIRST                    (HSFBASE+400)
#define IDS_MH_TITLE                    IDS_MH_FIRST+IDM_SORTBYTITLE
#define IDS_MH_ADDRESS                  IDS_MH_FIRST+IDM_SORTBYADDRESS
#define IDS_MH_VISITED                  IDS_MH_FIRST+IDM_SORTBYVISITED
#define IDS_MH_UPDATED                  IDS_MH_FIRST+IDM_SORTBYUPDATED
#define IDS_MH_NAME                     IDS_MH_FIRST+IDM_SORTBYNAME
#define IDS_MH_ADDRESS2                 IDS_MH_FIRST+IDM_SORTBYADDRESS2
#define IDS_MH_SIZE                     IDS_MH_FIRST+IDM_SORTBYSIZE
#define IDS_MH_EXPIRES2                 IDS_MH_FIRST+IDM_SORTBYEXPIRES2
#define IDS_MH_MODIFIED                 IDS_MH_FIRST+IDM_SORTBYMODIFIED
#define IDS_MH_ACCESSED                 IDS_MH_FIRST+IDM_SORTBYACCESSED
#define IDS_MH_CHECKED                  IDS_MH_FIRST+IDM_SORTBYCHECKED


#define  RSVIDM_FIRST                   1
#define  RSVIDM_OPEN                    RSVIDM_FIRST+0
#define  RSVIDM_COPY                    RSVIDM_FIRST+1
#define  RSVIDM_DELCACHE                RSVIDM_FIRST+2
#define  RSVIDM_PROPERTIES              RSVIDM_FIRST+3
#define  RSVIDM_NEWFOLDER               RSVIDM_FIRST+4
#define  RSVIDM_ADDTOFAVORITES          RSVIDM_FIRST+5
#define  RSVIDM_OPEN_NEWWINDOW          RSVIDM_FIRST+6
#define  RSVIDM_EXPAND                  RSVIDM_FIRST+7
#define  RSVIDM_COLLAPSE                RSVIDM_FIRST+8
#define  RSVIDM_LAST                    RSVIDM_COLLAPSE /* Adjust me if you add new RSVIDM_s. */

#define IDS_SB_FIRST                    (HSFBASE+380)
#define IDS_SB_OPEN                     IDS_SB_FIRST+RSVIDM_OPEN
#define IDS_SB_COPY                     IDS_SB_FIRST+RSVIDM_COPY
#define IDS_SB_DELETE                   IDS_SB_FIRST+RSVIDM_DELCACHE
#define IDS_SB_PROPERTIES               IDS_SB_FIRST+RSVIDM_PROPERTIES

#define IDS_WARN_DELETE_HISTORYITEM     (HSFBASE+500)
#define IDS_WARN_DELETE_MULTIHISTORY    (HSFBASE+501)
#define IDS_WARN_DELETE_CACHE           (HSFBASE+502)

#define DLG_CACHEITEMPROP               21080
#define IDD_ITEMICON                    21081
#define IDD_FILETYPE_TXT                21084
#define IDD_FILETYPE                    21085
#define IDD_FILESIZE                    21087
#define IDD_LINE_2                      21088
#define IDD_EXPIRES                     21092
#define IDD_LASTMODIFIED                21094
#define IDD_LASTACCESSED                21096
#define IDD_TITLE                       21097

#define DLG_HISTITEMPROP                21180

#define IDD_INTERNET_ADDRESS            21280
#define IDD_LAST_VISITED                21281
#define IDD_LAST_UPDATED                21282
#define IDD_HSFURL                      21283
#define IDD_LAST_ACCESSED               21284
#define IDD_LAST_MODIFIED               21285
#define IDD_CACHE_NAME                  21286
#define IDD_NUMHITS                     21287

#define DLG_HISTCACHE_WARNING           21380
#define IDD_TEXT4                       21382

#endif

#ifndef POSTPOSTSPLIT
#define IDD_ADDTOFAVORITES_TEMPLATE     21400

#if 0
#define IDD_ADDTOCHANNELS_TEMPLATE      21401
#define IDD_ACTIVATE_PLATINUM_CHANNEL   21402
#define IDD_SUBSCRIBE_FAV_CHANNEL       21403
#define IDD_SUBSCRIBE_FAVORITE          21404
#define IDD_ADDTOSOFTDISTCHANNELS_TEMPLATE  21405
#endif

#define IDC_SUBSCRIBE_CUSTOMIZE         1004

#define IDC_FAVORITE_NAME               1005
#define IDC_FAVORITE_CREATEIN           1006
#define IDC_FAVORITE_NEWFOLDER          1007
#define IDC_FAVORITE_ICON               1008

#define IDC_CHANNEL_NAME                1009
#define IDC_CHANNEL_URL                 1010
#define IDC_FOLDERLISTSTATIC            1011
#define IDC_NAMESTATIC                  1012

#endif
    //control id's for next 3 are important -- they're the id's
    //of the same object (non-placeholder) in dialog created
    //by SHBrowseForFolder.  REVIEW!
#define IDC_SUBSCRIBE_FOLDERLIST_PLACEHOLDER 0x3741
#define IDOK_PLACEHOLDER                0001
#define IDCANCEL_PLACEHOLDER            0002


#define IDM_CLOSE               FCIDM_LAST + 0x0011

#define HELP_ITEM_COUNT         10

#define IDS_HELP_FIRST          0x4000
#define IDS_HELP_OF(id)         ((id - DVIDM_FIRST)+IDS_HELP_FIRST)
#define IDS_HELP_OPEN           IDS_HELP_OF(DVIDM_OPEN           )
#define IDS_HELP_SAVE           IDS_HELP_OF(DVIDM_SAVE           )
#define IDS_HELP_SAVEASFILE     IDS_HELP_OF(DVIDM_SAVEASFILE     )
#define IDS_HELP_PAGESETUP      IDS_HELP_OF(DVIDM_PAGESETUP      )
#define IDS_HELP_PRINT          IDS_HELP_OF(DVIDM_PRINT          )
#define IDS_HELP_SEND           IDS_HELP_OF(DVIDM_SEND           )
#define IDS_HELP_SENDPAGE       IDS_HELP_OF(DVIDM_SENDPAGE       )
#define IDS_HELP_SENDSHORTCUT   IDS_HELP_OF(DVIDM_SENDSHORTCUT   )
#define IDS_HELP_SENDTODESKTOP  IDS_HELP_OF(DVIDM_DESKTOPSHORTCUT)
#define IDS_HELP_IMPORTEXPORT   IDS_HELP_OF(DVIDM_IMPORTEXPORT   )
#define IDS_HELP_PROPERTIES     IDS_HELP_OF(DVIDM_PROPERTIES     )
#define IDS_HELP_CUT            IDS_HELP_OF(DVIDM_CUT            )
#define IDS_HELP_COPY           IDS_HELP_OF(DVIDM_COPY           )
#define IDS_HELP_PASTE          IDS_HELP_OF(DVIDM_PASTE          )
#define IDS_HELP_STOPDOWNLOAD   IDS_HELP_OF(DVIDM_STOPDOWNLOAD   )
#define IDS_HELP_REFRESH        IDS_HELP_OF(DVIDM_REFRESH        )
#define IDS_HELP_GOHOME         IDS_HELP_OF(DVIDM_GOHOME         )
#define IDS_HELP_GOSEARCH       IDS_HELP_OF(DVIDM_GOSEARCH       )
#define IDS_HELP_NEWWINDOW      IDS_HELP_OF(DVIDM_NEWWINDOW      )
#define IDS_HELP_PRINTPREVIEW   IDS_HELP_OF(DVIDM_PRINTPREVIEW   )
#define IDS_HELP_PRINTFRAME     IDS_HELP_OF(DVIDM_PRINTFRAME     )
#define IDS_HELP_NEWMESSAGE     IDS_HELP_OF(DVIDM_NEWMESSAGE     )
#define IDS_HELP_DHFAVORITES    IDS_HELP_OF(DVIDM_DHFAVORITES    )
#define IDS_HELP_HELPABOUT      IDS_HELP_OF(DVIDM_HELPABOUT      )
#define IDS_HELP_HELPSEARCH     IDS_HELP_OF(DVIDM_HELPSEARCH     )
#define IDS_HELP_HELPTUTORIAL   IDS_HELP_OF(DVIDM_HELPTUTORIAL   )
#define IDS_HELP_HELPMSWEB      IDS_HELP_OF(DVIDM_HELPMSWEB      )

#define IDS_HELP_NEW            IDS_HELP_OF(DVIDM_NEW            )
#define IDS_HELP_NEWPOST        IDS_HELP_OF(DVIDM_NEWPOST        )
#define IDS_HELP_NEWAPPOINTMENT IDS_HELP_OF(DVIDM_NEWAPPOINTMENT )
#define IDS_HELP_NEWMEETING     IDS_HELP_OF(DVIDM_NEWMEETING     )
#define IDS_HELP_NEWCONTACT     IDS_HELP_OF(DVIDM_NEWCONTACT     )
#define IDS_HELP_NEWTASK        IDS_HELP_OF(DVIDM_NEWTASK        )
#define IDS_HELP_NEWTASKREQUEST IDS_HELP_OF(DVIDM_NEWTASKREQUEST )
#define IDS_HELP_NEWJOURNAL     IDS_HELP_OF(DVIDM_NEWJOURNAL     )
#define IDS_HELP_NEWNOTE        IDS_HELP_OF(DVIDM_NEWNOTE        )
#define IDS_HELP_CALL           IDS_HELP_OF(DVIDM_CALL           )

#define FCIDM_HELPNETSCAPEUSERS (DVIDM_HELPMSWEB+11)
#define FCIDM_HELPONLINESUPPORT (DVIDM_HELPMSWEB+4)
#define FCIDM_HELPSENDFEEDBACK  (DVIDM_HELPMSWEB+5)
#define FCIDM_PRODUCTUPDATES    (DVIDM_HELPMSWEB+2)

#define IDS_HELP_HELPNETSCAPEUSERS  IDS_HELP_OF(FCIDM_HELPNETSCAPEUSERS)
#define IDS_HELP_HELPONLINESUPPORT  IDS_HELP_OF(FCIDM_HELPONLINESUPPORT)
#define IDS_HELP_HELPSENDFEEDBACK   IDS_HELP_OF(FCIDM_HELPSENDFEEDBACK)
#define IDS_HELP_PRODUCTUPDATES     IDS_HELP_OF(FCIDM_PRODUCTUPDATES)
#define IDS_HELP_ADDTOFAVORITES     IDS_HELP_OF(FCIDM_ADDTOFAVORITES)
#define IDS_HELP_ORGANIZEFAVORITES  IDS_HELP_OF(FCIDM_ORGANIZEFAVORITES)



#define IDS_MAYSAVEDOCUMENT     0x201
#define IDS_CANTACCESSDOCUMENT  0x202


#define IDS_SSL40               0x205
#define IDS_SSL128              0x206
#define IDS_SSL_FORTEZZA        0x207
#define IDS_SSL56               0x208

// ENDBUGBUG
//

// We pull this resource from browseui.  Don't change this ID unless
// you change browseui to match!!!  [Similar note in browseui]
#define IDB_IEBRAND             0x130


#define DELTA_HOT 1                     // HOT icons are def icons +1

#define IDB_IETOOLBAR           0x145
#define IDB_IETOOLBARHOT        0x146   // IDB_IETOOLBAR + DELTA_HOT

#define IDB_IETOOLBAR16         0x147
#define IDB_IETOOLBARHOT16      0x148   // IDB_IETOOLBAR16 + DELTA_HOT

#define IDB_IETOOLBARHICOLOR    0x149
#define IDB_IETOOLBARHOTHICOLOR 0x14A   // IDB_IETOOLBARHICOLOR + DELTA_HOT

#define IDS_BROWSER_TB_LABELS   0x14F   // string table for cut copy paste encoding

// For splash screen
#define IDB_SPLASH_IEXPLORER    0x150
#define IDB_SPLASH_IEXPLORER_HI 0x151
#define IDS_SPLASH_FONT         0x152
#define IDS_SPLASH_STR1         0x153
#define IDS_SPLASH_STR2         0x154
#define IDS_SPLASH_SIZE         0x155
#define IDS_SPLASH_Y1           0x156
#define IDS_SPLASH_Y2           0x157


// constants for download dialogs
#define IDB_DOWNLOAD            0x215

#define IDB_HISTORYANDFAVBANDSDEF  0x216
#define IDB_HISTORYANDFAVBANDSHOT  0x217

// ReBar stuff
#define IDS_SUBSTR_PRD           0x22C
#define IDS_SUBSTR_PVER          0x22D

#define IDS_BAND_MESSAGE         0x232

// OC stuff
//
#define IDS_VERB_EDIT           0x240

// Progress bar text
#define IDS_BINDSTATUS          0x260
#define IDS_BINDSTATUS_FIN      0x261 // (IDS_BINDSTATUS+BINDSTATUS_FINDINGRESOURCE)
#define IDS_BINDSTATUS_CON      0x262 // (IDS_BINDSTATUS+BINDSTATUS_CONNECTING)
#define IDS_BINDSTATUS_RED      0x263 // (IDS_BINDSTATUS+BINDSTATUS_REDIRECTING)
#define IDS_BINDSTATUS_BEG      0x264 // (IDS_BINDSTATUS+BINDSTATUS_BEGINDOWNLOADDATA)
#define IDS_BINDSTATUS_DOW      0x265 // (IDS_BINDSTATUS+BINDSTATUS_DOWNLOADINGDATA  )
#define IDS_BINDSTATUS_END      0x266 // (IDS_BINDSTATUS+BINDSTATUS_ENDDOWNLOADDATA  )
#define IDS_BINDSTATUS_BEGC     0x267 // (IDS_BINDSTATUS+BINDSTATUS_BEGINDOWNLOADCOMPONENTS)
#define IDS_BINDSTATUS_INSC     0x268 // (IDS_BINDSTATUS+BINDSTATUS_INSTALLINGCOMPONENTS        )
#define IDS_BINDSTATUS_ENDC     0x269 // (IDS_BINDSTATUS+BINDSTATUS_ENDDOWNLOADCOMPONENTS)
#define IDS_BINDSTATUS_USEC     0x26a // (IDS_BINDSTATUS+BINDSTATUS_USINGCACHEDCOPY)
#define IDS_BINDSTATUS_SEND     0x26b // (IDS_BINDSTATUS+BINDSTATUS_SENDINGREQUEST )
#define IDS_BINDSTATUS_PROXYDETECTING 0x26c 



// Registration Strings
//


#define IDS_REG_HTTPNAME        0x350
#define IDS_REG_HTTPSNAME       0x351
#define IDS_REG_FTPNAME         0x352
#define IDS_REG_GOPHERNAME      0x353
#define IDS_REG_TELNETNAME      0x354
#define IDS_REG_RLOGINNAME      0x355
#define IDS_REG_TN3270NAME      0x356
#define IDS_REG_MAILTONAME      0x357
#define IDS_REG_NEWSNAME        0x358
#define IDS_REG_FILENAME        0x359
#define IDS_REG_INTSHNAME       0x35a
#define IDS_REG_THEINTERNET     0x35b
#define IDS_REG_URLEXECHOOK     0x35c
#define IDS_REG_OPEN            0x35d
#define IDS_REG_OPENSAME        0x35e
#define IDS_REG_SCFTYPENAME     0x35f
#define IDS_RELATEDSITESMENUTEXT    0x360   // 864
#define IDS_RELATEDSITESSTATUSBAR   0x361   // 865
#define IDS_RELATEDSITESBUTTONTEXT  0x362   // 866
#define IDS_TIPOFTHEDAYTEXT         0x363   // 867
#define IDS_TIPOFTHEDAYSTATUSBAR    0x364   // 868


////  3d0- 420 reserved for quick links

#define IDS_TYPELIB             0x4f0
#define IDS_SHELLEXPLORER       0x4f1
//                              0x4f2

#define IDS_CATDESKBAND         0x502

#define IDS_CATINFOBAND         0x504

#define IDS_CATCOMMBAND         0x509

#define IDS_ERRMSG_FIRST        0x1000
#define IDS_ERRMSG_LAST         0x1fff

// OC Bitmaps
//
#define IDB_FOLDER              0x101 // used in selfreg.inf
#define IDB_FOLDERVIEW          0x104 // used in selfreg.inf

// Dialog Boxes
#define DLG_DOWNLOADPROGRESS    0x1100
#define IDD_ANIMATE             0x1101
#define IDD_NAME                0x1102
#define IDD_OPENIT              0x1103
#define IDD_PROBAR              0x1104
#define IDD_TIMEEST             0x1105
#define IDD_SAVEAS              0x1106
#define IDD_DOWNLOADICON        0x1107
#define IDD_NOFILESIZE          0x1109
#define IDD_TRANSFERRATE        0x1110
#define IDD_DIR                 0x1112
#define IDD_DISMISS             0x1113
#define IDD_DNLDCOMPLETEICON    0x1114
#define IDD_DNLDCOMPLETETEXT    0x1115
#define IDD_DNLDESTTIME         0x1116
#define IDD_DNLDTIME            0x1117
#define IDD_BROWSEDIR           0x1118
#define IDD_OPENFILE            0x1119

#define DLG_SAFEOPEN            0x1140
#define IDC_SAFEOPEN_ICON       0x1141
#define IDC_SAFEOPEN_EXPL       0x1142
#define IDC_SAFEOPEN_AUTOOPEN   0x1143
#define IDC_SAFEOPEN_AUTOSAVE   0x1144
#define IDC_SAFEOPEN_ALWAYS     0x1145

#define IDD_ASSOC               0x1160
#define IDC_ASSOC_CHECK         0x1161
#define IDC_ASSOC_IE40          0x1163

#define IDD_PRINTOPTIONS        0x1165

#define DLG_RUN                 0x1170
#define IDD_ICON                0x1171
#define IDD_PROMPT              0x1172
#define IDD_COMMAND             0x1173
#define IDD_RUNDLGOPENPROMPT    0x1174
#define IDD_BROWSE              0x1175
#define IDC_ASWEBFOLDER         0x1176

#define DLG_NEWFOLDER           0x1180
#define IDD_SUBSCRIBE           0x1185

// UNIX only
#define DLG_RUNMOTIF            0x1187


// Resources for Internet Shortcut dialogs

#define IDD_INTSHCUT_PROP               1048
#undef IDC_ICON         // The one defined in winuser is obsolete
#define IDC_ICON                        1002
#define IDC_NAME                        1003
#define IDC_URL_TEXT                    1004
#define IDC_URL                         1005
#define IDC_HOTKEY_TEXT                 1006
#define IDC_START_IN_TEXT               1007
#define IDC_START_IN                    1008
#define IDC_SHOW_CMD                    1009
#define IDC_CHANGE_ICON                 1010
#define IDC_HOTKEY                      1011

#define IDC_WHATSNEW                    1011
#define IDC_RATING                      1012
#define IDC_AUTHOR                      1013
#define IDC_LAST_VISITED                1014
#define IDC_LAST_MODIFIED               1015
#define IDC_VISITCOUNT                  1016
#define IDC_DESC                        1017

#define IDC_VISITS_TEXT                 1018
#define IDC_VISITS                      1019
#define IDC_MAKE_OFFLINE                1020
#define IDC_SUMMARY                     1021
#define IDC_LAST_SYNC_TEXT              1022
#define IDC_LAST_SYNC                   1023
#define IDC_DOWNLOAD_SIZE_TEXT          1024
#define IDC_DOWNLOAD_SIZE               1025
#define IDC_DOWNLOAD_RESULT_TEXT        1026
#define IDC_DOWNLOAD_RESULT             1027
#define IDC_FAVORITE_DESC               1028
#define IDC_FREESPACE_TEXT              1029

#define IDS_ALLFILES                    1200
#define IDS_BROWSEFILTER                1201
#define IDS_DOWNLOADFAILED              1202
#define IDS_TRANSFERRATE                1203
#define IDS_DOWNLOADTOCACHE             1204
#define IDS_UNTITLE_SHORTCUT            1205
#define IDS_SECURITYALERT               1206
#define IDS_DOWNLOADDISALLOWED          1207
// unused - recycle me                  1208
#define IDS_URL_SEARCH_KEY              1210
#define IDS_SEARCH_URL                  1211
#define IDS_SEARCH_SUBSTITUTIONS        1212
#define IDS_SHURL_ERR_PARSE_NOTALLOWED  1213
#define IDS_SEARCH_INTRANETURL          1214

// UNIX only
#define IDS_SHURL_ERR_NOASSOC           1215
#define IDS_DOWNLOAD_BADCACHE           1216


#define IDS_SETHOME_TITLE               1220
#define IDS_SETHOME_TEXT                1221

// Warning strings must be sequential.
#define IDS_ADDTOFAV_WARNING            1230
#define IDS_ADDTOLINKS_WARNING          1231
#define IDS_MAKEHOME_WARNING            1232
#define IDS_DROP_WARNING                1233

#define IDS_CONFIRM_RESET_SAFEMODE      1526

// Internet shortcut-related IDs
#define IDS_SHORT_NEW_INTSHCUT              0x2730
#define IDS_NEW_INTSHCUT                    0x2731
#define IDS_INVALID_URL_SYNTAX              0x2732
#define IDS_UNREGISTERED_PROTOCOL           0x2733
#define IDS_SHORTCUT_ERROR_TITLE            0x2734
#define IDS_IS_EXEC_FAILED                  0x2735
#define IDS_IS_EXEC_OUT_OF_MEMORY           0x2736
#define IDS_IS_EXEC_UNREGISTERED_PROTOCOL   0x2737
#define IDS_IS_EXEC_INVALID_SYNTAX          0x2738
#define IDS_IS_LOADFROMFILE_FAILED          0x2739
#define IDS_INTERNET_SHORTCUT               0x273E
#define IDS_URL_DESC_FORMAT                 0x273F
#define IDS_FAV_LASTVISIT                   0x2740
#define IDS_FAV_LASTMOD                     0x2741
#define IDS_FAV_WHATSNEW                    0x2742
#define IDS_IS_APPLY_FAILED                 0x2744
#define IDS_FAV_STRING                      0x2745

#define IDS_AUTHOR                          0x2746
#define IDS_SUBJECT                         0x2747
#define IDS_COMMENTS                        0x2748
#define IDS_DOCTITLE                        0x2749

#define IDS_MENUOPEN                        0x2800

// Open Web Folder Dialogs
#define IDS_ERRORINTERNAL                   0x2940

// Internet shortcut menu help
#define IDS_MH_ISFIRST                      0x2750
#define IDS_MH_OPEN                         (IDS_MH_ISFIRST + 0)
#define IDS_MH_SYNCHRONIZE                  (IDS_MH_ISFIRST + 1)
#define IDS_MH_MAKE_OFFLINE                 (IDS_MH_ISFIRST + 2)

#define IDC_STATIC                      -1


// AVI
#define IDA_DOWNLOAD            0x100




//---------------------------------------------------------------------------
// Defines for the rc file.
//---------------------------------------------------------------------------

// BUGBUG: these are duplicated in browseui

// Commmand ID
#define FCIDM_FIRST             FCIDM_GLOBALFIRST
#define FCIDM_LAST              FCIDM_BROWSERLAST

#define FCIDM_BROWSER_EXPLORE   (FCIDM_BROWSERFIRST + 0x0120)

#define FCIDM_BROWSER_FILE      (FCIDM_BROWSERFIRST+0x0020)
#define FCIDM_PREVIOUSFOLDER    (FCIDM_BROWSER_FILE+0x0002) // shbrowse::EXEC (cannot change)
#define FCIDM_DELETE            (FCIDM_BROWSER_FILE+0x0003)
#define FCIDM_RENAME            (FCIDM_BROWSER_FILE+0x0004)
#define FCIDM_PROPERTIES        (FCIDM_BROWSER_FILE+0x0005)

#define FCIDM_BROWSER_EDIT      (FCIDM_BROWSERFIRST+0x0040)
#define FCIDM_MOVE              (FCIDM_BROWSER_EDIT+0x0001)
#define FCIDM_COPY              (FCIDM_BROWSER_EDIT+0x0002)
#define FCIDM_PASTE             (FCIDM_BROWSER_EDIT+0x0003)
#define FCIDM_LINK              (FCIDM_BROWSER_EDIT+0x0005)     // create shortcut

#define FCIDM_FAVS_FIRST        (FCIDM_BROWSER_EXPLORE  +0x0052)
#define FCIDM_ORGANIZEFAVORITES (FCIDM_FAVS_FIRST       +0x0000)
#define FCIDM_ADDTOFAVORITES    (FCIDM_FAVS_FIRST       +0x0001)
#define FCIDM_FAVS_MORE         (FCIDM_FAVS_FIRST       +0x0002)
#define FCIDM_FAVORITEFIRST     (FCIDM_FAVS_FIRST       +0x0003)
#define FCIDM_UPDATESUBSCRIPTIONS (FCIDM_FAVS_FIRST       +0x0004)
#define FCIDM_SORTBY            (FCIDM_FAVS_FIRST       +0x0005)
#define FCIDM_SORTBYNAME        (FCIDM_FAVS_FIRST       +0x0006)
#define FCIDM_SORTBYVISIT       (FCIDM_FAVS_FIRST       +0x0007)
#define FCIDM_SORTBYDATE        (FCIDM_FAVS_FIRST       +0x0008)
#define FCIDM_FAVAUTOARRANGE    (FCIDM_FAVS_FIRST       +0x0009)
#define FCIDM_SUBSCRIPTIONS     (FCIDM_FAVS_FIRST       +0x000A)
#define FCIDM_SUBSCRIBE         (FCIDM_FAVS_FIRST       +0x000B)

#define FCIDM_FAVORITELAST      (FCIDM_FAVORITEFIRST    +0x0050)
#define FCIDM_FAVORITE_ITEM     (FCIDM_FAVORITEFIRST + 0)
#define FCIDM_FAVORITECMDFIRST  (FCIDM_FAVS_FIRST)
#define FCIDM_FAVORITECMDLAST   (FCIDM_FAVORITELAST)
#define FCIDM_FAVS_LAST         (FCIDM_FAVORITELAST)


//---------------------------------------------------------------------------
#define FCIDM_BROWSER_VIEW      (FCIDM_BROWSERFIRST + 0x0200)
#define FCIDM_VIEWTOOLBAR       (FCIDM_BROWSER_VIEW + 0x0001)
#define FCIDM_VIEWSTATUSBAR     (FCIDM_BROWSER_VIEW + 0x0002)
#define FCIDM_VIEWOPTIONS       (FCIDM_BROWSER_VIEW + 0x0003)
#define FCIDM_VIEWTOOLS         (FCIDM_BROWSER_VIEW + 0x0004)
#define FCIDM_VIEWADDRESS       (FCIDM_BROWSER_VIEW + 0x0005)
#define FCIDM_VIEWLINKS         (FCIDM_BROWSER_VIEW + 0x0006)
#define FCIDM_VIEWTEXTLABELS    (FCIDM_BROWSER_VIEW + 0x0007)
#define FCIDM_VIEWTBCUST        (FCIDM_BROWSER_VIEW + 0x0008)
#define FCIDM_VIEWAUTOHIDE      (FCIDM_BROWSER_VIEW + 0x0009)
#define FCIDM_VIEWMENU          (FCIDM_BROWSER_VIEW + 0x000A)

#define FCIDM_STOP              (FCIDM_BROWSER_VIEW + 0x001a)
#define FCIDM_REFRESH           (FCIDM_BROWSER_VIEW + 0x0020) // ie4 shell32: must be A220 (cannot change)
#define FCIDM_ADDTOFAVNOUI      (FCIDM_BROWSER_VIEW + 0x0021)
#define FCIDM_VIEWITBAR         (FCIDM_BROWSER_VIEW + 0x0022)
#define FCIDM_VIEWSEARCH        (FCIDM_BROWSER_VIEW + 0x0017)
#define FCIDM_CUSTOMIZEFOLDER   (FCIDM_BROWSER_VIEW + 0x0018)
#define FCIDM_VIEWFONTS         (FCIDM_BROWSER_VIEW + 0x0019)
// 1a is FCIDM_STOP
#define FCIDM_THEATER           (FCIDM_BROWSER_VIEW + 0x001b)
#define FCIDM_JAVACONSOLE       (FCIDM_BROWSER_VIEW + 0x001c)
// 1d - FCIDM_VIEWTOOLBARCUSTOMIZE
#define FCIDM_ENCODING          (FCIDM_BROWSER_VIEW + 0x001e)
// (FCIDM_BROWSER_VIEW + 0x0030) through
// (FCIDM_BROWSER_VIEW + 0x003f) is taken

// Define string ids that go into resource file
#define IDS_CHANNEL             0x503

#define IDS_SUBS_UNKNOWN        711
#ifndef POSTPOSTSPLIT
#define IDS_NEED_CHANNEL_PASSWORD     716
#endif


// BUGBUG: these are duplciated in browseui
#define IDS_TITLE       723
#define IDS_ERROR_GOTO  724
// ENDBUGBUG

#define IDS_NONE        725
#define IDS_NAME        726     // Used for NAME member function for fram programmability

#define IDS_INVALIDURL   727    // Generic error message in OnStopBinding
#define IDS_CANTDOWNLOAD 728
#define IDS_TARGETFILE   730    // String for target file of downloading
#define IDS_DOWNLOADCOMPLETE 731 // Download completed

#define IDS_CREATE_SHORTCUT_MSG 734
#define IDS_UNDEFINEDERR 735
#define IDS_SAVING       736
#define IDS_OPENING      737

#define IDS_ESTIMATE    738     // Estimated time string for progress (B/sec)
// 739 below
#define IDS_SAVED       740
#define IDS_BYTESCOPIED 741     // Progress text when ulMax is 0 (unknown)
#define IDS_DEF_UPDATE  742
#define IDS_DEF_CHANNELGUIDE 743
#define IDS_DOCUMENT    744
#define IDS_ERR_OLESVR  745     // CoCreateInstance failed.
#define IDS_ERR_LOAD    746     // IPersistFile::Load failed.

#ifndef POSTPOSTSPLIT
#define IDS_FAVORITES          749
#define IDS_FAVORITEBROWSE     748

#define IDS_FAVS_SUBSCRIBE_TEXT 718
#define IDS_FAVS_SUBSCRIBE      719
#define IDS_FAVS_FOLDER        747
#define IDS_FAVS_BROWSETEXT    750
#define IDS_FAVS_NEWFOLDERBUTTON 751
#define IDS_FAVS_NAME          752
#define IDS_FAVS_ADVANCED      753
#define IDS_FAVS_ADDTOFAVORITES 757
#define IDS_FAVS_SAVE           758
#define IDS_FAVS_TITLE          759
#define IDS_FAVS_MORE           792
#define IDS_FAVS_FILEEXISTS     794
#define IDS_FAVS_INVALIDFN      795
#define IDS_FAVS_ADVANCED_EXPAND   812
#define IDS_FAVS_ADVANCED_COLLAPSE 813
#define IDS_FAVS_FNTOOLONG      810


#define IDS_EXCEPTIONMSGSH 739
#define IDS_EXCEPTIONMSG        754
#define IDS_EXCEPTIONNOMEMORY   755
#endif

#define IDS_CANTSHELLEX         756     // Shell Execute on the URL failed

#define IDS_TITLEBYTES          760   // Download title with % loaded
#define IDS_TITLEPERCENT        761     // Download title with bytes copied
#define IDS_HELPTUTORIAL        762

#define IDS_HELPMSWEB           763
// Don't use 763=779 because they are used by HELPMSWEB strings

// Don't use 780=790 because they wil be used by different URLs used in the product

#define IDS_DEFDLGTITLE         790

#define IDS_EXCHANGE            791 // Exchange mail client display name

#define IDS_CANTFINDURL         793 // Autosearching prompt on failed navigation

#ifndef POSTPOSTSPLIT
#define IDS_CHANNELS_FILEEXISTS 796
#endif
#define IDS_BYTESTIME           797
#define IDS_CANTFINDSEARCH      799
#define IDS_CLOSE               800
#define IDS_EXTDOCUMENT         811

#define IDS_OPENFROMINTERNET    920
#define IDS_SAVEFILETODISK      921

// Coolbar String IDs - starting from 950

////////////////// WARNING!!!  /////////////////
///  IDS_QLURL1 MUST be 1000, IDS_QLTEXT1 MUST BE 1010
// inetcpl depends on it ..  -Chee
// also, inetcpl hard codes that there are 5 quicklinks!
#define IDS_DEF_HOME    998  //// WARNING!!! DO NOT CHANGE THESE VALUES
#define IDS_DEF_SEARCH  999 //// WARNING!!!  INETCPL RELIES ON THEM

//////////////////////// END WARNING! //////////////////////


#define IDS_FOLDEROPTIONS       1030
#define IDS_INTERNETOPTIONS     1031


#define IDS_CONFIRM_RESETFLAG           1060

// Accelerator ID

#define ACCEL_DOCVIEW             0x101
#define ACCEL_DOCVIEW_NOFILEMENU  0x102
#define ACCEL_FAVBAR              0x103

#define MID_FOCUS               0x102

#define MENU_SCRDEBUG                   0x103


//#ifdef DEBUG
#define ALPHA_WARNING_IS_DUMB
//#endif

#ifndef ALPHA_WARNING_IS_DUMB
#define IDS_ALPHAWARNING        0x2000
#endif



// Title for properties dialog
#define IDS_INTERNETSECURITY    0x2003
// ID for running uninstall stubs
#define IDS_UNINSTALL         0x3010

#define IDS_CONFIRM_SCRIPT_CLOSE_TEXT 0x3035

// OPSProfile strings (0x3100 to 0x31BF)
#define IDR_TRACK                       0x3101

#define IDS_OPS_REQUEST                 0x3100
#define IDS_PROFILE_ASSISTANT           0x3101
#define IDS_OPS_CONFIRM                 0x3102
#define IDS_OPS_BLANK                   0x3103
#define IDS_OPS_NO_INFORMATION          0x3104

#define IDS_DEFAULT_FNAME               0x3140

#define IDS_OPS_COMMONNAME              0x3150
#define IDS_OPS_GIVENNAME               0x3151
#define IDS_OPS_LASTNAME                0x3152
#define IDS_OPS_MIDDLENAME              0x3153
#define IDS_OPS_GENDER                  0x3154
#define IDS_OPS_CELLULAR                0x3155
#define IDS_OPS_EMAIL                   0x3156
#define IDS_OPS_URL                     0x3157

#define IDS_OPS_COMPANY                 0x3158
#define IDS_OPS_DEPARTMENT              0x3159
#define IDS_OPS_JOBTITLE                0x315a
#define IDS_OPS_PAGER                   0x315b

#define IDS_OPS_HOME_ADDRESS            0x315c
#define IDS_OPS_HOME_CITY               0x315d
#define IDS_OPS_HOME_ZIPCODE            0x315e
#define IDS_OPS_HOME_STATE              0x315f
#define IDS_OPS_HOME_COUNTRY            0x3160
#define IDS_OPS_HOME_PHONE              0x3161
#define IDS_OPS_HOME_FAX                0x3162

#define IDS_OPS_BUSINESS_ADDRESS        0x3163
#define IDS_OPS_BUSINESS_CITY           0x3164
#define IDS_OPS_BUSINESS_ZIPCODE        0x3165
#define IDS_OPS_BUSINESS_STATE          0x3166
#define IDS_OPS_BUSINESS_COUNTRY        0x3167

#define IDS_OPS_BUSINESS_PHONE          0x3168
#define IDS_OPS_BUSINESS_FAX            0x3169
#define IDS_OPS_BUSINESS_URL            0x316a

#define IDS_OPS_OFFICE                  0x316b

#define IDS_NAVIGATEBACKTO              0x3170
#define IDS_NAVIGATEFORWARDTO           0x3171

// Usage strings. These have to be contiguous
#define IDS_OPS_USAGEUNK                0x31A0
#define IDS_OPS_USAGE0                  0x31A1
#define IDS_OPS_USAGE1                  0x31A2
#define IDS_OPS_USAGE2                  0x31A3
#define IDS_OPS_USAGE3                  0x31A4
#define IDS_OPS_USAGE4                  0x31A5
#define IDS_OPS_USAGE5                  0x31A6
#define IDS_OPS_USAGE6                  0x31A7
#define IDS_OPS_USAGE7                  0x31A8
#define IDS_OPS_USAGE8                  0x31A9
#define IDS_OPS_USAGE9                  0x31AA
#define IDS_OPS_USAGE10                 0x31AB
#define IDS_OPS_USAGE11                 0x31AC
#define IDS_OPS_USAGE12                 0x31AD
#define IDS_OPS_USAGEMAX                0x31AD

// Dialogs and controls.
#define IDD_OPS_CONSENT                 0x3200
#define IDD_OPS_UPDATE                  0x3201

#define IDC_OPS_LIST                    0x3210
#define IDC_VIEW_CERT                   0x3211
#define IDC_USAGE_STRING                0x3212
#define IDC_SITE_IDENTITY               0x3213
#define IDC_SECURITY_ICON               0x3214
#define IDC_USAGE_ICON                  0x3215
#define IDC_SECURE_CONNECTION           0x3216
#define IDC_UNSECURE_CONNECTION         0x3217
#define IDC_OPS_URL                     0x3218
#define IDC_EDIT_PROFILE                0x3219
#define IDC_KEEP_SETTINGS               0x321A
#define IDC_OPS_INFO_REQUESTED          0x321B
#define IDC_OPS_PRIVACY                 0x321C

// AutoSuggest dialogs and controls
#define IDD_AUTOSUGGEST_SAVEPASSWORD    0x3220
#define IDD_AUTOSUGGEST_CHANGEPASSWORD  0x3221
#define IDD_AUTOSUGGEST_DELETEPASSWORD  0x3222
#define IDD_AUTOSUGGEST_ASK_USER        0x3223
#define IDC_AUTOSUGGEST_NEVER           0x3225
#define IDC_AUTOSUGGEST_ICON            0x3226
#define IDA_AUTOSUGGEST                 0x3227
//#define IDI_AUTOSUGGEST                 0x3228
#define IDC_AUTOSUGGEST_HELP            0x324F

// Software Update Advertisment Dialogs
#define IDD_SUAVAILABLE                 0x3300
#define IDD_SUDOWNLOADED               0x3301
#define IDD_SUINSTALLED                0x3302

//ids for DLG_SUAVAIL, DLG_SUDOWNLOAD, DLG_SUINSTALL
#define IDC_ICONHOLD                    0x3310
#define IDC_REMIND                      0x3311
#define IDC_DETAILS                     0x3312
#define IDC_DETAILSTEXT                 0x3313

#define IDS_SUDETAILSFMT                0x3320
#define IDS_SUDETAILSOPEN               0x3321
#define IDS_SUDETAILSCLOSE              0x3322

#define IDS_HISTVIEW_FIRST              0x3331
#define IDS_HISTVIEW_DEFAULT            0x3331
#define IDS_HISTVIEW_SITE               0x3332
#define IDS_HISTVIEW_FREQUENCY          0x3333
#define IDS_HISTVIEW_TODAY              0x3334
#define IDS_HISTVIEW_LAST               0x3335

#define IDS_DONE_WITH_SCRIPT_ERRORS     0x3336
#define IDS_SCRIPT_ERROR_ON_PAGE        0x3337

/* ID for install stub progress dialog (template in \shell\inc\inststub.rc) */
#define IDD_InstallStubProgress         0x3340


// IDs for thicket save
#define IDD_SAVETHICKET                 0x3350
#define IDC_THICKETPROGRESS             0x3351
#define IDC_THICKETSAVING               0x3352
#define IDC_THICKETPCT                  0x3353

#define IDS_THICKETDIRFMT               0x3354
#define IDS_THICKETTEMPFMT              0x3355
#define IDS_THICKET_SAVE                0x3356
#define IDS_NOTHICKET_SAVE              0x3357
#define IDS_UNTITLED                    0x3358

#define IDD_ADDTOSAVE_DIALOG            0x3359
#define IDC_SAVE_CHARSET                0x335A

#define IDS_THICKETERRTITLE             0x335B
#define IDS_THICKETERRMEM               0x335C
#define IDS_THICKETERRMISC              0x335D
#define IDS_THICKETERRACC               0x335E
#define IDS_THICKETERRFULL              0x335F
#define IDS_THICKETABORT                0x3360
#define IDS_THICKETSAVINGFMT            0x3361
#define IDS_THICKETPCTFMT               0x3362
#define IDS_THICKETERRFNF               0x3363
#define IDS_NOMHTML_SAVE                0x3364

#define IDD_ADDTOSAVE_NT5_DIALOG        0x3365

#define IDD_IMPEXP                      0x3380
#define IDC_IMPORT                      0x3381
#define IDC_EXPORT                      0x3382
#define IDC_FAVORITES                   0x3383
#define IDC_BROWSEFORFAVORITES          0x3384

#define IDS_IMPORTCONVERTERROR          0x3385
#define IDS_NOTVALIDBOOKMARKS           0x3386
#define IDS_COULDNTOPENBOOKMARKS        0x3387
#define IDS_IMPORTFAILURE_FAV           0x3388
#define IDS_IMPORTSUCCESS_FAV           0x3389
#define IDS_EXPORTFAILURE_FAV           0x338A
#define IDS_EXPORTSUCCESS_FAV           0x338B
#define IDS_IMPORTFAILURE_COOK          0x338C
#define IDS_IMPORTSUCCESS_COOK          0x338D
#define IDS_EXPORTFAILURE_COOK          0x338E
#define IDS_EXPORTSUCCESS_COOK          0x338F
#define IDS_EXPORTDIALOGTITLE           0x3390
#define IDS_IMPORTDIALOGTITLE           0x3391
#define IDS_INVALIDURLFILE              0x3392
#define IDS_CONFIRM_IMPTTL_FAV          0x3393
#define IDS_CONFIRM_EXPTTL_FAV          0x3394
#define IDS_CONFIRM_IMPTTL_COOK         0x3395
#define IDS_CONFIRM_EXPTTL_COOK         0x3396
#define IDS_CONFIRM_IMPORT              0x3397
#define IDS_CONFIRM_EXPORT              0x3398
#define IDS_IMPORT_DISABLED             0x3399
#define IDS_EXPORT_DISABLED             0x339A
#define IDS_IMPORTEXPORTTITLE           0x339B

// Save-as warning dialog
#define DLG_SAVEAS_WARNING              0x3400
#define IDC_SAVEAS_WARNING_STATIC       0x3401
#define IDC_SAVEAS_WARNING_CB           0x3402
#define IDI_SAVEAS_WARNING              0x3403

// HTML dialog resources
#define RT_FILE                         2110

// Print dialog

#define IDC_LINKED                          8140
#define IDC_PREVIEW                         8141
#define IDC_SHORTCUTS                       8142
#define IDC_SCALING                         8143

// Page setup

#define IDC_HEADERFOOTER                    8145
#define IDC_STATICHEADER                    8146
#define IDC_EDITHEADER                      8147
#define IDC_STATICFOOTER                    8148
#define IDC_EDITFOOTER                      8149

#define IDR_PRINT_PREVIEW               8416
#define IDR_PRINT_PREVIEWONEDOC         8417
#define IDR_PRINT_PREVIEWALLDOCS        8418
#define IDR_PRINT_PREVIEWDISABLED       8422

#define IDS_PRINTTOFILE_TITLE           8419
#define IDS_PRINTTOFILE_OK              8420
#define IDS_PRINTTOFILE_SPEC            8421

///////////////////////////////////////////////////////
// Favorites, nsc, and explorer bars
#define IDS_FAVS_BAR_LABELS     3000
#define IDS_HIST_BAR_LABELS     3001
#define IDI_PINNED              3002
#define IDS_SEARCH_MENUOPT      3003
#define IDS_BAND_FAVORITES      3004
#define IDS_BAND_HISTORY        3005
#define IDS_BAND_CHANNELS       3006

#define IDS_RESTRICTED          3007

// (see histBand.cpp for more info...)
#define FCIDM_HISTBAND_FIRST      (FCIDM_BROWSERFIRST   + 0x0180)
#define FCIDM_HISTBAND_VIEW       (FCIDM_HISTBAND_FIRST + 0x0000)
#define FCIDM_HISTBAND_SEARCH     (FCIDM_HISTBAND_FIRST + 0x0001)

#define IDC_EDITHISTSEARCH       3205
#define IDD_HISTSRCH_ANIMATION   3206
#define IDA_HISTSEARCHAVI        3207
#define IDB_HISTSRCH_GO          3208
#define IDC_HISTSRCH_STATIC      3209
#define IDC_HISTCUSTOMLINE       3210

#define DLG_HISTSEARCH2          3211


#define POPUP_CONTEXT_NSC       3400

///////////////////////////////////////////////////////

// Flavors of refresh
#define IDM_REFRESH_TOP                  6041   // Normal refresh, topmost doc
#define IDM_REFRESH_THIS                 6042   // Normal refresh, nearest doc
#define IDM_REFRESH_TOP_FULL             6043   // Full refresh, topmost doc
#define IDM_REFRESH_THIS_FULL            6044   // Full refresh, nearest doc

// placeholder for context menu extensions
#define IDM_MENUEXT_PLACEHOLDER          6047
#define IDR_FORM_CONTEXT_MENU       24640  //0x6040  // bad id - not in core range
#define IDR_BROWSE_CONTEXT_MENU     24641  //0x6041  // bad id - not in core range


#define IDM_DEBUG_TRACETAGS         6004
#define IDM_DEBUG_RESFAIL           6005
#define IDM_DEBUG_DUMPOTRACK        6006
#define IDM_DEBUG_BREAK             6007
#define IDM_DEBUG_VIEW              6008
#define IDM_DEBUG_DUMPTREE          6009
#define IDM_DEBUG_DUMPLINES         6010
#define IDM_DEBUG_LOADHTML          6011
#define IDM_DEBUG_SAVEHTML          6012
#define IDM_DEBUG_MEMMON            6013
#define IDM_DEBUG_METERS            6014
#define IDM_DEBUG_DUMPDISPLAYTREE   6015
#define IDM_DEBUG_DUMPFORMATCACHES  6016


#ifdef UNIX
//EULA related entries
#define IDD_EULA                        0x4000
#define IDC_WIZARD                      0x4001
#define IDC_EULA_TEXT                   0x4002
#define IDC_ACCEPT                      0x4003
#define IDC_DONT_ACCEPT                 0x4004
#define IDC_MORE                        0x4005
#define IDC_BIGFONT                     0x4006
#define IDD_ALPHAWRNDLG                 0x4007
#define IDC_NOFUTUREDISPLAY             0x4008
#define IDC_ALIAS_NAME                  0x4009

#define IDS_NEWS_SCRIPT_ERROR           0x4010
#define IDS_NEWS_SCRIPT_ERROR_TITLE     0x4011

#define IDI_MONOFRAME                   0x4020

#define IDS_NS_BOOKMARKS_DIR               137
#endif


#define IDS_IMPFAVORITES                0x4201
#define IDS_IMPFAVORITESDESC            0x4202
#define IDS_EXPFAVORITES                0x4203
#define IDS_EXPFAVORITESDESC            0x4204
#define IDS_IMPCOOKIES                  0x4205
#define IDS_IMPCOOKIESDESC              0x4206
#define IDS_EXPCOOKIES                  0x4207
#define IDS_EXPCOOKIESDESC              0x4208
#define IDS_IMPEXPTRANSFERTYPE_TITLE    0x4209
#define IDS_IMPEXPTRANSFERTYPE_SUBTITLE 0x420A
#define IDS_IMPEXPIMPFAVSRC_TITLE       0x420B
#define IDS_IMPEXPIMPFAVSRC_SUBTITLE    0x420C
#define IDS_IMPEXPIMPFAVDES_TITLE       0x420D
#define IDS_IMPEXPIMPFAVDES_SUBTITLE    0x420E
#define IDS_IMPEXPEXPFAVSRC_TITLE       0x420F
#define IDS_IMPEXPEXPFAVSRC_SUBTITLE    0x4210
#define IDS_IMPEXPEXPFAVDES_TITLE       0x4211
#define IDS_IMPEXPEXPFAVDES_SUBTITLE    0x4212
#define IDS_IMPEXPIMPCKSRC_TITLE        0x4213
#define IDS_IMPEXPIMPCKSRC_SUBTITLE     0x4214
#define IDS_IMPEXPEXPCKDES_TITLE        0x4215
#define IDS_IMPEXPEXPCKDES_SUBTITLE     0x4216
#define IDS_IMPEXP_FILEEXISTS           0x4217
#define IDS_IMPEXP_FILENOTFOUND         0x4218
#define IDS_IMPEXP_COMPLETE_IMPCK       0x4219
#define IDS_IMPEXP_COMPLETE_EXPCK       0x421A
#define IDS_IMPEXP_COMPLETE_IMPFV       0x421B
#define IDS_IMPEXP_COMPLETE_EXPFV       0x421C
#define IDS_IMPEXP_CAPTION              0x421D
#define IDS_NS3_VERSION_CAPTION         0x421E
#define IDS_NS4_FRIENDLY_PROFILE_NAME   0x421F
#define IDS_FB_FRIENDLY_PROFILE_NAME    0x4220
#define IDS_IMPEXP_CHOSEBOOKMARKFILE    0x4221
#define IDS_IMPEXP_CHOSECOOKIEFILE      0x4222
#define IDS_IMPEXP_BOOKMARKFILTER       0x4223
#define IDS_IMPEXP_COOKIEFILTER         0x4224

#define IDS_NETSCAPE_COOKIE_FILE        0x4225
#define IDS_NETSCAPE_BOOKMARK_FILE      0x4226
#define IDS_NETSCAPE_USERS_DIR          0x4227

#define IDC_IMPEXPACTIONDESCSTATIC      0x4261
#define IDC_IMPEXPBROWSE                0x4262
#define IDC_IMPEXPFAVTREE               0x4263

#define IDD_IMPEXPWELCOME               0x4264
#define IDD_IMPEXPTRANSFERTYPE          0x4265
#define IDD_IMPEXPIMPFAVSRC             0x4266
#define IDD_IMPEXPIMPFAVDES             0x4267
#define IDD_IMPEXPEXPFAVSRC             0x4268
#define IDD_IMPEXPEXPFAVDES             0x4269
#define IDD_IMPEXPIMPCKSRC              0x4270
#define IDD_IMPEXPEXPCKDES              0x4271
#define IDD_IMPEXPCOMPLETE              0x4272
#define IDC_IMPEXPACTIONLISTBOX         0x4273
#define IDC_IMPEXPEXTERNALCOMBO         0x4274
#define IDC_IMPEXPMANUAL                0x4275
#define IDC_IMPEXPRADIOAPP              0x4276
#define IDC_IMPEXPRADIOFILE             0x4287
#define IDC_IMPEXPTITLETEXT             0x4288
#define IDC_IMPEXPCOMPLETECONFIRM       0x4289

#define IDB_IMPEXPWATERMARK             0x428A
#define IDB_IMPEXPHEADER                0x428B

#define IDS_MIME_SAVEAS_HEADER_FROM     0x4300
#define IDS_SAVING_STATUS_TEXT          0x4301

#define IDS_CACHECLN_DISPLAY            0x5020
#define IDS_CACHECLN_DESCRIPTION        0x5021
#define IDS_CACHECLN_BTNTEXT            0x5022
#define IDS_CACHEOFF_DISPLAY            0x5023
#define IDS_CACHEOFF_DESCRIPTION        0x5024
#define IDS_CACHEOFF_BTNTEXT            0x5025

#define IDS_ON_DESKTOP                  0x6000
#define IDS_FIND_TITLE                  0x6001

#define IDS_RESET_WEB_SETTINGS_TITLE    0x6002
#define IDS_RESET_WEB_SETTINGS_SUCCESS  0x6003
#define IDS_RESET_WEB_SETTINGS_FAILURE  0x6004
#define IDD_RESET_WEB_SETTINGS          0x6005
#define IDC_RESET_WEB_SETTINGS_HOMEPAGE 0x6006

#define IDS_ERR_NAV_FAILED              0x6007
#define IDS_ERR_NAV_FAILED_TITLE        0x6008
