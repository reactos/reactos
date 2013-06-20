/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED 'AS IS' WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.
   Author: Leon Finker  01/2001
   Modifications: removed ATL dependencies, Martin Fuchs 7/2003
**************************************************************************/

#include "dragdropimpl.h"

 /// OLE drop target for tree controls
class TreeDropTarget : public IDropTargetImpl
{
public:
	TreeDropTarget(HWND hTargetWnd) : IDropTargetImpl(hTargetWnd) {}

	virtual bool OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect)
	{
		if (pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
		{
			HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
			if (hDrop != NULL)
			{
				TCHAR szFileName[MAX_PATH];

				UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

				for(UINT i = 0; i < cFiles; ++i)
				{
					DragQueryFile(hDrop, i, szFileName, sizeof(szFileName));

					if (DROPEFFECT_COPY & *pdwEffect)
					{
						 // copy the file or dir

						///@todo Add the code to handle Copy

					}
					else if (DROPEFFECT_MOVE & *pdwEffect)
					{
						 // move the file or dir

						///@todo Add the code to handle Move

					}
				}
				//DragFinish(hDrop); // base class calls ReleaseStgMedium
			}
			GlobalUnlock(medium.hGlobal);
		}

		TreeView_SelectDropTarget(m_hTargetWnd, NULL);

		return true; //let base free the medium
	}

	virtual HRESULT STDMETHODCALLTYPE DragOver(
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ DWORD __RPC_FAR *pdwEffect)
	{
		TVHITTESTINFO hit;
		hit.pt.x = pt.x;
		hit.pt.y = pt.y;
		ScreenToClient(m_hTargetWnd, &hit.pt);
		hit.flags = TVHT_ONITEM;
		HTREEITEM hItem = TreeView_HitTest(m_hTargetWnd,&hit);

		if (hItem != NULL)
			TreeView_SelectDropTarget(m_hTargetWnd, hItem);

		return IDropTargetImpl::DragOver(grfKeyState, pt, pdwEffect);
	}

	virtual HRESULT STDMETHODCALLTYPE DragLeave(void)
	{
		TreeView_SelectDropTarget(m_hTargetWnd, NULL);

		return IDropTargetImpl::DragLeave();
	}
};
