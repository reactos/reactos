#ifndef _IDS_H_
#define _IDS_H_
// IDs for common UI resources (note that these MUST BE decimal numbers)
// More IDs are in unicpp\resource.h

// Custom window styles

//
//  ES_OEMCONVERT95 does OEM conversion on Win95 but not on NT, because
//  NT suporrts happy unicode everywhere so we shouldn't restrict people
//  to OEM-only filenames.
//

// Turn on and off folder tool tips.  See foldertip.h
#include "foldertip.h"

#ifdef  WINNT
#define ES_OEMCONVERT95     0
#else
#define ES_OEMCONVERT95     ES_OEMCONVERT
#endif

// Bitmap resource

// Cursor resources
#define IDC_HELPCOLD    1001
#define IDC_HELPHOT     1002
#define IDC_SCOPY       1003
#define IDC_MCOPY       1004
#define IDC_NULL        1005

// IDs of Overlay Images
// These are here for compatibility reasons only, so don't change and add
// new values!!! (dli)
#define IDOI_SHARE      1
#define IDOI_LINK       2

#define ACCEL_DEFVIEW   1
#define ACCEL_PRN_QUEUE 2

// matches stuff in winuser.h
#define IDIGNOREALL             10

// Icon id's
#define IDI_DOCUMENT            1      // generic document (blank page)
#define IDI_DOCASSOC            2      // generic associated document (with stuff on the page)
#define IDI_APP                 3      // application (exe)
#define IDI_FOLDER              4      // folder
#define IDI_FOLDEROPEN          5      // open folder
#define IDI_DRIVE525            6      // 5.25 floppy
#define IDI_DRIVE35             7      // 3.5  floppy
#define IDI_DRIVEREMOVE         8      // Removeable drive
#define IDI_DRIVEFIXED          9      // fix disk, regular hard drive
#define IDI_DRIVENET            10     // Remote drive
#define IDI_DRIVENETDISABLED    11     // Remote drive icon (disconnected)
#define IDI_DRIVECD             12     // CD-ROM drive
#define IDI_DRIVERAM            13     // RAM drive
#define IDI_WORLD               14     // World
#define IDI_NETWORK             15     // Network
#define IDI_SERVER              16     // Server
#define IDI_PRINTER             17     // Printer
#define IDI_MYNETWORK           18     // The My Network icon
#define IDI_GROUP               19     // Group

// Startmenu images.
#define IDI_STPROGS             20
#define IDI_STDOCS              21
#define IDI_STSETNGS            22
#define IDI_STFIND              23
#define IDI_STHELP              24
#define IDI_STRUN               25
#define IDI_STSUSPEND           26
#define IDI_STEJECT             27
#define IDI_STSHUTD             28

// Overlays.
#define IDI_SHARE               29     // shared thing (overlap)
#define IDI_LINK                30     // link icon (overlap)
#define IDI_SLOWFILE            31     // slow file icon (overlap)
#define IDI_RECYCLER            32
#define IDI_RECYCLERFULL        33
#define IDI_RNA                 34     // Remote Network Services
#define IDI_DESKTOP             35     // Desktop icon

// More Startmenu images.
#define IDI_STCPANEL            36
#define IDI_STSPROGS            37
#define IDI_STPRNTRS            38
#define IDI_STFONTS             39
#define IDI_STTASKBR            40
#define IDI_CDAUDIO             41      // CD Audio Disc

#define IDI_TREE                42      // Network Directory Tree
#define IDI_STCPROGS            43
#define IDI_STFAV               44      // Start menu's favorite icon
#define IDI_STLOGOFF            45
#define IDI_STFLDRPROP          46
#define IDI_WINUPDATE           47

#define IDI_MU_SECURITY         48
#define IDI_MU_DISCONN          49

#define IDI_TB_DOCFIND_CLR      50
#define IDI_TB_DOCFIND_GRAY     51
#define IDI_TB_COMPFIND_CLR     52
#define IDI_TB_COMPFIND_GRAY    53

#define IDI_DRIVEUNKNOWN        54

// Misc icons
#define IDI_MULDOC              133     // multiple documents
#define IDI_DOCFIND             134     // Used for document find window...
#define IDI_COMPFIND            135     // Used For find Computer window...
// available 136

#define IDI_CPLFLD              137      // Control panel folder icon
#define IDI_PRNFLD              138      // Printers folder icon
#define IDI_NEWPRN              139      // New printer icon
#define IDI_PRINTER_NET         140      // Network printer icon
#define IDI_PRINTER_FILE        141      // File printer icon

#define IDI_DELETE_FILE         142      // delete file confirm icon
#define IDI_DELETE_FOLDER       143      // delete folder confirm icon
#define IDI_DELETE_MULTIPLE     144      // delete files and folders
#define IDI_REPLACE_FILE        145      // replace file icon
#define IDI_REPLACE_FOLDER      146      // replace folder
#define IDI_RENAME              147      // rename file/folder
#define IDI_MOVE                148      // move file/folder

#define IDI_INIFILE             151      // .ini file
#define IDI_TXTFILE             152      // .txt file
#define IDI_BATFILE             153      // .bat file
#define IDI_SYSFILE             154      // system file (.54, .vxd, ...)
#define IDI_FONFILE             155      // .fon
#define IDI_TTFFILE             156      // .ttf
#define IDI_PFMFILE             157      // .pfm (Type 1 font)

#define IDI_RUNDLG              160      // Icon in the Run dialog
#define IDI_NUKEFILE            161

#define IDI_BACKUP              165
#define IDI_CHKDSK              166
#define IDI_DEFRAG              167

#define IDI_DEF_PRINTER         168
#define IDI_DEF_PRINTER_NET     169
#define IDI_DEF_PRINTER_FILE    170
#define IDI_NDSCONTAINER        171      // Novell NDS Container
#define IDI_SERVERSHARE         172      // \\server\share icon
#define IDI_FAVORITES           173

#define IDI_ATTRIBS             174      // "Advanced" file/folder attribs icon
#define IDI_NETCONNECT          175

#define IDI_ADDNETPLACE         176      // Network Places Wizard

#define IDI_FOLDERVIEW          177
#define IDI_HTTFILE             178

#define IDI_CSC                 179     // ClientSideCaching

// Warning: Do not change the order and sequence of the following IDI_* values. The code
// assumes and asserts if the order changes.
#define IDI_ACTIVEDESK_ON       180
#define IDI_ACTIVEDESK_OFF      181
#define IDI_WEBVIEW_ON          182
#define IDI_WEBVIEW_OFF         183
#define IDI_SAME_WINDOW         184
#define IDI_SEPARATE_WINDOW     185
#define IDI_SINGLE_CLICK        186
#define IDI_DOUBLE_CLICK        187
// End of warning: Do not change the order of the above IDI_ values.

// Old icons
#define IDI_OLD_RECYCLER        191
#define IDI_OLD_RECYCLER_FULL   192
#define IDI_OLD_MYNETWORK       193

#define IDI_PASSWORD            194
#define IDI_PSEARCH             195     // Printers search icon

// unicpp\resource.h defines more icons starting at 200

// Add new icons here, and update ..\inc\shellp.h with the image index


// Bitmap id's

#define IDB_ABOUT16             130
#define IDB_ABOUT256            131
#define IDB_ABOUT_SA16          132

#define IDB_BRF_TB_SMALL        140
#define IDB_BRF_TB_LARGE        141
#define IDB_LINK_MERGE          142
#define IDB_PLUS_MERGE          143
#define IDB_TTBITMAP            145
#define IDB_HYPERVIEW           146
#define IDB_TICKER              147
#define IDB_NEWS                148

#define IDB_SORT_UP             133
#define IDB_SORT_DN             134

#define IDB_FSEARCHTB_HOT       135
#define IDB_FSEARCHTB_DEFAULT   136

#define IDB_ABOUTBAND16         137
#define IDB_ABOUTBAND256        138

#define IDB_ABOUT_SA256         139


// unicpp\resource.h defines more values starting at 256

#define IDA_SEARCH              150     // animation
#define IDA_FINDFILE            151     // animation
#define IDA_FINDCOMP            152     // animation for finding computers..

#define IDA_FILEMOVE            160     // animation file mode
#define IDA_FILECOPY            161     // animation file copy
#define IDA_FILEDEL             162     // animation move to waste basket
#define IDA_FILENUKE            163     // animation empty waste basket.
#define IDA_FILEDELREAL         164     // animation true delete bypass recycle bin
#define IDA_APPLYATTRIBS        165     // animation for applying file attributes
#define IDA_ISEARCH             166     // animation for finding URLs on the internet


// RCDATA IDs

#define RCDATA_FONTSHORTCUT      0x4000
#define RCDATA_ADMINTOOLSHORTCUT 0x4001
// Dialog box IDs (note that these MUST BE decimal numbers)

#define DLG_BROWSE              1001
#define DLG_RESTART             1002
#define DLG_RUN                 1003
#define DLG_LINK_SEARCH         1004
#define DLG_DISKFULL            1005
#define DLG_DISKFULL_NEW        1006
#define DLG_RUNUSERLOGON        1007
#define DLG_DEADSHORTCUT_MATCH  1008
#define DLG_DEADSHORTCUT        1009

#define DLG_LFNTOFAT            1010
#define DLG_DELETE_FILE         1011
#define DLG_DELETE_FOLDER       1012
#define DLG_DELETE_MULTIPLE     1013
#define DLG_REPLACE_FILE        1014
#define DLG_REPLACE_FOLDER      1015
#define DLG_MOVE_FILE           1016
#define DLG_MOVE_FOLDER         1017
#define DLG_RENAME_FILE         1018
#define DLG_RENAME_FOLDER       1019
#define DLG_MOVECOPYPROGRESS    1020
#define DLG_WONT_RECYCLE_FOLDER 1021
#define DLG_WONT_RECYCLE_FILE   1022
#define DLG_STREAMLOSS_ON_COPY  1023
#define DLG_RETRYFOLDERENUM     1024
#define DLG_DELETE_FILE_ARP     1025
#define DLG_PATH_TOO_LONG       1026
#define DLG_FAILED_ENCRYPT      1027
#define DLG_WONT_RECYCLE_OFFLINE 1028

#define DLG_LINKPROP            1040
#define DLG_FILEPROP            1041
#define DLG_FILEMULTPROP        1042
#define DLG_VERSION             1043
#define DLG_FOLDERPROP          1044
#define DLG_BITBUCKET_GENCONFIG 1045
#define DLG_BITBUCKET_CONFIG    1046
#define DLG_DELETEDFILEPROP     1047
#define DLG_FOLDERSHORTCUTPROP  1048

#define DLG_BROWSEFORDIR        1050
#define DLG_FINDEXE             1051
// these are still in shell.dll
// #define DLG_ABOUT            1052

#define DLG_RUNSETUPLOGON       1053
#define DLG_FILEATTRIBS         1054
#define DLG_FOLDERATTRIBS       1055
#define DLG_ATTRIBS_RECURSIVE   1056
#define DLG_APPLYATTRIBSPROGRESS 1057
#define DLG_ATTRIBS_ERROR       1058

#define DLG_PICKICON            1060
#define DLG_ASSOCIATE           1061
#define DLG_FIND                1062
#define DLG_OPENAS              1063
#define DLG_TSINSTALLFAILURE    1064
#define DLG_FIND_BROWSE         1065
#define DLG_DFNAMELOC           1066
#define DLG_DFDETAILS           1067
#define DLG_DFDATE              1068
#define DLG_NFNAMELOC           1069
#define DLG_OPENAS_NOTYPE       1070
#define DLG_LOGOFFWINDOWS       1071
#define DLG_PRN_GENERAL         1072
#define DLG_PRN_DETAIL          1073
#define DLG_PRN_SETUP           1074
#define DLG_SPOOLSETTINGS       1075 // advanced part of DLG_PRN_DETAIL
#define DLG_ADDPORT             1076 // advanced part of DLG_PRN_DETAIL
#define DLG_DELPORT             1077 // advanced part of DLG_PRN_DETAIL
#define DLG_PRN_QUEUE           1078

#define DLG_BROWSEFORFOLDER     1079  // Browse for folders for doc and net finds

#define DLG_DRV_GENERAL         1080
#define DLG_DISKTOOLS           1081

#define DLG_APPCOMPAT           1082

#ifdef MEMMON
#define DLG_MEMMON              1083
#endif

#define DLG_CPL_FILLCACHE       1084

#define DLG_APPCOMPATWARN       1085
#define DLG_DISKCOPYPROGRESS    1086

#define DLG_BROWSEFORFOLDER2    1087  // New UI

#define DLG_DRV_HWTAB           1088

#define DLG_DISKERR                         1095

#define DLG_COLUMN_SETTINGS                 1096
#define DLG_MOUNTEDDRV_GENERAL              1098

#define DLG_FILETYPEOPTIONS     1099
#define DLG_FILETYPEOPTIONSEDIT 1100
#define DLG_FILETYPEOPTIONSCMD  1101

#define DLG_DOCFIND_SAVE        1102

#define DLG_NOOPEN              1103

#define DLG_ENCRYPTWARNING      1104

#define DLG_FSEARCH_MAIN        1105
#define DLG_FSEARCH_DATE        1106
#define DLG_FSEARCH_TYPE        1107
#define DLG_FSEARCH_SIZE        1108
#define DLG_FSEARCH_ADVANCED    1109

#define DLG_INDEXSERVER         1110

#define DLG_FILETYPEOPTIONSEDITNEW          1111

#define DLG_PSEARCH             1112
#define DLG_CSEARCH             1113
#define DLG_FSEARCH_OPTIONS     1114

#define DLG_RENAME_MESSAGEBOXCHECK  1115
#define IDC_MBC_TEXT                0x2000
#define IDC_MBC_ICON                0x2001

#define DLG_FSEARCH_SCOPEMISMATCH    1116
#define DLG_FSEARCH_INDEXNOTCOMPLETE 1117

// for Dead shortcut dialogs
#define IDC_DEADTEXT1 0x100
#define IDC_DEADTEXT2 0x101
#define IDC_DELETE    0x105

// unicpp\resource.h defines more DLG_ values starting at 0x7500

// String IDs (these are hex so that groups of 16 are easily distinguished)

#define IDS_VERSIONMSG        60
#define IDS_DEBUG             61
#define IDS_LDK               62
#define IDS_PERCENTFREE       63
#define IDS_DATESIZELINE      64
#define IDS_FILEDELETEWARNING 65
#define IDS_FOLDERDELETEWARNING 66
#define IDS_FILERECYCLEWARNING 67
#define IDS_FOLDERRECYCLEWARNING 68
#define IDS_LICENCEINFOKEY      69
#define IDS_REGUSER             70
#define IDS_REGORGANIZATION     71
#define IDS_CURRENTVERSION      72
#ifdef WINNT
#define IDS_PRODUCTID           73
#define IDS_OEMID               74
#define IDS_PROCESSORINFOKEY    75
#define IDS_PROCESSORIDENTIFIER 76
#endif

// all commands that can have help in the status bar or tool tips
// need to be put before IDS_LAST_COMMAND
#define IDS_LAST_COMMAND        0x2FFF

#define IDS_FIRST               0x1000

#define IDS_REPLACING           0x1000
#define IDS_UNFORMATTED         0x1001
#define IDS_LOCATION            0x1002
#define IDS_DESTFULL            0x1003
#define IDS_WRITEPROTECTFILE    0x1004
#define IDS_NETERR              0x1005
#define IDS_DRIVENOTREADY       0x1006
#define IDS_CREATELONGDIR       0x1007
#define IDS_CREATELONGDIRTITLE  0x1008
#define IDS_FMTERROR            0x1009
#define IDS_NOFMT               0x100a
#define IDS_CANTSHUTDOWN        0x100b
#define IDS_INVALIDFN           0x100d
#define IDS_INVALIDFNFAT        0x100e
#define IDS_ENUMABORTED         0x100f
#define IDS_WARNCHANGEEXT       0x1010

#define IDS_BYTES               0x1011
#define IDS_FORMAT_TITLE        0x1012
#define IDS_MVUNFORMATTED       0x1013
#define IDS_SIZEANDBYTES        0x1016
#define IDS_SIZE                0x1017

#define IDS_NOWINDISK           0x1018
#define IDS_VERBHELP            0x1019

#define IDS_DRIVELETTER         0x101A
#define IDS_NONULLNAME          0x101B

#define IDS_FINDNOTFINDABLE     0x101C
#define IDS_FINDMAXFILESFOUND   0x101D
#define IDS_FINDMONITORNEWITEMS 0x101E
#define IDS_FINDNOTHINGFOUND    0x101F
#define IDS_PATHNOTTHERE        0x1020
#define IDS_FINDVIEWEMPTY       0x1021
#define IDS_FILETYPENAME        0x1022
#define IDS_FOLDERTYPENAME      0x1023
#define IDS_FILEWONTFIT         0x1024
#define IDS_FILE                0x1025
#define IDS_FINDFILES           0x1026
#define IDS_FINDWRONGPATH       0x1027
#define IDS_FINDDATAREQUIRED    0x1028
#define IDS_FINDINVALIDNUMBER   0x1029
#define IDS_FINDINVALIDDATE     0x102a
#define IDS_FINDGT              0x102b
#define IDS_FINDLT              0x102c
#define IDS_FINDRESET           0x102d
#define IDS_FINDALLFILETYPES    0x102e
#define IDS_FINDOUTOFMEM        0x102f

#define IDS_UNDO_FILEOP         0x102f
#define IDS_MOVE                (IDS_UNDO_FILEOP + FO_MOVE)
#define IDS_COPY                (IDS_UNDO_FILEOP + FO_COPY)
#define IDS_DELETE              (IDS_UNDO_FILEOP + FO_DELETE)
#define IDS_RENAME              (IDS_UNDO_FILEOP + FO_RENAME)

#define IDS_RUN_NORMAL          0x1034
#define IDS_RUN_MINIMIZED       0x1035
#define IDS_RUN_MAXIMIZED       0x1036

#define IDS_LINKTITLE           0x1037
#define IDS_LINKTO              0x1038
#define IDS_NONE                0x103b
#define IDS_NEW                 0x103c
#define IDS_CLOSE               0x103d
#define IDS_LINKEXTENSION       0x103e
#define IDS_ANOTHER             0x103f
#define IDS_YETANOTHER          0x1040

// This definition is hard coded in \nt\private\windows\shell\cpls\powercfg,
// if you change it here, change it there.
#define IDS_CONTROLPANEL        0x1041
#define IDS_DESKTOP             0x1042
#define IDS_UNDO                0x1043
#define IDS_UNDOACCEL           0x1044
#define IDS_UNDOMENU            0x1045
#define IDS_DOCUMENTFOLDERS     0x1046

#define IDS_SELECTALLBUTHIDDEN  0x104a
#define IDS_SELECTALL           0x104b
#define IDS_ACTIVEDESKTOP       0x104d
#define IDS_COMPSETTINGS        0x104e
#define IDS_BACKSETTINGS        0x104f
#define IDS_APPEARANCESETTINGS  0x1050

#define IDS_COPYLONGPLATE       0x1052
#define IDS_BRIEFTEMPLATE       0x1053
#define IDS_BRIEFEXT            0x1054
#define IDS_BRIEFLONGPLATE      0x1055
#define IDS_BOOKMARK_S          0x1056
#define IDS_BOOKMARK_L          0x1057
#define IDS_FINDINVALIDFILENAME 0x1058
#define IDS_SCRAP_S             0x105a
#define IDS_SCRAP_L             0x105b
#define IDS_FIND_DATE_TYPES     0x105c

#define IDS_UNDO_FILEOPHELP     0x105f
#define IDS_MOVEHELP            (IDS_UNDO_FILEOPHELP + FO_MOVE)
#define IDS_COPYHELP            (IDS_UNDO_FILEOPHELP + FO_COPY)
#define IDS_DELETEHELP          (IDS_UNDO_FILEOPHELP + FO_DELETE)
#define IDS_RENAMEHELP          (IDS_UNDO_FILEOPHELP + FO_RENAME)

#define IDS_HTML_FILE_RENAME    0x1068
#define IDS_HTML_FOLDER_RENAME  0x1069

#define IDS_THUMBNAILVIEW       0x106d
#define IDS_THUMBHELPTEXT       0x106e
//#define IDS_CANNOTENABLETHUMBS  0x106f

#define IDS_LINKERROR           0x1070
#define IDS_LINKBADWORKDIR      0x1071
#define IDS_LINKBADPATH         0x1072
#define IDS_LINKNOTFOUND        0x1073
#define IDS_LINKCHANGED         0x1074
#define IDS_SPACEANDSPACE       0x1075
#define IDS_COMMASPACE          0x1076
#define IDS_LINKUNAVAILABLE     0x1077
#define IDS_LINKNOTLINK         0x1078
#define IDS_LINKCANTSAVE        0x1079
#define IDS_LINKTOLINK          0x107A

//#define IDS_CANNOTDISABLETHUMBS 0x107B

#define IDS_VOLUMELABEL         0x107C
#define IDS_MOUNTEDVOLUME       0x107D
#define IDS_UNLABELEDVOLUME     0x107E

#define IDS_ENUMERR_NETTEMPLATE1        0x1080
#define IDS_ENUMERR_NETTEMPLATE2        0x1081
#define IDS_ENUMERR_FSTEMPLATE          0x1082
#define IDS_ENUMERR_NETGENERIC          0x1083
#define IDS_ENUMERR_FSGENERIC           0x1084
#define IDS_ENUMERR_PATHNOTFOUND        0x1085
#define IDS_SHLEXEC_ERROR               0x1086
#define IDS_SHLEXEC_ERROR2              0x1087
#define IDS_ENUMERR_PATHTOOLONG         0x1088

#define IDS_APP_FAULTED_IN              0x1089
#define IDS_APP_NOT_FAULTED_IN          0x108a

//#define IDS_ERR_SETVOLUMELABEL          0x1090
#define IDS_ERR_VOLUMELABELBAD          0x1091
#define IDS_TITLE_VOLUMELABELBAD        0x1092

#define IDS_COL_CM_MORE                  0x1099

#define IDS_DSPTEMPLATE_WITH_BACKSLASH  0x10a0
#define IDS_DSPTEMPLATE_WITH_ON         0x10a1

//
// RestartDialog Text Strings
//
#define IDS_RSDLG_TITLE             0x10b0
#define IDS_RSDLG_SHUTDOWN          0x10b1
#define IDS_RSDLG_RESTART           0x10b2
#define IDS_RSDLG_PIFFILENAME       0x10b3
#define IDS_RSDLG_PIFSHORTFILENAME  0x10b4

// CopyDisk text strings
#define IDS_INSERTDEST                  0x10C0
#define IDS_INSERTSRC                   0x10C1
#define IDS_INSERTSRCDEST               0x10C2
#define IDS_FORMATTINGDEST              0x10C3
#define IDS_COPYSRCDESTINCOMPAT         0x10C4


//
// Reserve a range for DefView MenuHelp
//
#define IDS_MH_SFVIDM_FIRST     0x1100
#define IDS_MH_SFVIDM_LAST      0x11ff

//
// Reserve a range for DefView client MenuHelp
//
#define IDS_MH_FSIDM_FIRST      0x1200
#define IDS_MH_FSIDM_LAST       0x12ff

//
// Reserve a range for DefView ToolTips
//
#define IDS_TT_SFVIDM_FIRST     0x1300
#define IDS_TT_SFVIDM_LAST      0x13ff

//
// Reserve a range for DefView client ToolTips
//
#define IDS_TT_FSIDM_FIRST      0x1400
#define IDS_TT_FSIDM_LAST       0x14ff

//
// IDS for Open With Context Menu
// 
#define IDS_OPENWITH            0x1500
#define IDS_OPENWITHNEW         0x1501
#define IDS_OPENWITHBROWSE      0x1502

// IDS for stream loss copy information

#define IDS_DOCSUMINFOSTREAM    0x1510
#define IDS_SUMINFOSTREAM       0x1511
#define IDS_MACINFOSTREAM       0x1512
#define IDS_MACRESSTREAM        0x1513
#define IDS_UNKNOWNPROPSET      0x1514
#define IDS_GLOBALINFOSTREAM    0x1516
#define IDS_IMAGECONTENTS       0x1517
#define IDS_IMAGEINFO           0x1518
#define IDS_USERDEFPROP         0x1519
#define IDC_STREAMSLOST         0x151a
#define IDS_AUDIOSUMINFO        0x151b
#define IDS_VIDEOSUMINFO        0x151c
#define IDS_MEDIASUMINFO        0x151d

#define IDS_FILEERROR           0x1700
#define IDS_FILEERRORCOPY       (IDS_FILEERROR + FO_COPY)
#define IDS_FILEERRORMOVE       (IDS_FILEERROR + FO_MOVE)
#define IDS_FILEERRORDEL        (IDS_FILEERROR + FO_DELETE)
#define IDS_FILEERRORREN        (IDS_FILEERROR + FO_RENAME)
// space needed

#define IDS_ACTIONTITLE         0x1740
#define IDS_ACTIONTITLECOPY     (IDS_ACTIONTITLE + FO_COPY)
#define IDS_ACTIONTITLEMOVE     (IDS_ACTIONTITLE + FO_MOVE)
#define IDS_ACTIONTITLEDEL      (IDS_ACTIONTITLE + FO_DELETE)
#define IDS_ACTIONTITLEREN      (IDS_ACTIONTITLE + FO_RENAME)

#define IDS_FROMTO              0x1750
#define IDS_FROM                0x1751
#define IDS_PREPARINGTO         0x1752
#define IDS_COPYTO              0x1753
#define IDS_COPYERROR           0x1754
#define IDS_MH_SORTBYFREESPACE  0x1755
#define IDS_COPYING             0x1756
#define IDS_MOVEERROR           0x1757
#define IDS_MOVING              0x1758
#define IDS_CALCMOVETIME        0x1759
#define IDS_CALCCOPYTIME        0x175A

// space needed

#define IDS_VERBS               0x1780
#define IDS_VERBSCOPY           (IDS_VERBS + FO_COPY)
#define IDS_VERBSMOVE           (IDS_VERBS + FO_MOVE)
#define IDS_VERBSDEL            (IDS_VERBS + FO_DELETE)
#define IDS_VERBSREN            (IDS_VERBS + FO_RENAME)
// space needed

#define IDS_ACTIONS             0x17c0
#define IDS_ACTIONS1            (IDS_ACTIONS + 1)
#define IDS_ACTIONS2            (IDS_ACTIONS + 2)
// space needed

#define IDS_REASONS             0x1800
// internal type errors
#define IDS_REASONS_INVFUNCTION (IDS_REASONS + DE_INVFUNCTION)
#define IDS_REASONS_INVHANDLE   (IDS_REASONS + DE_INVHANDLE)
#define IDS_REASONS_INVFILEACC  (IDS_REASONS + DE_INVFILEACCESS)
#define IDS_REASONS_NOTSAMEDEV  (IDS_REASONS + DE_NOTSAMEDEVICE)
#define IDS_REASONS_PATHTODEEP  (IDS_REASONS + DE_PATHTODEEP)
#define IDS_REASONS_DELCURDIR   (IDS_REASONS + DE_DELCURDIR)

// regular dos errors
#define IDS_REASONS_NOHANDLES   (IDS_REASONS + DE_NOHANDLES)
#define IDS_REASONS_FILENOFOUND (IDS_REASONS + DE_FILENOTFOUND)
#define IDS_REASONS_PATHNOFOUND (IDS_REASONS + DE_PATHNOTFOUND)
#define IDS_REASONS_ACCDENIED   (IDS_REASONS + DE_ACCESSDENIED)
#define IDS_REASONS_INSMEM      (IDS_REASONS + DE_INSMEM)
#define IDS_REASONS_NODIRENTRY  (IDS_REASONS + DE_NODIRENTRY)

// extended dos errors
#define IDS_REASONS_WRITEPROT   (IDS_REASONS + DE_WRITEPROTECTED)
#define IDS_REASONS_NETACCDEN   (IDS_REASONS + DE_ACCESSDENIEDNET)
#define IDS_REASONS_BADNETNAME  (IDS_REASONS + DE_BADNETNAME)
#define IDS_REASONS_SHAREVIOLA  (IDS_REASONS + DE_SHARINGVIOLATION)
#define IDS_REASONS_WRITEFAULT  (IDS_REASONS + DE_WRITEFAULT)
#define IDS_REASONS_GENFAILURE  (IDS_REASONS + DE_GENERALFAILURE)

// our internal errors
#define IDS_REASONS_NODISKSPACE (IDS_REASONS + DE_NODISKSPACE)
#define IDS_REASONS_SAMEFILE    (IDS_REASONS + DE_SAMEFILE)
#define IDS_REASONS_MANYSRC1DST (IDS_REASONS + DE_MANYSRC1DEST)
#define IDS_REASONS_DIFFDIR     (IDS_REASONS + DE_DIFFDIR)
#define IDS_REASONS_ROOTDIR     (IDS_REASONS + DE_ROOTDIR)
#define IDS_REASONS_DESTSUBTREE (IDS_REASONS + DE_DESTSUBTREE)
#define IDS_REASONS_WINDOWSFILE (IDS_REASONS + DE_WINDOWSFILE)
#define IDS_REASONS_ACCDENYSRC  (IDS_REASONS + DE_ACCESSDENIEDSRC)
#define IDS_REASONS_MANYDEST    (IDS_REASONS + DE_MANYDEST)
#define IDS_REASONS_RENREPLACE  (IDS_REASONS + ERROR_ALREADY_EXISTS)
#define IDS_REASONS_INVFILES    (IDS_REASONS + DE_INVALIDFILES)
#define IDS_REASONS_DESTSAMETREE (IDS_REASONS + DE_DESTSAMETREE)
#define IDS_REASONS_FLDDESTISFILE (IDS_REASONS + DE_FLDDESTISFILE)
#define IDS_REASONS_COMPRESSEDVOLUME (IDS_REASONS + DE_COMPRESSEDVOLUME)
#define IDS_REASONS_FILEDESTISFLD (IDS_REASONS + DE_FILEDESTISFLD)
#define IDS_REASONS_FILENAMETOOLONG (IDS_REASONS + DE_FILENAMETOOLONG)
#define IDS_REASONS_DEST_IS_CDROM (IDS_REASONS + DE_DEST_IS_CDROM)
#define IDS_REASONS_DEST_IS_DVD (IDS_REASONS + DE_DEST_IS_DVD)

// space needed

#define IDS_STILLNOTFOUND       0x191d
#define IDS_PROGFOUND           0x191e
#define IDS_PROGNOTFOUND        0x191f
#define IDS_NOCOMMDLG           0x1920

#define IDS_CANTDELETESPECIALDIR   0x1922
#define IDS_CANTMOVESPECIALDIRHERE 0x1923
#define IDS_WNETOPENENUMERR     0x1924
#define IDS_NEWFOLDER_NOT_HERE  0x1925


#define IDS_CANTLOGON           0x192A

#define IDS_AD_NAME             0x1930

#define IDS_SHARINGERROR        0x1933
#define IDS_COULDNOTSHARE       0x1934

#define IDS_CREATIONERROR       0x1935
#define IDS_COULDNOTCREATE      0x1936

//#define IDS_SPECIALSEARCHTITLE 0x1937
#define IDS_CANTFINDORIGINAL   0x1938
#define IDS_ORIGINALONDESKTOP  0x1939

#define IDS_FOLDERDOESNTEXIST  0x193A
#define IDS_CREATEFOLDERPROMPT 0x193B
#define IDS_CREATEFOLDERFAILED 0x193C
#define IDS_DIRCREATEFAILED_TITLE 0x193D
#define IDS_FOLDER_NOT_ALLOWED_TITLE 0x193E
#define IDS_FOLDER_NOT_ALLOWED 0x193F

#define IDS_FINDORIGINAL       0x1940
#define IDS_CANTFINDCOMPONENT  0x1941


// WARNING!  These must be in exactly the order below because the message
// number is computed.
#define DIDS_FSHIDDEN          1
#define DIDS_FSSPACE           2
#define IDS_FSSTATUSBASE        0x1942
#define IDS_FSSTATUSHIDDEN      0x1943  // IDS_FSSTATUSBASE + DIDS_FSHIDDEN
#define IDS_FSSTATUSSPACE       0x1944  // IDS_FSSTATUSBASE + DIDS_FSSPACE
#define IDS_FSSTATUSHIDDENSPACE 0x1945  // IDS_FSSTATUSBASE + DIDS_FSHIDDEN + DIDS_FSSPACE

#define IDS_DRIVESSTATUSTEMPLATE        0x1946
#define IDS_DETAILSUNKNOWN              0x1947
#define IDS_MOVEBRIEFCASE               0x1948
#define IDS_DELETEBRIEFCASE             0x1949
#define IDS_NOTCUSTOMIZABLE             0x194A
#define IDS_HTMLFILE_NOTFOUND           0x194B
#define IDS_CUSTOMIZE_THIS_FOLDER       0x194C

#define IDS_FSSTATUSSELECTED    0x194D

/* These defines are used by setup to modify the user and company name which
   the about box will display.  The location of the user and company name
   are determined by looking for a search tag in the string resource table
   just before the user and company name.  This is why it is very important
   that the following 3 IDS's always be consecutive and within the same
   resource segment.  The same resource segment can be guaranteed by ensuring
   that the IDS's all be within a 16-aligned page (i.e. (n*16) to (n*16 + 15).
 */
#define IDS_SEARCH_TAG          0x1980
#define IDS_USER_NAME           0x1981
#define IDS_ORG_NAME            0x1982


// shutdown dialog #defines - 0x2000 - 0x20FF
// strings
#define IDS_SHUTDOWN_NAME       0x2000
#define IDS_SHUTDOWN_DESC       0x2001
#define IDS_RESTART_NAME        0x2002
#define IDS_RESTART_DESC        0x2003
#define IDS_SLEEP_NAME          0x2004
#define IDS_SLEEP_DESC          0x2005
#define IDS_SLEEP2_NAME         0x2006
#define IDS_SLEEP2_DESC         0x2007
#define IDS_HIBERNATE_NAME      0x2008
#define IDS_HIBERNATE_DESC      0x2009
#define IDS_LOGOFF_NAME         0x200A
#define IDS_LOGOFF_DESC         0x200B
#define IDS_RESTARTDOS_NAME     0x200C
#define IDS_RESTARTDOS_DESC     0x200D

// dialog and controls
#define IDC_EXITOPTIONS_COMBO   0x2020
#define IDC_EXITOPTIONS_DESCRIPTION 0x2021
#define IDD_EXITWINDOWS_DIALOG  0x2022

// icon
#define IDI_SHUTDOWN            0x2030


#ifdef UNICODE
#define IDS_PathNotFound         IDS_PathNotFoundW
#else // UNICODE
#define IDS_PathNotFound         IDS_PathNotFoundA
#endif // UNICODE

// these are bogus

#define IDS_LowMemError          0x2100
#define IDS_RunFileNotFound      0x2101
#define IDS_PathNotFoundA        0x2102
#define IDS_TooManyOpenFiles     0x2103
#define IDS_RunAccessDenied      0x2104
#define IDS_OldWindowsVer        0x2105
#define IDS_OS2AppError          0x2106
#define IDS_MultipleDS           0x2107
#define IDS_InvalidDLL           0x2108
#define IDS_ShareError           0x2109
#define IDS_AssocIncomplete      0x210a
#define IDS_DDEFailError         0x210b
#define IDS_NoAssocError         0x210c
#define IDS_BadFormat            0x210d
#define IDS_RModeApp             0x210e
#define IDS_PathNotFoundW        0x210f

#define IDS_MENUOPEN            0x2130
#define IDS_MENUPRINT           0x2131
#define IDS_MENUEXPLORE         0x2136
#define IDS_MENUFIND            0x2137
#define IDS_MENUAUTORUN         0x2138
#define IDS_MENURUNAS           0x2139
#define IDS_HELPOPEN            0x2140
#define IDS_HELPPRINT           0x2141
#define IDS_HELPPRINTTO         0x2142
#define IDS_HELPOPENAS          0x2143

#define IDS_EXITHELP            0x2150
#define IDS_WINDOWS_HLP         0x2151
#define IDS_WINDOWS             0x2152

// string ids for shpsht.c
#define IDS_NOPAGE              0x21f0

// string ids for mulprsht.c
#define IDS_MULTIPLEFILES       0x2200
#define IDS_MULTIPLETYPES       0x2201
#define IDS_ALLIN               0x2202
#define IDS_ALLOFTYPE           0x2203
#define IDS_MULTIPLEOBJECTS     0x2204
#define IDS_VARFOLDERS          0x2205
#define IDS_FOLDERSIZE          0x2206
#define IDS_NUMFILES            0x2207
#define IDS_ONEFILEPROP         0x2208
#define IDS_MANYFILEPROP        0x2209

// string ids for pickicon.c
#define IDS_BADPATHMSG          0x2210
#define IDS_NOICONSMSG1         0x2211
#define IDS_NOICONSMSG          0x2212

//#define IDS_CANNOTSETATTRIBUTES 0x2213
#define IDS_MAKINGDESKTOPLINK   0x2214
#define IDS_TRYDESKTOPLINK      0x2215
#define IDS_WOULDYOUCREATELINK  0x2216
#define IDS_CANNOTCREATEFILE    0x2218
#define IDS_CANNOTCREATELINK    0x2219
#define IDS_CANNOTCREATEFOLDER  0x221A

#define IDS_NFILES              0x2220
#define IDS_SELECTEDFILES       0x2221

// string ids for copy.c
// #define unused               0x2222
#define IDS_TIMEEST_MINUTES     0x2223
#define IDS_TIMEEST_SECONDS     0x2224

// string ids for version.c

#define IDS_VN_COMMENTS         0x2230
#define IDS_VN_COMPANYNAME      0x2231
#define IDS_VN_FILEDESCRIPTION  0x2232
#define IDS_VN_INTERNALNAME     0x2233
#define IDS_VN_LEGALTRADEMARKS  0x2234
#define IDS_VN_ORIGINALFILENAME 0x2235
#define IDS_VN_PRIVATEBUILD     0x2236
#define IDS_VN_PRODUCTNAME      0x2237
#define IDS_VN_PRODUCTVERSION   0x2238
#define IDS_VN_SPECIALBUILD     0x2239
#define IDS_VN_FILEVERSIONKEY   0x223A
#define IDS_VN_LANGUAGE         0x223B
#define IDS_VN_LANGUAGES        0x223C

// strings for "Run as" dialog
#define IDS_THECURRENTUSER      0x2240
#define IDS_ADMINISTRATOR       0x2241

// String ids for Associate dialog
//#define IDS_ASSOCIATE           0x2300
//#define IDS_ASSOCNONE           0x2301
//#define IDS_ASSOCNOTEXE         0x2302
//#define IDS_NOEXEASSOC          0x2303
#define IDS_WASTEBASKET         0x2304

// unused, use me!              0x2305
// unused, use me!              0x2306

#define IDS_RENAMEFILESINREG    0x2307
#define IDS_RECYCLEBININVALIDFORMAT 0x2308

// column headers for various listviews
#define IDS_NAME_COL            0x2310
#define IDS_PATH_COL            0x2311
#define IDS_SIZE_COL            0x2312
#define IDS_TYPE_COL            0x2313
#define IDS_MODIFIED_COL        0x2314
#define IDS_STATUS_COL          0x2315
#define IDS_ORIGINAL_COL        0x2316
#define IDS_COMMENT_COL         0x2317
#define IDS_WORKGROUP_COL       0x2318
#define IDS_DELETEDFROM_COL     0x2319
#define IDS_DATEDELETED_COL     0x231A
#define IDS_ATTRIB_COL          0x231B
#define IDS_ATTRIB_CHARS        0x231C
#define IDS_RANK_COL            0x231D
#define IDS_DESCRIPTION_COL     0x231E

#define IDS_EXCOL_TITLE         0x2320
#define IDS_EXCOL_AUTHOR        0x2321
#define IDS_EXCOL_PAGECOUNT     0x2322
#define IDS_EXCOL_COMMENT       0x2323
#define IDS_EXCOL_CREATE        0x2324
#define IDS_EXCOL_ACCESSTIME    0x2325
#define IDS_EXCOL_OWNER         0x2326

#define IDS_EXCOL_SUBJECT       0x2327
#define IDS_EXCOL_TEMPLATE      0x2328

#define IDS_FILEFOUND           0x2330
#define IDS_FILENOTFOUND        0x2331
#define IDS_FINDASSEXEBROWSETITLE 0x2332

#define IDS_DRIVETSHOOT         0x2333
#define IDS_SYSDMCPL            0x2334
#define IDS_EXE                 0x2335
#define IDS_PROGRAMSFILTER      0x2336
#define IDS_BROWSE              0x2337
#define IDS_OPENAS              0x2338
#define IDS_CLP                 0x2339
#define IDS_SEPARATORFILTER     0x233A
#define IDS_ICO                 0x233B
#define IDS_ICONSFILTER         0x233C
#define IDS_ALLFILESFILTER      0x233D
#define IDS_SAVEAS              0x233E
#define IDS_REASONS_URLINTEMPDIR 0x233F

// wastebasket strings
#define IDS_BB_RESTORINGFILES               0x2340
#define IDS_BB_EMPTYINGWASTEBASKET          0x2341
#define IDS_BB_DELETINGWASTEBASKETFILES     0x2342
#define IDS_BB_CANNOTCHANGESETTINGS         0x2343
#define IDS_APPWIZCPL                       0x2344

#define IDS_NO_BACKUP_APP                   0x2350
#define IDS_NO_OPTIMISE_APP                 0x2351
#define IDS_NO_DISKCHECK_APP                0x2352
#define IDS_NO_CLEANMGR_APP                 0x2353

// Drives Hardware tab string
#define IDS_THESEDRIVES         0x2354

//  Some more extended columns:
#define IDS_EXCOL_CATEGORY      0x2355
#define IDS_EXCOL_COPYRIGHT     0x2356
#define IDS_EXCOL_LAST          0x235F

// String ids for names of special ID Lists
#define IDS_CSIDL_HISTORY                   0x2370
#define IDS_CSIDL_COOKIES                   0x2371
#define IDS_CSIDL_CACHE                     0x2372
#define IDS_CSIDL_PRINTHOOD                 0x2373
#define IDS_CSIDL_MYPICTURES                0x2374
#define IDS_CSIDL_FONTS                     0x2375
#define IDS_CSIDL_TEMPLATES                 0x2376
#define IDS_CSIDL_PROGRAM_FILES             0x2378
#define IDS_CSIDL_ALTSTARTUP                0x2380
#define IDS_CSIDL_LOCAL_APPDATA             0x2381
#define IDS_CSIDL_APPDATA                   0x2382

#define IDS_CSIDL_DESKTOPDIRECTORY          0x2384

#define IDS_CSIDL_PROGRAMS                  0x2386

#define IDS_CSIDL_RECENT                    0x2388
#define IDS_CSIDL_SENDTO                    0x238a
#define IDS_CSIDL_PERSONAL                  0x238c
#define IDS_CSIDL_STARTUP                   0x238e

#define IDS_CSIDL_STARTMENU                 0x2390

#define IDS_CSIDL_FAVORITES                 0x2392

#define IDS_CSIDL_NETHOOD                   0x2394

#define IDS_CSIDL_ALLUSERS_DOCUMENTS        0x2398

#define IDS_WINHELPERROR                0x239B
#define IDS_WINHELPTITLE                0x239C

#define IDS_CSIDL_ADMINTOOLS                0x239D
#define IDS_CSIDL_COMMON_ADMINTOOLS         0x239E

#define IDS_QUICKLAUNCH                     0x239F

// String ids for the Root of All Evil (ultroot.c)
#define IDS_ROOTNAMES                   0x2400
#define IDS_DRIVEROOT                   (IDS_ROOTNAMES+0x00)
#define IDS_NETWORKROOT                 (IDS_ROOTNAMES+0x01)
#define IDS_RESTOFNET                   (IDS_ROOTNAMES+0x02)

// These are not roots, but save number of string tables...
#define IDS_525_FLOPPY_DRIVE            (IDS_ROOTNAMES+0x03)
#define IDS_35_FLOPPY_DRIVE             (IDS_ROOTNAMES+0x04)
#define IDS_UNK_FLOPPY_DRIVE            (IDS_ROOTNAMES+0x05)

// Okay, so this is a root...
#define IDS_INETROOT                    (IDS_ROOTNAMES+0x06)

// More that are not roots...
#define IDS_UNC_FORMAT                  (IDS_ROOTNAMES+0x07)
#define IDS_VOL_FORMAT                  (IDS_ROOTNAMES+0x08)

#define IDS_525_FLOPPY_DRIVE_UGLY       (IDS_ROOTNAMES+0x09)
#define IDS_35_FLOPPY_DRIVE_UGLY        (IDS_ROOTNAMES+0x0a)

#define IDS_MYDOCUMENTS                 (IDS_ROOTNAMES+0x0b)

#define IDS_VOL_FORMAT_LETTER_1ST       (IDS_ROOTNAMES+0x0c)

#define IDS_NECUNK_FLOPPY_DRIVE         (IDS_ROOTNAMES+0x0f)

// String ids for the Find dialog
#define IDS_FINDDLG                 0x2410
#define IDS_FILESFOUND                  (IDS_FINDDLG + 0x00)
#define IDS_COMPUTERSFOUND              (IDS_FINDDLG + 0x01)
#define IDS_SEARCHING                   (IDS_FINDDLG + 0x02)
#define IDS_FIND_SELECT_PATH            (IDS_FINDDLG + 0x03)
#define IDS_FIND_TITLE_NAME             (IDS_FINDDLG + 0x04)
#define IDS_FIND_TITLE_TYPE             (IDS_FINDDLG + 0x05)
#define IDS_FIND_TITLE_TYPE_NAME        (IDS_FINDDLG + 0x06)
#define IDS_FIND_TITLE_TEXT             (IDS_FINDDLG + 0x07)
#define IDS_FIND_TITLE_NAME_TEXT        (IDS_FINDDLG + 0x08)
#define IDS_FIND_TITLE_TYPE_TEXT        (IDS_FINDDLG + 0x09)
#define IDS_FIND_TITLE_TYPE_NAME_TEXT   (IDS_FINDDLG + 0x0a)
#define IDS_FIND_TITLE_ALL              (IDS_FINDDLG + 0x0b)
#define IDS_FIND_TITLE_COMPUTER         (IDS_FINDDLG + 0x0c)
#define IDS_FIND_SHORT_NAME             (IDS_FINDDLG + 0x0d)
#define IDS_FIND_TITLE_FIND             (IDS_FINDDLG + 0x0e)
#define IDS_FINDSEARCHTITLE             (IDS_FINDDLG + 0x0f)
#define IDS_FINDSEARCH_COMPUTER         (IDS_FINDDLG + 0x10)
#define IDS_FINDSEARCH_PRINTER          (IDS_FINDDLG + 0x11)
#define IDS_FINDSEARCH_ALLDRIVES        (IDS_FINDDLG + 0x12)
#define IDS_FINDFILESFILTER             (IDS_FINDDLG + 0x13)
#define IDS_FINDSAVERESULTSTITLE        (IDS_FINDDLG + 0x14)
#define IDS_SEARCHINGASYNC              (IDS_FINDDLG + 0x15)

// For menu help, must match IDM_xxx numbers
#define IDS_FIND_STATUS_FIRST           (IDS_FINDDLG + 0x20) // 0x2430
#define IDS_FIND_STATUS_OPENCONT        (IDS_FIND_STATUS_FIRST + 0)
#define IDS_FIND_STATUS_CASESENSITIVE   (IDS_FIND_STATUS_FIRST + 1)
#define IDS_FIND_STATUS_SAVESRCH        (IDS_FIND_STATUS_FIRST + 3)
#define IDS_FIND_STATUS_CLOSE           (IDS_FIND_STATUS_FIRST + 4)
#define IDS_FIND_STATUS_SAVERESULTS     (IDS_FIND_STATUS_FIRST + 5)
#define IDS_FIND_STATUS_WHATSTHIS       (IDS_FIND_STATUS_FIRST + 7)

// Control Panel stuff
#define IDS_CONTROL_START           0x2450
//#define IDS_LOADING                     (IDS_CONTROL_START+0x00)
//#define IDS_NAME                        (IDS_CONTROL_START+0x01)
#define IDS_CPL_EXCEPTION               (IDS_CONTROL_START+0x02)
//#define IDS_CPLINFO                     (IDS_CONTROL_START+0x03)

// Printer stuff
#define IDS_PRINTER_START           (IDS_CPL_EXCEPTION+2)
#define IDS_NEWPRN                      (IDS_PRINTER_START+0x00)
#define IDS_PRINTERS                    (IDS_PRINTER_START+0x01)
#define IDS_CHANGEDEFAULTPRINTER        (IDS_PRINTER_START+0x02)
//#define IDS_CHANGEPRINTPROCESSOR        (IDS_PRINTER_START+0x03)
//#define IDS_NETAVAIL_ALWAYS             (IDS_PRINTER_START+0x04)
//#define IDS_NETAVAIL_FMT                (IDS_PRINTER_START+0x05)
#define IDS_CANTVIEW_FILEPRN            (IDS_PRINTER_START+0x06)
#define IDS_PRINTERNAME_CHANGED         (IDS_PRINTER_START+0x07)
//#define IDS_PRINTERSINFOLDER            (IDS_PRINTER_START+0x08)
//#define IDS_WORKONLINE                  (IDS_PRINTER_START+0x09)
#define IDS_PRINTINGERROR               (IDS_PRINTER_START+0x0a)
#define IDS_SECURITYDENIED              (IDS_PRINTER_START+0x0b)
#define IDS_SECURITYDENIED_JOB          (IDS_PRINTER_START+0x0c)
#define IDS_MULTIPLEPRINTFILE           (IDS_PRINTER_START+0x0d)
#define IDS_NO_WORKONLINE               (IDS_PRINTER_START+0x0e)
#define IDS_CANTOPENMODALPROP           (IDS_PRINTER_START+0x0f)
#define IDS_CANTOPENDRIVERPROP          (IDS_PRINTER_START+0x10)
#define IDS_CANTPRINT                   (IDS_PRINTER_START+0x11)
//#define IDS_SERVERPROPERTIES            (IDS_PRINTER_START+0x12)

#define IDS_MUSTCOMPLETE                (IDS_PRINTER_START+0x13)
#define IDS_NETPRN_START            (IDS_MUSTCOMPLETE+1)
#define IDS_INSTALLNETPRINTER           (IDS_NETPRN_START+0x00)
#define IDS_REINSTALLNETPRINTER         (IDS_NETPRN_START+0x01)
#define IDS_CANTINSTALLRESOURCE         (IDS_NETPRN_START+0x02)

#define IDS_PSD_START               (IDS_CANTINSTALLRESOURCE+1)
#define IDS_PSD_PRNNAME                 (IDS_PSD_START+0x00)
#define IDS_PSD_QUEUESIZE               (IDS_PSD_START+0x01)
#define IDS_PSD_COMMENT                 (IDS_PSD_START+0x02)

#define IDS_PRQ_START               (IDS_PSD_COMMENT+1)
#define IDS_PRQ_STATUS                  (IDS_PRQ_START+0x00)
#define IDS_PRQ_DOCNAME                 (IDS_PRQ_START+0x01)
#define IDS_PRQ_OWNER                   (IDS_PRQ_START+0x02)
#define IDS_PRQ_TIME                    (IDS_PRQ_START+0x03)
#define IDS_PRQ_PROGRESS                (IDS_PRQ_START+0x04)
#define IDS_PRQ_PAGES                   (IDS_PRQ_START+0x05)
#define IDS_PRQ_PAGESPRINTED            (IDS_PRQ_START+0x06)
#define IDS_PRQ_BYTESPRINTED            (IDS_PRQ_START+0x07)
#define IDS_PRQ_JOBSINQUEUE             (IDS_PRQ_START+0x08)

#define IDS_PRQSTATUS_START         (IDS_PRQ_JOBSINQUEUE+1)
#define IDS_PRQSTATUS_SEPARATOR         (IDS_PRQSTATUS_START+0x00)
#define IDS_PRQSTATUS_PAUSED            (IDS_PRQSTATUS_START+0x01)
#define IDS_PRQSTATUS_ERROR             (IDS_PRQSTATUS_START+0x02)
#define IDS_PRQSTATUS_PENDING_DELETION  (IDS_PRQSTATUS_START+0x03)
#define IDS_PRQSTATUS_PAPER_JAM         (IDS_PRQSTATUS_START+0x04)
#define IDS_PRQSTATUS_PAPER_OUT         (IDS_PRQSTATUS_START+0x05)
#define IDS_PRQSTATUS_MANUAL_FEED       (IDS_PRQSTATUS_START+0x06)
#define IDS_PRQSTATUS_PAPER_PROBLEM     (IDS_PRQSTATUS_START+0x07)
#define IDS_PRQSTATUS_OFFLINE           (IDS_PRQSTATUS_START+0x08)
#define IDS_PRQSTATUS_IO_ACTIVE         (IDS_PRQSTATUS_START+0x09)
#define IDS_PRQSTATUS_BUSY              (IDS_PRQSTATUS_START+0x0a)
#define IDS_PRQSTATUS_PRINTING          (IDS_PRQSTATUS_START+0x0b)
#define IDS_PRQSTATUS_OUTPUT_BIN_FULL   (IDS_PRQSTATUS_START+0x0c)
#define IDS_PRQSTATUS_NOT_AVAILABLE     (IDS_PRQSTATUS_START+0x0d)
#define IDS_PRQSTATUS_WAITING           (IDS_PRQSTATUS_START+0x0e)
#define IDS_PRQSTATUS_PROCESSING        (IDS_PRQSTATUS_START+0x0f)
#define IDS_PRQSTATUS_INITIALIZING      (IDS_PRQSTATUS_START+0x10)
#define IDS_PRQSTATUS_WARMING_UP        (IDS_PRQSTATUS_START+0x11)
#define IDS_PRQSTATUS_TONER_LOW         (IDS_PRQSTATUS_START+0x12)
#define IDS_PRQSTATUS_NO_TONER          (IDS_PRQSTATUS_START+0x13)
#define IDS_PRQSTATUS_PAGE_PUNT         (IDS_PRQSTATUS_START+0x14)
#define IDS_PRQSTATUS_USER_INTERVENTION (IDS_PRQSTATUS_START+0x15)
#define IDS_PRQSTATUS_OUT_OF_MEMORY     (IDS_PRQSTATUS_START+0x16)
#define IDS_PRQSTATUS_DOOR_OPEN         (IDS_PRQSTATUS_START+0x17)
#define IDS_PRQSTATUS_UNAVAILABLE       (IDS_PRQSTATUS_START+0x18)
#define IDS_PRQSTATUS_PRINTED           (IDS_PRQSTATUS_START+0x19)
#define IDS_PRQSTATUS_SPOOLING          (IDS_PRQSTATUS_START+0x1a)
#define IDS_PRQSTATUS_WORK_OFFLINE      (IDS_PRQSTATUS_START+0x1b)

#define IDS_PRTPROP_START           (IDS_PRQSTATUS_WORK_OFFLINE+1)
#define IDS_PRTPROP_DRIVER_WARN         (IDS_PRTPROP_START+0x00)
#define IDS_PRTPROP_RENAME_ERROR        (IDS_PRTPROP_START+0x01)
#define IDS_PRTPROP_RENAME_NULL         (IDS_PRTPROP_START+0x02)
#define IDS_PRTPROP_RENAME_BADCHARS     (IDS_PRTPROP_START+0x03)
#define IDS_PRTPROP_RENAME_TOO_LONG     (IDS_PRTPROP_START+0x04)
#define IDS_PRTPROP_PORT_ERROR          (IDS_PRTPROP_START+0x05)
#define IDS_PRTPROP_SEP_ERROR           (IDS_PRTPROP_START+0x06)
#define IDS_PRTPROP_UNKNOWN_ERROR       (IDS_PRTPROP_START+0x07)
#define IDS_PRTPROP_CANNOT_OPEN         (IDS_PRTPROP_START+0x08)
#define IDS_PRTPROP_PORT_FORMAT         (IDS_PRTPROP_START+0x09)
#define IDS_PRTPROP_TESTPAGE_WARN       (IDS_PRTPROP_START+0x0A)
#define IDS_PRTPROP_ADDPORT_CANTDEL_BUSY  (IDS_PRTPROP_START+0x0B)
#define IDS_PRTPROP_ADDPORT_CANTDEL_LOCAL (IDS_PRTPROP_START+0x0C)
#define IDS_PRTPROP_UNIQUE_FORMAT       (IDS_PRTPROP_START+0x0D)
#define IDS_PRTPROP_UNKNOWNERROR        (IDS_PRTPROP_START+0x0E)

#define IDS_PRNSEP_START            (IDS_PRTPROP_UNKNOWNERROR+1)
#define IDS_PRNSEP_NONE                 (IDS_PRNSEP_START+0x00)
#define IDS_PRNSEP_SIMPLE               (IDS_PRNSEP_START+0x01)
#define IDS_PRNSEP_FULL                 (IDS_PRNSEP_START+0x02)

#define IDS_DELETE_START            (IDS_PRNSEP_FULL+1)
#define IDS_SUREDELETE                  (IDS_DELETE_START+0x00)
#define IDS_SUREDELETEREMOTE            (IDS_DELETE_START+0x01)
#define IDS_SUREDELETECONNECTION        (IDS_DELETE_START+0x02)
#define IDS_DELNEWDEFAULT               (IDS_DELETE_START+0x03)
#define IDS_DELNODEFAULT                (IDS_DELETE_START+0x04)
#define IDS_DELETE_END              IDS_DELNODEFAULT

#define IDS_DRIVES_START            (IDS_DELETE_END+1)
#define IDS_DRIVES_NAME                 (IDS_DRIVES_START+0x00)
#define IDS_DRIVES_TYPE                 (IDS_DRIVES_START+0x01)
#define IDS_DRIVES_SIZE                 (IDS_DRIVES_START+0x02)
#define IDS_DRIVES_FREE                 (IDS_DRIVES_START+0x03)

#define IDS_DRIVES_NETUNAVAIL           (IDS_DRIVES_START+0x04)
#define IDS_DRIVES_REMOVABLE            (IDS_DRIVES_START+0x05)
#define IDS_DRIVES_DRIVE525             (IDS_DRIVES_START+0x06)
#define IDS_DRIVES_DRIVE35              (IDS_DRIVES_START+0x07)
#define IDS_DRIVES_DRIVE525_UGLY        (IDS_DRIVES_START+0x08)
#define IDS_DRIVES_DRIVE35_UGLY         (IDS_DRIVES_START+0x09)
#define IDS_DRIVES_UGLY_TEST            (IDS_DRIVES_START+0x0a)

#define IDS_DRIVES_FIXED                (IDS_DRIVES_START+0x0b)
#define IDS_DRIVES_DVD                  (IDS_DRIVES_START+0x0c)
#define IDS_DRIVES_CDROM                (IDS_DRIVES_START+0x0d)
#define IDS_DRIVES_RAMDISK              (IDS_DRIVES_START+0x0e)
#define IDS_DRIVES_NETDRIVE             (IDS_DRIVES_START+0x0f)
#define IDS_DRIVES_REGITEM              (IDS_DRIVES_START+0x10)

#ifdef WINNT
//#define IDS_DRIVES_COMPRESS             (IDS_DRIVES_START+0x11)
#define IDS_DRIVES_NOOPTINSTALLED       (IDS_DRIVES_START+0x12)
//#define IDS_DRIVES_ENCRYPT              (IDS_DRIVES_START+0x13)
#endif

#define IDS_DRIVES_LASTCHECKDAYS        (IDS_DRIVES_START+0x16)
#define IDS_DRIVES_LASTBACKUPDAYS       (IDS_DRIVES_START+0x17)
#define IDS_DRIVES_LASTOPTIMIZEDAYS     (IDS_DRIVES_START+0x18)

#define IDS_DRIVES_LASTCHECKUNK         (IDS_DRIVES_START+0x19)
#define IDS_DRIVES_LASTBACKUPUNK        (IDS_DRIVES_START+0x20)
#define IDS_DRIVES_LASTOPTIMIZEUNK      (IDS_DRIVES_START+0x21)

#define IDS_DRIVES_END              IDS_DRIVES_LASTOPTIMIZEUNK

//#define IDS_LOADERR                 0x2500
//#define IDS_LOADERROR_UNKNOWN           (IDS_LOADERR-1)
//#define IDS_LOADERROR_MEMORY            (IDS_LOADERR+0)
//#define IDS_LOADERROR_CANTOPEN          (IDS_LOADERR+2)
//#define IDS_LOADERROR_CANTRUN           (IDS_LOADERR+6)
//#define IDS_LOADERROR_VERPROB           (IDS_LOADERR+10)
//#define IDS_LOADERROR_RMODE             (IDS_LOADERR+15)
//#define IDS_LOADERROR_SINGLEINST        (IDS_LOADERR+16)
//#define IDS_LOADERROR_SHARE             (IDS_LOADERR+SE_ERR_SHARE)
//#define IDS_LOADERROR_ASSOC             (IDS_LOADERR+SE_ERR_ASSOCINCOMPLETE)
//#define IDS_LOADERROR_DDETIMEOUT        (IDS_LOADERR+SE_ERR_DDETIMEOUT)
//#define IDS_LOADERROR_NOASSOC           (IDS_LOADERR+SE_ERR_NOASSOC)

#define IDS_APPCOMPATMSG                0x25A0
#define IDS_APPCOMPATWIN95              0x25A0
#define IDS_APPCOMPATWIN95L             0x25A1
#define IDS_APPCOMPATWIN95H             0x25A2
//#define IDS_APPCOMPATMEMPHIS            0x25A2
//#define IDS_APPCOMPATMEMPHISL           0x25A3
//#define IDS_APPCOMPATIE4                0x25A4
//#define IDS_APPCOMPATIE4L               0x25A5

#define IDS_PSD_LOCATION                0x25A6
#define IDS_PSD_MODEL                   0x25A7
#define IDS_PRN_INFOTIP_START           0x25A8
#define IDS_PRN_INFOTIP_NAME_FMT        0x25A9
#define IDS_PRN_INFOTIP_STATUS_FMT      0x25AA
#define IDS_PRN_INFOTIP_QUEUESIZE_FMT   0x25AB
#define IDS_PRN_INFOTIP_COMMENT_FMT     0x25AC
#define IDS_PRN_INFOTIP_LOCATION_FMT    0x25AD
#define IDS_PRN_INFOTIP_READY           0x25AE
#define IDS_PRN_OPNOTSUPPORTED          0x25AF
#define IDS_SUREPURGE                   0x25B0
#define IDS_SUREDELETECONNECTIONNOSERVERNAME 0x25B1

#define IDS_RESTRICTIONSTITLE           0x2600
#define IDS_RESTRICTIONS                0x2601

#define IDS_RESTOFNETTIP                0x2602

// Strings for pifmgr code

#define IDS_PIFPAGE_FIRST       0x2650
#define IDS_PIF_NONE            (IDS_PIFPAGE_FIRST+0x00)
//#define IDS_NONE_ABOVE          (IDS_PIFPAGE_FIRST+0x01)
#define IDS_AUTO                (IDS_PIFPAGE_FIRST+0x02)
#define IDS_AUTONORMAL          (IDS_PIFPAGE_FIRST+0x03)
#define IDS_PREVIEWTEXT         (IDS_PIFPAGE_FIRST+0x04)
#define IDS_NO_ICONS            (IDS_PIFPAGE_FIRST+0x05)
#define IDS_QUERY_ERROR         (IDS_PIFPAGE_FIRST+0x06)
#define IDS_UPDATE_ERROR        (IDS_PIFPAGE_FIRST+0x07)
#define IDS_PREVIEWTEXT_BILNG   (IDS_PIFPAGE_FIRST+0x08)

#define IDS_BAD_HOTKEY          (IDS_PIFPAGE_FIRST+0x0A)
#define IDS_BAD_MEMLOW          (IDS_PIFPAGE_FIRST+0x0B)
#define IDS_BAD_MEMEMS          (IDS_PIFPAGE_FIRST+0x0C)
#define IDS_BAD_MEMXMS          (IDS_PIFPAGE_FIRST+0x0D)
#define IDS_BAD_ENVIRONMENT     (IDS_PIFPAGE_FIRST+0x0E)
#define IDS_BAD_MEMDPMI         (IDS_PIFPAGE_FIRST+0x0F)
#define IDS_MEMORY_RELAUNCH     (IDS_PIFPAGE_FIRST+0x10)
#define IDS_ADVANCED_RELAUNCH   (IDS_PIFPAGE_FIRST+0x11)

#define IDS_PROGRAMDEFEXT       (IDS_PIFPAGE_FIRST+0x12)
#define IDS_PROGRAMFILTER       (IDS_PIFPAGE_FIRST+0x13)
#define IDS_PROGRAMBROWSE       (IDS_PIFPAGE_FIRST+0x14)

/*
 *  Careful!  pifvid.c assumes that these are in order.
 */
#define IDS_PIFVID_FIRST        0x2665
#define IDS_DEFAULTLINES        (IDS_PIFVID_FIRST+0x00)
#define IDS_25LINES             (IDS_PIFVID_FIRST+0x01)
#define IDS_43LINES             (IDS_PIFVID_FIRST+0x02)
#define IDS_50LINES             (IDS_PIFVID_FIRST+0x03)

#define IDS_PIFCONVERT          (IDS_PIFVID_FIRST+0x04)
#define IDS_PIFCONVERTEXE       (IDS_PIFVID_FIRST+0x05)
#define IDS_AUTOEXECTOP         (IDS_PIFVID_FIRST+0x06)
#define IDS_AUTOEXECBOTTOM      (IDS_PIFVID_FIRST+0x07)
#define IDS_DISKINSERT          (IDS_PIFVID_FIRST+0x08)
#define IDS_DISKREMOVE          (IDS_PIFVID_FIRST+0x09)

#define IDS_NORMALWINDOW        (IDS_PIFVID_FIRST+0x0A)
#define IDS_MINIMIZED           (IDS_PIFVID_FIRST+0x0B)
#define IDS_MAXIMIZED           (IDS_PIFVID_FIRST+0x0C)

#define IDS_APPSINF             (IDS_PIFVID_FIRST+0x0D)
#define IDS_NOAPPSINF           (IDS_PIFVID_FIRST+0x0E)
#define IDS_CANTOPENAPPSINF     (IDS_PIFVID_FIRST+0x0F)
#define IDS_APPSINFERROR        (IDS_PIFVID_FIRST+0x10)
#define IDS_CREATEPIF           (IDS_PIFVID_FIRST+0x11)
#define IDS_UNKNOWNAPP          (IDS_PIFVID_FIRST+0x12)
#define IDS_UNKNOWNAPPDEF       (IDS_PIFVID_FIRST+0x13)

#define IDS_EMM386_NOEMS        (IDS_PIFVID_FIRST+0x15)
//      IDS_EMM386_NOEMS+1      (IDS_PIFVID_FIRST+0x16)
#define IDS_QEMM_NOEMS          (IDS_PIFVID_FIRST+0x17)
//      IDS_QEMM_NOEMS+1        (IDS_PIFVID_FIRST+0x18)
#define IDS_RING0_NOEMS         (IDS_PIFVID_FIRST+0x19)
//      IDS_RING0_NOEMS+1       (IDS_PIFVID_FIRST+0x1A)
#define IDS_SYSINI_NOEMS        (IDS_PIFVID_FIRST+0x1B)
//      IDS_SYSINI_NOEMS+1      (IDS_PIFVID_FIRST+0x1C)

#define IDS_NUKECONFIGTITLE     (IDS_PIFVID_FIRST+0x1D)
#define IDS_NUKECONFIGMSG       (IDS_PIFVID_FIRST+0x1E)


#define IDS_ERROR               (IDS_PIFVID_FIRST+0x1F)  /* Not a string ID */
        /* Error messages go at IDS_ERROR + ERROR_WHATEVER */
    /* Right now, there is only one error string */


#define IDS_PIFFONT_FIRST       0x26a0
#define IDS_TTFACENAME_SBCS     (IDS_PIFFONT_FIRST+0x01)
#define IDS_TTFACENAME_DBCS     (IDS_PIFFONT_FIRST+0x02)
#define IDS_TTCACHESEC_SBCS     (IDS_PIFFONT_FIRST+0x03)
#define IDS_TTCACHESEC_DBCS     (IDS_PIFFONT_FIRST+0x04)

#define IDS_FAV_FIRST                    0x26a5
//#define IDS_FAV_NEW                     (IDS_FAV_FIRST + 0x01)
//#define IDS_FAV_CANNOTCREATEFILE        (IDS_FAV_FIRST + 0x02)

//#define IDS_FAV_SYSERROR_FSGENERIC      (IDS_FAV_FIRST + 0x03)
//#define IDS_FAV_SYSERROR_ERROR          (IDS_FAV_FIRST + 0x04)
//#define IDS_FAV_SYSERROR_ERROR2         (IDS_FAV_FIRST + 0x05)

#define IDS_FAV_COL_LAST_VISIT          (IDS_FAV_FIRST + 0x06)
#define IDS_FAV_COL_COUNT_VISIT         (IDS_FAV_FIRST + 0x07)
#define IDS_FAV_COL_SORT_BY_LAST_VISIT  (IDS_FAV_FIRST + 0x08)
#define IDS_FAV_COL_LAST_MOD            (IDS_FAV_FIRST + 0x09)
#define IDS_FAV_COL_SPLAT               (IDS_FAV_FIRST + 0x0a)

#define IDS_FAV_SPLAT                   (IDS_FAV_FIRST + 0x0b)

#define IDS_CANTRECYCLEREGITEMS_NAME        0x2700
#define IDS_CANTRECYCLEREGITEMS_INCL_NAME   0x2701
#define IDS_CANTRECYCLEREGITEMS_ALL         0x2702
#define IDS_CANTRECYCLEREGITEMS_SOME        0x2703
#define IDS_CONFIRMDELETEDESKTOPREGITEM     0x2704
#define IDS_CONFIRMDELETEDESKTOPREGITEMS    0x2705
#define IDS_CONFIRMDELETE_CAPTION           0x2706
#define IDS_CONFIRMNOTRECYCLABLE            0x2707

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


// File Type strings
#define IDS_FT                              0x2760
#define IDS_ADDNEWFILETYPE                  0x2761
#define IDS_FT_EDITTITLE                    0x2762
#define IDS_FT_CLOSE                        0x2763
#define IDS_FT_EXEFILE                      0x2764
#define IDS_FT_MB_EXTTEXT                   0x2765
#define IDS_FT_MB_NOEXT                     0x2766
#define IDS_FT_MB_NOACTION                  0x2767
#define IDS_FT_MB_EXETEXT                   0x2768
#define IDS_FT_MB_REMOVETYPE                0x2769
#define IDS_FT_MB_REMOVEACTION              0x276A
#define IDS_CAP_OPENAS                      0x276B
#define IDS_FT_EXE                          0x276C
#define IDS_PROGRAMSFILTER_NT               0x276D
#define IDS_PROGRAMSFILTER_WIN95            0x276E
#define IDS_FT_NA                           0x276F
#define IDS_FT_MB_REPLEXTTEXT               0x2770
#define IDS_FT_PROP_ADVANCED                0x2771
#define IDS_FT_PROP_EXTENSIONS              0x2772
#define IDS_FT_PROP_DETAILSFOR              0x2773
#define IDS_FT_EDIT_ALREADYASSOC            0x2774
#define IDS_FT_EDIT_ALRASSOCTITLE           0x2775
#define IDS_FT_EDIT_STATIC2A                0x2776 
#define IDS_FT_EDIT_STATIC2B                0x2777
#define IDS_FT_EDIT_STATIC2C                0x2778

#define IDS_FT_NEW							0x2779
#define IDS_FT_NODESCR						0x277A
#define IDS_FT_FTTEMPLATE					0x277B
#define IDS_FT_MB_NOSPACEINEXT              0x277C

#define IDS_FT_ADVBTNTEXTEXPAND             0x277D
#define IDS_FT_ADVBTNTEXTCOLLAPS            0x277E
#define IDS_FT_EXTALREADYUSE                0x277F

#define IDS_EXTTYPETEMPLATE                 0x2780
#define IDS_COMPUTERSNEARMELNK              0x2781
#define IDS_COMPUTERSNEARMETIP              0x2782
#define IDS_FT_PROP_DETAILSFORPROGID        0x2783
#define IDS_FT_PROP_ADVANCED_PROGID         0x2784

// #define unused range from                0x2785 to 0x2799

#define IDS_FT_PROP_BTN_ADVANCED            0x279a
#define IDS_FT_PROP_BTN_RESTORE             0x279b
#define IDS_FT_PROP_RESTORE                 0x279c

#define IDS_FT_MB_EXISTINGACTION            0x279d

// Items in the "Search Name Space"
#define IDS_SNS_DOCUMENTFOLDERS             0x2800
#define IDS_SNS_LOCALHARDDRIVES             0x2801
#define IDS_SNS_MYNETWORKPLACES             0x2802
#define IDS_SNS_ALL_FILE_TYPES              0x2803
#define IDS_SNS_BROWSER_FOR_DIR             0x2804
#define IDS_SNS_BROWSERFORDIR_TITLE         0x2805

//String id used in mdprsht.c
#define IDS_CONTENTS                        0x2830

#define IDS_RECENT                          0x2831
#define IDS_ACCESSINGMONIKER                0x2832

// used in recclean.c:
#define IDS_RECCLEAN_NAMETEXT               0x2833
#define IDS_RECCLEAN_DESCTEXT               0x2834
#define IDS_RECCLEAN_BTNTEXT                0x2835

#ifdef FEATURE_FOLDER_INFOTIP    
// used in foldertip.c:
#define IDS_FIT_Delimeter                   0x2921
#define IDS_FIT_ExtraItems                  0x2922
#define IDS_FIT_TipFormat                   0x2923
#define IDS_FIT_Size                        0x2924
#define IDS_FIT_Size_LT                     0x2925
#define IDS_FIT_Size_Empty                  0x2926
#define IDS_FIT_Files                       0x2927
#define IDS_FIT_Folders                     0x2928
#endif // FEATURE_FOLDER_INFOTIP    

// Encryption context menu
#define IDS_ECM_ENCRYPT                     0x2951
#define IDS_ECM_DECRYPT                     0x2952
#define IDS_ECM_ENCRYPT_HELP                0x2953
#define IDS_ECM_DECRYPT_HELP                0x2954

// Folder shortcut
#define IDS_BROWSEFORFS                     0x2929
#define IDS_FOLDERSHORTCUT_ERR_TITLE        0x292A
#define IDS_FOLDERSHORTCUT_ERR              0x292B

// dialog IDs of caller's dialog if FILEOP_CREATEPROGRESSDLG is not set

#define IDD_STATUS          100
#define IDD_TOSTATUS        101
#define IDD_NAME            102
#define IDD_TONAME          103
#define IDD_PROBAR          104
#define IDD_TIMEEST         105
#define IDD_ANIMATE         106

// numbers 0x3000 - 0x3FFF are taken up in Control IDs (buttons, etc)
// maybe?

#define IDD_BROWSE              0x3000
#define IDD_PROMPT              0x3001
#define IDD_PATH                0x3002
#define IDD_TEXT                0x3003
#define IDD_TEXT1               0x3004
#define IDD_TEXT2               0x3005
#define IDD_TEXT3               0x3006
#define IDD_TEXT4               0x3007
#define IDD_ARPWARNINGTEXT      0x3008 // NOTE: This is only supposed to be used by DLG_DELETE_FILE_ARP
#define IDD_ICON                0x3009
#define IDD_COMMAND             0x300A
#define IDD_STATE               0x300B
#define IDD_ICON_OLD            0x300C
#define IDD_ICON_NEW            0x300D
#define IDD_FILEINFO_OLD        0x300E
#define IDD_FILEINFO_NEW        0x300F
#define IDD_ICON_WASTEBASKET    0x3010
#define IDD_RUNDLGOPENPROMPT    0x3011
#define IDD_RUNINSEPARATE       0x3012

#define IDD_PAGELIST            0x3020
#define IDD_APPLYNOW            0x3021
#define IDD_DLGFRAME            0x3022

#define IDD_RESTORE             0x3023
#define IDD_SPOOL_TXT           0x3024
#define IDD_ARPLINKWINDOW       0x3025
#define IDD_ARPINFORMATION      0x3026

// Leave some room here just in case.

#define IDD_REFERENCE           0x3100
#define IDD_WORKDIR             0x3101

// these are for the confirmation dialogs
#define IDD_DIR                 0x3201
#define IDD_FROM                0x3202
#define IDD_TO                  0x3203
#define IDD_DATE1               0x3205
#define IDD_DATE2               0x3206
#define IDD_YESTOALL            0x3207
#define IDD_NOTOALL             0x3208
#define IDD_FLAGS               0x3209
#define IDD_STATIC              0x320a


// userlogon dialog
// needs only to be unique within itself
#define IDD_CURRENTUSER         0x100
#define IDC_USERNAME            0x101
#define IDC_PASSWORD            0x102
#define IDC_USECURRENTACCOUNT   0x103
#define IDC_USEOTHERACCOUNT     0x104
#define IDC_DOMAIN              0x105
#define IDC_USERNAMELBL         0x106
#define IDC_PASSWORDLBL         0x107
#define IDC_DOMAINLBL           0x108

#define IDD_OPEN                0x3210
#define IDD_EMPTY               0x3211
#define IDC_DISKFULL_CLEANUP    0x3212


// for general file dialog page
#define IDD_ITEMICON            0x3301
#define IDD_FILENAME            0x3302
#define IDD_FILETYPE            0x3303
#define IDD_NUMFILES            0x3304
#define IDD_ACTNAMES            0x3305
#define IDD_ACTKEYS             0x3306
#define IDD_FILESIZE            0x3308
#define IDD_LOCATION            0x3309
#define IDD_CREATED             0x3310
#define IDD_LASTMODIFIED        0x3311
#define IDD_LASTACCESSED        0x3312
#define IDD_READONLY            0x3313
#define IDD_HIDDEN              0x3314
#define IDD_ARCHIVE             0x3315
#define IDD_DELETED             0x3317
#define IDD_FILETYPE_TXT        0x3318
#define IDD_FILESIZE_TXT        0x3319
#define IDD_CONTAINS_TXT        0x3320
#define IDD_LOCATION_TXT        0x3321
#define IDD_FILENAME_TXT        0x3322
#define IDD_ATTR_GROUPBOX       0x3323
#define IDD_CREATED_TXT         0x3324
#define IDD_LASTMODIFIED_TXT    0x3325
#define IDD_LASTACCESSED_TXT    0x3326
#define IDD_LINE_1              0x3327
#define IDD_LINE_2              0x3328
#define IDD_LINE_3              0x3329
#define IDD_DELETED_TXT         0x3330

#define IDD_COMPRESS                0x3331 // "Compress" check box.
#define IDD_FILESIZE_COMPRESSED     0x3332 // Compressed size value text.
#define IDD_FILESIZE_COMPRESSED_TXT 0x3333 // "Compressed Size" text.
#define IDD_TYPEICON            0x3334

#define IDD_LINE_4              0x3335
#define IDD_OPENSWITH_NOCHANGE  0x3336

#define IDD_FILETYPE_TARGET     0x3337  // "Target:" text on mounted volume page

// for version dialog page

#define IDD_VERSION_FRAME           0x3350
#define IDD_VERSION_KEY             0x3351
#define IDD_VERSION_VALUE           0x3352
#define IDD_VERSION_FILEVERSION     0x3353
#define IDD_VERSION_DESCRIPTION     0x3354
#define IDD_VERSION_COPYRIGHT       0x3355

// new stuff on the file/folder properties
#define IDD_OPENSWITH_TXT       0x3360
#define IDD_OPENSWITH           0x3361
#define IDC_ADVANCED            0x3362
#define IDC_CHANGEFILETYPE      0x3363
#define IDD_NAMEEDIT            0x3364
#define IDC_MANAGEFILES_TXT     0x3365
#define IDD_INDEX               0x3366
#define IDD_ENCRYPT             0x3367
#define IDC_ENCRYPTDETAILS      0x3368
#define IDC_MANAGEFOLDERS_TXT   0x3369
#define IDD_ATTRIBSTOAPPLY      0x336a
#define IDD_NOTRECURSIVE        0x336b
#define IDD_RECURSIVE_TXT       0x336c
#define IDD_RECURSIVE           0x336d
#define IDD_MANAGEFOLDERS_TXT   0x336e
#define IDD_EJECT               0x336f
#define IDD_ERROR_TXT           0x3370

#define IDD_OPENWITH            0x3371


// Folder shortcut general dialog
#define IDD_TARGET              0x3380
#define IDD_TARGET_TXT          0x3381
#define IDD_COMMENT_TXT         0x3382


// unicpp\resource.h defines more values starting at 0x8500

////////////////////////////////
// fileview stuff

//------------------------------
// Menu IDs

#define POPUP_NONDEFAULTDD              200
#define POPUP_MOVEONLYDD                201
#define POPUP_BRIEFCASE_NONDEFAULTDD    202
#define POPUP_DRIVES_NONDEFAULTDD       203
#define POPUP_BOOKMARK_NONDEFAULTDD     204
#define POPUP_SCRAP                     205
#define POPUP_FILECONTENTS              206
#define POPUP_BRIEFCASE_FOLDER_NONDEFAULTDD 207
#define POPUP_DROPONEXE                 208
#define POPUP_BRIEFCASE_FILECONTENTS    209
#define POPUP_DESKTOPCONTENTS           199
#define POPUP_DESKTOPCONTENTS_IMG       198
#define POPUP_EMBEDDEDOBJECT            197
#define POPUP_TEMPLATEDD                196
//--------------------------------------------------------------------------
// Menu items for views (210-299 are reserved)
//

#define POPUP_DCM_ITEM  210

#define POPUP_SFV_BACKGROUND    215
#define POPUP_SFV_MAINMERGE     216
#define POPUP_SFV_MAINMERGENF   217
#define POPUP_SFV_BACKGROUND_AD 218

#define POPUP_FSVIEW_BACKGROUND 220
#define POPUP_FSVIEW_POPUPMERGE 221
#define POPUP_FSVIEW_ITEM       222
#define POPUP_FSVIEW_ITEM_COREL7_HACK 223 //win95 send to menu for corel suite 7
#define POPUP_FSVIEW_BACKGROUND_MERGE 224 // File menu background 

#define POPUP_DESKTOP_ITEM      225
#define POPUP_BITBUCKET_ITEM    226
#define POPUP_BITBUCKET_POPUPMERGE  227
#define POPUP_BITBUCKET_BACKGROUND 228

#define POPUP_FINDEXT_POPUPMERGE   229

#define POPUP_NETWORK_BACKGROUND 230
#define POPUP_NETWORK_POPUPMERGE 231
#define POPUP_NETWORK_ITEM      232
#define POPUP_NETWORK_PRINTER   233

#define POPUP_PRINTERS_POPUPMERGE 235

#define POPUP_CONTROLS_POPUPMERGE 240

#define POPUP_DRIVES_BACKGROUND 245
#define POPUP_DRIVES_POPUPMERGE 246
#define POPUP_DRIVES_ITEM       247
#define POPUP_DRIVES_PRINTERS   248

#define POPUP_BRIEFCASE         250

#define POPUP_FIND              255
#define POPUP_DOCFIND_POPUPMERGE   256
#define POPUP_NETFIND_POPUPMERGE   257
#define POPUP_DOCFIND_MERGE        258
#define POPUP_DOCFIND_ITEM_MERGE   259

#define POPUP_COMMDLG_POPUPMERGE   260


#define MENU_FINDDLG                303
#define MENU_FINDCOMPDLG            304

#define MENU_GENERIC_OPEN_VERBS     351

#define MENU_PRINTOBJ_NEWPRN        352
#define MENU_PRINTOBJ_VERBS         353

#define MENU_PRINTOBJ_NEWPRN_DD     355
#define MENU_PRINTOBJ_DD            356

#define MENU_PRINTERQUEUE           357
#define MENU_FAV_ITEMCONTEXT        358
#define MENU_STARTMENUSTATICITEMS   359

#define MENU_GENERIC_CONTROLPANEL_VERBS 360    

// unicpp\resource.h defines more IDs starting at 400

//------------------------------
#define IDM_NOOP                    855

#define IDM_OPENCONTAINING          0xa000
#define IDM_CASESENSITIVE           0xa001
#define IDM_REGULAREXP              0xa002
#define IDM_SAVESEARCH              0xa003
#define IDM_CLOSE                   0xa004
#define IDM_SAVERESULTS             0xa005
#define IDM_HELP_FIND               0xa006
#define IDM_HELP_WHATSTHIS          0xa007
#define IDM_MENU_OPTIONS            0xa010
#define IDM_FIND_MENU_FIRST         IDM_OPENCONTAINING
#define IDM_FIND_MENU_LAST          IDM_HELP_WHATSTHIS

// more menu item IDs in unicpp\resource.h starting at 0xa100

// for link dialog pages
#define IDD_LINK_DESCRIPTION        0X3401
#define IDD_LINK_COMMAND            0X3402
#define IDD_LINK_WORKINGDIR         0X3403
#define IDD_LINK_HOTKEY             0X3404
#define IDD_LINK_HELP               0X3405
#define IDD_FINDORIGINAL            0X3406
#define IDD_LINKDETAILS             0X3407
#define IDD_LINK_SHOWCMD            0x3408
#define IDD_LINK_RUNASUSER          0x3409

// Old SHELL.DLL control IDs (oldshell.dlg)
#define IDD_APPNAME                 0x3500
#define IDD_CONFIG                  0x3501
#define IDD_CONVTITLE               0x3502
#define IDD_CONVENTIONAL            0x3503
#define IDD_EMSFREE                 0x3504
#define IDD_SDTEXT                  0x3505
#define IDD_SDUSING                 0x3506
#define IDD_USERNAME                0x3507
#define IDD_COMPANYNAME             0x3508
#define IDD_SERIALNUM               0x3509
#define IDD_VERSION                 0x350b
#define IDD_EMSTEXT                 0x350c
#define IDD_OTHERSTUFF              0x350d
#define IDD_DOSVER                  0x350e
#ifdef WINNT
#define IDD_PROCESSOR               0x350f
#define IDD_PRODUCTID               0x3510
#define IDD_OEMID                   0x3511
#endif

#define IDD_APPLIST             0x3605
#define IDD_DESCRIPTION         0x3506
#define IDD_OTHER               0x3507
#define IDD_DESCRIPTIONTEXT     0x3508
#define IDD_MAKEASSOC           0x3509

// For find dialog
#define IDD_START                   0x3700
#define IDD_STOP                    0x3701
#define IDD_FILELIST                0x3702
#define IDD_NEWSEARCH               0x3703

//#define IDD_PATH                  (already defined)
//#define IDD_BROWSE                (already defined)
#define IDD_FILESPEC                0x3710
#define IDD_TOPLEVELONLY            0x3711
#define IDD_TEXTCASESEN             0x3712
#define IDD_TEXTREG                 0x3713
#define IDD_SEARCHSLOWFILES         0x3714

#define IDD_TYPECOMBO               0x3720
#define IDD_CONTAINS                0x3721
#define IDD_SIZECOMP                0x3722
#define IDD_SIZEVALUE               0x3723
#define IDD_SIZEUPDOWN              0x3724
#define IDD_SIZELBL                 0x3725

#define IDD_MDATE_ALL               0x3730
#define IDD_MDATE_PARTIAL           0x3731
#define IDD_MDATE_DAYS              0x3732
#define IDD_MDATE_MONTHS            0x3733
#define IDD_MDATE_BETWEEN           0x3734
#define IDD_MDATE_NUMDAYS           0x3735
#define IDD_MDATE_DAYSUPDOWN        0x3736
#define IDD_MDATE_NUMMONTHS         0x3737
#define IDD_MDATE_MONTHSUPDOWN      0x3738
#define IDD_MDATE_FROM              0x3739
#define IDD_MDATE_TO                0x373a
#define IDD_MDATE_TYPE              0x373b
#define IDD_MDATE_AND               0x373c
#define IDD_MDATE_MONTHLBL          0x373d
#define IDD_MDATE_DAYLBL            0x373e

#define IDD_COMMENT                 0x3740
#define IDD_FOLDERLIST              0x3741
#define IDD_BROWSETITLE             0x3742
#define IDD_BROWSESTATUS            0x3743
#define IDD_BROWSEEDIT              0x3744
#define IDD_SAVERESULTS             0x3745
#define IDD_NEWFOLDER_BUTTON        0x3746
#define IDD_BFF_RESIZE_TAB          0x3747
#define IDD_FOLDERLABLE             0x3748

// for encryption warning dialog
#define IDC_ENCRYPT_FILE            0x3750
#define IDC_ENCRYPT_PARENTFOLDER    0x3751

// 0x3800 - 0x3806 available - dsheldon

// for logoff dialog
#define IDD_LOGOFFICON              0x3808

#define DLG_ABOUT                   0x3810

// global ids
#define IDC_STATIC                  -1
#define IDC_GROUPBOX                300
#define IDC_GROUPBOX_2              301
#define IDC_GROUPBOX_3              302

//
// ids to disable context Help
//
#define IDC_NO_HELP_1               650 
#define IDC_NO_HELP_2               651
#define IDC_NO_HELP_3               652
#define IDC_NO_HELP_4               653

// for pifmgr pages
#define IDD_PROGRAM                 0x3820
#ifdef WINNT
#define IDD_PIFNTTEMPLT             0x3821
#else
#define IDD_ADVPROG                 0x3821
#endif
#define IDD_MEMORY                  0x3822
#define IDD_SCREEN                  0x3823
#define IDD_FONT                    0x3824
#define IDD_ADVFONT                 0x3825
#define IDD_MISC                    0x3826

// disk general page
#define IDC_DRV_FIRST                   0x3840
#define IDC_DRV_ICON                    (IDC_DRV_FIRST+0x00)
#define IDC_DRV_LABEL                   (IDC_DRV_FIRST+0x01)
#define IDC_DRV_TYPE                    (IDC_DRV_FIRST+0x02)
#define IDC_DRV_USEDCOLOR               (IDC_DRV_FIRST+0x03)
#define IDC_DRV_FREECOLOR               (IDC_DRV_FIRST+0x04)
#define IDC_DRV_USEDMB                  (IDC_DRV_FIRST+0x05)
#define IDC_DRV_USEDBYTES               (IDC_DRV_FIRST+0x06)
#define IDC_DRV_FREEBYTES               (IDC_DRV_FIRST+0x07)
#define IDC_DRV_FREEMB                  (IDC_DRV_FIRST+0x08)
#define IDC_DRV_TOTBYTES                (IDC_DRV_FIRST+0x09)
#define IDC_DRV_TOTMB                   (IDC_DRV_FIRST+0x0a)
#define IDC_DRV_PIE                     (IDC_DRV_FIRST+0x0b)
#define IDC_DRV_LETTER                  (IDC_DRV_FIRST+0x0c)
#define IDC_DRV_TOTSEP                  (IDC_DRV_FIRST+0x0d)
#define IDC_DRV_TYPE_TXT                (IDC_DRV_FIRST+0x0e)
#define IDC_DRV_TOTBYTES_TXT            (IDC_DRV_FIRST+0x0f)
#define IDC_DRV_USEDBYTES_TXT           (IDC_DRV_FIRST+0x10)
#define IDC_DRV_FREEBYTES_TXT           (IDC_DRV_FIRST+0x11)
//These ids are used in the mounted drive general page for the
//new controls not defined in the general drive page
#define IDC_DRV_LOCATION                (IDC_DRV_FIRST+0x16)
#define IDC_DRV_TARGET                  (IDC_DRV_FIRST+0x17)
#define IDC_DRV_CREATED                 (IDC_DRV_FIRST+0x18)
#define IDC_DRV_PROPERTIES              (IDC_DRV_FIRST+0x19)
#define IDC_DRV_FS_TXT                  (IDC_DRV_FIRST+0x1a)
#define IDC_DRV_FS                      (IDC_DRV_FIRST+0x1b)
#define IDC_DRV_CLEANUP                 (IDC_DRV_FIRST+0x1c)
#define IDC_DRV_TOTSEP2                 (IDC_DRV_FIRST+0x1d)

#define IDC_DISKTOOLS_FIRST             0x3850
#define IDC_DISKTOOLS_CHKNOW            (IDC_DISKTOOLS_FIRST+0x00)
#define IDC_DISKTOOLS_TRLIGHT           (IDC_DISKTOOLS_FIRST+0x01)
#define IDC_DISKTOOLS_BKPNOW            (IDC_DISKTOOLS_FIRST+0x02)
#define IDC_DISKTOOLS_CHKDAYS           (IDC_DISKTOOLS_FIRST+0x03)
#define IDC_DISKTOOLS_OPTNOW            (IDC_DISKTOOLS_FIRST+0x04)
#define IDC_DISKTOOLS_BKPDAYS           (IDC_DISKTOOLS_FIRST+0x05)
#define IDC_DISKTOOLS_OPTDAYS           (IDC_DISKTOOLS_FIRST+0x06)
#define IDC_DISKTOOLS_BKPTXT            (IDC_DISKTOOLS_FIRST+0x07)

// The order of these is significant (see pifsub.c:EnableEnumProc),
// The range points are IDC_ICONBMP, IDC_PIF_STATIC and
// IDC_REALMODEDISABLE.  The "safe" range (no enable/disable funny
// stuff is IDC_PIF_STATIC to IDC_REALMODE_DISABLE
#define IDC_PIFPAGES_FIRST              0x3860
#define IDC_CONVMEMGRP                  (IDC_PIFPAGES_FIRST+0x00)
#define IDC_LOCALENVLBL                 (IDC_PIFPAGES_FIRST+0x01)
#define IDC_ENVMEM                      (IDC_PIFPAGES_FIRST+0x02)
#define IDC_CONVMEMLBL                  (IDC_PIFPAGES_FIRST+0x03)
#define IDC_LOWMEM                      (IDC_PIFPAGES_FIRST+0x04)
#define IDC_LOWLOCKED                   (IDC_PIFPAGES_FIRST+0x05)
#define IDC_LOCALUMBS                   (IDC_PIFPAGES_FIRST+0x06)
#define IDC_GLOBALPROTECT               (IDC_PIFPAGES_FIRST+0x07)
#define IDC_EXPMEMGRP                   (IDC_PIFPAGES_FIRST+0x08)
#define IDC_EXPMEMLBL                   (IDC_PIFPAGES_FIRST+0x09)
#define IDC_EMSMEM                      (IDC_PIFPAGES_FIRST+0x0A)
#define IDC_EXTMEMGRP                   (IDC_PIFPAGES_FIRST+0x0C)
#define IDC_EXTMEMLBL                   (IDC_PIFPAGES_FIRST+0x0D)
#define IDC_XMSMEM                      (IDC_PIFPAGES_FIRST+0x0E)
#define IDC_HMA                         (IDC_PIFPAGES_FIRST+0x10)
#define IDC_FGNDGRP                     (IDC_PIFPAGES_FIRST+0x11)
#define IDC_FGNDEXCLUSIVE               (IDC_PIFPAGES_FIRST+0x12)
#define IDC_FGNDSCRNSAVER               (IDC_PIFPAGES_FIRST+0x13)
#define IDC_BGNDGRP                     (IDC_PIFPAGES_FIRST+0x14)
#define IDC_BGNDSUSPEND                 (IDC_PIFPAGES_FIRST+0x15)
#define IDC_IDLEGRP                     (IDC_PIFPAGES_FIRST+0x16)
#define IDC_IDLELOWLBL                  (IDC_PIFPAGES_FIRST+0x17)
#define IDC_IDLEHIGHLBL                 (IDC_PIFPAGES_FIRST+0x18)
#define IDC_IDLESENSE                   (IDC_PIFPAGES_FIRST+0x19)
#define IDC_TERMGRP                     (IDC_PIFPAGES_FIRST+0x1A)
#define IDC_WARNTERMINATE               (IDC_PIFPAGES_FIRST+0x1B)
#define IDC_TERMINATE                   (IDC_PIFPAGES_FIRST+0x1C)
#define IDC_SCREENUSAGEGRP              (IDC_PIFPAGES_FIRST+0x1D)
#define IDC_WINDOWED                    (IDC_PIFPAGES_FIRST+0x1E)
#define IDC_FULLSCREEN                  (IDC_PIFPAGES_FIRST+0x1F)
#define IDC_AUTOCONVERTFS               (IDC_PIFPAGES_FIRST+0x20)
#define IDC_SCREENLINESLBL              (IDC_PIFPAGES_FIRST+0x21)
#define IDC_SCREENLINES                 (IDC_PIFPAGES_FIRST+0x22)
#define IDC_WINDOWUSAGEGRP              (IDC_PIFPAGES_FIRST+0x23)
#define IDC_TOOLBAR                     (IDC_PIFPAGES_FIRST+0x24)
#define IDC_WINRESTORE                  (IDC_PIFPAGES_FIRST+0x25)
#define IDC_SCREENPERFGRP               (IDC_PIFPAGES_FIRST+0x26)
#define IDC_TEXTEMULATE                 (IDC_PIFPAGES_FIRST+0x27)
#define IDC_DYNAMICVIDMEM               (IDC_PIFPAGES_FIRST+0x28)
#define IDC_FONTSIZELBL                 (IDC_PIFPAGES_FIRST+0x29)
#define IDC_FONTSIZE                    (IDC_PIFPAGES_FIRST+0x2A)
#define IDC_FONTGRP                     (IDC_PIFPAGES_FIRST+0x2B)
#define IDC_RASTERFONTS                 (IDC_PIFPAGES_FIRST+0x2C)
#define IDC_TTFONTS                     (IDC_PIFPAGES_FIRST+0x2D)
#define IDC_BOTHFONTS                   (IDC_PIFPAGES_FIRST+0x2E)
#define IDC_WNDPREVIEWLBL               (IDC_PIFPAGES_FIRST+0x2F)
#define IDC_FONTPREVIEWLBL              (IDC_PIFPAGES_FIRST+0x30)
#define IDC_WNDPREVIEW                  (IDC_PIFPAGES_FIRST+0x31)
#define IDC_FONTPREVIEW                 (IDC_PIFPAGES_FIRST+0x32)
#define IDC_Unused1064                  (IDC_PIFPAGES_FIRST+0x33)
#define IDC_Unused1065                  (IDC_PIFPAGES_FIRST+0x34)
#define IDC_Unused1066                  (IDC_PIFPAGES_FIRST+0x35)
#define IDC_Unused1067                  (IDC_PIFPAGES_FIRST+0x36)
#define IDC_Unused1068                  (IDC_PIFPAGES_FIRST+0x37)
#define IDC_MISCKBDGRP                  (IDC_PIFPAGES_FIRST+0x38)
#define IDC_ALTESC                      (IDC_PIFPAGES_FIRST+0x39)
#define IDC_ALTTAB                      (IDC_PIFPAGES_FIRST+0x3A)
#define IDC_CTRLESC                     (IDC_PIFPAGES_FIRST+0x3B)
#define IDC_PRTSC                       (IDC_PIFPAGES_FIRST+0x3C)
#define IDC_ALTPRTSC                    (IDC_PIFPAGES_FIRST+0x3D)
#define IDC_ALTSPACE                    (IDC_PIFPAGES_FIRST+0x3E)
#define IDC_ALTENTER                    (IDC_PIFPAGES_FIRST+0x3F)
#define IDC_MISCMOUSEGRP                (IDC_PIFPAGES_FIRST+0x40)
#define IDC_QUICKEDIT                   (IDC_PIFPAGES_FIRST+0x41)
#define IDC_EXCLMOUSE                   (IDC_PIFPAGES_FIRST+0x42)
#define IDC_MISCOTHERGRP                (IDC_PIFPAGES_FIRST+0x43)
#define IDC_FASTPASTE                   (IDC_PIFPAGES_FIRST+0x44)
#define IDC_INSTRUCTIONS                (IDC_PIFPAGES_FIRST+0x45)
#define IDC_NOEMS                       (IDC_PIFPAGES_FIRST+0x47)
#define IDC_NOEMSDETAILS                (IDC_PIFPAGES_FIRST+0x48)
#define IDC_DPMIMEMGRP                  (IDC_PIFPAGES_FIRST+0x49)
#define IDC_DPMIMEMLBL                  (IDC_PIFPAGES_FIRST+0x4A)
#define IDC_DPMIMEM                     (IDC_PIFPAGES_FIRST+0x4B)
#define IDC_ICONBMP                     (IDC_PIFPAGES_FIRST+0x4C)
#define IDC_HOTKEYLBL                   (IDC_PIFPAGES_FIRST+0x4D)
#define IDC_HOTKEY                      (IDC_PIFPAGES_FIRST+0x4E)
#define IDC_WINDOWSTATELBL              (IDC_PIFPAGES_FIRST+0x4F)
#define IDC_WINDOWSTATE                 (IDC_PIFPAGES_FIRST+0x50)
#define IDC_WINLIE                      (IDC_PIFPAGES_FIRST+0x51)
#define IDC_SUGGESTMSDOS                (IDC_PIFPAGES_FIRST+0x52)
#define IDC_PIF_STATIC                  (IDC_PIFPAGES_FIRST+0x53)
#define IDC_TITLE                       (IDC_PIFPAGES_FIRST+0x54)
#define IDC_CMDLINE                     (IDC_PIFPAGES_FIRST+0x55)
#define IDC_CMDLINELBL                  (IDC_PIFPAGES_FIRST+0x56)
#define IDC_WORKDIRLBL                  (IDC_PIFPAGES_FIRST+0x57)
#define IDC_WORKDIR                     (IDC_PIFPAGES_FIRST+0x58)
#define IDC_BATCHFILELBL                (IDC_PIFPAGES_FIRST+0x59)
#define IDC_BATCHFILE                   (IDC_PIFPAGES_FIRST+0x5A)
#define IDC_ADVPROG                     (IDC_PIFPAGES_FIRST+0x5B)
#define IDC_REALMODE                    (IDC_PIFPAGES_FIRST+0x5C)
#define IDC_OK                          (IDC_PIFPAGES_FIRST+0x5D)
#define IDC_PIFNAMELBL                  (IDC_PIFPAGES_FIRST+0x5E)
#define IDC_PIFNAME                     (IDC_PIFPAGES_FIRST+0x5F)
#define IDC_CHANGEICON                  (IDC_PIFPAGES_FIRST+0x60)
#define IDC_CANCEL                      (IDC_PIFPAGES_FIRST+0x61)
#define IDC_CLOSEONEXIT                 (IDC_PIFPAGES_FIRST+0x62)
#ifdef NEW_UNICODE
#define IDC_SCREENXBUFLBL               (IDC_PIFPAGES_FIRST+0x63)
#define IDC_SCREENXBUF                  (IDC_PIFPAGES_FIRST+0x64)
#define IDC_SCREENYBUFLBL               (IDC_PIFPAGES_FIRST+0x65)
#define IDC_SCREENYBUF                  (IDC_PIFPAGES_FIRST+0x66)
#define IDC_WINXSIZELBL                 (IDC_PIFPAGES_FIRST+0x67)
#define IDC_WINXSIZE                    (IDC_PIFPAGES_FIRST+0x68)
#define IDC_WINYSIZELBL                 (IDC_PIFPAGES_FIRST+0x69)
#define IDC_WINYSIZE                    (IDC_PIFPAGES_FIRST+0x6A)
#else
#define IDC_Unused38CD                  (IDC_PIFPAGES_FIRST+0x63)
#define IDC_Unused38CE                  (IDC_PIFPAGES_FIRST+0x64)
#define IDC_Unused38CF                  (IDC_PIFPAGES_FIRST+0x65)
#define IDC_Unused38D0                  (IDC_PIFPAGES_FIRST+0x66)
#define IDC_Unused38D1                  (IDC_PIFPAGES_FIRST+0x67)
#define IDC_Unused38D2                  (IDC_PIFPAGES_FIRST+0x68)
#define IDC_Unused38D3                  (IDC_PIFPAGES_FIRST+0x69)
#define IDC_Unused38D4                  (IDC_PIFPAGES_FIRST+0x6A)
#endif
#define IDC_REALMODEDISABLE             (IDC_PIFPAGES_FIRST+0x80)
#define IDC_CONFIGLBL                   (IDC_PIFPAGES_FIRST+0x81)
#define IDC_CONFIG                      (IDC_PIFPAGES_FIRST+0x82)
#define IDC_AUTOEXECLBL                 (IDC_PIFPAGES_FIRST+0x83)
#define IDC_AUTOEXEC                    (IDC_PIFPAGES_FIRST+0x84)
//#define IDC_QUICKSTART                  (IDC_PIFPAGES_FIRST+0x85)
#define IDC_REALMODEWIZARD              (IDC_PIFPAGES_FIRST+0x86)
#define IDC_WARNMSDOS                   (IDC_PIFPAGES_FIRST+0x87)
#define IDC_CURCONFIG                   (IDC_PIFPAGES_FIRST+0x88)
#define IDC_CLEANCONFIG                 (IDC_PIFPAGES_FIRST+0x89)
#ifdef WINNT
#define IDC_DOS                         (IDC_PIFPAGES_FIRST+0x8A)
#define IDC_AUTOEXECNT                  (IDC_PIFPAGES_FIRST+0x8B)
#define IDC_CONFIGNT                    (IDC_PIFPAGES_FIRST+0x8C)
#define IDC_NTTIMER                     (IDC_PIFPAGES_FIRST+0x8D)
#endif

//--------------------------------------------------------------------------
// For Defview options page
#define IDC_SHOWALL          0x3720
#define IDC_SHOWEXTENSIONS   0x3721

#define IDC_STATIC              -1

//--------------------------------------------------------------------------
//  for DLG_FSEARCH_xxx dialogs
#define IDC_NOSCOPEWARNING          1001
#define IDC_FILESPEC                1001
#define IDC_GREPTEXT                1002
#define IDC_NAMESPACE               1003
#define IDC_SEARCH_START            1004
#define IDC_SEARCH_STOP             1005
#define IDC_USE_DATE                1007
#define IDC_USE_TYPE                1008
#define IDC_USE_SIZE                1009
#define IDC_USE_ADVANCED            1010
#define IDC_WHICH_DATE              1011
#define IDC_USE_RECENT_MONTHS       1012
#define IDC_RECENT_MONTHS           1013
#define IDC_RECENT_MONTHS_SPIN      1014
#define IDC_USE_RECENT_DAYS         1015
#define IDC_RECENT_DAYS             1016
#define IDC_RECENT_DAYS_SPIN        1017
#define IDC_USE_DATE_RANGE          1018
#define IDC_DATERANGE_BEGIN         1019
#define IDC_DATERANGE_END           1020
#define IDC_FILE_TYPE               1021
#define IDC_WHICH_SIZE              1022
#define IDC_FILESIZE                1023
#define IDC_FILESIZE_SPIN           1024
#define IDC_USE_SUBFOLDERS          1025
#define IDC_USE_CASE                1026
#ifdef  WINNT
#define IDC_USE_SLOW_FILES          1028
#endif // WINNT
#define IDC_FSEARCH_CAPTION         1029
#define IDC_FSEARCH_ICON            1030
#define IDC_INDEX_SERVER            1031
#define IDC_SEARCHLINK_FILES        1032
#define IDC_SEARCHLINK_COMPUTERS    1033
#define IDC_SEARCHLINK_PRINTERS     1034
#define IDC_SEARCHLINK_PEOPLE       1035
#define IDC_SEARCHLINK_INTERNET     1036
#define IDC_PSEARCH_ICON            1037
#define IDC_PSEARCH_CAPTION         1039
#define IDC_PSEARCH_NAME            1040
#define IDC_PSEARCH_LOCATION        1041
#define IDC_PSEARCH_MODEL           1042
#define IDC_CSEARCH_ICON            1043
#define IDC_CSEARCH_CAPTION         1044
#define IDC_CSEARCH_NAME            1045
#define IDC_SEARCHLINK_OPTIONS      1046
#define IDC_SEARCHLINK_PREVIOUS     1047
#define IDC_SEARCHLINK_CAPTION      1048
#define IDC_GROUPBTN_OPTIONS        1049
#define IDC_FSEARCH_DIV1            1050
#define IDC_FSEARCH_DIV2            1051
#define IDC_FSEARCH_DIV3            1052


//  for DLG_INDEXSERVER
#define IDC_CI_STATUS               1000
#define IDC_CI_PROMPT               1001
#define IDC_CI_HELP                 1002
#define IDC_ENABLE_CI               1003
#define IDC_BLOWOFF_CI              1004
#define IDC_CI_ADVANCED             1005

// for bitbucket prop pages
#define IDC_DISKSIZE            1000
#define IDC_BYTESIZE            1001
#define IDC_USEDSIZE            1002
#define IDC_DISKSIZEDATA        1003
#define IDC_BYTESIZEDATA        1004
#define IDC_USEDSIZEDATA        1005
#define IDC_NUKEONDELETE        1006
#define IDC_BBSIZE              1007
#define IDC_BBSIZETEXT          1008
#define IDC_INDEPENDENT         1009
#define IDC_GLOBAL              1010
#define IDC_TEXT                1011
#define IDC_CONFIRMDELETE       1012

// unicpp\resource.h defines more IDs starting at 0x8500

// File Type dialogs
// Note: Don't duplicate IDS between these dialogs
// as code is in place to know which help file is
// used...
#define IDC_FT_PROP_LV_FILETYPES        1000
#define IDC_FT_PROP_NEW                 1001
#define IDC_FT_PROP_REMOVE              1002
#define IDC_FT_PROP_EDIT                1003
#define IDC_FT_PROP_DOCICON             1004
#define IDC_FT_PROP_DOCEXTRO_TXT        1005
#define IDC_FT_PROP_DOCEXTRO            1006
#define IDC_FT_PROP_OPENICON            1007
#define IDC_FT_PROP_CONTTYPERO_TXT      1011
#define IDC_FT_PROP_CONTTYPERO          1012
#define IDC_FT_PROP_COMBO               1013
#define IDC_FT_PROP_FINDEXT_TXT         1014
#define IDC_FT_PROP_TYPEOFFILE          1015
#define IDC_FT_PROP_MODIFY              1016

#define IDC_FT_PROP_OPENEXE_TXT         1017
#define IDC_FT_PROP_OPENEXE             IDC_FT_PROP_OPENEXE_TXT + 1
#define IDC_FT_PROP_CHANGEOPENSWITH     IDC_FT_PROP_OPENEXE_TXT + 2
#define IDC_FT_PROP_TYPEOFFILE_TXT      IDC_FT_PROP_OPENEXE_TXT + 3
#define IDC_FT_PROP_EDITTYPEOFFILE      IDC_FT_PROP_OPENEXE_TXT + 4
#define IDC_FT_PROP_GROUPBOX            IDC_FT_PROP_OPENEXE_TXT + 5

#define IDC_FT_EDIT_EXT_EDIT            1023
#define IDC_FT_EDIT_ADVANCED            1024
#define IDC_FT_EDIT_PID_COMBO           1025
#define IDC_FT_EDIT_PID_COMBO_TEXT      1026
#define IDC_FT_EDIT_EXT_EDIT_TEXT       1027

#define IDC_FT_PROP_ANIM                1028

#define IDS_FOLDEROPTIONS               1030

#define IDC_FT_EDIT_DOCICON             1100
#define IDC_FT_EDIT_CHANGEICON          1101
#define IDC_FT_EDIT_DESC                1103
#define IDC_FT_EDIT_EXTTEXT             1104
#define IDC_FT_EDIT_EXT                 1105
#define IDC_FT_EDIT_LV_CMDSTEXT         1106
#define IDC_FT_EDIT_LV_CMDS             1107
#define IDC_FT_EDIT_NEW                 1108
#define IDC_FT_EDIT_EDIT                1109
#define IDC_FT_EDIT_REMOVE              1110
#define IDC_FT_EDIT_DEFAULT             1111
#define IDC_FT_EDIT_SHOWEXT             1113
#define IDC_FT_EDIT_DESCTEXT            1114
#define IDC_FT_COMBO_CONTTYPETEXT       1115
#define IDC_FT_COMBO_CONTTYPE           1116
#define IDC_FT_COMBO_DEFEXTTEXT         1117
#define IDC_FT_COMBO_DEFEXT             1118
#define IDC_FT_EDIT_CONFIRM_OPEN        1119
#define IDC_FT_EDIT_BROWSEINPLACE       1120

#define IDC_FT_CMD_ACTION               1200
#define IDC_FT_CMD_EXETEXT              1201
#define IDC_FT_CMD_EXE                  1202
#define IDC_FT_CMD_BROWSE               1203
#define IDC_FT_CMD_DDEGROUP             1204
#define IDC_FT_CMD_USEDDE               1205
#define IDC_FT_CMD_DDEMSG               1206
#define IDC_FT_CMD_DDEAPPNOT            1207
#define IDC_FT_CMD_DDETOPIC             1208
#define IDC_FT_CMD_DDEAPP               1209


//--------------------------------------------------------------------------
// For control panels & printer folder:

// RC IDs

#define IDC_PRINTER_ICON        1000
#define IDC_PRINTER_NAME        1001
#define IDDC_PRINTTO            1002
#define IDDB_ADD_PORT           1003
#define IDDB_DEL_PORT           1004
#define IDDC_DRIVER             1005
#define IDDB_NEWDRIVER          1006
#define IDC_TIMEOUTSETTING      1007
#define IDC_TIMEOUT_NOTSELECTED 1008
#define IDC_TIMEOUT_TRANSRETRY  1009
#define IDDB_SPOOL              1010
#define IDDB_PORT               1011
#define IDDB_SETUP              1012
#define IDGS_TYPE               1013
#define IDGS_LOCATION           1014
#define IDGE_COMMENT            1015
#define IDDB_TESTPAGE           1017
#define IDDC_SEPARATOR          1018
#define IDDB_CHANGESEPARATOR    1019
#define IDGS_TYPE_TXT           1020
#define IDGS_LOCATION_TXT       1021
#define IDGE_COMMENT_TXT        1022
#define IDGE_WHERE_TXT          1023
#define IDDC_SEPARATOR_TXT      1024
#define IDD_ADDPORT_NETWORK     1025
#define IDD_ADDPORT_PORTMON     1026
#define IDD_ADDPORT_NETPATH     1027
#define IDD_ADDPORT_BROWSE      1028
#define IDD_ADDPORT_LB          1029
#define IDD_DELPORT_LB          1030
#define IDD_DELPORT_TEXT_1      1031
#define IDC_TIMEOUTTEXT_1       1032
#define IDC_TIMEOUTTEXT_2       1033
#define IDC_TIMEOUTTEXT_3       1034
#define IDC_TIMEOUTTEXT_4       1035
#define IDDB_CAPTURE_PORT       1036
#define IDDB_RELEASE_PORT       1037
#define IDD_ENABLE_BIDI         1038
#define IDD_DISABLE_BIDI        1039

// Control IDs
//#define ID_LISTVIEW           200
#define ID_SETUP                210

// Commands for top level menu
#define ID_PRINTER_NEW                 111

// Menu items in the view queue dialog
#define ID_PRINTER_START               120
// DFM_CMD_PROPERTIES is -5
#define ID_PRINTER_PROPERTIES          (ID_PRINTER_START-5)

#define ID_DOCUMENT_PAUSE              130
#define ID_DOCUMENT_RESUME             131
#define ID_DOCUMENT_DELETE             132

#define ID_VIEW_STATUSBAR              140
#define ID_VIEW_TOOLBAR                141
#define ID_VIEW_REFRESH                142

#define ID_HELP_CONTENTS               150
#define ID_HELP_ABOUT                  151

// Help string ID's for printer/control folder
#define IDS_MH_PRINTOBJ_OPEN            (IDS_MH_PRINTFIRST+ID_PRINTOBJ_OPEN)
#define IDS_MH_PRINTOBJ_RESUME          (IDS_MH_PRINTFIRST+ID_PRINTOBJ_RESUME)
#define IDS_MH_PRINTOBJ_PAUSE           (IDS_MH_PRINTFIRST+ID_PRINTOBJ_PAUSE)
#define IDS_MH_PRINTOBJ_PURGE           (IDS_MH_PRINTFIRST+ID_PRINTOBJ_PURGE)
#define IDS_MH_PRINTOBJ_SETDEFAULT      (IDS_MH_PRINTFIRST+ID_PRINTOBJ_SETDEFAULT)

// Resources for the WinNT Format & Chkdsk Dialogs

#ifdef WINNT

#define IDC_GROUPBOX_1                  0x1202

#define DLG_FORMATDISK                  0x7000

#define IDC_CAPCOMBO                    (DLG_FORMATDISK + 1)
#define IDC_QFCHECK                     (DLG_FORMATDISK + 2)
#define IDC_ECCHECK                     (DLG_FORMATDISK + 3)
#define IDC_BLOCKSIZE                   (DLG_FORMATDISK + 4)
#define IDC_FSCOMBO                     (DLG_FORMATDISK + 5)
#define IDC_FMTPROGRESS                 (DLG_FORMATDISK + 6)
#define IDC_VLABEL                      (DLG_FORMATDISK + 7)
#define IDC_ASCOMBO                     (DLG_FORMATDISK + 8)

#define DLG_FORMATDISK_FIRSTCONTROL     (DLG_FORMATDISK + 1)
#define DLG_FORMATDISK_NUMCONTROLS      (8)

#define IDS_FORMATFAILED                (DLG_FORMATDISK + 10)
#define IDS_INCOMPATIBLEFS              (DLG_FORMATDISK + 11)
#define IDS_ACCESSDENIED                (DLG_FORMATDISK + 12)
#define IDS_WRITEPROTECTED              (DLG_FORMATDISK + 13)
#define IDS_CANTLOCK                    (DLG_FORMATDISK + 14)
#define IDS_CANTQUICKFORMAT             (DLG_FORMATDISK + 15)
#define IDS_IOERROR                     (DLG_FORMATDISK + 16)
#define IDS_BADLABEL                    (DLG_FORMATDISK + 17)
#define IDS_INCOMPATIBLEMEDIA           (DLG_FORMATDISK + 18)
#define IDS_FORMATCOMPLETE              (DLG_FORMATDISK + 19)
#define IDS_CANTCANCELFMT               (DLG_FORMATDISK + 20)
#define IDS_FORMATCANCELLED             (DLG_FORMATDISK + 21)
#define IDS_OKTOFORMAT                  (DLG_FORMATDISK + 22)
#define IDS_CANTENABLECOMP              (DLG_FORMATDISK + 23)

// these are required for uncode\format.c

// These must be in sequence
#define IDS_FMT_MEDIA0                  (DLG_FORMATDISK + 32)
#define IDS_FMT_MEDIA1                  (DLG_FORMATDISK + 33)
#define IDS_FMT_MEDIA2                  (DLG_FORMATDISK + 34)
#define IDS_FMT_MEDIA3                  (DLG_FORMATDISK + 35)
#define IDS_FMT_MEDIA4                  (DLG_FORMATDISK + 36)
#define IDS_FMT_MEDIA5                  (DLG_FORMATDISK + 37)
#define IDS_FMT_MEDIA6                  (DLG_FORMATDISK + 38)
#define IDS_FMT_MEDIA7                  (DLG_FORMATDISK + 39)
#define IDS_FMT_MEDIA8                  (DLG_FORMATDISK + 40)
#define IDS_FMT_MEDIA9                  (DLG_FORMATDISK + 41)
#define IDS_FMT_MEDIA10                 (DLG_FORMATDISK + 42)
#define IDS_FMT_MEDIA11                 (DLG_FORMATDISK + 43)
#define IDS_FMT_UNUSED                  (DLG_FORMATDISK + 44)
#define IDS_FMT_MEDIA13                 (DLG_FORMATDISK + 45)
#define IDS_FMT_MEDIA14                 (DLG_FORMATDISK + 46)
#define IDS_FMT_MEDIA15                 (DLG_FORMATDISK + 47)
#define IDS_FMT_MEDIA16                 (DLG_FORMATDISK + 48)
#define IDS_FMT_MEDIA17                 (DLG_FORMATDISK + 49)
#define IDS_FMT_MEDIA18                 (DLG_FORMATDISK + 50)
#define IDS_FMT_MEDIA19                 (DLG_FORMATDISK + 51)
#define IDS_FMT_MEDIA20                 (DLG_FORMATDISK + 52)
#define IDS_FMT_MEDIA21                 (DLG_FORMATDISK + 53)

// Japanese specific device types. These also must be in sequence.
#define IDS_FMT_MEDIA_J0                (DLG_FORMATDISK + 80)
#define IDS_FMT_MEDIA_J1                (DLG_FORMATDISK + 81)
#define IDS_FMT_MEDIA_J2                (DLG_FORMATDISK + 82)
#define IDS_FMT_MEDIA_J3                (DLG_FORMATDISK + 83)
#define IDS_FMT_MEDIA_J4                (DLG_FORMATDISK + 84)
#define IDS_FMT_MEDIA_J5                (DLG_FORMATDISK + 85)
#define IDS_FMT_MEDIA_J6                (DLG_FORMATDISK + 86)
#define IDS_FMT_MEDIA_J7                (DLG_FORMATDISK + 87)
#define IDS_FMT_MEDIA_J8                (DLG_FORMATDISK + 88)
#define IDS_FMT_MEDIA_J9                (DLG_FORMATDISK + 89)
#define IDS_FMT_MEDIA_J10               (DLG_FORMATDISK + 90)
#define IDS_FMT_MEDIA_J11               (DLG_FORMATDISK + 91)
#define IDS_FMT_UNUSED_J                (DLG_FORMATDISK + 92)
#define IDS_FMT_MEDIA_J13               (DLG_FORMATDISK + 93)
#define IDS_FMT_MEDIA_J14               (DLG_FORMATDISK + 94)
#define IDS_FMT_MEDIA_J15               (DLG_FORMATDISK + 95)
#define IDS_FMT_MEDIA_J16               (DLG_FORMATDISK + 96)
#define IDS_FMT_MEDIA_J17               (DLG_FORMATDISK + 97)
#define IDS_FMT_MEDIA_J18               (DLG_FORMATDISK + 98)
#define IDS_FMT_MEDIA_J19               (DLG_FORMATDISK + 99)
#define IDS_FMT_MEDIA_J20               (DLG_FORMATDISK + 100)
#define IDS_FMT_MEDIA_J21               (DLG_FORMATDISK + 101)

#ifdef DBCS
// Following definitions were just re-defined,
// because some media types were added. See above IDS_FMT_MEDIA*.
#define IDS_FMT_ALLOC0                  (DLG_FORMATDISK + 60)
#define IDS_FMT_ALLOC1                  (DLG_FORMATDISK + 61)
#define IDS_FMT_ALLOC2                  (DLG_FORMATDISK + 62)
#define IDS_FMT_ALLOC3                  (DLG_FORMATDISK + 63)
#define IDS_FMT_ALLOC4                  (DLG_FORMATDISK + 64)

#define IDS_FMT_CAPUNKNOWN              (DLG_FORMATDISK + 65)
#define IDS_FMT_KB                      (DLG_FORMATDISK + 66)
#define IDS_FMT_MB                      (DLG_FORMATDISK + 67)
#define IDS_FMT_GB                      (DLG_FORMATDISK + 68)
#define IDS_FMT_FORMATTING              (DLG_FORMATDISK + 69)
#define IDS_FMT_FORMAT                  (DLG_FORMATDISK + 70)
#define IDS_FMT_CANCEL                  (DLG_FORMATDISK + 71)
#define IDS_FMT_CLOSE                   (DLG_FORMATDISK + 72)
#else // DBCS
#define IDS_FMT_ALLOC0                  (DLG_FORMATDISK + 60)
#define IDS_FMT_ALLOC1                  (DLG_FORMATDISK + 61)
#define IDS_FMT_ALLOC2                  (DLG_FORMATDISK + 62)
#define IDS_FMT_ALLOC3                  (DLG_FORMATDISK + 63)
#define IDS_FMT_ALLOC4                  (DLG_FORMATDISK + 64)

#define IDS_FMT_CAPUNKNOWN              (DLG_FORMATDISK + 65)
#define IDS_FMT_KB                      (DLG_FORMATDISK + 66)
#define IDS_FMT_MB                      (DLG_FORMATDISK + 67)
#define IDS_FMT_GB                      (DLG_FORMATDISK + 68)
#define IDS_FMT_FORMATTING              (DLG_FORMATDISK + 69)
#define IDS_FMT_FORMAT                  (DLG_FORMATDISK + 70)
#define IDS_FMT_CANCEL                  (DLG_FORMATDISK + 71)
#define IDS_FMT_CLOSE                   (DLG_FORMATDISK + 72)
#endif // DBCS

#define DLG_CHKDSK                      0x7080

#define IDC_FIXERRORS                   (DLG_CHKDSK + 1)
#define IDC_RECOVERY                    (DLG_CHKDSK + 2)
#define IDC_CHKDSKPROGRESS              (DLG_CHKDSK + 3)

#define IDS_CHKDSKCOMPLETE              (DLG_CHKDSK + 4)
#define IDS_CHKDSKFAILED                (DLG_CHKDSK + 5)
#define IDS_CHKDSKCANCELLED             (DLG_CHKDSK + 6)
#define IDS_CANTCANCELCHKDSK            (DLG_CHKDSK + 7)

#define IDC_PHASE                       (DLG_CHKDSK + 8)

#define IDS_CHKACCESSDENIED             (DLG_CHKDSK + 9)
#define IDS_CHKONREBOOT                 (DLG_CHKDSK + 10)

#define IDS_CHKINPROGRESS               (DLG_CHKDSK + 11)
#define IDS_CHKDISK                     (DLG_CHKDSK + 12)
#define IDS_CHKPHASE                    (DLG_CHKDSK + 13)

#endif // WINNT

//
// Resources for NT Console property sheets in links
//
#ifdef WINNT

#define IDD_CONSOLE_SETTINGS            0x8000
#define IDC_CNSL_WINDOWED               (IDD_CONSOLE_SETTINGS +  1)
#define IDC_CNSL_FULLSCREEN             (IDD_CONSOLE_SETTINGS +  2)
#define IDC_CNSL_QUICKEDIT              (IDD_CONSOLE_SETTINGS +  3)
#define IDC_CNSL_INSERT                 (IDD_CONSOLE_SETTINGS +  4)
#define IDC_CNSL_CURSOR_SMALL           (IDD_CONSOLE_SETTINGS +  5)
#define IDC_CNSL_CURSOR_MEDIUM          (IDD_CONSOLE_SETTINGS +  6)
#define IDC_CNSL_CURSOR_LARGE           (IDD_CONSOLE_SETTINGS +  7)
#define IDC_CNSL_HISTORY_SIZE_LBL       (IDD_CONSOLE_SETTINGS +  8)
#define IDC_CNSL_HISTORY_SIZE           (IDD_CONSOLE_SETTINGS +  9)
#define IDC_CNSL_HISTORY_SIZESCROLL     (IDD_CONSOLE_SETTINGS + 10)
#define IDC_CNSL_HISTORY_NUM_LBL        (IDD_CONSOLE_SETTINGS + 11)
#define IDC_CNSL_HISTORY_NUM            (IDD_CONSOLE_SETTINGS + 12)
#define IDC_CNSL_HISTORY_NUMSCROLL      (IDD_CONSOLE_SETTINGS + 13)
#define IDC_CNSL_HISTORY_NODUP          (IDD_CONSOLE_SETTINGS + 14)
#define IDC_CNSL_LANGUAGELIST           (IDD_CONSOLE_SETTINGS + 15)

#define IDD_CONSOLE_FONTDLG             0x8025
#define IDC_CNSL_STATIC                 (IDD_CONSOLE_FONTDLG +  1)
#define IDC_CNSL_FACENAME               (IDD_CONSOLE_FONTDLG +  2)
#define IDC_CNSL_BOLDFONT               (IDD_CONSOLE_FONTDLG +  3)
#define IDC_CNSL_STATIC2                (IDD_CONSOLE_FONTDLG +  4)
#define IDC_CNSL_TEXTDIMENSIONS         (IDD_CONSOLE_FONTDLG +  5)
#define IDC_CNSL_PREVIEWLABEL           (IDD_CONSOLE_FONTDLG +  6)
#define IDC_CNSL_GROUP                  (IDD_CONSOLE_FONTDLG +  7)
#define IDC_CNSL_STATIC3                (IDD_CONSOLE_FONTDLG +  8)
#define IDC_CNSL_STATIC4                (IDD_CONSOLE_FONTDLG +  9)
#define IDC_CNSL_FONTWIDTH              (IDD_CONSOLE_FONTDLG + 10)
#define IDC_CNSL_FONTHEIGHT             (IDD_CONSOLE_FONTDLG + 11)
#define IDC_CNSL_FONTSIZE               (IDD_CONSOLE_FONTDLG + 12)
#define IDC_CNSL_POINTSLIST             (IDD_CONSOLE_FONTDLG + 13)
#define IDC_CNSL_PIXELSLIST             (IDD_CONSOLE_FONTDLG + 14)
#define IDC_CNSL_PREVIEWWINDOW          (IDD_CONSOLE_FONTDLG + 15)
#define IDC_CNSL_FONTWINDOW             (IDD_CONSOLE_FONTDLG + 16)

#define IDD_CONSOLE_SCRBUFSIZE          0x8050
#define IDC_CNSL_SCRBUF_WIDTH_LBL       (IDD_CONSOLE_SCRBUFSIZE +  1)
#define IDC_CNSL_SCRBUF_WIDTH           (IDD_CONSOLE_SCRBUFSIZE +  2)
#define IDC_CNSL_SCRBUF_WIDTHSCROLL     (IDD_CONSOLE_SCRBUFSIZE +  3)
#define IDC_CNSL_SCRBUF_HEIGHT_LBL      (IDD_CONSOLE_SCRBUFSIZE +  4)
#define IDC_CNSL_SCRBUF_HEIGHT          (IDD_CONSOLE_SCRBUFSIZE +  5)
#define IDC_CNSL_SCRBUF_HEIGHTSCROLL    (IDD_CONSOLE_SCRBUFSIZE +  6)
#define IDC_CNSL_WINDOW_WIDTH_LBL       (IDD_CONSOLE_SCRBUFSIZE +  7)
#define IDC_CNSL_WINDOW_WIDTH           (IDD_CONSOLE_SCRBUFSIZE +  8)
#define IDC_CNSL_WINDOW_WIDTHSCROLL     (IDD_CONSOLE_SCRBUFSIZE +  9)
#define IDC_CNSL_WINDOW_HEIGHT_LBL      (IDD_CONSOLE_SCRBUFSIZE + 10)
#define IDC_CNSL_WINDOW_HEIGHT          (IDD_CONSOLE_SCRBUFSIZE + 11)
#define IDC_CNSL_WINDOW_HEIGHTSCROLL    (IDD_CONSOLE_SCRBUFSIZE + 12)
#define IDC_CNSL_WINDOW_POSX_LBL        (IDD_CONSOLE_SCRBUFSIZE + 13)
#define IDC_CNSL_WINDOW_POSX            (IDD_CONSOLE_SCRBUFSIZE + 14)
#define IDC_CNSL_WINDOW_POSXSCROLL      (IDD_CONSOLE_SCRBUFSIZE + 15)
#define IDC_CNSL_WINDOW_POSY_LBL        (IDD_CONSOLE_SCRBUFSIZE + 16)
#define IDC_CNSL_WINDOW_POSY            (IDD_CONSOLE_SCRBUFSIZE + 17)
#define IDC_CNSL_WINDOW_POSYSCROLL      (IDD_CONSOLE_SCRBUFSIZE + 18)
#define IDC_CNSL_AUTO_POSITION          (IDD_CONSOLE_SCRBUFSIZE + 19)

#define IDD_CONSOLE_COLOR               0x8075
#define IDC_CNSL_COLOR_SCREEN_TEXT      (IDD_CONSOLE_COLOR +  1)
#define IDC_CNSL_COLOR_SCREEN_BKGND     (IDD_CONSOLE_COLOR +  2)
#define IDC_CNSL_COLOR_POPUP_TEXT       (IDD_CONSOLE_COLOR +  3)
#define IDC_CNSL_COLOR_POPUP_BKGND      (IDD_CONSOLE_COLOR +  4)
#define IDC_CNSL_COLOR_1                (IDD_CONSOLE_COLOR +  5)
#define IDC_CNSL_COLOR_2                (IDD_CONSOLE_COLOR +  6)
#define IDC_CNSL_COLOR_3                (IDD_CONSOLE_COLOR +  7)
#define IDC_CNSL_COLOR_4                (IDD_CONSOLE_COLOR +  8)
#define IDC_CNSL_COLOR_5                (IDD_CONSOLE_COLOR +  9)
#define IDC_CNSL_COLOR_6                (IDD_CONSOLE_COLOR + 10)
#define IDC_CNSL_COLOR_7                (IDD_CONSOLE_COLOR + 11)
#define IDC_CNSL_COLOR_8                (IDD_CONSOLE_COLOR + 12)
#define IDC_CNSL_COLOR_9                (IDD_CONSOLE_COLOR + 13)
#define IDC_CNSL_COLOR_10               (IDD_CONSOLE_COLOR + 14)
#define IDC_CNSL_COLOR_11               (IDD_CONSOLE_COLOR + 15)
#define IDC_CNSL_COLOR_12               (IDD_CONSOLE_COLOR + 16)
#define IDC_CNSL_COLOR_13               (IDD_CONSOLE_COLOR + 17)
#define IDC_CNSL_COLOR_14               (IDD_CONSOLE_COLOR + 18)
#define IDC_CNSL_COLOR_15               (IDD_CONSOLE_COLOR + 19)
#define IDC_CNSL_COLOR_16               (IDD_CONSOLE_COLOR + 20)
#define IDC_CNSL_COLOR_SCREEN_COLORS_LBL (IDD_CONSOLE_COLOR+ 21)
#define IDC_CNSL_COLOR_SCREEN_COLORS    (IDD_CONSOLE_COLOR + 22)
#define IDC_CNSL_COLOR_POPUP_COLORS_LBL (IDD_CONSOLE_COLOR + 23)
#define IDC_CNSL_COLOR_POPUP_COLORS     (IDD_CONSOLE_COLOR + 24)
#define IDC_CNSL_COLOR_RED_LBL          (IDD_CONSOLE_COLOR + 25)
#define IDC_CNSL_COLOR_RED              (IDD_CONSOLE_COLOR + 26)
#define IDC_CNSL_COLOR_REDSCROLL        (IDD_CONSOLE_COLOR + 27)
#define IDC_CNSL_COLOR_GREEN_LBL        (IDD_CONSOLE_COLOR + 28)
#define IDC_CNSL_COLOR_GREEN            (IDD_CONSOLE_COLOR + 29)
#define IDC_CNSL_COLOR_GREENSCROLL      (IDD_CONSOLE_COLOR + 30)
#define IDC_CNSL_COLOR_BLUE_LBL         (IDD_CONSOLE_COLOR + 31)
#define IDC_CNSL_COLOR_BLUE             (IDD_CONSOLE_COLOR + 32)
#define IDC_CNSL_COLOR_BLUESCROLL       (IDD_CONSOLE_COLOR + 33)

#define IDD_CONSOLE_ADVANCED            0x8100
#define IDC_CNSL_ADVANCED_LABEL         (IDD_CONSOLE_ADVANCED +  1)
#define IDC_CNSL_ADVANCED_LISTVIEW      (IDD_CONSOLE_ADVANCED +  2)

#define IDC_CNSL_GROUP0                 0x8120
#define IDC_CNSL_GROUP1                 (IDC_CNSL_GROUP0 +  1)
#define IDC_CNSL_GROUP2                 (IDC_CNSL_GROUP0 +  2)
#define IDC_CNSL_GROUP3                 (IDC_CNSL_GROUP0 +  3)
#define IDC_CNSL_GROUP4                 (IDC_CNSL_GROUP0 +  4)
#define IDC_CNSL_GROUP5                 (IDC_CNSL_GROUP0 +  5)


// string table constants
#define IDS_CNSL_NAME            0x8125
#define IDS_CNSL_INFO            (IDS_CNSL_NAME+1)
#define IDS_CNSL_TITLE           (IDS_CNSL_NAME+2)
#define IDS_CNSL_RASTERFONT      (IDS_CNSL_NAME+3)
#define IDS_CNSL_FONTSIZE        (IDS_CNSL_NAME+4)
#define IDS_CNSL_SELECTEDFONT    (IDS_CNSL_NAME+5)
#define IDS_CNSL_SAVE            (IDS_CNSL_NAME+6)
#if 0
// NT4.0 Far East doesn't uses
// -kazum
#define IDS_CNSL_DBCSFONTPAGE    (IDS_CNSL_NAME+7)
#define IDS_CNSL_SBCSFONTPAGE    (IDS_CNSL_NAME+8)

#define IDS_CNSL_PreviewText1     (IDS_CNSL_NAME+10)
#define IDS_CNSL_PreviewText2     (IDS_CNSL_NAME+11)
#define IDS_CNSL_PreviewText3     (IDS_CNSL_NAME+12)
#define IDS_CNSL_PreviewText4     (IDS_CNSL_NAME+13)
#define IDS_CNSL_PreviewText5     (IDS_CNSL_NAME+14)
#define IDS_CNSL_PreviewText6     (IDS_CNSL_NAME+15)
#define IDS_CNSL_PreviewText7     (IDS_CNSL_NAME+16)
#define IDS_CNSL_PreviewText8     (IDS_CNSL_NAME+17)

#define IDS_CNSL_PreviewTextDbcs1 (IDS_CNSL_NAME+20)
#define IDS_CNSL_PreviewTextDbcs2 (IDS_CNSL_NAME+21)
#define IDS_CNSL_PreviewTextDbcs3 (IDS_CNSL_NAME+22)
#define IDS_CNSL_PreviewTextDbcs4 (IDS_CNSL_NAME+23)
#define IDS_CNSL_PreviewTextDbcs5 (IDS_CNSL_NAME+24)
#define IDS_CNSL_PreviewTextDbcs6 (IDS_CNSL_NAME+25)
#define IDS_CNSL_PreviewTextDbcs7 (IDS_CNSL_NAME+26)
#define IDS_CNSL_PreviewTextDbcs8 (IDS_CNSL_NAME+27)
#endif
#endif // WINNT

#ifdef WINNT
#define IDS_SHUTDOWN                0x8200
#define IDS_RESTART                 (IDS_SHUTDOWN+1)
#define IDS_NO_PERMISSION_SHUTDOWN  (IDS_SHUTDOWN+2)
#define IDS_NO_PERMISSION_RESTART   (IDS_SHUTDOWN+3)
#endif

#ifdef WINNT
#define IDS_UNMOUNT_TITLE           0x8225
#define IDS_UNMOUNT_TEXT            (IDS_UNMOUNT_TITLE+1)
#define IDS_EJECT_TITLE             (IDS_UNMOUNT_TITLE+2)
#define IDS_EJECT_TEXT              (IDS_UNMOUNT_TITLE+3)
#define IDS_RETRY_UNMOUNT_TITLE     (IDS_UNMOUNT_TITLE+4)
#define IDS_RETRY_UNMOUNT_TEXT      (IDS_UNMOUNT_TITLE+5)
#endif

// Resources for common program groups/items
#define IDS_CSIDL_CSTARTMENU_L          0x7100
#define IDS_CSIDL_CSTARTMENU_S          0x7101
#define IDS_CSIDL_CPROGRAMS_L           0x7102
#define IDS_CSIDL_CPROGRAMS_S           0x7103
#define IDS_CSIDL_CSTARTUP_L            0x7104
#define IDS_CSIDL_CSTARTUP_S            0x7105
#define IDS_CSIDL_CDESKTOPDIRECTORY_L   0x7106
#define IDS_CSIDL_CDESKTOPDIRECTORY_S   0x7107
#define IDS_CSIDL_CFAVORITES_L          0x7108
#define IDS_CSIDL_CFAVORITES_S          0x7109
#define IDS_CSIDL_CAPPDATA_L            0x710a
#define IDS_CSIDL_CAPPDATA_S            0x710b

// strings for file/folder property sheet
#define IDS_READONLY                    0x7110
#define IDS_NOTREADONLY                 0x7111
#define IDS_HIDE                        0x7112
#define IDS_UNHIDE                      0x7113
#define IDS_ARCHIVE                     0x7114
#define IDS_UNARCHIVE                   0x7115
#define IDS_INDEX                       0x7116
#define IDS_DISABLEINDEX                0x7117
#define IDS_COMPRESS                    0x7118
#define IDS_UNCOMPRESS                  0x7119
#define IDS_ENCRYPT                     0x711a
#define IDS_DECRYPT                     0x711b

#define IDS_UNKNOWNAPPLICATION          0x711c
#define IDS_DESCRIPTION                 0x711d

#define IDS_APPLYINGATTRIBS             0x711e
#define IDS_CALCULATINGSIZE             0x711f
#define IDS_APPLYINGATTRIBSTO           0x7120
#define IDS_THISFOLDER                  0x7121
#define IDS_THESELECTEDITEMS            0x7122
#define IDS_OPENSWITH                   0x7123
#define IDS_SUPERHIDDENWARNING          0x7124
#define IDS_THISVOLUME                  0x7125

// string for find computer stuff (used by netviewx.c)
//                                      0x7200 // use me
//                                      0x7201 // me, too
#define IDS_FC_NAME                     0x7203

#define IDS_LINEBREAK_REMOVE            0x72A0  
#define IDS_LINEBREAK_PRESERVE          0x72A1

//  Strings used in DLG_FSEARCH_xxx, DLG_PSEARCH and DLG_CSEARCH dialogs
#define IDS_FSEARCH_FIRST               0x7300
#define IDS_FSEARCH_CAPTION             (IDS_FSEARCH_FIRST+0)
#define IDS_FSEARCH_TBLABELS            (IDS_FSEARCH_FIRST+1)
#define IDS_FSEARCH_MODIFIED_DATE       (IDS_FSEARCH_FIRST+2)
#define IDS_FSEARCH_CREATION_DATE       (IDS_FSEARCH_FIRST+3)
#define IDS_FSEARCH_ACCESSED_DATE       (IDS_FSEARCH_FIRST+4)
#define IDS_FSEARCH_SIZE_EQUAL          (IDS_FSEARCH_FIRST+5)
#define IDS_FSEARCH_SIZE_GREATEREQUAL   (IDS_FSEARCH_FIRST+6)
#define IDS_FSEARCH_SIZE_LESSEREQUAL    (IDS_FSEARCH_FIRST+7)
#define IDS_FSEARCH_FTP_SUBSTR          (IDS_FSEARCH_FIRST+8)
#define IDS_FSEARCH_FTP_NOT_SUPPORTED   (IDS_FSEARCH_FIRST+9)
#define IDS_FSEARCH_HTTP_SUBSTR         (IDS_FSEARCH_FIRST+10)
#define IDS_FSEARCH_HTTP_NOT_SUPPORTED  (IDS_FSEARCH_FIRST+11)
#define IDS_FSEARCH_CI_READY            (IDS_FSEARCH_FIRST+12)  // CI status text
#define IDS_FSEARCH_CI_READY_LINK       (IDS_FSEARCH_FIRST+13)  // CI link caption
#define IDS_FSEARCH_CI_BUSY             (IDS_FSEARCH_FIRST+14)  // CI status text
#define IDS_FSEARCH_CI_BUSY_LINK        (IDS_FSEARCH_FIRST+15)  // CI link caption
#define IDS_FSEARCH_CI_DISABLED         (IDS_FSEARCH_FIRST+16)  // status text
#define IDS_FSEARCH_CI_DISABLED_LINK    (IDS_FSEARCH_FIRST+17)  // link caption
#define IDS_FSEARCH_CI_STATUSFMT        (IDS_FSEARCH_FIRST+18)  // CI status formatting template
#define IDS_FSEARCH_INVALIDFOLDER_FMT   (IDS_FSEARCH_FIRST+19)  // '%s' is an invalid folder name
#define IDS_FSEARCH_EMPTYFOLDER         (IDS_FSEARCH_FIRST+20)  // You must enter a valid folder name.
#define IDS_FSEARCH_CI_DISABLED_WARNING (IDS_FSEARCH_FIRST+21)  // Can't do the query; CI is disabled.
#define IDS_FSEARCH_SEARCHLINK_FILES    (IDS_FSEARCH_FIRST+22)
#define IDS_FSEARCH_SEARCHLINK_COMPUTERS (IDS_FSEARCH_FIRST+23)
#define IDS_FSEARCH_SEARCHLINK_PRINTERS (IDS_FSEARCH_FIRST+24)
#define IDS_FSEARCH_SEARCHLINK_PEOPLE   (IDS_FSEARCH_FIRST+25)
#define IDS_FSEARCH_SEARCHLINK_INTERNET (IDS_FSEARCH_FIRST+26)
#define IDS_FSEARCH_SEARCHLINK_OPTIONS  (IDS_FSEARCH_FIRST+27)
#define IDS_FSEARCH_SEARCHLINK_PREVIOUS (IDS_FSEARCH_FIRST+28)
#define IDS_FSEARCH_GROUPBTN_OPTIONS    (IDS_FSEARCH_FIRST+29)
#define IDS_FINDVIEWEMPTYINIT           (IDS_FSEARCH_FIRST+30)
#define IDS_FINDVIEWEMPTYBUSY           (IDS_FSEARCH_FIRST+31)
#define IDS_FSEARCH_NEWINFOTIP          (IDS_FSEARCH_FIRST+32)
#define IDS_FSEARCH_HELPINFOTIP         (IDS_FSEARCH_FIRST+33)
#define IDS_CSEARCH_NEWINFOTIP          (IDS_FSEARCH_FIRST+34)
#define IDS_CSEARCH_HELPINFOTIP         (IDS_FSEARCH_FIRST+35)
#define IDS_FSEARCH_BANDCAPTION         (IDS_FSEARCH_FIRST+36)
#define IDS_FSEARCH_BANDWIDTH           (IDS_FSEARCH_FIRST+37)
#define IDS_DOCFIND_CONSTRAINT          (IDS_FSEARCH_FIRST+38)
#define IDS_DOCFIND_SCOPEERROR          (IDS_FSEARCH_FIRST+39)
#define IDS_DOCFIND_PATHNOTFOUND        (IDS_FSEARCH_FIRST+41)
#define IDS_FSEARCH_CI_ENABLED          (IDS_FSEARCH_FIRST+42)  // status text
#define IDS_FSEARCH_CI_ENABLED_LINK     (IDS_FSEARCH_FIRST+43)  // link caption
#define IDS_FSEARCH_STARTSTOPWIDTH      (IDS_FSEARCH_FIRST+44)  // width, in DBU, of start, stop buttons.
#define IDS_DOCFIND_CI_NOT_CASE_SEN     (IDS_FSEARCH_FIRST+45)  // ci is not case sensitive

#define IDS_LINKWINDOW_DEFAULTACTION    0x73FE
#define IDS_GROUPBTN_DEFAULTACTION      0x73FF

#define IDS_DD_FIRST                    0x7400
#define IDS_DD_COPY                     (IDS_DD_FIRST + DDIDM_COPY)
#define IDS_DD_MOVE                     (IDS_DD_FIRST + DDIDM_MOVE)
#define IDS_DD_LINK                     (IDS_DD_FIRST + DDIDM_LINK)
#define IDS_DD_SCRAP_COPY               (IDS_DD_FIRST + DDIDM_SCRAP_COPY)
#define IDS_DD_SCRAP_MOVE               (IDS_DD_FIRST + DDIDM_SCRAP_MOVE)
#define IDS_DD_DOCLINK                  (IDS_DD_FIRST + DDIDM_DOCLINK)
#define IDS_DD_CONTENTS_COPY            (IDS_DD_FIRST + DDIDM_CONTENTS_COPY)
#define IDS_DD_CONTENTS_MOVE            (IDS_DD_FIRST + DDIDM_CONTENTS_MOVE)
#define IDS_DD_SYNCCOPY                 (IDS_DD_FIRST + DDIDM_SYNCCOPY)
#define IDS_DD_SYNCCOPYTYPE             (IDS_DD_FIRST + DDIDM_SYNCCOPYTYPE)
#define IDS_DD_CONTENTS_LINK            (IDS_DD_FIRST + DDIDM_CONTENTS_LINK)
#define IDS_DD_CONTENTS_DESKCOMP        (IDS_DD_FIRST + DDIDM_CONTENTS_DESKCOMP)
#define IDS_DD_CONTENTS_DESKIMG         (IDS_DD_FIRST + DDIDM_CONTENTS_DESKIMG)
#define IDS_DD_OBJECT_COPY              (IDS_DD_FIRST + DDIDM_OBJECT_COPY)
#define IDS_DD_OBJECT_MOVE              (IDS_DD_FIRST + DDIDM_OBJECT_MOVE)
#define IDS_DD_CONTENTS_DESKURL         (IDS_DD_FIRST + DDIDM_CONTENTS_DESKURL)

// unicpp\resource.h defines more IDs starting at 0x8600

// Active desktop prop page
#define IDC_TICKERINTERVAL              1000
#define IDC_TICKERINTERVAL_SPIN         1001
#define IDC_NEWSINTERVAL                1002
#define IDC_NEWSINTERVAL_SPIN           1003
#define IDC_NEWSUPDATE                  1004
#define IDC_NEWSUPDATE_SPIN             1005
#define IDC_TICKERDISPLAY               1006
#define IDC_NEWSDISPLAY                 1007
#define IDC_NEWSSPEED                   1008
#define IDC_TICKERSPEED                 1009

// for column arrange dialog
#define IDC_COL_LVALL         1001
#define IDC_COL_UP            1003
#define IDC_COL_DOWN          1004
#define IDC_COL_WIDTH         1005
#define IDC_COL_SHOW          1006
#define IDC_COL_HIDE          1007

// Disk space error dialog
#define IDC_DISKERR_EXPLAIN             1000
#define IDC_DISKERR_LAUNCHCLEANUP       1001
#define IDC_DISKERR_STOPICON            1002


#define CLASS_NSC           TEXT(CLASS_NSCA)
#define CLASS_NSCA          "SHBrowseForFolder ShellNameSpace Control"


#define SMCM_STARTMENU_FIRST        0x5000
#define SMCM_OPEN                   (SMCM_STARTMENU_FIRST + 0)
#define SMCM_EXPLORE                (SMCM_STARTMENU_FIRST + 1)
#define SMCM_OPEN_ALLUSERS          (SMCM_STARTMENU_FIRST + 2)
#define SMCM_EXPLORE_ALLUSERS       (SMCM_STARTMENU_FIRST + 3)

#include "unicpp\resource.h"

#endif // _IDS_H_
