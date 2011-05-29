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

HSEMAPHORE ghsemPFFList;
LIST_ENTRY glePFFList = {&glePFFList, &glePFFList};


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

static
HFF
FONTDEV_hffLoadFontFile(
    PFONTDEV pfntdev,
    ULONG cFiles,
    ULONG_PTR *piFile,
    PVOID *ppvView,
    ULONG *pcjView,
    DESIGNVECTOR *pdv,
    ULONG ulLangID,
    ULONG ulFastCheckSum)
{
    HFF hff;
    ASSERT(gbAttachedCSRSS);

    /* Call the drivers DrvLoadFontFile function */
    hff = pfntdev->pldev->pfn.LoadFontFile(cFiles,
                                           piFile,
                                           ppvView,
                                           pcjView,
                                           pdv,
                                           ulLangID,
                                           ulFastCheckSum);

    if (hff == 0) return 0;

    return hff;
}

static
HFF
EngLoadFontFileFD(
    ULONG cFiles,
    PFONTFILEVIEW *ppffv,
    DESIGNVECTOR *pdv,
    ULONG ulCheckSum,
    PFONTDEV *ppfntdev)
{
    PULONG_PTR piFiles = (PULONG_PTR)ppffv;
    PVOID apvView[FD_MAX_FILES];
    ULONG acjView[FD_MAX_FILES];
    ULONG i, ulLangID = 0;
    PFONTDEV pfntdev;
    PLIST_ENTRY ple;
    HFF hff = 0;

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

        /* Try to load the font file */
        hff = FONTDEV_hffLoadFontFile(pfntdev,
                                      cFiles,
                                      piFiles,
                                      apvView,
                                      acjView,
                                      pdv,
                                      ulLangID,
                                      ulCheckSum);
        if (hff)
        {
            *ppfntdev = pfntdev;
            break;
        }
    }

    /* Release font friver list lock */
    EngReleaseSemaphore(ghsemFontDriver);

    return hff;
}

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

static PPFF
EngFindPFF(ULONG cFiles, PFONTFILEVIEW *ppffv)
{
    PLIST_ENTRY ple;
    PPFF ppff;
    ASSERT(cFiles >= 1 && cFiles <= FD_MAX_FILES);

    /* Acquire PFF list lock */
    EngAcquireSemaphore(ghsemPFFList);

    /* Loop all physical font files (PFF) */
    for (ple = glePFFList.Flink; ple != &glePFFList; ple = ple->Flink)
    {
        ppff = CONTAINING_RECORD(ple, PFF, leLink);

        /* Check if the files are already loaded */
        if (PFF_bCompareFiles(ppff, cFiles, ppffv)) break;
    }

    /* Release PFF list lock */
    EngReleaseSemaphore(ghsemPFFList);

    return ple != &glePFFList ? ppff : NULL;
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
EngLoadFontFile(
    IN ULONG cFiles,
    IN PWCHAR apwszFiles[],
    IN DESIGNVECTOR *pdv)
{
    PFONTFILEVIEW apffv[FD_MAX_FILES];
    KAPC_STATE ApcState;
    PPFF ppff = NULL;
    ULONG i, cjSize, cFaces, ulChecksum = 0;
    PFONTDEV pfntdev = NULL;
    HFF hff;

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

    /* Try to find an existing PFF */
    ppff = EngFindPFF(cFiles, apffv);
    if (ppff)
    {
        /* Cleanup loaded files, we don't need them anymore */
        for (i = 0; i < cFiles; i++) EngFreeModule(apffv[i]);
        return ppff;
    }

    /* Attach to CSRSS */
    AttachCSRSS(&ApcState);

    /* Try to load the font with any of the font drivers */
    hff = EngLoadFontFileFD(cFiles, apffv, pdv, ulChecksum, &pfntdev);
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
    for (i = 0; i < cFiles; i++) ppff->apffv[i] = apffv[i];

    /* Loop all faces in the font file */
    for (i = 0; i < cFaces; i++)
    {
        /* Initialize the face */
        PFE_vInitialize(&ppff->apfe[i], ppff, i + 1);
    }

    /* Insert the PFF into the list */
    EngAcquireSemaphore(ghsemPFFList);
    InsertTailList(&glePFFList, &ppff->leLink);
    EngReleaseSemaphore(ghsemPFFList);

leave:
    if (!ppff)
    {
        for (i = 0; i < cFiles; i++) EngFreeModule(apffv[i]);
    }

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
    /* Load freetype font driver */
    if (!EngLoadFontDriver(L"ftfd.dll"))
    {
        DPRINT1("Could not load freetype font driver\n");
        KeBugCheck(VIDEO_DRIVER_INIT_FAILURE);
    }


    /* TODO: Enumerate installed font drivers */
    DPRINT1("############ Started font drivers\n");
}

