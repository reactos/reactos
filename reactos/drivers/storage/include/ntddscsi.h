/* $Id: ntddscsi.h,v 1.1 2001/07/23 06:12:34 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/include/ntddscsi.h
 * PURPOSE:         Basic SCSI definitions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

#ifndef __STORAGE_INCLUDE_NTDDSCSI_H
#define __STORAGE_INCLUDE_NTDDSCSI_H


typedef struct _SCSI_BUS_DATA
{
  UCHAR NumberOfLogicalUnits;
  UCHAR InitiatorBusId;
  ULONG InquiryDataOffset;
}SCSI_BUS_DATA, *PSCSI_BUS_DATA;


typedef struct _SCSI_ADAPTER_BUS_INFO
{
  UCHAR NumberOfBuses;
  SCSI_BUS_DATA BusData[1];
} SCSI_ADAPTER_BUS_INFO, *PSCSI_ADAPTER_BUS_INFO;


typedef struct _IO_SCSI_CAPABILITIES
{
  ULONG Length;
  ULONG MaximumTransferLength;
  ULONG MaximumPhysicalPages;
  ULONG SupportedAsynchronousEvents;
  ULONG AlignmentMask;
  BOOLEAN TaggedQueuing;
  BOOLEAN AdapterScansDown;
  BOOLEAN AdapterUsesPio;
} IO_SCSI_CAPABILITIES, *PIO_SCSI_CAPABILITIES;


typedef struct _SCSI_INQUIRY_DATA
{
  UCHAR PathId;
  UCHAR TargetId;
  UCHAR Lun;
  BOOLEAN DeviceClaimed;
  ULONG InquiryDataLength;
  ULONG NextInquiryDataOffset;
  UCHAR InquiryData[1];
}SCSI_INQUIRY_DATA, *PSCSI_INQUIRY_DATA;


#endif /* __STORAGE_INCLUDE_NTDDSCSI_H */

/* EOF */