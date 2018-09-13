//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       stuffodg.cxx
//
//  Contents:   Tool to munge forms3.tlb to get methodinfos of event
//              interfaces.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#define NUM_COLDFILES   10

//+------------------------------------------------------------------------
//
//  Function:   ReleaseInterface
//
//  Synopsis:   Releases an interface pointer if it is non-NULL
//
//  Arguments:  [pUnk]
//
//-------------------------------------------------------------------------

void
ReleaseInterface(IUnknown * pUnk)
{
    if (pUnk)
        pUnk->Release();
}



//+------------------------------------------------------------------------
//
//  Function:   GetLastWin32Error
//
//  Synopsis:   Returns the last Win32 error, converted to an HRESULT.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
GetLastWin32Error( )
{
    // Win 95 can return 0, even when there's an error.
    DWORD dw = GetLastError();
    return dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
}


//+---------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   Print help.
//
//----------------------------------------------------------------------------

void
Usage()
{
    printf("Usage: stuffodg <docfile> <stream copy file> <odg files>\n");
}



//+---------------------------------------------------------------------------
//
//  Function:   main
//
//----------------------------------------------------------------------------

int __cdecl
main(int argc, char ** argv)
{
    HRESULT         hr = S_OK;
    int             ret;
    WCHAR           awch[MAX_PATH];
    LPWSTR          pstr;
    LPWSTR          pstrExt;
    int             i, j;
    IStorage *      pStgRoot = NULL;
    IStorage *      pStgSrc;
    IStorage *      pStgDst;
    WCHAR           achExt[4] = L"--";
    IStorage *      pStgStream;
    IStream *       pStmSrc;
    IStream *       pStmDst;
    WCHAR           achStream[8] = L"stream";
    ULARGE_INTEGER  uliMax;

    uliMax.QuadPart = _UI64_MAX;

    if (argc < 4)
    {
        Usage();
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(OleInitialize(NULL));
    if (hr)
        goto Cleanup;

    //
    // Create docfile to stuff odg's into.
    //

    ret = TW32(0, MultiByteToWideChar(
            CP_ACP, 0, argv[1], -1, awch, MAX_PATH));
    if (!ret)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

    hr = THR(StgCreateDocfile(
            awch,
            STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_CREATE,
            0,
            &pStgRoot));
    if (hr)
        goto Cleanup;

    for (i = 0; i < NUM_COLDFILES; i++)
    {
        pStgStream = NULL;
        pStmSrc = NULL;
        pStmDst = NULL;

        // Copy each odg file into the docfile.
        for (j = 3; j < argc; j++)
        {
            pStgSrc = NULL;
            pStgDst = NULL;
            ret = TW32(0, MultiByteToWideChar(
                    CP_ACP, 0, argv[j], -1, awch, MAX_PATH));
            if (!ret)
            {
                hr = GetLastWin32Error();
                goto Loop2Cleanup;
            }

            hr = THR(StgOpenStorage(
                    awch,
                    NULL,
                    STGM_READ | STGM_SHARE_DENY_WRITE | STGM_DIRECT,
                    NULL,
                    NULL,
                    &pStgSrc));
            if (hr)
                goto Cleanup;

            //
            // Create name of storage. It is the basename of the file, plus
            // for files in addition to the first, a suffix of "--x", where
            // x is the iteration.
            //

            pstr = wcsrchr(awch, L'\\');
            pstr = pstr ? pstr + 1 : awch;
            pstrExt = wcsrchr(pstr, L'.');
            if (pstrExt)
            {
                *pstrExt = 0;
            }

            if (i > 0)
            {
                achExt[2] = L'0' + i;
                wcscat(pstr, achExt);
            }

            hr = THR(pStgRoot->CreateStorage(
                    pstr,
                    STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_CREATE,
                    0,
                    0,
                    &pStgDst));
            if (hr)
                goto Loop2Cleanup;

            hr = THR(pStgSrc->CopyTo(
                    0,
                    NULL,
                    NULL,
                    pStgDst));
            if (hr)
                goto Loop2Cleanup;

Loop2Cleanup:
            ReleaseInterface(pStgSrc);
            ReleaseInterface(pStgDst);
            if (hr)
                goto LoopCleanup;
        }

        //
        // Copy designated "f" stream.
        //

        ret = TW32(0, MultiByteToWideChar(
                CP_ACP, 0, argv[2], -1, awch, MAX_PATH));
        if (!ret)
        {
            hr = GetLastWin32Error();
            goto LoopCleanup;
        }

        hr = THR(StgOpenStorage(
                awch,
                NULL,
                STGM_READ | STGM_SHARE_DENY_WRITE | STGM_DIRECT,
                NULL,
                NULL,
                &pStgStream));
        if (hr)
            goto LoopCleanup;

        hr = THR(pStgStream->OpenStream(
                L"f",
                NULL,
                STGM_READ | STGM_SHARE_EXCLUSIVE,
                0,
                &pStmSrc));
        if (hr)
            goto LoopCleanup;

        achStream[6] = L'0' + i;
        hr = THR(pStgRoot->CreateStream(
                achStream,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                0,
                0,
                &pStmDst));
        if (hr)
            goto LoopCleanup;

        hr = THR(pStmSrc->CopyTo(
                pStmDst,
                uliMax,
                NULL,
                NULL));
        if (hr)
            goto LoopCleanup;

LoopCleanup:
        ReleaseInterface(pStmDst);
        ReleaseInterface(pStmSrc);
        ReleaseInterface(pStgStream);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pStgRoot);
    OleUninitialize();

    if (FAILED(hr))
    {
        printf("Stuffodg exiting with error %08x\n.", hr);
    }

    RRETURN1(hr, S_FALSE);
}

