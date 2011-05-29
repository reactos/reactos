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

