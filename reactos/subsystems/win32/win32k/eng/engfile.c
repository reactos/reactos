/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engfile.c
 * PURPOSE:         File Support Routines
 * PROGRAMMERS:     
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

typedef struct _DRIVERS
{
    LIST_ENTRY ListEntry;
    HANDLE ImageHandle;
    UNICODE_STRING DriverName;
}DRIVERS, *PDRIVERS;

extern LIST_ENTRY GlobalDriverListHead;

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


/*
 * @implemented
 */
HANDLE
APIENTRY
EngLoadImage (LPWSTR DriverName)
{
    HANDLE hImageHandle = NULL;
    SYSTEM_GDI_DRIVER_INFORMATION GdiDriverInfo;
    NTSTATUS Status;

    RtlInitUnicodeString(&GdiDriverInfo.DriverName, DriverName);
    if( !IsListEmpty(&GlobalDriverListHead) )
    {
        PLIST_ENTRY CurrentEntry = GlobalDriverListHead.Flink;
        PDRIVERS Current;
        /* probably the driver was already loaded, let's try to find it out */
        while( CurrentEntry != &GlobalDriverListHead )
        {
            Current = CONTAINING_RECORD(CurrentEntry, DRIVERS, ListEntry);
            if( Current && (0 == RtlCompareUnicodeString(&GdiDriverInfo.DriverName, &Current->DriverName, FALSE)) ) {
                hImageHandle = Current->ImageHandle;
                break;
            }
            CurrentEntry = CurrentEntry->Flink;
        };
    }

    if( !hImageHandle )
    {
        /* the driver was not loaded before, so let's do that */
        Status = ZwSetSystemInformation(SystemLoadGdiDriverInformation, &GdiDriverInfo, sizeof(SYSTEM_GDI_DRIVER_INFORMATION));
        if (!NT_SUCCESS(Status)) {
            DPRINT1("ZwSetSystemInformation failed with Status 0x%lx\n", Status);
        }
        else {
            hImageHandle = (HANDLE)GdiDriverInfo.ImageAddress;
            PDRIVERS DriverInfo = ExAllocatePool(PagedPool, sizeof(DRIVERS));
            DriverInfo->DriverName.MaximumLength = GdiDriverInfo.DriverName.MaximumLength;
            DriverInfo->DriverName.Length = GdiDriverInfo.DriverName.Length;
            DriverInfo->DriverName.Buffer = ExAllocatePool(PagedPool, GdiDriverInfo.DriverName.MaximumLength);
            RtlCopyUnicodeString(&DriverInfo->DriverName, &GdiDriverInfo.DriverName);
            DriverInfo->ImageHandle = hImageHandle;
            InsertHeadList(&GlobalDriverListHead, &DriverInfo->ListEntry);
        }
    }

    return hImageHandle;
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
EngUnloadImage ( IN HANDLE hModule )
{
    NTSTATUS Status;

    DPRINT("hModule 0x%x\n", hModule);

    Status = ZwSetSystemInformation(SystemUnloadGdiDriverInformation,
        &hModule, sizeof(HANDLE));

    if(!NT_SUCCESS(Status))
    {
        DPRINT1("ZwSetSystemInformation failed with status 0x%08X\n",
        Status);
    }
    else
    {
        /* remove from the list */
        if( !IsListEmpty(&GlobalDriverListHead) )
        {
            PLIST_ENTRY CurrentEntry = GlobalDriverListHead.Flink;
            PDRIVERS Current;
            /* probably the driver was already loaded, let's try to find it out */
            while( CurrentEntry != &GlobalDriverListHead )
            {
                Current = CONTAINING_RECORD(CurrentEntry, DRIVERS, ListEntry);

                if( Current ) {
                    if(Current->ImageHandle == hModule) {
                        ExFreePool(Current->DriverName.Buffer);
                        RemoveEntryList(&Current->ListEntry);
                        ExFreePool(Current);
                        break;
                    }
                }
                CurrentEntry = CurrentEntry->Flink;
            };
        }
    }
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
