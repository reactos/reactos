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
#define E1000_STATUS_FD 0x00000001   /* full duplex */
#define E1000_STATUS_LU 0x00000002
#define E1000_STATUS_SPEED_SHIFT 6   /* bits 7:6 -> 0=10,1=100,2/3=1000 Mbps */
#define E1000_STATUS_SPEED_MASK  0x3
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

/* ---- register helpers: MMIO access to the mapped BAR via the KDNET
 * extensibility imports (READ/WRITE_REGISTER_ULONG) ---- */
ULONG E1kRead(PE1000_ADAPTER E1kAdapter, ULONG off)
{
    return READ_REGISTER_ULONG((PULONG)(E1kAdapter->RegBase + off));
}
VOID E1kWrite(PE1000_ADAPTER E1kAdapter, ULONG off, ULONG val)
{
    WRITE_REGISTER_ULONG((PULONG)(E1kAdapter->RegBase + off), val);
}

static VOID E1kDelayMicroseconds(ULONG Microseconds)
{
    ULONG transitions = (Microseconds / 15) + 1;
    UCHAR prev = (UCHAR)(READ_PORT_UCHAR((PUCHAR)0x61) & 0x10);
    while (transitions)
    {
        UCHAR cur = (UCHAR)(READ_PORT_UCHAR((PUCHAR)0x61) & 0x10);
        if (cur != prev)
        {
            prev = cur;
            --transitions;
        }
    }
}

ULONG E1kAlignUp(ULONG v, ULONG a)
{
    return (v + (a - 1)) & ~(a - 1);
}

/* Read an internal PHY register over MDIC. Returns 0xFFFF if there's no MDIO
 * PHY or the access errors/times out (e.g. emulated/fiber NICs). */
static USHORT E1kPhyRead(PE1000_ADAPTER E1kAdapter, ULONG reg)
{
    ULONG mdic, i;
    E1kWrite(E1kAdapter, E1000_MDIC,
             (reg << E1000_MDIC_REG_SHIFT) | (E1000_PHY_ADDR << E1000_MDIC_PHY_SHIFT) | E1000_MDIC_OP_READ);
    for (i = 0; i < 2000; i++)
    {
        mdic = E1kRead(E1kAdapter, E1000_MDIC);
        if (mdic & E1000_MDIC_READY)
            return (mdic & E1000_MDIC_ERROR) ? 0xFFFF : (USHORT)(mdic & E1000_MDIC_DATA);
        E1kDelayMicroseconds(50);
    }
    return 0xFFFF;
}

static VOID E1kPhyWrite(PE1000_ADAPTER E1kAdapter, ULONG reg, USHORT val)
{
    ULONG i;
    E1kWrite(E1kAdapter, E1000_MDIC,
             (ULONG)val | (reg << E1000_MDIC_REG_SHIFT) |
             (E1000_PHY_ADDR << E1000_MDIC_PHY_SHIFT) | E1000_MDIC_OP_WRITE);
    for (i = 0; i < 2000; i++)
    {
        if (E1kRead(E1kAdapter, E1000_MDIC) & E1000_MDIC_READY)
            return;
        E1kDelayMicroseconds(50);
    }
}

/* Read an EEPROM word. The address-shift and DONE bit differ between the legacy
 * 8254x and the 82571+/82574 families. */
static USHORT E1kEepromRead(PE1000_ADAPTER E1kAdapter, ULONG word, BOOLEAN newFormat)
{
    ULONG addrShift = newFormat ? 2 : 8;
    ULONG doneBit   = newFormat ? 0x02 : 0x10;
    ULONG eerd, i;
    E1kWrite(E1kAdapter, E1000_EERD, (word << addrShift) | E1000_EERD_START);
    for (i = 0; i < 2000; i++)
    {
        eerd = E1kRead(E1kAdapter, E1000_EERD);
        if (eerd & doneBit)
            return (USHORT)(eerd >> 16);
        E1kDelayMicroseconds(50);
    }
    return 0xFFFF;
}

/* 82571 and later (incl. 82574L 0x10D3) use the newer EEPROM (EERD) layout. */
BOOLEAN E1kIsNewerFamily(USHORT DeviceId)
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

ULONGLONG E1kPhys(PE1000_ADAPTER E1kAdapter, PVOID Va);

/* ---- descriptor accessors (branch legacy vs igb advanced) ---- */

BOOLEAN E1kTxDescDone(PE1000_ADAPTER E1kAdapter, ULONG idx)
{
    if (E1kAdapter->IsIgb)
        return (((PIGB_ADV_TX_DESC)E1kAdapter->TxRing)[idx].OlinfoStatus & IGB_TXD_STAT_DD) != 0;
    return (E1kAdapter->TxRing[idx].Status & E1000_TXD_STAT_DD) != 0;
}

VOID E1kTxDescSubmit(PE1000_ADAPTER E1kAdapter, ULONG idx, ULONG len)
{
    if (E1kAdapter->IsIgb)
    {
        PIGB_ADV_TX_DESC d = &((PIGB_ADV_TX_DESC)E1kAdapter->TxRing)[idx];
        d->CmdTypeLen = (len & 0xFFFF) | IGB_TXD_DTYP_DATA | IGB_TXD_DCMD_DEXT |
                        IGB_TXD_DCMD_EOP | IGB_TXD_DCMD_IFCS | IGB_TXD_DCMD_RS;
        d->OlinfoStatus = len << 14;   /* PAYLEN; hw clears DD then sets it on done */
    }
    else
    {
        PE1000_TX_DESC d = &E1kAdapter->TxRing[idx];
        d->Length = (USHORT)len;
        d->Cso = 0; d->Css = 0; d->Special = 0; d->Status = 0;
        d->Cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
    }
}

BOOLEAN E1kRxDescDone(PE1000_ADAPTER E1kAdapter, ULONG idx)
{
    if (E1kAdapter->IsIgb)
        return (((PIGB_ADV_RX_DESC)E1kAdapter->RxRing)[idx].u.Wb.StatusError & IGB_RXD_STAT_DD) != 0;
    return (E1kAdapter->RxRing[idx].Status & E1000_RXD_STAT_DD) != 0;
}

USHORT E1kRxDescLength(PE1000_ADAPTER E1kAdapter, ULONG idx)
{
    if (E1kAdapter->IsIgb)
        return ((PIGB_ADV_RX_DESC)E1kAdapter->RxRing)[idx].u.Wb.Length;
    return E1kAdapter->RxRing[idx].Length;
}

/* Re-arm an RX descriptor for hardware reuse (restore the read format). */
VOID E1kRxDescRecycle(PE1000_ADAPTER E1kAdapter, ULONG idx)
{
    if (E1kAdapter->IsIgb)
    {
        PIGB_ADV_RX_DESC d = &((PIGB_ADV_RX_DESC)E1kAdapter->RxRing)[idx];
        d->PktAddr = E1kPhys(E1kAdapter, E1kAdapter->RxBuffers + idx * E1000_BUF_SIZE);
        d->u.HdrAddr = 0;
    }
    else
    {
        E1kAdapter->RxRing[idx].Status = 0;
        E1kAdapter->RxRing[idx].Length = 0;
    }
}

ULONG
NTAPI
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

ULONGLONG
E1kPhys(PE1000_ADAPTER E1kAdapter, PVOID Va)
{
    return E1kAdapter->CtxPhys + (ULONGLONG)((PUCHAR)Va - E1kAdapter->CtxVa);
}

#define E1kDbg(E1kAdapter, ...) \
    do { if ((E1kAdapter)->Shared && KdNetExtensibilityImports && KdNetDbgPrintf) \
            KdNetDbgPrintf(__VA_ARGS__); } while (0)

NTSTATUS
NTAPI
E1000InitializeController(PKDNET_SHARED_DATA KdNet)
{
    PE1000_ADAPTER E1kAdapter;
    PUCHAR base;
    ULONG off, i, status;

    if (KdNetExtensibilityImports && KdNetDbgPrintf)
        KdNetDbgPrintf(
            "e1000: enter KdNet=%p Hw=%p Dev=%p\n",
            KdNet, KdNet ? KdNet->Hardware : NULL, KdNet ? KdNet->Device : NULL);

    if (!KdNet || !KdNet->Hardware || !KdNet->Device)
        return STATUS_INVALID_PARAMETER;
    if (!KdNet->Device->BaseAddress[0].Valid)
        return STATUS_INVALID_PARAMETER;

    if (KdNetExtensibilityImports && KdNetDbgPrintf)
        KdNetDbgPrintf(
            "e1000: BAR0 valid=%u type=%u xlat=%p\n",
            (ULONG)KdNet->Device->BaseAddress[0].Valid,
            (ULONG)KdNet->Device->BaseAddress[0].Type,
            KdNet->Device->BaseAddress[0].TranslatedAddress);

    /* Carve the context: adapter, rings, buffers. */
    E1kAdapter = (PE1000_ADAPTER)KdNet->Hardware;
    base = (PUCHAR)KdNet->Hardware;

    /* Zero only the adapter header (kdnet already zeroed the whole context). */
    {
        PUCHAR z = (PUCHAR)E1kAdapter; ULONG n = sizeof(*E1kAdapter);
        while (n--) *z++ = 0;
    }

    E1kAdapter->RegBase = KdNet->Device->BaseAddress[0].TranslatedAddress;
    E1kAdapter->Shared = KdNet;
    E1kAdapter->CtxVa = (PUCHAR)KdNet->Hardware;
    E1kAdapter->CtxPhys = (ULONGLONG)KdNet->Device->Memory.Start.QuadPart;
    E1kAdapter->DeviceId = KdNet->Device->DeviceID;
    E1kAdapter->IsIgb = E1kIsIgb(E1kAdapter->DeviceId);

    E1kDbg(E1kAdapter, "e1000: chip 8086:%04x%s\n", (ULONG)E1kAdapter->DeviceId,
           E1kAdapter->IsIgb ? " (igb/advanced)" : "");

    E1kDbg(E1kAdapter, "e1000: RegBase=%p CtxVa=%p CtxPhysLo=0x%lx\n",
           E1kAdapter->RegBase, E1kAdapter->CtxVa, (ULONG)E1kAdapter->CtxPhys);

    /* Ensure PCI Bus Master + Memory Space decode are enabled. QEMU's e1000
     * refuses to DMA received packets unless Bus Master is set, and the bit can
     * be left clear here. Use config-space I/O (0xCF8/0xCFC) through the KDNET
     * extensibility port accessors. */
     ULONG slot = KdNet->Device->Slot;
     ULONG bus = KdNet->Device->Bus;
     ULONG devn = slot & 0x1F;
     ULONG func = (slot >> 5) & 0x7;
     ULONG cfg = 0x80000000UL | (bus << 16) | (devn << 11) | (func << 8) | 0x04;
     ULONG cmd;
     WRITE_PORT_ULONG((PULONG)0xCF8, cfg);
     cmd = READ_PORT_ULONG((PULONG)0xCFC);
     WRITE_PORT_ULONG((PULONG)0xCF8, cfg);
     WRITE_PORT_ULONG((PULONG)0xCFC, cmd | 0x06);   /* Memory Space (1) + Bus Master (2) */
     E1kDbg(E1kAdapter, "e1000: PCI cmd 0x%lx -> 0x%lx (BM+MEM)\n", cmd, cmd | 0x06);


    off = E1kAlignUp((ULONG)sizeof(E1000_ADAPTER), 128);
    E1kAdapter->TxRing = (PE1000_TX_DESC)(base + off);
    off = E1kAlignUp(off + E1000_NUM_TX * (ULONG)sizeof(E1000_TX_DESC), 128);
    E1kAdapter->RxRing = (PE1000_RX_DESC)(base + off);
    off = E1kAlignUp(off + E1000_NUM_RX * (ULONG)sizeof(E1000_RX_DESC), 16);
    E1kAdapter->TxBuffers = base + off;
    off = E1kAlignUp(off + E1000_NUM_TX * E1000_BUF_SIZE, 16);
    E1kAdapter->RxBuffers = base + off;

    /* Mask interrupts, then reset the device. */
    E1kWrite(E1kAdapter, E1000_IMC, 0xFFFFFFFF);
    (void)E1kRead(E1kAdapter, E1000_ICR);
    E1kWrite(E1kAdapter, E1000_CTRL, E1kRead(E1kAdapter, E1000_CTRL) | E1000_CTRL_RST);
    KeStallExecutionProcessor(20000);   /* ~20ms for reset to settle */

    /* Re-mask interrupts (reset clears the mask). */
    E1kWrite(E1kAdapter, E1000_IMC, 0xFFFFFFFF);
    (void)E1kRead(E1kAdapter, E1000_ICR);


    E1kWrite(E1kAdapter, E1000_CTRL, E1000_CTRL_SLU | E1000_CTRL_ASDE);
    E1kWrite(E1kAdapter, E1000_FCAL, 0);
    E1kWrite(E1kAdapter, E1000_FCAH, 0);
    E1kWrite(E1kAdapter, E1000_FCT, 0);
    E1kWrite(E1kAdapter, E1000_FCTTV, 0);

    /* Read the MAC from the receive address registers. If they weren't
     * auto-loaded from the EEPROM (RA0 empty — happens on some real hardware /
     * after certain resets), read the MAC straight out of the EEPROM. */
    {
        ULONG ral = E1kRead(E1kAdapter, E1000_RAL0);
        ULONG rah = E1kRead(E1kAdapter, E1000_RAH0);
        E1kAdapter->Mac[0] = (UCHAR)(ral);
        E1kAdapter->Mac[1] = (UCHAR)(ral >> 8);
        E1kAdapter->Mac[2] = (UCHAR)(ral >> 16);
        E1kAdapter->Mac[3] = (UCHAR)(ral >> 24);
        E1kAdapter->Mac[4] = (UCHAR)(rah);
        E1kAdapter->Mac[5] = (UCHAR)(rah >> 8);

        if (ral == 0 && (rah & 0xFFFF) == 0)
        {
            BOOLEAN newF = E1kIsNewerFamily(E1kAdapter->DeviceId);
            USHORT w0 = E1kEepromRead(E1kAdapter, 0, newF);
            USHORT w1 = E1kEepromRead(E1kAdapter, 1, newF);
            USHORT w2 = E1kEepromRead(E1kAdapter, 2, newF);
            if (!(w0 == 0xFFFF && w1 == 0xFFFF && w2 == 0xFFFF))
            {
                E1kAdapter->Mac[0] = (UCHAR)w0;       E1kAdapter->Mac[1] = (UCHAR)(w0 >> 8);
                E1kAdapter->Mac[2] = (UCHAR)w1;       E1kAdapter->Mac[3] = (UCHAR)(w1 >> 8);
                E1kAdapter->Mac[4] = (UCHAR)w2;       E1kAdapter->Mac[5] = (UCHAR)(w2 >> 8);
                /* Program it back into RA0 so the hardware filters on it. */
                E1kWrite(E1kAdapter, E1000_RAL0,
                         (ULONG)E1kAdapter->Mac[0] | ((ULONG)E1kAdapter->Mac[1] << 8) |
                         ((ULONG)E1kAdapter->Mac[2] << 16) | ((ULONG)E1kAdapter->Mac[3] << 24));
                E1kWrite(E1kAdapter, E1000_RAH0,
                         (ULONG)E1kAdapter->Mac[4] | ((ULONG)E1kAdapter->Mac[5] << 8) | 0x80000000u /* AV */);
                E1kDbg(E1kAdapter, "e1000: MAC read from EEPROM\n");
            }
        }

        if (KdNet->TargetMacAddress)
        {
            for (i = 0; i < 6; i++)
                KdNet->TargetMacAddress[i] = E1kAdapter->Mac[i];
        }
        E1kDbg(E1kAdapter, "e1000: MAC %02x-%02x-%02x-%02x-%02x-%02x RAL=0x%lx RAH=0x%lx\n",
               E1kAdapter->Mac[0], E1kAdapter->Mac[1], E1kAdapter->Mac[2], E1kAdapter->Mac[3], E1kAdapter->Mac[4], E1kAdapter->Mac[5], ral, rah);
    }

    /* Clear the multicast table. */
    for (i = 0; i < 128; i++)
        E1kWrite(E1kAdapter, E1000_MTA + i * 4, 0);

    E1kAdapter->RxNext = 0;
    E1kAdapter->TxNext = 0;

    if (E1kAdapter->IsIgb)
    {
        ULONGLONG rphys = E1kPhys(E1kAdapter, E1kAdapter->RxRing);
        ULONGLONG tphys = E1kPhys(E1kAdapter, E1kAdapter->TxRing);
        ULONG t;

        /* igb: advanced descriptors + per-queue register block. Mask the
         * extended interrupts too. */
        E1kWrite(E1kAdapter, IGB_EIMC, 0xFFFFFFFF);

        /* --- RX queue 0 --- */
        E1kWrite(E1kAdapter, E1000_RCTL, 0);   /* disable RX while configuring */

        for (i = 0; i < E1000_NUM_RX; i++)
        {
            PIGB_ADV_RX_DESC d = &((PIGB_ADV_RX_DESC)E1kAdapter->RxRing)[i];
            d->PktAddr = E1kPhys(E1kAdapter, E1kAdapter->RxBuffers + i * E1000_BUF_SIZE);
            d->u.HdrAddr = 0;
        }
        E1kWrite(E1kAdapter, IGB_RDBAL0, (ULONG)rphys);
        E1kWrite(E1kAdapter, IGB_RDBAH0, (ULONG)(rphys >> 32));
        E1kWrite(E1kAdapter, IGB_RDLEN0, E1000_NUM_RX * (ULONG)sizeof(IGB_ADV_RX_DESC));
        E1kWrite(E1kAdapter, IGB_SRRCTL0, IGB_SRRCTL_BSIZE_2K | IGB_SRRCTL_DESCTYPE_ADV);
        E1kWrite(E1kAdapter, IGB_RDH0, 0);
        E1kWrite(E1kAdapter, IGB_RDT0, 0);
        /* Enable the queue and wait for hardware to acknowledge. */
        E1kWrite(E1kAdapter, IGB_RXDCTL0, IGB_XDCTL_ENABLE);
        for (t = 0; t < 1000; t++)
        {
            if (E1kRead(E1kAdapter, IGB_RXDCTL0) & IGB_XDCTL_ENABLE) break;
            E1kDelayMicroseconds(100);
        }
        E1kWrite(E1kAdapter, E1000_RCTL,
                 E1000_RCTL_EN | E1000_RCTL_UPE | E1000_RCTL_MPE |
                 E1000_RCTL_BAM | E1000_RCTL_SECRC);
        E1kWrite(E1kAdapter, IGB_RDT0, E1000_NUM_RX - 1);

        /* --- TX queue 0 --- */
        for (i = 0; i < E1000_NUM_TX; i++)
        {
            PIGB_ADV_TX_DESC d = &((PIGB_ADV_TX_DESC)E1kAdapter->TxRing)[i];
            d->BufferAddr = E1kPhys(E1kAdapter, E1kAdapter->TxBuffers + i * E1000_BUF_SIZE);
            d->CmdTypeLen = 0;
            d->OlinfoStatus = IGB_TXD_STAT_DD;   /* mark free */
        }
        E1kWrite(E1kAdapter, IGB_TDBAL0, (ULONG)tphys);
        E1kWrite(E1kAdapter, IGB_TDBAH0, (ULONG)(tphys >> 32));
        E1kWrite(E1kAdapter, IGB_TDLEN0, E1000_NUM_TX * (ULONG)sizeof(IGB_ADV_TX_DESC));
        E1kWrite(E1kAdapter, IGB_TDH0, 0);
        E1kWrite(E1kAdapter, IGB_TDT0, 0);
        E1kWrite(E1kAdapter, IGB_TXDCTL0, IGB_XDCTL_ENABLE);
        for (t = 0; t < 1000; t++)
        {
            if (E1kRead(E1kAdapter, IGB_TXDCTL0) & IGB_XDCTL_ENABLE) break;
            E1kDelayMicroseconds(100);
        }
        E1kWrite(E1kAdapter, E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT | E1000_TCTL_COLD);
    }
    else
    {
        /* Legacy 8254x/8257x descriptors. */
        for (i = 0; i < E1000_NUM_RX; i++)
        {
            E1kAdapter->RxRing[i].BufferAddr = E1kPhys(E1kAdapter, E1kAdapter->RxBuffers + i * E1000_BUF_SIZE);
            E1kAdapter->RxRing[i].Status = 0;
            E1kAdapter->RxRing[i].Length = 0;
        }
        {
            ULONGLONG rphys = E1kPhys(E1kAdapter, E1kAdapter->RxRing);
            E1kWrite(E1kAdapter, E1000_RDBAL, (ULONG)rphys);
            E1kWrite(E1kAdapter, E1000_RDBAH, (ULONG)(rphys >> 32));
            E1kWrite(E1kAdapter, E1000_RDLEN, E1000_NUM_RX * (ULONG)sizeof(E1000_RX_DESC));
            E1kWrite(E1kAdapter, E1000_RDH, 0);
            E1kWrite(E1kAdapter, E1000_RDT, 0);
            E1kWrite(E1kAdapter, E1000_RCTL,
                     E1000_RCTL_EN | E1000_RCTL_UPE | E1000_RCTL_MPE |
                     E1000_RCTL_BAM | E1000_RCTL_BSIZE_2048 | E1000_RCTL_SECRC);
            /* Release all descriptors to hardware AFTER enabling RX. */
            E1kWrite(E1kAdapter, E1000_RDT, E1000_NUM_RX - 1);
        }

        for (i = 0; i < E1000_NUM_TX; i++)
        {
            E1kAdapter->TxRing[i].BufferAddr = E1kPhys(E1kAdapter, E1kAdapter->TxBuffers + i * E1000_BUF_SIZE);
            E1kAdapter->TxRing[i].Status = E1000_TXD_STAT_DD;   /* mark free */
            E1kAdapter->TxRing[i].Cmd = 0;
        }
        {
            ULONGLONG tphys = E1kPhys(E1kAdapter, E1kAdapter->TxRing);
            E1kWrite(E1kAdapter, E1000_TDBAL, (ULONG)tphys);
            E1kWrite(E1kAdapter, E1000_TDBAH, (ULONG)(tphys >> 32));
            E1kWrite(E1kAdapter, E1000_TDLEN, E1000_NUM_TX * (ULONG)sizeof(E1000_TX_DESC));
            E1kWrite(E1kAdapter, E1000_TDH, 0);
            E1kWrite(E1kAdapter, E1000_TDT, 0);
            E1kWrite(E1kAdapter, E1000_TIPG, 0x00602008);
            E1kWrite(E1kAdapter, E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT | E1000_TCTL_COLD);
        }
    }

    if (!(E1kRead(E1kAdapter, E1000_STATUS) & E1000_STATUS_LU))
    {
        USHORT phyCtrl = E1kPhyRead(E1kAdapter, MII_PHY_CONTROL);
        if (phyCtrl != 0xFFFF)
        {
            E1kPhyWrite(E1kAdapter, MII_PHY_CONTROL,
                        (USHORT)(phyCtrl | MII_CR_AUTO_NEG_EN | MII_CR_RESTART_AUTO_NEG));
            E1kDbg(E1kAdapter, "e1000: PHY present (ctrl=0x%x), restarted auto-neg\n", (ULONG)phyCtrl);
        }
    }

    ULONG ms = 0;
    BOOLEAN linkUp;
    status = E1kRead(E1kAdapter, E1000_STATUS);
    linkUp = (status & E1000_STATUS_LU) != 0;
    while (ms < 10000 && !linkUp)
    {
        E1kDelayMicroseconds(1000);
        status = E1kRead(E1kAdapter, E1000_STATUS);
        linkUp = (status & E1000_STATUS_LU) != 0;
        ms++;
    }
    E1kDbg(E1kAdapter, "e1000: link wait %lums LU=%u STATUS=0x%lx\n",
           ms, (ULONG)(linkUp ? 1 : 0), status);

    /* Ensure at least ~1.3s elapsed */
    for (; ms < 1300; ms++)
        E1kDelayMicroseconds(1000);

    if (KdNet->LinkState)
        *KdNet->LinkState = (UCHAR)(linkUp ? 1 : 0);
    {
        /* Report the negotiated speed/duplex (ASDE made the MAC adopt them). */
        ULONG spd = (status >> E1000_STATUS_SPEED_SHIFT) & E1000_STATUS_SPEED_MASK;
        KdNet->LinkSpeed  = (spd == 0) ? 10 : (spd == 1) ? 100 : 1000;
        KdNet->LinkDuplex = (status & E1000_STATUS_FD) ? 1 : 0;
    }

    E1kDbg(E1kAdapter, "e1000: init done STATUS=0x%lx LinkUp=%u\n", status, (ULONG)(linkUp ? 1 : 0));

    return STATUS_SUCCESS;
}

VOID
NTAPI
E1000ShutdownController(PVOID Adapter)
{
    PE1000_ADAPTER E1kAdapter = (PE1000_ADAPTER)Adapter;
    if (E1kAdapter)
    {
        E1kWrite(E1kAdapter, E1000_RCTL, 0);
        E1kWrite(E1kAdapter, E1000_TCTL, 0);
        E1kWrite(E1kAdapter, E1000_IMC, 0xFFFFFFFF);
    }
}

NTSTATUS
NTAPI
E1000GetTxPacket(PVOID Adapter, PULONG Handle)
{
    PE1000_ADAPTER E1kAdapter = (PE1000_ADAPTER)Adapter;
    ULONG idx;

    if (!E1kAdapter || !Handle)
        return STATUS_INVALID_PARAMETER;

    idx = E1kAdapter->TxNext;
    /* The descriptor must be done (DD) before reuse. */
    if (!E1kTxDescDone(E1kAdapter, idx))
        return STATUS_IO_TIMEOUT;

    E1kAdapter->TxNext = (idx + 1) % E1000_NUM_TX;
    *Handle = idx | TRANSMIT_HANDLE;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
E1000SendTxPacket(PVOID Adapter, ULONG Handle, ULONG Length)
{
    PE1000_ADAPTER E1kAdapter = (PE1000_ADAPTER)Adapter;
    ULONG idx = Handle & ~HANDLE_FLAGS;
    ULONG spin;

    if (!E1kAdapter || idx >= E1000_NUM_TX)
        return STATUS_INVALID_PARAMETER;

    E1kTxDescSubmit(E1kAdapter, idx, Length);

    /* Advance the tail to the descriptor after this one. */
    E1kWrite(E1kAdapter, E1kAdapter->IsIgb ? IGB_TDT0 : E1000_TDT, (idx + 1) % E1000_NUM_TX);

    /* Poll for completion. */
    for (spin = 0; spin < 100000; spin++)
    {
        if (E1kTxDescDone(E1kAdapter, idx))
            return STATUS_SUCCESS;
        KeStallExecutionProcessor(2);
    }
    return STATUS_IO_TIMEOUT;
}

NTSTATUS
NTAPI
E1000GetRxPacket(PVOID Adapter, PULONG Handle, PVOID *Packet, PULONG Length)
{
    PE1000_ADAPTER E1kAdapter = (PE1000_ADAPTER)Adapter;
    ULONG idx;

    if (!E1kAdapter || !Handle || !Packet || !Length)
        return STATUS_INVALID_PARAMETER;

    idx = E1kAdapter->RxNext;
    if (!E1kRxDescDone(E1kAdapter, idx))
        return STATUS_IO_TIMEOUT;   /* nothing received */

    *Handle = idx;
    *Packet = E1kAdapter->RxBuffers + idx * E1000_BUF_SIZE;
    *Length = E1kRxDescLength(E1kAdapter, idx);
    return STATUS_SUCCESS;
}

VOID
NTAPI
E1000ReleaseRxPacket(PVOID Adapter, ULONG Handle)
{
    PE1000_ADAPTER E1kAdapter = (PE1000_ADAPTER)Adapter;
    ULONG idx = Handle & ~HANDLE_FLAGS;

    if (!E1kAdapter || idx >= E1000_NUM_RX)
        return;

    E1kRxDescRecycle(E1kAdapter, idx);

    /* Return the descriptor to hardware and advance. */
    E1kWrite(E1kAdapter, E1kAdapter->IsIgb ? IGB_RDT0 : E1000_RDT, idx);
    E1kAdapter->RxNext = (idx + 1) % E1000_NUM_RX;
}

PVOID
NTAPI
E1000GetPacketAddress(PVOID Adapter, ULONG Handle)
{
    PE1000_ADAPTER E1kAdapter = (PE1000_ADAPTER)Adapter;
    ULONG idx = Handle & ~HANDLE_FLAGS;

    if (!E1kAdapter)
        return NULL;
    if (Handle & TRANSMIT_HANDLE)
        return (idx < E1000_NUM_TX) ? (E1kAdapter->TxBuffers + idx * E1000_BUF_SIZE) : NULL;
    return (idx < E1000_NUM_RX) ? (E1kAdapter->RxBuffers + idx * E1000_BUF_SIZE) : NULL;
}

ULONG
NTAPI
E1000GetPacketLength(PVOID Adapter, ULONG Handle)
{
    PE1000_ADAPTER E1kAdapter = (PE1000_ADAPTER)Adapter;
    ULONG idx = Handle & ~HANDLE_FLAGS;

    if (!E1kAdapter)
        return 0;
    if (Handle & TRANSMIT_HANDLE)
        return E1000_BUF_SIZE;
    return (idx < E1000_NUM_RX) ? E1kRxDescLength(E1kAdapter, idx) : 0;
}
