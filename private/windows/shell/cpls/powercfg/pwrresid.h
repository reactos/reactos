/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       PWRRESID.H
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*******************************************************************************/

#ifndef _INC_PWRRESID
#define _INC_PWRRESID

#define IDC_STATIC                      -1

// Top level dialog box control identifiers.
#define IDD_POWERSCHEME                 100
#define IDD_POWERSCHEME_NOBAT           101
#define IDD_BATMETERCFG                 102
#define IDD_ALARMPOLICY                 103
#define IDD_ADVANCEDPOLICY              104
#define IDD_HIBERNATE                   105
#define IDD_APM                         110
#define IDD_UPS							111

// UPS dialog box ID's 
#define IDD_APCABOUT					112
#define IDD_UPSDETAILS					113
#define IDD_UPSSELECT					114
#define IDD_UPSCUSTOM					115
#define IDD_UPSCONFIGURATION			116

// Advanced dialogs box control identifiers.
#define IDD_ALARMACTIONS                106
#define IDD_ADVPOWERSCHEME              107
#define IDD_ADVPOWERSCHEME_NOBAT        108

// Misc. dialog box control identifiers.
#define IDD_SAVE                        109

//  Control identifiers of IDD_POWERSCHEME and IDD_POWERSCHEME_NOBAT

#define IDC_SCHEMECOMBO                 1000
#define IDC_SAVEAS                      1001
#define IDC_DELETE                      1002
#define IDC_SETTINGSFOR                 1004
#define IDC_STANDBYACCOMBO              1005
#define IDC_STANDBYDCCOMBO              1006
#define IDC_MONITORACCOMBO              1007
#define IDC_MONITORDCCOMBO              1008
#define IDC_GOONSTANDBY                 1009
#define IDC_TURNOFFMONITOR              1010
#define IDC_WHENCOMPUTERIS              1011
#define IDC_PLUGGEDIN                   1012
#define IDC_RUNNINGONBAT                1013
#define IDC_NO_HELP_0                   1014
#define IDC_POWERSCHEMESTEXT            1015
#define IDC_DISKACCOMBO                 1016
#define IDC_DISKDCCOMBO                 1017
#define IDC_TURNOFFHARDDISKS            1018
#define IDC_HIBERACCOMBO                1019
#define IDC_HIBERDCCOMBO                1020
#define IDC_SYSTEMHIBERNATES            1021
#define IDC_NO_HELP_7                   1022

//  Control identifiers of IDD_ALARMPOLICY
#define IDC_CRITACTION                  1100
#define IDC_LOWACTION                   1101
#define IDC_LOALARMSLIDER               1102
#define IDC_CRITALARMSLIDER             1103
#define IDC_LOWALARMLEVEL               1104
#define IDC_CRITALARMLEVEL              1105
#define IDC_LOBATALARMENABLE            1106
#define IDC_CRITBATALARMENABLE          1107
#define IDC_POWERCFGGROUPBOX3           1110
#define IDC_POWERCFGGROUPBOX4           1111
#define IDC_NO_HELP_1                   1112
#define IDC_NO_HELP_2                   1113
#define IDC_NO_HELP_3                   1114
#define IDC_NO_HELP_4                   1115

// Maintain order
#define IDC_LOALARMNOTIFICATION         1120
#define IDC_LOALARMPOWERMODE            1121
#ifdef WINNT
#define IDC_LOALARMPROGRAM              1122
#endif
#define IDC_CRITALARMNOTIFICATION       1123
#define IDC_CRITALARMPOWERMODE          1124
#ifdef WINNT
#define IDC_CRITALARMPROGRAM            1125
#endif

//  Control identifiers of IDD_BATMETERCFG
#define IDC_POWERSTATUSGROUPBOX         1201
#define IDC_SHOWTIME                    1202
#define IDC_ENABLEMULTI                 1204
#define IDC_STATIC_FRAME_BATMETER       1205

//  Control identifiers of IDD_ADVANCEDPOLICY
#define IDC_LIDCLOSETEXT                1300
#define IDC_LIDCLOSEACTION              1301
#define IDC_PWRBUTTONTEXT               1302
#define IDC_PWRBUTACTION                1303
#define IDC_POWERBUTGROUP               1304
#define IDC_ENABLEMETER                 1306
#define IDC_PASSWORD                    1307
#define IDC_NO_HELP_5                   1308
#define IDC_OPTIONSGROUPBOX             1309
#define IDC_SLPBUTTONTEXT               1310
#define IDC_SLPBUTACTION                1311
#define IDC_VIDEODIM                    1312

//  Control identifiers of IDD_HIBERNATE
#define IDC_HIBERNATE                   1400
#define IDC_FREESPACE                   1401
#define IDC_REQUIREDSPACE               1402
#define IDC_NOTENOUGHSPACE              1403
#define IDC_DISKSPACEGROUPBOX           1404
#define IDC_FREESPACETEXT               1405
#define IDC_REQUIREDSPACETEXT           1406
#define IDC_HIBERNATEGROUPBOX           1407
#define IDC_NO_HELP_6                   1408

// Control identifiers of IDD_APM
#define IDC_APMENABLE                   1500


//  Control identifiers of IDD_ALARMACTIONS.

#define IDC_ENABLELOWSTATE              1600
#define IDC_ALARMACTIONPOLICY           1601
#define IDC_ALARMIGNORENONRESP          1602
#define IDC_NOTIFYWITHSOUND             1603
#define IDC_NOTIFYWITHTEXT              1605
#define IDC_POWERCFGGROUPBOX5           1608
#define IDC_POWERCFGGROUPBOX6           1609
#define IDC_POWERCFGGROUPBOX7           1610
#ifdef WINNT
#define IDC_RUNPROGCHECKBOX             1611
#define IDC_RUNPROGWORKITEM             1612
#endif

//  Control identifiers of IDD_SAVE
#define IDC_SAVENAMEEDIT                1700
#define IDC_SAVETEXT                    1701

// Control identifiers of IDD_UPS
// UPS Selection dialog (1800-1819)
#define IDC_VENDOR_TEXT                 1800
#define IDC_VENDOR_LIST                 1801
#define IDC_MODEL_TEXT                  1802
#define IDC_MODEL_LIST                  1803
#define IDC_PORT_TEXT                   1804
#define IDC_PORT_LIST                   1805
#define IDB_SELECT_FINISH               1806
#define IDB_SELECT_NEXT					1807

// UPS Custom pin settings dialog  (1820-1839)
#define IDC_CUSTOM_FRAME                1820
#define IDC_ONBAT_CHECK                 1821
#define IDC_ONBAT_POS                   1822
#define IDC_ONBAT_NEG                   1823
#define IDC_LOWBAT_CHECK                1824
#define IDC_LOWBAT_POS                  1825
#define IDC_LOWBAT_NEG                  1826
#define IDC_TURNOFF_CHECK               1827
#define IDC_TURNOFF_POS                 1828
#define IDC_TURNOFF_NEG                 1829
#define IDB_CUSTOM_BACK                 1830
#define IDB_CUSTOM_FINISH               1831
#define IDC_CUSTOM_CAVEAT               1832

// UPS Configuration dialog  (1840 - 1879)
#define IDC_NOTIFYCHECKBOX              1840
#define IDC_WAITTEXT	                1841
#define IDC_WAITEDITBOX                 1842
#define IDC_WAITSPIN                    1843
#define IDC_REPEATTEXT	                1844
#define IDC_REPEATEDITBOX               1845
#define IDC_REPEATSPIN                  1846

#define IDC_SHUTDOWNGROUPBOX            1847
#define IDC_SHUTDOWNTEXT                1848
#define IDC_SHUTDOWNTIMEREDITBOX        1849
#define IDC_TIMERSPIN                   1850
#define IDC_LOWBATTERYSHUTDOWNTEXT      1851
#define IDC_POWERACTIONTEXT             1852
#define IDC_POWERACTIONCOMBO            1853
#define IDC_RUNTASKCHECKBOX             1854
#define IDC_CONFIGURETASKBUTTON         1855
#define IDC_LOWBATTERYCHECKBOX          1856
#define IDC_SHUTDOWNTIMERCHECKBOX       1857
#define IDC_TASKNAMETEXT                1858
#define IDC_TURNOFFCHECKBOX             1859

// UPS Details dialog (1880 - 1899)
#define IDC_UPS_INFO					1880
#define IDC_NO_DETAILED_INFO            1881
#define IDC_MANUFACTURER_LHS            1882
#define IDC_MANUFACTURER                1883
#define IDC_MODEL_LHS                   1884
#define IDC_MODEL                       1885
#define IDC_SERIAL_NUMBER_LHS           1886
#define IDC_SERIAL_NUMBER               1887
#define IDC_FIRMWARE_REVISION_LHS       1888
#define IDC_FIRMWARE_REVISION           1889


// UPS Page (1900 - 1949)

#define IDC_STATUS_GROUPBOX             1900
#define IDC_POWER_SOURCE_ICON			1901
#define IDC_POWER_SOURCE_LHS            1902
#define IDC_POWER_SOURCE                1903
#define IDC_RUNTIME_REMAINING_LHS       1904
#define IDC_RUNTIME_REMAINING           1905
#define IDC_BATTERY_CAPACITY_LHS        1906
#define IDC_BATTERY_CAPACITY            1907
#define IDC_BATTERY_STATUS_LHS          1908
#define IDC_BATTERY_STATUS              1909

#define IDC_DETAILS_GROUPBOX            1920
#define IDC_VENDOR_NAME_LHS             1921
#define IDC_VENDOR_NAME                 1922
#define IDC_MODEL_TYPE_LHS              1923
#define IDC_MODEL_TYPE                  1924

#define IDC_MESSAGE_ICON				1931
#define IDC_MESSAGE_TEXT                1932
#define IDC_SERVICE_TEXT                1933
#define IDC_SERVICE_PROGRESS            1934
#define IDC_SERVICE_CLOSE               1935

#define IDB_APCLOGO_SMALL               1940
#define IDB_APCLOGO_LARGE               1941
#define IDB_UPS_ICON_BUTTON             1942
#define IDB_INSTALL_UPS                 1943
#define IDB_CONFIGURE_SVC               1944
#define IDC_APC1						1945
#define IDC_APC2						1946
#define IDC_UPS_TITLE                   1947

// UPS About dialog ID's (2000-2010)
#define IDC_APCURL                      2000

// UPS String ID's
#define IDS_APCURL						300
#define IDS_OUTOFWAITRANGE              301
#define IDS_OUTOFREPEATRANGE            302
#define IDS_NOTIFYCAPTION               303
#define IDS_OUTOFSHUTDELAYRANGE         304
#define IDS_SHUTDOWNCAPTION             305
#define IDS_SHUTDOWN_TASKNAME			306

#define IDS_LOW_BATTERY_SHUTDOWN        310
#define IDS_ON_BATTERY_SHUTDOWN         311
#define IDS_NO_SHUTDOWN                 312
#define IDS_UPS_TURNOFF_IMMEDIATELY     313
#define IDS_UPS_TURNOFF_AFTER           314
#define IDS_UPS_NO_TURNOFF              315
#define IDS_DWORD                       316
#define IDS_STRING                      317
#define IDS_RUNTIME_REMAINING           318
#define IDS_CAPACITY					319
#define IDS_CAPACITY_CHARGING			320
#define IDS_UTILITYPOWER_UNKNOWN        321
#define IDS_UTILITYPOWER_ON             322
#define IDS_UTILITYPOWER_OFF            323
#define IDS_BATTERYSTATUS_UNKNOWN       324
#define IDS_BATTERYSTATUS_OK            325
#define IDS_BATTERYSTATUS_BAD           326
#define IDS_URLLINK                     327
#define IDS_NO_UPS_VENDOR               328
#define IDS_OTHER_UPS_VENDOR            329
#define IDS_CUSTOM_UPS_MODEL            330
#define IDS_COM_PORT_PREFIX             331
#define IDS_CUSTOM_CAPTION              332
#define IDS_CAVEAT_TEXT                 333
#define IDS_NO_UPS_INSTALLED			334		//info
#define IDS_UPS_STOPPED                 335		//warning
#define IDS_START_UPS                   336		//progress
#define IDS_STOP_UPS                    337		//progress
#define IDS_PRESS_APPLY                 338		//info
#define IDS_COMM_LOST                   339		//critical


//  String ID's.
#define IDS_APPNAME                     90
#define IDS_INFO                        91
#define IDS_LOWSOUNDEVENT               94
#define IDS_CRITSOUNDEVENT              95
#define IDS_ALARMACTIONS                96
#define IDS_ALARMLEVELFORMAT            97
#define IDS_ALARMSTATUSSOUND            98
#define IDS_ALARMSTATUSTEXT             99
#define IDS_CRITBAT                     102
#define IDS_LOWBAT                      103
#define IDS_SETTINGSFORMAT              104
#define IDS_ADVSETTINGSFORMAT           105
#define IDS_BLANKNAME                   106
#define IDS_CONFIRMDELETECAPTION        107
#define IDS_CONFIRMDELETE               108
#define IDS_SAVESCHEME                  109
#define IDS_BYTES                       110
#define IDS_MBYTES                      111
#define IDS_POWEREDBYUPS                112
#define IDS_NOACTION                    113
#define IDS_BROWSETITLE                 114

#define IDS_UNKNOWN                     150
#define IDS_NONE                        151
#define IDS_STANDBY                     153
#define IDS_HIBERNATE                   154
#define IDS_SHUTDOWN                    155
#define IDS_POWEROFF                    156

#define IDS_NEVER                       160
#define IDS_01_MIN                      161
#define IDS_02_MIN                      162
#define IDS_03_MIN                      163
#define IDS_05_MIN                      164
#define IDS_10_MIN                      165
#define IDS_15_MIN                      166
#define IDS_20_MIN                      167
#define IDS_25_MIN                      168
#define IDS_30_MIN                      169
#define IDS_45_MIN                      170
#define IDS_01_HOUR                     171
#define IDS_02_HOUR                     172
#define IDS_03_HOUR                     173
#define IDS_04_HOUR                     174
#define IDS_05_HOUR                     175
#define IDS_06_HOUR                     176

// Error string ID's
#define IDS_UNKNOWN_ERROR               177
#define IDS_UNABLETOSETHIBER            178
#define IDS_UNABLETOSETPOLICY           179
#define IDS_UNABLETOSETGLOBALPOLICY     180
#define IDS_UNABLETOSETACTIVEPOLICY     181
#define IDS_UNABLETOSETRUNPROG          182

// Icon ID's
#define IDI_PLUG                        200
#define IDI_BATTERY                     201
#define IDI_PWRMNG                      202
#define IDI_HIBERNATE                   203
#define IDI_UPS							204
#define IDI_INFO						205
#define IDI_ALERT						206
#define IDI_CRITICAL					207

// APM ID's
#define IDS_DEVCHANGE_RESTART           220
#define IDS_DEVCHANGE_CAPTION           221

#endif // _INC_PWRRESID
