/*
 * Copyright 2003, 2004 Martin Fuchs
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


#define	PM_SET_ICON_ALGORITHM	(WM_APP+0x19)
#define	PM_GET_ICON_ALGORITHM	(WM_APP+0x1A)
#define	PM_DISPLAY_VERSION		(WM_APP+0x24)


 /// subclassed Background window behind the visible desktop window
struct BackgroundWindow : public SubclassedWindow
{
	typedef SubclassedWindow super;

	BackgroundWindow(HWND hwnd);

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	void	DrawDesktopBkgnd(HDC hdc);

	int		_display_version;
};


 /// Implementation of the Explorer desktop window
struct DesktopWindow : public Window, public IShellBrowserImpl
{
	typedef Window super;

	DesktopWindow(HWND hwnd);
	~DesktopWindow();

	static HWND Create();

	virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND* lphwnd)
	{
		*lphwnd = _hwnd;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryActiveShellView(IShellView** ppshv)
	{
		_pShellView->AddRef();
		*ppshv = _pShellView;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetControlWindow(UINT id, HWND* lphwnd)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pret)
	{
		return E_NOTIMPL;
	}

protected:
	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	IShellView*	_pShellView;
	WindowHandle _desktopBar;

	virtual HRESULT OnDefaultCommand(LPIDA pida);
};


 /// OLE drop target for the desktop window
class DesktopDropTarget : public IDropTargetImpl
{
	typedef IDropTargetImpl super;

public:
	DesktopDropTarget(HWND hTargetWnd) : super(hTargetWnd) {}

	virtual bool OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect)
	{
		if (pFmtEtc->cfFormat==CF_HDROP && medium.tymed==TYMED_HGLOBAL) {
			HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);

			if (hDrop) {
				TCHAR szFileName[MAX_PATH];

				UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

				for(UINT i=0; i<cFiles; ++i) {
					DragQueryFile(hDrop, i, szFileName, sizeof(szFileName));

					if (DROPEFFECT_COPY & *pdwEffect) {
						 // copy the file or dir

						///@todo Add the code to handle Copy

					} else if (DROPEFFECT_MOVE & *pdwEffect) {
						 // move the file or dir

						///@todo Add the code to handle Move

					}
				}
				//DragFinish(hDrop); // base class calls ReleaseStgMedium
			}

			GlobalUnlock(medium.hGlobal);
		}

		//@@TreeView_SelectDropTarget(m_hTargetWnd, NULL);

		return true; //let base free the medium
	}

	virtual HRESULT STDMETHODCALLTYPE DragOver(
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ DWORD __RPC_FAR *pdwEffect)
	{
		TVHITTESTINFO hit;
		hit.pt = (POINT&)pt;
		ScreenToClient(m_hTargetWnd, &hit.pt);
		hit.flags = TVHT_ONITEM;

		/*@@
		HTREEITEM hItem = TreeView_HitTest(m_hTargetWnd,&hit);

		if (hItem != NULL)
			TreeView_SelectDropTarget(m_hTargetWnd, hItem);
		*/

		return super::DragOver(grfKeyState, pt, pdwEffect);
	}

	virtual HRESULT STDMETHODCALLTYPE DragLeave(void)
	{
		//@@ TreeView_SelectDropTarget(m_hTargetWnd, NULL);

		return super::DragLeave();
	}
};


 /// subclassed ShellView window
struct DesktopShellView : public SubclassedWindow
{
	typedef SubclassedWindow super;

	DesktopShellView(HWND hwnd, IShellView* pShellView);

	bool	InitDragDrop();

protected:
	IShellView* _pShellView;

	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);
	int		Notify(int id, NMHDR* pnmh);

	bool	DoContextMenu(int x, int y);
	HRESULT DoDesktopContextMenu(int x, int y);
	void	PositionIcons(int dir=1);

	DesktopDropTarget* _pDropTarget;
	HWND	_hwndListView;
	int		_icon_algo;
};
