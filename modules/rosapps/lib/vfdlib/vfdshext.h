/*
	vfdshext.h

	Virtual Floppy Drive for Windows
	Driver control library
	shell extension COM class header

	Copyright (c) 2003-2005 Ken Kato
*/

#ifndef _VFDSHEXT_H_
#define _VFDSHEXT_H_

//
// CVfdShExt
// COM Shell extension class
//
class CVfdShExt : public	IContextMenu,
							IShellExtInit,
							IShellPropSheetExt
//							IQueryInfo
{
protected:
	ULONG			m_cRefCnt;				//	reference count
	LPDATAOBJECT	m_pDataObj;				//	IDataObject pointer
	ULONG			m_nDevice;				//	VFD device number
	CHAR			m_sTarget[MAX_PATH];	//	target path
	BOOL			m_bDragDrop;

public:
	//	constructor / destructor
	CVfdShExt();
	~CVfdShExt();

	//	perform VFD operations
	DWORD DoVfdOpen(HWND hParent);
	DWORD DoVfdNew(HWND hParent);
	DWORD DoVfdClose(HWND hParent);
	DWORD DoVfdSave(HWND hParent);
	DWORD DoVfdProtect(HWND hParent);
	DWORD DoVfdDrop(HWND hParent);

	//	get current attributes
	ULONG	GetDevice()	{ return m_nDevice; }
	PCSTR	GetTarget()	{ return m_sTarget; }

	//	IUnknown inheritance
	STDMETHODIMP QueryInterface(REFIID, LPVOID *);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//	IShellExtInit inheritance
	STDMETHODIMP Initialize(
		LPCITEMIDLIST	pIDFolder,
		LPDATAOBJECT	pDataObj,
		HKEY			hKeyID);

	//	IContextMenu inheritance
	STDMETHODIMP QueryContextMenu(
		HMENU			hMenu,
		UINT			indexMenu,
		UINT			idCmdFirst,
		UINT			idCmdLast,
		UINT			uFlags);

	STDMETHODIMP InvokeCommand(
		LPCMINVOKECOMMANDINFO lpcmi);

	STDMETHODIMP GetCommandString(
#ifndef __REACTOS__
		UINT			idCmd,
#else
		UINT_PTR		idCmd,
#endif
		UINT			uFlags,
		UINT			*reserved,
		LPSTR			pszName,
		UINT			cchMax);

	//	IShellPropSheetExt inheritance
	STDMETHODIMP AddPages(
		LPFNADDPROPSHEETPAGE lpfnAddPage,
		LPARAM			lParam);

	STDMETHODIMP ReplacePage(
		UINT			uPageID,
		LPFNADDPROPSHEETPAGE lpfnReplaceWith,
		LPARAM			lParam);
/*
	//	IQueryInfo inheritance

	STDMETHODIMP GetInfoFlags(
		DWORD *pdwFlags);

	STDMETHODIMP GetInfoTip(
		DWORD dwFlags,
		LPWSTR *ppwszTip);
*/
};

typedef CVfdShExt *LPCVFDSHEXT;

#endif	// _VFDSHEXT_H_
