#ifndef __SMB_NEGO_H__
#define __SMB_NEGO_H__

/* smb types */
typedef union _SMB_ERROR
{
   // can be NTSTATUS or DOS 16bit error /error class
   NTSTATUS NT_Status;
} SMB_ERROR, *PSMB_ERROR;
#include "pshpack1.h"
typedef struct _SMB_Header
{
    UCHAR Protocol[4];
    UCHAR Command;
    SMB_ERROR Status;
    UCHAR Flags;
    USHORT Flags2;
    USHORT PIDHigh;
    UCHAR SecurityFeatures[8];
    USHORT Reserved;
    USHORT TID;
    USHORT PIDLow;
    USHORT UID;
    USHORT MID;
} SMB_Header, *PSMB_Header;
typedef struct _SMB_Parameters
{
    UCHAR WordCount;
    /*USHORT DialectIndex;
    UCHAR SecurityMode;
    USHORT MaxMpxCount;
    USHORT MaxNumberVcs;
    ULONG MaxBufferSize;
    ULONG MaxRawSize;
    ULONG SessionKey;
    ULONG Capabilities;
    FILETIME SystemTime;
    SHORT ServerTimeZone;
    UCHAR ChallengeLength;*/
} SMB_Parameters, *PSMB_Parameters;
typedef struct _SMB_Data
{
    USHORT ByteCount;
    //union _Bytes
    //{
        //GUID ServerGUID;
    //    UCHAR SecurityBlob;
    UCHAR Data;
    //} Bytes;
} SMB_Data, *PSMB_Data;
#include "backpack.h"

/* from smb - lib/util/asn1.h: */
#define ASN1_APPLICATION(x) ((x)+0x60)
#define ASN1_OID 0x6
#define ASN1_CONTEXT(x) ((x)+0xa0)
#define ASN1_SEQUENCE(x) ((x)+0x30)

#define SMB_FLAGS2_EXTENDED_SECURITY 0x0800;
#define SMB_FLAGS2_NT_STATUS         0x4000;

BOOL
smb_GenComNegoMsg(
    OUT PBYTE pOutBuf,
    IN OUT PULONG pBufLen);
BOOL
smb_GenComSessionSetupMsg(
    OUT PBYTE pOutBuf,
    IN OUT PULONG pBufLen,
    IN PBYTE ntlmMsg,
    IN ULONG ntlmMsgLen,
    IN USHORT smbSessionId,
    IN USHORT smbRequestCounter);
BOOL
smb_GetNTMLMsg(
    IN PBYTE pSMBBuf,
    IN ULONG smbBufLen,
    OUT PBYTE* pNegoBuf,
    OUT PULONG pNegoBufLen);

#endif
