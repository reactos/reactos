// Resource IDs for FTPFOLDR
//


// This means the control won't have a name.
#define IDC_UNUSED          -1


/*****************************************************************************
 *
 *      Icons
 *
 *****************************************************************************/

//#define IDI_FTPSERVER       1               // Icon for an FTP Server
#define IDI_FTPSERVER       IDI_FTPFOLDER   // Icon for an FTP Server
#define IDI_FTPFOLDER       2               // Icon for a closed folder on a FTP Server
#define IDI_FTPOPENFOLDER   3               // Icon for an open folder on a FTP Server
#define IDI_FTPMULTIDOC     4               // Icon for several ftp items.
#define IDI_DELETEITEM      5               // Delete File
// 2 Holes
#define IDI_REPLACE         8               // File being overwritten
#define IDI_KEY             9               // Key icon for Login As dialog
//#define IDI_NETFOLDER       10              // Folder for Proxy Blocking dialog
#define IDI_NETFOLDER       IDI_FTPFOLDER   // Folder for Proxy Blocking dialog

#define IDI_WRITE_ALLOWED   10              // This folder has write access 
#define IDI_WRITE_NOTALLOWED 11             // This folder does not have read access 

/*****************************************************************************
 *
 *      Menus
 *
 *****************************************************************************/

#define IDM_ITEMCONTEXT         1   // Context menu for items

#define IDM_M_FOLDERVERBS       0   // Verbs only for folders
#define IDM_M_FILEVERBS         1   // Verbs only for files
#define IDM_M_VERBS             2   // Verbs appropriate for all selected items (in addition to above)
#define IDM_M_SHAREDVERBS       3   // Verbs shared with defview (Common Shell verbs that aren't added in Context Menu)
#define IDM_M_BACKGROUNDVERBS   4   // Verbs for the background menu (only when nothing is selected)

#define IDM_M_BACKGROUND_POPUPMERGE     10   // Items that need to be merged with the current menu. (Arrange Items).

#define IDC_ITEM_OPEN           0   // &Open -- folders only
#define IDC_ITEM_EXPLORE        1   // &Explore -- folders only
#define IDC_ITEM_DOWNLOAD       2   // Do&wnload
#define IDC_ITEM_BKGNDPROP      3   // Properties for the background folder.
#define IDC_LOGIN_AS            4   // Login as...
#define IDC_ITEM_NEWFOLDER      5   // New Folder - Background Folder Only

#define IDC_ITEM_ABOUTSITE      6
#define IDC_ITEM_ABOUTFTP       7

#define IDM_SHARED_EDIT_CUT     8
#define IDM_SHARED_EDIT_COPY    9
#define IDM_SHARED_EDIT_PASTE   10
#define IDM_SHARED_FILE_LINK    11
#define IDM_SHARED_FILE_DELETE  12
#define IDM_SHARED_FILE_RENAME  13
#define IDM_SHARED_FILE_PROP    14

#define IDC_ITEM_FTPHELP        15

#define IDC_ITEM_MAX            16

/*****************************************************************************/

#define IDM_FTPMERGE            2    /* Menu bar */

/*
 *  These are biased by SFVIDM_CLIENT_FIRST and can go up to 255.
 *  However, IDM_SORT_* uses 0x30 through 0x3F.
 */
#define IDM_PROPERTIESBG        20

#define IDM_ID_DEBUG            0x40    /* 0x40 through 0x60 */

/*****************************************************************************/

#define IDM_DROPCONTEXT         3   /* Context menu for nondefault d/d */

/*****************************************************************************/

#define IDM_FOLDERCONTEXT       4   /* Context menu for folder background */

#define IDM_FOLDER_NEW          0
#define IDM_FOLDER_PROP         1

/*****************************************************************************\
    Shared dialog IDs

    Use these deltas whenever you want to use FtpDlg_InitDlg to
    initialize a group of controls based on a list of pidls.

    NOTE!  These cannot be an enum because the resource compiler doesn't
    understand enums.
\*****************************************************************************/

#define DLGTEML_FILENAME            0       // Name of file(s)
#define DLGTEML_FILENAMEEDITABLE    1       // Editable filename
#define DLGTEML_FILEICON            2       // Icon for file(s)
#define DLGTEML_FILESIZE            3       // Size of file(s)
#define DLGTEML_FILETIME            4       // Modification time of file(s)
#define DLGTEML_FILETYPE            5       // Type description for file(s)
#define DLGTEML_LOCATION            6       // Location of folder
#define DLGTEML_COUNT               7       // Location of count
#define DLGTEML_MAX                 8

#define DLGTEML_LABEL               20      // The label for an item (DLGTEML_FILENAME) equals (DLGTEML_FILENAME+DLGTEML_LABEL)

/*****************************************************************************
 *
 *      Derived dialog IDs
 *
 *****************************************************************************/

#define IDC_ITEM                110
#define IDC_FILENAME            (IDC_ITEM + DLGTEML_FILENAME)
#define IDC_FILENAME_EDITABLE   (IDC_ITEM + DLGTEML_FILENAMEEDITABLE)
#define IDC_FILEICON            (IDC_ITEM + DLGTEML_FILEICON)
#define IDC_FILESIZE            (IDC_ITEM + DLGTEML_FILESIZE)
#define IDC_FILETIME            (IDC_ITEM + DLGTEML_FILETIME)
#define IDC_FILETYPE            (IDC_ITEM + DLGTEML_FILETYPE)
#define IDC_LOCATION            (IDC_ITEM + DLGTEML_LOCATION)
#define IDC_COUNT               (IDC_ITEM + DLGTEML_COUNT)

#define IDC_FILETIME_LABEL      (IDC_ITEM + DLGTEML_FILETIME + DLGTEML_LABEL)
#define IDC_FILESIZE_LABEL      (IDC_ITEM + DLGTEML_FILESIZE + DLGTEML_LABEL)

#define IDC_ITEM2               120
#define IDC_FILENAME2           (IDC_ITEM2 + DLGTEML_FILENAME)
#define IDC_FILENAME_EDITABLE2  (IDC_ITEM2 + DLGTEML_FILENAMEEDITABLE)
#define IDC_FILEICON2           (IDC_ITEM2 + DLGTEML_FILEICON)
#define IDC_FILESIZE2           (IDC_ITEM2 + DLGTEML_FILESIZE)
#define IDC_FILETIME2           (IDC_ITEM2 + DLGTEML_FILETIME)
#define IDC_FILETYPE2           (IDC_ITEM2 + DLGTEML_FILETYPE)
#define IDC_LOCATION2           (IDC_ITEM2 + DLGTEML_LOCATION)
#define IDC_COUNT2              (IDC_ITEM2 + DLGTEML_COUNT)

#define IDC_FILETIME2_LABEL     (IDC_ITEM2 + DLGTEML_FILETIME + DLGTEML_LABEL)
#define IDC_FILESIZE2_LABEL     (IDC_ITEM2 + DLGTEML_FILESIZE + DLGTEML_LABEL)

/*****************************************************************************
 *
 *      Dialogs (and dialog controls)
 *
 *****************************************************************************/

#define IDD_REPLACE             1       /* File being overwritten */

#define IDC_REPLACE_YES         IDYES   /* Overwrite it */
#define IDC_REPLACE_YESTOALL    32      /* Overwrite it and everything else */
#define IDC_REPLACE_NO          IDNO    /* Skip this file */
#define IDC_REPLACE_NOTOALL     33      /* Skip all files that conflict */
#define IDC_REPLACE_CANCEL      IDCANCEL /* Stop copying */

#define IDC_REPLACE_OLDFILE     35      /* Description of old file */
#define IDC_REPLACE_NEWFILE     37      /* Description of new file */
#define IDC_REPLACE_NEWICON     38      /* Icon of new file */

/*****************************************************************************/

#define IDD_DELETEFILE          2       /* File being deleted */
#define IDD_DELETEFOLDER        3       /* Folder being deleted */
#define IDD_DELETEMULTI         4       /* Files/Folders being deleted */

/*****************************************************************************/

#define IDD_FILEPROP            32
#define IDC_READONLY            7

// Some items in the Login Dialog are the same across all three dialogs,
// but some change.
// These are the items that are all the same.
#define IDD_LOGINDLG                        40
#define IDC_LOGINDLG_FTPSERVER              (IDD_LOGINDLG + 1)
#define IDC_LOGINDLG_ANONYMOUS_CBOX         (IDD_LOGINDLG + 2)

// These are the items that are different.
#define IDC_LOGINDLG_USERNAME               (IDD_LOGINDLG + 3)
#define IDC_LOGINDLG_USERNAME_ANON          (IDD_LOGINDLG + 4)
#define IDC_LOGINDLG_MESSAGE_ANONREJECT     (IDD_LOGINDLG + 5)
#define IDC_LOGINDLG_MESSAGE_NORMAL         (IDD_LOGINDLG + 6)
#define IDC_LOGINDLG_MESSAGE_USERREJECT     (IDD_LOGINDLG + 7)
#define IDC_LOGINDLG_PASSWORD_DLG1          (IDD_LOGINDLG + 8)
#define IDC_LOGINDLG_PASSWORD_DLG2          (IDD_LOGINDLG + 9)
#define IDC_LOGINDLG_PASSWORD_LABEL_DLG1    (IDD_LOGINDLG + 10)
#define IDC_LOGINDLG_PASSWORD_LABEL_DLG2    (IDD_LOGINDLG + 11)
#define IDC_LOGINDLG_NOTES_DLG1             (IDD_LOGINDLG + 12)
#define IDC_LOGINDLG_NOTES_DLG2             (IDD_LOGINDLG + 13)
#define IDC_LOGINDLG_SAVE_PASSWORD          (IDD_LOGINDLG + 14)



#define IDD_MOTDDLG                         80
#define IDC_MOTDDLG_MESSAGE                 (IDD_MOTDDLG + 1)

#define IDD_DOWNLOADDIALOG                  90
#define IDC_DOWNLOAD_MESSAGE                (IDD_DOWNLOADDIALOG + 1)
#define IDC_DOWNLOAD_TITLE                  (IDD_DOWNLOADDIALOG + 2)
#define IDC_DOWNLOAD_DIR                    (IDD_DOWNLOADDIALOG + 3)
#define IDC_BROWSE_BUTTON                   (IDD_DOWNLOADDIALOG + 4)
#define IDC_DOWNLOAD_AS                     (IDD_DOWNLOADDIALOG + 5)
#define IDC_DOWNLOAD_AS_LIST                (IDD_DOWNLOADDIALOG + 6)
#define IDC_DOWNLOAD_BUTTON                 (IDD_DOWNLOADDIALOG + 7)

#define DLG_MOVECOPYPROGRESS                100
#define IDD_ANIMATE                         (DLG_MOVECOPYPROGRESS + 1)
#define IDD_NAME                            (DLG_MOVECOPYPROGRESS + 2)
#define IDD_TONAME                          (DLG_MOVECOPYPROGRESS + 3)
#define IDD_TIMEEST                         (DLG_MOVECOPYPROGRESS + 5)
#define IDD_PROBAR                          (DLG_MOVECOPYPROGRESS + 4)

#define IDD_PROXYDIALOG                     110
#define IDC_PROXY_MESSAGE                   (IDD_PROXYDIALOG + 0)

#define IDD_CHMOD                           140
#define IDC_CHMOD_OR                        (IDD_CHMOD + 0)
#define IDC_CHMOD_OW                        (IDD_CHMOD + 1)
#define IDC_CHMOD_OE                        (IDD_CHMOD + 2)
#define IDC_CHMOD_GR                        (IDD_CHMOD + 3)
#define IDC_CHMOD_GW                        (IDD_CHMOD + 4)
#define IDC_CHMOD_GE                        (IDD_CHMOD + 5)
#define IDC_CHMOD_AR                        (IDD_CHMOD + 6)
#define IDC_CHMOD_AW                        (IDD_CHMOD + 7)
#define IDC_CHMOD_AE                        (IDD_CHMOD + 8)

#define IDC_CHMOD_LABEL_EXECUTE             (IDD_CHMOD + 9)
#define IDC_CHMOD_LABEL_PERM                (IDD_CHMOD + 10)
#define IDC_CHMOD_LABEL_OWNER               (IDD_CHMOD + 11)
#define IDC_CHMOD_LABEL_GROUP               (IDD_CHMOD + 12)
#define IDC_CHMOD_LABEL_ALL                 (IDD_CHMOD + 13)
#define IDC_CHMOD_LABEL_READ                (IDD_CHMOD + 14)
#define IDC_CHMOD_LABEL_WRITE               (IDD_CHMOD + 15)
#define IDC_CHMOD_GROUPBOX                  (IDD_CHMOD + 16)

#define IDC_CHMOD_LAST                      (IDD_CHMOD + 16)
#define IDC_CHMOD_NOT_ALLOWED               (IDD_CHMOD + 17)


/*****************************************************************************
 *
 *      Column headings (for details view)
 *
 *****************************************************************************/

#define COL_NAME            0
#define COL_SIZE            1
#define COL_TYPE            2
#define COL_MODIFIED        3
#define COL_MAX             4

#define IDM_SORT_FIRST        0x0030
#define IDM_SORTBYNAME        (IDM_SORT_FIRST + 0x0000)
#define IDM_SORTBYSIZE        (IDM_SORT_FIRST + 0x0001)
#define IDM_SORTBYTYPE        (IDM_SORT_FIRST + 0x0002)
#define IDM_SORTBYDATE        (IDM_SORT_FIRST + 0x0003)

#define CONVERT_IDMID_TO_COLNAME(idc)      ((idc) - IDM_SORT_FIRST)

#if CONVERT_IDMID_TO_COLNAME(IDM_SORTBYNAME) != COL_NAME || \
    CONVERT_IDMID_TO_COLNAME(IDM_SORTBYSIZE) != COL_SIZE || \
    CONVERT_IDMID_TO_COLNAME(IDM_SORTBYTYPE) != COL_TYPE || \
    CONVERT_IDMID_TO_COLNAME(IDM_SORTBYDATE) != COL_MODIFIED
#error FSIDM_ and ici are out of sync.
#endif

/*****************************************************************************
 *
 *      Strings
 *
 *****************************************************************************/

/* 0 ... 31 reserved for help text for IDC_ITEM_XXX menu commands */
#define IDS_ITEM_HELP(idc)       (idc)

/* 32 .. 39 reserved for title text for iciXXX column indices */
#define IDS_HEADER_NAME(ici)    (32+ici)

/* 40 .. 47 reserved for help text for iciXXX column indices */
#define IDS_HEADER_HELP(ici)    (40+ici)

/* 64 .. 95 reserved for progress feedback */
#define IDS_EMPTY               64
#define IDS_CONNECTING          65
#define IDS_CHDIR               66
#define IDS_LS                  67
#define IDS_DELETING            68
#define IDS_RENAMING            69
#define IDS_GETFINDDATA         70
#define IDS_COPYING             71
#define IDS_DOWNLOADING         72
#define IDS_DL_SRC_DEST         73
#define IDS_COPY_TITLE          74
#define IDS_MOVE_TITLE          75
#define IDS_DELETE_TITLE        76
#define IDS_DOWNLOAD_TITLE      77
#define IDS_DL_TYPE_AUTOMATIC   78
#define IDS_DL_TYPE_ASCII       79
#define IDS_DL_TYPE_BINARY      80
#define IDS_DL_SRC_DIR          81


#define IDA_FTPDOWNLOAD         0x100   // This matches IDA_DOWNLOAD (in shdocvw.dll)
#define IDA_FTPUPLOAD           0x101   // FS->Ftp Animation (in msieftp.dll)
#define IDA_FTPDELETE           0x102   // Ftp->Air (Hard Delete) (in msieftp.dll)

/* 256 onward are just random strings */
#define IDS_NUMBERK             256
#define IDS_NUMBERTB            257

#define IDS_HELP_MSIEFTPTITLE   258
#define IDS_HELP_ABOUTFOLDER    259
#define IDS_HELP_ABOUTBOX       260
#define IDS_HELP_WELCOMEMSGTITLE 261
#define IDS_PROP_SHEET_TITLE    262
#define IDS_PRETTYNAMEFORMAT    263
#define IDS_SEVERAL_SELECTED    264
#define IDS_ELLIPSES            265
#define IDS_NEW_FOLDER_FIRST    266
#define IDS_NEW_FOLDER_TEMPLATE 267

#define IDS_CANTSHUTDOWN        280
#define IDS_PROGRESS_CANCEL     282
#define IDS_NO_MESSAGEOFTHEDAY  285
#define IDS_ITEMTYPE_FOLDER     286
#define IDS_ITEMTYPE_SERVER     287
#define IDS_PROGRESS_UPLOADTIMECALC   288
#define IDS_PROGRESS_DELETETIMECALC   289
#define IDS_PROGRESS_DOWNLOADTIMECALC 290
#define IDS_OFFLINE_PROMPTTOGOONLINE  291
#define IDS_RECYCLE_IS_PERM_WARNING   292

// STATUS BAR Strings

// Status Bar Icon ToolTips
// These Text Strings are the tool tips for the icons.
#define IDS_BEGIN_SB_TOOLTIPS   300
#define IDS_WRITE_ALLOWED       300              // This folder has write access 
#define IDS_WRITE_NOTALLOWED    301              // This folder does not have read access 

// Progress Bar ToolTips
#define IDS_PROG_ZERO           330              // Zero percent.
#define IDS_PROG_NPERCENT       331              // n percent.
#define IDS_PROG_DONE           332              // 100 percent.

// Zones
#define IDS_ZONES_UNKNOWN       340              // Unknown Zone
#define IDS_ZONES_TOOLTIP       341              // Zone Status Bar Pane Tooltip

// User Status Bar Pane
#define IDS_USER_TEMPLATE       350              // "User: <UserName>"
#define IDS_USER_ANNONYMOUS     351               // <UserName> = "Annonymous"
#define IDS_USER_USERTOOLTIP    352              // Info on type of user log-in. (non-Annonymously)
#define IDS_USER_ANNONTOOLTIP   353              // Info on type of user log-in. (Annonymously)

// FTP Errors
#define IDS_FTPERR_TITLE            400              // Title for Messages.
#define IDS_FTPERR_TITLE_ERROR      401              // Title for Error Messages.
#define IDS_FTPERR_UNKNOWN          402              // Unknown error.
#define IDS_FTPERR_WININET          403              // Prep Wininet Error
#define IDS_FTPERR_WININET_CONTINUE 404          // Prep Wininet Error and ask if user wants to continue
#define IDS_FTPERR_FILECOPY         405              // Error putting file on FTP Server
#define IDS_FTPERR_DIRCOPY          406              // Error Creating a directory on the FTP Server
#define IDS_FTPERR_FILERENAME       407              // Error renaming file on FTP Server
#define IDS_FTPERR_CHANGEDIR        408              // Error opening that folder on the FTP Server
#define IDS_FTPERR_DELETE           409              // Error deleting that folder or file on the FTP Server
#define IDS_FTPERR_OPENFOLDER       410              // Error opening that folder on the FTP Server
#define IDS_FTPERR_FOLDERENUM       411              // Error getting the rest of the file names in the folder on the FTP Server
#define IDS_FTPERR_NEWFOLDER        412              // Error create a new folder on the FTP Server
#define IDS_FTPERR_DROPFAIL         413              // Error dropping a file or folder.
#define IDS_FTPERR_INVALIDFTPNAME   414            // This file name isn't a valid FTP File Name.  Maybe unicode.
#define IDS_FTPERR_CREATEDIRPROMPT  415           // The directory doesn't exist, do you want to create it?
#define IDS_FTPERR_CREATEFAILED     416              // Attempting to create the directory failed.
#define IDS_FTPERR_GETDIRLISTING    417             // An error occured reading the contents of the folder
#define IDS_FTPERR_DOWNLOADING      418             // Download failed
#define IDS_FTPERR_RENAME_REPLACE   419           // A file with this name already exists.  Do you want to replace that file?
#define IDS_FTPERR_RENAME_EXT_WRN   420           // If you change a filename extension, the file may become unusable.\n\nAre you sure you want to change it?
#define IDS_FTPERR_RENAME_TITLE     421           // Rename title ("Rename")
#define IDS_FTP_PROXY_WARNING       422           // The folder '%s' is currently read-only.\n\nThe proxy server to which you are connected will only enable...
#define IDS_FTPERR_CHMOD            423           // An error occured changing the permissions on the file or folder on the FTP Server.  Make sure you have permission to change this item.
#define IDS_FTPERR_BAD_DL_TARGET    424           // Bad dir chosen in SHBrowseForFolder


// Login Dialog Message
#define IDS_LOGIN_LOGINAS           450              // 
#define IDS_LOGIN_SERVER            451              // 


// Dialog Strings
#define IDS_DLG_DOWNLOAD_TITLE      500              // Title for the 'Choose Download Directory' dialog.



#define IDS_INSTALL_TEMPLATE        700              // 
#define IDS_INSTALL_TEMPLATE_NT5    701              // 


// HTML dialog resources
#define RT_FILE                     2110


// BUGBUG - For some reason, I can't read \iedev\shell\shlwapi.w
#define IDC_MESSAGECHECKEX          0x1202
