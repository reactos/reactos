/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - common functions
 * COPYRIGHT:   Copyright 2019 mrmks04 (mrmks04@yandex.ru)
 */


#include "common.h"
#include "ntddk_ex.h"

#include <ntintsafe.h>
#include <ntstrsafe.h>


NTSTATUS
NTAPI
AuxKlibQueryModuleInformation(
	IN PULONG InformationLenght,
	IN ULONG SizePerModule,
	IN OUT PRTL_MODULE_EXTENDED_INFO ModuleInfo
);


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, AuxKlibQueryModuleInformation)
#endif


VOID
NTAPI
GetNameFromPath(
	IN PUNICODE_STRING Path,
	OUT PUNICODE_STRING Name
)
{
	PWCHAR nextSym;
	PWCHAR currSym;

	if (Path->Length >= 2) 
	{
		Name->Buffer = (wchar_t*)((char*)Path->Buffer + Path->Length - 2);
		Name->Length = 2;

		for (nextSym = Name->Buffer; ; Name->Buffer = nextSym) 
		{
			if (nextSym < Path->Buffer) 
			{
				Name->Length -= 2;
				++Name->Buffer;
				goto end;
			}
			currSym = Name->Buffer;

			if (*currSym == '\\')
			{
				break;
			}
			nextSym = currSym - 1;
			Name->Length += 2;
		}

		++Name->Buffer;
		Name->Length -= 2;

		if (Name->Length == 2) 
		{
			Name->Buffer = NULL;
			Name->Length = 0;
		}
	end:
		Name->MaximumLength = Name->Length;
	}
	else 
	{
		Name->Length = 0;
		Name->Buffer = NULL;
	}
}


NTSTATUS
NTAPI
GetImageName(
	IN PUNICODE_STRING DriverServiceName,
	IN ULONG Tag,
	IN PUNICODE_STRING ImageName
)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING name;
	UNICODE_STRING path;
	PKEY_VALUE_PARTIAL_INFORMATION pKeyValPartial;
	HANDLE KeyHandle;
	UNICODE_STRING ValueName;

	ImageName->Length = 0;
	ImageName->Buffer = NULL;
	name.Length = 0;
	name.Buffer = NULL;
	KeyHandle = NULL;
	pKeyValPartial = NULL;
	RtlInitUnicodeString(&ValueName, L"ImagePath");

	ObjectAttributes.ObjectName = DriverServiceName;
	ObjectAttributes.Length = 24;
	ObjectAttributes.RootDirectory = 0;
	ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;//576; OBJ_OPENIF;
	ObjectAttributes.SecurityDescriptor = 0;
	ObjectAttributes.SecurityQualityOfService = 0;

	status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);

	if (!NT_SUCCESS(status)) 
	{
		goto end;
	}

	status = FxLdrQueryData(KeyHandle, &ValueName, '1LxF', &pKeyValPartial);
	if (!NT_SUCCESS(status)) 
	{
		goto end;
	}

	if (pKeyValPartial->Type != REG_SZ &&
		pKeyValPartial->Type != REG_EXPAND_SZ) 
	{
		status = STATUS_OBJECT_TYPE_MISMATCH;
		goto error;
	}

	if (pKeyValPartial->DataLength == 0 ||
		pKeyValPartial->DataLength > 0xFFFF) 
	{
		status = STATUS_INVALID_PARAMETER;	
		goto error;
	}

	path.Buffer = (PWCH)pKeyValPartial->Data;
	path.Length = (USHORT)pKeyValPartial->DataLength;
	path.MaximumLength = (USHORT)pKeyValPartial->DataLength;

	if (pKeyValPartial->DataLength >= 2u &&
		!*(((WCHAR*)& pKeyValPartial->Data) + pKeyValPartial->DataLength / 2)) 
	{
		path.Length = (USHORT)pKeyValPartial->DataLength - 2;
	}

	GetNameFromPath(&path, &name);

	if (name.Length > 0) 
	{
		status = RtlUShortAdd(name.Length, 2u, &ImageName->Length);
		if (!NT_SUCCESS(status)) 
		{
			status = STATUS_INTEGER_OVERFLOW;
			if (WdfLdrDiags) {
				DbgPrint("WdfLdr: GetImageName - ");
				DbgPrint("ERROR: size computation failed with Status 0x%x\n", STATUS_INTEGER_OVERFLOW);
			}
		}
		else 
		{
			ImageName->Buffer = ExAllocatePoolWithTag(PagedPool, ImageName->Length, Tag);

			if (ImageName->Buffer != NULL) 
			{
				memset(ImageName->Buffer, 0, ImageName->Length);
				ImageName->MaximumLength = ImageName->Length;
				ImageName->Length = 0;
				RtlCopyUnicodeString(ImageName, &name);

				if (WdfLdrDiags) 
				{
					DbgPrint("WdfLdr: GetImageName - ");
					DbgPrint("Version Image Name \"%wZ\"\n", ImageName);
				}
			}
			else 
			{
				status = STATUS_INSUFFICIENT_RESOURCES;

				if (WdfLdrDiags) 
				{
					DbgPrint("WdfLdr: GetImageName - ");
					DbgPrint("ERROR: ExAllocatePoolWithTag failed with Status 0x%x\n", STATUS_INSUFFICIENT_RESOURCES);
				}
			}
		}
	}
	else 
	{
		status = STATUS_INVALID_PARAMETER;

		if (WdfLdrDiags) 
		{
			DbgPrint("WdfLdr: GetImageName - ");
			DbgPrint("ERROR: GetNameFromPathW could not find a name, status 0x%x\n", STATUS_INVALID_PARAMETER);
		}
	}

	goto end;

error:
	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: GetImageName - ");
		DbgPrint("ERROR: GetImageName failed with status 0x%x\n", status);
	}

end:
	if (KeyHandle) 
	{
		ZwClose(KeyHandle);
	}
	if (pKeyValPartial) 
	{
		ExFreePoolWithTag(pKeyValPartial, 0);
	}

	return status;
}


PCHAR
NTAPI
GetFileName(
	IN PCHAR Path
)
{
	size_t length;
	PCHAR currentSym;

	if (Path == NULL)
		return NULL;

	length = strlen(Path);
	if (length == 0)
		return NULL;

	for (currentSym = &Path[length]; currentSym >= Path; --currentSym) 
	{
		if (*currentSym == '\\')
			return currentSym + 1;
	}

	return Path;
}


NTSTATUS
NTAPI
AuxKlibQueryModuleInformation(
	IN PULONG InformationLenght,
	IN ULONG SizePerModule,
	IN OUT PRTL_MODULE_EXTENDED_INFO ModuleInfo
)
{
	NTSTATUS status;
	PRTL_PROCESS_MODULES pSysInfo;
	ULONG sysInfoLen;
	ULONG modulesSize;
	ULONG ResultLength;
	PULONG pInfoLength;
	ULONG index;
	RTL_PROCESS_MODULES systemInformation;

	PAGED_CODE();

	pInfoLength = InformationLenght;
	if (gKlibInitialized != 1) 
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (pfnRtlQueryModuleInformation != NULL)
	{
		return pfnRtlQueryModuleInformation(InformationLenght, SizePerModule, ModuleInfo);
	}

	status = STATUS_SUCCESS;
	if (SizePerModule != 4 && SizePerModule != 0x10C) 
	{
		return STATUS_INVALID_PARAMETER_2;
	}

	if ((SIZE_T)ModuleInfo & 3)
	{
		return STATUS_INVALID_PARAMETER_3;
	}

	pSysInfo = &systemInformation;
	for (sysInfoLen = 0x120; ; sysInfoLen = ResultLength) 
	{
		status = ZwQuerySystemInformation(SystemModuleInformation, pSysInfo, sysInfoLen, &ResultLength);
		if (NT_SUCCESS(status))
			break;

		if (status != STATUS_INFO_LENGTH_MISMATCH)
			goto clean;

		if (pSysInfo != &systemInformation)
			ExFreePoolWithTag(pSysInfo, 0);

		pSysInfo = ExAllocatePoolWithQuotaTag(PagedPool, ResultLength, WDFLDR_TAG);
		
		if (pSysInfo == NULL)
			return STATUS_INSUFFICIENT_RESOURCES;
	}

	if (SizePerModule * pSysInfo->NumberOfModules > 0xFFFFFFFF) 
	{
		status = STATUS_INTEGER_OVERFLOW;
		goto clean;
	}

	modulesSize = SizePerModule * pSysInfo->NumberOfModules;

	if (ModuleInfo == NULL)
	{
		goto clean;
	}

	if (*pInfoLength < modulesSize) 
	{
		status = STATUS_BUFFER_TOO_SMALL;
		goto end;
	}

	if (pSysInfo->NumberOfModules == 0)
		goto end;

	for (index = 0; index < pSysInfo->NumberOfModules; ++index, ModuleInfo++) 
	{
		ModuleInfo->BasicInfo.ImageBase = pSysInfo->Modules[index].ImageBase;
		ModuleInfo->ImageSize = pSysInfo->Modules[index].ImageSize;
		ModuleInfo->FileNameOffset = pSysInfo->Modules[index].OffsetToFileName;
		memcpy(ModuleInfo->FullPathName, pSysInfo->Modules[index].FullPathName, 0x100);
	}

end:
	*pInfoLength = modulesSize;
clean:
	if (pSysInfo != &systemInformation)
		ExFreePoolWithTag(pSysInfo, 0);

	return status;
}


NTSTATUS
NTAPI
GetImageBase(
	IN PCUNICODE_STRING ImageName,
	OUT PVOID* ImageBase,
	OUT PULONG ImageSize
)
{
	PCHAR fileName;
	PRTL_MODULE_EXTENDED_INFO pModuleInfo;
	PVOID infoBuffer;
	ULONG_PTR endOfArray;
	STRING ansiImageName;
	size_t fileNameLength;
	ULONG informationLength;
	ULONG totalSize;
	ULONG numberOfBytes;
	NTSTATUS status;

	infoBuffer = NULL;
	*ImageBase = 0;
	*ImageSize = 0;
	ansiImageName.Length = 0;
	ansiImageName.MaximumLength = 0;
	ansiImageName.Buffer = 0;
	informationLength = 0;
	pModuleInfo = NULL;
	status = RtlUnicodeStringToAnsiString(&ansiImageName, ImageName, TRUE);

	if (NT_SUCCESS(status) && ansiImageName.Buffer) 
	{
		ansiImageName.Buffer[ansiImageName.Length] = 0;
		fileName = GetFileName(ansiImageName.Buffer);
		fileNameLength = strlen(fileName);

		if (fileName == NULL ||	fileNameLength == 0)
		{
			status = STATUS_OBJECT_NAME_NOT_FOUND;
			goto clean;
		}
		totalSize = 0;

		for(;;)
		{
			numberOfBytes = 0;
			status = AuxKlibQueryModuleInformation(&numberOfBytes, sizeof(RTL_MODULE_EXTENDED_INFO), 0);

			if (!NT_SUCCESS(status) || !numberOfBytes)
			{
				if (!WdfLdrDiags)
					goto clean;
				break;
			}
			status = RtlULongAdd(numberOfBytes, totalSize, &informationLength);

			if (!NT_SUCCESS(status)) 
			{
				if (WdfLdrDiags) 
				{
					DbgPrint("WdfLdr: GetImageBase - ");
					DbgPrint("ERROR: RtlUlongAdd failed with Status 0x%x\n", status);
				}
			}
			else 
			{
				numberOfBytes = informationLength;
			}

			infoBuffer = ExAllocatePoolWithTag(PagedPool, numberOfBytes, WDFLDR_TAG);
			pModuleInfo = infoBuffer;

			if (pModuleInfo == NULL)
			{
				status = STATUS_INSUFFICIENT_RESOURCES;

				if (WdfLdrDiags) 
				{
					DbgPrint("WdfLdr: GetImageBase - ");
					DbgPrint("ERROR: ExAllocatePoolWithTag failed with Status 0x%x\n", status);
				}
				goto end;
			}

			memset(pModuleInfo, 0, numberOfBytes);
			informationLength = numberOfBytes;
			status = AuxKlibQueryModuleInformation(&informationLength, sizeof(RTL_MODULE_EXTENDED_INFO), pModuleInfo);

			if (status != STATUS_BUFFER_TOO_SMALL)
				break;

			ExFreePoolWithTag(pModuleInfo, 0);
			totalSize += sizeof(RTL_MODULE_EXTENDED_INFO);
			pModuleInfo = NULL;

			if (totalSize >= sizeof(RTL_MODULE_EXTENDED_INFO) * 10)
				goto clean;
		}

		if (!NT_SUCCESS(status))
		{
			if (!WdfLdrDiags)
				goto end;

			DbgPrint("WdfLdr: GetImageBase - ");
			DbgPrint("ERROR: AuxKlibQueryModuleInformation failed with Status 0x%x\n", status);
			goto end;
		}

		endOfArray = (ULONG_PTR)pModuleInfo + informationLength;
		numberOfBytes = informationLength;

		for(;;)
		{
			if (pModuleInfo->FileNameOffset < 0x100 &&
				strncmp(pModuleInfo->FullPathName, fileName, fileNameLength) == 0)
				break;

			pModuleInfo++;
			if ((ULONG_PTR)pModuleInfo >= endOfArray)
				goto end;
		}
		*ImageBase = pModuleInfo->BasicInfo.ImageBase;
		*ImageSize = pModuleInfo->ImageSize;
	}
	else 
	{
		if (WdfLdrDiags) 
		{
			DbgPrint("WdfLdr: GetImageBase - ");
			DbgPrint("ERROR: RtlUnicodeStringToAnsiString failed with Status 0x%x\n", status);
		}
		ansiImageName.Buffer = NULL;
	}
end:
	if (infoBuffer != NULL)
		ExFreePoolWithTag(infoBuffer, 0);
clean:
	if (ansiImageName.Buffer)
		RtlFreeAnsiString(&ansiImageName);

	return status;
}


BOOLEAN
NTAPI
ServiceCheckBootStart(
	IN PUNICODE_STRING Service
)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE KeyHandle;
	BOOLEAN result;
	ULONG value;
	UNICODE_STRING ValueName;

	KeyHandle = NULL;
	result = FALSE;
	ObjectAttributes.Length = 24;
	ObjectAttributes.RootDirectory = 0;
	ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;//576;
	ObjectAttributes.ObjectName = Service;
	ObjectAttributes.SecurityDescriptor = 0;
	ObjectAttributes.SecurityQualityOfService = 0;
	status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
	RtlInitUnicodeString(&ValueName, L"Start");

	if (status != STATUS_OBJECT_NAME_NOT_FOUND) 
	{
		if (NT_SUCCESS(status)) 
		{
			status = FxLdrQueryUlong(KeyHandle, &ValueName, &value);
			if (NT_SUCCESS(status))
			{
				result = value == 0;
			}
		}
		else if (WdfLdrDiags) 
		{
			DbgPrint("WdfLdr: ServiceCheckBootStart - ");
			DbgPrint("WdfLdr: ZwOpenKey(%wZ) failed: %08X\n", Service, status);
		}
	}

	if (KeyHandle)
		ZwClose(KeyHandle);

	return result;
}


NTSTATUS
NTAPI
FxLdrQueryUlong(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	OUT PULONG Value
)
{
	NTSTATUS status;
	ULONG resultLength;
	KEY_VALUE_PARTIAL_INFORMATION keyValue;

	keyValue.DataLength = 0;
	keyValue.TitleIndex = 0;
	keyValue.Type = 0;
	keyValue.Data[0] = 0;
	status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation, &keyValue, sizeof(keyValue), &resultLength);

	if (NT_SUCCESS(status)) 
	{
		if (keyValue.Type != REG_DWORD || keyValue.DataLength != 4) 
		{
			status = STATUS_INVALID_BUFFER_SIZE;
		}
		else 
		{
			*Value = keyValue.Data[0];
			status = STATUS_SUCCESS;
		}
	}
	else 
	{
		if (WdfLdrDiags) 
		{
			DbgPrint("WdfLdr: FxLdrQueryUlong - ");
			DbgPrint("ERROR: ZwQueryValueKey failed with Status 0x%x\n", status);
		}
	}

	return status;
}


NTSTATUS
NTAPI
FxLdrQueryData(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN ULONG Tag,
	OUT PKEY_VALUE_PARTIAL_INFORMATION* KeyValPartialInfo
)
{
	PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;
	NTSTATUS status;
	ULONG resultLength;

	*KeyValPartialInfo = NULL;
	for (;;)
	{
		status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation, NULL, 0, &resultLength);
		if (status != STATUS_BUFFER_TOO_SMALL) 
		{
			if (!NT_SUCCESS(status) && WdfLdrDiags) 
			{
				DbgPrint("WdfLdr: FxLdrQueryData - ");
				DbgPrint("ERROR: ZwQueryValueKey failed with status 0x%x\n", status);
			}

			return status;
		}

		status = RtlULongAdd(resultLength, 0xCu, &resultLength);
		if (!NT_SUCCESS(status)) 
		{
			if (WdfLdrDiags) 
			{
				DbgPrint("WdfLdr: FxLdrQueryData - ");
				DbgPrint("ERROR: Computing length of data under %wZ failed with status 0x%x\n", ValueName, status);
			}

			return status;
		}

		pKeyInfo = ExAllocatePoolWithTag(PagedPool, resultLength, Tag);

		if (pKeyInfo == NULL)
		{
			break;
		}

		memset(pKeyInfo, 0, resultLength);
		status = ZwQueryValueKey(
			KeyHandle,
			ValueName,
			KeyValuePartialInformation,
			pKeyInfo,
			resultLength,
			&resultLength);

		if (NT_SUCCESS(status)) 
		{
			*KeyValPartialInfo = pKeyInfo;
			return status;
		}

		ExFreePoolWithTag(pKeyInfo, 0);

		if (status != STATUS_BUFFER_TOO_SMALL) 
		{
			if (WdfLdrDiags) {
				DbgPrint("WdfLdr: FxLdrQueryData - ");
				DbgPrint("ERROR: ZwQueryValueKey (%wZ) failed with Status 0x%x\n", ValueName, status);
			}

			return status;
		}
	}

	if (WdfLdrDiags) 
	{
		DbgPrint("WdfLdr: FxLdrQueryData - ");
		DbgPrint("ERROR: ExAllocatePoolWithTag failed with Status 0x%x\n", STATUS_INSUFFICIENT_RESOURCES);
	}

	return STATUS_INSUFFICIENT_RESOURCES;
}

PWCHAR
FreeString(
	IN PUNICODE_STRING String
)
{
	PWCHAR buffer;
	buffer = String->Buffer;

	if (buffer) 
	{
		ExFreePoolWithTag(buffer, 0);
		buffer = 0;
		String->Length = 0;
		String->Buffer = 0;
	}

	return buffer;
}

VOID
FxLdrAcquireLoadedModuleLock()
{
	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&Resource, TRUE);
}

VOID
FxLdrReleaseLoadedModuleLock()
{
	ExReleaseResourceLite(&Resource);
	KeLeaveCriticalRegion();
}

NTSTATUS
NTAPI
ConvertUlongToWString(
	ULONG Value,
	PUNICODE_STRING String
)
{
	return RtlIntegerToUnicodeString(Value, 10, String);
}


NTSTATUS
NTAPI
BuildServicePath(
	IN PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation,
	IN ULONG Tag,
	IN PUNICODE_STRING ServicePath
)
{
	NTSTATUS status;
	PWCHAR buffer;
	PWCHAR lastSymbol;
	UNICODE_STRING name;
	CONST WCHAR regPath[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\%wZ";


	if (KeyValueInformation->Type != REG_SZ &&
		KeyValueInformation->Type != REG_EXPAND_SZ) 
	{
		status = STATUS_OBJECT_TYPE_MISMATCH;
		goto error;
	}

	if (KeyValueInformation->DataLength == 0 ||
		KeyValueInformation->DataLength > 0xFFFF) 
	{
		status = STATUS_INVALID_PARAMETER;
		goto error;
	}

	name.Buffer = (PWCH)KeyValueInformation->Data;
	name.Length = (USHORT)KeyValueInformation->DataLength;
	name.MaximumLength = (USHORT)KeyValueInformation->DataLength;

	lastSymbol = ((wchar_t*)KeyValueInformation->Data) + KeyValueInformation->DataLength / 2;
	if (KeyValueInformation->DataLength >= 2 &&	*lastSymbol == 0) 
	{
		name.Length = (USHORT)KeyValueInformation->DataLength - 2;
	}
		
	buffer = ExAllocatePoolWithTag(PagedPool, name.Length + sizeof(regPath), Tag);

	if (buffer != NULL)
	{
		ServicePath->Length = 0;
		ServicePath->MaximumLength = name.Length + sizeof(regPath);//106;
		ServicePath->Buffer = buffer;
		memset(ServicePath->Buffer, 0, ServicePath->MaximumLength);
		status = RtlUnicodeStringPrintf(ServicePath, regPath, &name);

		if (!NT_SUCCESS(status)) 
		{
			ExFreePoolWithTag(buffer, 0);
			ServicePath->Length = 0;
			ServicePath->Buffer = NULL;
		}
	}
	else 
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		if (WdfLdrDiags) 
		{
			DbgPrint("WdfLdr: BuildServicePath - ");
			DbgPrint("ERROR: ExAllocatePoolWithTag failed with Status 0x%x\n", STATUS_INSUFFICIENT_RESOURCES);
		}
	}

	goto done;

error:
	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: BuildServicePath - ");
		DbgPrint("ERROR: BuildServicePath failed with status 0x%x\n", status);
	}

done:

	return status;
}


VOID
NTAPI
GetNameFromUnicodePath(
	IN PUNICODE_STRING Path,
	IN OUT PWCHAR Dest,
	IN LONG DestSize
)
{
	PWCHAR stringEnd;
	PWCHAR current;
	NTSTATUS status;

	*Dest = 0;
	if (Path->Length >= 2) 
	{
		stringEnd = &Path->Buffer[Path->Length / 2];

		for (current = stringEnd - 1; *current != '\\'; --current) 
		{
			if (current == Path->Buffer)
				return;
		}

		status = RtlStringCchCopyNW(Dest, DestSize, current + 1, stringEnd - (current + 1));

		if (!NT_SUCCESS(status)) 
		{
			*Dest = L'\0';
		}
	}
}
