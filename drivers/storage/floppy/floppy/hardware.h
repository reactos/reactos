/*
 *  ReactOS Floppy Driver
 *  Copyright (C) 2004, Vizzini (vizzini@plasmic.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * PROJECT:         ReactOS Floppy Driver
 * FILE:            hardware.h
 * PURPOSE:         Header for FDC control routines
 * PROGRAMMER:      Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *                  15-Feb-2004 vizzini - Created
 *
 * NOTES:
 *     - Baesd on https://web.archive.org/web/20120130065947/http://www.nondot.org/sabre/os/files/Disk/FLOPPY.TXT
 *     - Some information taken from Intel 82077AA data sheet (order #290166-007)
 *     - Some definitions are PS/2-Specific; others include the original NEC PD765
 *     - Other information gathered from the comments in the NT 3.5 floppy driver
 *
 * TODO:
 *     - Convert these numbers to 100% absolute values; eliminate bit positions
 *       in favor of shifts or bitfields
 */

#pragma once

#define FLOPPY_DEFAULT_IRQ              0x6
#define FDC_PORT_BYTES                  0x8

/* Register offsets from base address (usually 0x3f8)  */
#define STATUS_REGISTER_A               0x0 /* Read; PS/2 Only */
#define STATUS_REGISTER_B               0x1 /* Read; PS/2 Only */
#define DIGITAL_OUTPUT_REGISTER         0x2 /* Read/Write */
#define TAPE_DRIVE_REGISTER             0x3 /* Read/Write */
#define MAIN_STATUS_REGISTER            0x4 /* Read */
#define DATA_RATE_SELECT_REGISTER       0x4 /* Write */
#define FIFO                            0x5 /* Read/Write */
#define RESERVED_REGISTER               0x6 /* Reserved */
#define DIGITAL_INPUT_REGISTER          0x7 /* Read; PS/2 Only */
#define CONFIGURATION_CONTROL_REGISTER  0x7 /* Write; PS/2 Only */

/* STATUS_REGISTER_A */
#define DSRA_DIRECTION                  0x1
#define DSRA_WRITE_PROTECT              0x2
#define DSRA_INDEX                      0x4
#define DSRA_HEAD_1_SELECT              0x8
#define DSRA_TRACK_0                    0x10
#define DSRA_STEP                       0x20
#define DSRA_SECOND_DRIVE_INSTALLED     0x40
#define DSRA_INTERRUPT_PENDING          0x80

/* STATUS_REGISTER_B */
#define DSRB_MOTOR_ENABLE_0             0x1
#define DSRB_MOTOR_ENABLE_1             0x2
#define DSRB_WRITE_ENABLE               0x4
#define DSRB_READ_DATA                  0x8
#define DSRB_WRITE_DATA                 0x10
#define DSRB_DRIVE_SELECT               0x20

/* DIGITAL_OUTPUT_REGISTER */
#define DOR_FLOPPY_DRIVE_SELECT         0x3 /* Covers 2 bits, defined below */
#define DOR_FDC_ENABLE                  0x4 /* from the website */
#define DOR_RESET                       0x4 /* from the Intel guide; 0 = resetting, 1 = enabled */
#define DOR_DMA_IO_INTERFACE_ENABLE     0x8 /* Reserved on PS/2 */
#define DOR_FLOPPY_MOTOR_ON_A           0x10
#define DOR_FLOPPY_MOTOR_ON_B           0x20
#define DOR_FLOPPY_MOTOR_ON_C           0x40 /* Reserved on PS/2 */
#define DOR_FLOPPY_MOTOR_ON_D           0x80 /* Reserved on PS/2 */

/* DOR_FLOPPY_DRIVE_SELECT */
#define DOR_FLOPPY_DRIVE_SELECT_A       0x0
#define DOR_FLOPPY_DRIVE_SELECT_B       0x1
#define DOR_FLOPPY_DRIVE_SELECT_C       0x2 /* Reserved on PS/2 */
#define DOR_FLOPPY_DRIVE_SELECT_D       0x3 /* Reserved on PS/2 */

/* MAIN_STATUS_REGISTER */
#define MSR_FLOPPY_BUSY_0               0x1
#define MSR_FLOPPY_BUSY_1               0x2
#define MSR_FLOPPY_BUSY_2               0x4 /* Reserved on PS/2 */
#define MSR_FLOPPY_BUSY_3               0x8 /* Reserved on PS/2 */
#define MSR_READ_WRITE_IN_PROGRESS      0x10
#define MSR_NON_DMA_MODE                0x20
#define MSR_IO_DIRECTION                0x40 /* Determines meaning of Command Status Registers */
#define MSR_DATA_REG_READY_FOR_IO       0x80

/* DATA_RATE_SELECT_REGISTER */
#define DRSR_DSEL                       0x3 /* covers two bits as defined below */
#define DRSR_PRECOMP                    0x1c /* covers three bits as defined below */
#define DRSR_MBZ                        0x20
#define DRSR_POWER_DOWN                 0x40
#define DRSR_SW_RESET                   0x80

/* DRSR_DSEL */
#define DRSR_DSEL_500KBPS               0x0
#define DRSR_DSEL_300KBPS               0x1
#define DRSR_DSEL_250KBPS               0x2
#define DRSR_DSEL_1MBPS                 0x3

/* STATUS_REGISTER_0 */
#define SR0_UNIT_SELECTED_AT_INTERRUPT  0x3 /* Covers two bits as defined below */
#define SR0_HEAD_NUMBER_AT_INTERRUPT    0x4 /* Values defined below */
#define SR0_NOT_READY_ON_READ_WRITE     0x8 /* Unused in PS/2 */
#define SR0_SS_ACCESS_TO_HEAD_1         0x8 /* Unused in PS/2 */
#define SR0_EQUIPMENT_CHECK             0x10
#define SR0_SEEK_COMPLETE               0x20
#define SR0_LAST_COMMAND_STATUS         0xC0 /* Covers two bits as defined below */

/* SR0_UNIT_SELECTED_AT_INTERRUPT */
#define SR0_UNIT_SELECTED_A             0x0
#define SR0_UNIT_SELECTED_B             0x1
#define SR0_UNIT_SELECTED_C             0x2
#define SR0_UNIT_SELECTED_D             0x3
#define SR0_PS2_UNIT_SELECTED_A         0x1 /* PS/2 uses only two drives: A = 01b  B = 10b */
#define SR0_PST_UNIT_SELECTED_B         0x2

/* SR0_HEAD_NUMBER_AT_INTERRUPT */
#define SR0_HEAD_0                      0x0
#define SR0_HEAD_1                      0x1

/* SR0_LAST_COMMAND_STATUS */
#define SR0_LCS_SUCCESS                 0x0
#define SR0_LCS_TERMINATED_ABNORMALLY   0x40
#define SR0_LCS_INVALID_COMMAND_ISSUED  0x80
#define SR0_LCS_READY_SIGNAL_CHANGED    0xc0 /* Reserved on PS/2; a/k/a abnormal termination due to polling */

/* STATUS_REGISTER_1 */
#define SR1_CANNOT_FIND_ID_ADDRESS      0x1 /* Mimics SR2_WRONG_CYLINDER_DETECTED */
#define SR1_WRITE_PROTECT_DETECTED      0x2
#define SR1_CANNOT_FIND_SECTOR_ID       0x4
#define SR1_OVERRUN                     0x10
#define SR1_CRC_ERROR                   0x20
#define SR1_END_OF_CYLINDER             0x80

/* STATUS_REGISTER_2 */
#define SR2_MISSING_ADDRESS_MARK        0x1
#define SR2_BAD_CYLINDER                0x2
#define SR2_SCAN_COMMAND_FAILED         0x4
#define SR2_SCAN_COMMAND_EQUAL          0x8
#define SR2_WRONG_CYLINDER_DETECTED     0x10 /* Mimics SR1_CANNOT_FIND_ID_ADDRESS */
#define SR2_CRC_ERROR_IN_SECTOR_DATA    0x20
#define SR2_SECTOR_WITH_DELETED_DATA    0x40

/* STATUS_REGISTER_3 */
#define SR3_UNIT_SELECTED               0x3 /* Covers two bits; defined below */
#define SR3_SIDE_HEAD_SELECT_STATUS     0x4 /* Values defined below */
#define SR3_TWO_SIDED_STATUS_SIGNAL     0x8
#define SR3_TRACK_ZERO_STATUS_SIGNAL    0x10
#define SR3_READY_STATUS_SIGNAL         0x20
#define SR3_WRITE_PROTECT_STATUS_SIGNAL 0x40
#define SR3_FAULT_STATUS_SIGNAL         0x80

/* SR3_UNIT_SELECTED */
#define SR3_UNIT_SELECTED_A             0x0
#define SR3_UNIT_SELECTED_B             0x1
#define SR3_UNIT_SELECTED_C             0x2
#define SR3_UNIT_SELECTED_D             0x3

/* SR3_SIDE_HEAD_SELECT_STATUS */
#define SR3_SHSS_HEAD_0                 0x0
#define SR3_SHSS_HEAD_1                 0x1

/* DIGITAL_INPUT_REGISTER */
#define DIR_HIGH_DENSITY_SELECT         0x1
#define DIR_DISKETTE_CHANGE             0x80

/* CONFIGURATION_CONTROL_REGISTER */
#define CCR_DRC                         0x3 /* Covers two bits, defined below */
#define CCR_DRC_0                       0x1
#define CCR_DRC_1                       0x2

/* CCR_DRC */
#define CCR_DRC_500000                  0x0
#define CCR_DRC_250000                  0x2

/* Commands */
#define COMMAND_READ_TRACK              0x2
#define COMMAND_SPECIFY                 0x3
#define COMMAND_SENSE_DRIVE_STATUS      0x4
#define COMMAND_WRITE_DATA              0x5
#define COMMAND_READ_DATA               0x6
#define COMMAND_RECALIBRATE             0x7
#define COMMAND_SENSE_INTERRUPT_STATUS  0x8
#define COMMAND_WRITE_DELETED_DATA      0x9
#define COMMAND_READ_ID                 0xA
#define COMMAND_READ_DELETED_DATA       0xC
#define COMMAND_FORMAT_TRACK            0xD
#define COMMAND_SEEK                    0xF
#define COMMAND_VERSION                 0x10
#define COMMAND_SCAN_EQUAL              0x11
#define COMMAND_CONFIGURE               0x13
#define COMMAND_SCAN_LOW_OR_EQUAL       0x19
#define COMMAND_SCAN_HIGH_OR_EQUAL      0x1D

/* COMMAND_READ_DATA constants */
#define READ_DATA_DS0                   0x1
#define READ_DATA_DS1                   0x2
#define READ_DATA_HDS                   0x4
#define READ_DATA_SK                    0x20
#define READ_DATA_MFM                   0x40
#define READ_DATA_MT                    0x80

/* COMMAND_READ_ID constants */
#define READ_ID_MFM                     0x40

/* COMMAND_SPECIFY constants */
#define SPECIFY_HLT_1M                  0x10  /* 16ms; based on intel data sheet */
#define SPECIFY_HLT_500K                0x8   /* 16ms; based on intel data sheet */
#define SPECIFY_HLT_300K                0x6   /* 16ms; based on intel data sheet */
#define SPECIFY_HLT_250K                0x4   /* 16ms; based on intel data sheet */
#define SPECIFY_HUT_1M                  0x0   /* Need to figure out these eight values; 0 is max */
#define SPECIFY_HUT_500K                0x0
#define SPECIFY_HUT_300K                0x0
#define SPECIFY_HUT_250K                0x0
#define SPECIFY_SRT_1M                  0x0
#define SPECIFY_SRT_500K                0x0
#define SPECIFY_SRT_300K                0x0
#define SPECIFY_SRT_250K                0x0

/* Command byte 1 constants */
#define COMMAND_UNIT_SELECT             0x3 /* Covers two bits; defined below */
#define COMMAND_UNIT_SELECT_0           0x1
#define COMMAND_UNIT_SELECT_1           0x2
#define COMMAND_HEAD_NUMBER             0x4
#define COMMAND_HEAD_NUMBER_SHIFT       0x2

/* COMMAND_VERSION */
#define VERSION_ENHANCED                0x90

/* COMMAND_UNIT_SELECT */
#define CUS_UNIT_0                      0x0
#define CUS_UNIT_1                      0x1

/* COMMAND_CONFIGURE constants */
#define CONFIGURE_FIFOTHR               0xf
#define CONFIGURE_POLL                  0x10
#define CONFIGURE_EFIFO                 0x20
#define CONFIGURE_EIS                   0x40
#define CONFIGURE_PRETRK                0xff

/* Command Head Number Constants */
#define COMMAND_HEAD_0                  0x0
#define COMMAND_HEAD_1                  0x1

/* Bytes per sector constants */
#define HW_128_BYTES_PER_SECTOR         0x0
#define HW_256_BYTES_PER_SECTOR         0x1
#define HW_512_BYTES_PER_SECTOR         0x2
#define HW_1024_BYTES_PER_SECTOR        0x3

/*
 * FUNCTIONS
 */
NTSTATUS NTAPI
HwTurnOnMotor(PDRIVE_INFO DriveInfo);

NTSTATUS NTAPI
HwSenseDriveStatus(PDRIVE_INFO DriveInfo);

NTSTATUS NTAPI
HwReadWriteData(PCONTROLLER_INFO ControllerInfo,
                BOOLEAN Read,
                UCHAR Unit,
                UCHAR Cylinder,
                UCHAR Head,
                UCHAR Sector,
                UCHAR BytesPerSector,
                UCHAR EndOfTrack,
                UCHAR Gap3Length,
                UCHAR DataLength);

NTSTATUS NTAPI
HwRecalibrate(PDRIVE_INFO DriveInfo);

NTSTATUS NTAPI
HwSenseInterruptStatus(PCONTROLLER_INFO ControllerInfo);

NTSTATUS NTAPI
HwReadId(PDRIVE_INFO DriveInfo, UCHAR Head);

NTSTATUS NTAPI
HwFormatTrack(PCONTROLLER_INFO ControllerInfo,
              UCHAR Unit,
              UCHAR Head,
              UCHAR BytesPerSector,
              UCHAR SectorsPerTrack,
              UCHAR Gap3Length,
              UCHAR FillerPattern);

NTSTATUS NTAPI
HwSeek(PDRIVE_INFO DriveInfo, UCHAR Cylinder);

NTSTATUS NTAPI
HwReadWriteResult(PCONTROLLER_INFO ControllerInfo);

NTSTATUS NTAPI
HwGetVersion(PCONTROLLER_INFO ControllerInfo);

NTSTATUS NTAPI
HwConfigure(PCONTROLLER_INFO ControllerInfo,
            BOOLEAN EIS,
            BOOLEAN EFIFO,
            BOOLEAN POLL,
            UCHAR FIFOTHR,
            UCHAR PRETRK) ;

NTSTATUS NTAPI
HwRecalibrateResult(PCONTROLLER_INFO ControllerInfo);

NTSTATUS NTAPI
HwDiskChanged(PDRIVE_INFO DriveInfo,
              PBOOLEAN DiskChanged);

NTSTATUS NTAPI
HwSenseDriveStatusResult(PCONTROLLER_INFO ControllerInfo,
                         PUCHAR Status);

NTSTATUS NTAPI
HwSpecify(PCONTROLLER_INFO ControllerInfo,
          UCHAR HeadLoadTime,
          UCHAR HeadUnloadTime,
          UCHAR StepRateTime,
          BOOLEAN NonDma);

NTSTATUS NTAPI
HwReadIdResult(PCONTROLLER_INFO ControllerInfo,
               PUCHAR CurCylinder,
               PUCHAR CurHead);

NTSTATUS NTAPI
HwSetDataRate(PCONTROLLER_INFO ControllerInfo, UCHAR DataRate);

NTSTATUS NTAPI
HwReset(PCONTROLLER_INFO Controller);

NTSTATUS NTAPI
HwPowerOff(PCONTROLLER_INFO ControllerInfo);

VOID NTAPI
HwDumpRegisters(PCONTROLLER_INFO ControllerInfo);

NTSTATUS NTAPI
HwTurnOffMotor(PCONTROLLER_INFO ControllerInfo);
