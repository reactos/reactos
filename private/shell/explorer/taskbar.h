#include "taskband.h"

#ifdef __cplusplus

class CTaskBar : public CSimpleOleWindow 
               , public IContextMenu
               , public IServiceProvider
               , public IRestrict
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CSimpleOleWindow::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void){ return CSimpleOleWindow::Release();};

    // *** IContextMenu methods ***
    STDMETHOD(QueryContextMenu)(HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags);

    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR    idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax);
    
    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void ** ppvObj);

    // *** IRestrict ***
    virtual STDMETHODIMP IsRestricted(const GUID * pguidID, DWORD dwRestrictAction, VARIANT * pvarArgs, DWORD * pdwRestrictionResult);
    
    // *** CSimpleOleWindow - IDeskBar ***
    STDMETHOD(OnPosRectChangeDB)(LPRECT prc);

    CTaskBar();
    
protected:
    //virtual ~CTaskBar();

    BITBOOL _fRestrictionsInited :1;        // Have we read in the restrictions?
    BITBOOL _fRestrictDDClose :1;           // Restrict: Add, Close, Drag & Drop
    BITBOOL _fRestrictMove :1;              // Restrict: Move
};

STDAPI_(void)  Tray_ContextMenuInvoke(int idCmd);
STDAPI_(HMENU) Tray_BuildContextMenu(BOOL fIncludeTime);
STDAPI_(void)  Tray_AsyncSaveSettings();
STDAPI_(IUnknown*) Tray_CreateView();

#endif
