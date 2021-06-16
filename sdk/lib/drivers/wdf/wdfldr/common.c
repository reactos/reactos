/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - common functions
 * COPYRIGHT:   Copyright 2019 Max Korostil <mrmks04@yandex.ru>
 *              Copyright 2021 Victor Perevertkin <victor.perevertkin@reactos.org>
 */

#include "wdfloader.h"


VOID
FxLdrAcquireLoadedModuleLock()
{
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&WdfLdrGlobals.LoadedModulesListLock, TRUE);
}

VOID
FxLdrReleaseLoadedModuleLock()
{
    ExReleaseResourceLite(&WdfLdrGlobals.LoadedModulesListLock);
    KeLeaveCriticalRegion();
}

VOID
GetNameFromPath(
    _In_ PCUNICODE_STRING Path,
    _Out_ PUNICODE_STRING Name)
{
    PWCHAR pNextSym;
    PWCHAR pCurrSym;

    if (Path->Length < sizeof(WCHAR))
    {
        Name->Length = 0;
        Name->Buffer = NULL;
        return;
    }
    
    Name->Buffer = Path->Buffer + (Path->Length / 2) - 1;
    Name->Length = sizeof(WCHAR);

    for (pNextSym = Name->Buffer; ; Name->Buffer = pNextSym)
    {
        if (pNextSym < Path->Buffer)
        {
            Name->Length -= sizeof(WCHAR);
            ++Name->Buffer;
            goto end;
        }
        pCurrSym = Name->Buffer;

        if (*pCurrSym == '\\')
        {
            break;
        }
        pNextSym = pCurrSym - 1;
        Name->Length += sizeof(WCHAR);
    }

    ++Name->Buffer;
    Name->Length -= sizeof(WCHAR);

    if (Name->Length == sizeof(WCHAR))
    {
        Name->Buffer = NULL;
        Name->Length = 0;
    }

end:
    Name->MaximumLength = Name->Length;
}

NTSTATUS
GetImageName(
    _In_ PCUNICODE_STRING ServicePath,
    _Out_ PUNICODE_STRING ImageName)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING name;
    UNICODE_STRING path;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValPartial = NULL;
    HANDLE keyHandle = NULL;
    UNICODE_STRING valueName = RTL_CONSTANT_STRING(L"ImagePath");

    InitializeObjectAttributes(&objectAttributes,
                               (PUNICODE_STRING)ServicePath,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey(&keyHandle, KEY_READ, &objectAttributes);

    if (!NT_SUCCESS(status)) 
    {
        __DBGPRINT(("ERROR: GetImageName failed with status 0x%x\n", status));
        return status;
    }

    status = FxLdrQueryData(keyHandle, &valueName, WDFLDR_TAG, &pKeyValPartial);
    if (!NT_SUCCESS(status)) 
    {
        __DBGPRINT(("ERROR: GetImageName failed with status 0x%x\n", status));
        return status;
    }

    if (pKeyValPartial->Type != REG_SZ &&
        pKeyValPartial->Type != REG_EXPAND_SZ) 
    {
        status = STATUS_OBJECT_TYPE_MISMATCH;
        __DBGPRINT(("ERROR: GetImageName failed with status 0x%x\n", status));
    }

    if (pKeyValPartial->DataLength == 0 ||
        pKeyValPartial->DataLength > 0xFFFF) 
    {
        status = STATUS_INVALID_PARAMETER;    
        __DBGPRINT(("ERROR: GetImageName failed with status 0x%x\n", status));
    }

    path.Buffer = (PWCH)pKeyValPartial->Data;
    path.Length = (USHORT)pKeyValPartial->DataLength;
    path.MaximumLength = (USHORT)pKeyValPartial->DataLength;

    if (pKeyValPartial->DataLength >= sizeof(WCHAR) &&
        !*(((WCHAR*)&pKeyValPartial->Data) + pKeyValPartial->DataLength / sizeof(WCHAR)))
    {
        path.Length = (USHORT)pKeyValPartial->DataLength - sizeof(WCHAR);
    }

    GetNameFromPath(&path, &name);

    if (name.Length == 0) 
    {
        status = STATUS_INVALID_PARAMETER;
        __DBGPRINT(("ERROR: GetNameFromPathW could not find a name, status 0x%x\n", status));
        goto clean;
    }

    status = RtlUShortAdd(name.Length, 2, &ImageName->Length);
        
    if (!NT_SUCCESS(status)) 
    {
        status = STATUS_INTEGER_OVERFLOW;
        __DBGPRINT(("ERROR: size computation failed with Status 0x%x\n", status));
        goto clean;
    }
    
    ImageName->Buffer = ExAllocatePoolWithTag(PagedPool, ImageName->Length, WDFLDR_TAG);

    if (ImageName->Buffer != NULL) 
    {
        RtlZeroMemory(ImageName->Buffer, ImageName->Length);
        ImageName->MaximumLength = ImageName->Length;
        ImageName->Length = 0;
        RtlCopyUnicodeString(ImageName, &name);

        __DBGPRINT(("Version Image Name \"%wZ\"\n", ImageName));
    }
    else 
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        __DBGPRINT(("ERROR: ExAllocatePoolWithTag failed with Status 0x%x\n", status));
    }

clean:
    if (keyHandle) 
    {
        ZwClose(keyHandle);
    }
    if (pKeyValPartial) 
    {
        ExFreePoolWithTag(pKeyValPartial, WDFLDR_TAG);
    }

    return status;
}

NTSTATUS
GetImageInfo(
    _In_ PCUNICODE_STRING ImageName,
    _Out_ PVOID* ImageBase,
    _Out_ PULONG ImageSize)
{
    ANSI_STRING ansiImageName;
    NTSTATUS status;

    status = RtlUnicodeStringToAnsiString(&ansiImageName, ImageName, TRUE);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: RtlUnicodeStringToAnsiString failed with Status 0x%x\n", status));
        return status;
    }

    PAUX_MODULE_EXTENDED_INFO modulesBuffer;
    ULONG modulesSize;
    for (;;)
    {
        status = AuxKlibQueryModuleInformation(&modulesSize,
                                               sizeof(AUX_MODULE_EXTENDED_INFO),
                                               NULL);
        if (!NT_SUCCESS(status))
        {
            goto clean;
        }

        modulesBuffer = ExAllocatePoolZero(PagedPool, modulesSize, WDFLDR_TAG);
        if (modulesBuffer == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            __DBGPRINT(("ERROR: ExAllocatePoolWithTag failed\n"));

            goto clean;
        }

        status = AuxKlibQueryModuleInformation(&modulesSize,
                                               sizeof(AUX_MODULE_EXTENDED_INFO),
                                               modulesBuffer);
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            ExFreePoolWithTag(modulesBuffer, WDFLDR_TAG);
            continue;
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        for (SIZE_T i = 0; i < modulesSize / sizeof(AUX_MODULE_EXTENDED_INFO); i++)
        {
            // Compare our image name to names returned by AuxKlib
            if (modulesBuffer[i].FileNameOffset < AUX_KLIB_MODULE_PATH_LEN &&
                _strnicmp(&modulesBuffer[i].FullPathName[modulesBuffer[i].FileNameOffset],
                          ansiImageName.Buffer, ansiImageName.Length) == 0)
            {
                *ImageBase = modulesBuffer[i].BasicInfo.ImageBase;
                *ImageSize = modulesBuffer[i].ImageSize;
                break;
            }
        }
    }
    else
    {
        __DBGPRINT(("ERROR: AuxKlibQueryModuleInformation failed with Status 0x%x\n", status));
    }

    ExFreePoolWithTag(modulesBuffer, WDFLDR_TAG);

clean:
    RtlFreeAnsiString(&ansiImageName);
    return status;
}
