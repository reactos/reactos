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


CSnapin::CSnapin(CSnapinCacheEntry *CacheEntry, UINT NodeId, PWSTR displayName)
    :m_CacheEntry(CacheEntry)
{
    m_NodeId = NodeId;
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

void CSnapin::SetDisplayName(LPWSTR pszName)
{
    m_DisplayName = pszName;
}

UINT
CSnapin::GetNodeId()
{
    return m_NodeId;
}

CSnapinCacheEntry *CSnapin::CacheEntry()
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
    WCHAR szBuffer[32];
    HRESULT hr = S_OK;

    /* <Node ID="3" ImageIdx="0" CLSID="{C96401CC-0E17-11D3-885B-00C04F72C717}" Preload="true"> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"Node", pParentNode, &pNodeElement));
    _swprintf(szBuffer, L"%u", m_NodeId);
    CHK_HR(mscFile->CreateAndAddAttributeNode(L"ID", szBuffer, pNodeElement));
    CHK_HR(mscFile->CreateAndAddAttributeNode(L"ImageIdx", L"0", pNodeElement)); /* FIXME */
    CHK_HR(mscFile->CreateAndAddAttributeNode(L"CLSID", CacheEntry()->GuidString().GetString(), pNodeElement));
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

HRESULT
CSnapin::ParseSnapin(CMainWnd *pMainWnd, MscFile *mscFile, IXMLDOMElement *pSnapinNode, CSnapin **ppSnapin)
{
    VARIANT SnapinID, SnapinCLSID, SnapinDisplayName;
    IXMLDOMElement *pStringElement = NULL;
    IXMLDOMElement *pNodesElement = NULL;
    IXMLDOMNodeList *pChildList = NULL;
    PWSTR pwstr;
    CSnapinCacheEntry *pCacheEntry = NULL;
    CSnapin *pSnapin = NULL;
    UINT uSnapinId;
    HRESULT hr = S_OK;

    VariantInit(&SnapinID);
    VariantInit(&SnapinCLSID);
    VariantInit(&SnapinDisplayName);

    /* Read the ID attribute */
    hr = mscFile->GetAttribute(pSnapinNode, (LPWSTR)L"ID", &SnapinID);
    if (hr != S_OK)
        goto done;

    /* Read the CLSID attribute */
    hr = mscFile->GetAttribute(pSnapinNode, (LPWSTR)L"CLSID", &SnapinCLSID);
    if (hr != S_OK)
        goto done;

    /* FIXME: Read the ImageIdx attribute */
    /* FIXME: Read the Preload attribute */

    /* Open the String Element */
    hr = mscFile->GetElement(pSnapinNode, (LPWSTR)L"String", &pStringElement);
    if (hr != S_OK)
        goto done;

    /* Read the Name attribute */
    hr = mscFile->GetAttribute(pStringElement, (LPWSTR)L"Name", &SnapinDisplayName);
    if (hr != S_OK)
        goto done;

    /* FIXME: Use a string cache and the ID attribute instead of Name later */

    /* Create the Snap-in */
    pCacheEntry = pMainWnd->GetSnapinCacheEntryByGuid(V_BSTR(&SnapinCLSID));
    if (pCacheEntry == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    uSnapinId = (UINT)wcstoul(V_BSTR(&SnapinID), &pwstr, 10);
    pMainWnd->UpdateNextNodeId(uSnapinId);

    pSnapin = new CSnapin(pCacheEntry,
                          uSnapinId,
                          V_BSTR(&SnapinDisplayName));
    if (pSnapin == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    /* FIXME: Read the Bitmaps element */
    /* FIXME: Read the ComponentDatas element */
    /* FIXME: Read the Component element */

    /* Get the Nodes element */
    hr = mscFile->GetElement(pSnapinNode, (LPWSTR)L"Nodes", &pNodesElement);
    if (hr != S_OK)
        goto done;

    hr = pNodesElement->get_childNodes(&pChildList);
    if (FAILED(hr))
        goto done;

done:
    if (pStringElement)
        pStringElement->Release();

    VariantInit(&SnapinDisplayName);
    VariantInit(&SnapinCLSID);
    VariantInit(&SnapinID);

    /* Parse Nodes Element and create Sub-Snapins */
    if (pChildList)
    {
        IXMLDOMNode *nextItem;
        DOMNodeType NodeType;
        BSTR NodeName = NULL;

        for (;;)
        {
            hr = pChildList->nextNode(&nextItem);
            if (hr == S_FALSE)
            {
                hr = S_OK;
                break;
            }

            hr = nextItem->get_nodeType(&NodeType);
            if (hr == S_OK)
            {
                if (NodeType == NODE_ELEMENT)
                {
                    NodeName = NULL;
                    hr = nextItem->get_baseName(&NodeName);
                    if (hr == S_OK)
                    {
                        if (_wcsicmp(NodeName, L"Node") == 0)
                        {
                            CSnapin *pSubSnapin;
                            hr = ParseSnapin(pMainWnd, mscFile, (IXMLDOMElement *)nextItem, &pSubSnapin);
                            if (hr == S_OK)
                            {
                                pSnapin->m_SubNodes.AddTail(pSubSnapin);
                            }
                        }
                        SysFreeString(NodeName);
                        NodeName = NULL;
                    }
                }
            }

            nextItem->Release();
        }

        pChildList->Release();

        if (NodeName)
            SysFreeString(NodeName);
    }

    if (pNodesElement)
        pNodesElement->Release();

    *ppSnapin = pSnapin;

    return hr;
}
