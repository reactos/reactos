/*
 * Copyright 2004 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


 //
 // Explorer clone
 //
 // webchild.cpp
 //
 // Martin Fuchs, 08.02.2004
 //


#include "../utility/utility.h"
#include "../explorer.h"

#include "../explorer_intres.h"

#include "webchild.h"

//#include <mshtml.h>


Variant::Variant(const VARIANT& var)
{
	VariantInit(this);
	CheckError(VariantCopy(this, const_cast<VARIANT*>(&var)));
}

Variant::Variant(const VARIANT* var)
{
	VariantInit(this);
	CheckError(VariantCopy(this, const_cast<VARIANT*>(var)));
}

Variant::~Variant()
{
	VariantClear(this);
}


Variant::operator long() const
{
	Variant v;
	CheckError(VariantChangeType(&v, (VARIANT*)this, 0, VT_I4));
	return V_I4(&v);
}

Variant::operator bool() const
{
	Variant v;
	CheckError(VariantChangeType(&v, (VARIANT*)this, 0, VT_BOOL));
	return V_BOOL(&v)? true: false;
}

Variant::operator IDispatch*() const
{
	Variant v;
	CheckError(VariantChangeType(&v, (VARIANT*)this, 0, VT_DISPATCH));
	return V_DISPATCH(&v);
}

Variant::operator VARIANT_BOOL() const
{
	Variant v;
	CheckError(VariantChangeType(&v, (VARIANT*)this, 0, VT_BOOL));
	return V_BOOL(&v);
}


void BStr::assign(BSTR s)
{
	if (!SysReAllocString(&_p, s))
		THROW_EXCEPTION(E_OUTOFMEMORY);
}

void BStr::assign(const VARIANT& var)
{
	if (V_VT(&var) == VT_BSTR)
		assign(V_BSTR(&var));
	else {
		Variant v;
		CheckError(VariantChangeType(&v, const_cast<VARIANT*>(&var), 0, VT_BSTR));
		assign(V_BSTR(&v));
	}
}


BrowserNavigator::BrowserNavigator()
 :	_browser_initialized(false)
{
}

void BrowserNavigator::attach(IWebBrowser* browser)
{
	_browser = browser;
}

void BrowserNavigator::goto_url(LPCTSTR url)
{
	if (_browser_initialized)
		_browser->Navigate(BStr(url), NULL, NULL, NULL, NULL);
	else {
		_new_url = url;

		_browser->Navigate(L"about:blank", NULL, NULL, NULL, NULL);
	}
}

void BrowserNavigator::set_html_page(const String& html_txt)
{
	_new_html_txt = html_txt;

	goto_url(TEXT("about:blank"));
}

void T2nA_binary(LPCTSTR s, LPSTR d, int len)
{
	while(len-- > 0)
		*d++ = (unsigned char)*s++;
}

void BrowserNavigator::navigated(LPCTSTR url)
{
	_browser_initialized = true;

	bool nav = false;

	if (!_new_url.empty()) {
		if (!_tcscmp(url,TEXT("about:blank")) && _new_url!=TEXT("about:blank")) {
			_browser->Navigate(BStr(_new_url), NULL, NULL, NULL, NULL);
			++nav;
		}

		_new_url.erase();
	}

	if (!nav && !_new_html_txt.empty()) {	///@todo move this into DocumentComplete() ?
		int len = _new_html_txt.length();
		HGLOBAL hHtmlText = GlobalAlloc(GPTR, len);

		if (!hHtmlText) {
			T2nA_binary(_new_html_txt, (char*)hHtmlText, len);
			_new_html_txt.erase();

			SIfacePtr<IStream> pStream;
			HRESULT hr = CreateStreamOnHGlobal(hHtmlText, TRUE, &pStream);

			if (SUCCEEDED(hr)) {
				SIfacePtr<IDispatch> pHtmlDoc;
				CheckError(_browser->get_Document(&pHtmlDoc));

				SIfacePtr<IPersistStreamInit> pPersistStreamInit;
				pHtmlDoc.QueryInterface(IID_IPersistStreamInit, &pPersistStreamInit);

				CheckError(pPersistStreamInit->InitNew());
				CheckError(pPersistStreamInit->Load(pStream));
			} else
				GlobalFree(hHtmlText);
		}
	}
}


HWND create_webchildwindow(const WebChildWndInfo& info)
{
	WebChildWindow* pWnd = WebChildWindow::create(info);

	if (!pWnd)
		return 0;

	return *pWnd;
}

static const CLSID CLSID_MozillaBrowser =
	{0x1339B54C, 0x3453, 0x11D2, {0x93, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};


WebChildWindow::WebChildWindow(HWND hwnd, const WebChildWndInfo& info)
 :	super(hwnd)
{
	 // first try to create a web control with MS IE's CLASSID
	HRESULT hr = create_control(hwnd, CLSID_WebBrowser, IID_IWebBrowser2);

	 // If this failed, try to use Mozilla's web control
	if (FAILED(hr))
		hr = create_control(hwnd, CLSID_MozillaBrowser, IID_IWebBrowser2);

	if (SUCCEEDED(hr)) {
		_navigator.attach(_control);

		HWND hwndFrame = GetParent(info._hmdiclient);

		 // handling events using DWebBrowserEvents2
		_evt_handler = auto_ptr<DWebBrowserEvents2Handler>(new DWebBrowserEvents2Handler(_hwnd, hwndFrame, _navigator));

#ifdef __MINGW32__	// MinGW is lacking vtMissing (as of 07.02.2004)
		Variant vtMissing;
#endif
		get_browser()->Navigate(BStr(info._path), &vtMissing, &vtMissing, &vtMissing, &vtMissing);
		//browser->Navigate2(&Variant(info._path), &vtMissing, &vtMissing, &vtMissing, &vtMissing);
	}
}

LRESULT WebChildWindow::WndProc(UINT message, WPARAM wparam, LPARAM lparam)
{
	try {
		switch(message) {
		  case WM_ERASEBKGND:
			if (!_control) {
				HDC hdc = (HDC)wparam;
				ClientRect rect(_hwnd);

				HBRUSH hbrush = CreateSolidBrush(RGB(200,200,235));
				BkMode mode(hdc, TRANSPARENT);
				TextColor color(hdc, RGB(200,40,40));
				FillRect(hdc, &rect, hbrush);
				DrawText(hdc, TEXT("Sorry - no web browser control could be loaded."), -1, &rect, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
				DeleteObject(hbrush);
			}

			return TRUE;

 		  case PM_DISPATCH_COMMAND: {
			if (_control) {
				HRESULT hr = E_FAIL;

				switch(LOWORD(wparam)) {
				  case ID_BROWSE_BACK:
					hr = _control->GoBack();
					break;

				  case ID_BROWSE_FORWARD:
					hr = _control->GoForward();
					break;

				  case ID_BROWSE_HOME:
					hr = _control->GoHome();
					break;

				  case ID_BROWSE_SEARCH:
					hr = _control->GoSearch();
					break;

				  case ID_REFRESH:
					hr = _control->Refresh();
					break;

				  case ID_STOP:
					hr = _control->Stop();
					break;

				  default:
					return FALSE;
				}

				if (FAILED(hr) && hr!=E_FAIL)
					THROW_EXCEPTION(hr);
			}

			return TRUE;}

		  default:
			return super::WndProc(message, wparam, lparam);
		}
	} catch(COMException& e) {
		HandleException(e, _hwnd);
	}

	return 0;
}
