/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include <include/font.h>

#define NDEBUG
#include <debug.h>

INT
FASTCALL
LFONT_GetObject(PLFONT plfnt, INT cjSize, PVOID pvBuffer)
{
    ASSERT(FALSE);
    return 0;
}

PPFE
NTAPI
LFONT_ppfe(PLFONT plfnt)
{
    PPFEOBJ ppfeobj;
    PPFE ppfe, ppfeSave;
    HGDIOBJ hPFE;
    ULONG ulPenalty;

    /* Check if the font has a PFE associated */
    hPFE = plfnt->hPFE;
    if (hPFE)
    {
        /* Try to lock the PFE */
        ppfeobj = (PVOID)GDIOBJ_ReferenceObjectByHandle(hPFE, 0x0c);

        /* If we succeeded, the font is still there and we can use it */
        if (ppfeobj) return ppfeobj->ppfe;

        /* The previous font is not loaded anymore, reset the handle */
        InterlockedCompareExchangePointer((PVOID*)&plfnt->hPFE, hPFE, NULL);
    }

    ulPenalty = PENALTY_Max;
#if 0
    /* Search the private font table */
    pti = PsGetCurrentThreadWin32Thread();
    ppfe = PFT_ppfeFindBestMatch(pti->ppftPrivate,
                                 plfnt->awchFace,
                                 plfnt->iNameHash,
                                 &plfnt->elfexw.elfEnumLogfontEx.elfLogFont,
                                 &ulPenalty);
#else
    ppfe = 0;
#endif
    /* Check if we got an exact match */
    if (ulPenalty != 0)
    {
        /* Not an exact match, save this PFE */
        ppfeSave = ppfe;

        /* Try to find a better match in the global table */
        ppfe = PFT_ppfeFindBestMatch(&gpftPublic,
                                     plfnt->awchFace,
                                     plfnt->iNameHash,
                                     &plfnt->elfexw.elfEnumLogfontEx.elfLogFont,
                                     &ulPenalty);

        /* If we didn't find a better one, use the old one */
        if (!ppfe) ppfe = ppfeSave;
    }

    /* Check if we got an exact match now */
    if (ulPenalty == 0)
    {
        __debugbreak();
        // should create a PFEOBJ and save it
    }

    ASSERT(ppfe);
    return ppfe;
}

PRFONT
NTAPI
LFONT_prfntFindRFONT(
    IN PLFONT plfnt)
{

    __debugbreak();

    return NULL;
}


HFONT
NTAPI
GreHfontCreate(
    IN ENUMLOGFONTEXDVW *pelfw,
    IN ULONG cjElfw,
    IN LFTYPE lft,
    IN FLONG  fl,
    IN PVOID pvCliData)
{
    PLFONT plfnt;
    HFONT hfont;

    /* Allocate an LFONT object */
    plfnt = (PLFONT)GDIOBJ_AllocObjWithHandle(GDIObjType_LFONT_TYPE << 16, sizeof(LFONT));
    if (!plfnt)
    {
        DPRINT1("Could not allocate LFONT object\n");
        return 0;
    }

    /* Get the handle */
    hfont = plfnt->baseobj.hHmgr;

    /* Set the fields */
    plfnt->lft = lft;
    plfnt->fl = fl;
    plfnt->elfexw = *pelfw;

    /* Upcase the font name */
    UpcaseString(plfnt->awchFace,
                 pelfw->elfEnumLogfontEx.elfLogFont.lfFaceName,
                 LF_FACESIZE);

    /* Calculate the name hash value */
    plfnt->iNameHash = CalculateNameHash(plfnt->awchFace);

    /* Set client data */
    GDIOBJ_vSetObjectAttr(&plfnt->baseobj, pvCliData);

    /* Unlock the object */
    GDIOBJ_vUnlockObject(&plfnt->baseobj);

    /* Return the handle */
    return hfont;
}


W32KAPI
HFONT
APIENTRY
NtGdiHfontCreate(
    IN ENUMLOGFONTEXDVW *pelfw,
    IN ULONG cjElfw,
    IN LFTYPE lft,
    IN FLONG  fl,
    IN PVOID pvCliData)
{
    ENUMLOGFONTEXDVW elfw;

    if (!pelfw)
    {
        DPRINT1("NtGdiHfontCreate: pelfw == NULL\n");
        return NULL;
    }

    /* Set maximum logfont size */
    if (cjElfw > sizeof(elfw)) cjElfw = sizeof(elfw);

    /* Enter SEH for buffer copy */
    _SEH2_TRY
    {
        ProbeForRead(pelfw, cjElfw, 1);
        RtlCopyMemory(&elfw, pelfw, cjElfw);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return NULL;)
    }
    _SEH2_END

    /* Call internal function */
    return GreHfontCreate(&elfw, cjElfw, lft, fl, pvCliData);
}

