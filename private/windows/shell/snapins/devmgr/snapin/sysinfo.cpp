/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    sysinfo.cpp

Abstract:

    This module implements CSystemInfo, the class that returns various
    system information

Author:

    William Hsieh (williamh) created

Revision History:


--*/


#include "devmgr.h"
#include "sysinfo.h"

// disk drive root template name. Used to retreive the disk's media
// information or geometry
const TCHAR* const DRIVE_ROOT = TEXT("\\\\.\\?:");
const int DRIVE_LETTER_IN_DRIVE_ROOT = 4;

// disk drive root directory template name. Used to retreive the disk's
// total and free space
const TCHAR* const DRIVE_ROOT_DIR = TEXT("?:\\");
const int DRIVE_LETTER_IN_DRIVE_ROOT_DIR = 0;

//
// Registry various subkey and value names used to retreive
// system information
//
const TCHAR* const REG_PATH_HARDWARE_SYSTEM = TEXT("HARDWARE\\DESCRIPTION\\System");
const TCHAR* const REG_VALUE_SYSTEMBIOSNAME = TEXT("SystemBiosName");
const TCHAR* const REG_VALUE_SYSTEMBIOSDATE = TEXT("SystemBiosDate");
const TCHAR* const REG_VALUE_SYSTEMBIOSVERSION = TEXT("SystemBiosVersion");
const TCHAR* const REG_VALUE_MACHINETYPE  = TEXT("Identifier");
const TCHAR* const REG_VALUE_PROCESSOR_SPEED = TEXT("~MHZ");

const TCHAR* const REG_PATH_WINDOWS_NT = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
const TCHAR* const REG_VALUE_REGISTERED_OWNER = TEXT("RegisteredOwner");
const TCHAR* const REG_VALUE_REGISTERED_ORGANIZATION = TEXT("RegisteredOrganization");
const TCHAR* const REG_VALUE_BUID_TYPE  = TEXT("CurrentType");
const TCHAR* const REG_VALUE_SYSTEMROOT = TEXT("SystemRoot");
const TCHAR* const REG_VALUE_INSTALLDATE = TEXT("InstallDate");
const TCHAR* const REG_VALUE_CURRENTBUILDNUMBER = TEXT("CurrentBuildNumber");
const TCHAR* const REG_VALUE_CURRENTVERSION = TEXT("CurrentVersion");
const TCHAR* const REG_VALUE_CSDVERSION = TEXT("CSDVersion");
const TCHAR* const REG_PATH_CPU = TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor");
const TCHAR* const REG_VALUE_CPU_TYPE = TEXT("Identifier");
const TCHAR* const REG_VALUE_CPU_SPEED = TEXT("~MHz");
const TCHAR* const REG_VALUE_CPU_VENDOR = TEXT("VendorIdentifier");

CSystemInfo::CSystemInfo(
    CMachine* pMachine
    )
{
    // assuming the machine is a local machine and initialize
    // the registry root key as well.
    m_hKeyMachine = HKEY_LOCAL_MACHINE;

    if (pMachine)
    {
    m_fLocalMachine = pMachine->IsLocal();
    m_strMachineName = TEXT("\\\\");
    m_strMachineName += pMachine->GetMachineDisplayName();
    }
    else
    {
    // local machine
    m_fLocalMachine = TRUE;
    m_strMachineName.GetComputerName();
    }
    if (!m_fLocalMachine)
    {
    // The machine is not local, connect to the registry
    TCHAR ComputerName[MAX_PATH];
    // the api requires an LPTSTR instead of LPCTSTR!!!!
    lstrcpy(ComputerName, (LPCTSTR)m_strMachineName);
    m_hKeyMachine = NULL;
    RegConnectRegistry(ComputerName, HKEY_LOCAL_MACHINE, &m_hKeyMachine);
    }
}

CSystemInfo::~CSystemInfo()
{
    if (!m_fLocalMachine && NULL != m_hKeyMachine)
    {
    RegCloseKey(m_hKeyMachine);
    // disconnect the machine
    WNetCancelConnection2(TEXT("\\server\\ipc$"), 0, TRUE);
    }
}
//
// This function gets the disk information about the given disk drive
// INPUT:
//  Drive -- the drive number. 0 for A, 1 for B and etc.
//  DiskInfo -- the DISK_INFO to be filled with the information about
//          the drive. DiskInfo.cbSize must be initialized before
//          the call.
// OUTPUT:
//  TRUE  -- if succeeded, DiskInfo is filled with information
//  FALSE -- if the drive information can not be retreived.
//       No appropriate error code is returned;
BOOL
CSystemInfo::GetDiskInfo(
    int  Drive,
    DISK_INFO& DiskInfo
    )
{

    // diskinfo only valid on local computer
    if (!m_fLocalMachine)
    return FALSE;
    TCHAR DriveLetter;
    TCHAR Root[MAX_PATH];
    DriveLetter = _T('A') + Drive;
    lstrcpy(Root, DRIVE_ROOT_DIR);
    Root[DRIVE_LETTER_IN_DRIVE_ROOT_DIR] = DriveLetter;
    UINT DriveType;
    DriveType = GetDriveType(Root);
    //
    // only valid for local drives
    //
    if (DRIVE_UNKNOWN == DriveType || DRIVE_REMOTE == DriveType ||
    DRIVE_NO_ROOT_DIR == DriveType)
    {
    return FALSE;
    }
    if (DiskInfo.cbSize < sizeof(DISK_INFO))
    {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
    }
    //
    // form the disk root name from template
    //
    lstrcpy(Root, DRIVE_ROOT);
    Root[DRIVE_LETTER_IN_DRIVE_ROOT] = DriveLetter;
    HANDLE hDisk;
    // FILE_READ_ATTRIBUTES is used here so that we will not get nasty
    // error or prompt if the disk is a removable drive and there is no
    // media available.
    hDisk = CreateFile(Root,
               FILE_READ_ATTRIBUTES | SYNCHRONIZE,
               FILE_SHARE_READ | FILE_SHARE_WRITE,
               NULL,
               OPEN_EXISTING,
               0,
               NULL);
    if (INVALID_HANDLE_VALUE != hDisk)
    {
    // form the disk root directory name from template
    lstrcpy(Root, DRIVE_ROOT_DIR);
    Root[DRIVE_LETTER_IN_DRIVE_ROOT_DIR] = DriveLetter;
    BYTE Buffer[512];
    DWORD BytesRequired = 0;
    if (DeviceIoControl(hDisk, IOCTL_STORAGE_GET_MEDIA_TYPES_EX, NULL, 0,
                Buffer, sizeof(Buffer), &BytesRequired, NULL))
    {
        GET_MEDIA_TYPES* pMediaList;
        DEVICE_MEDIA_INFO*  pMediaInfo;
        pMediaList = (GET_MEDIA_TYPES*)Buffer;
        pMediaInfo = pMediaList->MediaInfo;
        DWORD MediaCount = pMediaList->MediaInfoCount;
        ULARGE_INTEGER MaxSpace, NewSpace;
        DEVICE_MEDIA_INFO* pMaxMediaInfo;
        MaxSpace.QuadPart = 0;
        pMaxMediaInfo = NULL;
        for (DWORD i = 0; i < MediaCount; i++, pMediaInfo++)
        {

        // find the mediainfo which has max space
        // A disk drive may support multiple media types and the
        // one with maximum capacity is what we want to report.
        if (DRIVE_REMOVABLE == DriveType || DRIVE_CDROM == DriveType)
        {
            NewSpace.QuadPart =
            pMediaInfo->DeviceSpecific.RemovableDiskInfo.BytesPerSector *
            pMediaInfo->DeviceSpecific.RemovableDiskInfo.SectorsPerTrack *
            pMediaInfo->DeviceSpecific.RemovableDiskInfo.TracksPerCylinder*
            pMediaInfo->DeviceSpecific.RemovableDiskInfo.Cylinders.QuadPart;

        }
        else
        {
            NewSpace.QuadPart =
            pMediaInfo->DeviceSpecific.DiskInfo.BytesPerSector *
            pMediaInfo->DeviceSpecific.DiskInfo.SectorsPerTrack *
            pMediaInfo->DeviceSpecific.DiskInfo.TracksPerCylinder *
            pMediaInfo->DeviceSpecific.DiskInfo.Cylinders.QuadPart;

        }
        if (NewSpace.QuadPart > MaxSpace.QuadPart)
        {
            MaxSpace.QuadPart = NewSpace.QuadPart;
            pMaxMediaInfo = pMediaInfo;
        }
        }
        if (pMaxMediaInfo)
        {
        // a valid media information is found, compose  DISK_INFO
        // from the media information
        //
        DiskInfo.DriveType = DriveType;
        if (DRIVE_REMOVABLE == DriveType || DRIVE_CDROM == DriveType)
        {
            DiskInfo.MediaType = pMaxMediaInfo->DeviceSpecific.RemovableDiskInfo.MediaType;
            DiskInfo.Cylinders = pMaxMediaInfo->DeviceSpecific.RemovableDiskInfo.Cylinders;
            DiskInfo.Heads = pMaxMediaInfo->DeviceSpecific.RemovableDiskInfo.TracksPerCylinder;
            DiskInfo.BytesPerSector = pMaxMediaInfo->DeviceSpecific.RemovableDiskInfo.BytesPerSector;
            DiskInfo.SectorsPerTrack = pMaxMediaInfo->DeviceSpecific.RemovableDiskInfo.SectorsPerTrack;
            // Do not call GetDiskFreeSpaceEx on removable disk
            // or CD-ROM
            DiskInfo.TotalSpace = MaxSpace;
            DiskInfo.FreeSpace.QuadPart = -1;
        }
        else
        {
            DiskInfo.MediaType = pMaxMediaInfo->DeviceSpecific.DiskInfo.MediaType;
            DiskInfo.Cylinders = pMaxMediaInfo->DeviceSpecific.DiskInfo.Cylinders;
            DiskInfo.Heads = pMaxMediaInfo->DeviceSpecific.DiskInfo.TracksPerCylinder;
            DiskInfo.BytesPerSector = pMaxMediaInfo->DeviceSpecific.DiskInfo.BytesPerSector;
            DiskInfo.SectorsPerTrack = pMaxMediaInfo->DeviceSpecific.DiskInfo.SectorsPerTrack;
            lstrcpy(Root, DRIVE_ROOT_DIR);
            Root[DRIVE_LETTER_IN_DRIVE_ROOT_DIR] = DriveLetter;
            ULARGE_INTEGER FreeSpaceForCaller;
            if (!GetDiskFreeSpaceEx(Root, &FreeSpaceForCaller, &DiskInfo.TotalSpace, &DiskInfo.FreeSpace))
            {
            DiskInfo.TotalSpace = MaxSpace;
            // unknown
            DiskInfo.FreeSpace.QuadPart = -1;
            }
        }
        CloseHandle(hDisk);
        return TRUE;
        }

    }
    // we wouldn't go here if the drive is not  removable.
    // Basically, this is for floppy drives only.
    if (DRIVE_REMOVABLE == DriveType &&
        DeviceIoControl(hDisk, IOCTL_DISK_GET_MEDIA_TYPES, NULL, 0,
                Buffer, sizeof(Buffer), &BytesRequired, NULL))
    {
        int TotalMediaTypes = BytesRequired / sizeof(DISK_GEOMETRY);
        DISK_GEOMETRY* pGeometry;
        pGeometry = (DISK_GEOMETRY*)Buffer;
        ULARGE_INTEGER MaxSpace;
        ULARGE_INTEGER NewSpace;
        MaxSpace.QuadPart = 0;
        DISK_GEOMETRY* pMaxGeometry = NULL;
        for (int i = 0; i < TotalMediaTypes; i++, pGeometry++)
        {
        // find the geometry with maximum capacity
        //
        NewSpace.QuadPart = pGeometry->BytesPerSector *
                    pGeometry->SectorsPerTrack *
                    pGeometry->TracksPerCylinder *
                    pGeometry->Cylinders.QuadPart;
        if (NewSpace.QuadPart > MaxSpace.QuadPart)
        {
            pMaxGeometry = pGeometry;
            MaxSpace = NewSpace;
        }
        }
        if (pMaxGeometry)
        {
        DiskInfo.DriveType = DriveType;
        DiskInfo.MediaType = (STORAGE_MEDIA_TYPE)pMaxGeometry->MediaType;
        DiskInfo.Cylinders = pMaxGeometry->Cylinders;
        DiskInfo.Heads = pMaxGeometry->TracksPerCylinder;
        DiskInfo.BytesPerSector = pMaxGeometry->BytesPerSector;
        DiskInfo.SectorsPerTrack = pMaxGeometry->SectorsPerTrack;
        DiskInfo.TotalSpace = MaxSpace;
        DiskInfo.FreeSpace.QuadPart = -1;
        CloseHandle(hDisk);
        return TRUE;
        }
    }
    CloseHandle(hDisk);
    }
    return FALSE;
}


//
// This functions retreive the Window version information in text string
// INPUT:
//  Buffer      -- buffer to receive the text string
//  BufferSize  -- buffer size in char(in bytes on ANSI version)
// OUTPUT:
//  The size of the text string, not including the terminated NULL char
//  If the returned value is 0, GetLastError will returns the error code.
//  If the returned value is larger than BufferSize, Buffer is too small
//
DWORD
CSystemInfo::WindowsVersion(
    TCHAR* Buffer,
    DWORD  BufferSize
    )
{
    if (!Buffer && BufferSize)
    {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
    }
    TCHAR FinalText[1024];
    TCHAR Temp[LINE_LEN];
    TCHAR Build[128];

    LoadString(g_hInstance, IDS_WINDOWS_NT, FinalText, ARRAYLEN(FinalText));
    LoadString(g_hInstance, IDS_BUILD_NUMBER, Build, ARRAYLEN(Build));

    // the final version string will be:
    // Windows NT n.m [Service Pack 3] (Build xxyy)
    //
#if 0
    if (m_fLocalMachine)
    {
    // for local machine, get the version information from API
    OSVERSIONINFO   OsVerInfo;
    OsVerInfo.dwOSVersionInfoSize = sizeof(OsVerInfo);
    if (GetVersionEx(&OsVerInfo))
    {
        wsprintf(Temp, TEXT(" %d.%d"), OsVerInfo.dwMajorVersion,
             OsVerInfo.dwMinorVersion);
        lstrcat(FinalText, Temp);
        // if there is a service pack string, append it.
        if (_T('\0') != OsVerInfo.szCSDVersion[0])
        {
        lstrcat(FinalText, TEXT(" "));
        lstrcat(FinalText, OsVerInfo.szCSDVersion);
        }
        DWORD FinalLen = lstrlen(FinalText);
        TCHAR Number[16];
        // convert build number to string
        wsprintf(Number, TEXT("%d"), OsVerInfo.dwBuildNumber);
        //merge build number to build string
        wsprintf(FinalText + FinalLen, Build, Number);
    }
    }
    else
#endif
    {
    CSafeRegistry regWindowsNT;
    if (regWindowsNT.Open(m_hKeyMachine, REG_PATH_WINDOWS_NT, KEY_READ))
    {
        DWORD Type, Size;
        Size = sizeof(Temp);
        if (regWindowsNT.GetValue(REG_VALUE_CURRENTVERSION, &Type,
                       (PBYTE)Temp, &Size))
        {
        lstrcat(FinalText, Temp);
        }
        Size = sizeof(Temp);
        if (regWindowsNT.GetValue(REG_VALUE_CSDVERSION, &Type,
                       (PBYTE)Temp, &Size) && Size)
        {
        lstrcat(FinalText, TEXT(" "));
        lstrcat(FinalText, Temp);
        }
        Size = sizeof(Temp);
        if (regWindowsNT.GetValue(REG_VALUE_CURRENTBUILDNUMBER, &Type,
                       (PBYTE)Temp, &Size) && Size)
        {
        DWORD FinalLen = lstrlen(FinalText);
        wsprintf(FinalText + FinalLen, Build, Temp);
        }
    }
    }
    DWORD FinalLen = lstrlen(FinalText);
    if (BufferSize > FinalLen)
    {
    lstrcpyn(Buffer, FinalText, FinalLen + 1);
    SetLastError(ERROR_SUCCESS);
    }
    else
    {
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }
    return FinalLen;
}



//
// This functions retreive a REG_SZ from the registry
// INPUT:
//  SubkeyName -- registry subkey name.
//  ValueName  -- registry value name;
//  Buffer      -- buffer to receive the string
//  BufferSize  -- buffer size in char(in bytes on ANSI version)
//  hKeyAncestory -- the key under which Subkeyname should be opened.
//
// OUTPUT:
//  The size of the text string, not including the terminated NULL char
//  If the returned value is 0, GetLastError will returns the error code.
//  If the returned value is larger than BufferSize, Buffer is too small
//
DWORD
CSystemInfo::InfoFromRegistry(
    LPCTSTR SubkeyName,
    LPCTSTR ValueName,
    TCHAR* Buffer,
    DWORD BufferSize,
    HKEY    hKeyAncestor
    )
{
    // validate parameters
    if (!SubkeyName || !ValueName || _T('\0') == *SubkeyName ||
    _T('\0') == *SubkeyName || (!Buffer && BufferSize))
    {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
    }
    if (!hKeyAncestor)
    hKeyAncestor = m_hKeyMachine;

    CSafeRegistry regSubkey;
    if (regSubkey.Open(hKeyAncestor, SubkeyName))
    {
    TCHAR Temp[MAX_PATH];
    DWORD Type;
    DWORD Size;
    Size = sizeof(Temp);
    if (regSubkey.GetValue(ValueName, &Type, (PBYTE)Temp, &Size) && Size)
    {
        Size /= sizeof(TCHAR);
        if (BufferSize > Size)
        {
        lstrcpy(Buffer, Temp);
        }
        return Size;
    }
    }
    SetLastError(ERROR_SUCCESS);
    return 0;
}

//
// This functions retreive the system BIOS name information in text string
// INPUT:
//  Buffer      -- buffer to receive the text string
//  BufferSize  -- buffer size in char(in bytes on ANSI version)
// OUTPUT:
//  The size of the text string, not including the terminated NULL char
//  If the returned value is 0, GetLastError will returns the error code.
//  If the returned value is larger than BufferSize, Buffer is too small
//
DWORD
CSystemInfo::SystemBiosName(
    TCHAR* Buffer,
    DWORD BufferSize
    )
{
    SetLastError(ERROR_SUCCESS);
    return 0;
}


//
// This functions retreive the system BIOS date information in text string
// INPUT:
//  Buffer      -- buffer to receive the text string
//  BufferSize  -- buffer size in char(in bytes on ANSI version)
// OUTPUT:
//  The size of the text string, not including the terminated NULL char
//  If the returned value is 0, GetLastError will returns the error code.
//  If the returned value is larger than BufferSize, Buffer is too small
//
DWORD
CSystemInfo::SystemBiosDate(
    TCHAR* Buffer,
    DWORD BufferSize
    )
{
    return InfoFromRegistry(REG_PATH_HARDWARE_SYSTEM,
                REG_VALUE_SYSTEMBIOSDATE,
                Buffer, BufferSize);
}


//
// This functions retreive the system BIOS version information in text string
// INPUT:
//  Buffer      -- buffer to receive the text string
//  BufferSize  -- buffer size in char(in bytes on ANSI version)
// OUTPUT:
//  The size of the text string, not including the terminated NULL char
//  If the returned value is 0, GetLastError will returns the error code.
//  If the returned value is larger than BufferSize, Buffer is too small
//
DWORD
CSystemInfo::SystemBiosVersion(
    TCHAR* Buffer,
    DWORD BufferSize
    )
{
    return InfoFromRegistry(REG_PATH_HARDWARE_SYSTEM,
                REG_VALUE_SYSTEMBIOSVERSION,
                Buffer, BufferSize);

}

//
// This functions retreive the machine type  in text string
// INPUT:
//  Buffer      -- buffer to receive the text string
//  BufferSize  -- buffer size in char(in bytes on ANSI version)
// OUTPUT:
//  The size of the text string, not including the terminated NULL char
//  If the returned value is 0, GetLastError will returns the error code.
//  If the returned value is larger than BufferSize, Buffer is too small
//
DWORD
CSystemInfo::MachineType(
    TCHAR* Buffer,
    DWORD BufferSize
    )
{
    return InfoFromRegistry(REG_PATH_HARDWARE_SYSTEM,
                REG_VALUE_MACHINETYPE,
                Buffer, BufferSize);
}

//
// This functions retreive the registered owner name
// INPUT:
//  Buffer      -- buffer to receive the text string
//  BufferSize  -- buffer size in char(in bytes on ANSI version)
// OUTPUT:
//  The size of the text string, not including the terminated NULL char
//  If the returned value is 0, GetLastError will returns the error code.
//  If the returned value is larger than BufferSize, Buffer is too small
//
DWORD
CSystemInfo::RegisteredOwner(
    TCHAR* Buffer,
    DWORD  BufferSize
    )
{
    return InfoFromRegistry(REG_PATH_WINDOWS_NT,
                REG_VALUE_REGISTERED_OWNER,
                Buffer,
                BufferSize
                );
}

//
// This functions retreive the registered organization name
// INPUT:
//  Buffer      -- buffer to receive the text string
//  BufferSize  -- buffer size in char(in bytes on ANSI version)
// OUTPUT:
//  The size of the text string, not including the terminated NULL char
//  If the returned value is 0, GetLastError will returns the error code.
//  If the returned value is larger than BufferSize, Buffer is too small
//
DWORD
CSystemInfo::RegisteredOrganization(
    TCHAR* Buffer,
    DWORD  BufferSize
    )
{
    return InfoFromRegistry(REG_PATH_WINDOWS_NT,
                REG_VALUE_REGISTERED_ORGANIZATION,
                Buffer,
                BufferSize
                );
}

// This function resturns the number of processors on the computer
// INPUT:
//  NONE
// OUTPUT:
//  Number of processor.
//
DWORD
CSystemInfo::NumberOfProcessors()
{
    CSafeRegistry regCPU;
    DWORD CPUs = 0;
    if (regCPU.Open(m_hKeyMachine, REG_PATH_CPU, KEY_READ))
    {
    TCHAR SubkeyName[MAX_PATH + 1];
    DWORD SubkeySize = ARRAYLEN(SubkeyName);
    while (regCPU.EnumerateSubkey(CPUs, SubkeyName, &SubkeySize))
    {
        SubkeySize = ARRAYLEN(SubkeyName);
        CPUs++;
    }
    }
    return CPUs;
}

// This function returns the processor vendor in text string
// INPUT:
//  Buffer -- buffer to receive the string
//  BufferSize -- size of the buffer in char(bytes in ANSI)
// OUTPUT:
//  The size of the text string, not including the terminated NULL char
//  If the returned value is 0, GetLastError will returns the error code.
//  If the returned value is larger than BufferSize, Buffer is too small
//
//  The system assumes that all processor in the machine must
//  have the same type, therefore, this function does not take
//  processor number as a parameter.
DWORD
CSystemInfo::ProcessorVendor(
    TCHAR* Buffer,
    DWORD BufferSize
    )
{
    return ProcessorInfo(REG_VALUE_CPU_VENDOR, Buffer, BufferSize);
}

// This function returns the processor type in text string
// INPUT:
//  Buffer -- buffer to receive the string
//  BufferSize -- size of the buffer in char(bytes in ANSI)
// OUTPUT:
//  The size of the text string, not including the terminated NULL char
//  If the returned value is 0, GetLastError will returns the error code.
//  If the returned value is larger than BufferSize, Buffer is too small
//
//  The system assumes that all processor in the machine must
//  have the same type, therefore, this function does not take
//  processor number as a parameter.
DWORD
CSystemInfo::ProcessorType(
    TCHAR* Buffer,
    DWORD BufferSize
    )
{
    return ProcessorInfo(REG_VALUE_CPU_TYPE, Buffer, BufferSize);
}

DWORD
CSystemInfo::ProcessorInfo(
    LPCTSTR ValueName,
    TCHAR* Buffer,
    DWORD  BufferSize
    )
{
    if (!ValueName || (!Buffer && BufferSize))
    {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
    }

    CSafeRegistry regCPU;
    DWORD CPUIndex = 0;
    TCHAR CPUInfo[MAX_PATH];
    DWORD CPUInfoSize = 0;
    DWORD Type;
    if (regCPU.Open(m_hKeyMachine, REG_PATH_CPU, KEY_READ))
    {
    TCHAR CPUKey[MAX_PATH + 1];
    DWORD Size;
    Size = ARRAYLEN(CPUKey);
    // loop through all cpus until we find something interesting
    while (CPUInfoSize <= sizeof(TCHAR) &&
           regCPU.EnumerateSubkey(CPUIndex, CPUKey, &Size))
    {
        CSafeRegistry regTheCPU;
        if (regTheCPU.Open(regCPU, CPUKey, KEY_READ))
        {
        CPUInfoSize = sizeof(CPUInfo);
        regTheCPU.GetValue(ValueName, &Type, (PBYTE)CPUInfo, &CPUInfoSize);
        }
        CPUIndex++;
    }
    // CPUInfoSize != 0 means we find something
    //
    if (CPUInfoSize > sizeof(TCHAR))
    {
        CPUInfoSize = CPUInfoSize / sizeof(TCHAR) - 1;
        if (BufferSize > CPUInfoSize)
        {
        lstrcpyn(Buffer, CPUInfo, CPUInfoSize + 1);
        }
        return CPUInfoSize;
    }
    }
    return 0;
}

#if 0
DWORD
CSystemInfo::ProcessorType(
    TCHAR* Buffer,
    DWORD BufferSize
    )
{
    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);
    TCHAR Format[MAX_PATH];
    TCHAR CPUType[MAX_PATH];
    TCHAR Revision[128];
    switch (SysInfo.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        // decode the processor level and revision
        if (3 == SysInfo.wProcessorLevel ||
        4 == SysInfo.wProcessorLevel)
        {

        int StringId = (3 == SysInfo.wProcessorLevel) ?
                   IDS_CPU_INTEL_386 : IDS_CPU_INTEL_486;
        LoadString(g_hInstance, StringId, CPUType, ARRAYLEN(CPUType));

        if (SysInfo.wProcessorRevision & 0xFF00 == 0xFF00)
        {
            LoadString(g_hInstance, IDS_CPU_REVISION_MODEL_STEPPING,
                   Format, ARRAYLEN(Format));
            wsprintf(Revision, Format,
                 ((SysInfo.wProcessorRevision & 0x00F0) > 4) - 10,
                 SysInfo.wProcessorRevision & 0x000F);
        }
        else
        {
            LoadString(g_hInstance, IDS_CPU_REVISION_STEPPING,
                   Format, ARRAYLEN(Format));
            wsprintf(Revision, Format,
                 (SysInfo.wProcessorRevision >> 8) + _T('A'),
                 SysInfo.wProcessorRevision & 0x00FF);
        }
        }
        else if (5 == SysInfo.wProcessorLevel)
        {
        LoadString(g_hInstance, IDS_CPU_INTEL_PENTIUM, CPUType, ARRAYLEN(CPUType));
        LoadString(g_hInstance, IDS_CPU_REVISION_MODEL_STEPPING,
               Format, ARRAYLEN(Format));
        wsprintf(Revision, Format, SysInfo.wProcessorRevision >> 8,
             SysInfo.wProcessorRevision & 0x00FF);
        }
        lstrcat(CPUType, Revision);
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
        LoadString(g_hInstance, IDS_CPU_ALPHA, Format, ARRAYLEN(Format));
        wsprintf(CPUType, Format, SysInfo.wProcessorLevel);
        LoadString(g_hInstance, IDS_CPU_REVISION_ALPHA, Format,
               ARRAYLEN(Format));
        wsprintf(Revision, Format, (SysInfo.wProcessorRevision >> 8) + _T('A'),
             SysInfo.wProcessorRevision & 0x00FF);
        lstrcat(CPUType, Revision);
    default:
        CPUType[0] = _T('\0');
    }
    DWORD Size = lstrlen(CPUType);
    if (BufferSize > Size)
    lstrcpy(Buffer, CPUType);
    SetLastError(ERROR_SUCCESS);
    return Size;
}
#endif


//
// This function returns the total physical memeory in KB
// INPUT:
//  NONE
// OUTPUT:
//  Total Memory in KB
//
void
CSystemInfo::TotalPhysicalMemory(
    ULARGE_INTEGER& Size
    )
{
    if (m_fLocalMachine)
    {
    SYSTEM_BASIC_INFORMATION SysBasicInfo;
    NTSTATUS Status;
    Status = NtQuerySystemInformation(SystemBasicInformation,
                      (PVOID)&SysBasicInfo,
                      sizeof(SysBasicInfo),
                      NULL);
    if(NT_SUCCESS(Status))
    {
        Size.QuadPart = Int32x32To64(SysBasicInfo.PageSize,
                     SysBasicInfo.NumberOfPhysicalPages
                     );
    }
    else
    {
        MEMORYSTATUS MemoryStatus;
        GlobalMemoryStatus(&MemoryStatus);
        Size.LowPart = (ULONG)MemoryStatus.dwTotalPhys;
        Size.HighPart = 0;
    }
    }
    else
    {
    Size.QuadPart = 0;
    SetLastError(ERROR_SUCCESS);
    }
}
