//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       resource.h
//
//--------------------------------------------------------------------------

#define IDI_HDWWIZICON                  100
#define IDI_BLANK                       101
#define IDB_WATERMARK                   102
#define IDB_BANNER                      103
#define IDH_HDWWIZAPPLET                104

#define IDD_HDWINTRO                    200
#define IDD_HDWTASKS                    201
#define IDD_HDWINVOKETASK               202
#define IDD_HIDDENTASKSDLG              203

#define IDD_ADDDEVICE_PNPENUM           204
#define IDD_ADDDEVICE_PNPFINISH         205
#define IDD_ADDDEVICE_PROBLIST          206
#define IDD_ADDDEVICE_PROBLIST_FINISH   207
#define IDD_ADDDEVICE_ASKDETECT         208
#define IDD_ADDDEVICE_DETECTION         209
#define IDD_ADDDEVICE_DETECTINSTALL     210
#define IDD_ADDDEVICE_DETECTREBOOT      211
#define IDD_ADDDEVICE_SELECTCLASS       212
#define IDD_ADDDEVICE_SELECTDEVICE      213
#define IDD_ADDDEVICE_ANALYZEDEV        214
#define IDD_ADDDEVICE_INSTALLDEV        215
#define IDD_ADDDEVICE_FINISH            216

#define IDD_REMDEVICE_CHOICE            217
#define IDD_REMDEVICE_UNINSTALLLIST     218
#define IDD_REMDEVICE_UNINSTALL_CONFIRM 219
#define IDD_REMDEVICE_UNINSTALL_FINISH  220

#define IDD_WIZARDEXT_PRESELECT         250
#define IDD_WIZARDEXT_SELECT            IDD_DYNAWIZ_SELECTCLASS_PAGE
                   // setupapi contains IDD_DYNAWIZ_SELECTDEV_PAGE
#define IDD_WIZARDEXT_PREANALYZE        IDD_DYNAWIZ_ANALYZEDEV_PAGE
#define IDD_WIZARDEXT_PREANALYZE_END    251
#define IDD_WIZARDEXT_POSTANALYZE       252
#define IDD_WIZARDEXT_POSTANALYZE_END   253
#define IDD_WIZARDEXT_FINISHINSTALL     254
#define IDD_WIZARDEXT_FINISHINSTALL_END 255
#define IDD_WIZARDEXT_UNPLUG            256
#define IDD_WIZARDEXT_UNPLUG_END        257

#define IDD_INSTALLNEWDEVICE            258


#define IDC_HDWNAME                     300
#define IDC_HDWDESC                     301
#define IDC_HDWPROBLIST                 302
#define IDC_HDWPROBTEXT                 303
#define IDC_ERRORTEXT                   304
#define IDC_HDWUNINSTALL                305

#define IDC_HWTASK_ADDNEW               306
#define IDC_HWTASK_REMOVE               307
#define IDC_HWTASK_TROUBLE              309
#define IDC_HDWTASKDESC                 311

#define IDC_HDW_TEXT                    312
#define IDC_FOUNDPNP_TEXT               313
#define IDC_FOUNDPNP_LIST               314
#define IDC_RESTART                     315

#define IDC_ADDDEVICE_ASKDETECT_AUTO        316
#define IDC_ADDDEVICE_ASKDETECT_SPECIFIC    317

#define IDC_HDW_DETWARN_PROGRESSTEXT        318
#define IDC_HDW_DETWARN_PROGRESSBAR         319
#define IDC_HDW_DETWARN_TOTALPROGRESSTEXT   320
#define IDC_HDW_DETWARN_TOTALPROGRESSBAR    321
#define IDC_HDW_INSTALLDET_LISTTITLE        322
#define IDC_HDW_INSTALLDET_LIST             323
#define IDC_HDW_PICKCLASS_HWTYPES           324
#define IDC_HDW_PICKCLASS_CLASSLIST         325
#define IDC_HDW_DESCRIPTION                 326
#define IDC_CLASSICON                       327
#define IDC_HDW_DISPLAYRESOURCE             328
#define IDC_HDW_DISABLEDEVICE               329
#define IDC_CHOICE_UNINSTALL                430
#define IDC_CHOICE_UNPLUG                   431
#define IDC_HDWUNINSTALLLIST                432
#define IDC_SHOW_HIDDEN                     433
#define IDC_UNINSTALL_CONFIRM_YES           434
#define IDC_UNINSTALL_CONFIRM_NO            435
#define IDC_PROBLEM_DESC                    436
#define IDC_HDW_CHANGERESOURCE              437

#define IDS_HDWWIZ                          1000
#define IDS_HDWWIZNAME                      1001
#define IDS_HDWWIZINFO                      1002
#define IDS_HDWWIZTASKS                     1003
#define IDS_HDWWIZTASKSINFO                 1004
#define IDS_ADDDEVICE_PROBLIST              1005
#define IDS_ADDDEVICE_PROBLIST_INFO         1006
#define IDS_HDWWIZ_SELDEVICE                1007
#define IDS_HDWWIZINFO_SELDEVICE            1008
#define IDS_UNKNOWN                         1012
#define IDS_DEVICES                         1013
#define IDS_DEVLOCATION                     1014
#define IDS_STATUS                          1015
#define IDS_UNKNOWNDEVICE                   1016
#define IDS_HDWUNINSTALL_NOPRIVILEGE        1017
#define IDS_HWTASK_TROUBLE                  1018
#define IDS_DEVINSTALLED                    1019
#define IDS_ADDDEVICE_PNPENUMERATE          1020
#define IDS_ADDDEVICE_PNPENUMDONE           1021
#define IDS_ADDDEVICE_PNPENUMREBOOT         1022
#define IDS_ADDDEVICE_PNPENUM               1025
#define IDS_ADDDEVICE_PNPENUM_INFO          1026
#define IDS_ADDDEVICE_ASKDETECT             1027
#define IDS_ADDDEVICE_ASKDETECT_INFO        1028
#define IDS_ADDDEVICE_DETECTION             1029
#define IDS_ADDDEVICE_DETECTION_INFO        1030
#define IDS_ADDDEVICE_DETECTINSTALL         1031
#define IDS_ADDDEVICE_DETECTINSTALL_INFO    1032
#define IDS_DETECTPROGRESS                  1035
#define IDS_DETECTCLASS                     1036
#define IDS_HDW_REBOOTDET                   1039
#define IDS_HDW_NOREBOOTDET                 1040
#define IDS_INSTALL_LEGACY_DEVICE           1041
#define IDS_UNINSTALL_LEGACY_DEVICE         1042
#define IDS_HDW_NONEDET1                    1043
#define IDS_HDW_NONEDET2                    1044
#define IDS_HDW_INSTALLDET1                 1045
#define IDS_HDW_PICKCLASS1                  1047
#define IDS_HDW_DUPLICATE1                  1048
#define IDS_HDW_DUPLICATE2                  1049
#define IDS_HDW_DUPLICATE3                  1050
#define IDS_HDW_ANALYZEPNPDEV1              1051
#define IDS_HDW_ANALYZEPNPDEV2              1052
#define IDS_HDW_ANALYZEERR1                 1053
#define IDS_HDW_ANALYZEERR2                 1054
#define IDS_HDW_ANALYZEERR3                 1055
#define IDS_HDW_STDCFG1                     1056
#define IDS_HDW_STDCFG2                     1057
#define IDS_ADDNEWDEVICE                    1058
#define IDS_HDW_ERRORFIN1                   1059
#define IDS_HDW_ERRORFIN2                   1060
#define IDS_ADDDEVICE_SELECTCLASS           1061
#define IDS_ADDDEVICE_SELECTCLASS_INFO      1062
#define IDS_ADDDEVICE_SELECTDEVICE          1063
#define IDS_ADDDEVICE_SELECTDEVICE_INFO     1064
#define IDS_ADDDEVICE_ANALYZEDEV            1065
#define IDS_ADDDEVICE_ANALYZEDEV_INFO       1066
#define IDS_HDW_RUNNING_TITLE               1067
#define IDS_HDW_RUNNING_MSG                 1068
#define IDS_HDW_ERRORFIN1_PNP               1069
#define IDS_HDW_ERRORFIN2_PNP               1070
#define IDS_HDW_NORMAL_LEGACY_FINISH1       1071
#define IDS_HDW_NORMALFINISH1               1073
#define IDS_INSTALL_PROBLEM                 1074
#define IDS_INSTALL_PROBLEM_PNP             1075
#define IDS_NEEDREBOOT                      1076
#define IDS_ADDDEVICE_INSTALLDEV            1077
#define IDS_ADDDEVICE_INSTALLDEV_INFO       1078
#define IDS_HDW_NONEDEVICES                 1081
#define IDS_REMDEVICE_CHOICE                1084
#define IDS_REMDEVICE_CHOICE_INFO           1085
#define IDS_REMDEVICE_UNINSTALLLIST         1086
#define IDS_REMDEVICE_UNINSTALLLIST_INFO    1087
#define IDS_REMDEVICE_UNINSTALL_CONFIRM     1088
#define IDS_REMDEVICE_UNINSTALL_CONFIRM_INFO    1089
#define IDS_DESCENDANTS_VETO                1094
#define IDS_UNINSTALL_FAILED                1095
#define IDS_UNINSTALL_SUCCESS               1096
#define IDS_UNINSTALL_FAIL                  1097
#define IDS_ADDDEVICE_PNPENUMERROR          1098
#define IDS_UNINSTALL_NEEDREBOOT            1099
#define IDS_INSTALLNEWDEVICE                1100
#define IDS_INSTALLNEWDEVICE_INFO           1101
#define IDS_NEED_FORCED_CONFIG              1102

#define IDC_STATIC                          -1
