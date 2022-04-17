/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/versatile/hwkmi.c
 * PURPOSE:         LLB KMI Support for Versatile
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

//
// Control Register Bits
//
#define KMICR_TYPE		        (1 << 5)
#define KMICR_RXINTREN	        (1 << 4)
#define KMICR_TXINTREN	        (1 << 3)
#define KMICR_EN		        (1 << 2)
#define KMICR_FD		        (1 << 1)
#define KMICR_FC		        (1 << 0)

//
// Status Register Bits
//
#define KMISTAT_TXEMPTY		    (1 << 6)
#define KMISTAT_TXBUSY		    (1 << 5)
#define KMISTAT_RXFULL		    (1 << 4)
#define KMISTAT_RXBUSY		    (1 << 3)
#define KMISTAT_RXPARITY	    (1 << 2)
#define KMISTAT_IC		        (1 << 1)
#define KMISTAT_ID              (1 << 0)

//
// KMI Registers
//
#define PL050_KMICR		        (LlbHwVersaKmiBase + 0x00)
#define PL050_KMISTAT		    (LlbHwVersaKmiBase + 0x04)
#define PL050_KMIDATA		    (LlbHwVersaKmiBase + 0x08)
#define PL050_KMICLKDIV	        (LlbHwVersaKmiBase + 0x0c)
static const ULONG LlbHwVersaKmiBase = 0x10006000;

//
// PS/2 Commands/Requests
//
#define PS2_O_RESET		        0xff
#define PS2_O_RESEND		    0xfe
#define PS2_O_DISABLE		    0xf5
#define PS2_O_ENABLE		    0xf4
#define PS2_O_ECHO		        0xee
#define PS2_O_SET_DEFAULT	    0xf6
#define PS2_O_SET_RATE_DELAY    0xf3
#define PS2_O_SET_SCANSET	    0xf0
#define PS2_O_INDICATORS	    0xed
#define PS2_I_RESEND		    0xfe
#define PS2_I_DIAGFAIL		    0xfd
#define PS2_I_ACK		        0xfa
#define PS2_I_BREAK		        0xf0
#define PS2_I_ECHO		        0xee
#define PS2_I_BAT_OK		    0xaa

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
LlbHwVersaKmiSendAndWait(IN ULONG Value)
{
    volatile int i = 1000;

    /* Send the value */
    LlbHwKbdSend(Value);

    /* Wait a bit */
    while (--i);

    /* Now make sure we received an ACK */
    if (LlbHwKbdRead() != PS2_I_ACK) DbgPrint("PS/2 FAILURE!\n");
}

VOID
NTAPI
LlbHwVersaKmiInitialize(VOID)
{
    UCHAR Divisor;

    /* Setup divisor and enable KMI */
    Divisor = (LlbHwGetPClk() / 8000000) - 1;
    WRITE_REGISTER_UCHAR(PL050_KMICLKDIV, Divisor);
    WRITE_REGISTER_UCHAR(PL050_KMICR, KMICR_EN);

    /* Reset PS/2 controller */
    LlbHwVersaKmiSendAndWait(PS2_O_RESET);
    if (LlbHwKbdRead() != PS2_I_BAT_OK) DbgPrint("PS/2 RESET FAILURE!\n");

    /* Send PS/2 Initialization Stream */
    LlbHwVersaKmiSendAndWait(PS2_O_DISABLE);
    LlbHwVersaKmiSendAndWait(PS2_O_SET_DEFAULT);
    LlbHwVersaKmiSendAndWait(PS2_O_SET_SCANSET);
    LlbHwVersaKmiSendAndWait(1);
    LlbHwVersaKmiSendAndWait(PS2_O_ENABLE);
}

VOID
NTAPI
LlbHwKbdSend(IN ULONG Value)
{
    ULONG Status;

    /* Wait for ready signal */
    do
    {
        /* Read TX buffer state */
        Status = READ_REGISTER_UCHAR(PL050_KMISTAT);
    } while (!(Status & KMISTAT_TXEMPTY));

    /* Send value */
    WRITE_REGISTER_UCHAR(PL050_KMIDATA, Value);
}

BOOLEAN
NTAPI
LlbHwKbdReady(VOID)
{
    return READ_REGISTER_UCHAR(PL050_KMISTAT) & KMISTAT_RXFULL;
}

INT
NTAPI
LlbHwKbdRead(VOID)
{
    /* Read current data on keyboard */
    return READ_REGISTER_UCHAR(PL050_KMIDATA);
}

/* EOF */
