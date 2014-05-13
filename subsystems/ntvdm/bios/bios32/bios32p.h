/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios32.h
 * PURPOSE:         VDM 32-bit BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _BIOS32P_H_
#define _BIOS32P_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"
#include "../bios.h"

/**/ #include "callback.h" /**/

/* DEFINES ********************************************************************/

#define BIOS_PIC_MASTER_INT 0x08
#define BIOS_PIC_SLAVE_INT  0x70

#define BIOS_EQUIPMENT_INTERRUPT    0x11
#define BIOS_MEMORY_SIZE            0x12
#define BIOS_MISC_INTERRUPT         0x15
#define BIOS_TIME_INTERRUPT         0x1A
#define BIOS_SYS_TIMER_INTERRUPT    0x1C

/* FUNCTIONS ******************************************************************/

extern CALLBACK16 BiosContext;
#define RegisterBiosInt32(IntNumber, IntHandler) \
do { \
    BiosContext.NextOffset += RegisterInt32(MAKELONG(BiosContext.NextOffset,  \
                                                     BiosContext.Segment),    \
                                            (IntNumber), (IntHandler), NULL); \
} while(0);

VOID EnableHwIRQ(UCHAR hwirq, EMULATOR_INT32_PROC func);
VOID PicIRQComplete(LPWORD Stack);

#endif // _BIOS32P_H_

/* EOF */
