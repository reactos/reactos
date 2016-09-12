/*
 * System File Checker (Windows File Protection)
 *
 * Copyright 2008 Pierre Schweitzer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "precomp.h"

HINSTANCE hLibModule;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hinstDLL);
            hLibModule = hinstDLL;
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            break;
        }
    }

    return TRUE;
}

DWORD WINAPI sfc_8()
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI sfc_9()
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOL WINAPI SRSetRestorePointA(PRESTOREPOINTINFOA pRestorePtSpec, PSTATEMGRSTATUS pSMgrStatus)
{
    HMODULE hModule;
    PSRSRPA pSRSRPA;
    BOOL bStatus = FALSE;
    LPCWSTR lpLibFileName = L"srclient.dll";
    LPCSTR lpProcName = "SRSetRestorePointA";

    hModule = LoadLibraryW(lpLibFileName);
    if (hModule)
    {
        pSRSRPA = (PSRSRPA)GetProcAddress(hModule, lpProcName);
        if (pSRSRPA)
        {
            bStatus = pSRSRPA(pRestorePtSpec, pSMgrStatus);
        }
        else
        {
            if (pSMgrStatus)
            {
                pSMgrStatus->nStatus = ERROR_CALL_NOT_IMPLEMENTED;
            }
        }
        FreeLibrary(hModule);
    }
    else
    {
        if (pSMgrStatus)
        {
            pSMgrStatus->nStatus = ERROR_CALL_NOT_IMPLEMENTED;
        }
    }

    return bStatus;
}

BOOL WINAPI SRSetRestorePointW(PRESTOREPOINTINFOW pRestorePtSpec, PSTATEMGRSTATUS pSMgrStatus)
{
    HMODULE hModule;
    PSRSRPW pSRSRPW;
    BOOL bStatus = FALSE;
    LPCWSTR lpLibFileName = L"srclient.dll";
    LPCSTR lpProcName = "SRSetRestorePointW";

    hModule = LoadLibraryW(lpLibFileName);
    if (hModule)
    {
        pSRSRPW = (PSRSRPW)GetProcAddress(hModule, lpProcName);
        if (pSRSRPW)
        {
            bStatus = pSRSRPW(pRestorePtSpec, pSMgrStatus);
        }
        else
        {
            if (pSMgrStatus)
            {
                pSMgrStatus->nStatus = ERROR_CALL_NOT_IMPLEMENTED;
            }
        }
        FreeLibrary(hModule);
    }
    else
    {
        if (pSMgrStatus)
        {
            pSMgrStatus->nStatus = ERROR_CALL_NOT_IMPLEMENTED;
        }
    }

    return bStatus;
}

BOOL WINAPI SfpVerifyFile(LPCSTR pszFileName, LPSTR pszError, DWORD dwErrSize)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
