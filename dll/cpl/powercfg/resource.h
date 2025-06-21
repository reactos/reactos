#pragma once

/* IDs */

/* Set default to IDI_AC */
#define IDC_CPLICON_1 IDI_AC

#define IDS_PERCENT  49
#define IDS_SOUND    50
#define IDS_TEXT     51
#define IDS_CONFIG1  53
#define IDS_CONFIG2  54

#define IDD_POWERSCHEMESPAGE_ACDC 70
#define IDD_POWERSCHEMESPAGE_AC   71
#define IDD_PROPPAGEALARMS        72
#define IDD_PROPPAGEPOWERMETER    73
#define IDD_PROPPAGEADVANCED      74
#define IDD_PROPPAGEHIBERNATE     75
#define IDD_SAVEPOWERSCHEME       76
#define IDD_POWERMETERDETAILS     77

#define IDS_NOACTION               100
#define IDS_PowerActionNone1       114
#define IDS_PowerActionUnknown     115
#define IDS_PowerActionSleep       116
#define IDS_PowerActionHibernate   117
#define IDS_PowerActionShutdown    118
#define IDS_PowerActionRestart     119
#define IDS_PowerActionShutdownOff 120
#define IDS_PowerActionWarmEject   121
#define IDS_PowerActionNone2       122
#define IDS_TIMEOUT16              150
#define IDS_TIMEOUT1               151
#define IDS_TIMEOUT2               152
#define IDS_TIMEOUT3               153
#define IDS_TIMEOUT4               154
#define IDS_TIMEOUT5               155
#define IDS_TIMEOUT6               156
#define IDS_TIMEOUT7               157
#define IDS_TIMEOUT8               158
#define IDS_TIMEOUT9               159
#define IDS_TIMEOUT10              160
#define IDS_TIMEOUT11              161
#define IDS_TIMEOUT12              162
#define IDS_TIMEOUT13              163
#define IDS_TIMEOUT14              164
#define IDS_TIMEOUT15              165
#define IDS_DEL_SCHEME             166
#define IDS_DEL_SCHEME_TITLE       167
#define IDS_DEL_SCHEME_ERROR       168
#define IDS_OFFLINE                169
#define IDS_CHARGING               170
#define IDS_ONLINE                 171
#define IDS_DETAILEDBATTERY        172
#define IDC_BATTERYNAME            173
#define IDC_BATTERYUNIQUEID        174
#define IDC_BATTERYCHEMISTRY       175
#define IDC_BATTERYPOWERSTATE      176
#define IDC_BATTERYMANUFACTURER    177
#define IDS_DISCHARGING            178

#define IDI_AC        175
#define IDI_DC        176
#define IDI_ACDC      177
#define IDI_SCREEN    178
#define IDI_HIBERNATE 179

#define IDS_CPLNAME_1       190
#define IDC_ENERGYLIST      200
#define IDC_GRPDETAIL       201
#define IDC_SAT             202
#define IDC_IAC             203
#define IDC_SAC             204
#define IDC_IDC             205
#define IDC_SDC             206
#define IDC_STANDBYACLIST   207
#define IDC_STANDBYDCLIST   208
#define IDC_MONITORACLIST   209
#define IDC_MONITORDCLIST   210
#define IDC_DISKACLIST      216
#define IDC_DISKDCLIST      217
#define IDC_DISK            218
#define IDC_HIBERNATEACLIST 219
#define IDC_HIBERNATEDCLIST 220
#define IDC_HIBERNATE       221

#define IDC_STANDBY             211
#define IDC_MONITOR             212
#define IDC_GRPPOWERSTATUS      213
#define IDC_MULTIBATTERYDISPLAY 214

#define IDC_ALARMMSG1    301
#define IDC_ALARMBAR1    302
#define IDC_ALARMBAR2    303
#define IDC_ALARMVALUE1  304
#define IDC_ALARMVALUE2  305
#define IDC_ALARM1       306
#define IDC_ALARM2       307
#define IDC_ALARMAKTION1 320
#define IDC_ALARMPROG1   322
#define IDC_ALARMMSG2    324
#define IDC_ALARMAKTION2 323
#define IDC_ALARMPROG2   325

#define IDC_SYSTRAYBATTERYMETER 400
#define IDC_PASSWORDLOGON       401
#define IDC_VIDEODIMDISPLAY     402
#define IDC_SLIDCLOSE           403
#define IDC_LIDCLOSE            404
#define IDC_SPOWERBUTTON        405
#define IDC_POWERBUTTON         406
#define IDC_SSLEEPBUTTON        407
#define IDC_SLEEPBUTTON         408

#define IDC_HIBERNATEFILE         500
#define IDC_FREESPACE             501
#define IDC_SPACEFORHIBERNATEFILE 502
#define IDC_TOLESSFREESPACE       503

#define IDC_DELETE_BTN 504
#define IDC_SAVEAS_BTN 505

#define IDC_SCHEMENAME           620

#define IDS_ALPERTLOWENERGY      715
#define IDS_ALPERTCRITICLEENERGY 716
#define IDS_CRITCLENERGY         717
#define IDS_LOWENERGY            718
#define IDS_UNKNOWN              719

#define IDC_SHOWDETAILS          800
#define IDC_POWERSOURCE          801
#define IDC_POWERSTATUS          802
#define IDC_REFRESH              803

#define IDC_BATTERY0             810
#define IDC_BATTERY1             (IDC_BATTERY0 + 1)
#define IDC_BATTERY2             (IDC_BATTERY0 + 2)
#define IDC_BATTERY3             (IDC_BATTERY0 + 3)
#define IDC_BATTERY4             (IDC_BATTERY0 + 4)
#define IDC_BATTERY5             (IDC_BATTERY0 + 5)
#define IDC_BATTERY6             (IDC_BATTERY0 + 6)
#define IDC_BATTERY7             (IDC_BATTERY0 + 7)
#define IDI_BATTERYDETAIL0       820
#define IDI_BATTERYDETAIL1       (IDI_BATTERYDETAIL0 + 1)
#define IDI_BATTERYDETAIL2       (IDI_BATTERYDETAIL0 + 2)
#define IDI_BATTERYDETAIL3       (IDI_BATTERYDETAIL0 + 3)
#define IDI_BATTERYDETAIL4       (IDI_BATTERYDETAIL0 + 4)
#define IDI_BATTERYDETAIL5       (IDI_BATTERYDETAIL0 + 5)
#define IDI_BATTERYDETAIL6       (IDI_BATTERYDETAIL0 + 6)
#define IDI_BATTERYDETAIL7       (IDI_BATTERYDETAIL0 + 7)
#define IDC_BATTERYPERCENT0      830
#define IDC_BATTERYPERCENT1      (IDC_BATTERYPERCENT0 + 1)
#define IDC_BATTERYPERCENT2      (IDC_BATTERYPERCENT0 + 2)
#define IDC_BATTERYPERCENT3      (IDC_BATTERYPERCENT0 + 3)
#define IDC_BATTERYPERCENT4      (IDC_BATTERYPERCENT0 + 4)
#define IDC_BATTERYPERCENT5      (IDC_BATTERYPERCENT0 + 5)
#define IDC_BATTERYPERCENT6      (IDC_BATTERYPERCENT0 + 6)
#define IDC_BATTERYPERCENT7      (IDC_BATTERYPERCENT0 + 7)
#define IDC_BATTERYCHARGING0     840
#define IDC_BATTERYCHARGING1     (IDC_BATTERYCHARGING0 + 1)
#define IDC_BATTERYCHARGING2     (IDC_BATTERYCHARGING0 + 2)
#define IDC_BATTERYCHARGING3     (IDC_BATTERYCHARGING0 + 3)
#define IDC_BATTERYCHARGING4     (IDC_BATTERYCHARGING0 + 4)
#define IDC_BATTERYCHARGING5     (IDC_BATTERYCHARGING0 + 5)
#define IDC_BATTERYCHARGING6     (IDC_BATTERYCHARGING0 + 6)
#define IDC_BATTERYCHARGING7     (IDC_BATTERYCHARGING0 + 7)

#define IDS_CPLDESCRIPTION_1 901
