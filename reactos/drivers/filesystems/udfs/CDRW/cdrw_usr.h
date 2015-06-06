////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*

Module Name:

    cdrw_usr.h

Abstract:

    This file defines the interface between CD-Writer's driver & user
    applications.

Environment:

    NT kernel mode or Win32 app
 */

#ifndef __CDRW_PUBLIC_H__
#define __CDRW_PUBLIC_H__

#define CDRW_SIGNATURE_v1 "ALWA CD-R/W v1"

//#define CDRW_RESTRICT_ACCESS    // require W-acces rights for some IOCTLs

#include "cdrw_hw.h"
//#include "ntdddisk.h"

#ifndef CTL_CODE
#pragma pack(push, 8)
#include "winioctl.h"
#pragma pack(pop)
#endif

#if defined(CDRW_EXPORTS) || defined(FileSys_EXPORTS)
#include "mountmgr.h"
#endif

#ifndef FILE_DEVICE_SECURE_OPEN
#define FILE_DEVICE_SECURE_OPEN 0x00000100
#endif //FILE_DEVICE_SECURE_OPEN

#pragma pack(push, 1)

#ifndef IRP_MJ_PNP
#define IRP_MJ_PNP      IRP_MJ_PNP_POWER // Obsolete....
#endif //IRP_MJ_PNP

#ifndef FILE_DEVICE_CDRW
#define FILE_DEVICE_CDRW        0x00000999
#endif

#ifndef FILE_DEVICE_CD_ROM
#define FILE_DEVICE_CD_ROM      0x00000002
#endif  //FILE_DEVICE_CD_ROM

#ifndef IOCTL_CDROM_BASE
#define IOCTL_CDROM_BASE        FILE_DEVICE_CD_ROM
#endif  //IOCTL_CDROM_BASE

#ifndef FILE_DEVICE_DVD
#define FILE_DEVICE_DVD         0x00000033
#endif  //FILE_DEVICE_DVD

#ifndef IOCTL_DVD_BASE
#define IOCTL_DVD_BASE          FILE_DEVICE_DVD
#endif  //IOCTL_DVD_BASE

#ifndef FILE_DEVICE_DISK
#define FILE_DEVICE_DISK        0x00000007
#endif  //FILE_DEVICE_DISK

#ifndef IOCTL_DISK_BASE
#define IOCTL_DISK_BASE         FILE_DEVICE_DISK
#endif  //IOCTL_DISK_BASE

#ifndef IOCTL_CDROM_UNLOAD_DRIVER
#define IOCTL_CDROM_UNLOAD_DRIVER    CTL_CODE(IOCTL_CDROM_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_CDROM_READ_TOC         CTL_CODE(IOCTL_CDROM_BASE, 0x0000, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_CONTROL      CTL_CODE(IOCTL_CDROM_BASE, 0x000D, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_PLAY_AUDIO_MSF   CTL_CODE(IOCTL_CDROM_BASE, 0x0006, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_SEEK_AUDIO_MSF   CTL_CODE(IOCTL_CDROM_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_STOP_AUDIO       CTL_CODE(IOCTL_CDROM_BASE, 0x0002, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_PAUSE_AUDIO      CTL_CODE(IOCTL_CDROM_BASE, 0x0003, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RESUME_AUDIO     CTL_CODE(IOCTL_CDROM_BASE, 0x0004, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_VOLUME       CTL_CODE(IOCTL_CDROM_BASE, 0x0005, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_SET_VOLUME       CTL_CODE(IOCTL_CDROM_BASE, 0x000A, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_READ_Q_CHANNEL   CTL_CODE(IOCTL_CDROM_BASE, 0x000B, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_LAST_SESSION CTL_CODE(IOCTL_CDROM_BASE, 0x000E, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RAW_READ         CTL_CODE(IOCTL_CDROM_BASE, 0x000F, METHOD_OUT_DIRECT,  FILE_READ_ACCESS)
#define IOCTL_CDROM_DISK_TYPE        CTL_CODE(IOCTL_CDROM_BASE, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_CDROM_CHECK_VERIFY     CTL_CODE(IOCTL_CDROM_BASE, 0x0200, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_MEDIA_REMOVAL    CTL_CODE(IOCTL_CDROM_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_EJECT_MEDIA      CTL_CODE(IOCTL_CDROM_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_LOAD_MEDIA       CTL_CODE(IOCTL_CDROM_BASE, 0x0203, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RESERVE          CTL_CODE(IOCTL_CDROM_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RELEASE          CTL_CODE(IOCTL_CDROM_BASE, 0x0205, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_FIND_NEW_DEVICES CTL_CODE(IOCTL_CDROM_BASE, 0x0206, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_CDROM_GET_DRIVE_GEOMETRY    CTL_CODE(IOCTL_CDROM_BASE, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX CTL_CODE(IOCTL_CDROM_BASE, 0x0014, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_CDROM_READ_TOC_EX           CTL_CODE(IOCTL_CDROM_BASE, 0x0015, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_CONFIGURATION     CTL_CODE(IOCTL_CDROM_BASE, 0x0016, METHOD_BUFFERED, FILE_READ_ACCESS)

#endif  //IOCTL_CDROM_UNLOAD_DRIVER

#ifdef CDRW_RESTRICT_ACCESS

#define CDRW_CTL_CODE_R(a,b)    CTL_CODE(FILE_DEVICE_CDRW, a,b, FILE_READ_DATA)
#define CDRW_CTL_CODE_W(a,b)    CTL_CODE(FILE_DEVICE_CDRW, a,b, FILE_READ_DATA)
#define CDRW_CTL_CODE_W(a,b)    CTL_CODE(FILE_DEVICE_CDRW, a,b, FILE_WRITE_DATA )
#define CDRW_CTL_CODE_A(a,b)    CTL_CODE(FILE_DEVICE_CDRW, a,b, FILE_READ_DATA | FILE_WRITE_DATA )

#else //CDRW_RESTRICT_ACCESS

#define CDRW_CTL_CODE_R(a,b)    CTL_CODE(FILE_DEVICE_CDRW, a,b, FILE_READ_DATA)
#define CDRW_CTL_CODE_W(a,b)    CTL_CODE(FILE_DEVICE_CDRW, a,b, FILE_READ_DATA)
#define CDRW_CTL_CODE_A(a,b)    CTL_CODE(FILE_DEVICE_CDRW, a,b, FILE_READ_DATA)
#define CDRW_CTL_CODE_X(a,b)    CTL_CODE(FILE_DEVICE_CDRW, a,b, FILE_ANY_ACCESS )

#endif //CDRW_RESTRICT_ACCESS

#define IOCTL_CDRW_LOCK_DOOR            CDRW_CTL_CODE_R(0x801, METHOD_BUFFERED)
#define IOCTL_CDRW_SET_SPEED            CDRW_CTL_CODE_R(0x802, METHOD_BUFFERED)
#define IOCTL_CDRW_SYNC_CACHE           CDRW_CTL_CODE_W(0x803, METHOD_BUFFERED)
#define IOCTL_CDRW_GET_CAPABILITIES     CDRW_CTL_CODE_X(0x804, METHOD_BUFFERED)
#define IOCTL_CDRW_GET_SPEED            IOCTL_CDRW_GET_CAPABILITIES
#define IOCTL_CDRW_GET_MEDIA_TYPE       CDRW_CTL_CODE_X(0x805, METHOD_BUFFERED)
#define IOCTL_CDRW_GET_WRITE_MODE       CDRW_CTL_CODE_R(0x806, METHOD_BUFFERED)
#define IOCTL_CDRW_SET_WRITE_MODE       CDRW_CTL_CODE_W(0x807, METHOD_BUFFERED)
#define IOCTL_CDRW_RESERVE_TRACK        CDRW_CTL_CODE_W(0x808, METHOD_BUFFERED)
#define IOCTL_CDRW_BLANK                CDRW_CTL_CODE_R(0x809, METHOD_BUFFERED)
#define IOCTL_CDRW_CLOSE_TRK_SES        CDRW_CTL_CODE_W(0x80a, METHOD_BUFFERED)
//#define IOCTL_CDRW_LL_WRITE             CDRW_CTL_CODE_R(0x80b, METHOD_OUT_DIRECT)
#define IOCTL_CDRW_LL_WRITE             CDRW_CTL_CODE_R(0x80b, METHOD_BUFFERED)
#define IOCTL_CDRW_READ_TRACK_INFO      CDRW_CTL_CODE_R(0x80c, METHOD_IN_DIRECT)
#define IOCTL_CDRW_READ_DISC_INFO       CDRW_CTL_CODE_R(0x80d, METHOD_IN_DIRECT)
#define IOCTL_CDRW_BUFFER_CAPACITY      CDRW_CTL_CODE_A(0x80e, METHOD_IN_DIRECT)
#define IOCTL_CDRW_GET_SIGNATURE        CDRW_CTL_CODE_X(0x80f, METHOD_BUFFERED)
#define IOCTL_CDRW_RESET_DRIVER         CDRW_CTL_CODE_A(0x810, METHOD_BUFFERED)
//#ifndef WITHOUT_FORMATTER
#define IOCTL_CDRW_FORMAT_UNIT          CDRW_CTL_CODE_W(0x811, METHOD_BUFFERED)
#define IOCTL_CDRW_SET_RANDOM_ACCESS    CDRW_CTL_CODE_W(0x812, METHOD_BUFFERED)
//#endif //WITHOUT_FORMATTER
#define IOCTL_CDRW_TEST_UNIT_READY      CDRW_CTL_CODE_X(0x813, METHOD_BUFFERED)
#define IOCTL_CDRW_RESET_WRITE_STATUS   CDRW_CTL_CODE_X(0x814, METHOD_BUFFERED)
#define IOCTL_CDRW_GET_LAST_ERROR       CDRW_CTL_CODE_R(0x815, METHOD_BUFFERED)
#define IOCTL_CDRW_MODE_SENSE           CDRW_CTL_CODE_X(0x816, METHOD_BUFFERED)
#define IOCTL_CDRW_MODE_SELECT          CDRW_CTL_CODE_R(0x817, METHOD_BUFFERED)
#define IOCTL_CDRW_SET_READ_AHEAD       CDRW_CTL_CODE_R(0x818, METHOD_BUFFERED)
#define IOCTL_CDRW_SET_DEFAULT_SESSION  CDRW_CTL_CODE_R(0x819, METHOD_BUFFERED) // RESERVED !!!
#define IOCTL_CDRW_NOTIFY_MEDIA_CHANGE  CDRW_CTL_CODE_X(0x81a, METHOD_BUFFERED)
#define IOCTL_CDRW_SEND_OPC_INFO        CDRW_CTL_CODE_W(0x81b, METHOD_BUFFERED)
#define IOCTL_CDRW_LL_READ              CDRW_CTL_CODE_R(0x81c, METHOD_BUFFERED)
#define IOCTL_CDRW_SEND_CUE_SHEET       CDRW_CTL_CODE_W(0x81d, METHOD_OUT_DIRECT)
#define IOCTL_CDRW_INIT_DEINIT          CDRW_CTL_CODE_A(0x81e, METHOD_BUFFERED)
#define IOCTL_CDRW_READ_FULL_TOC        CDRW_CTL_CODE_R(0x81f, METHOD_BUFFERED)
#define IOCTL_CDRW_READ_PMA             CDRW_CTL_CODE_R(0x820, METHOD_BUFFERED)
#define IOCTL_CDRW_READ_SESSION_INFO    CDRW_CTL_CODE_R(0x821, METHOD_BUFFERED)
#define IOCTL_CDRW_READ_ATIP            CDRW_CTL_CODE_R(0x822, METHOD_BUFFERED)
#define IOCTL_CDRW_READ_CD_TEXT         CDRW_CTL_CODE_R(0x823, METHOD_BUFFERED)
#define IOCTL_CDRW_READ_TOC_EX          CDRW_CTL_CODE_R(0x824, METHOD_BUFFERED)
#define IOCTL_CDRW_GET_DEVICE_INFO      CDRW_CTL_CODE_R(0x825, METHOD_BUFFERED)
#define IOCTL_CDRW_GET_EVENT            CDRW_CTL_CODE_R(0x826, METHOD_IN_DIRECT)
#define IOCTL_CDRW_GET_DEVICE_NAME      CDRW_CTL_CODE_R(0x827, METHOD_BUFFERED)
#define IOCTL_CDRW_RESET_DRIVER_EX      CDRW_CTL_CODE_A(0x828, METHOD_BUFFERED)
#define IOCTL_CDRW_GET_MEDIA_TYPE_EX    CDRW_CTL_CODE_X(0x829, METHOD_BUFFERED)
#ifndef WITHOUT_FORMATTER
#define IOCTL_CDRW_GET_MRW_MODE         CDRW_CTL_CODE_X(0x82a, METHOD_BUFFERED)
#define IOCTL_CDRW_SET_MRW_MODE         CDRW_CTL_CODE_X(0x82b, METHOD_BUFFERED)
#endif //WITHOUT_FORMATTER
#define IOCTL_CDRW_READ_CAPACITY        CDRW_CTL_CODE_R(0x82c, METHOD_IN_DIRECT)
#define IOCTL_CDRW_GET_DISC_LAYOUT      CDRW_CTL_CODE_R(0x82d, METHOD_IN_DIRECT)
#define IOCTL_CDRW_SET_STREAMING        CDRW_CTL_CODE_W(0x82e, METHOD_BUFFERED)

#define IOCTL_CDRW_UNLOAD_DRIVER     IOCTL_CDROM_UNLOAD_DRIVER

#define IOCTL_CDRW_READ_TOC          IOCTL_CDROM_READ_TOC
#define IOCTL_CDRW_GET_CONTROL       IOCTL_CDROM_GET_CONTROL
#define IOCTL_CDRW_PLAY_AUDIO_MSF    IOCTL_CDROM_PLAY_AUDIO_MSF
#define IOCTL_CDRW_SEEK_AUDIO_MSF    IOCTL_CDROM_SEEK_AUDIO_MSF
#define IOCTL_CDRW_STOP_AUDIO        IOCTL_CDROM_STOP_AUDIO
#define IOCTL_CDRW_PAUSE_AUDIO       IOCTL_CDROM_PAUSE_AUDIO
#define IOCTL_CDRW_RESUME_AUDIO      IOCTL_CDROM_RESUME_AUDIO
#define IOCTL_CDRW_GET_VOLUME        IOCTL_CDROM_GET_VOLUME
#define IOCTL_CDRW_SET_VOLUME        IOCTL_CDROM_SET_VOLUME
#define IOCTL_CDRW_READ_Q_CHANNEL    IOCTL_CDROM_READ_Q_CHANNEL
#define IOCTL_CDRW_GET_LAST_SESSION  IOCTL_CDROM_GET_LAST_SESSION 
#define IOCTL_CDRW_RAW_READ          IOCTL_CDROM_RAW_READ
#define IOCTL_CDRW_DISK_TYPE         IOCTL_CDROM_DISK_TYPE

#define IOCTL_CDRW_CHECK_VERIFY      IOCTL_CDROM_CHECK_VERIFY
#define IOCTL_CDRW_MEDIA_REMOVAL     IOCTL_CDROM_MEDIA_REMOVAL
#define IOCTL_CDRW_EJECT_MEDIA       IOCTL_CDROM_EJECT_MEDIA
#define IOCTL_CDRW_LOAD_MEDIA        IOCTL_CDROM_LOAD_MEDIA
#define IOCTL_CDRW_RESERVE           IOCTL_CDROM_RESERVE
#define IOCTL_CDRW_RELEASE           IOCTL_CDROM_RELEASE
#define IOCTL_CDRW_FIND_NEW_DEVICES  IOCTL_CDROM_FIND_NEW_DEVICES

#ifndef IOCTL_DVD_READ_STRUCTURE
#define IOCTL_DVD_READ_STRUCTURE     CTL_CODE(IOCTL_DVD_BASE, 0x0450, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_DVD_START_SESSION      CTL_CODE(IOCTL_DVD_BASE, 0x0400, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_READ_KEY           CTL_CODE(IOCTL_DVD_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_SEND_KEY           CTL_CODE(IOCTL_DVD_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_END_SESSION        CTL_CODE(IOCTL_DVD_BASE, 0x0403, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_SET_READ_AHEAD     CTL_CODE(IOCTL_DVD_BASE, 0x0404, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_GET_REGION         CTL_CODE(IOCTL_DVD_BASE, 0x0405, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_SEND_KEY2          CTL_CODE(IOCTL_DVD_BASE, 0x0406, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif  //IOCTL_DVD_READ_STRUCTURE

#ifndef IOCTL_DISK_GET_DRIVE_GEOMETRY
#define IOCTL_DISK_GET_DRIVE_GEOMETRY   CTL_CODE(IOCTL_DISK_BASE, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_VERIFY               CTL_CODE(IOCTL_DISK_BASE, 0x0005, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_FORMAT_TRACKS        CTL_CODE(IOCTL_DISK_BASE, 0x0006, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_IS_WRITABLE          CTL_CODE(IOCTL_DISK_BASE, 0x0009, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_FORMAT_TRACKS_EX     CTL_CODE(IOCTL_DISK_BASE, 0x000b, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_DISK_CHECK_VERIFY     CTL_CODE(IOCTL_DISK_BASE, 0x0200, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_MEDIA_REMOVAL    CTL_CODE(IOCTL_DISK_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_EJECT_MEDIA      CTL_CODE(IOCTL_DISK_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_LOAD_MEDIA       CTL_CODE(IOCTL_DISK_BASE, 0x0203, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_RESERVE          CTL_CODE(IOCTL_DISK_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_RELEASE          CTL_CODE(IOCTL_DISK_BASE, 0x0205, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_FIND_NEW_DEVICES CTL_CODE(IOCTL_DISK_BASE, 0x0206, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_GET_MEDIA_TYPES  CTL_CODE(IOCTL_DISK_BASE, 0x0300, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif  //IOCTL_DISK_GET_DRIVE_GEOMETRY

#ifndef IOCTL_STORAGE_SET_READ_AHEAD
#define IOCTL_STORAGE_SET_READ_AHEAD CTL_CODE(IOCTL_STORAGE_BASE, 0x0100, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif  //IOCTL_STORAGE_SET_READ_AHEAD

#ifndef IOCTL_STORAGE_GET_MEDIA_TYPES_EX
#define IOCTL_STORAGE_GET_MEDIA_TYPES_EX CTL_CODE(IOCTL_STORAGE_BASE, 0x0301, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef enum _STORAGE_MEDIA_TYPE {
    //
    // Following are defined in ntdddisk.h in the MEDIA_TYPE enum
    //
    // Unknown,                // Format is unknown
    // F5_1Pt2_512,            // 5.25", 1.2MB,  512 bytes/sector
    // F3_1Pt44_512,           // 3.5",  1.44MB, 512 bytes/sector
    // F3_2Pt88_512,           // 3.5",  2.88MB, 512 bytes/sector
    // F3_20Pt8_512,           // 3.5",  20.8MB, 512 bytes/sector
    // F3_720_512,             // 3.5",  720KB,  512 bytes/sector
    // F5_360_512,             // 5.25", 360KB,  512 bytes/sector
    // F5_320_512,             // 5.25", 320KB,  512 bytes/sector
    // F5_320_1024,            // 5.25", 320KB,  1024 bytes/sector
    // F5_180_512,             // 5.25", 180KB,  512 bytes/sector
    // F5_160_512,             // 5.25", 160KB,  512 bytes/sector
    // RemovableMedia,         // Removable media other than floppy
    // FixedMedia,             // Fixed hard disk media
    // F3_120M_512,            // 3.5", 120M Floppy
    // F3_640_512,             // 3.5" ,  640KB,  512 bytes/sector
    // F5_640_512,             // 5.25",  640KB,  512 bytes/sector
    // F5_720_512,             // 5.25",  720KB,  512 bytes/sector
    // F3_1Pt2_512,            // 3.5" ,  1.2Mb,  512 bytes/sector
    // F3_1Pt23_1024,          // 3.5" ,  1.23Mb, 1024 bytes/sector
    // F5_1Pt23_1024,          // 5.25",  1.23MB, 1024 bytes/sector
    // F3_128Mb_512,           // 3.5" MO 128Mb   512 bytes/sector
    // F3_230Mb_512,           // 3.5" MO 230Mb   512 bytes/sector
    // F8_256_128,             // 8",     256KB,  128 bytes/sector
    //

    DDS_4mm = 0x20,            // Tape - DAT DDS1,2,... (all vendors)
    MiniQic,                   // Tape - miniQIC Tape
    Travan,                    // Tape - Travan TR-1,2,3,...
    QIC,                       // Tape - QIC
    MP_8mm,                    // Tape - 8mm Exabyte Metal Particle
    AME_8mm,                   // Tape - 8mm Exabyte Advanced Metal Evap
    AIT1_8mm,                  // Tape - 8mm Sony AIT1
    DLT,                       // Tape - DLT Compact IIIxt, IV
    NCTP,                      // Tape - Philips NCTP
    IBM_3480,                  // Tape - IBM 3480
    IBM_3490E,                 // Tape - IBM 3490E
    IBM_Magstar_3590,          // Tape - IBM Magstar 3590
    IBM_Magstar_MP,            // Tape - IBM Magstar MP
    STK_DATA_D3,               // Tape - STK Data D3
    SONY_DTF,                  // Tape - Sony DTF
    DV_6mm,                    // Tape - 6mm Digital Video
    DMI,                       // Tape - Exabyte DMI and compatibles
    SONY_D2,                   // Tape - Sony D2S and D2L
    CLEANER_CARTRIDGE,         // Cleaner - All Drive types that support Drive Cleaners
    CD_ROM,                    // Opt_Disk - CD
    CD_R,                      // Opt_Disk - CD-Recordable (Write Once)
    CD_RW,                     // Opt_Disk - CD-Rewriteable
    DVD_ROM,                   // Opt_Disk - DVD-ROM
    DVD_R,                     // Opt_Disk - DVD-Recordable (Write Once)
    DVD_RW,                    // Opt_Disk - DVD-Rewriteable
    MO_3_RW,                   // Opt_Disk - 3.5" Rewriteable MO Disk
    MO_5_WO,                   // Opt_Disk - MO 5.25" Write Once
    MO_5_RW,                   // Opt_Disk - MO 5.25" Rewriteable (not LIMDOW)
    MO_5_LIMDOW,               // Opt_Disk - MO 5.25" Rewriteable (LIMDOW)
    PC_5_WO,                   // Opt_Disk - Phase Change 5.25" Write Once Optical
    PC_5_RW,                   // Opt_Disk - Phase Change 5.25" Rewriteable
    PD_5_RW,                   // Opt_Disk - PhaseChange Dual Rewriteable
    ABL_5_WO,                  // Opt_Disk - Ablative 5.25" Write Once Optical
    PINNACLE_APEX_5_RW,        // Opt_Disk - Pinnacle Apex 4.6GB Rewriteable Optical
    SONY_12_WO,                // Opt_Disk - Sony 12" Write Once
    PHILIPS_12_WO,             // Opt_Disk - Philips/LMS 12" Write Once
    HITACHI_12_WO,             // Opt_Disk - Hitachi 12" Write Once
    CYGNET_12_WO,              // Opt_Disk - Cygnet/ATG 12" Write Once
    KODAK_14_WO,               // Opt_Disk - Kodak 14" Write Once
    MO_NFR_525,                // Opt_Disk - Near Field Recording (Terastor)
    NIKON_12_RW,               // Opt_Disk - Nikon 12" Rewriteable
    IOMEGA_ZIP,                // Mag_Disk - Iomega Zip
    IOMEGA_JAZ,                // Mag_Disk - Iomega Jaz
    SYQUEST_EZ135,             // Mag_Disk - Syquest EZ135
    SYQUEST_EZFLYER,           // Mag_Disk - Syquest EzFlyer
    SYQUEST_SYJET,             // Mag_Disk - Syquest SyJet
    AVATAR_F2,                 // Mag_Disk - 2.5" Floppy
    MP2_8mm,                   // Tape - 8mm Hitachi
    DST_S,                     // Ampex DST Small Tapes
    DST_M,                     // Ampex DST Medium Tapes
    DST_L,                     // Ampex DST Large Tapes
    VXATape_1,                 // Ecrix 8mm Tape
    VXATape_2,                 // Ecrix 8mm Tape
    STK_EAGLE,                 // STK Eagle
    LTO_Ultrium,               // IBM, HP, Seagate LTO Ultrium
    LTO_Accelis                // IBM, HP, Seagate LTO Accelis
} STORAGE_MEDIA_TYPE, *PSTORAGE_MEDIA_TYPE;

#endif  //IOCTL_STORAGE_GET_MEDIA_TYPES_EX


//**********************************************************************************************

typedef struct _SET_CD_SPEED_USER_IN {
    ULONG ReadSpeed;        // Kbyte/sec        176 = 1X
    ULONG WriteSpeed;       // Kbyte/sec
} SET_CD_SPEED_USER_IN, *PSET_CD_SPEED_USER_IN;

typedef struct _SET_CD_SPEED_EX_USER_IN {
    ULONG ReadSpeed;        // Kbyte/sec        176 = 1X
    ULONG WriteSpeed;       // Kbyte/sec
    UCHAR RotCtrl;
    UCHAR Reserved[3];
} SET_CD_SPEED_EX_USER_IN, *PSET_CD_SPEED_EX_USER_IN;

//**********************************************************************************************

typedef struct _SET_STREAMING_USER_IN {
    union {
        UCHAR Flags;
        struct {
            UCHAR RA    : 1; // Random Access
            UCHAR Exact : 1;
            UCHAR RDD   : 1; // Restore Defaults
            UCHAR WRC   : 2;
            UCHAR Reserved0 : 3;
        } Fields;
    } Options;

    UCHAR Reserved1[3];

    ULONG StartLBA;
    ULONG EndLBA;

    ULONG ReadSize;  // KBytes
    ULONG ReadTime;  // ms

    ULONG WriteSize; // KBytes
    ULONG WriteTime; // ms    
} SET_STREAMING_USER_IN, *PSET_STREAMING_USER_IN;

//**********************************************************************************************

/*
#ifndef SyncCache_RELADR
#define SyncCache_RELADR        0x01
#define SyncCache_Immed         0x02
#endif //SyncCache_RELADR
*/

typedef struct _SYNC_CACHE_USER_IN {
    union {
        UCHAR Flags;
        struct {
            UCHAR RELADR    : 1;
            UCHAR Immed     : 1;
            UCHAR Reserved0 : 6;
        } Fields;
    } Byte1;
    ULONG LBA;
    USHORT NumOfBlocks;
} SYNC_CACHE_USER_IN, *PSYNC_CACHE_USER_IN;

//**********************************************************************************************

typedef struct _BUFFER_CAPACITY_BLOCK_USER_OUT {
    UCHAR Reserved0[4];
    ULONG BufferLength;
    ULONG BlankBufferLength;
    ULONG WriteStatus;
    BOOLEAN LostStreaming;
    ULONG MaximumPhysicalPages;
    ULONG MaximumTransferLength;
    ULONG ActualMaximumTransferLength;
} BUFFER_CAPACITY_BLOCK_USER_OUT, *PBUFFER_CAPACITY_BLOCK_USER_OUT;

//**********************************************************************************************

typedef struct _TRACK_INFO_BLOCK_USER_OUT {
    USHORT DataLength;
    UCHAR _TrackNum;
    UCHAR _SesNum;
    UCHAR Reserved0;

    union {
        UCHAR Flags;
        struct {
            UCHAR TrackMode : 4;
            UCHAR Copy      : 1;
            UCHAR Damage    : 1;
            UCHAR Reserved1 : 2; 
        } Fields;
    } TrackParam;

    union {
        UCHAR Flags;
        struct {
            UCHAR DataMode  : 4;
            UCHAR FP        : 1;
            UCHAR Packet    : 1;
            UCHAR Blank     : 1;
            UCHAR RT        : 1;
        } Fields;
    } DataParam;

    UCHAR NWA_V;
    ULONG TrackStartLBA;
    ULONG NextWriteLBA;
    ULONG FreeBlocks;
    ULONG FixPacketSize;
    ULONG TrackLength;

// MMC-3

    ULONG LastRecordedAddr;
    UCHAR _TrackNum2;  // MSB
    UCHAR _SesNum2;    // MSB
    UCHAR Reserved2[2];

// MMC-5

    ULONG ReadCompatLBA;

// Additional

    UCHAR TrackStartMSF[3];
    UCHAR NextWriteMSF[3];
    ULONG TrackNum;
    ULONG SesNum;

} TRACK_INFO_BLOCK_USER_OUT, *PTRACK_INFO_BLOCK_USER_OUT;

//**********************************************************************************************

typedef struct _TRACK_INFO_BLOCK_USER_IN {
        BOOLEAN Track;
        ULONG LBA_TrkNum;
} TRACK_INFO_BLOCK_USER_IN, *PTRACK_INFO_BLOCK_USER_IN;

//**********************************************************************************************

typedef READ_CAPACITY_DATA  READ_CAPACITY_USER_OUT;
typedef PREAD_CAPACITY_DATA PREAD_CAPACITY_USER_OUT;

//**********************************************************************************************

typedef struct _GET_SIGNATURE_USER_OUT {
        ULONG MagicDword;
        USHORT VersiomMajor ;
        USHORT VersiomMinor ;
        USHORT VersiomId ;          // alfa/beta/...
        USHORT Reserved;
        UCHAR VendorId[32];
} GET_SIGNATURE_USER_OUT, *PGET_SIGNATURE_USER_OUT;

//**********************************************************************************************

/*
#ifndef BlankMedia_Mask
#define BlankMedia_Mask             0x07
#define BlankMedia_Complete         0x00
#define BlankMedia_Minimal          0x01
#define BlankMedia_Track            0x02
#define BlankMedia_UnreserveTrack   0x03
#define BlankMedia_TrackTail        0x04
#define BlankMedia_UncloseLastSes   0x05
#define BlankMedia_EraseSes         0x06
#define BlankMedia_Immed            0x10
#endif //BlankMedia_Mask
*/

typedef struct _BLANK_MEDIA_USER_IN {
    union {
        UCHAR Flags;
        ULONG Reserved;
        struct {
            UCHAR BlankType : 3;
            UCHAR Reserved0 : 1;
            UCHAR Immed     : 1;
            UCHAR Reserved1 : 3;
        } Fields;
    } Byte1;
    ULONG StartAddr_TrkNum;
} BLANK_MEDIA_USER_IN, *PBLANK_MEDIA_USER_IN;

//**********************************************************************************************

typedef struct _RESERVE_TRACK_USER_IN {
    ULONG Size;
} RESERVE_TRACK_USER_IN, *PRESERVE_TRACK_USER_IN;

#define RESERVE_TRACK_EX_SIZE          0x0
#define RESERVE_TRACK_EX_START_LBA     0x1
#define RESERVE_TRACK_EX_RMZ           0x2

typedef struct _RESERVE_TRACK_EX_USER_IN {
    ULONG Flags;
    union {
        ULONG Size;
        ULONG StartLBA;
    };
} RESERVE_TRACK_EX_USER_IN, *PRESERVE_TRACK_EX_USER_IN;

//**********************************************************************************************

typedef struct _LL_WRITE_USER_IN {
    union {
        UCHAR Flags;
        ULONG Reserved;
        struct {
            UCHAR RELADR    : 1;
            UCHAR Reserved0 : 2;
            UCHAR FUA       : 1;
            UCHAR DPO       : 1;
            UCHAR Reserved1 : 3;
        } Fields;
    } Flags;
    ULONG LBA;
    USHORT NumOfBlocks;
    UCHAR Reserved1[2];
} LL_WRITE_USER_IN, *PLL_WRITE_USER_IN;

//**********************************************************************************************

//#ifndef WITHOUT_FORMATTER

/*
#ifndef FormatDesc_Grow
#define FormatDesc_Grow     0x40
#define FormatDesc_Ses      0x80
#endif
*/

#define FORMAT_UNIT_FORCE_STD_MODE  0x80000000
#define FORMAT_UNIT_FORCE_FULL_FMT  0x40000000
#define FORMAT_UNIT_RESTART_MRW     0x01000000

typedef struct _FORMAT_CDRW_PARAMETERS_USER_IN {
    union {
        UCHAR Flags;
        ULONG FlagsEx;
        struct {
            UCHAR Reserved0: 6;
            UCHAR Grow: 1;
            UCHAR Ses: 1;
        } Fields;
    } Flags;
    LONG BlockCount;
} FORMAT_CDRW_PARAMETERS_USER_IN, *PFORMAT_CDRW_PARAMETERS_USER_IN;

//#endif //WITHOUT_FORMATTER

//**********************************************************************************************

/*
#ifndef CloseTrkSes_Immed
#define CloseTrkSes_Immed   0x01

#define CloseTrkSes_Trk   0x01
#define CloseTrkSes_Ses   0x02

#define CloseTrkSes_LastTrkSes  0xff
#endif
*/

typedef struct _CLOSE_TRK_SES_USER_IN {
    union {
        UCHAR Flags;
        struct {
            UCHAR Immed     : 1;
            UCHAR Reserved0 : 7;
        } Fields;
    } Byte1;
    union {
        UCHAR Flags;
        struct {
            UCHAR Track     : 1;
            UCHAR Session   : 1;
            UCHAR Reserved0 : 6;
        } Fields;
    } Byte2;
    UCHAR TrackNum;
} CLOSE_TRK_SES_USER_IN, *PCLOSE_TRK_SES_USER_IN;

//**********************************************************************************************

typedef struct _PREVENT_MEDIA_REMOVAL_USER_IN {
    BOOLEAN PreventMediaRemoval;
} PREVENT_MEDIA_REMOVAL_USER_IN, *PPREVENT_MEDIA_REMOVAL_USER_IN;

//**********************************************************************************************

typedef struct _SET_RANDOM_ACCESS_USER_IN {
    BOOLEAN RandomAccessMode;
} SET_RANDOM_ACCESS_USER_IN, *PSET_RANDOM_ACCESS_USER_IN;

//**********************************************************************************************

/*
#ifndef DiscInfo_Disk_Mask
#define DiscInfo_Disk_Mask          0x03
#define DiscInfo_Disk_Empty         0x00
#define DiscInfo_Disk_Appendable    0x01
#define DiscInfo_Disk_Complete      0x02

#define DiscInfo_Ses_Mask       0x0C
#define DiscInfo_Ses_Empty      0x00
#define DiscInfo_Ses_Incomplete 0x04
#define DiscInfo_Ses_Complete   0x0C

#define DiscInfo_Disk_Erasable  0x10

#define DiscInfo_URU            0x10
#define DiscInfo_DBC_V          0x20
#define DiscInfo_DID_V          0x40

#define DiscInfo_Type_cdrom     0x00    // CD-DA / CD-ROM
#define DiscInfo_Type_cdi       0x10    // CD-I
#define DiscInfo_Type_cdromxa   0x20    // CD-ROM XA
#define DiscInfo_Type_unknown   0xFF    // HZ ;)

#endif
*/

typedef struct _DISC_STATUS_INFO_USER_OUT {
    UCHAR ErrorCode;
    UCHAR SenseKey;
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
    UCHAR SrbStatus;
    BOOLEAN RandomAccessMode;
    BOOLEAN CUE_sent;
    UCHAR Flags;
    LARGE_INTEGER MediaChangeTime;

    UCHAR LastSesLeadInMSF[4];
    UCHAR LastSesLeadOutMSF[4];

    ULONG NumOfSes;
    ULONG FirstTrackNumLastSes;
    ULONG LastTrackNumLastSes;
    ULONG Reserved1;                            // this is used to align data

} DISC_STATUS_INFO_USER_OUT, *PDISC_STATUS_INFO_USER_OUT;

#define DiscStatus_Formattable    0x01

typedef struct _DISC_INFO_BLOCK_USER_OUT {        // 

    DISC_STATUS_INFO_USER_OUT Status;

    USHORT DataLength;

    union {
        UCHAR Flags;
        struct {
            UCHAR DiscStat : 2;
            UCHAR LastSesStat : 2;
            UCHAR Erasable : 1;
            UCHAR Reserved0: 3;
        } Fields;
    } DiscStat;

    UCHAR FirstTrackNum;
    UCHAR NumOfSes;
    UCHAR FirstTrackNumLastSes;
    UCHAR LastTrackNumLastSes;

    union {
        UCHAR Flags;
        struct {
            UCHAR Reserved1: 5;
            UCHAR URU      : 1;
            UCHAR DBC_V    : 1; // 0
            UCHAR DID_V    : 1;
        } Fields;
    } Flags;

    UCHAR DiskType;
    UCHAR NumOfSes2;              // MSB MMC-3
    UCHAR FirstTrackNumLastSes2;  // MSB MMC-3
    UCHAR LastTrackNumLastSes2;   // MSB MMC-3
    UCHAR DiskId [4];
    ULONG LastSesLeadInLBA;
    ULONG LastSesLeadOutLBA;
    UCHAR DiskBarCode [8];
    UCHAR Reserved3;
    UCHAR OPCNum;
} DISC_INFO_BLOCK_USER_OUT, *PDISC_INFO_BLOCK_USER_OUT;

//**********************************************************************************************

typedef struct _TEST_UNIT_READY_USER_IN {
    ULONG MaxReadyRetry;
} TEST_UNIT_READY_USER_IN, *PTEST_UNIT_READY_USER_IN;

typedef struct _TEST_UNIT_READY_USER_OUT {
    UCHAR ErrorCode;
    UCHAR SenseKey;
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
} TEST_UNIT_READY_USER_OUT, *PTEST_UNIT_READY_USER_OUT;

//**********************************************************************************************

/*
#ifndef MediaType_Unknown

#define MediaType_Unknown                        0x00
#define MediaType_120mm_CDROM_DataOnly           0x01
#define MediaType_120mm_CDROM_AudioOnly          0x02        //CDDA
#define MediaType_120mm_CDROM_DataAudioCombined  0x03
#define MediaType_120mm_CDROM_Hybrid_PhotoCD     0x04
#define MediaType_80mm_CDROM_DataOnly            0x05
#define MediaType_80mm_CDROM_AudioOnly           0x06        //CDDA
#define MediaType_80mm_CDROM_DataAudioCombined   0x07
#define MediaType_80mm_CDROM_Hybrid_PhotoCD      0x08

#define MediaType_UnknownSize_CDR                0x10
#define MediaType_120mm_CDR_DataOnly             0x11
#define MediaType_120mm_CDR_AudioOnly            0x12        //CDDA
#define MediaType_120mm_CDR_DataAudioCombined    0x13
#define MediaType_120mm_CDR_Hybrid_PhotoCD       0x14
#define MediaType_80mm_CDR_DataOnly              0x15
#define MediaType_80mm_CDR_AudioOnly             0x16        //CDDA
#define MediaType_80mm_CDR_DataAudioCombined     0x17
#define MediaType_80mm_CDR_Hybrid_Photo_CD       0x18

#define MediaType_UnknownSize_CDRW               0x20
#define MediaType_120mm_CDRW_DataOnly            0x21
#define MediaType_120mm_CDRW_AudioOnly           0x22        //CDDA
#define MediaType_120mm_CDRW_DataAudioCombined   0x23
#define MediaType_120mm_CDRW_Hybrid              0x24
#define MediaType_80mm_CDRW_DataOnly             0x25
#define MediaType_80mm_CDRW_AudioOnly            0x26        //CDDA
#define MediaType_80mm_CDRW_DataAudioCombined    0x27
#define MediaType_80mm_CDRW_Hybrid               0x28

#define MediaType_UnknownSize_Unknown            0x30

#define MediaType_NoDiscPresent                  0x70
#define MediaType_DoorOpen                       0x71

#endif
*/

typedef struct _GET_MEDIA_TYPE_USER_OUT {
    UCHAR MediaType;
} GET_MEDIA_TYPE_USER_OUT, *PGET_MEDIA_TYPE_USER_OUT;

typedef struct _GET_MEDIA_TYPE_EX_USER_OUT {
    UCHAR OldStyleMediaType; // see GET_MEDIA_TYPE_USER_OUT
    UCHAR MediaClass;
    UCHAR MediaSize;
    UCHAR DataType;
    UCHAR MediaClassEx;
    UCHAR DataClassEx;
    UCHAR CapFlags;
    UCHAR Layers;                    // Number of layers - 1 (e.g. 0 => 1 layer)
    UCHAR Reserved[8];               // for future implementation
} GET_MEDIA_TYPE_EX_USER_OUT, *PGET_MEDIA_TYPE_EX_USER_OUT;
 
#define CdMediaClass_CDROM         0x00
#define CdMediaClass_CDR           0x01
#define CdMediaClass_CDRW          0x02
#define CdMediaClass_DVDROM        0x03
#define CdMediaClass_DVDRAM        0x05
#define CdMediaClass_DVDR          0x06
#define CdMediaClass_DVDRW         0x07
#define CdMediaClass_DVDpR         0x08
#define CdMediaClass_DVDpRW        0x09
#define CdMediaClass_DDCDROM       0x0a
#define CdMediaClass_DDCDR         0x0b
#define CdMediaClass_DDCDRW        0x0c
#define CdMediaClass_BDROM         0x0d
#define CdMediaClass_BDRE          0x0e
#define CdMediaClass_BDR           0x0f
#define CdMediaClass_HD_DVDROM     0x10
#define CdMediaClass_HD_DVDRAM     0x11
#define CdMediaClass_HD_DVDR       0x12
#define CdMediaClass_HD_DVDRW      0x13
#define CdMediaClass_NoDiscPresent 0x70
#define CdMediaClass_DoorOpen      0x71
#define CdMediaClass_Unknown       0xff

#define CdMediaClass_Max           CdMediaClass_HD_DVDRW

#define CdMediaSize_Unknown        0
#define CdMediaSize_120mm          1
#define CdMediaSize_80mm           2

#define CdDataType_Unknown             0
#define CdDataType_DataOnly            1
#define CdDataType_AudioOnly           2        //CDDA
#define CdDataType_DataAudioCombined   3
#define CdDataType_Hybrid              4
#define CdDataType_DataOnlyMRW         5

#define CdMediaClassEx_CD          0x00
#define CdMediaClassEx_DVD         0x01
#define CdMediaClassEx_DDCD        0x02
#define CdMediaClassEx_BD          0x03
#define CdMediaClassEx_HD_DVD      0x04
#define CdMediaClassEx_None        0x70
#define CdMediaClassEx_Unknown     0xff

#define CdDataClassEx_ROM          0x00
#define CdDataClassEx_R            0x01
#define CdDataClassEx_RW           0x02
#define CdDataClassEx_Unknown      0xff

#define CdCapFlags_Writable        0x01
#define CdCapFlags_Erasable        0x02
#define CdCapFlags_Formatable      0x04
#define CdCapFlags_WriteParamsReq  0x08
#define CdCapFlags_RandomWritable  0x10
#define CdCapFlags_Cav             0x20

#define CdrwMediaClassEx_IsRAM(MediaClassEx) ( \
  ((MediaClassEx) == CdMediaClass_DVDRAM) || \
  ((MediaClassEx) == CdMediaClass_BDRE) || \
  ((MediaClassEx) == CdMediaClass_HD_DVDRAM) )

#define CdrwIsDvdOverwritable(MediaClassEx) \
          ((MediaClassEx) == CdMediaClass_DVDRW || \
           (MediaClassEx) == CdMediaClass_DVDpRW || \
           (MediaClassEx) == CdMediaClass_DVDRAM || \
           (MediaClassEx) == CdMediaClass_BDRE || \
           (MediaClassEx) == CdMediaClass_HD_DVDRW || \
           (MediaClassEx) == CdMediaClass_HD_DVDRAM \
          )

//**********************************************************************************************

/*
#ifndef MAX_PAGE_SIZE
#define MAX_PAGE_SIZE   0x100
#endif
*/

typedef struct _MODE_SENSE_USER_IN {
    union {
        UCHAR Byte;
        struct {
            UCHAR PageCode : 6;
            UCHAR Reserved0: 1;
            UCHAR PageSavable : 1;
        } Fields;
    } PageCode;
} MODE_SENSE_USER_IN, *PMODE_SENSE_USER_IN;

typedef struct _MODE_SENSE_USER_OUT {
    MODE_PARAMETER_HEADER   Header;
} MODE_SENSE_USER_OUT, *PMODE_SENSE_USER_OUT;

//**********************************************************************************************

typedef struct _MODE_SELECT_USER_IN {
    MODE_PARAMETER_HEADER   Header;
} MODE_SELECT_USER_IN, *PMODE_SELECT_USER_IN;

//**********************************************************************************************

typedef struct _MODE_WRITE_PARAMS_PAGE_USER {        // 0x05
    UCHAR PageCode : 6;
    UCHAR Reserved0: 1;
    UCHAR PageSavable : 1;

    UCHAR PageLength;               // 0x32

/*
#ifndef WParam_WType_Mask
#define WParam_WType_Mask   0x0f
#define WParam_WType_Packet 0x00
#define WParam_WType_TAO    0x01
#define WParam_WType_Ses    0x02
#define WParam_WType_Raw    0x03
#define WParam_TestWrite    0x10
#define WParam_LS_V         0x20
#define WParam_BUFF         0x40
#endif
*/

    union {
        UCHAR Flags;
        struct {
            UCHAR WriteType: 4;             // 1
            UCHAR TestWrite: 1;
            UCHAR LS_V: 1;
            UCHAR BUFF: 1;
            UCHAR Reserved1: 1;
        } Fields;
    } Byte2;

/*
#ifndef WParam_TrkMode_Mask
#define WParam_TrkMode_Mask             0x0f
#define WParam_TrkMode_None             0x00
#define WParam_TrkMode_Audio            0x00
#define WParam_TrkMode_Audio_PreEmph    0x01
#define WParam_TrkMode_Data             0x04
#define WParam_TrkMode_IncrData         0x05
#define WParam_TrkMode_QAudio_PreEmph   0x08
#define WParam_TrkMode_AllowCpy         0x02
#define WParam_Copy             0x10
#define WParam_FP               0x20
#define WParam_MultiSes_Mask    0xc0
#define WParam_Multises_None    0x00
#define WParam_Multises_Final   0x80
#define WParam_Multises_Multi   0xc0
#endif
*/

    union {
        UCHAR Flags;
        struct {
            UCHAR TrackMode: 4;             // 4
            UCHAR Copy     : 1;             // 0
            UCHAR FP       : 1;             // 0
            UCHAR Multisession: 2;          // 11
        } Fields;
    } Byte3;

/*
#ifndef WParam_BlkType_Mask
#define WParam_BlkType_Mask         0x0f
#define WParam_BlkType_Raw_2352     0x00
#define WParam_BlkType_RawPQ_2368   0x01
#define WParam_BlkType_RawPW_2448   0x02
#define WParam_BlkType_RawPW_R_2448 0x03
#define WParam_BlkType_M1_2048      0x08
#define WParam_BlkType_M2_2336      0x09
#define WParam_BlkType_M2XAF1_2048  0x0a
#define WParam_BlkType_M2XAF1SH_2056 0x0b
#define WParam_BlkType_M2XAF2_2324  0x0c
#define WParam_BlkType_M2XAFXSH_2332 0x0d
#endif
*/

    union {
        UCHAR Flags;
        struct {
            UCHAR DataBlockType: 4;         // 8
            UCHAR Reserved2: 4;
        } Fields;
    } Byte4;

    UCHAR LinkSize;
    UCHAR Reserved3;    

    union {
        UCHAR Flags;
        struct {
            UCHAR HostAppCode : 6;  // 0
            UCHAR Reserved4: 2;
        } Fields;
    } Byte7;

/*
#ifndef WParam_SesFmt_CdRom
#define WParam_SesFmt_CdRom     0x00
#define WParam_SesFmt_CdI       0x10
#define WParam_SesFmt_CdRomXa   0x20
#endif
*/

    UCHAR SesFmt;                   // 0
    UCHAR Reserved5;
    ULONG PacketSize;               // 0
    USHORT AudioPause;              // 150

    UCHAR Reserved6: 7;
    UCHAR MCVAL    : 1;

    UCHAR N[13];
    UCHAR Zero;
    UCHAR AFRAME;

    UCHAR Reserved7: 7;
    UCHAR TCVAL    : 1;

    UCHAR I[12];
    UCHAR Zero_2;
    UCHAR AFRAME_2;
    UCHAR Reserved8;

    struct {
        union {
            UCHAR MSF[3];
            struct _SubHdrParams1 {
                UCHAR FileNum;
                UCHAR ChannelNum;

/*
#define WParam_SubHdr_SubMode0          0x00
#define WParam_SubHdr_SubMode1          0x08
*/

                UCHAR SubMode;
            } Params1;
        } Params;

/*
#define WParam_SubHdr_Mode_Mask         0x03
#define WParam_SubHdr_Mode0             0x00
#define WParam_SubHdr_Mode1             0x01
#define WParam_SubHdr_Mode2             0x02
#define WParam_SubHdr_Format_Mask       0xe0
#define WParam_SubHdr_Format_UserData   0x00
#define WParam_SubHdr_Format_RunIn4     0x20
#define WParam_SubHdr_Format_RunIn3     0x40
#define WParam_SubHdr_Format_RunIn2     0x60
#define WParam_SubHdr_Format_RunIn1     0x80
#define WParam_SubHdr_Format_Link       0xa0
#define WParam_SubHdr_Format_RunOut2    0xc0
#define WParam_SubHdr_Format_RunOut1    0xe0
*/

        union {
            UCHAR Flags;
            struct {
                UCHAR Mode      : 2;
                UCHAR Reserved  : 3;
                UCHAR Format    : 3;
            } Fields;
        } Mode;
    } SubHeader;
} MODE_WRITE_PARAMS_PAGE_USER, *PMODE_WRITE_PARAMS_PAGE_USER;

typedef MODE_WRITE_PARAMS_PAGE_USER      GET_WRITE_MODE_USER_OUT;
typedef PMODE_WRITE_PARAMS_PAGE_USER     PGET_WRITE_MODE_USER_OUT;

typedef MODE_WRITE_PARAMS_PAGE_USER      SET_WRITE_MODE_USER_IN;
typedef PMODE_WRITE_PARAMS_PAGE_USER     PSET_WRITE_MODE_USER_IN;

//**********************************************************************************************

#ifndef WITHOUT_FORMATTER

typedef MODE_MRW_PAGE                    GET_MRW_MODE_USER_OUT;
typedef PMODE_MRW_PAGE                   PGET_MRW_MODE_USER_OUT;

typedef MODE_MRW_PAGE                    SET_MRW_MODE_USER_IN;
typedef PMODE_MRW_PAGE                   PSET_MRW_MODE_USER_IN;

#endif //WITHOUT_FORMATTER

//**********************************************************************************************

typedef struct _SET_READ_AHEAD_USER_IN {
    ULONG TriggerLBA;
    ULONG ReadAheadLBA;
} SET_READ_AHEAD_USER_IN, *PSET_READ_AHEAD_USER_IN;

//**********************************************************************************************

typedef struct _GET_CAPABILITIES_USER_OUT {
    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;

    UCHAR PageLength;

/*
#ifndef DevCap_read_cd_r
#define DevCap_read_cd_r          0x01 // reserved in 1.2 
#define DevCap_read_cd_rw         0x02 // reserved in 1.2 
#define DevCap_method2            0x04
#define DevCap_read_dvd_rom       0x08
#define DevCap_read_dvd_r         0x10
#define DevCap_read_dvd_ram       0x20
#endif //DevCap_cd_r_read
*/

    UCHAR ReadCap;            // DevCap_*_read
/*    UCHAR cd_r_read         : 1; // reserved in 1.2 
    UCHAR cd_rw_read        : 1; // reserved in 1.2 
    UCHAR method2           : 1;
    UCHAR dvd_rom           : 1;
    UCHAR dvd_r_read        : 1;
    UCHAR dvd_ram_read      : 1;
    UCHAR Reserved2            : 2;*/

/*
#ifndef DevCap_write_cd_r
#define DevCap_write_cd_r         0x01 // reserved in 1.2 
#define DevCap_write_cd_rw        0x02 // reserved in 1.2 
#define DevCap_test_write         0x04
#define DevCap_write_dvd_r        0x10
#define DevCap_write_dvd_ram      0x20
#endif //DevCap_cd_r_write
*/

    UCHAR WriteCap;            // DevCap_*_write
/*    UCHAR cd_r_write        : 1; // reserved in 1.2 
    UCHAR cd_rw_write        : 1; // reserved in 1.2 
    UCHAR dvd_ram_write     : 1;
    UCHAR dvd_r_write       : 1;
    UCHAR reserved3a        : 1;
    UCHAR test_write        : 1;
    UCHAR Reserved3         : 2;*/

/*
#ifndef DevCap_audio_play
#define DevCap_audio_play          0x01
#define DevCap_composite          0x02
#define DevCap_digport1           0x04
#define DevCap_digport2           0x08
#define DevCap_mode2_form1        0x10
#define DevCap_mode2_form2        0x20
#define DevCap_multisession       0x40
#endif //DevCap_audio_play
*/

    UCHAR Capabilities0;
/*    UCHAR audio_play        : 1;
    UCHAR composite         : 1;
    UCHAR digport1          : 1;
    UCHAR digport2          : 1;
    UCHAR mode2_form1       : 1;
    UCHAR mode2_form2       : 1;
    UCHAR multisession      : 1;
    UCHAR Reserved4         : 1;*/

/*
#ifndef DevCap_cdda
#define DevCap_cdda               0x01
#define DevCap_cdda_accurate      0x02
#define DevCap_rw_supported       0x04
#define DevCap_rw_corr            0x08
#define DevCap_c2_pointers        0x10
#define DevCap_isrc               0x20
#define DevCap_upc                0x40
#define DevCap_read_bar_code      0x80
#endif //DevCap_cdda
*/

    UCHAR Capabilities1;
/*    UCHAR cdda              : 1;
    UCHAR cdda_accurate     : 1;
    UCHAR rw_supported      : 1;
    UCHAR rw_corr           : 1;
    UCHAR c2_pointers       : 1;
    UCHAR isrc              : 1;
    UCHAR upc               : 1;
    UCHAR Reserved5         : 1;*/

/*
#ifndef DevCap_lock
#define DevCap_lock               0x01
#define DevCap_lock_state         0x02
#define DevCap_prevent_jumper     0x04
#define DevCap_eject              0x08
#define DevCap_mechtype_mask                 0xE0
#define DevCap_mechtype_caddy                 0x00
#define DevCap_mechtype_tray                (0x01<<5)
#define DevCap_mechtype_popup                (0x02<<5)
#define DevCap_mechtype_individual_changer    (0x04<<5)
#define DevCap_mechtype_cartridge_changer    (0x05<<5)
#endif //DevCap_lock
*/

    UCHAR Capabilities2;
/*    UCHAR lock              : 1;
    UCHAR lock_state        : 1;
    UCHAR prevent_jumper    : 1;
    UCHAR eject             : 1;
    UCHAR Reserved6         : 1;
    UCHAR mechtype            : 3;*/

/*
#ifndef DevCap_separate_volume
#define DevCap_separate_volume    0x01
#define DevCap_separate_mute      0x02
#define DevCap_disc_present       0x04          // reserved in 1.2 
#define DevCap_sw_slot_select     0x08          // reserved in 1.2 
#define DevCap_change_side_cap    0x10 
#define DevCap_rw_leadin_read     0x20
#endif //DevCap_separate_volume
*/

    UCHAR Capabilities3;
/*    UCHAR separate_volume   : 1;
    UCHAR separate_mute     : 1;
    UCHAR disc_present      : 1;  // reserved in 1.2 
    UCHAR sss               : 1;  // reserved in 1.2 
    UCHAR Reserved7         : 4;*/

    USHORT MaximumSpeedSupported;
    USHORT NumberVolumeLevels;
    USHORT BufferSize;
    USHORT CurrentSpeed;

    UCHAR Reserved8;

    UCHAR SpecialParameters0;
/*  UCHAR Reserved9        : 1;
    UCHAR BCK           : 1;
    UCHAR RCK           : 1;
    UCHAR LSBF          : 1;
    UCHAR Length        : 2;
    UCHAR Reserved10    : 2;*/

    USHORT MaximumWriteSpeedSupported;
    USHORT CurrentWriteSpeed;
    USHORT CopyManagementRevision;
    UCHAR Reserved11[2];

} GET_CAPABILITIES_USER_OUT, *PGET_CAPABILITIES_USER_OUT;

typedef struct _GET_CAPABILITIES_3_USER_OUT {

    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;

    UCHAR PageLength;
    UCHAR ReadCap;             // DevCap_*_read
    UCHAR WriteCap;            // DevCap_*_write
    UCHAR Capabilities0;
    UCHAR Capabilities1;
    UCHAR Capabilities2;
    UCHAR Capabilities3;
    USHORT MaximumSpeedSupported;
    USHORT NumberVolumeLevels;
    USHORT BufferSize;
    USHORT CurrentSpeed;

    UCHAR Reserved8;
    UCHAR SpecialParameters0;

    USHORT MaximumWriteSpeedSupported;
    USHORT CurrentWriteSpeed;
    USHORT CopyManagementRevision;
    UCHAR Reserved11[2];

// MMC 3

    UCHAR Reserved12;

    UCHAR SpecialParameters1;
/*  UCHAR RCS           : 2; // rotation control selected
    UCHAR Reserved13    : 6; */

    USHORT CurrentWriteSpeed3;
    USHORT LunWPerfDescriptorCount;

//    LUN_WRITE_PERF_DESC_USER WritePerfDescs[0];

} GET_CAPABILITIES_3_USER_OUT, *PGET_CAPABILITIES_3_USER_OUT;

typedef struct _LUN_WRITE_PERF_DESC_USER {
    UCHAR Reserved;

#define LunWPerf_RotCtrl_Mask   0x07
#define LunWPerf_RotCtrl_CLV    0x00
#define LunWPerf_RotCtrl_CAV    0x01

    UCHAR RotationControl;
    USHORT WriteSpeedSupported; // kbps

} LUN_WRITE_PERF_DESC_USER, *PLUN_WRITE_PERF_DESC_USER;

//**********************************************************************************************

typedef struct _SEND_OPC_INFO_USER_IN {
    USHORT Speed;
    UCHAR OpcValue[6];
} SEND_OPC_INFO_USER_IN, *PSEND_OPC_INFO_USER_IN;

typedef struct _SEND_OPC_INFO_HEADER_USER_IN {
    BOOLEAN DoOpc;
    USHORT OpcBlocksNumber;
} SEND_OPC_INFO_HEADER_USER_IN, *PSEND_OPC_INFO_HEADER_USER_IN;

//**********************************************************************************************

typedef struct _LL_READ_USER_IN {

#define ReadCd_BlkType_Mask 0x1c
#define ReadCd_BlkType_Any  (0x00<<2)
#define ReadCd_BlkType_CDDA (0x01<<2)
#define ReadCd_BlkType_M1   (0x02<<2)
#define ReadCd_BlkType_M2FX (0x03<<2)
#define ReadCd_BlkType_M2F1 (0x04<<2)
#define ReadCd_BlkType_M2F2 (0x05<<2)

    UCHAR ExpectedBlkType;

    ULONG LBA;          // negative value (-1) indicates tha (H)MSF must be used
    ULONG NumOfBlocks;

#define ReadCd_Error_Mask       0x0006
#define ReadCd_Error_None       0x0000
#define ReadCd_Error_C2         0x0002
#define ReadCd_Error_C2ex       0x0004
#define ReadCd_Include_EDC      0x0008
#define ReadCd_Include_UsrData  0x0010
#define ReadCd_Header_Mask      0x0060
#define ReadCd_Header_None      0x0000
#define ReadCd_Header_Hdr       0x0020
#define ReadCd_Header_SubHdr    0x0040
#define ReadCd_Header_AllHdr    0x0060
#define ReadCd_Include_SyncData 0x0080
#define ReadCd_SubChan_Mask     0x0700
#define ReadCd_SubChan_None     0x0000
#define ReadCd_SubChan_Raw      0x0100
#define ReadCd_SubChan_Q        0x0200
#define ReadCd_SubChan_PW       0x0400
#define ReadCd_SubChan_All      ReadCd_SubChan_Mask

    union {
        USHORT Flags;
        struct {
            UCHAR Reserved2 : 1;
            UCHAR ErrorFlags : 2;
            UCHAR IncludeEDC : 1;
            UCHAR IncludeUserData : 1;
            UCHAR HeaderCode : 2;
            UCHAR IncludeSyncData : 1;

            UCHAR SubChannelSelection : 3;
            UCHAR Reserved3 : 5;
        } Fields;
    } Flags;

    BOOLEAN UseMFS;
    CHAR Starting_MSF[3];
    CHAR Ending_MSF[3];

} LL_READ_USER_IN, *PLL_READ_USER_IN;

//**********************************************************************************************

typedef struct _GET_LAST_ERROR_USER_OUT {

    UCHAR ErrorCode;
    UCHAR SenseKey;
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
    UCHAR SrbStatus;
    ULONG LastError;
    BOOLEAN RandomAccessMode;
    LARGE_INTEGER MediaChangeTime;
    ULONG MediaChangeCount;

} GET_LAST_ERROR_USER_OUT, *PGET_LAST_ERROR_USER_OUT;

//**********************************************************************************************

typedef enum _TRACK_MODE_TYPE {
    YellowMode2,
    XAForm2,
    CDDA
} TRACK_MODE_TYPE, *PTRACK_MODE_TYPE;

typedef struct _RAW_READ_USER_IN {
    LARGE_INTEGER DiskOffset;
    ULONG    SectorCount;
    TRACK_MODE_TYPE TrackMode;
} RAW_READ_USER_IN, *PRAW_READ_USER_IN;

//**********************************************************************************************

typedef struct _PLAY_AUDIO_MSF_USER_IN {
    UCHAR StartingMSF[3];
    UCHAR EndingMSF[3];
} PLAY_AUDIO_MSF_USER_IN, *PPLAY_AUDIO_MSF_USER_IN;

//**********************************************************************************************

#define AudioStatus_NotSupported    0x00
#define AudioStatus_InProgress      0x11
#define AudioStatus_Paused          0x12
#define AudioStatus_PlayComplete    0x13
#define AudioStatus_PlayError       0x14
#define AudioStatus_NoStatus        0x15

typedef struct _SUB_Q_HEADER {
    UCHAR Reserved;
    UCHAR AudioStatus;
    UCHAR DataLength[2];
} SUB_Q_HEADER, *PSUB_Q_HEADER;

typedef struct _SUB_Q_CURRENT_POSITION {
    SUB_Q_HEADER Header;
    UCHAR FormatCode;
    UCHAR Control : 4;
    UCHAR ADR : 4;
    UCHAR TrackNumber;
    UCHAR IndexNumber;
    UCHAR AbsoluteAddress[4];
    UCHAR TrackRelativeAddress[4];
} SUB_Q_CURRENT_POSITION, *PSUB_Q_CURRENT_POSITION;

typedef struct _SUB_Q_MEDIA_CATALOG_NUMBER {
    SUB_Q_HEADER Header;
    UCHAR FormatCode;
    UCHAR Reserved[3];
    UCHAR Reserved1 : 7;
    UCHAR Mcval : 1;
    UCHAR MediaCatalog[15];
} SUB_Q_MEDIA_CATALOG_NUMBER, *PSUB_Q_MEDIA_CATALOG_NUMBER;

typedef struct _SUB_Q_TRACK_ISRC {
    SUB_Q_HEADER Header;
    UCHAR FormatCode;
    UCHAR Reserved0;
    UCHAR Track;
    UCHAR Reserved1;
    UCHAR Reserved2 : 7;
    UCHAR Tcval : 1;
    UCHAR TrackIsrc[15];
} SUB_Q_TRACK_ISRC, *PSUB_Q_TRACK_ISRC;

typedef union _SUB_Q_CHANNEL_DATA_USER_OUT {
    SUB_Q_CURRENT_POSITION CurrentPosition;
    SUB_Q_MEDIA_CATALOG_NUMBER MediaCatalog;
    SUB_Q_TRACK_ISRC TrackIsrc;
} SUB_Q_CHANNEL_DATA_USER_OUT, *PSUB_Q_CHANNEL_DATA_USER_OUT;

#define IOCTL_CDROM_SUB_Q_CHANNEL    0x00
#define IOCTL_CDROM_CURRENT_POSITION 0x01
#define IOCTL_CDROM_MEDIA_CATALOG    0x02
#define IOCTL_CDROM_TRACK_ISRC       0x03

typedef struct _SUB_Q_CHANNEL_DATA_USER_IN {
    UCHAR Format;
    UCHAR Track;
} SUB_Q_CHANNEL_DATA_USER_IN, *PSUB_Q_CHANNEL_DATA_USER_IN;

//**********************************************************************************************

typedef struct _SEEK_AUDIO_MSF_USER_IN {
    UCHAR MSF[3];
} SEEK_AUDIO_MSF_USER_IN, *PSEEK_AUDIO_MSF_USER_IN;

//**********************************************************************************************

typedef struct _AUDIO_CONTROL_USER_OUT {
    UCHAR LbaFormat;
    USHORT LogicalBlocksPerSecond;
} AUDIO_CONTROL_USER_OUT, *PAUDIO_CONTROL_USER_OUT;

//**********************************************************************************************

typedef READ_TOC_TOC    READ_TOC_USER_OUT;
typedef PREAD_TOC_TOC   PREAD_TOC_USER_OUT;

typedef READ_TOC_SES    GET_LAST_SESSION_USER_OUT;
typedef PREAD_TOC_SES   PGET_LAST_SESSION_USER_OUT;

typedef READ_TOC_FULL_TOC   READ_FULL_TOC_USER_OUT;
typedef PREAD_TOC_FULL_TOC  PREAD_FULL_TOC_USER_OUT;

typedef READ_TOC_FULL_TOC   READ_PMA_USER_OUT;
typedef PREAD_TOC_FULL_TOC  PREAD_PMA_USER_OUT;

typedef READ_TOC_ATIP   READ_ATIP_USER_OUT;
typedef PREAD_TOC_ATIP  PREAD_ATIP_USER_OUT;

typedef READ_TOC_CD_TEXT   READ_CD_TEXT_USER_OUT;
typedef PREAD_TOC_CD_TEXT  PREAD_CD_TEXT_USER_OUT;

//**********************************************************************************************

typedef struct _VOLUME_CONTROL {
    UCHAR PortVolume[4];
} VOLUME_CONTROL, *PVOLUME_CONTROL;

typedef VOLUME_CONTROL  VOLUME_CONTROL_USER_IN;
typedef PVOLUME_CONTROL PVOLUME_CONTROL_USER_IN;

typedef VOLUME_CONTROL  VOLUME_CONTROL_USER_OUT;
typedef PVOLUME_CONTROL PVOLUME_CONTROL_USER_OUT;

//**********************************************************************************************

typedef struct _INIT_DEINIT_USER_IN {
    BOOLEAN PassThrough;
    BOOLEAN Reserved;                   // For separate device (de)initialization
} INIT_DEINIT_USER_IN, *PINIT_DEINIT_USER_IN;

typedef INIT_DEINIT_USER_IN     INIT_DEINIT_USER_OUT;
typedef PINIT_DEINIT_USER_IN    PINIT_DEINIT_USER_OUT;

//**********************************************************************************************

typedef struct _READ_SESSION_INFO_USER_IN {
    BOOLEAN UseLBA;
    UCHAR Session;
} READ_SESSION_INFO_USER_IN, *PREAD_SESSION_INFO_USER_IN;

typedef READ_TOC_SES    READ_SESSION_INFO_USER_OUT;
typedef PREAD_TOC_SES   PREAD_SESSION_INFO_USER_OUT;

//**********************************************************************************************

typedef struct _READ_TOC_EX_USER_IN {
    BOOLEAN UseLBA;
    UCHAR Track;
} READ_TOC_EX_USER_IN, *PREAD_TOC_EX_USER_IN;

typedef READ_TOC_SES    READ_TOC_EX_USER_OUT;
typedef PREAD_TOC_SES   PREAD_TOC_EX_USER_OUT;

//**********************************************************************************************

#define DefSession_LastAvailable    0xff

typedef struct _SET_DEFAULT_SESSION_USER_IN {
    UCHAR LastSes;
} SET_DEFAULT_SESSION_USER_IN, *PSET_DEFAULT_SESSION_USER_IN;

//**********************************************************************************************

typedef struct _NOTIFY_MEDIA_CHANGE_USER_IN {
    BOOLEAN Autorun;
} NOTIFY_MEDIA_CHANGE_USER_IN, *PNOTIFY_MEDIA_CHANGE_USER_IN;

//**********************************************************************************************

typedef DISK_GEOMETRY  GET_DRIVE_GEOMETRY_USER_OUT;
typedef PDISK_GEOMETRY PGET_DRIVE_GEOMETRY_USER_OUT;

//**********************************************************************************************

typedef struct _GET_DEVICE_INFO_OLD_USER_OUT {
    UCHAR WModes [4][16];
    UCHAR VendorId[25];
    UCHAR SimulatedWModes [4][16];
    ULONG DeviceNumber;
    ULONG Features;
    INQUIRYDATA InquiryData;
    ULONG Features2[4];                        // form GET_CONFIG   0 - 128
} GET_DEVICE_INFO_OLD_USER_OUT, *PGET_DEVICE_INFO_OLD_USER_OUT;

typedef struct _GET_DEVICE_INFO_USER_OUT {
    ULONG Tag;
    ULONG Length;
    UCHAR WModes [4][16];
    UCHAR VendorId[25];
    UCHAR SimulatedWModes [4][16];
    ULONG DeviceNumber;
    ULONG Features;
    INQUIRYDATA InquiryData;
    ULONG Features2[4];                        // from GET_CONFIG
    ULONG Features2ex[64-4];                   // from GET_CONFIG (reserved for higher values)
    ULONG WriteCaps;                           // CDRW_DEV_CAPABILITY_xxx
    ULONG ReadCaps;                            // CDRW_DEV_CAPABILITY_xxx
} GET_DEVICE_INFO_USER_OUT, *PGET_DEVICE_INFO_USER_OUT;

#define CDRW_DEV_CAPABILITY_TAG     0xCA10AB11

#define WMODE_SUPPORTED             0x01
#define WMODE_SUPPORTED_FP          0x02
#define WMODE_SUPPORTED_VP          0x04
#define WMODE_NOT_SUPPORTED         0xff
#define WMODE_NOT_TESTED            0x00

#define CDRW_FEATURE_OPC            0x00000001 // use OPC regardless of OPCn in DISK_INFO
#define CDRW_FEATURE_EVENT          0x00000002
#define CDRW_FEATURE_GET_CFG        0x00000004
#define CDRW_FEATURE_NO_LOCK_REP    0x00000008 // device doesn't report tray lock state
#define CDRW_FEATURE_SYNC_ON_WRITE  0x00000010 // device preferes Sync Cache after each Write
#define CDRW_FEATURE_BAD_RW_SEEK    0x00000020 // seek error occures with status Illegal Sector Mode For This Track
                                               //   on old CdRoms when they attempt to read outer sectors on FP formatted
                                               //   disk. Workaround: perform sequence of seeks from lower address
                                               //   to required with step about 0x800 blocks.
#define CDRW_FEATURE_FP_ADDRESSING_PROBLEM  0x00000040
#define CDRW_FEATURE_MRW_ADDRESSING_PROBLEM 0x00000080
#define CDRW_FEATURE_FORCE_SYNC_ON_WRITE    0x00000100 // device requires Sync Cache after each Write
#define CDRW_FEATURE_BAD_DVD_LAST_LBA       0x00000200 // device cannot determile LastLba on not closed DVD disks
#define CDRW_FEATURE_FULL_BLANK_ON_FORMAT   0x00000400 // device cannot format disk until it is full-blanked
#define CDRW_FEATURE_STREAMING              0x00000800 // device supports streaming read/write
#define CDRW_FEATURE_FORCE_SYNC_BEFORE_READ 0x00001000 // device requires Sync Cache on Write -> Read state transition
#define CDRW_FEATURE_CHANGER        0x80000000

#define DEV_CAP_GET_PROFILE(arr, pf) \
    (((pf) > PFNUM_Max) ? 0 : (((arr)[(pf)/32] >> (pf)%32) & 1))

#define CDRW_DEV_CAPABILITY_CDROM    ((ULONG)1 << CdMediaClass_CDROM  )
#define CDRW_DEV_CAPABILITY_CDR      ((ULONG)1 << CdMediaClass_CDR    )
#define CDRW_DEV_CAPABILITY_CDRW     ((ULONG)1 << CdMediaClass_CDRW   )
#define CDRW_DEV_CAPABILITY_DVDROM   ((ULONG)1 << CdMediaClass_DVDROM )
#define CDRW_DEV_CAPABILITY_DVDRAM   ((ULONG)1 << CdMediaClass_DVDRAM )
#define CDRW_DEV_CAPABILITY_DVDR     ((ULONG)1 << CdMediaClass_DVDR   )
#define CDRW_DEV_CAPABILITY_DVDRW    ((ULONG)1 << CdMediaClass_DVDRW  )
#define CDRW_DEV_CAPABILITY_DVDpR    ((ULONG)1 << CdMediaClass_DVDpR  )
#define CDRW_DEV_CAPABILITY_DVDpRW   ((ULONG)1 << CdMediaClass_DVDpRW )
#define CDRW_DEV_CAPABILITY_DDCDROM  ((ULONG)1 << CdMediaClass_DDCDROM)
#define CDRW_DEV_CAPABILITY_DDCDR    ((ULONG)1 << CdMediaClass_DDCDR  )
#define CDRW_DEV_CAPABILITY_DDCDRW   ((ULONG)1 << CdMediaClass_DDCDRW )

//**********************************************************************************************

typedef ULONG CHECK_VERIFY_USER_OUT, *PCHECK_VERIFY_USER_OUT;

//**********************************************************************************************

/*
#ifndef EventStat_Class_OpChange
#define EventStat_Class_OpChange    0x01
#define EventStat_Class_PM          0x02
#define EventStat_Class_Media       0x04
#define EventStat_Class_DevBusy     0x06
#endif // end EventStat_Class_OpChange
*/

typedef struct _GET_EVENT_USER_IN {
    UCHAR EventClass;
    BOOLEAN Immed;
} GET_EVENT_USER_IN, *PGET_EVENT_USER_IN;

typedef union _GET_EVENT_USER_OUT {
    EVENT_STAT_OPERATIONAL_BLOCK   Operational;
    EVENT_STAT_PM_BLOCK            PowerManagement;
    EVENT_STAT_EXT_REQ_BLOCK       ExternalReq;
    EVENT_STAT_MEDIA_BLOCK         MediaChange;
    EVENT_STAT_DEV_BUSY_BLOCK      DeviceBusy;
} GET_EVENT_USER_OUT, *PGET_EVENT_USER_OUT;

//**********************************************************************************************


//**********************************************************************************************

typedef enum DVD_STRUCTURE_FORMAT {
    DvdPhysicalDescriptor,
    DvdCopyrightDescriptor,
    DvdDiskKeyDescriptor,
    DvdBCADescriptor,
    DvdManufacturerDescriptor,
    DvdMaxDescriptor
} DVD_STRUCTURE_FORMAT, *PDVD_STRUCTURE_FORMAT;

typedef ULONG DVD_SESSION_ID, *PDVD_SESSION_ID;

typedef struct _DVD_READ_STRUCTURE_USER_IN {
    LARGE_INTEGER BlockByteOffset;
    DVD_STRUCTURE_FORMAT Format;
    DVD_SESSION_ID SessionId;
    UCHAR LayerNumber;
} DVD_READ_STRUCTURE_USER_IN, *PDVD_READ_STRUCTURE_USER_IN;

typedef struct _DVD_READ_STRUCTURE_USER_OUT {
    USHORT Length;
    UCHAR Reserved[2];
//    UCHAR Data[0];
} DVD_READ_STRUCTURE_USER_OUT, *PDVD_READ_STRUCTURE_USER_OUT;

//**********************************************************************************************

typedef struct _DVD_START_SESSION_USER_OUT {
    DVD_SESSION_ID SessionId;
} DVD_START_SESSION_USER_OUT, *PDVD_START_SESSION_USER_OUT;

//**********************************************************************************************

typedef enum {
    DvdChallengeKey = 0x01,
    DvdBusKey1,
    DvdBusKey2,
    DvdTitleKey,
    DvdAsf,
    DvdSetRpcKey = 0x6,
    DvdGetRpcKey = 0x8,
    DvdDiskKey = 0x80,
    DvdInvalidateAGID = 0x3f
} DVD_KEY_TYPE;

typedef struct _DVD_READ_KEY_USER_IN {
    ULONG KeyLength;
    DVD_SESSION_ID SessionId;
    DVD_KEY_TYPE KeyType;
    ULONG KeyFlags;
    union {
        HANDLE FileHandle;
        LARGE_INTEGER TitleOffset;
    } Parameters;
//    UCHAR KeyData[0];
} DVD_READ_KEY_USER_IN, *PDVD_READ_KEY_USER_IN;

typedef DVD_READ_KEY_USER_IN  DVD_COPY_PROTECT_KEY;
typedef PDVD_READ_KEY_USER_IN PDVD_COPY_PROTECT_KEY;

typedef DVD_READ_KEY_USER_IN  DVD_READ_KEY_USER_OUT;
typedef PDVD_READ_KEY_USER_IN PDVD_READ_KEY_USER_OUT;

//**********************************************************************************************

typedef DVD_START_SESSION_USER_OUT  DVD_END_SESSION_USER_IN;
typedef PDVD_START_SESSION_USER_OUT PDVD_END_SESSION_USER_IN;

//**********************************************************************************************

typedef DVD_READ_KEY_USER_IN  DVD_SEND_KEY_USER_IN;
typedef PDVD_READ_KEY_USER_IN PDVD_SEND_KEY_USER_IN;

typedef struct _DVD_SET_RPC_KEY {
    UCHAR PreferredDriveRegionCode;
    UCHAR Reserved[3];
} DVD_SET_RPC_KEY, * PDVD_SET_RPC_KEY;

//**********************************************************************************************

// Predefined (Mt. Fuji) key sizes
// Add sizeof(DVD_COPY_PROTECT_KEY) to get allocation size for
// the full key structure

#define DVD_CHALLENGE_KEY_LENGTH    (12 + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_BUS_KEY_LENGTH          (8 + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_TITLE_KEY_LENGTH        (8 + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_DISK_KEY_LENGTH         (2048 + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_RPC_KEY_LENGTH          (sizeof(DVD_RPC_KEY) + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_SET_RPC_KEY_LENGTH      (sizeof(DVD_SET_RPC_KEY) + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_ASF_LENGTH              (sizeof(DVD_ASF) + sizeof(DVD_COPY_PROTECT_KEY))

//**********************************************************************************************

typedef struct _DISK_VERIFY_USER_IN {
    LONGLONG StartingOffset;
    ULONG Length;
} DISK_VERIFY_USER_IN, *PDISK_VERIFY_USER_IN;

//**********************************************************************************************

typedef struct _CDROM_DISK_DATA_USER_OUT {

    ULONG DiskData;

} CDROM_DISK_DATA_USER_OUT, *PCDROM_DISK_DATA_USER_OUT;

#define CDROM_DISK_AUDIO_TRACK      (0x00000001)
#define CDROM_DISK_DATA_TRACK       (0x00000002)

//**********************************************************************************************

typedef struct _CDRW_RESET_DRIVER_USER_IN {
    BOOLEAN UnlockTray;
    BOOLEAN Reserved[3];
    ULONG   MagicWord;
} CDRW_RESET_DRIVER_USER_IN, *PCDRW_RESET_DRIVER_USER_IN;

//**********************************************************************************************

typedef struct _MediaTrackMap {
    ULONG FirstLba;
    ULONG LastLba;
    ULONG NWA;
    ULONG PacketSize;
    ULONG Session;
    UCHAR  TrackParam;
    UCHAR  DataParam;
    UCHAR  NWA_V;

    UCHAR  Flags;
#define     TrackMap_AllowCopyBit_variated     0x01
#define     TrackMap_CopyBit_variated          0x02
#define     TrackMap_Try_variation             0x04
#define     TrackMap_Use_variation             0x08
#define     TrackMap_FixFPAddressing           0x10
#define     TrackMap_FixMRWAddressing          0x20

    // are used only if FixFPAddressing is enabled
    ULONG TrackFPOffset;
    ULONG PacketFPOffset;

} MediaTrackMap, *PMediaTrackMap;

typedef struct _GET_DISK_LAYOUT_USER_OUT {
    ULONG           Tag;
    ULONG           Length;
    ULONG           DiskLayoutFlags;
    // Number of last session
    ULONG           LastSession;
    ULONG           FirstTrackNum;
    ULONG           LastTrackNum;
    // First & Last LBA of the last session
    ULONG           FirstLBA;
    ULONG           LastLBA;
    // Last writable LBA
    ULONG           LastPossibleLBA;
    // First writable LBA
    ULONG           NWA;
    // sector type map
    struct _MediaTrackMap*  TrackMap;
    // 
    ULONG           BlockSize;
    ULONG           WriteBlockSize;
    // disk state
    BOOLEAN         FP_disc;
    UCHAR           OPCNum;
    UCHAR           MediaClassEx;
    UCHAR           DiscStat;
    ULONG           PhSerialNumber;
    UCHAR           PhErasable;
    UCHAR           PhDiskType;
    UCHAR           MRWStatus;
} GET_DISK_LAYOUT_USER_OUT, *PGET_DISK_LAYOUT_USER_OUT;

#define         DiskLayout_FLAGS_TRACKMAP              (0x00002000)
#define         DiskLayout_FLAGS_RAW_DISK              (0x00040000)

//**********************************************************************************************

// Error codes returned by IOCTL_CDRW_GEL_LAST_ERROR

//#ifndef CDRW_ERR_NO_ERROR

#define CDRW_ERR_NO_ERROR                       0x0000
#define CDRW_ERR_WRITE_IN_PROGRESS_BUSY         0x0001
#define CDRW_ERR_FORMAT_IN_PROGRESS_BUSY        0x0002
#define CDRW_ERR_CLOSE_IN_PROGRESS_BUSY         0x0003
#define CDRW_ERR_BAD_ADDR_ALIGNMENT             0x0004
#define CDRW_ERR_BAD_SIZE_ALIGNMENT             0x0005
#define CDRW_ERR_STREAM_LOSS                    0x0006
#define CDRW_ERR_TEST_WRITE_UNSUPPORTED         0x0007
#define CDRW_ERR_UNHANDLED_WRITE_TYPE           0x0008
#define CDRW_ERR_CANT_ALLOC_TMP_BUFFER          0x0009
#define CDRW_ERR_BUFFER_IS_FULL                 0x000a
#define CDRW_ERR_VERIFY_REQUIRED                0x000b
#define CDRW_ERR_PLAY_IN_PROGRESS_BUSY          0x000c
#define CDRW_ERR_TOO_LONG_BLOCK_TO_TRANSFER     0x000d
#define CDRW_ERR_INWALID_WRITE_PARAMETERS       0x000e  // use SET_WRITE_PARAMS properly
#define CDRW_ERR_INVALID_IO_BUFFER_ADDRESS      0x000f
#define CDRW_ERR_INVALID_INPUT_BUFFER_SIZE      0x0010
#define CDRW_ERR_INVALID_OUTPUT_BUFFER_SIZE     0x0011
#define CDRW_ERR_UNRECOGNIZED_MEDIA             0x0012
#define CDRW_ERR_MEDIA_WRITE_PROTECTED          0x0013
#define CDRW_ERR_NO_MEDIA                       0x0014
#define CDRW_ERR_TRAY_OPEN                      0x0015
#define CDRW_ERR_MEDIA_NOT_APPENDABLE           0x0016
#define CDRW_ERR_INVALID_LBA                    0x0017
#define CDRW_ERR_INVALID_FIXED_PACKET_SIZE      0x0018
#define CDRW_ERR_INVALID_WRITE_TYPE_FOR_MEDIA   0x0019
#define CDRW_ERR_CUE_SHEET_REQUIRED             0x001a  // you sould send cue sheet before SAO
#define CDRW_ERR_CANT_DEINIT_IN_CLASS_MODE      0x001b  // there is no underlayered driver
#define CDRW_ERR_INVALID_FORMAT_UNIT_SETTINGS   0x001c  // use SET_WRITE_PARAMS properly before
                                                        // calling FormatUnit
#define CDRW_ERR_UNHANDLED_FORMAT_UNIT_MODE     0x001d  // this mode is not supported by
                                                        // Workaround module
#define CDRW_ERR_CANT_READ_BUFFER_CAPACITY      0x001e
#define CDRW_ERR_DEVICE_WRITE_ERROR             0x001f
#define CDRW_ERR_UNHANDLED_IOCTL                0x0020
#define CDRW_ERR_UNHANDLED_FORMAT_WORKAROUND_MODE   0x0021 // check your Registry settings
#define CDRW_ERR_DOOR_LOCKED_BUSY               0x0022
#define CDRW_ERR_MAGIC_WORD_REQUIRED            0x0023
#define CDRW_ERR_INVALID_SECTOR_MODE            0x0024
#define CDRW_ERR_DVD_LICENSE_VIOLATION          0x0025
#define CDRW_ERR_INVALID_DVD_KEY_TYPE           0x0026
#define CDRW_ERR_INVALID_DVD_REGION_CODE        0x0027
#define CDRW_ERR_PAGE_IS_NOT_SUPPORTED          0x0028
#define CDRW_ERR_STD_FORMAT_REQUIRED            0x0029
//#define CDRW_ERR_                    0x00
//#define CDRW_ERR_                    0x00
//#define CDRW_ERR_                    0x00
//#define CDRW_ERR_                    0x00

//#endif

// Registry keys
#define REG_TIMEOUT_NAME_USER    ("TimeOutValue")
#define REG_AUTORUN_NAME_USER    ("Autorun")
#define REG_LOADMODE_NAME_USER   ("LoadMode")

#define LOADMODE_CDRW_ONLY      0
#define LOADMODE_ALWAYS         1
#define LOADMODE_NEVER          2

#define REG_PACKETSIZE_NAME_USER    ("PacketSize")      // Initial packet size (FP)

#define PACKETSIZE_STREAM       0
#define PACKETSIZE_UDF          32

#ifndef WITHOUT_FORMATTER
#define REG_FORMATUNIT_NAME_USER    ("FormatUnitMode")  // FORMAT_UNIT workaround mode

#define FORMATUNIT_FP           0                       // simulation via FP
#define FORMATUNIT_STD          1
#endif //WITHOUT_FORMATTER

#define REG_R_SPLIT_SIZE_NAME_USER  ("ReadSplitSize")       // Read request spliting limit
#define REG_W_SPLIT_SIZE_NAME_USER  ("WriteSplitSize")      // Write request spliting limit

#define REG_CDR_SIMULATION_NAME_USER    ("CdRSimulationMode")  // Influence on READ_DISC_INFO
                                                        // capability check on startup
#define CDR_SIMULATION_CDROM    0
#define CDR_SIMULATION_ALWAYS   1
#define CDR_SIMULATION_NEVER    2

#define REG_SPEEDMODE_NAME_USER     ("SpeedDetectionMode")

#define SPEEDMODE_ASSUME_OK     0
#define SPEEDMODE_REREAD        1

//#define REG_MAX_WRITE_SPEED_R_NAME_USER   ("MaxWriteSpeedCDR")
//#define REG_MAX_WRITE_SPEED_RW_NAME_USER  ("MaxWriteSpeedCDRW")

//#define REG_SIMULATION_TABLE_NAME_USER ("WModeSimulationTable") // via Raw

#define REG_WMODE_SIMULATION_NAME_USER ("WModeSimulation") // via Raw
#define WMODE_SIMULATION_ON     1
#define WMODE_SIMULATION_OFF    0
#define WMODE_ASSUME_OK         2

#define REG_SYNC_PACKETS_NAME_USER  ("SyncPacketsMode")
#define SYNC_PACKETS_ALWAYS     0
#define SYNC_PACKETS_RESET_DRV  1
#define SYNC_PACKETS_NEVER      2
#define SYNC_PACKETS_FP         3
#define SYNC_PACKETS_DISABLED   4
#define SYNC_PACKETS_VP_ONLY    5
#define SYNC_PACKETS_BY_W_THROUGH 6

#define REG_ASSURE_READY_NAME_USER  ("AssureReadiness")
#define ASSURE_READY_TEST_UNIT  0
#define ASSURE_READY_DELAY_100  1
#define ASSURE_READY_NONE       2

#define REG_WAIT_PACKETS_NAME_USER  ("WaitPackets")
#define WAIT_PACKETS_ALWAYS     0
#define WAIT_PACKETS_STREAM     1

#define REG_BAD_RW_SEEK_NAME_USER   ("BadRWSeek")

#define REG_ALLOW_PACKET_ON_CDR_NAME_USER  ("AllowPacketOnCdR")
#define ALLOW_PACKET_ON_CDR_OFF 0
#define ALLOW_PACKET_ON_CDR_ON  1

#define REG_MAX_READY_RETRY_NAME_USER      ("MaxReadyRetry")

#define REG_BAD_DVD_READ_TRACK_INFO_NAME_USER  ("DvdBadReadTrackInfo")
#define REG_BAD_DVD_READ_DISC_INFO_NAME_USER   ("DvdBadReadDiscInfo")
#define REG_BAD_DVD_READ_CAPACITY_NAME_USER    ("DvdBadReadCapacity")
#define REG_BAD_DVD_LAST_LBA_NAME_USER         ("DvdBadLastLba")
#define REG_BAD_DVD_LAST_LBA_NAME_USER         ("DvdBadLastLba")

#define REG_FULL_BLANK_ON_FORMAT_NAME_USER     ("FullEraseBeforeFormat")

#define DEFAULT_LAST_LBA_FP_CD  276159
#define DEFAULT_LAST_LBA_DVD    0x23053f
#define DEFAULT_LAST_LBA_BD     (25*1000*1000/2-1)


#pragma pack(pop)

#endif //__CDRW_PUBLIC_H__
