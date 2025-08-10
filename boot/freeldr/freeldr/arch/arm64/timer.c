/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 Generic Timer support - Based on U-Boot patterns
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <freeldr.h>
#include <arch/arm64/arm64.h>
#include <debug.h>

/* Generic Timer register definitions */
#define CNTKCTL_EL1_EL0PCTEN    (1ULL << 0)  /* EL0 physical counter access */
#define CNTKCTL_EL1_EL0VCTEN    (1ULL << 1)  /* EL0 virtual counter access */
#define CNTKCTL_EL1_EL0VTEN     (1ULL << 8)  /* EL0 virtual timer access */
#define CNTKCTL_EL1_EL0PTEN     (1ULL << 9)  /* EL0 physical timer access */

/* Timer control bits */
#define CNTV_CTL_ENABLE         (1ULL << 0)  /* Timer enable */
#define CNTV_CTL_IMASK          (1ULL << 1)  /* Interrupt mask */
#define CNTV_CTL_ISTATUS        (1ULL << 2)  /* Interrupt status */

#define CNTP_CTL_ENABLE         (1ULL << 0)  /* Timer enable */
#define CNTP_CTL_IMASK          (1ULL << 1)  /* Interrupt mask */
#define CNTP_CTL_ISTATUS        (1ULL << 2)  /* Interrupt status */

/* Global timer state */
static ULONGLONG timer_frequency = 0;
static BOOLEAN timer_initialized = FALSE;

/* Workaround flags for known timer erratas */
static BOOLEAN fsl_erratum_a008585 = FALSE;
static BOOLEAN sunxi_a64_erratum = FALSE;

/* Get timer frequency */
ULONGLONG Arm64GetTimerFrequency(VOID)
{
    ULONGLONG cntfrq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    return cntfrq;
}

/* Read counter with errata workarounds - Based on U-Boot */
static ULONGLONG timer_read_counter_safe(VOID)
{
    ULONGLONG cntpct, temp;
    
    if (fsl_erratum_a008585) {
        /*
         * FSL erratum A-008585: ARM generic timer counter has the
         * potential to contain an erroneous value for a small number
         * of core clock cycles every time the timer value changes.
         * Workaround: Read twice and only return when values match.
         */
        __asm__ volatile("isb");
        __asm__ volatile("mrs %0, cntpct_el0" : "=r" (cntpct));
        __asm__ volatile("mrs %0, cntpct_el0" : "=r" (temp));
        while (temp != cntpct) {
            __asm__ volatile("mrs %0, cntpct_el0" : "=r" (cntpct));
            __asm__ volatile("mrs %0, cntpct_el0" : "=r" (temp));
        }
        return cntpct;
    } else if (sunxi_a64_erratum) {
        /*
         * Sunxi A64 erratum: Sometimes flips lower 11 bits of counter
         * to all 0's or all 1's. Workaround: Check and discard these values.
         */
        do {
            __asm__ volatile("isb");
            __asm__ volatile("mrs %0, cntpct_el0" : "=r" (cntpct));
        } while ((cntpct & 0x7FF) == 0x7FF || (cntpct & 0x7FF) == 0x000);
        return cntpct;
    } else {
        /* Standard read */
        __asm__ volatile("isb");
        __asm__ volatile("mrs %0, cntpct_el0" : "=r" (cntpct));
        return cntpct;
    }
}

/* Initialize ARM64 Generic Timer */
VOID Arm64InitializeTimer(VOID)
{
    ULONGLONG cntkctl, midr;
    
    if (timer_initialized)
        return;
    
    TRACE("ARM64: Initializing Generic Timer\n");
    
    /* Get timer frequency */
    timer_frequency = Arm64GetTimerFrequency();
    if (timer_frequency == 0) {
        WARN("ARM64: Timer frequency is 0, using default 24MHz\n");
        timer_frequency = 24000000;  /* Default fallback */
    }
    
    TRACE("ARM64: Timer frequency: %llu Hz\n", timer_frequency);
    
    /* Check for known timer erratas based on CPU ID */
    midr = ARM64_READ_SYSREG(midr_el1);
    
    /* Check for FSL erratum A-008585 (some ARM Cortex-A57/A53) */
    ULONG implementer = (ULONG)((midr >> 24) & 0xFF);
    ULONG part_num = (ULONG)((midr >> 4) & 0xFFF);
    
    if (implementer == 0x41) {  /* ARM Limited */
        if (part_num == 0xD07 || part_num == 0xD03) {  /* Cortex-A57/A53 */
            /* Could enable workaround based on specific revisions */
            TRACE("ARM64: Detected Cortex-A57/A53, checking for timer erratas\n");
        }
    }
    
    /* Enable timer access for lower exception levels if needed */
    cntkctl = ARM64_READ_SYSREG(cntkctl_el1);
    cntkctl |= CNTKCTL_EL1_EL0PCTEN | CNTKCTL_EL1_EL0VCTEN;
    ARM64_WRITE_SYSREG(cntkctl_el1, cntkctl);
    
    /* Disable virtual timer (we'll use physical timer) */
    ARM64_WRITE_SYSREG(cntv_ctl_el0, CNTV_CTL_IMASK);
    
    /* Disable physical timer initially */
    ARM64_WRITE_SYSREG(cntp_ctl_el0, CNTP_CTL_IMASK);
    
    ARM64_ISB();
    
    timer_initialized = TRUE;
    
    TRACE("ARM64: Generic Timer initialized\n");
}

/* Get current timer counter value */
ULONGLONG Arm64GetTimerCount(VOID)
{
    if (!timer_initialized) {
        Arm64InitializeTimer();
    }
    
    return timer_read_counter_safe();
}

/* Get timer frequency */
ULONGLONG Arm64GetTimerFreq(VOID)
{
    if (!timer_initialized) {
        Arm64InitializeTimer();
    }
    
    return timer_frequency;
}

/* Convert timer ticks to microseconds */
ULONGLONG Arm64TimerTicksToMicroseconds(ULONGLONG ticks)
{
    if (!timer_initialized || timer_frequency == 0) {
        return 0;
    }
    
    /* Avoid overflow in calculation */
    if (ticks > (ULONGLONG_MAX / 1000000ULL)) {
        return (ticks / timer_frequency) * 1000000ULL;
    } else {
        return (ticks * 1000000ULL) / timer_frequency;
    }
}

/* Convert microseconds to timer ticks */
ULONGLONG Arm64MicrosecondsToTimerTicks(ULONGLONG microseconds)
{
    if (!timer_initialized || timer_frequency == 0) {
        return 0;
    }
    
    /* Avoid overflow in calculation */
    if (microseconds > (ULONGLONG_MAX / timer_frequency)) {
        return (microseconds / 1000000ULL) * timer_frequency;
    } else {
        return (microseconds * timer_frequency) / 1000000ULL;
    }
}

/* Delay for specified microseconds */
VOID Arm64DelayMicroseconds(ULONG microseconds)
{
    ULONGLONG start_count, target_ticks;
    
    if (!timer_initialized) {
        Arm64InitializeTimer();
    }
    
    start_count = Arm64GetTimerCount();
    target_ticks = Arm64MicrosecondsToTimerTicks(microseconds);
    
    while ((Arm64GetTimerCount() - start_count) < target_ticks) {
        /* Busy wait */
        __asm__ volatile("nop");
    }
}

/* Get elapsed time since boot in microseconds */
ULONGLONG Arm64GetElapsedMicroseconds(VOID)
{
    ULONGLONG current_count;
    
    if (!timer_initialized) {
        return 0;
    }
    
    current_count = Arm64GetTimerCount();
    return Arm64TimerTicksToMicroseconds(current_count);
}

/* Set up a one-shot timer interrupt (if supported) */
BOOLEAN Arm64SetTimerInterrupt(ULONGLONG microseconds)
{
    ULONGLONG current_count, target_count;
    ULONGLONG cntp_ctl;
    
    if (!timer_initialized) {
        Arm64InitializeTimer();
    }
    
    current_count = Arm64GetTimerCount();
    target_count = current_count + Arm64MicrosecondsToTimerTicks(microseconds);
    
    /* Set compare value */
    ARM64_WRITE_SYSREG(cntp_cval_el0, target_count);
    
    /* Enable timer with interrupt unmasked */
    cntp_ctl = CNTP_CTL_ENABLE;  /* Enable, interrupt unmasked */
    ARM64_WRITE_SYSREG(cntp_ctl_el0, cntp_ctl);
    
    ARM64_ISB();
    
    TRACE("ARM64: Timer interrupt set for %llu us\n", microseconds);
    return TRUE;
}

/* Disable timer interrupt */
VOID Arm64DisableTimerInterrupt(VOID)
{
    if (!timer_initialized) {
        return;
    }
    
    /* Disable and mask timer interrupt */
    ARM64_WRITE_SYSREG(cntp_ctl_el0, CNTP_CTL_IMASK);
    ARM64_ISB();
}

/* Check if timer interrupt is pending */
BOOLEAN Arm64IsTimerInterruptPending(VOID)
{
    ULONGLONG cntp_ctl;
    
    if (!timer_initialized) {
        return FALSE;
    }
    
    cntp_ctl = ARM64_READ_SYSREG(cntp_ctl_el0);
    return (cntp_ctl & CNTP_CTL_ISTATUS) != 0;
}

/* Handle timer interrupt */
VOID Arm64HandleTimerInterrupt(VOID)
{
    if (!timer_initialized) {
        return;
    }
    
    /* Disable timer to clear interrupt */
    ARM64_WRITE_SYSREG(cntp_ctl_el0, CNTP_CTL_IMASK);
    ARM64_ISB();
    
    TRACE("ARM64: Timer interrupt handled\n");
}

/* Get system time in a simple format for boot loader use */
ULONG Arm64GetSystemTime(VOID)
{
    ULONGLONG microseconds;
    
    if (!timer_initialized) {
        Arm64InitializeTimer();
    }
    
    microseconds = Arm64GetElapsedMicroseconds();
    
    /* Return time in milliseconds, truncated to 32-bit */
    return (ULONG)(microseconds / 1000ULL);
}

/* Simple performance counter for profiling */
ULONGLONG Arm64ReadPerformanceCounter(VOID)
{
    return Arm64GetTimerCount();
}

/* Get performance counter frequency */
ULONGLONG Arm64GetPerformanceFrequency(VOID)
{
    return Arm64GetTimerFreq();
}