/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Retrieve / store information about a snapin
 * COPYRIGHT:   Copyright 2017-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once


// https://web.archive.org/web/20010614160959/msdn.microsoft.com/library/psdk/mmc/mmc12apx02_8jc5.htm
// https://msdn.microsoft.com/en-us/library/aa814846(v=vs.85).aspx


// https://msdn.microsoft.com/en-us/library/aa815510(v=vs.85).aspx
// MMC initializes a snap-in by calling the Initialize method of the IComponentData object.
// During initialization, IComponentData should query the console for its IConsoleNamespace and IConsole interfaces using
// the console's IUnknown interface pointer that is passed into the call to Initialize.
// The snap-in should then cache the returned pointers and use them for calling IConsoleNamespace and IConsole interface methods.


class CSnapin
{
private:
    GUID m_Guid;
    CAtlString m_Name;
    CAtlString m_Provider;
    int m_ImageIndex;
    //CAtlString m_About;


    CComPtr<ISnapInAbout> m_SnapinAbout;
    //CComPtr<CImageList> m_Imagelist;

    CSnapin(const GUID& guid, const CAtlString& name, const CAtlString& provider, const CAtlString& about)
        :m_Guid(guid), m_Name(name), m_Provider(provider), m_ImageIndex(-1)
    {
        GUID aboutGuid;
        if (!about.IsEmpty() &&
            !FAILED_UNEXPECTEDLY(::CLSIDFromString(about, &aboutGuid)))
        {
            HRESULT hr = CoCreateInstance(aboutGuid, NULL, CLSCTX_INPROC, IID_PPV_ARG(ISnapInAbout, &m_SnapinAbout));
            if (!FAILED_UNEXPECTEDLY(hr))
            {
                CComHeapPtr<WCHAR> ProviderName;
                if (!FAILED_UNEXPECTEDLY(m_SnapinAbout->GetProvider(&ProviderName)))
                {
                    m_Provider = ProviderName;
                }
            }
        }
    }

public:

    ~CSnapin()
    {
    }

    const CAtlString& Name() const { return m_Name; }
    const CAtlString& Provider() const { return m_Provider; }
    int ImageIndex() const { return m_ImageIndex; }

    void SetImageIndex(int index)
    {
        m_ImageIndex = index;
    }

    HRESULT GetIcon(HICON* hAppIcon)
    {
        if (!m_SnapinAbout)
            return E_NOINTERFACE;

        return m_SnapinAbout->GetSnapinImage(hAppIcon);
    }

    void OnAdd(IConsole* console)
    {
        CComPtr<IComponentData> spComponentData;
        HRESULT hr = CoCreateInstance(m_Guid, NULL, CLSCTX_INPROC, IID_PPV_ARG(IComponentData, &spComponentData));

        if (SUCCEEDED(hr))
        {
            //CComPtr<IExtendPropertySheet> spPropertySheet;
        }

    }


    void OnAccept(IConsole* console)
    {

        CComPtr<IComponentData> spComponentData;
        HRESULT hr = CoCreateInstance(m_Guid, NULL, CLSCTX_INPROC, IID_PPV_ARG(IComponentData, &spComponentData));

        if (SUCCEEDED(hr))
        {
            hr = spComponentData->Initialize(console);
            // CComponentData::CreatePropertyPages(
            if (SUCCEEDED(hr))
            {
                SCOPEDATAITEM item = { 0 };
                item.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;

                hr = spComponentData->GetDisplayInfo(&item);

                printf("%S\n", item.displayname);


                CComPtr<IDataObject> spDataObject;

                hr = spComponentData->QueryDataObject(NULL, CCT_SCOPE, &spDataObject);

                if (SUCCEEDED(hr))
                {
                    //spDataObject->
                }
            }
        }
    }


    static CSnapin* Create(CRegKey& key, PCWSTR guidString)
    {
        GUID guid;
        /* Not a guid, 'FX:' ?? */
        if (!SUCCEEDED(::CLSIDFromString(guidString, &guid)))
            return NULL;

        CRegKey snapin;
        if (ERROR_SUCCESS != snapin.Open(key, guidString, KEY_READ))
            return NULL;

        // Can we use this?
        CRegKey NodeTypes;
        if (ERROR_SUCCESS != NodeTypes.Open(snapin, L"StandAlone", KEY_READ))
            return NULL;

        CAtlString Name;
        QueryString(snapin, L"NameStringIndirect", Name);
        if (!Name.IsEmpty())
        {
            WCHAR IndirectBuffer[MAX_PATH];
            if (SUCCEEDED(SHLoadIndirectString(Name.GetString(), IndirectBuffer, _countof(IndirectBuffer), NULL)))
            {
                Name = IndirectBuffer;
            }
            else
            {
                Name.Empty();
            }
        }
        if (Name.IsEmpty())
            QueryString(snapin, L"NameString", Name);

        if (Name.IsEmpty())
            return NULL;

        CAtlString Provider;
        QueryString(snapin, L"Provider", Provider);

        CAtlString About;
        QueryString(snapin, L"About", About);

        return new CSnapin(guid, Name, Provider, About);
    }

    static void QueryString(CRegKey& key, PCWSTR Name, CAtlString& Value)
    {
        WCHAR Buffer[MAX_PATH];
        ULONG nChars = _countof(Buffer);

        Value.Empty();
        // FIXME: if too short, allocate
        if (ERROR_SUCCESS == key.QueryStringValue(Name, Buffer, &nChars))
        {
            Value = Buffer;
        }
    }
};

