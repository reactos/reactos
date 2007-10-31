/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL v2 - See COPYING in the top level directory
 * FILE:        lib/recyclebin/recyclebin_generic.c
 * PURPOSE:     Deals with a system-wide recycle bin
 * PROGRAMMERS: Copyright 2007 Hervé Poussineau (hpoussin@reactos.org)
 */

#define COBJMACROS
#include "recyclebin_private.h"
#include <stdio.h>

struct RecycleBinGeneric
{
	ULONG ref;
	IRecycleBin recycleBinImpl;
};

static HRESULT STDMETHODCALLTYPE
RecycleBinGenericVtbl_RecycleBin_QueryInterface( 
	IRecycleBin *This,
	REFIID riid,
	void **ppvObject)
{
	struct RecycleBinGeneric *s = CONTAINING_RECORD(This, struct RecycleBinGeneric, recycleBinImpl);

	if (!ppvObject)
		return E_POINTER;

	if (IsEqualIID(riid, &IID_IUnknown))
		*ppvObject = &s->recycleBinImpl;
	else if (IsEqualIID(riid, &IID_IRecycleBin))
		*ppvObject = &s->recycleBinImpl;
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}

	IUnknown_AddRef(This);
	return S_OK;
}

static ULONG STDMETHODCALLTYPE
RecycleBinGenericVtbl_RecycleBin_AddRef(
	IRecycleBin *This)
{
	struct RecycleBinGeneric *s = CONTAINING_RECORD(This, struct RecycleBinGeneric, recycleBinImpl);
	ULONG refCount = InterlockedIncrement((PLONG)&s->ref);
	return refCount;
}

static ULONG STDMETHODCALLTYPE
RecycleBinGenericVtbl_RecycleBin_Release(
	IRecycleBin *This)
{
	struct RecycleBinGeneric *s = CONTAINING_RECORD(This, struct RecycleBinGeneric, recycleBinImpl);
	ULONG refCount;

	if (!This)
		return E_POINTER;

	refCount = InterlockedDecrement((PLONG)&s->ref);

	if (refCount == 0)
		CoTaskMemFree(s);

	return refCount;
}

static HRESULT STDMETHODCALLTYPE
RecycleBinGenericVtbl_RecycleBin_DeleteFile(
	IN IRecycleBin *This,
	IN LPCWSTR szFileName)
{
	IRecycleBin *prb;
	LPWSTR szFullName = NULL;
	DWORD dwBufferLength = 0;
	DWORD len;
	WCHAR szVolume[MAX_PATH];
	HRESULT hr;

	/* Get full file name */
	while (TRUE)
	{
		len = GetFullPathNameW(szFileName, dwBufferLength, szFullName, NULL);
		if (len == 0)
		{
			if (szFullName)
				CoTaskMemFree(szFullName);
			return HRESULT_FROM_WIN32(GetLastError());
		}
		else if (len < dwBufferLength)
			break;
		if (szFullName)
			CoTaskMemFree(szFullName);
		dwBufferLength = len;
		szFullName = CoTaskMemAlloc(dwBufferLength * sizeof(WCHAR));
		if (!szFullName)
			return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}

	/* Get associated volume path */
#ifndef __REACTOS__
	if (!GetVolumePathNameW(szFullName, szVolume, MAX_PATH))
	{
		CoTaskMemFree(szFullName);
		return HRESULT_FROM_WIN32(GetLastError());
	}
#else
	swprintf(szVolume, L"%c:\\", szFullName[0]);
#endif

	/* Skip namespace (if any) */
	if (szVolume[0] == '\\'
	 && szVolume[1] == '\\'
	 && (szVolume[2] == '.' || szVolume[2] == '?')
	 && szVolume[3] == '\\')
	{
		MoveMemory(szVolume, &szVolume[4], (MAX_PATH - 4) * sizeof(WCHAR));
	}

	hr = GetDefaultRecycleBin(szVolume, &prb);
	if (!SUCCEEDED(hr))
	{
		CoTaskMemFree(szFullName);
		return hr;
	}

	hr = IRecycleBin_DeleteFile(prb, szFullName);
	CoTaskMemFree(szFullName);
	IRecycleBin_Release(prb);
	return hr;
}

static HRESULT STDMETHODCALLTYPE
RecycleBinGenericVtbl_RecycleBin_EmptyRecycleBin(
	IN IRecycleBin *This)
{
	WCHAR szVolumeName[MAX_PATH];
	DWORD dwLogicalDrives, i;
	IRecycleBin *prb;
	HRESULT hr;

	dwLogicalDrives = GetLogicalDrives();
	if (dwLogicalDrives == 0)
		return HRESULT_FROM_WIN32(GetLastError());

	for (i = 0; i < 26; i++)
	{
		if (!(dwLogicalDrives & (1 << i)))
			continue;
		swprintf(szVolumeName, L"%c:\\", 'A' + i);
		if (GetDriveTypeW(szVolumeName) != DRIVE_FIXED)
			continue;
		
		hr = GetDefaultRecycleBin(szVolumeName, &prb);
		if (!SUCCEEDED(hr))
			return hr;

		hr = IRecycleBin_EmptyRecycleBin(prb);
		IRecycleBin_Release(prb);
	}

	return S_OK;
}

static HRESULT STDMETHODCALLTYPE
RecycleBinGenericVtbl_RecycleBin_EnumObjects(
	IN IRecycleBin *This,
	OUT IRecycleBinEnumList **ppEnumList)
{
	return RecycleBinGeneric_Enumerator_Constructor(ppEnumList);
}

CONST_VTBL struct IRecycleBinVtbl RecycleBinGenericVtbl =
{
	RecycleBinGenericVtbl_RecycleBin_QueryInterface,
	RecycleBinGenericVtbl_RecycleBin_AddRef,
	RecycleBinGenericVtbl_RecycleBin_Release,
	RecycleBinGenericVtbl_RecycleBin_DeleteFile,
	RecycleBinGenericVtbl_RecycleBin_EmptyRecycleBin,
	RecycleBinGenericVtbl_RecycleBin_EnumObjects,
};

HRESULT RecycleBinGeneric_Constructor(OUT IUnknown **ppUnknown)
{
	/* This RecycleBin implementation was introduced to be able to manage all
	 * drives at once, and instanciate the 'real' implementations when needed */
	struct RecycleBinGeneric *s;

	s = CoTaskMemAlloc(sizeof(struct RecycleBinGeneric));
	if (!s)
		return E_OUTOFMEMORY;
	s->ref = 1;
	s->recycleBinImpl.lpVtbl = &RecycleBinGenericVtbl;

	*ppUnknown = (IUnknown *)&s->recycleBinImpl;
	return S_OK;
}
