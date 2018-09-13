//
// File: dlgcset.cpp
//
// This file contains the code that implements CNativeFont class.
//
// history:
//     7-21-97 created; 
// 
#include "ctlspriv.h"
#include "ccontrol.h"

#define THISCLASS CNativeFont
#define SUPERCLASS CControl

typedef enum 
{
    FAS_NOTINITIALIZED = 0,
    FAS_DISABLED,
    FAS_ENABLED,
} FASTATUS;

class CNativeFont : public CControl
{
public:
    //Function Memebers
    virtual LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT NativeFontWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
protected:
    
    CNativeFont();
    
    //Function Members    

    virtual void v_OnPaint(HDC hdc) ;
    virtual LRESULT v_OnCreate();
    virtual void v_OnSize(int x, int y)  {};

    virtual LRESULT v_OnCommand(WPARAM wParam, LPARAM lParam);
    virtual LRESULT v_OnNotify(WPARAM wParam, LPARAM lParam);
    virtual DWORD v_OnStyleChanged(WPARAM wParam, LPARAM lParam) { return 0; };    
    
    HRESULT _GetNativeDialogFont(HWND hDlg);
    static HRESULT _GetFontAssocStatus(FASTATUS  *uiAssoced);
    static BOOL _SetFontEnumProc(HWND hwnd, LPARAM lparam);
    static LRESULT _SubclassDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam, WPARAM uIdSubclass, ULONG_PTR dwRefData);

    HFONT   m_hfontOrg;
    HFONT   m_hfontNative;
    HFONT   m_hfontDelete;
    typedef struct {
                HFONT hfontSet;
                DWORD dwStyle;
            } NFENUMCHILDDATA;
    static FASTATUS _s_uiFontAssocStatus;
};

// static variable initialization
FASTATUS CNativeFont::_s_uiFontAssocStatus = FAS_NOTINITIALIZED;

// reg keys
static const TCHAR s_szRegFASettings[] = TEXT("System\\CurrentControlSet\\Control\\FontAssoc\\Associated Charset");

CNativeFont::CNativeFont(void)
{
    m_hfontOrg = NULL;
    m_hfontNative = NULL;
    m_hfontDelete = NULL;
}

LRESULT THISCLASS::NativeFontWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CNativeFont *pn = (CNativeFont *)GetWindowLongPtr(hwnd, 0);
    if (uMsg == WM_CREATE) {
        ASSERT(!pn);
        pn = new CNativeFont();
        if (!pn)
            return 0L;
    } 

    if (pn) {
        return pn->v_WndProc(hwnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void THISCLASS::v_OnPaint(HDC hdc)
{
    return;
}

LRESULT THISCLASS::v_OnCommand(WPARAM wParam, LPARAM lParam)
{
    // forward to parent (do we really need this?)
    return SendMessage(ci.hwndParent, WM_COMMAND, wParam, lParam);
}

LRESULT THISCLASS::v_OnNotify(WPARAM wParam, LPARAM lParam)
{
    // forward to parent
    LPNMHDR lpNmhdr = (LPNMHDR)lParam;
    
    return SendNotifyEx(ci.hwndParent, (HWND) -1,
                         lpNmhdr->code, lpNmhdr, ci.bUnicode);
}

LRESULT THISCLASS::v_OnCreate()
{
    return TRUE;
}

BOOL THISCLASS::_SetFontEnumProc(HWND hwnd, LPARAM lparam)
{
     NFENUMCHILDDATA *  pdt = (NFENUMCHILDDATA *)lparam; 
     BOOL bMatch = FALSE;
     
     if (pdt && pdt->hfontSet)
     {
         if (pdt->dwStyle & NFS_ALL)
         {
             bMatch = TRUE;
         }
         else
         {
             TCHAR szClass[32];
             
             GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
             
             if (pdt->dwStyle & NFS_EDIT)
             {
                 bMatch |= (lstrcmpi(TEXT("Edit"), szClass) == 0);
                 bMatch |= (lstrcmpi(TEXT("RichEdit20A"), szClass) == 0);
                 bMatch |= (lstrcmpi(TEXT("RichEdit20W"), szClass) == 0);
             }
             
             if (pdt->dwStyle & NFS_STATIC)
                 bMatch |= (lstrcmpi(TEXT("Static"), szClass) == 0);
             
             if (pdt->dwStyle & NFS_BUTTON)
                 bMatch |= (lstrcmpi(TEXT("Button"), szClass) == 0);

             if (pdt->dwStyle & NFS_LISTCOMBO)
             {
                 bMatch |= (lstrcmpi(TEXT("ListBox"), szClass) == 0);
                 bMatch |= (lstrcmpi(TEXT("ComboBox"), szClass) == 0);
                 bMatch |= (lstrcmpi(TEXT("ComboBoxEx32"), szClass) == 0);
                 bMatch |= (lstrcmpi(TEXT("SysListView32"), szClass) == 0);
             }
         }

         if (bMatch) 
             SendMessage(hwnd, WM_SETFONT, (WPARAM)pdt->hfontSet, MAKELPARAM(FALSE, 0));

         return TRUE;
     }
     else
         return FALSE;
}

LRESULT THISCLASS::_SubclassDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam, WPARAM uIdSubclass, ULONG_PTR dwRefData)
{
    LRESULT lret = 0;
    CNativeFont * pnf = (CNativeFont *)dwRefData;
    
    if (pnf)
    {
    
        switch (uMsg)
        {
            case WM_INITDIALOG:
                // we enumerate its children so they get font 
                // in native charset selected if necessary
                // 
                if (S_OK == pnf->_GetNativeDialogFont(hdlg))
                {
                    // S_OK means we have different charset from 
                    // the default of the platform on which we're 
                    // running.
                    NFENUMCHILDDATA dt;
                    dt.hfontSet = pnf->m_hfontNative;
                    dt.dwStyle = pnf->ci.style;
                    EnumChildWindows(hdlg, (WNDENUMPROC)pnf->_SetFontEnumProc, (LPARAM)&dt);
                }
                // we no longer need subclass procedure.
                // assumes no one has subclassed this dialog by now
                break;

            case WM_DESTROY:
                // if we've created a font, we have to clean it up.
                if (pnf->m_hfontDelete)
                {
                    NFENUMCHILDDATA dt;
                
                    dt.hfontSet = pnf->m_hfontOrg;
                    dt.dwStyle = pnf->ci.style;
                    // just in case anyone is still alive
                    EnumChildWindows(hdlg, (WNDENUMPROC)pnf->_SetFontEnumProc, (LPARAM)&dt);
                    DeleteObject(pnf->m_hfontDelete);
                    pnf->m_hfontDelete = NULL;
                }
                RemoveWindowSubclass(hdlg, pnf->_SubclassDlgProc, 0);

                break;
        }
    
        lret = DefSubclassProc(hdlg, uMsg, wParam, lParam);
    }
 
    return lret;
}

LRESULT THISCLASS::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hdlg;
    
    switch (uMsg)
    {
        case WM_CREATE:
        // subclass the parent dialog just to get notified for WM_INITDIALOG
            hdlg = GetParent(hwnd);
            if (hdlg)
            {
                // if we had an error just do nothing, we have to succeed in creating
                // window anyway otherwise dialog fails.
                SetWindowSubclass(hdlg, _SubclassDlgProc, 0, (ULONG_PTR)this);
            }
            break;
    }
    return SUPERCLASS::v_WndProc(hwnd, uMsg, wParam, lParam);
}

// _GetNativeDialogFont
//
// Retreive font handle in platform native character set
//
// returns S_OK if the given dialogbox requires setting font
//              in native charset
//         S_FALSE if the given dialogbox already has native
//              charset.
//         E_FAIL if anyother error occurs
//
HRESULT THISCLASS::_GetNativeDialogFont(HWND hDlg)
{
    HRESULT hres = E_FAIL;
    
    if(!m_hfontNative)
    {
        HFONT hfontNative, hfont = GetWindowFont(hDlg);
        LOGFONT lf, lfNative;
        FASTATUS uiFAStat = FAS_NOTINITIALIZED;
        GetObject(hfont, sizeof(LOGFONT), &lf);

        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lfNative, 0);
        
        // there are two cases we don't want to create/set font
        // for the platform native character set.
        // 1) we already have matching character set
        // 2) the platform has 'font assoc' enabled or 'font link'
        //    and our client wants to use it instead of
        //    setting the right character set. (NFS_USEFONTASSOC)
        //    this solution sometimes provides better
        //    appearance (thought it is broken in its 
        //    font metrics) because it would use 
        //    'western font' as is.
        if (ci.style & NFS_USEFONTASSOC)
        {
            _GetFontAssocStatus(&uiFAStat);
        }

        if ( uiFAStat == FAS_ENABLED
           || lfNative.lfCharSet == lf.lfCharSet)
        {
                
            m_hfontOrg = m_hfontNative = hfont;
        }
        else
        {
            // we have non-native charset for the platform
            // Save away the original font first.
            m_hfontOrg = hfont;
            
            // Use the height of original dialog font
            lfNative.lfHeight = lf.lfHeight;
            if (!(hfontNative=CreateFontIndirect(&lfNative)))
            {
                hfontNative = hfont;
            }

            // save it away so we can delete it later
            if (hfontNative != hfont)
                m_hfontDelete = hfont;
        
            // set this variable to avoid calling createfont twice
            // if we get called again.
            m_hfontNative = hfontNative;
        }
    }

    return hres = (m_hfontNative == m_hfontOrg ? S_FALSE : S_OK);
}

//
// _GetFontAssocStatus
//
// synopsis: check to see if the platform has "Font Association"
//           enabled or 'Font Link' capability
//
HRESULT THISCLASS::_GetFontAssocStatus(FASTATUS  *puiAssoced)
{
    HRESULT hr = S_OK;
    ASSERT(puiAssoced);
    
    // I assume the setting won't change without rebooting
    // the system
    //
    if (FAS_NOTINITIALIZED == _s_uiFontAssocStatus)
    {
        if (g_bRunOnNT5)
        {
            // NT5 has fontlink functionality
            _s_uiFontAssocStatus = FAS_ENABLED;
        }
        else
        {
            HKEY hkey;
            TCHAR szYesOrNo[16] = {0};
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                              s_szRegFASettings,
                             0, KEY_READ, &hkey) == ERROR_SUCCESS) 
            {
                DWORD dwSize = sizeof(szYesOrNo);
                RegQueryValueEx(hkey, TEXT("ANSI(00)"), 0, NULL, (LPBYTE)szYesOrNo, &dwSize);
                RegCloseKey(hkey);
            }
            else
            {
                // this only indicates the reg func failed
                // we can't always assume the key is there (Western)
                //
                hr = S_FALSE; 
            }
        
            if (SUCCEEDED(hr) && !lstrcmpi(szYesOrNo, TEXT("yes")))
            {
                // font assoc is enabled
                _s_uiFontAssocStatus = FAS_ENABLED;
            }
            else
                _s_uiFontAssocStatus = FAS_DISABLED;
        }
    }
    *puiAssoced = _s_uiFontAssocStatus;

    return hr;
}

extern "C" {
    
BOOL InitNativeFontCtl(HINSTANCE hinst)
{
    WNDCLASS wc;

    if (!GetClassInfo(hinst, WC_NATIVEFONTCTL, &wc)) {
        wc.lpfnWndProc     = THISCLASS::NativeFontWndProc;
        wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon           = NULL;
        wc.lpszMenuName    = NULL;
        wc.hInstance       = hinst;
        wc.lpszClassName   = WC_NATIVEFONTCTL;
        wc.hbrBackground   = (HBRUSH)(COLOR_BTNFACE + 1); // NULL;
        wc.style           = CS_GLOBALCLASS;
        wc.cbWndExtra      = sizeof(LPVOID);
        wc.cbClsExtra      = 0;

        return RegisterClass(&wc);
    }
    return TRUE;
}

};
