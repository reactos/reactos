/*
 * Copyright 2003 Martin Fuchs
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
 // desktop.h
 //
 // Martin Fuchs, 09.08.2003
 //


struct BackgroundWindow : public SubclassedWindow
{
	typedef SubclassedWindow super;

	BackgroundWindow(HWND hwnd) : super(hwnd) {}

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
};


struct DesktopWindow : public Window, public IShellBrowserImpl
{
	typedef Window super;

	DesktopWindow(HWND hwnd);
	~DesktopWindow();

	static HWND Create();

	STDMETHOD(GetWindow)(HWND* lphwnd)
	{
		*lphwnd = _hwnd;
		return S_OK;
	}

	STDMETHOD(QueryActiveShellView)(IShellView** ppshv)
	{
		_pShellView->AddRef();
		*ppshv = _pShellView;
		return S_OK;
	}

	STDMETHOD(GetControlWindow)(UINT id, HWND* lphwnd)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(SendControlMsg)(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pret)
	{
		return E_NOTIMPL;
	}

protected:
	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	IShellView*	_pShellView;
	WindowHandle _desktopBar;
};
