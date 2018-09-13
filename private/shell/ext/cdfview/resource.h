//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// resource.h 
//
//   Defines for the resources used in cdf view.
//
//   History:
//
//       3/20/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Strings.
//

#define IDS_COLUMN_NAME             0x1000

#define IDS_ERROR_DLG_TEXT          0x1100
#define IDS_ERROR_DLG_TITLE         0x1101
#define IDS_ERROR_NO_CACHE_ENTRY    0x1102

#define IDS_CHANNEL_FOLDER          0x1200
#define IDS_SOFTWAREUPDATE_FOLDER   0x1201

#define IDS_SHARING                 0x1300
#define IDS_RENAME                  0x1301
#define IDS_SENDTO                  0x1302
#define IDS_PROPERTIES              0x1303

#define IDS_OVERWRITE_DLG_TEXT      0x1400
#define IDS_OVERWRITE_DLG_TITLE     0x1401

#define IDS_BROWSERONLY_DLG_TEXT    0x1500
#define IDS_BROWSERONLY_DLG_TITLE   0x1501

#define IDS_INFO_MUST_CONNECT       0x1600
#define IDS_INFO_DLG_TITLE          0x1601

#define IDS_VALUE_UNKNOWN           0x1700

//
// Icons.
//

#define IDI_FIRSTICON           0x2000
#define IDI_CHANNEL             0x2000
#define IDI_STORY               0x2001
#define IDI_OPENSUBCHANNEL      0x2002
#define IDI_CLOSESUBCHANNEL     0x2003
#define IDI_CHANNELFOLDER       0x2004
#define IDI_DESKTOP             0x2005

//
// Menu items.
//

#define IDM_CONTEXTMENU         0x3000
#define IDM_OPEN                0x0001
#define IDM_PROPERTIES          0x0002

#define IDM_SUBSCRIBEDMENU      0x3200
#define IDM_UNSUBSCRIBEDMENU    0x3300
#define IDM_NOSUBSCRIBEMENU     0x3400

#define IDM_UNSUBSCRIBE         0x0001
#define IDM_EDITSUBSCRIPTION    0x0002
#define IDM_UPDATESUBSCRIPTION  0x0003
#define IDM_REFRESHCHANNEL      0x0004
#define IDM_VIEWSOURCE          0x0005
#define IDM_SUBSCRIBE           0x0006


//
// AVI files
//

#define IDA_DOWNLOAD            0x4800

//
// Dialogs
//

#define IDD_CHANNELREFRESH          0x5000
#define IDC_DOWNLOADPROGRESS        0x0100
#define IDC_DOWNLOADMSG             0x0101
#define IDC_DOWNLOADANIMATE         0x0102

#define IDD_CHANNEL_PROP            0x5100
#define IDC_ICONEX2                 0x0103
#define IDC_NAME                    0x0104
#define IDC_URL_TEXT                0x0105
#define IDC_URL                     0x0106
#define IDC_HOTKEY_TEXT             0x0107
#define IDC_HOTKEY                  0x0108

#define IDC_VISITS_TEXT             0x1018
#define IDC_VISITS                  0x1019
#define IDC_MAKE_OFFLINE            0x1020
#define IDC_SUMMARY                 0x1021
#define IDC_LAST_SYNC_TEXT          0x1022
#define IDC_LAST_SYNC               0x1023
#define IDC_DOWNLOAD_SIZE_TEXT      0x1024
#define IDC_DOWNLOAD_SIZE           0x1025
#define IDC_DOWNLOAD_RESULT_TEXT    0x1026
#define IDC_DOWNLOAD_RESULT         0x1027
#define IDC_FREE_SPACE_TEXT         0x1028


//
// HTML files
//

// these pairs must match up (apart from the quotes)
#define IDH_XMLERRORPAGE     xmlerror.htm
#define SZH_XMLERRORPAGE     TEXT("xmlerror.htm")

#define IDH_HRERRORPAGE     hrerror.htm
#define SZH_HRERRORPAGE     TEXT("hrerror.htm")

#define IDH_CACHEERRORPAGE   cacheerr.htm
#define SZH_CACHEERRORPAGE   TEXT("cacheerr.htm")

