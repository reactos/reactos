/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for mapping files and sections
 * FILE:              subsys/win32k/eng/device.c
 * PROGRAMER:
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

HANDLE
APIENTRY
EngLoadModule(LPWSTR pwsz)
{
    UNIMPLEMENTED;
    return NULL;
}

HANDLE
APIENTRY
EngLoadModuleForWrite(
	IN LPWSTR pwsz,
	IN ULONG  cjSizeOfModule)
{
    // www.osr.com/ddk/graphics/gdifncs_98rr.htm
    UNIMPLEMENTED;
    return NULL;
}

PVOID
APIENTRY
EngMapModule(
	IN  HANDLE h,
	OUT PULONG pulSize)
{
    // www.osr.com/ddk/graphics/gdifncs_9b1j.htm
    UNIMPLEMENTED;
    return NULL;
}

VOID
APIENTRY
EngFreeModule (IN HANDLE h)
{
    // www.osr.com/ddk/graphics/gdifncs_9fzb.htm
    UNIMPLEMENTED;
}

PVOID
APIENTRY
EngMapFile(
    IN LPWSTR pwsz,
    IN ULONG cjSize,
    OUT ULONG_PTR *piFile)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
EngUnmapFile(
    IN ULONG_PTR iFile)
{
    UNIMPLEMENTED;
    return FALSE;
}


BOOL
APIENTRY
EngMapFontFileFD(
	IN  ULONG_PTR iFile,
	OUT PULONG    *ppjBuf,
	OUT ULONG     *pcjBuf)
{
    // www.osr.com/ddk/graphics/gdifncs_0co7.htm
    UNIMPLEMENTED;
    return FALSE;
}

VOID
APIENTRY
EngUnmapFontFileFD(
    IN ULONG_PTR iFile)
{
    // http://www.osr.com/ddk/graphics/gdifncs_6wbr.htm
    UNIMPLEMENTED;
}

BOOL
APIENTRY
EngMapFontFile(
	ULONG_PTR iFile,
	PULONG    *ppjBuf,
	ULONG     *pcjBuf)
{
    // www.osr.com/ddk/graphics/gdifncs_3up3.htm
    return EngMapFontFileFD(iFile, ppjBuf, pcjBuf);
}

VOID
APIENTRY
EngUnmapFontFile(
    IN ULONG_PTR iFile)
{
    // www.osr.com/ddk/graphics/gdifncs_09wn.htm
    EngUnmapFontFileFD(iFile);
}


BOOLEAN
APIENTRY
EngMapSection(IN PVOID Section,
              IN BOOLEAN Map,
              IN HANDLE Process,
              IN PVOID* BaseAddress)
{
    UNIMPLEMENTED;
    return FALSE;
}

PVOID
APIENTRY
EngAllocSectionMem(IN PVOID SectionObject,
                   IN ULONG Flags,
                   IN SIZE_T MemSize,
                   IN ULONG Tag)
{
    UNIMPLEMENTED;
    return NULL;
}


BOOLEAN
APIENTRY
EngFreeSectionMem(IN PVOID SectionObject OPTIONAL,
                  IN PVOID MappedBase)
{
    UNIMPLEMENTED;
    return FALSE;
}
