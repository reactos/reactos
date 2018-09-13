#ifndef _CWNDPROC_H_
#define _CWNDPROC_H_

// CImpWndProc
//
// Use this class when you want to associate a window with
// an object using a virtual wndproc.
// 
// NOTE: The window's lifetime must be encompassed by the object.
//       I.E. NO REFCOUNT IS HELD ON THE OBJECT!
//
// Messages after WM_NCCREATE up to and including WM_DESTROY
// are forwarded to v_WndProc.
//
// _hwnd is non-NULL from WM_NCCREATE up to but not during WM_DESTROY.
// (Not during because the final release may be tied to WM_DESTROY
// so we cannot reference member variables after forwarding thie message.)
//
class CImpWndProc
{
public:
    virtual ULONG __stdcall AddRef() = 0;
    virtual ULONG __stdcall Release() = 0;

protected:
    virtual LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) PURE;
    static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND _hwnd;

} ;


// CNotifySubclassWndProc
//
// This class subclasses an HWND, registers for SHChangeNotify events,
// and forwards them to the inheritor's IShellChangeNotify implementation.
//
// You need one instance of this class per window you want to subclass
// and register for events against. (So if you need >1 window hooked up
// in this matter, you need to have member implementations that inherit
// from this class.)
//
class CNotifySubclassWndProc
{
public:
    virtual STDMETHODIMP OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) PURE;

protected:
    BOOL _SubclassWindow(HWND hwnd);
    void _UnsubclassWindow(HWND hwnd);
    void _RegisterWindow(HWND hwnd, LPCITEMIDLIST pidl, long lEvents,
                         UINT uFlags = (SHCNRF_ShellLevel | SHCNRF_InterruptLevel));
    void _UnregisterWindow(HWND hwnd);
    virtual LRESULT _DefWindowProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

    void _FlushNotifyMessages(HWND hwnd);

private:
    static LRESULT CALLBACK _SubclassWndProc(
                                  HWND hwnd, UINT uMessage, 
                                  WPARAM wParam, LPARAM lParam,
                                  UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    UINT        _uRegister;         // SHChangeNotify id

#ifdef DEBUG
    HWND        _hwndSubclassed;
#endif
} ;

#endif _CWNDPROC_H_
