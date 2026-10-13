/*
 * PROJECT:     ReactOS NVM Express Miniport Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Controller and namespace identification
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "stornvme.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

/**
 * @brief Issues Identify with the given CNS into the scratch page.
 */
static
BOOLEAN
NvmpIdentify(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ ULONG NamespaceId,
    _In_ UCHAR ControllerOrNamespace)
{
    NVME_COMMAND Command;

    RtlZeroMemory(&Command, sizeof(Command));
    RtlZeroMemory(Adapter->ScratchBuffer, PAGE_SIZE);

    Command.CDW0.OPC = NVME_ADMIN_COMMAND_IDENTIFY;
    Command.NSID = NamespaceId;
    Command.PRP1 = Adapter->ScratchAddress.QuadPart;
    Command.u.IDENTIFY.CDW10.CNS = ControllerOrNamespace;

    return NvmpIssueAdminCommand(Adapter, &Command, NULL);
}


/**
 * @brief Converts MDTS to bytes, capped at NVME_MAX_TRANSFER_LENGTH.
 *
 * MDTS is a power of two in units of CAP.MPSMIN, 0 means no limit.
 */
static
ULONG
NvmpMaximumTransfer(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_IDENTIFY_CONTROLLER_DATA Data)
{
    ULONG MinimumPage;
    ULONG Length;

    if (Data->MDTS == 0 || Data->MDTS >= 32)
        return NVME_MAX_TRANSFER_LENGTH;

    MinimumPage = 1UL << (NVME_MIN_PAGE_SHIFT + Adapter->Capabilities.MPSMIN);

    /* Avoid overflowing the shift */
    if (Data->MDTS >= 31 || (ULONG)Data->MDTS > 31 - NVME_MIN_PAGE_SHIFT)
        return NVME_MAX_TRANSFER_LENGTH;

    Length = MinimumPage << Data->MDTS;
    if (Length > NVME_MAX_TRANSFER_LENGTH)
        Length = NVME_MAX_TRANSFER_LENGTH;

    return Length;
}


BOOLEAN
NvmpIdentifyController(
    _In_ PNVME_ADAPTER_EXTENSION Adapter)
{
    PNVME_IDENTIFY_CONTROLLER_DATA Data;

    if (!NvmpIdentify(Adapter, 0, NVME_IDENTIFY_CNS_CONTROLLER))
    {
        DPRINT1("Identify controller failed\n");
        return FALSE;
    }

    Data = Adapter->ScratchBuffer;

    Adapter->VendorId = Data->VID;
    Adapter->PowerStateCount = Data->NPSS;
    Adapter->NamespaceCount = Data->NN;
    Adapter->VolatileWriteCache = (Data->VWC.Present != 0);
    Adapter->MaximumTransferLength = NvmpMaximumTransfer(Adapter, Data);

    RtlCopyMemory(Adapter->SerialNumber, Data->SN, sizeof(Adapter->SerialNumber));
    RtlCopyMemory(Adapter->ModelNumber, Data->MN, sizeof(Adapter->ModelNumber));
    RtlCopyMemory(Adapter->FirmwareRevision, Data->FR, sizeof(Adapter->FirmwareRevision));

    DPRINT1("Controller has %lu namespaces, transfers up to %lu bytes%s\n",
            Adapter->NamespaceCount,
            Adapter->MaximumTransferLength,
            Adapter->VolatileWriteCache ? ", with a write cache" : "");

    if (Adapter->NamespaceCount > NVME_MAX_NAMESPACES)
        Adapter->NamespaceCount = NVME_MAX_NAMESPACES;

    return TRUE;
}


BOOLEAN
NvmpIdentifyNamespace(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ ULONG NamespaceId,
    _Out_ PNVME_NAMESPACE Namespace)
{
    PNVME_IDENTIFY_NAMESPACE_DATA Data;
    ULONG Format;

    RtlZeroMemory(Namespace, sizeof(*Namespace));

    if (!NvmpIdentify(Adapter, NamespaceId, NVME_IDENTIFY_CNS_SPECIFIC_NAMESPACE))
    {
        DPRINT1("Identify namespace %lu failed\n", NamespaceId);
        return FALSE;
    }

    Data = Adapter->ScratchBuffer;

    /* NSZE is 0 for an inactive namespace */
    if (Data->NSZE == 0)
        return FALSE;

    Format = Data->FLBAS.LbaFormatIndex;
    if (Format > Data->NLBAF)
    {
        DPRINT1("Namespace %lu names format %lu of %u\n",
                NamespaceId, Format, Data->NLBAF);
        return FALSE;
    }

    /* Metadata formats are not supported */
    if (Data->LBAF[Format].MS != 0)
    {
        DPRINT1("Namespace %lu wants %u metadata bytes per block\n",
                NamespaceId, Data->LBAF[Format].MS);
        return FALSE;
    }

    /* LBADS below 9 means the format is not supported */
    if (Data->LBAF[Format].LBADS < NVME_MIN_BLOCK_SHIFT)
    {
        DPRINT1("Namespace %lu names an unusable block size of 2^%u\n",
                NamespaceId, Data->LBAF[Format].LBADS);
        return FALSE;
    }

    Namespace->NamespaceId = NamespaceId;
    Namespace->BlockCount = Data->NSZE;
    Namespace->BlockShift = Data->LBAF[Format].LBADS;
    Namespace->BlockSize = 1UL << Namespace->BlockShift;
    Namespace->ThinProvisioning = Data->NSFEAT.ThinProvisioning;
    RtlCopyMemory(Namespace->Nguid, Data->NGUID, sizeof(Namespace->Nguid));
    RtlCopyMemory(Namespace->Eui64, Data->EUI64, sizeof(Namespace->Eui64));
    Namespace->Present = TRUE;

    DPRINT1("Namespace %lu holds %I64u blocks of %lu bytes\n",
            NamespaceId, Namespace->BlockCount, Namespace->BlockSize);

    return TRUE;
}


/**
 * @brief Identifies NSID 1 through NN and records the active namespaces.
 */
VOID
NvmpEnumerateNamespaces(
    _In_ PNVME_ADAPTER_EXTENSION Adapter)
{
    ULONG NamespaceId;
    ULONG Found = 0;

    for (NamespaceId = 1; NamespaceId <= Adapter->NamespaceCount; NamespaceId++)
    {
        if (NvmpIdentifyNamespace(Adapter,
                                  NamespaceId,
                                  &Adapter->Namespaces[NamespaceId - 1]))
        {
            Found++;
        }
    }

    DPRINT1("Found %lu attached namespaces\n", Found);
}
