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
PPFF gppffList;
LIST_ENTRY glePFFList = {&glePFFList, &glePFFList};


static
BOOL
ComparePFF(
    PPFF ppff,
    ULONG cFiles,
    PFONTFILEVIEW pffv[])
{
    ULONG i;
    ASSERT(cFiles >= 1 && cFiles <= FD_MAX_FILES);

    /* Check if number of files matches */
    if (ppff->cFiles != cFiles) return FALSE;

    /* Loop all files */
    for (i = 0; i < cFiles; i++)
    {
        /* Check if the files match */
        if (pffv[i] != ppff->ppfv[i]) return FALSE;
    }

    return TRUE;
}

PPFF
NTAPI
EngLoadFontFile(
    IN PWCHAR apwszFiles[],
    IN ULONG cFiles)
{
    PFONTFILEVIEW apffv[FD_MAX_FILES];
    PLIST_ENTRY ple;
    PPFF ppff;
    ULONG i, cjSize;

    /* Loop the files */
    for (i = 0; i < cFiles; i++)
    {
        /* Try to load the file */
        apffv[i] = (PVOID)EngLoadModuleEx(apwszFiles[i], 0, FVF_FONTFILE);
        if (!apffv[i])
        {
            /* Cleanup and return */
            while (i--) EngFreeModule(apffv[i]);
            return NULL;
        }
    }

    /* Acquire PFF list lock */
    EngAcquireSemaphore(ghsemPFFList);

    /* Loop all physical font files (PFF) */
    for (ple = glePFFList.Flink; ple != &glePFFList; ple = ple->Flink)
    {
        ppff = CONTAINING_RECORD(ple, PFF, leLink);

        /* Check if the files are already loaded */
        if (ComparePFF(ppff, cFiles, apffv))
        {
            /* Unload the loaded files */
            while (i--) EngFreeModule(apffv[i]);
            goto leave;
        }
    }

    /* Allocate a new PFF */
    cjSize = sizeof(PFF) + cFiles * sizeof(PVOID);
    ppff = EngAllocMem(0, cjSize, 'ffpG');
    if (!ppff)
    {
        goto leave;
    }

    ppff->sizeofThis = cjSize;
    ppff->cFiles = cFiles;
    ppff->ppfv = (PVOID)(ppff + 1);

    /* Copy the FONTFILEVIEWs */
    for (i = 0; i < cFiles; i++) ppff->ppfv[i] = apffv[i];

    /* Insert the PFF into the list */
    InsertTailList(&glePFFList, &ppff->leLink);

leave:
    /* Release PFF list lock */
    EngReleaseSemaphore(ghsemPFFList);

    return ppff;
}

INT
NTAPI
GreAddFontResourceInternal(
    IN PWCHAR apwszFiles[],
    IN ULONG cFiles,
    IN FLONG f,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv)
{


    ASSERT(FALSE);
    return 0;
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
    if (cFiles > FD_MAX_FILES || cwc < 3 || cwc > FD_MAX_FILES * MAX_PATH)
    {
        return 0;
    }

    /* Allocate a buffer */
    pvBuffer = EngAllocMem(0, (cwc + 1) * sizeof(WCHAR), 'pmTG');
    if (!pvBuffer)
    {
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

