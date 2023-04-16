/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/processor.c
 * PURPOSE:         HAL Processor Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KAFFINITY HalpActiveProcessors;
KAFFINITY HalpDefaultInterruptAffinity;
BOOLEAN HalpProcessorIdentified;
BOOLEAN HalpTestCleanSupported;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
HalpIdentifyProcessor(VOID)
{
    ARM_ID_CODE_REGISTER IdRegister;

    /* Don't do it again */
    HalpProcessorIdentified = TRUE;

    // fixfix: Use Pcr->ProcessorId

    /* Read the ID Code */
    IdRegister = KeArmIdCodeRegisterGet();

    /* Architecture "6" CPUs support test-and-clean (926EJ-S and 1026EJ-S) */
    HalpTestCleanSupported = (IdRegister.Architecture == 6);
}

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalInitializeProcessor(IN ULONG ProcessorNumber,
                       IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Do nothing */
    for(;;)
    {

    }
    return;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalAllProcessorsStarted(VOID)
{
    /* Do nothing */
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalStartNextProcessor(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                      IN PKPROCESSOR_STATE ProcessorState)
{
    /* Ready to start */
    return FALSE;
}

/*
 * @implemented
 */
VOID
NTAPI
HalProcessorIdle(VOID)
{
    /* Enable interrupts and halt the processor */
    _enable();
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @implemented
 */
VOID
NTAPI
HalRequestIpi(KAFFINITY TargetProcessors)
{
    /* Not implemented on UP */
    UNIMPLEMENTED;
    while (TRUE);
}
#define QEMUUART 0x09000000
volatile unsigned int * UART0DR = (unsigned int *) QEMUUART;

VOID
ARMWriteToUART(UCHAR Data)
{
    *UART0DR = Data;
}

ULONG
DbgPrintEarly(const char *fmt, ...)
{
    va_list args;
    unsigned int i;
    char Buffer[1024];
    PCHAR String = Buffer;

    va_start(args, fmt);
    i = vsprintf(Buffer, fmt, args);
    va_end(args);

    /* Output the message */
    while (*String != 0)
    {
        if (*String == '\n')
        {

            ARMWriteToUART('\r');
        }
        ARMWriteToUART(*String);
        String++;
    }

    return STATUS_SUCCESS;
}



VOID v7_flush_dcache_all(VOID);
/*
 * @implemented
 */
VOID
HalSweepDcache(VOID)
{
    DbgPrintEarly("HalSweepDcache: try dcache sweep");
    /*
     * We get called very early on, before HalInitSystem or any of the Hal*
     * processor routines, so we need to figure out what CPU we're on.
     */
    if (!HalpProcessorIdentified) HalpIdentifyProcessor();

    /* We need to do it it by set/way. For now always call ARMv7 function */
    v7_flush_dcache_all();
}

/*
 * @implemented
 */
VOID
HalSweepIcache(VOID)
{

    DbgPrintEarly("HAL JUMP");
    /* All ARM cores support the same Icache flush command */
   // KeArmFlushIcache();
}

/* EOF */
