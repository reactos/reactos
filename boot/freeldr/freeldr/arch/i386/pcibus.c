/* $Id: xboxvideo.c 25800 2007-02-14 20:30:33Z ion $
 *
 *  FreeLoader
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Programmer: Aleksey Bragin <aleksey reactos org>
 *    based on ReactOS HAL source code
 */

#include <freeldr.h>

//
// PCI Type 1 Ports
//
#define PCI_TYPE1_ADDRESS_PORT      (PULONG)0xCF8
#define PCI_TYPE1_DATA_PORT         0xCFC

//
// PCI Type 2 Ports
//
#define PCI_TYPE2_CSE_PORT          (PUCHAR)0xCF8
#define PCI_TYPE2_FORWARD_PORT      (PUCHAR)0xCFA
#define PCI_TYPE2_ADDRESS_BASE      0xC

//
// PCI Type 1 Configuration Register
//
typedef struct _PCI_TYPE1_CFG_BITS
{
    union
    {
        struct
        {
            ULONG Reserved1:2;
            ULONG RegisterNumber:6;
            ULONG FunctionNumber:3;
            ULONG DeviceNumber:5;
            ULONG BusNumber:8;
            ULONG Reserved2:7;
            ULONG Enable:1;
        } bits;

        ULONG AsULONG;
    } u;
} PCI_TYPE1_CFG_BITS, *PPCI_TYPE1_CFG_BITS;

//
// PCI Type 2 Address Register
//
typedef struct _PCI_TYPE2_ADDRESS_BITS
{
    union
    {
        struct
        {
            USHORT RegisterNumber:8;
            USHORT Agent:4;
            USHORT AddressBase:4;
        } bits;

        USHORT AsUSHORT;
    } u;
} PCI_TYPE2_ADDRESS_BITS, *PPCI_TYPE2_ADDRESS_BITS;

//
// Helper Macros
//
#define PASTE2(x,y)                                                     x ## y
#define POINTER_TO_(x)                                                  PASTE2(P,x)
#define READ_FROM(x)                                                    PASTE2(READ_PORT_, x)
#define WRITE_TO(x)                                                     PASTE2(WRITE_PORT_, x)

//
// Declares a PCI Register Read/Write Routine
//
#define TYPE_DEFINE(x, y)                                               \
    ULONG                                                               \
    NTAPI                                                               \
    x(                                                                  \
        IN y PciCfg,                                                    \
        IN PUCHAR Buffer,                                               \
        IN ULONG Offset                                                 \
    )
#define TYPE1_DEFINE(x) TYPE_DEFINE(x, PPCI_TYPE1_CFG_BITS);
#define TYPE2_DEFINE(x) TYPE_DEFINE(x, PPCI_TYPE2_ADDRESS_BITS);

//
// Defines a PCI Register Read/Write Type 1 Routine Prologue and Epilogue
//
#define TYPE1_START(x, y)                                               \
    TYPE_DEFINE(x, PPCI_TYPE1_CFG_BITS)                                 \
{                                                                       \
    ULONG i = Offset % sizeof(ULONG);                                   \
    PciCfg->u.bits.RegisterNumber = Offset / sizeof(ULONG);             \
    WRITE_PORT_ULONG(PCI_TYPE1_ADDRESS_PORT, PciCfg->u.AsULONG);
#define TYPE1_END(y)                                                    \
    return sizeof(y); }
#define TYPE2_END       TYPE1_END

//
// PCI Register Read Type 1 Routine
//
#define TYPE1_READ(x, y)                                                \
    TYPE1_START(x, y)                                                   \
    *((POINTER_TO_(y))Buffer) =                                         \
    READ_FROM(y)((POINTER_TO_(y))(PCI_TYPE1_DATA_PORT + i));     \
    TYPE1_END(y)

//
// PCI Register Write Type 1 Routine
//
#define TYPE1_WRITE(x, y)                                               \
    TYPE1_START(x, y)                                                   \
    WRITE_TO(y)((POINTER_TO_(y))(PCI_TYPE1_DATA_PORT + i),       \
                *((POINTER_TO_(y))Buffer));                             \
    TYPE1_END(y)

//
// Defines a PCI Register Read/Write Type 2 Routine Prologue and Epilogue
//
#define TYPE2_START(x, y)                                               \
    TYPE_DEFINE(x, PPCI_TYPE2_ADDRESS_BITS)                             \
{                                                                       \
    PciCfg->u.bits.RegisterNumber = (USHORT)Offset;

//
// PCI Register Read Type 2 Routine
//
#define TYPE2_READ(x, y)                                                \
    TYPE2_START(x, y)                                                   \
    *((POINTER_TO_(y))Buffer) =                                         \
        READ_FROM(y)((POINTER_TO_(y))(ULONG)PciCfg->u.AsUSHORT);        \
    TYPE2_END(y)

//
// PCI Register Write Type 2 Routine
//
#define TYPE2_WRITE(x, y)                                               \
    TYPE2_START(x, y)                                                   \
    WRITE_TO(y)((POINTER_TO_(y))(ULONG)PciCfg->u.AsUSHORT,              \
                *((POINTER_TO_(y))Buffer));                             \
    TYPE2_END(y)

typedef ULONG
(NTAPI *FncConfigIO)(
    IN PVOID State,
    IN PUCHAR Buffer,
    IN ULONG Offset
);

typedef struct _PCI_CONFIG_HANDLER
{
    FncConfigIO ConfigRead[3];
    FncConfigIO ConfigWrite[3];
} PCI_CONFIG_HANDLER, *PPCI_CONFIG_HANDLER;

// header ends

/* PCI Operation Matrix */
UCHAR PCIDeref[4][4] =
{
    {0, 1, 2, 2},   // ULONG-aligned offset
    {1, 1, 1, 1},   // UCHAR-aligned offset
    {2, 1, 2, 2},   // USHORT-aligned offset
    {1, 1, 1, 1}    // UCHAR-aligned offset
};

TYPE1_READ(HalpPCIReadUcharType1, UCHAR)
TYPE1_READ(HalpPCIReadUshortType1, USHORT)
TYPE1_READ(HalpPCIReadUlongType1, ULONG)
TYPE1_WRITE(HalpPCIWriteUcharType1, UCHAR)
TYPE1_WRITE(HalpPCIWriteUshortType1, USHORT)
TYPE1_WRITE(HalpPCIWriteUlongType1, ULONG)
TYPE2_READ(HalpPCIReadUcharType2, UCHAR)
TYPE2_READ(HalpPCIReadUshortType2, USHORT)
TYPE2_READ(HalpPCIReadUlongType2, ULONG)
TYPE2_WRITE(HalpPCIWriteUcharType2, UCHAR)
TYPE2_WRITE(HalpPCIWriteUshortType2, USHORT)
TYPE2_WRITE(HalpPCIWriteUlongType2, ULONG)

VOID
NTAPI
HalpPCISynchronizeType1(IN USHORT BusNumber,
                        IN PCI_SLOT_NUMBER Slot,
                        IN PPCI_TYPE1_CFG_BITS PciCfg1)
{
    /* Setup the PCI Configuration Register */
    PciCfg1->u.AsULONG = 0;
    PciCfg1->u.bits.BusNumber = BusNumber;
    PciCfg1->u.bits.DeviceNumber = Slot.u.bits.DeviceNumber;
    PciCfg1->u.bits.FunctionNumber = Slot.u.bits.FunctionNumber;
    PciCfg1->u.bits.Enable = TRUE;
}

VOID
NTAPI
HalpPCIReleaseSynchronzationType1()
{
    PCI_TYPE1_CFG_BITS PciCfg1;

    /* Clear the PCI Configuration Register */
    PciCfg1.u.AsULONG = 0;
    WRITE_PORT_ULONG(PCI_TYPE1_ADDRESS_PORT, PciCfg1.u.AsULONG);
}


/* Type 1 PCI Bus */
PCI_CONFIG_HANDLER PCIConfigHandlerType1 =
{
    /* Read */
    {
        (FncConfigIO)HalpPCIReadUlongType1,
        (FncConfigIO)HalpPCIReadUcharType1,
        (FncConfigIO)HalpPCIReadUshortType1
    },

    /* Write */
    {
        (FncConfigIO)HalpPCIWriteUlongType1,
        (FncConfigIO)HalpPCIWriteUcharType1,
        (FncConfigIO)HalpPCIWriteUshortType1
    }
};


VOID
NTAPI
HalpPCIConfig(IN USHORT BusNumber,
              IN PCI_SLOT_NUMBER Slot,
              IN PUCHAR Buffer,
              IN ULONG Offset,
              IN ULONG Length,
              IN FncConfigIO *ConfigIO)
{
    ULONG i;
    UCHAR State[20];

    HalpPCISynchronizeType1(BusNumber, Slot, State);

    /* Loop every increment */
    while (Length)
    {
        /* Find out the type of read/write we need to do */
        i = PCIDeref[Offset % sizeof(ULONG)][Length % sizeof(ULONG)];

        /* Do the read/write and return the number of bytes */
        i = ConfigIO[i](State,
                        Buffer,
                        Offset);

        /* Increment the buffer position and offset, and decrease the length */
        Offset += i;
        Buffer += i;
        Length -= i;
    }

    HalpPCIReleaseSynchronzationType1();
}

VOID
NTAPI
HalReadPCIConfig(IN USHORT BusNumber,
                 IN PCI_SLOT_NUMBER Slot,
                 IN PVOID Buffer,
                 IN ULONG Offset,
                 IN ULONG Length)
{
        /* Send the request */
        HalpPCIConfig(BusNumber,
                      Slot,
                      Buffer,
                      Offset,
                      Length,
                      PCIConfigHandlerType1.ConfigRead);
}

VOID
NTAPI
HalWritePCIConfig(IN USHORT BusNumber,
                  IN PCI_SLOT_NUMBER Slot,
                  IN PVOID Buffer,
                  IN ULONG Offset,
                  IN ULONG Length)
{
        /* Send the request */
        HalpPCIConfig(BusNumber,
                      Slot,
                      Buffer,
                      Offset,
                      Length,
                      PCIConfigHandlerType1.ConfigWrite);
}

#define PCI_NUM_RESOURCES	11

#define PCI_COMMAND		0x04	/* 16 bits */
#define  PCI_COMMAND_IO		0x1	/* Enable response in I/O space */
#define  PCI_COMMAND_MEMORY	0x2	/* Enable response in Memory space */

ULONG
PciEnableResources(IN USHORT BusNumber,
                   IN PCI_SLOT_NUMBER Slot,
                   ULONG Mask)
{
    USHORT cmd, old_cmd;
    ULONG idx;
    //struct resource *r;

    HalReadPCIConfig(BusNumber, Slot, &cmd, PCI_COMMAND, sizeof(USHORT));
    old_cmd = cmd;
    for(idx = 0; idx < PCI_NUM_RESOURCES; idx++)
    {
        /* Only set up the requested stuff */
        if (!(Mask & (1<<idx)))
            continue;

        //r = &dev->resource[idx];
        /*if (!r->start && r->end)
        {
            printk(KERN_ERR "PCI: Device %s not available because of resource collisions\n", pci_name(dev));
            return -EINVAL;
        }*/

        //if (r->flags & IORESOURCE_IO)
        //    cmd |= PCI_COMMAND_IO;
        //if (r->flags & IORESOURCE_MEM)
            cmd |= PCI_COMMAND_MEMORY;
    }
    //if (dev->resource[PCI_ROM_RESOURCE].start)
      //  cmd |= PCI_COMMAND_MEMORY;

    if (cmd != old_cmd)
    {
        ofwprintf("PCI: Enabling device (%x -> %x)\n", old_cmd, cmd);
        HalWritePCIConfig(BusNumber, Slot, &cmd, PCI_COMMAND, sizeof(USHORT));
    }
    return 0;
}
