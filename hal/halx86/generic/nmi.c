/*
 * PROJECT:         ReactOS Hardware Abstraction Layer
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         NMI handling
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include <drivers/bootvid/display.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS  *******************************************************************/

BOOLEAN HalpNMIInProgress;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalHandleNMI(
    IN PVOID NmiInfo)
{
    UNREFERENCED_PARAMETER(NmiInfo);
#ifndef _MINIHAL_
    SYSTEM_CONTROL_PORT_B_REGISTER SystemControl;

    /* Don't recurse */
    if (HalpNMIInProgress++)
        ERROR_DBGBREAK();

    /* Get NMI reason from hardware */
#if defined(SARCH_PC98)
    SystemControl.Bits = __inbyte(PPI_IO_i_PORT_B);
#else
    SystemControl.Bits = __inbyte(SYSTEM_CONTROL_PORT_B);
#endif

    /* Switch to boot video */
    if (InbvIsBootDriverInstalled())
    {
        /* Acquire ownership */
        InbvAcquireDisplayOwnership();
        InbvResetDisplay();

        /* Fill the screen */
        InbvSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_RED);
        InbvSetScrollRegion(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

        /* Enable text */
        InbvSetTextColor(BV_COLOR_WHITE);
        InbvInstallDisplayStringFilter(NULL);
        InbvEnableDisplayString(TRUE);
    }

    /* Display NMI failure string */
    InbvDisplayString("\r\n*** Hardware Malfunction\r\n\r\n");
    InbvDisplayString("Call your hardware vendor for support\r\n\r\n");

#if defined(SARCH_PC98)
    /* Check for parity error */
    if (SystemControl.MemoryParityCheck)
    {
        InbvDisplayString("NMI: Parity Check / Memory Parity Error\r\n");
    }
    if (SystemControl.ExtendedMemoryParityCheck)
    {
        InbvDisplayString("NMI: Parity Check / Extended Memory Parity Error\r\n");
    }
#else
    /* Check for parity error */
    if (SystemControl.ParityCheck)
    {
        InbvDisplayString("NMI: Parity Check / Memory Parity Error\r\n");
    }

    /* Check for I/O failure */
    if (SystemControl.ChannelCheck)
    {
        InbvDisplayString("NMI: Channel Check / IOCHK\r\n");
    }
#endif

    /* Check for EISA systems */
    if (HalpBusType == MACHINE_TYPE_EISA)
    {
        /* FIXME: Not supported */
        UNIMPLEMENTED;
    }

    /* Halt the system */
    InbvDisplayString("\r\n*** The system has halted ***\r\n");

    /* Enter the debugger if possible */
    KiBugCheckData[0] = (ULONG_PTR)KeServiceDescriptorTable; /* NMI Corruption? */
    //if (!(KdDebuggerNotPresent) && (KdDebuggerEnabled)) KeEnterKernelDebugger();
#endif /* !_MINIHAL_ */

    /* Freeze the system */
    while (TRUE)
        NOTHING;
}
