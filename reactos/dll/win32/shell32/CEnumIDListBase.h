/*
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

#ifndef __ENUMIDLIST_H__
#define __ENUMIDLIST_H__

struct ENUMLIST
{
	ENUMLIST				*pNext;
	LPITEMIDLIST			pidl;
};

class CEnumIDListBase :
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public IEnumIDList
{
private:
	ENUMLIST				*mpFirst;
	ENUMLIST				*mpLast;
	ENUMLIST				*mpCurrent;
public:
	CEnumIDListBase();
	~CEnumIDListBase();
	BOOL AddToEnumList(LPITEMIDLIST pidl);
	BOOL DeleteList();
	BOOL HasItemWithCLSID(LPITEMIDLIST pidl);
	BOOL CreateFolderEnumList(LPCWSTR lpszPath, DWORD dwFlags);

	// *** IEnumIDList methods ***
	virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
	virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
	virtual HRESULT STDMETHODCALLTYPE Reset();
	virtual HRESULT STDMETHODCALLTYPE Clone(IEnumIDList **ppenum);

BEGIN_COM_MAP(CEnumIDListBase)
	COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
END_COM_MAP()
};

#endif /* __ENUMIDLIST_H__ */
