

#ifndef __CFOLDER_H__
#define __CFOLDER_H__
/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    cfolder.h

Abstract:

    header file for cfolder.cpp.

Author:

    William Hsieh (williamh) created

Revision History:


--*/

class CFolder;
class CResultView;
class CViewTreeByType;
class CViewTreeByConnection;
class CViewResourceTree;
class CComponent;

const int IMAGE_INDEX_START = 20;
const int IMAGE_INDEX_DEVMGR    = IMAGE_INDEX_START;
const int IMAGE_INDEX_DEVICES   = IMAGE_INDEX_DEVMGR;
const int OPEN_IMAGE_INDEX_DEVMGR = IMAGE_INDEX_DEVMGR;
const int OPEN_IMAGE_INDEX_DEVICES = OPEN_IMAGE_INDEX_DEVMGR;

const int IMAGE_RESOURCE_INDEX_START = 0;

typedef enum tagViewType {
    VIEW_DEVICESBYTYPE = 0,
    VIEW_DEVICESBYCONNECTION,
    VIEW_RESOURCESBYTYPE,
    VIEW_RESOURCESBYCONNECTION,
    VIEW_NONE
}VIEWTYPE, *PVIEWTYPE;

typedef struct tagMMCMenuItem{
    int     idName;
    int     idStatusBar;
    long    lCommandId;
    VIEWTYPE Type;
}MMCMENUITEM, *PMMCMENUITEM;

const VIEWTYPE VIEW_FIRST = VIEW_DEVICESBYTYPE;
const VIEWTYPE VIEW_LAST = VIEW_RESOURCESBYCONNECTION;

const int TOTAL_VIEWS = VIEW_LAST - VIEW_FIRST + 1;
const int TOTAL_RESOURCE_TYPES = 4;

extern const MMCMENUITEM ViewDevicesMenuItems[];

typedef struct tagDevMgrFolderStates
{
    COOKIE_TYPE Type;
    VIEWTYPE    CurViewType;
}DEVMGRFOLDER_STATES, *PDEVMGRFOLDER_STATES;

typedef DWORD FOLDER_SIGNATURE;

const FOLDER_SIGNATURE_DEVMGR  = 0x00FF00FF;

//
// This is the class created and maintained by IComponentData
//
class CScopeItem
{
public:
    CScopeItem(COOKIE_TYPE ct, int iImage, int iOpenImage, int iNameStringId, int iDescStringId, int iDisplayNameFormatId)
    {
        m_iImage = iImage;
        m_iOpenImage = iOpenImage;
        m_iNameStringId = iNameStringId;
        m_iDescStringId = iDescStringId;
        m_Enumerated = FALSE;
        m_hScopeItem = NULL;
        m_Type = ct;
    }
    virtual CFolder* CreateFolder(CComponent* pComponent);
    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK pCallback, long* pInsertionAllowed);
    virtual HRESULT MenuCommand(long lCommandId);
    virtual HRESULT QueryPagesFor();
    virtual HRESULT CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    LONG_PTR handle);
    virtual ~CScopeItem();
    virtual BOOL Create();
    HRESULT GetDisplayInfo(LPSCOPEDATAITEM pScopeDataItem);
    void SetScopeItemID(HSCOPEITEM hScopeItem)
    {
        m_hScopeItem = hScopeItem;
    }
    const int   GetImageIndex() const
    {
        return m_iImage;
    }
    const int   GetOpenImageIndex() const
    {
        return m_iOpenImage;
    }
    const TCHAR* GetNameString()    const
    {
        return (LPCTSTR)m_strName;
    }
    const TCHAR* GetDescString() const
    {
        return (LPCTSTR)m_strDesc;
    }
    const TCHAR* GetDisplayNameFormat()
    {
        return (LPCTSTR)m_strDisplayNameFormat;
    }
    operator HSCOPEITEM()
    {
        return m_hScopeItem;
    }
    BOOL EnumerateChildren(int Index, CScopeItem** ppChild);
    int  GetChildCount()
    {
        return m_listChildren.GetCount();
    }
    COOKIE_TYPE GetType()
    {
        return m_Type;
    }
    void SetHandle(HSCOPEITEM hScopeItem)
    {
        m_hScopeItem = hScopeItem;
    }

    BOOL IsEnumerated()
    {
        return m_Enumerated;
    }
    void Enumerated()
    {
        m_Enumerated = TRUE;
    }
    HRESULT Reset();

protected:
    CCookie* FindSelectedCookieData(CResultView** ppResultView);
    int        m_iNameStringId;
    int        m_iDescStringId;
    int        m_iImage;
    int        m_iOpenImage;
    String    m_strName;
    String    m_strDesc;
    String    m_strDisplayNameFormat;
    BOOL       m_Enumerated;
    HSCOPEITEM m_hScopeItem;
    COOKIE_TYPE m_Type;
    CList<CFolder*, CFolder*> m_listFolder;
    CList<CScopeItem*, CScopeItem*> m_listChildren;
};


// While ScopeItem objects are created and managed by CComponentData,
// CFolder objects are created and managed by CComponent class.
// CComponent objects are created and managed by CComponentData.
// A CComponent object is created when a new window is created.
// For each CScopeItem, CComponent creates a CFolder object to
// represent that CScopeItem in the CComponent's window.
// CFolder is responsible for painting the result pane that represents
// the visual states of its associated CScopeItem.
//
class CFolder
{
public:
    CFolder(CScopeItem* pScopeItem, CComponent* pComponent);
    virtual ~CFolder();

    virtual HRESULT Compare(MMC_COOKIE cookieA, MMC_COOKIE cookieB, int nCol, int* pnResult);
    virtual HRESULT GetDisplayInfo(LPRESULTDATAITEM pResultDataItem);
    virtual HRESULT AddMenuItems(CCookie* pCookie, LPCONTEXTMENUCALLBACK pCallback, long* pInsertionAllowed);
    virtual HRESULT MenuCommand(CCookie* pCookie, long lCommandId);
    virtual HRESULT QueryPagesFor(CCookie* pCookie);
    virtual HRESULT CreatePropertyPages(CCookie* pCookie, LPPROPERTYSHEETCALLBACK lpProvider,
                    LONG_PTR handle);
    virtual HRESULT OnShow(BOOL fShow);
    virtual HRESULT GetResultViewType(LPOLESTR* ppViewType, long* pViewOptions);
    virtual CResultView* GetCurResultView()
        {
            return m_pCurView;
        }
    virtual int GetPersistDataSize()
    {
        return sizeof(DEVMGRFOLDER_STATES);
    }
    virtual HRESULT GetPersistData(PBYTE pBuffer, int BufferSize);
    virtual HRESULT SetPersistData(PBYTE pData, int Size);
    LPCTSTR GetNameString()
    {
        return m_pScopeItem->GetNameString();
    }
    LPCTSTR GetDescString()
    {
        return m_pScopeItem->GetDescString();
    }
    FOLDER_SIGNATURE GetSignature()
    {
        return m_Signature;
    }
    virtual HRESULT tvNotify(HWND hwndTV, CCookie* pCookie, TV_NOTIFY_CODE Code, LPARAM arg, LPARAM param);
    virtual HRESULT MachinePropertyChanged(CMachine* pMachine);
    virtual HRESULT OnOcxNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    virtual HRESULT OnAddImages(IImageList* pIImageList, HSCOPEITEM hScopeItem)
    {
        return m_pComponent->LoadScopeIconsForResultPane(pIImageList);
    }
    virtual HRESULT OnRestoreView(BOOL* pfHandled);
    virtual HRESULT OnSelect()
    {
            // If the folder is selected, reset the console verbs.
            if (m_bSelect)
                return S_FALSE;
            else
                return S_OK;
    }
    ULONG AddRef()
    {
        return ++m_Ref;
    }
    ULONG Release()
    {
        ASSERT(m_Ref);
        --m_Ref;

        if (!m_Ref)
        {
        delete this;
        return 0;
        }
        return m_Ref;
    }
    virtual HRESULT Reset();
    BOOL ShowHiddenDevices()
    {
        return m_ShowHiddenDevices;
    }
    BOOL SelectView(VIEWTYPE ViewType, BOOL fShowHiddenDevices);
    HRESULT DoProperties(HWND hwndParent, CCookie* pCookie);
    HRESULT DoContextMenu(HWND hwndParent, CCookie* pCookie, LPARAM param);
#ifdef DISPLAY_STATUS_BAR
    BOOL SetDescBarText(LPCTSTR DescBarText);
#endif
    CComponent*     m_pComponent;
    CScopeItem*     m_pScopeItem;
    CMachine*       m_pMachine;
    BOOL            m_bSelect;          // Saved by MMCN_SELECT for MenuCommand
protected:
    FOLDER_SIGNATURE m_Signature;
    BOOL        m_Show;
    String      m_strScratch;
    LPOLESTR        m_pOleTaskString;
    ULONG       m_Ref;
private:
    CViewTreeByType*        m_pViewTreeByType;
    CViewTreeByConnection*  m_pViewTreeByConnection;
    CViewResourceTree*      m_pViewResourcesByType;
    CViewResourceTree*      m_pViewResourcesByConnection;
    CResultView*        m_pCurView;
    VIEWTYPE            m_CurViewType;
    BOOL                    m_ShowHiddenDevices;
    BOOL            m_FirstTimeOnShow;
};

class CResultView
{
public:
    CResultView(int DescStringId) :
    m_DescStringId(DescStringId),
    m_pFolder(NULL),
    m_pMachine(NULL),
    m_pCookieComputer(NULL),
    m_pIDMTVOCX(NULL),
    m_hwndTV(NULL),
    m_pSelectedCookie(NULL),
        m_SelectOk(FALSE),
    m_pSelectedItem(NULL)
    {}
    virtual ~CResultView();
    virtual HRESULT GetDisplayInfo(LPRESULTDATAITEM pResultDataItem)
    {
        return S_OK;
    }
    virtual HRESULT OnShow(BOOL fShow);
    LPCTSTR GetStartupDeviceId();
    LPCTSTR GetStartupCommand();
    void SetFolder(CFolder* pFolder)
    {
        m_pFolder = pFolder;
    }
    int GetDescriptionStringID()
    {
        return m_DescStringId;
    }
    virtual CCookie* GetSelectedCookie()
        {
            return m_pSelectedCookie;
        }
    void SetSelectOk(BOOL fSelect)
    {
            m_SelectOk = fSelect;
    }
    HRESULT MachinePropertyChanged(CMachine* pMachine);
    void SaveTreeStates(CCookie* pCookieStart);
    void DestroySavedTreeStates();
    void RestoreSavedTreeState(CCookie* pCookie);
    BOOL DisplayTree();
    HRESULT GetResultViewType(LPOLESTR* ppViewType, long* pViewOptions);
    HRESULT AddMenuItems(CCookie* pCookie, LPCONTEXTMENUCALLBACK pCallback,
                                 long* pInsertionAllowed, BOOL fContextMenu);
    HRESULT MenuCommand(CCookie* pCookie, long lCommandId);
    HRESULT QueryPagesFor(CCookie* pCookie);
    HRESULT CreatePropertyPages(CCookie* pCookie, LPPROPERTYSHEETCALLBACK pProvider, LONG_PTR handle);
    HRESULT tvNotify(HWND hwndTV, CCookie* pCookie, TV_NOTIFY_CODE Code, LPARAM arg, LPARAM param);
    HRESULT OnOcxNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    HRESULT DoProperties(HWND hwndParent, CCookie* pCookie);
    HRESULT DoContextMenu(HWND hwndParent, CCookie* pCookie, POINT* pPoint);
    HRESULT DoPrint();
#ifdef DISPLAY_STATUS_BAR
    HRESULT UpdateViewDescText(LPCTSTR NewText = NULL);
#endif
protected:
    BOOL    DisplaySubtree(HTREEITEM htiParent, CCookie* pCookieStart, BOOL* pReportProblem = NULL);
    HRESULT UpdateConsoleVerbs(CCookie* pCookie);
    CFolder*    m_pFolder;
    int     m_DescStringId;
    CMachine*   m_pMachine;
    CCookie*        m_pCookieComputer;
    IDMTVOCX*       m_pIDMTVOCX;
    CCookie*        m_pSelectedCookie;
    BOOL            m_SelectOk;
    HWND        m_hwndTV;
    CList<CItemIdentifier*, CItemIdentifier*> m_listExpandedItems;
    CItemIdentifier*        m_pSelectedItem;
private:
    HRESULT RemoveDevice(CDevice* pDevice);
};

class CViewDeviceTree : public CResultView
{
public:
    CViewDeviceTree(int DescStringId)
        : CResultView(DescStringId)
    {}
    virtual ~CViewDeviceTree() {}
    virtual HRESULT OnShow(BOOL fShow);
//    static BOOL AddPropPage(HPROPSHEETPAGE hPropPage, LPARAM lParam);
    virtual BOOL  CreateDeviceTree();
#if DBG
    void DumpDeviceTree();
    void DumpDeviceSubtree(LPCTSTR Insert, CCookie* pCookie);
#endif

protected:
#ifdef PSS_TROUBLESHOOTING
    HRESULT DoTroubleshooting(CCookie* pCookie);
#endif
private:
};

class CViewTreeByType : public CViewDeviceTree
{
public:
    CViewTreeByType()
        : CViewDeviceTree(IDS_STATUS_DEVICES_BYTYPE)
    {}
    virtual BOOL CreateDeviceTree();
};

class CViewTreeByConnection : public CViewDeviceTree
{
public:
    CViewTreeByConnection()
          : CViewDeviceTree(IDS_STATUS_DEVICES_BYCONN)
    {}
    virtual BOOL CreateDeviceTree();
private:
    BOOL CreateSubtree(CCookie* pCookieParent, CCookie* pCookieSibling, CDevice* pDeviceStart);
};

class CViewResourceTree : public CResultView
{
public:
    CViewResourceTree(int DescStringId)
    : CResultView(DescStringId)
    {
        int i;

        for (i = 0; i < TOTAL_RESOURCE_TYPES; i++)
        {
        m_pResourceList[i] = NULL;
        m_pResourceType[i] = NULL;
        }
    }
    ~CViewResourceTree();
    virtual HRESULT OnShow(BOOL fShow);
protected:
private:
    void    CreateResourceTree();
    void    CreateResourceSubtree(CCookie* pCookieParent,
                  CCookie* pCookieSibling,
                  CDevice* pDevice, CCookie** ppLastCookie = NULL);
    BOOL    InsertCookieToTree(CCookie* pCookie, CCookie* pCookieStart,
                   BOOL ForcedInsert);
    void    DestroyResourceTree();
    CResourceList*  m_pResourceList[TOTAL_RESOURCE_TYPES];
    CResourceType*  m_pResourceType[TOTAL_RESOURCE_TYPES];
};

typedef BOOL (CALLBACK* PROPSHEET_PROVIDER_PROC)(
    PSP_PROPSHEETPAGE_REQUEST   PropPageRequest,
    LPFNADDPROPSHEETPAGE    lpfnAddPropPageProc,
    LPARAM          lParam
    );

class CPropPageProvider
{
public:
    CPropPageProvider() : m_hDll(NULL)
    {}
    virtual ~CPropPageProvider()
    {
         if (m_hDll)
        FreeLibrary(m_hDll);
    }
    virtual BOOL EnumPages(CDevice* pDevice, CPropSheetData* ppsd) = 0;
protected:
    HMODULE m_hDll;
};

class CBusPropPageProvider : public CPropPageProvider
{
public:
    virtual BOOL EnumPages(CDevice* pDevice, CPropSheetData* ppsd);
};

#endif // __CFOLDER_H__
