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
#define PDE_PRESENT_BIT 1
#define PDE_W_BIT 2
#define PDE_PS_BIT 0x80

/* VARIABLES ***************************************************************/

extern ULONG MmGlobalKernelPageDirectory[1024];
static BOOLEAN KdpPhysAccess = FALSE;
ULONG_PTR IdentityMapAddrHigh, IdentityMapAddrLow;
extern PFN_TYPE NTAPI MmAllocEarlyPage();

ULONGLONG
FASTCALL
KdpPhysRead(ULONG_PTR Addr, LONG Len)
{
    ULONGLONG Result = 0;
    ULONG_PTR OldCR3 = __readcr3(), OldCR4 = __readcr4();

    if (Addr & HIGH_PHYS_MASK)
    {
        Addr &= ~HIGH_PHYS_MASK;
        __writecr3(IdentityMapAddrHigh);
    }
    else
        __writecr3(IdentityMapAddrLow);

    __writecr4(OldCR4|CR4_PAGE_SIZE_BIT); // Turn on large page translation
    __invlpg((PVOID)Addr);

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
    __writecr4(OldCR4); // Turn off large page translation
    __writecr3(OldCR3);
    __invlpg((PVOID)Addr);

    return Result;
}


VOID
NTAPI
KdpPhysWrite(ULONG_PTR Addr, LONG Len, ULONGLONG Value)
{
    ULONG_PTR OldCR3 = __readcr3(), OldCR4 = __readcr4();

    if (Addr & HIGH_PHYS_MASK)
    {
        Addr &= ~HIGH_PHYS_MASK;
        __writecr3(IdentityMapAddrHigh);
    }
    else
        __writecr3(IdentityMapAddrLow);

    __writecr4(OldCR4|CR4_PAGE_SIZE_BIT); // Turn on large page translation
    __invlpg((PVOID)Addr);

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
    __writecr4(OldCR4); // Turn off large page translation
    __writecr3(OldCR3);    
    __invlpg((PVOID)Addr);
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
KdpEnableSafeMem()
{
    int i;
    PULONG IdentityMapVirt;
    PHYSICAL_ADDRESS IdentityMapPhys, Highest = { };

    if (KdpPhysAccess)
        return;

    Highest.LowPart = (ULONG)-1;
    /* Allocate a physical page and map it to copy the phys copy code onto */
    IdentityMapVirt = (PULONG)MmAllocateContiguousMemory(2 * PAGE_SIZE, Highest);
    IdentityMapPhys = MmGetPhysicalAddress(IdentityMapVirt);
    IdentityMapAddrHigh = IdentityMapPhys.LowPart;

    /* Copy the kernel space */
    memcpy(IdentityMapVirt,
           MmGlobalKernelPageDirectory,
           PAGE_SIZE);

    /* Set up 512 4Mb pages (high 2Gig identity mapped) */
    for (i = 0; i < 512; i++)
    {
        IdentityMapVirt[i] = 
            HIGH_PHYS_MASK | (i << 22) | PDE_PS_BIT | PDE_W_BIT | PDE_PRESENT_BIT;
    }

    /* Allocate a physical page and map it to copy the phys copy code onto */
    IdentityMapAddrLow = IdentityMapAddrHigh + PAGE_SIZE;
    IdentityMapVirt += PAGE_SIZE / sizeof(ULONG);

    /* Copy the kernel space */
    memcpy(IdentityMapVirt,
           MmGlobalKernelPageDirectory,
           PAGE_SIZE);

    /* Set up 512 4Mb pages (low 2Gig identity mapped) */
    for (i = 0; i < 512; i++)
    {
        IdentityMapVirt[i] = (i << 22) | PDE_PS_BIT | PDE_W_BIT | PDE_PRESENT_BIT;
    }

    KdpPhysAccess = TRUE;
}
