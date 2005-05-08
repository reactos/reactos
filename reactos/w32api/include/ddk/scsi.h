/*
 * scsi.h
 *
 * SCSI port and class interface.
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __SCSI_H
#define __SCSI_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"

typedef union _CDB {
  struct _CDB6GENERIC {
    UCHAR  OperationCode;
    UCHAR  Immediate : 1;
    UCHAR  CommandUniqueBits : 4;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  CommandUniqueBytes[3];
    UCHAR  Link : 1;
    UCHAR  Flag : 1;
    UCHAR  Reserved : 4;
    UCHAR  VendorUnique : 2;
  } CDB6GENERIC, *PCDB6GENERIC;
  
  struct _CDB6READWRITE {
    UCHAR  OperationCode;
    UCHAR  LogicalBlockMsb1 : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  LogicalBlockMsb0;
    UCHAR  LogicalBlockLsb;
    UCHAR  TransferBlocks;
    UCHAR  Control;
  } CDB6READWRITE, *PCDB6READWRITE;
  
  struct _CDB6INQUIRY {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  PageCode;
    UCHAR  IReserved;
    UCHAR  AllocationLength;
    UCHAR  Control;
  } CDB6INQUIRY, *PCDB6INQUIRY;
  
  struct _CDB6INQUIRY3 {
    UCHAR  OperationCode;
    UCHAR  EnableVitalProductData : 1;
    UCHAR  CommandSupportData : 1;
    UCHAR  Reserved1 : 6;
    UCHAR  PageCode;
    UCHAR  Reserved2;
    UCHAR  AllocationLength;
    UCHAR  Control;
  } CDB6INQUIRY3, *PCDB6INQUIRY3;
  
  struct _CDB6VERIFY {
    UCHAR  OperationCode;
    UCHAR  Fixed : 1;
    UCHAR  ByteCompare : 1;
    UCHAR  Immediate : 1;
    UCHAR  Reserved : 2;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  VerificationLength[3];
    UCHAR  Control;
  } CDB6VERIFY, *PCDB6VERIFY;
  
  struct _CDB6FORMAT {
    UCHAR  OperationCode;
    UCHAR  FormatControl : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  FReserved1;
    UCHAR  InterleaveMsb;
    UCHAR  InterleaveLsb;
    UCHAR  FReserved2;
  } CDB6FORMAT, *PCDB6FORMAT;
  
  struct _CDB10 {
    UCHAR  OperationCode;
    UCHAR  RelativeAddress : 1;
    UCHAR  Reserved1 : 2;
    UCHAR  ForceUnitAccess : 1;
    UCHAR  DisablePageOut : 1;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  LogicalBlockByte0;
    UCHAR  LogicalBlockByte1;
    UCHAR  LogicalBlockByte2;
    UCHAR  LogicalBlockByte3;
    UCHAR  Reserved2;
    UCHAR  TransferBlocksMsb;
    UCHAR  TransferBlocksLsb;
    UCHAR  Control;
  } CDB10, *PCDB10;
  
  struct _CDB12 {
    UCHAR  OperationCode;
    UCHAR  RelativeAddress : 1;
    UCHAR  Reserved1 : 2;
    UCHAR  ForceUnitAccess : 1;
    UCHAR  DisablePageOut : 1;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  LogicalBlock[4];
    UCHAR  TransferLength[4];
    UCHAR  Reserved2;
    UCHAR  Control;
  } CDB12, *PCDB12;
  
  struct _PAUSE_RESUME {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  Reserved2[6];
    UCHAR  Action;
    UCHAR  Control;
  } PAUSE_RESUME, *PPAUSE_RESUME;
  
  struct _READ_TOC {
    UCHAR  OperationCode;
    UCHAR  Reserved0 : 1;
    UCHAR  Msf : 1;
    UCHAR  Reserved1 : 3;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  Format2 : 4;
    UCHAR  Reserved2 : 4;
    UCHAR  Reserved3[3];
    UCHAR  StartingTrack;
    UCHAR  AllocationLength[2];
    UCHAR  Control : 6;
    UCHAR  Format : 2;
  } READ_TOC, *PREAD_TOC;
  
  struct _READ_DISK_INFORMATION {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  Lun : 3;
    UCHAR  Reserved2[5];
    UCHAR  AllocationLength[2];
    UCHAR  Control;
  } READ_DISK_INFORMATION, *PREAD_DISK_INFORMATION;
  
  struct _READ_TRACK_INFORMATION {
    UCHAR  OperationCode;
    UCHAR  Track : 1;
    UCHAR  Reserved1 : 3;
    UCHAR  Reserved2 : 1;
    UCHAR  Lun : 3;
    UCHAR  BlockAddress[4];
    UCHAR  Reserved3;
    UCHAR  AllocationLength[2];
    UCHAR  Control;
  } READ_TRACK_INFORMATION, *PREAD_TRACK_INFORMATION;
  
  struct _RESERVE_TRACK_RZONE {
    UCHAR  OperationCode;
    UCHAR  Reserved1[4];
    UCHAR  ReservationSize[4];
    UCHAR  Control;
  } RESERVE_TRACK_RZONE, *PRESERVE_TRACK_RZONE;
  
  struct _SEND_OPC_INFORMATION {
    UCHAR  OperationCode;
    UCHAR  DoOpc    : 1;
    UCHAR  Reserved : 7;
    UCHAR  Reserved1[5];
    UCHAR  ParameterListLength[2];
    UCHAR  Reserved2;
  } SEND_OPC_INFORMATION, *PSEND_OPC_INFORMATION;
  
  struct _CLOSE_TRACK {
    UCHAR  OperationCode;
    UCHAR  Immediate : 1;
    UCHAR  Reserved1 : 7;
    UCHAR  Track     : 1;
    UCHAR  Session   : 1;
    UCHAR  Reserved2 : 6;
    UCHAR  Reserved3;
    UCHAR  TrackNumber[2];
    UCHAR  Reserved4[3];
    UCHAR  Control;
  } CLOSE_TRACK, *PCLOSE_TRACK;
  
  struct _SEND_CUE_SHEET {
    UCHAR  OperationCode;
    UCHAR  Reserved[5];
    UCHAR  CueSheetSize[3];
    UCHAR  Control;
  } SEND_CUE_SHEET, *PSEND_CUE_SHEET;
  
  struct _READ_HEADER {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 1;
    UCHAR  Msf : 1;
    UCHAR  Reserved2 : 3;
    UCHAR  Lun : 3;
    UCHAR  LogicalBlockAddress[4];
    UCHAR  Reserved3;
    UCHAR  AllocationLength[2];
    UCHAR  Control;
  } READ_HEADER, *PREAD_HEADER;
  
  struct _PLAY_AUDIO {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  StartingBlockAddress[4];
    UCHAR  Reserved2;
    UCHAR  PlayLength[2];
    UCHAR  Control;
  } PLAY_AUDIO, *PPLAY_AUDIO;
  
  struct _PLAY_AUDIO_MSF { 
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  Reserved2;
    UCHAR  StartingM;
    UCHAR  StartingS;
    UCHAR  StartingF;
    UCHAR  EndingM;
    UCHAR  EndingS;
    UCHAR  EndingF;
    UCHAR  Control;
  } PLAY_AUDIO_MSF, *PPLAY_AUDIO_MSF;
  
/* FIXME: Should the union be anonymous in C++ too?  If so,
   can't define named types _LBA and _MSF within anonymous union
   for C++. */ 	
  struct _PLAY_CD {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 1;
    UCHAR  CMSF : 1;
    UCHAR  ExpectedSectorType : 3;
    UCHAR  Lun : 3;
#ifndef __cplusplus
  _ANONYMOUS_UNION
#endif
  union {
      struct _LBA {
            UCHAR  StartingBlockAddress[4];
            UCHAR  PlayLength[4];
      } LBA;
  
      struct _MSF {
            UCHAR  Reserved1;
            UCHAR  StartingM;
            UCHAR  StartingS;
            UCHAR  StartingF;
            UCHAR  EndingM;
            UCHAR  EndingS;
            UCHAR  EndingF;
            UCHAR  Reserved2;
      } MSF;
  #ifndef __cplusplus
  }DUMMYUNIONNAME;
  #else
  }u;
  #endif
  
    UCHAR  Audio : 1;
    UCHAR  Composite : 1;
    UCHAR  Port1 : 1;
    UCHAR  Port2 : 1;
    UCHAR  Reserved2 : 3;
    UCHAR  Speed : 1;
    UCHAR  Control;
  } PLAY_CD, *PPLAY_CD;
  
  struct _SCAN_CD {
    UCHAR  OperationCode;
    UCHAR  RelativeAddress : 1;
    UCHAR  Reserved1 : 3;
    UCHAR  Direct : 1;
    UCHAR  Lun : 3;
    UCHAR  StartingAddress[4];
    UCHAR  Reserved2[3];
    UCHAR  Reserved3 : 6;
    UCHAR  Type : 2;
    UCHAR  Reserved4;
    UCHAR  Control;
  } SCAN_CD, *PSCAN_CD;
  
  struct _STOP_PLAY_SCAN {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  Lun : 3;
    UCHAR  Reserved2[7];
    UCHAR  Control;
  } STOP_PLAY_SCAN, *PSTOP_PLAY_SCAN;
  
  struct _SUBCHANNEL {
    UCHAR  OperationCode;
    UCHAR  Reserved0 : 1;
    UCHAR  Msf : 1;
    UCHAR  Reserved1 : 3;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  Reserved2 : 6;
    UCHAR  SubQ : 1;
    UCHAR  Reserved3 : 1;
    UCHAR  Format;
    UCHAR  Reserved4[2];
    UCHAR  TrackNumber;
    UCHAR  AllocationLength[2];
    UCHAR  Control;
  } SUBCHANNEL, *PSUBCHANNEL;
  
  struct _READ_CD { 
    UCHAR  OperationCode;
    UCHAR  RelativeAddress : 1;
    UCHAR  Reserved0 : 1;
    UCHAR  ExpectedSectorType : 3;
    UCHAR  Lun : 3;
    UCHAR  StartingLBA[4];
    UCHAR  TransferBlocks[3];
    UCHAR  Reserved2 : 1;
    UCHAR  ErrorFlags : 2;
    UCHAR  IncludeEDC : 1;
    UCHAR  IncludeUserData : 1;
    UCHAR  HeaderCode : 2;
    UCHAR  IncludeSyncData : 1;
    UCHAR  SubChannelSelection : 3;
    UCHAR  Reserved3 : 5;
    UCHAR  Control;
  } READ_CD, *PREAD_CD;
  
  struct _READ_CD_MSF {
    UCHAR  OperationCode;
    UCHAR  RelativeAddress : 1;
    UCHAR  Reserved1 : 1;
    UCHAR  ExpectedSectorType : 3;
    UCHAR  Lun : 3;
    UCHAR  Reserved2;
    UCHAR  StartingM;
    UCHAR  StartingS;
    UCHAR  StartingF;
    UCHAR  EndingM;
    UCHAR  EndingS;
    UCHAR  EndingF;
    UCHAR  Reserved3;
    UCHAR  Reserved4 : 1;
    UCHAR  ErrorFlags : 2;
    UCHAR  IncludeEDC : 1;
    UCHAR  IncludeUserData : 1;
    UCHAR  HeaderCode : 2;
    UCHAR  IncludeSyncData : 1;
    UCHAR  SubChannelSelection : 3;
    UCHAR  Reserved5 : 5;
    UCHAR  Control;
  } READ_CD_MSF, *PREAD_CD_MSF;
  
  struct _PLXTR_READ_CDDA {
    UCHAR  OperationCode;
    UCHAR  Reserved0 : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  LogicalBlockByte0;
    UCHAR  LogicalBlockByte1;
    UCHAR  LogicalBlockByte2;
    UCHAR  LogicalBlockByte3;
    UCHAR  TransferBlockByte0;
    UCHAR  TransferBlockByte1;
    UCHAR  TransferBlockByte2;
    UCHAR  TransferBlockByte3;
    UCHAR  SubCode;
    UCHAR  Control;
  } PLXTR_READ_CDDA, *PPLXTR_READ_CDDA;
  
  struct _NEC_READ_CDDA {
    UCHAR  OperationCode;
    UCHAR  Reserved0;
    UCHAR  LogicalBlockByte0;
    UCHAR  LogicalBlockByte1;
    UCHAR  LogicalBlockByte2;
    UCHAR  LogicalBlockByte3;
    UCHAR  Reserved1;
    UCHAR  TransferBlockByte0;
    UCHAR  TransferBlockByte1;
    UCHAR  Control;
  } NEC_READ_CDDA, *PNEC_READ_CDDA;
  
  struct _MODE_SENSE {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 3;
    UCHAR  Dbd : 1;
    UCHAR  Reserved2 : 1;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  PageCode : 6;
    UCHAR  Pc : 2;
    UCHAR  Reserved3;
    UCHAR  AllocationLength;
    UCHAR  Control;
  } MODE_SENSE, *PMODE_SENSE;
  
  struct _MODE_SENSE10 {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 3;
    UCHAR  Dbd : 1;
    UCHAR  Reserved2 : 1;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  PageCode : 6;
    UCHAR  Pc : 2;
    UCHAR  Reserved3[4];
    UCHAR  AllocationLength[2];
    UCHAR  Control;
  } MODE_SENSE10, *PMODE_SENSE10;
  
  struct _MODE_SELECT {
    UCHAR  OperationCode;
    UCHAR  SPBit : 1;
    UCHAR  Reserved1 : 3;
    UCHAR  PFBit : 1;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  Reserved2[2];
    UCHAR  ParameterListLength;
    UCHAR  Control;
  } MODE_SELECT, *PMODE_SELECT;
  
  struct _MODE_SELECT10 {
    UCHAR  OperationCode;
    UCHAR  SPBit : 1;
    UCHAR  Reserved1 : 3;
    UCHAR  PFBit : 1;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  Reserved2[5];
    UCHAR  ParameterListLength[2];
    UCHAR  Control;
  } MODE_SELECT10, *PMODE_SELECT10;
  
  struct _LOCATE {
    UCHAR  OperationCode;
    UCHAR  Immediate : 1;
    UCHAR  CPBit : 1;
    UCHAR  BTBit : 1;
    UCHAR  Reserved1 : 2;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  Reserved3;
    UCHAR  LogicalBlockAddress[4];
    UCHAR  Reserved4;
    UCHAR  Partition;
    UCHAR  Control;
  } LOCATE, *PLOCATE;
  
  struct _LOGSENSE {
    UCHAR  OperationCode;
    UCHAR  SPBit : 1;
    UCHAR  PPCBit : 1;
    UCHAR  Reserved1 : 3;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  PageCode : 6;
    UCHAR  PCBit : 2;
    UCHAR  Reserved2;
    UCHAR  Reserved3;
    UCHAR  ParameterPointer[2];
    UCHAR  AllocationLength[2];
    UCHAR  Control;
  } LOGSENSE, *PLOGSENSE;
  
  struct _LOGSELECT {
    UCHAR  OperationCode;
    UCHAR  SPBit : 1;
    UCHAR  PCRBit : 1;
    UCHAR  Reserved1 : 3;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  Reserved : 6;
    UCHAR  PCBit : 2;
    UCHAR  Reserved2[4];
    UCHAR  ParameterListLength[2];
    UCHAR  Control;
  } LOGSELECT, *PLOGSELECT;
  
  struct _PRINT {
    UCHAR  OperationCode;
    UCHAR  Reserved : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  TransferLength[3];
    UCHAR  Control;
  } PRINT, *PPRINT;
  
  struct _SEEK {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  LogicalBlockAddress[4];
    UCHAR  Reserved2[3];
    UCHAR  Control;
  } SEEK, *PSEEK;
  
  struct _ERASE {
    UCHAR  OperationCode;
    UCHAR  Long : 1;
    UCHAR  Immediate : 1;
    UCHAR  Reserved1 : 3;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  Reserved2[3];
    UCHAR  Control;
  } ERASE, *PERASE;
  
  struct _START_STOP {
    UCHAR  OperationCode;
    UCHAR  Immediate: 1;
    UCHAR  Reserved1 : 4;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  Reserved2[2];
    UCHAR  Start : 1;
    UCHAR  LoadEject : 1;
    UCHAR  Reserved3 : 6;
    UCHAR  Control;
  } START_STOP, *PSTART_STOP;
  
  struct _MEDIA_REMOVAL {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  Reserved2[2];
  
    UCHAR  Prevent : 1;
    UCHAR  Persistant : 1;
    UCHAR  Reserved3 : 6;
  
    UCHAR  Control;
  } MEDIA_REMOVAL, *PMEDIA_REMOVAL;
  
  struct _SEEK_BLOCK {
    UCHAR  OperationCode;
    UCHAR  Immediate : 1;
    UCHAR  Reserved1 : 7;
    UCHAR  BlockAddress[3];
    UCHAR  Link : 1;
    UCHAR  Flag : 1;
    UCHAR  Reserved2 : 4;
    UCHAR  VendorUnique : 2;
  } SEEK_BLOCK, *PSEEK_BLOCK;
  
  struct _REQUEST_BLOCK_ADDRESS {
    UCHAR  OperationCode;
    UCHAR  Reserved1[3];
    UCHAR  AllocationLength;
    UCHAR  Link : 1;
    UCHAR  Flag : 1;
    UCHAR  Reserved2 : 4;
    UCHAR  VendorUnique : 2;
  } REQUEST_BLOCK_ADDRESS, *PREQUEST_BLOCK_ADDRESS;
  
  struct _PARTITION {
    UCHAR  OperationCode;
    UCHAR  Immediate : 1;
    UCHAR  Sel: 1;
    UCHAR  PartitionSelect : 6;
    UCHAR  Reserved1[3];
    UCHAR  Control;
  } PARTITION, *PPARTITION;
  
  struct _WRITE_TAPE_MARKS {
    UCHAR  OperationCode;
    UCHAR  Immediate : 1;
    UCHAR  WriteSetMarks: 1;
    UCHAR  Reserved : 3;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  TransferLength[3];
    UCHAR  Control;
  } WRITE_TAPE_MARKS, *PWRITE_TAPE_MARKS;
  
  struct _SPACE_TAPE_MARKS {
    UCHAR  OperationCode;
    UCHAR  Code : 3;
    UCHAR  Reserved : 2;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  NumMarksMSB ;
    UCHAR  NumMarks;
    UCHAR  NumMarksLSB;
  union {
        UCHAR  value;
      struct {
            UCHAR  Link : 1;
            UCHAR  Flag : 1;
            UCHAR  Reserved : 4;
            UCHAR  VendorUnique : 2;
      } Fields;
  } Byte6;
  } SPACE_TAPE_MARKS, *PSPACE_TAPE_MARKS;
  
  struct _READ_POSITION {
    UCHAR  Operation;
    UCHAR  BlockType : 1;
    UCHAR  Reserved1 : 4;
    UCHAR  Lun : 3;
    UCHAR  Reserved2[7];
    UCHAR  Control;
  } READ_POSITION, *PREAD_POSITION;
  
  struct _CDB6READWRITETAPE {
    UCHAR  OperationCode;
    UCHAR  VendorSpecific : 5;
    UCHAR  Reserved : 3;
    UCHAR  TransferLenMSB;
    UCHAR  TransferLen;
    UCHAR  TransferLenLSB;
    UCHAR  Link : 1;
    UCHAR  Flag : 1;
    UCHAR  Reserved1 : 4;
    UCHAR  VendorUnique : 2;
  } CDB6READWRITETAPE, *PCDB6READWRITETAPE;
  
  struct _INIT_ELEMENT_STATUS {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  LogicalUnitNubmer : 3;
    UCHAR  Reserved2[3];
    UCHAR  Reserved3 : 7;
    UCHAR  NoBarCode : 1;
  } INIT_ELEMENT_STATUS, *PINIT_ELEMENT_STATUS;
  
  struct _INITIALIZE_ELEMENT_RANGE {
    UCHAR  OperationCode;
    UCHAR  Range : 1;
    UCHAR  Reserved1 : 4;
    UCHAR  LogicalUnitNubmer : 3;
    UCHAR  FirstElementAddress[2];
    UCHAR  Reserved2[2];
    UCHAR  NumberOfElements[2];
    UCHAR  Reserved3;
    UCHAR  Reserved4 : 7;
    UCHAR  NoBarCode : 1;
  } INITIALIZE_ELEMENT_RANGE, *PINITIALIZE_ELEMENT_RANGE;
  
  struct _POSITION_TO_ELEMENT {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  TransportElementAddress[2];
    UCHAR  DestinationElementAddress[2];
    UCHAR  Reserved2[2];
    UCHAR  Flip : 1;
    UCHAR  Reserved3 : 7;
    UCHAR  Control;
  } POSITION_TO_ELEMENT, *PPOSITION_TO_ELEMENT;
  
  struct _MOVE_MEDIUM {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  TransportElementAddress[2];
    UCHAR  SourceElementAddress[2];
    UCHAR  DestinationElementAddress[2];
    UCHAR  Reserved2[2];
    UCHAR  Flip : 1;
    UCHAR  Reserved3 : 7;
    UCHAR  Control;
  } MOVE_MEDIUM, *PMOVE_MEDIUM;
  
  struct _EXCHANGE_MEDIUM {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  TransportElementAddress[2];
    UCHAR  SourceElementAddress[2];
    UCHAR  Destination1ElementAddress[2];
    UCHAR  Destination2ElementAddress[2];
    UCHAR  Flip1 : 1;
    UCHAR  Flip2 : 1;
    UCHAR  Reserved3 : 6;
    UCHAR  Control;
  } EXCHANGE_MEDIUM, *PEXCHANGE_MEDIUM;
  
  struct _READ_ELEMENT_STATUS {
    UCHAR  OperationCode;
    UCHAR  ElementType : 4;
    UCHAR  VolTag : 1;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  StartingElementAddress[2];
    UCHAR  NumberOfElements[2];
    UCHAR  Reserved1;
    UCHAR  AllocationLength[3];
    UCHAR  Reserved2;
    UCHAR  Control;
  } READ_ELEMENT_STATUS, *PREAD_ELEMENT_STATUS;
  
  struct _SEND_VOLUME_TAG {
    UCHAR  OperationCode;
    UCHAR  ElementType : 4;
    UCHAR  Reserved1 : 1;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  StartingElementAddress[2];
    UCHAR  Reserved2;
    UCHAR  ActionCode : 5;
    UCHAR  Reserved3 : 3;
    UCHAR  Reserved4[2];
    UCHAR  ParameterListLength[2];
    UCHAR  Reserved5;
    UCHAR  Control;
  } SEND_VOLUME_TAG, *PSEND_VOLUME_TAG;
  
  struct _REQUEST_VOLUME_ELEMENT_ADDRESS {
    UCHAR  OperationCode;
    UCHAR  ElementType : 4;
    UCHAR  VolTag : 1;
    UCHAR  LogicalUnitNumber : 3;
    UCHAR  StartingElementAddress[2];
    UCHAR  NumberElements[2];
    UCHAR  Reserved1;
    UCHAR  AllocationLength[3];
    UCHAR  Reserved2;
    UCHAR  Control;
  } REQUEST_VOLUME_ELEMENT_ADDRESS, *PREQUEST_VOLUME_ELEMENT_ADDRESS;
  
  struct _LOAD_UNLOAD {
    UCHAR  OperationCode;
    UCHAR  Immediate : 1;
    UCHAR  Reserved1 : 4;
    UCHAR  Lun : 3;
    UCHAR  Reserved2[2];
    UCHAR  Start : 1;
    UCHAR  LoadEject : 1;
    UCHAR  Reserved3: 6;
    UCHAR  Reserved4[3];
    UCHAR  Slot;
    UCHAR  Reserved5[3];
  } LOAD_UNLOAD, *PLOAD_UNLOAD;
  
  struct _MECH_STATUS {
    UCHAR  OperationCode;
    UCHAR  Reserved : 5;
    UCHAR  Lun : 3;
    UCHAR  Reserved1[6];
    UCHAR  AllocationLength[2];
    UCHAR  Reserved2[1];
    UCHAR  Control;
  } MECH_STATUS, *PMECH_STATUS;
  
  struct _SYNCHRONIZE_CACHE10 {
  
    UCHAR  OperationCode;
  
    UCHAR  RelAddr : 1;
    UCHAR  Immediate : 1;
    UCHAR  Reserved : 3;
    UCHAR  Lun : 3;
  
    UCHAR  LogicalBlockAddress[4];
    UCHAR  Reserved2;
    UCHAR  BlockCount[2];
    UCHAR  Control;
  } SYNCHRONIZE_CACHE10, *PSYNCHRONIZE_CACHE10;
  
  struct _GET_EVENT_STATUS_NOTIFICATION {
    UCHAR  OperationCode;
  
    UCHAR  Immediate : 1;
    UCHAR  Reserved : 4;
    UCHAR  Lun : 3;
  
    UCHAR  Reserved2[2];
    UCHAR  NotificationClassRequest;
    UCHAR  Reserved3[2];
    UCHAR  EventListLength[2];
  
    UCHAR  Control;
  } GET_EVENT_STATUS_NOTIFICATION, *PGET_EVENT_STATUS_NOTIFICATION;
  
  struct _READ_DVD_STRUCTURE {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  Lun : 3;
    UCHAR  RMDBlockNumber[4];
    UCHAR  LayerNumber;
    UCHAR  Format;
    UCHAR  AllocationLength[2];
    UCHAR  Reserved3 : 6;
    UCHAR  AGID : 2;
    UCHAR  Control;
  } READ_DVD_STRUCTURE, *PREAD_DVD_STRUCTURE;
  
  struct _SEND_DVD_STRUCTURE {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  Lun : 3;
    UCHAR  Reserved2[5];
    UCHAR  Format;
    UCHAR  ParameterListLength[2];
    UCHAR  Reserved3;
    UCHAR  Control;
  } SEND_DVD_STRUCTURE, *PSEND_DVD_STRUCTURE;
  
  struct _SEND_KEY {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  Lun : 3;
    UCHAR  Reserved2[6];
    UCHAR  ParameterListLength[2];
    UCHAR  KeyFormat : 6;
    UCHAR  AGID : 2;
    UCHAR  Control;
  } SEND_KEY, *PSEND_KEY;
  
  struct _REPORT_KEY {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  Lun : 3;
    UCHAR  LogicalBlockAddress[4];
    UCHAR  Reserved2[2];
    UCHAR  AllocationLength[2];
    UCHAR  KeyFormat : 6;
    UCHAR  AGID : 2;
    UCHAR  Control;
  } REPORT_KEY, *PREPORT_KEY;
  
  struct _SET_READ_AHEAD {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  Lun : 3;
    UCHAR  TriggerLBA[4];
    UCHAR  ReadAheadLBA[4];
    UCHAR  Reserved2;
    UCHAR  Control;
  } SET_READ_AHEAD, *PSET_READ_AHEAD;
  
  struct _READ_FORMATTED_CAPACITIES {
    UCHAR  OperationCode;
    UCHAR  Reserved1 : 5;
    UCHAR  Lun : 3;
    UCHAR  Reserved2[5];
    UCHAR  AllocationLength[2];
    UCHAR  Control;
  } READ_FORMATTED_CAPACITIES, *PREAD_FORMATTED_CAPACITIES;
  
  struct _REPORT_LUNS {
    UCHAR  OperationCode;
    UCHAR  Reserved1[5];
    UCHAR  AllocationLength[4];
    UCHAR  Reserved2[1];
    UCHAR  Control;
  } REPORT_LUNS, *PREPORT_LUNS;
  
  struct _PERSISTENT_RESERVE_IN {
    UCHAR  OperationCode;
    UCHAR  ServiceAction : 5;
    UCHAR  Reserved1 : 3;
    UCHAR  Reserved2[5];
    UCHAR  AllocationLength[2];
    UCHAR  Control;
  } PERSISTENT_RESERVE_IN, *PPERSISTENT_RESERVE_IN;
  
  struct _PERSISTENT_RESERVE_OUT {
    UCHAR  OperationCode;
    UCHAR  ServiceAction : 5;
    UCHAR  Reserved1 : 3;
    UCHAR  Type : 4;
    UCHAR  Scope : 4;
    UCHAR  Reserved2[4];
    UCHAR  ParameterListLength[2];
    UCHAR  Control;
  } PERSISTENT_RESERVE_OUT, *PPERSISTENT_RESERVE_OUT;
  
  struct _GET_CONFIGURATION {
    UCHAR  OperationCode;
    UCHAR  RequestType : 1;
    UCHAR  Reserved1   : 7;
    UCHAR  StartingFeature[2];
    UCHAR  Reserved2[3];
    UCHAR  AllocationLength[2];
    UCHAR  Control;
  } GET_CONFIGURATION, *PGET_CONFIGURATION;
  
  struct _SET_CD_SPEED {
    UCHAR  OperationCode;
    UCHAR  Reserved1;
    UCHAR  ReadSpeed[2];
    UCHAR  WriteSpeed[2];
    UCHAR  Reserved2[5];
    UCHAR  Control;
  } SET_CD_SPEED, *PSET_CD_SPEED;
  
  ULONG AsUlong[4];
    UCHAR  AsByte[16];
} CDB, *PCDB;

#ifndef _INQUIRYDATA_DEFINED /* also in minitape.h */
#define _INQUIRYDATA_DEFINED

#define INQUIRYDATABUFFERSIZE             36

typedef struct _INQUIRYDATA {
  UCHAR  DeviceType : 5;
  UCHAR  DeviceTypeQualifier : 3;
  UCHAR  DeviceTypeModifier : 7;
  UCHAR  RemovableMedia : 1;
  _ANONYMOUS_UNION union {
    UCHAR  Versions;
    _ANONYMOUS_STRUCT struct {
      UCHAR  ANSIVersion : 3;
      UCHAR  ECMAVersion : 3;
      UCHAR  ISOVersion : 2;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  UCHAR  ResponseDataFormat : 4;
  UCHAR  HiSupport : 1;
  UCHAR  NormACA : 1;
  UCHAR  TerminateTask : 1;
  UCHAR  AERC : 1;
  UCHAR  AdditionalLength;
  UCHAR  Reserved;
  UCHAR  Addr16 : 1;
  UCHAR  Addr32 : 1;
  UCHAR  AckReqQ: 1;
  UCHAR  MediumChanger : 1;
  UCHAR  MultiPort : 1;
  UCHAR  ReservedBit2 : 1;
  UCHAR  EnclosureServices : 1;
  UCHAR  ReservedBit3 : 1;
  UCHAR  SoftReset : 1;
  UCHAR  CommandQueue : 1;
  UCHAR  TransferDisable : 1;
  UCHAR  LinkedCommands : 1;
  UCHAR  Synchronous : 1;
  UCHAR  Wide16Bit : 1;
  UCHAR  Wide32Bit : 1;
  UCHAR  RelativeAddressing : 1;
  UCHAR  VendorId[8];
  UCHAR  ProductId[16];
  UCHAR  ProductRevisionLevel[4];
  UCHAR  VendorSpecific[20];
  UCHAR  Reserved3[40];
} INQUIRYDATA, *PINQUIRYDATA;
#endif

/* INQUIRYDATA.DeviceType constants */
#define DIRECT_ACCESS_DEVICE              0x00
#define SEQUENTIAL_ACCESS_DEVICE          0x01
#define PRINTER_DEVICE                    0x02
#define PROCESSOR_DEVICE                  0x03
#define WRITE_ONCE_READ_MULTIPLE_DEVICE   0x04
#define READ_ONLY_DIRECT_ACCESS_DEVICE    0x05
#define SCANNER_DEVICE                    0x06
#define OPTICAL_DEVICE                    0x07
#define MEDIUM_CHANGER                    0x08
#define COMMUNICATION_DEVICE              0x09
#define LOGICAL_UNIT_NOT_PRESENT_DEVICE   0x7F
#define DEVICE_QUALIFIER_NOT_SUPPORTED    0x03

/* INQUIRYDATA.DeviceTypeQualifier constants */
#define DEVICE_CONNECTED 0x00

#define SCSISTAT_GOOD                     0x00
#define SCSISTAT_CHECK_CONDITION          0x02
#define SCSISTAT_CONDITION_MET            0x04
#define SCSISTAT_BUSY                     0x08
#define SCSISTAT_INTERMEDIATE             0x10
#define SCSISTAT_INTERMEDIATE_COND_MET    0x14
#define SCSISTAT_RESERVATION_CONFLICT     0x18
#define SCSISTAT_COMMAND_TERMINATED       0x22
#define SCSISTAT_QUEUE_FULL               0x28

/* Mode Sense/Select page constants */
#define MODE_PAGE_ERROR_RECOVERY          0x01
#define MODE_PAGE_DISCONNECT              0x02
#define MODE_PAGE_FORMAT_DEVICE           0x03
#define MODE_PAGE_RIGID_GEOMETRY          0x04
#define MODE_PAGE_FLEXIBILE               0x05
#define MODE_PAGE_WRITE_PARAMETERS        0x05
#define MODE_PAGE_VERIFY_ERROR            0x07
#define MODE_PAGE_CACHING                 0x08
#define MODE_PAGE_PERIPHERAL              0x09
#define MODE_PAGE_CONTROL                 0x0A
#define MODE_PAGE_MEDIUM_TYPES            0x0B
#define MODE_PAGE_NOTCH_PARTITION         0x0C
#define MODE_PAGE_CD_AUDIO_CONTROL        0x0E
#define MODE_PAGE_DATA_COMPRESS           0x0F
#define MODE_PAGE_DEVICE_CONFIG           0x10
#define MODE_PAGE_MEDIUM_PARTITION        0x11
#define MODE_PAGE_CDVD_FEATURE_SET        0x18
#define MODE_PAGE_POWER_CONDITION         0x1A
#define MODE_PAGE_FAULT_REPORTING         0x1C
#define MODE_PAGE_CDVD_INACTIVITY         0x1D
#define MODE_PAGE_ELEMENT_ADDRESS         0x1D
#define MODE_PAGE_TRANSPORT_GEOMETRY      0x1E
#define MODE_PAGE_DEVICE_CAPABILITIES     0x1F
#define MODE_PAGE_CAPABILITIES            0x2A
#define MODE_SENSE_RETURN_ALL             0x3f
#define MODE_SENSE_CURRENT_VALUES         0x00
#define MODE_SENSE_CHANGEABLE_VALUES      0x40
#define MODE_SENSE_DEFAULT_VAULES         0x80
#define MODE_SENSE_SAVED_VALUES           0xc0

/* SCSI CDB operation codes */
#define SCSIOP_TEST_UNIT_READY            0x00
#define SCSIOP_REZERO_UNIT                0x01
#define SCSIOP_REWIND                     0x01
#define SCSIOP_REQUEST_BLOCK_ADDR         0x02
#define SCSIOP_REQUEST_SENSE              0x03
#define SCSIOP_FORMAT_UNIT                0x04
#define SCSIOP_READ_BLOCK_LIMITS          0x05
#define SCSIOP_REASSIGN_BLOCKS            0x07
#define SCSIOP_INIT_ELEMENT_STATUS        0x07
#define SCSIOP_READ6                      0x08
#define SCSIOP_RECEIVE                    0x08
#define SCSIOP_WRITE6                     0x0A
#define SCSIOP_PRINT                      0x0A
#define SCSIOP_SEND                       0x0A
#define SCSIOP_SEEK6                      0x0B
#define SCSIOP_TRACK_SELECT               0x0B
#define SCSIOP_SLEW_PRINT                 0x0B
#define SCSIOP_SEEK_BLOCK                 0x0C
#define SCSIOP_PARTITION                  0x0D
#define SCSIOP_READ_REVERSE               0x0F
#define SCSIOP_WRITE_FILEMARKS            0x10
#define SCSIOP_FLUSH_BUFFER               0x10
#define SCSIOP_SPACE                      0x11
#define SCSIOP_INQUIRY                    0x12
#define SCSIOP_VERIFY6                    0x13
#define SCSIOP_RECOVER_BUF_DATA           0x14
#define SCSIOP_MODE_SELECT                0x15
#define SCSIOP_RESERVE_UNIT               0x16
#define SCSIOP_RELEASE_UNIT               0x17
#define SCSIOP_COPY                       0x18
#define SCSIOP_ERASE                      0x19
#define SCSIOP_MODE_SENSE                 0x1A
#define SCSIOP_START_STOP_UNIT            0x1B
#define SCSIOP_STOP_PRINT                 0x1B
#define SCSIOP_LOAD_UNLOAD                0x1B
#define SCSIOP_RECEIVE_DIAGNOSTIC         0x1C
#define SCSIOP_SEND_DIAGNOSTIC            0x1D
#define SCSIOP_MEDIUM_REMOVAL             0x1E

#define SCSIOP_READ_FORMATTED_CAPACITY    0x23
#define SCSIOP_READ_CAPACITY              0x25
#define SCSIOP_READ                       0x28
#define SCSIOP_WRITE                      0x2A
#define SCSIOP_SEEK                       0x2B
#define SCSIOP_LOCATE                     0x2B
#define SCSIOP_POSITION_TO_ELEMENT        0x2B
#define SCSIOP_WRITE_VERIFY               0x2E
#define SCSIOP_VERIFY                     0x2F
#define SCSIOP_SEARCH_DATA_HIGH           0x30
#define SCSIOP_SEARCH_DATA_EQUAL          0x31
#define SCSIOP_SEARCH_DATA_LOW            0x32
#define SCSIOP_SET_LIMITS                 0x33
#define SCSIOP_READ_POSITION              0x34
#define SCSIOP_SYNCHRONIZE_CACHE          0x35
#define SCSIOP_COMPARE                    0x39
#define SCSIOP_COPY_COMPARE               0x3A
#define SCSIOP_WRITE_DATA_BUFF            0x3B
#define SCSIOP_READ_DATA_BUFF             0x3C
#define SCSIOP_CHANGE_DEFINITION          0x40
#define SCSIOP_READ_SUB_CHANNEL           0x42
#define SCSIOP_READ_TOC                   0x43
#define SCSIOP_READ_HEADER                0x44
#define SCSIOP_PLAY_AUDIO                 0x45
#define SCSIOP_GET_CONFIGURATION          0x46
#define SCSIOP_PLAY_AUDIO_MSF             0x47
#define SCSIOP_PLAY_TRACK_INDEX           0x48
#define SCSIOP_PLAY_TRACK_RELATIVE        0x49
#define SCSIOP_GET_EVENT_STATUS           0x4A
#define SCSIOP_PAUSE_RESUME               0x4B
#define SCSIOP_LOG_SELECT                 0x4C
#define SCSIOP_LOG_SENSE                  0x4D
#define SCSIOP_STOP_PLAY_SCAN             0x4E
#define SCSIOP_READ_DISK_INFORMATION      0x51
#define SCSIOP_READ_TRACK_INFORMATION     0x52
#define SCSIOP_RESERVE_TRACK_RZONE        0x53
#define SCSIOP_SEND_OPC_INFORMATION       0x54
#define SCSIOP_MODE_SELECT10              0x55
#define SCSIOP_MODE_SENSE10               0x5A
#define SCSIOP_CLOSE_TRACK_SESSION        0x5B
#define SCSIOP_READ_BUFFER_CAPACITY       0x5C
#define SCSIOP_SEND_CUE_SHEET             0x5D
#define SCSIOP_PERSISTENT_RESERVE_IN      0x5E
#define SCSIOP_PERSISTENT_RESERVE_OUT     0x5F

#define SCSIOP_REPORT_LUNS                0xA0
#define SCSIOP_BLANK                      0xA1
#define SCSIOP_SEND_KEY                   0xA3
#define SCSIOP_REPORT_KEY                 0xA4
#define SCSIOP_MOVE_MEDIUM                0xA5
#define SCSIOP_LOAD_UNLOAD_SLOT           0xA6
#define SCSIOP_EXCHANGE_MEDIUM            0xA6
#define SCSIOP_SET_READ_AHEAD             0xA7
#define SCSIOP_READ_DVD_STRUCTURE         0xAD
#define SCSIOP_REQUEST_VOL_ELEMENT        0xB5
#define SCSIOP_SEND_VOLUME_TAG            0xB6
#define SCSIOP_READ_ELEMENT_STATUS        0xB8
#define SCSIOP_READ_CD_MSF                0xB9
#define SCSIOP_SCAN_CD                    0xBA
#define SCSIOP_SET_CD_SPEED               0xBB
#define SCSIOP_PLAY_CD                    0xBC
#define SCSIOP_MECHANISM_STATUS           0xBD
#define SCSIOP_READ_CD                    0xBE
#define SCSIOP_SEND_DVD_STRUCTURE         0xBF
#define SCSIOP_INIT_ELEMENT_RANGE         0xE7

#define SCSIOP_DENON_EJECT_DISC           0xE6
#define SCSIOP_DENON_STOP_AUDIO           0xE7
#define SCSIOP_DENON_PLAY_AUDIO           0xE8
#define SCSIOP_DENON_READ_TOC             0xE9
#define SCSIOP_DENON_READ_SUBCODE         0xEB

#define SCSIMESS_MODIFY_DATA_POINTER      0x00
#define SCSIMESS_SYNCHRONOUS_DATA_REQ     0x01
#define SCSIMESS_WIDE_DATA_REQUEST        0x03

#define SCSIMESS_MODIFY_DATA_LENGTH       5
#define SCSIMESS_SYNCH_DATA_LENGTH        3
#define SCSIMESS_WIDE_DATA_LENGTH         2

#define SCSIMESS_ABORT                    0x06
#define SCSIMESS_ABORT_WITH_TAG           0x0D
#define SCSIMESS_BUS_DEVICE_RESET         0x0C
#define SCSIMESS_CLEAR_QUEUE              0x0E
#define SCSIMESS_COMMAND_COMPLETE         0x00
#define SCSIMESS_DISCONNECT               0x04
#define SCSIMESS_EXTENDED_MESSAGE         0x01
#define SCSIMESS_IDENTIFY                 0x80
#define SCSIMESS_IDENTIFY_WITH_DISCON     0xC0
#define SCSIMESS_IGNORE_WIDE_RESIDUE      0x23
#define SCSIMESS_INITIATE_RECOVERY        0x0F
#define SCSIMESS_INIT_DETECTED_ERROR      0x05
#define SCSIMESS_LINK_CMD_COMP            0x0A
#define SCSIMESS_LINK_CMD_COMP_W_FLAG     0x0B
#define SCSIMESS_MESS_PARITY_ERROR        0x09
#define SCSIMESS_MESSAGE_REJECT           0x07
#define SCSIMESS_NO_OPERATION             0x08
#define SCSIMESS_HEAD_OF_QUEUE_TAG        0x21
#define SCSIMESS_ORDERED_QUEUE_TAG        0x22
#define SCSIMESS_SIMPLE_QUEUE_TAG         0x20
#define SCSIMESS_RELEASE_RECOVERY         0x10
#define SCSIMESS_RESTORE_POINTERS         0x03
#define SCSIMESS_SAVE_DATA_POINTER        0x02
#define SCSIMESS_TERMINATE_IO_PROCESS     0x11

#define CDB_FORCE_MEDIA_ACCESS            0x08

#define CDB_RETURN_ON_COMPLETION          0
#define CDB_RETURN_IMMEDIATE              1

#define CDB_INQUIRY_EVPD                  0x01

#define LUN0_FORMAT_SAVING_DEFECT_LIST    0
#define USE_DEFAULTMSB                    0
#define USE_DEFAULTLSB                    0

#define START_UNIT_CODE                   0x01
#define STOP_UNIT_CODE                    0x00

typedef struct _SENSE_DATA {
  UCHAR  ErrorCode : 7;
  UCHAR  Valid : 1;
  UCHAR  SegmentNumber;
  UCHAR  SenseKey : 4;
  UCHAR  Reserved : 1;
  UCHAR  IncorrectLength : 1;
  UCHAR  EndOfMedia : 1;
  UCHAR  FileMark : 1;
  UCHAR  Information[4];
  UCHAR  AdditionalSenseLength;
  UCHAR  CommandSpecificInformation[4];
  UCHAR  AdditionalSenseCode;
  UCHAR  AdditionalSenseCodeQualifier;
  UCHAR  FieldReplaceableUnitCode;
  UCHAR  SenseKeySpecific[3];
} SENSE_DATA, *PSENSE_DATA;

#define SENSE_BUFFER_SIZE                 18

/* Sense codes */
#define SCSI_SENSE_NO_SENSE               0x00
#define SCSI_SENSE_RECOVERED_ERROR        0x01
#define SCSI_SENSE_NOT_READY              0x02
#define SCSI_SENSE_MEDIUM_ERROR           0x03
#define SCSI_SENSE_HARDWARE_ERROR         0x04
#define SCSI_SENSE_ILLEGAL_REQUEST        0x05
#define SCSI_SENSE_UNIT_ATTENTION         0x06
#define SCSI_SENSE_DATA_PROTECT           0x07
#define SCSI_SENSE_BLANK_CHECK            0x08
#define SCSI_SENSE_UNIQUE                 0x09
#define SCSI_SENSE_COPY_ABORTED           0x0A
#define SCSI_SENSE_ABORTED_COMMAND        0x0B
#define SCSI_SENSE_EQUAL                  0x0C
#define SCSI_SENSE_VOL_OVERFLOW           0x0D
#define SCSI_SENSE_MISCOMPARE             0x0E
#define SCSI_SENSE_RESERVED               0x0F

/* Additional tape bit */
#define SCSI_ILLEGAL_LENGTH               0x20
#define SCSI_EOM                          0x40
#define SCSI_FILE_MARK                    0x80

/* Additional Sense codes */
#define SCSI_ADSENSE_NO_SENSE                              0x00
#define SCSI_ADSENSE_NO_SEEK_COMPLETE                      0x02
#define SCSI_ADSENSE_LUN_NOT_READY                         0x04
#define SCSI_ADSENSE_WRITE_ERROR                           0x0C
#define SCSI_ADSENSE_TRACK_ERROR                           0x14
#define SCSI_ADSENSE_SEEK_ERROR                            0x15
#define SCSI_ADSENSE_REC_DATA_NOECC                        0x17
#define SCSI_ADSENSE_REC_DATA_ECC                          0x18
#define SCSI_ADSENSE_ILLEGAL_COMMAND                       0x20
#define SCSI_ADSENSE_ILLEGAL_BLOCK                         0x21
#define SCSI_ADSENSE_INVALID_CDB                           0x24
#define SCSI_ADSENSE_INVALID_LUN                           0x25
#define SCSI_ADSENSE_WRITE_PROTECT                         0x27
#define SCSI_ADSENSE_MEDIUM_CHANGED                        0x28
#define SCSI_ADSENSE_BUS_RESET                             0x29
#define SCSI_ADSENSE_INSUFFICIENT_TIME_FOR_OPERATION       0x2E
#define SCSI_ADSENSE_INVALID_MEDIA                         0x30
#define SCSI_ADSENSE_NO_MEDIA_IN_DEVICE                    0x3a
#define SCSI_ADSENSE_POSITION_ERROR                        0x3b
#define SCSI_ADSENSE_OPERATOR_REQUEST                      0x5a
#define SCSI_ADSENSE_FAILURE_PREDICTION_THRESHOLD_EXCEEDED 0x5d
#define SCSI_ADSENSE_ILLEGAL_MODE_FOR_THIS_TRACK           0x64
#define SCSI_ADSENSE_COPY_PROTECTION_FAILURE               0x6f
#define SCSI_ADSENSE_POWER_CALIBRATION_ERROR               0x73
#define SCSI_ADSENSE_VENDOR_UNIQUE                         0x80
#define SCSI_ADSENSE_MUSIC_AREA                            0xA0
#define SCSI_ADSENSE_DATA_AREA                             0xA1
#define SCSI_ADSENSE_VOLUME_OVERFLOW                       0xA7

#define SCSI_SENSEQ_CAUSE_NOT_REPORTABLE                   0x00
#define SCSI_SENSEQ_BECOMING_READY                         0x01
#define SCSI_SENSEQ_INIT_COMMAND_REQUIRED                  0x02
#define SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED           0x03
#define SCSI_SENSEQ_FORMAT_IN_PROGRESS                     0x04
#define SCSI_SENSEQ_REBUILD_IN_PROGRESS                    0x05
#define SCSI_SENSEQ_RECALCULATION_IN_PROGRESS              0x06
#define SCSI_SENSEQ_OPERATION_IN_PROGRESS                  0x07
#define SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS                 0x08
#define SCSI_SENSEQ_LOSS_OF_STREAMING                      0x09
#define SCSI_SENSEQ_PADDING_BLOCKS_ADDED                   0x0A


#define FILE_DEVICE_SCSI 0x0000001b

#define IOCTL_SCSI_EXECUTE_IN ((FILE_DEVICE_SCSI << 16) + 0x0011)
#define IOCTL_SCSI_EXECUTE_OUT ((FILE_DEVICE_SCSI << 16) + 0x0012)
#define IOCTL_SCSI_EXECUTE_NONE ((FILE_DEVICE_SCSI << 16) + 0x0013)

/* SMART support in ATAPI */
#define IOCTL_SCSI_MINIPORT_SMART_VERSION               ((FILE_DEVICE_SCSI << 16) + 0x0500)
#define IOCTL_SCSI_MINIPORT_IDENTIFY                    ((FILE_DEVICE_SCSI << 16) + 0x0501)
#define IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS          ((FILE_DEVICE_SCSI << 16) + 0x0502)
#define IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS       ((FILE_DEVICE_SCSI << 16) + 0x0503)
#define IOCTL_SCSI_MINIPORT_ENABLE_SMART                ((FILE_DEVICE_SCSI << 16) + 0x0504)
#define IOCTL_SCSI_MINIPORT_DISABLE_SMART               ((FILE_DEVICE_SCSI << 16) + 0x0505)
#define IOCTL_SCSI_MINIPORT_RETURN_STATUS               ((FILE_DEVICE_SCSI << 16) + 0x0506)
#define IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE     ((FILE_DEVICE_SCSI << 16) + 0x0507)
#define IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES       ((FILE_DEVICE_SCSI << 16) + 0x0508)
#define IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS       ((FILE_DEVICE_SCSI << 16) + 0x0509)
#define IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTO_OFFLINE ((FILE_DEVICE_SCSI << 16) + 0x050a)
#define IOCTL_SCSI_MINIPORT_READ_SMART_LOG              ((FILE_DEVICE_SCSI << 16) + 0x050b)
#define IOCTL_SCSI_MINIPORT_WRITE_SMART_LOG             ((FILE_DEVICE_SCSI << 16) + 0x050c)

/* CLUSTER support */
#define IOCTL_SCSI_MINIPORT_NOT_QUORUM_CAPABLE  ((FILE_DEVICE_SCSI << 16) + 0x0520)
#define IOCTL_SCSI_MINIPORT_NOT_CLUSTER_CAPABLE ((FILE_DEVICE_SCSI << 16) + 0x0521)

/* Read Capacity Data. Returned in Big Endian format */
typedef struct _READ_CAPACITY_DATA {
  ULONG  LogicalBlockAddress;
  ULONG  BytesPerBlock;
} READ_CAPACITY_DATA, *PREAD_CAPACITY_DATA;

/* Read Block Limits Data. Returned in Big Endian format */
typedef struct _READ_BLOCK_LIMITS {
  UCHAR  Reserved;
  UCHAR  BlockMaximumSize[3];
  UCHAR  BlockMinimumSize[2];
} READ_BLOCK_LIMITS_DATA, *PREAD_BLOCK_LIMITS_DATA;


typedef struct _MODE_PARAMETER_HEADER {
  UCHAR  ModeDataLength;
  UCHAR  MediumType;
  UCHAR  DeviceSpecificParameter;
  UCHAR  BlockDescriptorLength;
}MODE_PARAMETER_HEADER, *PMODE_PARAMETER_HEADER;

typedef struct _MODE_PARAMETER_HEADER10 {
  UCHAR  ModeDataLength[2];
  UCHAR  MediumType;
  UCHAR  DeviceSpecificParameter;
  UCHAR  Reserved[2];
  UCHAR  BlockDescriptorLength[2];
} MODE_PARAMETER_HEADER10, *PMODE_PARAMETER_HEADER10;

#define MODE_FD_SINGLE_SIDE               0x01
#define MODE_FD_DOUBLE_SIDE               0x02
#define MODE_FD_MAXIMUM_TYPE              0x1E
#define MODE_DSP_FUA_SUPPORTED            0x10
#define MODE_DSP_WRITE_PROTECT            0x80

typedef struct _MODE_PARAMETER_BLOCK {
  UCHAR  DensityCode;
  UCHAR  NumberOfBlocks[3];
  UCHAR  Reserved;
  UCHAR  BlockLength[3];
} MODE_PARAMETER_BLOCK, *PMODE_PARAMETER_BLOCK;

typedef struct _MODE_DISCONNECT_PAGE {
  UCHAR  PageCode : 6;
  UCHAR  Reserved : 1;
  UCHAR  PageSavable : 1;
  UCHAR  PageLength;
  UCHAR  BufferFullRatio;
  UCHAR  BufferEmptyRatio;
  UCHAR  BusInactivityLimit[2];
  UCHAR  BusDisconnectTime[2];
  UCHAR  BusConnectTime[2];
  UCHAR  MaximumBurstSize[2];
  UCHAR  DataTransferDisconnect : 2;
  UCHAR  Reserved2[3];
}MODE_DISCONNECT_PAGE, *PMODE_DISCONNECT_PAGE;

typedef struct _MODE_CACHING_PAGE {
  UCHAR  PageCode : 6;
  UCHAR  Reserved : 1;
  UCHAR  PageSavable : 1;
  UCHAR  PageLength;
  UCHAR  ReadDisableCache : 1;
  UCHAR  MultiplicationFactor : 1;
  UCHAR  WriteCacheEnable : 1;
  UCHAR  Reserved2 : 5;
  UCHAR  WriteRetensionPriority : 4;
  UCHAR  ReadRetensionPriority : 4;
  UCHAR  DisablePrefetchTransfer[2];
  UCHAR  MinimumPrefetch[2];
  UCHAR  MaximumPrefetch[2];
  UCHAR  MaximumPrefetchCeiling[2];
}MODE_CACHING_PAGE, *PMODE_CACHING_PAGE;

typedef struct _MODE_CDROM_WRITE_PARAMETERS_PAGE {
  UCHAR  PageLength;
  UCHAR  WriteType : 4;
  UCHAR  TestWrite : 1;
  UCHAR  LinkSizeValid : 1;
  UCHAR  BufferUnderrunFreeEnabled : 1;
  UCHAR  Reserved2 : 1;
  UCHAR  TrackMode : 4;
  UCHAR  Copy : 1;
  UCHAR  FixedPacket : 1;
  UCHAR  MultiSession : 2;
  UCHAR  DataBlockType : 4;
  UCHAR  Reserved3 : 4;    
  UCHAR  LinkSize;
  UCHAR  Reserved4;
  UCHAR  HostApplicationCode : 6;
  UCHAR  Reserved5 : 2;    
  UCHAR  SessionFormat;
  UCHAR  Reserved6;
  UCHAR  PacketSize[4];
  UCHAR  AudioPauseLength[2];
  UCHAR  Reserved7 : 7;
  UCHAR  MediaCatalogNumberValid : 1;
  UCHAR  MediaCatalogNumber[13];
  UCHAR  MediaCatalogNumberZero;
  UCHAR  MediaCatalogNumberAFrame;
  UCHAR  Reserved8 : 7;
  UCHAR  ISRCValid : 1;
  UCHAR  ISRCCountry[2];
  UCHAR  ISRCOwner[3];
  UCHAR  ISRCRecordingYear[2];
  UCHAR  ISRCSerialNumber[5];
  UCHAR  ISRCZero;
  UCHAR  ISRCAFrame;
  UCHAR  ISRCReserved;
  UCHAR  SubHeaderData[4];
} MODE_CDROM_WRITE_PARAMETERS_PAGE, *PMODE_CDROM_WRITE_PARAMETERS_PAGE;

typedef struct _MODE_FLEXIBLE_DISK_PAGE {
  UCHAR  PageCode : 6;
  UCHAR  Reserved : 1;
  UCHAR  PageSavable : 1;
  UCHAR  PageLength;
  UCHAR  TransferRate[2];
  UCHAR  NumberOfHeads;
  UCHAR  SectorsPerTrack;
  UCHAR  BytesPerSector[2];
  UCHAR  NumberOfCylinders[2];
  UCHAR  StartWritePrecom[2];
  UCHAR  StartReducedCurrent[2];
  UCHAR  StepRate[2];
  UCHAR  StepPluseWidth;
  UCHAR  HeadSettleDelay[2];
  UCHAR  MotorOnDelay;
  UCHAR  MotorOffDelay;
  UCHAR  Reserved2 : 5;
  UCHAR  MotorOnAsserted : 1;
  UCHAR  StartSectorNumber : 1;
  UCHAR  TrueReadySignal : 1;
  UCHAR  StepPlusePerCyclynder : 4;
  UCHAR  Reserved3 : 4;
  UCHAR  WriteCompenstation;
  UCHAR  HeadLoadDelay;
  UCHAR  HeadUnloadDelay;
  UCHAR  Pin2Usage : 4;
  UCHAR  Pin34Usage : 4;
  UCHAR  Pin1Usage : 4;
  UCHAR  Pin4Usage : 4;
  UCHAR  MediumRotationRate[2];
  UCHAR  Reserved4[2];
} MODE_FLEXIBLE_DISK_PAGE, *PMODE_FLEXIBLE_DISK_PAGE;

typedef struct _MODE_FORMAT_PAGE {
  UCHAR  PageCode : 6;
  UCHAR  Reserved : 1;
  UCHAR  PageSavable : 1;
  UCHAR  PageLength;
  UCHAR  TracksPerZone[2];
  UCHAR  AlternateSectorsPerZone[2];
  UCHAR  AlternateTracksPerZone[2];
  UCHAR  AlternateTracksPerLogicalUnit[2];
  UCHAR  SectorsPerTrack[2];
  UCHAR  BytesPerPhysicalSector[2];
  UCHAR  Interleave[2];
  UCHAR  TrackSkewFactor[2];
  UCHAR  CylinderSkewFactor[2];
  UCHAR  Reserved2 : 4;
  UCHAR  SurfaceFirst : 1;
  UCHAR  RemovableMedia : 1;
  UCHAR  HardSectorFormating : 1;
  UCHAR  SoftSectorFormating : 1;
  UCHAR  Reserved3[3];
} MODE_FORMAT_PAGE, *PMODE_FORMAT_PAGE;

typedef struct _MODE_RIGID_GEOMETRY_PAGE {
  UCHAR  PageCode : 6;
  UCHAR  Reserved : 1;
  UCHAR  PageSavable : 1;
  UCHAR  PageLength;
  UCHAR  NumberOfCylinders[3];
  UCHAR  NumberOfHeads;
  UCHAR  StartWritePrecom[3];
  UCHAR  StartReducedCurrent[3];
  UCHAR  DriveStepRate[2];
  UCHAR  LandZoneCyclinder[3];
  UCHAR  RotationalPositionLock : 2;
  UCHAR  Reserved2 : 6;
  UCHAR  RotationOffset;
  UCHAR  Reserved3;
  UCHAR  RoataionRate[2];
  UCHAR  Reserved4[2];
} MODE_RIGID_GEOMETRY_PAGE, *PMODE_RIGID_GEOMETRY_PAGE;

typedef struct _MODE_READ_WRITE_RECOVERY_PAGE {
  UCHAR  PageCode : 6;
  UCHAR  Reserved1 : 1;
  UCHAR  PSBit : 1;
  UCHAR  PageLength;
  UCHAR  DCRBit : 1;
  UCHAR  DTEBit : 1;
  UCHAR  PERBit : 1;
  UCHAR  EERBit : 1;
  UCHAR  RCBit : 1;
  UCHAR  TBBit : 1;
  UCHAR  ARRE : 1;
  UCHAR  AWRE : 1;
  UCHAR  ReadRetryCount;
  UCHAR  Reserved4[4];
  UCHAR  WriteRetryCount;
  UCHAR  Reserved5[3];
} MODE_READ_WRITE_RECOVERY_PAGE, *PMODE_READ_WRITE_RECOVERY_PAGE;

typedef struct _MODE_READ_RECOVERY_PAGE {
  UCHAR  PageCode : 6;
  UCHAR  Reserved1 : 1;
  UCHAR  PSBit : 1;
  UCHAR  PageLength;
  UCHAR  DCRBit : 1;
  UCHAR  DTEBit : 1;
  UCHAR  PERBit : 1;
  UCHAR  Reserved2 : 1;
  UCHAR  RCBit : 1;
  UCHAR  TBBit : 1;
  UCHAR  Reserved3 : 2;
  UCHAR  ReadRetryCount;
  UCHAR  Reserved4[4];
} MODE_READ_RECOVERY_PAGE, *PMODE_READ_RECOVERY_PAGE;

typedef struct _MODE_INFO_EXCEPTIONS {
  UCHAR  PageCode : 6;
  UCHAR  Reserved1 : 1;
  UCHAR  PSBit : 1;
  UCHAR  PageLength;
  _ANONYMOUS_UNION union {
    UCHAR  Flags;
    _ANONYMOUS_STRUCT struct {
      UCHAR  LogErr : 1;
      UCHAR  Reserved2 : 1;
      UCHAR  Test : 1;
      UCHAR  Dexcpt : 1;
      UCHAR  Reserved3 : 3;
      UCHAR  Perf : 1;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  UCHAR  ReportMethod : 4;
  UCHAR  Reserved4 : 4;
  UCHAR  IntervalTimer[4];
  UCHAR  ReportCount[4];
} MODE_INFO_EXCEPTIONS, *PMODE_INFO_EXCEPTIONS;

/* CDROM audio control */
#define CDB_AUDIO_PAUSE                   0x00
#define CDB_AUDIO_RESUME                  0x01
#define CDB_DEVICE_START                  0x11
#define CDB_DEVICE_STOP                   0x10
#define CDB_EJECT_MEDIA                   0x10
#define CDB_LOAD_MEDIA                    0x01
#define CDB_SUBCHANNEL_HEADER             0x00
#define CDB_SUBCHANNEL_BLOCK              0x01

#define CDROM_AUDIO_CONTROL_PAGE          0x0E
#define MODE_SELECT_IMMEDIATE             0x04
#define MODE_SELECT_PFBIT                 0x10

#define CDB_USE_MSF                       0x01

typedef struct _PORT_OUTPUT {
  UCHAR  ChannelSelection;
  UCHAR  Volume;
} PORT_OUTPUT, *PPORT_OUTPUT;

typedef struct _AUDIO_OUTPUT {
  UCHAR  CodePage;
  UCHAR  ParameterLength;
  UCHAR  Immediate;
  UCHAR  Reserved[2];
  UCHAR  LbaFormat;
  UCHAR  LogicalBlocksPerSecond[2];
  PORT_OUTPUT  PortOutput[4];
} AUDIO_OUTPUT, *PAUDIO_OUTPUT;

/* Multisession CDROMs */
#define GET_LAST_SESSION 0x01
#define GET_SESSION_DATA 0x02;

/* Atapi 2.5 changers */
typedef struct _MECHANICAL_STATUS_INFORMATION_HEADER {
  UCHAR  CurrentSlot : 5;
  UCHAR  ChangerState : 2;
  UCHAR  Fault : 1;
  UCHAR  Reserved : 5;
  UCHAR  MechanismState : 3;
  UCHAR  CurrentLogicalBlockAddress[3];
  UCHAR  NumberAvailableSlots;
  UCHAR  SlotTableLength[2];
} MECHANICAL_STATUS_INFORMATION_HEADER, *PMECHANICAL_STATUS_INFORMATION_HEADER;

typedef struct _SLOT_TABLE_INFORMATION {
  UCHAR  DiscChanged : 1;
  UCHAR  Reserved : 6;
  UCHAR  DiscPresent : 1;
  UCHAR  Reserved2[3];
} SLOT_TABLE_INFORMATION, *PSLOT_TABLE_INFORMATION;

typedef struct _MECHANICAL_STATUS {
  MECHANICAL_STATUS_INFORMATION_HEADER  MechanicalStatusHeader;
  SLOT_TABLE_INFORMATION  SlotTableInfo[1];
} MECHANICAL_STATUS, *PMECHANICAL_STATUS;


/* Tape definitions */
typedef struct _TAPE_POSITION_DATA {
	UCHAR  Reserved1 : 2;
	UCHAR  BlockPositionUnsupported : 1;
	UCHAR  Reserved2 : 3;
	UCHAR  EndOfPartition : 1;
	UCHAR  BeginningOfPartition : 1;
	UCHAR  PartitionNumber;
	USHORT  Reserved3;
	UCHAR  FirstBlock[4];
	UCHAR  LastBlock[4];
	UCHAR  Reserved4;
	UCHAR  NumberOfBlocks[3];
	UCHAR  NumberOfBytes[4];
} TAPE_POSITION_DATA, *PTAPE_POSITION_DATA;

/* This structure is used to convert little endian ULONGs
   to SCSI CDB big endians values. */
typedef union _EIGHT_BYTE {
  _ANONYMOUS_STRUCT struct {
    UCHAR  Byte0;
    UCHAR  Byte1;
    UCHAR  Byte2;
    UCHAR  Byte3;
    UCHAR  Byte4;
    UCHAR  Byte5;
    UCHAR  Byte6;
    UCHAR  Byte7;
  } DUMMYSTRUCTNAME;
  ULONGLONG  AsULongLong;
} EIGHT_BYTE, *PEIGHT_BYTE;

typedef union _FOUR_BYTE {
  _ANONYMOUS_STRUCT struct {
    UCHAR  Byte0;
    UCHAR  Byte1;
    UCHAR  Byte2;
    UCHAR  Byte3;
  } DUMMYSTRUCTNAME;
  ULONG  AsULong;
} FOUR_BYTE, *PFOUR_BYTE;

typedef union _TWO_BYTE {
  _ANONYMOUS_STRUCT struct {
    UCHAR  Byte0;
    UCHAR  Byte1;
  } DUMMYSTRUCTNAME;
  USHORT  AsUShort;
} TWO_BYTE, *PTWO_BYTE;

/* Byte reversing macro for converting between
   big- and little-endian formats */
#define REVERSE_BYTES_QUAD(Destination, Source) { \
    PEIGHT_BYTE _val1 = (PEIGHT_BYTE)(Destination); \
    PEIGHT_BYTE _val2 = (PEIGHT_BYTE)(Source); \
    _val1->Byte7 = _val2->Byte0; \
    _val1->Byte6 = _val2->Byte1; \
    _val1->Byte5 = _val2->Byte2; \
    _val1->Byte4 = _val2->Byte3; \
    _val1->Byte3 = _val2->Byte4; \
    _val1->Byte2 = _val2->Byte5; \
    _val1->Byte1 = _val2->Byte6; \
    _val1->Byte0 = _val2->Byte7; \
}

#define REVERSE_BYTES(Destination, Source) { \
    PFOUR_BYTE _val1 = (PFOUR_BYTE)(Destination); \
    PFOUR_BYTE _val2 = (PFOUR_BYTE)(Source); \
    _val1->Byte3 = _val2->Byte0; \
    _val1->Byte2 = _val2->Byte1; \
    _val1->Byte1 = _val2->Byte2; \
    _val1->Byte0 = _val2->Byte3; \
}

#define REVERSE_BYTES_SHORT(Destination, Source) { \
  PTWO_BYTE _val1 = (PTWO_BYTE)(Destination); \
  PTWO_BYTE _val2 = (PTWO_BYTE)(Source); \
  _val1->Byte1 = _val2->Byte0; \
  _val1->Byte0 = _val2->Byte1; \
}

#define REVERSE_SHORT(Short) { \
  UCHAR _val; \
  PTWO_BYTE _val2 = (PTWO_BYTE)(Short); \
  _val = _val2->Byte0; \
  _val2->Byte0 = _val2->Byte1; \
  _val2->Byte1 = _val; \
}

#define REVERSE_LONG(Long) { \
  UCHAR _val; \
  PFOUR_BYTE _val2 = (PFOUR_BYTE)(Long); \
  _val = _val2->Byte3; \
  _val2->Byte3 = _val2->Byte0; \
  _val2->Byte0 = _val; \
  _val = _val2->Byte2; \
  _val2->Byte2 = _val2->Byte1; \
  _val2->Byte1 = _val; \
}

#define WHICH_BIT(Data, Bit) { \
  UCHAR _val; \
  for (_val = 0; _val < 32; _val++) { \
    if (((Data) >> _val) == 1) { \
      break; \
    } \
  } \
  ASSERT(_val != 32); \
  (Bit) = _val; \
}

#ifdef __cplusplus
}
#endif

#endif /* __SCSI_H */
