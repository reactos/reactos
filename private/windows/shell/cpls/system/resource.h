/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    resource.h

Abstract:

    Resource IDs for the System Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#ifndef _SYSDM_RESOURCE_H_
#define _SYSDM_RESOURCE_H_

//
// Icons
//
#define ID_ICON                       1
#define IDI_PROFILE                   2
#define DOCK_ICON                     3
#define UP_ICON                       4
#define DOWN_ICON                     5
#define IDI_COMPUTER                  6
#define PERF_ICON                     7
#define ENVVAR_ICON                   8
#define CRASHDUMP_ICON                9

//
// Bitmaps
//
#define IDB_WINDOWS                   1

//
// String table constants
//
#define IDS_NAME                      1
#define IDS_INFO                      2
#define IDS_TITLE                     3

#define IDS_DEBUG                     4
#define IDS_XDOTX_MB                  5

#define IDS_PAGESIZE                  6

#define IDS_DUMPFILE                  7

#define IDS_USERENVVARS               8

#define IDS_UP_NAME                   9
#define IDS_UP_SIZE                   10
#define IDS_UP_TYPE                   11
#define IDS_UP_MOD                    12
#define IDS_UP_KB                     13

#define IDS_UP_LOCAL                  14
#define IDS_UP_FLOATING               15
#define IDS_UP_MANDATORY              16
#define IDS_UP_CONFIRM                17
#define IDS_UP_CONFIRMTITLE           18
#define IDS_UP_DIRPICK                19
#define IDS_UP_ERRORTITLE             20
#define IDS_UP_PICKUSER               21
#define IDS_UP_ACCUNKNOWN             22
#define IDS_UP_ACCDELETED             23

#define IDS_NETID_DLL_NAME            55

#define INITS                         100
#define SYSTEM                        110

//
// Edit Environment Variable strings
//
#define IDS_NEW_SYSVAR_CAPTION        200
#define IDS_EDIT_SYSVAR_CAPTION       201
#define IDS_NEW_USERVAR_CAPTION       202
#define IDS_EDIT_USERVAR_CAPTION      203

#define HWP                         60

#define HWP_DEF_FRIENDLYNAME        HWP+0
#define HWP_CURRENT_TAG             HWP+1
#define HWP_UNAVAILABLE             HWP+2
#define HWP_ERROR_CAPTION           HWP+3
#define HWP_ERROR_PROFILE_IN_USE    HWP+4
#define HWP_ERROR_INVALID_CHAR      HWP+5
#define HWP_ERROR_IN_USE            HWP+6
#define HWP_CONFIRM_DELETE_CAP      HWP+7
#define HWP_CONFIRM_DELETE          HWP+8
#define HWP_INVALID_WAIT            HWP+9
#define HWP_CONFIRM_NOT_PORTABLE    HWP+10


//
// Dialog box ID's
//
#define IDD_USERPROFILE          100
#define IDD_GENERAL              101
#define IDD_PHONESUP             102
#define IDD_PERFORMANCE          103
#define IDD_STARTUP              104
#define IDD_ENVVARS              105
#define IDD_ENVVAREDIT            42
#define DLG_HWPROFILES           106
#define DLG_HWP_RENAME           107
#define DLG_HWP_COPY             108
#define DLG_HWP_GENERAL          109
#define IDD_UP_TYPE              110
#define IDD_UP_COPY              111
#define IDD_ADVANCED             115
#define IDD_HARDWARE		    2000
#define DLG_VIRTUALMEM           41


//
// Shared text id's
//
#define IDC_TEXT_1                 10
#define IDC_TEXT_2                 11
#define IDC_TEXT_3                 12
#define IDC_TEXT_4                 13


//
// General page
//
#define IDC_GEN_WINDOWS_IMAGE            51
#define IDC_GEN_SERIAL_NUMBER            52
#define IDC_GEN_SERVICE_PACK             53
#define IDC_GEN_REGISTERED_0             54
#define IDC_GEN_REGISTERED_1             55
#define IDC_GEN_REGISTERED_2             56
#define IDC_GEN_REGISTERED_3             57
#define IDC_GEN_OEM_NUDGE                58
#define IDC_GEN_MACHINE                  59
#define IDC_GEN_OEM_IMAGE                60
#define IDC_GEN_MACHINE_0                61
#define IDC_GEN_MACHINE_1                62
#define IDC_GEN_MACHINE_2                63
#define IDC_GEN_MACHINE_3                64
#define IDC_GEN_MACHINE_4                65
#define IDC_GEN_OEM_SUPPORT              66


//
// Phone support dialog
//
#define IDC_SUPPORT_TEXT                 70

//
// Performace dialog
//
#define IDC_PERF_CHANGE                 201
#define IDC_PERF_VM_ALLOCD              202
#define IDC_PERF_GROUP                  203
#define IDC_PERF_WORKSTATION            204
#define IDC_PERF_SERVER                 205
#define IDC_PERF_VM_GROUP               206
#define IDC_PERF_VM_ALLOCD_LABEL        207
#define IDC_PERF_TEXT                   208


//
// Startup page
//
#define IDC_STARTUP_SYS_OS              300
#define IDC_STARTUP_SYS_SECONDS         301
#define IDC_STARTUP_SYS_SECSCROLL       302
#define IDC_STARTUP_SYS_ENABLECOUNTDOWN 303
#define IDC_STARTUP_SYSTEM_GRP          304
#define IDC_STARTUP_SYS_SECONDS_LABEL   305

#define IDC_STARTUP_CDMP_GRP            601
#define IDC_STARTUP_CDMP_TXT1           602
#define IDC_STARTUP_CDMP_LOG            603
#define IDC_STARTUP_CDMP_SEND           604
#define IDC_STARTUP_CDMP_WRITE          605
#define IDC_STARTUP_CDMP_FILENAME       606
#define IDC_STARTUP_CDMP_OVERWRITE      607
#define IDC_STARTUP_CDMP_AUTOREBOOT     608
#define IDC_STARTUP_CDMP_KERNELONLY     609


//
// Environment Variables dialog
//
#define IDC_ENVVAR_SYS_LB_SYSVARS       400
#define IDC_ENVVAR_SYS_USERENV          401
#define IDC_ENVVAR_SYS_LB_USERVARS      402
#define IDC_ENVVAR_SYS_VAR              403
#define IDC_ENVVAR_SYS_VALUE            404
#define IDC_ENVVAR_SYS_NEWSV            407
#define IDC_ENVVAR_SYS_EDITSV           408
#define IDC_ENVVAR_SYS_DELSV            409
#define IDC_ENVVAR_SYS_SYSVARS          410
#define IDC_ENVVAR_SYS_USERGROUP        411
#define IDC_ENVVAR_SYS_NEWUV            412
#define IDC_ENVVAR_SYS_EDITUV           413
#define IDC_ENVVAR_SYS_NDELUV           414

#define IDC_ENVVAR_SYS_SETUV            405
#define IDC_ENVVAR_SYS_DELUV            406

//
// Environment Variables "New..."/"Edit.." dialog
//
#define IDC_ENVVAR_EDIT_NAME_LABEL      100
#define IDC_ENVVAR_EDIT_NAME            101
#define IDC_ENVVAR_EDIT_VALUE_LABEL     102
#define IDC_ENVVAR_EDIT_VALUE           103



//
// IF IDS ARE ADDED OR REMOVED, THEN ADD/REMOVE THE CORRESPONDING
// HELP IDS IN HWPROF.C ALSO!!
//
#define IDD_HWP_PROFILES                300
#define IDD_HWP_PROPERTIES              301
#define IDD_HWP_COPY                    302
#define IDD_HWP_RENAME                  303
#define IDD_HWP_DELETE                  304
#define IDD_HWP_ST_MULTIPLE             305
#define IDD_HWP_WAITFOREVER             307
#define IDD_HWP_WAITUSER                308
#define IDD_HWP_SECONDS                 309
#define IDD_HWP_SECSCROLL               310
#define IDD_HWP_COPYTO                  311
#define IDD_HWP_COPYFROM                312
#define IDD_HWP_ST_DOCKID               313
#define IDD_HWP_ST_SERIALNUM            314
#define IDD_HWP_DOCKID                  315
#define IDD_HWP_SERIALNUM               316
#define IDD_HWP_PORTABLE                317
#define IDD_HWP_UNKNOWN                 319
#define IDD_HWP_DOCKED                  320
#define IDD_HWP_UNDOCKED                321
#define IDD_HWP_ST_PROFILE              322
#define IDD_HWP_ORDERUP                 323
#define IDD_HWP_ORDERDOWN               324
#define IDD_HWP_RENAMEFROM              325
#define IDD_HWP_RENAMETO                326
#define IDD_HWP_UNUSED_1                340
#define IDD_HWP_UNUSED_2                341
#define IDD_HWP_UNUSED_3                342
#define IDD_HWP_UNUSED_4                343
#define IDD_HWP_UNUSED_5                344
#define IDD_HWP_UNUSED_6                345

//
// NOTE: The following ID ranges are reserved for use by property
// page providers for the Hardware Profiles and should not be used
// by the main hardware profiles dialog or the Hardware Profiles
// General property page. All property page providers (dlls that
// add pages to the Hardware Profiles properties) will use help
// IDs within the range allocated for Hardware Profiles (IDH_HWPROFILE)
//
// RESERVE FOR:
//
//      No Net Property Page Extension:
//          Control IDs:    500-549
//          Help IDs:       IDH_HWPROFILE+500 - IDH_HWPROFILE+550
//
//      Other Property Page Extensions...
//
//          Control IDs:    550-599  - reserved for later use
//          Control IDs:    600-649  - reserved for later use
//          Control IDs:    650-699  - reserved for later use
//


//
// Static text id
//
#define IDC_STATIC                -1


//
// User profile page
//
#define IDC_UP_LISTVIEW         1000
#define IDC_UP_DELETE           1001
#define IDC_UP_TYPE             1002
#define IDC_UP_COPY             1003
#define IDC_UP_ICON             1004
#define IDC_UP_TEXT             1005


//
// User profile 'change type' dialog
//
#define IDC_UPTYPE_LOCAL        1020
#define IDC_UPTYPE_FLOAT        1021
#define IDC_UPTYPE_SLOW         1022
#define IDC_UPTYPE_SLOW_TEXT    1023
#define IDC_UPTYPE_GROUP        1024


//
// User profile 'copy to' dialog
//
#define IDC_COPY_PATH           1030
#define IDC_COPY_BROWSE         1031
#define IDC_COPY_USER           1032
#define IDC_COPY_CHANGE         1033
#define IDC_COPY_GROUP          1034


//
// Virtual Mem dlg
//
#define IDD_VM_DRIVE_HDR        1140
#define IDD_VM_PF_SIZE_LABEL    1142
#define IDD_VM_DRIVE_LABEL      1144
#define IDD_VM_SPACE_LABEL      1146
#define IDD_VM_MIN_LABEL        1148
#define IDD_VM_RECOMMEND_LABEL  1150
#define IDD_VM_ALLOCD_LABEL     1152
#define IDD_VM_RSL_ALLOCD_LABEL 1154
#define IDD_VM_RSL_LABEL        1156
#define IDD_VM_VOLUMES          1160
#define IDD_VM_SF_DRIVE         1161
#define IDD_VM_SF_SPACE         1162
#define IDD_VM_SF_SIZE          1163
#define IDD_VM_SF_SIZEMAX       1164
#define IDD_VM_SF_SET           1165
#define IDD_VM_MIN              1166
#define IDD_VM_RECOMMEND        1167
#define IDD_VM_ALLOCD           1168
#define IDD_VM_ST_INITSIZE      1169
#define IDD_VM_ST_MAXSIZE       1170
#define IDD_VMEM_ICON           1171
#define IDD_VMEM_MESSAGE        1172
#define IDD_VM_REG_SIZE_LIM     1173
#define IDD_VM_REG_SIZE_TXT     1174
#define IDD_VM_RSL_ALLOCD       1175


//
// Hardware dlg
//
#define IDC_WIZARD_ICON		      2001
#define IDC_WIZARD_TEXT 	      2002
#define IDC_WIZARD_START	      2003
#define IDC_DEVMGR_ICON 	      2004
#define IDC_DEVMGR_TEXT 	      2005
#define IDC_DEVMGR_START	      2006
#define IDC_HWPROFILES_START      2007
#define IDC_HWPROFILES_ICON       2008
#define IDC_HWPROFILES_START_TEXT 2009

//
// Advanced dlg
//
#define IDC_ADV_PERF_ICON       100
#define IDC_ADV_PERF_TEXT       101
#define IDC_ADV_PERF_BTN        110
#define IDC_ADV_ENV_ICON        120
#define IDC_ADV_ENV_TEXT        121
#define IDC_ADV_ENV_BTN         130
#define IDC_ADV_RECOVERY_ICON   140
#define IDC_ADV_RECOVERY_TEXT   141
#define IDC_ADV_RECOVERY_BTN    150

#endif // _SYSDM_RESOURCE_H_
