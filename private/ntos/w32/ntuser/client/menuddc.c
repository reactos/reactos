/****************************** Module Header ******************************\
* Module Name: menudd.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Menu drag and drop - client
*
* History:
* 10/29/96  GerardoB    Created
\***************************************************************************/
#include "precomp.h"
#pragma hdrstop

/*
 * OLE's GUID initialization
 */
#include "initguid.h"

/*
 * Macro to cast OLE's IDropTarget pointer to internal pointer
 */
#define PMNIDT(pdt) ((PMNIDROPTARGET)pdt)

/*
 * The mndt* functions implement the IDropTarget interface
 */
/**************************************************************************\
* mndtAddRef
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
ULONG mndtAddRef(LPDROPTARGET pdt)
{
    return ++(PMNIDT(pdt)->dwRefCount);
}

/**************************************************************************\
* mndtQueryInterface
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
HRESULT mndtQueryInterface(LPDROPTARGET pdt, REFIID riid, PVOID * ppvObj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDropTarget)) {
        mndtAddRef(pdt);
        *ppvObj = pdt;
        return NOERROR;
    } else {
        return E_NOINTERFACE;
    }
}


/**************************************************************************\
* mndtRelease
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
ULONG mndtRelease(LPDROPTARGET pdt)
{
    if (--(PMNIDT(pdt)->dwRefCount) != 0) {
        return PMNIDT(pdt)->dwRefCount;
    }

    LocalFree(pdt);
    return NOERROR;
}

/**************************************************************************\
* mndtDragOver
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
HRESULT mndtDragOver(LPDROPTARGET pdt, DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    MNDRAGOVERINFO mndoi;
    MENUGETOBJECTINFO mngoi;

    /*
     * Get the dragover info for the selection corresponding to this point
     */
    if (!NtUserMNDragOver((POINT *)&ptl, &mndoi)) {
        RIPMSG0(RIP_WARNING, "mndtDragOver: NtUserDragOver failed");
        *pdwEffect = DROPEFFECT_NONE;
        return NOERROR;
    }

    /*
     * If not switching items or crossing gap boundaries, pass the
     *  the drag over.
     */
    if (!(mndoi.dwFlags & MNGOF_CROSSBOUNDARY)) {
        if (PMNIDT(pdt)->pidt != NULL) {
            return PMNIDT(pdt)->pidt->lpVtbl->DragOver(PMNIDT(pdt)->pidt, grfKeyState, ptl, pdwEffect);
        }
    } else {
        /*
         *  DragLeave and Release the current item, if any
         */
        if (PMNIDT(pdt)->pidt != NULL) {
            PMNIDT(pdt)->pidt->lpVtbl->DragLeave(PMNIDT(pdt)->pidt);
            PMNIDT(pdt)->pidt->lpVtbl->Release(PMNIDT(pdt)->pidt);
            PMNIDT(pdt)->pidt = NULL;
        }

        /*
         * If an item is selected, Get the interface for it
         */
        if (mndoi.uItemIndex != MFMWFP_NOITEM) {
            mngoi.hmenu = mndoi.hmenu;
            mngoi.dwFlags = mndoi.dwFlags & MNGOF_GAP;
            mngoi.uPos = mndoi.uItemIndex;
            mngoi.riid = (PVOID)&IID_IDropTarget;
            mngoi.pvObj = NULL;

            if (MNGO_NOERROR == SendMessage(mndoi.hwndNotify, WM_MENUGETOBJECT, 0, (LPARAM)&mngoi)) {
                PMNIDT(pdt)->pidt = mngoi.pvObj;
            }
        }

        /*
         * If we got a new interface, AddRef and DragEnter it
         */
        if (PMNIDT(pdt)->pidt != NULL) {
            PMNIDT(pdt)->pidt->lpVtbl->AddRef(PMNIDT(pdt)->pidt);
            return PMNIDT(pdt)->pidt->lpVtbl->DragEnter(PMNIDT(pdt)->pidt, PMNIDT(pdt)->pido, grfKeyState, ptl, pdwEffect);
        }
    } /* if (!(mndoi.dwFlags & MNGOF_CROSSBOUNDARY)) */

    *pdwEffect = DROPEFFECT_NONE;
    return NOERROR;
}
/**************************************************************************\
* mndtDragEnter
*
* 10/28/96 GerardoB     Created
\**************************************************************************/

HRESULT mndtDragEnter(LPDROPTARGET pdt, LPDATAOBJECT pdo, DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    /*
     * Save the IDataObject
     */
    PMNIDT(pdt)->pido = pdo;

    /*
     * DragEnter is the same as a DragOver; only that we will never fail it
     */
    mndtDragOver(pdt, grfKeyState, ptl, pdwEffect);

    return NOERROR;
}
/**************************************************************************\
* mndtDragLeave
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
HRESULT mndtDragLeave(LPDROPTARGET pdt)
{
    /*
     * Let the kernel mode clean up
     */
    NtUserMNDragLeave();

    /*
     *  DragLeave and Release the current item, if any
     */
    if (PMNIDT(pdt)->pidt != NULL) {
        PMNIDT(pdt)->pidt->lpVtbl->DragLeave(PMNIDT(pdt)->pidt);
        PMNIDT(pdt)->pidt->lpVtbl->Release(PMNIDT(pdt)->pidt);
        PMNIDT(pdt)->pidt = NULL;
    }

    return NOERROR;
}

/**************************************************************************\
* mndtDrop
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
HRESULT mndtDrop(LPDROPTARGET pdt, LPDATAOBJECT pdo, DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    HRESULT hres;

    /*
     * If we got a target, pass the drop and release it.
     */
    if (PMNIDT(pdt)->pidt != NULL) {
        hres = PMNIDT(pdt)->pidt->lpVtbl->Drop(PMNIDT(pdt)->pidt, pdo, grfKeyState, ptl, pdwEffect);
        PMNIDT(pdt)->pidt->lpVtbl->Release(PMNIDT(pdt)->pidt);
        PMNIDT(pdt)->pidt = NULL;
    } else {
        *pdwEffect = DROPEFFECT_NONE;
        hres = NOERROR;
    }

    /*
     * Clean up
     */
    mndtDragLeave(pdt);

    return hres;
}

/**************************************************************************\
* Drop target VTable
*
\**************************************************************************/
IDropTargetVtbl idtVtbl = {
    mndtQueryInterface,
    mndtAddRef,
    mndtRelease,
    mndtDragEnter,
    mndtDragOver,
    mndtDragLeave,
    mndtDrop
};
/**************************************************************************\
* __ClientRegisterDragDrop
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
DWORD __ClientRegisterDragDrop(HWND * phwnd)
{
    HRESULT hres = STATUS_UNSUCCESSFUL;
    PMNIDROPTARGET pmnidt;

    /*
     * Allocate the IDropTarget interface struct & additional data
     */
    pmnidt = (PMNIDROPTARGET)LocalAlloc(LPTR, sizeof(MNIDROPTARGET));
    if (pmnidt == NULL) {
        RIPMSG0(RIP_WARNING, "__ClientRegisterDragDrop allocation Failed");
        hres = STATUS_UNSUCCESSFUL;
        goto BackToKernel;
    }

    /*
     * Initialize it
     */
    pmnidt->idt.lpVtbl = &idtVtbl;

    /*
     * Call RegisterDragDrop
     */
    hres = (*(REGISTERDDPROC)gpfnOLERegisterDD)(*phwnd, (LPDROPTARGET)pmnidt);
    if (!SUCCEEDED(hres)) {
        RIPMSG1(RIP_WARNING, "__ClientRegisterDragDrop Failed:%#lx", hres);
    }

BackToKernel:
    return UserCallbackReturn(NULL, 0, hres);
}
/**************************************************************************\
* __ClientRevokeDragDrop
*
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
DWORD __ClientRevokeDragDrop(HWND * phwnd)
{
    HRESULT hres;

    /*
     * Call RevokeDragDrop
     */
    hres = (*(REVOKEDDPROC)gpfnOLERevokeDD)(*phwnd);
    if (!SUCCEEDED(hres)) {
        RIPMSG1(RIP_WARNING, "__ClientRevokeDragDrop Failed:%#lx", hres);
    }

    return UserCallbackReturn(NULL, 0, hres);
}
/**************************************************************************\
* LoadOLEOnce
*
*
* 10/31/96 GerardoB     Created
\**************************************************************************/
NTSTATUS LoadOLEOnce (void) {

    NTSTATUS Status = STATUS_SUCCESS;
    OLEINITIALIZEPROC pfnOLEOleInitialize;

    /*
     * These are the functions that we'll call.
     */
    GETPROCINFO gpi [] = {
        {&((FARPROC)pfnOLEOleInitialize), (LPCSTR)"OleInitialize"},
        {&gpfnOLEOleUninitialize, (LPCSTR)"OleUninitialize"},
        {&gpfnOLERegisterDD, (LPCSTR)"RegisterDragDrop"},
        {&gpfnOLERevokeDD, (LPCSTR)"RevokeDragDrop"},
        {NULL, NULL}
    };

    GETPROCINFO * pgpi = gpi;

    /*
     * We should come here only once
     */
    UserAssert(ghinstOLE == NULL);

    /*
     * Load it
     */
    ghinstOLE = LoadLibrary(L"OLE32.DLL");
    if (ghinstOLE == NULL) {
        RIPMSG1(RIP_WARNING, "LoadOLEOnce: Failed to load OLE32.DLL: %#lx", GetLastError());
        goto OLEWontLoad;
    }

    /*
     * Get the address of all procs
     */
    while (pgpi->ppfn != NULL) {
        *(pgpi->ppfn) = GetProcAddress(ghinstOLE, pgpi->lpsz);
        if (*(pgpi->ppfn) == NULL) {
            RIPMSG2(RIP_WARNING, "LoadOLEOnce: GetProcAddress failed: '%s': %#lx",
                    pgpi->lpsz, GetLastError());
            break;
        }
        pgpi++;
    }

    /*
     * If it got all procs, call OleInitialize
     */
    if (pgpi->ppfn == NULL) {
        Status = (*pfnOLEOleInitialize)(NULL);
        if (SUCCEEDED(Status)) {
            goto BackToKernel;
        } else {
            RIPMSG1(RIP_WARNING, "LoadOLEOnce: OleInitialize failed:%#lx", Status);
        }
    }

    /*
     * Something failed; NULL out all function pointers
     *   free the library and mark hinstOLE so we won't comeback here
     */
    pgpi = gpi;
    while (pgpi->ppfn != NULL) {
        *(pgpi->ppfn) = NULL;
        pgpi++;
    }
    FreeLibrary(ghinstOLE);

OLEWontLoad:
    ghinstOLE = OLEWONTLOAD;
    Status = STATUS_UNSUCCESSFUL;

BackToKernel:
    return Status;
}
/**************************************************************************\
* __ClientLoadOLE
*
*
* 10/31/96 GerardoB     Created
\**************************************************************************/
DWORD __ClientLoadOLE (PVOID p) {

    NTSTATUS Status;

    UNREFERENCED_PARAMETER(p);

    if (ghinstOLE == NULL) {
        Status = LoadOLEOnce();
    } else if (ghinstOLE == OLEWONTLOAD) {
        Status = STATUS_UNSUCCESSFUL;
    } else {
        UserAssert(gpfnOLERegisterDD != NULL);
        UserAssert(gpfnOLERevokeDD != NULL);
        Status = STATUS_SUCCESS;
    }

    return UserCallbackReturn(NULL, 0, Status);
}

