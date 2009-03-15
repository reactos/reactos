/*
 * Copyright (C) 2004 Filip Navara
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef RESOURCE_H
#define RESOURCE_H

#define IDB_WATERMARK                    100
#define IDB_HEADER                       101

#define IDC_STATIC                        -1

#define IDD_WELCOMEPAGE                 1000
#define IDC_WELCOMETITLE                1001

#define IDD_ACKPAGE                     1010
#define IDC_PROJECTS                    1011
#define IDC_VIEWGPL                     1012

#define IDD_OWNERPAGE                   1020
#define IDC_OWNERNAME                   1021
#define IDC_OWNERORGANIZATION           1022

#define IDD_COMPUTERPAGE                1030
#define IDC_COMPUTERNAME                1031
#define IDC_ADMINPASSWORD1              1032
#define IDC_ADMINPASSWORD2              1033

#define IDD_LOCALEPAGE                  1040
#define IDC_LOCALETEXT                  1041
#define IDC_CUSTOMLOCALE                1042
#define IDC_LAYOUTTEXT                  1043
#define IDC_CUSTOMLAYOUT                1044

#define IDD_DATETIMEPAGE                1050
#define IDC_DATEPICKER                  1051
#define IDC_TIMEPICKER                  1052
#define IDC_TIMEZONELIST                1053
#define IDC_AUTODAYLIGHT                1054

#define IDD_PROCESSPAGE                 1060
#define IDC_ACTIVITY                    1061
#define IDC_ITEM                        1062
#define IDC_PROCESSPROGRESS             1063

#define IDD_FINISHPAGE                  1070
#define IDC_FINISHTITLE                 1071
#define IDC_RESTART_PROGRESS            1072

#define IDD_GPL                         2100
#define IDC_GPL_TEXT                    2101

#define IDD_STATUSWINDOW_DLG            2200
#define IDC_STATUSLABEL                 2201

#define IDS_ACKTITLE                    3010
#define IDS_ACKSUBTITLE                 3011

#define IDS_OWNERTITLE                  3020
#define IDS_OWNERSUBTITLE               3021

#define IDS_COMPUTERTITLE               3030
#define IDS_COMPUTERSUBTITLE            3031

#define IDS_LOCALETITLE                 3040
#define IDS_LOCALESUBTITLE              3041

#define IDS_DATETIMETITLE               3050
#define IDS_DATETIMESUBTITLE            3051

#define IDS_PROCESSTITLE                3060
#define IDS_PROCESSSUBTITLE             3061

#define IDS_ACKPROJECTS                 3100

#define IDS_ACCESSORIES                 3200
#define IDS_GAMES                       3201
#define IDS_SYS_TOOLS                   3202
#define IDS_SYS_ACCESSIBILITY           3203

#define IDS_CMT_CALC                    3210
#define IDS_CMT_CHARMAP                 3211
#define IDS_CMT_CMD                     3212
#define IDS_CMT_DEVMGMT                 3213
#define IDS_CMT_DOWNLOADER              3214
#define IDS_CMT_EVENTVIEW               3215
#define IDS_CMT_EXPLORER                3216
#define IDS_CMT_KBSWITCH                3217
#define IDS_CMT_MAGNIFY                 3218
#define IDS_CMT_MPLAY32                 3219
#define IDS_CMT_MSCONFIG                3220
#define IDS_CMT_NOTEPAD                 3221
#define IDS_CMT_RDESKTOP                3222
#define IDS_CMT_REGEDIT                 3223
#define IDS_CMT_SERVMAN                 3224
#define IDS_CMT_SCREENSHOT              3225
#define IDS_CMT_SOLITAIRE               3226
#define IDS_CMT_WINEMINE                3227
#define IDS_CMT_WORDPAD                 3228

#define IDS_REACTOS_SETUP               3300
#define IDS_UNKNOWN_ERROR               3301
#define IDS_REGISTERING_COMPONENTS      3302
#define IDS_LOADLIBRARY_FAILED          3303
#define IDS_GETPROCADDR_FAILED          3304
#define IDS_REGSVR_FAILED               3305
#define IDS_DLLINSTALL_FAILED           3306
#define IDS_TIMEOUT                     3307
#define IDS_REASON_UNKNOWN              3308

#define IDS_SHORT_CALC          3400
#define IDS_SHORT_CHARMAP       3401
#define IDS_SHORT_CMD           3402
#define IDS_SHORT_DEVICE        3403
#define IDS_SHORT_DOWNLOADER    3404
#define IDS_SHORT_EVENTVIEW     3405
#define IDS_SHORT_EXPLORER      3406
#define IDS_SHORT_KBSWITCH      3407
#define IDS_SHORT_MAGNIFY       3408
#define IDS_SHORT_MPLAY32       3409
#define IDS_SHORT_MSCONFIG      3410
#define IDS_SHORT_NOTEPAD       3411
#define IDS_SHORT_RDESKTOP      3412
#define IDS_SHORT_REGEDIT       3413
#define IDS_SHORT_SERVICE       3414
#define IDS_SHORT_SNAP          3415
#define IDS_SHORT_SOLITAIRE     3416
#define IDS_SHORT_WINEMINE      3417
#define IDS_SHORT_WORDPAD       3418

#define IDS_WZD_NAME            3450
#define IDS_WZD_SETCOMPUTERNAME 3451
#define IDS_WZD_COMPUTERNAME    3452
#define IDS_WZD_PASSWORDEMPTY   3453
#define IDS_WZD_PASSWORDMATCH   3454
#define IDS_WZD_PASSWORDCHAR    3455
#define IDS_WZD_LOCALTIME       3456

#define IDS_STATUS_INSTALL_DEV  3500

#define IDR_GPL                 4000

#endif /* RESOURCE_H */
