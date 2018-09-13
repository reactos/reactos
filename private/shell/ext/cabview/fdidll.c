/*
 *  FDIDLL.C -- FDI interface using CABINET.DLL
 *
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1997
 *  All Rights Reserved.
 *
 *  Author:
 *      Mike Sliger
 *
 *  History:
 *      21-Jan-1997 msliger Initial version
 *      24-Jan-1997 msliger Changed to public include file
 *
 *  Overview:
 *      This code is a wrapper which provides access to the actual FDI code
 *      in CABINET.DLL.  CABINET.DLL dynamically loads/unloads as needed.
 */

#include <windows.h>

#include "fdi.h"

static HINSTANCE hCabinetDll;   /* DLL module handle */

/* pointers to the functions in the DLL */

static HFDI (FAR DIAMONDAPI *pfnFDICreate)(
        PFNALLOC            pfnalloc,
        PFNFREE             pfnfree,
        PFNOPEN             pfnopen,
        PFNREAD             pfnread,
        PFNWRITE            pfnwrite,
        PFNCLOSE            pfnclose,
        PFNSEEK             pfnseek,
        int                 cpuType,
        PERF                perf);
static BOOL (FAR DIAMONDAPI *pfnFDIIsCabinet)(
        HFDI                hfdi,
        INT_PTR             hf,
        PFDICABINETINFO     pfdici);
static BOOL (FAR DIAMONDAPI *pfnFDICopy)(
        HFDI                hfdi,
        char                *pszCabinet,
        char                *pszCabPath,
        int                 flags,
        PFNFDINOTIFY        pfnfdin,
        PFNFDIDECRYPT       pfnfdid,
        void                *pvUser);
static BOOL (FAR DIAMONDAPI *pfnFDIDestroy)(
        HFDI                hfdi);


/*
 *  FDICreate -- Create an FDI context
 *
 *  See fdi_int.h for entry/exit conditions.
 */

HFDI FAR DIAMONDAPI FDICreate(PFNALLOC pfnalloc,
                              PFNFREE  pfnfree,
                              PFNOPEN  pfnopen,
                              PFNREAD  pfnread,
                              PFNWRITE pfnwrite,
                              PFNCLOSE pfnclose,
                              PFNSEEK  pfnseek,
                              int      cpuType,
                              PERF     perf)
{
    HFDI hfdi;

    hCabinetDll = LoadLibrary(TEXT("CABINET"));
    if (hCabinetDll == NULL)
    {
        return(NULL);
    }

    pfnFDICreate = (void *) GetProcAddress(hCabinetDll,"FDICreate");
    pfnFDICopy = (void *) GetProcAddress(hCabinetDll,"FDICopy");
    pfnFDIIsCabinet = (void *) GetProcAddress(hCabinetDll,"FDIIsCabinet");
    pfnFDIDestroy = (void *) GetProcAddress(hCabinetDll,"FDIDestroy");

    if ((pfnFDICreate == NULL) ||
        (pfnFDICopy == NULL) ||
        (pfnFDIIsCabinet == NULL) ||
        (pfnFDIDestroy == NULL))
    {
        FreeLibrary(hCabinetDll);

        return(NULL);
    }

    hfdi = pfnFDICreate(pfnalloc,pfnfree,
            pfnopen,pfnread,pfnwrite,pfnclose,pfnseek,cpuType,perf);
    if (hfdi == NULL)
    {
        FreeLibrary(hCabinetDll);
    }

    return(hfdi);
}


/*
 *  FDIIsCabinet -- Determines if file is a cabinet, returns info if it is
 *
 *  See fdi_int.h for entry/exit conditions.
 */

BOOL FAR DIAMONDAPI FDIIsCabinet(HFDI            hfdi,
                                 INT_PTR         hf,
                                 PFDICABINETINFO pfdici)
{
    if (pfnFDIIsCabinet == NULL)
    {
        return(FALSE);
    }

    return(pfnFDIIsCabinet(hfdi,hf,pfdici));
}


/*
 *  FDICopy -- extracts files from a cabinet
 *
 *  See fdi_int.h for entry/exit conditions.
 */

BOOL FAR DIAMONDAPI FDICopy(HFDI          hfdi,
                            char         *pszCabinet,
                            char         *pszCabPath,
                            int           flags,
                            PFNFDINOTIFY  pfnfdin,
                            PFNFDIDECRYPT pfnfdid,
                            void         *pvUser)
{
    if (pfnFDICopy == NULL)
    {
        return(FALSE);
    }

    return(pfnFDICopy(hfdi,pszCabinet,pszCabPath,flags,pfnfdin,pfnfdid,pvUser));
}


/*
 *  FDIDestroy -- Destroy an FDI context
 *
 *  See fdi_int.h for entry/exit conditions.
 */

BOOL FAR DIAMONDAPI FDIDestroy(HFDI hfdi)
{
    BOOL rc;

    if (pfnFDIDestroy == NULL)
    {
        return(FALSE);
    }

    rc = pfnFDIDestroy(hfdi);
    if (rc == TRUE)
    {
        FreeLibrary(hCabinetDll);
    }

    return(rc);
}
