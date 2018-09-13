//*************************************************************
//
//  Resource.h      -   Header file for userenv.rc
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uevents.h"

#define IDS_FAILED_LOAD_PROFILE       0
#define IDS_ACCESSDENIED              1
#define IDS_FAILEDDIRCREATE           2
#define IDS_FAILEDDIRCREATE2          3
#define IDS_CENTRAL_NOT_AVAILABLE     4
#define IDS_TEMP_DIR_FAILED           5
#define IDS_FAILED_LOAD_LOCAL         6
#define IDS_SECURITY_FAILED           7
#define IDS_CENTRAL_UPDATE_FAILED     8
#define IDS_ADMIN_OVERRIDE            9
#define IDS_CENTRAL_NOT_AVAILABLE2    10
#define IDS_MANDATORY_NOT_AVAILABLE   11
#define IDS_MANDATORY_NOT_AVAILABLE2  12
#define IDS_MISSINGPOLICYFILEENTRY    13
#define IDS_REGLOADKEYFAILED          14
#define IDS_COMMON                    15
#define IDS_COPYERROR                 16
#define IDS_FAILED_LOAD_1009          17
#define IDS_TEMPPROFILEASSIGNED       18
#define IDS_TEMPPROFILEASSIGNED1      19
#define IDS_FAILED_LOAD_1450          20
#define IDS_FAILED_HIVE_UNLOAD        21
#define IDS_FAILED_HIVE_UNLOAD1       22
#define IDS_FAILED_ALLOCATION         23
#define IDS_PROFILEUPDATE_6002        24
#define IDS_PROFILES_ROOT             25
#define IDS_PROFILE_PATH_TOOLONG      26
#define IDS_PROFILE_DIR_BACKEDUP      27
#define IDS_HIVE_UNLOAD_RETRY         28

#define IDS_SH_APPDATA                30
#define IDS_SH_DESKTOP                31
#define IDS_SH_FAVORITES              32
#define IDS_SH_NETHOOD                33
#define IDS_SH_PRINTHOOD              34
#define IDS_SH_RECENT                 35
#define IDS_SH_SENDTO                 36
#define IDS_SH_STARTMENU              37
#define IDS_SH_TEMPLATES              38
#define IDS_SH_PERSONAL               39
#define IDS_SH_PROGRAMS               40
#define IDS_SH_STARTUP                41
#define IDS_SH_TEMP                   42
#define IDS_SH_LOCALSETTINGS          43
#define IDS_SH_LOCALAPPDATA           44
#define IDS_SH_CACHE                  45
#define IDS_SH_COOKIES                46
#define IDS_SH_HISTORY                47
#define IDS_SH_MYPICTURES             48
#define IDS_SH_SHAREDDOCS             49

#define IDS_SH_PERSONAL2              70
#define IDS_SH_MYPICTURES2            71
#define IDS_SH_TEMPLATES2             72

#define IDS_PROFILE_FORMAT            75
#define IDS_PROFILEDOMAINNAME_FORMAT  76

#define IDS_NT_AUTHORITY              80
#define IDS_BUILTIN                   81

#define IDS_COPYING                  100
#define IDS_CREATING                 101

#define IDS_LOCALGPONAME             103
#define IDS_TEMPINTERNETFILES        104
#define IDS_HISTORY                  105
#define IDS_EXCLUSIONLIST            106
#define IDS_REGISTRYNAME             107
#define IDS_CALLEXTENSION            108
#define IDS_USER_SETTINGS            109
#define IDS_COMPUTER_SETTINGS        110


#define IDS_NO_LOCAL_GPO             500
#define IDS_FAILED_WLDAP32           501
#define IDS_FAILED_DS_CONNECT        502
#define IDS_FAILED_DS_BIND           503
#define IDS_FAILED_NETAPI32          504
#define IDS_FAILED_QUERY_SITE        505
#define IDS_FAILED_ROOT_SEARCH       506
#define IDS_NO_NAMING_CONTEXT        507
#define IDS_SEARCHING                508
#define IDS_FOUND_GPO                509
#define IDS_FOUND_LOCAL_GPO          510
#define IDS_GPO_FORCE                511
#define IDS_GPO_NO_FORCE             512
#define IDS_GPO_DISABLED             513
#define IDS_GPO_LINK_DISABLED        514
#define IDS_NO_GPOS                  515
#define IDS_OBJECT_NOT_FOUND         516
#define IDS_SET_STRING_VALUE         517
#define IDS_SET_DWORD_VALUE          518
#define IDS_SET_UNKNOWN_VALUE        519
#define IDS_FAILED_SET               520
#define IDS_FAILED_CREATE            521
#define IDS_DELETED_VALUE            522
#define IDS_FAIL_DELETE_VALUE        523
#define IDS_DELETED_KEY              524
#define IDS_USER_POLICY_APPLIED      525
#define IDS_MACHINE_POLICY_APPLIED   526
#define IDS_USERNAME                 527
#define IDS_DOMAINNAME               528
#define IDS_DCNAME                   529
#define IDS_SLOWLINK                 530
#define IDS_GPO_QUERY_FAILED         531
#define IDS_GPO_LIST                 532
#define IDS_START_MACHINE_POLICY     533
#define IDS_START_USER_POLICY        534
#define IDS_NO_GPOS2                 535
#define IDS_FAILED_CRITICAL_SECTION  536
#define IDS_PROCESSING               537
#define IDS_CALL_EXTENSION           538
#define IDS_EXT_LOAD_FAIL            539
#define IDS_EXT_FUNC_FAIL            540
#define IDS_EXT_MISSING_FUNC         541
#define IDS_EXT_MISSING_DLLNAME      542
#define IDS_REMOVEGPOS               543
#define IDS_REMOVEGPO                544
#define IDS_FINISHED_REMOVEGPOS      545
#define IDS_GPO_LIST_CHANGED         546
#define IDS_NO_REGISTRY              547
#define IDS_NO_ACCESS                548
#define IDS_NO_CHANGES               549
#define IDS_FAILED_READ_GPO_LIST     550
#define IDS_FAILED_SAVE_GPO_LIST     551
#define IDS_FAILED_REMOVE_GPO_LIST   552
#define IDS_GPO_DELETED              553
#define IDS_NO_DS_OBJECT             554
#define IDS_ROLE_STANDALONE          555
#define IDS_ROLE_DOWNLEVEL_DOMAIN    556
#define IDS_ROLE_DS_DOMAIN           557
#define IDS_FAILED_ROLE              558
#define IDS_FAILED_USERNAME          559
#define IDS_FAILED_DSNAME            560

#define IDS_FAILED_MACHINENAME       562
#define IDS_FAILED_SECUR32           563


#define IDS_NO_MACHINE_DOMAIN        569
#define IDS_CORRUPT_GPO_FSPATH       570
#define IDS_CORRUPT_GPO_COMMONNAME   571
#define IDS_SKIP_GPO                 572
#define IDS_BLOCK_ENABLED            573
#define IDS_OUT_OF_MEMORY            574
#define IDS_FAILED_ACCESS_CHECK      575
#define IDS_READ_EXT_FAILED          576
#define IDS_EXT_SKIPPED              577
#define IDS_GPO_PROC_STOPPED         578
#define IDS_CHANGES_FAILED           579
#define IDS_FAILED_GETDELETED_LIST   580
#define IDS_EXT_SKIPPED_DUETO_FAILED_REG 581
#define IDS_CORRUPT_GPO_FUNCVERSION  582
#define IDS_GPO_TOO_OLD              583
#define IDS_GPO_NO_DATA              584
#define IDS_SETUP_GPOFILTER_FAILED   585
#define IDS_EXT_HAS_EMPTY_LISTS      586
#define IDS_INCORRECT_CLASS          587
#define IDS_FAILED_GET_SID           588
#define IDS_FAILED_ICMP              589
#define IDS_FAILED_WSOCK32           590

#define IDS_FAILED_DSAPI             592
#define IDS_CAUGHT_EXCEPTION         593
#define IDS_FAILED_GPO_SEARCH        594
#define IDS_FAILED_OU_SEARCH         595
#define IDS_FAILED_IMPERSONATE       596
#define IDS_FAILED_TIMER             597
#define IDS_FAILED_SHELL32API        598
#define IDS_FAILED_WRITE_SID_MAPPING 599
#define IDS_FAILED_MIGRATION         600
#define IDS_EXT_FAILED               601
#define IDS_LOOPBACK_DISABLED1       602
#define IDS_LOOPBACK_DISABLED2       603
#define IDS_TOO_MANY_GPOS            604

//
// Profile icon
//

#define IDI_PROFILE                  1


//
// Slow link test data
//

#define IDB_SLOWLINK                 1


//
// Slow link dialog
//

#define IDD_SLOW_LINK             1000
#define IDC_DOWNLOAD              1001
#define IDC_LOCAL                 1002
#define IDC_TIMEOUT               1004


//
// Error dialog
//

#define IDD_ERROR                 3000
#define IDC_ERRORTEXT             3001


//
// Profile coping status
//

#define IDD_COPYSTATUS            4000
#define IDC_PROGRESS              4001


//
// File copy error dialog
//

#define IDD_COPYERROR               5000
#define IDC_ERRORMSG                5000
#define IDC_SOURCE                  5001
#define IDC_DESTINATION             5002
#define IDC_TIME                    5003
#define IDC_TIMETITLE               5004
#define IDC_ABORT                   5005
#define IDC_SKIPFILE                5006
