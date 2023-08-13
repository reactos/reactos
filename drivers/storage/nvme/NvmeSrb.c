/**
 *******************************************************************************
 ** Copyright (c) 2011-2012                                                   **
 **                                                                           **
 **   Integrated Device Technology, Inc.                                      **
 **   Intel Corporation                                                       **
 **   LSI Corporation                                                         **
 **                                                                           **
 ** All rights reserved.                                                      **
 **                                                                           **
 *******************************************************************************
 **                                                                           **
 ** Redistribution and use in source and binary forms, with or without        **
 ** modification, are permitted provided that the following conditions are    **
 ** met:                                                                      **
 **                                                                           **
 **   1. Redistributions of source code must retain the above copyright       **
 **      notice, this list of conditions and the following disclaimer.        **
 **                                                                           **
 **   2. Redistributions in binary form must reproduce the above copyright    **
 **      notice, this list of conditions and the following disclaimer in the  **
 **      documentation and/or other materials provided with the distribution. **
 **                                                                           **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS   **
 ** IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, **
 ** THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR    **
 ** PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR         **
 ** CONTRIBUTORS BE LIABLE FOR ANY DIRECT,INDIRECT, INCIDENTAL, SPECIAL,      **
 ** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       **
 ** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        **
 ** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
 ** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
 ** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
 ** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
 **                                                                           **
 ** The views and conclusions contained in the software and documentation     **
 ** are those of the authors and should not be interpreted as representing    **
 ** official policies, either expressed or implied, of Intel Corporation,     **
 ** Integrated Device Technology Inc., or Sandforce Corporation.              **
 **                                                                           **
 *******************************************************************************
**/

#include "precomp.h"

VOID 
SetScsiSenseData(PSCSI_REQUEST_BLOCK pSrb, UCHAR scsiStatus, UCHAR senseKey, UCHAR asc, UCHAR ascq)
{
    PSENSE_DATA pSenseData = NULL;
    UCHAR senseInfoBufferLength = 0;
    PVOID senseInfoBuffer = NULL;
    UCHAR senseDataLength = sizeof(SENSE_DATA);
    pSrb->ScsiStatus = scsiStatus;
    senseInfoBufferLength = pSrb->SenseInfoBufferLength;
    senseInfoBuffer = pSrb->SenseInfoBuffer;
    if ((scsiStatus != SCSISTAT_GOOD) &&
        (senseInfoBufferLength >= senseDataLength)) {

        pSenseData = (PSENSE_DATA)senseInfoBuffer;

        RtlZeroMemory(pSenseData, senseInfoBufferLength);
        pSenseData->ErrorCode                    = FIXED_SENSE_DATA;
        pSenseData->SenseKey                     = senseKey;
        pSenseData->AdditionalSenseCode          = asc;
        pSenseData->AdditionalSenseCodeQualifier = ascq;

        pSenseData->AdditionalSenseLength = senseDataLength - 
            FIELD_OFFSET(SENSE_DATA, CommandSpecificInformation);
        pSrb->SenseInfoBufferLength = sizeof(SENSE_DATA);
        pSrb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
    } else {
        pSrb->SenseInfoBufferLength = 0;
    }
}

BOOLEAN 
NvmeTranslateSglToPrp(PNVME_DEVICE_EXTENSION pDevExt, PSCSI_REQUEST_BLOCK pSrb, PNVMe_COMMAND pNvmeCmd, ULONG transferLen)
{
    if (pDevExt == NULL || pSrb == NULL || pNvmeCmd == 0)
        return FALSE;

    PUINT64 pPrp1   = &pNvmeCmd->PRP1;
    PUINT64 pPrp2   = &pNvmeCmd->PRP2;
    ULONG remLength = min(pSrb->DataTransferLength, transferLen);
    PUINT8 buffer   = (PUINT8)pSrb->DataBuffer;
    PHYSICAL_ADDRESS phyAddr = {0};
    ULONG numPrpEntries = 0;
    PUINT64 prpList     = (PUINT64)pDevExt->QueueInfo.SubQueueInfo[1].PRPListStartVirtual;

    while(remLength) {
        ULONG bufferLen = 0;
        phyAddr = ScsiPortGetPhysicalAddress(pDevExt, pSrb, buffer, &bufferLen);

        /* Update the remaining length and buffer ptr */
        remLength = (bufferLen > remLength) ? 0 : remLength - bufferLen;
        buffer   += bufferLen;

        /* Update number of entries */
        numPrpEntries++;

        /* 
        * Only first and last entries can be below page sizes. 
        * If its the first entry it should be PRP1 or first entry in PRP2
        * If its the last entry remLength == 0
        */
        if (((bufferLen % PAGE_SIZE) != 0)) {
            if ((numPrpEntries != 1 || numPrpEntries != 2) && remLength != 0)
            /* Bad buffer? We should gracefully fail here */
                ASSERT(FALSE);
        }

        /* SGL could also have a contigious block break it up here */
        while(bufferLen) {
            /* The first element will always go into PRP 1 */
            if (numPrpEntries == 1) {
                *pPrp1 = phyAddr.QuadPart;
            }
            /* The second element will go into PRP 2 _if_ there is no PRP List */
            else if (numPrpEntries == 2) {
                *pPrp2 = phyAddr.QuadPart;
            }
            /* Alright so we have a list. Lets move the PRP 2 pointer into list
               and copy the list pointer into PRP 2 */
            else if (numPrpEntries == 3) {
                *prpList = *pPrp2;
                *pPrp2 = (UINT64)prpList;
                prpList++;
                *prpList = phyAddr.QuadPart;
            }
            /* Keep appending list, we only have one list */
            else {
                *prpList = phyAddr.QuadPart;
                prpList++;
            }

            /* If multiple contigious pages in this SGL */
            if (bufferLen > PAGE_SIZE) {
                bufferLen -= PAGE_SIZE;
                phyAddr.QuadPart += PAGE_SIZE;
                numPrpEntries++;
            }
            /* Done with this SGL */ 
            else {
                bufferLen =  0;
                phyAddr.QuadPart = 0;
            }
        }
    }

    return TRUE;
}

BOOLEAN 
NVMeExecuteReadCapacity(PSCSI_REQUEST_BLOCK pSrb, PNVME_DEVICE_EXTENSION pDevExt)
{
    if (pSrb == NULL || pDevExt == NULL) {
        ASSERT(FALSE);
        return FALSE;
    }

    ULONG expectedCap = 0;
   
    if (pSrb->Cdb[0] == SCSIOP_READ_CAPACITY16) {
        READ_CAPACITY_DATA_EX readCapEx = {0};

        if (pSrb->DataTransferLength < sizeof(readCapEx)) {
            expectedCap = sizeof(readCapEx);
            goto ERR_NOT_ENOUGH_MEM;
        }

        ULONG blockSize = 1 << pDevExt->NSInfo.identifyData.LBAFx[pDevExt->NSInfo.identifyData.FLBAS.SupportedCombination].LBADS;
        REVERSE_BYTES(&readCapEx.BytesPerBlock, &blockSize);

        ULONGLONG nLBA = pDevExt->NSInfo.identifyData.NSZE;
        REVERSE_BYTES_QUAD(&readCapEx.LogicalBlockAddress, &nLBA);

        RtlCopyMemory(pSrb->DataBuffer, &readCapEx, min(sizeof(readCapEx), pSrb->DataTransferLength));
    } else {
        READ_CAPACITY_DATA readCap = {0};

        if (pSrb->DataTransferLength < sizeof(readCap)) {
            expectedCap = sizeof(readCap);
            goto ERR_NOT_ENOUGH_MEM;
        }

        ULONG blockSize = 1 << pDevExt->NSInfo.identifyData.LBAFx[pDevExt->NSInfo.identifyData.FLBAS.SupportedCombination].LBADS;
        REVERSE_BYTES(&readCap.BytesPerBlock, &blockSize);

        ULONG nLBA = pDevExt->NSInfo.identifyData.NSZE;
        REVERSE_BYTES(&readCap.LogicalBlockAddress, &nLBA);
        
        RtlCopyMemory(pSrb->DataBuffer, &readCap, min(sizeof(readCap), pSrb->DataTransferLength));
    }

    pSrb->SrbStatus = SRB_STATUS_SUCCESS;
    return TRUE;

ERR_NOT_ENOUGH_MEM:
    DbgPrint("Not enough memory allocated for Read Capacity. Expected 0x%x, Allocated 0x%x\n", 
            expectedCap, pSrb->DataTransferLength);

    SetScsiSenseData(pSrb,
                    SCSISTAT_CHECK_CONDITION,
                    SCSI_SENSE_ILLEGAL_REQUEST,
                    SCSI_ADSENSE_INVALID_CDB,
                    SCSI_ADSENSE_NO_SENSE);
    pSrb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
    pSrb->DataTransferLength = 0;
    return FALSE;
}

BOOLEAN 
NVMeExecuteInquiry(PSCSI_REQUEST_BLOCK pSrb, PNVME_DEVICE_EXTENSION pDevExt)
{
    if (pSrb == NULL || pDevExt == NULL) {
        ASSERT(FALSE);
        return FALSE;
    }

    INQUIRYDATA InquiryData = {0};
    PCDB pInquiry = (PCDB)pSrb->Cdb;

    /* 
    * ROS: We dont support anything other than standard inquiry. Neither the 
    * EVPD inquiry pages or any other page code. Lets fail it here
    */
    if (pInquiry->CDB6INQUIRY3.PageCode || pInquiry->CDB6INQUIRY3.EnableVitalProductData) {
        DbgPrint("SCSI Inquiry Page Code 0x%x not supported, EVPD: 0x%x\n", 
                pInquiry->CDB6INQUIRY3.PageCode, pInquiry->CDB6INQUIRY3.EnableVitalProductData);
        SetScsiSenseData(pSrb,
                            SCSISTAT_CHECK_CONDITION,
                            SCSI_SENSE_ILLEGAL_REQUEST,
                            SCSI_ADSENSE_INVALID_CDB,
                            SCSI_ADSENSE_NO_SENSE);
        pSrb->SrbStatus |= SRB_STATUS_INVALID_REQUEST;
        pSrb->DataTransferLength = 0;
        return FALSE;
    }

    /* Not enough buffer? */
    if (sizeof(InquiryData) > pSrb->DataTransferLength) {
        DbgPrint("Not enough buffer allocated, failing the Inquiry command. Expected 0x%x, Passed 0x%x\n", sizeof(InquiryData), pSrb->DataTransferLength);
        SetScsiSenseData(pSrb,
                        SCSISTAT_CHECK_CONDITION,
                        SCSI_SENSE_ILLEGAL_REQUEST,
                        SCSI_ADSENSE_NO_SENSE,
                        SCSI_ADSENSE_NO_SENSE);
        pSrb->SrbStatus |= SRB_STATUS_INVALID_REQUEST;
        pSrb->DataTransferLength = 0;
        return FALSE; 
    }

    InquiryData.DeviceType          = DIRECT_ACCESS_DEVICE;
    InquiryData.DeviceTypeQualifier = DEVICE_CONNECTED;
    InquiryData.RemovableMedia      = UNREMOVABLE_MEDIA;
    InquiryData.Versions            = VERSION_SPC_4;
    InquiryData.NormACA             = ACA_UNSUPPORTED;
    InquiryData.HiSupport           = HIERARCHAL_ADDR_UNSUPPORTED;
    InquiryData.ResponseDataFormat  = RESPONSE_DATA_FORMAT_SPC_4;
    InquiryData.AdditionalLength    = ADDITIONAL_STD_INQ_LENGTH;
    InquiryData.EnclosureServices   = EMBEDDED_ENCLOSURE_SERVICES_UNSUPPORTED;
    InquiryData.MediumChanger       = MEDIUM_CHANGER_UNSUPPORTED;
    InquiryData.CommandQueue        = COMMAND_MANAGEMENT_MODEL;
    InquiryData.Wide16Bit           = WIDE_16_BIT_XFERS_UNSUPPORTED;
    InquiryData.Addr16              = WIDE_16_BIT_ADDRESES_UNSUPPORTED;
    InquiryData.Synchronous         = SYNCHRONOUS_DATA_XFERS_UNSUPPORTED;
    InquiryData.Reserved3[0]        = RESERVED_FIELD;

    /*
        *  Fields not defined in Standard Inquiry page from storport.h
        *
        *    - SCCS:    Embedded Storage Arrays
        *    - ACC:     Access Control Coordinator
        *    - TPGS:    Target Port Groupo Suppport
        *    - 3PC:     3rd Party Copy
        *    - Protect: LUN Protection Information
        *    - SPT:     Type of protection LUN supports
        */

    /* T10 Vendor Id */
    InquiryData.VendorId[BYTE_0] = 'N';
    InquiryData.VendorId[BYTE_1] = 'V';
    InquiryData.VendorId[BYTE_2] = 'M';
    InquiryData.VendorId[BYTE_3] = 'e';
    InquiryData.VendorId[BYTE_4] = ' ';
    InquiryData.VendorId[BYTE_5] = ' ';
    InquiryData.VendorId[BYTE_6] = ' ';
    InquiryData.VendorId[BYTE_7] = ' ';

    /* Product Id - First 16 bytes of model # in Controller Identify structure*/
    RtlCopyMemory(InquiryData.ProductId, pDevExt->controllerIdentifyData.MN, PRODUCT_ID_SIZE);

    /* Product Revision Level */
    RtlCopyMemory(InquiryData.ProductRevisionLevel, pDevExt->controllerIdentifyData.FR, PRODUCT_REVISION_LEVEL_SIZE);

    /* Lets copy output buffer */
    RtlCopyMemory(pSrb->DataBuffer, &InquiryData, sizeof(InquiryData));

    pSrb->DataTransferLength = min(pSrb->DataTransferLength, sizeof(InquiryData));

    pSrb->SrbStatus = SRB_STATUS_SUCCESS;

    return TRUE;
}

BOOLEAN 
NVMeExecuteRead(PSCSI_REQUEST_BLOCK pSrb, PNVME_DEVICE_EXTENSION pDevExt)
{
    if (pSrb == NULL || pDevExt == NULL) {
        ASSERT(FALSE);
        return FALSE;
    }

    PNVME_LUN_EXTENSION pLunExt = &pDevExt->NSInfo;
    NVMe_COMMAND nvmeCmd = {0};
    NVMe_COMPLETION_QUEUE_ENTRY compEntry = {0};
    UINT32 sLBA = 0;
    USHORT nLBA = 0;
    UINT16 transferLen = 0, minTLen = 0;
    UINT16 blockSize = pDevExt->NSInfo.BlockSize;
    PCDB pCdb  = (PCDB)pSrb->Cdb;

    /*
    * ROS: We shouldn't recieve any opcodes other than Read10 
    * but ideally we should implement them.
    * Doesn't make sense to wait for the FS to catch up
    */
    if (pSrb->Cdb[0] != SCSIOP_READ) {
        ASSERT(FALSE);
        SetScsiSenseData(pSrb,
                        SCSISTAT_CHECK_CONDITION,
                        SCSI_SENSE_ILLEGAL_REQUEST,
                        SCSI_ADSENSE_ILLEGAL_COMMAND,
                        SCSI_ADSENSE_NO_SENSE);
        pSrb->SrbStatus |= SRB_STATUS_INVALID_REQUEST;
        pSrb->DataTransferLength = 0;
        return FALSE;
    }

    /* Big Endidan -> Little Endian */
    REVERSE_BYTES(&sLBA, &pCdb->CDB10.LogicalBlockByte0);
    REVERSE_BYTES_SHORT(&transferLen, &pCdb->CDB10.TransferBlocksMsb);
 
    /* Calculate nLba. This is 0 based value */
    nLBA = transferLen/blockSize;
    if (transferLen % blockSize == 0) nLBA--;

    //DbgPrint("READ: TLen 0x%x, nLBA: 0x%x sLBA 0x%lx\n", transferLen, nLBA, sLBA);

    /* Bad buffer */
    minTLen = (nLBA + 1) * blockSize;
    if (pSrb->DataTransferLength < minTLen) {
        SetScsiSenseData(pSrb,
                        SCSISTAT_CHECK_CONDITION,
                        SCSI_SENSE_ILLEGAL_REQUEST,
                        SCSI_SENSE_NO_SENSE,
                        SCSI_ADSENSE_NO_SENSE);
        pSrb->SrbStatus |= SRB_STATUS_INVALID_REQUEST;
        pSrb->DataTransferLength = 0;
        return FALSE;
    }

    /*
    * Does SCSIPort honour our MAXxfer constraints?
    * Need to check if we need to split IO
    */
    if (transferLen > DFT_TX_SIZE) {
        DbgPrint("IO Greater than MaxXfersize. Need to split\n");
        ASSERT(FALSE);
        return FALSE;
    }

    if(pDevExt->QueueInfo.SubQueueInfo[1].CID == 0xFFFF)
        pDevExt->QueueInfo.SubQueueInfo[1].CID = 1;

    nvmeCmd.CDW0.OPC = NVME_READ;
    nvmeCmd.NSID = pLunExt->namespaceId;
    nvmeCmd.CDW10 = sLBA & 0xFFFF;
    ((PNVM_READ_COMMAND_DW12)&nvmeCmd.CDW12)->NLB = nLBA;

    if (!NvmeTranslateSglToPrp(pDevExt, pSrb, &nvmeCmd, minTLen))
        goto ERR_STATUS_BUSY;

    if (!NVMeIssueCmd(pDevExt, &nvmeCmd, &compEntry, TRUE))
        goto ERR_STATUS_BUSY;

    if (!NVMeIsCmdSuccessful(&compEntry))
        goto ERR_STATUS_BUSY;

    pSrb->SrbStatus = SRB_STATUS_SUCCESS;
    return TRUE;

ERR_STATUS_BUSY:
    /* Lets retry later */
    SetScsiSenseData(pSrb,
                    SCSISTAT_BUSY,
                    SCSI_ADSENSE_NO_SENSE,
                    SCSI_ADSENSE_NO_SENSE,
                    SCSI_ADSENSE_NO_SENSE);
    pSrb->SrbStatus |= SRB_STATUS_ERROR;
    return FALSE;
}

BOOLEAN 
NVMeExecuteModeSense(PSCSI_REQUEST_BLOCK pSrb, PNVME_DEVICE_EXTENSION pDevExt)
{
    if (pSrb == NULL || pDevExt == NULL)
        return FALSE;

    PCDB pCdb = (PCDB)pSrb->Cdb;

    /*
    * ROS: We shouldn't recieve any opcodes other than MODE_SENSE6 
    * but ideally we should implement M_S10 as well.
    * Doesn't make sense to wait for the FS to catch up
    */
    if (pCdb->MODE_SENSE.OperationCode != SCSIOP_MODE_SENSE) {
        ASSERT(FALSE);
        SetScsiSenseData(pSrb,
                        SCSISTAT_CHECK_CONDITION,
                        SCSI_SENSE_ILLEGAL_REQUEST,
                        SCSI_ADSENSE_ILLEGAL_COMMAND,
                        SCSI_ADSENSE_NO_SENSE);
        pSrb->SrbStatus |= SRB_STATUS_INVALID_REQUEST;
        pSrb->DataTransferLength = 0;
        return FALSE;        
    }

    switch(pCdb->MODE_SENSE10.PageCode) {
        case MODE_PAGE_CACHING: 
            NVMeExecuteModeSenseCaching(pSrb, pDevExt);
        break;
        default:
            SetScsiSenseData(pSrb,
                            SCSISTAT_CHECK_CONDITION,
                            SCSI_SENSE_ILLEGAL_REQUEST,
                            SCSI_ADSENSE_ILLEGAL_COMMAND,
                            SCSI_ADSENSE_NO_SENSE);
            pSrb->SrbStatus |= SRB_STATUS_INVALID_REQUEST;
            pSrb->DataTransferLength = 0;
            DbgPrint("Unimplemented pageCode 0x%x\n", pCdb->MODE_SENSE10.PageCode);
    }

    return TRUE;
}

BOOLEAN 
NVMeExecuteWrite(PSCSI_REQUEST_BLOCK pSrb, PNVME_DEVICE_EXTENSION pDevExt)
{
    if (pSrb == NULL || pDevExt == NULL) {
        ASSERT(FALSE);
        return FALSE;
    }

    PNVME_LUN_EXTENSION pLunExt = &pDevExt->NSInfo;
    NVMe_COMMAND nvmeCmd = {0};
    UINT32 sLBA          = 0;
    UINT16 transferLen   = 0;
    PCDB pCdb            = (PCDB)pSrb->Cdb;
    USHORT nLBA          = 0;
    NVMe_COMPLETION_QUEUE_ENTRY compEntry = {0};
    UINT16 blockSize     = pDevExt->NSInfo.BlockSize;
    ULONG minTLen        = 0;

    /*
    * ROS: We ideally shouldn't recieve any opcodes other than Write10 
    * but ideally we should implement them.
    * Doesn't make sense to wait for the FS to catch up
    */
    if (pSrb->Cdb[0] != SCSIOP_WRITE) {
        ASSERT(FALSE);
        SetScsiSenseData(pSrb,
                        SCSISTAT_CHECK_CONDITION,
                        SCSI_SENSE_ILLEGAL_REQUEST,
                        SCSI_ADSENSE_ILLEGAL_COMMAND,
                        SCSI_ADSENSE_NO_SENSE);
        pSrb->SrbStatus |= SRB_STATUS_INVALID_REQUEST;
        pSrb->DataTransferLength = 0;
        return FALSE;
    }

    /* Big Endidan -> Little Endian */
    REVERSE_BYTES(&sLBA, &pCdb->CDB10.LogicalBlockByte0);
    REVERSE_BYTES_SHORT(&transferLen, &pCdb->CDB10.TransferBlocksMsb);

    /* Calculate nLba. This is 0 based value */
    nLBA = transferLen/blockSize;
    if (transferLen % blockSize == 0) nLBA--;

    //DbgPrint("WRITE: TLen 0x%x, nLBA: 0x%x sLBA 0x%lx\n", transferLen, nLBA, sLBA);

    /* Bad buffer */
    minTLen = (nLBA + 1) * blockSize;
    if (pSrb->DataTransferLength < minTLen) {
        SetScsiSenseData(pSrb,
                        SCSISTAT_CHECK_CONDITION,
                        SCSI_SENSE_ILLEGAL_REQUEST,
                        SCSI_ADSENSE_PARAMETER_LIST_LENGTH,
                        SCSI_ADSENSE_NO_SENSE);
        pSrb->SrbStatus |= SRB_STATUS_INVALID_REQUEST;
        pSrb->DataTransferLength = 0;
        return FALSE;
    }
    /*
    * Does SCSIPort honour our MAXxfer constraints?
    * Need to check if we need to split IO
    */
    if (transferLen > DFT_TX_SIZE) {
        DbgPrint("IO Greater than MaxXfersize. Need to split\n");
        ASSERT(FALSE);
        return FALSE;
    }

    if(pDevExt->QueueInfo.SubQueueInfo[1].CID == 0xFFFF)
        pDevExt->QueueInfo.SubQueueInfo[1].CID = 1;

    nvmeCmd.CDW0.OPC = NVME_WRITE;
    nvmeCmd.NSID     = pLunExt->namespaceId;
    nvmeCmd.CDW10    = sLBA & 0xFFFF;
    //nvmeCmd.CDW11 = (sLBA  & 0xFFFF0000) >> 32;
    ((PNVM_READ_COMMAND_DW12)&nvmeCmd.CDW12)->NLB = nLBA;

    if (!NvmeTranslateSglToPrp(pDevExt, pSrb, &nvmeCmd, (nLBA + 1) * blockSize))
        goto ERR_STATUS_BUSY;

    if (!NVMeIssueCmd(pDevExt, &nvmeCmd, &compEntry, TRUE))
        goto ERR_STATUS_BUSY;

    if (!NVMeIsCmdSuccessful(&compEntry))
        goto ERR_STATUS_BUSY;

    pSrb->SrbStatus = SRB_STATUS_SUCCESS;
    return TRUE;

ERR_STATUS_BUSY:
    /* Lets retry later */
    SetScsiSenseData(pSrb,
                    SCSISTAT_BUSY,
                    SCSI_ADSENSE_NO_SENSE,
                    SCSI_ADSENSE_NO_SENSE,
                    SCSI_ADSENSE_NO_SENSE);
    pSrb->SrbStatus |= SRB_STATUS_ERROR;
    return FALSE;
}

BOOLEAN 
NVMeExecuteTestUnitRead(PSCSI_REQUEST_BLOCK pSrb, PNVME_DEVICE_EXTENSION pDevExt)
{
    if (pSrb == NULL || pDevExt == NULL)
        return FALSE;
    
    if (pDevExt->NvmeCntrlState != NVMeWaitOnRDY || pDevExt->NSInfo.nsReady != TRUE) 
    {
        SetScsiSenseData(pSrb,
                        SCSISTAT_CHECK_CONDITION,
                        SCSI_SENSE_NOT_READY,
                        SCSI_ADSENSE_LUN_NOT_READY,
                        SCSI_ADSENSE_NO_SENSE);
        pSrb->SrbStatus |= SRB_STATUS_ERROR;
        return FALSE;
    }

    pSrb->SrbStatus = SRB_STATUS_SUCCESS;
    pSrb->DataTransferLength = 0;
    return TRUE;
}

VOID
NVMeExecuteSrb(PSCSI_REQUEST_BLOCK pSrb, PNVME_DEVICE_EXTENSION pDevExt)
{
    if (pSrb == NULL || pDevExt == NULL)
        return;

    /*
    * Follwing commands are supported under ROS:
    * SCSIOP_INQUIRY
    * SCSIOP_READ_CAPACITY
    * SCSIOP_READ10
    * SCSIOP_WRITE10
    * SCSIOP_MODE_SENSE
    * SCSIOP_TEST_UNIT_READY
    */ 
    switch (pSrb->Cdb[0]) {
        case SCSIOP_INQUIRY:
            NVMeExecuteInquiry(pSrb, pDevExt);
            break;

        case SCSIOP_READ_CAPACITY:
        case SCSIOP_READ_CAPACITY16:
            NVMeExecuteReadCapacity(pSrb, pDevExt);
            break;

        case SCSIOP_READ:
        case SCSIOP_READ6:
        case SCSIOP_READ12:
        case SCSIOP_READ16:
            NVMeExecuteRead(pSrb, pDevExt);
            break;

        case SCSIOP_WRITE:
        case SCSIOP_WRITE6:
        case SCSIOP_WRITE12:
        case SCSIOP_WRITE16:
            NVMeExecuteWrite(pSrb, pDevExt);
            break;

        case SCSIOP_MODE_SENSE:
        case SCSIOP_MODE_SENSE10:
            NVMeExecuteModeSense(pSrb, pDevExt);
            break;

        case SCSIOP_TEST_UNIT_READY:
            NVMeExecuteTestUnitRead(pSrb, pDevExt);
            break;

        default:
            pSrb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
            DbgPrint("Unsupported CDB 0x%x\n", pSrb->Cdb[0]);
    }
}