/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Font resource handling
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include "font.h"

#define NDEBUG
#include <debug.h>

HSEMAPHORE ghsemPFFList;
static LIST_ENTRY glePrivatePFFList = {&glePrivatePFFList, &glePrivatePFFList};
static LIST_ENTRY glePublicPFFList = {&glePublicPFFList, &glePublicPFFList};

static
BOOL
PFF_bCompareFiles(
    PPFF ppff,
    ULONG cFiles,
    PFONTFILEVIEW pffv[])
{
    ULONG i;

    /* Check if number of files matches */
    if (ppff->cFiles != cFiles) return FALSE;

    /* Loop all files */
    for (i = 0; i < cFiles; i++)
    {
        /* Check if the files match */
        if (pffv[i] != ppff->apffv[i]) return FALSE;
    }

    return TRUE;
}

INT
NTAPI
GreAddFontResourceInternal(
    IN PWCHAR apwszFiles[],
    IN ULONG cFiles,
    IN FLONG fl,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv)
{
    PFONTFILEVIEW apffv[FD_MAX_FILES];
    PPFF ppff = NULL;
    PLIST_ENTRY ple, pleListHead;
    ULONG i, ulCheckSum = 0;

    /* Loop the files */
    for (i = 0; i < cFiles; i++)
    {
        /* Try to load the file */
        apffv[i] = (PVOID)EngLoadModuleEx(apwszFiles[i], 0, FVF_FONTFILE);
        if (!apffv[i])
        {
            DPRINT1("Failed to load file: '%ls'\n", apwszFiles[i]);
            /* Cleanup and return */
            while (i--) EngFreeModule(apffv[i]);
            return 0;
        }
    }

    pleListHead = fl & FR_PRIVATE ? &glePrivatePFFList : &glePublicPFFList;

    /* Acquire PFF list lock */
    EngAcquireSemaphore(ghsemPFFList);

    /* Loop all physical font files (PFF) */
    for (ple = pleListHead->Flink; ple != pleListHead; ple = ple->Flink)
    {
        ppff = CONTAINING_RECORD(ple, PFF, leLink);

        /* Check if the files are already loaded */
        if (PFF_bCompareFiles(ppff, cFiles, apffv)) break;
    }

    /* Release PFF list lock */
    EngReleaseSemaphore(ghsemPFFList);

    if (ple == pleListHead)
    {
        /* Cleanup loaded files, we don't need them anymore */
        for (i = 0; i < cFiles; i++) EngFreeModule(apffv[i]);
        return ppff->cFonts;
    }

    /* Load the font file with a font driver */
    ppff = EngLoadFontFileFD(cFiles, apffv, pdv, ulCheckSum);
    if (!ppff)
    {
        DPRINT1("Failed to load font with font driver\n");
        return 0;
    }

    /* Insert the PFF into the list */
    EngAcquireSemaphore(ghsemPFFList);
    InsertTailList(pleListHead, &ppff->leLink);
    EngReleaseSemaphore(ghsemPFFList);

    return ppff->cFonts;
}

static
BOOL
SeperateFileNames(
    PWCHAR apwszFiles[],
    PWCHAR pwcDest,
    PWCHAR pwszFiles,
    ULONG cwc,
    ULONG cFiles)
{
    PWCHAR pwszEnd = pwszFiles + cwc;
    WCHAR wc;
    ULONG i = 0;

    apwszFiles[0] = pwcDest;

    /* Loop the file name string */
    while (pwszFiles < pwszEnd)
    {
        wc = *pwszFiles++;

        /* Must not be terminated before the end */
        if (wc == 0) return FALSE;

        /* Check for a seperator */
        if (wc == '|')
        {
            /* Zero terminate current path name */
            *pwcDest++ = 0;

            /* Go to next file name and check if its too many */
            if (++i >= cFiles) return FALSE;
            apwszFiles[i] = pwcDest;
        }
        else
        {
            *pwcDest++ = wc;
        }
    }

    /* Must be terminated now */
    if (*pwszFiles != 0 || i != cFiles - 1)
    {
        return FALSE;
    }

    return TRUE;
}


W32KAPI
INT
APIENTRY
NtGdiAddFontResourceW(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN FLONG fl,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv)
{
    PVOID pvBuffer;
    PWCHAR apwszFiles[FD_MAX_FILES];
    ULONG cjSize;
    DESIGNVECTOR dv;
    INT iRes = 0;

    /* Check parameters */
    if (cFiles == 0 || cFiles > FD_MAX_FILES ||
        cwc < 6 ||  cwc > FD_MAX_FILES * MAX_PATH)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Allocate a buffer */
    pvBuffer = EngAllocMem(0, (cwc + 1) * sizeof(WCHAR), 'pmTG');
    if (!pvBuffer)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    _SEH2_TRY
    {
        ProbeForRead(pwszFiles, cwc * sizeof(WCHAR), 2);
        if (!SeperateFileNames(apwszFiles, pvBuffer, pwszFiles, cwc, cFiles))
        {
            _SEH2_YIELD(goto cleanup);
        }

        /* Check if we have a DESIGNVECTOR */
        if (pdv)
        {
            /* Probe and check first 2 fields */
            ProbeForRead(pdv, 2 * sizeof(DWORD), sizeof(DWORD));
            if (pdv->dvReserved != STAMP_DESIGNVECTOR ||
                pdv->dvNumAxes > MM_MAX_NUMAXES)
            {
                _SEH2_YIELD(goto cleanup);
            }

            /* Copy the vector */
            cjSize = FIELD_OFFSET(DESIGNVECTOR, dvValues) + pdv->dvNumAxes * sizeof(LONG);
            ProbeForRead(pdv, cjSize, sizeof(DWORD));
            RtlCopyMemory(&dv, pdv, cjSize);
            pdv = &dv;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(goto cleanup);
    }
    _SEH2_END

    /* Call the internal function */
    iRes = GreAddFontResourceInternal(apwszFiles, cFiles, fl, dwPidTid, pdv);

cleanup:
    EngFreeMem(pvBuffer);

    return iRes;
}

W32KAPI
HANDLE
APIENTRY
NtGdiAddFontMemResourceEx(
    IN PVOID pvBuffer,
    IN DWORD cjBuffer,
    IN DESIGNVECTOR *pdv,
    IN ULONG cjDV,
    OUT DWORD *pNumFonts)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
BOOL
APIENTRY
NtGdiRemoveFontResourceW(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN ULONG fl,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
BOOL
APIENTRY
NtGdiRemoveFontMemResourceEx(
    IN HANDLE hMMFont)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
BOOL
APIENTRY
NtGdiUnmapMemFont(
    IN PVOID pvView)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetFontResourceInfoInternalW(
    IN LPWSTR pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN UINT cjIn,
    OUT LPDWORD pdwBytes,
    OUT LPVOID pvBuf,
    IN DWORD iType)
{
    ASSERT(FALSE);
    return 0;
}

