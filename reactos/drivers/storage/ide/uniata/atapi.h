/*++

Copyright (c) 2002-2005 Alexandr A. Telyatnikov (Alter)

Module Name:
    atapi.h

Abstract:
    This file contains IDE, ATA, ATAPI and SCSI Miniport definitions
    and function prototypes.

Author:
    Alexander A. Telyatnikov (Alter)

Environment:
    kernel mode only

Notes:

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Revision History:

    Some definitions were taken from standard ATAPI.SYS sources from NT4 DDK by
         Mike Glass (MGlass)

    Some definitions were taken from FreeBSD 4.3-4.6 ATA driver by
         Søren Schmidt, Copyright (c) 1998,1999,2000,2001

    Code was changed/updated by
         Alter, Copyright (c) 2002-2004


--*/
#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#ifndef USER_MODE
#include "config.h"
#endif //USER_MODE

#include "scsi.h"
#include "stdio.h"
#include "string.h"

#ifdef _DEBUG


#ifndef _DBGNT_

#ifdef KdPrint
#undef KdPrint
#endif

#ifdef USE_DBGPRINT_LOGGER
#include "inc\PostDbgMesg.h"
#define DbgPrint             DbgDump_Printf
#define Connect_DbgPrint()   {DbgDump_SetAutoReconnect(TRUE); DbgDump_Reconnect();}
#else // USE_DBGPRINT_LOGGER
#define Connect_DbgPrint()   {;}
#endif // USE_DBGPRINT_LOGGER

#ifdef SCSI_PORT_DBG_PRINT

SCSIPORT_API
VOID
DDKCDECLAPI
ScsiDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define PRINT_PREFIX                0, 

#define KdPrint2(_x_) {ScsiDebugPrint("%x: ", PsGetCurrentThread()) ; ScsiDebugPrint _x_ ; }
#define KdPrint(_x_) ScsiDebugPrint _x_

#else // SCSI_PORT_DBG_PRINT

#ifndef USE_DBGPRINT_LOGGER
ULONG
_cdecl
DbgPrint(
    const CHAR * Format,
    ...
    );
#endif // USE_DBGPRINT_LOGGER

#define PRINT_PREFIX

// Note, that using DbgPrint on raised IRQL will crash w2k
// tis will not happen immediately, so we shall see some logs
//#define LOG_ON_RAISED_IRQL_W2K    TRUE
//#define LOG_ON_RAISED_IRQL_W2K    FALSE

#define KdPrint2(_x_) {if(LOG_ON_RAISED_IRQL_W2K || MajorVersion < 0x05 || KeGetCurrentIrql() <= 2){/*DbgPrint("%x: ", PsGetCurrentThread()) ;*/ DbgPrint _x_ ; if(g_LogToDisplay){ PrintNtConsole _x_ ;} }}
#define KdPrint(_x_)  {if(LOG_ON_RAISED_IRQL_W2K || MajorVersion < 0x05 || KeGetCurrentIrql() <= 2){/*DbgPrint("%x: ", PsGetCurrentThread()) ;*/ DbgPrint _x_ ; if(g_LogToDisplay){ PrintNtConsole _x_ ;} }}
/*
#define PRINT_PREFIX_PTR ((PCHAR)&__tmp__kdprint__buff__)
#define PRINT_UPREFIX_PTR ((PWCHAR)&__tmp__kdprint__ubuff__)
#define PRINT_PREFIX     PRINT_PREFIX_PTR, 
#define KdPrint2(_x_) \
{ \
    WCHAR __tmp__kdprint__ubuff__[256]; \
    CHAR __tmp__kdprint__buff__[256]; \
    UNICODE_STRING __tmp__usrt__buff__; \
    sprintf _x_; \
    swprintf (PRINT_UPREFIX_PTR, L"%hs", PRINT_PREFIX_PTR); \
    __tmp__usrt__buff__.Buffer = PRINT_UPREFIX_PTR; \
    __tmp__usrt__buff__.Length = \
    __tmp__usrt__buff__.MaximumLength = strlen(PRINT_PREFIX_PTR); \
    NtDisplayString(&__tmp__usrt__buff__); \
};
#define KdPrint(_x_)  DbgPrint _x_
*/
#endif // SCSI_PORT_DBG_PRINT

//#define AtapiStallExecution(dt)  { KdPrint2(("  AtapiStallExecution(%d)\n", dt)); ScsiPortStallExecution(dt); }
#define AtapiStallExecution(dt)  { ScsiPortStallExecution(dt); }

#endif // _DBGNT_

#else // _DEBUG

#define KdPrint2(_x_)
#define KdPrint(_x_)
#define Connect_DbgPrint()   {;}

#define AtapiStallExecution(dt)  ScsiPortStallExecution(dt)

#endif // _DEBUG

// IDE register definition

#pragma pack(push, 1)

typedef union _IDE_REGISTERS_1 {
    struct _o {
        UCHAR Data;
        UCHAR Feature;
        UCHAR BlockCount;
        UCHAR BlockNumber;
        UCHAR CylinderLow;
        UCHAR CylinderHigh;
        UCHAR DriveSelect;
        UCHAR Command;
    } o;

    struct _i {
        UCHAR Data;
        UCHAR Error;
        UCHAR BlockCount;
        UCHAR BlockNumber;
        UCHAR CylinderLow;
        UCHAR CylinderHigh;
        UCHAR DriveSelect;
        UCHAR Status;
    } i;

} IDE_REGISTERS_1, *PIDE_REGISTERS_1;

#define IDX_IO1                     0
#define IDX_IO1_SZ                  sizeof(IDE_REGISTERS_1)

#define IDX_IO1                     0
#define IDX_IO1_SZ                  sizeof(IDE_REGISTERS_1)
#define IDX_IO1_i_Data              (FIELD_OFFSET(IDE_REGISTERS_1, i.Data        )+IDX_IO1)
#define IDX_IO1_i_Error             (FIELD_OFFSET(IDE_REGISTERS_1, i.Error       )+IDX_IO1)
#define IDX_IO1_i_BlockCount        (FIELD_OFFSET(IDE_REGISTERS_1, i.BlockCount  )+IDX_IO1)
#define IDX_IO1_i_BlockNumber       (FIELD_OFFSET(IDE_REGISTERS_1, i.BlockNumber )+IDX_IO1)
#define IDX_IO1_i_CylinderLow       (FIELD_OFFSET(IDE_REGISTERS_1, i.CylinderLow )+IDX_IO1)
#define IDX_IO1_i_CylinderHigh      (FIELD_OFFSET(IDE_REGISTERS_1, i.CylinderHigh)+IDX_IO1)
#define IDX_IO1_i_DriveSelect       (FIELD_OFFSET(IDE_REGISTERS_1, i.DriveSelect )+IDX_IO1)
#define IDX_IO1_i_Status            (FIELD_OFFSET(IDE_REGISTERS_1, i.Status      )+IDX_IO1)

#define IDX_IO1_o                   IDX_IO1_SZ
#define IDX_IO1_o_SZ                sizeof(IDE_REGISTERS_1)

#define IDX_IO1_o_Data              (FIELD_OFFSET(IDE_REGISTERS_1, o.Data        )+IDX_IO1_o)
#define IDX_IO1_o_Feature           (FIELD_OFFSET(IDE_REGISTERS_1, o.Feature     )+IDX_IO1_o)
#define IDX_IO1_o_BlockCount        (FIELD_OFFSET(IDE_REGISTERS_1, o.BlockCount  )+IDX_IO1_o)
#define IDX_IO1_o_BlockNumber       (FIELD_OFFSET(IDE_REGISTERS_1, o.BlockNumber )+IDX_IO1_o)
#define IDX_IO1_o_CylinderLow       (FIELD_OFFSET(IDE_REGISTERS_1, o.CylinderLow )+IDX_IO1_o)
#define IDX_IO1_o_CylinderHigh      (FIELD_OFFSET(IDE_REGISTERS_1, o.CylinderHigh)+IDX_IO1_o)
#define IDX_IO1_o_DriveSelect       (FIELD_OFFSET(IDE_REGISTERS_1, o.DriveSelect )+IDX_IO1_o)
#define IDX_IO1_o_Command           (FIELD_OFFSET(IDE_REGISTERS_1, o.Command     )+IDX_IO1_o)

typedef struct _IDE_REGISTERS_2 {
    UCHAR AltStatus;
    UCHAR DriveAddress;
} IDE_REGISTERS_2, *PIDE_REGISTERS_2;

#define IDX_IO2                     (IDX_IO1_o+IDX_IO1_o_SZ)
#define IDX_IO2_SZ                  sizeof(IDE_REGISTERS_2)

#define IDX_IO2_AltStatus           (FIELD_OFFSET(IDE_REGISTERS_2, AltStatus   )+IDX_IO2)
#define IDX_IO2_DriveAddress        (FIELD_OFFSET(IDE_REGISTERS_2, DriveAddress)+IDX_IO2)

#define IDX_IO2_o                   (IDX_IO2+IDX_IO2_SZ)
#define IDX_IO2_o_SZ                sizeof(IDE_REGISTERS_2)

#define IDX_IO2_o_Control           (FIELD_OFFSET(IDE_REGISTERS_2, AltStatus   )+IDX_IO2_o)
//
// Device Extension Device Flags
//

#define DFLAGS_DEVICE_PRESENT        0x0001    // Indicates that some device is present.
#define DFLAGS_ATAPI_DEVICE          0x0002    // Indicates whether ATAPI commands can be used.
#define DFLAGS_TAPE_DEVICE           0x0004    // Indicates whether this is a tape device.
#define DFLAGS_INT_DRQ               0x0008    // Indicates whether device interrupts as DRQ is set after
                                               // receiving ATAPI Packet Command
#define DFLAGS_REMOVABLE_DRIVE       0x0010    // Indicates that the drive has the 'removable' bit set in
                                               // identify data (offset 128)
#define DFLAGS_MEDIA_STATUS_ENABLED  0x0020    // Media status notification enabled
#define DFLAGS_ATAPI_CHANGER         0x0040    // Indicates atapi 2.5 changer present.
#define DFLAGS_SANYO_ATAPI_CHANGER   0x0080    // Indicates multi-platter device, not conforming to the 2.5 spec.
#define DFLAGS_CHANGER_INITED        0x0100    // Indicates that the init path for changers has already been done.
#define DFLAGS_LBA_ENABLED           0x0200    // Indicates that we should use LBA addressing rather than CHS
#define DFLAGS_DWORDIO_ENABLED       0x0400    // Indicates that we should use 32-bit IO
#define DFLAGS_WCACHE_ENABLED        0x0800    // Indicates that we use write cache
#define DFLAGS_RCACHE_ENABLED        0x1000    // Indicates that we use read cache
#define DFLAGS_ORIG_GEOMETRY         0x2000    // 
#define DFLAGS_REINIT_DMA            0x4000    // 
//
// Used to disable 'advanced' features.
//

#define MAX_ERRORS                     4

//
// ATAPI command definitions
//

#define ATAPI_MODE_SENSE   0x5A
#define ATAPI_MODE_SELECT  0x55
#define ATAPI_FORMAT_UNIT  0x24

// ATAPI Command Descriptor Block

typedef struct _MODE_SENSE_10 {
        UCHAR OperationCode;
        UCHAR Reserved1;
        UCHAR PageCode : 6;
        UCHAR Pc : 2;
        UCHAR Reserved2[4];
        UCHAR ParameterListLengthMsb;
        UCHAR ParameterListLengthLsb;
        UCHAR Reserved3[3];
} MODE_SENSE_10, *PMODE_SENSE_10;

typedef struct _MODE_SELECT_10 {
        UCHAR OperationCode;
        UCHAR Reserved1 : 4;
        UCHAR PFBit : 1;
        UCHAR Reserved2 : 3;
        UCHAR Reserved3[5];
        UCHAR ParameterListLengthMsb;
        UCHAR ParameterListLengthLsb;
        UCHAR Reserved4[3];
} MODE_SELECT_10, *PMODE_SELECT_10;

typedef struct _MODE_PARAMETER_HEADER_10 {
    UCHAR ModeDataLengthMsb;
    UCHAR ModeDataLengthLsb;
    UCHAR MediumType;
    UCHAR Reserved[5];
}MODE_PARAMETER_HEADER_10, *PMODE_PARAMETER_HEADER_10;

//
// IDE command definitions
//

#define IDE_COMMAND_ATAPI_RESET      0x08
#define IDE_COMMAND_RECALIBRATE      0x10
#define IDE_COMMAND_READ             0x20
#define IDE_COMMAND_READ48           0x24
#define IDE_COMMAND_READ_DMA48       0x25
#define IDE_COMMAND_READ_DMA_Q48     0x26
#define IDE_COMMAND_READ_NATIVE_SIZE48   0x27
#define IDE_COMMAND_READ_MUL48           0x29
#define IDE_COMMAND_READ_STREAM_DMA48    0x2A
#define IDE_COMMAND_READ_STREAM48        0x2B
#define IDE_COMMAND_READ_LOG48           0x2f
#define IDE_COMMAND_WRITE                0x30
#define IDE_COMMAND_WRITE48              0x34
#define IDE_COMMAND_WRITE_DMA48          0x35
#define IDE_COMMAND_WRITE_DMA_Q48        0x36
#define IDE_COMMAND_SET_NATIVE_SIZE48    0x37
#define IDE_COMMAND_WRITE_MUL48          0x39
#define IDE_COMMAND_WRITE_STREAM_DMA48   0x3a
#define IDE_COMMAND_WRITE_STREAM48       0x3b
#define IDE_COMMAND_WRITE_FUA_DMA48      0x3d
#define IDE_COMMAND_WRITE_FUA_DMA_Q48    0x3e
#define IDE_COMMAND_WRITE_LOG48          0x3f
#define IDE_COMMAND_VERIFY               0x40
#define IDE_COMMAND_VERIFY48             0x42
#define IDE_COMMAND_READ_LOG_DMA48       0x47
#define IDE_COMMAND_WRITE_LOG_DMA48      0x57
#define IDE_COMMAND_TRUSTED_RCV          0x5c
#define IDE_COMMAND_TRUSTED_RCV_DMA      0x5d
#define IDE_COMMAND_TRUSTED_SEND         0x5e
#define IDE_COMMAND_TRUSTED_SEND_DMA     0x5f
#define IDE_COMMAND_SEEK                 0x70
#define IDE_COMMAND_SET_DRIVE_PARAMETERS 0x91
#define IDE_COMMAND_ATAPI_PACKET     0xA0
#define IDE_COMMAND_ATAPI_IDENTIFY   0xA1
#define IDE_COMMAND_READ_MULTIPLE    0xC4
#define IDE_COMMAND_WRITE_MULTIPLE   0xC5
#define IDE_COMMAND_SET_MULTIPLE     0xC6
#define IDE_COMMAND_READ_DMA_Q       0xC7
#define IDE_COMMAND_READ_DMA         0xC8
#define IDE_COMMAND_WRITE_DMA        0xCA
#define IDE_COMMAND_WRITE_DMA_Q      0xCC
#define IDE_COMMAND_WRITE_MUL_FUA48  0xCE
#define IDE_COMMAND_GET_MEDIA_STATUS 0xDA
#define IDE_COMMAND_DOOR_LOCK        0xDE
#define IDE_COMMAND_DOOR_UNLOCK      0xDF
#define IDE_COMMAND_STANDBY_IMMED    0xE0 // flush and spin down
#define IDE_COMMAND_STANDBY          0xE2 // flush and spin down and enable autopowerdown timer
#define IDE_COMMAND_SLEEP            0xE6 // flush, spin down and deactivate interface
#define IDE_COMMAND_FLUSH_CACHE      0xE7
#define IDE_COMMAND_IDENTIFY         0xEC
#define IDE_COMMAND_MEDIA_EJECT      0xED
#define IDE_COMMAND_FLUSH_CACHE48    0xEA
#define IDE_COMMAND_ENABLE_MEDIA_STATUS  0xEF
#define	IDE_COMMAND_SET_FEATURES     0xEF      /* features command, 
                                                 IDE_COMMAND_ENABLE_MEDIA_STATUS */
#define IDE_COMMAND_READ_NATIVE_SIZE 0xF8
#define IDE_COMMAND_SET_NATIVE_SIZE  0xF9

#define SCSIOP_ATA_PASSTHROUGH       0xCC // 

//
// IDE status definitions
//

#define IDE_STATUS_SUCCESS           0x00
#define IDE_STATUS_ERROR             0x01
#define IDE_STATUS_INDEX             0x02
#define IDE_STATUS_CORRECTED_ERROR   0x04
#define IDE_STATUS_DRQ               0x08
#define IDE_STATUS_DSC               0x10
//#define IDE_STATUS_DWF               0x10      /* drive write fault */
#define IDE_STATUS_DMA               0x20      /* DMA ready */
#define IDE_STATUS_DWF               0x20      /* drive write fault */
#define IDE_STATUS_DRDY              0x40
#define IDE_STATUS_IDLE              0x50
#define IDE_STATUS_BUSY              0x80


//
// IDE drive select/head definitions
//

#define IDE_DRIVE_SELECT             0xA0
#define IDE_DRIVE_1                  0x00
#define IDE_DRIVE_2                  0x10
#define IDE_DRIVE_SELECT_1           (IDE_DRIVE_SELECT | IDE_DRIVE_1)
#define IDE_DRIVE_SELECT_2           (IDE_DRIVE_SELECT | IDE_DRIVE_2)

#define IDE_USE_LBA                  0x40

//
// IDE drive control definitions
//

#define IDE_DC_DISABLE_INTERRUPTS    0x02
#define IDE_DC_RESET_CONTROLLER      0x04
#define IDE_DC_A_4BIT                0x80
#define IDE_DC_USE_HOB               0x80 // use high-order byte(s)
#define IDE_DC_REENABLE_CONTROLLER   0x00

// IDE error definitions
//

#define IDE_ERROR_ICRC               0x80
#define IDE_ERROR_BAD_BLOCK          0x80
#define IDE_ERROR_DATA_ERROR         0x40
#define IDE_ERROR_MEDIA_CHANGE       0x20
#define IDE_ERROR_ID_NOT_FOUND       0x10
#define IDE_ERROR_MEDIA_CHANGE_REQ   0x08
#define IDE_ERROR_COMMAND_ABORTED    0x04
#define IDE_ERROR_END_OF_MEDIA       0x02
#define IDE_ERROR_NO_MEDIA           0x02
#define IDE_ERROR_ILLEGAL_LENGTH     0x01

//
// ATAPI register definition
//

typedef union _ATAPI_REGISTERS_1 {
    struct _o {
        UCHAR Data;
        UCHAR Feature;
        UCHAR Unused0;
        UCHAR Unused1;
        UCHAR ByteCountLow;
        UCHAR ByteCountHigh;
        UCHAR DriveSelect;
        UCHAR Command;
    } o;

    struct _i {
        UCHAR Data;
        UCHAR Error;
        UCHAR InterruptReason;
        UCHAR Unused1;
        UCHAR ByteCountLow;
        UCHAR ByteCountHigh;
        UCHAR DriveSelect;
        UCHAR Status;
    } i;

    //IDE_REGISTERS_1 ide;

} ATAPI_REGISTERS_1, *PATAPI_REGISTERS_1;

#define IDX_ATAPI_IO1                     IDX_IO1
#define IDX_ATAPI_IO1_SZ                  sizeof(ATAPI_REGISTERS_1)

#define IDX_ATAPI_IO1_i_Data              (FIELD_OFFSET(ATAPI_REGISTERS_1, i.Data           )+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_i_Error             (FIELD_OFFSET(ATAPI_REGISTERS_1, i.Error          )+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_i_InterruptReason   (FIELD_OFFSET(ATAPI_REGISTERS_1, i.InterruptReason)+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_i_Unused1           (FIELD_OFFSET(ATAPI_REGISTERS_1, i.Unused1        )+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_i_ByteCountLow      (FIELD_OFFSET(ATAPI_REGISTERS_1, i.ByteCountLow   )+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_i_ByteCountHigh     (FIELD_OFFSET(ATAPI_REGISTERS_1, i.ByteCountHigh  )+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_i_DriveSelect       (FIELD_OFFSET(ATAPI_REGISTERS_1, i.DriveSelect    )+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_i_Status            (FIELD_OFFSET(ATAPI_REGISTERS_1, i.Status         )+IDX_ATAPI_IO1)

#define IDX_ATAPI_IO1_o_Data              (FIELD_OFFSET(ATAPI_REGISTERS_1, o.Data         )+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_o_Feature           (FIELD_OFFSET(ATAPI_REGISTERS_1, o.Feature      )+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_o_Unused0           (FIELD_OFFSET(ATAPI_REGISTERS_1, o.Unused0      )+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_o_Unused1           (FIELD_OFFSET(ATAPI_REGISTERS_1, o.Unused1      )+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_o_ByteCountLow      (FIELD_OFFSET(ATAPI_REGISTERS_1, o.ByteCountLow )+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_o_ByteCountHigh     (FIELD_OFFSET(ATAPI_REGISTERS_1, o.ByteCountHigh)+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_o_DriveSelect       (FIELD_OFFSET(ATAPI_REGISTERS_1, o.DriveSelect  )+IDX_ATAPI_IO1)
#define IDX_ATAPI_IO1_o_Command           (FIELD_OFFSET(ATAPI_REGISTERS_1, o.Command      )+IDX_ATAPI_IO1)

/*
typedef union _ATAPI_REGISTERS_2 {
    struct {
        UCHAR AltStatus;
        UCHAR DriveAddress;
    };

    //IDE_REGISTERS_2 ide;

} ATAPI_REGISTERS_2, *PATAPI_REGISTERS_2;

#define IDX_ATAPI_IO2               IDX_ATAPI_IO2_SZ
#define IDX_ATAPI_IO2_SZ            sizeof(ATAPI_REGISTERS_2)
*/

//
// ATAPI interrupt reasons
//

#define ATAPI_IR_COD 0x01
#define ATAPI_IR_IO  0x02

//
// ATA Features
//

#define         ATA_F_DMA               0x01    /* enable DMA */
#define         ATA_F_OVL               0x02    /* enable overlap */

#define		ATA_C_F_SETXFER	        0x03	/* set transfer mode */

#define         ATA_C_F_ENAB_WCACHE     0x02    /* enable write cache */
#define         ATA_C_F_DIS_WCACHE      0x82    /* disable write cache */

#define         ATA_C_F_ENAB_RCACHE     0xaa    /* enable readahead cache */
#define         ATA_C_F_DIS_RCACHE      0x55    /* disable readahead cache */

#define         ATA_C_F_ENAB_RELIRQ     0x5d    /* enable release interrupt */
#define         ATA_C_F_DIS_RELIRQ      0xdd    /* disable release interrupt */

#define         ATA_C_F_ENAB_SRVIRQ     0x5e    /* enable service interrupt */
#define         ATA_C_F_DIS_SRVIRQ      0xde    /* disable service interrupt */

#define         ATA_C_F_ENAB_MEDIASTAT  0x95    /* enable media status */
#define         ATA_C_F_DIS_MEDIASTAT   0x31    /* disable media status */

//
// ATAPI interrupt reasons
//

#define		ATA_I_CMD		0x01	/* cmd (1) | data (0) */
#define		ATA_I_IN		0x02	/* read (1) | write (0) */
#define		ATA_I_RELEASE		0x04	/* released bus (1) */
#define		ATA_I_TAGMASK		0xf8	/* tag mask */

// IDENTIFY data
//

typedef struct _IDENTIFY_DATA {
    UCHAR  AtapiCmdSize:2;                 // 00 00
#define         ATAPI_PSIZE_12          0       /* 12 bytes */
#define         ATAPI_PSIZE_16          1       /* 16 bytes */
    UCHAR  :3;
    UCHAR  DrqType:2;                      // 00 00
#define         ATAPI_DRQT_MPROC        0       /* cpu    3 ms delay */
#define         ATAPI_DRQT_INTR         1       /* intr  10 ms delay */
#define         ATAPI_DRQT_ACCEL        2       /* accel 50 us delay */
    UCHAR  Removable:1;

    UCHAR  DeviceType:5;
#define         ATAPI_TYPE_DIRECT       0       /* disk/floppy */
#define         ATAPI_TYPE_TAPE         1       /* streaming tape */
#define         ATAPI_TYPE_CDROM        5       /* CD-ROM device */
#define         ATAPI_TYPE_OPTICAL      7       /* optical disk */
    UCHAR  :1;
    UCHAR  CmdProtocol:2;                      // 00 00
#define         ATAPI_PROTO_ATAPI       2
//    USHORT GeneralConfiguration;            // 00 00

    USHORT NumberOfCylinders;               // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumberOfHeads;                   // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4  // Now obsolete
    USHORT UnformattedBytesPerSector;       // 0A  5  // Now obsolete
    USHORT SectorsPerTrack;                 // 0C  6

    USHORT VendorUnique1[3];                // 0E  7-9
    UCHAR  SerialNumber[20];                // 14  10-19

    USHORT BufferType;                      // 28  20
#define ATA_BT_SINGLEPORTSECTOR		1	/* 1 port, 1 sector buffer */
#define ATA_BT_DUALPORTMULTI		2	/* 2 port, mult sector buffer */
#define ATA_BT_DUALPORTMULTICACHE	3	/* above plus track cache */

    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    USHORT FirmwareRevision[4];             // 2E  23-26
    USHORT ModelNumber[20];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F

    USHORT DoubleWordIo;                    // 60  48

    USHORT Reserved62_0:8;                  // 62  49
    USHORT SupportDma:1;                    
    USHORT SupportLba:1;
    USHORT DisableIordy:1;
    USHORT SupportIordy:1;
    USHORT SoftReset:1;
    USHORT StandbyOverlap:1;
    USHORT SupportQTag:1;                                 /* supports queuing overlap */
    USHORT SupportIDma:1;                                 /* interleaved DMA supported */
/*    USHORT Capabilities;                    // 62  49
#define IDENTIFY_CAPABILITIES_SUPPORT_DMA   0x0100
#define IDENTIFY_CAPABILITIES_SUPPORT_LBA   0x0200
#define IDENTIFY_CAPABILITIES_DISABLE_IORDY 0x0400
#define IDENTIFY_CAPABILITIES_SUPPORT_IORDY 0x0800
#define IDENTIFY_CAPABILITIES_SOFT_RESET    0x1000
#define IDENTIFY_CAPABILITIES_STDBY_OVLP    0x2000
#define IDENTIFY_CAPABILITIES_SUPPORT_QTAG  0x4000
#define IDENTIFY_CAPABILITIES_SUPPORT_IDMA  0x8000*/

    USHORT DeviceStandbyMin:1;              // 64  50      
    USHORT Reserved50_1:13;
    USHORT DeviceCapability1:1;
    USHORT DeviceCapability0:1;
//    USHORT Reserved2;                       

    UCHAR  Vendor51;                        // 66  51
    UCHAR  PioCycleTimingMode;              // 67

    UCHAR  Vendor52;                        // 68  52
    UCHAR  DmaCycleTimingMode;              // 69

    USHORT TranslationFieldsValid:1;        // 6A  53    /* 54-58 */
    USHORT PioTimingsValid:1;                            /* 64-70 */
    USHORT UdmaModesValid:1;                             /* 88 */
    USHORT Reserved3:13;

    USHORT NumberOfCurrentCylinders;        // 6C  54    \-
    USHORT NumberOfCurrentHeads;            // 6E  55     \-
    USHORT CurrentSectorsPerTrack;          // 70  56     /- obsolete USHORT[5]
    ULONG  CurrentSectorCapacity;           // 72  57-58 /-

    USHORT CurrentMultiSector:8;            //     59
    USHORT CurrentMultiSectorValid:1;
    USHORT Reserved59_9:7;

    ULONG  UserAddressableSectors;          //     60-61

    USHORT SingleWordDMASupport : 8;        //     62  \- obsolete
    USHORT SingleWordDMAActive : 8;         //         /-

    USHORT MultiWordDMASupport : 8;         //     63
    USHORT MultiWordDMAActive : 8;

    USHORT AdvancedPIOModes : 8;            //     64
    USHORT Reserved4 : 8;

#define AdvancedPIOModes_3                  1
#define AdvancedPIOModes_4                  2
#define AdvancedPIOModes_5                  4      // non-standard

    USHORT MinimumMWXferCycleTime;          //     65
    USHORT RecommendedMWXferCycleTime;      //     66
    USHORT MinimumPIOCycleTime;             //     67
    USHORT MinimumPIOCycleTimeIORDY;        //     68

    USHORT Reserved69_70[2];                //     69-70
    USHORT ReleaseTimeOverlapped;           //     71
    USHORT ReleaseTimeServiceCommand;       //     72
    USHORT Reserved73_74[2];                //     73-74

    USHORT QueueLength : 5;                 //     75
    USHORT Reserved75_6 : 11;

    USHORT SataCapabilities;                //     76
#define ATA_SATA_GEN1			0x0002
#define ATA_SATA_GEN2			0x0004
#define ATA_SUPPORT_NCQ			0x0100
#define ATA_SUPPORT_IFPWRMNGTRCV	0x0200

    USHORT Reserved77;                      //     77

    USHORT SataSupport;                     //     78
#define ATA_SUPPORT_NONZERO		0x0002
#define ATA_SUPPORT_AUTOACTIVATE	0x0004
#define ATA_SUPPORT_IFPWRMNGT		0x0008
#define ATA_SUPPORT_INORDERDATA		0x0010

    USHORT SataEnable;                      //     79
    USHORT MajorRevision;                   //     80
    USHORT MinorRevision;                   //     81

    struct {
        USHORT Smart:1;                     //     82/85
        USHORT Security:1;
        USHORT Removable:1;
        USHORT PowerMngt:1;
        USHORT Packet:1;
        USHORT WriteCache:1;
        USHORT LookAhead:1;
        USHORT ReleaseDRQ:1;
        USHORT ServiceDRQ:1;
        USHORT Reset:1;
        USHORT Protected:1;
        USHORT Reserved_82_11:1;
        USHORT WriteBuffer:1;
        USHORT ReadBuffer:1;
        USHORT Nop:1;
        USHORT Reserved_82_15:1;

        USHORT Microcode:1;                  //     83/86
        USHORT Queued:1;                     //     
        USHORT CFA:1;                        //     
        USHORT APM:1;                        //     
        USHORT Notify:1;                     //     
        USHORT Standby:1;                    //     
        USHORT Spinup:1;                     //     
        USHORT Reserver_83_7:1;
        USHORT MaxSecurity:1;                //
        USHORT AutoAcoustic:1;               //
        USHORT Address48:1;                  //
        USHORT ConfigOverlay:1;              //
        USHORT FlushCache:1;                 //
        USHORT FlushCache48:1;               //
        USHORT SupportOne:1;                 //
        USHORT SupportZero:1;                //

        USHORT SmartErrorLog:1;              //     84/87
        USHORT SmartSelfTest:1;
        USHORT MediaSerialNo:1;
        USHORT MediaCardPass:1;
        USHORT Streaming:1;
        USHORT Logging:1;
        USHORT Reserver_84_6:8;
        USHORT ExtendedOne:1;                 //
        USHORT ExtendedZero:1;                //
    } FeaturesSupport, FeaturesEnabled;

    USHORT UltraDMASupport : 8;             //     88
    USHORT UltraDMAActive : 8;
    
    USHORT EraseTime;                       //     89
    USHORT EnhancedEraseTime;               //     90
    USHORT CurentAPMLevel;                  //     91

    USHORT MasterPasswdRevision;            //     92

    USHORT HwResMaster : 8;                 //     93
    USHORT HwResSlave : 5;
    USHORT HwResCableId : 1;
    USHORT HwResValid : 2;

    USHORT CurrentAcoustic : 8;             //     94
    USHORT VendorAcoustic : 8;

    USHORT StreamMinReqSize;                //     95
    USHORT StreamTransferTime;              //     96
    USHORT StreamAccessLatency;             //     97
    ULONG  StreamGranularity;               //     98-99

    ULONGLONG UserAddressableSectors48;     //     100-103

    USHORT Reserved104[2];                  //     104-105
    union {
        USHORT PhysLogSectorSize;               //     106
        struct {
            USHORT PLSS_Size:4;
            USHORT PLSS_Reserved:8;
            USHORT PLSS_LargeL:1;  // =1 if 117-118 are valid
            USHORT PLSS_LargeP:1;
            USHORT PLSS_Signature:2; // = 0x01 = 01b
        };
    };
    USHORT Reserved107[10];                 //     107-116

    ULONG  LargeSectorSize;                 //     117-118
    
    USHORT Reserved117[8];                  //     119-126
    
    USHORT RemovableStatus;                 //     127
    USHORT SecurityStatus;                  //     128

    USHORT FeaturesSupport4;                //     129
    USHORT Reserved130[30];                 //     130-159
    USHORT CfAdvPowerMode;                  //     160
    USHORT Reserved161[14];                 //     161-175
    USHORT MediaSerial[30];                 //     176-205
    USHORT Reserved206[49];                 //     205-254
    USHORT Integrity;                       // 255
} IDENTIFY_DATA, *PIDENTIFY_DATA;

//
// Identify data without the Reserved4.
//

#define IDENTIFY_DATA2      IDENTIFY_DATA
#define PIDENTIFY_DATA2     PIDENTIFY_DATA

/*typedef struct _IDENTIFY_DATA2 {
    UCHAR  AtapiCmdSize:2;                 // 00 00
    UCHAR  :3;
    UCHAR  DrqType:2;                      // 00 00
    UCHAR  Removable:1;

    UCHAR  DeviceType:5;
    UCHAR  :1;
    UCHAR  CmdProtocol:2;                      // 00 00
//    USHORT GeneralConfiguration;            // 00
  
    USHORT NumberOfCylinders;               // 02
    USHORT Reserved1;                       // 04
    USHORT NumberOfHeads;                   // 06
    USHORT UnformattedBytesPerTrack;        // 08
    USHORT UnformattedBytesPerSector;       // 0A
    USHORT SectorsPerTrack;                 // 0C
    USHORT VendorUnique1[3];                // 0E
    UCHAR  SerialNumber[20];                // 14
    USHORT BufferType;                      // 28
    USHORT BufferSectorSize;                // 2A
    USHORT NumberOfEccBytes;                // 2C
    USHORT FirmwareRevision[4];             // 2E
    USHORT ModelNumber[20];                 // 36
    UCHAR  MaximumBlockTransfer;            // 5E
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60
    USHORT Capabilities;                    // 62
    USHORT Reserved2;                       // 64
    UCHAR  VendorUnique3;                   // 66
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:1;        // 6A
    USHORT Reserved3:15;
    USHORT NumberOfCurrentCylinders;        // 6C
    USHORT NumberOfCurrentHeads;            // 6E
    USHORT CurrentSectorsPerTrack;          // 70
    ULONG  CurrentSectorCapacity;           // 72
} IDENTIFY_DATA2, *PIDENTIFY_DATA2;*/

#define IDENTIFY_DATA_SIZE sizeof(IDENTIFY_DATA)

// IDENTIFY DMA timing cycle modes.
#define IDENTIFY_DMA_CYCLES_MODE_0 0x00
#define IDENTIFY_DMA_CYCLES_MODE_1 0x01
#define IDENTIFY_DMA_CYCLES_MODE_2 0x02
/*
#define PCI_DEV_HW_SPEC(idhi, idlo) \
    { #idlo, 4, #idhi, 4}

typedef struct _BROKEN_CONTROLLER_INFORMATION {
    PCHAR   VendorId;
    ULONG   VendorIdLength;
    PCHAR   DeviceId;
    ULONG   DeviceIdLength;
}BROKEN_CONTROLLER_INFORMATION, *PBROKEN_CONTROLLER_INFORMATION;

BROKEN_CONTROLLER_INFORMATION const BrokenAdapters[] = {
    // CMD 640 ATA controller !WARNING! buggy chip data loss possible
    PCI_DEV_HW_SPEC( 0640, 1095 ), //{ "1095", 4, "0640", 4},
    // ??
    PCI_DEV_HW_SPEC( 0601, 1039 ), //{ "1039", 4, "0601", 4}
    // RZ 100? ATA controller !WARNING! buggy chip data loss possible
    PCI_DEV_HW_SPEC( 1000, 1042 ),
    PCI_DEV_HW_SPEC( 1001, 1042 )
};

#define BROKEN_ADAPTERS (sizeof(BrokenAdapters) / sizeof(BROKEN_CONTROLLER_INFORMATION))

typedef struct _NATIVE_MODE_CONTROLLER_INFORMATION {
    PCHAR   VendorId;
    ULONG   VendorIdLength;
    PCHAR   DeviceId;
    ULONG   DeviceIdLength;
}NATIVE_MODE_CONTROLLER_INFORMATION, *PNATIVE_MODE_CONTROLLER_INFORMATION;

NATIVE_MODE_CONTROLLER_INFORMATION const NativeModeAdapters[] = {
    PCI_DEV_HW_SPEC( 0105, 10ad ) //{ "10ad", 4, "0105", 4}
};

#define NUM_NATIVE_MODE_ADAPTERS (sizeof(NativeModeAdapters) / sizeof(NATIVE_MODE_CONTROLLER_INFORMATION))
*/
//
// Beautification macros
//

#ifndef USER_MODE

#define GetStatus(chan, Status) \
    Status = AtapiReadPort1(chan, IDX_IO2_AltStatus);

#define GetBaseStatus(chan, pStatus) \
    pStatus = AtapiReadPort1(chan, IDX_IO1_i_Status);

#define WriteCommand(chan, _Command) \
    AtapiWritePort1(chan, IDX_IO1_o_Command, _Command);


#define SelectDrive(chan, unit) \
    AtapiWritePort1(chan, IDX_IO1_o_DriveSelect, (unit) ? IDE_DRIVE_SELECT_2 : IDE_DRIVE_SELECT_1);


#define ReadBuffer(chan, Buffer, Count, timing) \
    AtapiReadBuffer2(chan, IDX_IO1_i_Data, \
                                 Buffer, \
                                 Count, \
                                 timing);

#define WriteBuffer(chan, Buffer, Count, timing) \
    AtapiWriteBuffer2(chan, IDX_IO1_o_Data, \
                                  Buffer, \
                                 Count, \
                                 timing);

#define ReadBuffer2(chan, Buffer, Count, timing) \
    AtapiReadBuffer4(chan, IDX_IO1_i_Data, \
                             Buffer, \
                                 Count, \
                                 timing);

#define WriteBuffer2(chan, Buffer, Count, timing) \
    AtapiWriteBuffer4(chan, IDX_IO1_o_Data, \
                              Buffer, \
                                 Count, \
                                 timing);

UCHAR
DDKFASTAPI
WaitOnBusy(
    IN struct _HW_CHANNEL*   chan/*,
    PIDE_REGISTERS_2 BaseIoAddress*/
    );

UCHAR
DDKFASTAPI
WaitOnBusyLong(
    IN struct _HW_CHANNEL*   chan/*,
    PIDE_REGISTERS_2 BaseIoAddress*/
    );

UCHAR
DDKFASTAPI
WaitOnBaseBusy(
    IN struct _HW_CHANNEL*   chan/*,
    PIDE_REGISTERS_1 BaseIoAddress*/
    );

UCHAR
DDKFASTAPI
WaitOnBaseBusyLong(
    IN struct _HW_CHANNEL*   chan/*,
    PIDE_REGISTERS_1 BaseIoAddress*/
    );

UCHAR
DDKFASTAPI
WaitForDrq(
    IN struct _HW_CHANNEL*   chan/*,
    PIDE_REGISTERS_2 BaseIoAddress*/
    );

UCHAR
DDKFASTAPI
WaitShortForDrq(
    IN struct _HW_CHANNEL*   chan/*,
    PIDE_REGISTERS_2 BaseIoAddress*/
    );

VOID
DDKFASTAPI
AtapiSoftReset(
    IN struct _HW_CHANNEL*   chan,/*
    PIDE_REGISTERS_1 BaseIoAddress*/
    ULONG            DeviceNumber
    );

/*#define IdeHardReset(BaseIoAddress,result) \
{\
    UCHAR statusByte;\
    ULONG i;\
    SelectDrive(BaseIoAddress,DeviceNumber); \
    AtapiWritePort1(&BaseIoAddress->AltStatus,/*IDE_DC_DISABLE_INTERRUPTS | *//*IDE_DC_RESET_CONTROLLER );\
/*    ScsiPortStallExecution(50 * 1000);\
    AtapiWritePort1(&BaseIoAddress->AltStatus,IDE_DC_REENABLE_CONTROLLER);\
    /* 5 seconds for reset */ \
/*    for (i = 0; i < 1000 * (1+11); i++) {\
        statusByte = AtapiReadPort1(&BaseIoAddress->AltStatus);\
        if (statusByte != IDE_STATUS_IDLE && statusByte != IDE_STATUS_SUCCESS) {\
            ScsiPortStallExecution((i<1000) ? 5 : 500);\
        } else {\
            break;\
        }\
    }\
    KdPrint2((PRINT_PREFIX "IdeHardReset: Status %x\n", statusByte)); \
    if (i == 1000*1000) {\
        result = FALSE;\
    }\
    result = TRUE;\
}*/

#endif //USER_MODE

#define IS_RDP(OperationCode)\
    ((OperationCode == SCSIOP_ERASE)||\
    (OperationCode == SCSIOP_LOAD_UNLOAD)||\
    (OperationCode == SCSIOP_LOCATE)||\
    (OperationCode == SCSIOP_REWIND) ||\
    (OperationCode == SCSIOP_SPACE)||\
    (OperationCode == SCSIOP_SEEK)||\
/*    (OperationCode == SCSIOP_FORMAT_UNIT)||\
    (OperationCode == SCSIOP_BLANK)||*/ \
    (OperationCode == SCSIOP_WRITE_FILEMARKS))

#ifndef USER_MODE

PSCSI_REQUEST_BLOCK
BuildMechanismStatusSrb (
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

PSCSI_REQUEST_BLOCK
BuildRequestSenseSrb (
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
AtapiHwInitializeChanger (
    IN PVOID HwDeviceExtension,
    IN ULONG TargetId,
    IN PMECHANICAL_STATUS_INFORMATION_HEADER MechanismStatus
    );

ULONG
AtapiSendCommand(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN ULONG CmdAction
    );

ULONG
IdeSendCommand(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN ULONG CmdAction
    );

#define AtapiCopyMemory RtlCopyMemory

VOID
AtapiHexToString (
    ULONG Value,
    PCHAR *Buffer
    );

#define AtapiStringCmp(s1, s2, n)  _strnicmp(s1, s2, n)

BOOLEAN
AtapiInterrupt(
    IN PVOID HwDeviceExtension
    );

BOOLEAN
AtapiInterrupt__(
    IN PVOID HwDeviceExtension,
    IN UCHAR c
    );

BOOLEAN
AtapiHwInitialize(
    IN PVOID HwDeviceExtension
        );

ULONG
IdeBuildSenseBuffer(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
IdeMediaStatus(
    IN BOOLEAN EnableMSN,
    IN PVOID HwDeviceExtension,
    IN UCHAR Channel
    );

ULONG DDKAPI
AtapiFindController(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

ULONG
AtapiParseArgumentString(
    IN PCHAR String,
    IN PCHAR KeyWord
    );

BOOLEAN
IssueIdentify(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG Channel,
    IN UCHAR Command,
    IN BOOLEAN NoSetup
    );

BOOLEAN
SetDriveParameters(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG Channel
    );

ULONG
CheckDevice(
    IN PVOID   HwDeviceExtension,
    IN ULONG   Channel,
    IN ULONG   deviceNumber,
    IN BOOLEAN ResetBus
    );

BOOLEAN
FindDevices(
    IN PVOID HwDeviceExtension,
    IN BOOLEAN AtapiOnly,
    IN ULONG   Channel
    );

#endif //USER_MODE

#ifdef __cplusplus
};
#endif //__cplusplus

#ifndef USER_MODE

BOOLEAN
AtapiResetController(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
    );

BOOLEAN
AtapiStartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN
AtapiStartIo__(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN BOOLEAN TopLevel
    );

extern UCHAR
AtaCommand48(
//    IN PVOID HwDeviceExtension,
    IN struct _HW_DEVICE_EXTENSION* deviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG Channel,
    IN UCHAR command,
    IN ULONGLONG lba,
    IN USHORT count,
    IN USHORT feature,
    IN ULONG flags
    );

extern UCHAR
AtaCommand(
//    IN PVOID HwDeviceExtension,
    IN struct _HW_DEVICE_EXTENSION* deviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG Channel,
    IN UCHAR command,
    IN USHORT cylinder,
    IN UCHAR head,
    IN UCHAR sector, 
    IN UCHAR count,
    IN UCHAR feature,
    IN ULONG flags
    );

extern LONG
AtaPioMode(PIDENTIFY_DATA2 ident);

extern LONG
AtaWmode(PIDENTIFY_DATA2 ident);

extern LONG
AtaUmode(PIDENTIFY_DATA2 ident);

extern VOID
AtapiDpcDispatch(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );
 

//#define AtaCommand(de, devn, chan, cmd, cyl, hd, sec, cnt, feat, flg) \

extern LONG
AtaPio2Mode(LONG pio);

extern LONG
AtaPioMode(PIDENTIFY_DATA2 ident);

extern VOID
AtapiEnableInterrupts(
    IN PVOID HwDeviceExtension,
    IN ULONG c
    );

extern VOID
AtapiDisableInterrupts(
    IN PVOID HwDeviceExtension,
    IN ULONG c
    );

#define CHAN_NOT_SPECIFIED      (0xffffffffL)
#define DEVNUM_NOT_SPECIFIED    (0xffffffffL)

extern ULONG
AtapiRegCheckDevValue(
    IN PVOID HwDeviceExtension,
    IN ULONG chan,
    IN ULONG dev,
    IN PWSTR Name,
    IN ULONG Default
    );

extern ULONG
AtapiRegCheckParameterValue(
    IN PVOID HwDeviceExtension,
    IN PWSTR PathSuffix,
    IN PWSTR Name,
    IN ULONG Default
    );

extern ULONG  g_LogToDisplay;

extern "C"
VOID
_cdecl
_PrintNtConsole(
    PCHAR DebugMessage,
    ...
    );

VOID
UniataInitMapBM(
    IN struct _HW_DEVICE_EXTENSION* deviceExtension,
    IN struct _IDE_BUSMASTER_REGISTERS* BaseIoAddressBM_0,
    IN BOOLEAN MemIo
    );

VOID
UniataInitMapBase(
    IN struct _HW_CHANNEL* chan,
    IN PIDE_REGISTERS_1 BaseIoAddress1,
    IN PIDE_REGISTERS_2 BaseIoAddress2
    );

VOID
UniataInitSyncBaseIO(
    IN struct _HW_CHANNEL* chan
    );

UCHAR
DDKFASTAPI
UniataIsIdle(
    IN struct _HW_DEVICE_EXTENSION* deviceExtension,
    IN UCHAR Status
    );

VOID
UniataDumpATARegs(
    IN struct _HW_CHANNEL* chan
    );

ULONG
EncodeVendorStr(
   OUT PWCHAR Buffer,
    IN PUCHAR Str,
    IN ULONG  Length
    );

ULONGLONG
UniAtaCalculateLBARegsBack(
    struct _HW_LU_EXTENSION* LunExt,
    ULONGLONG            lba
    );

BOOLEAN
UniataAnybodyHome(
    IN PVOID   HwDeviceExtension,
    IN ULONG   Channel,
    IN ULONG   deviceNumber
    );

#define ATA_CMD_FLAG_LBAIOsupp 0x01
#define ATA_CMD_FLAG_48supp    0x02
#define ATA_CMD_FLAG_48        0x04
#define ATA_CMD_FLAG_DMA       0x08

extern UCHAR AtaCommands48[256];
extern UCHAR AtaCommandFlags[256];

#ifdef _DEBUG
#define PrintNtConsole  _PrintNtConsole
#else //_DEBUG
#define PrintNtConsole(x)  {;}
#endif //_DEBUG

#endif //USER_MODE

#pragma pack(pop)

#endif // __GLOBAL_H__
