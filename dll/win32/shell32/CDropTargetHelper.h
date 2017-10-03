/*
 *	file system folder
 *
 *	Copyright 1997			    Marcus Meissner
 *	Copyright 1998, 1999, 2002	Juergen Schmied
 *	Copyright 2009              Andrew Hill
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _DRAGDROPHELPER_H_
#define _DRAGDROPHELPER_H_

class CDropTargetHelper :
	public CComCoClass<CDropTargetHelper, &CLSID_DragDropHelper>,
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public IDragSourceHelper,
	public IDropTargetHelper
{
private:
public:
	CDropTargetHelper();
	~CDropTargetHelper();

	virtual HRESULT WINAPI InitializeFromBitmap(LPSHDRAGIMAGE pshdi, IDataObject *pDataObject);
	virtual HRESULT WINAPI InitializeFromWindow(HWND hwnd, POINT *ppt, IDataObject *pDataObject);

	virtual HRESULT WINAPI DragEnter (HWND hwndTarget, IDataObject* pDataObject, POINT* ppt, DWORD dwEffect);
	virtual HRESULT WINAPI DragLeave();
	virtual HRESULT WINAPI DragOver(POINT *ppt, DWORD dwEffect);
	virtual HRESULT WINAPI Drop(IDataObject* pDataObject, POINT* ppt, DWORD dwEffect);
	virtual HRESULT WINAPI Show(BOOL fShow);

DECLARE_REGISTRY_RESOURCEID(IDR_DRAGDROPHELPER)
DECLARE_NOT_AGGREGATABLE(CDropTargetHelper)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDropTargetHelper)
	COM_INTERFACE_ENTRY_IID(IID_IDragSourceHelper, IDragSourceHelper)
	COM_INTERFACE_ENTRY_IID(IID_IDropTargetHelper, IDropTargetHelper)
END_COM_MAP()
};

#endif /* _DRAGDROPHELPER_H_ */
