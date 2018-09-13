/*****************************************************************************\
    FILE: ftpobj.h
\*****************************************************************************/

#ifndef _FTPOBJ_H
#define _FTPOBJ_H

#include "ftpefe.h"


typedef struct
{
    DVTARGETDEVICE dvTargetDevice;
    FORMATETC formatEtc;
    STGMEDIUM medium;
} FORMATETC_STGMEDIUM;


/*****************************************************************************\
    CLASS: CFtpObj

    Careful!  The elements of m_stgCache are rather weird due to delayed
    rendering.  If m_stgCache[].tymed == TYMED_HGLOBAL but
    m_stgCache[].hGlobal == 0, then the FORMATETC exists in the DataObject,
    but hasn't been rendered yet.

    It will be rendered when you call CFtpObj::_ForceRender().

    This weirdness with delayed rendering means that you have to be
    careful when you try to access the gizmo.

    1. Before trying to use the gizmo, use CFtpObj::_ForceRender().
    2. When trying to free the gizmo, use CFtpObj::_ReleasePstg().

    Yet another weirdness with m_stgCache is that all hGlobal's have a
    special babysitter pUnkForRelease.  This is important so that
    interactions between CFtpObj::GetData and CFtpObj::SetData are isolated.

    (If you were lazy and used the CFtpObj itself as the pUnkForRelease,
    then you'd run into trouble if somebody tried to SetData into the
    data object, which overwrites an hGlobal you had previously given away.)

    m_nStartIndex/m_nEndIndex: We give out a list of FILEDESCRIPTORS in the
    FILEGROUPDESCRIPTOR.  If the directory attribute is set, the caller will
    just create the directory.  If it's a file, it will call IDataObject::GetData()
    with DROP_FCont.  We would like to display progress on the old shell because
    it normally doesn't display progress until NT5.  We need to decide when to start
    and stop.  We set m_nStartIndex to -1 to indicate that we don't know.  When we
    get a DROP_FCont call, we then calculate the first and the last.  We will then
    display the progress dialog until the caller has either called the last one or
    errored out.

    The data is kept in two places.  The data we offer and render is in m_stgCache.
    The data we will carry is stored in m_hdsaSetData.
\*****************************************************************************/
class CFtpObj           : public IDataObject
                        , public IPersistStream
                        , public IInternetSecurityMgrSite
                        , public IAsyncOperation
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IDataObject ***
    virtual STDMETHODIMP GetData(FORMATETC *pfmtetcIn, STGMEDIUM *pstgmed);
    virtual STDMETHODIMP GetDataHere(FORMATETC *pfmtetc, STGMEDIUM *pstgpmed);
    virtual STDMETHODIMP QueryGetData(FORMATETC *pfmtetc);
    virtual STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pfmtetcIn, FORMATETC *pfmtetcOut);
    virtual STDMETHODIMP SetData(FORMATETC *pfmtetc, STGMEDIUM *pstgmed, BOOL bRelease);
    virtual STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppienumFormatEtc);
    virtual STDMETHODIMP DAdvise(FORMATETC *pfmtetc, DWORD dwAdviseFlags, IAdviseSink * piadvsink, DWORD * pdwConnection);
    virtual STDMETHODIMP DUnadvise(DWORD dwConnection);
    virtual STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppienumStatData);
    
    // *** IPersist ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID){ *pClassID = CLSID_FtpDataObject; return S_OK; }
    
    // *** IPersistStream ***
    virtual STDMETHODIMP IsDirty(void) {return S_OK;}       // Indicate that we are dirty and ::Save() needs to be called.
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);
    virtual STDMETHODIMP GetSizeMax(ULARGE_INTEGER * pcbSize);

    // *** IInternetSecurityMgrSite ***
    virtual STDMETHODIMP GetWindow(HWND * phwnd) { if (phwnd) *phwnd = NULL; return S_OK; };
    virtual STDMETHODIMP EnableModeless(BOOL fEnable) {return E_NOTIMPL;};

    // *** IAsyncOperation methods ***
    virtual STDMETHODIMP SetAsyncMode(BOOL fDoOpAsync) {return E_NOTIMPL;};
    virtual STDMETHODIMP GetAsyncMode(BOOL * pfIsOpAsync);
    virtual STDMETHODIMP StartOperation(IBindCtx * pbcReserved);
    virtual STDMETHODIMP InOperation(BOOL * pfInAsyncOp);
    virtual STDMETHODIMP EndOperation(HRESULT hResult, IBindCtx * pbcReserved, DWORD dwEffects);

public:
    CFtpObj();
    ~CFtpObj(void);

    // Public Member Functions
    static int _DSA_FreeCB(LPVOID pvItem, LPVOID pvlparam);
    CFtpPidlList * GetHfpl() { return m_pflHfpl;};

    // Friend Functions
    friend HRESULT CFtpObj_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, REFIID riid, LPVOID * ppvObj);
    friend HRESULT CFtpObj_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, CFtpObj ** ppfo);
    friend HRESULT CFtpObj_Create(REFIID riid, void ** ppvObj);
    friend class CFtpEfe;

protected:
    // Private Member Variables
    int                     m_cRef;

    CFtpFolder *            m_pff;          // My dad
    CFtpDir *               m_pfd;          // My dad's home
    CFtpPidlList *          m_pflHfpl;      // List/Array of pidls
    STGMEDIUM               m_stgCache[DROP_MAX];
    HDSA                    m_hdsaSetData;  // Array of SetData.  Each item is a FORMATETC_STGMEDIUM.

    // Members for Progress on Legacy systems.
    IProgressDialog *       m_ppd;
    ULARGE_INTEGER          m_uliCompleted;
    ULARGE_INTEGER          m_uliTotal;
    int                     m_nStartIndex;  // Commented above in CLASS: CFtpObj
    int                     m_nEndIndex;    // Commented above in CLASS: CFtpObj
    BOOL                    m_fFGDRendered; // Did we expand m_pflHfpl?
    BOOL                    m_fCheckSecurity;  // TRUE means check security and display UI.  FALSE means it's unsafe and cancel w/o UI because it was already shown..
    BOOL                    m_fDidAsynchStart; // Did the IDropTarget call IAsynchDataObject::StartOperation() to start the copy? (To show he supports it)
    BOOL                    m_fErrAlreadyDisplayed; // Did was already display the error?
    IUnknown *              m_punkThreadRef; // Don't allow the browser closing to cancel our drag/drop operation.

    // Private Member Functions
    void _CheckStg(void);
    BOOL _IsLindexOkay(int ife, FORMATETC *pfeWant);
    HRESULT _FindData(FORMATETC *pfe, PINT piOut);
    HRESULT _FindDataForGet(FORMATETC *pfe, PINT piOut);
    HGLOBAL _DelayRender_FGD(BOOL fUnicode);
    HRESULT _DelayRender_IDList(STGMEDIUM * pStgMedium);
    HRESULT _DelayRender_URL(STGMEDIUM * pStgMedium);
    HRESULT _DelayRender_PrefDe(STGMEDIUM * pStgMedium);
    HRESULT _RenderOlePersist(STGMEDIUM * pStgMedium);
    HRESULT _RenderFGD(int nIndex, STGMEDIUM * pStgMedium);
    HRESULT _ForceRender(int ife);
    HRESULT _RefThread(void);
    CFtpPidlList * _ExpandPidlListRecursively(CFtpPidlList * ppidlListSrc);

    int _FindExtraDataIndex(FORMATETC *pfe);
    HRESULT _SetExtraData(FORMATETC *pfe, STGMEDIUM *pstg, BOOL fRelease);
    HRESULT _RenderFileContents(LPFORMATETC pfe, LPSTGMEDIUM pstg);

    HRESULT _DoProgressForLegacySystemsPre(void);
    HRESULT _DoProgressForLegacySystemsStart(LPCITEMIDLIST pidl, int nIndex);
    HRESULT _DoProgressForLegacySystemsPost(LPCITEMIDLIST pidl, BOOL fLast);
    HRESULT _SetProgressDialogValues(int nIndex);
    HRESULT _CloseProgressDialog(void);
};

#endif // _FTPOBJ_H
