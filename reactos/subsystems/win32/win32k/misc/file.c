/*
 * COPYRIGHT:        GPL, see COPYING in the top level directory
 * PROJECT:          ReactOS win32 kernel mode subsystem server
 * PURPOSE:          File access support routines
 * FILE:             subsystem/win32/win32k/misc/registry.c
 * PROGRAMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <w32k.h>

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

    DPRINT("Leaving W32kOpenFile, Status=0x%x, hFile=0x%x\n", Status, hFile);
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
    HANDLE hSection = NULL;
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
    }

    DPRINT("Leaving W32kCreateFileSection, Status=0x%x, hSection=0x%x\n", Status, hSection);

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
    PVOID pvBase = 0;

    ulViewSize =
    liSectionOffset.QuadPart = ulSectionOffset;
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
    }

    DPRINT("Leaving W32kMapViewOfSection, Status=0x%x, pvBase=0x%x\n", Status, pvBase);

    return pvBase;
}

typedef struct tagBITMAPV5INFO
{
    BITMAPV5HEADER bmiHeader;
    RGBQUAD        bmiColors[256];
} BITMAPV5INFO, *PBITMAPV5INFO;

// FIXME: this should go to dibobj.c
NTSTATUS
ProbeAndConvertBitmapInfo(
    OUT PBITMAPV5INFO pbmiDst,
    IN  PBITMAPINFO pbmiUnsafe)
{
    BITMAPV5HEADER *pbmhDst = (BITMAPV5HEADER*)&pbmiDst->bmiHeader;
    DWORD dwSize;
    RGBQUAD *pbmiColors;
    ULONG ulWidthBytes;

    /* Get the size and probe */
    ProbeForRead(&pbmiUnsafe->bmiHeader.biSize, sizeof(DWORD), 1);
    dwSize = pbmiUnsafe->bmiHeader.biSize;
    ProbeForRead(pbmiUnsafe, dwSize, 1);

    /* Check the size */
    // FIXME: are intermediate sizes allowed? As what are they interpreted?
    //        make sure we don't use a too big dwSize later
    if (dwSize != sizeof(BITMAPCOREHEADER) &&
        dwSize != sizeof(BITMAPINFOHEADER) &&
        dwSize != sizeof(BITMAPV4HEADER) &&
        dwSize != sizeof(BITMAPV5HEADER))
    {
        return STATUS_INVALID_PARAMETER;
    }

    pbmiColors = (RGBQUAD*)((PCHAR)pbmiUnsafe + dwSize);

    pbmhDst->bV5Size = sizeof(BITMAPV5HEADER);

    if (dwSize == sizeof(BITMAPCOREHEADER))
    {
        PBITMAPCOREHEADER pbch = (PBITMAPCOREHEADER)pbmiUnsafe;

        /* Manually copy the fields that are present */
        pbmhDst->bV5Width = pbch->bcWidth;
        pbmhDst->bV5Height = pbch->bcHeight;
        pbmhDst->bV5Planes = pbch->bcPlanes;
        pbmhDst->bV5BitCount = pbch->bcBitCount;

        /* Set some default values */
        pbmhDst->bV5Compression = BI_RGB;
        pbmhDst->bV5SizeImage = 0;
        pbmhDst->bV5XPelsPerMeter = 72;
        pbmhDst->bV5YPelsPerMeter = 72;
        pbmhDst->bV5ClrUsed = 0;
        pbmhDst->bV5ClrImportant = 0;
    }
    else
    {
        /* Copy valid fields */
        memcpy(pbmhDst, pbmiUnsafe, dwSize);

        /* Zero out the rest of the V5 header */
        memset((char*)pbmhDst + dwSize, 0, sizeof(BITMAPV5HEADER) - dwSize);
    }


    if (dwSize < sizeof(BITMAPV4HEADER))
    {
        if (pbmhDst->bV5Compression == BI_BITFIELDS)
        {
            DWORD *pMasks = (DWORD*)pbmiColors;
            pbmhDst->bV5RedMask = pMasks[0];
            pbmhDst->bV5GreenMask = pMasks[1];
            pbmhDst->bV5BlueMask = pMasks[2];
            pbmhDst->bV5AlphaMask = 0;
            pbmhDst->bV5ClrUsed = 0;
        }

//        pbmhDst->bV5CSType;
//        pbmhDst->bV5Endpoints;
//        pbmhDst->bV5GammaRed;
//        pbmhDst->bV5GammaGreen;
//        pbmhDst->bV5GammaBlue;
    }

    if (dwSize < sizeof(BITMAPV5HEADER))
    {
//        pbmhDst->bV5Intent;
//        pbmhDst->bV5ProfileData;
//        pbmhDst->bV5ProfileSize;
//        pbmhDst->bV5Reserved;
    }

    ulWidthBytes = ((pbmhDst->bV5Width * pbmhDst->bV5Planes *
                     pbmhDst->bV5BitCount + 31) & ~31) / 8;

    if (pbmhDst->bV5SizeImage == 0)
        pbmhDst->bV5SizeImage = abs(ulWidthBytes * pbmhDst->bV5Height);

    if (pbmhDst->bV5ClrUsed == 0)
        pbmhDst->bV5ClrUsed = pbmhDst->bV5BitCount == 1 ? 2 :
                              (pbmhDst->bV5BitCount == 4 ? 16 :
                              (pbmhDst->bV5BitCount == 8 ? 256 : 0));

    if (pbmhDst->bV5Planes != 1)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (pbmhDst->bV5BitCount != 0 && pbmhDst->bV5BitCount != 1 &&
        pbmhDst->bV5BitCount != 4 && pbmhDst->bV5BitCount != 8 &&
        pbmhDst->bV5BitCount != 16 && pbmhDst->bV5BitCount != 24 &&
        pbmhDst->bV5BitCount != 32)
    {
        DPRINT("Invalid bit count: %d\n", pbmhDst->bV5BitCount);
        return STATUS_INVALID_PARAMETER;
    }

    if ((pbmhDst->bV5BitCount == 0 &&
         pbmhDst->bV5Compression != BI_JPEG && pbmhDst->bV5Compression != BI_PNG))
    {
        DPRINT("Bit count 0 is invalid for compression %d.\n", pbmhDst->bV5Compression);
        return STATUS_INVALID_PARAMETER;
    }

    if (pbmhDst->bV5Compression == BI_BITFIELDS &&
        pbmhDst->bV5BitCount != 16 && pbmhDst->bV5BitCount != 32)
    {
        DPRINT("Bit count %d is invalid for compression BI_BITFIELDS.\n", pbmhDst->bV5BitCount);
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}


HBITMAP
NTAPI
UserLoadImage(PCWSTR pwszName)
{
    NTSTATUS Status;
    HANDLE hFile, hSection;
    BITMAPFILEHEADER *pbmfh;
    LPBITMAPINFO pbmi;
    ULONG cjInfoSize;
    PVOID pvBits;
    HBITMAP hbmp = 0;
    BITMAPV5INFO bmiLocal;

    /* Open the file */
    hFile = W32kOpenFile(pwszName, FILE_READ_DATA);
    if (hFile == INVALID_HANDLE_VALUE)
          return NULL;

    /* Create a section */
    hSection = W32kCreateFileSection(hFile, SEC_COMMIT, PAGE_READONLY, 0);
    ZwClose(hFile);
    if (!hSection)
          return NULL;

    /* Map the section */
    pbmfh = W32kMapViewOfSection(hSection, PAGE_READONLY, 0);
    ZwClose(hSection);
    if (!pbmfh)
          return NULL;

    /* Get a pointer to the BITMAPINFO */
    pbmi = (LPBITMAPINFO)(pbmfh + 1);

    /* Create a normalized local BITMAPINFO */
    _SEH2_TRY
    {
        Status = ProbeAndConvertBitmapInfo(&bmiLocal, pbmi);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (NT_SUCCESS(Status))
    {
        cjInfoSize = bmiLocal.bmiHeader.bV5Size +
                     bmiLocal.bmiHeader.bV5ClrUsed * sizeof(RGBQUAD);
        pvBits = (PVOID)((PCHAR)pbmi + cjInfoSize);

        // FIXME: use Gre... so that the BITMAPINFO doesn't get probed
        hbmp = NtGdiCreateDIBitmapInternal(NULL,
                                           bmiLocal.bmiHeader.bV5Width,
                                           bmiLocal.bmiHeader.bV5Height,
                                           CBM_INIT,
                                           pvBits,
                                           pbmi,
                                           DIB_RGB_COLORS,
                                           bmiLocal.bmiHeader.bV5Size,
                                           bmiLocal.bmiHeader.bV5SizeImage,
                                           0,
                                           0);
    }

    /* Unmap our section, we don't need it anymore */
    ZwUnmapViewOfSection(NtCurrentProcess(), pbmfh);

    DPRINT("Leaving UserLoadImage, hbmp = %p\n", hbmp);
    return hbmp;
}
