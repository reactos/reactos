/*
 * COPYRIGHT:        GPL, see COPYING in the top level directory
 * PROJECT:          ReactOS win32 kernel mode subsystem server
 * PURPOSE:          File access support routines
 * FILE:             win32ss/user/ntuser/misc/file.c
 * PROGRAMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

BOOL
NTAPI
W32kDosPathNameToNtPathName(
    IN PCWSTR pwszDosPathName,
    OUT PUNICODE_STRING pustrNtPathName)
{
    NTSTATUS Status;

    /* Prepend "\??\" */
    pustrNtPathName->Length = 0;
    Status = RtlAppendUnicodeToString(pustrNtPathName, L"\\??\\");
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* Append the dos name */
    Status = RtlAppendUnicodeToString(pustrNtPathName, pwszDosPathName);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    return TRUE;
}


HANDLE
NTAPI
W32kOpenFile(PCWSTR pwszFileName, DWORD dwDesiredAccess)
{
    UNICODE_STRING ustrFile;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    NTSTATUS Status;

    DPRINT("W32kOpenFile(%S)\n", pwszFileName);

    RtlInitUnicodeString(&ustrFile, pwszFileName);

    InitializeObjectAttributes(&ObjectAttributes, &ustrFile, 0, NULL, NULL);

    Status = ZwCreateFile(&hFile,
                          dwDesiredAccess,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        hFile = NULL;
    }

    DPRINT("Leaving W32kOpenFile, Status=0x%lx, hFile=0x%p\n", Status, hFile);
    return hFile;
}

HANDLE
NTAPI
W32kCreateFileSection(HANDLE hFile,
                      ULONG flAllocation,
                      DWORD flPageProtection,
                      ULONGLONG ullMaxSize)
{
    NTSTATUS Status;
    HANDLE hSection;
    ACCESS_MASK amDesiredAccess;

    /* Set access mask */
    amDesiredAccess = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ;

    /* Check if write access is requested */
    if (flPageProtection == PAGE_READWRITE)
    {
        /* Add it to access mask */
        amDesiredAccess |= SECTION_MAP_WRITE;
    }

    /* Now create the actual section */
    Status = ZwCreateSection(&hSection,
                             amDesiredAccess,
                             NULL,
                             NULL,
                             flPageProtection,
                             flAllocation,
                             hFile);
    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        hSection = NULL;
    }

    DPRINT("Leaving W32kCreateFileSection, Status=0x%lx, hSection=0x%p\n", Status, hSection);

    /* Return section handle */
    return hSection;
}

PVOID
NTAPI
W32kMapViewOfSection(
    HANDLE hSection,
    DWORD dwPageProtect,
    ULONG_PTR ulSectionOffset)
{
    NTSTATUS Status;
    LARGE_INTEGER liSectionOffset;
    ULONG_PTR ulViewSize;
    PVOID pvBase = NULL;

    liSectionOffset.QuadPart = ulViewSize = ulSectionOffset;
    Status = ZwMapViewOfSection(hSection,
                                NtCurrentProcess(),
                                &pvBase,
                                0,
                                0,
                                &liSectionOffset,
                                &ulViewSize,
                                ViewShare,
                                0,
                                dwPageProtect);
    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        pvBase = NULL;
    }

    DPRINT("Leaving W32kMapViewOfSection, Status=0x%lx, pvBase=0x%p\n", Status, pvBase);

    return pvBase;
}

HBITMAP
NTAPI
UserLoadImage(PCWSTR pwszName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE hFile, hSection;
    BITMAPFILEHEADER *pbmfh;
    LPBITMAPINFO pbmi;
    PVOID pvBits;
    HBITMAP hbmp = 0;

    DPRINT("Enter UserLoadImage(%ls)\n", pwszName);

    /* Open the file */
    hFile = W32kOpenFile(pwszName, FILE_READ_DATA);
    if (!hFile)
    {
        return NULL;
    }

    /* Create a section */
    hSection = W32kCreateFileSection(hFile, SEC_COMMIT, PAGE_READONLY, 0);
    ZwClose(hFile);
    if (!hSection)
    {
        return NULL;
    }

    /* Map the section */
    pbmfh = W32kMapViewOfSection(hSection, PAGE_READONLY, 0);
    ZwClose(hSection);
    if (!pbmfh)
    {
        return NULL;
    }

    /* Get a pointer to the BITMAPINFO */
    pbmi = (LPBITMAPINFO)(pbmfh + 1);

	_SEH2_TRY
	{
		ProbeForRead(&pbmfh->bfSize, sizeof(DWORD), 1);
		ProbeForRead(pbmfh, pbmfh->bfSize, 1);
	}
	_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		Status = _SEH2_GetExceptionCode();
	}
	_SEH2_END

	if(!NT_SUCCESS(Status))
	{
		DPRINT1("Bad File?\n");
		goto leave;
	}

    if (pbmfh->bfType == 0x4D42 /* 'BM' */)
    {
		/* Could be BITMAPCOREINFO */
		BITMAPINFO* pConvertedInfo;
		HDC hdc;

		pvBits = (PVOID)((PCHAR)pbmfh + pbmfh->bfOffBits);

		pConvertedInfo = DIB_ConvertBitmapInfo(pbmi, DIB_RGB_COLORS);
		if(!pConvertedInfo)
		{
			DPRINT1("Unable to convert the bitmap Info\n");
			goto leave;
		}

		hdc = IntGdiCreateDC(NULL, NULL, NULL, NULL,FALSE);

        hbmp = GreCreateDIBitmapInternal(hdc,
                                         pConvertedInfo->bmiHeader.biWidth,
				                         pConvertedInfo->bmiHeader.biHeight,
                                         CBM_INIT,
                                         pvBits,
                                         pConvertedInfo,
                                         DIB_RGB_COLORS,
                                         0,
                                         pbmfh->bfSize - pbmfh->bfOffBits,
                                         0);

		NtGdiDeleteObjectApp(hdc);
		DIB_FreeConvertedBitmapInfo(pConvertedInfo, pbmi, -1);
    }
	else
	{
		DPRINT1("Unknown file type!\n");
	}

leave:
    /* Unmap our section, we don't need it anymore */
    ZwUnmapViewOfSection(NtCurrentProcess(), pbmfh);

    DPRINT("Leaving UserLoadImage, hbmp = %p\n", hbmp);
    return hbmp;
}
