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
protected:
	ENUMLIST				*mpFirst;
	ENUMLIST				*mpLast;
	ENUMLIST				*mpCurrent;
public:
	CEnumIDListBase();
	virtual ~CEnumIDListBase();
	BOOL AddToEnumList(LPITEMIDLIST pidl);
	BOOL DeleteList();
	BOOL HasItemWithCLSID(LPITEMIDLIST pidl);
	HRESULT AppendItemsFromEnumerator(IEnumIDList* pEnum);

	template <class T> BOOL HasItemWithCLSIDImpl(LPCITEMIDLIST pidl)
	{
		const CLSID * const pClsid = static_cast<T*>(this)->GetPidlClsid((PCUITEMID_CHILD)pidl);
		for (ENUMLIST *pCur = mpFirst; pClsid && pCur; pCur = pCur->pNext)
		{
			const CLSID * const pEnumClsid = static_cast<T*>(this)->GetPidlClsid((PCUITEMID_CHILD)pCur->pidl);
			if (pEnumClsid && IsEqualCLSID(*pClsid, *pEnumClsid))
				return TRUE;
		}
		return FALSE;
	}

	// *** IEnumIDList methods ***
	STDMETHOD(Next)(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched) override;
	STDMETHOD(Skip)(ULONG celt) override;
	STDMETHOD(Reset)() override;
	STDMETHOD(Clone)(IEnumIDList **ppenum) override;

BEGIN_COM_MAP(CEnumIDListBase)
	COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
END_COM_MAP()
};

#endif /* __ENUMIDLIST_H__ */
