/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL v2 - See COPYING in the top level directory
 * FILE:        lib/recyclebin/recyclebin_generic_enumerator.c
 * PURPOSE:     Enumerates contents of all recycle bins
 * PROGRAMMERS: Copyright 2007 Hervé Poussineau (hpoussin@reactos.org)
 */

#define COBJMACROS
#include "recyclebin.h"
#include <stdio.h>

struct RecycleBinGenericEnum
{
	ULONG ref;
	IRecycleBinEnumList recycleBinEnumImpl;
	IRecycleBinEnumList *current;
	DWORD dwLogicalDrives;
	SIZE_T skip;
};

static HRESULT STDMETHODCALLTYPE
RecycleBinGenericEnum_RecycleBinEnumList_QueryInterface(
	IN IRecycleBinEnumList *This,
	IN REFIID riid,
	OUT void **ppvObject)
{
	struct RecycleBinGenericEnum *s = CONTAINING_RECORD(This, struct RecycleBinGenericEnum, recycleBinEnumImpl);

	if (!ppvObject)
		return E_POINTER;

	if (IsEqualIID(riid, &IID_IUnknown))
		*ppvObject = &s->recycleBinEnumImpl;
	else if (IsEqualIID(riid, &IID_IRecycleBinEnumList))
		*ppvObject = &s->recycleBinEnumImpl;
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}

	IUnknown_AddRef(This);
	return S_OK;
}

static ULONG STDMETHODCALLTYPE
RecycleBinGenericEnum_RecycleBinEnumList_AddRef(
	IN IRecycleBinEnumList *This)
{
	struct RecycleBinGenericEnum *s = CONTAINING_RECORD(This, struct RecycleBinGenericEnum, recycleBinEnumImpl);
	ULONG refCount = InterlockedIncrement((PLONG)&s->ref);
	return refCount;
}

static ULONG STDMETHODCALLTYPE
RecycleBinGenericEnum_RecycleBinEnumList_Release(
	IN IRecycleBinEnumList *This)
{
	struct RecycleBinGenericEnum *s = CONTAINING_RECORD(This, struct RecycleBinGenericEnum, recycleBinEnumImpl);
	ULONG refCount;

	if (!This)
		return E_POINTER;

	refCount = InterlockedDecrement((PLONG)&s->ref);

	if (refCount == 0)
	{
		if (s->current)
			IRecycleBinEnumList_Release(s->current);
		CoTaskMemFree(s);
	}

	return refCount;
}

static HRESULT STDMETHODCALLTYPE
RecycleBinGenericEnum_RecycleBinEnumList_Next(
	IN IRecycleBinEnumList *This,
	IN DWORD celt,
	IN OUT IRecycleBinFile **rgelt,
	OUT DWORD *pceltFetched)
{
	struct RecycleBinGenericEnum *s = CONTAINING_RECORD(This, struct RecycleBinGenericEnum, recycleBinEnumImpl);
	IRecycleBin *prb;
	DWORD i;
	DWORD fetched = 0, newFetched;
	HRESULT hr;

	if (!rgelt)
		return E_POINTER;
	if (!pceltFetched && celt > 1)
		return E_INVALIDARG;

	while (TRUE)
	{
		/* Get enumerator implementation */
		if (!s->current && s->dwLogicalDrives)
		{
			for (i = 0; i < 26; i++)
				if (s->dwLogicalDrives & (1 << i))
				{
					WCHAR szVolumeName[4];
					szVolumeName[0] = (WCHAR)('A' + i);
					szVolumeName[1] = ':';
					szVolumeName[2] = '\\';
					szVolumeName[3] = UNICODE_NULL;
					if (GetDriveTypeW(szVolumeName) != DRIVE_FIXED)
					{
						s->dwLogicalDrives &= ~(1 << i);
						continue;
					}
					hr = GetDefaultRecycleBin(szVolumeName, &prb);
					if (!SUCCEEDED(hr))
						return hr;
					hr = IRecycleBin_EnumObjects(prb, &s->current);
					IRecycleBin_Release(prb);
					if (!SUCCEEDED(hr))
						return hr;
					s->dwLogicalDrives &= ~(1 << i);
					break;
				}
		}
		if (!s->current)
		{
			/* Nothing more to enumerate */
			if (pceltFetched)
				*pceltFetched = fetched;
			return S_FALSE;
		}

		/* Skip some elements */
		while (s->skip > 0)
		{
			IRecycleBinFile *rbf;
			hr = IRecycleBinEnumList_Next(s->current, 1, &rbf, NULL);
			if (hr == S_OK)
				hr = IRecycleBinFile_Release(rbf);
			else if (hr == S_FALSE)
				break;
			else if (!SUCCEEDED(hr))
				return hr;
		}
		if (s->skip > 0)
			continue;

		/* Fill area */
		hr = IRecycleBinEnumList_Next(s->current, celt - fetched, &rgelt[fetched], &newFetched);
		if (SUCCEEDED(hr))
			fetched += newFetched;
		if (hr == S_FALSE || newFetched == 0)
		{
			hr = IRecycleBinEnumList_Release(s->current);
			s->current = NULL;
		}
		else if (!SUCCEEDED(hr))
			return hr;
		if (fetched == celt)
		{
			if (pceltFetched)
				*pceltFetched = fetched;
			return S_OK;
		}
	}

	/* Never go here */
}

static HRESULT STDMETHODCALLTYPE
RecycleBinGenericEnum_RecycleBinEnumList_Skip(
	IN IRecycleBinEnumList *This,
	IN DWORD celt)
{
	struct RecycleBinGenericEnum *s = CONTAINING_RECORD(This, struct RecycleBinGenericEnum, recycleBinEnumImpl);
	s->skip += celt;
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE
RecycleBinGenericEnum_RecycleBinEnumList_Reset(
	IN IRecycleBinEnumList *This)
{
	struct RecycleBinGenericEnum *s = CONTAINING_RECORD(This, struct RecycleBinGenericEnum, recycleBinEnumImpl);

	if (s->current)
	{
		IRecycleBinEnumList_Release(s->current);
		s->current = NULL;
		s->skip = 0;
	}
	s->dwLogicalDrives = GetLogicalDrives();
	return S_OK;
}

CONST_VTBL struct IRecycleBinEnumListVtbl RecycleBinGenericEnumVtbl =
{
	RecycleBinGenericEnum_RecycleBinEnumList_QueryInterface,
	RecycleBinGenericEnum_RecycleBinEnumList_AddRef,
	RecycleBinGenericEnum_RecycleBinEnumList_Release,
	RecycleBinGenericEnum_RecycleBinEnumList_Next,
	RecycleBinGenericEnum_RecycleBinEnumList_Skip,
	RecycleBinGenericEnum_RecycleBinEnumList_Reset,
};

HRESULT
RecycleBinGeneric_Enumerator_Constructor(
	OUT IRecycleBinEnumList **pprbel)
{
	struct RecycleBinGenericEnum *s;

	s = CoTaskMemAlloc(sizeof(struct RecycleBinGenericEnum));
	if (!s)
		return E_OUTOFMEMORY;
	ZeroMemory(s, sizeof(struct RecycleBinGenericEnum));
	s->ref = 1;
	s->recycleBinEnumImpl.lpVtbl = &RecycleBinGenericEnumVtbl;

	*pprbel = &s->recycleBinEnumImpl;
	return IRecycleBinEnumList_Reset(*pprbel);
}
