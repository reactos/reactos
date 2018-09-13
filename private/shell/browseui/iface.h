#ifndef _IFACE_H
#define _IFACE_H

// Interfaces and IIDs defined here are private to shdocvw.dll
//


//
// IDocNavigate
//
// DocHost needs to notify the browser of certain events
//
//131A6950-7F78-11D0-A979-00C04FD705A2
#undef  INTERFACE
#define INTERFACE  IDocNavigate
DECLARE_INTERFACE_(IDocNavigate, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IDocNavigate methods ***
    STDMETHOD(OnReadyStateChange)(THIS_ IShellView* psvSource, DWORD dwReadyState) PURE;
    STDMETHOD(get_ReadyState)(THIS_ DWORD * pdwReadyState) PURE;

} ;

//
// IBandNavigate
//
//  band needs to navigate its UI to a specific pidl.
//
#undef  INTERFACE
#define INTERFACE  IBandNavigate
DECLARE_INTERFACE_(IBandNavigate, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IBandNavigate methods ***
    STDMETHOD(Select)(THIS_ LPCITEMIDLIST pidl) PURE;

} ;


//
// IEFrameAuto
//
// CIEFrameAuto private interface to hold randum stuff
//
//131A6953-7F78-11D0-A979-00C04FD705A2
#undef  INTERFACE
#define INTERFACE  IEFrameAuto
DECLARE_INTERFACE_(IEFrameAuto, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IEFrameAuto methods ***
    STDMETHOD(SetOwnerHwnd)(THIS_ HWND hwndOwner) PURE;
    STDMETHOD(put_DefaultReadyState)(THIS_ DWORD dwDefaultReadyState, BOOL fUpdateBrowserReadyState) PURE;
    STDMETHOD(OnDocumentComplete)(THIS) PURE;
    STDMETHOD(OnWindowsListMarshalled)(THIS) PURE;
} ;

//
// IPrivateOleObject
//
// a cut down version of IOleObject used for the WebBrowserOC to communicate with
// objects hosted via CDocObjectView
#undef INTERFACE
#define INTERFACE IPrivateOleObject
DECLARE_INTERFACE_(IPrivateOleObject, IUnknown )
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // IPrivateOleObject
    STDMETHOD( SetExtent )( DWORD dwDrawAspect, SIZEL *psizel) PURE;
    STDMETHOD( GetExtent )( DWORD dwDrawAspect, SIZEL *psizel) PURE;
};


STDAPI AddUrlToUrlHistoryStg(LPCWSTR pwszUrl, LPCWSTR pwszTitle, LPUNKNOWN punk, 
                             BOOL fWriteToHistory, IOleCommandTarget *poctNotify, IUnknown *punkSFHistory,
                             UINT* pcodepage);

#ifdef __cplusplus

//
// LATER: Move all ITravelLog/ITravelEntry definitions here
//
// TLOG_BACKEXTERNAL -- succeeds only if the previous entry is external
//
#define TLOG_BACKEXTERNAL   -0x7fffffff

#endif // __cplusplus

#endif // _IFACE_H

