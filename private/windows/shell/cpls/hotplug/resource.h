//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       resource.h
//
//--------------------------------------------------------------------------

#define IDI_HOTPLUGICON             100
#define IDI_UNDOCKICON              101

#define IDB_HOTPLUGBMP              150
#define IDB_TASKBARBMP              151
#define IDB_HOTPULLBMP              152
#define IDB_UNDOCKBMP               153
#define IDH_HOTPLUGAPPLET           175

#define IDM_DEVICETREE_CONTEXT      200

#define DLG_DEVTREE                 300
#define IDC_DEVICETREE              301

#define IDC_HDWDEVICES              304
#define IDC_STOPDEVICE              305
#define IDC_MACHINENAME             306
#define IDC_VIEWOPTION              307
#define IDC_SYSTRAYOPTION           308
#define IDC_DEVICEDESC              309
#define IDC_UNDOCK_MESSAGE          310

#define IDC_NOHELP1                 500
#define IDC_NOHELP2                 501
#define IDC_NOHELP3                 502
#define IDC_NOHELP4                 503
#define IDC_NOHELP5                 504

#define IDC_INSTRUCTION             310
#define IDC_PROPERTIES              311

#define DLG_CONFIRMREMOVE           320
#define IDC_REMOVELIST              321

#define DLG_SURPRISEWARN            330

#define IDD_DYNAWIZ_DEVTREE         331
#define IDD_DYNAWIZ_CONFIRMREMOVE   332
#define IDD_DYNAWIZ_FINISH          333

#define IDC_CLASSICON               400
#define IDC_DEVICE_DESCRIPTION      401
#define IDC_HOTPLUG_TEXT            402
#define IDC_TASKBAR                 403
#define IDC_HDWNAME                 404
#define IDC_UNDOCK                  405

#define IDC_HOTPULL                 530

#define DLG_REMOVAL_VETOED          600
#define DLG_REMOVAL_COMPLETE        601
#define DLG_UNDOCK_COMPLETE         602
#define DLG_SURPRISEUNDOCK          603

#define IDC_VETOTEXT                650
#define IDC_REMOVALTEXT             651

#define IDS_HOTPLUGNAME             1000
#define IDS_HOTPLUGINFO             1001

#define IDS_UNKNOWN                 1003
#define IDS_PROB_NONE               1004
#define IDS_PROB_UNKNOWN            1005

#define IDS_LOCATION                1006
#define IDS_LOCATION_NOUINUMBER     1009
#define IDS_UI_NUMBER_DESC_FORMAT   1011
#define IDS_AT                      1012

#define IDS_STOP                    1101
#define IDS_PROPERTIES              1102

#define IDS_TASKBARTEXT             1105
#define IDS_INSTRUCTION             1108

#define IDS_CONFIRM_STOP            1110

//
//IDS_VETO_BASE must be the same value as the first veto string
//
#define IDS_VETO_BASE                           1200
#define IDS_VETO_UNKNOWN                        1200
#define IDS_VETO_LEGACYDEVICE                   1201
#define IDS_VETO_PENDINGCLOSE                   1202
#define IDS_VETO_WINDOWSAPP                     1203
#define IDS_VETO_WINDOWSSERVICE                 1204
#define IDS_VETO_OUTSTANDINGOPEN                1205
#define IDS_VETO_DEVICE                         1206
#define IDS_VETO_DRIVER                         1207
#define IDS_VETO_ILLEGALDEVICEREQUEST           1208
#define IDS_VETO_INSUFFICIENTPOWER              1209
#define IDS_VETO_NONDISABLEABLE                 1210
#define IDS_VETO_LEGACYDRIVER                   1211
#define IDS_VETO_INSUFFICIENT_RIGHTS            1212
#define IDS_VETO_UNKNOWNWINDOWSAPP              1299

#define IDS_DOCKVETO_BASE                       1500
#define IDS_DOCKVETO_UNKNOWN                    1500
#define IDS_DOCKVETO_LEGACYDEVICE               1501
#define IDS_DOCKVETO_PENDINGCLOSE               1502
#define IDS_DOCKVETO_WINDOWSAPP                 1503
#define IDS_DOCKVETO_WINDOWSSERVICE             1504
#define IDS_DOCKVETO_OUTSTANDINGOPEN            1505
#define IDS_DOCKVETO_DEVICE                     1506
#define IDS_DOCKVETO_DRIVER                     1507
#define IDS_DOCKVETO_ILLEGALDEVICEREQUEST       1508
#define IDS_DOCKVETO_INSUFFICIENTPOWER          1509
#define IDS_DOCKVETO_NONDISABLEABLE             1510
#define IDS_DOCKVETO_LEGACYDRIVER               1511
#define IDS_DOCKVETO_INSUFFICIENT_RIGHTS        1512
#define IDS_DOCKVETO_WARM_EJECT                 1598
#define IDS_DOCKVETO_UNKNOWNWINDOWSAPP          1599

#define IDS_SLEEPVETO_BASE                      1600
#define IDS_SLEEPVETO_UNKNOWN                   1600
#define IDS_SLEEPVETO_LEGACYDEVICE              1601
#define IDS_SLEEPVETO_PENDINGCLOSE              1602
#define IDS_SLEEPVETO_WINDOWSAPP                1603
#define IDS_SLEEPVETO_WINDOWSSERVICE            1604
#define IDS_SLEEPVETO_OUTSTANDINGOPEN           1605
#define IDS_SLEEPVETO_DEVICE                    1606
#define IDS_SLEEPVETO_DRIVER                    1607
#define IDS_SLEEPVETO_ILLEGALDEVICEREQUEST      1608
#define IDS_SLEEPVETO_INSUFFICIENTPOWER         1609
#define IDS_SLEEPVETO_NONDISABLEABLE            1610
#define IDS_SLEEPVETO_LEGACYDRIVER              1611
#define IDS_SLEEPVETO_INSUFFICIENT_RIGHTS       1612
#define IDS_SLEEPVETO_UNKNOWNWINDOWSAPP         1699

#define IDS_HIBERNATEVETO_BASE                  1700
#define IDS_HIBERNATEVETO_UNKNOWN               1700
#define IDS_HIBERNATEVETO_LEGACYDEVICE          1701
#define IDS_HIBERNATEVETO_PENDINGCLOSE          1702
#define IDS_HIBERNATEVETO_WINDOWSAPP            1703
#define IDS_HIBERNATEVETO_WINDOWSSERVICE        1704
#define IDS_HIBERNATEVETO_OUTSTANDINGOPEN       1705
#define IDS_HIBERNATEVETO_DEVICE                1706
#define IDS_HIBERNATEVETO_DRIVER                1707
#define IDS_HIBERNATEVETO_ILLEGALDEVICEREQUEST  1708
#define IDS_HIBERNATEVETO_INSUFFICIENTPOWER     1709
#define IDS_HIBERNATEVETO_NONDISABLEABLE        1710
#define IDS_HIBERNATEVETO_LEGACYDRIVER          1711
#define IDS_HIBERNATEVETO_INSUFFICIENT_RIGHTS   1712
#define IDS_HIBERNATEVETO_UNKNOWNWINDOWSAPP     1799

#define IDS_HOTPLUGWIZ_DEVTREE          2250
#define IDS_HOTPLUGWIZ_DEVTREE_INFO     2251
#define IDS_HOTPLUGWIZ_CONFIRMSTOP      2252
#define IDS_HOTPLUGWIZ_CONFIRMSTOP_INFO 2253
#define IDS_VETOED_EJECT_TITLE          2254
#define IDS_DRIVELETTERS                2255
#define IDS_DISKDRIVE                   2256
#define IDS_VETOED_REMOVAL_TITLE        2257
#define IDS_VETOED_UNDOCK_TITLE         2258
#define IDS_UNDOCK_COMPLETE_TEXT        2259
#define IDS_REMOVAL_COMPLETE_TEXT       2260
#define IDS_REMOVAL_COMPLETE_TITLE      2261
#define IDS_UNDOCK_COMPLETE_TITLE       2262
#define IDS_UNSAFE_UNDOCK               2263
#define IDS_VETOED_STANDBY_TITLE        2264
#define IDS_VETOED_HIBERNATION_TITLE    2265


#define IDC_STATIC                  -1
