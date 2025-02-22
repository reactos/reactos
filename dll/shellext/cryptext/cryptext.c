/*
 * PROJECT:     ReactOS CryptExt Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     cryptext implementation
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

HINSTANCE g_hInstance;

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
        break;
    }

    return TRUE;
}

EXTERN_C
VOID WINAPI CryptExtOpenCERW(HWND hWnd, HINSTANCE hInst, LPCWSTR file, DWORD nCmdShow)
{
    PCCERT_CONTEXT pvContext;
    if (file)
    {
        if (CryptQueryObject(CERT_QUERY_OBJECT_FILE, file, CERT_QUERY_CONTENT_FLAG_CERT, CERT_QUERY_FORMAT_FLAG_ALL,
                             0, NULL, NULL, NULL, NULL, NULL, (CONST VOID**)&pvContext))
        {
            CRYPTUI_VIEWCERTIFICATE_STRUCTW CertViewInfo = {0};
            CertViewInfo.dwSize = sizeof(CertViewInfo);
            CertViewInfo.pCertContext = pvContext;
            CryptUIDlgViewCertificateW(&CertViewInfo, NULL);
            CertFreeCertificateContext(pvContext);
        }
        else
        {
            WCHAR Message[MAX_PATH];

            if (LoadStringW(g_hInstance, IDS_INVALIDFILE, Message, MAX_PATH))
            {
                MessageBoxW(NULL, Message, NULL, MB_OK);
            }
            else
            {
                MessageBoxW(NULL, L"This is not a valid certificate file.", NULL, MB_OK);
            }
        }
    }
}

EXTERN_C
VOID WINAPI CryptExtOpenCER(HWND hWnd, HINSTANCE hInst, LPCSTR file, DWORD nCmdShow)
{
    LPWSTR fileW;
    int len;

    if (file)
    {

        len = MultiByteToWideChar(CP_ACP, 0, file, -1, NULL, 0);
        fileW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (fileW)
        {
            MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, len);
            CryptExtOpenCERW(hWnd, hInst, fileW, nCmdShow);
            HeapFree(GetProcessHeap(), 0, fileW);
        }
    }
}
