/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            pic.h
 * PURPOSE:         Programmable Interrupt Controller emulation
 *                  (Interrupt Controller Adapter (ICA) in Windows terminology)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _PIC_H_
#define _PIC_H_

/* DEFINES ********************************************************************/

#define PIC_MASTER_CMD  0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_CMD   0xA0
#define PIC_SLAVE_DATA  0xA1

#define PIC_ICW1        0x10
#define PIC_ICW1_ICW4   (1 << 0)
#define PIC_ICW1_SINGLE (1 << 1)
#define PIC_ICW4_8086   (1 << 0)
#define PIC_ICW4_AEOI   (1 << 1)

#define PIC_OCW2_NUM_MASK   0x07
#define PIC_OCW2_EOI        (1 << 5)
#define PIC_OCW2_SL         (1 << 6)

#define PIC_OCW3            (1 << 3)
#define PIC_OCW3_READ_ISR   0x0B

/* FUNCTIONS ******************************************************************/

VOID PicInterruptRequest(BYTE Number);
BYTE PicGetInterrupt(VOID);

VOID PicInitialize(VOID);

#endif // _PIC_H_

/* EOF */
