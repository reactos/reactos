/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/hardware/ps2.h
 * PURPOSE:         PS/2 controller emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _PS2_H_
#define _PS2_H_

/* DEFINES ********************************************************************/

/* I/O Ports */
#define PS2_DATA_PORT       0x60
#define PS2_CONTROL_PORT    0x64

/* Controller Status Register flags */
#define PS2_STAT_OUT_BUF_FULL       (1 << 0)
// #define PS2_STAT_IN_BUF_FULL        (1 << 1)
#define PS2_STAT_SYSTEM             (1 << 2)
#define PS2_STAT_COMMAND            (1 << 3)
#define PS2_STAT_KBD_ENABLE         (1 << 4)    // 0: Locked; 1: Not locked
#define PS2_STAT_AUX_OUT_BUF_FULL   (1 << 5)
#define PS2_STAT_GEN_TIMEOUT        (1 << 6)
#define PS2_STAT_PARITY_ERROR       (1 << 7)

/* Controller Configuration Byte flags */
#define PS2_CONFIG_KBD_INT          (1 << 0)
#define PS2_CONFIG_AUX_INT          (1 << 1)
#define PS2_CONFIG_SYSTEM           (1 << 2)
#define PS2_CONFIG_NO_KEYLOCK       (1 << 3)
#define PS2_CONFIG_KBD_DISABLE      (1 << 4)
#define PS2_CONFIG_AUX_DISABLE      (1 << 5)
// #define PS2_CONFIG_KBD_XLAT         (1 << 6)

/* Output Port flags */
#define PS2_OUT_CPU_NO_RESET        (1 << 0)
#define PS2_OUT_A20_SET             (1 << 1)
#define PS2_OUT_AUX_DATA            (1 << 2)
// #define PS2_OUT_AUX_CLOCK           (1 << 3)
#define PS2_OUT_IRQ01               (1 << 4)
#define PS2_OUT_IRQ12               (1 << 5)
// #define PS2_OUT_KBD_CLOCK           (1 << 6)
#define PS2_OUT_KBD_DATA            (1 << 7)

typedef VOID (WINAPI *PS2_DEVICE_CMDPROC)(LPVOID Param, BYTE Command);

/* FUNCTIONS ******************************************************************/

VOID PS2SetDeviceCmdProc(BYTE PS2Port, LPVOID Param, PS2_DEVICE_CMDPROC DeviceCommand);

BOOLEAN PS2QueuePush(BYTE PS2Port, BYTE Data);
BOOLEAN PS2PortQueueRead(BYTE PS2Port);

BOOLEAN PS2Initialize(VOID);
VOID PS2Cleanup(VOID);

#endif // _PS2_H_

/* EOF */
