/*
 * PROJECT:         ReactOS Hardware Abstraction Layer (HAL)
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halx86/generic/misc.c
 * PURPOSE:         NMI, I/O Mapping and x86 Subs
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include <drivers/bootvid/display.h>

#define NDEBUG
#include <debug.h>

#if defined(ALLOC_PRAGMA) && !defined(_MINIHAL_)
#pragma alloc_text(INIT, HalpMarkAcpiHal)
#pragma alloc_text(INIT, HalpReportSerialNumber)
#endif

/* GLOBALS  *******************************************************************/

BOOLEAN HalpNMIInProgress;

UCHAR HalpSerialLen;
CHAR HalpSerialNumber[31];

/* PRIVATE FUNCTIONS **********************************************************/

#ifndef _MINIHAL_
INIT_FUNCTION
VOID
NTAPI
HalpReportSerialNumber(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING KeyString;
    HANDLE Handle;

    /* Make sure there is a serial number */
    if (!HalpSerialLen) return;

    /* Open the system key */
    RtlInitUnicodeString(&KeyString, L"\\Registry\\Machine\\Hardware\\Description\\System");
    Status = HalpOpenRegistryKey(&Handle, 0, &KeyString, KEY_ALL_ACCESS, FALSE);
    if (NT_SUCCESS(Status))
    {
        /* Add the serial number */
        RtlInitUnicodeString(&KeyString, L"Serial Number");
        ZwSetValueKey(Handle,
                      &KeyString,
                      0,
                      REG_BINARY,
                      HalpSerialNumber,
                      HalpSerialLen);

        /* Close the handle */
        ZwClose(Handle);
    }
}

INIT_FUNCTION
NTSTATUS
NTAPI
HalpMarkAcpiHal(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING KeyString;
    HANDLE KeyHandle;
    HANDLE Handle;
    ULONG Value = HalDisableFirmwareMapper ? 1 : 0;

    /* Open the control set key */
    RtlInitUnicodeString(&KeyString,
                         L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET");
    Status = HalpOpenRegistryKey(&Handle, 0, &KeyString, KEY_ALL_ACCESS, FALSE);
    if (NT_SUCCESS(Status))
    {
        /* Open the PNP key */
        RtlInitUnicodeString(&KeyString, L"Control\\Pnp");
        Status = HalpOpenRegistryKey(&KeyHandle,
                                     Handle,
                                     &KeyString,
                                     KEY_ALL_ACCESS,
                                     TRUE);
        /* Close root key */
        ZwClose(Handle);

        /* Check if PNP BIOS key exists */
        if (NT_SUCCESS(Status))
        {
            /* Set the disable value to false -- we need the mapper */
            RtlInitUnicodeString(&KeyString, L"DisableFirmwareMapper");
            Status = ZwSetValueKey(KeyHandle,
                                   &KeyString,
                                   0,
                                   REG_DWORD,
                                   &Value,
                                   sizeof(Value));

            /* Close subkey */
            ZwClose(KeyHandle);
        }
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
HalpOpenRegistryKey(IN PHANDLE KeyHandle,
                    IN HANDLE RootKey,
                    IN PUNICODE_STRING KeyName,
                    IN ACCESS_MASK DesiredAccess,
                    IN BOOLEAN Create)
{
    NTSTATUS Status;
    ULONG Disposition;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Setup the attributes we received */
    InitializeObjectAttributes(&ObjectAttributes,
                               KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);

    /* What to do? */
    if ( Create )
    {
        /* Create the key */
        Status = ZwCreateKey(KeyHandle,
                             DesiredAccess,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             &Disposition);
    }
    else
    {
        /* Open the key */
        Status = ZwOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);
    }

    /* We're done */
    return Status;
}
#endif

VOID
NTAPI
HalpCheckPowerButton(VOID)
{
    //
    // Nothing to do on non-ACPI
    //
    return;
}

VOID
NTAPI
HalpFlushTLB(VOID)
{
    ULONG_PTR Flags, Cr4;
    INT CpuInfo[4];
    ULONG_PTR PageDirectory;

    //
    // Disable interrupts
    //
    Flags = __readeflags();
    _disable();

    //
    // Get page table directory base
    //
    PageDirectory = __readcr3();

    //
    // Check for CPUID support
    //
    if (KeGetCurrentPrcb()->CpuID)
    {
        //
        // Check for global bit in CPU features
        //
        __cpuid(CpuInfo, 1);
        if (CpuInfo[3] & 0x2000)
        {
            //
            // Get current CR4 value
            //
            Cr4 = __readcr4();

            //
            // Disable global bit
            //
            __writecr4(Cr4 & ~CR4_PGE);

            //
            // Flush TLB and re-enable global bit
            //
            __writecr3(PageDirectory);
            __writecr4(Cr4);

            //
            // Restore interrupts
            //
            __writeeflags(Flags);
            return;
        }
    }

    //
    // Legacy: just flush TLB
    //
    __writecr3(PageDirectory);
    __writeeflags(Flags);
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalHandleNMI(IN PVOID NmiInfo)
{
#ifndef _MINIHAL_
    SYSTEM_CONTROL_PORT_B_REGISTER SystemControl;

    //
    // Don't recurse
    //
    if (HalpNMIInProgress++) ERROR_DBGBREAK();

    //
    // Read the system control register B
    //
    SystemControl.Bits = __inbyte(SYSTEM_CONTROL_PORT_B);

    //
    // Switch to boot video
    //
    if (InbvIsBootDriverInstalled())
    {
        //
        // Acquire ownership
        //
        InbvAcquireDisplayOwnership();
        InbvResetDisplay();

        //
        // Fill the screen
        //
        InbvSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_RED);
        InbvSetScrollRegion(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

        //
        // Enable text
        //
        InbvSetTextColor(BV_COLOR_WHITE);
        InbvInstallDisplayStringFilter(NULL);
        InbvEnableDisplayString(TRUE);
    }

    //
    // Display NMI failure string
    //
    InbvDisplayString("\r\n*** Hardware Malfunction\r\n\r\n");
    InbvDisplayString("Call your hardware vendor for support\r\n\r\n");

    //
    // Check for parity error
    //
    if (SystemControl.ParityCheck)
    {
        //
        // Display message
        //
        InbvDisplayString("NMI: Parity Check / Memory Parity Error\r\n");
    }

    //
    // Check for I/O failure
    //
    if (SystemControl.ChannelCheck)
    {
        //
        // Display message
        //
        InbvDisplayString("NMI: Channel Check / IOCHK\r\n");
    }

    //
    // Check for EISA systems
    //
    if (HalpBusType == MACHINE_TYPE_EISA)
    {
        //
        // FIXME: Not supported
        //
        UNIMPLEMENTED;
    }

    //
    // Halt the system
    //
    InbvDisplayString("\r\n*** The system has halted ***\r\n");


    //
    // Enter the debugger if possible
    //
    KiBugCheckData[0] = (ULONG_PTR)KeServiceDescriptorTable; /* NMI Corruption? */
    //if (!(KdDebuggerNotPresent) && (KdDebuggerEnabled)) KeEnterKernelDebugger();
#endif
    //
    // Freeze the system
    //
    while (TRUE);
}

/*
 * @implemented
 */
UCHAR
FASTCALL
HalSystemVectorDispatchEntry(IN ULONG Vector,
                             OUT PKINTERRUPT_ROUTINE **FlatDispatch,
                             OUT PKINTERRUPT_ROUTINE *NoConnection)
{
    //
    // Not implemented on x86
    //
    return 0;
}

/*
 * @implemented
 */
VOID
NTAPI
KeFlushWriteBuffer(VOID)
{
    //
    // Not implemented on x86
    //
    return;
}

#ifdef _M_IX86
/* x86 fastcall wrappers */

#undef KeRaiseIrql
/*
 * @implemented
 */
VOID
NTAPI
KeRaiseIrql(KIRQL NewIrql,
            PKIRQL OldIrql)
{
    /* Call the fastcall function */
    *OldIrql = KfRaiseIrql(NewIrql);
}

#undef KeLowerIrql
/*
 * @implemented
 */
VOID
NTAPI
KeLowerIrql(KIRQL NewIrql)
{
    /* Call the fastcall function */
    KfLowerIrql(NewIrql);
}

#undef KeAcquireSpinLock
/*
 * @implemented
 */
VOID
NTAPI
KeAcquireSpinLock(PKSPIN_LOCK SpinLock,
                  PKIRQL OldIrql)
{
    /* Call the fastcall function */
    *OldIrql = KfAcquireSpinLock(SpinLock);
}

#undef KeReleaseSpinLock
/*
 * @implemented
 */
VOID
NTAPI
KeReleaseSpinLock(PKSPIN_LOCK SpinLock,
                  KIRQL NewIrql)
{
    /* Call the fastcall function */
    KfReleaseSpinLock(SpinLock, NewIrql);
}

#endif

