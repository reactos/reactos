// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  SDM.H
//
//  This knows how to talk to SDM 16 and 32.
//
//  NOTE:  We don't expose child controls as independent objects.  It's too
//  much work, for not much purpose.  But this means you can't get at listbox
//  items individually.  Better than nothing for 1.0 though.
//
// --------------------------------------------------------------------------


class CSdm32 : public CClient
{
    public:
        // IAccessible
        virtual STDMETHODIMP    get_accChild(VARIANT, IDispatch**);
        virtual STDMETHODIMP    get_accName(VARIANT, BSTR*);
        virtual STDMETHODIMP    get_accValue(VARIANT, BSTR*);
        virtual STDMETHODIMP    get_accRole(VARIANT, VARIANT*);
        virtual STDMETHODIMP    get_accState(VARIANT, VARIANT*);
        virtual STDMETHODIMP    get_accKeyboardShortcut(VARIANT, BSTR*);
        virtual STDMETHODIMP    get_accFocus(VARIANT*);

        virtual STDMETHODIMP    accSelect(long, VARIANT);
        virtual STDMETHODIMP    accLocation(long*, long*, long*, long*, VARIANT);
        virtual STDMETHODIMP    accNavigate(long, VARIANT, VARIANT*);
        virtual STDMETHODIMP    accHitTest(long, long, VARIANT*);

        CSdm32(HWND, long);
        void    SetupChildren(void);
        BOOL    GetSdmControl(long index, LPWCTL32 lpwc, long* plFirstRadio);
        BOOL    GetSdmControlName(long index, LPTSTR* ppszName);
		long	GetSdmChildIdFromSdmId (long lSdmId);

    protected:
        BOOL    m_f16Bits;          // 16-bit control
        UINT    m_cbWctlSize;       // Size of Wctl struct
        UINT    m_cbWtxiSize;       // Size of Wtxi struct
};


class CSdmList : public CAccessible
{
    public:
        // IAccessible
        virtual STDMETHODIMP    get_accName(VARIANT, BSTR*);
        virtual STDMETHODIMP    get_accValue(VARIANT, BSTR*);
        virtual STDMETHODIMP    get_accRole(VARIANT, VARIANT*);
        virtual STDMETHODIMP    get_accState(VARIANT, VARIANT*);
        virtual STDMETHODIMP    get_accKeyboardShortcut(VARIANT, BSTR*);
        virtual STDMETHODIMP    get_accFocus(VARIANT*);

        virtual STDMETHODIMP    accSelect(long, VARIANT);
        virtual STDMETHODIMP    accLocation(long*, long*, long*, long*, VARIANT);
        virtual STDMETHODIMP    accNavigate(long, VARIANT, VARIANT*);
        virtual STDMETHODIMP    accHitTest(long, long, VARIANT*);

        // IEnumVARIANT
        virtual STDMETHODIMP    Clone(IEnumVARIANT** ppenum);
        void                    SetupChildren(void);
        BOOL                    GetListItemName(long index, LPTSTR lpszName, UINT cchMax);

        CSdmList(CSdm32* psdmParent, HWND hwnd, long idSdm, long idAccess, BOOL f16bits,
            BOOL fPageTab, UINT cbWtx, long idChildCur, HWND hwndList);
        ~CSdmList();

    protected:
        BOOL    m_fPageTabs;
        BOOL    m_f16Bits;
        BOOL    m_cbWtxi;
        CSdm32* m_psdmParent;
        long    m_idSdm;
        long    m_idAccess;
		HWND	m_hwndList;
};


HRESULT     CreateSdmList(CSdm32*, HWND hwnd, long, long, BOOL, BOOL, UINT, long, HWND, REFIID, void**);

LPVOID      MyMapLS(LPVOID);
VOID        MyUnMapLS(LPVOID);
