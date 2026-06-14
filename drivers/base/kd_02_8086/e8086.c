/*
 * PROJECT:     ReactOS Intel KDNET Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Intel E1000 and "later" code
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "kd8086.h"

/*
 * This implementation is stupidly simplified, 
 * the better decision is to port the intel UNDI, which as a compatible license
 * and wrapping EDK2 types is easy for us. But for now, Small PRs!
 */
#undef READ_REGISTER_ULONG
#undef WRITE_REGISTER_ULONG
#undef KeStallExecutionProcessor

#if defined(_MSC_VER)
unsigned char __inbyte(unsigned short Port);
#pragma intrinsic(__inbyte)
#endif

/* ---- e1000 register offsets (BAR0) ---- */
#define E1000_CTRL    0x0000
#define E1000_STATUS  0x0008
#define E1000_ICR     0x00C0
#define E1000_IMC     0x00D8
#define E1000_RCTL    0x0100
#define E1000_TCTL    0x0400
#define E1000_TIPG    0x0410
#define E1000_RDBAL   0x2800
#define E1000_RDBAH   0x2804
#define E1000_RDLEN   0x2808
#define E1000_RDH     0x2810
#define E1000_RDT     0x2818
#define E1000_TDBAL   0x3800
#define E1000_TDBAH   0x3804
#define E1000_TDLEN   0x3808
#define E1000_TDH     0x3810
#define E1000_TDT     0x3818
#define E1000_MTA     0x5200
#define E1000_RAL0    0x5400
#define E1000_RAH0    0x5404
#define E1000_FCAL    0x0028
#define E1000_FCAH    0x002C
#define E1000_FCT     0x0030
#define E1000_FCTTV   0x0170
#define E1000_EERD    0x0014   /* EEPROM read */
#define E1000_MDIC    0x0020   /* PHY management (MDI control) */

/* MDIC (PHY access) fields. */
#define E1000_MDIC_DATA      0x0000FFFF
#define E1000_MDIC_REG_SHIFT 16
#define E1000_MDIC_PHY_SHIFT 21
#define E1000_MDIC_OP_WRITE  (1u << 26)
#define E1000_MDIC_OP_READ   (2u << 26)
#define E1000_MDIC_READY     (1u << 28)
#define E1000_MDIC_ERROR     (1u << 30)
#define E1000_PHY_ADDR       1

/* IEEE MII PHY registers / bits. */
#define MII_PHY_CONTROL          0x00
#define MII_PHY_STATUS           0x01
#define MII_CR_RESET             0x8000
#define MII_CR_AUTO_NEG_EN       0x1000
#define MII_CR_RESTART_AUTO_NEG  0x0200
#define MII_SR_LINK_STATUS       0x0004

/* EERD: shared START; address shift and DONE bit differ by family. */
#define E1000_EERD_START         0x0001

/* CTRL bits */
#define E1000_CTRL_FD       0x00000001   /* full duplex */
#define E1000_CTRL_ASDE     0x00000020   /* auto-speed detect */
#define E1000_CTRL_SLU      0x00000040   /* set link up */
#define E1000_CTRL_SPEED_1000 0x00000200 /* speed 1000 (bits 8-9 = 10b) */
#define E1000_CTRL_FRCSPD   0x00000800   /* force speed */
#define E1000_CTRL_FRCDPLX  0x00001000   /* force duplex */
#define E1000_CTRL_RST      0x04000000
/* STATUS bits */
#define E1000_STATUS_LU 0x00000002
/* RCTL bits */
#define E1000_RCTL_EN    0x00000002
#define E1000_RCTL_UPE   0x00000008   /* unicast promiscuous */
#define E1000_RCTL_MPE   0x00000010   /* multicast promiscuous */
#define E1000_RCTL_BAM   0x00008000
#define E1000_RCTL_BSIZE_2048 0x00000000
#define E1000_RCTL_SECRC 0x04000000
/* TCTL bits */
#define E1000_TCTL_EN  0x00000002
#define E1000_TCTL_PSP 0x00000008
#define E1000_TCTL_CT  (0x0F << 4)
#define E1000_TCTL_COLD (0x40 << 12)
/* TX descriptor CMD bits */
#define E1000_TXD_CMD_EOP  0x01
#define E1000_TXD_CMD_IFCS 0x02
#define E1000_TXD_CMD_RS   0x08
#define E1000_TXD_STAT_DD  0x01
/* RX descriptor STATUS bits */
#define E1000_RXD_STAT_DD  0x01
#define E1000_RXD_STAT_EOP 0x02

#define E1000_NUM_TX 16
#define E1000_NUM_RX 16
#define E1000_BUF_SIZE 2048

#include <pshpack1.h>
typedef struct _E1000_TX_DESC
{
    ULONGLONG BufferAddr;
    USHORT    Length;
    UCHAR     Cso;
    UCHAR     Cmd;
    UCHAR     Status;
    UCHAR     Css;
    USHORT    Special;
} E1000_TX_DESC, *PE1000_TX_DESC;

typedef struct _E1000_RX_DESC
{
    ULONGLONG BufferAddr;
    USHORT    Length;
    USHORT    Checksum;
    UCHAR     Status;
    UCHAR     Errors;
    USHORT    Special;
} E1000_RX_DESC, *PE1000_RX_DESC;

/*
 * igb (82575/82576/82580/I210/I350) advanced descriptors (16 bytes, same ring
 * stride as the legacy ones, so the ring memory is reused). The RX descriptor
 * has a read format (driver writes buffer addresses) and a write-back format
 * (hardware writes status/length); they overlap.
 */
typedef struct _IGB_ADV_RX_DESC
{
    ULONGLONG PktAddr;       /* read: packet buffer phys; wb: lo/hi dwords */
    union
    {
        ULONGLONG HdrAddr;   /* read: header buffer phys (0 = single buffer) */
        struct
        {
            ULONG  StatusError;  /* wb: status(0-19) | error; DD=bit0, EOP=bit1 */
            USHORT Length;       /* wb: packet length */
            USHORT Vlan;
        } Wb;
    } u;
} IGB_ADV_RX_DESC, *PIGB_ADV_RX_DESC;

typedef struct _IGB_ADV_TX_DESC
{
    ULONGLONG BufferAddr;
    ULONG     CmdTypeLen;    /* DTYP | DCMD | length */
    ULONG     OlinfoStatus;  /* PAYLEN<<14; hw writes DD into bit0 */
} IGB_ADV_TX_DESC, *PIGB_ADV_TX_DESC;
#include <poppack.h>

/* igb per-queue register block (queue 0). */
#define IGB_RDBAL0   0xC000
#define IGB_RDBAH0   0xC004
#define IGB_RDLEN0   0xC008
#define IGB_SRRCTL0  0xC00C
#define IGB_RDH0     0xC010
#define IGB_RDT0     0xC018
#define IGB_RXDCTL0  0xC028
#define IGB_TDBAL0   0xE000
#define IGB_TDBAH0   0xE004
#define IGB_TDLEN0   0xE008
#define IGB_TDH0     0xE010
#define IGB_TDT0     0xE018
#define IGB_TXDCTL0  0xE028
#define IGB_EIMC     0x1528   /* extended interrupt mask clear */

#define IGB_SRRCTL_BSIZE_2K      0x02         /* 2KB packet buffer (1KB units) */
#define IGB_SRRCTL_DESCTYPE_ADV  0x02000000   /* advanced, one buffer */
#define IGB_SRRCTL_DROP_EN       0x80000000
#define IGB_XDCTL_ENABLE         0x02000000   /* RXDCTL/TXDCTL queue enable */

/* Advanced TX cmd_type_len bits. */
#define IGB_TXD_DTYP_DATA   0x00300000
#define IGB_TXD_DCMD_EOP    0x01000000
#define IGB_TXD_DCMD_IFCS   0x02000000
#define IGB_TXD_DCMD_RS     0x08000000
#define IGB_TXD_DCMD_DEXT   0x20000000
#define IGB_TXD_STAT_DD     0x00000001   /* in OlinfoStatus after write-back */
#define IGB_RXD_STAT_DD     0x00000001
#define IGB_RXD_STAT_EOP    0x00000002

typedef struct _E1000_ADAPTER
{
    PUCHAR             RegBase;
    PKDNET_SHARED_DATA Shared;
    PE1000_TX_DESC     TxRing;
    PE1000_RX_DESC     RxRing;
    PUCHAR             TxBuffers;
    PUCHAR             RxBuffers;
    ULONGLONG          CtxPhys;    /* physical base of the hardware context */
    PUCHAR             CtxVa;      /* virtual base of the hardware context  */
    ULONG              RxNext;     /* next RX descriptor to inspect */
    ULONG              TxNext;     /* next TX descriptor to hand out */
    UCHAR              Mac[6];
    USHORT             DeviceId;   /* PCI device id (chip variant) */
    BOOLEAN            IsIgb;      /* igb family: advanced descriptors + per-queue regs */
} E1000_ADAPTER, *PE1000_ADAPTER;

/* ---- register helpers: direct MMIO access to the mapped BAR ---- */
static __forceinline ULONG E1kRead(PE1000_ADAPTER a, ULONG off)
{
    return *(volatile ULONG *)(a->RegBase + off);
}
static __forceinline VOID E1kWrite(PE1000_ADAPTER a, ULONG off, ULONG val)
{
    *(volatile ULONG *)(a->RegBase + off) = val;
}

static VOID E1kDelayMicroseconds(ULONG Microseconds)
{
    ULONG transitions = (Microseconds / 15) + 1;
    UCHAR prev = (UCHAR)(__inbyte(0x61) & 0x10);
    while (transitions)
    {
        UCHAR cur = (UCHAR)(__inbyte(0x61) & 0x10);
        if (cur != prev)
        {
            prev = cur;
            --transitions;
        }
    }
}

static __forceinline ULONG E1kAlignUp(ULONG v, ULONG a)
{
    return (v + (a - 1)) & ~(a - 1);
}

/* Read an internal PHY register over MDIC. Returns 0xFFFF if there's no MDIO
 * PHY or the access errors/times out (e.g. emulated/fiber NICs). */
static USHORT E1kPhyRead(PE1000_ADAPTER a, ULONG reg)
{
    ULONG mdic, i;
    E1kWrite(a, E1000_MDIC,
             (reg << E1000_MDIC_REG_SHIFT) | (E1000_PHY_ADDR << E1000_MDIC_PHY_SHIFT) | E1000_MDIC_OP_READ);
    for (i = 0; i < 2000; i++)
    {
        mdic = E1kRead(a, E1000_MDIC);
        if (mdic & E1000_MDIC_READY)
            return (mdic & E1000_MDIC_ERROR) ? 0xFFFF : (USHORT)(mdic & E1000_MDIC_DATA);
        E1kDelayMicroseconds(50);
    }
    return 0xFFFF;
}

static VOID E1kPhyWrite(PE1000_ADAPTER a, ULONG reg, USHORT val)
{
    ULONG i;
    E1kWrite(a, E1000_MDIC,
             (ULONG)val | (reg << E1000_MDIC_REG_SHIFT) |
             (E1000_PHY_ADDR << E1000_MDIC_PHY_SHIFT) | E1000_MDIC_OP_WRITE);
    for (i = 0; i < 2000; i++)
    {
        if (E1kRead(a, E1000_MDIC) & E1000_MDIC_READY)
            return;
        E1kDelayMicroseconds(50);
    }
}

/* Read an EEPROM word. The address-shift and DONE bit differ between the legacy
 * 8254x and the 82571+/82574 families. */
static USHORT E1kEepromRead(PE1000_ADAPTER a, ULONG word, BOOLEAN newFormat)
{
    ULONG addrShift = newFormat ? 2 : 8;
    ULONG doneBit   = newFormat ? 0x02 : 0x10;
    ULONG eerd, i;
    E1kWrite(a, E1000_EERD, (word << addrShift) | E1000_EERD_START);
    for (i = 0; i < 2000; i++)
    {
        eerd = E1kRead(a, E1000_EERD);
        if (eerd & doneBit)
            return (USHORT)(eerd >> 16);
        E1kDelayMicroseconds(50);
    }
    return 0xFFFF;
}

/* 82571 and later (incl. 82574L 0x10D3) use the newer EEPROM (EERD) layout. */
static __forceinline BOOLEAN E1kIsNewerFamily(USHORT DeviceId)
{
    return (DeviceId >= 0x105E);
}

/* igb family (82575/82576/82580/I350/I354/I210/I211): advanced descriptors,
 * per-queue register block. QEMU's "igb" is the 82576 (0x10C9). */
static BOOLEAN E1kIsIgb(USHORT id)
{
    return (id == 0x10A7 || id == 0x10A9 || id == 0x10D6 ||                      /* 82575 */
            id == 0x10C9 || id == 0x10E6 || id == 0x10E7 || id == 0x10E8 ||      /* 82576 */
            id == 0x150A || id == 0x150D || id == 0x1518 || id == 0x1526 ||      /* 82576 */
            (id >= 0x150E && id <= 0x1511) || id == 0x1516 || id == 0x1527 ||    /* 82580 */
            (id >= 0x1521 && id <= 0x1524) ||                                    /* I350   */
            (id >= 0x1533 && id <= 0x1538) || id == 0x1539 ||                    /* I210/I211 */
            id == 0x157B || id == 0x157C ||                                      /* I210   */
            id == 0x1F40 || id == 0x1F41 || id == 0x1F45);                       /* I354   */
}

static __forceinline ULONGLONG E1kPhys(PE1000_ADAPTER a, PVOID Va);

/* ---- descriptor accessors (branch legacy vs igb advanced) ---- */

static __forceinline BOOLEAN E1kTxDescDone(PE1000_ADAPTER a, ULONG idx)
{
    if (a->IsIgb)
        return (((PIGB_ADV_TX_DESC)a->TxRing)[idx].OlinfoStatus & IGB_TXD_STAT_DD) != 0;
    return (a->TxRing[idx].Status & E1000_TXD_STAT_DD) != 0;
}

static __forceinline VOID E1kTxDescSubmit(PE1000_ADAPTER a, ULONG idx, ULONG len)
{
    if (a->IsIgb)
    {
        PIGB_ADV_TX_DESC d = &((PIGB_ADV_TX_DESC)a->TxRing)[idx];
        d->CmdTypeLen = (len & 0xFFFF) | IGB_TXD_DTYP_DATA | IGB_TXD_DCMD_DEXT |
                        IGB_TXD_DCMD_EOP | IGB_TXD_DCMD_IFCS | IGB_TXD_DCMD_RS;
        d->OlinfoStatus = len << 14;   /* PAYLEN; hw clears DD then sets it on done */
    }
    else
    {
        PE1000_TX_DESC d = &a->TxRing[idx];
        d->Length = (USHORT)len;
        d->Cso = 0; d->Css = 0; d->Special = 0; d->Status = 0;
        d->Cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
    }
}

static __forceinline BOOLEAN E1kRxDescDone(PE1000_ADAPTER a, ULONG idx)
{
    if (a->IsIgb)
        return (((PIGB_ADV_RX_DESC)a->RxRing)[idx].u.Wb.StatusError & IGB_RXD_STAT_DD) != 0;
    return (a->RxRing[idx].Status & E1000_RXD_STAT_DD) != 0;
}

static __forceinline USHORT E1kRxDescLength(PE1000_ADAPTER a, ULONG idx)
{
    if (a->IsIgb)
        return ((PIGB_ADV_RX_DESC)a->RxRing)[idx].u.Wb.Length;
    return a->RxRing[idx].Length;
}

/* Re-arm an RX descriptor for hardware reuse (restore the read format). */
static __forceinline VOID E1kRxDescRecycle(PE1000_ADAPTER a, ULONG idx)
{
    if (a->IsIgb)
    {
        PIGB_ADV_RX_DESC d = &((PIGB_ADV_RX_DESC)a->RxRing)[idx];
        d->PktAddr = E1kPhys(a, a->RxBuffers + idx * E1000_BUF_SIZE);
        d->u.HdrAddr = 0;
    }
    else
    {
        a->RxRing[idx].Status = 0;
        a->RxRing[idx].Length = 0;
    }
}

ULONG
E1000GetHardwareContextSize(VOID)
{
    /* adapter + two 128-byte-aligned rings + TX/RX buffers, generously rounded. */
    ULONG size = (ULONG)sizeof(E1000_ADAPTER);
    size = E1kAlignUp(size, 128) + E1000_NUM_TX * (ULONG)sizeof(E1000_TX_DESC);
    size = E1kAlignUp(size, 128) + E1000_NUM_RX * (ULONG)sizeof(E1000_RX_DESC);
    size = E1kAlignUp(size, 16)  + E1000_NUM_TX * E1000_BUF_SIZE;
    size = E1kAlignUp(size, 16)  + E1000_NUM_RX * E1000_BUF_SIZE;
    return E1kAlignUp(size, 0x1000) + 0x1000;
}

static __forceinline ULONGLONG
E1kPhys(PE1000_ADAPTER a, PVOID Va)
{
    return a->CtxPhys + (ULONGLONG)((PUCHAR)Va - a->CtxVa);
}

#define E1kDbg(a, ...) \
    do { if ((a)->Shared && KdNetExtensibilityImports && KdNetExtensibilityImports->KdNetDbgPrintf) \
            KdNetExtensibilityImports->KdNetDbgPrintf(__VA_ARGS__); } while (0)

NTSTATUS
E1000InitializeController(PKDNET_SHARED_DATA KdNet)
{
    PE1000_ADAPTER a;
    PUCHAR base;
    ULONG off, i, status;

    if (KdNetExtensibilityImports && KdNetExtensibilityImports->KdNetDbgPrintf)
        KdNetExtensibilityImports->KdNetDbgPrintf(
            "e1000: enter KdNet=%p Hw=%p Dev=%p\n",
            KdNet, KdNet ? KdNet->Hardware : NULL, KdNet ? KdNet->Device : NULL);

    if (!KdNet || !KdNet->Hardware || !KdNet->Device)
        return STATUS_INVALID_PARAMETER;
    if (!KdNet->Device->BaseAddress[0].Valid)
        return STATUS_INVALID_PARAMETER;

    if (KdNetExtensibilityImports && KdNetExtensibilityImports->KdNetDbgPrintf)
        KdNetExtensibilityImports->KdNetDbgPrintf(
            "e1000: BAR0 valid=%u type=%u xlat=%p\n",
            (ULONG)KdNet->Device->BaseAddress[0].Valid,
            (ULONG)KdNet->Device->BaseAddress[0].Type,
            KdNet->Device->BaseAddress[0].TranslatedAddress);

    /* Carve the context: adapter, rings, buffers. */
    a = (PE1000_ADAPTER)KdNet->Hardware;
    base = (PUCHAR)KdNet->Hardware;

    /* Zero only the adapter header (kdnet already zeroed the whole context). */
    {
        PUCHAR z = (PUCHAR)a; ULONG n = sizeof(*a);
        while (n--) *z++ = 0;
    }

    a->RegBase = KdNet->Device->BaseAddress[0].TranslatedAddress;
    a->Shared = KdNet;
    a->CtxVa = (PUCHAR)KdNet->Hardware;
    a->CtxPhys = (ULONGLONG)KdNet->Device->Memory.Start.QuadPart;
    a->DeviceId = KdNet->Device->DeviceID;
    a->IsIgb = E1kIsIgb(a->DeviceId);

    E1kDbg(a, "e1000: chip 8086:%04x%s\n", (ULONG)a->DeviceId,
           a->IsIgb ? " (igb/advanced)" : "");

    E1kDbg(a, "e1000: RegBase=%p CtxVa=%p CtxPhysLo=0x%lx\n",
           a->RegBase, a->CtxVa, (ULONG)a->CtxPhys);

    /* Ensure PCI Bus Master + Memory Space decode are enabled. QEMU's e1000
     * refuses to DMA received packets unless Bus Master is set, and the bit can
     * be left clear here. Use direct config-space I/O (0xCF8/0xCFC). */
     ULONG slot = KdNet->Device->Slot;
     ULONG bus = KdNet->Device->Bus;
     ULONG devn = slot & 0x1F;
     ULONG func = (slot >> 5) & 0x7;
     ULONG cfg = 0x80000000UL | (bus << 16) | (devn << 11) | (func << 8) | 0x04;
     ULONG cmd;
     __outdword(0xCF8, cfg);
     cmd = __indword(0xCFC);
     __outdword(0xCF8, cfg);
     __outdword(0xCFC, cmd | 0x06);   /* Memory Space (1) + Bus Master (2) */
     E1kDbg(a, "e1000: PCI cmd 0x%lx -> 0x%lx (BM+MEM)\n", cmd, cmd | 0x06);


    off = E1kAlignUp((ULONG)sizeof(E1000_ADAPTER), 128);
    a->TxRing = (PE1000_TX_DESC)(base + off);
    off = E1kAlignUp(off + E1000_NUM_TX * (ULONG)sizeof(E1000_TX_DESC), 128);
    a->RxRing = (PE1000_RX_DESC)(base + off);
    off = E1kAlignUp(off + E1000_NUM_RX * (ULONG)sizeof(E1000_RX_DESC), 16);
    a->TxBuffers = base + off;
    off = E1kAlignUp(off + E1000_NUM_TX * E1000_BUF_SIZE, 16);
    a->RxBuffers = base + off;

    /* Mask interrupts, then reset the device. */
    E1kWrite(a, E1000_IMC, 0xFFFFFFFF);
    (void)E1kRead(a, E1000_ICR);
    E1kWrite(a, E1000_CTRL, E1kRead(a, E1000_CTRL) | E1000_CTRL_RST);
    KeStallExecutionProcessor(20000);   /* ~20ms for reset to settle */

    /* Re-mask interrupts (reset clears the mask). */
    E1kWrite(a, E1000_IMC, 0xFFFFFFFF);
    (void)E1kRead(a, E1000_ICR);

    /* Bring the link up; disable flow control. */
    /* Force 1000/full-duplex and set the link up. QEMU brings the link up from
     * just SLU|ASDE, but VirtualBox is happier with a forced speed/duplex
     * (no auto-negotiation), which makes it assert the link deterministically. */
    E1kWrite(a, E1000_CTRL,
             E1000_CTRL_SLU | E1000_CTRL_FD | E1000_CTRL_SPEED_1000 |
             E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPLX);
    E1kWrite(a, E1000_FCAL, 0);
    E1kWrite(a, E1000_FCAH, 0);
    E1kWrite(a, E1000_FCT, 0);
    E1kWrite(a, E1000_FCTTV, 0);

    /* Read the MAC from the receive address registers. If they weren't
     * auto-loaded from the EEPROM (RA0 empty — happens on some real hardware /
     * after certain resets), read the MAC straight out of the EEPROM. */
    {
        ULONG ral = E1kRead(a, E1000_RAL0);
        ULONG rah = E1kRead(a, E1000_RAH0);
        a->Mac[0] = (UCHAR)(ral);
        a->Mac[1] = (UCHAR)(ral >> 8);
        a->Mac[2] = (UCHAR)(ral >> 16);
        a->Mac[3] = (UCHAR)(ral >> 24);
        a->Mac[4] = (UCHAR)(rah);
        a->Mac[5] = (UCHAR)(rah >> 8);

        if (ral == 0 && (rah & 0xFFFF) == 0)
        {
            BOOLEAN newF = E1kIsNewerFamily(a->DeviceId);
            USHORT w0 = E1kEepromRead(a, 0, newF);
            USHORT w1 = E1kEepromRead(a, 1, newF);
            USHORT w2 = E1kEepromRead(a, 2, newF);
            if (!(w0 == 0xFFFF && w1 == 0xFFFF && w2 == 0xFFFF))
            {
                a->Mac[0] = (UCHAR)w0;       a->Mac[1] = (UCHAR)(w0 >> 8);
                a->Mac[2] = (UCHAR)w1;       a->Mac[3] = (UCHAR)(w1 >> 8);
                a->Mac[4] = (UCHAR)w2;       a->Mac[5] = (UCHAR)(w2 >> 8);
                /* Program it back into RA0 so the hardware filters on it. */
                E1kWrite(a, E1000_RAL0,
                         (ULONG)a->Mac[0] | ((ULONG)a->Mac[1] << 8) |
                         ((ULONG)a->Mac[2] << 16) | ((ULONG)a->Mac[3] << 24));
                E1kWrite(a, E1000_RAH0,
                         (ULONG)a->Mac[4] | ((ULONG)a->Mac[5] << 8) | 0x80000000u /* AV */);
                E1kDbg(a, "e1000: MAC read from EEPROM\n");
            }
        }

        if (KdNet->TargetMacAddress)
        {
            for (i = 0; i < 6; i++)
                KdNet->TargetMacAddress[i] = a->Mac[i];
        }
        E1kDbg(a, "e1000: MAC %02x-%02x-%02x-%02x-%02x-%02x RAL=0x%lx RAH=0x%lx\n",
               a->Mac[0], a->Mac[1], a->Mac[2], a->Mac[3], a->Mac[4], a->Mac[5], ral, rah);
    }

    /* Clear the multicast table. */
    for (i = 0; i < 128; i++)
        E1kWrite(a, E1000_MTA + i * 4, 0);

    a->RxNext = 0;
    a->TxNext = 0;

    if (a->IsIgb)
    {
        ULONGLONG rphys = E1kPhys(a, a->RxRing);
        ULONGLONG tphys = E1kPhys(a, a->TxRing);
        ULONG t;

        /* igb: advanced descriptors + per-queue register block. Mask the
         * extended interrupts too. */
        E1kWrite(a, IGB_EIMC, 0xFFFFFFFF);

        /* --- RX queue 0 --- */
        E1kWrite(a, E1000_RCTL, 0);   /* disable RX while configuring */

        for (i = 0; i < E1000_NUM_RX; i++)
        {
            PIGB_ADV_RX_DESC d = &((PIGB_ADV_RX_DESC)a->RxRing)[i];
            d->PktAddr = E1kPhys(a, a->RxBuffers + i * E1000_BUF_SIZE);
            d->u.HdrAddr = 0;
        }
        E1kWrite(a, IGB_RDBAL0, (ULONG)rphys);
        E1kWrite(a, IGB_RDBAH0, (ULONG)(rphys >> 32));
        E1kWrite(a, IGB_RDLEN0, E1000_NUM_RX * (ULONG)sizeof(IGB_ADV_RX_DESC));
        E1kWrite(a, IGB_SRRCTL0, IGB_SRRCTL_BSIZE_2K | IGB_SRRCTL_DESCTYPE_ADV);
        E1kWrite(a, IGB_RDH0, 0);
        E1kWrite(a, IGB_RDT0, 0);
        /* Enable the queue and wait for hardware to acknowledge. */
        E1kWrite(a, IGB_RXDCTL0, IGB_XDCTL_ENABLE);
        for (t = 0; t < 1000; t++)
        {
            if (E1kRead(a, IGB_RXDCTL0) & IGB_XDCTL_ENABLE) break;
            E1kDelayMicroseconds(100);
        }
        E1kWrite(a, E1000_RCTL,
                 E1000_RCTL_EN | E1000_RCTL_UPE | E1000_RCTL_MPE |
                 E1000_RCTL_BAM | E1000_RCTL_SECRC);
        E1kWrite(a, IGB_RDT0, E1000_NUM_RX - 1);

        /* --- TX queue 0 --- */
        for (i = 0; i < E1000_NUM_TX; i++)
        {
            PIGB_ADV_TX_DESC d = &((PIGB_ADV_TX_DESC)a->TxRing)[i];
            d->BufferAddr = E1kPhys(a, a->TxBuffers + i * E1000_BUF_SIZE);
            d->CmdTypeLen = 0;
            d->OlinfoStatus = IGB_TXD_STAT_DD;   /* mark free */
        }
        E1kWrite(a, IGB_TDBAL0, (ULONG)tphys);
        E1kWrite(a, IGB_TDBAH0, (ULONG)(tphys >> 32));
        E1kWrite(a, IGB_TDLEN0, E1000_NUM_TX * (ULONG)sizeof(IGB_ADV_TX_DESC));
        E1kWrite(a, IGB_TDH0, 0);
        E1kWrite(a, IGB_TDT0, 0);
        E1kWrite(a, IGB_TXDCTL0, IGB_XDCTL_ENABLE);
        for (t = 0; t < 1000; t++)
        {
            if (E1kRead(a, IGB_TXDCTL0) & IGB_XDCTL_ENABLE) break;
            E1kDelayMicroseconds(100);
        }
        E1kWrite(a, E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT | E1000_TCTL_COLD);
    }
    else
    {
        /* Legacy 8254x/8257x descriptors. */
        for (i = 0; i < E1000_NUM_RX; i++)
        {
            a->RxRing[i].BufferAddr = E1kPhys(a, a->RxBuffers + i * E1000_BUF_SIZE);
            a->RxRing[i].Status = 0;
            a->RxRing[i].Length = 0;
        }
        {
            ULONGLONG rphys = E1kPhys(a, a->RxRing);
            E1kWrite(a, E1000_RDBAL, (ULONG)rphys);
            E1kWrite(a, E1000_RDBAH, (ULONG)(rphys >> 32));
            E1kWrite(a, E1000_RDLEN, E1000_NUM_RX * (ULONG)sizeof(E1000_RX_DESC));
            E1kWrite(a, E1000_RDH, 0);
            E1kWrite(a, E1000_RDT, 0);
            E1kWrite(a, E1000_RCTL,
                     E1000_RCTL_EN | E1000_RCTL_UPE | E1000_RCTL_MPE |
                     E1000_RCTL_BAM | E1000_RCTL_BSIZE_2048 | E1000_RCTL_SECRC);
            /* Release all descriptors to hardware AFTER enabling RX. */
            E1kWrite(a, E1000_RDT, E1000_NUM_RX - 1);
        }

        for (i = 0; i < E1000_NUM_TX; i++)
        {
            a->TxRing[i].BufferAddr = E1kPhys(a, a->TxBuffers + i * E1000_BUF_SIZE);
            a->TxRing[i].Status = E1000_TXD_STAT_DD;   /* mark free */
            a->TxRing[i].Cmd = 0;
        }
        {
            ULONGLONG tphys = E1kPhys(a, a->TxRing);
            E1kWrite(a, E1000_TDBAL, (ULONG)tphys);
            E1kWrite(a, E1000_TDBAH, (ULONG)(tphys >> 32));
            E1kWrite(a, E1000_TDLEN, E1000_NUM_TX * (ULONG)sizeof(E1000_TX_DESC));
            E1kWrite(a, E1000_TDH, 0);
            E1kWrite(a, E1000_TDT, 0);
            E1kWrite(a, E1000_TIPG, 0x00602008);
            E1kWrite(a, E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT | E1000_TCTL_COLD);
        }
    }

    if (!(E1kRead(a, E1000_STATUS) & E1000_STATUS_LU))
    {
        USHORT phyCtrl = E1kPhyRead(a, MII_PHY_CONTROL);
        if (phyCtrl != 0xFFFF)
        {
            E1kPhyWrite(a, MII_PHY_CONTROL,
                        (USHORT)(phyCtrl | MII_CR_AUTO_NEG_EN | MII_CR_RESTART_AUTO_NEG));
            E1kDbg(a, "e1000: PHY present (ctrl=0x%x), restarted auto-neg\n", (ULONG)phyCtrl);
        }
    }

    /*
     * Wait for the link to come up.
     */
    ULONG ms = 0;
    BOOLEAN linkUp;
    status = E1kRead(a, E1000_STATUS);
    linkUp = (status & E1000_STATUS_LU) != 0;
    while (ms < 8000 && !linkUp)
    {
        E1kDelayMicroseconds(1000);   /* 1ms real time (PIT) */
        status = E1kRead(a, E1000_STATUS);
        linkUp = (status & E1000_STATUS_LU) != 0;
        /* On real hardware the PHY may report link slightly before the MAC
         * reflects it in STATUS.LU; accept either */
        if (!linkUp && (ms & 0x3F) == 0)
        {
            USHORT phySts = E1kPhyRead(a, MII_PHY_STATUS);
            if (phySts != 0xFFFF && (phySts & MII_SR_LINK_STATUS))
                linkUp = TRUE;
        }
        ms++;
    }
    /* Ensure at least ~1.3s elapsed for QEMU's flush_queue_timer even if the
     * link came up sooner. */
    for (; ms < 1300; ms++)
        E1kDelayMicroseconds(1000);

    if (KdNet->LinkState)
        *KdNet->LinkState = (UCHAR)(linkUp ? 1 : 0);
    KdNet->LinkSpeed = 1000;
    KdNet->LinkDuplex = 1;

    E1kDbg(a, "e1000: init done STATUS=0x%lx LinkUp=%u\n", status, (ULONG)(linkUp ? 1 : 0));

    return STATUS_SUCCESS;
}

VOID
E1000ShutdownController(PVOID Adapter)
{
    PE1000_ADAPTER a = (PE1000_ADAPTER)Adapter;
    if (a)
    {
        E1kWrite(a, E1000_RCTL, 0);
        E1kWrite(a, E1000_TCTL, 0);
        E1kWrite(a, E1000_IMC, 0xFFFFFFFF);
    }
}

NTSTATUS
E1000GetTxPacket(PVOID Adapter, PULONG Handle)
{
    PE1000_ADAPTER a = (PE1000_ADAPTER)Adapter;
    ULONG idx;

    if (!a || !Handle)
        return STATUS_INVALID_PARAMETER;

    idx = a->TxNext;
    /* The descriptor must be done (DD) before reuse. */
    if (!E1kTxDescDone(a, idx))
        return STATUS_IO_TIMEOUT;

    a->TxNext = (idx + 1) % E1000_NUM_TX;
    *Handle = idx | TRANSMIT_HANDLE;
    return STATUS_SUCCESS;
}

NTSTATUS
E1000SendTxPacket(PVOID Adapter, ULONG Handle, ULONG Length)
{
    PE1000_ADAPTER a = (PE1000_ADAPTER)Adapter;
    ULONG idx = Handle & ~HANDLE_FLAGS;
    ULONG spin;

    if (!a || idx >= E1000_NUM_TX)
        return STATUS_INVALID_PARAMETER;

    E1kTxDescSubmit(a, idx, Length);

    /* Advance the tail to the descriptor after this one. */
    E1kWrite(a, a->IsIgb ? IGB_TDT0 : E1000_TDT, (idx + 1) % E1000_NUM_TX);

    /* Poll for completion. */
    for (spin = 0; spin < 100000; spin++)
    {
        if (E1kTxDescDone(a, idx))
            return STATUS_SUCCESS;
        KeStallExecutionProcessor(2);
    }
    return STATUS_IO_TIMEOUT;
}

NTSTATUS
E1000GetRxPacket(PVOID Adapter, PULONG Handle, PVOID *Packet, PULONG Length)
{
    PE1000_ADAPTER a = (PE1000_ADAPTER)Adapter;
    ULONG idx;

    if (!a || !Handle || !Packet || !Length)
        return STATUS_INVALID_PARAMETER;

    idx = a->RxNext;
    if (!E1kRxDescDone(a, idx))
        return STATUS_IO_TIMEOUT;   /* nothing received */

    *Handle = idx;
    *Packet = a->RxBuffers + idx * E1000_BUF_SIZE;
    *Length = E1kRxDescLength(a, idx);
    return STATUS_SUCCESS;
}

VOID
E1000ReleaseRxPacket(PVOID Adapter, ULONG Handle)
{
    PE1000_ADAPTER a = (PE1000_ADAPTER)Adapter;
    ULONG idx = Handle & ~HANDLE_FLAGS;

    if (!a || idx >= E1000_NUM_RX)
        return;

    E1kRxDescRecycle(a, idx);
    /* Return the descriptor to hardware and advance. */
    E1kWrite(a, a->IsIgb ? IGB_RDT0 : E1000_RDT, idx);
    a->RxNext = (idx + 1) % E1000_NUM_RX;
}

PVOID
E1000GetPacketAddress(PVOID Adapter, ULONG Handle)
{
    PE1000_ADAPTER a = (PE1000_ADAPTER)Adapter;
    ULONG idx = Handle & ~HANDLE_FLAGS;

    if (!a)
        return NULL;
    if (Handle & TRANSMIT_HANDLE)
        return (idx < E1000_NUM_TX) ? (a->TxBuffers + idx * E1000_BUF_SIZE) : NULL;
    return (idx < E1000_NUM_RX) ? (a->RxBuffers + idx * E1000_BUF_SIZE) : NULL;
}

ULONG
E1000GetPacketLength(PVOID Adapter, ULONG Handle)
{
    PE1000_ADAPTER a = (PE1000_ADAPTER)Adapter;
    ULONG idx = Handle & ~HANDLE_FLAGS;

    if (!a)
        return 0;
    if (Handle & TRANSMIT_HANDLE)
        return E1000_BUF_SIZE;
    return (idx < E1000_NUM_RX) ? E1kRxDescLength(a, idx) : 0;
}
