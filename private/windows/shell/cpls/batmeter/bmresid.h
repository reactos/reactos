/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       BMRESID.H
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*
*  Resource identifiers for the battery meter helper DLL.
*
*******************************************************************************/

#define IDC_STATIC                      -1

//  Dialog box control identifiers.
#define IDD_BATMETER                    100
#define IDD_BATDETAIL                   126
#define IDD_MOREINFO                    127

//  Control identifiers of IDD_BATMETER
#define IDC_BATTERYLEVEL                1001
#define IDC_REMAINING                   1002
#define IDC_POWERSTATUSICON             1003
#define IDC_POWERSTATUSBAR              1004
#define IDC_BARPERCENT                  1005
#define IDC_CHARGING                    1006
#define IDC_MOREINFO                    1007
#define IDC_BATNUM0                     1008
#define IDC_TOTALTIME                   1067
#define IDC_TIMEREMAINING               1068
#define IDC_CURRENTPOWERSOURCE          1069
#define IDC_TOTALBATPWRREMAINING        1070

// next eight must be consecutive...
#define IDC_POWERSTATUSICON1            1010
#define IDC_POWERSTATUSICON2            1011
#define IDC_POWERSTATUSICON3            1012
#define IDC_POWERSTATUSICON4            1013
#define IDC_POWERSTATUSICON5            1014
#define IDC_POWERSTATUSICON6            1015
#define IDC_POWERSTATUSICON7            1016
#define IDC_POWERSTATUSICON8            1017

// next eight must be consecutive...
#define IDC_REMAINING1                  1020
#define IDC_REMAINING2                  1021
#define IDC_REMAINING3                  1022
#define IDC_REMAINING4                  1023
#define IDC_REMAINING5                  1024
#define IDC_REMAINING6                  1025
#define IDC_REMAINING7                  1026
#define IDC_REMAINING8                  1027

// next eight must be consecutive...
#define IDC_STATUS1                     1030
#define IDC_STATUS2                     1031
#define IDC_STATUS3                     1032
#define IDC_STATUS4                     1033
#define IDC_STATUS5                     1034
#define IDC_STATUS6                     1035
#define IDC_STATUS7                     1036
#define IDC_STATUS8                     1037

// next eight must be consecutive...
#define IDC_BATNUM1                     1040
#define IDC_BATNUM2                     1041
#define IDC_BATNUM3                     1042
#define IDC_BATNUM4                     1043
#define IDC_BATNUM5                     1044
#define IDC_BATNUM6                     1045
#define IDC_BATNUM7                     1046
#define IDC_BATNUM8                     1047


//  Control identifiers of IDD_BATDETAIL
#define IDC_BAT_NUM_GROUP               1100
#define IDC_STATE                       1101
#define IDC_CHEM                        1102
#define IDC_DEVNAME                     1103
#define IDC_BATMANDATE                  1104
#define IDC_BATID                       1105
#define IDC_BATMANNAME                  1106
#define IDC_REFRESH                     1107
#define IDC_BATMETERGROUPBOX            1108
#define IDC_BATMETERGROUPBOX1           1109
#define IDC_BATTERYNAME                 1110
#define IDC_UNIQUEID                    1111
#define IDC_MANUFACTURE                 1112
#define IDC_DATEMANUFACTURED            1113
#define IDC_CHEMISTRY                   1114
#define IDC_POWERSTATE                  1115

// String identifiers of IDD_BATMETER.
#define IDS_ACLINEONLINE                        100
#define IDS_BATTERYLEVELFORMAT                  101
#define IDS_UNKNOWN                             102
#define IDS_PERCENTREMAININGFORMAT              104
#define IDS_TIMEREMFORMATHOUR                   105
#define IDS_TIMEREMFORMATMIN                    106
#define IDS_BATTERIES                           109
#define IDS_NOT_PRESENT                         110
#define IDS_BATTCHARGING                        111
#define IDS_BATNUM                              112
#define IDS_BATTERYNUMDETAILS                   113
#define IDS_BATTERY_POWER_ON_LINE               114
#define IDS_BATTERY_DISCHARGING                 115
#define IDS_BATTERY_CHARGING                    116
#define IDS_BATTERY_CRITICAL                    117

// Image identifiers for IDB_BATTS, the IDI_* values MUST be sequential
// and in this order. Images are contained in the IDB_BATTS resource.
#define IDI_BATFULL     200
#define IDI_BATHALF     201
#define IDI_BATLOW      202
#define IDI_BATDEAD     203
#define IDI_UNKNOWN     204
#define IDI_BATGONE     205
#define IDI_PLUG        206
#define IDI_CHARGE      207
#define IDI_BATTPLUG    208
#define IDI_BATTERY     209

// Bitmap identifiers of IDD_BATMETER.
#define IDB_BATTS                       300
#define IDB_BATTS16                     301

// Definitions for image list.
#define FIRST_ICON_IMAGE        IDI_BATFULL

