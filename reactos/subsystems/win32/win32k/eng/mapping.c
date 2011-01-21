/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for mapping files and sections
 * FILE:              subsys/win32k/eng/device.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

// HACK!!!
#define MmMapViewInSessionSpace MmMapViewInSystemSpace
#define MmUnmapViewInSessionSpace MmUnmapViewInSystemSpace

typedef struct _ENGSECTION
{
    PVOID pvSectionObject;
    PVOID pvMappedBase;
    SIZE_T cjViewSize;
    ULONG ulTag;
} ENGSECTION, *PENGSECTION;

typedef struct _FILEVIEW
{
    LARGE_INTEGER  LastWriteTime;  
    PVOID          pvKView;
    PVOID          pvViewFD;  
    SIZE_T         cjView;  
    PVOID          pSection;  
} FILEVIEW, *PFILEVIEW;   

typedef struct _FONTFILEVIEW 
{
    FILEVIEW;
    DWORD          reserved[2];
    PWSTR          pwszPath;
    SIZE_T         ulRegionSize;
    ULONG          cKRefCount;
    ULONG          cRefCountFD;
    PVOID          pvSpoolerBase;
    DWORD          dwSpoolerPid;
} FONTFILEVIEW, *PFONTFILEVIEW;

enum
{
    FVF_SYSTEMROOT = 1,
    FVF_READONLY = 2,
    FVF_FONTFILE = 4,
};

HANDLE ghSystem32Directory;
HANDLE ghRootDirectory;


PVOID
NTAPI
EngCreateSection(
    IN ULONG fl,
    IN SIZE_T cjSize,
    IN ULONG ulTag)
{
    NTSTATUS Status;
    PENGSECTION pSection;
    PVOID pvSectionObject;
    LARGE_INTEGER liSize;

    /* Allocate a section object */
    pSection = EngAllocMem(0, sizeof(ENGSECTION), 'stsU');
    if (!pSection) return NULL;

    liSize.QuadPart = cjSize;
    Status = MmCreateSection(&pvSectionObject,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &liSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL,
                             NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create a section Status=0x%x\n", Status);
        EngFreeMem(pSection);
        return NULL;
    }

    /* Set the fields of the section */
    pSection->ulTag = ulTag;
    pSection->pvSectionObject = pvSectionObject;
    pSection->pvMappedBase = NULL;
    pSection->cjViewSize = cjSize;

    return pSection;
}


BOOL
APIENTRY
EngMapSection(
    IN PVOID pvSection,
    IN BOOL bMap,
    IN HANDLE hProcess,
    OUT PVOID* pvBaseAddress)
{
    NTSTATUS Status;
    PENGSECTION pSection = pvSection;
    PEPROCESS pepProcess;

    /* Get a pointer to the process */
    Status = ObReferenceObjectByHandle(hProcess,
                                       PROCESS_VM_OPERATION,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)&pepProcess,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not access process %p, Status=0x%lx\n", hProcess, Status);
        return FALSE;
    }

    if (bMap)
    {
        /* Make sure the section isn't already mapped */
        ASSERT(pSection->pvMappedBase == NULL);

        /* Map the section into the process address space */
        Status = MmMapViewOfSection(pSection->pvSectionObject,
                                    pepProcess,
                                    &pSection->pvMappedBase,
                                    0,
                                    pSection->cjViewSize,
                                    NULL,
                                    &pSection->cjViewSize,
                                    0,
                                    0,
                                    PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to map a section Status=0x%x\n", Status);
        }
    }
    else
    {
        /* Make sure the section is mapped */
        ASSERT(pSection->pvMappedBase);

        /* Unmap the section from the process address space */
        Status = MmUnmapViewOfSection(pepProcess, pSection->pvMappedBase);
        if (NT_SUCCESS(Status))
        {
            pSection->pvMappedBase = NULL;
        }
        else
        {
            DPRINT1("Failed to unmap a section @ &p Status=0x%x\n", 
                    pSection->pvMappedBase, Status);
        }
    }

    /* Dereference the process */
    ObDereferenceObject(pepProcess);

    /* Set the new mapping base and return bool status */
    *pvBaseAddress = pSection->pvMappedBase;
    return NT_SUCCESS(Status);
}

BOOL
APIENTRY
EngFreeSectionMem(
    IN PVOID pvSection OPTIONAL,
    IN PVOID pvMappedBase OPTIONAL)
{
    NTSTATUS Status;
    PENGSECTION pSection = pvSection;
    BOOL bResult = TRUE;

    /* Did the caller give us a mapping base? */
    if (pvMappedBase)
    {
        Status = MmUnmapViewInSessionSpace(pvMappedBase);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("MmUnmapViewInSessionSpace failed: 0x%lx\n", Status);
            bResult = FALSE;
        }
    }

    /* Check if we should free the section as well */
    if (pSection)
    {
        /* Dereference the kernel section */
        ObDereferenceObject(pSection->pvSectionObject);

        /* Finally free the section memory itself */
        EngFreeMem(pSection);
    }

    return bResult;
}

PVOID
APIENTRY
EngAllocSectionMem(
    OUT PVOID *ppvSection,
    IN ULONG fl,
    IN SIZE_T cjSize,
    IN ULONG ulTag)
{
    NTSTATUS Status;
    PENGSECTION pSection;

    /* Check parameter */
    if (cjSize == 0) return NULL;

    /* Allocate a section object */
    pSection = EngCreateSection(fl, cjSize, ulTag);
    if (!pSection)
    {
        *ppvSection = NULL;
        return NULL;
    }

    /* Map the section in session space */
    Status = MmMapViewInSessionSpace(pSection->pvSectionObject,
                                     &pSection->pvMappedBase,
                                     &pSection->cjViewSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to map a section Status=0x%x\n", Status);
        *ppvSection = NULL;
        EngFreeSectionMem(pSection, NULL);
        return NULL;
    }

    if (fl & FL_ZERO_MEMORY)
    {
        RtlZeroMemory(pSection->pvMappedBase, cjSize);
    }

    /* Set section pointer and return base address */
    *ppvSection = pSection;
    return pSection->pvMappedBase;
}


PFILEVIEW
NTAPI
EngLoadModuleEx(
    LPWSTR pwsz,
    ULONG cjSizeOfModule,
    FLONG fl)
{
    PFILEVIEW pFileView = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hRootDir;
    UNICODE_STRING ustrFileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileInformation;
    HANDLE hFile;
    NTSTATUS Status;
    LARGE_INTEGER liSize;

    if (fl & FVF_FONTFILE)
    {
        pFileView = EngAllocMem(0, sizeof(FONTFILEVIEW), 'vffG');
    }
    else
    {
        pFileView = EngAllocMem(0, sizeof(FILEVIEW), 'liFg');
    }

    /* Check for success */
    if (!pFileView) return NULL;

    /* Check if the file is relative to system32 */
    if (fl & FVF_SYSTEMROOT)
    {
        hRootDir = ghSystem32Directory;
    }
    else
    {
        hRootDir = ghRootDirectory;
    }

    /* Initialize unicode string and object attributes */
    RtlInitUnicodeString(&ustrFileName, pwsz);
    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrFileName,
                               OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                               hRootDir,
                               NULL);

    /* Now open the file */
    Status = ZwCreateFile(&hFile,
                          FILE_READ_DATA,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0);

    Status = ZwQueryInformationFile(hFile,
                                    &IoStatusBlock,
                                    &FileInformation,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation);
    if (NT_SUCCESS(Status))
    {
        pFileView->LastWriteTime = FileInformation.LastWriteTime;
    }

    /* Create a section from the file */
    liSize.QuadPart = cjSizeOfModule;
    Status = MmCreateSection(&pFileView->pSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             cjSizeOfModule ? &liSize : NULL,
                             fl & FVF_READONLY ? PAGE_EXECUTE_READ : PAGE_EXECUTE_READWRITE,
                             SEC_COMMIT,
                             hFile,
                             NULL);

    /* Close the file handle */
    ZwClose(hFile);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create a section Status=0x%x\n", Status);
        EngFreeMem(pFileView);
        return NULL;
    }


    pFileView->pvKView = NULL;
    pFileView->pvViewFD = NULL;
    pFileView->cjView = 0;

    return pFileView;
}

HANDLE
APIENTRY
EngLoadModule(LPWSTR pwsz)
{
    /* Forward to EngLoadModuleEx */
    return (HANDLE)EngLoadModuleEx(pwsz, 0, FVF_READONLY | FVF_SYSTEMROOT);
}

HANDLE
APIENTRY
EngLoadModuleForWrite(
	IN LPWSTR pwsz,
	IN ULONG  cjSizeOfModule)
{
    /* Forward to EngLoadModuleEx */
    return (HANDLE)EngLoadModuleEx(pwsz, cjSizeOfModule, FVF_SYSTEMROOT);
}

PVOID
APIENTRY
EngMapModule(
	IN  HANDLE h,
	OUT PULONG pulSize)
{
    PFILEVIEW pFileView = (PFILEVIEW)h;
    NTSTATUS Status;

    pFileView->cjView = 0;

    /* Map the section in session space */
    Status = MmMapViewInSessionSpace(pFileView->pSection,
                                     &pFileView->pvKView,
                                     &pFileView->cjView);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to map a section Status=0x%x\n", Status);
        *pulSize = 0;
        return NULL;
    }

    *pulSize = pFileView->cjView;
    return pFileView->pvKView;
}

VOID
APIENTRY
EngFreeModule(IN HANDLE h)
{
    PFILEVIEW pFileView = (PFILEVIEW)h;
    NTSTATUS Status;

    /* Unmap the section */
    Status = MmUnmapViewInSessionSpace(pFileView->pvKView);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MmUnmapViewInSessionSpace failed: 0x%lx\n", Status);
        ASSERT(FALSE);
    }

    /* Dereference the section */
    ObDereferenceObject(pFileView->pSection);

    /* Free the file view memory */
    EngFreeMem(pFileView);
}

PVOID
APIENTRY
EngMapFile(
    IN LPWSTR pwsz,
    IN ULONG cjSize,
    OUT ULONG_PTR *piFile)
{
    HANDLE hModule;
    PVOID pvBase;

    /* Load the file */
    hModule = EngLoadModuleEx(pwsz, 0, 0);
    if (!hModule)
    {
        *piFile = 0;
        return NULL;
    }

    /* Map the file */
    pvBase = EngMapModule(hModule, &cjSize);
    if (!pvBase)
    {
        EngFreeModule(hModule);
        hModule = NULL;
    }

    /* Set iFile and return mapped base */
    *piFile = (ULONG_PTR)hModule;
    return pvBase;
}

BOOL
APIENTRY
EngUnmapFile(
    IN ULONG_PTR iFile)
{
    HANDLE hModule = (HANDLE)iFile;

    EngFreeModule(hModule);

    return TRUE;
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
