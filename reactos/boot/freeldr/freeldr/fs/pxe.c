/*
 *  FreeLoader
 *  Copyright (C) 2011  Hervé Poussineau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

#define TAG_PXE_FILE 'FexP'
#define NO_FILE ((ULONG)-1)

DBG_DEFAULT_CHANNEL(FILESYSTEM);

static IP4 _ServerIP = { 0, };
static ULONG _OpenFile = NO_FILE;
static CHAR _OpenFileName[128];
static ULONG _FileSize = 0;
static ULONG _FilePosition = 0;
static ULONG _PacketPosition = 0;
static UCHAR _Packet[4096];
static UCHAR* _CachedFile = NULL;
static ULONG _CachedLength = 0;

static PPXE
FindPxeStructure(VOID)
{
    PPXE Ptr;
    UCHAR Checksum;
    UCHAR i;

    /* Find the '!PXE' structure */
    Ptr = (PPXE)0xA0000;
    while ((ULONG)Ptr > 0x10000)
    {
        Ptr = (PPXE)((ULONG)Ptr - 0x10);

        /* Look for signature */
        if (memcmp(Ptr, "!PXE", 4) != 0)
            continue;

        /* Check size */
        if (Ptr->StructLength != sizeof(PXE))
            continue;

        /* Check checksum */
        Checksum = 0;
        for (i = 0; i < Ptr->StructLength; i++)
            Checksum += *((PUCHAR)Ptr + i);
        if (Checksum != 0)
            continue;

        TRACE("!PXE structure found at %p\n", Ptr);
        return Ptr;
    }

    return NULL;
}

static PPXE GetPxeStructure(VOID)
{
    static PPXE pPxe = NULL;
    static BOOLEAN bPxeSearched = FALSE;
    if (!bPxeSearched)
    {
        pPxe = FindPxeStructure();
        bPxeSearched = TRUE;
    }
    return pPxe;
}

extern PXENV_EXIT PxeCallApi(UINT16 Segment, UINT16 Offset, UINT16 Service, VOID *Parameter);
BOOLEAN CallPxe(UINT16 Service, PVOID Parameter)
{
    PPXE pxe;
    PXENV_EXIT exit;

    pxe = GetPxeStructure();
    if (!pxe)
        return FALSE;

    if (Service != PXENV_TFTP_READ)
    {
        // HACK: this delay shouldn't be necessary
        KeStallExecutionProcessor(100 * 1000); // 100 ms
        TRACE("PxeCall(0x%x, %p)\n", Service, Parameter);
    }

    exit = PxeCallApi(pxe->EntryPointSP.segment, pxe->EntryPointSP.offset, Service, Parameter);
    if (exit != PXENV_EXIT_SUCCESS)
    {
        ERR("PxeCall(0x%x, %p) failed with exit=%d status=0x%x\n",
                Service, Parameter, exit, *(PXENV_STATUS*)Parameter);
        return FALSE;
    }
    if (*(PXENV_STATUS*)Parameter != PXENV_STATUS_SUCCESS)
    {
        ERR("PxeCall(0x%x, %p) succeeded, but returned error status 0x%x\n",
                Service, Parameter, *(PXENV_STATUS*)Parameter);
        return FALSE;
    }
    return TRUE;
}

static ARC_STATUS PxeClose(ULONG FileId)
{
    t_PXENV_TFTP_CLOSE closeData;

    if (_OpenFile == NO_FILE || FileId != _OpenFile)
        return EBADF;

    RtlZeroMemory(&closeData, sizeof(closeData));
    if (!CallPxe(PXENV_TFTP_CLOSE, &closeData))
        return EIO;

    _OpenFile = NO_FILE;
    if (_CachedFile)
    {
        FrLdrTempFree(_CachedFile, TAG_PXE_FILE);
        _CachedFile = NULL;
    }
    return ESUCCESS;
}

static ARC_STATUS PxeGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    if (_OpenFile == NO_FILE || FileId != _OpenFile)
        return EBADF;

    RtlZeroMemory(Information, sizeof(*Information));
    Information->EndingAddress.LowPart = _FileSize;
    Information->CurrentAddress.LowPart = _FilePosition;

    return ESUCCESS;
}

static ARC_STATUS PxeOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    t_PXENV_TFTP_GET_FSIZE sizeData;
    t_PXENV_TFTP_OPEN openData;
    SIZE_T PathLen, i;

    if (_OpenFile != NO_FILE)
        return EIO;
    if (OpenMode != OpenReadOnly)
        return EACCES;

    /* Retrieve the path length without NULL terminator */
    PathLen = (Path ? min(strlen(Path), sizeof(_OpenFileName) - 1) : 0);

    /* Lowercase the path and always use slashes as separators */
    for (i = 0; i < PathLen; i++)
    {
        if (Path[i] == '\\')
            _OpenFileName[i] = '/';
        else
            _OpenFileName[i] = tolower(Path[i]);
    }

    /* Zero out rest of the file name */
    RtlZeroMemory(_OpenFileName + PathLen, sizeof(_OpenFileName) - PathLen);

    RtlZeroMemory(&sizeData, sizeof(sizeData));
    sizeData.ServerIPAddress = _ServerIP;
    RtlCopyMemory(sizeData.FileName, _OpenFileName, sizeof(_OpenFileName));
    if (!CallPxe(PXENV_TFTP_GET_FSIZE, &sizeData))
    {
        ERR("Failed to get '%s' size\n", Path);
        return EIO;
    }

    _FileSize = sizeData.FileSize;
    if (_FileSize < 1024 * 1024)
    {
        _CachedFile = FrLdrTempAlloc(_FileSize, TAG_PXE_FILE);
        // Don't check for allocation failure, we support _CachedFile == NULL
    }
    _CachedLength = 0;

    RtlZeroMemory(&openData, sizeof(openData));
    openData.ServerIPAddress = _ServerIP;
    RtlCopyMemory(openData.FileName, _OpenFileName, sizeof(_OpenFileName));
    openData.PacketSize = sizeof(_Packet);

    if (!CallPxe(PXENV_TFTP_OPEN, &openData))
    {
        if (_CachedFile)
        {
            FrLdrTempFree(_CachedFile, TAG_PXE_FILE);
            _CachedFile = NULL;
        }
        return ENOENT;
    }

    _FilePosition = 0;
    _PacketPosition = 0;

    _OpenFile = *FileId;
    return ESUCCESS;
}

static ARC_STATUS PxeRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    t_PXENV_TFTP_READ readData;
    ULONG i;

    *Count = 0;

    if (_OpenFile == NO_FILE || FileId != _OpenFile)
        return EBADF;

    RtlZeroMemory(&readData, sizeof(readData));
    readData.Buffer.segment = ((UINT32)_Packet & 0xf0000) / 16;
    readData.Buffer.offset = (UINT32)_Packet & 0xffff;

    // Get new packets as required
    while (N > 0)
    {
        if (N < _CachedLength - _FilePosition)
            i = N;
        else
            i = _CachedLength - _FilePosition;
        if (_CachedFile)
            RtlCopyMemory(Buffer, _CachedFile + _FilePosition, i);
        else
            RtlCopyMemory(Buffer, _Packet + _FilePosition - _PacketPosition, i);
        _FilePosition += i;
        Buffer = (UCHAR*)Buffer + i;
        *Count += i;
        N -= i;
        if (N == 0)
            break;

        if (!CallPxe(PXENV_TFTP_READ, &readData))
            return EIO;
        if (_CachedFile)
            RtlCopyMemory(_CachedFile + _CachedLength, _Packet, readData.BufferSize);
        _PacketPosition = _CachedLength;
        _CachedLength += readData.BufferSize;
    }

    return ESUCCESS;
}

static ARC_STATUS PxeSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    t_PXENV_TFTP_READ readData;

    if (_OpenFile == NO_FILE || FileId != _OpenFile)
        return EBADF;

    if (Position->HighPart != 0 || SeekMode != SeekAbsolute)
        return EINVAL;

    if (!_CachedFile && Position->LowPart < _FilePosition)
    {
        // Close and reopen the file to go to position 0
        if (PxeClose(FileId) != ESUCCESS)
            return EIO;
        if (PxeOpen(_OpenFileName, OpenReadOnly, &FileId) != ESUCCESS)
            return EIO;
    }

    RtlZeroMemory(&readData, sizeof(readData));
    readData.Buffer.segment = ((UINT32)_Packet & 0xf0000) / 16;
    readData.Buffer.offset = (UINT32)_Packet & 0xffff;

    // Get new packets as required
    while (Position->LowPart > _CachedLength)
    {
        if (!CallPxe(PXENV_TFTP_READ, &readData))
            return EIO;
        if (_CachedFile)
        {
            RtlCopyMemory(_CachedFile + _CachedLength, _Packet, readData.BufferSize);
        }
        _PacketPosition = _CachedLength;
        _CachedLength += readData.BufferSize;
    }

    _FilePosition = Position->LowPart;
    return ESUCCESS;
}

static const DEVVTBL PxeVtbl = {
    PxeClose,
    PxeGetFileInformation,
    PxeOpen,
    PxeRead,
    PxeSeek,
};

const DEVVTBL* PxeMount(ULONG DeviceId)
{
    if (GetPxeStructure() == NULL)
        return NULL;
    return &PxeVtbl;
}

static ARC_STATUS PxeDiskClose(ULONG FileId)
{
    // Nothing to do
    return ESUCCESS;
}

static ARC_STATUS PxeDiskGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    // No disk access in PXE mode
    return EINVAL;
}

static ARC_STATUS PxeDiskOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    // Nothing to do
    return ESUCCESS;
}

static ARC_STATUS PxeDiskRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    // No disk access in PXE mode
    return EINVAL;
}

static ARC_STATUS PxeDiskSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    // No disk access in PXE mode
    return EINVAL;
}

static const DEVVTBL PxeDiskVtbl = {
    PxeDiskClose,
    PxeDiskGetFileInformation,
    PxeDiskOpen,
    PxeDiskRead,
    PxeDiskSeek,
};

static BOOLEAN GetCachedInfo(VOID)
{
    t_PXENV_GET_CACHED_INFO Data;
    BOOLEAN res;
    UCHAR* Packet;

    RtlZeroMemory(&Data, sizeof(Data));
    Data.PacketType = PXENV_PACKET_TYPE_CACHED_REPLY;

    res = CallPxe(PXENV_GET_CACHED_INFO, &Data);
    if (!res)
        return FALSE;
    if (Data.BufferSize < 36)
        return FALSE;
    Packet = (UCHAR*)((UINT32)(Data.Buffer.segment << 4) + Data.Buffer.offset);
    RtlCopyMemory(&_ServerIP, Packet + 20, sizeof(IP4));
    return TRUE;
}

BOOLEAN PxeInit(VOID)
{
    static BOOLEAN Initialized = FALSE;
    static BOOLEAN Success = FALSE;

    // Do initialization only once
    if (Initialized)
        return Success;
    Initialized = TRUE;

    // Check if PXE is available
    if (GetPxeStructure() && GetCachedInfo())
    {
        FsRegisterDevice("net(0)", &PxeDiskVtbl);
        Success = TRUE;
    }

    return Success;
}

