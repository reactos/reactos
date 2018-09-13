/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    translate.c

Abstract:

    This is the default pnp IRQ translator.

Author:

    Andy Thornton (andrewth) 7-June-97

Environment:

    Kernel Mode Driver.

Notes:

    This should only be temporary and will be replaced by a call into the HAL
    to retrieve its translators.

Revision History:

--*/


#include "ntos.h"
#include "haldisp.h"
#include <wdmguid.h>

//
// Iteration macros
//

//
// Control macro (used like a for loop) which iterates over all entries in
// a standard doubly linked list.  Head is the list head and the entries are of
// type Type.  A member called ListEntry is assumed to be the LIST_ENTRY
// structure linking the entries together.  Current contains a pointer to each
// entry in turn.
//
#define FOR_ALL_IN_LIST(Type, Head, Current)                            \
    for((Current) = CONTAINING_RECORD((Head)->Flink, Type, ListEntry);  \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = CONTAINING_RECORD((Current)->ListEntry.Flink,        \
                                     Type,                              \
                                     ListEntry)                         \
       )
//
// Similar to the above only iteration is over an array of length _Size.
//
#define FOR_ALL_IN_ARRAY(_Array, _Size, _Current)                       \
    for ( (_Current) = (_Array);                                        \
          (_Current) < (_Array) + (_Size);                              \
          (_Current)++ )

//
// As above only iteration begins with the entry _Current
//
#define FOR_REST_IN_ARRAY(_Array, _Size, _Current)                      \
    for ( ;                                                             \
          (_Current) < (_Array) + (_Size);                              \
          (_Current)++ )

#define HAL_IRQ_TRANSLATOR_VERSION 0

NTSTATUS
FstubTranslateResource(
    IN  PVOID Context,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN  RESOURCE_TRANSLATION_DIRECTION Direction,
    IN  ULONG AlternativesCount OPTIONAL,
    IN  IO_RESOURCE_DESCRIPTOR Alternatives[] OPTIONAL,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );

NTSTATUS
FstubTranslateRequirement (
    IN  PVOID Context,
    IN  PIO_RESOURCE_DESCRIPTOR Source,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );

VOID
FstubTranslatorNull(
    IN PVOID Context
    );

extern PDEVICE_OBJECT McaPhysicalBusDevice;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,xHalGetInterruptTranslator)
#pragma alloc_text(PAGE,FstubTranslateResource)
#pragma alloc_text(PAGE,FstubTranslateRequirement)
#pragma alloc_text(PAGE,FstubTranslatorNull)
#endif


NTSTATUS
xHalGetInterruptTranslator(
	IN INTERFACE_TYPE ParentInterfaceType,
	IN ULONG ParentBusNumber,
	IN INTERFACE_TYPE BridgeInterfaceType,
	IN USHORT Size,
	IN USHORT Version,
	OUT PTRANSLATOR_INTERFACE Translator,
	OUT PULONG BridgeBusNumber
	)
/*++

Routine Description:


Arguments:

	ParentInterfaceType - The type of the bus the bridge lives on (normally PCI).

	ParentBusNumber - The number of the bus the bridge lives on.

	ParentSlotNumber - The slot number the bridge lives in (where valid).

	BridgeInterfaceType - The bus type the bridge provides (ie ISA for a PCI-ISA bridge).

	ResourceType - The resource type we want to translate.

	Size - The size of the translator buffer.

	Version - The version of the translator interface requested.

	Translator - Pointer to the buffer where the translator should be returned

	BridgeBusNumber - Pointer to where the bus number of the bridge bus should be returned

Return Value:

    Returns the status of this operation.

--*/
{
    PAGED_CODE();

#if defined(NO_LEGACY_DRIVERS)
    return STATUS_SUCCESS;
}
#else

    UNREFERENCED_PARAMETER(ParentInterfaceType);
    UNREFERENCED_PARAMETER(ParentBusNumber);

    ASSERT(Version == HAL_IRQ_TRANSLATOR_VERSION);
    ASSERT(Size >= sizeof (TRANSLATOR_INTERFACE));

    switch (BridgeInterfaceType) {
    case Eisa:
    case Isa:
    case MicroChannel:
    case InterfaceTypeUndefined:    // special "IDE" cookie

        //
        // Pass back an interface for an IRQ translator.
        //
        RtlZeroMemory(Translator, sizeof (TRANSLATOR_INTERFACE));

        Translator->Size = sizeof (TRANSLATOR_INTERFACE);
        Translator->Version = HAL_IRQ_TRANSLATOR_VERSION;
        Translator->InterfaceReference = &FstubTranslatorNull;
        Translator->InterfaceDereference = &FstubTranslatorNull;
        Translator->TranslateResources = &FstubTranslateResource;
        Translator->TranslateResourceRequirements = &FstubTranslateRequirement;

        if (BridgeInterfaceType == InterfaceTypeUndefined) {
            Translator->Context = (PVOID)Isa;
        } else {
            Translator->Context = (PVOID)BridgeInterfaceType;
        }

        return STATUS_SUCCESS;

    default:
        return STATUS_NOT_IMPLEMENTED;
    }
}

NTSTATUS
FstubTranslateResource(
    IN  PVOID Context,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN  RESOURCE_TRANSLATION_DIRECTION Direction,
    IN  ULONG AlternativesCount OPTIONAL,
    IN  IO_RESOURCE_DESCRIPTOR Alternatives[] OPTIONAL,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    )
{
    NTSTATUS status;
    ULONG affinity, currentVector, translatedVector;
    KIRQL irql;
    PIO_RESOURCE_DESCRIPTOR currentAlternative;

    PAGED_CODE();
    ASSERT(Source->Type == CmResourceTypeInterrupt);

    //
    // Copy unchanged fields
    //

    *Target = *Source;

    switch (Direction) {
    case TranslateChildToParent:

        //
        // Perform the translation - The interrupt source is
        // ISA.
        //

        Target->u.Interrupt.Vector = HalGetInterruptVector(
                                         (INTERFACE_TYPE)(ULONG_PTR)Context,
                                         0,     // assume bus 0 ??? BUGBUG ???
                                         Source->u.Interrupt.Vector,
                                         Source->u.Interrupt.Vector,
                                         &irql,
                                         &affinity
                                         );

        Target->u.Interrupt.Level = irql;
        Target->u.Interrupt.Affinity = affinity;

        status = STATUS_TRANSLATION_COMPLETE;

        break;

    case TranslateParentToChild:

        //
        // Translate each alternative and when we match then use the value we
        // just translated
        //

        FOR_ALL_IN_ARRAY(Alternatives, AlternativesCount, currentAlternative) {

            ASSERT(currentAlternative->Type == CmResourceTypeInterrupt);

            currentVector = currentAlternative->u.Interrupt.MinimumVector;

            while (currentVector <=
                       currentAlternative->u.Interrupt.MaximumVector) {

                translatedVector = HalGetInterruptVector((INTERFACE_TYPE)(ULONG_PTR)Context,
                                                         0,// BUGBUG - assume bus 0
                                                         currentVector,
                                                         currentVector,
                                                         &irql,
                                                         &affinity
                                                        );



                if (translatedVector == Source->u.Interrupt.Vector) {

                    //
                    // We found our vector - fill in the target and return
                    //

                    Target->u.Interrupt.Vector = currentVector;
                    Target->u.Interrupt.Level = Target->u.Interrupt.Vector;
                    Target->u.Interrupt.Affinity = 0xFFFFFFFF;
                    return STATUS_SUCCESS;
                }

                currentVector++;
            }

        }

        status = STATUS_UNSUCCESSFUL;

        break;
    }

    return status;
}
NTSTATUS
FstubTranslateRequirement (
    IN  PVOID Context,
    IN  PIO_RESOURCE_DESCRIPTOR Source,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )
{
    ULONG affinity;
    KIRQL irql;

    PAGED_CODE();
    ASSERT(Source->Type == CmResourceTypeInterrupt);

    *Target = ExAllocatePoolWithTag(PagedPool,
                                    sizeof(IO_RESOURCE_DESCRIPTOR),
                                    'btsF'
                                    );

    if (!*Target) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *TargetCount = 1;

    //
    // Copy unchanged fields
    //

    **Target = *Source;

    (*Target)->u.Interrupt.MinimumVector =
        HalGetInterruptVector(
            (INTERFACE_TYPE)(ULONG_PTR)Context,
            0,     // assume bus 0 ??? BUGBUG ???
            Source->u.Interrupt.MinimumVector,
            Source->u.Interrupt.MinimumVector,
            &irql,
            &affinity
            );


    (*Target)->u.Interrupt.MaximumVector =
        HalGetInterruptVector(
            (INTERFACE_TYPE)(ULONG_PTR)Context,
            0,     // assume bus 0 ??? BUGBUG ???
            Source->u.Interrupt.MaximumVector,
            Source->u.Interrupt.MaximumVector,
            &irql,
            &affinity
            );


    return STATUS_TRANSLATION_COMPLETE;
}

VOID
FstubTranslatorNull(
    IN PVOID Context
    )
{
    PAGED_CODE();
    return;
}
#endif // NO_LEGACY_DRIVERS


