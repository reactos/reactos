/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    events.cpp

Abstract:

    implementation of CComponent functions

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"



HRESULT CComponent::OnShow(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    // Note - arg is TRUE when it is time to enumerate
    CFolder* pFolder = FindFolder(cookie);
    if (arg)
    {
        m_pCurFolder = pFolder;
    }
    if (pFolder)
        return pFolder->OnShow((BOOL)arg);
    else
        return S_OK;
}

HRESULT CComponent::OnMinimize(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return S_OK;
}

HRESULT CComponent::OnViewChange(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    return S_OK;
}

HRESULT CComponent::OnProperties(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    return S_OK;
}

HRESULT CComponent::OnResultItemClick(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    return S_OK;
}

HRESULT CComponent::OnResultItemDblClick(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    return S_FALSE;
}

HRESULT CComponent::OnActivate(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    return S_OK;
}

HRESULT CComponent::OnSelect(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    CFolder* pFolder;
    pFolder  = FindFolder(cookie);
    if (pFolder && LOWORD(arg))
    {
        // LOWORD(arg) being set indicated this is for the scope pane item.
        // Save the bSelect value for use by the MenuCommand.
        pFolder->m_bSelect = (BOOL) HIWORD(arg);
    }
    if (!pFolder || S_FALSE == pFolder->OnSelect())
    {
        //
        // either we can not find the responsible folder
        // or the responsible folder asks us to do it,
        // set the console verb to its defaults
        //
        m_pConsoleVerb->SetVerbState(MMC_VERB_OPEN, HIDDEN, TRUE);
        m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, TRUE);
        m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);
        m_pConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, TRUE);
        m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, TRUE);
        m_pConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, TRUE);
        m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
    }
    return S_OK;
}

HRESULT CComponent::OnOcxNotify(
    MMC_NOTIFY_TYPE event,
    LPARAM arg,
    LPARAM param
    )
{
    //TRACE1(TEXT("Componet:OnOcxNotify, event = %lx\n"), event);

    if (m_pCurFolder)
        return m_pCurFolder->OnOcxNotify(event, arg, param);

    return S_OK;
}

HRESULT CComponent::OnBtnClick(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    return S_OK;
}

HRESULT CComponent::OnAddImages(
    MMC_COOKIE  cookie,
    IImageList* pIImageList,
    HSCOPEITEM hScopeItem
    )
{
    if (cookie)
    {
        CFolder* pFolder;
        pFolder = FindFolder(cookie);
        if (pFolder)
            return pFolder->OnAddImages(pIImageList, hScopeItem);
        else
            return S_OK;
    }
    return LoadScopeIconsForResultPane(pIImageList);
}

HRESULT CComponent::OnRestoreView(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    if (!param)
    {
        return E_INVALIDARG;
    }
    CFolder* pFolder;
    pFolder = FindFolder(cookie);
    if (pFolder)
        return pFolder->OnRestoreView((BOOL*)param);
    else
    {
        *((BOOL *)param) = FALSE;
        return S_OK;
    }
}

HRESULT CComponent::OnContextHelp(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    String strHelpOverview;
    String strHelpTopic;

    // Load help file and overview topic strings.
    strHelpOverview.LoadString(g_hInstance, IDS_HTMLHELP_NAME);
    strHelpTopic.LoadString(g_hInstance, IDS_HTMLHELP_OVERVIEW_TOPIC);

    strHelpOverview += TEXT("::");
    strHelpOverview += strHelpTopic;
    return m_pDisplayHelp->ShowTopic(const_cast<BSTR>((LPCTSTR)strHelpOverview));
}

HRESULT CComponent::LoadScopeIconsForResultPane(
    IImageList* pIImageList
    )
{
    if (pIImageList)
    {
        HICON hIcon;
        HRESULT hr = S_OK;
        hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_DEVMGR));
        if (hIcon)
        {
            hr = pIImageList->ImageListSetIcon((PLONG_PTR)hIcon, IMAGE_INDEX_DEVMGR);
            DeleteObject(hIcon);
        }
        return hr;
    }
    else
    {
        return E_INVALIDARG;
    }
}

#if DBG
TCHAR *tvNotifyStr[] = {
    TEXT("CLICK"),
    TEXT("DBLCLK"),
    TEXT("RCLICK"),
    TEXT("RDBLCLK"),
    TEXT("KEYDOWN"),
    TEXT("CONTEXTMENU"),
    TEXT("EXPANDING"),
    TEXT("EXPANDED"),
    TEXT("SELCHANGING"),
    TEXT("SELCHANGED"),
    TEXT("GETDISPINFO"),
    TEXT("FOCUSCHANGED"),
    TEXT("UNKNOWN")
    };
#endif

HRESULT CComponent::tvNotify(
    HWND hwndTV,
    MMC_COOKIE cookie,
    TV_NOTIFY_CODE Code,
    LPARAM arg,
    LPARAM param
    )
{

#if DBG
    int i = Code;
    if (Code > TV_NOTIFY_CODE_UNKNOWN)
    {
        i = TV_NOTIFY_CODE_UNKNOWN;
    }
    //TRACE3(TEXT("Componet:tvNotify, Code = %lx %s cookie = %lx\n"), Code, tvNotifyStr[i], cookie);
#endif

    CFolder* pFolder;
    pFolder = FindFolder(cookie);
    if (pFolder)
        return pFolder->tvNotify(hwndTV, GetActiveCookie(cookie), Code, arg, param);
    return S_FALSE;
}

////////////////////////////////////////////////////////////////////////////
//// IComponentData events handlers
////


HRESULT
CComponentData::OnProperties(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    return S_OK;
}

HRESULT
CComponentData::OnBtnClick(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    return S_OK;
}
HRESULT
CComponentData::OnDelete(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    return S_OK;
}
HRESULT
CComponentData::OnRename(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    return S_OK;
}

//
// This function handles the MMCN_EXPAND notification code
// Input: lpDataObject -- point to the target IDataObject
//        arg          -- TRUE if expanding, FALSE if collapsing.
//        param        -- not used.
//
// Output: HRESULT
HRESULT
CComponentData::OnExpand(
    LPDATAOBJECT lpDataObject,
    LPARAM arg,
    LPARAM param
    )
{
    INTERNAL_DATA tID;
    HRESULT hr;

    // if we are not expanding, do nothing
    if (!arg)
        return S_OK;

    hr = ExtractData(lpDataObject, CDataObject::m_cfSnapinInternal,
                         (PBYTE)&tID, sizeof(tID));
    if (SUCCEEDED(hr))
    {
        hr = CreateScopeItems();
        if (SUCCEEDED(hr) && !m_pMachine)
        {
            if (!g_MachineList.CreateMachine(m_hwndMain, m_strMachineName, &m_pMachine))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }

        if (SUCCEEDED(hr))
        {
            CCookie* pCookie = GetActiveCookie(tID.cookie);
            ASSERT(pCookie);
            HSCOPEITEM hScopeParent = (HSCOPEITEM)param;
            CScopeItem* pScopeItem = pCookie->GetScopeItem();
            ASSERT(pScopeItem);
            pScopeItem->SetHandle(hScopeParent);

            //
            // If we have children and this is the first time we
            // are expanding, insert all the children to scope pane.
            //
            if (pCookie->GetChild() && !pScopeItem->IsEnumerated())
            {
                SCOPEDATAITEM ScopeDataItem;

                CCookie* pTheCookie = pCookie->GetChild();
                do
                {
                    CScopeItem* pTheItem = pTheCookie->GetScopeItem();
                    ASSERT(pTheItem);
                    ScopeDataItem.relativeID = hScopeParent;
                    ScopeDataItem.nState = 0;
                    ScopeDataItem.displayname = (LPOLESTR)(-1);
                    ScopeDataItem.mask = SDI_IMAGE | SDI_OPENIMAGE | SDI_CHILDREN |
                                         SDI_STR | SDI_PARAM | SDI_STATE;
                    ScopeDataItem.nImage = pTheItem->GetImageIndex();
                    ScopeDataItem.nOpenImage = pTheItem->GetOpenImageIndex();
                    ScopeDataItem.cChildren = pTheItem->GetChildCount();
                    ScopeDataItem.lParam = reinterpret_cast<LPARAM>(pTheCookie);
                    hr = m_pScope->InsertItem(&ScopeDataItem);
                    if (FAILED(hr))
                        break;
                    pTheItem->SetHandle(ScopeDataItem.ID);
                    pTheCookie = pTheCookie->GetSibling();
                } while (pTheCookie);
            }
            pScopeItem->Enumerated();
        }
    }

    else
    {
        // The provided lpDataObject is not ours, we are being
        // expanded as an extension snapin. Find out what
        // node type the data object is. If it is "MyComputer"
        // system tools, attach our scope items to
        // it.

        CLSID   CLSID_NodeType;
        hr = ExtractData(lpDataObject, CDataObject::m_cfNodeType,
                         (PBYTE)&CLSID_NodeType, sizeof(CLSID_NodeType));
        if (FAILED(hr))
            return hr;

        if (CLSID_SYSTOOLS == CLSID_NodeType)
        {
            TCHAR MachineName[MAX_PATH + 1];
            MachineName[0] = _T('\0');
            hr = ExtractData(lpDataObject, CDataObject::m_cfMachineName,
                             (BYTE*)MachineName, sizeof(MachineName));
            if (SUCCEEDED(hr))
            {
                m_ctRoot = COOKIE_TYPE_SCOPEITEM_DEVMGR;

                m_strMachineName.Empty();
                if (_T('\0') != MachineName[0])
                {
                    if (_T('\\') != MachineName[0])
                    {
                        m_strMachineName = TEXT("\\\\");
                    }
                    m_strMachineName += MachineName;
                }

                hr = CreateScopeItems();
                
                if (SUCCEEDED(hr))
                {
                    CMachine* pMachine;
                    pMachine = g_MachineList.FindMachine(m_strMachineName);
                    
                    if (!pMachine || pMachine != m_pMachine)
                    {
                        if (!g_MachineList.CreateMachine(m_hwndMain, m_strMachineName, &m_pMachine))
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                        }
                    }
                }
            }

            if (SUCCEEDED(hr))
            {

                //
                // Always insert "Device Manager" node because
                // we are expanding as an extention to Computer Management
                //
                CCookie* pCookie = GetActiveCookie(0);
                ASSERT(pCookie);
                CScopeItem* pScopeItem = pCookie->GetScopeItem();
                ASSERT(pScopeItem);
                SCOPEDATAITEM ScopeDataItem;
                memset(&ScopeDataItem, 0, sizeof(ScopeDataItem));
                ScopeDataItem.relativeID = (HSCOPEITEM)(param);
                ScopeDataItem.nState = 0;
                ScopeDataItem.displayname = (LPOLESTR)(-1);
                ScopeDataItem.mask = SDI_IMAGE | SDI_OPENIMAGE | SDI_CHILDREN |
                                     SDI_STR | SDI_PARAM | SDI_STATE;
                ScopeDataItem.nImage = pScopeItem->GetImageIndex();
                ScopeDataItem.nOpenImage = pScopeItem->GetOpenImageIndex();
                ScopeDataItem.cChildren = pScopeItem->GetChildCount();
                ScopeDataItem.lParam = reinterpret_cast<LPARAM>(pCookie);
                hr = m_pScope->InsertItem(&ScopeDataItem);
                pScopeItem->SetHandle(ScopeDataItem.ID);
            }
        }

    }

    return hr;
}
HRESULT
CComponentData::OnContextMenu(
    MMC_COOKIE cookie,
    LPARAM arg,
    LPARAM param
    )
{
    return S_OK;
}
