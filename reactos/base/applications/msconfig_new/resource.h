#pragma once

#define IDC_STATIC                      -1

#define IDS_YES                         100
#define IDS_NO                          101
#define IDS_UNKNOWN                     102

/* Page & dialog IDs */
#define IDD_TOOLS_PAGE                  200
#define IDD_SERVICES_PAGE               201
#define IDD_GENERAL_PAGE                202
#define IDD_STARTUP_PAGE                203
#define IDD_FREELDR_PAGE                204
#define IDD_SYSTEM_PAGE                 205
#define IDD_FREELDR_ADVANCED_PAGE       206
#define IDD_FILE_EXTRACT_DIALOG         207
#define IDD_REQUIRED_SERVICES_DISABLING_DIALOG 208

/* General page controls */
#define IDC_RB_NORMAL_STARTUP           1000
#define IDC_RB_DIAGNOSTIC_STARTUP       1001
#define IDC_RB_SELECTIVE_STARTUP        1002
#define IDC_CBX_SYSTEM_INI              1003
#define IDC_CBX_WIN_INI                 1004
#define IDC_CBX_LOAD_SYSTEM_SERVICES    1005
#define IDC_CBX_LOAD_STARTUP_ITEMS      1006
#define IDC_CBX_USE_ORIGINAL_BOOTCFG    1007
#define IDC_BTN_SYSTEM_RESTORE_START    1008
#define IDC_BTN_FILE_EXTRACTION         1009

/* System page controls */
#define IDS_TAB_SYSTEM                  2000
#define IDS_TAB_WIN                     2001
#define IDC_BTN_SYSTEM_UP               1217
#define IDC_BTN_SYSTEM_DOWN             1218
#define IDC_BTN_SYSTEM_ENABLE           1219
#define IDC_BTN_SYSTEM_DISABLE          1220
#define IDC_BTN_SYSTEM_FIND             1221
#define IDC_BTN_SYSTEM_NEW              1222
#define IDC_BTN_SYSTEM_EDIT             1223
#define IDC_SYSTEM_TREE                 1224
#define IDC_BTN_SYSTEM_ENABLE_ALL       1225
#define IDC_BTN_SYSTEM_DISABLE_ALL      1226
#define IDC_BTN_SYSTEM_DELETE           1227

/* FreeLdr page controls */
#define IDS_TAB_FREELDR                 2002
#define IDS_TAB_BOOT                    2003
#define IDC_LIST_BOX                    1010
#define IDC_BTN_CHECK_BOOT_PATH         1011
#define IDC_BTN_SET_DEFAULT_BOOT        1012
#define IDC_BTN_MOVE_UP_BOOT_OPTION     1013
#define IDC_BTN_MOVE_DOWN_BOOT_OPTION   1014
#define IDC_BTN_DELETE                  1015
#define IDC_CBX_SAFE_BOOT               1016
#define IDC_CBX_NO_GUI_BOOT             1017
#define IDC_CBX_BOOT_LOG                1018
#define IDC_CBX_BASE_VIDEO              1019
#define IDC_CBX_SOS                     1020
#define IDC_BTN_ADVANCED_OPTIONS        1021
#define IDC_TXT_BOOT_TIMEOUT            1022
#define IDC_RADIO1                      1023 // FIXME!
#define IDC_RADIO2                      1024 // FIXME!
#define IDC_RADIO3                      1025 // FIXME!
#define IDC_RADIO4                      1026 // FIXME!

/* Services page controls */
#define IDC_SERVICES_LIST               1027
#define IDC_BTN_SERVICES_ACTIVATE       1028
#define IDC_BTN_SERVICES_DEACTIVATE     1029
#define IDC_CBX_SERVICES_MASK_PROPRIETARY_SVCS 1030
#define IDC_STATIC_SERVICES_WARNING     1031

/* Startup page controls */
#define IDC_STARTUP_LIST                1032
#define IDC_BTN_STARTUP_ACTIVATE        1033
#define IDC_BTN_STARTUP_DEACTIVATE      1034

/* Tools page controls */
#define IDC_TOOLS_LIST                  1035
#define IDC_TOOLS_CMDLINE               1036
#define IDC_BTN_RUN                     1037
#define IDC_CBX_TOOLS_ADVOPT            1038

#define IDS_TOOLS_COLUMN_NAME           2020
#define IDS_TOOLS_COLUMN_DESCR          2021
#define IDS_TOOLS_COLUMN_STANDARD       2022

/* File extract dialog */
#define IDC_BTN_BROWSE_ALL_FILES        1039
#define IDC_BTN_BROWSE_CAB_FILES        1040
#define IDC_BTN_BROWSE_DIRS             1041
#define IDC_TXT_FILE_TO_RESTORE         1042
#define IDC_DRP_CAB_FILE                1043
#define IDC_DRP_DEST_DIR                1044

/* Essential services warning dialog */
#define IDC_CBX_REQSVCSDIS_NO_MSG_ANYMORE 1045
#define IDC_STATIC_REQSVCSDIS_INFO      1046


#define IDC_CBX_MAX_MEM               1200
#define IDC_TXT_MAX_MEM               1201
#define IDC_SCR_MAX_MEM               1202
#define IDC_CBX_NUM_PROC              1203
#define IDC_DRP_NUM_PROC              1204
#define IDC_CBX_PCI_LOCK              1205
#define IDC_CBX_PROFILE               1206
#define IDC_CBX_IRQ                   1207
#define IDC_TXT_IRQ                   1208
#define IDC_CBX_DEBUG                 1209
#define IDC_CBX_DEBUG_PORT            1210
#define IDC_DRP_DEBUG_PORT            1211
#define IDC_CBX_BAUD_RATE             1212
#define IDC_DRP_DRP_BAUD_RATE         1213
#define IDC_CBX_CHANNEL               1214
#define IDC_TXT_CHANNEL               1215
#define IDC_SCR_CHANNEL               1216

#define IDS_TAB_STARTUP  2023

#define IDS_SERVICES_COLUMN_SERVICE 2024
#define IDS_SERVICES_COLUMN_REQ     2025
#define IDS_SERVICES_COLUMN_VENDOR  2026
#define IDS_SERVICES_COLUMN_STATUS  2027
#define IDS_SERVICES_COLUMN_DATEDISABLED 2028

#define IDS_SERVICES_STATUS_STOPPED 2029
#define IDS_SERVICES_STATUS_RUNNING 2030


#define IDS_STARTUP_COLUMN_ELEMENT  2031
#define IDS_STARTUP_COLUMN_CMD      2032
#define IDS_STARTUP_COLUMN_PATH     2033


#define IDS_MSCONFIG    3000
#define IDS_MSCONFIG_2  3001

#define IDI_APPICON     3010
#define IDR_MSCONFIG    3020

#define IDM_ABOUT       3030
#define IDS_ABOUT       3031
#define IDD_ABOUTBOX    3032
