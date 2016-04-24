/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/dma.c
 * PURPOSE:         DMA Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @unimplemented
 */
PADAPTER_OBJECT
NTAPI
HalGetAdapter(IN PDEVICE_DESCRIPTION DeviceDescription,
              OUT PULONG NumberOfMapRegisters)
{
    UNIMPLEMENTED;
    while (TRUE);
    return NULL;
}

/*
 * @unimplemented
 */
VOID
NTAPI
HalPutDmaAdapter(IN PADAPTER_OBJECT AdapterObject)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @unimplemented
 */
PVOID
NTAPI
HalAllocateCommonBuffer(IN PADAPTER_OBJECT AdapterObject,
                        IN ULONG Length,
                        IN PPHYSICAL_ADDRESS LogicalAddress,
                        IN BOOLEAN CacheEnabled)
{
    UNIMPLEMENTED;
    while (TRUE);
    return NULL;
}

/*
 * @unimplemented
 */
VOID
NTAPI
HalFreeCommonBuffer(IN PADAPTER_OBJECT AdapterObject,
                    IN ULONG Length,
                    IN PHYSICAL_ADDRESS LogicalAddress,
                    IN PVOID VirtualAddress,
                    IN BOOLEAN CacheEnabled)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @unimplemented
 */
ULONG
NTAPI
HalReadDmaCounter(IN PADAPTER_OBJECT AdapterObject)
{
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
HalAllocateAdapterChannel(IN PADAPTER_OBJECT AdapterObject,
                          IN PWAIT_CONTEXT_BLOCK WaitContextBlock,
                          IN ULONG NumberOfMapRegisters,
                          IN PDRIVER_CONTROL ExecutionRoutine)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
VOID
NTAPI
IoFreeAdapterChannel(IN PADAPTER_OBJECT AdapterObject)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @unimplemented
 */
VOID
NTAPI
IoFreeMapRegisters(IN PADAPTER_OBJECT AdapterObject,
                   IN PVOID MapRegisterBase,
                   IN ULONG NumberOfMapRegisters)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
IoFlushAdapterBuffers(IN PADAPTER_OBJECT AdapterObject,
                      IN PMDL Mdl,
                      IN PVOID MapRegisterBase,
                      IN PVOID CurrentVa,
                      IN ULONG Length,
                      IN BOOLEAN WriteToDevice)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

/*
 * @unimplemented
 */
PHYSICAL_ADDRESS
NTAPI
IoMapTransfer(IN PADAPTER_OBJECT AdapterObject,
              IN PMDL Mdl,
              IN PVOID MapRegisterBase,
              IN PVOID CurrentVa,
              IN OUT PULONG Length,
              IN BOOLEAN WriteToDevice)
{
    PHYSICAL_ADDRESS Address;

    UNIMPLEMENTED;
    while (TRUE);
    
    Address.QuadPart = 0;
    return Address;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
HalFlushCommonBuffer(IN PADAPTER_OBJECT AdapterObject,
                     IN ULONG Length,
                     IN PHYSICAL_ADDRESS LogicalAddress,
                     IN PVOID VirtualAddress)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
HalAllocateCrashDumpRegisters(IN PADAPTER_OBJECT AdapterObject,
                              IN OUT PULONG NumberOfMapRegisters)
{
    UNIMPLEMENTED;
    while (TRUE);
    return NULL;
}

/* EOF */
