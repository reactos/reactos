/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Snapin cache entry class
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

#define IID_PPV_ARG(Itype, ppType) IID_##Itype, reinterpret_cast<void**>((static_cast<Itype**>(ppType)))

CSnapinCacheEntry::CSnapinCacheEntry(const CAtlString& guidString, const CAtlString& aboutGuidString, const CAtlString& name, HIMAGELIST hImageList)
    :m_GuidString(guidString), m_AboutGuidString(aboutGuidString), m_Name(name), m_Loaded(FALSE)
{
    ::CLSIDFromString(m_GuidString, &m_Guid);
    ::CLSIDFromString(m_AboutGuidString, &m_AboutGuid);

    CComPtr<ISnapinAbout> pSnapinAbout;
    HRESULT hr = CoCreateInstance(m_AboutGuid, NULL, CLSCTX_INPROC, IID_PPV_ARG(ISnapinAbout, &pSnapinAbout));
    DPRINT("CoCreateInstance %lx\n", hr);
    if (SUCCEEDED(hr))
    {
        LPOLESTR Str;

        hr = pSnapinAbout->GetProvider(&Str);
        DPRINT("GetProvider %lx\n", hr);
        if (SUCCEEDED(hr))
        {
            m_Provider = Str;
            CoTaskMemFree(Str);
        }

        hr = pSnapinAbout->GetSnapinDescription(&Str);
        DPRINT("GetDescription %lx\n", hr);
        if (SUCCEEDED(hr))
        {
            m_Description = Str;
            CoTaskMemFree(Str);
        }

        hr = pSnapinAbout->GetSnapinVersion(&Str);
        DPRINT("GetVersion %lx\n", hr);
        if (SUCCEEDED(hr))
        {
            m_Version = Str;
            CoTaskMemFree(Str);
        }

        HBITMAP hSmallImage, hSmallImageOpen, hLargeImage;
        COLORREF colorMask;

        hr = pSnapinAbout->GetStaticFolderImage(&hSmallImage,
                                                &hSmallImageOpen,
                                                &hLargeImage,
                                                &colorMask);
        if (SUCCEEDED(hr))
        {
            m_ImageNormal = ImageList_AddMasked(hImageList,
                                                hSmallImage,
                                                colorMask);

            m_ImageOpen = ImageList_AddMasked(hImageList,
                                              hSmallImageOpen,
                                              colorMask);

            DeleteObject(hSmallImage);
            DeleteObject(hSmallImageOpen);
            DeleteObject(hLargeImage);
        }
    }
}

CSnapinCacheEntry::~CSnapinCacheEntry()
{
}
