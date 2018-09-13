//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       msgreg.cxx
//
//  Contents:   Stuff to register pad as an Exchange custom form
//
//----------------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_MSG_HXX_
#define X_MSG_HXX_
#include "msg.hxx"
#endif

#ifndef X_PADRC2_H_
#define X_PADRC2_H_
#include "padrc2.h"
#endif

HRESULT
RegisterOneClass(HKEY hkeyCLSID, TCHAR *pchFile, REFCLSID clsid, TCHAR *pchProgID, TCHAR *pchFriendly);


//+------------------------------------------------------------------------
//
// Function:    WriteFileResource
//
// Synopsis:    Write out a resource as a file in temp directory
//
//-------------------------------------------------------------------------

static HRESULT
WriteFileResource(DWORD dwResId, TCHAR * pchFilePath)
{
    HRESULT hr = E_FAIL;
    HRSRC hrsrc = 0;
    HGLOBAL hglbl = 0;
    void* pvRes;
    DWORD cbRes;
    DWORD cbWritten;
    HANDLE hFile =  INVALID_HANDLE_VALUE;

    hrsrc = FindResource(g_hInstResource, MAKEINTRESOURCE(dwResId), MAKEINTRESOURCE(FILERESOURCE));
    if(!hrsrc)
        goto Cleanup;

    cbRes = SizeofResource(g_hInstResource, hrsrc);
    Assert(cbRes > 0);

    hglbl = LoadResource(g_hInstResource, hrsrc);
    if(!hglbl)
        goto Cleanup;

    pvRes = LockResource(hglbl);
    if(!pvRes)
        goto Cleanup;

    hFile = CreateFile(
                pchFilePath,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        goto Cleanup;

    if(!WriteFile(
                hFile,
                pvRes,
                cbRes,
                &cbWritten,
                NULL))
        goto Cleanup;

    Assert(cbWritten == cbRes);

    hr = S_OK;

Cleanup:
    CloseHandle(hFile);
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
// Function:    RegisterMsgServer
//
// Synopsis:    Register PadMessage for this server.
//
//-------------------------------------------------------------------------

static HRESULT
RegisterMsgServer()
{
    HRESULT hr;
    HKEY    hkeyCLSID = 0;
    TCHAR   achExe[MAX_PATH];

    GetModuleFileName(0, achExe, ARRAY_SIZE(achExe));

    hr = THR(RegDbOpenCLSIDKey(&hkeyCLSID));
    if (hr)
        RRETURN(hr);

    hr = THR(RegisterOneClass(
            hkeyCLSID,
            achExe,
            CLSID_CPadMessage,
            _T("IPM.Note.Trident"),
            _T("Trident Message")));
    if (hr)
        goto Cleanup;

Cleanup:
    RegCloseKey(hkeyCLSID);
    RegFlushKey(HKEY_CLASSES_ROOT);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
// Function:    RegisterMsg
//
// Synopsis:    Register PadMessage as an Exchange custom form
//
//-------------------------------------------------------------------------

HRESULT
RegisterMsg(BOOL fDialog)
{
    HRESULT hr;
    LPMAPIFORMCONTAINER pFormContainer;
    TCHAR achFilePath[MAX_PATH];
    TCHAR * pchFileName;
    DWORD cchTempPath;
    char achBuf[MAX_PATH];

    hr = THR(RegisterMsgServer());
    if (hr)
        goto Cleanup;

    cchTempPath = GetTempPath(MAX_PATH, achFilePath);
    if(!cchTempPath)
        goto Cleanup;

    pchFileName = achFilePath + cchTempPath;

    wcscpy(pchFileName, _T("padicon.ico"));

    hr = THR(WriteFileResource(IDF_LARGE_ICON, achFilePath));
    if (hr)
        goto Cleanup;

    wcscpy(pchFileName, _T("padsmall.ico"));

    hr = THR(WriteFileResource(IDF_SMALL_ICON, achFilePath));
    if (hr)
        goto Cleanup;

    wcscpy(pchFileName, _T("msg.cfg"));

    hr = THR(WriteFileResource(IDF_CFG_FILE, achFilePath));
    if (hr)
        goto Cleanup;

    hr = THR(MAPIInitialize(NULL));
    if (hr)
        goto Cleanup;

    hr = THR(MAPIOpenLocalFormContainer(&pFormContainer));
    if (hr)
        goto Cleanup;

    // Mapi needs ANSI
    WideCharToMultiByte(CP_ACP, 0, achFilePath, -1,
                achBuf, sizeof(achBuf), NULL, NULL);

    hr = THR(pFormContainer->InstallForm(
                NULL,
                MAPIFORM_INSTALL_OVERWRITEONCONFLICT,
                (LPCTSTR)achBuf));
    if (hr)
        goto Cleanup;


Cleanup:
    ReleaseInterface(pFormContainer);
    MAPIUninitialize();

    if (fDialog)
    {
        if (!hr)
        {
            MessageBox(NULL,
                       _T("Trident has been installed as an Exchange Form"),
                       _T("Trident Exchange Mail"),
                       MB_OK);
        }
        else
        {
            char ach[300];

            wsprintfA(ach,
                      "Unable to register Trident as an Exchange Form (error %08x).",
                      hr);

            MessageBoxA(NULL,
                        ach,
                        "Trident Exchange Mail",
                        MB_OK);
        }
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
// Function:    UnregisterMsg
//
// Synopsis:    Unregister PadMessage as an Exchange custom form
//
//-------------------------------------------------------------------------

HRESULT
UnregisterMsg()
{
    HRESULT hr;
    LPMAPIFORMCONTAINER pFormContainer;

    hr = THR(MAPIInitialize(NULL));
    if (hr)
        goto Cleanup;

    hr = THR(MAPIOpenLocalFormContainer(&pFormContainer));
    if (hr)
        goto Cleanup;

    hr = THR(pFormContainer->RemoveForm(g_achFormClassName));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pFormContainer);
    MAPIUninitialize();
    RRETURN(hr);
}

