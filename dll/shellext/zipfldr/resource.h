#pragma once

/* Bitmaps */
#define IDB_ZIPFLDR                    164
#define IDB_HEADER                     200
#define IDB_WATERMARK                  201

/* registry stuff */
#define IDR_ZIPFLDR                     8000


/* Dialogs */

#define IDD_PROPPAGEDESTINATION     1000
#define IDC_DIRECTORY               1001
#define IDC_BROWSE                  1002
#define IDC_PASSWORD                1003
#define IDC_PROGRESS                1004
#define IDC_STATUSTEXT              1005

#define IDD_PROPPAGECOMPLETE        1100
#define IDC_DESTDIR                 1101
#define IDC_SHOW_EXTRACTED          1102

#define IDD_CONFIRM_FILE_REPLACE    1200
#define IDYESALL                    1202
#define IDC_EXCLAMATION_ICON        1205
#define IDC_MESSAGE                 1206

#define IDD_PASSWORD                1300
#define IDSKIP                      1301


/* Strings */
#define IDS_COL_NAME        100
#define IDS_COL_TYPE        101
#define IDS_COL_COMPRSIZE   102
#define IDS_COL_PASSWORD    103
#define IDS_COL_SIZE        104
#define IDS_COL_RATIO       105
#define IDS_COL_DATE_MOD    106
#define IDS_YES             107
#define IDS_NO              108
#define IDS_ERRORTITLE      109
#define IDS_CANTSTARTTHREAD 110
#define IDS_NOFILES         111
#define IDS_CANTCREATEZIP   112
#define IDS_CANTREADFILE    113
#define IDS_EXTRACTING      114
#define IDS_CANTEXTRACTFILE 115
#define IDS_DECOMPRESSERROR 116
#define IDS_UNKNOWNERROR    117

/* Wizard titles */
#define IDS_WIZ_TITLE           8000
#define IDS_WIZ_DEST_TITLE      8001
#define IDS_WIZ_DEST_SUBTITLE   8002
#define IDS_WIZ_COMPL_TITLE     8003
#define IDS_WIZ_COMPL_SUBTITLE  8004

#define IDS_WIZ_BROWSE_TITLE    8010

/* Questions */
#define IDS_OVERWRITEFILE_TEXT  9000
#define IDS_PASSWORD_FILE_TEXT  9001
#define IDS_PASSWORD_ZIP_TEXT   9002


/* Context menu / ExplorerCommand strings */
#define IDS_MENUITEM        10039
#define IDS_HELPTEXT        10041
#define IDS_FRIENDLYNAME    10195


#ifndef IDC_STATIC
#define IDC_STATIC -1
#endif
