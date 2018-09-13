// IeLogppg.h : Declaration of the CIeLogppg

#ifndef __IELOGPPG_H_
#define __IELOGPPG_H_

#include "resource.h"       // main symbols
#include "ImgLog.h"

EXTERN_C const CLSID CLSID_IeLogppg;

/////////////////////////////////////////////////////////////////////////////
// CIeLogppg
class ATL_NO_VTABLE CIeLogppg :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIeLogppg, &CLSID_IeLogppg>,
	public IPropertyPageImpl<CIeLogppg>,
	public CDialogImpl<CIeLogppg>
{
public:
	CIeLogppg() 
	{
		m_dwTitleID = IDS_TITLEIeLogppg;
		m_dwHelpFileID = IDS_HELPFILEIeLogppg;
		m_dwDocStringID = IDS_DOCSTRINGIeLogppg;
	}

	enum {IDD = IDD_IELOGPPG};

DECLARE_REGISTRY_RESOURCEID(IDR_IELOGPPG)

BEGIN_COM_MAP(CIeLogppg) 
	COM_INTERFACE_ENTRY_IMPL(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CIeLogppg)
    COMMAND_HANDLER(IDC_IMAGES, EN_CHANGE, OnImagesChange)
    COMMAND_HANDLER(IDC_TOPURL, EN_CHANGE, OnImagesChange)
	CHAIN_MSG_MAP(IPropertyPageImpl<CIeLogppg>)
END_MSG_MAP()

	STDMETHOD(Apply)(void)
	{
        USES_CONVERSION;
        HRESULT hr = E_FAIL;
		ATLTRACE(_T("CIeLogppg::Apply\n"));
		for (UINT i = 0; i < m_nObjects; i++)
		{
            CComQIPtr<IIeLogControl, &IID_IIeLogControl>pIeLog(m_ppUnk[i]);
            short   nImages = (short)GetDlgItemInt(IDC_IMAGES);
            if (nImages > 0)
                pIeLog->put_images(nImages);

            LPSTR   pURL = NULL;
            pURL = new char[MAX_PATH];
            if (pURL && GetDlgItemText(IDC_TOPURL, pURL, MAX_PATH))
            {
                BSTR bstrURL = SysAllocString((OLECHAR *)pURL);
                if FAILED(pIeLog->put_URL(bstrURL))
                {
                    CComPtr<IErrorInfo> pErr;
                    CComBSTR            strErr;
                    GetErrorInfo(0, &pErr);
                    pErr->GetDescription(&strErr);
                    MessageBox(OLE2T(strErr), _T("Error"), MB_ICONEXCLAMATION);
                }
                else
                    hr = S_OK;

                delete [] pURL;
                SysFreeString(bstrURL);
            }
         
            if FAILED(hr)
                return hr;
		}
		m_bDirty = FALSE;
		return S_OK;
	}

    LRESULT OnImagesChange(WORD wNotify, WORD wID, HWND hWnd, BOOL& bHandled)
    {
        SetDirty(TRUE);
        return 0;
    }

    void SpitError()
    {
        USES_CONVERSION;
        CComPtr<IErrorInfo> pErr;
        CComBSTR            strErr;
        GetErrorInfo(0, &pErr);
        pErr->GetDescription(&strErr);
        MessageBox(OLE2T(strErr), _T("Error"), MB_ICONEXCLAMATION);
        return;
    }
};

#endif //__IELOGPPG_H_
