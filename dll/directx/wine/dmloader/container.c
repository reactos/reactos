/* IDirectMusicContainerImpl
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

#include "dmloader_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmloader);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

#define DMUS_MAX_CATEGORY_SIZE DMUS_MAX_CATEGORY*sizeof(WCHAR)
#define DMUS_MAX_NAME_SIZE     DMUS_MAX_NAME*sizeof(WCHAR)
#define DMUS_MAX_FILENAME_SIZE DMUS_MAX_FILENAME*sizeof(WCHAR)

typedef struct riff_chunk {
    FOURCC fccID;
    DWORD dwSize;
} WINE_CHUNK;

/* contained object entry */
typedef struct container_entry {
    struct list entry;
    DMUS_OBJECTDESC Desc;
    BOOL bIsRIFF;
    DWORD dwFlags; /* DMUS_CONTAINED_OBJF_KEEP: keep object in loader's cache, even when container is released */
    WCHAR *wszAlias;
    IDirectMusicObject *pObject; /* needed when releasing from loader's cache on container release */
} WINE_CONTAINER_ENTRY, *LPWINE_CONTAINER_ENTRY;

/*****************************************************************************
 * IDirectMusicContainerImpl implementation
 */
typedef struct IDirectMusicContainerImpl {
    IDirectMusicContainer IDirectMusicContainer_iface;
    struct dmobject dmobj;
    LONG ref;
    IStream *pStream;
    DMUS_IO_CONTAINER_HEADER Header; /* header */
    struct list *pContainedObjects;  /* data */
} IDirectMusicContainerImpl;

static inline IDirectMusicContainerImpl *impl_from_IDirectMusicContainer(IDirectMusicContainer *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicContainerImpl, IDirectMusicContainer_iface);
}

static HRESULT destroy_dmcontainer(IDirectMusicContainerImpl *This)
{
	LPDIRECTMUSICLOADER pLoader;
	LPDIRECTMUSICGETLOADER pGetLoader;
	struct list *pEntry;
	LPWINE_CONTAINER_ENTRY pContainedObject;

	/* get loader (from stream we loaded from) */
	TRACE(": getting loader\n");
	IStream_QueryInterface (This->pStream, &IID_IDirectMusicGetLoader, (LPVOID*)&pGetLoader);
	IDirectMusicGetLoader_GetLoader (pGetLoader, &pLoader);
	IDirectMusicGetLoader_Release (pGetLoader);

	/* release objects from loader's cache (if appropriate) */
	TRACE(": releasing objects from loader's cache\n");
	LIST_FOR_EACH (pEntry, This->pContainedObjects) {
		pContainedObject = LIST_ENTRY (pEntry, WINE_CONTAINER_ENTRY, entry);
		/* my tests indicate that container releases objects *only*
		   if they were loaded at its load-time (makes sense, it doesn't
		   have pointers to objects otherwise); BTW: native container seems
		   to ignore the flags (I won't) */
		if (pContainedObject->pObject && !(pContainedObject->dwFlags & DMUS_CONTAINED_OBJF_KEEP)) {
			/* flags say it shouldn't be kept in loader's cache */
			IDirectMusicLoader_ReleaseObject (pLoader, pContainedObject->pObject);
		}
	}
	IDirectMusicLoader_Release (pLoader);

	/* release stream we loaded from */
	IStream_Release (This->pStream);

	/* FIXME: release allocated entries */

	return S_OK;
}

static HRESULT WINAPI IDirectMusicContainerImpl_QueryInterface(IDirectMusicContainer *iface, REFIID riid, void **ret_iface)
{
    IDirectMusicContainerImpl *This = impl_from_IDirectMusicContainer(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID (riid, &IID_IUnknown) || IsEqualIID (riid, &IID_IDirectMusicContainer))
        *ret_iface = &This->IDirectMusicContainer_iface;
    else if (IsEqualIID (riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID (riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else {
        WARN("Unknown interface %s\n", debugstr_dmguid(riid));
        *ret_iface = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI IDirectMusicContainerImpl_AddRef(IDirectMusicContainer *iface)
{
    IDirectMusicContainerImpl *This = impl_from_IDirectMusicContainer(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicContainerImpl_Release(IDirectMusicContainer *iface)
{
    IDirectMusicContainerImpl *This = impl_from_IDirectMusicContainer(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        if (This->pStream)
            destroy_dmcontainer(This);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IDirectMusicContainerImpl_EnumObject(IDirectMusicContainer *iface,
        REFGUID rguidClass, DWORD dwIndex, DMUS_OBJECTDESC *pDesc, WCHAR *pwszAlias)
{
	IDirectMusicContainerImpl *This = impl_from_IDirectMusicContainer(iface);
	struct list *pEntry;
	LPWINE_CONTAINER_ENTRY pContainedObject;
	DWORD dwCount = 0;

	TRACE("(%p, %s, %ld, %p, %p)\n", This, debugstr_dmguid(rguidClass), dwIndex, pDesc, pwszAlias);

	if (!pDesc)
		return E_POINTER;
	if (pDesc->dwSize != sizeof(DMUS_OBJECTDESC)) {
		ERR(": invalid pDesc->dwSize %ld\n", pDesc->dwSize);
		return E_INVALIDARG;
	}

	DM_STRUCT_INIT(pDesc);

	LIST_FOR_EACH (pEntry, This->pContainedObjects) {
		pContainedObject = LIST_ENTRY (pEntry, WINE_CONTAINER_ENTRY, entry);
		
		if (IsEqualGUID (rguidClass, &GUID_DirectMusicAllTypes) || IsEqualGUID (rguidClass, &pContainedObject->Desc.guidClass)) {
			if (dwCount == dwIndex) {
				HRESULT result = S_OK;
				if (pwszAlias) {
					lstrcpynW (pwszAlias, pContainedObject->wszAlias, DMUS_MAX_FILENAME);
					if (lstrlenW (pContainedObject->wszAlias) > DMUS_MAX_FILENAME)
						result = DMUS_S_STRING_TRUNCATED;
				}
				*pDesc = pContainedObject->Desc;
				return result;
			}
			dwCount++;
		}
	}		
	
	TRACE(": not found\n");
	return S_FALSE;
}

static const IDirectMusicContainerVtbl dmcontainer_vtbl = {
    IDirectMusicContainerImpl_QueryInterface,
    IDirectMusicContainerImpl_AddRef,
    IDirectMusicContainerImpl_Release,
    IDirectMusicContainerImpl_EnumObject
};

/* IDirectMusicObject part: */
static HRESULT WINAPI cont_IDirectMusicObject_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream)
        return E_POINTER;
    if (!desc || desc->dwSize != sizeof(*desc))
        return E_INVALIDARG;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || riff.type != DMUS_FOURCC_CONTAINER_FORM) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_DESCEND_CHUNK_FAIL;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc,
            DMUS_OBJ_OBJECT|DMUS_OBJ_CLASS|DMUS_OBJ_NAME|DMUS_OBJ_CATEGORY|DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicContainer;
    desc->dwValidData |= DMUS_OBJ_CLASS;

    TRACE("returning descriptor:\n");
    dump_DMUS_OBJECTDESC (desc);
    return S_OK;
}

static const IDirectMusicObjectVtbl dmobject_vtbl = {
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    cont_IDirectMusicObject_ParseDescriptor
};

/* IPersistStream part: */
static inline IDirectMusicContainerImpl *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicContainerImpl, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI IPersistStreamImpl_Load(IPersistStream *iface, IStream *pStm)
{
        IDirectMusicContainerImpl *This = impl_from_IPersistStream(iface);
	WINE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[3], ListCount[3];
	LARGE_INTEGER liMove; /* used when skipping chunks */
	ULARGE_INTEGER uliPos; /* needed when dealing with RIFF chunks */
	LPDIRECTMUSICGETLOADER pGetLoader;
	LPDIRECTMUSICLOADER pLoader;
	HRESULT result = S_OK;

	TRACE("(%p, %p):\n", This, pStm);
	
	/* check whether pStm is valid read pointer */
	if (IsBadReadPtr (pStm, sizeof(LPVOID))) {
		ERR(": pStm bad read pointer\n");
		return E_POINTER;
	}
	/* if stream is already set, this means the container is already loaded */
	if (This->pStream) {
		TRACE(": stream is already set, which means container is already loaded\n");
		return DMUS_E_ALREADY_LOADED;
	}

	/* get loader since it will be needed later */
	if (FAILED(IStream_QueryInterface (pStm, &IID_IDirectMusicGetLoader, (LPVOID*)&pGetLoader))) {
		ERR(": stream not supported\n");
		return DMUS_E_UNSUPPORTED_STREAM;
	}
	IDirectMusicGetLoader_GetLoader (pGetLoader, &pLoader);
	IDirectMusicGetLoader_Release (pGetLoader);
	
	This->pStream = pStm;
	IStream_AddRef (pStm); /* add count for later references */
	
	/* start with load */
	IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = %#lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
			StreamSize = Chunk.dwSize - sizeof(FOURCC);
			StreamCount = 0;
			switch (Chunk.fccID) {
				case DMUS_FOURCC_CONTAINER_FORM: {
					TRACE_(dmfile)(": container form\n");
					This->dmobj.desc.guidClass = CLSID_DirectMusicContainer;
					This->dmobj.desc.dwValidData |= DMUS_OBJ_CLASS;
					do {
						IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
						StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
						TRACE_(dmfile)(": %s chunk (size = %#lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
						switch (Chunk.fccID) {
							case DMUS_FOURCC_CONTAINER_CHUNK: {
								IStream_Read (pStm, &This->Header, Chunk.dwSize, NULL);
								TRACE_(dmfile)(": container header chunk:\n%s\n", debugstr_DMUS_IO_CONTAINER_HEADER(&This->Header));
								break;	
							}
							case DMUS_FOURCC_GUID_CHUNK: {
								TRACE_(dmfile)(": GUID chunk\n");
								IStream_Read (pStm, &This->dmobj.desc.guidObject, Chunk.dwSize, NULL);
								This->dmobj.desc.dwValidData |= DMUS_OBJ_OBJECT;
								break;
							}
							case DMUS_FOURCC_VERSION_CHUNK: {
								TRACE_(dmfile)(": version chunk\n");
								IStream_Read (pStm, &This->dmobj.desc.vVersion, Chunk.dwSize, NULL);
								This->dmobj.desc.dwValidData |= DMUS_OBJ_VERSION;
								break;
							}
							case DMUS_FOURCC_DATE_CHUNK: {
								TRACE_(dmfile)(": date chunk\n");
								IStream_Read (pStm, &This->dmobj.desc.ftDate, Chunk.dwSize, NULL);
								This->dmobj.desc.dwValidData |= DMUS_OBJ_DATE;
								break;
							}
							case DMUS_FOURCC_CATEGORY_CHUNK: {
								TRACE_(dmfile)(": category chunk\n");
								/* if it happens that string is too long,
								   read what we can and skip the rest*/
								if (Chunk.dwSize > DMUS_MAX_CATEGORY_SIZE) {
									IStream_Read (pStm, This->dmobj.desc.wszCategory, DMUS_MAX_CATEGORY_SIZE, NULL);
									liMove.QuadPart = Chunk.dwSize - DMUS_MAX_CATEGORY_SIZE;
									IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
								} else {
									IStream_Read (pStm, This->dmobj.desc.wszCategory, Chunk.dwSize, NULL);
								}
								This->dmobj.desc.dwValidData |= DMUS_OBJ_CATEGORY;
								break;
							}
							case FOURCC_LIST: {
								IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
								TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
								ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
								ListCount[0] = 0;
								switch (Chunk.fccID) {
									case DMUS_FOURCC_UNFO_LIST: {
										TRACE_(dmfile)(": UNFO list\n");
										do {
											IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
											ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
											TRACE_(dmfile)(": %s chunk (size = %#lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
											switch (Chunk.fccID) {
												/* don't ask me why, but M$ puts INFO elements in UNFO list sometimes
                                              (though strings seem to be valid unicode) */
												case mmioFOURCC('I','N','A','M'):
												case DMUS_FOURCC_UNAM_CHUNK: {
													TRACE_(dmfile)(": name chunk\n");
													/* if it happens that string is too long,
													   read what we can and skip the rest*/
													if (Chunk.dwSize > DMUS_MAX_NAME_SIZE) {
														IStream_Read (pStm, This->dmobj.desc.wszName, DMUS_MAX_NAME_SIZE, NULL);
														liMove.QuadPart = Chunk.dwSize - DMUS_MAX_NAME_SIZE;
														IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													} else {
														IStream_Read (pStm, This->dmobj.desc.wszName, Chunk.dwSize, NULL);
													}
													This->dmobj.desc.dwValidData |= DMUS_OBJ_NAME;
													break;
												}
												default: {
													TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;						
												}
											}
											TRACE_(dmfile)(": ListCount[0] = %#lx < ListSize[0] = %#lx\n", ListCount[0], ListSize[0]);
										} while (ListCount[0] < ListSize[0]);
										break;
									}
									case DMUS_FOURCC_CONTAINED_OBJECTS_LIST: {
										TRACE_(dmfile)(": contained objects list\n");
										do {
											IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
											ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
											TRACE_(dmfile)(": %s chunk (size = %#lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
											switch (Chunk.fccID) {
												case FOURCC_LIST: {
													IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
													TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
													ListSize[1] = Chunk.dwSize - sizeof(FOURCC);
													ListCount[1] = 0;
													switch (Chunk.fccID) {
														case DMUS_FOURCC_CONTAINED_OBJECT_LIST: {
															LPWINE_CONTAINER_ENTRY pNewEntry;
															TRACE_(dmfile)(": contained object list\n");
															pNewEntry = calloc(1, sizeof(*pNewEntry));
															DM_STRUCT_INIT(&pNewEntry->Desc);
															do {
																IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
																ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
																TRACE_(dmfile)(": %s chunk (size = %#lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
																switch (Chunk.fccID) {
																	case DMUS_FOURCC_CONTAINED_ALIAS_CHUNK: {
																		TRACE_(dmfile)(": alias chunk\n");
																		pNewEntry->wszAlias = calloc(1, Chunk.dwSize);
																		IStream_Read (pStm, pNewEntry->wszAlias, Chunk.dwSize, NULL);
																		TRACE_(dmfile)(": alias: %s\n", debugstr_w(pNewEntry->wszAlias));
																		break;
																	}
																	case DMUS_FOURCC_CONTAINED_OBJECT_CHUNK: {
																		DMUS_IO_CONTAINED_OBJECT_HEADER tmpObjectHeader;
																		IStream_Read (pStm, &tmpObjectHeader, Chunk.dwSize, NULL);
																		TRACE_(dmfile)(": contained object header:\n%s\n", debugstr_DMUS_IO_CONTAINED_OBJECT_HEADER(&tmpObjectHeader));
																		/* copy guidClass */
																		pNewEntry->Desc.dwValidData |= DMUS_OBJ_CLASS;
																		pNewEntry->Desc.guidClass = tmpObjectHeader.guidClassID;
																		/* store flags */
																		pNewEntry->dwFlags = tmpObjectHeader.dwFlags;
																		break;
																	}
																	/* now read data... it may be safe to read everything after object header chunk, 
																		but I'm not comfortable with MSDN's "the header is *normally* followed by ..." */
																	case FOURCC_LIST: {
																		IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
																		TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
																		ListSize[2] = Chunk.dwSize - sizeof(FOURCC);
																		ListCount[2] = 0;
																		switch (Chunk.fccID) {
																			case DMUS_FOURCC_REF_LIST: {
																				TRACE_(dmfile)(": reference list\n");
																				pNewEntry->bIsRIFF = 0;
																				do {
																					IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
																					ListCount[2] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
																					TRACE_(dmfile)(": %s chunk (size = %#lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
																					switch (Chunk.fccID) {
																						case DMUS_FOURCC_REF_CHUNK: {
																							DMUS_IO_REFERENCE tmpReferenceHeader; /* temporary structure */
																							TRACE_(dmfile)(": reference header chunk\n");
																							memset (&tmpReferenceHeader, 0, sizeof(DMUS_IO_REFERENCE));
																							IStream_Read (pStm, &tmpReferenceHeader, Chunk.dwSize, NULL);
																							/* copy retrieved data to DMUS_OBJECTDESC */
																							if (!IsEqualCLSID (&pNewEntry->Desc.guidClass, &tmpReferenceHeader.guidClassID)) ERR(": object header declares different CLSID than reference header?\n");
																							/* it shouldn't be necessary to copy guidClass, since it was set in contained object header already...
																							   yet if they happen to be different, I'd rather stick to this one */
																							pNewEntry->Desc.guidClass = tmpReferenceHeader.guidClassID;
																							pNewEntry->Desc.dwValidData |= tmpReferenceHeader.dwValidData;
																							break;																	
																						}
																						case DMUS_FOURCC_GUID_CHUNK: {
																							TRACE_(dmfile)(": guid chunk\n");
																							/* no need to set flags since they were copied from reference header */
																							IStream_Read (pStm, &pNewEntry->Desc.guidObject, Chunk.dwSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_DATE_CHUNK: {
																							TRACE_(dmfile)(": file date chunk\n");
																							/* no need to set flags since they were copied from reference header */
																							IStream_Read (pStm, &pNewEntry->Desc.ftDate, Chunk.dwSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_NAME_CHUNK: {
																							TRACE_(dmfile)(": name chunk\n");
																							/* no need to set flags since they were copied from reference header */
																							IStream_Read (pStm, pNewEntry->Desc.wszName, Chunk.dwSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_FILE_CHUNK: {
																							TRACE_(dmfile)(": file name chunk\n");
																							/* no need to set flags since they were copied from reference header */
																							IStream_Read (pStm, pNewEntry->Desc.wszFileName, Chunk.dwSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_CATEGORY_CHUNK: {
																							TRACE_(dmfile)(": category chunk\n");
																							/* no need to set flags since they were copied from reference header */
																							IStream_Read (pStm, pNewEntry->Desc.wszCategory, Chunk.dwSize, NULL);
																							break;
																						}
																						case DMUS_FOURCC_VERSION_CHUNK: {
																							TRACE_(dmfile)(": version chunk\n");
																							/* no need to set flags since they were copied from reference header */
																							IStream_Read (pStm, &pNewEntry->Desc.vVersion, Chunk.dwSize, NULL);
																							break;
																						}
																						default: {
																							TRACE_(dmfile)(": unknown chunk (skipping)\n");
																							liMove.QuadPart = Chunk.dwSize;
																							IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip this chunk */
																							break;
																						}
																					}
																					TRACE_(dmfile)(": ListCount[2] = %#lx < ListSize[2] = %#lx\n", ListCount[2], ListSize[2]);
																				} while (ListCount[2] < ListSize[2]);
																				break;
																			}
																			default: {
																				TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
																				return E_FAIL;
																			}
																		}
																		break;
																	}
																	
																	case FOURCC_RIFF: {
																		IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);
																		TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
																		if (IS_VALID_DMFORM (Chunk.fccID)) {
																			TRACE_(dmfile)(": valid DMUSIC form\n");
																			pNewEntry->bIsRIFF = 1;
																			/* we'll have to skip whole RIFF chunk after SetObject is called */
																			liMove.QuadPart = 0;
																			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, &uliPos);
																			uliPos.QuadPart += (Chunk.dwSize - sizeof(FOURCC)); /* set uliPos at the end of RIFF chunk */																			
																			/* move at the beginning of RIFF chunk */
																			liMove.QuadPart = 0;
																			liMove.QuadPart -= (sizeof(FOURCC)+sizeof(DWORD)+sizeof(FOURCC));
																			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
																			/* put pointer to stream in descriptor */
																			pNewEntry->Desc.dwValidData |= DMUS_OBJ_STREAM;
																			pNewEntry->Desc.pStream = pStm; /* we don't have to worry about cloning, since SetObject will perform it */
																			/* wait till we get on the end of object list */
																		} else {
																			TRACE_(dmfile)(": invalid DMUSIC form (skipping)\n");
																			liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
																			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
																			/* FIXME: should we return E_FAIL? */
																		}
																		break;
																	}
																	default: {
																		TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
																		liMove.QuadPart = Chunk.dwSize;
																		IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
																		break;						
																	}
																}
																TRACE_(dmfile)(": ListCount[1] = %#lx < ListSize[1] = %#lx\n", ListCount[1], ListSize[1]);
															} while (ListCount[1] < ListSize[1]);
															/* SetObject: this will fill descriptor with additional info	and add alias in loader's cache */
															IDirectMusicLoader_SetObject (pLoader, &pNewEntry->Desc);
															/* now that SetObject collected appropriate info into descriptor we can live happily ever after;
															   or not, since we have to clean evidence of loading through stream... *sigh*
															   and we have to skip rest of the chunk, if we loaded through RIFF */
															if (pNewEntry->bIsRIFF) {
																liMove.QuadPart = uliPos.QuadPart;
																IStream_Seek (pStm, liMove, STREAM_SEEK_SET, NULL);
																pNewEntry->Desc.dwValidData &= ~DMUS_OBJ_STREAM; /* clear flag (and with bitwise complement) */
																pNewEntry->Desc.pStream = NULL;											
															}
															/* add entry to list of objects */
															list_add_tail (This->pContainedObjects, &pNewEntry->entry);
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
													TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;						
												}
											}
											TRACE_(dmfile)(": ListCount[0] = %#lx < ListSize[0] = %#lx\n", ListCount[0], ListSize[0]);
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
								TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
								liMove.QuadPart = Chunk.dwSize;
								IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
								break;						
							}
						}
						TRACE_(dmfile)(": StreamCount[0] = %#lx < StreamSize[0] = %#lx\n", StreamCount, StreamSize);
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
			This->dmobj.desc.dwValidData |= DMUS_OBJ_LOADED;
                        dump_DMUS_OBJECTDESC(&This->dmobj.desc);
			break;
		}
		default: {
			TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
			liMove.QuadPart = Chunk.dwSize;
			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return E_FAIL;
		}
	}
	
	/* now, if DMUS_CONTAINER_NOLOADS is not set, we are supposed to load contained objects;
	   so when we call GetObject later, they'll already be in cache */
	if (!(This->Header.dwFlags & DMUS_CONTAINER_NOLOADS)) {
		struct list *pEntry;
		LPWINE_CONTAINER_ENTRY pContainedObject;

		TRACE(": DMUS_CONTAINER_NOLOADS not set... load all objects\n");
		
		LIST_FOR_EACH (pEntry, This->pContainedObjects) {
			IDirectMusicObject* pObject;
			pContainedObject = LIST_ENTRY (pEntry, WINE_CONTAINER_ENTRY, entry);		
			/* get object from loader and then release it */
			if (SUCCEEDED(IDirectMusicLoader_GetObject (pLoader, &pContainedObject->Desc, &IID_IDirectMusicObject, (LPVOID*)&pObject))) {
				pContainedObject->pObject = pObject; /* for final release */
				IDirectMusicObject_Release (pObject); /* we don't really need this one */
			} else {
				WARN(": failed to load contained object\n");
				result = DMUS_S_PARTIALLOAD;
			}
		}
	}
	
	IDirectMusicLoader_Release (pLoader); /* release loader */

	return result;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    IPersistStreamImpl_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmcontainer(REFIID lpcGUID, void **ppobj)
{
	IDirectMusicContainerImpl* obj;
    HRESULT hr;

	*ppobj = NULL;
	if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicContainer_iface.lpVtbl = &dmcontainer_vtbl;
    obj->ref = 1;
    dmobject_init(&obj->dmobj, &CLSID_DirectMusicContainer,
                    (IUnknown*)&obj->IDirectMusicContainer_iface);
    obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
    obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
	obj->pContainedObjects = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(struct list));
	list_init (obj->pContainedObjects);

    hr = IDirectMusicContainer_QueryInterface(&obj->IDirectMusicContainer_iface, lpcGUID, ppobj);
    IDirectMusicContainer_Release(&obj->IDirectMusicContainer_iface);

    return hr;
}
