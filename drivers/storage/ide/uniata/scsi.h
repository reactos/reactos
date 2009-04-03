/*++

Copyright (c) 2002-2005 Alexandr A. Telyatnikov (Alter)

Module Name:
    scsi.h

Abstract:
    This file contains SCSI protocol definitions

Author:
    Alexander A. Telyatnikov (Alter)

Environment:
    kernel mode only

Notes:

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Revision History:

--*/
#ifndef __CDRW_DEVICE_H__
#define __CDRW_DEVICE_H__

#include "srb.h"

#pragma pack(push, 1)

// Command Descriptor Block. Passed by SCSI controller chip over the SCSI bus

typedef union _CDB {

    // Generic 6-Byte CDB
    struct _CDB6 {
       UCHAR  OperationCode;
       UCHAR  Immediate : 1;
       UCHAR  CommandUniqueBits : 4;
       UCHAR  Lun : 3;
       UCHAR  CommandUniqueBytes[3];
       UCHAR  Link : 1;
       UCHAR  Flag : 1;
       UCHAR  Reserved : 4;
       UCHAR  VendorUnique : 2;
    } CDB6, *PCDB6;

    struct _REQUEST_SENSE {
        UCHAR OperationCode;
        UCHAR Reserved0 : 5;
        UCHAR Lun : 3;
        UCHAR Reserved1[2];
        UCHAR AllocationLength;
        UCHAR Link : 1;
        UCHAR Flag : 1;
        UCHAR Reserved2 : 6;
    } REQUEST_SENSE, *PREQUEST_SENSE;

    // Standard 6-byte CDB
    struct _CDB6READWRITE {
        UCHAR OperationCode;
        UCHAR LBA2 : 5;
        UCHAR Lun : 3;
        UCHAR LBA0[2];
        UCHAR NumOfBlocks;
        UCHAR Control;
    } CDB6READWRITE, *PCDB6READWRITE;

    // SCSI Inquiry CDB
    struct _CDB6INQUIRY {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR Lun : 3;
        UCHAR PageCode;
        UCHAR IReserved;
        UCHAR AllocationLength;
        UCHAR Control;
    } CDB6INQUIRY, *PCDB6INQUIRY;

    // SCSI Format CDB

    struct _ERASE {
        UCHAR OperationCode;
        UCHAR Long : 1;
        UCHAR Immediate : 1;
        UCHAR Reserved1 : 3;
        UCHAR Lun : 3;
        UCHAR Reserved2[3];
        UCHAR Control;
    } ERASE, *PERASE;

#define FormatUnit_Code_Mask    0x07
#define FormatUnit_Cmp          0x08
#define FormatUnit_Fmt          0x10

    struct _CDB6FORMAT {
        UCHAR OperationCode;
        union {
            UCHAR Flags;
            struct {
                UCHAR FormatCode : 3;
                UCHAR Cmp:1;
                UCHAR Fmt:1;
                UCHAR Lun : 3;
            } Fields;
        } Byte1;
        UCHAR FReserved1;
        UCHAR Interleave[2];
        UCHAR FReserved2;
    } CDB6FORMAT, *PCDB6FORMAT;

    // Standard 10-byte CDB
    struct _CDB10 {
        UCHAR OperationCode;
        UCHAR RelativeAddress : 1;
        UCHAR Reserved1 : 2;
        UCHAR ForceUnitAccess : 1;
        UCHAR DisablePageOut : 1;
        UCHAR Lun : 3;
        UCHAR LBA[4];
        UCHAR Reserved2;
        UCHAR TransferBlocks[2];
        UCHAR Control;
    } CDB10, *PCDB10;

    // CD Rom Audio CDBs

#define PauseResume_Pause   0x00
#define PauseResume_Resume  0x01

    struct _PAUSE_RESUME {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR Lun : 3;
        UCHAR Reserved2[6];
        UCHAR Action;
        UCHAR Control;
    } PAUSE_RESUME, *PPAUSE_RESUME;

    // Read Table of Contents (TOC)

#define ReadTOC_Format_Mask     0x0f
#define ReadTOC_Format_TOC      0x00
#define ReadTOC_Format_SesInfo  0x01
#define ReadTOC_Format_FullTOC  0x02
#define ReadTOC_Format_PMA      0x03
#define ReadTOC_Format_ATIP     0x04
#define ReadTOC_Format_CdText   0x05

    struct _READ_TOC {
        UCHAR OperationCode;
        UCHAR Reserved0 : 1;
        UCHAR Msf : 1;  // HMSF MMC-3
        UCHAR Reserved1 : 3;
        UCHAR Lun : 3;

        union {
            UCHAR Flags;
            struct {
                UCHAR Format : 4;
                UCHAR Reserved : 4;
            } Fields;
        } Byte2;

        UCHAR Reserved2[3];
        UCHAR Start_TrackSes;
        UCHAR AllocationLength[2];
        UCHAR Control : 6;
        UCHAR Format : 2;
    } READ_TOC, *PREAD_TOC;

    // Play Audio MSF
    struct _PLAY_AUDIO_MSF {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR Lun : 3;
        UCHAR Reserved2;
        UCHAR StartingMSF[3];
        UCHAR EndingMSF[3];
        UCHAR Control;
    } PLAY_AUDIO_MSF, *PPLAY_AUDIO_MSF;

    // Read SubChannel Data

#define SubChannel_SubQ_Header      0x00
#define SubChannel_SubQ_Block       0x01

    struct _SUBCHANNEL {
        UCHAR OperationCode;
        UCHAR Reserved0 : 1;
        UCHAR Msf : 1;
        UCHAR Reserved1 : 3;
        UCHAR Lun : 3;
        UCHAR Reserved2 : 6;
        UCHAR SubQ : 1;
        UCHAR Reserved3 : 1;
        UCHAR Format;
        UCHAR Reserved4[2];
        UCHAR TrackNumber;
        UCHAR AllocationLength[2];
        UCHAR Control;
    } SUBCHANNEL, *PSUBCHANNEL;

    // Read CD (by LBA/MSF). Used by Atapi for raw sector reads.

#define ReadCD_SecType_Mask    0x1c
#define ReadCD_SecType_Any     0x00
#define ReadCD_SecType_CDDA    0x04
#define ReadCD_SecType_M1      0x08
#define ReadCD_SecType_M2      0x0c
#define ReadCD_SecType_M2F1    0x10
#define ReadCD_SecType_M2F2    0x14

    struct _READ_CD_MSF {
        UCHAR OperationCode;

        UCHAR Reserved0 : 2;
        UCHAR ExpectedSectorType : 3;
        UCHAR Reserved1 : 3;

        UCHAR Reserved2;
        UCHAR Starting_MSF[3];
        UCHAR Ending_MSF[3];

        UCHAR Reserved3 : 1;
        UCHAR ErrorFlags : 2;
        UCHAR IncludeEDC : 1;
        UCHAR IncludeUserData : 1;
        UCHAR HeaderCode : 2;
        UCHAR IncludeSyncData : 1;

        UCHAR SubChannelSelection : 3;
        UCHAR Reserved4 : 5;

        UCHAR Control;
    } READ_CD_MSF, *PREAD_CD_MSF;

    struct _READ_CD {
        UCHAR OperationCode;

        UCHAR Reserved0 : 2;
        UCHAR ExpectedSectorType : 3;
        UCHAR Reserved1 : 3;

        UCHAR LBA[4];
        UCHAR NumOfBlocks[3];

        UCHAR Reserved3 : 1;
        UCHAR ErrorFlags : 2;
        UCHAR IncludeEDC : 1;
        UCHAR IncludeUserData : 1;
        UCHAR HeaderCode : 2;
        UCHAR IncludeSyncData : 1;

        UCHAR SubChannelSelection : 3;
        UCHAR Reserved4 : 5;

        UCHAR Control;
    } READ_CD, *PREAD_CD;

#define WriteCd_RELADR  0x01
#define WriteCd_FUA     0x08
#define WriteCd_DPO     0x10

    struct _WRITE_CD {
        UCHAR OperationCode;
        union {
            UCHAR Flags;
            struct {
                UCHAR RELADR    : 1;
                UCHAR Reserved0 : 2;
                UCHAR FUA       : 1;
                UCHAR DPO       : 1;
                UCHAR Reserved1 : 3;
            } Fields;
        } Byte1;
        UCHAR LBA [4];
        UCHAR Reserved1;
        UCHAR NumOfBlocks [2];
        UCHAR Reserved2 [3];
    } WRITE_CD, *PWRITE_CD;

    // Mode sense
    struct _MODE_SENSE {
        UCHAR OperationCode;
        UCHAR Reserved1 : 3;
        UCHAR Dbd : 1;
        UCHAR Reserved2 : 1;
        UCHAR Lun : 3;
        UCHAR PageCode : 6;
        UCHAR Pc : 2;
        UCHAR Reserved3;
        UCHAR AllocationLength;
        UCHAR Control;
    } MODE_SENSE, *PMODE_SENSE;

    struct _MODE_SENSE10 {
        UCHAR OperationCode;
        UCHAR Reserved1 : 3;
        UCHAR Dbd : 1;
        UCHAR Reserved2 : 1;
        UCHAR Lun : 3;
        UCHAR PageCode : 6;
        UCHAR Pc : 2;
        UCHAR Reserved3[4];
        UCHAR AllocationLength[2];
        UCHAR Control;
    } MODE_SENSE10, *PMODE_SENSE10;

    // Mode select
    struct _MODE_SELECT {
        UCHAR OperationCode;
        UCHAR SPBit : 1;
        UCHAR Reserved1 : 3;
        UCHAR PFBit : 1;
        UCHAR Lun : 3;
        UCHAR Reserved2[2];
        UCHAR ParameterListLength;
        UCHAR Control;
    } MODE_SELECT, *PMODE_SELECT;

    struct _MODE_SELECT10 {
        UCHAR OperationCode;
        UCHAR SPBit : 1;
        UCHAR Reserved1 : 3;
        UCHAR PFBit : 1;
        UCHAR Lun : 3;
        UCHAR Reserved2[5];
        UCHAR ParameterListLength[2];
        UCHAR Control;
    } MODE_SELECT10, *PMODE_SELECT10;

    struct _LOGSENSE {
        UCHAR OperationCode;
        UCHAR SPBit : 1;
        UCHAR PPCBit : 1;
        UCHAR Reserved1 : 3;
        UCHAR Lun : 3;
        UCHAR PageCode : 6;
        UCHAR PCBit : 2;
        UCHAR Reserved2;
        UCHAR Reserved3;
        UCHAR ParameterPointer[2];  // [0]=MSB, [1]=LSB
        UCHAR AllocationLength[2];  // [0]=MSB, [1]=LSB
        UCHAR Control;
    } LOGSENSE, *PLOGSENSE;

    struct _SEEK {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR Lun : 3;
        UCHAR LBA[4];
        UCHAR Reserved2[3];
        UCHAR Control;
    } SEEK, *PSEEK;

#define StartStop_Start     0x01
#define StartStop_Load      0x02

    struct _START_STOP {
        UCHAR OperationCode;
        UCHAR Immediate: 1;
        UCHAR Reserved1 : 4;
        UCHAR Lun : 3;
        UCHAR Reserved2[2];
        UCHAR Start : 1;
        UCHAR LoadEject : 1;
        UCHAR Reserved3 : 6;
        UCHAR Control;
    } START_STOP, *PSTART_STOP;

    struct _MEDIA_REMOVAL {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR Lun : 3;
        UCHAR Reserved2[2];
        UCHAR Prevent;
        UCHAR Control;
    } MEDIA_REMOVAL, *PMEDIA_REMOVAL;

    struct _READ_FORMAT_CAPACITIES {
        UCHAR OperationCode;
        union {
            UCHAR Flags;
            struct {
                UCHAR Reserved0 : 5;
                UCHAR Reserved1 : 3;
            } Fields;
        } Byte1;
        UCHAR Reserved0[5];
        UCHAR AllocationLength[2];
        UCHAR Control;
    } READ_FORMAT_CAPACITIES, *PREAD_FORMAT_CAPACITIES;

    // Atapi 2.5 Changer 12-byte CDBs
#define LoadUnload_Start    0x01
#define LoadUnload_Load     0x02

    struct _LOAD_UNLOAD {
        UCHAR OperationCode;
        UCHAR Immediate : 1;
        UCHAR Reserved1 : 7;
        UCHAR Reserved2[2];
        UCHAR Start : 1;
        UCHAR LoadEject : 1;
        UCHAR Reserved3: 6;
        UCHAR Reserved4[3];
        UCHAR Slot;
        UCHAR Reserved5[3];
    } LOAD_UNLOAD, *PLOAD_UNLOAD;

    struct _MECH_STATUS {
        UCHAR OperationCode;
        UCHAR Reserved0[7];
        UCHAR AllocationLength[2];
        UCHAR Reserved2[2];
    } MECH_STATUS, *PMECH_STATUS;

    struct _LOCK_DOOR {
        UCHAR OperationCode;
        UCHAR Reserved0[9];
    } LOCK_DOOR, *PLOCK_DOOR;

#define EventStat_Immed             0x01

#define EventStat_Class_OpChange    0x02
#define EventStat_Class_PM          0x04
#define EventStat_Class_ExternalReq 0x08
#define EventStat_Class_Media       0x10
#define EventStat_Class_MultiInit   0x20
#define EventStat_Class_DevBusy     0x40

    struct _GET_EVENT_STATUS {
        UCHAR OperationCode;
        union {
            UCHAR Flags;
            struct {
                UCHAR Immed     : 1;
                UCHAR Reserved0 : 7;
            } Fields;
        } Byte1;
        UCHAR Reserved0[2];
        UCHAR NotificationClass;
        UCHAR Reserved1[2];
        UCHAR AllocationLength[2];
        UCHAR Control;
    } GET_EVENT_STATUS, *PGET_EVENT_STATUS;

    struct _READ_DISC_INFO {
        UCHAR OperationCode;
        UCHAR Reserved0[6];
        UCHAR AllocationLength[2];
        UCHAR Reserved2[3];
    } READ_DISC_INFO, *PREAD_DISC_INFO;

#define ReadTrackInfo_Type_Mask    0x01
#define ReadTrackInfo_Type_LBA     0x00
#define ReadTrackInfo_Type_Track   0x01

#define ReadTrackInfo_LastTrk      0xff

    struct _READ_TRACK_INFO {
        UCHAR OperationCode;
        UCHAR Track     : 1;
        UCHAR Reserved0 : 7;
        UCHAR LBA_TrkNum [4];
        UCHAR Reserved1;
        UCHAR AllocationLength[2];
        UCHAR Reserved2[3];
    } READ_TRACK_INFO, *PREAD_TRACK_INFO;

#define ReadTrackInfo3_Type_Mask    0x03
#define ReadTrackInfo3_Type_LBA     ReadTrackInfo_Type_LBA
#define ReadTrackInfo3_Type_Track   ReadTrackInfo_Type_Track
#define ReadTrackInfo3_Type_Ses     0x02

#define ReadTrackInfo3_LastTrk      ReadTrackInfo_LastTrk
#define ReadTrackInfo3_DiscLeadIn   0x00  // for Track type

    struct _READ_TRACK_INFO_3 {
        UCHAR OperationCode;
        UCHAR DataType     : 2;
        UCHAR Reserved0 : 6;
        UCHAR LBA_TrkNum [4];
        UCHAR Reserved1;
        UCHAR AllocationLength[2];
        UCHAR Reserved2[3];
    } READ_TRACK_INFO_3, *PREAD_TRACK_INFO_3;

    struct _RESERVE_TRACK {
        UCHAR OperationCode;
        UCHAR Reserved0[4];
        UCHAR Size[4];
        UCHAR Reserved2[3];
    } RESERVE_TRACK, *PRESERVE_TRACK;

#define CloseTrkSes_Immed   0x01

#define CloseTrkSes_Trk   0x01
#define CloseTrkSes_Ses   0x02

#define CloseTrkSes_LastTrkSes  0xff

#define CloseTrkSes_Delay   DEF_I64(3100000000)        //  310 s

    struct _CLOSE_TRACK_SESSION {
        UCHAR OperationCode;
        union {
            UCHAR Flags;
            struct {
                UCHAR Immed     : 1;
                UCHAR Reserved0 : 7;
            } Fields;
        } Byte1;
        union {
            UCHAR Flags;
            struct {
                UCHAR Track     : 1;
                UCHAR Session   : 1;
                UCHAR Reserved0 : 6;
            } Fields;
        } Byte2;
        UCHAR Reserved1 [2];
        UCHAR TrackNum;
        UCHAR Reserved2 [6];

    } CLOSE_TRACK_SESSION, *PCLOSE_TRACK_SESSION;

    struct _SET_CD_SPEED {
        UCHAR OperationCode;
        UCHAR Reserved0;
        UCHAR ReadSpeed [2];        // Kbyte/sec
        UCHAR WriteSpeed [2];        // Kbyte/sec
        UCHAR Reserved1[6];
    } SET_CD_SPEED, *PSET_CD_SPEED;

#define SyncCache_RELADR        0x01
#define SyncCache_Immed         0x02

    struct _SYNCHRONIZE_CACHE {
        UCHAR OperationCode;
        union {
            UCHAR Flags;
            struct {
                UCHAR RELADR    : 1;
                UCHAR Immed     : 1;
                UCHAR Reserved0 : 6;            // All these are unused by drive
            } Fields;
        } Byte1;
        UCHAR LBA [4];
        UCHAR Reserved1;
        UCHAR NumOfBlocks [2];
        UCHAR Reserved2 [3];
/*
        UCHAR Unused [11];*/
    } SYNCHRONIZE_CACHE, *PSYNCHRONIZE_CACHE;

#define BlankMedia_Mask             0x07
#define BlankMedia_Complete         0x00
#define BlankMedia_Minimal          0x01
#define BlankMedia_Track            0x02
#define BlankMedia_UnreserveTrack   0x03
#define BlankMedia_TrackTail        0x04
#define BlankMedia_UncloseLastSes   0x05
#define BlankMedia_EraseSes         0x06
#define BlankMedia_Immed            0x10

    struct _BLANK_MEDIA {
        UCHAR OperationCode;
        union {
            UCHAR Flags;
            struct {
                UCHAR BlankType : 3;
                UCHAR Reserved0 : 1;
                UCHAR Immed     : 1;
                UCHAR Reserved1 : 3;
            } Fields;
        } Byte1;
        UCHAR StartAddr_TrkNum [4];
        UCHAR Reserved2 [6];
    } BLANK_MEDIA, *PBLANK_MEDIA;

#define SendKey_ReportAGID            0x00
#define SendKey_ChallengeKey          0x01
#define SendKey_Key1                  0x02
#define SendKey_Key2                  0x03
#define SendKey_TitleKey              0x04
#define SendKey_ReportASF             0x05
#define SendKey_InvalidateAGID        0x3F

    struct _SEND_KEY {
        UCHAR OperationCode;
        UCHAR Reserved1 : 5;
        UCHAR Lun : 3;
        UCHAR Reserved2[6];
        UCHAR ParameterListLength[2];
        UCHAR KeyFormat : 6;
        UCHAR AGID : 2;
        UCHAR Control;
    } SEND_KEY, *PSEND_KEY;

    struct _REPORT_KEY {
        UCHAR OperationCode;    // 0xA4
        UCHAR Reserved1 : 5;
        UCHAR Lun : 3;
        UCHAR LBA[4];           // for title key
        UCHAR Reserved2[2];
        UCHAR AllocationLength[2];
        UCHAR KeyFormat : 6;
        UCHAR AGID : 2;
        UCHAR Control;
    } REPORT_KEY, *PREPORT_KEY;

    struct _READ_DVD_STRUCTURE {
        UCHAR OperationCode;    // 0xAD
        UCHAR Reserved1 : 5;
        UCHAR Lun : 3;
        UCHAR RMDBlockNumber[4];
        UCHAR LayerNumber;
        UCHAR Format;
        UCHAR AllocationLength[2];  // [0]=MSB, [1]=LSB
        UCHAR Reserved3 : 6;
        UCHAR AGID : 2;
        UCHAR Control;
    } READ_DVD_STRUCTURE, *PREAD_DVD_STRUCTURE;

    struct _READ_BUFFER_CAPACITY {
        UCHAR OperationCode;
        UCHAR Reserved0 [6];
        UCHAR AllocationLength[2];
        UCHAR Reserved1 [3];
    } READ_BUFFER_CAPACITY, *PREAD_BUFFER_CAPACITY;

    struct _GET_CONFIGURATION {
        UCHAR OperationCode;
        union {
            UCHAR Flags;
            struct {
                UCHAR RT        : 2;
                UCHAR Reserved0 : 6;
            } Fields;
        } Byte1;
        UCHAR StartFeatureNum [2];
        UCHAR Reserved0 [3];
        UCHAR AllocationLength[2];
        UCHAR Control;
    } GET_CONFIGURATION, *PGET_CONFIGURATION;

    struct _SET_READ_AHEAD {
        UCHAR OperationCode;
        UCHAR Reserved0;
        UCHAR TriggerLBA[4];
        UCHAR ReadAheadLBA[4];
        UCHAR Reserved1;
        UCHAR Control;
    } SET_READ_AHEAD, *PSET_READ_AHEAD;

#define SendOpc_DoOpc   0x01

    struct _SEND_OPC_INFO {
        UCHAR OperationCode;
        union {
            UCHAR Flags;
            struct {
                UCHAR DoOpc     : 1;
                UCHAR Reserved0 : 4;
                UCHAR Reserved1 : 3;
            } Fields;
        } Byte1;
        UCHAR Reserved0 [5];
        UCHAR AllocationLength[2];
        UCHAR Control;
    } SEND_OPC_INFO, *PSEND_OPC_INFO;

    struct _SEND_CUE_SHEET {
        UCHAR OperationCode;
        UCHAR Reserved0 [5];
        UCHAR AllocationLength[3];
        UCHAR Control;
    } SEND_CUE_SHEET, *PSEND_CUE_SHEET;

    struct _CDB12 {
        UCHAR OperationCode;
        UCHAR Params[11];
    } CDB12, *PCDB12;

    struct _CDB12READWRITE {
        UCHAR OperationCode;
        union {
            UCHAR Flags;
            struct {
                UCHAR RELADR    : 1;
                UCHAR Reserved0 : 2;
                UCHAR FUA       : 1;
                UCHAR DPO       : 1;
                UCHAR Reserved1 : 3;
            } Fields;
        } Byte1;
        UCHAR LBA [4];
        UCHAR NumOfBlocks [4];
        UCHAR Reserved1[2];
    } CDB12READWRITE, *PCDB12READWRITE;

    // Plextor Read CD-DA
    struct _PLXTR_READ_CDDA {
        UCHAR OperationCode;
        UCHAR Reserved0 : 5;
        UCHAR Lun :3;
        UCHAR LBA[4];
        UCHAR TransferBlock[4];
        UCHAR SubCode;
        UCHAR Control;
    } PLXTR_READ_CDDA, *PPLXTR_READ_CDDA;

    // NEC Read CD-DA
    struct _NEC_READ_CDDA {
        UCHAR OperationCode;
        UCHAR Reserved0;
        UCHAR LBA[4];
        UCHAR Reserved1;
        UCHAR TransferBlock[2];
        UCHAR Control;
    } NEC_READ_CDDA, *PNEC_READ_CDDA;

} CDB, *PCDB;

// Command Descriptor Block constants.

#define CDB6GENERIC_LENGTH                  6
#define CDB10GENERIC_LENGTH                 10
#define CDB12GENERIC_LENGTH                 12

#define MAXIMUM_NUMBER_OF_TRACKS            100
#define MAXIMUM_NUMBER_OF_SESSIONS          1024    //maximal number of entries in Read Full TOC

#define SETBITON                            1
#define SETBITOFF                           0

// Mode Sense/Select page constants.

#define MODE_PAGE_ERROR_RECOVERY        0x01
#define MODE_PAGE_WRITE_PARAMS          0x05
#define MODE_PAGE_VERIFY_ERROR          0x07        // shall not be used
#define MODE_PAGE_CACHING               0x08        // undocumented, but used by DirectCd
#define MODE_PAGE_MEDIUM_TYPES          0x0B        // shall not be used
#define MODE_PAGE_CD_DEVICE_PARAMS      0x0D
#define MODE_PAGE_CD_AUDIO_CONTROL      0x0E
#define MODE_PAGE_POWER_CONDITION       0x1A
#define MODE_PAGE_FAIL_REPORT           0x1C
#define MODE_PAGE_TIMEOUT_AND_PROTECT   0x1D
#define MODE_PAGE_PHILIPS_SECTOR_TYPE   0x21
#define MODE_PAGE_CAPABILITIES          0x2A

#define MODE_SENSE_RETURN_ALL           0x3f

#define MODE_SENSE_CURRENT_VALUES       0x00
#define MODE_SENSE_CHANGEABLE_VALUES    0x40
#define MODE_SENSE_DEFAULT_VAULES       0x80
#define MODE_SENSE_SAVED_VALUES         0xc0

// SCSI CDB operation codes

#define SCSIOP_TEST_UNIT_READY      0x00
#define SCSIOP_REZERO_UNIT          0x01
#define SCSIOP_REWIND               0x01
#define SCSIOP_REQUEST_BLOCK_ADDR   0x02
#define SCSIOP_REQUEST_SENSE        0x03
#define SCSIOP_FORMAT_UNIT          0x04
#define SCSIOP_READ_BLOCK_LIMITS    0x05
#define SCSIOP_REASSIGN_BLOCKS      0x07
#define SCSIOP_READ6                0x08
#define SCSIOP_RECEIVE              0x08
#define SCSIOP_WRITE6               0x0A
#define SCSIOP_PRINT                0x0A
#define SCSIOP_SEND                 0x0A
#define SCSIOP_SEEK6                0x0B
#define SCSIOP_TRACK_SELECT         0x0B
#define SCSIOP_SLEW_PRINT           0x0B
#define SCSIOP_SEEK_BLOCK           0x0C
#define SCSIOP_PARTITION            0x0D
#define SCSIOP_READ_REVERSE         0x0F
#define SCSIOP_WRITE_FILEMARKS      0x10
#define SCSIOP_FLUSH_BUFFER         0x10
#define SCSIOP_SPACE                0x11
#define SCSIOP_INQUIRY              0x12
#define SCSIOP_VERIFY6              0x13
#define SCSIOP_RECOVER_BUF_DATA     0x14
#define SCSIOP_MODE_SELECT          0x15
#define SCSIOP_RESERVE_UNIT         0x16
#define SCSIOP_RELEASE_UNIT         0x17
#define SCSIOP_COPY                 0x18
#define SCSIOP_ERASE                0x19
#define SCSIOP_MODE_SENSE           0x1A
#define SCSIOP_START_STOP_UNIT      0x1B
#define SCSIOP_STOP_PRINT           0x1B
#define SCSIOP_LOAD_UNLOAD          0x1B
#define SCSIOP_RECEIVE_DIAGNOSTIC   0x1C
#define SCSIOP_SEND_DIAGNOSTIC      0x1D
#define SCSIOP_MEDIUM_REMOVAL       0x1E
#define SCSIOP_READ_FORMAT_CAPACITY 0x23
#define SCSIOP_READ_CAPACITY        0x25
#define SCSIOP_READ                 0x28
#define SCSIOP_WRITE                0x2A
#define SCSIOP_WRITE_CD             0x2A
#define SCSIOP_SEEK                 0x2B
#define SCSIOP_LOCATE               0x2B
#define SCSIOP_ERASE10              0x2C
#define SCSIOP_WRITE_VERIFY         0x2E
#define SCSIOP_VERIFY               0x2F
#define SCSIOP_SEARCH_DATA_HIGH     0x30
#define SCSIOP_SEARCH_DATA_EQUAL    0x31
#define SCSIOP_SEARCH_DATA_LOW      0x32
#define SCSIOP_SET_LIMITS           0x33
#define SCSIOP_READ_POSITION        0x34
#define SCSIOP_SYNCHRONIZE_CACHE    0x35
#define SCSIOP_COMPARE              0x39
#define SCSIOP_COPY_COMPARE         0x3A
#define SCSIOP_COPY_VERIFY          0x3A
#define SCSIOP_WRITE_DATA_BUFF      0x3B
#define SCSIOP_READ_DATA_BUFF       0x3C
#define SCSIOP_CHANGE_DEFINITION    0x40
#define SCSIOP_PLAY_AUDIO10         0x41
#define SCSIOP_READ_SUB_CHANNEL     0x42
#define SCSIOP_READ_TOC             0x43
#define SCSIOP_READ_HEADER          0x44
#define SCSIOP_PLAY_AUDIO           0x45
#define SCSIOP_GET_CONFIGURATION    0x46
#define SCSIOP_PLAY_AUDIO_MSF       0x47
#define SCSIOP_PLAY_TRACK_INDEX     0x48
#define SCSIOP_PLAY_TRACK_RELATIVE  0x49
#define SCSIOP_GET_EVENT_STATUS     0x4A
#define SCSIOP_PAUSE_RESUME         0x4B
#define SCSIOP_LOG_SELECT           0x4C
#define SCSIOP_LOG_SENSE            0x4D
#define SCSIOP_STOP_PLAY_SCAN       0x4E
#define SCSIOP_READ_DISC_INFO       0x51
#define SCSIOP_READ_TRACK_INFO      0x52
#define SCSIOP_RESERVE_TRACK        0x53
#define SCSIOP_SEND_OPC_INFO        0x54
#define SCSIOP_MODE_SELECT10        0x55
#define SCSIOP_REPAIR_TRACK         0x58    // obsolete
#define SCSIOP_READ_MASTER_CUE      0x59
#define SCSIOP_MODE_SENSE10         0x5A
#define SCSIOP_CLOSE_TRACK_SESSION  0x5B
#define SCSIOP_READ_BUFFER_CAPACITY 0x5C
#define SCSIOP_SEND_CUE_SHEET       0x5D
#define SCSIOP_BLANK                0xA1
#define SCSIOP_SEND_KEY             0xA3
#define SCSIOP_REPORT_KEY           0xA4
#define SCSIOP_PLAY_AUDIO12         0xA5
#define SCSIOP_LOAD_UNLOAD_SLOT     0xA6
#define SCSIOP_SET_READ_AHEAD       0xA7
#define SCSIOP_READ12               0xA8
#define SCSIOP_WRITE12              0xAA
#define SCSIOP_SEEK12               0xAB
#define SCSIOP_GET_PERFORMANCE      0xAC
#define SCSIOP_READ_DVD_STRUCTURE   0xAD
#define SCSIOP_WRITE_VERIFY12       0xAE
#define SCSIOP_VERIFY12             0xAF
#define SCSIOP_SET_STREAMING        0xB6
#define SCSIOP_READ_CD_MSF          0xB9
#define SCSIOP_SET_CD_SPEED         0xBB
#define SCSIOP_MECHANISM_STATUS     0xBD
#define SCSIOP_READ_CD              0xBE
#define SCSIOP_SEND_DVD_STRUCTURE   0xBF
#define SCSIOP_DOORLOCK             0xDE    // lock door on removable drives
#define SCSIOP_DOORUNLOCK           0xDF    // unlock door on removable drives

// If the IMMED bit is 1, status is returned as soon
// as the operation is initiated. If the IMMED bit
// is 0, status is not returned until the operation
// is completed.

#define CDB_RETURN_ON_COMPLETION   0
#define CDB_RETURN_IMMEDIATE       1

// end_ntminitape

// CDB Force media access used in extended read and write commands.

#define CDB_FORCE_MEDIA_ACCESS 0x08

// Denon CD ROM operation codes

#define SCSIOP_DENON_EJECT_DISC    0xE6
#define SCSIOP_DENON_STOP_AUDIO    0xE7
#define SCSIOP_DENON_PLAY_AUDIO    0xE8
#define SCSIOP_DENON_READ_TOC      0xE9
#define SCSIOP_DENON_READ_SUBCODE  0xEB

// Philips/Matshushita CD-R(W) operation codes

#define SCSIOP_PHILIPS_GET_NWA                0xE2
#define SCSIOP_PHILIPS_RESERVE_TRACK          0xE4
#define SCSIOP_PHILIPS_WRITE_TRACK            0xE6
#define SCSIOP_PHILIPS_LOAD_UNLOAD            0xE7
#define SCSIOP_PHILIPS_CLOSE_TRACK_SESSION    0xE9
#define SCSIOP_PHILIPS_RECOVER_BUF_DATA       0xEC
#define SCSIOP_PHILIPS_READ_SESSION_INFO      0xEE

// Plextor operation codes

#define SCSIOP_PLEXTOR_READ_CDDA   0xD8

// NEC operation codes

#define SCSIOP_NEC_READ_CDDA       0xD4

// SCSI Bus Messages

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

// SCSI Extended Message operation codes

#define SCSIMESS_MODIFY_DATA_POINTER  0X00
#define SCSIMESS_SYNCHRONOUS_DATA_REQ 0X01
#define SCSIMESS_WIDE_DATA_REQUEST    0X03

// SCSI Extended Message Lengths

#define SCSIMESS_MODIFY_DATA_LENGTH   5
#define SCSIMESS_SYNCH_DATA_LENGTH    3
#define SCSIMESS_WIDE_DATA_LENGTH     2

// SCSI extended message structure

typedef struct _SCSI_EXTENDED_MESSAGE {
    UCHAR InitialMessageCode;
    UCHAR MessageLength;
    UCHAR MessageType;
    union _EXTENDED_ARGUMENTS {

        struct {
            UCHAR Modifier[4];
        } Modify;

        struct {
            UCHAR TransferPeriod;
            UCHAR ReqAckOffset;
        } Synchronous;

        struct{
            UCHAR Width;
        } Wide;
    }ExtendedArguments;
}SCSI_EXTENDED_MESSAGE, *PSCSI_EXTENDED_MESSAGE;

// SCSI bus status codes.

#define SCSISTAT_GOOD                  0x00
#define SCSISTAT_CHECK_CONDITION       0x02
#define SCSISTAT_CONDITION_MET         0x04
#define SCSISTAT_BUSY                  0x08
#define SCSISTAT_INTERMEDIATE          0x10
#define SCSISTAT_INTERMEDIATE_COND_MET 0x14
#define SCSISTAT_RESERVATION_CONFLICT  0x18
#define SCSISTAT_COMMAND_TERMINATED    0x22
#define SCSISTAT_QUEUE_FULL            0x28

// Enable Vital Product Data Flag (EVPD)
// used with INQUIRY command.

#define CDB_INQUIRY_EVPD           0x01

// retry time (in deci-seconds)
#define NOT_READY_RETRY_INTERVAL    20

// Defines for format CDB
#define LUN0_FORMAT_SAVING_DEFECT_LIST 0
#define USE_DEFAULTMSB  0
#define USE_DEFAULTLSB  0

#define START_UNIT_CODE 0x01
#define STOP_UNIT_CODE  0x00

// Inquiry buffer structure. This is the data returned from the target
// after it receives an inquiry.
//
// This structure may be extended by the number of bytes specified
// in the field AdditionalLength. The defined size constant only
// includes fields through ProductRevisionLevel.
//
// The NT SCSI drivers are only interested in the first 36 bytes of data.

#define INQUIRYDATABUFFERSIZE 36

typedef struct _INQUIRYDATA {
    UCHAR DeviceType : 5;
    UCHAR DeviceTypeQualifier : 3;
    UCHAR DeviceTypeModifier : 7;
    UCHAR RemovableMedia : 1;
    UCHAR Versions;
    UCHAR ResponseDataFormat;
    UCHAR AdditionalLength;
    UCHAR Reserved[2];
    UCHAR SoftReset : 1;
    UCHAR CommandQueue : 1;
    UCHAR Reserved2 : 1;
    UCHAR LinkedCommands : 1;
    UCHAR Synchronous : 1;
    UCHAR Wide16Bit : 1;
    UCHAR Wide32Bit : 1;
    UCHAR RelativeAddressing : 1;
    UCHAR VendorId[8];
    UCHAR ProductId[16];
    UCHAR ProductRevisionLevel[4];
    UCHAR VendorSpecific[20];
    UCHAR Reserved3[40];
} INQUIRYDATA, *PINQUIRYDATA;

// Inquiry defines. Used to interpret data returned from target as result
// of inquiry command.

// DeviceType field

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

// DeviceTypeQualifier field

#define DEVICE_CONNECTED 0x00

// Sense Data Format

typedef struct _SENSE_DATA {
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

// Default request sense buffer size

#define SENSE_BUFFER_SIZE 18

// Sense keys

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

// Additional Sense codes

// SK = 0x00
#define SCSI_ADSENSE_NO_SENSE       0x00

// SK = 0x01
#define SCSI_ADSENSE_WARNING        0x0B
#define SCSI_ADSENSE_REC_DATA_NOECC 0x17
#define SCSI_ADSENSE_REC_DATA_ECC   0x18
#define SCSI_ADSENSE_ROUNDED_PARAM  0x37
#define SCSI_ADSENSE_FAILURE_PREDICTED 0x5D
#define SCSI_ADSENSE_CD_CONTROL_ERR 0x73

// SK = 0x02
#define SCSI_ADSENSE_LUN_NOT_READY  0x04
#define SCSI_ADSENSE_INCOMPATIBLE_MEDIA 0x30
#define SCSI_ADSENSE_INVALID_MEDIA  SCSI_ADSENSE_INCOMPATIBLE_MEDIA // for w2k
#define SCSI_ADSENSE_NO_MEDIA_IN_DEVICE 0x3A
#define SCSI_ADSENSE_POSITION_ERROR 0x3B
#define SCSI_ADSENSE_NOT_SELF_CONFIGURED 0x3E


// SK = 0x03
#define SCSI_ADSENSE_NO_SEEK        0x02
#define SCSI_ADSENSE_NO_REFERENCE   0x06
#define SCSI_ADSENSE_CD_WRITE_ERROR 0x0C
#define SCSI_ADSENSE_CD_READ_ERROR  0x11
#define SCSI_ADSENSE_TRACK_ERROR    0x14
#define SCSI_ADSENSE_SEEK_ERROR     0x15
#define SCSI_ADSENSE_FORMAT_CORRUPTED 0x31
#define SCSI_ADSENSE_ENCLOSURE_FAILURE 0x34
#define SCSI_ADSENSE_ENCLOSURE_SERVICE 0x35
#define SCSI_ADSENSE_ERASE_ERROR    0x51
#define SCSI_ADSENSE_UNRECOVERED_TOC 0x57
#define SCSI_ADSENSE_SESSION_FIXATION 0x71
//#define SCSI_ADSENSE_CD_CONTROL_ERR 0x73  // redefinition

// SK = 0x04
#define SCSI_ADSENSE_CLEAN_REQUEST  0x00
#define SCSI_ADSENSE_SELECT         0x04
#define SCSI_ADSENSE_COMMUNICATION  0x08
#define SCSI_ADSENSE_LOST_STREAMING 0x09
#define SCSI_ADSENSE_SYNC_ERROR     0x1B
#define SCSI_ADSENSE_MECH_ERROR     0x3B
#define SCSI_ADSENSE_LUN_ERROR      0x3E
#define SCSI_ADSENSE_DIAGNOSTIC     0x40
#define SCSI_ADSENSE_INTERNAL       0x44
#define SCSI_ADSENSE_SOFT_RESET     0x46
#define SCSI_ADSENSE_SCSI_PARITY    0x47
#define SCSI_ADSENSE_CMD_PHASE      0x4A
#define SCSI_ADSENSE_DATA_PHASE     0x4B
#define SCSI_ADSENSE_SELF_CONFIG    0x4C
#define SCSI_ADSENSE_MEDIUM_REMOVAL 0x53
#define SCSI_ADSENSE_VOLTAGE        0x65

// SK = 0x05
#define SCSI_ADSENSE_AUDIO_PLAY     0x00
#define SCSI_ADSENSE_MULTISELECT    0x07
#define SCSI_ADSENSE_INVALID_PARAM_LENGTH 0x1A
#define SCSI_ADSENSE_ILLEGAL_COMMAND 0x20
#define SCSI_ADSENSE_ILLEGAL_BLOCK  0x21
#define SCSI_ADSENSE_INVALID_CDB    0x24
#define SCSI_ADSENSE_INVALID_LUN    0x25
#define SCSI_ADSENSE_INVALID_VALUE  0x26
#define SCSI_ADSENSE_WRITE_PROTECT  0x27
#define SCSI_ADSENSE_CANT_DISCONNECT 0x2B
#define SCSI_ADSENSE_INVALID_CMD_SEQUENCE 0x2C
#define SCSI_ADSENSE_INVALID_SESSION_MODE 0x30
#define SCSI_ADSENSE_SAVE_NOT_SUPPORTED 0x35
#define SCSI_ADSENSE_INVALID_BITS_IN_IDENT_MSG 0x3D
#define SCSI_ADSENSE_MSG_ERROR      0x43
//#define SCSI_ADSENSE_MEDIUM_REMOVAL 0x53  // redefinition
#define SCSI_ADSENSE_SYS_RESOURCE_FAILURE 0x55
#define SCSI_ADSENSE_OUT_OF_SPACE   0x63
#define SCSI_ADSENSE_ILLEGAL_MODE_FOR_THIS_TRACK 0x64
#define SCSI_ADSENSE_CD_COPY_ERROR  0x6F
#define SCSI_ADSENSE_INCOMPLETE_DATA 0x72
#define SCSI_ADSENSE_VENDOR_UNIQUE  0x80
#define SCSI_ADSENSE_MUSIC_AREA     0xA0
#define SCSI_ADSENSE_DATA_AREA      0xA1
#define SCSI_ADSENSE_VOLUME_OVERFLOW 0xA7

// SK = 0x06
#define SCSI_ADSENSE_LOG_OVERFLOW   0x0A
#define SCSI_ADSENSE_MEDIUM_CHANGED 0x28
#define SCSI_ADSENSE_BUS_RESET      0x29
#define SCSI_ADSENSE_PARAM_CHANGE   0x2A
#define SCSI_ADSENSE_CMD_CLEARED_BY_ANOTHER 0x2F
#define SCSI_ADSENSE_MEDIA_STATE    0x3B
#define SCSI_ADSENSE_FUNCTIONALTY_CHANGE 0x3F
#define SCSI_ADSENSE_OPERATOR       0x5A
#define SCSI_ADSENSE_MAX_LOG        0x5B
#define SCSI_ADSENSE_POWER          0x5E

// SK = 0x0B
#define SCSI_ADSENSE_READ_LOST_STREAMING 0x11
#define SCSI_ADSENSE_RESELECT_FAILURE 0x45
#define SCSI_ADSENSE_ERR_MSG_DETECTED 0x48
#define SCSI_ADSENSE_INVALID_ERR_MSG 0x49
#define SCSI_ADSENSE_TEGGED_OVERLAPPED 0x4D
#define SCSI_ADSENSE_OVERLAPPED_ATTEMPT 0x4E

// Additional sense code qualifier

#define SCSI_SENSEQ_NO_SENSE 0x00

// SK:ASC = 02:04
//#define SCSI_SENSEQ_NO_SENSE 0x00
#define SCSI_SENSEQ_CAUSE_NOT_REPORTABLE    0x00
#define SCSI_SENSEQ_BECOMING_READY          0x01
#define SCSI_SENSEQ_INIT_COMMAND_REQUIRED   0x02
#define SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED 0x03
#define SCSI_SENSEQ_FORMAT_IN_PROGRESS      0x04
#define SCSI_SENSEQ_OPERATION_IN_PROGRESS   0x07
#define SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS  0x08

// SK:ASC = 02:30
#define SCSI_SENSEQ_INCOMPATIBLE_MEDIA_INSTALLED 0x00
#define SCSI_SENSEQ_UNKNOWN_FORMAT      0x01
#define SCSI_SENSEQ_INCOMPATIBLE_FORMAT 0x02
#define SCSI_SENSEQ_CLEANING_CARTRIDGE_INSTALLED 0x03
#define SCSI_SENSEQ_WRITE_UNKNOWN_FORMAT 0x04
#define SCSI_SENSEQ_WRITE_INCOMPATIBLE_FORMAT 0x05
#define SCSI_SENSEQ_FORMAT_INCOMPATIBLE_MEDIUM 0x06
#define SCSI_SENSEQ_CLEANING_FAILURE    0x07

// SK:ASC = 02:3A
#define SCSI_SENSEQ_TRAY_CLOSED         0x01
#define SCSI_SENSEQ_TRAY_OPEN           0x02

// SK:ASC = 03:0C
#define SENSEQ_W_RECOVERY_NEEDED        0x07
#define SENSEQ_W_RECOVERY_FAILED        0x08
#define SENSEQ_LOST_STREAMING           0x09
#define SENSEQ_PADDING_BLOCKS_ADDED     0x0A

// SK:ASC = 03:72
//#define SCSI_SENSEQ_NO_SENSE 0x00
#define SCSI_SENSEQ_LEAD_IN_ERROR       0x01
#define SCSI_SENSEQ_LEAD_OUT_ERRROR     0x02
#define SCSI_SENSEQ_INCOMPLETE_TRACK    0x03
#define SCSI_SENSEQ_INCOMPLETE_RESERVED_TRACK 0x04
#define SCSI_SENSEQ_NO_MORE_RESERVATION 0x05

// SK:ASC = 05:26
#define SCSI_SENSEQ_PARAM_NOT_SUPPORTED 0x01
#define SCSI_SENSEQ_PARAM_INVALID_VALUE 0x02
#define SCSI_SENSEQ_THRESHOLD_PARAM_NOT_SUPPORTED 0x03
#define SCSI_SENSEQ_INVALID_RELEASE_OF_PERSISTENT_RESERVATION 0x04

// SK:ASC = 05:27
#define SCSI_SENSEQ_HW_PROTECTION       0x01
#define SCSI_SENSEQ_LUN_SOFT_PROTECTION 0x02
#define SCSI_SENSEQ_ASSOCIATED_PROTECTION 0x03
#define SCSI_SENSEQ_PERSIST_PROTECTION  0x04
#define SCSI_SENSEQ_PERMANENT_PROTECTION 0x05

// SK:ASC = 05:2C
#define SCSI_SENSEQ_PROGRAMM_AREA_NOT_EMPTY 0x03
#define SCSI_SENSEQ_PROGRAMM_AREA_EMPTY 0x04

// SK:ASC = 05:30
#define SCSI_SENSEQ_APP_CODE_MISSMATCH  0x08
#define SCSI_SENSEQ_NOT_FIXED_FOR_APPEND 0x09

// SK:ASC = 05:6F
#define SCSI_SENSEQ_AUTHENTICATION_FAILURE 0x00
#define SCSI_SENSEQ_KEY_NOT_PRESENT        0x01
#define SCSI_SENSEQ_KEY_NOT_ESTABLISHED    0x02
#define SCSI_SENSEQ_READ_OF_SCRAMBLED_SECTOR_WITHOUT_AUTHENTICATION 0x03
#define SCSI_SENSEQ_MEDIA_CODE_MISMATCHED_TO_LOGICAL_UNIT 0x04
#define SCSI_SENSEQ_LOGICAL_UNIT_RESET_COUNT_ERROR 0x05

// SK:ASC = 06:28
#define SCSI_SENSEQ_IMPORT_OR_EXPERT_ELEMENT_ACCESS   0x01

// SK:ASC = 06:29
#define SCSI_SENSEQ_POWER_ON            0x01
#define SCSI_SENSEQ_SCSI_BUS            0x02
#define SCSI_SENSEQ_BUS_DEVICE_FUNCTION 0x03
#define SCSI_SENSEQ_DEVICE_INTERNAL     0x04

// SK:ASC = 06:2A
#define SCSI_SENSEQ_MODE_PARAMETERS     0x01
#define SCSI_SENSEQ_LOG_PARAMETERS      0x02
#define SCSI_SENSEQ_RESERVATIONS_PREEMPTED 0x03

// SK:ASC = 06:3B
#define SCSI_SENSEQ_DESTINATION_ELEMENT_FULL 0x0D
#define SCSI_SENSEQ_SOURCE_ELEMENT_EMPTY 0x0E
#define SCSI_SENSEQ_END_OF_MEDIUM       0x0F
#define SCSI_SENSEQ_MAGAZINE_NOT_ACCESSIBLE 0x11
#define SCSI_SENSEQ_MAGAZINE_REMOVED    0x12
#define SCSI_SENSEQ_MAGAZINE_INSERTED   0x13
#define SCSI_SENSEQ_MAGAZINE_LOCKED     0x14
#define SCSI_SENSEQ_MAGAZINE_UNLOCKED   0x15

// SK:ASC = 06:3F
#define SCSI_SENSEQ_MICROCODE           0x01
#define SCSI_SENSEQ_OPERATION_DEFINITION 0x02
#define SCSI_SENSEQ_INQUIRY_DATA        0x03

// SK:ASC = 06:5A
#define SCSI_SENSEQ_MEDIUM_CHANGE_REQ   0x01
#define SCSI_SENSEQ_W_PROTECT_SELECTED  0x02
#define SCSI_SENSEQ_W_PROTECT_PERMITED  0x03

// SK:ASC = 06:5E
#define SCSI_SENSEQ_LOW_POWER_COND      0x00
#define SCSI_SENSEQ_IDLE_BY_TIMER       0x01
#define SCSI_SENSEQ_STANDBY_BY_TIMER    0x02
#define SCSI_SENSEQ_IDLE_BY_CMD         0x03
#define SCSI_SENSEQ_STANDBY_BY_CMD      0x04

#define SCSI_SENSEQ_FILEMARK_DETECTED 0x01
#define SCSI_SENSEQ_SETMARK_DETECTED 0x03
#define SCSI_SENSEQ_END_OF_MEDIA_DETECTED 0x02
#define SCSI_SENSEQ_BEGINNING_OF_MEDIA_DETECTED 0x04

// SCSI IO Device Control Codes

#define FILE_DEVICE_SCSI 0x0000001b

#define IOCTL_SCSI_EXECUTE_IN   ((FILE_DEVICE_SCSI << 16) + 0x0011)
#define IOCTL_SCSI_EXECUTE_OUT  ((FILE_DEVICE_SCSI << 16) + 0x0012)
#define IOCTL_SCSI_EXECUTE_NONE ((FILE_DEVICE_SCSI << 16) + 0x0013)

// SMART support in atapi

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

// Read Capacity Data - returned in Big Endian format

typedef struct _READ_CAPACITY_DATA {
    ULONG LogicalBlockAddress;
    ULONG BytesPerBlock;
} READ_CAPACITY_DATA, *PREAD_CAPACITY_DATA;

// CD ROM Read Table Of Contents (TOC) structures
// Format 0 - Get table of contents

#define TocControl_TrkMode_Mask           WParam_TrkMode_Mask
#define TocControl_TrkMode_Audio          WParam_TrkMode_Audio
#define TocControl_TrkMode_Audio_PreEmph  WParam_TrkMode_Audio_PreEmph
#define TocControl_TrkMode_Data           WParam_TrkMode_Data
#define TocControl_TrkMode_IncrData       WParam_TrkMode_IncrData
#define TocControl_TrkMode_QAudio_PreEmph WParam_TrkMode_QAudio_PreEmph
#define TocControl_TrkMode_AllowCpy       WParam_TrkMode_AllowCpy

typedef struct _TOC_TRACK_INFO {
    UCHAR Reserved;
    UCHAR Control : 4;
    UCHAR Adr : 4;
    UCHAR TrackNum;
    UCHAR Reserved1;
    UCHAR LBA[4];
} TOC_TRACK_INFO, *PTOC_TRACK_INFO;

typedef struct _READ_TOC_HEADER {
    UCHAR Length[2];
    UCHAR First_TrackSes;
    UCHAR Last_TrackSes;
} READ_TOC_HEADER, *PREAD_TOC_HEADER;

#define TOC_LastTrack_ID                    0xAA

typedef struct _READ_TOC_TOC {
    READ_TOC_HEADER Tracks;
    TOC_TRACK_INFO TrackData[MAXIMUM_NUMBER_OF_TRACKS+1];
} READ_TOC_TOC, *PREAD_TOC_TOC;

// Format 1 - Session Info

typedef struct _READ_TOC_SES {
    READ_TOC_HEADER Sessions;
    TOC_TRACK_INFO LastSes_1stTrack;            // First Track Number In Last Complete Session
} READ_TOC_SES, *PREAD_TOC_SES;

// Format 2,3 - Full TOC, PMA

// ADR = 1
#define POINT_StartPositionOfTrack_Min      0x01
#define POINT_StartPositionOfTrack_Max      0x63
#define POINT_FirstTrackNum                 0xA0
#define POINT_LastTrackNum                  0xA1
#define POINT_StartPositionOfLeadOut        0xA2
// ADR = 5
#define POINT_SkipInterval_Min              0x01
#define POINT_SkipInterval_Max              0x40
#define POINT_StartPositionOfNextProgramArea 0xB0
#define POINT_NumOfSkips                    0xB1
#define POINT_SkipTrackAssignmet_Min        0xB2
#define POINT_SkipTrackAssignmet_Max        0xB4
#define POINT_StartPositionOfFirstLeadIn    0xC0
#define POINT_CopyOfAdditionalAreaInATIP    0xC1

typedef struct _TOC_SES_INFO {
    UCHAR SesNumber;
    UCHAR Control : 4;
    UCHAR Adr : 4;
    UCHAR TNO;
    UCHAR POINT;

    union {

        struct {
            UCHAR MSF[3];
            UCHAR Reserved;
            UCHAR P_MSF[3];
        } GENERIC;

// ADR = 1:

//0x01 - 0x63
        struct {
            UCHAR ATIME[3];
            UCHAR Zero;
            UCHAR MSF[3];
        } StartPositionOfTrack;

//0xA0
#define FullTOC_DiscType_CDDA_or_M1  WParam_SesFmt_CdRom        // 0x00
#define FullTOC_DiscType_CDI         WParam_SesFmt_CdI          // 0x10
#define FullTOC_DiscType_CDXA_M2     WParam_SesFmt_CdRomXa      // 0x20

        struct {
            UCHAR ATIME[3];
            UCHAR Zero;
            UCHAR FirstTrackNum;
            UCHAR DiscType;
            UCHAR Zero1;
        } FirstTrackNum;

//0xA1
        struct {
            UCHAR ATIME[3];
            UCHAR Zero;
            UCHAR LastTrackNum;
            UCHAR Zero1[2];
        } LastTrackNum;

//0xA2
        struct {
            UCHAR ATIME[3];
            UCHAR Zero;
            UCHAR MSF[3];
        } StartPositionOfLeadOut;

// ADR = 5:

//0x01 - 0x40
        struct {
            UCHAR End_MSF[3];
            UCHAR Reserved;
            UCHAR Start_MSF[3];
        } SkipInterval;

//0xB0
        struct {
            UCHAR Program_MSF[3];
            UCHAR NumOfPointers_M5;
            UCHAR MaxLeadOut_MSF[3];
        } StartPositionOfNextProgramArea;

//0xB1
        struct {
            UCHAR Zero[4];
            UCHAR Intervals;
            UCHAR Tracks;
            UCHAR Zero1;
        } NumOfSkips;

//0xB2 - 0xB4
        struct {
            UCHAR SkipNum[7];
        } SkipTrackAsignment;

//0xC0
        struct {
            UCHAR OptimumRecordingPower;
            UCHAR SpecInfoATIP[3];
            UCHAR LeadIn_MSF[3];
        } StartPositionOfFirstLeadIn;

//0xC1
        struct {
            UCHAR Bytes[7];
        } AdditionalAreaInATIP;

    } Params;
} TOC_SES_INFO, *PTOC_SES_INFO;

typedef struct _READ_TOC_FULL_TOC {
    READ_TOC_HEADER Sessions;
    TOC_SES_INFO SessionData[MAXIMUM_NUMBER_OF_SESSIONS];
} READ_TOC_FULL_TOC, *PREAD_TOC_FULL_TOC;

typedef READ_TOC_FULL_TOC   READ_TOC_PMA;
typedef PREAD_TOC_FULL_TOC  PREAD_TOC_PMA;

// Format 4 - ATIP

typedef struct _READ_TOC_ATIP {
    UCHAR Length[2];
    UCHAR Reserved[2];

#define ATIP_SpeedRef_Mask  0x07
#define ATIP_SpeedRef_2X    0x01
#define ATIP_WritingPower_Mask  0x07

    union {
        UCHAR Flags;
        struct {
            UCHAR SpeedRef: 3;
            UCHAR Reserved: 1;
            UCHAR WritingPower: 3;
            UCHAR One: 1;
        } Fields;
    } Flags1;

#define ATIP_URU            0x40

    union {
        UCHAR Flags;
        struct {
            UCHAR Reserved: 6;
            UCHAR URU: 1;
            UCHAR Zero: 1;
        } Fields;
    } Flags2;

#define ATIP_A1             0x01        // 16-18 are valid
#define ATIP_A2             0x02        // 20-22 are valid
#define ATIP_A3             0x04        // 24-26 are valid
#define ATIP_SubType_Mask   0x38        // shall be set to zero
#define ATIP_Type_Mask      0x40
#define ATIP_Type_CDR       0x00
#define ATIP_Type_CDRW      0x40

    union {
        UCHAR Flags;
        struct {
            UCHAR A1: 1;
            UCHAR A2: 1;
            UCHAR A3: 1;
            UCHAR SubType: 3;
            UCHAR Type: 1;
            UCHAR One: 1;
        } Fields;
    } DiscType;

    UCHAR Reserved0;
    UCHAR LeadIn_MSF[3];
    UCHAR Reserved1;
    UCHAR LeadOut_MSF[3];
    UCHAR Reserved2;

#define ATIP_MinSpeedCVL_Mask   0x70
#define ATIP_MinSpeedCVL_2X     0x10
#define ATIP_MaxSpeedCVL_Mask   0x0f
#define ATIP_MaxSpeedCVL_2X     0x01
#define ATIP_MaxSpeedCVL_4X     0x02
#define ATIP_MaxSpeedCVL_6X     0x03
#define ATIP_MaxSpeedCVL_8X     0x04

    union {
        UCHAR Flags;
        struct {
            UCHAR MaxSpeedCVL: 4;
            UCHAR MinSpeedCVL: 3;
            UCHAR Zero: 1;
        } Fields;
    } Speed;

#define ATIP_Power_Y_Mask       0x0e
#define ATIP_Power_P_Mask       0x70

    union {
        UCHAR Flags;
        struct {
            UCHAR Reserved: 1;
            UCHAR Y_value: 3;
            UCHAR P_factor: 3;
            UCHAR Zero: 1;
        } Fields;
    } Power;

#define ATIP_PW_ratio_Mask      0x70

    union {
        UCHAR Flags;
        struct {
            UCHAR Reserved: 4;
            UCHAR P_W_ratio: 3;
            UCHAR Zero: 1;
        } Fields;
    } ErasePower;

    UCHAR Reserved3;
    UCHAR A1_value[3];
    UCHAR Reserved4;
    UCHAR A2_value[3];
    UCHAR Reserved5;
    UCHAR A3_value[3];

} READ_TOC_ATIP, *PREAD_TOC_ATIP;

// Format 5 - CD-TEXT

typedef struct _CD_TEXT_PACK_DATA {

#define CdText_ID1_Title        0x80    // ID2 = 0 - Album, ID2 = 1-63 - Track
#define CdText_ID1_Performer    0x81
#define CdText_ID1_Songwriter   0x82
#define CdText_ID1_Composer     0x83
#define CdText_ID1_Arranger     0x84
#define CdText_ID1_Message      0x85
#define CdText_ID1_DiscID       0x86
#define CdText_ID1_GenreInfo    0x87
#define CdText_ID1_TOC          0x88
#define CdText_ID1_TOC2         0x89
#define CdText_ID1_Special      0x8D
#define CdText_ID1_UPC_EAN_ISRC 0x8E
#define CdText_ID1_BlockSizeInfo 0x8F

    UCHAR ID1;
    UCHAR ID2;
    UCHAR ID3;

#define CdText_CharPos_Mask     0x0f
#define CdText_BlkNum_Mask      0x70

    union {
        UCHAR Flags;
        struct {
            UCHAR CharPos: 4;
            UCHAR BlkNum: 3;
            UCHAR DBCC: 1;          // used to indicate Double Byte text encoding (Unicode ?)
        } Fields;
    } BlkNum_CharPos;

    UCHAR TextData[12];
    UCHAR CRC[2];
} CD_TEXT_PACK_DATA, *PCD_TEXT_PACK_DATA;

typedef struct _READ_TOC_CD_TEXT {
    UCHAR Length[2];
    UCHAR Reserved[2];
    CD_TEXT_PACK_DATA Chunk0;
} READ_TOC_CD_TEXT, *PREAD_TOC_CD_TEXT;

// OPC block

typedef struct _OPC_INFO_BLOCK {
    UCHAR Speed[2];
    UCHAR OpcValue[6];
} OPC_INFO_BLOCK, *POPC_INFO_BLOCK;

// Buffer Capacity format

typedef struct _BUFFER_CAPACITY_BLOCK {
    UCHAR DataLength[2];
    UCHAR Reserved0[2];
    UCHAR BufferLength[4];
    UCHAR BlankBufferLength[4];
} BUFFER_CAPACITY_BLOCK, *PBUFFER_CAPACITY_BLOCK;

// Format Unit structures

typedef struct _FORMAT_LIST_HEADER {
    UCHAR Reserved0;

#define FormatHeader_VS     0x01
#define FormatHeader_Immed  0x02
#define FormatHeader_DSP    0x04
#define FormatHeader_IP     0x08
#define FormatHeader_STPF   0x10
#define FormatHeader_DCRT   0x20
#define FormatHeader_DPRY   0x40
#define FormatHeader_FOV    0x80

    union {
        UCHAR Flags;
        struct {
            UCHAR VS: 1;
            UCHAR Immed: 1;
            UCHAR DSP: 1;
            UCHAR IP: 1;
            UCHAR STPF: 1;
            UCHAR DCRT: 1;
            UCHAR DPRY: 1;
            UCHAR FOV: 1;
        } Fields;
    } Flags;
    UCHAR FormatDescriptorLength[2];            // =0x0008
} FORMAT_LIST_HEADER, *PFORMAT_LIST_HEADER;

typedef struct _CDRW_FORMAT_DESCRIPTOR {

#define FormatDesc_Grow     0x40
#define FormatDesc_Ses      0x80

    union {
        UCHAR Flags;
        struct {
            UCHAR Reserved0: 6;
            UCHAR Grow: 1;
            UCHAR Ses: 1;
        } Fields;
    } Flags;
    UCHAR Reserved1[3];
    UCHAR FormatSize[4];
} CDRW_FORMAT_DESCRIPTOR, *PCDRW_FORMAT_DESCRIPTOR;

typedef struct _FORMAT_UNIT_PARAMETER_LIST {
    FORMAT_LIST_HEADER Header;
    UCHAR InitPatternDescr[4];
    CDRW_FORMAT_DESCRIPTOR FormatDescr;
} FORMAT_UNIT_PARAMETER_LIST, *PFORMAT_UNIT_PARAMETER_LIST;

// define Read Format Capacities info blocks

typedef struct _CAPACITY_LIST_HEADER {
    UCHAR Reserved[3];
    UCHAR Length;
} CAPACITY_LIST_HEADER, *PCAPACITY_LIST_HEADER;

typedef struct _FORMATTABLE_CAPACITY_DESCRIPTOR {
    UCHAR NumOfBlocks [4];
    union {
        UCHAR Flags;
        struct {
            UCHAR DescType: 2;
            UCHAR Reserved0: 6;
        } Fields;
    } Flags;
    UCHAR BlockSize [3];
} FORMATTABLE_CAPACITY_DESCRIPTOR, *PFORMATTABLE_CAPACITY_DESCRIPTOR;

typedef struct _FORMAT_CAPACITIES_DATA {
    CAPACITY_LIST_HEADER Header;
} FORMAT_CAPACITIES_DATA, *PFORMAT_CAPACITIES_DATA;

// Define Event Status info blocks

typedef struct _EVENT_STAT_HEADER {
    UCHAR DataLength[2];

#define EventRetStat_Class_Mask       0x07
#define EventRetStat_Class_OpChange   0x01
#define EventRetStat_Class_PM         0x02
#define EventRetStat_Class_Media      0x04
#define EventRetStat_Class_DevBusy    0x06
#define EventRetStat_NEA              0x80

    union {
        UCHAR Flags;
        struct {
            UCHAR Class: 3;
            UCHAR Reserved0: 4;
            UCHAR NEA: 1;
        } Fields;
    } Flags;

    UCHAR SupportedClasses;  // see command format

} EVENT_STAT_HEADER, *PEVENT_STAT_HEADER;

typedef struct _EVENT_STAT_OPERATIONAL_BLOCK {

    EVENT_STAT_HEADER Header;

#define EventStat_OpEvent_Mask      0x0f

    union {
        UCHAR Flags;
        struct {
            UCHAR OpEvent  : 4;
            UCHAR Reserved0: 4;
        } Fields;
    } Byte0;

#define EventStat_OpStat_Mask       0x0f
#define EventStat_OpStat_Ready      0x00
#define EventStat_OpStat_TempBusy   0x01
#define EventStat_OpStat_Busy       0x02

    union {
        UCHAR Flags;
        struct {
            UCHAR OpStatus  : 4;
            UCHAR Reserved0 : 3;
            UCHAR PersistentPrevent: 1;
        } Fields;
    } Byte1;

#define EventStat_OpReport_NoChg        0x00
#define EventStat_OpReport_Change       0x01
#define EventStat_OpReport_AddChg       0x02
#define EventStat_OpReport_Reset        0x03
#define EventStat_OpReport_FirmwareChg  0x04 // microcode change
#define EventStat_OpReport_InquaryChg   0x05
#define EventStat_OpReport_CtrlReq      0x06
#define EventStat_OpReport_CtrlRelease  0x07

    UCHAR OpReport[2];

} EVENT_STAT_OPERATIONAL_BLOCK, *PEVENT_STAT_OPERATIONAL_BLOCK;

typedef struct _EVENT_STAT_PM_BLOCK {

    EVENT_STAT_HEADER Header;

#define EventStat_PowerEvent_Mask       0x0f
#define EventStat_PowerEvent_NoChg      0x00
#define EventStat_PowerEvent_ChgOK      0x01
#define EventStat_PowerEvent_ChgFail    0x02

    union {
        UCHAR Flags;
        struct {
            UCHAR PowerEvent : 4;
            UCHAR Reserved0  : 4;
        } Fields;
    } Byte0;

#define EventStat_PowerStat_Mask        0x0f
#define EventStat_PowerStat_Active      0x01
#define EventStat_PowerStat_Idle        0x02
#define EventStat_PowerStat_Standby     0x03
#define EventStat_PowerStat_Sleep       0x04

    union {
        UCHAR Flags;
        struct {
            UCHAR PowerStatus: 4;
            UCHAR Reserved0  : 4;
        } Fields;
    } Byte1;

    UCHAR Reserved0[2];

} EVENT_STAT_PM_BLOCK, *PEVENT_STAT_PM_BLOCK;

typedef struct _EVENT_STAT_MEDIA_BLOCK {

    EVENT_STAT_HEADER Header;

#define EventStat_MediaEvent_Mask       0x0f
#define EventStat_MediaEvent_None       0x00
#define EventStat_MediaEvent_EjectReq   0x01
#define EventStat_MediaEvent_New        0x02
#define EventStat_MediaEvent_Removal    0x03
#define EventStat_MediaEvent_Chg        0x04

    union {
        UCHAR Flags;
        struct {
            UCHAR MediaEvent : 4;
            UCHAR Reserved0  : 4;
        } Fields;
    } Byte0;

#define EventStat_MediaStat_DoorOpen    0x01
#define EventStat_MediaStat_Present     0x02

    union {
        UCHAR Flags;
        struct {
            UCHAR DoorOpen  : 1;
            UCHAR Present   : 1;
            UCHAR Reserved0 : 6;
        } Fields;
    } Byte1;

    UCHAR StartSlot;
    UCHAR EndSlot;

} EVENT_STAT_MEDIA_BLOCK, *PEVENT_STAT_MEDIA_BLOCK;

typedef struct _EVENT_STAT_DEV_BUSY_BLOCK {

    EVENT_STAT_HEADER Header;

#define EventStat_BusyEvent_Mask       0x0f
#define EventStat_BusyEvent_None       0x00
#define EventStat_BusyEvent_Busy       0x01

    union {
        UCHAR Flags;
        struct {
            UCHAR BusyEvent : 4;
            UCHAR Reserved0 : 4;
        } Fields;
    } Byte0;

#define EventStat_BusyStat_Mask        0x0f
#define EventStat_BusyStat_NoEvent     0x00
#define EventStat_BusyStat_Power       0x01
#define EventStat_BusyStat_Immed       0x02
#define EventStat_BusyStat_Deferred    0x03

    union {
        UCHAR Flags;
        struct {
            UCHAR BusyStatus: 4;
            UCHAR Reserved0 : 4;
        } Fields;
    } Byte1;

    UCHAR Time[2];

} EVENT_STAT_DEV_BUSY_BLOCK, *PEVENT_STAT_DEV_BUSY_BLOCK;

// Define mode disc info block.

typedef struct _DISC_INFO_BLOCK {        //
    UCHAR DataLength [2];

#define DiscInfo_Disk_Mask          0x03
#define DiscInfo_Disk_Empty         0x00
#define DiscInfo_Disk_Appendable    0x01
#define DiscInfo_Disk_Complete      0x02

#define DiscInfo_Ses_Mask       0x0C
#define DiscInfo_Ses_Empty      0x00
#define DiscInfo_Ses_Incomplete 0x04
#define DiscInfo_Ses_Complete   0x0C

#define DiscInfo_Disk_Erasable  0x10

    union {
        UCHAR Flags;
        struct {
            UCHAR DiscStat : 2;
            UCHAR LastSesStat : 2;
            UCHAR Erasable : 1;
            UCHAR Reserved0: 3;
        } Fields;
    } DiscStat;

    UCHAR FirstTrackNum;
    UCHAR NumOfSes;
    UCHAR FirstTrackNumLastSes;
    UCHAR LastTrackNumLastSes;

#define DiscInfo_URU            0x20
#define DiscInfo_DBC_V          0x40
#define DiscInfo_DID_V          0x80

    union {
        UCHAR Flags;
        struct {
            UCHAR Reserved1: 5;
            UCHAR URU      : 1;
            UCHAR DBC_V    : 1; // 0
            UCHAR DID_V    : 1;
        } Fields;
    } Flags;

#define DiscInfo_Type_cdrom     0x00    // CD-DA / CD-ROM
#define DiscInfo_Type_cdi       0x10    // CD-I
#define DiscInfo_Type_cdromxa   0x20    // CD-ROM XA
#define DiscInfo_Type_unknown   0xFF    // HZ ;)

    UCHAR DiskType;
    UCHAR NumOfSes2;              // MSB MMC-3
    UCHAR FirstTrackNumLastSes2;  // MSB MMC-3
    UCHAR LastTrackNumLastSes2;   // MSB MMC-3
    UCHAR DiskId [4];
    UCHAR LastSesLeadInTime [4];  // MSF
    UCHAR LastSesLeadOutTime [4]; // MSF
    UCHAR DiskBarCode [8];
    UCHAR Reserved3;
    UCHAR OPCNum;
} DISC_INFO_BLOCK, *PDISC_INFO_BLOCK;

// Define track info block.

typedef struct _TRACK_INFO_BLOCK {
    UCHAR DataLength [2];
    UCHAR TrackNum;
    UCHAR SesNum;
    UCHAR Reserved0;

#define TrkInfo_Trk_Mask    0x0F
#define TrkInfo_Trk_Mode1   0x01
#define TrkInfo_Trk_Mode2   0x02
#define TrkInfo_Trk_XA      0x02
#define TrkInfo_Trk_DDCD    0x04  // MMC-3
#define TrkInfo_Trk_NonCD   0x04  // MMC-3
#define TrkInfo_Trk_Inc     0x05  // MMC-3
#define TrkInfo_Trk_unknown 0x0F
#define TrkInfo_Copy        0x10
#define TrkInfo_Damage      0x20

    UCHAR TrackParam;
/*  UCHAR TrackMode : 4;
    UCHAR Copy      : 1;
    UCHAR Damage    : 1;
    UCHAR Reserved1 : 2; */

#define TrkInfo_Dat_Mask    0x0F
#define TrkInfo_Dat_Mode1   0x01
#define TrkInfo_Dat_Mode2   0x02
#define TrkInfo_Dat_XA      0x02
#define TrkInfo_Dat_DDCD    0x02
#define TrkInfo_Dat_unknown 0x0F
#define TrkInfo_FP          0x10
#define TrkInfo_Packet      0x20
#define TrkInfo_Blank       0x40
#define TrkInfo_RT          0x80

    UCHAR DataParam;
/*  UCHAR DataMode  : 4;
    UCHAR FP        : 1;
    UCHAR Packet    : 1;
    UCHAR Blank     : 1;
    UCHAR RT        : 1; */

#define TrkInfo_NWA_V       0x01
#define TrkInfo_LRA_V       0x02  // MMC-3

    UCHAR NWA_V;
/*  UCHAR NWA_V     : 1;
    UCHAR LRA_V     : 1;
    UCHAR Reserved  : 6; */

    UCHAR TrackStartAddr [4];
    UCHAR NextWriteAddr [4];
    UCHAR FreeBlocks [4];
    UCHAR FixPacketSize [4];
    UCHAR TrackLength [4];

// MMC-3

    UCHAR LastRecordedAddr [4];
    UCHAR TrackNum2;  // MSB
    UCHAR SesNum2;    // MSB
    UCHAR Reserved2[2];

}TRACK_INFO_BLOCK, *PTRACK_INFO_BLOCK;

// Mode data structures.

// Define Mode parameter header.

#define MediaType_Unknown                        0x00
#define MediaType_120mm_CDROM_DataOnly           0x01
#define MediaType_120mm_CDROM_AudioOnly          0x02        //CDDA
#define MediaType_120mm_CDROM_DataAudioCombined  0x03
#define MediaType_120mm_CDROM_Hybrid_PhotoCD     0x04
#define MediaType_80mm_CDROM_DataOnly            0x05
#define MediaType_80mm_CDROM_AudioOnly           0x06        //CDDA
#define MediaType_80mm_CDROM_DataAudioCombined   0x07
#define MediaType_80mm_CDROM_Hybrid_PhotoCD      0x08

#define MediaType_UnknownSize_CDR                0x10
#define MediaType_120mm_CDR_DataOnly             0x11
#define MediaType_120mm_CDR_AudioOnly            0x12        //CDDA
#define MediaType_120mm_CDR_DataAudioCombined    0x13
#define MediaType_120mm_CDR_Hybrid_PhotoCD       0x14
#define MediaType_80mm_CDR_DataOnly              0x15
#define MediaType_80mm_CDR_AudioOnly             0x16        //CDDA
#define MediaType_80mm_CDR_DataAudioCombined     0x17
#define MediaType_80mm_CDR_Hybrid_Photo_CD       0x18

#define MediaType_UnknownSize_CDRW               0x20
#define MediaType_120mm_CDRW_DataOnly            0x21
#define MediaType_120mm_CDRW_AudioOnly           0x22        //CDDA
#define MediaType_120mm_CDRW_DataAudioCombined   0x23
#define MediaType_120mm_CDRW_Hybrid              0x24
#define MediaType_80mm_CDRW_DataOnly             0x25
#define MediaType_80mm_CDRW_AudioOnly            0x26        //CDDA
#define MediaType_80mm_CDRW_DataAudioCombined    0x27
#define MediaType_80mm_CDRW_Hybrid               0x28

#define MediaType_NoDiscPresent                  0x70
#define MediaType_DoorOpen                       0x71

//*********************************************************************************************

typedef struct _MODE_PARAMETER_HEADER {
    UCHAR ModeDataLength;
    UCHAR MediumType;
    UCHAR DeviceSpecificParameter;
    UCHAR BlockDescriptorLength;
} MODE_PARAMETER_HEADER, *PMODE_PARAMETER_HEADER;

typedef struct _MODE_PARAMETER_HEADER10 {
    UCHAR ModeDataLength[2];
    UCHAR MediumType;
    UCHAR DeviceSpecificParameter;
    UCHAR Reserved[2];
    UCHAR BlockDescriptorLength[2];
} MODE_PARAMETER_HEADER10, *PMODE_PARAMETER_HEADER10;

#define MODE_FD_SINGLE_SIDE     0x01
#define MODE_FD_DOUBLE_SIDE     0x02
#define MODE_FD_MAXIMUM_TYPE    0x1E
#define MODE_DSP_FUA_SUPPORTED  0x10
#define MODE_DSP_WRITE_PROTECT  0x80

// Define the mode parameter block.

typedef struct _MODE_PARAMETER_BLOCK {
    UCHAR DensityCode;
    UCHAR NumberOfBlocks[3];
    UCHAR Reserved;
    UCHAR BlockLength[3];
} MODE_PARAMETER_BLOCK, *PMODE_PARAMETER_BLOCK;

typedef struct _MODE_PARM_READ_WRITE {

   MODE_PARAMETER_HEADER  ParameterListHeader;  // List Header Format
   MODE_PARAMETER_BLOCK   ParameterListBlock;   // List Block Descriptor

} MODE_PARM_READ_WRITE_DATA, *PMODE_PARM_READ_WRITE_DATA;

//*********************************************************************************************
// Define read write recovery page

typedef struct _MODE_READ_WRITE_RECOVERY_PAGE {     // 0x01

    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;
    union {
        UCHAR Flags;
        struct {
            UCHAR DCRBit : 1;
            UCHAR DTEBit : 1;
            UCHAR PERBit : 1;
            UCHAR EERBit : 1;
            UCHAR RCBit : 1;
            UCHAR TBBit : 1;
            UCHAR ARRE : 1;
            UCHAR AWRE : 1;
        } Fields;
    } ErrorRecoveryParam;
    UCHAR ReadRetryCount;
    UCHAR CorrectionSpan;           //SCSI CBS only
    UCHAR HeadOffsetCount;          //SCSI CBS only
    UCHAR DataStrobOffsetCount;     //SCSI CBS only
    UCHAR Reserved4;
    UCHAR WriteRetryCount;
    UCHAR Reserved5;
    UCHAR RecoveryTimeLimit[2];     // 0

} MODE_READ_WRITE_RECOVERY_PAGE, *PMODE_READ_WRITE_RECOVERY_PAGE;

// Define Read Recovery page - cdrom

typedef struct _MODE_READ_RECOVERY_PAGE {       // 0x01

    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;
    UCHAR DCRBit : 1;
    UCHAR DTEBit : 1;
    UCHAR PERBit : 1;
    UCHAR Reserved2 : 1;
    UCHAR RCBit : 1;
    UCHAR TBBit : 1;
    UCHAR Reserved3 : 2;
    UCHAR ReadRetryCount;
    UCHAR Reserved4[4];

} MODE_READ_RECOVERY_PAGE, *PMODE_READ_RECOVERY_PAGE;

//*********************************************************************************************
// Define mode write parameters page.

typedef struct _MODE_WRITE_PARAMS_PAGE {        // 0x05
    UCHAR PageCode : 6;
    UCHAR Reserved1: 1;
    UCHAR PageSavable : 1;

    UCHAR PageLength;               // 0x32

#define WParam_WType_Mask   0x0f
#define WParam_WType_Packet 0x00
#define WParam_WType_TAO    0x01
#define WParam_WType_Ses    0x02
#define WParam_WType_Raw    0x03
#define WParam_TestWrite    0x10
#define WParam_LS_V         0x20
#define WParam_BUFF         0x40

    union {
        UCHAR Flags;
        struct {
            UCHAR WriteType: 4;             // 1
            UCHAR TestWrite: 1;
            UCHAR LS_V: 1;
            UCHAR BUFF: 1;
            UCHAR Reserved1: 1;
        } Fields;
    } Byte2;

#define WParam_TrkMode_Mask             0x0d // xx0x
#define WParam_TrkMode_None             0x00
#define WParam_TrkMode_Audio            0x00
#define WParam_TrkMode_Audio_PreEmph    0x01
#define WParam_TrkMode_Data             0x04
#define WParam_TrkMode_IncrData         0x05
#define WParam_TrkMode_QAudio_PreEmph   0x08
#define WParam_TrkMode_AllowCpy         0x02
#define WParam_Copy             0x10
#define WParam_FP               0x20
#define WParam_MultiSes_Mask    0xc0
#define WParam_Multises_None    0x00
#define WParam_Multises_Final   0x80
#define WParam_Multises_Multi   0xc0

    union {
        UCHAR Flags;
        struct {
            UCHAR TrackMode: 4;             // 4
            UCHAR Copy     : 1;             // 0
            UCHAR FP       : 1;             // 0
            UCHAR Multisession: 2;          // 11
        } Fields;
    } Byte3;

#define WParam_BlkType_Mask         0x0f
#define WParam_BlkType_Raw_2352     0x00
#define WParam_BlkType_RawPQ_2368   0x01
#define WParam_BlkType_RawPW_2448   0x02
#define WParam_BlkType_RawPW_R_2448 0x03
#define WParam_BlkType_VendorSpec1  0x07
#define WParam_BlkType_M1_2048      0x08
#define WParam_BlkType_M2_2336      0x09
#define WParam_BlkType_M2XAF1_2048  0x0a
#define WParam_BlkType_M2XAF1SH_2056 0x0b
#define WParam_BlkType_M2XAF2_2324  0x0c
#define WParam_BlkType_M2XAFXSH_2332 0x0d
#define WParam_BlkType_VendorSpec2  0x0f

    union {
        UCHAR Flags;
        struct {
            UCHAR DataBlockType: 4;         // 8
            UCHAR Reserved2: 4;
        } Fields;
    } Byte4;

    UCHAR LinkSize;
    UCHAR Reserved3;

    union {
        UCHAR Flags;
        struct {
            UCHAR HostAppCode : 6;          // 0
            UCHAR Reserved4   : 2;
        } Fields;
    } Byte7;

#define WParam_SesFmt_CdRom     0x00
#define WParam_SesFmt_CdI       0x10
#define WParam_SesFmt_CdRomXa   0x20

    UCHAR SesFmt;                   // 0
    UCHAR Reserved5;
    UCHAR PacketSize[4];            // 0
    UCHAR AudioPause[2];            // 150

    UCHAR Reserved6: 7;
    UCHAR MCVAL    : 1;

    UCHAR N[13];
    UCHAR Zero;
    UCHAR AFRAME;

    UCHAR Reserved7: 7;
    UCHAR TCVAL    : 1;

    UCHAR I[12];
    UCHAR Zero_2;
    UCHAR AFRAME_2;
    UCHAR Reserved8;

    struct {
        union {
            UCHAR MSF[3];
            struct _SubHdrParams1 {
                UCHAR FileNum;
                UCHAR ChannelNum;

#define WParam_SubHdr_SubMode0          0x00
#define WParam_SubHdr_SubMode1          0x08

                UCHAR SubMode;
            } Params1;
        } Params;

#define WParam_SubHdr_Mode_Mask         0x03
#define WParam_SubHdr_Mode0             0x00
#define WParam_SubHdr_Mode1             0x01
#define WParam_SubHdr_Mode2             0x02
#define WParam_SubHdr_Format_Mask       0xe0
#define WParam_SubHdr_Format_UserData   0x00
#define WParam_SubHdr_Format_RunIn4     0x20
#define WParam_SubHdr_Format_RunIn3     0x40
#define WParam_SubHdr_Format_RunIn2     0x60
#define WParam_SubHdr_Format_RunIn1     0x80
#define WParam_SubHdr_Format_Link       0xa0
#define WParam_SubHdr_Format_RunOut2    0xc0
#define WParam_SubHdr_Format_RunOut1    0xe0

        union {
            UCHAR Flags;
            struct {
                UCHAR Mode      : 2;
                UCHAR Reserved  : 3;
                UCHAR Format    : 3;
            } Fields;
        } Mode;
    } SubHeader ;

} MODE_WRITE_PARAMS_PAGE, *PMODE_WRITE_PARAMS_PAGE;

typedef struct _MODE_WRITE_PARAMS_PAGE_3 {
    MODE_WRITE_PARAMS_PAGE Standard;
    UCHAR VendorSpec[4];
} MODE_WRITE_PARAMS_PAGE_3, *PMODE_WRITE_PARAMS_PAGE_3;

//*********************************************************************************************
// Define Caching page.

typedef struct _MODE_CACHING_PAGE {         // 0x08
    UCHAR PageCode : 6;
    UCHAR Reserved1: 1;
    UCHAR PageSavable : 1;
    UCHAR PageLength;
    UCHAR ReadDisableCache : 1;
    UCHAR MultiplicationFactor : 1;
    UCHAR WriteCacheEnable : 1;
    UCHAR Reserved2 : 5;
    UCHAR WriteRetensionPriority : 4;
    UCHAR ReadRetensionPriority : 4;
    UCHAR DisablePrefetchTransfer[2];
    UCHAR MinimumPrefetch[2];
    UCHAR MaximumPrefetch[2];
    UCHAR MaximumPrefetchCeiling[2];
} MODE_CACHING_PAGE, *PMODE_CACHING_PAGE;

//*********************************************************************************************
// Define CD Parameters page.

typedef struct _MODE_CD_PARAMS_PAGE {         // 0x0D
    UCHAR PageCode : 6;
    UCHAR Reserved : 1;
    UCHAR PageSavable : 1;

    UCHAR PageLength;                       // 0x06
    UCHAR Reserved1;

#define CdParams_InactvityTime_Mask     0x0f

    union {
        UCHAR Flags;
        struct {
            UCHAR InactivityTime: 4;        // 1 - 125ms, 2 - 250ms... 9 - 32s, A - 1min...
            UCHAR Reserved0 : 4;
        } Fields;
    } Byte2;

    UCHAR SUnits_per_MUnit[2];
    UCHAR FUnits_per_SUnit[2];
} MODE_CD_PARAMS_PAGE, *PMODE_CD_PARAMS_PAGE;

//*********************************************************************************************
// Define CD Audio Control Mode page.

typedef struct _CDDA_PORT_CONTROL {

#define CddaPort_Channel_Mask       0x0f
#define CddaPort_Channel_Mute       0x00
#define CddaPort_Channel_0          0x01
#define CddaPort_Channel_1          0x02
#define CddaPort_Channel_0_1        0x03
#define CddaPort_Channel_2          0x04
#define CddaPort_Channel_3          0x08

    UCHAR ChannelSelection;
    UCHAR Volume;
} CDDA_PORT_CONTROL, *PCDDA_PORT_CONTROL;

typedef struct _MODE_CD_AUDIO_CONTROL_PAGE {         // 0x0E
    UCHAR PageCode : 6;
    UCHAR Reserved1: 1;
    UCHAR PageSavable : 1;

    UCHAR PageLength;                       // 0x0E

#define CdAudio_SOTC        0x02
#define CdAudio_Immed       0x04

    union {
        UCHAR Flags;
        struct {
            UCHAR Reserved0 : 1;
            UCHAR SOTC      : 1;
            UCHAR Immed     : 1;
            UCHAR Reserved1 : 5;
        } Fields;
    } Byte2;

    UCHAR Reserved2[2];
    UCHAR LbaFormat;
    UCHAR LogicalBlocksPerSecond[2];
    CDDA_PORT_CONTROL Port[4];
} MODE_CD_AUDIO_CONTROL_PAGE, *PMODE_CD_AUDIO_CONTROL_PAGE;

//*********************************************************************************************
// Define Power Condition Mode page.

typedef struct _MODE_POWER_CONDITION_PAGE {         // 0x1A
    UCHAR PageCode : 6;
    UCHAR Reserved1: 1;
    UCHAR PageSavable : 1;

    UCHAR PageLength;                       // 0x0A
    UCHAR Reserved2;

#define PowerCond_Standby       0x01
#define PowerCond_Idle          0x02

    union {
        UCHAR Flags;
        struct {
            UCHAR Standby   : 1;
            UCHAR Idle      : 1;
            UCHAR Reserved1 : 6;
        } Fields;
    } Byte3;

    UCHAR IdleTimer[4];                 // 1unit = 100ms
    UCHAR StandbyTimer[4];              // 1unit = 100ms
} MODE_POWER_CONDITION_PAGE, *PMODE_POWER_CONDITION_PAGE;

//*********************************************************************************************
// Define Fault/Failure Reporting Control page.

typedef struct _MODE_FAIL_REPORT_PAGE {         // 0x1C
    UCHAR PageCode : 6;
    UCHAR Reserved1: 1;
    UCHAR PageSavable : 1;

    UCHAR PageLength;                       // 0x0A

#define FailReport_LogErr       0x01
#define FailReport_Test         0x04
#define FailReport_DExcept      0x08
#define FailReport_Perf         0x80

    union {
        UCHAR Flags;
        struct {
            UCHAR LogErr    : 1;
            UCHAR Reserved1 : 1;
            UCHAR Test      : 1;
            UCHAR DExcept   : 1;
            UCHAR Reserved2 : 3;
            UCHAR Perf      : 1;
        } Fields;
    } Byte2;

    union {
        UCHAR Flags;
        struct {
            UCHAR MRIE      : 4;
            UCHAR Reserved1 : 4;
        } Fields;
    } Byte3;

    UCHAR IntervalTimer[4];                 // 1unit = 100ms
    UCHAR ReportCount[4];
} MODE_FAIL_REPORT_PAGE, *PMODE_FAIL_REPORT_PAGE;

//*********************************************************************************************
// Define Time-out and Protect page.

typedef struct _MODE_TIMEOUT_AND_PROTECT_PAGE {         // 0x1D
    UCHAR PageCode : 6;
    UCHAR Reserved1: 1;
    UCHAR PageSavable : 1;

    UCHAR PageLength;                       // 0x08

    UCHAR Reserved2[2];

#define Timeout_SW          0x01
#define Timeout_DISP        0x02
#define Timeout_TMOE        0x04

    union {
        UCHAR Flags;
        struct {
            UCHAR SW       : 1;
            UCHAR DISP     : 1;
            UCHAR TMOE     : 1;
            UCHAR Reserved : 5;
        } Fields;
    } Byte4;

    UCHAR Reserved3;

    UCHAR Group1_Timeout[2];                 // 1unit = 1s
    UCHAR Group2_Timeout[2];                 // 1unit = 1s
} MODE_TIMEOUT_AND_PROTECT_PAGE, *PMODE_TIMEOUT_AND_PROTECT_PAGE;

//*********************************************************************************************
// Define Philips CD-R(W) Sector Mode page.

typedef struct _MODE_PHILIPS_SECTOR_TYPE_PAGE {   // 0x21
    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;

    UCHAR PageLength;

    UCHAR Reserved0[2];

    union {
        UCHAR Flags;
        struct {
            UCHAR DataBlockType: 4;         // 8
            UCHAR Reserved2: 4;
        } Fields;
    } Byte4;

#define WParams_Philips_CreateNewTrack      0

    UCHAR Track;
    UCHAR ISRC[9];

    UCHAR Reserved3[2];
} MODE_PHILIPS_SECTOR_TYPE_PAGE, *PMODE_PHILIPS_SECTOR_TYPE_PAGE;

//*********************************************************************************************
// Define CD-X Capabilities and Mechanical Status page.

typedef struct _MODE_CAPABILITIES_PAGE2 {   // 0x2A
    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;

    UCHAR PageLength;

#define DevCap_read_cd_r          0x01 // reserved in 1.2
#define DevCap_read_cd_rw         0x02 // reserved in 1.2
#define DevCap_method2            0x04
#define DevCap_read_dvd_rom       0x08
#define DevCap_read_dvd_r         0x10
#define DevCap_read_dvd_ram       0x20

    UCHAR ReadCap;            // DevCap_*_read
/*    UCHAR cd_r_read         : 1; // reserved in 1.2
    UCHAR cd_rw_read        : 1; // reserved in 1.2
    UCHAR method2           : 1;
    UCHAR dvd_rom           : 1;
    UCHAR dvd_r_read        : 1;
    UCHAR dvd_ram_read      : 1;
    UCHAR Reserved2            : 2;*/

#define DevCap_write_cd_r         0x01 // reserved in 1.2
#define DevCap_write_cd_rw        0x02 // reserved in 1.2
#define DevCap_test_write         0x04
#define DevCap_write_dvd_r        0x10
#define DevCap_write_dvd_ram      0x20

    UCHAR WriteCap;            // DevCap_*_write
/*    UCHAR cd_r_write        : 1; // reserved in 1.2
    UCHAR cd_rw_write        : 1; // reserved in 1.2
    UCHAR test_write        : 1;
    UCHAR reserved3a        : 1;
    UCHAR dvd_r_write       : 1;
    UCHAR dvd_ram_write     : 1;
    UCHAR Reserved3         : 2;*/

#define DevCap_audio_play          0x01
#define DevCap_composite          0x02
#define DevCap_digport1           0x04
#define DevCap_digport2           0x08
#define DevCap_mode2_form1        0x10
#define DevCap_mode2_form2        0x20
#define DevCap_multisession       0x40

    UCHAR Capabilities0;
/*    UCHAR audio_play        : 1;
    UCHAR composite         : 1;
    UCHAR digport1          : 1;
    UCHAR digport2          : 1;
    UCHAR mode2_form1       : 1;
    UCHAR mode2_form2       : 1;
    UCHAR multisession      : 1;
    UCHAR Reserved4         : 1;*/

#define DevCap_cdda               0x01
#define DevCap_cdda_accurate      0x02
#define DevCap_rw_supported       0x04
#define DevCap_rw_corr            0x08
#define DevCap_c2_pointers        0x10
#define DevCap_isrc               0x20
#define DevCap_upc                0x40
#define DevCap_read_bar_code      0x80

    UCHAR Capabilities1;
/*    UCHAR cdda              : 1;
    UCHAR cdda_accurate     : 1;
    UCHAR rw_supported      : 1;
    UCHAR rw_corr           : 1;
    UCHAR c2_pointers       : 1;
    UCHAR isrc              : 1;
    UCHAR upc               : 1;
    UCHAR Reserved5         : 1;*/

#define DevCap_lock               0x01
#define DevCap_lock_state         0x02
#define DevCap_prevent_jumper     0x04
#define DevCap_eject              0x08
#define DevCap_mechtype_mask      0xE0
#define DevCap_mechtype_caddy      0x00
#define DevCap_mechtype_tray      (0x01<<5)
#define DevCap_mechtype_popup      (0x02<<5)
#define DevCap_mechtype_individual_changer      (0x04<<5)
#define DevCap_mechtype_cartridge_changer      (0x05<<5)

    UCHAR Capabilities2;
/*    UCHAR lock              : 1;
    UCHAR lock_state        : 1;
    UCHAR prevent_jumper    : 1;
    UCHAR eject             : 1;
    UCHAR Reserved6         : 1;
    UCHAR mechtype        : 3;*/

#define DevCap_separate_volume    0x01
#define DevCap_separate_mute      0x02
#define DevCap_disc_present       0x04          // reserved in 1.2
#define DevCap_sw_slot_select     0x08          // reserved in 1.2
#define DevCap_change_side_cap    0x10
#define DevCap_rw_leadin_read     0x20

    UCHAR Capabilities3;
/*    UCHAR separate_volume   : 1;
    UCHAR separate_mute     : 1;
    UCHAR disc_present      : 1;  // reserved in 1.2
    UCHAR sss               : 1;  // reserved in 1.2
    UCHAR Reserved7         : 4;*/

    UCHAR MaximumSpeedSupported[2];
    UCHAR NumberVolumeLevels[2];
    UCHAR BufferSize[2];
    UCHAR CurrentSpeed[2];

    UCHAR Reserved8;

    UCHAR SpecialParameters0;
/*  UCHAR Reserved9        : 1;
    UCHAR BCK           : 1;
    UCHAR RCK           : 1;
    UCHAR LSBF          : 1;
    UCHAR Length        : 2;
    UCHAR Reserved10    : 2;*/

    UCHAR MaximumWriteSpeedSupported[2];
    UCHAR CurrentWriteSpeed[2];
    UCHAR CopyManagementRevision[2];
    UCHAR Reserved11[2];

// MMC3

    UCHAR Reserved12;

    UCHAR SpecialParameters1;
/*  UCHAR RCS           : 2; // rotation control selected
    UCHAR Reserved13    : 6; */

    UCHAR CurrentWriteSpeed3[2];
    UCHAR LunWPerfDescriptorCount[2];

//    LUN_WRITE_PERF_DESC  LunWPerfDescriptor[0];

} MODE_CAPABILITIES_PAGE2, *PMODE_CAPABILITIES_PAGE2;

typedef struct _LUN_WRITE_PERF_DESC {
    UCHAR Reserved;

#define LunWPerf_RotCtrl_Mask   0x07
#define LunWPerf_RotCtrl_CLV    0x00
#define LunWPerf_RotCtrl_CAV    0x01

    UCHAR RotationControl;
    UCHAR WriteSpeedSupported[2]; // kbps

} LUN_WRITE_PERF_DESC, *PLUN_WRITE_PERF_DESC;

// Mode parameter list block descriptor -
// set the block length for reading/writing

#define MODE_BLOCK_DESC_LENGTH               8
#define MODE_HEADER_LENGTH                   4
#define MODE_HEADER_LENGTH10                 8

#define CDB_USE_MSF                0x01

// Atapi 2.5 changer
typedef struct _MECHANICAL_STATUS_INFORMATION_HEADER {
    UCHAR CurrentSlot : 5;
    UCHAR ChangerState : 2;
    UCHAR Fault : 1;
    UCHAR Reserved : 5;
    UCHAR MechanismState : 3;
    UCHAR CurrentLogicalBlockAddress[3];
    UCHAR NumberAvailableSlots;
    UCHAR SlotTableLength[2];
} MECHANICAL_STATUS_INFORMATION_HEADER, *PMECHANICAL_STATUS_INFORMATION_HEADER;

typedef struct _SLOT_TABLE_INFORMATION {
    UCHAR DiscChanged : 1;
    UCHAR Reserved : 6;
    UCHAR DiscPresent : 1;
    UCHAR Reserved2[3];
} SLOT_TABLE_INFORMATION, *PSLOT_TABLE_INFORMATION;

typedef struct _MECHANICAL_STATUS {
    MECHANICAL_STATUS_INFORMATION_HEADER MechanicalStatusHeader;
    SLOT_TABLE_INFORMATION SlotTableInfo[1];
} MECHANICAL_STATUS, *PMECHANICAL_STATUS;

// DVD structure blocks

typedef struct _DVD_DESCRIPTOR_HEADER {
    UCHAR Length[2];
    UCHAR Reserved[2];
} DVD_DESCRIPTOR_HEADER, *PDVD_DESCRIPTOR_HEADER;

typedef struct _DVD_LAYER_DESCRIPTOR {
    DVD_DESCRIPTOR_HEADER Header;
    UCHAR Length[2];
    UCHAR BookVersion : 4;
    UCHAR BookType : 4;
    UCHAR MinimumRate : 4;
    UCHAR DiskSize : 4;
    UCHAR LayerType : 4;
    UCHAR TrackPath : 1;
    UCHAR NumberOfLayers : 2;
    UCHAR Reserved1 : 1;
    UCHAR TrackDensity : 4;
    UCHAR LinearDensity : 4;
    UCHAR StartingDataSector[4];
    UCHAR EndDataSector[4];
    UCHAR EndLayerZeroSector[4];
    UCHAR Reserved5 : 7;
    UCHAR BCAFlag : 1;
    UCHAR Reserved6;
} DVD_LAYER_DESCRIPTOR, *PDVD_LAYER_DESCRIPTOR;

typedef struct _DVD_COPYRIGHT_INFORMATION {
    UCHAR CopyrightProtectionSystemType;
    UCHAR RegionManagementInformation;
    UCHAR Reserved[2];
} DVD_COPYRIGHT_INFORMATION, *PDVD_COPYRIGHT_INFORMATION;

typedef struct _DVD_DISK_KEY_STRUCTURES {
    UCHAR DiskKeyData[2048];
} DVD_DISK_KEY_STRUCTURES, *PDVD_DISK_KEY_STRUCTURES;

typedef struct _CDVD_KEY_HEADER {
    UCHAR DataLength[2];
    UCHAR Reserved[2];
} CDVD_KEY_HEADER, *PCDVD_KEY_HEADER;

typedef struct _CDVD_REPORT_AGID_DATA {
    CDVD_KEY_HEADER Header;
    UCHAR Reserved1[3];
    UCHAR Reserved2 : 6;
    UCHAR AGID : 2;
} CDVD_REPORT_AGID_DATA, *PCDVD_REPORT_AGID_DATA;

typedef struct _CDVD_CHALLENGE_KEY_DATA {
    CDVD_KEY_HEADER Header;
    UCHAR ChallengeKeyValue[10];
    UCHAR Reserved[2];
} CDVD_CHALLENGE_KEY_DATA, *PCDVD_CHALLENGE_KEY_DATA;

typedef struct _CDVD_KEY_DATA {
    CDVD_KEY_HEADER Header;
    UCHAR Key[5];
    UCHAR Reserved[3];
} CDVD_KEY_DATA, *PCDVD_KEY_DATA;

typedef struct _CDVD_REPORT_ASF_DATA {
    CDVD_KEY_HEADER Header;
    UCHAR Reserved1[3];
    UCHAR Success : 1;
    UCHAR Reserved2 : 7;
} CDVD_REPORT_ASF_DATA, *PCDVD_REPORT_ASF_DATA;

typedef struct _CDVD_TITLE_KEY_HEADER {
    CDVD_KEY_HEADER Header;
    UCHAR DataLength[2];
    UCHAR Reserved1[1];
    UCHAR Reserved2 : 3;
    UCHAR CGMS : 2;
    UCHAR CP_SEC : 1;
    UCHAR CPM : 1;
    UCHAR Zero : 1;
    CDVD_KEY_DATA TitleKey;
} CDVD_TITLE_KEY_HEADER, *PCDVD_TITLE_KEY_HEADER;

typedef struct _DVD_COPYRIGHT_DESCRIPTOR {
    UCHAR CopyrightProtectionType;
    UCHAR RegionManagementInformation;
    UCHAR Reserved[2];
} DVD_COPYRIGHT_DESCRIPTOR, *PDVD_COPYRIGHT_DESCRIPTOR;

typedef struct _DVD_RPC_KEY {
    UCHAR UserResetsAvailable:3;
    UCHAR ManufacturerResetsAvailable:3;
    UCHAR TypeCode:2;
    UCHAR RegionMask;
    UCHAR RpcScheme;
    UCHAR Reserved2[1];
} DVD_RPC_KEY, * PDVD_RPC_KEY;

#pragma pack(pop)

#endif //__CDRW_DEVICE_H__
