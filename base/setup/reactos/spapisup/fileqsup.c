/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS GUI first stage setup application
 * FILE:            base/setup/lib/fileqsup.c
 * PURPOSE:         Interfacing with Setup* API File Queue support functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#include "reactos.h"

#define NDEBUG
#include <debug.h>

#ifndef WINE_SETUPAPI_CAB_EXTRACTION_IS_NOT_SLOW_ANYMORE

#define _USETUP_PCH_ // Disable USetup header inclusion

#define NTOS_MODE_USER
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>

#include <ntstrsafe.h>

#include "../../usetup/spapisup/cabinet.h"
#include "../../usetup/spapisup/cabinet.c"

#define SetupOpenFileQueue      _SetupOpenFileQueue
#define SetupCloseFileQueue     _SetupCloseFileQueue
#define SetupQueueDeleteW       _SetupQueueDeleteW
#define SetupQueueRenameW       _SetupQueueRenameW
#define SetupCommitFileQueueW   _SetupCommitFileQueueW

#include "../../usetup/spapisup/fileqsup.c"

#else

/* SETUP* API COMPATIBILITY FUNCTIONS ****************************************/

/* A simplified version of SetupQueueCopyW that wraps Cabinet support around */
BOOL
WINAPI
SpFileQueueCopy_NtToWin32(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR SourceRootPath,
    IN PCWSTR SourcePath OPTIONAL,
    IN PCWSTR SourceFileName,
    IN PCWSTR SourceDescription OPTIONAL,
    IN PCWSTR SourceCabinet OPTIONAL,
    IN PCWSTR SourceTagFile OPTIONAL,
    IN PCWSTR TargetDirectory,
    IN PCWSTR TargetFileName OPTIONAL,
    IN ULONG CopyStyle)
{
    WCHAR Win32SourceRootPath[MAX_PATH];
    WCHAR Win32TargetDirectory[MAX_PATH];

    /*
     * SpFileQueueCopy is called within setuplib with NT paths, however
     * the Win32 SetupQueueCopyW API only takes Win32 paths. We therefore
     * map the NT path to Win32 path and then call the Win32 API.
     */
    if (!ConvertNtPathToWin32Path(&SetupData.MappingList,
                                  Win32SourceRootPath,
                                  _countof(Win32SourceRootPath),
                                  SourceRootPath))
    {
        return FALSE;
    }
    /* SourcePath, SourceFileName and SourceCabinet are appended to SourceRootPath by the SetupApi function */

    if (!ConvertNtPathToWin32Path(&SetupData.MappingList,
                                  Win32TargetDirectory,
                                  _countof(Win32TargetDirectory),
                                  TargetDirectory))
    {
        return FALSE;
    }
    /* TargetFileName is appended to TargetDirectory by the SetupApi function */

    /*
     * Use the undocumented way of copying files from within a given cabinet file
     * *ONLY IF* the files do not already exist in the same directory where
     * the cabinet file resides!!
     */
    return SetupQueueCopyW(QueueHandle,
                           Win32SourceRootPath,
                           SourcePath,
                           SourceFileName,
    // Undocumented on MSDN is the fact that this parameter is mandatory *IF* one wants to take the TagFile into account!
                           L"ReactOS",
    // SourceTagFile -- Special behaviour: use cabinet file present in ArchiveDir path! The API does not check for a ".cab" extension.
                           SourceCabinet,
                           Win32TargetDirectory,
                           TargetFileName,
    // We choose to decompress the archive, so do NOT specify SP_COPY_NODECOMP !
                           SP_COPY_NOOVERWRITE /* | SP_COPY_SOURCE_ABSOLUTE | SP_COPY_SOURCEPATH_ABSOLUTE */
                           );
}

BOOL
WINAPI
SpFileQueueDelete_NtToWin32(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR PathPart1,
    IN PCWSTR PathPart2 OPTIONAL)
{
    WCHAR Win32PathPart1[MAX_PATH];

    /*
     * SpFileQueueDelete is called within setuplib with NT paths, however
     * the Win32 SetupQueueDeleteW API only takes Win32 paths. We therefore
     * map the NT path to Win32 path and then call the Win32 API.
     */
    if (!ConvertNtPathToWin32Path(&SetupData.MappingList,
                                  Win32PathPart1,
                                  _countof(Win32PathPart1),
                                  PathPart1))
    {
        return FALSE;
    }
    /* PathPart2 is appended to PathPart1 by the SetupApi function */

    return SetupQueueDeleteW(QueueHandle, Win32PathPart1, PathPart2);
}

BOOL
WINAPI
SpFileQueueRename_NtToWin32(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR SourcePath,
    IN PCWSTR SourceFileName OPTIONAL,
    IN PCWSTR TargetPath OPTIONAL,
    IN PCWSTR TargetFileName)
{
    WCHAR Win32SourcePath[MAX_PATH];
    WCHAR Win32TargetPath[MAX_PATH];

    /*
     * SpFileQueueRename is called within setuplib with NT paths, however
     * the Win32 SetupQueueRenameW API only takes Win32 paths. We therefore
     * map the NT path to Win32 path and then call the Win32 API.
     */
    if (!ConvertNtPathToWin32Path(&SetupData.MappingList,
                                  Win32SourcePath,
                                  _countof(Win32SourcePath),
                                  SourcePath))
    {
        return FALSE;
    }
    /* SourceFileName is appended to SourcePath by the SetupApi function */

    if (TargetPath)
    {
        if (!ConvertNtPathToWin32Path(&SetupData.MappingList,
                                      Win32TargetPath,
                                      _countof(Win32TargetPath),
                                      TargetPath))
        {
            return FALSE;
        }
    }
    /* TargetFileName is appended to TargetPath by the SetupApi function */

    return SetupQueueRenameW(QueueHandle,
                             Win32SourcePath,
                             SourceFileName,
                             TargetPath ? Win32TargetPath : NULL,
                             TargetFileName);
}


/* GLOBALS *******************************************************************/

SPFILE_EXPORTS SpFileExports =
{
    SetupOpenFileQueue,
    SetupCloseFileQueue,
    SpFileQueueCopy_NtToWin32,
    SpFileQueueDelete_NtToWin32,
    SpFileQueueRename_NtToWin32,
    SetupCommitFileQueueW
};

#endif

/* EOF */
