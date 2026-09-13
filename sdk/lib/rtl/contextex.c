/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Functions related to extended contexts
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

static
ULONG
RtlpGetXSaveSize(
    _In_ const XSTATE_CONFIGURATION* XStateConfig,
    _In_ ULONG64 CompactionMask)
{
    /* Check if compaction is enabled */
    if (XStateConfig->CompactionEnabled)
    {
        /* Start at 0x240, after legacy fields and header */
        ULONG Size = 0x240;

        /* Now add up the sizes for each requested feature */
        for (ULONG Feature = 2; Feature < 64; Feature++)
        {
            ULONG64 FeatureBit = (1ULL << Feature);
            if (CompactionMask & FeatureBit)
            {
                /* Check if this is an aligned feature */
                if (XStateConfig->AlignedFeatures & FeatureBit)
                {
                    /* Add padding for alignment */
                    Size = ALIGN_UP_BY(Size, 64);
                }

                Size += XStateConfig->AllFeatures[Feature];
            }
        }

        return Size;
    }
    else
    {
        /* Compaction is not enabled, return the full size */
        return XStateConfig->Size;
    }
}

static
ULONG
RtlpGetXStateFeatureOffset(
    _In_ const XSTATE_CONFIGURATION* XStateConfig,
    _In_ ULONG64 FeatureMask,
    _In_ ULONG FeatureIndex,
    _Out_ PULONG FeatureSize)
{
    if ((FeatureIndex < 2) || (FeatureIndex >= 64))
    {
        *FeatureSize = 0;
        return 0;
    }

    if ((FeatureMask & (1ULL << FeatureIndex)) == 0)
    {
        *FeatureSize = 0;
        return 0;
    }

    *FeatureSize = XStateConfig->AllFeatures[FeatureIndex];

    /* Check if compaction is enabled */
    if (XStateConfig->CompactionEnabled)
    {
        /* Start at 0x240, after legacy fields and header */
        ULONG Offset = 0x240;

        /* Add all present feature sizes up to the requested one */
        for (ULONG i = 2; i < FeatureIndex; i++)
        {
            ULONG64 FeatureBit = (1ULL << i);
            if (FeatureMask & FeatureBit)
            {
                /* Check if this is an aligned feature */
                if (XStateConfig->AlignedFeatures & FeatureBit)
                {
                    /* Add padding for alignment */
                    Offset = ALIGN_UP_BY(Offset, 64);
                }

                Offset += XStateConfig->AllFeatures[i];
            }
        }

        return Offset;
    }
    else
    {
        return XStateConfig->Features[FeatureIndex].Offset;
    }
}

static
VOID
RtlpInitializeXSaveArea(
    _Out_ PXSAVE_AREA_HEADER XSaveAreaHeader,
    _In_ ULONG XSaveAreaLength,
    _In_ ULONG64 CompactionMask)
{
    /* Initialize the XSaveAreaHeader */
    RtlZeroMemory(XSaveAreaHeader, XSaveAreaLength);
    XSaveAreaHeader->Mask = 0;
    XSaveAreaHeader->CompactionMask = CompactionMask;
    if (SharedUserData->XState.CompactionEnabled)
    {
        XSaveAreaHeader->CompactionMask |= 0x8000000000000000ull;
    }
    else
    {
        XSaveAreaHeader->CompactionMask &= ~0x8000000000000000ull;
    }
}

ULONG64
NTAPI
RtlGetEnabledExtendedFeatures(
    _In_ ULONG64 FeatureMask)
{
    PXSTATE_CONFIGURATION XstateConfig = &SharedUserData->XState;
    ULONG64 AllEnabledFeatures = XstateConfig->EnabledFeatures |
                                 XstateConfig->EnabledUserVisibleSupervisorFeatures;

    return AllEnabledFeatures & FeatureMask;
}

NTSTATUS
NTAPI
RtlGetExtendedContextLength(
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength)
{
    PXSTATE_CONFIGURATION XstateConfig = &SharedUserData->XState;
    ULONG64 AllEnabledFeatures = XstateConfig->EnabledFeatures |
                                 XstateConfig->EnabledUserVisibleSupervisorFeatures;
    return RtlGetExtendedContextLength2(ContextFlags, ContextLength, AllEnabledFeatures);
}

static
ULONG
RtlpGetExtendedContextXStateLength(
    _In_ ULONG64 FeatureMask)
{
    PXSTATE_CONFIGURATION XStateConfig = &SharedUserData->XState;
    ULONG64 AllEnabledFeatures = XStateConfig->EnabledFeatures |
                                 XStateConfig->EnabledUserVisibleSupervisorFeatures;

    /* Sanitize FeatureMask */
    FeatureMask &= AllEnabledFeatures;

    /* Calculate the XSave size based on the sanitized feature mask */
    ULONG XSaveSize = RtlpGetXSaveSize(XStateConfig, FeatureMask);

    /* Subtract 0x200, as the legacy part is not part of the extended context chunk */
    return XSaveSize - 0x200;
}

static
NTSTATUS
RtlGetExtendedContextLengthI386(
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength,
    _In_ ULONG64 XStateCompactionMask)
{
    ULONG ExtendedContextLength;

    /* Validate flags for i386 context */
    if (ContextFlags & ~I386_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Start with optional alignment space */
    ExtendedContextLength = TYPE_ALIGNMENT(I386_CONTEXT) - 1;

    /* Add space for the I386_CONTEXT structure */
    ExtendedContextLength += sizeof(I386_CONTEXT);

    /* Add space for the CONTEXT_EX structure */
    ExtendedContextLength += sizeof(CONTEXT_EX);

    /* Check for XSTATE */
    if ((ContextFlags & I386_CONTEXT_XSTATE) == I386_CONTEXT_XSTATE)
    {
        /* We need up to 3 * 4 bytes to get to 16 byte alignment */
        ExtendedContextLength += 3 * sizeof(ULONG);

        /* We need up to 3 * 16 bytes to get to 64 byte alignment */
        ExtendedContextLength += 0x30;

        /* Now add XSTATE length */
        ExtendedContextLength += RtlpGetExtendedContextXStateLength(XStateCompactionMask);
    }

    /* Add bytes for optional alignment */
    *ContextLength = ExtendedContextLength;
    return STATUS_SUCCESS;
}

static
NTSTATUS
RtlGetExtendedContextLengthAmd64(
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength,
    _In_ ULONG64 XStateCompactionMask)
{
    ULONG ExtendedContextLength;

    /* Validate flags for AMD64 context */
    if (ContextFlags & ~AMD64_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Start with optional alignment space */
    ExtendedContextLength = TYPE_ALIGNMENT(AMD64_CONTEXT) - 1;

    /* Add space for the AMD64_CONTEXT structure */
    ExtendedContextLength += sizeof(AMD64_CONTEXT);

    /* Add space for the CONTEXT_EX structure */
    ExtendedContextLength += sizeof(CONTEXT_EX);

    /* Check for XSTATE */
    if ((ContextFlags & AMD64_CONTEXT_XSTATE) == AMD64_CONTEXT_XSTATE)
    {
        /* We need up 8 bytes to get to 16 byte alignment */
        ExtendedContextLength += sizeof(ULONG64);

        /* We need up to 3 * 16 bytes to get to 64 byte alignment */
        ExtendedContextLength += 0x30;

        /* Now add XSTATE length */
        ExtendedContextLength += RtlpGetExtendedContextXStateLength(XStateCompactionMask);
    }

    *ContextLength = ExtendedContextLength;
    return STATUS_SUCCESS;
}

static
NTSTATUS
RtlGetExtendedContextLengthArm32(
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength,
    _In_ ULONG64 XStateCompactionMask)
{
    ULONG ExtendedContextLength;

    /* Validate flags for ARM32 context */
    if (ContextFlags & ~ARM32_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Start with optional alignment space */
    ExtendedContextLength = TYPE_ALIGNMENT(ARM32_CONTEXT) - 1;

    /* Add space for the ARM32_CONTEXT structure */
    ExtendedContextLength += sizeof(ARM32_CONTEXT);

    /* Add space for the CONTEXT_EX structure */
    ExtendedContextLength += sizeof(CONTEXT_EX);

    *ContextLength = ExtendedContextLength;
    return STATUS_SUCCESS;
}

static
NTSTATUS
RtlGetExtendedContextLengthArm64(
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength,
    _In_ ULONG64 XStateCompactionMask)
{
    ULONG ExtendedContextLength;

    /* Validate flags for ARM64 context */
    if (ContextFlags & ~ARM64_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Start with optional alignment space */
    ExtendedContextLength = TYPE_ALIGNMENT(ARM64_CONTEXT) - 1;

    /* Add space for the ARM64_CONTEXT structure */
    ExtendedContextLength += sizeof(ARM64_CONTEXT);

    /* Add space for the CONTEXT_EX structure */
    ExtendedContextLength += sizeof(CONTEXT_EX);

    *ContextLength = ExtendedContextLength;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlGetExtendedContextLength2(
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength,
    _In_ ULONG64 XStateCompactionMask)
{
    /* Check architecture flags */
    ULONG Architecture = ContextFlags & CONTEXT_ARCHITECTURE_MASK;
    switch (Architecture)
    {
        case CONTEXT_i386:
            return RtlGetExtendedContextLengthI386(ContextFlags, ContextLength, XStateCompactionMask);

        case CONTEXT_AMD64:
            return RtlGetExtendedContextLengthAmd64(ContextFlags, ContextLength, XStateCompactionMask);

        case CONTEXT_ARM32:
            return RtlGetExtendedContextLengthArm32(ContextFlags, ContextLength, XStateCompactionMask);

        case CONTEXT_ARM64:
            return RtlGetExtendedContextLengthArm64(ContextFlags, ContextLength, XStateCompactionMask);

        default:
            return STATUS_INVALID_PARAMETER;
    }
}

NTSTATUS
NTAPI
RtlInitializeExtendedContext(
    _Out_ PVOID ContextBuffer,
    _In_ ULONG ContextFlags,
    _Outptr_ PCONTEXT_EX* OutContextEx)
{
    PXSTATE_CONFIGURATION XStateConfig = &SharedUserData->XState;
    ULONG64 AllEnabledFeatures = XStateConfig->EnabledFeatures |
                                 XStateConfig->EnabledUserVisibleSupervisorFeatures;

    /* Call the extended context initialization function */
    return RtlInitializeExtendedContext2(ContextBuffer,
                                         ContextFlags,
                                         OutContextEx,
                                         AllEnabledFeatures);
}

static
NTSTATUS
RtlInitializeExtendedContextI386(
    _Out_ PVOID ContextBuffer,
    _In_ ULONG ContextFlags,
    _Outptr_ PCONTEXT_EX* OutContextEx,
    _In_ ULONG64 XStateCompactionMask)
{
    PCONTEXT_EX ContextEx;
    PI386_CONTEXT ContextI386;
    PVOID XStateContext;

    /* Validate flags for i386 context */
    if (ContextFlags & ~I386_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get an aligned context pointer and initialize the context flags */
    ContextI386 = ALIGN_UP_POINTER_BY(ContextBuffer, TYPE_ALIGNMENT(I386_CONTEXT));
    ContextI386->ContextFlags = ContextFlags;

    /* The CONTEXT_EX follows right after the legacy context */
    ContextEx = (PCONTEXT_EX)(ContextI386 + 1);

    /* Set up the legacy chunk */
    ContextEx->Legacy.Offset = -(LONG)sizeof(I386_CONTEXT);
    ContextEx->All.Offset = ContextEx->Legacy.Offset;
    if ((ContextFlags & I386_CONTEXT_EXTENDED_REGISTERS) == I386_CONTEXT_EXTENDED_REGISTERS)
    {
        ContextEx->Legacy.Length = sizeof(I386_CONTEXT);
    }
    else
    {
        ContextEx->Legacy.Length = FIELD_OFFSET(I386_CONTEXT, ExtendedRegisters);
    }

    /* Check if CONTEXT_XSTATE is requested */
    if ((ContextFlags & I386_CONTEXT_XSTATE) == I386_CONTEXT_XSTATE)
    {
        /* The extended context follows after the CONTEXT_EX and must be 64 byte aligned */
        XStateContext = ALIGN_UP_POINTER_BY(ContextEx + 1, 64);

        /* Calculate XSTATE offset */
        ContextEx->XState.Offset = (ULONG)((PUCHAR)XStateContext - (PUCHAR)ContextEx);

        /* Calculate XSTATE length based on the compaction mask */
        ContextEx->XState.Length = RtlpGetExtendedContextXStateLength(XStateCompactionMask);

        /* Zero out the extended context */
        RtlpInitializeXSaveArea(XStateContext, ContextEx->XState.Length, XStateCompactionMask);

        /* Set up full length */
        ContextEx->All.Length = sizeof(I386_CONTEXT) + ContextEx->XState.Offset + ContextEx->XState.Length;
    }
    else
    {
        /* Set default values for XState (1 byte past the buffer) */
        ContextEx->XState.Offset = sizeof(CONTEXT_EX) + 1;
        ContextEx->XState.Length = 0;
        ContextEx->All.Length = sizeof(I386_CONTEXT) + sizeof(CONTEXT_EX);
    }

    *OutContextEx = ContextEx;
    return STATUS_SUCCESS;
}

static
NTSTATUS
RtlInitializeExtendedContextAmd64(
    _Out_ PVOID ContextBuffer,
    _In_ ULONG ContextFlags,
    _Outptr_ PCONTEXT_EX* OutContextEx,
    _In_ ULONG64 XStateCompactionMask)
{
    PCONTEXT_EX ContextEx;
    PAMD64_CONTEXT ContextAMD64;
    PVOID XStateContext;

    /* Validate flags for AMD64 context */
    if (ContextFlags & ~AMD64_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get an aligned context pointer and initialize the context flags */
    ContextAMD64 = ALIGN_UP_POINTER_BY(ContextBuffer, TYPE_ALIGNMENT(AMD64_CONTEXT));
    ContextAMD64->ContextFlags = ContextFlags;

    /* The CONTEXT_EX follows right after the legacy context */
    ContextEx = (PCONTEXT_EX)(ContextAMD64 + 1);

    /* Set up the legacy chunk and All offset */
    ContextEx->Legacy.Offset = -(LONG)sizeof(AMD64_CONTEXT);
    ContextEx->Legacy.Length = sizeof(AMD64_CONTEXT);
    ContextEx->All.Offset = ContextEx->Legacy.Offset;

    /* Check if CONTEXT_XSTATE is requested */
    if ((ContextFlags & AMD64_CONTEXT_XSTATE) == AMD64_CONTEXT_XSTATE)
    {
        /* The extended context follows after the CONTEXT_EX and must be 64 byte aligned */
        XStateContext = ALIGN_UP_POINTER_BY(ContextEx + 1, 64);

        /* Calculate XSTATE offset */
        ContextEx->XState.Offset = (ULONG)((PUCHAR)XStateContext - (PUCHAR)ContextEx);

        /* Calculate XSTATE length based on the compaction mask */
        ContextEx->XState.Length = RtlpGetExtendedContextXStateLength(XStateCompactionMask);

        /* Zero out the extended context */
        RtlpInitializeXSaveArea(XStateContext, ContextEx->XState.Length, XStateCompactionMask);

        /* Set up full length */
        ContextEx->All.Length = sizeof(AMD64_CONTEXT) + ContextEx->XState.Offset + ContextEx->XState.Length;
    }
    else
    {
        /* Set default values for XState (1 byte past the buffer) */
        ContextEx->XState.Offset = sizeof(CONTEXT_EX) + 1;
        ContextEx->XState.Length = 0;
        ContextEx->All.Length = sizeof(AMD64_CONTEXT) + sizeof(CONTEXT_EX);
    }

    *OutContextEx = ContextEx;
    return STATUS_SUCCESS;
}

static
NTSTATUS
RtlInitializeExtendedContextArm32(
    _Out_ PVOID ContextBuffer,
    _In_ ULONG ContextFlags,
    _Outptr_ PCONTEXT_EX* OutContextEx,
    _In_ ULONG64 XStateCompactionMask)
{
    PCONTEXT_EX ContextEx;
    PARM32_CONTEXT ContextArm32;

    /* Validate flags for ARM32 context */
    if (ContextFlags & ~ARM32_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get an aligned context pointer and initialize the context flags */
    ContextArm32 = ALIGN_UP_POINTER_BY(ContextBuffer, TYPE_ALIGNMENT(ARM32_CONTEXT));
    ContextArm32->ContextFlags = ContextFlags;

    /* The CONTEXT_EX follows right after the legacy context */
    ContextEx = (PCONTEXT_EX)(ContextArm32 + 1);

    /* Set up the chunks (XState is 1 byte past the buffer)*/
    ContextEx->Legacy.Offset = -(LONG)sizeof(ARM32_CONTEXT);
    ContextEx->Legacy.Length = sizeof(ARM32_CONTEXT);
    ContextEx->XState.Offset = sizeof(CONTEXT_EX) + 1;
    ContextEx->XState.Length = 0;
    ContextEx->All.Offset = ContextEx->Legacy.Offset;
    ContextEx->All.Length = sizeof(ARM32_CONTEXT) + sizeof(CONTEXT_EX);

    *OutContextEx = ContextEx;
    return STATUS_SUCCESS;
}

static
NTSTATUS
RtlInitializeExtendedContextArm64(
    _Out_ PVOID ContextBuffer,
    _In_ ULONG ContextFlags,
    _Outptr_ PCONTEXT_EX* OutContextEx,
    _In_ ULONG64 XStateCompactionMask)
{
    PCONTEXT_EX ContextEx;
    PARM64_CONTEXT ContextArm64;

    /* Validate flags for ARM64 context */
    if (ContextFlags & ~ARM64_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get an aligned context pointer and initialize the context flags */
    ContextArm64 = ALIGN_UP_POINTER_BY(ContextBuffer, TYPE_ALIGNMENT(ARM64_CONTEXT));
    ContextArm64->ContextFlags = ContextFlags;

    /* The CONTEXT_EX follows right after the legacy context */
    ContextEx = (PCONTEXT_EX)(ContextArm64 + 1);

    /* Set up the chunks (XState is 1 byte past the buffer) */
    ContextEx->Legacy.Offset = -(LONG)sizeof(ARM64_CONTEXT);
    ContextEx->Legacy.Length = sizeof(ARM64_CONTEXT);
    ContextEx->XState.Offset = sizeof(CONTEXT_EX) + 1;
    ContextEx->XState.Length = 0;
    ContextEx->All.Offset = ContextEx->Legacy.Offset;
    ContextEx->All.Length = sizeof(ARM64_CONTEXT) + sizeof(CONTEXT_EX);

    *OutContextEx = ContextEx;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlInitializeExtendedContext2(
    _Out_ PVOID ContextBuffer,
    _In_ ULONG ContextFlags,
    _Outptr_ PCONTEXT_EX* OutContextEx,
    _In_ ULONG64 XStateCompactionMask)
{
    /* Validate ContextEx output parameter */
    if (OutContextEx == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check architecture flags */
    ULONG Architecture = ContextFlags & CONTEXT_ARCHITECTURE_MASK;
    switch (Architecture)
    {
        case CONTEXT_i386:
            return RtlInitializeExtendedContextI386(ContextBuffer,
                                                    ContextFlags,
                                                    OutContextEx,
                                                    XStateCompactionMask);

        case CONTEXT_AMD64:
            return RtlInitializeExtendedContextAmd64(ContextBuffer,
                                                     ContextFlags,
                                                     OutContextEx,
                                                     XStateCompactionMask);

        case CONTEXT_ARM32:
            return RtlInitializeExtendedContextArm32(ContextBuffer,
                                                     ContextFlags,
                                                     OutContextEx,
                                                     XStateCompactionMask);

        case CONTEXT_ARM64:
            return RtlInitializeExtendedContextArm64(ContextBuffer,
                                                     ContextFlags,
                                                     OutContextEx,
                                                     XStateCompactionMask);

        default:
            return STATUS_INVALID_PARAMETER;
    }
}

PCONTEXT
NTAPI
RtlLocateLegacyContext(
    _In_ const CONTEXT_EX* ContextEx,
    _Out_opt_ PULONG Length)
{
    PCONTEXT LegacyContext = (PCONTEXT)((PUCHAR)ContextEx + ContextEx->Legacy.Offset);

    if (Length != NULL)
    {
        *Length = ContextEx->Legacy.Length;
    }

    return LegacyContext;
}

PVOID
NTAPI
RtlLocateExtendedFeature(
    _In_ const CONTEXT_EX* ContextEx,
    _In_ ULONG FeatureId,
    _Out_opt_ PULONG Length)
{
    PXSTATE_CONFIGURATION XStateConfig = &SharedUserData->XState;
    return RtlLocateExtendedFeature2(ContextEx, FeatureId, XStateConfig, Length);
}

static
PVOID
RtlpLocateXStateContext(
    _In_ const CONTEXT_EX* ContextEx)
{
    PVOID XStateContext, ExpectedXStateContext;

    /* Check if the XState.Offset is present and valid */
    if (ContextEx->XState.Offset == 0)
    {
        return NULL;
    }

    XStateContext = ((PUCHAR)ContextEx + ContextEx->XState.Offset);

    /* Make sure the XState context is where it is supposed to be */
    ExpectedXStateContext = ALIGN_UP_POINTER_BY((PUCHAR)ContextEx + sizeof(CONTEXT_EX), 64);
    if (XStateContext != ExpectedXStateContext)
    {
        return NULL;
    }

    return XStateContext;
}

PVOID
NTAPI
RtlLocateExtendedFeature2(
    _In_ const CONTEXT_EX* ContextEx,
    _In_ ULONG FeatureId,
    _In_ const XSTATE_CONFIGURATION* XStateConfig,
    _Out_opt_ PULONG Length)
{
    PXSAVE_AREA_HEADER XSaveAreaHeader;
    ULONG64 EnabledFeatures, PresentFeatures;
    PVOID FeatureData = NULL;
    ULONG FeatureOffset;
    ULONG FeatureLength = 0;

    /* Make sure FeatureId is within the valid range */
    if ((FeatureId < 2) || (FeatureId >= 64))
    {
        return NULL;
    }

    /* Check FeatureId against the enabled features */
    EnabledFeatures = XStateConfig->EnabledFeatures |
                      XStateConfig->EnabledUserVisibleSupervisorFeatures;
    if (((1ULL << FeatureId) & EnabledFeatures) == 0)
    {
        return NULL;
    }

    /* Get a pointer to the XState context */
    XSaveAreaHeader = RtlpLocateXStateContext(ContextEx);
    if (XSaveAreaHeader == NULL)
    {
        return NULL;
    }

    if (XStateConfig->CompactionEnabled)
    {
        /* If compaction is enabled, we need to get the present features from the XSTATE data */
        PresentFeatures = XSaveAreaHeader->CompactionMask;
    }
    else
    {
        /* If compaction is not enabled, we can get the present features from the XSTATE configuration */
        PresentFeatures = EnabledFeatures;
    }

    /* Get the feature offset and length */
    FeatureOffset = RtlpGetXStateFeatureOffset(XStateConfig, PresentFeatures, FeatureId, &FeatureLength);
    if (FeatureOffset == 0)
    {
        return NULL;
    }

    /* Calculate the pointer to the feature data */
    FeatureData = (PVOID)((PUCHAR)XSaveAreaHeader + FeatureOffset - 0x200);

    if (Length != NULL)
    {
        *Length = FeatureLength;
    }

    return FeatureData;
}

ULONG64
NTAPI
RtlGetExtendedFeaturesMask(
    _In_ const CONTEXT_EX* ContextEx)
{
    PXSAVE_AREA_HEADER XSaveAreaHeader;

    /* Windows does not validate that the XState context is present or valid */
    XSaveAreaHeader = RtlpLocateXStateContext(ContextEx);
    if (XSaveAreaHeader == NULL)
    {
        return 0;
    }

    return XSaveAreaHeader->Mask & ~3;
}

VOID
NTAPI
RtlSetExtendedFeaturesMask(
    _Inout_ PCONTEXT_EX ContextEx,
    _In_ ULONG64 FeatureMask)
{
    PXSAVE_AREA_HEADER XSaveAreaHeader;

    /* Sanatize the Feature mask and disable legacy features */
    FeatureMask = RtlGetEnabledExtendedFeatures(FeatureMask) & ~3;

    /* Windows does not validate that the XState context is present or valid */
    XSaveAreaHeader = RtlpLocateXStateContext(ContextEx);
    if (XSaveAreaHeader == NULL)
    {
        return;
    }

    XSaveAreaHeader->Mask = FeatureMask;
}

static
NTSTATUS
RtlpCopyXStateContext(
    _Out_ PCONTEXT_EX DstContextEx,
    _In_ const CONTEXT_EX* SrcContextEx)
{
    PXSAVE_AREA_HEADER SrcXSave, DstXSave;

    SrcXSave = RtlpLocateXStateContext(SrcContextEx);
    DstXSave = RtlpLocateXStateContext(DstContextEx);
    if ((SrcXSave == NULL) || (DstXSave == NULL))
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    PXSTATE_CONFIGURATION XstateConfig = &SharedUserData->XState;
    ULONG64 EnabledFeatures = XstateConfig->EnabledFeatures |
                              XstateConfig->EnabledUserVisibleSupervisorFeatures;

    ULONG64 FeatureMask = SrcXSave->Mask & EnabledFeatures;

    /* Validate the source */
    if (((SrcXSave->Mask & ~EnabledFeatures) != 0) ||
        ((SrcXSave->CompactionMask & ~(EnabledFeatures | 0x8000000000000000ULL)) != 0))
    {
        /* Source is invalid, clear the destination */
        RtlZeroMemory(DstXSave, DstContextEx->XState.Length);
        DstXSave->Mask = FeatureMask & ~3;
        DstXSave->CompactionMask = FeatureMask;
        if (XstateConfig->CompactionEnabled)
        {
            DstXSave->CompactionMask |= 0x8000000000000000ULL;
        }
        return STATUS_SUCCESS;
    }

    /* Get the required size */
    ULONG XStateLength = RtlpGetExtendedContextXStateLength(FeatureMask);

    /* Make sure both source and destination are large enough */
    if ((SrcContextEx->XState.Length < XStateLength) ||
        (DstContextEx->XState.Length < XStateLength))
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    
    /* Copy the XSTATE data */
    RtlCopyMemory(DstXSave, SrcXSave, XStateLength);

    return STATUS_SUCCESS;
}

static
NTSTATUS
RtlCopyExtendedContextI386(
    _Out_ PCONTEXT_EX DstContextEx,
    _In_ ULONG ContextFlags,
    _In_ const CONTEXT_EX* SrcContextEx)
{
    PI386_CONTEXT SrcContextI386, DstContextI386;
    NTSTATUS Status;

    ASSERT((ContextFlags & CONTEXT_i386) == CONTEXT_i386);

    /* Validate flags for i386 context */
    if (ContextFlags & ~I386_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate legacy context offsets */
    if ((SrcContextEx->Legacy.Offset != -(LONG)sizeof(I386_CONTEXT)) ||
        (DstContextEx->Legacy.Offset != -(LONG)sizeof(I386_CONTEXT)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if XSTATE is requested */
    if ((ContextFlags & I386_CONTEXT_XSTATE) == I386_CONTEXT_XSTATE)
    {
        /* Make sure both source and destination have XSTATE data */
        if ((SrcContextEx->XState.Length == 0) || (DstContextEx->XState.Length == 0))
        {
            return STATUS_BUFFER_OVERFLOW;
        }
    }

    /* Get the source and destination i386 contexts */
    SrcContextI386 = (PI386_CONTEXT)((PUCHAR)SrcContextEx + SrcContextEx->Legacy.Offset);
    DstContextI386 = (PI386_CONTEXT)((PUCHAR)DstContextEx + DstContextEx->Legacy.Offset);

    /* Copy the required legacy context chunks */
    RtlpCopyContextI386Internal(DstContextI386, ContextFlags, SrcContextI386);
    DstContextI386->ContextFlags = ContextFlags;

    /* Check if XSTATE is requested */
    if ((ContextFlags & I386_CONTEXT_XSTATE) == I386_CONTEXT_XSTATE)
    {
        /* We need to copy the XSTATE data as well */
        Status = RtlpCopyXStateContext(DstContextEx, SrcContextEx);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
RtlCopyExtendedContextAmd64(
    _Out_ PCONTEXT_EX DstContextEx,
    _In_ ULONG ContextFlags,
    _In_ const CONTEXT_EX* SrcContextEx)
{
    PAMD64_CONTEXT SrcContextAmd64, DstContextAmd64;
    NTSTATUS Status;

    ASSERT((ContextFlags & CONTEXT_AMD64) == CONTEXT_AMD64);

    /* Validate flags for AMD64 context */
    if (ContextFlags & ~AMD64_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate legacy context offsets */
    if ((SrcContextEx->Legacy.Offset != -(LONG)sizeof(AMD64_CONTEXT)) ||
        (DstContextEx->Legacy.Offset != -(LONG)sizeof(AMD64_CONTEXT)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if XSTATE is requested */
    if ((ContextFlags & AMD64_CONTEXT_XSTATE) == AMD64_CONTEXT_XSTATE)
    {
        /* Make sure both source and destination have XSTATE data */
        if ((SrcContextEx->XState.Length == 0) || (DstContextEx->XState.Length == 0))
        {
            return STATUS_BUFFER_OVERFLOW;
        }
    }

    /* Get the source and destination AMD64 contexts */
    SrcContextAmd64 = (PAMD64_CONTEXT)((PUCHAR)SrcContextEx + SrcContextEx->Legacy.Offset);
    DstContextAmd64 = (PAMD64_CONTEXT)((PUCHAR)DstContextEx + DstContextEx->Legacy.Offset);

    /* Copy the required legacy context chunks */
    RtlpCopyContextAmd64Internal(DstContextAmd64, ContextFlags, SrcContextAmd64);
    DstContextAmd64->ContextFlags = ContextFlags;

    /* Check if XSTATE is requested */
    if ((ContextFlags & AMD64_CONTEXT_XSTATE) == AMD64_CONTEXT_XSTATE)
    {
        /* We need to copy the XSTATE data as well */
        Status = RtlpCopyXStateContext(DstContextEx, SrcContextEx);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
RtlCopyExtendedContextArm32(
    _Out_ PCONTEXT_EX DstContextEx,
    _In_ ULONG ContextFlags,
    _In_ const CONTEXT_EX* SrcContextEx)
{
    PARM32_CONTEXT SrcContextArm32, DstContextArm32;

    ASSERT((ContextFlags & CONTEXT_ARM32) == CONTEXT_ARM32);

    /* Validate flags for ARM32 context */
    if (ContextFlags & ~ARM32_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate legacy context offsets */
    if ((SrcContextEx->Legacy.Offset != -(LONG)sizeof(ARM32_CONTEXT)) ||
        (DstContextEx->Legacy.Offset != -(LONG)sizeof(ARM32_CONTEXT)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the source and destination ARM32 contexts */
    SrcContextArm32 = (PARM32_CONTEXT)((PUCHAR)SrcContextEx + SrcContextEx->Legacy.Offset);
    DstContextArm32 = (PARM32_CONTEXT)((PUCHAR)DstContextEx + DstContextEx->Legacy.Offset);

    /* Copy the required legacy context chunks */
    RtlpCopyContextArm32Internal(DstContextArm32, ContextFlags, SrcContextArm32);
    DstContextArm32->ContextFlags = ContextFlags;

    return STATUS_SUCCESS;
}

static
NTSTATUS
RtlCopyExtendedContextArm64(
    _Out_ PCONTEXT_EX DstContextEx,
    _In_ ULONG ContextFlags,
    _In_ const CONTEXT_EX* SrcContextEx)
{
    PARM64_CONTEXT SrcContextArm64, DstContextArm64;

    ASSERT((ContextFlags & CONTEXT_ARM64) == CONTEXT_ARM64);

    /* Validate flags for ARM64 context */
    if (ContextFlags & ~ARM64_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate legacy context offsets */
    if ((SrcContextEx->Legacy.Offset != -(LONG)sizeof(ARM64_CONTEXT)) ||
        (DstContextEx->Legacy.Offset != -(LONG)sizeof(ARM64_CONTEXT)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the source and destination ARM64 contexts */
    SrcContextArm64 = (PARM64_CONTEXT)((PUCHAR)SrcContextEx + SrcContextEx->Legacy.Offset);
    DstContextArm64 = (PARM64_CONTEXT)((PUCHAR)DstContextEx + DstContextEx->Legacy.Offset);

    /* Copy the required legacy context chunks */
    RtlpCopyContextArm64Internal(DstContextArm64, ContextFlags, SrcContextArm64);
    DstContextArm64->ContextFlags = ContextFlags;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlCopyExtendedContext(
    _Out_ PCONTEXT_EX DstContextEx,
    _In_ ULONG ContextFlags,
    _In_ const CONTEXT_EX* SrcContextEx)
{
    ULONG Architecture;

    /* Validate source and dest pointers */
    if ((DstContextEx == NULL) || (SrcContextEx == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure the sizes are OK */
    if (DstContextEx->Legacy.Length < SrcContextEx->Legacy.Length)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Architecture = ContextFlags & CONTEXT_ARCHITECTURE_MASK;
    switch (Architecture)
    {
        case CONTEXT_i386:
            return RtlCopyExtendedContextI386(DstContextEx, ContextFlags, SrcContextEx);

        case CONTEXT_AMD64:
            return RtlCopyExtendedContextAmd64(DstContextEx, ContextFlags, SrcContextEx);

        case CONTEXT_ARM32:
            return RtlCopyExtendedContextArm32(DstContextEx, ContextFlags, SrcContextEx);

        case CONTEXT_ARM64:
            return RtlCopyExtendedContextArm64(DstContextEx, ContextFlags, SrcContextEx);

        default:
            return STATUS_INVALID_PARAMETER;
    }
}
