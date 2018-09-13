#define     IDS_MICROSOFT         1
#define     IDS_UNKNOWN_PARENS    2
#define     IDS_LOCATEFILE        3
#define     IDS_OUTOFMEMORY       4
#define     IDS_ADDITIONALMODELS  5

//
// Standard class icons
//
#define     ICON_DISPLAY          1
#define     ICON_MOUSE            2
#define     ICON_KEYBOARD         3
#define     ICON_PRINTER          4

#define     ICON_NET              5
#define     ICON_NETTRANS         6
#define     ICON_NETCLIENT        7
#define     ICON_NETSERVICE       8

#define     ICON_CONTROLLER       9

#define     ICON_SCSI            10
#define     ICON_PCCARD          11

#define     ICON_UNKNOWN         18
#define     ICON_DEFAULT         19

#define     ICON_USB             20
#define     ICON_1394            21
#define     ICON_GPS             22
#define     ICON_PORT            23
#define     ICON_HID             24
#define     ICON_SMARTCARDREADER 25
#define     ICON_MULTIPORTSERIAL 26
#define     ICON_SYSTEM          27

//
// AVIs
//
#define     IDA_FILECOPY       30
#define     IDA_FILEDEL        31
#define     IDA_ANIMATION      32

//
// Icons for various dialogs.
//
#define     ICON_FLOPPY        50
#define     ICON_CD            51
#define     ICON_NETWORK       52
#define     ICON_HARD          53
#define     ICON_SETUP         54
#define     ICON_EBD          105

//
// Bitmaps.
//
#define     BMP_DRIVERTYPES  1201
#define     BMP_CRYPTO       1202

//
// Files needed strings in disk prompt/file error dialog
//
#define     IDS_FILESNEEDED     100
#define     IDS_FILESNEEDED2    101
#define     IDS_COPYFROM        102
#define     IDS_COPYFROMOEM     103
#define     IDS_DISKPROMPT1     104
#define     IDS_DISKPROMPT2     105
#define     IDS_DISKPROMPTOEM   106
#define     IDS_PROMPTACTION1   107
#define     IDS_PROMPTACTION2   108
#define     IDS_PROMPTTITLE     109

#define     IDS_COPYERROR       110
#define     IDS_FILEERRCOPY     111

#define     IDS_SURESKIP        112
#define     IDS_SURECANCEL      113

#define     IDS_COPYERROROEM    114
#define     IDS_COPYERROR1      115
#define     IDS_COPYERROR2      116

#define     IDS_WARNING         117

#define     IDS_ERRORDETAILS    120
#define     IDS_ERROR           121

#define     IDS_RENAMEERROR     122
#define     IDS_DELETEERROR     123
#define     IDS_BACKUPERROR     124

#define     IDS_CANCELALL       125
#define     IDS_RETRY           126

//
// Strings used in progress dialog.
//
#define     IDS_FILEOP_BACKUP   142
#define     IDS_FILEOP_FROM     143
#define     IDS_FILEOP_TO       144
#define     IDS_FILEOP_FILE     145

#define     IDS_COPY_CAPTION1   146
#define     IDS_COPY_CAPTION2   147
#define     IDS_RENAME_CAPTION1 148
#define     IDS_RENAME_CAPTION2 149
#define     IDS_DELETE_CAPTION1 150
#define     IDS_DELETE_CAPTION2 151
#define     IDS_BACKUP_CAPTION1 152
#define     IDS_BACKUP_CAPTION2 153

#define     IDS_CANCELFILEOPS   155

//
// Strings used in Add New Device Wizard
//
#define IDS_OEMTITLE            309
#define IDS_NDW_PICKDEV1        430
#define IDS_SELECT_DEVICE       704
#define IDS_NDWSEL_MODELSLABEL 2011

//
// Strings used in Select Device dialog
// (setupx ID + 100 to avoid overlap)
//
#define IDS_INSTALLSTR0             200
#define IDS_INSTALLSTR1             201
#define IDS_INSTALLCLASS            205
#define IDS_INSTALLOEM              206
#define IDS_INSTALLOEM1             212

//
// Other device installer strings
//
#define IDS_DEVICEINSTALLER    4206

//
// String ID used for localized language name used in legacy INFs.
//
#define IDS_LEGACYINFLANG       600

//
// Strings for resource selection dialogs
//
#define IDS_RESOURCETYPE                 1000
#define IDS_RESOURCESETTING              1001
#define IDS_DEVRES_NO_CHANGE_MF          1002
#define IDS_BASICCONFIG                  1003
#define IDS_RESTYPE_FULL                 1004
#define IDS_MEMORY_FULL                  1005
#define IDS_IO_FULL                      1006
#define IDS_DMA_FULL                     1007
#define IDS_IRQ_FULL                     1008
#define IDS_IRQ_FULL_LC                  1009
#define IDS_DMA_FULL_LC                  1010
#define IDS_MEMORY_FULL_LC               1011
#define IDS_IO_FULL_LC                   1012
#define IDS_RESOURCE_BASE                1100
#define IDS_MEMORY                       1101
#define IDS_IO                           1102
#define IDS_DMA                          1103
#define IDS_IRQ                          1104
#define IDS_OWNER                        1105
#define IDS_UNKNOWN                      1107

#define IDS_FORCEDCONFIG_PARTIAL         1020

#define IDS_DEVRES_NOALLOC_DISABLED      1022
#define IDS_DEVRES_NOALLOC_PROBLEM       1023
#define IDS_DEVRES_NORMAL_CONFLICT       1024
#define IDS_DEVRES_NOMATCHINGLC          1025
#define IDS_DEVRES_NOMODIFYTITLE         1026
#define IDS_EDITRES_RANGEINSTR1          1027
#define IDS_EDITRES_RANGEINSTR2          1028
#define IDS_EDITRES_ENTRYERROR           1029
#define IDS_EDITRES_VALIDATEERROR1       1030
#define IDS_EDITRES_VALIDATEERROR2       1031
#define IDS_EDITRES_VALIDATEERROR3       1032
#define IDS_ERROR_BADMEMTEXT             1033
#define IDS_ERROR_BADIOTEXT              1034
#define IDS_ERROR_BADDMATEXT             1035
#define IDS_ERROR_BADIRQTEXT             1036
#define IDS_EDITRES_CONFLICTWARNMSG      1037
#define IDS_EDITRES_CONFLICTWARNTITLE    1038
#define IDS_DEVRES_NOCONFLICTINFO        1049
#define IDS_DEVRES_NOMODIFYALL           1050
#define IDS_DEVRES_NOMODIFYSINGLE        1051
#define IDS_EDITRES_SINGLEINSTR1         1052
#define IDS_EDITRES_SINGLEINSTR2         1053
#define IDS_EDITRES_TITLE                1054
#define IDS_EDITRES_UNKNOWNCONFLICT      1055
#define IDS_EDITRES_UNKNOWNCONFLICTINGDEVS 1056
#define IDS_EDITRES_NOCONFLICT           1057
#define IDS_EDITRES_NOCONFLICTINGDEVS    1058
#define IDS_DEVRES_NOMODIFYREMOTE        1059
#define IDS_MAKE_FORCED_TITLE            1060
#define IDS_FORCEDCONFIG_WARN1           1061
#define IDS_FORCEDCONFIG_WARN2           1062
#define IDS_FORCEDCONFIG_WARN3           1063
#define IDS_FORCEDCONFIG_WARN4           1064
#define IDS_EDITRES_DEVCONFLICT          1065
#define IDS_CONFLICT_FMT                 1066
#define IDS_DEVRES_NOCONFLICTDEVS        1067
#define IDS_DEVRES_NO_RESOURCES          1068
#define IDS_DEVRES_NOMODIFYSELECT        1069

#define IDS_CONFLICT_UNAVAILABLE         1071
#define IDS_CONFLICT_GENERALERROR        1072
#define IDS_DEVNAME_UNK                  1073
#define IDS_GENERIC_DEVNAME              1074
#define IDS_EDITRES_RESERVED             1075
#define IDS_EDITRES_RESERVEDRANGE        1076
#define IDS_CURRENTCONFIG                1077

#define IDS_LOGSEVINFORMATION            3001
#define IDS_LOGSEVWARNING                3002
#define IDS_LOGSEVERROR                  3003
#define IDS_LOGSEVFATALERROR             3004

#define IDS_CERTIFICATION_TITLE          5308
#define IDS_CERT_BAD_FILE                5316
#define IDS_CERT_BAD_CATALOG             5317
#define IDS_CERT_BAD_INF                 5318
#define IDS_UNKNOWN_SOFTWARE             5319
#define IDS_UNKNOWN_DRIVER               5320

#define IDS_DRIVER_UPDATE_TITLE          5330
#define IDS_DRIVER_NOMATCH1              5331

#define IDS_DRIVERCACHE_DESC             5332

#define IDS_NDW_NO_DRIVERS               5333
#define IDS_NDW_RETRIEVING_LIST          5334
#define IDS_NDW_NODRIVERS_WARNING        5335

#define IDS_DRIVER_NOMATCH2              5336
#define IDS_DRIVER_NOMATCH3              5337

#define IDS_NDW_SELECTDEVICE             5340
#define IDS_NDW_SELECTDEVICE_INFO        5341

#define IDS_VERSION                      5342

//
// Include dialogs header files also
//
#include "prompt.h"

