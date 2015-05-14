/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/ctxswtch.s
 * PURPOSE:         Context Switch and Idle Thread on ARM
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 *                  ReactOS Portable Systems Group
 */

#include <ksarm.h>

    IMPORT KeLowerIrql

    TEXTAREA

/*!
 * \name KiSwapContextInternal
 *
 * \brief
 *     The KiSwapContextInternal routine switches context to another thread.
 *
 * \param r0
 *     Pointer to the KTHREAD to which the caller wishes to switch to.
 *
 * \param r1
 *     Pointer to the KTHREAD to which the caller wishes to switch from.
 *
 * \param r2
 *     APC bypass
 *
 * \return
 *     None.
 *
 * \remarks
 *     ...
 *
 *--*/
    NESTED_ENTRY KiSwapContextInternal

    /* Push a KSWITCH_FRAME on the stack */
    stmdb sp!,{r2,r3,r11,lr} // FIXME: what is the 2nd field?
    // PROLOG_PUSH {r2,r3,r11,lr}

    PROLOG_END KiSwapContextInternal

    /* Save kernel stack of old thread */
    str sp, [r1, #ThKernelStack]

    /* Save new thread in R11 */
    mov r11, r0

    //bl KiSwapContextSuspend
    __debugbreak

    /* Load stack of new thread */
    ldr sp, [r11, #ThKernelStack]

    /* Reload APC bypass */
    ldr r2, [sp, #SwApcBypass]

    //bl KiSwapContextResume
    __debugbreak

    /* Restore R2, R11 and return */
    ldmia sp!,{r2,r3,r11,pc}

    NESTED_END KiSwapContextInternal


/*!
 * KiSwapContext
 *
 * \brief
 *     The KiSwapContext routine switches context to another thread.
 *
 * BOOLEAN
 * KiSwapContext(
 *     _In_ KIRQL WaitIrql,
 *     _Inout_ PKTHREAD CurrentThread);
 *
 * \param WaitIrql <r0>
 *     ...
 *
 * \param CurrentThread <r1>
 *     Pointer to the KTHREAD of the current thread.
 *
 * \return
 *     The WaitStatus of the Target Thread.
 *
 * \remarks
 *     This is a wrapper around KiSwapContextInternal which will save all the
 *     non-volatile registers so that the Internal function can use all of
 *     them. It will also save the old current thread and set the new one.
 *
 *     The calling thread does not return after KiSwapContextInternal until
 *     another thread switches to it.
 *
 *--*/
    NESTED_ENTRY KiSwapContext

    /* Push non-volatiles and return address on the stack */
    stmdb sp!,{r4,r5,r6,r7,r8,r9,r10,r11,lr}
    // PROLOG_PUSH {r4,r5,r6,r7,r8,r9,r10,r11,lr}

    PROLOG_END KiSwapContext

    /* Do the swap with the registers correctly setup */
    mov32 r0, 0xFFDFF000 // FIXME: properly load the PCR into r0 (PCR should be in CP15, c3, TPIDRPRW)
    ldr r0, [r0, #PcCurrentThread] /* Pointer to the new thread */
    bl KiSwapContextInternal

    /* Restore non-volatiles and return */
    ldmia sp!,{r4,r5,r6,r7,r8,r9,r10,r11,pc}

    NESTED_END KiSwapContext



    NESTED_ENTRY KiThreadStartup
    PROLOG_END KiThreadStartup

    /* Lower IRQL to APC_LEVEL */
    mov a1, #1
    bl KeLowerIrql

    /* Set the start address and startup context */
    mov a1, r6
    mov a2, r5
    blx r7

    /* The function must not return! */
    __assertfail

    NESTED_END KiThreadStartup


    NESTED_ENTRY KiSwitchThreads
    PROLOG_END KiSwitchThreads

	// UNIMPLEMENTED!
	__debugbreak

    NESTED_END KiSwitchThreads

    END
/* EOF */
