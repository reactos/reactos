/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include "font.h"

#define NDEBUG
#include <debug.h>

typedef struct _FONTDEV
{
    LIST_ENTRY leLink;
    DHPDEV dhpdev;
    PLDEVOBJ pldev;
    HSURF ahsurf[HS_DDI_MAX];
    DEVINFO devinfo;

    GDIINFO gdiinfo; // FIXME: something else?
} FONTDEV, *PFONTDEV;

C_ASSERT(sizeof(GDIINFO) == 0x130);


BOOL gbAttachedCSRSS;

HSEMAPHORE ghsemFontDriver;
LIST_ENTRY gleFontDriverList = {&gleFontDriverList, &gleFontDriverList};

extern HSEMAPHORE ghsemPFFList;

BOOL FASTCALL
InitFontSupport(VOID)
{
    ghsemFontDriver = EngCreateSemaphore();
    if (!ghsemFontDriver) return FALSE;
    ghsemPFFList = EngCreateSemaphore();
    if (!ghsemPFFList) return FALSE;
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

VOID
NTAPI
RFONT_vInitDeviceMetrics(
    PRFONT prfnt)
{
    PFONTDEV pfntdev = (PFONTDEV)prfnt->hdevProducer;

    pfntdev->pldev->pfn.QueryFontData(prfnt->dhpdev,
                                      &prfnt->fobj,
                                      QFD_MAXEXTENTS,
                                      -1,
                                      NULL,
                                      &prfnt->fddm,
                                      sizeof(FD_DEVICEMETRICS));
}

static void
PFE_vInitialize(
    PPFE ppfe,
    PPFF ppff,
    ULONG iFace)
{
    PFONTDEV pfntdev = (PFONTDEV)ppff->hdev;
    PLDEVOBJ pldev = pfntdev->pldev;
    ppfe->pPFF = ppff;
    ppfe->iFont = iFace;
    ppfe->flPFE = 0;

    ppfe->pid = HandleToUlong(PsGetCurrentProcessId());
    ppfe->tid = HandleToUlong(PsGetCurrentThreadId());

    /* Query IFIMETRICS */
    ppfe->pifi = pldev->pfn.QueryFont(pfntdev->dhpdev,
                                      ppff->hff,
                                      iFace,
                                      &ppfe->idifi);

    /* Query FD_GLYPHSET */
    ppfe->pfdg = pldev->pfn.QueryFontTree(pfntdev->dhpdev,
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

PPFF
NTAPI
EngLoadFontFileFD(
    ULONG cFiles,
    PFONTFILEVIEW *ppffv,
    DESIGNVECTOR *pdv,
    ULONG ulCheckSum)
{
    PULONG_PTR piFiles = (PULONG_PTR)ppffv;
    PVOID apvView[FD_MAX_FILES];
    ULONG acjView[FD_MAX_FILES];
    KAPC_STATE ApcState;
    PLIST_ENTRY ple;
    PFONTDEV pfntdev = NULL;
    HFF hff = 0;
    ULONG cFaces, cjSize, i, ulLangID = 0;
    PPFF ppff = NULL;

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
    EngAcquireSemaphore(ghsemFontDriver);

    /* Loop all installed font drivers */
    for (ple = gleFontDriverList.Flink;
         ple != &gleFontDriverList;
         ple = ple->Flink)
    {
        pfntdev = CONTAINING_RECORD(ple, FONTDEV, leLink);

        /* Call the drivers DrvLoadFontFile function */
        hff = pfntdev->pldev->pfn.LoadFontFile(cFiles,
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
    cFaces = pfntdev->pldev->pfn.QueryFontFile(hff, QFF_NUMFACES, 0, NULL);

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
    ppff->hdev = (HDEV)pfntdev;
    ppff->hff = hff;

    /* Copy the FONTFILEVIEW pointers */
    for (i = 0; i < cFiles; i++) ppff->apffv[i] = ppffv[i];

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
EngLoadFontDriver(
    IN PWSTR pwszDriverName)
{
    PLDEVOBJ pldev;
    PFONTDEV pfntdev;

    /* Load the driver */
    pldev = EngLoadImageEx(pwszDriverName, LDEV_FONT);
    if (!pldev)
    {
        DPRINT1("Failed to load freetype font driver\n");
        return FALSE;
    }

    // CHECK if all functions are there


    /* Allocate a FONTDEV structure */
    pfntdev = EngAllocMem(0, sizeof(FONTDEV), 'vdfG');
    if (!pfntdev)
    {
        DPRINT1("Failed to allocate FONTDEV structure\n");
        EngUnloadImage(pldev);
        return FALSE;
    }

    pfntdev->pldev = pldev;

    /* Call the drivers DrvEnablePDEV function */
    pfntdev->dhpdev = pldev->pfn.EnablePDEV(NULL,
                                            NULL,
                                            HS_DDI_MAX,
                                            pfntdev->ahsurf,
                                            sizeof(GDIINFO),
                                            &pfntdev->gdiinfo,
                                            sizeof(DEVINFO),
                                            &pfntdev->devinfo,
                                            (HDEV)pfntdev,
                                            NULL,
                                            NULL);

    /* Call the drivers DrvCompletePDEV function */
    pldev->pfn.CompletePDEV(pfntdev->dhpdev, (HDEV)pfntdev);

    /* Insert the driver into the list */
    EngAcquireSemaphore(ghsemFontDriver);
    InsertTailList(&gleFontDriverList, &pfntdev->leLink);
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
    cFonts = GreAddFontResourceInternal(&pwszFile, 1, 0, 0, NULL);
    ASSERT(cFonts > 0);

}

