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

BOOL gbAttachedCSRSS;

HSEMAPHORE ghsemFontDriver;
LIST_ENTRY gleFontDriverList = {&gleFontDriverList, &gleFontDriverList};


BOOL FASTCALL
InitFontSupport(VOID)
{
    ghsemFontDriver = EngCreateSemaphore();
    if (!ghsemFontDriver) return FALSE;
    return TRUE;
}


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


//static
VOID
AttachCSRSS(KAPC_STATE *pApcState)
{
    ASSERT(gpepCSRSS);
    KeStackAttachProcess(&gpepCSRSS->Pcb, pApcState);
    gbAttachedCSRSS = TRUE;
}

//static
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

HFF
NTAPI
EngLoadFontFileFD(
    ULONG cFiles,
    PULONG_PTR piFiles,
    DESIGNVECTOR *pdv,
    ULONG ulCheckSum,
    HDEV *phdev)
{
    KAPC_STATE ApcState;
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

    /* Attach to CSRSS */
    AttachCSRSS(&ApcState);

    /* Acquire font friver list lock */
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
            *phdev = (HDEV)pfntdev;
            break;
        }
    }

    /* Release font friver list lock */
    EngReleaseSemaphore(ghsemFontDriver);

    /* Detach from CSRSS */
    DetachCSRSS(&ApcState);

    return hff;
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

