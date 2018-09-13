#ifndef I_SDV16RC_H_
#define I_SDV16RC_H_
#pragma INCMSG("--- Beg 'sdv16rc.h'")

// resource IDs stolen from shdocvw for win16 sdvskel
// Numbers converted from hex to decimal.

#define DLG_DOWNLOADPROGRESS    4352 // 0x1100
#define DLG_SAFEOPEN            4416 // 0x1140
#define DLG_UNKNOWNFILE            4424 // 0x1148

#define IDD_ANIMATE             4353 // 0x1101
#define IDD_DOWNLOADICON        4359 // 0x1107
#define IDS_DEFDLGTITLE         790
#define IDA_DOWNLOAD            256 // 0x100
#define IDS_DOWNLOADFAILED              1202
#define IDD_SAVEAS              4358 // 0x1106
#define IDS_DOWNLOADCOMPLETE 731 // Download completed
#define IDS_DOCUMENT    744
#define IDS_ALLFILES                    1200
#define IDD_NAME                4354 // 0x1102
#define IDD_NAMESTATIC          4483 // 0x1183
#define IDS_TITLEPERCENT        761     // Download title with bytes copied
#define IDS_ESTIMATE    738     // Estimated time string for progress (B/sec)
#define IDS_TITLEBYTES          760   // Download title with % loaded
#define IDS_BYTESCOPIED 741     // Progress text when ulMax is 0 (unknown)
#define IDS_TRANSFERRATE                1203
#define IDD_PROBAR              4356 // 0x1104
#define IDD_NOFILESIZE          4361 // 0x1109
#define IDD_TIMEEST             4357 // 0x1105
#define IDD_TRANSFERRATE        4368 // 0x1110
#define IDS_INVALIDURL   727
#define IDS_CANTDOWNLOAD 728
#define IDS_OPENING      737
#define IDS_SAVING       736
#define IDD_OPENIT              4355 // 0x1103
#define IDS_DOWNLOADTOCACHE             1204
#define IDD_DIR                 4370 // 0x1112
#define IDC_SAFEOPEN_ALWAYS     4421 // 0x1145
#define IDS_OPENFROMINTERNET    920
#define IDC_SAFEOPEN_AUTOOPEN   4419 // 0x1143
#define IDS_SAVEFILETODISK      921
#define IDC_SAFEOPEN_AUTOSAVE   4420 // 0x1144
#define IDC_SAFEOPEN_EXPL       4418 // 0x1142
#define IDC_UNKNOWNFILE_ICON       4426 // 0x114a
#define IDC_UNKNOWNFILE_EXPL       4427 // 0x114b
#define IDC_UNKNOWNFILE_AUTOSAVE   4429 // 0x114d

#define IDS_TARGETFILE   730    // String for target file of downloading
#define IDM_MOREINFO                    30
#define IDC_UNKNOWNFILE_AUTOOPEN   4428 // 0x114c

#define IDS_ERR_OLESVR  745     // CoCreateInstance failed.
#define IDS_ERR_LOAD    746
#define IDS_TITLE       723

#define IDB_DOWNLOAD            533 // 0x215

#define IDS_ERR_FILE_NOTFOUND           1300
#define IDS_ERR_URL_NOTFOUND            1301
#define IDS_ERR_RESOURCE_NOTFOUND       1302
#define IDS_ERR_UNKNOWN                 1303
#define IDS_ERR_DOWNLOAD_FAILURE        1304
#define IDS_ERR_UNSUPPORTED_PROTOCOL    1305

#pragma INCMSG("--- End 'sdv16rc.h'")
#else
#pragma INCMSG("*** Dup 'sdv16rc.h'")
#endif
