/* IDirectMusicCollection Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

static ULONG WINAPI IDirectMusicCollectionImpl_IUnknown_AddRef (LPUNKNOWN iface);
static ULONG WINAPI IDirectMusicCollectionImpl_IDirectMusicCollection_AddRef (LPDIRECTMUSICCOLLECTION iface);
static ULONG WINAPI IDirectMusicCollectionImpl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface);
static ULONG WINAPI IDirectMusicCollectionImpl_IPersistStream_AddRef (LPPERSISTSTREAM iface);

/*****************************************************************************
 * IDirectMusicCollectionImpl implementation
 */
/* IDirectMusicCollectionImpl IUnknown part: */
static HRESULT WINAPI IDirectMusicCollectionImpl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, UnknownVtbl, iface);
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = &This->UnknownVtbl;
		IDirectMusicCollectionImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;	
	} else if (IsEqualIID (riid, &IID_IDirectMusicCollection)) {
		*ppobj = &This->CollectionVtbl;
		IDirectMusicCollectionImpl_IDirectMusicCollection_AddRef ((LPDIRECTMUSICCOLLECTION)&This->CollectionVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicObject)) {
		*ppobj = &This->ObjectVtbl;
		IDirectMusicCollectionImpl_IDirectMusicObject_AddRef ((LPDIRECTMUSICOBJECT)&This->ObjectVtbl);		
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = &This->PersistStreamVtbl;
		IDirectMusicCollectionImpl_IPersistStream_AddRef ((LPPERSISTSTREAM)&This->PersistStreamVtbl);		
		return S_OK;
	}
	
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicCollectionImpl_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, UnknownVtbl, iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

	DMUSIC_LockModule();

	return refCount;
}

static ULONG WINAPI IDirectMusicCollectionImpl_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, UnknownVtbl, iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

	if (!refCount) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMUSIC_UnlockModule();

	return refCount;
}

static const IUnknownVtbl DirectMusicCollection_Unknown_Vtbl = {
	IDirectMusicCollectionImpl_IUnknown_QueryInterface,
	IDirectMusicCollectionImpl_IUnknown_AddRef,
	IDirectMusicCollectionImpl_IUnknown_Release
};

/* IDirectMusicCollectionImpl IDirectMusicCollection part: */
static HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicCollection_QueryInterface (LPDIRECTMUSICCOLLECTION iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, CollectionVtbl, iface);
	return IDirectMusicCollectionImpl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicCollectionImpl_IDirectMusicCollection_AddRef (LPDIRECTMUSICCOLLECTION iface) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, CollectionVtbl, iface);
	return IDirectMusicCollectionImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicCollectionImpl_IDirectMusicCollection_Release (LPDIRECTMUSICCOLLECTION iface) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, CollectionVtbl, iface);
	return IDirectMusicCollectionImpl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

/* IDirectMusicCollection Interface follow: */
static HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicCollection_GetInstrument (LPDIRECTMUSICCOLLECTION iface, DWORD dwPatch, IDirectMusicInstrument** ppInstrument) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, CollectionVtbl, iface);
	DMUS_PRIVATE_INSTRUMENTENTRY *tmpEntry;
	struct list *listEntry;
	DWORD dwInstPatch;

	TRACE("(%p, %d, %p)\n", This, dwPatch, ppInstrument);
	
	LIST_FOR_EACH (listEntry, &This->Instruments) {
		tmpEntry = LIST_ENTRY(listEntry, DMUS_PRIVATE_INSTRUMENTENTRY, entry);
		IDirectMusicInstrument_GetPatch (tmpEntry->pInstrument, &dwInstPatch);
		if (dwPatch == dwInstPatch) {
			*ppInstrument = tmpEntry->pInstrument;
			IDirectMusicInstrument_AddRef (tmpEntry->pInstrument);
			IDirectMusicInstrumentImpl_Custom_Load (tmpEntry->pInstrument, This->pStm); /* load instrument before returning it */
			TRACE(": returning instrument %p\n", *ppInstrument);
			return S_OK;
		}
			
	}
	TRACE(": instrument not found\n");
	
	return DMUS_E_INVALIDPATCH;
}

static HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicCollection_EnumInstrument (LPDIRECTMUSICCOLLECTION iface, DWORD dwIndex, DWORD* pdwPatch, LPWSTR pwszName, DWORD dwNameLen) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, CollectionVtbl, iface);
	unsigned int r = 0;
	DMUS_PRIVATE_INSTRUMENTENTRY *tmpEntry;
	struct list *listEntry;
       DWORD dwLen;
		
	TRACE("(%p, %d, %p, %p, %d)\n", This, dwIndex, pdwPatch, pwszName, dwNameLen);
	LIST_FOR_EACH (listEntry, &This->Instruments) {
		tmpEntry = LIST_ENTRY(listEntry, DMUS_PRIVATE_INSTRUMENTENTRY, entry);
		if (r == dwIndex) {
			ICOM_NAME_MULTI (IDirectMusicInstrumentImpl, InstrumentVtbl, tmpEntry->pInstrument, pInstrument);
			IDirectMusicInstrument_GetPatch (tmpEntry->pInstrument, pdwPatch);
                       if (pwszName) {
                               dwLen = min(strlenW(pInstrument->wszName),dwNameLen-1);
                               memcpy (pwszName, pInstrument->wszName, dwLen * sizeof(WCHAR));
                               pwszName[dwLen] = '\0';
                       }
			return S_OK;
		}
		r++;		
	}
	
	return S_FALSE;
}

static const IDirectMusicCollectionVtbl DirectMusicCollection_Collection_Vtbl = {
	IDirectMusicCollectionImpl_IDirectMusicCollection_QueryInterface,
	IDirectMusicCollectionImpl_IDirectMusicCollection_AddRef,
	IDirectMusicCollectionImpl_IDirectMusicCollection_Release,
	IDirectMusicCollectionImpl_IDirectMusicCollection_GetInstrument,
	IDirectMusicCollectionImpl_IDirectMusicCollection_EnumInstrument
};

/* IDirectMusicCollectionImpl IDirectMusicObject part: */
static HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, ObjectVtbl, iface);
	return IDirectMusicCollectionImpl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicCollectionImpl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, ObjectVtbl, iface);
	return IDirectMusicCollectionImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicCollectionImpl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, ObjectVtbl, iface);
	return IDirectMusicCollectionImpl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, ObjectVtbl, iface);
	TRACE("(%p, %p)\n", This, pDesc);
	/* I think we shouldn't return pointer here since then values can be changed; it'd be a mess */
	memcpy (pDesc, This->pDesc, This->pDesc->dwSize);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, ObjectVtbl, iface);
	TRACE("(%p, %p): setting descriptor:\n%s\n", This, pDesc, debugstr_DMUS_OBJECTDESC (pDesc));

	/* According to MSDN, we should copy only given values, not whole struct */	
	if (pDesc->dwValidData & DMUS_OBJ_OBJECT)
		This->pDesc->guidObject = pDesc->guidObject;
	if (pDesc->dwValidData & DMUS_OBJ_CLASS)
		This->pDesc->guidClass = pDesc->guidClass;
	if (pDesc->dwValidData & DMUS_OBJ_NAME)
               lstrcpynW(This->pDesc->wszName, pDesc->wszName, DMUS_MAX_NAME);
	if (pDesc->dwValidData & DMUS_OBJ_CATEGORY)
               lstrcpynW(This->pDesc->wszCategory, pDesc->wszCategory, DMUS_MAX_CATEGORY);
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME)
               lstrcpynW(This->pDesc->wszFileName, pDesc->wszFileName, DMUS_MAX_FILENAME);
	if (pDesc->dwValidData & DMUS_OBJ_VERSION)
		This->pDesc->vVersion = pDesc->vVersion;
	if (pDesc->dwValidData & DMUS_OBJ_DATE)
		This->pDesc->ftDate = pDesc->ftDate;
	if (pDesc->dwValidData & DMUS_OBJ_MEMORY) {
		memcpy (&This->pDesc->llMemLength, &pDesc->llMemLength, sizeof (pDesc->llMemLength));				
		memcpy (This->pDesc->pbMemData, pDesc->pbMemData, sizeof (pDesc->pbMemData));
	}
	if (pDesc->dwValidData & DMUS_OBJ_STREAM) {
		/* according to MSDN, we copy the stream */
		IStream_Clone (pDesc->pStream, &This->pDesc->pStream);	
	}
	
	/* add new flags */
	This->pDesc->dwValidData |= pDesc->dwValidData;

	return S_OK;
}

static HRESULT WINAPI IDirectMusicCollectionImpl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, ObjectVtbl, iface);
	DMUS_PRIVATE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[1], ListCount[1];
	LARGE_INTEGER liMove; /* used when skipping chunks */

	TRACE("(%p, %p, %p)\n", This, pStream, pDesc);

	/* FIXME: should this be determined from stream? */
	pDesc->dwValidData |= DMUS_OBJ_CLASS;
	pDesc->guidClass = CLSID_DirectMusicCollection;

	IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStream, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
			StreamSize = Chunk.dwSize - sizeof(FOURCC);
			StreamCount = 0;
			if (Chunk.fccID == mmioFOURCC('D','L','S',' ')) {
				TRACE_(dmfile)(": collection form\n");
				do {
					IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
					StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
					TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
					switch (Chunk.fccID) {
						case FOURCC_DLID: {
							TRACE_(dmfile)(": GUID chunk\n");
							pDesc->dwValidData |= DMUS_OBJ_OBJECT;
							IStream_Read (pStream, &pDesc->guidObject, Chunk.dwSize, NULL);
							break;
						}
						case DMUS_FOURCC_VERSION_CHUNK: {
							TRACE_(dmfile)(": version chunk\n");
							pDesc->dwValidData |= DMUS_OBJ_VERSION;
							IStream_Read (pStream, &pDesc->vVersion, Chunk.dwSize, NULL);
							break;
						}
						case DMUS_FOURCC_CATEGORY_CHUNK: {
							TRACE_(dmfile)(": category chunk\n");
							pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
							IStream_Read (pStream, pDesc->wszCategory, Chunk.dwSize, NULL);
							break;
						}
						case FOURCC_LIST: {
							IStream_Read (pStream, &Chunk.fccID, sizeof(FOURCC), NULL);				
							TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
							ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
							ListCount[0] = 0;
							switch (Chunk.fccID) {
								/* pure INFO list, such can be found in dls collections */
								case mmioFOURCC('I','N','F','O'): {
									TRACE_(dmfile)(": INFO list\n");
									do {
										IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
										ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
										TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
										switch (Chunk.fccID) {
											case mmioFOURCC('I','N','A','M'):{
												CHAR szName[DMUS_MAX_NAME];												
												TRACE_(dmfile)(": name chunk\n");
												pDesc->dwValidData |= DMUS_OBJ_NAME;
												IStream_Read (pStream, szName, Chunk.dwSize, NULL);
												MultiByteToWideChar (CP_ACP, 0, szName, -1, pDesc->wszName, DMUS_MAX_NAME);
												if (even_or_odd(Chunk.dwSize)) {
													ListCount[0] ++;
													liMove.QuadPart = 1;
													IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												}
												break;
											}
											case mmioFOURCC('I','A','R','T'): {
												TRACE_(dmfile)(": artist chunk (ignored)\n");
												if (even_or_odd(Chunk.dwSize)) {
													ListCount[0] ++;
													Chunk.dwSize++;
												}
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;
											}
											case mmioFOURCC('I','C','O','P'): {
												TRACE_(dmfile)(": copyright chunk (ignored)\n");
												if (even_or_odd(Chunk.dwSize)) {
													ListCount[0] ++;
													Chunk.dwSize++;
												}
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;
											}
											case mmioFOURCC('I','S','B','J'): {
												TRACE_(dmfile)(": subject chunk (ignored)\n");
												if (even_or_odd(Chunk.dwSize)) {
													ListCount[0] ++;
													Chunk.dwSize++;
												}
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;
											}
											case mmioFOURCC('I','C','M','T'): {
												TRACE_(dmfile)(": comment chunk (ignored)\n");
												if (even_or_odd(Chunk.dwSize)) {
													ListCount[0] ++;
													Chunk.dwSize++;
												}
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;
											}
											default: {
												TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
												if (even_or_odd(Chunk.dwSize)) {
													ListCount[0] ++;
													Chunk.dwSize++;
												}
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;						
											}
										}
										TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
									} while (ListCount[0] < ListSize[0]);
									break;
								}
								default: {
									TRACE_(dmfile)(": unknown (skipping)\n");
									liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
									IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
									break;						
								}
							}
							break;
						}	
						default: {
							TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
							liMove.QuadPart = Chunk.dwSize;
							IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
							break;						
						}
					}
					TRACE_(dmfile)(": StreamCount[0] = %d < StreamSize[0] = %d\n", StreamCount, StreamSize);
				} while (StreamCount < StreamSize);
			} else {
				TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
				liMove.QuadPart = StreamSize;
				IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
				return E_FAIL;
			}
		
			TRACE_(dmfile)(": reading finished\n");
			break;
		}
		default: {
			TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
			liMove.QuadPart = Chunk.dwSize;
			IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return DMUS_E_INVALIDFILE;
		}
	}	
	
	TRACE(": returning descriptor:\n%s\n", debugstr_DMUS_OBJECTDESC (pDesc));
	
	return S_OK;
}

static const IDirectMusicObjectVtbl DirectMusicCollection_Object_Vtbl = {
	IDirectMusicCollectionImpl_IDirectMusicObject_QueryInterface,
	IDirectMusicCollectionImpl_IDirectMusicObject_AddRef,
	IDirectMusicCollectionImpl_IDirectMusicObject_Release,
	IDirectMusicCollectionImpl_IDirectMusicObject_GetDescriptor,
	IDirectMusicCollectionImpl_IDirectMusicObject_SetDescriptor,
	IDirectMusicCollectionImpl_IDirectMusicObject_ParseDescriptor
};

/* IDirectMusicCollectionImpl IPersistStream part: */
static HRESULT WINAPI IDirectMusicCollectionImpl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, PersistStreamVtbl, iface);
	return IDirectMusicCollectionImpl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicCollectionImpl_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, PersistStreamVtbl, iface);
	return IDirectMusicCollectionImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicCollectionImpl_IPersistStream_Release (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, PersistStreamVtbl, iface);
	return IDirectMusicCollectionImpl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicCollectionImpl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicCollectionImpl_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicCollectionImpl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	ICOM_THIS_MULTI(IDirectMusicCollectionImpl, PersistStreamVtbl, iface);

	DMUS_PRIVATE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[3], ListCount[3];
	LARGE_INTEGER liMove; /* used when skipping chunks */
	ULARGE_INTEGER dlibCollectionPosition, dlibInstrumentPosition, dlibWavePoolPosition;
	
	IStream_AddRef (pStm); /* add count for later references */
	liMove.QuadPart = 0;
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, &dlibCollectionPosition); /* store offset, in case it'll be needed later */
	This->liCollectionPosition.QuadPart = dlibCollectionPosition.QuadPart;
	This->pStm = pStm;
	
	IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
			StreamSize = Chunk.dwSize - sizeof(FOURCC);
			StreamCount = 0;
			switch (Chunk.fccID) {
				case FOURCC_DLS: {
					TRACE_(dmfile)(": collection form\n");
					do {
						IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
						StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
						TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
						switch (Chunk.fccID) {
							case FOURCC_COLH: {
								TRACE_(dmfile)(": collection header chunk\n");
								This->pHeader = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, Chunk.dwSize);
								IStream_Read (pStm, This->pHeader, Chunk.dwSize, NULL);
								break;								
							}
							case FOURCC_DLID: {
								TRACE_(dmfile)(": DLID (GUID) chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_OBJECT;
								IStream_Read (pStm, &This->pDesc->guidObject, Chunk.dwSize, NULL);
								break;
							}
							case FOURCC_VERS: {
								TRACE_(dmfile)(": version chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_VERSION;
								IStream_Read (pStm, &This->pDesc->vVersion, Chunk.dwSize, NULL);
								break;
							}
							case FOURCC_PTBL: {
								TRACE_(dmfile)(": pool table chunk\n");
								This->pPoolTable = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(POOLTABLE));
								IStream_Read (pStm, This->pPoolTable, sizeof(POOLTABLE), NULL);
								Chunk.dwSize -= sizeof(POOLTABLE);
								This->pPoolCues = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, This->pPoolTable->cCues*sizeof(POOLCUE));
								IStream_Read (pStm, This->pPoolCues, Chunk.dwSize, NULL);
								break;
							}
							case FOURCC_LIST: {
								IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
								TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
								ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
								ListCount[0] = 0;
								switch (Chunk.fccID) {
									case mmioFOURCC('I','N','F','O'): {
										TRACE_(dmfile)(": INFO list\n");
										do {
											IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
											ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
											TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
											switch (Chunk.fccID) {
												case mmioFOURCC('I','N','A','M'): {
													CHAR szName[DMUS_MAX_NAME];
													TRACE_(dmfile)(": name chunk\n");
													This->pDesc->dwValidData |= DMUS_OBJ_NAME;
													IStream_Read (pStm, szName, Chunk.dwSize, NULL);
													MultiByteToWideChar (CP_ACP, 0, szName, -1, This->pDesc->wszName, DMUS_MAX_NAME);
													if (even_or_odd(Chunk.dwSize)) {
														ListCount[0] ++;
														liMove.QuadPart = 1;
														IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													}
													break;
												}
												case mmioFOURCC('I','A','R','T'): {
													TRACE_(dmfile)(": artist chunk (ignored)\n");
													if (even_or_odd(Chunk.dwSize)) {
														ListCount[0] ++;
														Chunk.dwSize++;
													}
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','O','P'): {
													TRACE_(dmfile)(": copyright chunk\n");
													This->szCopyright = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, Chunk.dwSize);
													IStream_Read (pStm, This->szCopyright, Chunk.dwSize, NULL);
													if (even_or_odd(Chunk.dwSize)) {
														ListCount[0] ++;
														liMove.QuadPart = 1;
														IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													}
													break;
												}
												case mmioFOURCC('I','S','B','J'): {
													TRACE_(dmfile)(": subject chunk (ignored)\n");
													if (even_or_odd(Chunk.dwSize)) {
														ListCount[0] ++;
														Chunk.dwSize++;
													}
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','M','T'): {
													TRACE_(dmfile)(": comment chunk (ignored)\n");
													if (even_or_odd(Chunk.dwSize)) {
														ListCount[0] ++;
														Chunk.dwSize++;
													}
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												default: {
													TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
													if (even_or_odd(Chunk.dwSize)) {
														ListCount[0] ++;
														Chunk.dwSize++;
													}
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;						
												}
											}
											TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
										} while (ListCount[0] < ListSize[0]);
										break;
									}
									case FOURCC_WVPL: {
										TRACE_(dmfile)(": wave pool list (mark & skip)\n");
										liMove.QuadPart = 0;
										IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, &dlibWavePoolPosition); /* store position */
										This->liWavePoolTablePosition.QuadPart = dlibWavePoolPosition.QuadPart;
										liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
										IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
										break;	
									}
									case FOURCC_LINS: {
										TRACE_(dmfile)(": instruments list\n");
										do {
											IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
											ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
											TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
											switch (Chunk.fccID) {
												case FOURCC_LIST: {
													IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
													TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
													ListSize[1] = Chunk.dwSize - sizeof(FOURCC);
													ListCount[1] = 0;													
													switch (Chunk.fccID) {
														case FOURCC_INS: {
															LPDMUS_PRIVATE_INSTRUMENTENTRY pNewInstrument = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_INSTRUMENTENTRY));
															TRACE_(dmfile)(": instrument list\n");
															DMUSIC_CreateDirectMusicInstrumentImpl (&IID_IDirectMusicInstrument, (LPVOID*)&pNewInstrument->pInstrument, NULL); /* only way to create this one... even M$ does it discretely */
                                                                                                                        {
                                                                                                                            ICOM_NAME_MULTI (IDirectMusicInstrumentImpl, InstrumentVtbl, pNewInstrument->pInstrument, pInstrument);
                                                                                                                            liMove.QuadPart = 0;
                                                                                                                            IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, &dlibInstrumentPosition);
                                                                                                                            pInstrument->liInstrumentPosition.QuadPart = dlibInstrumentPosition.QuadPart - (2*sizeof(FOURCC) + sizeof(DWORD)); /* store offset, it'll be needed later */

                                                                                                                            do {
																IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
																ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
																TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
																switch (Chunk.fccID) {
																	case FOURCC_INSH: {
																		TRACE_(dmfile)(": instrument header chunk\n");
																		pInstrument->pHeader = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, Chunk.dwSize);
																		IStream_Read (pStm, pInstrument->pHeader, Chunk.dwSize, NULL);
																		break;	
																	}
																	case FOURCC_DLID: {
																		TRACE_(dmfile)(": DLID (GUID) chunk\n");
																		pInstrument->pInstrumentID = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, Chunk.dwSize);
																		IStream_Read (pStm, pInstrument->pInstrumentID, Chunk.dwSize, NULL);
																		break;
																	}
																	case FOURCC_LIST: {
																		IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
																		TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
																		ListSize[2] = Chunk.dwSize - sizeof(FOURCC);
																		ListCount[2] = 0;
																		switch (Chunk.fccID) {
																			default: {
																				TRACE_(dmfile)(": unknown (skipping)\n");
																				liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
																				IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
																				break;						
																			}
																		}
																		break;
																	}				
																	default: {
																		TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
																		liMove.QuadPart = Chunk.dwSize;
																		IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
																		break;						
																	}
																}
																TRACE_(dmfile)(": ListCount[1] = %d < ListSize[1] = %d\n", ListCount[1], ListSize[1]);
                                                                                                                            } while (ListCount[1] < ListSize[1]);
                                                                                                                            /* DEBUG: dumps whole instrument object tree: */
                                                                                                                            if (TRACE_ON(dmusic)) {
																TRACE("*** IDirectMusicInstrument (%p) ***\n", pInstrument);
																if (pInstrument->pInstrumentID)
																	TRACE(" - GUID = %s\n", debugstr_dmguid(pInstrument->pInstrumentID));
																
																TRACE(" - Instrument header:\n");
																TRACE("    - cRegions: %d\n", pInstrument->pHeader->cRegions);
																TRACE("    - Locale:\n");
																TRACE("       - ulBank: %d\n", pInstrument->pHeader->Locale.ulBank);
																TRACE("       - ulInstrument: %d\n", pInstrument->pHeader->Locale.ulInstrument);
																TRACE("       => dwPatch: %d\n", MIDILOCALE2Patch(&pInstrument->pHeader->Locale));
                                                                                                                            }
                                                                                                                            list_add_tail (&This->Instruments, &pNewInstrument->entry);
                                                                                                                        }
															break;
														}
													}
													break;
												}
												default: {
													TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;						
												}
											}
											TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
										} while (ListCount[0] < ListSize[0]);
										break;
									}
									default: {
										TRACE_(dmfile)(": unknown (skipping)\n");
										liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
										IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
										break;						
									}
								}
								break;
							}	
							default: {
								TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
								liMove.QuadPart = Chunk.dwSize;
								IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
								break;						
							}
						}
						TRACE_(dmfile)(": StreamCount = %d < StreamSize = %d\n", StreamCount, StreamSize);
					} while (StreamCount < StreamSize);
					break;
				}
				default: {
					TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
					liMove.QuadPart = StreamSize;
					IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
					return E_FAIL;
				}
			}
			TRACE_(dmfile)(": reading finished\n");
			break;
		}
		default: {
			TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
			liMove.QuadPart = Chunk.dwSize;
			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return E_FAIL;
		}
	}

	/* DEBUG: dumps whole collection object tree: */
	if (TRACE_ON(dmusic)) {
		int r = 0;
		DMUS_PRIVATE_INSTRUMENTENTRY *tmpEntry;
		struct list *listEntry;

		TRACE("*** IDirectMusicCollection (%p) ***\n", This->CollectionVtbl);
		if (This->pDesc->dwValidData & DMUS_OBJ_OBJECT)
			TRACE(" - GUID = %s\n", debugstr_dmguid(&This->pDesc->guidObject));
		if (This->pDesc->dwValidData & DMUS_OBJ_VERSION)
			TRACE(" - Version = %i,%i,%i,%i\n", (This->pDesc->vVersion.dwVersionMS >> 8) & 0x0000FFFF, This->pDesc->vVersion.dwVersionMS & 0x0000FFFF,
				(This->pDesc->vVersion.dwVersionLS >> 8) & 0x0000FFFF, This->pDesc->vVersion.dwVersionLS & 0x0000FFFF);
		if (This->pDesc->dwValidData & DMUS_OBJ_NAME)
			TRACE(" - Name = %s\n", debugstr_w(This->pDesc->wszName));
		
		TRACE(" - Collection header:\n");
		TRACE("    - cInstruments: %d\n", This->pHeader->cInstruments);
		TRACE(" - Instruments:\n");
		
		LIST_FOR_EACH (listEntry, &This->Instruments) {
			tmpEntry = LIST_ENTRY( listEntry, DMUS_PRIVATE_INSTRUMENTENTRY, entry );
			TRACE("    - Instrument[%i]: %p\n", r, tmpEntry->pInstrument);
			r++;
		}
	}
	
	return S_OK;
}

static HRESULT WINAPI IDirectMusicCollectionImpl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
	return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicCollectionImpl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
	return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicCollection_PersistStream_Vtbl = {
	IDirectMusicCollectionImpl_IPersistStream_QueryInterface,
	IDirectMusicCollectionImpl_IPersistStream_AddRef,
	IDirectMusicCollectionImpl_IPersistStream_Release,
	IDirectMusicCollectionImpl_IPersistStream_GetClassID,
	IDirectMusicCollectionImpl_IPersistStream_IsDirty,
	IDirectMusicCollectionImpl_IPersistStream_Load,
	IDirectMusicCollectionImpl_IPersistStream_Save,
	IDirectMusicCollectionImpl_IPersistStream_GetSizeMax
};


/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicCollectionImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicCollectionImpl* obj;
	
	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicCollectionImpl));
	if (NULL == obj) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	obj->UnknownVtbl = &DirectMusicCollection_Unknown_Vtbl;
	obj->CollectionVtbl = &DirectMusicCollection_Collection_Vtbl;
	obj->ObjectVtbl = &DirectMusicCollection_Object_Vtbl;
	obj->PersistStreamVtbl = &DirectMusicCollection_PersistStream_Vtbl;
	obj->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(obj->pDesc);
	obj->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	obj->pDesc->guidClass = CLSID_DirectMusicCollection;
	obj->ref = 0; /* will be inited by QueryInterface */
	list_init (&obj->Instruments);

	return IDirectMusicCollectionImpl_IUnknown_QueryInterface ((LPUNKNOWN)&obj->UnknownVtbl, lpcGUID, ppobj);
}
