#include "precomp.h"

BOOLEAN
NVMeExecuteModeSenseCaching(PSCSI_REQUEST_BLOCK pSrb, PNVME_DEVICE_EXTENSION pDevExt)
{
    if (pSrb == NULL || pDevExt == NULL)
        return FALSE;
    
    PCDB pCdb = (PCDB)pSrb->Cdb;

    if (pCdb->MODE_SENSE.OperationCode != SCSIOP_MODE_SENSE) 
        return FALSE;
    
    ULONG totalSize = sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_CACHING_PAGE);

    if (pSrb->DataTransferLength < totalSize) {
        SetScsiSenseData(pSrb,
                SCSISTAT_CHECK_CONDITION,
                SCSI_SENSE_ILLEGAL_REQUEST,
                SCSI_ADSENSE_INVALID_CDB,
                SCSI_ADSENSE_NO_SENSE);
        pSrb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        pSrb->DataTransferLength = 0;
        return FALSE;
    }

    MODE_PARAMETER_HEADER paraHeader = {0};
    paraHeader.MediumType = DIRECT_ACCESS_DEVICE;
    paraHeader.BlockDescriptorLength = 0;
    paraHeader.ModeDataLength = totalSize - 1;

    MODE_CACHING_PAGE cachingPage  = {0};
    cachingPage.PageCode           = pCdb->MODE_SENSE.PageCode;
    cachingPage.PageLength         = sizeof(MODE_CACHING_PAGE);
    cachingPage.ReadDisableCache     = 1;
    cachingPage.MultiplicationFactor = 0;
    cachingPage.WriteCacheEnable     = 0;

    RtlCopyMemory(pSrb->DataBuffer, &paraHeader, sizeof(MODE_PARAMETER_HEADER));
    RtlCopyMemory((PUCHAR)pSrb->DataBuffer + sizeof(MODE_PARAMETER_HEADER), &cachingPage, sizeof(MODE_CACHING_PAGE));

    pSrb->DataTransferLength = totalSize;
    pSrb->SrbStatus = SRB_STATUS_SUCCESS;
    return TRUE;
}