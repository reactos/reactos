/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/enum.c
 * PURPOSE:         Handles DSound device enumeration
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

VOID
LoadResourceString(
    UINT ResourceId,
    LPVOID Buffer,
    UINT ccount,
    LPVOID DefaultString,
    BOOL bUnicode)
{
    if (bUnicode)
    {
        /* load localized string */
        if (!LoadStringW(dsound_hInstance, ResourceId, (LPWSTR)Buffer, ccount))
        {
            /* default device name */
            wcscpy((LPWSTR)Buffer, (LPWSTR)DefaultString);
        }
    }
    else
    {
        /* load localized string */
        if (!LoadStringA(dsound_hInstance, ResourceId, (LPSTR)Buffer, ccount))
        {
            /* default device name */
            strcpy((LPSTR)Buffer, (LPSTR)DefaultString);
        }
    }
}


BOOL
DoDSoundCallback(
    LPDSENUMCALLBACKA lpDSEnumCallbackA,
    LPDSENUMCALLBACKW lpDSEnumCallbackW,
    LPGUID DeviceGuid,
    UINT ResourceId,
    LPWSTR ProductName,
    LPWSTR DriverName,
    LPVOID lpContext)
{
    WCHAR Buffer[200] = {0};
    char DriverNameA[200];

    static LPWSTR SoundDriverW = L"Primary Sound Driver";
    static LPWSTR SoundDriverA = L"Primary Sound Driver";

    if (lpDSEnumCallbackW)
    {
        if (ResourceId)
        {
            /* load resource string */
            Buffer[0] = 0;
            LoadResourceString(ResourceId, (LPVOID)Buffer, sizeof(Buffer)/sizeof(WCHAR), (LPVOID)SoundDriverW, TRUE);
            Buffer[(sizeof(Buffer)/sizeof(WCHAR))-1] = '\0';
        }
        else
        {
            /* use passed string */
            ASSERT(ProductName);
            wcscpy(Buffer, ProductName);
        }

        /* perform callback */
        return lpDSEnumCallbackW(DeviceGuid, Buffer, DriverName, lpContext);
    }
    else
    {
        if (ResourceId)
        {
            /* load resource string */
            Buffer[0] = 0;
            LoadResourceString(ResourceId, (LPVOID)Buffer, sizeof(Buffer)/sizeof(char), (LPVOID)SoundDriverA, FALSE);
            Buffer[(sizeof(Buffer)/sizeof(WCHAR))-1] = 0;
        }
        else
        {
            /* use passed string */
            Buffer[0] = 0;
            WideCharToMultiByte(CP_ACP, 0, ProductName, -1, (LPSTR)Buffer, sizeof(Buffer) / sizeof(char), NULL, NULL);
            Buffer[(sizeof(Buffer)/sizeof(WCHAR))-1] = 0;
        }

        DriverNameA[0] = 0;
        if (ProductName)
        {
            WideCharToMultiByte(CP_ACP, 0, ProductName, -1, DriverNameA, sizeof(DriverNameA) / sizeof(char), NULL, NULL);
            DriverNameA[(sizeof(DriverNameA) / sizeof(char))-1] = 0;
        }

        return lpDSEnumCallbackA(DeviceGuid, (LPSTR)Buffer, DriverNameA, lpContext);
    }
}


HRESULT
DSoundEnumerate(
    LPDSENUMCALLBACKA lpDSEnumCallbackA,
    LPDSENUMCALLBACKW lpDSEnumCallbackW,
    LPVOID lpContext,
    BOOL bPlayback)
{
    ULONG ResourceId;
    BOOL bResult;
    LPFILTERINFO CurInfo;
    WAVEOUTCAPSW WaveOutCaps;
    WAVEINCAPSW  WaveInCaps;

    if (!RootInfo)
    {
        EnumAudioDeviceInterfaces(&RootInfo);
    }

    if (lpDSEnumCallbackA == NULL && lpDSEnumCallbackW == NULL)
    {
        DPRINT("No callback\n");
        return DSERR_INVALIDPARAM;
    }

    if (bPlayback)
    {
        /* use resource id of playback string */
        ResourceId = IDS_PRIMARY_PLAYBACK_DEVICE;
    }
    else
    {
        /* use resource id of playback string */
        ResourceId = IDS_PRIMARY_RECORD_DEVICE;
    }

    if (RootInfo)
    {
        /* perform first callback */
        bResult = DoDSoundCallback(lpDSEnumCallbackA, lpDSEnumCallbackW, NULL, ResourceId, NULL, L"", lpContext);
        if (!bResult)
        {
            /* callback asked as to stop */
            return DS_OK;
        }

        /* now iterate through all devices */
        CurInfo = RootInfo;

        do
        {
            if (bPlayback && !IsEqualGUID(&CurInfo->DeviceGuid[1], &GUID_NULL))
            {
                RtlZeroMemory(&WaveOutCaps, sizeof(WAVEOUTCAPSW));

                /* sanity check */
                ASSERT(CurInfo->MappedId[1] != ULONG_MAX);

                /* get wave out caps */
                waveOutGetDevCapsW((UINT_PTR)CurInfo->MappedId[1], &WaveOutCaps, sizeof(WAVEOUTCAPSW));
                WaveOutCaps.szPname[MAXPNAMELEN-1] = L'\0';

                bResult = DoDSoundCallback(lpDSEnumCallbackA, lpDSEnumCallbackW, &CurInfo->DeviceGuid[1], 0, WaveOutCaps.szPname, L"" /* FIXME */, lpContext);
                if (!bResult)
                {
                    /* callback asked as to stop */
                    return DS_OK;
                }
            }
            else if (!bPlayback && !IsEqualGUID(&CurInfo->DeviceGuid[0], &GUID_NULL))
            {
                RtlZeroMemory(&WaveInCaps, sizeof(WAVEINCAPSW));

                /* sanity check */
                ASSERT(CurInfo->MappedId[1] != ULONG_MAX);

                /* get wave in caps */
                waveInGetDevCapsW((UINT_PTR)CurInfo->MappedId[0], &WaveInCaps, sizeof(WAVEINCAPSW));
                WaveInCaps.szPname[MAXPNAMELEN-1] = L'\0';

                bResult = DoDSoundCallback(lpDSEnumCallbackA, lpDSEnumCallbackW, &CurInfo->DeviceGuid[0], 0, WaveInCaps.szPname, L"" /* FIXME */, lpContext);
                if (!bResult)
                {
                    /* callback asked as to stop */
                    return DS_OK;
                }
            }

            /* move to next entry */
            CurInfo = CurInfo->lpNext;
        }while(CurInfo);
    }
    return DS_OK;
}

HRESULT
WINAPI
DirectSoundEnumerateA(
    LPDSENUMCALLBACKA lpDSEnumCallback,
    LPVOID lpContext)
{
    return DSoundEnumerate(lpDSEnumCallback, NULL, lpContext, TRUE);
}

HRESULT
WINAPI
DirectSoundEnumerateW(
    LPDSENUMCALLBACKW lpDSEnumCallback,
    LPVOID lpContext )
{
    return DSoundEnumerate(NULL, lpDSEnumCallback, lpContext, TRUE);
}

HRESULT
WINAPI
DirectSoundCaptureEnumerateA(
    LPDSENUMCALLBACKA lpDSEnumCallback,
    LPVOID lpContext)
{
    return DSoundEnumerate(lpDSEnumCallback, NULL, lpContext, FALSE);
}

HRESULT
WINAPI
DirectSoundCaptureEnumerateW(
    LPDSENUMCALLBACKW lpDSEnumCallback,
    LPVOID lpContext)
{
    return DSoundEnumerate(NULL, lpDSEnumCallback, lpContext, FALSE);
}
