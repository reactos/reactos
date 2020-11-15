/*
 * scsiwmi.h
 *
 * SCSI WMILIB interface.
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

#pragma once

#ifndef _SCSIWMI_
#define _SCSIWMI_

#include "srb.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push,4)

typedef struct _SCSIWMI_REQUEST_CONTEXT {
  PVOID UserContext;
  ULONG BufferSize;
  PUCHAR Buffer;
  UCHAR MinorFunction;
  UCHAR ReturnStatus;
  ULONG ReturnSize;
} SCSIWMI_REQUEST_CONTEXT, *PSCSIWMI_REQUEST_CONTEXT;

#ifdef _GUID_DEFINED
# warning _GUID_DEFINED is deprecated, use GUID_DEFINED instead
#endif

#if ! (defined _GUID_DEFINED || defined GUID_DEFINED)
#define GUID_DEFINED
typedef struct _GUID {
  unsigned long  Data1;
  unsigned short Data2;
  unsigned short Data3;
  unsigned char Data4[8];
} GUID;
#endif

typedef struct _SCSIWMIGUIDREGINFO {
  LPCGUID Guid;
  ULONG InstanceCount;
  ULONG Flags;
} SCSIWMIGUIDREGINFO, *PSCSIWMIGUIDREGINFO;

typedef
_Must_inspect_result_
UCHAR
(NTAPI *PSCSIWMI_QUERY_REGINFO)(
  _In_ PVOID DeviceContext,
  _In_ PSCSIWMI_REQUEST_CONTEXT RequestContext,
  _Out_ PWCHAR *MofResourceName);

typedef
_Must_inspect_result_
BOOLEAN
(NTAPI *PSCSIWMI_QUERY_DATABLOCK)(
  _In_ PVOID Context,
  _In_ PSCSIWMI_REQUEST_CONTEXT DispatchContext,
  _In_ ULONG GuidIndex,
  _In_ ULONG InstanceIndex,
  _In_ ULONG InstanceCount,
  _Inout_ PULONG InstanceLengthArray,
  _In_ ULONG BufferAvail,
  _Out_writes_bytes_(BufferAvail) PUCHAR Buffer);

typedef
_Must_inspect_result_
BOOLEAN
(NTAPI *PSCSIWMI_SET_DATABLOCK)(
  _In_ PVOID DeviceContext,
  _In_ PSCSIWMI_REQUEST_CONTEXT RequestContext,
  _In_ ULONG GuidIndex,
  _In_ ULONG InstanceIndex,
  _In_ ULONG BufferSize,
  _In_reads_bytes_(BufferSize) PUCHAR Buffer);

typedef
_Must_inspect_result_
BOOLEAN
(NTAPI *PSCSIWMI_SET_DATAITEM)(
  _In_ PVOID DeviceContext,
  _In_ PSCSIWMI_REQUEST_CONTEXT RequestContext,
  _In_ ULONG GuidIndex,
  _In_ ULONG InstanceIndex,
  _In_ ULONG DataItemId,
  _In_ ULONG BufferSize,
  _In_reads_bytes_(BufferSize) PUCHAR Buffer);

typedef
_Must_inspect_result_
BOOLEAN
(NTAPI *PSCSIWMI_EXECUTE_METHOD)(
  _In_ PVOID DeviceContext,
  _In_ PSCSIWMI_REQUEST_CONTEXT RequestContext,
  _In_ ULONG GuidIndex,
  _In_ ULONG InstanceIndex,
  _In_ ULONG MethodId,
  _In_ ULONG InBufferSize,
  _In_ ULONG OutBufferSize,
  _Inout_updates_bytes_to_(InBufferSize, OutBufferSize) PUCHAR Buffer);

typedef enum _SCSIWMI_ENABLE_DISABLE_CONTROL {
  ScsiWmiEventControl,
  ScsiWmiDataBlockControl
} SCSIWMI_ENABLE_DISABLE_CONTROL;

typedef
_Must_inspect_result_
BOOLEAN
(NTAPI *PSCSIWMI_FUNCTION_CONTROL)(
  _In_ PVOID DeviceContext,
  _In_ PSCSIWMI_REQUEST_CONTEXT RequestContext,
  _In_ ULONG GuidIndex,
  _In_ SCSIWMI_ENABLE_DISABLE_CONTROL Function,
  _In_ BOOLEAN Enable);

typedef struct _SCSIWMILIB_CONTEXT {
  ULONG GuidCount;
  PSCSIWMIGUIDREGINFO GuidList;
  PSCSIWMI_QUERY_REGINFO QueryWmiRegInfo;
  PSCSIWMI_QUERY_DATABLOCK QueryWmiDataBlock;
  PSCSIWMI_SET_DATABLOCK SetWmiDataBlock;
  PSCSIWMI_SET_DATAITEM SetWmiDataItem;
  PSCSIWMI_EXECUTE_METHOD ExecuteWmiMethod;
  PSCSIWMI_FUNCTION_CONTROL WmiFunctionControl;
} SCSI_WMILIB_CONTEXT, *PSCSI_WMILIB_CONTEXT;

_Must_inspect_result_
SCSIPORT_API
BOOLEAN
NTAPI
ScsiPortWmiDispatchFunction(
  _In_ PSCSI_WMILIB_CONTEXT WmiLibInfo,
  _In_ UCHAR MinorFunction,
  _In_ PVOID DeviceContext,
  _In_ PSCSIWMI_REQUEST_CONTEXT RequestContext,
  _In_ PVOID DataPath,
  _In_ ULONG BufferSize,
  _In_ PVOID Buffer);

#define ScsiPortWmiFireAdapterEvent(    \
  HwDeviceExtension,                    \
  Guid,                                 \
  InstanceIndex,                        \
  EventDataSize,                        \
  EventData)                            \
    ScsiPortWmiFireLogicalUnitEvent(    \
      HwDeviceExtension,                \
      0xff,                             \
      0,                                \
      0,                                \
      Guid,                             \
      InstanceIndex,                    \
      EventDataSize,                    \
      EventData)

/*
 * ULONG
 * ScsiPortWmiGetReturnSize(
 *   PSCSIWMI_REQUEST_CONTEXT RequestContext);
 */
#define ScsiPortWmiGetReturnSize(RequestContext) \
  ((RequestContext)->ReturnSize)

/* UCHAR
 * ScsiPortWmiGetReturnStatus(
 *   PSCSIWMI_REQUEST_CONTEXT RequestContext);
 */
#define ScsiPortWmiGetReturnStatus(RequestContext) \
  ((RequestContext)->ReturnStatus)

SCSIPORT_API
VOID
NTAPI
ScsiPortWmiPostProcess(
  _Inout_ PSCSIWMI_REQUEST_CONTEXT RequestContext,
  _In_ UCHAR SrbStatus,
  _In_ ULONG BufferUsed);

SCSIPORT_API
VOID
NTAPI
ScsiPortWmiFireLogicalUnitEvent(
  _In_ PVOID HwDeviceExtension,
  _In_ UCHAR PathId,
  _In_ UCHAR TargetId,
  _In_ UCHAR Lun,
  _In_ LPGUID Guid,
  _In_ ULONG InstanceIndex,
  _In_ ULONG EventDataSize,
  _In_ PVOID EventData);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* _SCSIWMI_ */
