/* $Id: scsi.h,v 1.6 2002/09/08 10:22:23 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/include/scsi.h
 * PURPOSE:         SCSI class driver definitions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

#ifndef __STORAGE_INCLUDE_SCSI_H
#define __STORAGE_INCLUDE_SCSI_H


/* Command Descriptor Block */

typedef union _CDB
{
  /* Generic 6-Byte CDB */
  struct _CDB6GENERIC
    {
      UCHAR OperationCode;
      UCHAR Immediate:1;
      UCHAR CommandUniqueBits:4;
      UCHAR LogicalUnitNumber:3;
      UCHAR CommandUniqueBytes[3];
      UCHAR Link:1;
      UCHAR Flag:1;
      UCHAR Reserved:4;
      UCHAR VendorUnique:2;
    } CDB6GENERIC, *PCDB6GENERIC;

  /* Standard 6-byte CDB */
  struct _CDB6READWRITE
    {
      UCHAR OperationCode;
      UCHAR LogicalBlockMsb1:5;
      UCHAR LogicalUnitNumber:3;
      UCHAR LogicalBlockMsb0;
      UCHAR LogicalBlockLsb;
      UCHAR TransferBlocks;
      UCHAR Control;
    } CDB6READWRITE, *PCDB6READWRITE;

  /* SCSI Inquiry CDB */
  struct _CDB6INQUIRY
    {
      UCHAR OperationCode;
      UCHAR Reserved1:5;
      UCHAR LogicalUnitNumber:3;
      UCHAR PageCode;
      UCHAR IReserved;
      UCHAR AllocationLength;
      UCHAR Control;
    } CDB6INQUIRY, *PCDB6INQUIRY;

  /* SCSI Format CDB */
  struct _CDB6FORMAT
    {
      UCHAR OperationCode;
      UCHAR FormatControl:5;
      UCHAR LogicalUnitNumber:3;
      UCHAR FReserved1;
      UCHAR InterleaveMsb;
      UCHAR InterleaveLsb;
      UCHAR FReserved2;
    } CDB6FORMAT, *PCDB6FORMAT;

  /* Standard 10-byte CDB */
  struct _CDB10
    {
      UCHAR OperationCode;
      UCHAR RelativeAddress:1;
      UCHAR Reserved1:2;
      UCHAR ForceUnitAccess:1;
      UCHAR DisablePageOut:1;
      UCHAR LogicalUnitNumber:3;
      UCHAR LogicalBlockByte0;
      UCHAR LogicalBlockByte1;
      UCHAR LogicalBlockByte2;
      UCHAR LogicalBlockByte3;
      UCHAR Reserved2;
      UCHAR TransferBlocksMsb;
      UCHAR TransferBlocksLsb;
      UCHAR Control;
    } CDB10, *PCDB10;

  /* CD Rom Audio CDBs */
  struct _PAUSE_RESUME
    {
      UCHAR OperationCode;
      UCHAR Reserved1:5;
      UCHAR LogicalUnitNumber:3;
      UCHAR Reserved2[6];
      UCHAR Action;
      UCHAR Control;
    } PAUSE_RESUME, *PPAUSE_RESUME;

  /* Read Table of Contents */
  struct _READ_TOC
    {
      UCHAR OperationCode;
      UCHAR Reserved0:1;
      UCHAR Msf:1;
      UCHAR Reserved1:3;
      UCHAR LogicalUnitNumber:3;
      UCHAR Reserved2[4];
      UCHAR StartingTrack;
      UCHAR AllocationLength[2];
      UCHAR Control:6;
      UCHAR Format:2;
    } READ_TOC, *PREAD_TOC;

  struct _PLAY_AUDIO_MSF
    {
      UCHAR OperationCode;
      UCHAR Reserved1:5;
      UCHAR LogicalUnitNumber:3;
      UCHAR Reserved2;
      UCHAR StartingM;
      UCHAR StartingS;
      UCHAR StartingF;
      UCHAR EndingM;
      UCHAR EndingS;
      UCHAR EndingF;
      UCHAR Control;
    } PLAY_AUDIO_MSF, *PPLAY_AUDIO_MSF;

  /* Read SubChannel Data */
  struct _SUBCHANNEL
    {
      UCHAR OperationCode;
      UCHAR Reserved0:1;
      UCHAR Msf:1;
      UCHAR Reserved1:3;
      UCHAR LogicalUnitNumber:3;
      UCHAR Reserved2:6;
      UCHAR SubQ:1;
      UCHAR Reserved3:1;
      UCHAR Format;
      UCHAR Reserved4[2];
      UCHAR TrackNumber;
      UCHAR AllocationLength[2];
      UCHAR Control;
    } SUBCHANNEL, *PSUBCHANNEL;

  /* Read CD. Used by Atapi for raw sector reads. */
  struct _READ_CD
    {
      UCHAR OperationCode;
      UCHAR Reserved0:2;
      UCHAR ExpectedSectorType:3;
      UCHAR Reserved1:3;
      UCHAR StartingLBA[4];
      UCHAR TransferBlocks[3];
      UCHAR Reserved2:1;
      UCHAR ErrorFlags:2;
      UCHAR IncludeEDC:1;
      UCHAR IncludeUserData:1;
      UCHAR HeaderCode:2;
      UCHAR IncludeSyncData:1;
      UCHAR SubChannelSelection:3;
      UCHAR Reserved3:5;
      UCHAR Reserved4;
    } READ_CD, *PREAD_CD;

  /* Plextor Read CD-DA */
  struct _PLXTR_READ_CDDA
    {
      UCHAR OperationCode;
      UCHAR Reserved0:5;
      UCHAR LogicalUnitNumber:3;
      UCHAR LogicalBlockByte0;
      UCHAR LogicalBlockByte1;
      UCHAR LogicalBlockByte2;
      UCHAR LogicalBlockByte3;
      UCHAR TransferBlockByte0;
      UCHAR TransferBlockByte1;
      UCHAR TransferBlockByte2;
      UCHAR TransferBlockByte3;
      UCHAR SubCode;
      UCHAR Control;
    } PLXTR_READ_CDDA, *PPLXTR_READ_CDDA;

  /* NEC Read CD-DA */
  struct _NEC_READ_CDDA
    {
      UCHAR OperationCode;
      UCHAR Reserved0;
      UCHAR LogicalBlockByte0;
      UCHAR LogicalBlockByte1;
      UCHAR LogicalBlockByte2;
      UCHAR LogicalBlockByte3;
      UCHAR Reserved1;
      UCHAR TransferBlockByte0;
      UCHAR TransferBlockByte1;
      UCHAR Control;
    } NEC_READ_CDDA, *PNEC_READ_CDDA;

  /* Mode sense */
  struct _MODE_SENSE
    {
      UCHAR OperationCode;
      UCHAR Reserved1:3;
      UCHAR Dbd:1;
      UCHAR Reserved2:1;
      UCHAR LogicalUnitNumber:3;
      UCHAR PageCode:6;
      UCHAR Pc:2;
      UCHAR Reserved3;
      UCHAR AllocationLength;
      UCHAR Control;
    } MODE_SENSE, *PMODE_SENSE;

  struct _MODE_SENSE10
    {
      UCHAR OperationCode;
      UCHAR Reserved1:3;
      UCHAR Dbd:1;
      UCHAR Reserved2:1;
      UCHAR LogicalUnitNumber:3;
      UCHAR PageCode:6;
      UCHAR Pc:2;
      UCHAR Reserved3[4];
      UCHAR AllocationLength[2];
      UCHAR Control;
    } MODE_SENSE10, *PMODE_SENSE10;

  /* Mode select */
  struct _MODE_SELECT
    {
      UCHAR OperationCode;
      UCHAR SPBit:1;
      UCHAR Reserved1:3;
      UCHAR PFBit:1;
      UCHAR LogicalUnitNumber:3;
      UCHAR Reserved2[2];
      UCHAR ParameterListLength;
      UCHAR Control;
    } MODE_SELECT, *PMODE_SELECT;

  struct _MODE_SELECT10
    {
      UCHAR OperationCode;
      UCHAR SPBit:1;
      UCHAR Reserved1:3;
      UCHAR PFBit:1;
      UCHAR LogicalUnitNumber:3;
      UCHAR Reserved2[5];
      UCHAR ParameterListLength[2];
      UCHAR Control;
    } MODE_SELECT10, *PMODE_SELECT10;

  struct _LOCATE
    {
      UCHAR OperationCode;
      UCHAR Immediate:1;
      UCHAR CPBit:1;
      UCHAR BTBit:1;
      UCHAR Reserved1:2;
      UCHAR LogicalUnitNumber:3;
      UCHAR Reserved3;
      UCHAR LogicalBlockAddress[4];
      UCHAR Reserved4;
      UCHAR Partition;
      UCHAR Control;
    } LOCATE, *PLOCATE;

  struct _LOGSENSE
    {
      UCHAR OperationCode;
      UCHAR SPBit:1;
      UCHAR PPCBit:1;
      UCHAR Reserved1:3;
      UCHAR LogicalUnitNumber:3;
      UCHAR PageCode:6;
      UCHAR PCBit:2;
      UCHAR Reserved2;
      UCHAR Reserved3;
      UCHAR ParameterPointer[2];	/* [0]=MSB, [1]=LSB */
      UCHAR AllocationLength[2];	/* [0]=MSB, [1]=LSB */
      UCHAR Control;
    } LOGSENSE, *PLOGSENSE;

  struct _PRINT
    {
      UCHAR OperationCode;
      UCHAR Reserved:5;
      UCHAR LogicalUnitNumber:3;
      UCHAR TransferLength[3];
      UCHAR Control;
    } PRINT, *PPRINT;

  struct _SEEK
    {
      UCHAR OperationCode;
      UCHAR Reserved1:5;
      UCHAR LogicalUnitNumber:3;
      UCHAR LogicalBlockAddress[4];
      UCHAR Reserved2[3];
      UCHAR Control;
    } SEEK, *PSEEK;

  struct _ERASE
    {
      UCHAR OperationCode;
      UCHAR Long:1;
      UCHAR Immediate:1;
      UCHAR Reserved1:3;
      UCHAR LogicalUnitNumber:3;
      UCHAR Reserved2[3];
      UCHAR Control;
    } ERASE, *PERASE;

  struct _START_STOP
    {
      UCHAR OperationCode;
      UCHAR Immediate:1;
      UCHAR Reserved1:4;
      UCHAR LogicalUnitNumber:3;
      UCHAR Reserved2[2];
      UCHAR Start:1;
      UCHAR LoadEject:1;
      UCHAR Reserved3:6;
      UCHAR Control;
    } START_STOP, *PSTART_STOP;

  struct _MEDIA_REMOVAL
    {
      UCHAR OperationCode;
      UCHAR Reserved1:5;
      UCHAR LogicalUnitNumber:3;
      UCHAR Reserved2[2];
      UCHAR Prevent;
      UCHAR Control;
    } MEDIA_REMOVAL, *PMEDIA_REMOVAL;

  /* Tape CDBs */
  struct _SEEK_BLOCK
    {
      UCHAR OperationCode;
      UCHAR Immediate:1;
      UCHAR Reserved1:7;
      UCHAR BlockAddress[3];
      UCHAR Link:1;
      UCHAR Flag:1;
      UCHAR Reserved2:4;
      UCHAR VendorUnique:2;
    } SEEK_BLOCK, *PSEEK_BLOCK;

  struct _REQUEST_BLOCK_ADDRESS
    {
      UCHAR OperationCode;
      UCHAR Reserved1[3];
      UCHAR AllocationLength;
      UCHAR Link:1;
      UCHAR Flag:1;
      UCHAR Reserved2:4;
      UCHAR VendorUnique:2;
    } REQUEST_BLOCK_ADDRESS, *PREQUEST_BLOCK_ADDRESS;

  struct _PARTITION
    {
      UCHAR OperationCode;
      UCHAR Immediate:1;
      UCHAR Sel:1;
      UCHAR PartitionSelect:6;
      UCHAR Reserved1[3];
      UCHAR Control;
    } PARTITION, *PPARTITION;

  struct _WRITE_TAPE_MARKS
    {
      UCHAR OperationCode;
      UCHAR Immediate:1;
      UCHAR WriteSetMarks:1;
      UCHAR Reserved:3;
      UCHAR LogicalUnitNumber:3;
      UCHAR TransferLength[3];
      UCHAR Control;
    } WRITE_TAPE_MARKS, *PWRITE_TAPE_MARKS;

  struct _SPACE_TAPE_MARKS
    {
      UCHAR OperationCode;
      UCHAR Code:3;
      UCHAR Reserved:2;
      UCHAR LogicalUnitNumber:3;
      UCHAR NumMarksMSB ;
      UCHAR NumMarks;
      UCHAR NumMarksLSB;
      union
	{
	  UCHAR value;
	  struct
	    {
	      UCHAR Link:1;
	      UCHAR Flag:1;
	      UCHAR Reserved:4;
	      UCHAR VendorUnique:2;
	    } Fields;
	} Byte6;
    } SPACE_TAPE_MARKS, *PSPACE_TAPE_MARKS;

  /* Read tape position */
  struct _READ_POSITION
    {
      UCHAR Operation;
      UCHAR BlockType:1;
      UCHAR Reserved1:4;
      UCHAR Lun:3;
      UCHAR Reserved2[7];
      UCHAR Control;
    } READ_POSITION, *PREAD_POSITION;

  /* ReadWrite for Tape */
  struct _CDB6READWRITETAPE
    {
      UCHAR OperationCode;
      UCHAR VendorSpecific:5;
      UCHAR Reserved:3;
      UCHAR TransferLenMSB;
      UCHAR TransferLen;
      UCHAR TransferLenLSB;
      UCHAR Link:1;
      UCHAR Flag:1;
      UCHAR Reserved1:4;
      UCHAR VendorUnique:2;
     } CDB6READWRITETAPE, *PCDB6READWRITETAPE;

  /* Atapi 2.5 Changer 12-byte CDBs */
  struct _LOAD_UNLOAD
    {
      UCHAR OperationCode;
      UCHAR Immediate:1;
      UCHAR Reserved1:7;
      UCHAR Reserved2[2];
      UCHAR Start:1;
      UCHAR LoadEject:1;
      UCHAR Reserved3:6;
      UCHAR Reserved4[3];
      UCHAR Slot;
      UCHAR Reserved5[3];
    } LOAD_UNLOAD, *PLOAD_UNLOAD;

  struct _MECH_STATUS
    {
      UCHAR OperationCode;
      UCHAR Reserved1[7];
      UCHAR AllocationLength[2];
      UCHAR Reserved2[2];
    } MECH_STATUS, *PMECH_STATUS;
} CDB, *PCDB;


/* Command Descriptor Block constants */

#define CDB6GENERIC_LENGTH                   6
#define CDB10GENERIC_LENGTH                  10
#define CDB12GENERIC_LENGTH                  12

#define SETBITON                             1
#define SETBITOFF                            0


/* Mode Sense/Select page constants */

#define MODE_PAGE_ERROR_RECOVERY        0x01
#define MODE_PAGE_DISCONNECT            0x02
#define MODE_PAGE_FORMAT_DEVICE         0x03
#define MODE_PAGE_RIGID_GEOMETRY        0x04
#define MODE_PAGE_FLEXIBILE             0x05
#define MODE_PAGE_VERIFY_ERROR          0x07
#define MODE_PAGE_CACHING               0x08
#define MODE_PAGE_PERIPHERAL            0x09
#define MODE_PAGE_CONTROL               0x0A
#define MODE_PAGE_MEDIUM_TYPES          0x0B
#define MODE_PAGE_NOTCH_PARTITION       0x0C
#define MODE_SENSE_RETURN_ALL           0x3f
#define MODE_SENSE_CURRENT_VALUES       0x00
#define MODE_SENSE_CHANGEABLE_VALUES    0x40
#define MODE_SENSE_DEFAULT_VAULES       0x80
#define MODE_SENSE_SAVED_VALUES         0xc0
#define MODE_PAGE_DEVICE_CONFIG         0x10
#define MODE_PAGE_MEDIUM_PARTITION      0x11
#define MODE_PAGE_DATA_COMPRESS         0x0f
#define MODE_PAGE_CAPABILITIES          0x2A


/* SCSI CDB operation codes */

#define SCSIOP_TEST_UNIT_READY     0x00
#define SCSIOP_REZERO_UNIT         0x01
#define SCSIOP_REWIND              0x01
#define SCSIOP_REQUEST_BLOCK_ADDR  0x02
#define SCSIOP_REQUEST_SENSE       0x03
#define SCSIOP_FORMAT_UNIT         0x04
#define SCSIOP_READ_BLOCK_LIMITS   0x05
#define SCSIOP_REASSIGN_BLOCKS     0x07
#define SCSIOP_READ6               0x08
#define SCSIOP_RECEIVE             0x08
#define SCSIOP_WRITE6              0x0A
#define SCSIOP_PRINT               0x0A
#define SCSIOP_SEND                0x0A
#define SCSIOP_SEEK6               0x0B
#define SCSIOP_TRACK_SELECT        0x0B
#define SCSIOP_SLEW_PRINT          0x0B
#define SCSIOP_SEEK_BLOCK          0x0C
#define SCSIOP_PARTITION           0x0D
#define SCSIOP_READ_REVERSE        0x0F
#define SCSIOP_WRITE_FILEMARKS     0x10
#define SCSIOP_FLUSH_BUFFER        0x10
#define SCSIOP_SPACE               0x11
#define SCSIOP_INQUIRY             0x12
#define SCSIOP_VERIFY6             0x13
#define SCSIOP_RECOVER_BUF_DATA    0x14
#define SCSIOP_MODE_SELECT         0x15
#define SCSIOP_RESERVE_UNIT        0x16
#define SCSIOP_RELEASE_UNIT        0x17
#define SCSIOP_COPY                0x18
#define SCSIOP_ERASE               0x19
#define SCSIOP_MODE_SENSE          0x1A
#define SCSIOP_START_STOP_UNIT     0x1B
#define SCSIOP_STOP_PRINT          0x1B
#define SCSIOP_LOAD_UNLOAD         0x1B
#define SCSIOP_RECEIVE_DIAGNOSTIC  0x1C
#define SCSIOP_SEND_DIAGNOSTIC     0x1D
#define SCSIOP_MEDIUM_REMOVAL      0x1E
#define SCSIOP_READ_CAPACITY       0x25
#define SCSIOP_READ                0x28
#define SCSIOP_WRITE               0x2A
#define SCSIOP_SEEK                0x2B
#define SCSIOP_LOCATE              0x2B
#define SCSIOP_WRITE_VERIFY        0x2E
#define SCSIOP_VERIFY              0x2F
#define SCSIOP_SEARCH_DATA_HIGH    0x30
#define SCSIOP_SEARCH_DATA_EQUAL   0x31
#define SCSIOP_SEARCH_DATA_LOW     0x32
#define SCSIOP_SET_LIMITS          0x33
#define SCSIOP_READ_POSITION       0x34
#define SCSIOP_SYNCHRONIZE_CACHE   0x35
#define SCSIOP_COMPARE             0x39
#define SCSIOP_COPY_COMPARE        0x3A
#define SCSIOP_WRITE_DATA_BUFF     0x3B
#define SCSIOP_READ_DATA_BUFF      0x3C
#define SCSIOP_CHANGE_DEFINITION   0x40
#define SCSIOP_READ_SUB_CHANNEL    0x42
#define SCSIOP_READ_TOC            0x43
#define SCSIOP_READ_HEADER         0x44
#define SCSIOP_PLAY_AUDIO          0x45
#define SCSIOP_PLAY_AUDIO_MSF      0x47
#define SCSIOP_PLAY_TRACK_INDEX    0x48
#define SCSIOP_PLAY_TRACK_RELATIVE 0x49
#define SCSIOP_PAUSE_RESUME        0x4B
#define SCSIOP_LOG_SELECT          0x4C
#define SCSIOP_LOG_SENSE           0x4D
#define SCSIOP_MODE_SELECT10       0x55
#define SCSIOP_MODE_SENSE10        0x5A
#define SCSIOP_LOAD_UNLOAD_SLOT    0xA6
#define SCSIOP_MECHANISM_STATUS    0xBD
#define SCSIOP_READ_CD             0xBE

//
// If the IMMED bit is 1, status is returned as soon
// as the operation is initiated. If the IMMED bit
// is 0, status is not returned until the operation
// is completed.
//

#define CDB_RETURN_ON_COMPLETION   0
#define CDB_RETURN_IMMEDIATE       1


//
// CDB Force media access used in extended read and write commands.
//

#define CDB_FORCE_MEDIA_ACCESS 0x08

//
// Denon CD ROM operation codes
//

#define SCSIOP_DENON_EJECT_DISC    0xE6
#define SCSIOP_DENON_STOP_AUDIO    0xE7
#define SCSIOP_DENON_PLAY_AUDIO    0xE8
#define SCSIOP_DENON_READ_TOC      0xE9
#define SCSIOP_DENON_READ_SUBCODE  0xEB

//
// SCSI Bus Messages
//

#define SCSIMESS_ABORT                0x06
#define SCSIMESS_ABORT_WITH_TAG       0x0D
#define SCSIMESS_BUS_DEVICE_RESET     0X0C
#define SCSIMESS_CLEAR_QUEUE          0X0E
#define SCSIMESS_COMMAND_COMPLETE     0X00
#define SCSIMESS_DISCONNECT           0X04
#define SCSIMESS_EXTENDED_MESSAGE     0X01
#define SCSIMESS_IDENTIFY             0X80
#define SCSIMESS_IDENTIFY_WITH_DISCON 0XC0
#define SCSIMESS_IGNORE_WIDE_RESIDUE  0X23
#define SCSIMESS_INITIATE_RECOVERY    0X0F
#define SCSIMESS_INIT_DETECTED_ERROR  0X05
#define SCSIMESS_LINK_CMD_COMP        0X0A
#define SCSIMESS_LINK_CMD_COMP_W_FLAG 0X0B
#define SCSIMESS_MESS_PARITY_ERROR    0X09
#define SCSIMESS_MESSAGE_REJECT       0X07
#define SCSIMESS_NO_OPERATION         0X08
#define SCSIMESS_HEAD_OF_QUEUE_TAG    0X21
#define SCSIMESS_ORDERED_QUEUE_TAG    0X22
#define SCSIMESS_SIMPLE_QUEUE_TAG     0X20
#define SCSIMESS_RELEASE_RECOVERY     0X10
#define SCSIMESS_RESTORE_POINTERS     0X03
#define SCSIMESS_SAVE_DATA_POINTER    0X02
#define SCSIMESS_TERMINATE_IO_PROCESS 0X11

//
// SCSI Extended Message operation codes
//

#define SCSIMESS_MODIFY_DATA_POINTER  0X00
#define SCSIMESS_SYNCHRONOUS_DATA_REQ 0X01
#define SCSIMESS_WIDE_DATA_REQUEST    0X03

//
// SCSI Extended Message Lengths
//

#define SCSIMESS_MODIFY_DATA_LENGTH   5
#define SCSIMESS_SYNCH_DATA_LENGTH    3
#define SCSIMESS_WIDE_DATA_LENGTH     2

//
// SCSI extended message structure
//

#pragma pack (1)
typedef struct _SCSI_EXTENDED_MESSAGE
{
  UCHAR InitialMessageCode;
  UCHAR MessageLength;
  UCHAR MessageType;
  union _EXTENDED_ARGUMENTS
    {
      struct
	{
	  UCHAR Modifier[4];
	} Modify;

      struct
	{
	  UCHAR TransferPeriod;
	  UCHAR ReqAckOffset;
	} Synchronous;

      struct
	{
	  UCHAR Width;
	} Wide;
    } ExtendedArguments;
}SCSI_EXTENDED_MESSAGE, *PSCSI_EXTENDED_MESSAGE;
#pragma pack ()


/* SCSI bus status codes */

#define SCSISTAT_GOOD                  0x00
#define SCSISTAT_CHECK_CONDITION       0x02
#define SCSISTAT_CONDITION_MET         0x04
#define SCSISTAT_BUSY                  0x08
#define SCSISTAT_INTERMEDIATE          0x10
#define SCSISTAT_INTERMEDIATE_COND_MET 0x14
#define SCSISTAT_RESERVATION_CONFLICT  0x18
#define SCSISTAT_COMMAND_TERMINATED    0x22
#define SCSISTAT_QUEUE_FULL            0x28

//
// Enable Vital Product Data Flag (EVPD)
// used with INQUIRY command.
//

#define CDB_INQUIRY_EVPD           0x01

//
// Defines for format CDB
//

#define LUN0_FORMAT_SAVING_DEFECT_LIST 0
#define USE_DEFAULTMSB  0
#define USE_DEFAULTLSB  0

#define START_UNIT_CODE 0x01
#define STOP_UNIT_CODE  0x00



//
// Inquiry buffer structure. This is the data returned from the target
// after it receives an inquiry.
//
// This structure may be extended by the number of bytes specified
// in the field AdditionalLength. The defined size constant only
// includes fields through ProductRevisionLevel.
//
// The NT SCSI drivers are only interested in the first 36 bytes of data.
//

#define INQUIRYDATABUFFERSIZE 36


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

//
// Inquiry defines. Used to interpret data returned from target as result
// of inquiry command.
//
// DeviceType field
//

#define DIRECT_ACCESS_DEVICE            0x00    // disks
#define SEQUENTIAL_ACCESS_DEVICE        0x01    // tapes
#define PRINTER_DEVICE                  0x02    // printers
#define PROCESSOR_DEVICE                0x03    // scanners, printers, etc
#define WRITE_ONCE_READ_MULTIPLE_DEVICE 0x04    // worms
#define READ_ONLY_DIRECT_ACCESS_DEVICE  0x05    // cdroms
#define SCANNER_DEVICE                  0x06    // scanners
#define OPTICAL_DEVICE                  0x07    // optical disks
#define MEDIUM_CHANGER                  0x08    // jukebox
#define COMMUNICATION_DEVICE            0x09    // network
#define LOGICAL_UNIT_NOT_PRESENT_DEVICE 0x7F
#define DEVICE_QUALIFIER_NOT_SUPPORTED  0x03

//
// DeviceTypeQualifier field
//

#define DEVICE_CONNECTED 0x00


/* Sense Data Format */

typedef struct _SENSE_DATA
{
  UCHAR ErrorCode:7;
  UCHAR Valid:1;
  UCHAR SegmentNumber;
  UCHAR SenseKey:4;
  UCHAR Reserved:1;
  UCHAR IncorrectLength:1;
  UCHAR EndOfMedia:1;
  UCHAR FileMark:1;
  UCHAR Information[4];
  UCHAR AdditionalSenseLength;
  UCHAR CommandSpecificInformation[4];
  UCHAR AdditionalSenseCode;
  UCHAR AdditionalSenseCodeQualifier;
  UCHAR FieldReplaceableUnitCode;
  UCHAR SenseKeySpecific[3];
} SENSE_DATA, *PSENSE_DATA;


/* Default request sense buffer size */

#define SENSE_BUFFER_SIZE 18

/* Sense codes */

#define SCSI_SENSE_NO_SENSE         0x00
#define SCSI_SENSE_RECOVERED_ERROR  0x01
#define SCSI_SENSE_NOT_READY        0x02
#define SCSI_SENSE_MEDIUM_ERROR     0x03
#define SCSI_SENSE_HARDWARE_ERROR   0x04
#define SCSI_SENSE_ILLEGAL_REQUEST  0x05
#define SCSI_SENSE_UNIT_ATTENTION   0x06
#define SCSI_SENSE_DATA_PROTECT     0x07
#define SCSI_SENSE_BLANK_CHECK      0x08
#define SCSI_SENSE_UNIQUE           0x09
#define SCSI_SENSE_COPY_ABORTED     0x0A
#define SCSI_SENSE_ABORTED_COMMAND  0x0B
#define SCSI_SENSE_EQUAL            0x0C
#define SCSI_SENSE_VOL_OVERFLOW     0x0D
#define SCSI_SENSE_MISCOMPARE       0x0E
#define SCSI_SENSE_RESERVED         0x0F

/* Additional tape bit */

#define SCSI_ILLEGAL_LENGTH         0x20
#define SCSI_EOM                    0x40
#define SCSI_FILE_MARK              0x80

/* Additional Sense codes */

#define SCSI_ADSENSE_NO_SENSE       0x00
#define SCSI_ADSENSE_LUN_NOT_READY  0x04
#define SCSI_ADSENSE_ILLEGAL_COMMAND 0x20
#define SCSI_ADSENSE_ILLEGAL_BLOCK  0x21
#define SCSI_ADSENSE_INVALID_LUN    0x25
#define SCSI_ADSENSE_INVALID_CDB    0x24
#define SCSI_ADSENSE_MUSIC_AREA     0xA0
#define SCSI_ADSENSE_DATA_AREA      0xA1
#define SCSI_ADSENSE_VOLUME_OVERFLOW 0xA7

#define SCSI_ADSENSE_NO_MEDIA_IN_DEVICE 0x3a
#define SCSI_ADWRITE_PROTECT        0x27
#define SCSI_ADSENSE_MEDIUM_CHANGED 0x28
#define SCSI_ADSENSE_BUS_RESET      0x29
#define SCSI_ADSENSE_TRACK_ERROR    0x14
#define SCSI_ADSENSE_SEEK_ERROR     0x15
#define SCSI_ADSENSE_REC_DATA_NOECC 0x17
#define SCSI_ADSENSE_REC_DATA_ECC   0x18

/* Additional sense code qualifier */

#define SCSI_SENSEQ_FORMAT_IN_PROGRESS 0x04
#define SCSI_SENSEQ_INIT_COMMAND_REQUIRED 0x02
#define SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED 0x03
#define SCSI_SENSEQ_BECOMING_READY 0x01
#define SCSI_SENSEQ_FILEMARK_DETECTED 0x01
#define SCSI_SENSEQ_SETMARK_DETECTED 0x03
#define SCSI_SENSEQ_END_OF_MEDIA_DETECTED 0x02
#define SCSI_SENSEQ_BEGINNING_OF_MEDIA_DETECTED 0x04

/* SCSI IO Device Control Codes */

#define FILE_DEVICE_SCSI 0x0000001b

#define IOCTL_SCSI_EXECUTE_IN   ((FILE_DEVICE_SCSI << 16) + 0x0011)
#define IOCTL_SCSI_EXECUTE_OUT  ((FILE_DEVICE_SCSI << 16) + 0x0012)
#define IOCTL_SCSI_EXECUTE_NONE ((FILE_DEVICE_SCSI << 16) + 0x0013)

/* SMART support in atapi */

#define IOCTL_SCSI_MINIPORT_SMART_VERSION           ((FILE_DEVICE_SCSI << 16) + 0x0500)
#define IOCTL_SCSI_MINIPORT_IDENTIFY                ((FILE_DEVICE_SCSI << 16) + 0x0501)
#define IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS      ((FILE_DEVICE_SCSI << 16) + 0x0502)
#define IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS   ((FILE_DEVICE_SCSI << 16) + 0x0503)
#define IOCTL_SCSI_MINIPORT_ENABLE_SMART            ((FILE_DEVICE_SCSI << 16) + 0x0504)
#define IOCTL_SCSI_MINIPORT_DISABLE_SMART           ((FILE_DEVICE_SCSI << 16) + 0x0505)
#define IOCTL_SCSI_MINIPORT_RETURN_STATUS           ((FILE_DEVICE_SCSI << 16) + 0x0506)
#define IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE ((FILE_DEVICE_SCSI << 16) + 0x0507)
#define IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES   ((FILE_DEVICE_SCSI << 16) + 0x0508)
#define IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS   ((FILE_DEVICE_SCSI << 16) + 0x0509)


/* Read Capacity Data - returned in Big Endian format */

typedef struct _READ_CAPACITY_DATA
{
  ULONG LogicalBlockAddress;
  ULONG BytesPerBlock;
} READ_CAPACITY_DATA, *PREAD_CAPACITY_DATA;


//
// Read Block Limits Data - returned in Big Endian format
// This structure returns the maximum and minimum block
// size for a TAPE device.
//

typedef struct _READ_BLOCK_LIMITS
{
  UCHAR Reserved;
  UCHAR BlockMaximumSize[3];
  UCHAR BlockMinimumSize[2];
} READ_BLOCK_LIMITS_DATA, *PREAD_BLOCK_LIMITS_DATA;


//
// Mode data structures.
//

//
// Define Mode parameter header.
//

typedef struct _MODE_PARAMETER_HEADER
{
  UCHAR ModeDataLength;
  UCHAR MediumType;
  UCHAR DeviceSpecificParameter;
  UCHAR BlockDescriptorLength;
}MODE_PARAMETER_HEADER, *PMODE_PARAMETER_HEADER;

typedef struct _MODE_PARAMETER_HEADER10
{
  UCHAR ModeDataLength[2];
  UCHAR MediumType;
  UCHAR DeviceSpecificParameter;
  UCHAR Reserved[2];
  UCHAR BlockDescriptorLength[2];
}MODE_PARAMETER_HEADER10, *PMODE_PARAMETER_HEADER10;

#define MODE_FD_SINGLE_SIDE     0x01
#define MODE_FD_DOUBLE_SIDE     0x02
#define MODE_FD_MAXIMUM_TYPE    0x1E
#define MODE_DSP_FUA_SUPPORTED  0x10
#define MODE_DSP_WRITE_PROTECT  0x80

//
// Define the mode parameter block.
//

typedef struct _MODE_PARAMETER_BLOCK
{
  UCHAR DensityCode;
  UCHAR NumberOfBlocks[3];
  UCHAR Reserved;
  UCHAR BlockLength[3];
}MODE_PARAMETER_BLOCK, *PMODE_PARAMETER_BLOCK;


/* Define Disconnect-Reconnect page */
typedef struct _MODE_DISCONNECT_PAGE
{
  UCHAR PageCode:6;
  UCHAR Reserved:1;
  UCHAR PageSavable:1;
  UCHAR PageLength;
  UCHAR BufferFullRatio;
  UCHAR BufferEmptyRatio;
  UCHAR BusInactivityLimit[2];
  UCHAR BusDisconnectTime[2];
  UCHAR BusConnectTime[2];
  UCHAR MaximumBurstSize[2];
  UCHAR DataTransferDisconnect:2;
  UCHAR Reserved2[3];
}MODE_DISCONNECT_PAGE, *PMODE_DISCONNECT_PAGE;

//
// Define mode caching page.
//

typedef struct _MODE_CACHING_PAGE
{
  UCHAR PageCode:6;
  UCHAR Reserved:1;
  UCHAR PageSavable:1;
  UCHAR PageLength;
  UCHAR ReadDisableCache:1;
  UCHAR MultiplicationFactor:1;
  UCHAR WriteCacheEnable:1;
  UCHAR Reserved2:5;
  UCHAR WriteRetensionPriority:4;
  UCHAR ReadRetensionPriority:4;
  UCHAR DisablePrefetchTransfer[2];
  UCHAR MinimumPrefetch[2];
  UCHAR MaximumPrefetch[2];
  UCHAR MaximumPrefetchCeiling[2];
}MODE_CACHING_PAGE, *PMODE_CACHING_PAGE;

//
// Define mode flexible disk page.
//

typedef struct _MODE_FLEXIBLE_DISK_PAGE
{
  UCHAR PageCode:6;
  UCHAR Reserved:1;
  UCHAR PageSavable:1;
  UCHAR PageLength;
  UCHAR TransferRate[2];
  UCHAR NumberOfHeads;
  UCHAR SectorsPerTrack;
  UCHAR BytesPerSector[2];
  UCHAR NumberOfCylinders[2];
  UCHAR StartWritePrecom[2];
  UCHAR StartReducedCurrent[2];
  UCHAR StepRate[2];
  UCHAR StepPluseWidth;
  UCHAR HeadSettleDelay[2];
  UCHAR MotorOnDelay;
  UCHAR MotorOffDelay;
  UCHAR Reserved2:5;
  UCHAR MotorOnAsserted:1;
  UCHAR StartSectorNumber:1;
  UCHAR TrueReadySignal:1;
  UCHAR StepPlusePerCyclynder:4;
  UCHAR Reserved3:4;
  UCHAR WriteCompenstation;
  UCHAR HeadLoadDelay;
  UCHAR HeadUnloadDelay;
  UCHAR Pin2Usage:4;
  UCHAR Pin34Usage:4;
  UCHAR Pin1Usage:4;
  UCHAR Pin4Usage:4;
  UCHAR MediumRotationRate[2];
  UCHAR Reserved4[2];
}MODE_FLEXIBLE_DISK_PAGE, *PMODE_FLEXIBLE_DISK_PAGE;

//
// Define mode format page.
//

typedef struct _MODE_FORMAT_PAGE
{
  UCHAR PageCode:6;
  UCHAR Reserved:1;
  UCHAR PageSavable:1;
  UCHAR PageLength;
  UCHAR TracksPerZone[2];
  UCHAR AlternateSectorsPerZone[2];
  UCHAR AlternateTracksPerZone[2];
  UCHAR AlternateTracksPerLogicalUnit[2];
  UCHAR SectorsPerTrack[2];
  UCHAR BytesPerPhysicalSector[2];
  UCHAR Interleave[2];
  UCHAR TrackSkewFactor[2];
  UCHAR CylinderSkewFactor[2];
  UCHAR Reserved2:4;
  UCHAR SurfaceFirst:1;
  UCHAR RemovableMedia:1;
  UCHAR HardSectorFormating:1;
  UCHAR SoftSectorFormating:1;
  UCHAR Reserved3[2];
}MODE_FORMAT_PAGE, *PMODE_FORMAT_PAGE;

//
// Define rigid disk driver geometry page.
//

typedef struct _MODE_RIGID_GEOMETRY_PAGE
{
  UCHAR PageCode:6;
  UCHAR Reserved:1;
  UCHAR PageSavable:1;
  UCHAR PageLength;
  UCHAR NumberOfCylinders[2];
  UCHAR NumberOfHeads;
  UCHAR StartWritePrecom[2];
  UCHAR StartReducedCurrent[2];
  UCHAR DriveStepRate[2];
  UCHAR LandZoneCyclinder[2];
  UCHAR RotationalPositionLock:2;
  UCHAR Reserved2:6;
  UCHAR RotationOffset;
  UCHAR Reserved3;
  UCHAR RoataionRate[2];
  UCHAR Reserved4[2];
}MODE_RIGID_GEOMETRY_PAGE, *PMODE_RIGID_GEOMETRY_PAGE;

//
// Define read write recovery page
//

typedef struct _MODE_READ_WRITE_RECOVERY_PAGE
{
  UCHAR PageCode:6;
  UCHAR Reserved1:1;
  UCHAR PSBit:1;
  UCHAR PageLength;
  UCHAR DCRBit:1;
  UCHAR DTEBit:1;
  UCHAR PERBit:1;
  UCHAR EERBit:1;
  UCHAR RCBit:1;
  UCHAR TBBit:1;
  UCHAR ARRE:1;
  UCHAR AWRE:1;
  UCHAR ReadRetryCount;
  UCHAR Reserved4[4];
  UCHAR WriteRetryCount;
  UCHAR Reserved5[3];
} MODE_READ_WRITE_RECOVERY_PAGE, *PMODE_READ_WRITE_RECOVERY_PAGE;

//
// Define read recovery page - cdrom
//

typedef struct _MODE_READ_RECOVERY_PAGE
{
  UCHAR PageCode:6;
  UCHAR Reserved1:1;
  UCHAR PSBit:1;
  UCHAR PageLength;
  UCHAR DCRBit:1;
  UCHAR DTEBit:1;
  UCHAR PERBit:1;
  UCHAR Reserved2:1;
  UCHAR RCBit:1;
  UCHAR TBBit:1;
  UCHAR Reserved3:2;
  UCHAR ReadRetryCount;
  UCHAR Reserved4[4];
} MODE_READ_RECOVERY_PAGE, *PMODE_READ_RECOVERY_PAGE;

//
// Define CD-ROM Capabilities and Mechanical Status Page.
//

typedef struct _MODE_CAPABILITIES_PAGE2
{
  UCHAR PageCode:6;
  UCHAR Reserved1:1;
  UCHAR PSBit:1;
  UCHAR PageLength;
  UCHAR Reserved2[2];
  UCHAR Capabilities[4];
  UCHAR MaximumSpeedSupported[2];
  UCHAR Reserved3;
  UCHAR NumberVolumeLevels;
  UCHAR BufferSize[2];
  UCHAR CurrentSpeed[2];
  UCHAR Reserved4;
  UCHAR Reserved5:1;
  UCHAR DigitalOutput:4;
  UCHAR Reserved6:3;
  UCHAR Reserved7[2];
} MODE_CAPABILITIES_PAGE2, *PMODE_CAPABILITIES_PAGE2;

//
// Bit definitions of Capabilites and DigitalOutput
//

#define AUDIO_PLAY_SUPPORTED    0x01
#define MODE2_FORM1_SUPPORTED   0x10
#define MODE2_FORM2_SUPPORTED   0x20
#define MULTI_SESSION_SUPPORTED 0x40

#define READ_CD_SUPPORTED       0x01

#define MEDIA_LOCKING_SUPPORTED 0x01
#define CURRENT_LOCK_STATE      0x02
#define SOFTWARE_EJECT_SUPPORT  0x08

#define DISC_PRESENT_REPORTING 0x04

//
// Mode parameter list block descriptor -
// set the block length for reading/writing
//
//

#define MODE_BLOCK_DESC_LENGTH               8
#define MODE_HEADER_LENGTH                   4
#define MODE_HEADER_LENGTH10                 8

typedef struct _MODE_PARM_READ_WRITE
{
  MODE_PARAMETER_HEADER ParameterListHeader;	/* List Header Format */
  MODE_PARAMETER_BLOCK ParameterListBlock;	/* List Block Descriptor */
} MODE_PARM_READ_WRITE_DATA, *PMODE_PARM_READ_WRITE_DATA;


//
// CDROM audio control (0x0E)
//

#define CDB_AUDIO_PAUSE 0
#define CDB_AUDIO_RESUME 1

#define CDB_DEVICE_START 0x11
#define CDB_DEVICE_STOP 0x10

#define CDB_EJECT_MEDIA 0x10
#define CDB_LOAD_MEDIA 0x01

#define CDB_SUBCHANNEL_HEADER      0x00
#define CDB_SUBCHANNEL_BLOCK       0x01

#define CDROM_AUDIO_CONTROL_PAGE   0x0E
#define MODE_SELECT_IMMEDIATE      0x04
#define MODE_SELECT_PFBIT          0x10

#define CDB_USE_MSF                0x01

typedef struct _PORT_OUTPUT
{
  UCHAR ChannelSelection;
  UCHAR Volume;
} PORT_OUTPUT, *PPORT_OUTPUT;

typedef struct _AUDIO_OUTPUT
{
  UCHAR CodePage;
  UCHAR ParameterLength;
  UCHAR Immediate;
  UCHAR Reserved[2];
  UCHAR LbaFormat;
  UCHAR LogicalBlocksPerSecond[2];
  PORT_OUTPUT PortOutput[4];
} AUDIO_OUTPUT, *PAUDIO_OUTPUT;


/* Multisession CDROM */

#define GET_LAST_SESSION 0x01
#define GET_SESSION_DATA 0x02;


/* Atapi 2.5 changer */

typedef struct _MECHANICAL_STATUS_INFORMATION_HEADER
{
  UCHAR CurrentSlot:5;
  UCHAR ChangerState:2;
  UCHAR Fault:1;
  UCHAR Reserved:5;
  UCHAR MechanismState:3;
  UCHAR CurrentLogicalBlockAddress[3];
  UCHAR NumberAvailableSlots;
  UCHAR SlotTableLength[2];
} MECHANICAL_STATUS_INFORMATION_HEADER, *PMECHANICAL_STATUS_INFORMATION_HEADER;

typedef struct _SLOT_TABLE_INFORMATION
{
  UCHAR DiscChanged:1;
  UCHAR Reserved:6;
  UCHAR DiscPresent:1;
  UCHAR Reserved2[3];
} SLOT_TABLE_INFORMATION, *PSLOT_TABLE_INFORMATION;

typedef struct _MECHANICAL_STATUS
{
  MECHANICAL_STATUS_INFORMATION_HEADER MechanicalStatusHeader;
  SLOT_TABLE_INFORMATION SlotTableInfo[1];
} MECHANICAL_STATUS, *PMECHANICAL_STATUS;


/* Tape definitions */

typedef struct _TAPE_POSITION_DATA
{
  UCHAR Reserved1:2;
  UCHAR BlockPositionUnsupported:1;
  UCHAR Reserved2:3;
  UCHAR EndOfPartition:1;
  UCHAR BeginningOfPartition:1;
  UCHAR PartitionNumber;
  USHORT Reserved3;
  UCHAR FirstBlock[4];
  UCHAR LastBlock[4];
  UCHAR Reserved4;
  UCHAR NumberOfBlocks[3];
  UCHAR NumberOfBytes[4];
} TAPE_POSITION_DATA, *PTAPE_POSITION_DATA;

/*
 * This structure is used to convert little endian
 * ULONGs to SCSI CDB 4 byte big endians values.
 */

typedef struct _FOUR_BYTE
{
  UCHAR Byte0;
  UCHAR Byte1;
  UCHAR Byte2;
  UCHAR Byte3;
} FOUR_BYTE, *PFOUR_BYTE;

/*
 * Byte reversing macro for converting
 * between big- and little-endian formats
 */

#define REVERSE_BYTES(Destination, Source) \
{ \
  PFOUR_BYTE d = (PFOUR_BYTE)(Destination); \
  PFOUR_BYTE s = (PFOUR_BYTE)(Source); \
  d->Byte3 = s->Byte0; \
  d->Byte2 = s->Byte1; \
  d->Byte1 = s->Byte2; \
  d->Byte0 = s->Byte3; \
}


/* This macro has the effect of Bit = log2(Data) */

#define WHICH_BIT(Data, Bit) \
{ \
  for (Bit = 0; Bit < 32; Bit++) \
    { \
      if ((Data >> Bit) == 1) \
        { \
          break; \
        } \
    } \
}


#endif /* __STORAGE_INCLUDE_SCSI_H */

/* EOF */