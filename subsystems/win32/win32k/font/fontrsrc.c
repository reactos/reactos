/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Font resource handling
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include <include/font.h>
DBG_DEFAULT_CHANNEL(GdiFont);

PFT gpftPublic;
ULONG gulHashBucketId = 0;

static
PFONTSUBSTITUTE
GetFontSubstitute(PWSTR pwszFace)
{
    // HACK
    WARN("GetFontSubstitute is unimplemented.\n");
    return NULL;
}

VOID
NTAPI
UpcaseString(
    OUT PWSTR pwszDest,
    IN PWSTR pwszSource,
    IN ULONG cwc)
{
    WCHAR wc;

    while (--cwc)
    {
        wc = *pwszSource++;
        if (wc == 0) break;
        *pwszDest++ = RtlUpcaseUnicodeChar(wc);
    }

    *pwszDest = 0;
}


ULONG
NTAPI
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


static
PHASHBUCKET
HASHBUCKET_Allocate(void)
{
    PHASHBUCKET pbkt;


    pbkt = ExAllocatePoolWithTag(PagedPool,
                                 sizeof(HASHBUCKET),
                                 GDITAG_PFE_HASHBUCKET);
    if (!pbkt) return NULL;

    RtlZeroMemory(pbkt, sizeof(HASHBUCKET));
    pbkt->ulTime = InterlockedIncrement((LONG*)gulHashBucketId);

    return pbkt;
}

static
BOOL
HASHBUCKET_bLinkPFE(
    IN PHASHBUCKET pbkt,
    IN PPFE ppfe)
{
    PPFELINK ppfel;

    ppfel = ExAllocatePoolWithTag(PagedPool, sizeof(PFELINK), GDITAG_PFE_LINK);
    if (!ppfel)
    {
        return FALSE;
    }

    ppfel->ppfe = ppfe;

    ppfel->ppfelNext = pbkt->ppfelEnumHead;
    pbkt->ppfelEnumHead = ppfel;
    if (!ppfel->ppfelNext) pbkt->ppfelEnumTail = ppfel;

    return TRUE;
}

static
VOID
HASHBUCKET_vUnlinkPFE(
    IN PHASHBUCKET pbkt,
    IN PPFE ppfe)
{
    PPFELINK ppfel, ppfelPrev;

    /* List must not be empty */
    ASSERT(pbkt->ppfelEnumHead);
    ASSERT(pbkt->ppfelEnumTail);

    /* First check the list head */
    ppfel = pbkt->ppfelEnumHead;
    if (ppfel->ppfe == ppfe)
    {
        pbkt->ppfelEnumHead = ppfel->ppfelNext;
        ppfelPrev = NULL;
    }
    else
    {
        /* Loop through the rest of the list */
        while (TRUE)
        {
            ppfelPrev = ppfel;
            ppfel = ppfel->ppfelNext;

            if (!ppfel)
            {
                ERR("PFE not found!\n");
                ASSERT(FALSE);
            }

            if (ppfel && (ppfel->ppfe == ppfe))
            {
                ppfelPrev->ppfelNext = ppfel->ppfelNext;
                break;
            }
        }
    }

    /* Check list tail */
    if (ppfel == pbkt->ppfelEnumTail) pbkt->ppfelEnumTail = ppfelPrev;

    /* Free the PFELINK */
    ExFreePoolWithTag(ppfel, GDITAG_PFE_LINK);
}

BYTE
NTAPI
GreGetDefaultCharset()
{
    USHORT usACP, usOEM;

    RtlGetDefaultCodePage(&usACP, &usOEM);
    WARN("GreGetDefaultCharset is unimplemented.\n");

    // HACK!
    return ANSI_CHARSET;
}

ULONG
NTAPI
PFE_ulPenalty(
    PPFE ppfe,
    LOGFONTW *plfw,
    ULONG ulMargin)
{
    PIFIMETRICS pifi = ppfe->pifi;
    ULONG i, ulPenalty = 0;
    PBYTE pjCharSets;
    BYTE jCharSet;

    jCharSet = plfw->lfCharSet;
    if (jCharSet == DEFAULT_CHARSET)
    {
        /* Get the default charset */
        jCharSet = GreGetDefaultCharset();
    }

    /* Check charsets */
    if (pifi->dpCharSets)
    {
        /* Get a pointer to the charset array */
        pjCharSets = (PBYTE)pifi + pifi->dpCharSets;

        /* Loop charsets, until the requeste one is found */
        for (i = 0; pjCharSets[i] != jCharSet; i++)
        {
            /* Check if this is the last charset */
            if ((i == 15) || (pjCharSets[i] == DEFAULT_CHARSET))
            {
                /* Charset wasn't found, add penalty */
                ulPenalty += PENALTY_CharSet;
                if (ulPenalty > ulMargin) return ulPenalty;
                break;
            }
        }
    }
    else
    {
        /* Otherwise check if the win charset matches */
        if (pifi->jWinCharSet != jCharSet)
        {
            /* Charset doesn't match, add penalty */
            ulPenalty += PENALTY_CharSet;
            if (ulPenalty > ulMargin) return ulPenalty;
        }
    }


    /* There are contrary claims about the font mapper behaviour towards
       different lfOutPrecision values between MS LOGFONT structure docs
       and the font mapper article. */
    if ((plfw->lfOutPrecision == OUT_DEVICE_PRECIS) &&
        ((ppfe->flPFE & PFE_DEVICEFONT) == 0))
    {
        ulPenalty += PENALTY_OutputPrecision;
    }

    /* Check if pitch matches */
    if (((plfw->lfPitchAndFamily & 0x0f) == FIXED_PITCH) &&
        (pifi->jWinPitchAndFamily & 0x0f) != FIXED_PITCH)
    {
        ulPenalty += PENALTY_FixedPitch;
        if (ulPenalty > ulMargin) return ulPenalty;
    }
    else if (((plfw->lfPitchAndFamily & 0x0f) == VARIABLE_PITCH) &&
        (pifi->jWinPitchAndFamily & 0x0f) != VARIABLE_PITCH)
    {
        ulPenalty += PENALTY_PitchVariable;
        if (ulPenalty > ulMargin) return ulPenalty;
    }


    if ((plfw->lfPitchAndFamily & 0xf0) != FF_DONTCARE)
    {
        if ((pifi->jWinPitchAndFamily & 0xf0) == FF_DONTCARE)
        {
            ulPenalty += PENALTY_FamilyUnknown;
        }
        else if ((plfw->lfPitchAndFamily & 0xf0) != (pifi->jWinPitchAndFamily & 0xf0))
        {
            ulPenalty += PENALTY_Family;
        }
    }

// PENALTY_HeightSmaller            150
// PENALTY_HeightBiggerDifference   150
// PENALTY_FamilyUnlikely            50
// PENALTY_Width                     50
// PENALTY_SizeSynth                 50
// PENALTY_Aspect                    30
// PENALTY_IntSizeSynth              20
// PENALTY_UnevenSizeSynth            4

    if (!plfw->lfItalic && (pifi->fsSelection & FM_SEL_ITALIC))
    {
        ulPenalty += PENALTY_Italic;
    }

    if ((plfw->lfOutPrecision == OUT_TT_ONLY_PRECIS) &&
        !(pifi->flInfo & FM_INFO_TECH_TRUETYPE))
    {
        ulPenalty += 500000; // more then can be accounted for
    }

    if ((plfw->lfOutPrecision == OUT_TT_PRECIS) &&
        !(pifi->flInfo & FM_INFO_TECH_TRUETYPE))
    {
        ulPenalty += PENALTY_NotTrueType;
    }


    ulPenalty += (abs(plfw->lfWeight - pifi->usWinWeight) / 10) * PENALTY_Weight;

    if (!plfw->lfUnderline && (pifi->fsSelection & FM_SEL_UNDERSCORE))
    {
        ulPenalty += PENALTY_Underline;
    }

    if (!plfw->lfStrikeOut && (pifi->fsSelection & FM_SEL_STRIKEOUT))
    {
        ulPenalty += PENALTY_StrikeOut;
    }

// PENALTY_VectorHeightSmaller        2
// PENALTY_DeviceFavor                2
// PENALTY_ItalicSim                  1

    if (((plfw->lfPitchAndFamily & 0x0f) == DEFAULT_PITCH) &&
        (pifi->jWinPitchAndFamily & 0x0f) != FIXED_PITCH)
    {
        ulPenalty += PENALTY_DefaultPitchFixed;
    }

// PENALTY_SmallPenalty               1
// PENALTY_VectorHeightBigger         1
    return ulPenalty;
}


static
PPFE
HASHBUCKET_ppfeFindBestMatch(
    PHASHBUCKET pbkt,
    LOGFONTW *plf,
    ULONG *pulPenalty)
{
    PPFELINK ppfel;
    ULONG ulPenalty, ulBestMatch = *pulPenalty;
    PPFE ppfeBestMatch;

    /* Loop all PFELINKs */
    for (ppfel = pbkt->ppfelEnumHead; ppfel; ppfel = ppfel->ppfelNext)
    {
        ulPenalty = PFE_ulPenalty(ppfel->ppfe, plf, ulBestMatch);
        if (ulPenalty < ulBestMatch)
        {
            ppfeBestMatch = ppfel->ppfe;
            ulBestMatch = ulPenalty;
        }
    }

    return ppfeBestMatch;
}

static
PFONTHASH
FONTHASH_Allocate(
    FONT_HASH_TYPE fht,
    ULONG cBuckets)
{
    PFONTHASH pfh;

    pfh = ExAllocatePoolWithTag(PagedPool,
                                sizeof(FONTHASH) + cBuckets * sizeof(PVOID),
                                GDITAG_PFE_HASHTABLE);

    if (!pfh) return NULL;

    pfh->id = 'HSAH';
    pfh->fht = fht;
    pfh->cBuckets = cBuckets;
    pfh->cUsed = 0;
    pfh->cCollisions = 0;
    pfh->pbktFirst = NULL;
    pfh->pbktLast = NULL;
    RtlZeroMemory(pfh->apbkt, cBuckets * sizeof(PVOID));

    return pfh;
}


static
VOID
FONTHASH_vInsertBucket(
    PFONTHASH pfh,
    PHASHBUCKET pbkt,
    ULONG iHashValue)
{
    ULONG iHashIndex = iHashValue % pfh->cBuckets;

    pbkt->iHashValue = iHashValue;

    /* Insert the bucket into the list */
    pbkt->pbktPrev = pfh->pbktLast;
    pbkt->pbktNext = NULL;
    pfh->pbktLast = pbkt;
    if (!pfh->pbktFirst) pfh->pbktFirst = pbkt;

    /* Insert the bucket into the slot */
    pbkt->pbktCollision = pfh->apbkt[iHashIndex];
    pfh->apbkt[iHashIndex] = pbkt;

    /* Update counter */
    if (pbkt->pbktCollision) pfh->cCollisions++;
    else pfh->cUsed++;
}



static
PHASHBUCKET
FONTHASH_pbktFindBucketByName(
    PFONTHASH pfh,
    PWSTR pwszCapName,
    ULONG iHashValue)
{
    ULONG iHashIndex = iHashValue % pfh->cBuckets;
    PHASHBUCKET pbkt;

    /* Loop all colliding hash buckets */
    for (pbkt = pfh->apbkt[iHashIndex]; pbkt; pbkt = pbkt->pbktCollision)
    {
        /* Quick check */
        if (pbkt->iHashValue != iHashValue) continue;

        /* Compare the font name */
        if (wcsncmp(pbkt->u.wcCapName, pwszCapName, LF_FACESIZE) == 0) break;
    }

    return pbkt;
}


static
VOID
FONTHASH_vInsertPFE(
    PFONTHASH pfh,
    PPFE ppfe)
{
    PIFIMETRICS pifi = ppfe->pifi;
    PWSTR pwszName;
    WCHAR awcCapName[LF_FACESIZE];
    ULONG iHashValue;
    PHASHBUCKET pbkt;


    if (pfh->fht == FHT_UFI)
    {
        ASSERT(FALSE);
    }
    else
    {
        if (pfh->fht == FHT_FACE)
            pwszName = (PWSTR)((PUCHAR)pifi + pifi->dpwszFaceName);
        else
            pwszName = (PWSTR)((PUCHAR)pifi + pifi->dpwszFamilyName);

        UpcaseString(awcCapName, pwszName, LF_FACESIZE);
        iHashValue = CalculateNameHash(awcCapName);

        pbkt = FONTHASH_pbktFindBucketByName(pfh, awcCapName, iHashValue);
    }

    if (!pbkt)
    {
        pbkt = HASHBUCKET_Allocate();

        if (!pbkt)
        {
            ASSERT(FALSE);
        }

        RtlCopyMemory(pbkt->u.wcCapName, awcCapName, sizeof(awcCapName));

        FONTHASH_vInsertBucket(pfh, pbkt, iHashValue);
    }

    /* Finally add the PFE to the HASHBUCKET */
    if (!HASHBUCKET_bLinkPFE(pbkt, ppfe))
    {
        ASSERT(FALSE);
    }


}


BOOL
NTAPI
PFT_bInit(
    PFT *ppft)
{

    RtlZeroMemory(ppft, sizeof(PFT));

    ppft->hsem = EngCreateSemaphore();
    if (!ppft->hsem) goto failed;

    ppft->cBuckets = MAX_FONT_LIST;

    ppft->pfhFace = FONTHASH_Allocate(FHT_FACE, MAX_FONT_LIST);
    if (!ppft->pfhFace) goto failed;

    ppft->pfhFamily = FONTHASH_Allocate(FHT_FAMILY, MAX_FONT_LIST);
    if (!ppft->pfhFace) goto failed;

    ppft->pfhUFI = FONTHASH_Allocate(FHT_UFI, MAX_FONT_LIST);
    if (!ppft->pfhFace) goto failed;

    return TRUE;

failed:
    // FIXME: cleanup
    ASSERT(FALSE);

    return FALSE;
}

PPFE
NTAPI
PFT_ppfeFindBestMatch(
    _In_ PPFT ppft,
    _In_ PWSTR pwszCapName,
    _In_ ULONG iHashValue,
    _In_ LOGFONTW *plf,
    _Inout_ PULONG pulPenalty)
{
    PHASHBUCKET pbkt;
    ULONG ulPenalty = *pulPenalty;
    PFONTSUBSTITUTE pfs = NULL;
    PPFE ppfe;

    /* Acquire PFT lock for reading */
    EngAcquireSemaphoreShared(ppft->hsem);

    /* Try to find a HASHBUCKET by the face name */
    pbkt = FONTHASH_pbktFindBucketByName(ppft->pfhFace, pwszCapName, iHashValue);

    /* Check if we found the face name */
    if (!pbkt)
    {
        /* Check if there is a font substitute */
        pfs = GetFontSubstitute(pwszCapName);
        if (pfs)
        {
            /* Try again with the substitute name */
            pbkt = FONTHASH_pbktFindBucketByName(ppft->pfhFace,
                                                 pfs->awcCapSubstName,
                                                 pfs->iHashValue);
        }
    }

    if (pbkt)
    {
        ppfe = HASHBUCKET_ppfeFindBestMatch(pbkt, plf, &ulPenalty);
        if (pfs) ulPenalty += PENALTY_FaceNameSubst;

    }
    else
    {
        TRACE("Font with face name '%ls' was not found!\n", pwszCapName);
        ulPenalty = PENALTY_FaceName;
    }


    if (ulPenalty >= PENALTY_FaceName)
    {
    }

    /* Release PFT lock */
    EngReleaseSemaphore(ppft->hsem);

    *pulPenalty = ulPenalty;
    return ppfe;
}



static
PPFF
PFT_pffFindFontByFile(
    PFT *ppft,
    PWSTR pwszFiles,
    ULONG cwc,
    ULONG cFiles,
    ULONG iFileNameHash)
{
    ULONG iListIndex = iFileNameHash % ppft->cBuckets;
    PPFF ppff = NULL;

    /* Acquire PFT lock for reading */
    EngAcquireSemaphoreShared(ppft->hsem);

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
PFT_vInsertPFF(
    PPFT ppft,
    PPFF ppff,
    ULONG iFileNameHash)
{
    ULONG i, iHashIndex = iFileNameHash % ppft->cBuckets;

    ppff->iFileNameHash = iFileNameHash;

    /* Acquire PFT lock */
    EngAcquireSemaphore(ppft->hsem);

    /* Insert the font file into the hash bucket */
    ppff->pPFFPrev = NULL;
    ppff->pPFFNext = ppft->apPFF[iHashIndex];
    ppft->apPFF[iHashIndex] = ppff;

    ppft->cFiles++;

    /* Loop all PFE's */
    for (i = 0; i < ppff->cFonts; i++)
    {
        FONTHASH_vInsertPFE(ppft->pfhFace, &ppff->apfe[i]);
        FONTHASH_vInsertPFE(ppft->pfhFamily, &ppff->apfe[i]);
        //FONTHASH_vInsertPFE(ppft->pfhUFI, &ppff->apfe[i]);
    }

    /* Release PFT lock */
    EngReleaseSemaphore(ppft->hsem);
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
    ppff = PFT_pffFindFontByFile(ppft, pwszFiles, cwc, cFiles, iFileNameHash);

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
        ERR("Failed to load font with font driver\n");
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
        UpcaseString(pwszUpcase, pwszFiles, cwc);

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

