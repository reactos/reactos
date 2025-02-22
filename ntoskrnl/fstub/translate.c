/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/fstub/translate.c
* PURPOSE:         Interrupt Translator Routines
* PROGRAMMERS:     Pierre Schweitzer (pierre.schweitzer@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
FstubTranslatorNull(PVOID Context)
{
    PAGED_CODE();

    /* Do nothing */
    return;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FstubTranslateResource(IN OUT PVOID Context OPTIONAL,
                       IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
                       IN RESOURCE_TRANSLATION_DIRECTION Direction,
                       IN ULONG AlternativesCount OPTIONAL,
                       IN IO_RESOURCE_DESCRIPTOR Alternatives[],
                       IN PDEVICE_OBJECT PhysicalDeviceObject,
                       OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target)
{
    KIRQL Irql;
    KAFFINITY Affinity;
    ULONG MinimumVector, Vector, k;
    PIO_RESOURCE_DESCRIPTOR Alternative;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PAGED_CODE();

    ASSERT(Source->Type == CmResourceTypeInterrupt);

    /* Copy common information */
    Target->Type = Source->Type;
    Target->ShareDisposition = Source->ShareDisposition;
    Target->Flags = Source->Flags;

    if (Direction == TranslateChildToParent)
    {
        /* Get IRQL, affinity & system vector for the device vector */
        Target->u.Interrupt.Vector = HalGetInterruptVector((INTERFACE_TYPE)Context, 0,
                                                           Source->u.Interrupt.Vector,
                                                           Source->u.Interrupt.Vector,
                                                           &Irql, &Affinity);
        Target->u.Interrupt.Level = Irql;
        Target->u.Interrupt.Affinity = Affinity;
        Status = STATUS_TRANSLATION_COMPLETE;
    }
    else if (Direction == TranslateParentToChild)
    {
        /* Browse all the resources */
        for (k = 0; k < AlternativesCount; k++)
        {
            Alternative = &(Alternatives[k]);

            ASSERT(Alternative->Type == CmResourceTypeInterrupt);

            /* Try to find the device vector, proceeding by trial & error
             * We try a vector, and translate it
             */
            MinimumVector = Alternative->u.Interrupt.MinimumVector;
            while (MinimumVector <= Alternative->u.Interrupt.MaximumVector)
            {
                /* Translate the vector */
                Vector = HalGetInterruptVector((INTERFACE_TYPE)Context, 0,
                                               MinimumVector,
                                               MinimumVector,
                                               &Irql, &Affinity);

                /* If the translated vector is matching the given translated vector */
                if (Vector == Source->u.Interrupt.Vector)
                {
                    /* We are done, send back device vector */
                    Target->u.Interrupt.Affinity = -1;
                    Target->u.Interrupt.Vector = MinimumVector;
                    Target->u.Interrupt.Level = MinimumVector;

                    return STATUS_SUCCESS;
                }

                MinimumVector++;
            }
        }
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FstubTranslateRequirement(IN OUT PVOID Context OPTIONAL,
                          IN PIO_RESOURCE_DESCRIPTOR Source,
                          IN PDEVICE_OBJECT PhysicalDeviceObject,
                          OUT PULONG TargetCount,
                          OUT PIO_RESOURCE_DESCRIPTOR *Target)
{
    KIRQL Irql;
    KAFFINITY Affinity;
    PAGED_CODE();

    ASSERT(Source->Type == CmResourceTypeInterrupt);

    /* Allocate output buffer */
    *Target = ExAllocatePoolWithTag(PagedPool, sizeof(IO_RESOURCE_DESCRIPTOR), 'btsF');
    if (!*Target)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Zero & set out count to 1 */
    RtlZeroMemory(*Target, sizeof(IO_RESOURCE_DESCRIPTOR));
    *TargetCount = 1;

    /* Translate minimum interrupt vector */
    (*Target)->u.Interrupt.MinimumVector = HalGetInterruptVector((INTERFACE_TYPE)Context, 0,
                                                                 Source->u.Interrupt.MinimumVector,
                                                                 Source->u.Interrupt.MinimumVector,
                                                                 &Irql, &Affinity);

    /* Translate maximum interrupt vector */
    (*Target)->u.Interrupt.MaximumVector = HalGetInterruptVector((INTERFACE_TYPE)Context, 0,
                                                                 Source->u.Interrupt.MaximumVector,
                                                                 Source->u.Interrupt.MaximumVector,
                                                                 &Irql, &Affinity);

    return STATUS_TRANSLATION_COMPLETE;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
xHalGetInterruptTranslator(IN INTERFACE_TYPE ParentInterfaceType,
                           IN ULONG ParentBusNumber,
                           IN INTERFACE_TYPE BridgeInterfaceType,
                           IN USHORT Size,
                           IN USHORT Version,
                           OUT PTRANSLATOR_INTERFACE Translator,
                           OUT PULONG BridgeBusNumber)
{
    PAGED_CODE();

    ASSERT(Version == HAL_IRQ_TRANSLATOR_VERSION);
    ASSERT(Size >= sizeof(TRANSLATOR_INTERFACE));

    /* Only (E)ISA interfaces are supported */
    if (BridgeInterfaceType == Internal || BridgeInterfaceType >= MicroChannel)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Fill in output struct */
    Translator->Size = sizeof(TRANSLATOR_INTERFACE);
    Translator->Version = HAL_IRQ_TRANSLATOR_VERSION;
    /* In case caller set interface to undefined, faulty it to ISA */
    Translator->Context = UlongToPtr((BridgeInterfaceType == InterfaceTypeUndefined) ? Isa : BridgeInterfaceType);
    Translator->InterfaceReference = FstubTranslatorNull;
    Translator->InterfaceDereference = FstubTranslatorNull;
    Translator->TranslateResources = FstubTranslateResource;
    Translator->TranslateResourceRequirements = FstubTranslateRequirement;

    return STATUS_SUCCESS;
}
