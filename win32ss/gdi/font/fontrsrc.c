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

PFT gpftPublic;
static LIST_ENTRY glePrivatePFFList = {&glePrivatePFFList, &glePrivatePFFList};
static LIST_ENTRY glePublicPFFList = {&glePublicPFFList, &glePublicPFFList};

static
BOOL
PFF_bCompareFiles(
    PPFF ppff,
    ULONG cFiles,
    PFONTFILEVIEW apffv[])
{
    ULONG i;

    /* Check if number of files matches */
    if (ppff->cFiles != cFiles) return FALSE;

    /* Loop all files */
    for (i = 0; i < cFiles; i++)
    {
        /* Check if the files match */
        if (apffv[i] != ppff->apffv[i]) return FALSE;
    }

    return TRUE;
}

BOOL
NTAPI
PFT_bInit(
    PFT *ppft)
{

    RtlZeroMemory(ppft, sizeof(PFT));

    ppft->hsem = EngCreateSemaphore();
    if (!ppft->hsem) return FALSE;

    return TRUE;
}

static
PPFF
PFT_pffFindFont(
    PFT *ppft,
    PWSTR pwszFiles,
    ULONG cwc,
    ULONG cFiles,
    ULONG iFileNameHash)
{
    ULONG iListIndex = iFileNameHash % MAX_FONT_LIST;
    PPFF ppff = NULL;

    /* Acquire PFT lock */
    EngAcquireSemaphore(ppft->hsem);

    /* Loop all PFFs in the slot */
    for (ppff = ppft->apPFF[iListIndex]; ppff; ppff = ppff->pPFFNext)
    {
        /* Quick check */
        if (ppff->iFileNameHash != iFileNameHash) continue;

        /* Do a full check */
        if (!wcsncmp(ppff->pwszPathname, pwszFiles, cwc)) break;
    }

    /* Release PFT lock */
    EngReleaseSemaphore(ppft->hsem);

    return ppff;
}

static
VOID
PFT_vInsertPFE(
    PPFT ppft,
    PPFE ppfe)
{
    UCHAR ajWinChatSet[2] = {0, DEFAULT_CHARSET};
    UCHAR *pjCharSets;
    PIFIMETRICS pifi = ppfe->pifi;

    if (pifi->dpCharSets)
    {
        pjCharSets = (PUCHAR)pifi + pifi->dpCharSets;
    }
    else
    {
        ajWinChatSet[0] = pifi->jWinCharSet;
        pjCharSets = ajWinChatSet;
    }


}

static
VOID
PFT_vInsertPFF(
    PPFT ppft,
    PPFF ppff,
    ULONG iFileNameHash)
{
    ULONG i, iListIndex = iFileNameHash % MAX_FONT_LIST;

    ppff->iFileNameHash = iFileNameHash;

    /* Acquire PFT lock */
    EngAcquireSemaphore(ppft->hsem);

    /* Insert the font file into the hash bucket */
    ppff->pPFFPrev = NULL;
    ppff->pPFFNext = ppft->apPFF[iListIndex];
    ppft->apPFF[iListIndex] = ppff;

    /* Loop all PFE's */
    for (i = 0; i < ppff->cFonts; i++)
    {
        PFT_vInsertPFE(ppft, &ppff->apfe[i]);
    }

    /* Release PFT lock */
    EngReleaseSemaphore(ppft->hsem);
}

static
ULONG
CalculateNameHash(PWSTR pwszName)
{
    ULONG iHash = 0;
    WCHAR wc;

    while ((wc = *pwszName++) != 0)
    {
        iHash = _rotl(iHash, 7);
        iHash += wc;
    }

    return iHash;
}



INT
NTAPI
GreAddFontResourceW(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN FLONG fl,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv)
{
    PPFT ppft;
    PPFF ppff = NULL;
    ULONG ulCheckSum = 0;
    PPROCESSINFO ppi;
    ULONG iFileNameHash;

    // HACK: only global list for now
    fl &= ~FR_PRIVATE;

    /* Add to private table? */
    if (fl & FR_PRIVATE)
    {
        /* Use the process owned private font table */
        ppi = PsGetCurrentProcessWin32Process();
        ppft = ppi->ppftPrivate;
    }
    else
    {
        /* Use the global font table */
        ppft = &gpftPublic;
    }

    /* Get a hash value for the path name */
    iFileNameHash = CalculateNameHash(pwszFiles);

    /* Try to find the font in the font table */
    ppff = PFT_pffFindFont(ppft, pwszFiles, cwc, cFiles, iFileNameHash);

    /* Did we find the font? */
    if (ppff)
    {
        /* Return the number of faces */
        return ppff->cFonts;
    }

    // FIXME: check other list, "copy" pft if found



    /* Load the font file with a font driver */
    ppff = EngLoadFontFileFD(pwszFiles, cwc, cFiles, pdv, ulCheckSum);
    if (!ppff)
    {
        DPRINT1("Failed to load font with font driver\n");
        return 0;
    }

    /* Insert the PFF into the list */
    PFT_vInsertPFF(ppft, ppff, iFileNameHash);

    /* Return the number of faces */
    return ppff->cFonts;
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
    PWCHAR pwszUpcase;
    ULONG cjSize;
    DESIGNVECTOR dv;
    INT iRes = 0;
    ULONG i;

    /* Check parameters */
    if (cFiles == 0 || cFiles > FD_MAX_FILES ||
        cwc < 6 ||  cwc > FD_MAX_FILES * MAX_PATH)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Allocate a buffer */
    pwszUpcase = EngAllocMem(0, (cwc + 1) * sizeof(WCHAR), 'pmTG');
    if (!pwszUpcase)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    _SEH2_TRY
    {
        ProbeForRead(pwszFiles, cwc * sizeof(WCHAR), 2);

        /* Verify zero termination */
        if (pwszFiles[cwc] != 0)
        {
            _SEH2_YIELD(goto cleanup);
        }

        /* Convert the string to upper case */
        for (i = 0; i < cwc; i++)
        {
            pwszUpcase[i] = RtlUpcaseUnicodeChar(pwszFiles[i]);
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
    iRes = GreAddFontResourceW(pwszUpcase, cwc, cFiles, fl, dwPidTid, pdv);

cleanup:
    EngFreeMem(pwszUpcase);

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

