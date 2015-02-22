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

extern "C" {
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <ndk/rtlfuncs.h>
}

// All the possible values are defined here because I want the type field to be 
// "persistable" and not change if more types are added in the future.
enum OBJECT_TYPE {
    DIRECTORY_OBJECT, SYMBOLICLINK_OBJECT,
    MUTANT_OBJECT, SECTION_OBJECT, EVENT_OBJECT, SEMAPHORE_OBJECT,
    TIMER_OBJECT, KEY_OBJECT, EVENTPAIR_OBJECT, IOCOMPLETITION_OBJECT,
    DEVICE_OBJECT, FILE_OBJECT, CONTROLLER_OBJECT, PROFILE_OBJECT,
    TYPE_OBJECT, DESKTOP_OBJECT, WINDOWSTATION_OBJECT, DRIVER_OBJECT,
    TOKEN_OBJECT, PROCESS_OBJECT, THREAD_OBJECT, ADAPTER_OBJECT, PORT_OBJECT,

    UNKNOWN_OBJECT_TYPE = -1
};
extern const LPCWSTR ObjectTypeNames [];

#define NT_OBJECT_PIDL_MAGIC (USHORT)0x9A03

#include <pshpack1.h>
struct NtPidlEntry
{
    USHORT cb;
    USHORT magic; // 0x9A03 ~~~ "NTOB"

    // If this is -1, there will be a NtPidlTypeData following this, and before any other extensions
    OBJECT_TYPE	objectType;

    OBJECT_BASIC_INFORMATION objectInformation;

    USHORT entryNameLength;
    WCHAR entryName[0];

};

struct NtPidlTypeData
{
    USHORT typeNameLength;
    WCHAR typeName[0];
};

struct NtPidlSymlinkData
{
    USHORT targetNameLength;
    WCHAR targetName[0];
};
#include <poppack.h>

HRESULT EnumerateNtDirectory(HDPA hdpa, PCWSTR path, UINT * hdpaCount);