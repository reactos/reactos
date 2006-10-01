/*
 * PROJECT:         WinLoader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            freeldr/winldr/peloader.c
 * PURPOSE:         Provides routines for loading PE files. To be merged with
 *                  arch/i386/loader.c in future
 *                  This article was very handy during development:
 *                  http://msdn.microsoft.com/msdnmag/issues/02/03/PE2/
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 *                  The source code in this file is based on the work of respective
 *                  authors of PE loading code in ReactOS and Brian Palmer and
 *                  Alex Ionescu's arch/i386/loader.c, and my research project
 *                  (creating a native EFI loader for Windows)
 */

/* INCLUDES ***************************************************************/
#include <freeldr.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS **************************************************************/

BOOLEAN
WinLdrScanImportDescriptorTable(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                                IN PCCH DirectoryPath,
                                IN PLDR_DATA_TABLE_ENTRY ScanDTE)
{
	return FALSE;
}

BOOLEAN
WinLdrAllocateDataTableEntry(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                             IN PCCH BaseDllName,
                             IN PCCH FullDllName,
                             IN PVOID BasePA,
                             OUT PLDR_DATA_TABLE_ENTRY *NewEntry)
{
	return FALSE;
}

/* WinLdrLoadImage loads the specified image from the file (it doesn't
   perform any additional operations on the filename, just directly
   calls the file I/O routines), and relocates it so that it's ready
   to be used when paging is enabled.
   Addressing mode: physical
 */
BOOLEAN
WinLdrLoadImage(IN PCHAR FileName,
                OUT PVOID *ImageBasePA)
{
	return FALSE;
}

/* PRIVATE FUNCTIONS *******************************************************/
