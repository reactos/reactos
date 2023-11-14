/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Extended processor state management
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ntoskrnl.h>
#include <x86x64/Cpuid.h>
#include <x86x64/Msr.h>
#define NDEBUG
#include <debug.h>

// These are not officially documented
#define XSTATE_HDC                          13
#define XSTATE_UINTR                        14
#define XSTATE_LBR                          15
#define XSTATE_MASK_HDC                     (1LL << (XSTATE_HDC))
#define XSTATE_MASK_UINTR                   (1LL << (XSTATE_UINTR))
#define XSTATE_MASK_LBR                     (1LL << (XSTATE_LBR))

#define XSTATE_MASK_SUPERVISOR \
    (XSTATE_MASK_IPT | \
     XSTATE_MASK_PASID | \
     XSTATE_MASK_CET_U | \
     XSTATE_MASK_CET_S | \
     XSTATE_MASK_HDC | \
     XSTATE_MASK_UINTR | \
     XSTATE_MASK_LBR)

// See https://windows-internals.com/cet-on-windows/#3-xstate-configuration

// KeXSavePolicyId

static
VOID
KiGetXStateConfiguration(
    PXSTATE_CONFIGURATION XStateConfig)
{
    ULONG64 SupportedUserMask;
    ULONG64 SupportedSupervisorMask;
    ULONG64 SupportedComponentMask;
    //ULONG64 EnabledMask = XSTATE_MASK_LEGACY;
    ULONG NextUserOffset, NextSupervisorOffset, CurrentOffset;
    //__debugbreak();
    RtlZeroMemory(XStateConfig, sizeof(*XStateConfig));

    /* Check if XSAVE is supported */
    if ((KeFeatureBits & KF_XSTATE) == 0)
    {
        /* XSAVE is not supported */
        XStateConfig->EnabledFeatures = XSTATE_MASK_LEGACY;
        return;
    }

    /* Read CPUID_EXTENDED_STATE main leaf (0x0D, 0x00) */
    CPUID_EXTENDED_STATE_MAIN_LEAF_REGS ExtStateMain;
    __cpuidex(ExtStateMain.AsInt32,
              CPUID_EXTENDED_STATE,
              CPUID_EXTENDED_STATE_MAIN_LEAF);

    /* Get the supported XCR0 bits */
    SupportedUserMask = (ULONG64)ExtStateMain.Edx << 32 |
                        (ULONG64)ExtStateMain.Eax.Uint32;

    /* Mask the allowed components */
    SupportedUserMask &= XSTATE_MASK_ALLOWED & ~XSTATE_MASK_SUPERVISOR;

    /* Read CPUID_EXTENDED_STATE sub-leaf (0x0D, 0x01) */
    CPUID_EXTENDED_STATE_SUB_LEAF_REGS ExtStateSub;
    __cpuidex(ExtStateSub.AsInt32,
              CPUID_EXTENDED_STATE,
              CPUID_EXTENDED_STATE_SUB_LEAF);

    /* Save control flags */
    XStateConfig->OptimizedSave = ExtStateSub.Eax.Bits.XSAVEOPT;
    XStateConfig->CompactionEnabled = ExtStateSub.Eax.Bits.XSAVEC;

    /* Determine supported supervisor features */
    SupportedSupervisorMask = 0;
    if (ExtStateSub.Eax.Bits.XSAVES)
    {
        SupportedSupervisorMask = (ULONG64)ExtStateSub.Edx << 32 |
                                  (ULONG64)ExtStateSub.Ecx.Uint32;
        SupportedSupervisorMask &= XSTATE_MASK_ALLOWED & XSTATE_MASK_SUPERVISOR;
    }

    /* Calculate full mask */
    SupportedComponentMask = SupportedUserMask | SupportedSupervisorMask;

    /* Basic features (always enabled) */
    XStateConfig->EnabledFeatures = XSTATE_MASK_LEGACY;
    XStateConfig->Features[XSTATE_MASK_LEGACY_FLOATING_POINT].Offset = 0;
    XStateConfig->Features[XSTATE_MASK_LEGACY_FLOATING_POINT].Size = FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters);
    XStateConfig->Features[XSTATE_MASK_LEGACY_SSE].Offset = FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters);
    XStateConfig->Features[XSTATE_MASK_LEGACY_SSE].Size = FIELD_SIZE(XSAVE_FORMAT, XmmRegisters);

    /* Other components start after legacy state + header */
    NextUserOffset = NextSupervisorOffset = sizeof(XSAVE_AREA);

    /* Loop all components from 2 up */
    for (ULONG Component = 2; Component < MAXIMUM_XSTATE_FEATURES; Component++)
    {
        ULONG64 ComponentBit = (1ULL << Component);

        /* Query component features */
        CPUID_EXTENDED_STATE_SIZE_OFFSET_REGS ExtStateComponent;
        __cpuidex(ExtStateComponent.AsInt32,
                  CPUID_EXTENDED_STATE,
                  Component);

        /* Save size for all features */
        XStateConfig->AllFeatures[Component] = ExtStateComponent.Size;

        /* Skip components that are not supported */
        if ((ComponentBit & SupportedComponentMask) == 0) continue;

        /* If the offset is 0, this component isn't valid */
        if (ExtStateComponent.Size == 0) __debugbreak();

        /* Update the supervisor component offset */
        if (ExtStateComponent.Ecx.Bits.Aligned)
        {
            NextSupervisorOffset = ALIGN_UP(NextSupervisorOffset, 64);
            XStateConfig->AlignedFeatures |= ComponentBit;
        }
        NextSupervisorOffset += ExtStateComponent.Size;

        /* Skip the rest for supervisor components */
        if ((ComponentBit & SupportedSupervisorMask) != 0) continue;

        /* Save component size */
        XStateConfig->Features[Component].Size = ExtStateComponent.Size;

        /* Check if compaction is enabled */
        if (XStateConfig->CompactionEnabled)
        {
            /* Align the offset, if needed */
            if (ExtStateComponent.Ecx.Bits.Aligned)
            {
                NextUserOffset = ALIGN_UP(NextUserOffset, 64);
                XStateConfig->AlignedFeatures |= ComponentBit;
            }

            /* Save and update the offset */
            XStateConfig->Features[Component].Offset = NextUserOffset;
            NextUserOffset += ExtStateComponent.Size;
        }
        else
        {
            /* Not compacted, we use the offset specified by the CPUID */
            XStateConfig->Features[Component].Offset = ExtStateComponent.Offset;

            /* Update the offset, using the highest value */
            CurrentOffset = XStateConfig->Features[Component].Offset +
                            XStateConfig->Features[Component].Size;
            NextUserOffset = max(NextUserOffset, CurrentOffset);
        }
    }

     /* Save the sizes */
    XStateConfig->Size = NextUserOffset;
    XStateConfig->AllFeatureSize = NextSupervisorOffset;

    /* Save the features to be enabled */
    XStateConfig->EnabledFeatures = SupportedUserMask;
    XStateConfig->EnabledSupervisorFeatures = SupportedSupervisorMask;


    /* Query CPUID again to get the full size based on enabled components */
    __cpuidex(ExtStateMain.AsInt32,
              CPUID_EXTENDED_STATE,
              CPUID_EXTENDED_STATE_MAIN_LEAF);

    /* Get the full size */
    XStateConfig->AllFeatureSize = ExtStateMain.Ecx;
}

static
VOID
ValidateXStateConfig(
    PXSTATE_CONFIGURATION XState)
{
    PXSTATE_CONFIGURATION GlobalXState = &SharedUserData->XState;

    if ((XState->EnabledFeatures != GlobalXState->EnabledFeatures) ||
        (XState->EnabledSupervisorFeatures != GlobalXState->EnabledSupervisorFeatures) ||
        (XState->Size != GlobalXState->Size) ||
        (XState->AllFeatureSize != GlobalXState->AllFeatureSize))
    {
        /* Invalid features */
        KeBugCheck(MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED);
    }

    for (ULONG i = 0; i < MAXIMUM_XSTATE_FEATURES; i++)
    {
        if ((XState->Features[i].Size != GlobalXState->Features[i].Size) ||
            (XState->Features[i].Offset != GlobalXState->Features[i].Offset) ||
            (XState->AllFeatures[i] != GlobalXState->AllFeatures[i]))
        {
            /* Invalid features */
            KeBugCheck(MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED);
        }
    }
}

VOID
NTAPI
KiInitializeXStateConfiguration(
    ULONG ProcessorNumber)
{
    //__debugbreak();
    if (ProcessorNumber == 0)
    {
        KiGetXStateConfiguration(&SharedUserData->XState);
        KeXStateLength = SharedUserData->XState.AllFeatureSize;
    }
    else
    {
        XSTATE_CONFIGURATION XState;

        KiGetXStateConfiguration(&XState);
        ValidateXStateConfig(&XState);
    }

    /* Enable the user mode components in XCR0 */
    ULONG64 oldMask = _xgetbv(0);
    _xsetbv(0, SharedUserData->XState.EnabledFeatures);
    (void)oldMask;

    /* If we have supervisor components, enable them in IA32_XSS */
    if (SharedUserData->XState.EnabledSupervisorFeatures != 0)
    {
        ULONG64 oldXss = __readmsr(MSR_IA32_XSS);
        __writemsr(MSR_IA32_XSS, SharedUserData->XState.EnabledSupervisorFeatures);
        (void)oldXss;
    }


}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
_Ret_range_(<=, 0)
_When_(return==0, _Kernel_float_saved_)
NTKERNELAPI
NTSTATUS
NTAPI
KeSaveExtendedProcessorState(
  _In_ ULONG64 Mask,
  _Out_ _Requires_lock_not_held_(*_Curr_)
  _When_(return==0, _Acquires_lock_(*_Curr_))
    PXSTATE_SAVE XStateSave)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();
    ULONG64 EnabledMask = SharedUserData->XState.EnabledFeatures;
    ULONG Length;

    /* Validate the mask */
    EnabledMask = SharedUserData->XState.EnabledFeatures |
                  SharedUserData->XState.EnabledSupervisorFeatures;
    if ((Mask & ~EnabledMask) != 0)
    {
        /* Invalid mask */
        return STATUS_INVALID_PARAMETER;
    }

    Length = (ULONG)KeXStateLength; // TODO: calculate needed size

    /* Initialize the structure */
    XStateSave->XStateContext.Mask = Mask;
    XStateSave->XStateContext.Length = Length;
    XStateSave->Thread = CurrentThread;
#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)
    XStateSave->Level = CurrentThread->XStateSave ?
        CurrentThread->XStateSave->Level + 1 : 1;
    XStateSave->Prev = CurrentThread->XStateSave;
#else
    XStateSave->Level = 1;
    XStateSave->Prev = NULL;
#endif

    /* Check if we already saved */
    if (XStateSave->Prev != NULL)
    {
        /* We already saved once, we need to allocate a new buffer */
        PVOID Buffer = ExAllocatePoolWithTag(PagedPool, Length + 63, 'XsSK');
        if (Buffer == NULL)
        {
            /* Failed to allocate */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        XStateSave->XStateContext.Buffer = Buffer;
        XStateSave->XStateContext.Area = ALIGN_UP_POINTER_BY(Buffer, 64);
    }
    else
    {
        /* We didn't save yet, we can use the reserved stack buffer */
        XStateSave->XStateContext.Buffer = NULL;
        XStateSave->XStateContext.Area = (PXSAVE_AREA)CurrentThread->StateSaveArea;
    }

    /* Save the state */
    KiSaveXState(&XStateSave->XStateContext.Area, Mask);

    /* Link this up to the thread */
    XStateSave->Thread = CurrentThread;
#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)
    XStateSave->Level = CurrentThread->XStateSave ?
        CurrentThread->XStateSave->Level + 1 : 1;
    XStateSave->Prev = CurrentThread->XStateSave;
    CurrentThread->XStateSave = XStateSave;
#endif

    return STATUS_SUCCESS;
}

_Kernel_float_restored_
NTKERNELAPI
VOID
NTAPI
KeRestoreExtendedProcessorState(
  _In_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_)
    PXSTATE_SAVE XStateSave)
{
    /* Restore the state */
    KiRestoreXState(XStateSave->XStateContext.Area,
                    XStateSave->XStateContext.Mask);

    /* If we allocated a buffer, free it */
    if (XStateSave->XStateContext.Buffer != NULL)
    {
        ExFreePoolWithTag(XStateSave->XStateContext.Buffer, 'XsSK');
    }

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)
    /* Unlink the XStateSave from the thread */
    PKTHREAD CurrentThread = KeGetCurrentThread();
    CurrentThread->XStateSave = XStateSave->Prev;
#endif
}
