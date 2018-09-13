#ifndef __COMPONET_H_
#define __COMPONET_H_

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    componet.h

Abstract:

    header file defines CComponent class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

class CComponent :
    public IComponent,
    public IResultDataCompare,
    public IExtendContextMenu,
    public IExtendControlbar,
    public IExtendPropertySheet,
#ifdef PERSIST_VIEW
    public IPersistStream,
#endif
    public ISnapinCallback
{
public:
    CComponent(CComponentData* pComponentData);
    ~CComponent();
// IUNKNOWN
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

// IComponent interface members
public:
    STDMETHOD(Initialize)(LPCONSOLE pConsole);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, MMC_COOKIE param);
    STDMETHOD(Destroy)(MMC_COOKIE cookie);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions);
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM* pResultDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IResultDataCompare interface member
    STDMETHOD(Compare)(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult);

// IExtendContextMenu
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject,
                LPCONTEXTMENUCALLBACK pCallbackUnknown,
                long* pInsertionAllowed
                );
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

// IExtendControlbar
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

// IExtendPropertySheet
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, LPDATAOBJECT lpDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

#ifdef PERSIST_VIEW
// IPersistStream
    STDMETHOD(GetClassID)(CLSID* pClassId);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream* pStm);
    STDMETHOD(Save)(IStream* pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);
#endif

// ISnapinCallback
    STDMETHOD(tvNotify)(HWND hwndTV, MMC_COOKIE cookie, TV_NOTIFY_CODE Code, LPARAM arg, LPARAM param);

protected:
    HRESULT OnShow(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnMinimize(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnResultItemClick(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnResultItemDblClick(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnProperties(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnViewChange(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnBtnClick(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnSelect(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnActivate(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnAddImages(MMC_COOKIE cookie, IImageList* pIImageList, HSCOPEITEM hScopeItem);
    HRESULT OnOcxNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    HRESULT OnRestoreView(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnContextHelp(MMC_COOKIE cookie, LPARAM arg, LPARAM param);

// Helper functions
    HRESULT DoPropertySheet(MMC_COOKIE cookie);
public:
    void SetDirty()
    {
#ifdef PERSIST_VIEW
        m_Dirty = TRUE;
#endif
    }
    HRESULT UpdateToolbar(CCookie* pCookie);
    HRESULT CreateFolderList(CCookie* pCookie);
    CScopeItem* FindScopeItem(MMC_COOKIE cookie)
    {
    return m_pComponentData->FindScopeItem(cookie);
    }
    CCookie* GetActiveCookie(MMC_COOKIE cookie)
    {
    return m_pComponentData->GetActiveCookie(cookie);
    }
    CMachine* GetMachine()
    {
    return m_pComponentData->m_pMachine;
    }
    LPCTSTR GetStartupDeviceId()
    {
    return m_pComponentData->m_strStartupDeviceId;
    }
    LPCTSTR GetStartupCommand()
    {
    return m_pComponentData->m_strStartupCommand;
    }
    HRESULT LoadScopeIconsForResultPane(IImageList* pIImageList);

    int MessageBox(LPCTSTR Msg, LPCTSTR Caption, DWORD Flags);
    BOOL AttachFolderToMachine(CFolder* pFolder, CMachine** ppMachine);

    LPCONSOLE       m_pConsole;         // Console's Interface
    LPHEADERCTRL    m_pHeader;          // Result pane's header control interface

    LPRESULTDATA    m_pResult;          // Interface pointer to the result pane
    LPCONSOLEVERB   m_pConsoleVerb;     //
    LPPROPERTYSHEETPROVIDER m_pPropSheetProvider;
    LPDISPLAYHELP   m_pDisplayHelp;
    LPTOOLBAR       m_pToolbar;         // Toolbar for view
    LPCONTROLBAR    m_pControlbar;      // Control bar to hold toolbar
private:
    CFolder* FindFolder(MMC_COOKIE cookie);
    BOOL DestroyFolderList(MMC_COOKIE cookie);
    HRESULT SaveFolderPersistData(CFolder* pFolder, IStream* pStm, BOOL fClearDirty);
    HRESULT LoadFolderPersistData(CFolder* pFolder, IStream* pStm);
    void DetachAllFoldersFromMachine();
    CComponentData* m_pComponentData;
    CFolder*        m_pCurFolder;
    CList<CFolder*, CFolder*> m_listFolder;
    CList<String*, String*> m_listDescBarText;
    DWORD       m_DescBarTextSP;
    BOOL        m_Dirty;
    BOOL        m_MachineAttached;
    ULONG       m_Ref;
};

#endif  // __COMPONET_H_
