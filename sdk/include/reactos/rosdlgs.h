#pragma once

#include <atlwin.h>

namespace ATL
{
    template <class T>
    class CPropertyPageImpl: public CDialogImplBaseT<CWindow>
    {
    public:
        PROPSHEETPAGE m_psp;

        operator PROPSHEETPAGE*()
        {
            return &m_psp;
        }

        CPropertyPageImpl(LPCTSTR lpszTitle = NULL)
        {
            T* pT = static_cast<T*>(this);

            memset(&m_psp, 0, sizeof(m_psp));
            m_psp.dwSize = sizeof(m_psp);
            m_psp.hInstance = _AtlBaseModule.GetResourceInstance();
            m_psp.pszTemplate = MAKEINTRESOURCE(T::IDD);
            m_psp.pszTitle = lpszTitle;
            m_psp.pfnDlgProc = T::StartDialogProc;
            m_psp.pfnCallback = T::PropPageCallback;
            m_psp.lParam = reinterpret_cast<LPARAM>(pT);

            m_psp.dwFlags = PSP_USECALLBACK;
            if (lpszTitle)
                m_psp.dwFlags |= PSP_USETITLE;
        }

        static UINT CALLBACK PropPageCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
        {
            T* pT = reinterpret_cast<T*>(ppsp->lParam);
            CDialogImplBaseT<CWindow>* pThis = static_cast<CDialogImplBaseT<CWindow>*>(pT);

            switch (uMsg)
            {
                case PSPCB_CREATE:
                    _AtlWinModule.AddCreateWndData(&pThis->m_thunk.cd, pThis);
                    return pT->OnPageCreate();
                case PSPCB_ADDREF:
                    pT->OnPageAddRef();
                    break;
                case PSPCB_RELEASE:
                    pT->OnPageRelease();
                    break;
            }

            return 0;
        }

        HPROPSHEETPAGE Create()
        {
            return ::CreatePropertySheetPage(&m_psp);
        }

        BOOL EndDialog(_In_ int nRetCode)
        {
            return FALSE;
        }

        void SetModified(BOOL bChanged = TRUE)
        {
            ::SendMessage(GetParent(), bChanged ? PSM_CHANGED : PSM_UNCHANGED, (WPARAM)m_hWnd, 0L);
        }

        void SetWizardButtons(DWORD dwFlags)
        {
            ::PostMessage(GetParent(), PSM_SETWIZBUTTONS, 0, (LPARAM)dwFlags);
        }

        BOOL OnPageCreate()
        {
            return TRUE;
        }

        VOID OnPageAddRef()
        {
        }

        VOID OnPageRelease()
        {
        }

        int OnSetActive()
        {
            return 0;
        }

        BOOL OnKillActive()
        {
            return FALSE;
        }

        int OnApply()
        {
            return PSNRET_NOERROR;
        }

        void OnReset()
        {
        }

        void OnHelp()
        {
        }

        int OnWizardBack()
        {
            return 0;
        }

        int OnWizardNext()
        {
            return 0;
        }

        BOOL OnWizardFinish()
        {
            return FALSE;
        }

        BOOL OnQueryCancel()
        {
            return FALSE;
        }

        BOOL OnGetObject(LPNMOBJECTNOTIFY lpObjectNotify)
        {
            return FALSE;
        }

        int OnTranslateAccelerator(LPMSG lpMsg)
        {
            return 0;
        }

        HWND OnQueryInitialFocus(HWND hwnd)
        {
            return NULL;
        }

        // message map and handlers
        typedef CPropertyPageImpl<T> thisClass;
        BEGIN_MSG_MAP(thisClass)
            MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        END_MSG_MAP()

        LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
        {
            NMHDR* pnmh = (NMHDR*)lParam;
            T* pThis = static_cast<T*>(this);

            if (pnmh->hwndFrom != GetParent() && pnmh->hwndFrom != m_hWnd)
            {
                bHandled = FALSE;
                return TRUE;
            }

            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    return pThis->OnSetActive();
                case PSN_KILLACTIVE:
                    return pThis->OnKillActive();
                case PSN_APPLY:
                    return pThis->OnApply();
                case PSN_RESET:
                    pThis->OnReset();
                    return 0;
                case PSN_HELP:
                    pThis->OnHelp();
                    return 0;
                case PSN_WIZBACK:
                    return pThis->OnWizardBack();
                case PSN_WIZNEXT:
                    return pThis->OnWizardNext();
                case PSN_WIZFINISH:
                    return pThis->OnWizardFinish();
                case PSN_QUERYCANCEL:
                    return pThis->OnQueryCancel();
                case PSN_GETOBJECT:
                    return pThis->OnGetObject((LPNMOBJECTNOTIFY)lParam);
                case PSN_TRANSLATEACCELERATOR:
                    return pThis->OnTranslateAccelerator((LPMSG)lParam);
                case PSN_QUERYINITIALFOCUS:
                    return (LRESULT)pThis->OnQueryInitialFocus((HWND)lParam);
            }

            bHandled = FALSE;
            return 0;
        }
    };
}
