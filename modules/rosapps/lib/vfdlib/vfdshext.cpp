/*
	vfdshext.cpp

	Virtual Floppy Drive for Windows
	Driver control library
	shell extension COM shell extension class

	Copyright (c) 2003-2005 Ken Kato
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include "vfdtypes.h"
#include "vfdapi.h"
#include "vfdlib.h"

//	class header
#include "vfdshext.h"

//
//	Constructor
//
CVfdShExt::CVfdShExt()
{
	VFDTRACE(0, ("CVfdShExt::CVfdShExt()\n"));

	m_cRefCnt		= 0L;
	m_pDataObj		= NULL;
	m_nDevice		= (ULONG)-1;
	m_sTarget[0]	= '\0';
	m_bDragDrop		= FALSE;

	g_cDllRefCnt++;
}

//
//	Destructor
//
CVfdShExt::~CVfdShExt()
{
	VFDTRACE(0, ("CVfdShExt::~CVfdShExt()\n"));

	if (m_pDataObj) {
		m_pDataObj->Release();
	}

	g_cDllRefCnt--;
}

//	IUnknown members

STDMETHODIMP CVfdShExt::QueryInterface(
	REFIID			riid,
	LPVOID			*ppv)
{
	*ppv = NULL;

	if (IsEqualIID(riid, IID_IShellExtInit) ||
		IsEqualIID(riid, IID_IUnknown)) {
		VFDTRACE(0,
			("CVfdShExt::QueryInterface()==>IID_IShellExtInit\n"));

		*ppv = (LPSHELLEXTINIT)this;
	}
	else if (IsEqualIID(riid, IID_IContextMenu)) {
		VFDTRACE(0,
			("CVfdShExt::QueryInterface()==>IID_IContextMenu\n"));

		*ppv = (LPCONTEXTMENU)this;
	}
	else if (IsEqualIID(riid, IID_IShellPropSheetExt)) {
		VFDTRACE(0,
			("CVfdShExt::QueryInterface()==>IID_IShellPropSheetExt\n"));

		*ppv = (LPSHELLPROPSHEETEXT)this;
	}

	if (*ppv) {
		AddRef();

		return NOERROR;
	}

	VFDTRACE(0,
		("CVfdShExt::QueryInterface()==>Unknown Interface!\n"));

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CVfdShExt::AddRef()
{
	VFDTRACE(0, ("CVfdShExt::AddRef()\n"));

	return ++m_cRefCnt;
}

STDMETHODIMP_(ULONG) CVfdShExt::Release()
{
	VFDTRACE(0, ("CVfdShExt::Release()\n"));

	if (--m_cRefCnt) {
		return m_cRefCnt;
	}

#ifndef __REACTOS__
	delete this;
#endif

	return 0L;
}

//	IShellExtInit members

//
//	Initialize
//	Called by the shell to initialize the shell extension object
//
STDMETHODIMP CVfdShExt::Initialize(
	LPCITEMIDLIST	pIDFolder,
	LPDATAOBJECT	pDataObj,
	HKEY			hRegKey)
{
	CHAR drive = '\0';

	VFDTRACE(0, ("CVfdShExt::Initialize()\n"));

	UNREFERENCED_PARAMETER(hRegKey);

	// Initialize can be called more than once

	if (m_pDataObj) {
		m_pDataObj->Release();
		m_pDataObj = NULL;
	}

	m_nDevice = (ULONG)-1;
	m_sTarget[0] = '\0';

	//	Get the folder name
	if (SHGetPathFromIDList(pIDFolder, m_sTarget)) {

		//	act as a Drag-and-Drop Handler

		VFDTRACE(0, ("Drag-Drop: %s\n", m_sTarget));

		if (GetDriveType(m_sTarget) != DRIVE_REMOVABLE) {
			VFDTRACE(0, ("Not a VFD drive\n"));
			return NOERROR;
		}

		drive = m_sTarget[0];
		m_bDragDrop = TRUE;
	}
	else {

		//	act as a context menu handler

		VFDTRACE(0, ("Context menu:\n"));
		m_bDragDrop = FALSE;
	}

	// Extract the target object name

	if (pDataObj) {

		STGMEDIUM medium;
		FORMATETC fmt = {
			CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
		};

		if (SUCCEEDED(pDataObj->GetData(&fmt, &medium))) {
			if (DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0)) {

				DragQueryFile((HDROP)medium.hGlobal,
					0, m_sTarget, sizeof(m_sTarget));
			}

			ReleaseStgMedium(&medium);
		}
	}

	VFDTRACE(0, ("Target %s\n", m_sTarget));

	if (!drive) {
		//	Contect menu handler
		//	-- Data object is the target drive
		drive = m_sTarget[0];
	}

	HANDLE hDevice = VfdOpenDevice(drive);

	if (hDevice == INVALID_HANDLE_VALUE) {
		VFDTRACE(0, ("Not a VFD drive\n"));
		return NOERROR;
	}

	ULONG ret = VfdGetDeviceNumber(hDevice, &m_nDevice);

	CloseHandle(hDevice);

	if (ret != ERROR_SUCCESS) {
		m_nDevice = (ULONG)-1;
		return NOERROR;
	}

	VFDTRACE(0, ("VFD device %d\n", m_nDevice));
	//	Store the data object

	m_pDataObj = pDataObj;
	m_pDataObj->AddRef();

	return NOERROR;
}

/*
STDMETHODIMP CVfdShExt::GetInfoFlags(
	DWORD *pdwFlags)
{
	VFDTRACE(0, ("CVfdShExt::GetInfoFlags\n"));
	*pdwFlags = 0;
	return NOERROR;
}

STDMETHODIMP CVfdShExt::GetInfoTip(
	DWORD dwFlags,
	LPWSTR *ppwszTip)
{
	VFDTRACE(0, ("CVfdShExt::GetInfoTip\n"));
	*ppwszTip = NULL;
	return NOERROR;
}
*/
