/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Extended processor state management
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
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

/*!
 *   \brief Determines the extended state configuration for the current processor
 *
 *   \param XStateConfig - Pointer to a XSTATE_CONFIGURATION structure that receives the configuration
 *
 *   \see https://windows-internals.com/cet-on-windows/#3-xstate-configuration
 */
CODE_SEG("INIT")
static
VOID
KiGetXStateConfiguration(
    _Out_ PXSTATE_CONFIGURATION XStateConfig)
{
    ULONG64 SupportedUserMask;
    ULONG64 SupportedSupervisorMask;
    ULONG64 SupportedComponentMask;
    ULONG NextUserOffset, NextSupervisorOffset, NextOffset;

    RtlZeroMemory(XStateConfig, sizeof(*XStateConfig));

    /* Check if XSAVE is supported on this processor */
    if ((KeGetCurrentPrcb()->FeatureBits & KF_XSTATE) == 0)
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
        if (ExtStateComponent.Size == 0) continue;

        /* Check if compaction is enabled */
        if (XStateConfig->CompactionEnabled)
        {
            /* Align the offsets, if needed */
            if (ExtStateComponent.Ecx.Bits.Aligned)
            {
                XStateConfig->AlignedFeatures |= ComponentBit;
                NextSupervisorOffset = ALIGN_UP(NextSupervisorOffset, 64);
                if ((ComponentBit & SupportedSupervisorMask) == 0)
                {
                    NextUserOffset = ALIGN_UP(NextUserOffset, 64);
                }
            }

            /* Update the supervisor offset */
            NextSupervisorOffset += ExtStateComponent.Size;

            /* For user components save and update the offset and size */
            if ((ComponentBit & SupportedSupervisorMask) == 0)
            {
                XStateConfig->Features[Component].Offset = NextUserOffset;
                XStateConfig->Features[Component].Size = ExtStateComponent.Size;
                NextUserOffset += ExtStateComponent.Size;
            }
        }
        else
        {
            /* Not compacted, use the offset and size specified by the CPUID */
            NextOffset = ExtStateComponent.Offset + ExtStateComponent.Size;
            NextSupervisorOffset = max(NextSupervisorOffset, NextOffset);

            /* For user components save and update the offset and size */
            if ((ComponentBit & SupportedSupervisorMask) == 0)
            {
                XStateConfig->Features[Component].Offset = ExtStateComponent.Offset;
                XStateConfig->Features[Component].Size = ExtStateComponent.Size;
                NextUserOffset = max(NextUserOffset, NextOffset);
            }
        }
    }

    /* Save the features to be enabled */
    XStateConfig->EnabledFeatures = SupportedUserMask;
    XStateConfig->EnabledSupervisorFeatures = SupportedSupervisorMask;

     /* Save the calculated sizes */
    XStateConfig->Size = NextUserOffset;
    XStateConfig->AllFeatureSize = NextSupervisorOffset;
    ASSERT(XStateConfig->AllFeatureSize >= XStateConfig->Size);
}

/*!
 *   \brief Validates the provided extended state configuration against the global one
 *
 *   \param XStateConfig - Pointer to a XSTATE_CONFIGURATION structure containing the configuration
 */
CODE_SEG("INIT")
static
VOID
ValidateXStateConfig(
    _In_ PXSTATE_CONFIGURATION XState)
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

/*!
 *   \brief Initializes the extended state configuration for the current processor
 *
 *   \param ProcessorNumber - Number of the current processor
 */
CODE_SEG("INIT")
VOID
NTAPI
KiInitializeXStateConfiguration(
    _In_ ULONG ProcessorNumber)
{
    if (ProcessorNumber == 0)
    {
        /* Processor 0: Retrieve the global configuration */
        KiGetXStateConfiguration(&SharedUserData->XState);
        KeXStateLength = SharedUserData->XState.AllFeatureSize;
    }
    else
    {
        /* Application processor: validate the configuration against the global one */
        XSTATE_CONFIGURATION XState;
        KiGetXStateConfiguration(&XState);
        ValidateXStateConfig(&XState);
    }

    /* Enable the user mode components in XCR0 */
    _xsetbv(0, SharedUserData->XState.EnabledFeatures);

    /* Now that we have set everything up, query CPUID again to get the required
       size based on components enabled in XCR0 */
    CPUID_EXTENDED_STATE_MAIN_LEAF_REGS ExtStateMain;
    __cpuidex(ExtStateMain.AsInt32,
              CPUID_EXTENDED_STATE,
              CPUID_EXTENDED_STATE_MAIN_LEAF);

    /* Make sure our calculation adds up! We only make sure that our size is
       large enough, as some VMs always report the full size, independent of
       what has been enabled in XCR0 */
    if (ExtStateMain.Ebx > SharedUserData->XState.Size)
    {
        KeBugCheck(MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED);
    }

    /* Check if we have any supervisor components enabled */
    if (SharedUserData->XState.EnabledSupervisorFeatures != 0)
    {
        /* Enable the supervisor components in IA32_XSS */
        __writemsr(MSR_IA32_XSS, SharedUserData->XState.EnabledSupervisorFeatures);

        /* Get the required size for features enabled in both XCR0 and IA32_XSS */
        CPUID_EXTENDED_STATE_SUB_LEAF_REGS ExtStateSubLeaf;
        __cpuidex(ExtStateSubLeaf.AsInt32,
                  CPUID_EXTENDED_STATE,
                  CPUID_EXTENDED_STATE_SUB_LEAF);

        /* Check if all components fit into what we calculated */
        if (ExtStateSubLeaf.Ebx.XSaveAreaSize > SharedUserData->XState.AllFeatureSize)
        {
            KeBugCheck(MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED);
        }
    }
}
