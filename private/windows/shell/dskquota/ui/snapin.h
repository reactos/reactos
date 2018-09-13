#ifndef _INC_DSKQUOTA_SNAPIN_H
#define _INC_DSKQUOTA_SNAPIN_H
///////////////////////////////////////////////////////////////////////////////
/*  File: snapin.h

    Description: Declares classes used in the disk quota policy MMC snapin.
        Indentation indicates inheritance.

        Classes:
            CSnapInComp         - implements IComponent.
            CSnapInCompData     - implements IComponentData.
            CDataObject

            CSnapInItem         - abstract base class for scope/result items.
                CScopeItem      - MMC scope pane item
                CResultItem     - MMC result pane item

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    02/14/98    Initial creation.                                    BrianAu
    06/25/98    Disabled snapin code with #ifdef POLICY_MMC_SNAPIN.  BrianAu
                Switching to ADM-file approach to entering policy
                data.  Keeping snapin code available in case
                we decide to switch back at a later time.
*/
///////////////////////////////////////////////////////////////////////////////
#ifdef POLICY_MMC_SNAPIN

#ifndef __mmc_h__
#   include <mmc.h>
#endif

#ifndef _GPEDIT_H_
#   include <gpedit.h>
#endif

#ifndef _INC_DSKQUOTA_STRCLASS_H
#   include "strclass.h"
#endif

#ifndef _INC_DSKQUOTA_CARRAY_H
#   include "carray.h"
#endif

//
// This CSnapInItem hierarchy represents the relationships between scope-pane
// items and result-pane items.  CSnapInItem contains data common to both
// types of items.  It also allows us to give the node mgr CSnapInItem ptrs
// and use virtual functions to retrieve the proper type-specific data
// when required.
//                          
//                     CSnapInItem (pure virtual)
//                           |
//                         is a
//            +--------------+---------------+
//            |                              |
//            |                              |
//            |                              |
//       CScopeItem ----- contains ---->> CResultItem
//
//
class CSnapInCompData; // fwd decl.
class CResultItem;     // fwd decl.

//
// Pure virtual base class for snap-in scope items and result items.
//
class CSnapInItem 
{
    public:
        CSnapInItem(LPCTSTR pszDisplayName, int iImage)
            : m_strDisplayName(pszDisplayName),
              m_iImage(iImage)
              { DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CSnapInItem::CSnapInItem"))); }

        virtual ~CSnapInItem(void) 
            { DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CSnapInItem::~CSnapInItem"))); }

        int ImageIndex(void) const
            { return m_iImage; }

        const CString& DisplayName(void) const
            { return m_strDisplayName; }

        //
        // All derived types must provide implementations for
        // NodeType and RenderData.
        //
        virtual const GUID& NodeType(void) const = 0;
        virtual HRESULT RenderData(UINT cf, LPSTGMEDIUM pMedium) const = 0;

    protected:
        //
        // Helper functions for rendering data.
        //
        static void GUIDToString(const GUID& guid, CString *pstr);
        static HRESULT RenderData(LPVOID pvData, int cbData, LPSTGMEDIUM pMedium);

    private:
        int     m_iImage;           // Index of image in imagelist.
        CString m_strDisplayName;   // Item's display name string.

        //
        // Prevent copy.
        //
        CSnapInItem(const CSnapInItem& rhs);
        CSnapInItem& operator = (const CSnapInItem& rhs);
};

//
// An item in the MMC scope pane.
//
class CScopeItem : public CSnapInItem
{
    public:
        CScopeItem(const GUID& NodeType, 
                   const CSnapInCompData& cd,
                   CScopeItem *pParent,
                   LPCTSTR pszDisplayName, 
                   int iImage,
                   int iImageOpen)
            : CSnapInItem(pszDisplayName, iImage),
              m_idType(NodeType),
              m_pParent(pParent),
              m_cd(cd),
              m_iOpenImage(iImageOpen)
              { DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CScopeItem::CScopeItem"))); }

        virtual ~CScopeItem(void);

        int OpenImageIndex(void) const
            { return m_iOpenImage; }

        int NumChildren(void) const
            { return m_rgpChildren.Count(); }

        void AddChild(CScopeItem *pNode)
            { m_rgpChildren.Append(pNode); }

        CScopeItem *Child(int iChild) const
            { return m_rgpChildren[iChild]; }

        int NumResultItems(void) const
            { return m_rgpResultItems.Count(); }

        CResultItem *ResultItem(int iItem) const
            { return m_rgpResultItems[iItem]; }

        void AddResultItem(CResultItem *pItem)
            { m_rgpResultItems.Append(pItem); }

        virtual const GUID& NodeType(void) const
            { return m_idType; }

        virtual HRESULT RenderData(UINT cf, LPSTGMEDIUM pMedium) const;

    private:
        const GUID&            m_idType;         // Node type GUID.
        const CSnapInCompData& m_cd;             // Snapin's component data.
        int                    m_iOpenImage;     // Index of "open" image.
        CScopeItem            *m_pParent;        // Parent scope pane item.
        CArray<CScopeItem *>   m_rgpChildren;    // Scope pane children.
        CArray<CResultItem *>  m_rgpResultItems; // Result items.

        //
        // Standard clipboard formats required by MMC.
        //
        static UINT m_cfNodeType;
        static UINT m_cfNodeTypeString;
        static UINT m_cfDisplayName;
        static UINT m_cfCoClass;

        //
        // Prevent copy.
        //
        CScopeItem(const CScopeItem& rhs);
        CScopeItem& operator = (const CScopeItem& rhs);
};


//
// An item in the MMC result pane.
//
class CResultItem : public CSnapInItem
{
    public:
        CResultItem(LPCTSTR pszDisplayName, int iImage, CScopeItem& si)
            : CSnapInItem(pszDisplayName, iImage),
              m_scopeItem(si)
              { DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CResultItem::CResultItem"))); }

        virtual ~CResultItem(void)
              { DBGTRACE((DM_SNAPIN, DL_MID, TEXT("CResultItem::~CResultItem"))); }

        CScopeItem& ScopeItem(void) const
            { return m_scopeItem; }

        virtual const GUID& NodeType(void) const
            { return m_scopeItem.NodeType(); }

        virtual HRESULT RenderData(UINT cf, LPSTGMEDIUM pMedium) const
            { return m_scopeItem.RenderData(cf, pMedium); }

    private:
        CScopeItem& m_scopeItem;    // Item's "owner" in scope pane.

        //
        // Prevent copy.
        //
        CResultItem(const CResultItem& rhs);
        CResultItem& operator = (const CResultItem& rhs);
};


//
// Required implementation for IComponent.
//
class CSnapInComp : public IComponent,
                    public IExtendContextMenu
{
    public:
        CSnapInComp(HINSTANCE hInstance, CSnapInCompData& cd);
        ~CSnapInComp(void);

        HRESULT GetScopeItem(HSCOPEITEM hItem, CScopeItem **ppsi) const;

        //
        // IUnknown methods.
        //
        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(VOID);
        STDMETHODIMP_(ULONG) Release(VOID);

        //
        // IComponent methods.
        //
        STDMETHODIMP Initialize(LPCONSOLE lpConsole);
        STDMETHODIMP Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param);
        STDMETHODIMP Destroy(LONG cookie);
        STDMETHODIMP QueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT *ppDataObject);
        STDMETHODIMP GetResultViewType(long cookie, LPOLESTR *ppViewType, long *pViewOptions);
        STDMETHODIMP GetDisplayInfo(RESULTDATAITEM *pResultDataItem); 
        STDMETHODIMP CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

        //
        // IExtendContextMenu methods.
        //
        STDMETHODIMP AddMenuItems(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK piCallback, long *pInsertionAllowed);
        STDMETHODIMP Command(long lCommandID, LPDATAOBJECT pDataObject);

    private:
        LONG             m_cRef;            // COM ref count.
        HINSTANCE        m_hInstance;       // For getting resources.
        CSnapInCompData& m_cd;              // Component's ComponentData
        LPCONSOLE        m_pConsole;        // The MMC console.
        LPRESULTDATA     m_pResult;         // The result pane.
        LPHEADERCTRL     m_pHeader;         // The result pane header ctrl.
        LPIMAGELIST      m_pImageResult;    // The result pane imagelist.
        CString          m_strColumn;       // Column header title.
        int              m_cxColumn;        // Column header width.
        long             m_lViewMode;       // Result pane view mode.

        //
        // Prevent copy.
        //
        CSnapInComp(const CSnapInComp& rhs);
        CSnapInComp& operator = (const CSnapInComp& rhs);
};

//
// Custom interface for data object.  Primary purpose is to distinguish
// our data object from any other data object.  This is done by calling
// QI for IQuotaDataObject.  If it succeeds, it's ours.
//
#undef INTERFACE
#define INTERFACE IQuotaDataObject
DECLARE_INTERFACE_(IQuotaDataObject, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;


    // *** IQuotaDataObject methods ***
    STDMETHOD(SetType) (THIS_ DATA_OBJECT_TYPES type) PURE;
    STDMETHOD(GetType) (THIS_ DATA_OBJECT_TYPES *type) PURE;

    STDMETHOD(SetItem) (THIS_ CSnapInItem *pItem) PURE;
    STDMETHOD(GetItem) (THIS_ CSnapInItem **pItem) PURE;
};
typedef IQuotaDataObject *LPQUOTADATAOBJECT;


//
// The snap-in's data object.
//
class CDataObject : public IDataObject,
                    public IQuotaDataObject
{
    public:
        CDataObject(CSnapInCompData& cd);
        ~CDataObject(void);

        //
        // IUnknown methods.
        //
        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(VOID);
        STDMETHODIMP_(ULONG) Release(VOID);

        //
        // IDataObject.  All but GetDataHere are unimplemented.
        //
        STDMETHODIMP GetDataHere(FORMATETC *pFormatetc, STGMEDIUM *pmedium);
        
        STDMETHODIMP GetData(FORMATETC *pFormatetc,  STGMEDIUM *pmedium)
            { return E_NOTIMPL; }
        STDMETHODIMP QueryGetData(FORMATETC *pFormatetc)
            { return E_NOTIMPL; }
        STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pFormatetcIn,  FORMATETC *pFormatetcOut)
            { return E_NOTIMPL; }
        STDMETHODIMP SetData(FORMATETC *pFormatetc, STGMEDIUM *pmedium, BOOL fRelease)
            { return E_NOTIMPL; }
        STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatetc)
            { return E_NOTIMPL; }
        STDMETHODIMP DAdvise(FORMATETC *pFormatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD * pdwConnection)
            { return E_NOTIMPL; }
        STDMETHODIMP DUnadvise(DWORD dwConnection)
            { return E_NOTIMPL; }
        STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
            { return E_NOTIMPL; }

        //
        // IQuotaDataObject.
        //
        STDMETHODIMP SetType(DATA_OBJECT_TYPES type)
            { m_type = type; return S_OK; }
        STDMETHODIMP GetType(DATA_OBJECT_TYPES *type)
            { *type = m_type; return S_OK; }
        STDMETHODIMP SetItem(CSnapInItem *pItem)
            { m_pItem = pItem; return S_OK; }
        STDMETHODIMP GetItem(CSnapInItem **ppItem)
            { *ppItem = m_pItem; return S_OK; }

    private:
        LONG               m_cRef;  // COM ref count.
        CSnapInCompData&   m_cd;    // Related component data object.
        DATA_OBJECT_TYPES  m_type;  // Type defined by MMC.
        CSnapInItem       *m_pItem; // Related snapin item.

        //
        // Prevent copy.
        //
        CDataObject(const CDataObject& rhs);
        CDataObject& operator = (const CDataObject& rhs);
};


//
// Required implementation for IComponentData.
//
class CSnapInCompData : public IComponentData,
                        public IPersistStreamInit,
                        public IExtendContextMenu
{
    public:
        //
        // Icon indexes in scope image list.
        //
        enum { iICON_QUOTA = 0,
               iICON_QUOTA_OPEN };

        CSnapInCompData(HINSTANCE hInstance, LPCTSTR pszDisplayName, const GUID& idClass);
        ~CSnapInCompData(void);

        const CString& DisplayName(void) const
            { return m_strDisplayName; }

        const GUID& ClassId(void) const
            { return m_idClass; }

        HRESULT OpenVolumeQuotaProperties(void);

        //
        // IUnknown methods.
        //
        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(VOID);
        STDMETHODIMP_(ULONG) Release(VOID);

        //
        // IComponentData methods.
        //
        STDMETHODIMP Initialize(LPUNKNOWN pUnknown);
        STDMETHODIMP CreateComponent (LPCOMPONENT *ppComponent);
        STDMETHODIMP Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param);
        STDMETHODIMP Destroy(void);
        STDMETHODIMP GetDisplayInfo(SCOPEDATAITEM *pScopeDataItem);
        STDMETHODIMP CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);
        STDMETHODIMP QueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT *ppDataObject);

        //
        // IPersistStreamInit methods.
        //
        STDMETHODIMP GetClassID(CLSID *pClassID);
        STDMETHODIMP IsDirty(void);
        STDMETHODIMP Load(IStream *pStm);
        STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);
        STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);
        STDMETHODIMP InitNew(void);

        //
        // IExtendContextMenu methods.
        //
        STDMETHODIMP AddMenuItems(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK piCallback, long *pInsertionAllowed);
        STDMETHODIMP Command(long lCommandID, LPDATAOBJECT pDataObject);


    private:
        LONG                 m_cRef;             // obj ref counter.
        HINSTANCE            m_hInstance;        // for getting resources.
        CString              m_strDisplayName;   // Component's display name.
        const GUID&          m_idClass;          // Component's class ID.
        LPCONSOLE            m_pConsole;         // SnapIn mgr's console interface.
        LPCONSOLENAMESPACE   m_pScope;           // SnapIn mgr's scope interface.
        CScopeItem          *m_pRootScopeItem;   // Scope pane node tree root.
        HSCOPEITEM           m_hRoot;            // Root of extension's namespace
        LPGPEINFORMATION     m_pGPEInformation;

        HRESULT EnumerateScopePane(HSCOPEITEM hParent);
        static BOOL CALLBACK AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam);
        static DWORD PropPageThreadProc(LPVOID pvParam);

        //
        // Prevent copy.
        //
        CSnapInCompData(const CSnapInCompData& rhs);
        CSnapInCompData& operator = (const CSnapInCompData& rhs);

        friend class CSnapInComp;
};

#endif // POLICY_MMC_SNAPIN

#endif // _INC_DSKQUOTA_SNAPIN_H
