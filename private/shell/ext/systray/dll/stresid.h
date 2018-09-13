/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       BMRESID.H
*
*  VERSION:     2.0
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        20 Feb 1994
*
*  Resource identifiers for the battery meter.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  20 Feb 1994 TCS Original implementation.  Seperated from RESOURCE.H so that
*                  some documentation could be added without AppStudio screwing
*                  it up later.
*
*******************************************************************************/

#ifndef _INC_STRESID
#define _INC_STRESID

//  Main battery meter dialog box.
#define IDD_BATTERYMETER                100

//  Control identifiers of IDD_BATTERYMETER.
#define IDC_STATIC_FRAME_BATMETER       1000
#define IDC_POWERSTATUSGROUPBOX         1001
#define IDC_ENABLEMETER                 1002
#define IDC_ENABLEMULTI                 1003

// Control identifiers for hotplug
#define IDI_HOTPLUG                     210
#define IDS_HOTPLUGTIP                  211
#define IDS_LOCATION                    212
// 213 available
#define IDS_LOCATION_NOUINUMBER         214
#define IDS_HPLUGMENU_PROPERTIES        215
#define IDS_HPLUGMENU_REMOVE            216
#define IDS_RUNHPLUGPROPERTIES          217
#define IDS_SEPARATOR			218
#define IDS_DISKDRIVE			219
#define IDS_DISKDRIVES			220
#define IDS_DRIVELETTERS		221


//  Control identifiers for Volume
#define IDI_VOLUME                      230
#define IDI_MUTE                        231
#define IDS_MMSYSPROPTITLE              233
#define IDS_MMSYSPROPTAB                234

#define IDS_VOLUME                      252
#define IDS_VOLUMEMENU1                 255
#define IDS_VOLUMEMENU2                 256
#define IDS_VOLUMEAPP                   257
#define IDS_MUTED                       258




// Control identifiers for Sticky Keys

#define IDI_STK000                      300
#define IDI_STK001                      301
#define IDI_STK002                      302
#define IDI_STK003                      303
#define IDI_STK004                      304
#define IDI_STK005                      305
#define IDI_STK006                      306
#define IDI_STK007                      307
#define IDI_STK008                      308
#define IDI_STK009                      309
#define IDI_STK00A                      310
#define IDI_STK00B                      311
#define IDI_STK00C                      312
#define IDI_STK00D                      313
#define IDI_STK00E                      314
#define IDI_STK00F                      315

#define IDI_MKTT                        316
#define IDI_MKTB                        317
#define IDI_MKTG                        318
#define IDI_MKBT                        319
#define IDI_MKBB                        320
#define IDI_MKBG                        321
#define IDI_MKGT                        322
#define IDI_MKGB                        323
#define IDI_MKGG                        324
#define IDI_MKPASS                      325

#define IDI_FILTER                      326
// access strings
#define IDS_STICKYKEYS                  330
#define IDS_MOUSEKEYS                   331
#define IDS_FILTERKEYS                  332

#define IDS_PROPFORPOWER                152
#define IDS_OPEN                        153
#define IDS_RUNPOWERPROPERTIES          157
#define IDS_REMAINING                   158
#define IDS_CHARGING                    159
#define IDS_UNKNOWN                     160
#define IDS_ACPOWER                     161
#define IDS_TIMEREMFORMATHOUR           162
#define IDS_TIMEREMFORMATMIN            163

#define IDI_BATTERYPLUG                 200

// Control identifiers for Fax

// controls
#define IDC_FAX_ANIMATE                 1001
#define IDC_FAX_STATUS                  1002
#define IDC_FAX_ETIME                   1003
#define IDC_FAX_FROMTO                  1004
#define IDC_FAX_DETAILS                 1005
#define IDC_ANSWER_NEXT_CALL            1006
#define IDC_END_FAX_CALL                1007
#define IDC_FAX_DETAILS_LIST            1008

// dialogs
#define IDD_FAX_STATUS                  1101
#define IDD_FAX_ANSWER_CALL             1102

// resources
#define IDR_FAX_IDLE                    1201
#define IDR_FAX_SEND                    1202
#define IDR_FAX_RECEIVE                 1203

// icons
#define IDI_FAX_NORMAL                  1301
#define IDI_FAX_INFO                    1302

// strings
#define IDS_FAX_DIALING                 1401
#define IDS_FAX_SENDING                 1402
#define IDS_FAX_RECEIVING               1403
#define IDS_FAX_COMPLETED               1404
#define IDS_FAX_BUSY                    1405
#define IDS_FAX_NO_ANSWER               1406
#define IDS_FAX_BAD_ADDRESS             1407
#define IDS_FAX_NO_DIAL_TONE            1408
#define IDS_FAX_DISCONNECTED            1409
#define IDS_FAX_FATAL_ERROR_SND         1410
#define IDS_FAX_FATAL_ERROR_RCV         1411
#define IDS_FAX_NOT_FAX_CALL            1412
#define IDS_FAX_CALL_DELAYED            1413
#define IDS_FAX_CALL_BLACKLISTED        1414
#define IDS_FAX_RINGING                 1415
#define IDS_FAX_ABORTING                1416
#define IDS_FAX_MODEM_POWERED_OFF       1417
#define IDS_FAX_IDLE                    1418
#define IDS_FAX_IDLE_RECEIVE            1419
#define IDS_FAX_ANSWERED                1420
#define IDS_FAX_ETIME                   1421
#define IDS_FAX_FROM                    1422
#define IDS_FAX_TO                      1423
#define IDS_FAX_TIMELABEL               1424
#define IDS_FAX_EVENTLABEL              1425
#define IDS_FAX_MENU_CFG                1426
#define IDS_FAX_MENU_QUEUE              1427
#define IDS_FAX_MENU_FOLDER             1428
#define IDS_FAX_RCV_COMPLETE            1429
#define IDS_FAX_DETAILS_EXPAND          1430
#define IDS_FAX_DETAILS_COLLAPSE        1431

#endif // _INC_STRESID
