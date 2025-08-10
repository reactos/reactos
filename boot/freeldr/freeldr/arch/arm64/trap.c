/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 exception and trap handlers
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <freeldr.h>
#include <arch/arm64/arm64.h>
#include <debug.h>

/* Exception syndrome register bits */
#define ESR_ELx_EC_SHIFT    26
#define ESR_ELx_EC_MASK     0x3F
#define ESR_ELx_IL          (1UL << 25)
#define ESR_ELx_ISS_MASK    0x1FFFFFF

/* Exception classes */
#define ESR_ELx_EC_UNKNOWN      0x00
#define ESR_ELx_EC_WFx          0x01
#define ESR_ELx_EC_SVC64        0x15
#define ESR_ELx_EC_HVC64        0x16
#define ESR_ELx_EC_SMC64        0x17
#define ESR_ELx_EC_SYS64        0x18
#define ESR_ELx_EC_IABT_LOW     0x20
#define ESR_ELx_EC_IABT_CUR     0x21
#define ESR_ELx_EC_PC_ALIGN     0x22
#define ESR_ELx_EC_DABT_LOW     0x24
#define ESR_ELx_EC_DABT_CUR     0x25
#define ESR_ELx_EC_SP_ALIGN     0x26
#define ESR_ELx_EC_BREAKPT_CUR  0x31
#define ESR_ELx_EC_SOFTSTP_CUR  0x33
#define ESR_ELx_EC_WATCHPT_CUR  0x35
#define ESR_ELx_EC_BRK64        0x3C

/* Data abort ISS bits */
#define ESR_ELx_ISV             (1UL << 24)
#define ESR_ELx_SAS_SHIFT       22
#define ESR_ELx_SAS_MASK        0x3
#define ESR_ELx_SSE             (1UL << 21)
#define ESR_ELx_SRT_SHIFT       16
#define ESR_ELx_SRT_MASK        0x1F
#define ESR_ELx_SF              (1UL << 15)
#define ESR_ELx_AR              (1UL << 14)
#define ESR_ELx_EA              (1UL << 9)
#define ESR_ELx_CM              (1UL << 8)
#define ESR_ELx_S1PTW           (1UL << 7)
#define ESR_ELx_WNR             (1UL << 6)
#define ESR_ELx_DFSC_MASK       0x3F

/* Fault status codes */
#define ESR_ELx_FSC_ADDR_SIZE_0 0x00
#define ESR_ELx_FSC_ADDR_SIZE_1 0x01
#define ESR_ELx_FSC_ADDR_SIZE_2 0x02
#define ESR_ELx_FSC_ADDR_SIZE_3 0x03
#define ESR_ELx_FSC_TRANS_0     0x04
#define ESR_ELx_FSC_TRANS_1     0x05
#define ESR_ELx_FSC_TRANS_2     0x06
#define ESR_ELx_FSC_TRANS_3     0x07
#define ESR_ELx_FSC_ACCESS_0    0x08
#define ESR_ELx_FSC_ACCESS_1    0x09
#define ESR_ELx_FSC_ACCESS_2    0x0A
#define ESR_ELx_FSC_ACCESS_3    0x0B
#define ESR_ELx_FSC_PERM_0      0x0C
#define ESR_ELx_FSC_PERM_1      0x0D
#define ESR_ELx_FSC_PERM_2      0x0E
#define ESR_ELx_FSC_PERM_3      0x0F

static const char* Arm64ExceptionNames[] = {
    "Unknown",
    "WFI/WFE",
    "Unknown", "CP15 32-bit", "CP15 64-bit", "CP14 MRC/MCR", "CP14 LDC/STC", "FP/ASIMD",
    "CP10 ID", "Unknown", "Unknown", "Unknown", "CP14 64-bit", "Unknown", "ILL", "Unknown",
    "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "SVC", "HVC", "SMC",
    "SYS", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "IMP DEF",
    "IABT (lower)", "IABT (current)", "PC align", "Unknown", "DABT (lower)", "DABT (current)", "SP align", "Unknown",
    "FP exception (32-bit)", "Unknown", "Unknown", "Unknown", "FP exception (64-bit)", "Unknown", "Unknown", "SError",
    "Breakpoint (lower)", "Breakpoint (current)", "Soft step (lower)", "Soft step (current)", 
    "Watchpoint (lower)", "Watchpoint (current)", "Unknown", "Unknown",
    "BKPT (32-bit)", "Unknown", "Unknown", "Unknown", "BRK (64-bit)", "Unknown", "Unknown", "Unknown"
};

static const char* Arm64FaultStatusNames[] = {
    "Address size fault (level 0)", "Address size fault (level 1)", "Address size fault (level 2)", "Address size fault (level 3)",
    "Translation fault (level 0)", "Translation fault (level 1)", "Translation fault (level 2)", "Translation fault (level 3)",
    "Access flag fault (level 0)", "Access flag fault (level 1)", "Access flag fault (level 2)", "Access flag fault (level 3)",
    "Permission fault (level 0)", "Permission fault (level 1)", "Permission fault (level 2)", "Permission fault (level 3)"
};

VOID Arm64DumpContext(PARM64_CONTEXT Context)
{
    ULONG i;
    
    ERR("ARM64 Exception Context:\n");
    ERR("PC: 0x%016llx  SP: 0x%016llx  PSTATE: 0x%016llx\n", 
        Context->PC, Context->SP, Context->PSTATE);
    
    for (i = 0; i < 31; i += 2)
    {
        if (i < 30)
        {
            ERR("X%02lu: 0x%016llx  X%02lu: 0x%016llx\n",
                i, Context->X[i], i + 1, Context->X[i + 1]);
        }
        else
        {
            ERR("X%02lu: 0x%016llx\n", i, Context->X[i]);
        }
    }
}

VOID Arm64HandleSynchronousException(PARM64_CONTEXT Context, ULONGLONG Esr, ULONGLONG FaultAddr, ULONGLONG PC)
{
    ULONG ExceptionClass = (Esr >> ESR_ELx_EC_SHIFT) & ESR_ELx_EC_MASK;
    ULONG ISS = Esr & ESR_ELx_ISS_MASK;
    const char* ExceptionName;
    
    /* Get exception name */
    if (ExceptionClass < ARRAYSIZE(Arm64ExceptionNames))
    {
        ExceptionName = Arm64ExceptionNames[ExceptionClass];
    }
    else
    {
        ExceptionName = "Invalid";
    }
    
    ERR("ARM64 Synchronous Exception: %s (EC=0x%02lx)\n", ExceptionName, ExceptionClass);
    ERR("ESR: 0x%016llx  FAR: 0x%016llx  ELR: 0x%016llx\n", Esr, FaultAddr, PC);
    
    switch (ExceptionClass)
    {
        case ESR_ELx_EC_DABT_CUR:
        case ESR_ELx_EC_DABT_LOW:
        {
            ULONG FaultStatus = ISS & ESR_ELx_DFSC_MASK;
            BOOLEAN IsWrite = (ISS & ESR_ELx_WNR) != 0;
            
            ERR("Data Abort: %s at 0x%016llx (%s)\n",
                IsWrite ? "Write" : "Read", FaultAddr,
                (FaultStatus < ARRAYSIZE(Arm64FaultStatusNames)) ? 
                Arm64FaultStatusNames[FaultStatus] : "Unknown fault");
            break;
        }
        
        case ESR_ELx_EC_IABT_CUR:
        case ESR_ELx_EC_IABT_LOW:
        {
            ULONG FaultStatus = ISS & ESR_ELx_DFSC_MASK;
            
            ERR("Instruction Abort at 0x%016llx (%s)\n", FaultAddr,
                (FaultStatus < ARRAYSIZE(Arm64FaultStatusNames)) ? 
                Arm64FaultStatusNames[FaultStatus] : "Unknown fault");
            break;
        }
        
        case ESR_ELx_EC_PC_ALIGN:
            ERR("PC Alignment fault at 0x%016llx\n", PC);
            break;
            
        case ESR_ELx_EC_SP_ALIGN:
            ERR("SP Alignment fault, SP=0x%016llx\n", Context->SP);
            break;
            
        case ESR_ELx_EC_SVC64:
            ERR("System call (SVC) with immediate 0x%04lx\n", ISS & 0xFFFF);
            /* Could handle system calls here if needed */
            break;
            
        case ESR_ELx_EC_BRK64:
            ERR("Software breakpoint (BRK) with immediate 0x%04lx\n", ISS & 0xFFFF);
            break;
            
        case ESR_ELx_EC_UNKNOWN:
        default:
            ERR("Unknown exception class 0x%02lx, ISS=0x%08lx\n", ExceptionClass, ISS);
            break;
    }
    
    /* Dump register context */
    Arm64DumpContext(Context);
    
    /* For now, halt on any exception */
    ERR("ARM64: Halting due to unhandled exception\n");
    for (;;)
    {
        __asm__ volatile ("wfi");
    }
}

VOID Arm64HandleIrq(PARM64_CONTEXT Context)
{
    /* IRQ handling - for now just acknowledge and return */
    /* In a full implementation, this would: */
    /* 1. Read interrupt controller (GICv3/GICv2) */
    /* 2. Identify interrupt source */
    /* 3. Call appropriate handler */
    /* 4. EOI (End of Interrupt) */
    
    TRACE("ARM64: IRQ received (not implemented)\n");
    
    /* For UEFI boot loader, we typically don't handle IRQs */
    /* Just return and let UEFI handle it */
}

VOID Arm64HandleFiq(PARM64_CONTEXT Context)
{
    /* FIQ handling - fast interrupt request */
    /* Usually used for critical/high-priority interrupts */
    
    TRACE("ARM64: FIQ received (not implemented)\n");
    
    /* FIQs are typically disabled in boot loader */
}

VOID Arm64HandleSerror(PARM64_CONTEXT Context, ULONGLONG Esr)
{
    ERR("ARM64 System Error (SError) Exception\n");
    ERR("ESR: 0x%016llx\n", Esr);
    
    /* SError is an asynchronous external abort */
    /* Usually indicates serious system problems */
    
    /* Dump context for debugging */
    Arm64DumpContext(Context);
    
    ERR("ARM64: System error - halting\n");
    for (;;)
    {
        __asm__ volatile ("wfi");
    }
}

/* Initialize ARM64 exception handling */
VOID Arm64InitializeExceptions(VOID)
{
    TRACE("ARM64: Initializing exception handling\n");
    
    /* Exception vectors are set up in entry.S */
    /* Additional initialization can be done here */
    
    /* Enable floating point if present */
    ULONGLONG cpacr = ARM64_READ_SYSREG(cpacr_el1);
    cpacr |= (3ULL << 20); /* FPEN bits - enable FP/SIMD at EL1 and EL0 */
    ARM64_WRITE_SYSREG(cpacr_el1, cpacr);
    
    /* Enable cycle counter if present */
    ULONGLONG pmcr = ARM64_READ_SYSREG(pmcr_el0);
    pmcr |= (1ULL << 0);   /* Enable all counters */
    pmcr |= (1ULL << 1);   /* Reset all counters */
    pmcr |= (1ULL << 2);   /* Clock divider */
    ARM64_WRITE_SYSREG(pmcr_el0, pmcr);
    
    TRACE("ARM64: Exception handling initialized\n");
}