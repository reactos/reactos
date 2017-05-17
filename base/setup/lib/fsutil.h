/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Filesystem support functions
 * COPYRIGHT:   Copyright 2002-2018 Eric Kohl
 *              Copyright 2003-2018 Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Copyright 2017-2018 Hermes Belusca-Maito
 */

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

/* EOF */
