/*
 * Copyright 2004, 2005 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <precomp.h>

#include "ntobjutil.h"
#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(ntobjshex);

typedef NTSTATUS(__stdcall* pfnNtGenericOpen)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
typedef NTSTATUS(__stdcall* pfnNtOpenFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);

const LPCWSTR ObjectTypeNames [] = {
    L"Directory", L"SymbolicLink",
    L"Mutant", L"Section", L"Event", L"Semaphore",
    L"Timer", L"Key", L"EventPair", L"IoCompletion",
    L"Device", L"File", L"Controller", L"Profile",
    L"Type", L"Desktop", L"WindowStatiom", L"Driver",
    L"Token", L"Process", L"Thread", L"Adapter", L"Port",
    0
};

const LPCWSTR RegistryTypeNames[] = {
    L"REG_NONE",
    L"REG_SZ",
    L"REG_EXPAND_SZ",
    L"REG_BINARY",
    L"REG_DWORD",
    L"REG_DWORD_BIG_ENDIAN",
    L"REG_LINK",
    L"REG_MULTI_SZ",
    L"REG_RESOURCE_LIST",
    L"REG_FULL_RESOURCE_DESCRIPTOR",
    L"REG_RESOURCE_REQUIREMENTS_LIST ",
    L"REG_QWORD"
};

static DWORD NtOpenObject(OBJECT_TYPE type, PHANDLE phandle, DWORD access, LPCWSTR path)
{
    UNICODE_STRING ustr;

    RtlInitUnicodeString(&ustr, path);

    OBJECT_ATTRIBUTES open_struct = { sizeof(OBJECT_ATTRIBUTES), 0x00, &ustr, 0x40 };

    if (type != FILE_OBJECT)
        access |= STANDARD_RIGHTS_READ;

    IO_STATUS_BLOCK ioStatusBlock;

    switch (type)
    {
    case DIRECTORY_OBJECT:      return NtOpenDirectoryObject(phandle, access, &open_struct);
    case SYMBOLICLINK_OBJECT:   return NtOpenSymbolicLinkObject(phandle, access, &open_struct);
    case MUTANT_OBJECT:         return NtOpenMutant(phandle, access, &open_struct);
    case SECTION_OBJECT:        return NtOpenSection(phandle, access, &open_struct);
    case EVENT_OBJECT:          return NtOpenEvent(phandle, access, &open_struct);
    case SEMAPHORE_OBJECT:      return NtOpenSemaphore(phandle, access, &open_struct);
    case TIMER_OBJECT:          return NtOpenTimer(phandle, access, &open_struct);
    case KEY_OBJECT:            return NtOpenKey(phandle, access, &open_struct);
    case EVENTPAIR_OBJECT:      return NtOpenEventPair(phandle, access, &open_struct);
    case IOCOMPLETITION_OBJECT: return NtOpenIoCompletion(phandle, access, &open_struct);
    case FILE_OBJECT:           return NtOpenFile(phandle, access, &open_struct, &ioStatusBlock, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0);
    default:
        return ERROR_INVALID_FUNCTION;
    }
}

OBJECT_TYPE MapTypeNameToType(LPCWSTR TypeName, DWORD cbTypeName)
{
    if (!TypeName)
        return UNKNOWN_OBJECT_TYPE;

    for (UINT i = 0; i < _countof(ObjectTypeNames); i++)
    {
        LPCWSTR typeName = ObjectTypeNames[i];
        if (!StrCmpNW(typeName, TypeName, cbTypeName / sizeof(WCHAR)))
        {
            return (OBJECT_TYPE) i;
        }
    }

    return UNKNOWN_OBJECT_TYPE;
}

HRESULT EnumerateNtDirectory(HDPA hdpa, PCWSTR path, UINT * hdpaCount)
{
    WCHAR buffer[MAX_PATH];
    PWSTR pend;

    *hdpaCount = 0;

    StringCbCopyExW(buffer, sizeof(buffer), path, &pend, NULL, 0);

    ULONG enumContext = 0;
    HANDLE directory = NULL;

    DWORD err = NtOpenObject(DIRECTORY_OBJECT, &directory, FILE_LIST_DIRECTORY, buffer);
    if (!NT_SUCCESS(err))
    {
        ERR("NtOpenDirectoryObject failed for path %S with status=%x\n", buffer, err);
        return HRESULT_FROM_NT(err);
    }

    if (pend[-1] != '\\')
        *pend++ = '\\';


    BYTE dirbuffer[2048];

    BOOL first = TRUE;
    while (NtQueryDirectoryObject(directory, dirbuffer, 2048, TRUE, first, &enumContext, NULL) == STATUS_SUCCESS)
    {
        first = FALSE;
        POBJECT_DIRECTORY_INFORMATION info = (POBJECT_DIRECTORY_INFORMATION) dirbuffer;
        //for (; info->Name.Buffer != NULL; info++)
        {
            if (info->Name.Buffer)
            {
                StringCbCopyNW(pend, sizeof(buffer), info->Name.Buffer, info->Name.Length);
            }

            OBJECT_TYPE otype = MapTypeNameToType(info->TypeName.Buffer, info->TypeName.Length);
            OBJECT_BASIC_INFORMATION object = { 0 };

            WCHAR wbLink[_MAX_PATH] = { 0 };
            UNICODE_STRING link;
            RtlInitEmptyUnicodeString(&link, wbLink, sizeof(wbLink));

            DWORD entryBufferLength = FIELD_OFFSET(NtPidlEntry,entryName) + sizeof(WCHAR);
            if (info->Name.Buffer)
                entryBufferLength += info->Name.Length;

            if (otype < 0)
            {
                entryBufferLength += FIELD_OFFSET(NtPidlTypeData,typeName) + sizeof(WCHAR);

                if (info->TypeName.Buffer)
                {
                    entryBufferLength += info->TypeName.Length;
                }
            }

            if (otype == SYMBOLICLINK_OBJECT)
            {
                entryBufferLength += FIELD_OFFSET(NtPidlSymlinkData,targetName) + sizeof(WCHAR);
            }

            DWORD access = STANDARD_RIGHTS_READ;
            if ((otype == DIRECTORY_OBJECT) ||
                (otype == SYMBOLICLINK_OBJECT))
                access |= FILE_LIST_DIRECTORY;

            HANDLE handle;
            if (!NtOpenObject(otype, &handle, access, buffer))
            {
                DWORD read;

                if (!NT_SUCCESS(NtQueryObject(handle, ObjectBasicInformation, &object, sizeof(OBJECT_BASIC_INFORMATION), &read)))
                {
                    ZeroMemory(&object, sizeof(OBJECT_BASIC_INFORMATION));
                }

                if (otype == SYMBOLICLINK_OBJECT)
                {
                    if (NtQuerySymbolicLinkObject(handle, &link, NULL) == STATUS_SUCCESS)
                    {
                        entryBufferLength += link.Length;
                    }
                    else
                    {
                        link.Length = 0;
                    }
                }

                NtClose(handle);
            }

            NtPidlEntry* entry = (NtPidlEntry*) CoTaskMemAlloc(entryBufferLength);
            if (!entry)
                return E_OUTOFMEMORY;

            memset(entry, 0, entryBufferLength);

            entry->cb = FIELD_OFFSET(NtPidlEntry,entryName);
            entry->magic = NT_OBJECT_PIDL_MAGIC;
            entry->objectType = otype;
            entry->objectInformation = object;
            memset(entry->objectInformation.Reserved, 0, sizeof(entry->objectInformation.Reserved));

            if (info->Name.Buffer)
            {
                entry->entryNameLength = info->Name.Length;
                StringCbCopyNW(entry->entryName, entryBufferLength, info->Name.Buffer, info->Name.Length);
                entry->cb += entry->entryNameLength + sizeof(WCHAR);
            }
            else
            {
                entry->entryNameLength = 0;
                entry->entryName[0] = 0;
                entry->cb += sizeof(WCHAR);
            }

            if (otype < 0)
            {
                NtPidlTypeData * typedata = (NtPidlTypeData*) ((PBYTE) entry + entry->cb);
                DWORD remainingSpace = entryBufferLength - ((PBYTE) (typedata->typeName) - (PBYTE) entry);

                if (info->TypeName.Buffer)
                {
                    typedata->typeNameLength = info->TypeName.Length;
                    StringCbCopyNW(typedata->typeName, remainingSpace, info->TypeName.Buffer, info->TypeName.Length);

                    entry->cb += typedata->typeNameLength + sizeof(WCHAR);
                }
                else
                {
                    typedata->typeNameLength = 0;
                    typedata->typeName[0] = 0;
                    entry->cb += typedata->typeNameLength + sizeof(WCHAR);
                }
            }

            if (otype == SYMBOLICLINK_OBJECT)
            {
                NtPidlSymlinkData * symlink = (NtPidlSymlinkData*) ((PBYTE) entry + entry->cb);
                DWORD remainingSpace = entryBufferLength - ((PBYTE) (symlink->targetName) - (PBYTE) entry);

                symlink->targetNameLength = link.Length;
                StringCbCopyNW(symlink->targetName, remainingSpace, link.Buffer, link.Length);

                entry->cb += symlink->targetNameLength + sizeof(WCHAR);
            }

            DPA_AppendPtr(hdpa, entry);
            (*hdpaCount)++;
        }
    }

    NtClose(directory);

    return S_OK;
}

HRESULT EnumerateRegistryKey(HDPA hdpa, PCWSTR path, HKEY root, UINT * hdpaCount)
{
    *hdpaCount = 0;

    HKEY hkey;

    DWORD res;
    if (root)
    {
        res = RegOpenKeyExW(root, *path == '\\' ? path + 1 : path, 0, STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkey);
    }
    else
    {
        res = NtOpenObject(KEY_OBJECT, (PHANDLE)&hkey, STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, path);
    }
    if (!NT_SUCCESS(res))
    {
        ERR("RegOpenKeyExW failed for path %S with status=%x\n", path, res);
        return HRESULT_FROM_NT(res);
    }

    for (int idx = 0;; ++idx)
    {
        WCHAR name[MAX_PATH];
        DWORD cchName = _countof(name);

        WCHAR className[MAX_PATH];
        DWORD cchClass = _countof(className);

        if (RegEnumKeyExW(hkey, idx, name, &cchName, 0, className, &cchClass, NULL))
            break;

        name[cchName] = 0;
        className[cchClass] = 0;

        REG_ENTRY_TYPE otype = REG_ENTRY_KEY;

        DWORD entryBufferLength = FIELD_OFFSET(RegPidlEntry,entryName) + sizeof(WCHAR) + cchName * sizeof(WCHAR);

        if (cchClass > 0)
        {
            entryBufferLength += sizeof(WCHAR) + cchClass * sizeof(WCHAR);
        }
        
        RegPidlEntry* entry = (RegPidlEntry*) CoTaskMemAlloc(entryBufferLength);
        if (!entry)
            return E_OUTOFMEMORY;

        memset(entry, 0, entryBufferLength);

        entry->cb = FIELD_OFFSET(NtPidlEntry,entryName);
        entry->magic = REGISTRY_PIDL_MAGIC;
        entry->entryType = otype;

        if (cchName > 0)
        {
            entry->entryNameLength = cchName * sizeof(WCHAR);
            StringCbCopyNW(entry->entryName, entryBufferLength, name, entry->entryNameLength);
            entry->cb += entry->entryNameLength + sizeof(WCHAR);
        }
        else
        {
            entry->entryNameLength = 0;
            entry->entryName[0] = 0;
            entry->cb += sizeof(WCHAR);
        }

        if (cchClass)
        {
            PWSTR contentData = (PWSTR) ((PBYTE) entry + entry->cb);
            DWORD remainingSpace = entryBufferLength - entry->cb;

            entry->contentsLength = cchClass * sizeof(WCHAR);
            StringCbCopyNW(contentData, remainingSpace, className, entry->contentsLength);

            entry->cb += entry->contentsLength + sizeof(WCHAR);
        }

        DPA_AppendPtr(hdpa, entry);
        (*hdpaCount)++;

    }
        
    for (int idx = 0;; ++idx)
    {
        WCHAR name[MAX_PATH];
        DWORD cchName = _countof(name);
        DWORD type;
        DWORD dataSize;

        if (RegEnumValueW(hkey, idx, name, &cchName, 0, &type, NULL, &dataSize))
            break;

        REG_ENTRY_TYPE otype = REG_ENTRY_VALUE;

        DWORD entryBufferLength = FIELD_OFFSET(RegPidlEntry,entryName) + sizeof(WCHAR) + cchName * sizeof(WCHAR);

        BOOL copyData = dataSize < 32;
        if (copyData)
        {
            entryBufferLength += dataSize + sizeof(WCHAR);

            otype = REG_ENTRY_VALUE_WITH_CONTENT;
        }
        
        RegPidlEntry* entry = (RegPidlEntry*) CoTaskMemAlloc(entryBufferLength);
        if (!entry)
            return E_OUTOFMEMORY;

        memset(entry, 0, entryBufferLength);

        entry->cb = FIELD_OFFSET(RegPidlEntry, entryName);
        entry->magic = REGISTRY_PIDL_MAGIC;
        entry->entryType = otype;
        entry->contentType = type;

        if (cchName > 0)
        {
            entry->entryNameLength = cchName * sizeof(WCHAR);
            StringCbCopyNW(entry->entryName, entryBufferLength, name, entry->entryNameLength);
            entry->cb += entry->entryNameLength + sizeof(WCHAR);
        }
        else
        {
            entry->entryNameLength = 0;
            entry->entryName[0] = 0;
            entry->cb += sizeof(WCHAR);
        }

        if (copyData)
        {
            PBYTE contentData = (PBYTE) ((PBYTE) entry + entry->cb);

            entry->contentsLength = dataSize;

            // In case it's an unterminated string, RegGetValue will add the NULL termination
            dataSize += sizeof(WCHAR);

            if (!RegQueryValueExW(hkey, name, NULL, NULL, contentData, &dataSize))
            {
                entry->cb += entry->contentsLength + sizeof(WCHAR);
            }
            else
            {
                entry->contentsLength = 0;
                entry->cb += sizeof(WCHAR);
            }

        }

        DPA_AppendPtr(hdpa, entry);
        (*hdpaCount)++;
    }

    RegCloseKey(hkey);

    return S_OK;
}

HRESULT ReadRegistryValue(HKEY root, PCWSTR path, PCWSTR valueName, PVOID * valueData, PDWORD valueLength)
{
    HKEY hkey;

    DWORD res;
    if (root)
    {
        res = RegOpenKeyExW(root, *path == '\\' ? path + 1 : path, 0, STANDARD_RIGHTS_READ | KEY_QUERY_VALUE, &hkey);
    }
    else
    {
        res = NtOpenObject(KEY_OBJECT, (PHANDLE) &hkey, STANDARD_RIGHTS_READ | KEY_QUERY_VALUE, path);
    }
    if (!NT_SUCCESS(res))
    {
        ERR("RegOpenKeyExW failed for path %S with status=%x\n", path, res);
        return HRESULT_FROM_NT(res);
    }

    res = RegQueryValueExW(hkey, valueName, NULL, NULL, NULL, valueLength);

    if (*valueLength > 0)
    {
        *valueData = (PBYTE) CoTaskMemAlloc(*valueLength);

        res = RegQueryValueExW(hkey, valueName, NULL, NULL, (PBYTE) *valueData, valueLength);
        if (!NT_SUCCESS(res))
        {
            CoTaskMemFree(*valueData);
            *valueData = NULL;

            RegCloseKey(hkey);

            ERR("RegOpenKeyExW failed for path %S with status=%x\n", path, res);
            return HRESULT_FROM_NT(res);
        }
    }
    else
    {
        *valueData = NULL;
    }

    RegCloseKey(hkey);

    return S_OK;
}