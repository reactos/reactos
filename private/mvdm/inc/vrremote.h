/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrremote.h

Abstract:

    Prototypes for vrremote module

Author:

    Richard L Firth (rfirth) 28-Oct-1991

Revision History:

    29-Oct-1991 rfirth
        Created

--*/

NET_API_STATUS
VrTransaction(
    IN      LPSTR   ServerName,
    IN      LPBYTE  SendParmBuffer,
    IN      DWORD   SendParmBufLen,
    IN      LPBYTE  SendDataBuffer,
    IN      DWORD   SendDataBufLen,
    OUT     LPBYTE  ReceiveParmBuffer,
    IN      DWORD   ReceiveParmBufLen,
    IN      LPBYTE  ReceiveDataBuffer,
    IN OUT  LPDWORD ReceiveDataBufLen,
    IN      BOOL    NullSessionFlag
    );

NET_API_STATUS
VrRemoteApi(
    IN  DWORD   ApiNumber,
    IN  LPBYTE  ServerNamePointer,
    IN  LPSTR   ParameterDescriptor,
    IN  LPSTR   DataDescriptor,
    IN  LPSTR   AuxDescriptor OPTIONAL,
    IN  BOOL    NullSessionFlag
    );

//
// private routine prototypes
//

DWORD
VrpGetStructureSize(
    IN  LPSTR   Descriptor,
    IN  LPDWORD AuxOffset
    );

DWORD
VrpGetArrayLength(
    IN  LPSTR   type_ptr,
    IN  LPSTR*  type_ptr_addr
    );

DWORD
VrpGetFieldSize(
    IN  LPSTR   Descriptor,
    IN  LPSTR*  pDescriptor
    );

VOID
VrpConvertReceiveBuffer(
    IN  LPBYTE  ReceiveBuffer,
    IN  WORD    BufferSelector,
    IN  WORD    BufferOffset,
    IN  WORD    ConverterWord,
    IN  DWORD   NumberStructs,
    IN  LPSTR   DataDescriptor,
    IN  LPSTR   AuxDescriptor
    );

VOID
VrpConvertVdmPointer(
    IN  ULPWORD TargetPointer,
    IN  WORD    BufferSegment,
    IN  WORD    BufferOffset,
    IN  WORD    ConverterWord
    );

NET_API_STATUS
VrpPackSendBuffer(
    IN OUT  LPBYTE* SendBufferPtr,
    IN OUT  LPDWORD SendBufLenPtr,
    OUT     LPBOOL  BufferAllocFlagPtr,
    IN OUT  LPSTR   DataDescriptor,
    IN      LPSTR   AuxDescriptor,
    IN      DWORD   StructureSize,
    IN      DWORD   AuxOffset,
    IN      DWORD   AuxSize,
    IN      BOOL    SetInfoFlag,
    IN      BOOL    OkToModifyDescriptor
    );
