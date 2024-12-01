/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/resource.h
 * PURPOSE:         Manages all the partitions of the OS in an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */

#pragma once

#define IDS_NONE -1
#define MSG_NONE 0

#define IDS_APP_HEADER        0
#define IDS_APP_USAGE         1
#define IDS_APP_LICENSE       2
#define IDS_APP_CURR_COMPUTER 3
#define IDS_APP_LEAVING       4
#define IDS_APP_PROMPT        5

#define IDS_ACTIVE_FAIL                1000
#define IDS_ACTIVE_SUCCESS             1001
#define IDS_ACTIVE_ALREADY             1002

#define IDS_CLEAN_FAIL                 1020
#define IDS_CLEAN_SUCCESS              1021
#define IDS_CLEAN_SYSTEM               1022

#define IDS_CREATE_PARTITION_FAIL      1050
#define IDS_CREATE_PARTITION_SUCCESS   1051

#define IDS_DELETE_PARTITION_FAIL      1070
#define IDS_DELETE_PARTITION_SUCCESS   1071

#define IDS_DETAIL_INFO_DISK_ID        1107
#define IDS_DETAIL_INFO_TYPE           1108
#define IDS_DETAIL_INFO_STATUS         1109
#define IDS_DETAIL_INFO_PATH           1110
#define IDS_DETAIL_INFO_TARGET         1111
#define IDS_DETAIL_INFO_LUN_ID         1112
#define IDS_DETAIL_INFO_LOC_PATH       1113
#define IDS_DETAIL_INFO_CURR_RO_STATE  1114
#define IDS_DETAIL_INFO_RO             1115
#define IDS_DETAIL_INFO_BOOT_DSK       1116
#define IDS_DETAIL_INFO_PAGE_FILE_DSK  1117
#define IDS_DETAIL_INFO_HIBER_FILE_DSK 1118
#define IDS_DETAIL_INFO_CRASH_DSK      1119
#define IDS_DETAIL_INFO_CLST_DSK       1120

#define IDS_DETAIL_PARTITION_NUMBER    1130
#define IDS_DETAIL_PARTITION_TYPE      1131
#define IDS_DETAIL_PARTITION_HIDDEN    1132
#define IDS_DETAIL_PARTITION_ACTIVE    1133
#define IDS_DETAIL_PARTITION_OFFSET    1134

#define IDS_DETAIL_NO_DISKS            1135
#define IDS_DETAIL_NO_VOLUME           1136

#define IDS_FILESYSTEMS_CURRENT        1180
#define IDS_FILESYSTEMS_FORMATTING     1181
#define IDS_FILESYSTEMS_TYPE           1182
#define IDS_FILESYSTEMS_CLUSTERSIZE    1183

#define IDS_HELP_FORMAT_STRING         1200

#define IDS_INACTIVE_FAIL              1210
#define IDS_INACTIVE_SUCCESS           1211
#define IDS_INACTIVE_ALREADY           1212


#define IDS_LIST_DISK_HEAD             3300
#define IDS_LIST_DISK_LINE             3301
#define IDS_LIST_DISK_FORMAT           3302
#define IDS_LIST_PARTITION_HEAD        3303
#define IDS_LIST_PARTITION_LINE        3304
#define IDS_LIST_PARTITION_FORMAT      3305
#define IDS_LIST_PARTITION_NO_DISK     3306
#define IDS_LIST_VOLUME_HEAD           3307
#define IDS_LIST_VOLUME_LINE           3308
#define IDS_LIST_VOLUME_FORMAT         3309

#define IDS_RESCAN_START               4100
#define IDS_RESCAN_END                 4101

#define IDS_SELECT_NO_DISK             4400
#define IDS_SELECT_DISK                4401
#define IDS_SELECT_DISK_INVALID        4402
#define IDS_SELECT_DISK_ENUM_NO_START  4403
#define IDS_SELECT_DISK_ENUM_FINISHED  4404
#define IDS_SELECT_NO_PARTITION        4405
#define IDS_SELECT_PARTITION           4406
#define IDS_SELECT_PARTITION_NO_DISK   4407
#define IDS_SELECT_PARTITION_INVALID   4408
#define IDS_SELECT_NO_VOLUME           4409
#define IDS_SELECT_VOLUME              4410
#define IDS_SELECT_VOLUME_INVALID      4411

#define IDS_SETID_FAIL                 4450
#define IDS_SETID_SUCCESS              4451
#define IDS_SETID_INVALID_FORMAT       4452
#define IDS_SETID_INVALID_TYPE         4453


#define IDS_STATUS_YES          31
#define IDS_STATUS_NO           32
#define IDS_STATUS_DISK_HEALTHY 33
#define IDS_STATUS_DISK_SICK    34
#define IDS_STATUS_UNAVAILABLE  35
#define IDS_STATUS_ONLINE       36
#define IDS_STATUS_OFFLINE      37
#define IDS_STATUS_NO_MEDIA     38

#define IDS_MSG_ARG_SYNTAX_ERROR   41

#define IDS_HELP_ACTIVE                     58
#define IDS_HELP_ADD                        59
#define IDS_HELP_ASSIGN                     60
#define IDS_HELP_ATTRIBUTES                 61
#define IDS_HELP_ATTACH                     62
#define IDS_HELP_AUTOMOUNT                  63
#define IDS_HELP_BREAK                      64
#define IDS_HELP_CLEAN                      65
#define IDS_HELP_COMPACT                    66
#define IDS_HELP_CONVERT                    67

#define IDS_HELP_CREATE                     68
#define IDS_HELP_CREATE_PARTITION           69
#define IDS_HELP_CREATE_PARTITION_EFI       70
#define IDS_HELP_CREATE_PARTITION_EXTENDED  71
#define IDS_HELP_CREATE_PARTITION_LOGICAL   72
#define IDS_HELP_CREATE_PARTITION_MSR       73
#define IDS_HELP_CREATE_PARTITION_PRIMARY   74
#define IDS_HELP_CREATE_VOLUME              75
#define IDS_HELP_CREATE_VDISK               76

#define IDS_HELP_DELETE                     77
#define IDS_HELP_DELETE_DISK                78
#define IDS_HELP_DELETE_PARTITION           79
#define IDS_HELP_DELETE_VOLUME              80

#define IDS_HELP_DETACH                     81

#define IDS_HELP_DETAIL                     82
#define IDS_HELP_DETAIL_DISK                83
#define IDS_HELP_DETAIL_PARTITION           84
#define IDS_HELP_DETAIL_VOLUME              85

#define IDS_HELP_EXIT                       86
#define IDS_HELP_EXPAND                     87
#define IDS_HELP_EXTEND                     88
#define IDS_HELP_FILESYSTEMS                89
#define IDS_HELP_FORMAT                     90
#define IDS_HELP_GPT                        91
#define IDS_HELP_HELP                       92
#define IDS_HELP_IMPORT                     93
#define IDS_HELP_INACTIVE                   94

#define IDS_HELP_LIST                       95
#define IDS_HELP_LIST_DISK                  96
#define IDS_HELP_LIST_PARTITION             97
#define IDS_HELP_LIST_VOLUME                98
#define IDS_HELP_LIST_VDISK                 99

#define IDS_HELP_MERGE                     100
#define IDS_HELP_ONLINE                    101
#define IDS_HELP_OFFLINE                   102
#define IDS_HELP_RECOVER                   103
#define IDS_HELP_REM                       104
#define IDS_HELP_REMOVE                    105
#define IDS_HELP_REPAIR                    106
#define IDS_HELP_RESCAN                    107
#define IDS_HELP_RETAIN                    108
#define IDS_HELP_SAN                       109

#define IDS_HELP_SELECT                    110
#define IDS_HELP_SELECT_DISK               111
#define IDS_HELP_SELECT_PARTITION          112
#define IDS_HELP_SELECT_VOLUME             113
#define IDS_HELP_SELECT_VDISK              114

#define IDS_HELP_SETID                     115
#define IDS_HELP_SHRINK                    116

#define IDS_HELP_UNIQUEID                  117
#define IDS_HELP_UNIQUEID_DISK             118

#define IDS_ERROR_MSG_NO_SCRIPT  2000
#define IDS_ERROR_MSG_BAD_ARG    2001
#define IDS_ERROR_INVALID_ARGS   2002
#define IDS_ERROR_NO_MEDIUM      2003
