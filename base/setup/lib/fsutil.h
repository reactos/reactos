
#pragma once

#include <fmifs/fmifs.h>

typedef struct _FILE_SYSTEM
{
    PCWSTR FileSystemName;
    FORMATEX FormatFunc;
    CHKDSKEX ChkdskFunc;
} FILE_SYSTEM, *PFILE_SYSTEM;

PFILE_SYSTEM
GetRegisteredFileSystems(OUT PULONG Count);

PFILE_SYSTEM
GetFileSystemByName(
    // IN PFILE_SYSTEM_LIST List,
    IN PCWSTR FileSystemName);

struct _PARTENTRY; // Defined in partlist.h

PFILE_SYSTEM
GetFileSystem(
    // IN PFILE_SYSTEM_LIST FileSystemList,
    IN struct _PARTENTRY* PartEntry);


BOOLEAN
PreparePartitionForFormatting(
    IN struct _PARTENTRY* PartEntry,
    IN PFILE_SYSTEM FileSystem);


#if 0 // Not used anymore. This portion of code is actually called in format.c "FormatPartition" function...
#define HOST_FormatPartition      NATIVE_FormatPartition

BOOLEAN
HOST_FormatPartition(
    IN PFILE_SYSTEM FileSystem, // IN PFILE_SYSTEM_ITEM FileSystem,
    IN PCUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback);

#endif
