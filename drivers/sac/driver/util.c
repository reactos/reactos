/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/util.c
 * PURPOSE:		 Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

ULONG
ConvertAnsiToUnicode(
	IN PWCHAR pwch,
	IN PCHAR pch,
	IN ULONG length
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
IsCmdEventRegistrationProcess(
	IN PFILE_OBJECT FileObject
	)
{
	return FALSE;
}

VOID
InitializeCmdEventInfo(
	VOID
	)
{

}

BOOLEAN
VerifyEventWaitable(
	IN PVOID Object,
	OUT PVOID *WaitObject,
	OUT PVOID *ActualWaitObject
	)
{
	return FALSE;
}

NTSTATUS
InvokeUserModeService(
	VOID
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

VOID
SacFormatMessage(
	IN PWCHAR FormattedString,
	IN PWCHAR MessageString,
	IN ULONG MessageSize
	)
{

}

NTSTATUS
TearDownGlobalMessageTable(
	VOID
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

PWCHAR
GetMessage(
	IN ULONG MessageIndex
	)
{
	return NULL;
}

BOOLEAN
SacTranslateUtf8ToUnicode(
	IN CHAR Utf8Char,
	IN PCHAR UnicodeBuffer, 
	OUT PCHAR Utf8Value
	)
{
	return FALSE;
}

BOOLEAN
SacTranslateUnicodeToUtf8(
	IN PWCHAR SourceBuffer,
	IN ULONG SourceBufferLength,
	OUT PCHAR DestinationBuffer,
	IN ULONG DestinationBufferSize,
	IN ULONG UTF8Count,
	OUT PULONG ProcessedCount
	)
{
	return FALSE;
}

NTSTATUS
GetRegistryValueBuffer(
	IN PCWSTR KeyName,
	IN PWCHAR ValueName,
	IN PKEY_VALUE_PARTIAL_INFORMATION ValueBuffer
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SetRegistryValue(
	IN PCWSTR KeyName,
	IN PWCHAR ValueName,
	IN ULONG Type,
	IN PVOID Data,
	IN ULONG DataSize
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CopyRegistryValueData(
	IN PVOID Dest,
	IN PKEY_VALUE_PARTIAL_INFORMATION ValueBuffer
	)
{
   return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
TranslateMachineInformationText(
	IN PWCHAR Buffer)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
TranslateMachineInformationXML(
	IN PWCHAR Buffer,
	IN PWCHAR ExtraData
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RegisterBlueScreenMachineInformation(
	VOID
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

VOID
FreeMachineInformation(
	VOID
	)
{

}

NTSTATUS
SerialBufferGetChar(
	OUT PCHAR Char
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
GetCommandConsoleLaunchingPermission(
	OUT PBOOLEAN Permission
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ImposeSacCmdServiceStartTypePolicy(
	VOID
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CopyAndInsertStringAtInterval(
	IN PWCHAR SourceStr,
	IN ULONG Interval,
	IN PWCHAR InsertStr,
	OUT PWCHAR pDestStr
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

ULONG
GetMessageLineCount(
	IN ULONG MessageIndex
	)
{
	return 0;
}

NTSTATUS
RegisterSacCmdEvent(
	IN PVOID Object,
	IN PKEVENT SetupCmdEvent[]
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
UnregisterSacCmdEvent(
	IN PFILE_OBJECT FileObject
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
UTF8EncodeAndSend(
	IN PWCHAR String
	)
{
	return STATUS_NOT_IMPLEMENTED;
}
