/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     IImageList (MMC Specific!) implementation
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once

class CImageList :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IImageList
{
private:
    HIMAGELIST m_hLarge;
    HIMAGELIST m_hSmall;

public:
    CImageList(HIMAGELIST hLarge, HIMAGELIST hSmall)
    {
        m_hLarge = ImageList_Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), ILC_MASK | ILC_COLOR24, 1, 1);
        m_hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_MASK | ILC_COLOR24, 1, 1);

        HICON defIcon = LoadIcon(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDI_FOLDERICON));
        ImageList_AddIcon(m_hLarge, defIcon);
        ImageList_AddIcon(m_hSmall, defIcon);
        DestroyIcon(defIcon);
    }

    ~CImageList()
    {
        ImageList_Destroy(m_hLarge);
        ImageList_Destroy(m_hSmall);
    }

    void AddToListView(HWND hWndListView)
    {
        ListView_SetImageList(hWndListView, m_hLarge, LVSIL_NORMAL);
        ListView_SetImageList(hWndListView, m_hSmall, LVSIL_SMALL);
    }

    // ILSI_SMALL_ICON, ILSI_LARGE_ICON
    STDMETHODIMP ImageListSetIcon(LONG_PTR *Icon, LONG nLoc)
    {
        DPRINT("%s(Icon=%p, nLoc=%lld)\n", __FUNCTION__, Icon, nLoc);
        __debugbreak();
        return S_OK;
    }

    STDMETHODIMP ImageListSetStrip(LONG_PTR *BMapSm, LONG_PTR *BMapLg, LONG nStartLoc, COLORREF cMask)
    {
        DPRINT("%s(BMapSm=%p, BMapLg=%p, nStartLoc=%llu, cMask=0x%x)\n", __FUNCTION__, BMapSm, BMapLg, nStartLoc, cMask);
        __debugbreak();
        return S_OK;
    }


    BEGIN_COM_MAP(CImageList)
        COM_INTERFACE_ENTRY_IID(IID_IImageList, IImageList)
    END_COM_MAP()
};

