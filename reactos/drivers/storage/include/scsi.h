/* $Id: scsi.h,v 1.1 2001/07/23 06:12:34 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/include/scsi.h
 * PURPOSE:         SCSI class driver definitions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

#ifndef __STORAGE_INCLUDE_SCSI_H
#define __STORAGE_INCLUDE_SCSI_H


typedef struct _INQUIRYDATA
{
  UCHAR DeviceType:5;
  UCHAR DeviceTypeQualifier:3;
  UCHAR DeviceTypeModifier:7;
  UCHAR RemovableMedia:1;
  UCHAR Versions;
  UCHAR ResponseDataFormat;
  UCHAR AdditionalLength;
  UCHAR Reserved[2];
  UCHAR SoftReset:1;
  UCHAR CommandQueue:1;
  UCHAR Reserved2:1;
  UCHAR LinkedCommands:1;
  UCHAR Synchronous:1;
  UCHAR Wide16Bit:1;
  UCHAR Wide32Bit:1;
  UCHAR RelativeAddressing:1;
  UCHAR VendorId[8];
  UCHAR ProductId[16];
  UCHAR ProductRevisionLevel[4];
  UCHAR VendorSpecific[20];
  UCHAR Reserved3[40];
} INQUIRYDATA, *PINQUIRYDATA;


#endif /* __STORAGE_INCLUDE_SCSI_H */

/* EOF */