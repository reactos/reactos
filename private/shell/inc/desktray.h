#ifndef _DESKTRAY_H_
#define _DESKTRAY_H_

#undef  INTERFACE
#define INTERFACE   IDeskTray

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

DECLARE_INTERFACE_(IDeskTray, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IDeskTray methods ***
    STDMETHOD_(UINT, AppBarGetState)(THIS) PURE;
    STDMETHOD(GetTrayWindow)(THIS_ HWND* phwndTray) PURE;
    STDMETHOD(SetDesktopWindow)(THIS_ HWND hwndDesktop) PURE;

    // WARNING!  BEFORE CALLING THE SetVar METHOD YOU MUST DETECT
    // THE EXPLORER VERSION BECAUSE IE 4.00 WILL CRASH IF YOU TRY
    // TO CALL IT

    STDMETHOD(SetVar)(THIS_ int var, DWORD value) PURE;
};

#define SVTRAY_EXITEXPLORER     0   // g_fExitExplorer

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // _DESKTRAY_H_
