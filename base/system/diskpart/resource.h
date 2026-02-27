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
#define IDS_ACTIVE_NO_MBR              1003

#define IDS_AUTOMOUNT_ENABLED          1010
#define IDS_AUTOMOUNT_DISABLED         1011
#define IDS_AUTOMOUNT_SCRUBBED         1012

#define IDS_ASSIGN_FAIL                1020
#define IDS_ASSIGN_SUCCESS             1021
#define IDS_ASSIGN_ALREADY_ASSIGNED    1022
#define IDS_ASSIGN_INVALID_LETTER      1023
#define IDS_ASSIGN_NO_MORE_LETTER      1024
#define IDS_ASSIGN_SYSTEM_VOLUME       1025

#define IDS_CLEAN_FAIL                 1030
#define IDS_CLEAN_SUCCESS              1031
#define IDS_CLEAN_SYSTEM               1032

#define IDS_CONVERT_GPT_ALREADY        1040
#define IDS_CONVERT_GPT_NOT_EMPTY      1041
#define IDS_CONVERT_GPT_SUCCESS        1042
#define IDS_CONVERT_MBR_ALREADY        1043
#define IDS_CONVERT_MBR_NOT_EMPTY      1044
#define IDS_CONVERT_MBR_SUCCESS        1045

#define IDS_CREATE_PARTITION_FAIL      1050
#define IDS_CREATE_PARTITION_SUCCESS   1051
#define IDS_CREATE_PARTITION_INVALID_STYLE 1052

#define IDS_DELETE_PARTITION_FAIL      1070
#define IDS_DELETE_PARTITION_SUCCESS   1071
#define IDS_DELETE_PARTITION_SYSTEM    1072

#define IDS_DETAIL_DISK_DESCRIPTION    1106
#define IDS_DETAIL_DISK_ID             1107
#define IDS_DETAIL_DISK_TYPE           1108
#define IDS_DETAIL_DISK_STATUS         1109
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
#define IDS_DETAIL_PARTITION_REQUIRED  1135
#define IDS_DETAIL_PARTITION_ATTRIBUTE 1136
#define IDS_DETAIL_NO_DISKS            1137
#define IDS_DETAIL_NO_VOLUME           1138

#define IDS_FILESYSTEMS_CURRENT        1170
#define IDS_FILESYSTEMS_FORMATTING     1171
#define IDS_FILESYSTEMS_TYPE           1172
#define IDS_FILESYSTEMS_CLUSTERSIZE    1173
#define IDS_FILESYSTEMS_SERIAL_NUMBER  1174
#define IDS_FILESYSTEMS_DEFAULT        1175

#define IDS_FORMAT_FAIL                1180
#define IDS_FORMAT_SUCCESS             1181
#define IDS_FORMAT_PROGRESS            1182

#define IDS_GPT_FAIL                   1190
#define IDS_GPT_SUCCESS                1191

#define IDS_HELP_FORMAT_STRING         1200

#define IDS_INACTIVE_FAIL              1210
#define IDS_INACTIVE_SUCCESS           1211
#define IDS_INACTIVE_ALREADY           1212
#define IDS_INACTIVE_NO_MBR            1213

#define IDS_LIST_DISK_HEAD             3300
#define IDS_LIST_DISK_LINE             3301
#define IDS_LIST_DISK_FORMAT           3302
#define IDS_LIST_PARTITION_HEAD        3303
#define IDS_LIST_PARTITION_LINE        3304
#define IDS_LIST_PARTITION_FORMAT      3305
#define IDS_LIST_PARTITION_NO_DISK     3306
#define IDS_LIST_PARTITION_NONE        3307
#define IDS_LIST_VOLUME_HEAD           3308
#define IDS_LIST_VOLUME_LINE           3309
#define IDS_LIST_VOLUME_FORMAT         3310

#define IDS_REMOVE_FAIL                4000
#define IDS_REMOVE_SUCCESS             4001
#define IDS_REMOVE_NO_LETTER           4002
#define IDS_REMOVE_WRONG_LETTER        4003

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

#define IDS_UNIQUID_DISK_INVALID_STYLE 4500

#define IDS_STATUS_YES          31
#define IDS_STATUS_NO           32
#define IDS_STATUS_DISK_HEALTHY 33
#define IDS_STATUS_DISK_SICK    34
#define IDS_STATUS_UNAVAILABLE  35
#define IDS_STATUS_ONLINE       36
#define IDS_STATUS_OFFLINE      37
#define IDS_STATUS_NO_MEDIA     38
#define IDS_INFO_BOOT           39
#define IDS_INFO_SYSTEM         40

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
#define IDS_HELP_CONVERT_GPT                68
#define IDS_HELP_CONVERT_MBR                69

#define IDS_HELP_CREATE                     70
#define IDS_HELP_CREATE_PARTITION           71
#define IDS_HELP_CREATE_PARTITION_EFI       72
#define IDS_HELP_CREATE_PARTITION_EXTENDED  73
#define IDS_HELP_CREATE_PARTITION_LOGICAL   74
#define IDS_HELP_CREATE_PARTITION_MSR       75
#define IDS_HELP_CREATE_PARTITION_PRIMARY   76
#define IDS_HELP_CREATE_VOLUME              77
#define IDS_HELP_CREATE_VDISK               78

#define IDS_HELP_DELETE                     79
#define IDS_HELP_DELETE_DISK                80
#define IDS_HELP_DELETE_PARTITION           81
#define IDS_HELP_DELETE_VOLUME              82

#define IDS_HELP_DETACH                     83

#define IDS_HELP_DETAIL                     84
#define IDS_HELP_DETAIL_DISK                85
#define IDS_HELP_DETAIL_PARTITION           86
#define IDS_HELP_DETAIL_VOLUME              87

#define IDS_HELP_EXIT                       88
#define IDS_HELP_EXPAND                     89
#define IDS_HELP_EXTEND                     90
#define IDS_HELP_FILESYSTEMS                91
#define IDS_HELP_FORMAT                     92
#define IDS_HELP_GPT                        93
#define IDS_HELP_HELP                       94
#define IDS_HELP_IMPORT                     95
#define IDS_HELP_INACTIVE                   96

#define IDS_HELP_LIST                       97
#define IDS_HELP_LIST_DISK                  98
#define IDS_HELP_LIST_PARTITION             99
#define IDS_HELP_LIST_VOLUME               100
#define IDS_HELP_LIST_VDISK                101

#define IDS_HELP_MERGE                     102
#define IDS_HELP_ONLINE                    103
#define IDS_HELP_OFFLINE                   104
#define IDS_HELP_RECOVER                   105
#define IDS_HELP_REM                       106
#define IDS_HELP_REMOVE                    107
#define IDS_HELP_REPAIR                    108
#define IDS_HELP_RESCAN                    109
#define IDS_HELP_RETAIN                    110
#define IDS_HELP_SAN                       111

#define IDS_HELP_SELECT                    112
#define IDS_HELP_SELECT_DISK               113
#define IDS_HELP_SELECT_PARTITION          114
#define IDS_HELP_SELECT_VOLUME             115
#define IDS_HELP_SELECT_VDISK              116

#define IDS_HELP_SETID                     117
#define IDS_HELP_SHRINK                    118

#define IDS_HELP_UNIQUEID                  119
#define IDS_HELP_UNIQUEID_DISK             120

#define IDS_ERROR_MSG_NO_SCRIPT  5000
#define IDS_ERROR_MSG_BAD_ARG    5001
#define IDS_ERROR_INVALID_ARGS   5002
#define IDS_ERROR_NO_MEDIUM      5003

#define IDS_BUSTYPE_UNKNOWN      5100
#define IDS_BUSTYPE_SCSI         5101
#define IDS_BUSTYPE_ATAPI        5102
#define IDS_BUSTYPE_ATA          5103
#define IDS_BUSTYPE_1394         5104
#define IDS_BUSTYPE_SSA          5105
#define IDS_BUSTYPE_FIBRE        5106
#define IDS_BUSTYPE_USB          5107
#define IDS_BUSTYPE_RAID         5108
#define IDS_BUSTYPE_ISCSI        5109
#define IDS_BUSTYPE_SAS          5110
#define IDS_BUSTYPE_SATA         5111
#define IDS_BUSTYPE_SD           5112
#define IDS_BUSTYPE_MMC          5113
#define IDS_BUSTYPE_VIRTUAL      5114
#define IDS_BUSTYPE_FBV          5115
#define IDS_BUSTYPE_SPACES       5116
#define IDS_BUSTYPE_NVME         5117
#define IDS_BUSTYPE_SCM          5118
#define IDS_BUSTYPE_UFS          5119
#define IDS_BUSTYPE_NVMEOF       5120
#define IDS_BUSTYPE_OTHER        5121

#define IDS_PARTITION_TYPE_EXTENDED     5200
#define IDS_PARTITION_TYPE_LOGICAL      5201
#define IDS_PARTITION_TYPE_PRIMARY      5202
#define IDS_PARTITION_TYPE_RESERVED     5203
#define IDS_PARTITION_TYPE_SYSTEM       5204
#define IDS_PARTITION_TYPE_UNKNOWN      5205
#define IDS_PARTITION_TYPE_UNUSED       5206

#define IDS_VOLUME_TYPE_DVD             5250
#define IDS_VOLUME_TYPE_PARTITION       5251
#define IDS_VOLUME_TYPE_REMOVABLE       5252
#define IDS_VOLUME_TYPE_UNKNOWN         5253

#define IDS_UNIT_TB                     5260
#define IDS_UNIT_GB                     5261
#define IDS_UNIT_MB                     5262
#define IDS_UNIT_KB                     5263
#define IDS_UNIT_B                      5264
