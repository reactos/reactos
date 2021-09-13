/*
 * Copyright 2004 Martin Fuchs
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
#pragma once


// All the possible values are defined here because I want the type field to be
// "persistable" and not change if more types are added in the future.
enum OBJECT_TYPE {
    DIRECTORY_OBJECT, SYMBOLICLINK_OBJECT,
    MUTANT_OBJECT, SECTION_OBJECT, EVENT_OBJECT, SEMAPHORE_OBJECT,
    TIMER_OBJECT, KEY_OBJECT, EVENTPAIR_OBJECT, IOCOMPLETION_OBJECT,
    DEVICE_OBJECT, FILE_OBJECT, CONTROLLER_OBJECT, PROFILE_OBJECT,
    TYPE_OBJECT, DESKTOP_OBJECT, WINDOWSTATION_OBJECT, DRIVER_OBJECT,
    TOKEN_OBJECT, PROCESS_OBJECT, THREAD_OBJECT, ADAPTER_OBJECT, PORT_OBJECT,

    UNKNOWN_OBJECT_TYPE = -1
};
extern const LPCWSTR ObjectTypeNames[];

#define NT_OBJECT_PIDL_MAGIC (USHORT)0x9A03
#define REGISTRY_PIDL_MAGIC (USHORT)0x5364

#include <pshpack1.h>

// NT OBJECT browser
struct NtPidlEntry
{
    USHORT cb;
    USHORT magic; // 0x9A03 ~~~ "NTOB"

    // If this is -1, there will be a NtPidlTypeData following this, and before any other extensions
    OBJECT_TYPE objectType;

    USHORT entryNameLength;
    WCHAR entryName[ANYSIZE_ARRAY];
};

struct NtPidlTypeData
{
    USHORT typeNameLength;
    WCHAR typeName[ANYSIZE_ARRAY];
};

// REGISTRY browser
enum REG_ENTRY_TYPE
{
    REG_ENTRY_ROOT,
    REG_ENTRY_KEY,
    REG_ENTRY_VALUE,
    REG_ENTRY_VALUE_WITH_CONTENT
    // any more?
};
extern const LPCWSTR RegistryTypeNames [];

struct RegPidlEntry
{
    USHORT cb;
    USHORT magic; // 0x5364 ~~~ "REGK"

    REG_ENTRY_TYPE entryType;

    USHORT entryNameLength;

    union {
        struct {
            // For Value entries, this contains the value contents, if it's reasonably small.
            // For Key entries, this contains the custom class name
            DWORD contentType;
            USHORT contentsLength;
        };

        HKEY rootKey;
    };

    WCHAR entryName[ANYSIZE_ARRAY];

};


#include <poppack.h>

HRESULT ReadRegistryValue(HKEY root, PCWSTR path, PCWSTR valueName, PVOID * valueData, PDWORD valueLength);

HRESULT GetEnumRegistryRoot(IEnumIDList ** ppil);
HRESULT GetEnumRegistryKey(LPCWSTR path, HKEY root, IEnumIDList ** ppil);
HRESULT GetEnumNTDirectory(LPCWSTR path, IEnumIDList ** ppil);

HRESULT GetNTObjectSymbolicLinkTarget(LPCWSTR path, LPCWSTR entryName, PUNICODE_STRING LinkTarget);