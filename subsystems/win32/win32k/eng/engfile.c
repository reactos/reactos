/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engfile.c
 * PURPOSE:         File Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
EngDeleteFile(IN PWSTR pwszFileName)
{
    UNIMPLEMENTED;
	return FALSE;
}

NTSTATUS
APIENTRY
EngFileIoControl(IN PFILE_OBJECT FileObject,
                 IN ULONG IoControlCode,
                 IN PVOID InputBuffer,
                 IN SIZE_T InputBufferLength,
                 OUT PVOID OutputBuffer,
                 IN SIZE_T OutputBufferLength,
                 OUT PULONG Information)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
APIENTRY
EngFileWrite(IN PFILE_OBJECT FileObject,
             IN PVOID Buffer,
             IN SIZE_T Length,
             IN PSIZE_T BytesWritten)
{
    UNIMPLEMENTED;
}

BOOL
APIENTRY
EngGetFileChangeTime(IN HANDLE hFile,
                     OUT PLARGE_INTEGER ChangeTime)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngGetFilePath(IN HANDLE hFile,
               OUT WCHAR  (*pDest)[MAX_PATH+1])
{
    UNIMPLEMENTED;
	return FALSE;
}

HANDLE
APIENTRY
EngLoadImage(IN PWSTR pwszDriver)
{
    UNIMPLEMENTED;
	return NULL;
}

HANDLE
APIENTRY
EngLoadModule(IN PWSTR pwszModule)
{
    UNIMPLEMENTED;
	return NULL;
}

HANDLE
APIENTRY
EngLoadModuleForWrite(IN PWSTR pwszModule,
                      IN ULONG  cjSizeOfModule)
{
    UNIMPLEMENTED;
	return NULL;
}

VOID
APIENTRY
EngFreeModule(IN HANDLE hModule)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
EngUnloadImage(IN HANDLE hModule)
{
    UNIMPLEMENTED;
}

PVOID
APIENTRY
EngMapFile(IN PWSTR pwsz,
           IN ULONG  cjSize,
           OUT PULONG_PTR piFile)
{
    UNIMPLEMENTED;
	return NULL;
}
  
BOOL
APIENTRY
EngUnmapFile(IN ULONG_PTR  iFile)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngMapFontFile(ULONG_PTR  iFile,
               PULONG*  ppjBuf,
               PULONG  pcjBuf)
{
    UNIMPLEMENTED;
	return FALSE;
}
  
VOID
APIENTRY
EngUnmapFontFile(ULONG_PTR iFile)
{
    UNIMPLEMENTED;
}

BOOL
APIENTRY
EngMapFontFileFD(IN ULONG_PTR  iFile,
                 OUT PULONG*  ppjBuf,
                 OUT PULONG  pcjBuf)
{
    UNIMPLEMENTED;
	return FALSE;
}

PVOID
APIENTRY
EngMapModule(IN HANDLE hModule,
             OUT PULONG Size)
{
    UNIMPLEMENTED;
	return NULL;
}

VOID
APIENTRY
EngUnmapFontFileFD(IN ULONG_PTR iFile)
{
    UNIMPLEMENTED;
}
