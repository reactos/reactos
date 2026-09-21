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
    RemoveAllSubnodes();
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

void
CSnapin::RemoveAllSubnodes()
{
    CSnapin *SubNode;
    POSITION pos = m_SubNodes.GetHeadPosition();
    while (pos != NULL)
    {
        SubNode = (CSnapin*)m_SubNodes.GetNext(pos);
        if (SubNode)
            delete SubNode;
    }
    m_SubNodes.RemoveAll();
}

void
CSnapin::OnAdd(IConsole* console)
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

void
CSnapin::OnAccept(IConsole* console)
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

VOID
CSnapin::SaveNode(MscFile *mscFile, IXMLDOMElement *pParentNode)
{
    IXMLDOMElement *pNodeElement = NULL;
    IXMLDOMElement *pNodesElement = NULL;
    IXMLDOMElement *pStringElement = NULL;
    IXMLDOMElement *pBitmapsElement = NULL;
    IXMLDOMElement *pComponentDatasElement = NULL;
    IXMLDOMElement *pComponentDataElement = NULL;
    IXMLDOMElement *pComponentsElement = NULL;
    POSITION pos;
    CSnapin *SubNode;
    HRESULT hr = S_OK;

    /* <Node ID="3" ImageIdx="0" CLSID="{C96401CC-0E17-11D3-885B-00C04F72C717}" Preload="true"> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"Node", pParentNode, &pNodeElement));
    CHK_HR(mscFile->CreateAndAddAttributeNode(L"ID", L"0", pNodeElement)); /* FIXME */
    CHK_HR(mscFile->CreateAndAddAttributeNode(L"ImageIdx", L"0", pNodeElement)); /* FIXME */
    CHK_HR(mscFile->CreateAndAddAttributeNode(L"CLSID", GetCacheEntry()->GuidString().GetString(), pNodeElement));
    CHK_HR(mscFile->CreateAndAddAttributeNode(L"Preload", L"true", pNodeElement)); /* FIXME */

    /* <Nodes> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"Nodes", pNodeElement, &pNodesElement));
    pos = m_SubNodes.GetHeadPosition();
    while (pos != NULL)
    {
        SubNode = (CSnapin*)m_SubNodes.GetNext(pos);
        if (SubNode)
            SubNode->SaveNode(mscFile, pNodesElement);
    }
    SAFE_RELEASE(pNodesElement); /* </Nodes> */

    /* <String Name="Name" ID="2"/> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"String", pNodeElement, &pStringElement));
    CHK_HR(mscFile->CreateAndAddAttributeNode(L"Name", m_DisplayName.GetString(), pStringElement)); /* FIXME */
    SAFE_RELEASE(pStringElement); /* </String> */

    /* <Bitmaps> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"Bitmaps", pNodeElement, &pBitmapsElement));
/*
        <BinaryData Name="Small" BinaryRefIndex="3"/>
        <BinaryData Name="Large" BinaryRefIndex="4"/>
*/
    SAFE_RELEASE(pBitmapsElement); /* </Bitmaps> */

    /* <ComponentDatas> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"ComponentDatas", pNodeElement, &pComponentDatasElement));
    /* <ComponentData> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"ComponentData", pComponentDatasElement, &pComponentDataElement));
/*
        <GUID Name="Snapin">{C96401CC-0E17-11D3-885B-00C04F72C717}</GUID>
        <Stream BinaryRefIndex="5"/>
*/
    SAFE_RELEASE(pComponentDataElement); /* </ComponentData> */
    SAFE_RELEASE(pComponentDatasElement); /* </ComponentDatas> */

    /* <Components /> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"Components", pNodeElement, &pComponentsElement));
    SAFE_RELEASE(pComponentsElement); /* </Components> */

CleanUp:
    SAFE_RELEASE(pNodeElement); /* </Node> */
}
