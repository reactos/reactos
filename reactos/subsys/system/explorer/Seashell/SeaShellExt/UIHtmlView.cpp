//*******************************************************************************
// COPYRIGHT NOTES
// ---------------
// You may use this source code, compile or redistribute it as part of your application 
// for free. You cannot redistribute it as a part of a software development 
// library without the agreement of the author. If the sources are 
// distributed along with the application, you should leave the original 
// copyright notes in the source code without any changes.
// This code can be used WITHOUT ANY WARRANTIES at your own risk.
// 
// For the latest updates to this code, check this site:
// http://www.masmex.com 
// after Sept 2000
// 
// Copyright(C) 2000 Philip Oldaker <email: philip@masmex.com>
//*******************************************************************************

// HtmlMsgView.cpp : implementation file
//

#include "stdafx.h"
#include "UIHtmlView.h"
#include "UIMessages.h"
#include <atlbase.h>
#include <mshtml.h>
#include "UIres.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUIHtmlView

IMPLEMENT_DYNAMIC(CUIHtmlView, CHtmlView)

CUIHtmlView::CUIHtmlView()
{
	//{{AFX_DATA_INIT(CUIHtmlView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pHTMLDocument2 = NULL;
	m_hNotifyWnd = NULL;
	m_bSetCursor = false;
}

CUIHtmlView::~CUIHtmlView()
{
	ReleaseDocument();
}

BOOL CUIHtmlView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	cs.lpszClass = AfxRegisterWndClass(
				  CS_DBLCLKS,                       
				  NULL,                             
				  NULL,                             
				  NULL); 
	ASSERT(cs.lpszClass);
	BOOL bRet = CHtmlView::PreCreateWindow(cs);
	cs.dwExStyle |= WS_EX_CLIENTEDGE;
//	cs.style |= WS_BORDER;
	return bRet;	
}

void CUIHtmlView::ReleaseDocument()
{
	if (m_pHTMLDocument2)
	{
		m_pHTMLDocument2->Release();
		m_pHTMLDocument2 = NULL;
	}
}

void CUIHtmlView::DocumentReady()
{
	m_bSetCursor = false;
}

bool CUIHtmlView::ExecScript(LPCTSTR pszScript,LPCTSTR pszLang,_variant_t *pvt)
{
	bool bRet = false;
	CWaitCursor w;
	IHTMLWindow2 *pW2=NULL;
	IHTMLDocument2 *pDoc = GetHTMLDocument();
	if (pDoc == NULL)
		return bRet;
	HRESULT hr = pDoc->get_parentWindow(&pW2);
	if (SUCCEEDED(hr))
	{
		if (pszLang == NULL)
			pszLang = _T("JScript");
		_variant_t v;
		hr = pW2->execScript(_bstr_t(pszScript),_bstr_t(pszLang),&v);
		if (pvt)
			*pvt = v;
		pW2->Release();
		bRet= true;
	}
	return bRet;
}

CString CUIHtmlView::GetBodyText()
{
	IHTMLElement *pElem=NULL;
	GetHTMLDocument()->get_body(&pElem);
	_bstr_t bstText;
	BSTR bsText;
	pElem->get_innerText(&bsText);
	pElem->Release();
	bstText = bsText;
	return (LPCTSTR)bstText;
}

CString CUIHtmlView::GetElementValue(LPCTSTR pszElemID)
{
	IHTMLElement *pElem=NULL;
	GetElement(pszElemID,&pElem);
	BSTR bsText;
	_bstr_t bstText;
	if (pElem)
	{
		IHTMLInputTextElement *pInputElem=NULL;
		HRESULT	hr = pElem->QueryInterface(IID_IHTMLInputTextElement,(LPVOID*)&pInputElem);
		if (SUCCEEDED(hr))
		{
			pInputElem->get_value(&bsText);
			bstText= bsText;
			pInputElem->Release();
			pInputElem = NULL;
		}
		pElem->Release();
	}
	return (LPCTSTR)bstText;
}

CString CUIHtmlView::GetElementText(LPCTSTR pszElemID)
{
	IHTMLElement *pElem=NULL;
	GetElement(pszElemID,&pElem);
	if (pElem == NULL)
		return _T("");
	BSTR bsText;
	_bstr_t bstText;
	pElem->get_innerText(&bsText);
	bstText = bsText;
	pElem->Release();
	pElem = NULL;
	return (LPCTSTR)bstText;
}

CString CUIHtmlView::GetElementHTML(LPCTSTR pszElemID)
{
	IHTMLElement *pElem=NULL;
	GetElement(pszElemID,&pElem);
	BSTR bsText;
	_bstr_t bstText;
	if (pElem)
	{
		pElem->get_innerHTML(&bsText);
		bstText = bsText;
		pElem->Release();
		pElem = NULL;
	}
	return (LPCTSTR)bstText;
}

bool CUIHtmlView::SetElementValue(LPCTSTR pszElemID,LPCTSTR pszText)
{
	bool bRet=false;
	IHTMLElement *pElem=NULL;
	GetElement(pszElemID,&pElem);
	if (pElem)
	{
		IHTMLInputTextElement *pInputElem=NULL;
		HRESULT	hr = pElem->QueryInterface(IID_IHTMLInputTextElement,(LPVOID*)&pInputElem);
		if (SUCCEEDED(hr))
		{
			pInputElem->put_value(_bstr_t(pszText));
			pInputElem->Release();
			pInputElem = NULL;
			bRet = true;
		}
		pElem->Release();
		pElem = NULL;
	}
	return bRet;
}

bool CUIHtmlView::SetElementText(LPCTSTR pszElemID,LPCTSTR pszText)
{
	bool bRet=false;
	IHTMLElement *pElem=NULL;
	GetElement(pszElemID,&pElem);
	if (pElem == NULL)
		return bRet;
	pElem->put_innerText(_bstr_t(pszText));
	pElem->Release();
	pElem = NULL;
	bRet= true;
	return bRet;
}

bool CUIHtmlView::SetElementHTML(LPCTSTR pszElemID,LPCTSTR pszText)
{
	bool bRet=false;
	IHTMLElement *pElem=NULL;
	GetElement(pszElemID,&pElem);
	if (pElem == NULL)
		return bRet;
	pElem->put_innerHTML(_bstr_t(pszText));
	pElem->Release();
	pElem = NULL;
	bRet= true;
	return bRet;
}

bool CUIHtmlView::SetImageSource(LPCTSTR pszElemID,LPCTSTR pszText)
{
	bool bRet=false;
	IHTMLElement *pElem=NULL;
	GetElement(pszElemID,&pElem);
	if (pElem == NULL)
		return bRet;
	IHTMLImgElement *pImgElem=NULL;
	HRESULT	hr = pElem->QueryInterface(IID_IHTMLImgElement,(LPVOID*)&pImgElem);
	pElem->Release();
	if (SUCCEEDED(hr))
	{
		pImgElem->put_src(_bstr_t(pszText));
		pImgElem->Release();
		bRet = true;
	}
	return bRet;
}

bool CUIHtmlView::GetOptionString(LPCTSTR pszElemID,CString &sText,CString &sValue)
{
	IHTMLElement *pElement = NULL;
	GetElement(pszElemID,&pElement);
	bool bRet=false;
	if (pElement == NULL)
		return bRet;
	IHTMLSelectElement *pSelElem=NULL;
	HRESULT hr = pElement->QueryInterface(IID_IHTMLSelectElement,(LPVOID*)&pSelElem);
	pElement->Release();
	pElement = NULL;
	if (FAILED(hr))
		return bRet;
	long nSelIndex=-1;
	pSelElem->get_selectedIndex(&nSelIndex);
	if (nSelIndex == -1)
	{
		pSelElem->Release();
		return bRet;
	}
	IDispatch *pDisp=NULL;
	_variant_t vtName(nSelIndex);
	_variant_t vtIndex;
	pSelElem->item(vtName,vtIndex,&pDisp);
	IHTMLOptionElement *pOptElem=NULL;
	hr = pDisp->QueryInterface(IID_IHTMLOptionElement,(LPVOID*)&pOptElem);
	if (SUCCEEDED(hr))
	{
		_bstr_t bstValue;
		BSTR bsValue;
		pOptElem->get_value(&bsValue);
		bstValue = bsValue;
		sValue = (LPCTSTR)bstValue;
		BSTR bsText;
		_bstr_t bstText;
		pOptElem->get_text(&bsText);
		bstText = bsText;
		sText = (LPCTSTR)bstText;
		pOptElem->Release();
		bRet=true;
	}
	if (pSelElem)
		pSelElem->Release();
	return bRet;
}

bool CUIHtmlView::SetOptionString(LPCTSTR pszElemID,LPCTSTR pszText)
{
	IHTMLElement *pElement = NULL;
	GetElement(pszElemID,&pElement);
	bool bRet=false;
	if (pElement == NULL)
		return bRet;
	IHTMLSelectElement *pSelElem=NULL;
	IDispatch *pDisp=NULL;
	HRESULT hr = pElement->QueryInterface(IID_IHTMLSelectElement,(LPVOID*)&pSelElem);
	if (FAILED(hr))
		goto SOS_CleanUp;
	{
		long nLength=0;
		pSelElem->get_length(&nLength);
		for(long i=0;i < nLength;i++)
		{
			_variant_t vtName(i);
			_variant_t vtIndex;
			pSelElem->item(vtName,vtIndex,&pDisp);
			if (pDisp == NULL)
				continue;
			IHTMLOptionElement *pOptElem=NULL;
			hr = pDisp->QueryInterface(IID_IHTMLOptionElement,(LPVOID*)&pOptElem);
			pDisp->Release();
			pDisp = NULL;
			if (SUCCEEDED(hr))
			{
				_bstr_t bstValue;
				_bstr_t bstText;
				BSTR bsValue;
				BSTR bsText;
				pOptElem->get_value(&bsValue);
				pOptElem->get_text(&bsText);
				bstValue = bsValue;
				bstText = bsText;
				pOptElem->Release();
				if (_tcsicmp((LPCTSTR)bstText,pszText) == 0)
				{
					pSelElem->put_selectedIndex(i);
					bRet=true;
					break;
				}
			}
		}
	}
SOS_CleanUp:
	if (pElement)
		pElement->Release();
	if (pSelElem)
		pSelElem->Release();
	return bRet;
}

bool CUIHtmlView::AddOptionString(LPCTSTR pszElemID,LPCTSTR pszText,LPCTSTR pszValue,bool bSelect)
{
	IHTMLElement *pElement = NULL;
	GetElement(pszElemID,&pElement);
	bool bRet=false;
	if (pElement == NULL)
		return bRet;
	IHTMLSelectElement *pSelElem=NULL;
	HRESULT hr = pElement->QueryInterface(IID_IHTMLSelectElement,(LPVOID*)&pSelElem);
	pElement->Release();
	if (FAILED(hr))
		return bRet;
	IHTMLElement *pNewElem=NULL;
	GetHTMLDocument()->createElement(_bstr_t(_T("OPTION")),&pNewElem);
	IHTMLOptionElement *pNewOptElem=NULL;
	hr = E_FAIL;
	if (pNewElem)
		hr = pNewElem->QueryInterface(IID_IHTMLOptionElement,(LPVOID*)&pNewOptElem);
	if (SUCCEEDED(hr))
	{
		if (pszValue)
		{
			_bstr_t bsValue(pszValue);
			pNewOptElem->put_value(bsValue);
		}
		_bstr_t bsText(pszText);
		pNewOptElem->put_text(bsText);
		pSelElem->add(pNewElem,_variant_t(vtMissing));
		pNewOptElem->Release();
		if (bSelect)
		{
			long nLength=0;
			pSelElem->get_length(&nLength);
			if (nLength > 0)
				pSelElem->put_selectedIndex(nLength-1);
		}
		bRet=true;
	}
	if (pSelElem)
		pSelElem->Release();
	return bRet;
}

void CUIHtmlView::GetElement(LPCTSTR pszID,IHTMLElement **pElement)
{
	*pElement=NULL;
	if (m_pHTMLDocument2 == NULL)
		return;
	CComQIPtr<IHTMLElementCollection> spAllElements;	
	m_pHTMLDocument2->get_all(&spAllElements);
	if (spAllElements)
	{
		IDispatch *pDisp=NULL;
		HRESULT hr = spAllElements->item(CComVariant(pszID),CComVariant(0),&pDisp);
		if (SUCCEEDED(hr))
		{
			hr = pDisp->QueryInterface(IID_IHTMLElement,(LPVOID*)pElement);
		}
		pDisp->Release();
	}
}

void CUIHtmlView::ParseDocument()  
{
	if(m_pHTMLDocument2 == NULL)
		return;
	try
	{
		CComQIPtr<IHTMLDocument2> spDocument(m_pHTMLDocument2);
		CComQIPtr<IHTMLElementCollection> spAllElements;	
		spDocument->get_all(&spAllElements);
 		CComBSTR bsIsControl(_T("OBJECT"));
		CComBSTR bsIsImage(_T("IMG"));
		long nElems;
		spAllElements->get_length(&nElems);
		for(long i = 0; i < nElems; i++)
		{
			CComVariant vIndex(i, VT_I4);
			LPDISPATCH pDisp=NULL;
			spAllElements->item(vIndex,vIndex,&pDisp);
			CComQIPtr<IHTMLElement> spAnElement(pDisp);
			pDisp->Release();
			CComBSTR bsTagName;
			spAnElement->get_tagName(&bsTagName);
			if(bsTagName == bsIsControl)
			{
				// This will get you any ActiveX controls in a page.  It is possible 
				// to call methods and properties of the  control off the IHTMLElementPtr.
				CComQIPtr<IHTMLObjectElement> spObj(spAnElement);
				ActiveXControl(spObj);
			}
			else if(bsTagName == bsIsImage)
			{
				CComQIPtr<IHTMLImgElement> spImg(spAnElement);
				ImageElement(spImg);
			}
			else
				Element(spAnElement);
		}
	}
	catch(...)
	{
#ifdef _DEBUG
		AfxMessageBox(_T("Unspecified exception thrown in UIHtmlView"),MB_ICONSTOP);
#endif 
		throw;
	}
}

void CUIHtmlView::ActiveXControl(IHTMLObjectElement *pObj)
{
}

void CUIHtmlView::ImageElement(IHTMLImgElement *pImg)
{
}

void CUIHtmlView::Element(IHTMLElement *pElement)
{
}

void CUIHtmlView::DoDataExchange(CDataExchange* pDX)
{
	CHtmlView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUIHtmlView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUIHtmlView, CHtmlView)
	//{{AFX_MSG_MAP(CUIHtmlView)
	ON_WM_SETCURSOR()
	ON_COMMAND(ID_BROWSER_GO_BACK, OnBrowserGoBack)
	ON_COMMAND(ID_BROWSER_GO_FORWARD, OnBrowserGoForward)
	ON_COMMAND(ID_BROWSER_GO_BACK, OnBrowserGoBack)
	ON_UPDATE_COMMAND_UI(ID_BROWSER_GO_FORWARD, OnUpdateBrowserGoForward)
	ON_UPDATE_COMMAND_UI(ID_BROWSER_GO_BACK, OnUpdateBrowserGoBack)
	ON_COMMAND(ID_BROWSER_GO_HOME, OnBrowserGoHome)
	ON_COMMAND(ID_BROWSER_REFRESH, OnBrowserRefresh)
	ON_COMMAND(ID_BROWSER_STOP, OnBrowserStop)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUIHtmlView diagnostics

#ifdef _DEBUG
void CUIHtmlView::AssertValid() const
{
	CHtmlView::AssertValid();
}

void CUIHtmlView::Dump(CDumpContext& dc) const
{
	CHtmlView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CUIHtmlView message handlers

void CUIHtmlView::DocumentComplete(LPDISPATCH pDisp, VARIANT* URL)
{
	// TODO: Add your specialized code here and/or call the base class
	
   HRESULT   hr;
   LPUNKNOWN lpUnknown;
   LPUNKNOWN lpUnknownWB = NULL;
   LPUNKNOWN lpUnknownDC = NULL;

   lpUnknown = m_wndBrowser.GetControlUnknown();
   ASSERT(lpUnknown);

   if (lpUnknown)
   {
      // Get the IUnknown of the WebBrowser control being hosted.
      // The IUnknown returned from GetControlUnknown is not the 
      // IUnknown of the WebBrowser control.  It's actually a
      // IOleObject pointer.
      // 
      hr = lpUnknown->QueryInterface(IID_IUnknown, 
                             (LPVOID*)&lpUnknownWB);

      ASSERT(SUCCEEDED(hr));

      if (FAILED(hr))
         return;

      // Get the IUnknown of the object that fired this event.
      //
      hr = pDisp->QueryInterface(IID_IUnknown, (LPVOID*)&lpUnknownDC);


      ASSERT(SUCCEEDED(hr));

      if (SUCCEEDED(hr) && lpUnknownWB == lpUnknownDC)
      {
         // The document has finished loading.
         //
		  LPDISPATCH pDispatch = GetHtmlDocument();
		  if (pDispatch)
		  {
				hr = pDispatch->QueryInterface(IID_IHTMLDocument2,(LPVOID*)&m_pHTMLDocument2);
				DocumentReady();
				pDispatch->Release();
		  }
      }
      if (lpUnknownWB)
         lpUnknownWB->Release();

      if (lpUnknownDC)
         lpUnknownDC->Release();
   }
}

void CUIHtmlView::OnBeforeNavigate2(LPCTSTR lpszURL, DWORD nFlags, LPCTSTR lpszTargetFrameName, CByteArray& baPostedData, LPCTSTR lpszHeaders, BOOL* pbCancel) 
{
	// TODO: Add your specialized code here and/or call the base class	ReleaseDocument();
	m_bSetCursor = true;
#ifdef _DEBUG
	if (GetKeyState(VK_LCONTROL) < 0)
	{
		if (baPostedData.GetSize() > 0)
		{
			LPTSTR pszData = new TCHAR[baPostedData.GetSize()+1];
			LPTSTR pszPosted = pszData;
			for(int i=0;i < baPostedData.GetSize();i++)
			{
				*pszPosted = baPostedData[i];
				pszPosted =_tcsinc(pszPosted);
			}
			*pszPosted = '\0';
			CString sMess;
			sMess = _T("Posted Data: ");
			sMess += pszData;
			sMess += _T("\n");
			sMess += _T("URL: ");
			sMess += lpszURL;
			AfxMessageBox(sMess);
			delete pszData;
		}
	}
#endif
	ReleaseDocument();
	if (m_hNotifyWnd)
	{
		::SendMessage(m_hNotifyWnd,WM_APP_CB_IE_SET_EDIT_TEXT,(WPARAM)lpszURL,0);	
	}
	CHtmlView::OnBeforeNavigate2(lpszURL, nFlags,	lpszTargetFrameName, baPostedData, lpszHeaders, pbCancel);
}

BOOL CUIHtmlView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	// TODO: Add your message handler code here and/or call default
	if (m_bSetCursor)
	{
		SetCursor(::LoadCursor(NULL,IDC_APPSTARTING));
		return TRUE;
	}
	
	return CHtmlView::OnSetCursor(pWnd, nHitTest, message);
}

void CUIHtmlView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// TODO: Add your specialized code here and/or call the base class
	
}

void CUIHtmlView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	// TODO: Add your specialized code here and/or call the base class	
	CHtmlView::OnActivateView(bActivate, pActivateView, pDeactiveView);
	if (bActivate && pDeactiveView != NULL) 
	{
		OnTitleChange(GetLocationURL());
		SetActiveWindow();
	}
}

void CUIHtmlView::OnCommandStateChange(long nCommand, BOOL bEnable) 
{
	// TODO: Add your specialized code here and/or call the base class
	switch(nCommand) 
	{
       case CSC_NAVIGATEFORWARD: 
		   m_bGoForward = bEnable;
           break;
       case CSC_NAVIGATEBACK:
           m_bGoBack = bEnable;
           break;       
	   default:
           break;
	}  
	
	CHtmlView::OnCommandStateChange(nCommand, bEnable);
}

void CUIHtmlView::OnUpdateBrowserGoBack(CCmdUI *pUI)
{
	pUI->Enable(m_bGoBack);
}

void CUIHtmlView::OnUpdateBrowserGoForward(CCmdUI *pUI)
{
	pUI->Enable(m_bGoForward);
}

void CUIHtmlView::OnBrowserGoBack() 
{
	// TODO: Add your command handler code here
	GoBack();
}

void CUIHtmlView::OnBrowserGoForward() 
{
	// TODO: Add your command handler code here
	GoForward();	
}

void CUIHtmlView::OnBrowserGoHome() 
{
	// TODO: Add your command handler code here
	GoHome();	
}

void CUIHtmlView::OnBrowserRefresh() 
{
	// TODO: Add your command handler code here
	Refresh();	
}

void CUIHtmlView::OnBrowserStop() 
{
	// TODO: Add your command handler code here
	Stop();	
}

void CUIHtmlView::OnDocumentComplete(LPCTSTR lpszUrl)
{
	// make sure the main frame has the new URL.  This call also stops the animation
	if (m_hNotifyWnd)
	{
		::SendMessage(m_hNotifyWnd,WM_APP_CB_IE_SET_EDIT_TEXT,(WPARAM)lpszUrl,0);	
	}
	CHtmlView::OnDocumentComplete(lpszUrl);
}

void CUIHtmlView::OnTitleChange(LPCTSTR lpszText)
{
	// this will change the main frame's title bar
	if (m_pDocument != NULL)
		m_pDocument->SetTitle(lpszText);
}
