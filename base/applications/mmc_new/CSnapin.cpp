/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Retrieve / store information about a snapin
 * COPYRIGHT:   Copyright 2017-2019 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

// https://web.archive.org/web/20010614160959/msdn.microsoft.com/library/psdk/mmc/mmc12apx02_8jc5.htm
// https://msdn.microsoft.com/en-us/library/aa814846(v=vs.85).aspx


// https://msdn.microsoft.com/en-us/library/aa815510(v=vs.85).aspx
// MMC initializes a snap-in by calling the Initialize method of the IComponentData object.
// During initialization, IComponentData should query the console for its IConsoleNamespace and IConsole interfaces using
// the console's IUnknown interface pointer that is passed into the call to Initialize.
// The snap-in should then cache the returned pointers and use them for calling IConsoleNamespace and IConsole interface methods.


CSnapin::CSnapin(CSnapinCacheEntry *CacheEntry, PWSTR displayName)
    :m_CacheEntry(CacheEntry)
{
    if (displayName)
        m_DisplayName = displayName;
    else
        m_DisplayName = m_CacheEntry->Name();
}

CSnapin::~CSnapin()
{
}

const CAtlString& CSnapin::Name() const { return m_CacheEntry->Name(); }
const CAtlString& CSnapin::Provider() const { return m_CacheEntry->Provider(); }
const CAtlString& CSnapin::Description() const { return m_CacheEntry->Description(); }
const CAtlString& CSnapin::Version() const { return m_CacheEntry->Version(); }
const CAtlString& CSnapin::DisplayName() const { return m_DisplayName; }

CSnapinCacheEntry *CSnapin::GetCacheEntry()
{
    return m_CacheEntry;
}

void CSnapin::OnAdd(IConsole* console)
{
#if 0
    CComPtr<IComponentData> spComponentData;
    HRESULT hr = CoCreateInstance(m_Guid, NULL, CLSCTX_INPROC, IID_PPV_ARG(IComponentData, &spComponentData));

    if (SUCCEEDED(hr))
    {
        //CComPtr<IExtendPropertySheet> spPropertySheet;
    }
#endif
}

void CSnapin::OnAccept(IConsole* console)
{
#if 0
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
#endif
}
