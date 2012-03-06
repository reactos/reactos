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

C_ASSERT(sizeof(GDIINFO) == 0x130);


BOOL gbAttachedCSRSS;

HSEMAPHORE ghsemFontDriver;
LIST_ENTRY gleFontDriverList = {&gleFontDriverList, &gleFontDriverList};

extern PFT gpftPublic;

BOOL FASTCALL
InitFontSupport(VOID)
{
    ghsemFontDriver = EngCreateSemaphore();
    if (!ghsemFontDriver) return FALSE;

    /* Initialize the global public font taböe */
    if (!PFT_bInit(&gpftPublic)) return FALSE;

    return TRUE;
}

static
VOID
AttachCSRSS(KAPC_STATE *pApcState)
{
    ASSERT(gpepCSRSS);
    KeStackAttachProcess(&gpepCSRSS->Pcb, pApcState);
    gbAttachedCSRSS = TRUE;
}

static
VOID
DetachCSRSS(KAPC_STATE *pApcState)
{
    ASSERT(gbAttachedCSRSS);
    KeUnstackDetachProcess(pApcState);
    gbAttachedCSRSS = FALSE;
}

static
VOID
PFE_vInitialize(
    PPFE ppfe,
    PPFF ppff,
    ULONG iFace)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)ppff->hdev;
    PLDEVOBJ pldev = ppdev->pldev;
    ppfe->pPFF = ppff;
    ppfe->iFont = iFace;
    ppfe->flPFE = 0;

    ppfe->pid = HandleToUlong(PsGetCurrentProcessId());
    ppfe->tid = HandleToUlong(PsGetCurrentThreadId());

    /* Query IFIMETRICS */
    ppfe->pifi = pldev->pfn.QueryFont(ppdev->dhpdev,
                                      ppff->hff,
                                      iFace,
                                      &ppfe->idifi);

    /* Query FD_GLYPHSET */
    ppfe->pfdg = pldev->pfn.QueryFontTree(ppdev->dhpdev,
                                          ppff->hff,
                                          iFace,
                                          QFT_GLYPHSET,
                                          &ppfe->idfdg);

    /* No kerning pairs for now */
    ppfe->pkp = NULL;
    ppfe->idkp = 0;
    ppfe->ckp = 0;

    ppfe->iOrientation = 0;
    ppfe->cjEfdwPFE = 0;
    //ppfe->pgiset = 0;
    ppfe->ulTimeStamp = 0;
    //ppfe->ufi = 0;
    //ppfe->ql;
    ppfe->pFlEntry = 0;
    ppfe->cAlt = 0;
    ppfe->cPfdgRef = 0;
    //ppfe->aiFamilyName[];

}

ULONG
NTAPI
PFE_ulQueryTrueTypeTable(
    PPFE ppfe,
    ULONG ulTableTag,
    PTRDIFF dpStart,
    ULONG cjBuffer,
    PVOID pvBuffer)
{
    PPDEVOBJ ppdev = (PDEVOBJ*)ppfe->pPFF->hdev;
    KAPC_STATE ApcState;
    ULONG ulResult;

    /* Attach to CSRSS */
    AttachCSRSS(&ApcState);

    /* Call the driver to copy the requested data */
    ulResult = ppdev->pfn.QueryTrueTypeTable(ppfe->pPFF->hff,
                                             ppfe->iFont,
                                             ulTableTag,
                                             dpStart,
                                             cjBuffer,
                                             pvBuffer,
                                             NULL,
                                             NULL);

    /* Detach from CSRSS */
    DetachCSRSS(&ApcState);

    /* Return the result */
    return ulResult;
}


static
VOID
CopyFileName(
    OUT PWCHAR pwcDest,
    ULONG cwcBufSize,
    IN OUT PWCHAR *ppwcSource)
{
    PWCHAR pwcSource = *ppwcSource;
    ULONG cwc = 0;
    WCHAR wc;

    while (++cwc < cwcBufSize)
    {
        wc = *pwcSource++;

        if ((wc == '|') || (wc == 0)) break;

        *pwcDest++ = wc;
    }

    /* Zero terminate the destination string */
    *pwcDest = 0;

    *ppwcSource = pwcSource;
}

PPFF
NTAPI
EngLoadFontFileFD(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    DESIGNVECTOR *pdv,
    ULONG ulCheckSum)
{
    PVOID apvView[FD_MAX_FILES];
    ULONG acjView[FD_MAX_FILES];
    PFONTFILEVIEW apffv[FD_MAX_FILES];
    PULONG_PTR piFiles = (PULONG_PTR)apffv;
    WCHAR awcFileName[MAX_PATH];
    PWCHAR pwcCurrent;
    KAPC_STATE ApcState;
    PLIST_ENTRY ple;
    PPDEVOBJ ppdev = NULL;
    HFF hff = 0;
    ULONG cFaces, cjSize, i, ulLangID = 0;
    PPFF ppff = NULL;

    pwcCurrent = pwszFiles;

    /* Loop the files */
    for (i = 0; i < cFiles; i++)
    {
        /* Extract a file name */
        CopyFileName(awcFileName, MAX_PATH, &pwcCurrent);

        /* Try to load the file */
        apffv[i] = (PVOID)EngLoadModuleEx(awcFileName, 0, FVF_FONTFILE);
        if (!apffv[i])
        {
            DPRINT1("Failed to load file: '%ls'\n", awcFileName);
            /* Cleanup and return */
            while (i--) EngFreeModule(apffv[i]);
            return NULL;
        }
    }

    /* Attach to CSRSS */
    AttachCSRSS(&ApcState);

    /* Loop all files */
    for (i = 0; i < cFiles; i++)
    {
        /* Map the font file */
        if (!EngMapFontFileFD(piFiles[i], (PULONG*)&apvView[i], &acjView[i]))
        {
            ASSERT(FALSE);
        }
    }

    /* Acquire font driver list lock */
    EngAcquireSemaphoreShared(ghsemFontDriver);

    /* Loop all installed font drivers */
    for (ple = gleFontDriverList.Flink;
         ple != &gleFontDriverList;
         ple = ple->Flink)
    {
        ppdev = CONTAINING_RECORD(ple, PDEVOBJ, leLink);

        /* Call the drivers DrvLoadFontFile function */
        hff = ppdev->pldev->pfn.LoadFontFile(cFiles,
                                             piFiles,
                                             apvView,
                                             acjView,
                                             pdv,
                                             ulLangID,
                                             ulCheckSum);
        if (hff) break;
    }

    /* Release font driver list lock */
    EngReleaseSemaphore(ghsemFontDriver);

    if (!hff)
    {
        DPRINT1("File format is not supported by any font driver\n");
        goto leave;
    }

    /* Query the number of faces in the font file */
    cFaces = ppdev->pldev->pfn.QueryFontFile(hff, QFF_NUMFACES, 0, NULL);

    /* Allocate a new PFF */
    cjSize = FIELD_OFFSET(PFF, apfe[cFaces]);
    ppff = EngAllocMem(FL_ZERO_MEMORY, cjSize, 'ffpG');
    if (!ppff)
    {
        DPRINT1("Failed to allocate %ld bytes\n", cjSize);
        goto leave;
    }

    /* Fill the structure */
    ppff->sizeofThis = cjSize;
    ppff->cFiles = cFiles;
    ppff->cFonts = cFaces;
    ppff->hdev = (HDEV)ppdev;
    ppff->hff = hff;

    /* Copy the FONTFILEVIEW pointers */
    for (i = 0; i < cFiles; i++) ppff->apffv[i] = apffv[i];

    /* Loop all faces in the font file */
    for (i = 0; i < cFaces; i++)
    {
        /* Initialize the face */
        PFE_vInitialize(&ppff->apfe[i], ppff, i + 1);
    }

leave:

    /* Detach from CSRSS */
    DetachCSRSS(&ApcState);

    return ppff;
}

BOOL
NTAPI
EngLoadFontDriver(
    IN PWSTR pwszDriverName)
{
    PLDEVOBJ pldev;
    PPDEVOBJ ppdev;

    /* Try to load the driver */
    pldev = EngLoadImageEx(pwszDriverName, LDEV_FONT);
    if (!pldev)
    {
        DPRINT1("Could not load display driver '%ls'\n", pwszDriverName);
        return FALSE;
    }

    /* Create a new PDEVOBJ */
    ppdev = PDEVOBJ_CreatePDEV(pldev, NULL, NULL, NULL);
    if (!ppdev)
    {
        DPRINT1("failed to allocate a PDEV\n");
        EngUnloadImage(pldev);
        return FALSE;
    }

    /* Insert the driver into the list */
    EngAcquireSemaphore(ghsemFontDriver);
    InsertTailList(&gleFontDriverList, &ppdev->leLink);
    EngReleaseSemaphore(ghsemFontDriver);

    return TRUE;
}

VOID
NTAPI
GreStartupFontDrivers(VOID)
{
    ULONG cFonts;
    static PWSTR pwszFile = L"\\??\\c:\\ReactOS\\Fonts\\tahoma.ttf";

    /* Load freetype font driver */
    if (!EngLoadFontDriver(L"ftfd.dll"))
    {
        DPRINT1("Could not load freetype font driver\n");
        KeBugCheck(VIDEO_DRIVER_INIT_FAILURE);
    }


    /* TODO: Enumerate installed font drivers */
    DPRINT1("############ Started font drivers\n");

    // lets load some fonts
    cFonts = GreAddFontResourceW(pwszFile, 31, 1, 0, 0, NULL);
    ASSERT(cFonts > 0);

}

