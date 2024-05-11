/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Registry layer manipulation functions
 * COPYRIGHT:   Copyright 2015-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "strsafe.h"
#include <ntndk.h>
#include "apphelp.h"

#define GPLK_USER                   1
#define GPLK_MACHINE                2
#define MAX_LAYER_LENGTH            256
#define LAYER_APPLY_TO_SYSTEM_EXES  1
#define LAYER_UNK_FLAG2             2

#ifndef REG_SZ
#define REG_SZ                      1
#endif

#if defined(__GNUC__)
#define APPCOMPAT_LAYER_KEY     (const WCHAR[]){'\\','S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',' ','N','T','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','A','p','p','C','o','m','p','a','t','F','l','a','g','s','\\','L','a','y','e','r','s',0}
#define REGISTRY_MACHINE        (const WCHAR[]){'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e',0}
#define SPACE_ONLY              (const WCHAR[]){' ',0}
#define DISALLOWED_LAYER_CHARS  (const WCHAR[]){' ','#','!',0}
#define LAYER_SEPARATORS        (const WCHAR[]){' ','\t',0}
#define SIGN_MEDIA_FMT          (const WCHAR[]){'S','I','G','N','.','M','E','D','I','A','=','%','X',' ','%','s',0}

#else
#define APPCOMPAT_LAYER_KEY     L"\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"
#define REGISTRY_MACHINE        L"\\Registry\\Machine"
#define SPACE_ONLY              L" "
#define DISALLOWED_LAYER_CHARS  L" #!"
#define LAYER_SEPARATORS        L" \t"
#define SIGN_MEDIA_FMT          L"SIGN.MEDIA=%X %s"
#endif

/* Fixme: use RTL_UNICODE_STRING_BUFFER */
typedef struct SDB_TMP_STR
{
    UNICODE_STRING Str;
    WCHAR FixedBuffer[MAX_PATH];
} SDB_TMP_STR, *PSDB_TMP_STR;

void SdbpInitTempStr(PSDB_TMP_STR String)
{
    String->Str.Buffer = String->FixedBuffer;
    String->Str.Length = 0;
    String->Str.MaximumLength = sizeof(String->FixedBuffer);
}

void SdbpFreeTempStr(PSDB_TMP_STR String)
{
    if (String->Str.Buffer != String->FixedBuffer)
    {
        SdbFree(String->Str.Buffer);
    }
}

void SdbpResizeTempStr(PSDB_TMP_STR String, WORD newLength)
{
    if (newLength > String->Str.MaximumLength)
    {
        SdbpFreeTempStr(String);
        String->Str.MaximumLength = newLength * sizeof(WCHAR);
        String->Str.Buffer = SdbAlloc(String->Str.MaximumLength);
        String->Str.Length = 0;
    }
}

BOOL SdbpGetLongPathName(PCWSTR wszPath, PSDB_TMP_STR Result)
{
    DWORD max = Result->Str.MaximumLength / 2;
    DWORD ret = GetLongPathNameW(wszPath, Result->Str.Buffer, max);
    if (ret)
    {
        if (ret >= max)
        {
            SdbpResizeTempStr(Result, ret);
            max = Result->Str.MaximumLength / 2;
            ret = GetLongPathNameW(wszPath, Result->Str.Buffer, max);
        }
        if (ret && ret < max)
        {
            Result->Str.Length = ret * 2;
            return TRUE;
        }
    }
    SHIM_ERR("Failed to convert short path to long path error 0x%lx\n", GetLastError());
    return FALSE;
}

BOOL SdbpIsPathOnRemovableMedia(PCWSTR Path)
{
    WCHAR tmp[] = { 'A',':','\\',0 };
    ULONG type;
    if (!Path || Path[0] == UNICODE_NULL)
    {
        SHIM_ERR("Invalid argument\n");
        return FALSE;
    }
    switch (Path[1])
    {
    case L':':
        break;
    case L'\\':
        SHIM_INFO("\"%S\" is a network path.\n", Path);
        return FALSE;
    default:
        SHIM_INFO("\"%S\" not a full path we can operate on.\n", Path);
        return FALSE;
    }
    tmp[0] = Path[0];
    type = GetDriveTypeW(tmp);

    return type == DRIVE_REMOVABLE || type == DRIVE_CDROM;
}

/* Convert a path on removable media to 'SIGN.MEDIA=%X filename' */
BOOL SdbpBuildSignMediaId(PSDB_TMP_STR LongPath)
{
    SDB_TMP_STR Scratch;
    PWCHAR Ptr;

    SdbpInitTempStr(&Scratch);
    SdbpResizeTempStr(&Scratch, LongPath->Str.Length / sizeof(WCHAR) + 30);
    StringCbCopyNW(Scratch.Str.Buffer, Scratch.Str.MaximumLength, LongPath->Str.Buffer, LongPath->Str.Length);
    Ptr = wcsrchr(LongPath->Str.Buffer, '\\');
    if (Ptr)
    {
        HANDLE FindHandle;
        WIN32_FIND_DATAW FindData;
        Ptr[1] = '*';
        Ptr[2] = '\0';
        FindHandle = FindFirstFileW(LongPath->Str.Buffer, &FindData);
        if (FindHandle != INVALID_HANDLE_VALUE)
        {
            DWORD SignMedia = 0;
            do
            {
                if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && FindData.nFileSizeLow)
                    SignMedia = SignMedia << 1 ^ FindData.nFileSizeLow;
            } while (FindNextFileW(FindHandle, &FindData));

            FindClose(FindHandle);
            SdbpResizeTempStr(LongPath, (LongPath->Str.Length >> 1) + 20);
            StringCbPrintfW(LongPath->Str.Buffer, LongPath->Str.MaximumLength, SIGN_MEDIA_FMT, SignMedia, Scratch.Str.Buffer + 3);
            LongPath->Str.Length = (USHORT)SdbpStrlen(LongPath->Str.Buffer) * sizeof(WCHAR);
            SdbpFreeTempStr(&Scratch);
            return TRUE;
        }
    }
    SdbpFreeTempStr(&Scratch);
    SdbpFreeTempStr(LongPath);
    return FALSE;
}

/* Convert a given path to a long or media path */
BOOL SdbpResolvePath(PSDB_TMP_STR LongPath, PCWSTR wszPath)
{
    SdbpInitTempStr(LongPath);
    if (!SdbpGetLongPathName(wszPath, LongPath))
    {
        SdbpFreeTempStr(LongPath);
        return FALSE;
    }
    if (SdbpIsPathOnRemovableMedia(LongPath->Str.Buffer))
    {
        return SdbpBuildSignMediaId(LongPath);
    }
    return TRUE;
}

static ACCESS_MASK g_QueryFlag = 0xffffffff;
ACCESS_MASK Wow64QueryFlag(void)
{
    if (g_QueryFlag == 0xffffffff)
    {
        ULONG_PTR wow64_ptr = 0;
        NTSTATUS Status = NtQueryInformationProcess(NtCurrentProcess(), ProcessWow64Information, &wow64_ptr, sizeof(wow64_ptr), NULL);
        g_QueryFlag = (NT_SUCCESS(Status) && wow64_ptr != 0) ? KEY_WOW64_64KEY : 0;
    }
    return g_QueryFlag;
}

NTSTATUS SdbpOpenKey(PUNICODE_STRING FullPath, BOOL bMachine, ACCESS_MASK Access, PHANDLE KeyHandle)
{
    UNICODE_STRING BasePath;
    const WCHAR* LayersKey = APPCOMPAT_LAYER_KEY;
    OBJECT_ATTRIBUTES ObjectLayer = RTL_INIT_OBJECT_ATTRIBUTES(FullPath, OBJ_CASE_INSENSITIVE);
    NTSTATUS Status;
    FullPath->Buffer = NULL;
    FullPath->Length = FullPath->MaximumLength = 0;
    if (bMachine)
    {
        RtlInitUnicodeString(&BasePath, REGISTRY_MACHINE);
    }
    else
    {
        Status = RtlFormatCurrentUserKeyPath(&BasePath);
        if (!NT_SUCCESS(Status))
        {
            SHIM_ERR("Unable to acquire user registry key, Error: 0x%lx\n", Status);
            return Status;
        }
    }
    FullPath->MaximumLength = (USHORT)(BasePath.Length + SdbpStrsize(LayersKey));
    FullPath->Buffer = SdbAlloc(FullPath->MaximumLength);
    FullPath->Length = 0;
    RtlAppendUnicodeStringToString(FullPath, &BasePath);
    if (!bMachine)
        RtlFreeUnicodeString(&BasePath);
    RtlAppendUnicodeToString(FullPath, LayersKey);

    Status = NtOpenKey(KeyHandle, Access | Wow64QueryFlag(), &ObjectLayer);
    if (!NT_SUCCESS(Status))
    {
        SHIM_ERR("Unable to open Key  \"%wZ\" Status 0x%lx\n", FullPath, Status);
        SdbFree(FullPath->Buffer);
        FullPath->Buffer = NULL;
    }
    return Status;
}


BOOL SdbpGetPermLayersInternal(PUNICODE_STRING FullPath, PWSTR pwszLayers, PDWORD pdwBytes, BOOL bMachine)
{
    UNICODE_STRING FullKey;
    ULONG ValueBuffer[(MAX_LAYER_LENGTH * sizeof(WCHAR) + sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG) - 1) / sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PVOID)ValueBuffer;
    ULONG Length = 0;
    HANDLE KeyHandle;
    NTSTATUS Status;

    Status = SdbpOpenKey(&FullKey, bMachine, KEY_QUERY_VALUE, &KeyHandle);
    if (NT_SUCCESS(Status))
    {
        Status = NtQueryValueKey(KeyHandle, FullPath, KeyValuePartialInformation, PartialInfo, sizeof(ValueBuffer), &Length);
        if (NT_SUCCESS(Status))
        {
            StringCbCopyNW(pwszLayers, *pdwBytes, (PCWSTR)PartialInfo->Data, PartialInfo->DataLength);
            *pdwBytes = PartialInfo->DataLength;
        }
        else
        {
            SHIM_INFO("Failed to read value info from Key \"%wZ\" Status 0x%lx\n", &FullKey, Status);
        }
        NtClose(KeyHandle);
        SdbFree(FullKey.Buffer);
    }
    return NT_SUCCESS(Status);
}

BOOL SdbDeletePermLayerKeys(PCWSTR wszPath, BOOL bMachine)
{
    UNICODE_STRING FullKey;
    SDB_TMP_STR LongPath;
    HANDLE KeyHandle;
    NTSTATUS Status;

    if (!SdbpResolvePath(&LongPath, wszPath))
        return FALSE;

    Status = SdbpOpenKey(&FullKey, bMachine, KEY_SET_VALUE, &KeyHandle);
    if (NT_SUCCESS(Status))
    {
        Status = NtDeleteValueKey(KeyHandle, &LongPath.Str);
        if (!NT_SUCCESS(Status))
        {
            SHIM_INFO("Failed to delete value from Key \"%wZ\" Status 0x%lx\n", &FullKey, Status);
            /* This is what we want, so if the key didnt exist, we should not fail :) */
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_SUCCESS;
        }
        NtClose(KeyHandle);
        SdbFree(FullKey.Buffer);
    }
    SdbpFreeTempStr(&LongPath);
    return NT_SUCCESS(Status);
}

BOOL SdbpMatchLayer(PCWSTR start, PCWSTR end, PCWSTR compare)
{
    size_t len;
    if (!end)
        return !_wcsicmp(start, compare);
    len = end - start;
    return wcslen(compare) == len && !_wcsnicmp(start, compare, len);
}

BOOL SdbpAppendLayer(PWSTR target, DWORD len, PCWSTR layer, PCWSTR end)
{
    NTSTATUS Status = STATUS_SUCCESS;
    if (target[0])
        Status = StringCbCatW(target, len, SPACE_ONLY);

    if (NT_SUCCESS(Status))
    {
        if (end)
            Status = StringCbCatNW(target, len, layer, (end - layer) * sizeof(WCHAR));
        else
            Status = StringCbCatW(target, len, layer);
    }

    return NT_SUCCESS(Status);
}


/**
 * Determine if we allow permission layers to apply on this file.
 *
 * @param [in]  Path    Full pathname of the file, only the drive part is used.
 *
 * @return  TRUE if we allow permission layer, FALSE if not.
 */
BOOL WINAPI AllowPermLayer(PCWSTR Path)
{
    WCHAR tmp[] = { 'A',':','\\', 0 };
    ULONG type;
    if (!Path)
    {
        SHIM_ERR("Invalid argument\n");
        return FALSE;
    }
    switch (Path[1])
    {
    case L':':
        break;
    case L'\\':
        SHIM_INFO("\"%S\" is a network path.\n", Path);
        return FALSE;
    default:
        SHIM_INFO("\"%S\" not a full path we can operate on.\n", Path);
        return FALSE;
    }
    tmp[0] = Path[0];
    type = GetDriveTypeW(tmp);
    if (type == DRIVE_REMOTE)
    {
        /* The logging here indicates that it does not like a CDROM or removable media, but it only
            seems to bail out on a media that reports it is remote...
            I have included correct logging, I doubt anyone would parse the logging, so this shouldnt break anything. */
        SHIM_INFO("\"%S\" is on a remote drive.\n", Path);
        return FALSE;
    }
    return TRUE;
}

/**
 * Read the layers specified for the application.
 *
 * @param [in]  wszPath     Full pathname of the file.
 * @param [out] pwszLayers  On return, the layers set on the file.
 * @param   pdwBytes        The size of the pwszLayers buffer in bytes, and on return the size of
 *                          the data written (in bytes)
 * @param [in]  dwFlags     The flags, [GPLK_USER | GPLK_MACHINE].
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbGetPermLayerKeys(PCWSTR wszPath, PWSTR pwszLayers, PDWORD pdwBytes, DWORD dwFlags)
{
    BOOL Result = FALSE;
    SDB_TMP_STR LongPath;
    DWORD dwBytes, dwTotal = 0;
    if (!wszPath || !pdwBytes)
    {
        SHIM_ERR("NULL parameter passed for wszPath or pdwBytes.\n");
        return FALSE;
    }

    if (!SdbpResolvePath(&LongPath, wszPath))
        return FALSE;
    dwBytes = *pdwBytes;
    if (dwFlags & GPLK_MACHINE)
    {
        if (SdbpGetPermLayersInternal(&LongPath.Str, pwszLayers, &dwBytes, TRUE))
        {
            Result = TRUE;
            dwTotal = dwBytes - sizeof(WCHAR); /* Compensate for the nullterm. */
            pwszLayers += dwTotal / sizeof(WCHAR);
            dwBytes = *pdwBytes - dwBytes;
            if (dwFlags & GPLK_USER)
            {
                *(pwszLayers++) = L' ';
                *pwszLayers = L'\0';
                dwBytes -= sizeof(WCHAR);
                dwTotal += sizeof(WCHAR);
            }
        }
    }
    if (dwFlags & GPLK_USER)
    {
        if (SdbpGetPermLayersInternal(&LongPath.Str, pwszLayers, &dwBytes, FALSE))
        {
            Result = TRUE;
            dwTotal += dwBytes - sizeof(WCHAR); /* Compensate for the nullterm. */
        }
        else if (dwTotal > 0 && pwszLayers[-1] == L' ')
        {
            pwszLayers[-1] = '\0';
            dwTotal -= sizeof(WCHAR);
        }
    }
    if (dwTotal)
        dwTotal += sizeof(WCHAR);
    *pdwBytes = dwTotal;
    SdbpFreeTempStr(&LongPath);
    return Result;
}

/**
 * Set or clear the Layer key.
 *
 * @param [in]  wszPath     Full pathname of the file.
 * @param [in]  wszLayers   The layers to add (space separated), or an empty string / NULL to
 *                          remove all layers.
 * @param [in]  bMachine    TRUE to machine.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbSetPermLayerKeys(PCWSTR wszPath, PCWSTR wszLayers, BOOL bMachine)
{
    UNICODE_STRING FullKey;
    SDB_TMP_STR LongPath;
    HANDLE KeyHandle;
    NTSTATUS Status;

    if (!wszLayers || *wszLayers == '\0')
        return SdbDeletePermLayerKeys(wszPath, bMachine);

    if (!SdbpResolvePath(&LongPath, wszPath))
        return FALSE;

    Status = SdbpOpenKey(&FullKey, bMachine, KEY_SET_VALUE, &KeyHandle);
    if (NT_SUCCESS(Status))
    {
        Status = NtSetValueKey(KeyHandle, &LongPath.Str, 0, REG_SZ, (PVOID)wszLayers, SdbpStrsize(wszLayers));
        if (!NT_SUCCESS(Status))
        {
            SHIM_INFO("Failed to write a value to Key \"%wZ\" Status 0x%lx\n", &FullKey, Status);
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_SUCCESS;
        }
        NtClose(KeyHandle);
        SdbFree(FullKey.Buffer);
    }
    SdbpFreeTempStr(&LongPath);
    return NT_SUCCESS(Status);
}

/**
 * Adds or removes a single layer entry.
 *
 * @param [in]  wszPath     Full pathname of the file.
 * @param [in]  wszLayer    The layer to add or remove.
 * @param [in]  dwFlags     Additional flags to add / remove [LAYER_APPLY_TO_SYSTEM_EXES | ???].
 * @param [in]  bMachine    When TRUE, the setting applies to all users, when FALSE only applies
 *                          to the current user.
 * @param [in]  bEnable     TRUE to enable, FALSE to disable a layer / flag specified.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SetPermLayerState(PCWSTR wszPath, PCWSTR wszLayer, DWORD dwFlags, BOOL bMachine, BOOL bEnable)
{
    WCHAR fullLayer[MAX_LAYER_LENGTH] = { 0 };
    WCHAR newLayer[MAX_LAYER_LENGTH] = { 0 };
    DWORD dwBytes = sizeof(fullLayer), dwWriteFlags = 0;
    PWSTR start, p;

    if (!wszLayer)
    {
        SHIM_ERR("Invalid argument\n");
        return FALSE;
    }
    if (dwFlags & ~(LAYER_APPLY_TO_SYSTEM_EXES | LAYER_UNK_FLAG2))
    {
        SHIM_ERR("Invalid flags\n");
        return FALSE;
    }
    p = wcspbrk(wszLayer, DISALLOWED_LAYER_CHARS);
    if (p)
    {
        switch (*p)
        {
        case ' ':
            SHIM_ERR("Only one layer can be passed in at a time.\n");
            return FALSE;
        case '#':
        case '!':
            SHIM_ERR("Flags cannot be passed in with the layer name.\n");
            return FALSE;
        }
    }
    if (!SdbGetPermLayerKeys(wszPath, fullLayer, &dwBytes, bMachine ? GPLK_MACHINE : GPLK_USER))
    {
        fullLayer[0] = '\0';
        dwBytes = sizeof(fullLayer);
    }

    start = fullLayer;
    while (*start == '!' || *start == '#' || *start == ' ' || *start == '\t')
    {
        if (*start == '#')
            dwWriteFlags |= LAYER_APPLY_TO_SYSTEM_EXES;
        else if (*start == '!')
            dwWriteFlags |= LAYER_UNK_FLAG2;
        start++;
    }
    if (bEnable)
        dwWriteFlags |= dwFlags;
    else
        dwWriteFlags &= ~dwFlags;

    p = newLayer;
    if (dwWriteFlags & LAYER_UNK_FLAG2)
        *(p++) = '!';
    if (dwWriteFlags & LAYER_APPLY_TO_SYSTEM_EXES)
        *(p++) = '#';

    do
    {
        while (*start == ' ' || *start == '\t')
            ++start;

        if (*start == '\0')
            break;
        p = wcspbrk(start, LAYER_SEPARATORS);
        if (!SdbpMatchLayer(start, p, wszLayer))
        {
            SdbpAppendLayer(newLayer, sizeof(newLayer), start, p);
        }
        start = p + 1;
    } while (p);

    if (bEnable && wszLayer[0])
    {
        SdbpAppendLayer(newLayer, sizeof(newLayer), wszLayer, NULL);
    }

    return SdbSetPermLayerKeys(wszPath, newLayer, bMachine);
}
