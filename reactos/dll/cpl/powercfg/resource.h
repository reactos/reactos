#ifndef __CPL_RESOURCE_H
#define __CPL_RESOURCE_H

/* metrics */
#define PROPSHEETWIDTH  246
#define PROPSHEETHEIGHT 228
#define PROPSHEETPADDING        6
#define SYSTEM_COLUMN   (18 * PROPSHEETPADDING)
#define LABELLINE(x)    (((PROPSHEETPADDING + 2) * x) + (x + 2))
#define ICONSIZE        16

/* ids */
#define IDC_CPLICON_1	                1

#define IDS_PROCENT                     49
#define IDS_SOUND                       50
#define IDS_TEXT                        51
#define IDS_CONFIG1                     53
#define IDS_CONFIG2                     54
#define IDS_SIZEBYTS                    55
#define IDS_SIZEMB                      56

#define IDD_PROPPAGEPOWERSHEMES         70
#define IDD_PROPPAGEALARMS              71
#define IDD_PROPPAGEPOWERMETER          72
#define IDD_PROPPAGEADVANCED            73
#define IDD_PROPPAGEHIBERNATE           74

#define IDS_NOACTION                    100
#define IDS_PowerActionNone1            114
#define IDS_PowerActionUnknown          115
#define IDS_PowerActionSleep            116
#define IDS_PowerActionHibernate        117
#define IDS_PowerActionShutdown         118
#define IDS_PowerActionRestart          119
#define IDS_PowerActionShutdownOff      120
#define IDS_PowerActionWarmEject        121
#define IDS_PowerActionNone2            122
#define IDS_TIMEOUT16                   150
#define IDS_TIMEOUT1                    151
#define IDS_TIMEOUT2                    152
#define IDS_TIMEOUT3                    153
#define IDS_TIMEOUT4                    154
#define IDS_TIMEOUT5                    155
#define IDS_TIMEOUT6                    156
#define IDS_TIMEOUT7                    157
#define IDS_TIMEOUT8                    158
#define IDS_TIMEOUT9                    159
#define IDS_TIMEOUT10                   160
#define IDS_TIMEOUT11                   161
#define IDS_TIMEOUT12                   162
#define IDS_TIMEOUT13                   163
#define IDS_TIMEOUT14                   164
#define IDS_TIMEOUT15                   165

#define IDI_AC                          175
#define IDI_DC                          176
#define IDI_ACDC                        177
#define IDI_SCREEN                      178

#define IDS_CPLNAME_1	                190
#define IDC_ENERGYLIST                  200
#define IDC_GRPDETAIL                   201
#define IDC_SAT                         202
#define IDC_IAC                         203
#define IDC_SAC                         204
#define IDC_IDC                         205
#define IDC_SDC                         206
#define IDC_STANDBYACLIST               207
#define IDC_STANDBYDCLIST               208
#define IDC_MONITORACLIST               209
#define IDC_MONITORDCLIST               210
#define IDC_DISKACLIST                  216
#define IDC_DISKDCLIST                  217
#define IDC_DISK                        218
#define IDC_HYBERNATEACLIST             219
#define IDC_HYBERNATEDCLIST             220
#define IDC_HYBERNATE                   221

#define IDC_STANDBY                     211
#define IDC_MONITOR                     212
#define IDC_GRPPOWERSTATUS              213
#define IDC_MULTIBATTERYDISPLAY         214

#define IDC_ALARMMSG1                   301
#define IDC_ALARMBAR1                   302
#define IDC_ALARMBAR2                   303
#define IDC_ALARMVALUE1                 304
#define IDC_ALARMVALUE2                 305
#define IDC_ALARM1                      306
#define IDC_ALARM2                      307
#define IDC_ALARMAKTION1                320
#define IDC_ALARMPROG1                  322
#define IDC_ALARMMSG2                   324
#define IDC_ALARMAKTION2                323
#define IDC_ALARMPROG2                  325

#define IDC_SYSTRAYBATTERYMETER         400
#define IDC_PASSWORDLOGON               401
#define IDC_VIDEODIMDISPLAY             402
#define IDC_SLIDCLOSE                   403
#define IDC_LIDCLOSE                    404
#define IDC_SPOWERBUTTON                405
#define IDC_POWERBUTTON                 406
#define IDC_SSLEEPBUTTON                407
#define IDC_SLEEPBUTTON                 408

#define IDC_HIBERNATEFILE               500
#define IDC_FREESPACE                   501
#define IDC_SPACEFORHIBERNATEFILE       502
#define IDC_TOLESSFREESPACE             503

#define IDS_ALPERTLOWENERGY						715
#define IDS_ALPERTCRITICLEENERGY				716
#define IDS_CRITCLENERGY						717
#define IDS_LOWENERGY							718
#define IDS_UNKNOWN								719

#define IDS_CPLDESCRIPTION_1	        901

#endif /* __CPL_RESOURCE_H */

/* EOF */
