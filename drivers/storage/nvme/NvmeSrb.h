#ifndef __NVME_SRB_H__
#define __NVME_SRB_H__

VOID 
SetScsiSenseData(PSCSI_REQUEST_BLOCK pSrb, UCHAR scsiStatus, UCHAR senseKey, UCHAR asc, UCHAR ascq);

VOID
NVMeExecuteSrb(PSCSI_REQUEST_BLOCK Srb, PNVME_DEVICE_EXTENSION pDevExt);
#endif