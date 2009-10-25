/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/kd/i386/kdmemsup.c
 * PURPOSE:         Kernel Debugger Safe Memory Support
 *
 * PROGRAMMERS:     arty
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define HIGH_PHYS_MASK 0x80000000
#define PAGE_TABLE_MASK 0x3ff
#define BIG_PAGE_SIZE (1<<22)
#define CR4_PAGE_SIZE_BIT 0x10
#define PDE_PRESENT_BIT 0x01
#define PDE_W_BIT 0x02
#define PDE_PWT_BIT 0x08
#define PDE_PCD_BIT 0x10
#define PDE_ACCESSED_BIT 0x20
#define PDE_DIRTY_BIT 0x40
#define PDE_PS_BIT 0x80

#define MI_KDBG_TMP_PAGE_1 (HYPER_SPACE + 0x400000 - PAGE_SIZE)
#define MI_KDBG_TMP_PAGE_0 (MI_KDBG_TMP_PAGE_1 - PAGE_SIZE)

/* VARIABLES ***************************************************************/

static BOOLEAN KdpPhysAccess = FALSE;

static
ULONG_PTR
KdpPhysMap(ULONG_PTR PhysAddr, LONG Len)
{
    MMPTE TempPte;
    PMMPTE PointerPte;
    ULONG_PTR VirtAddr;

    TempPte.u.Long = PDE_PRESENT_BIT | PDE_W_BIT | PDE_PWT_BIT |
                     PDE_PCD_BIT | PDE_ACCESSED_BIT | PDE_DIRTY_BIT;

    if ((PhysAddr & (PAGE_SIZE - 1)) + Len > PAGE_SIZE)
    {
        TempPte.u.Hard.PageFrameNumber = (PhysAddr >> PAGE_SHIFT) + 1;
        PointerPte = MiAddressToPte(MI_KDBG_TMP_PAGE_1);
        *PointerPte = TempPte;
        VirtAddr = (ULONG_PTR)PointerPte << 10;
        __invlpg((PVOID)VirtAddr);
    }

    TempPte.u.Hard.PageFrameNumber = PhysAddr >> PAGE_SHIFT;
    PointerPte = MiAddressToPte(MI_KDBG_TMP_PAGE_0);
    *PointerPte = TempPte;
    VirtAddr = (ULONG_PTR)PointerPte << 10;
    __invlpg((PVOID)VirtAddr);

    return VirtAddr + (PhysAddr & (PAGE_SIZE - 1));
}

static
ULONGLONG
KdpPhysRead(ULONG_PTR PhysAddr, LONG Len)
{
    ULONG_PTR Addr;
    ULONGLONG Result = 0;

    Addr = KdpPhysMap(PhysAddr, Len);

    switch (Len)
    {
    case 8:
        Result = *((PULONGLONG)Addr);
        break;
    case 4:
        Result = *((PULONG)Addr);
        break;
    case 2:
        Result = *((PUSHORT)Addr);
        break;
    case 1:
        Result = *((PUCHAR)Addr);
        break;
    }

    return Result;
}

static
VOID
KdpPhysWrite(ULONG_PTR PhysAddr, LONG Len, ULONGLONG Value)
{
    ULONG_PTR Addr;

    Addr = KdpPhysMap(PhysAddr, Len);

    switch (Len)
    {
    case 8:
        *((PULONGLONG)Addr) = Value;
        break;
    case 4:
        *((PULONG)Addr) = Value;
        break;
    case 2:
        *((PUSHORT)Addr) = Value;
        break;
    case 1:
        *((PUCHAR)Addr) = Value;
        break;
    }
}

BOOLEAN
NTAPI
KdpTranslateAddress(ULONG_PTR Addr, PULONG_PTR ResultAddr)
{
    ULONG_PTR CR3Value = __readcr3();
    ULONG_PTR CR4Value = __readcr4();
    ULONG_PTR PageDirectory = (CR3Value & ~(PAGE_SIZE-1)) + 
        ((Addr >> 22) * sizeof(ULONG));
    ULONG_PTR PageDirectoryEntry = KdpPhysRead(PageDirectory, sizeof(ULONG));

    /* Not present -> fail */
    if (!(PageDirectoryEntry & PDE_PRESENT_BIT))
    {
        return FALSE;
    }

    /* Big Page? */
    if ((PageDirectoryEntry & PDE_PS_BIT) && (CR4Value & CR4_PAGE_SIZE_BIT))
    {
        *ResultAddr = (PageDirectoryEntry & ~(BIG_PAGE_SIZE-1)) +
            (Addr & (BIG_PAGE_SIZE-1));
        return TRUE;
    }
    else
    {
        ULONG_PTR PageTableAddr = 
            (PageDirectoryEntry & ~(PAGE_SIZE-1)) +
            ((Addr >> PAGE_SHIFT) & PAGE_TABLE_MASK) * sizeof(ULONG);
        ULONG_PTR PageTableEntry = KdpPhysRead(PageTableAddr, sizeof(ULONG));
        if (PageTableEntry & PDE_PRESENT_BIT)
        {
            *ResultAddr = (PageTableEntry & ~(PAGE_SIZE-1)) +
                (Addr & (PAGE_SIZE-1));
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
NTAPI
KdpSafeReadMemory(ULONG_PTR Addr, LONG Len, PVOID Value)
{
    ULONG_PTR ResultPhysAddr;

    if (!KdpPhysAccess)
    {
        memcpy(Value, (PVOID)Addr, Len);
        return TRUE;
    }

    memset(Value, 0, Len);
            
    if (!KdpTranslateAddress(Addr, &ResultPhysAddr))
        return FALSE;

    switch (Len)
    {
    case 8:
        *((PULONGLONG)Value) = KdpPhysRead(ResultPhysAddr, Len);
        break;
    case 4:
        *((PULONG)Value) = KdpPhysRead(ResultPhysAddr, Len);
        break;
    case 2:
        *((PUSHORT)Value) = KdpPhysRead(ResultPhysAddr, Len);
        break;
    case 1:
        *((PUCHAR)Value) = KdpPhysRead(ResultPhysAddr, Len);
        break;
    }

    return TRUE;
}

BOOLEAN
NTAPI
KdpSafeWriteMemory(ULONG_PTR Addr, LONG Len, ULONGLONG Value)
{
    ULONG_PTR ResultPhysAddr;

    if (!KdpPhysAccess)
    {
        memcpy((PVOID)Addr, &Value, Len);
        return TRUE;
    }
            
    if (!KdpTranslateAddress(Addr, &ResultPhysAddr))
        return FALSE;

    KdpPhysWrite(ResultPhysAddr, Len, Value);
    return TRUE;
}

VOID
NTAPI
KdpEnableSafeMem(VOID)
{
    KdpPhysAccess = TRUE;
}

/* EOF */
