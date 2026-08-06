/* IDirectMusicScript
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2003-2004 Raphael Junqueira
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


#include <stdio.h>

#include "dmscript_private.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmscript);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);


/*****************************************************************************
 * IDirectMusicScriptImpl implementation
 */
typedef struct IDirectMusicScriptImpl {
    IDirectMusicScript IDirectMusicScript_iface;
    struct dmobject dmobj;
    LONG ref;
    IDirectMusicPerformance *pPerformance;
    DMUS_IO_SCRIPT_HEADER *pHeader;
    DMUS_IO_VERSION *pVersion;
    WCHAR *pwzLanguage;
    WCHAR *pwzSource;
} IDirectMusicScriptImpl;

static inline IDirectMusicScriptImpl *impl_from_IDirectMusicScript(IDirectMusicScript *iface)
{
  return CONTAINING_RECORD(iface, IDirectMusicScriptImpl, IDirectMusicScript_iface);
}

static HRESULT WINAPI IDirectMusicScriptImpl_QueryInterface(IDirectMusicScript *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicScript))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI IDirectMusicScriptImpl_AddRef(IDirectMusicScript *iface)
{
    IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicScriptImpl_Release(IDirectMusicScript *iface)
{
    IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        free(This->pHeader);
        free(This->pVersion);
        free(This->pwzLanguage);
        free(This->pwzSource);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IDirectMusicScriptImpl_Init(IDirectMusicScript *iface,
        IDirectMusicPerformance *pPerformance, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %p, %p): stub\n", This, pPerformance, pErrorInfo);
  This->pPerformance = pPerformance;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_CallRoutine(IDirectMusicScript *iface,
        WCHAR *pwszRoutineName, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %p): stub\n", This, debugstr_w(pwszRoutineName), pErrorInfo);
  /*return E_NOTIMPL;*/
  return S_OK;
  /*return E_FAIL;*/
}

static HRESULT WINAPI IDirectMusicScriptImpl_SetVariableVariant(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, VARIANT varValue, BOOL fSetRef, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, FIXME, %d, %p): stub\n", This, debugstr_w(pwszVariableName),/* varValue,*/ fSetRef, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_GetVariableVariant(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, VARIANT *pvarValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %p, %p): stub\n", This, debugstr_w(pwszVariableName), pvarValue, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_SetVariableNumber(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, LONG lValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %li, %p): stub\n", This, debugstr_w(pwszVariableName), lValue, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_GetVariableNumber(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, LONG *plValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %p, %p): stub\n", This, debugstr_w(pwszVariableName), plValue, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_SetVariableObject(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, IUnknown *punkValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %p, %p): stub\n", This, debugstr_w(pwszVariableName), punkValue, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_GetVariableObject(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, REFIID riid, void **ppv, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %s, %p, %p): stub\n", This, debugstr_w(pwszVariableName), debugstr_dmguid(riid), ppv, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_EnumRoutine(IDirectMusicScript *iface, DWORD dwIndex,
        WCHAR *pwszName)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_EnumVariable(IDirectMusicScript *iface, DWORD dwIndex,
        WCHAR *pwszName)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);
  return S_OK;
}

static const IDirectMusicScriptVtbl dmscript_vtbl = {
    IDirectMusicScriptImpl_QueryInterface,
    IDirectMusicScriptImpl_AddRef,
    IDirectMusicScriptImpl_Release,
    IDirectMusicScriptImpl_Init,
    IDirectMusicScriptImpl_CallRoutine,
    IDirectMusicScriptImpl_SetVariableVariant,
    IDirectMusicScriptImpl_GetVariableVariant,
    IDirectMusicScriptImpl_SetVariableNumber,
    IDirectMusicScriptImpl_GetVariableNumber,
    IDirectMusicScriptImpl_SetVariableObject,
    IDirectMusicScriptImpl_GetVariableObject,
    IDirectMusicScriptImpl_EnumRoutine,
    IDirectMusicScriptImpl_EnumVariable
};

static HRESULT WINAPI script_IDirectMusicObject_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream || !desc)
        return E_POINTER;

     if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || riff.type != DMUS_FOURCC_SCRIPT_FORM) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_SCRIPT_INVALID_FILE;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc,
            DMUS_OBJ_OBJECT|DMUS_OBJ_NAME|DMUS_OBJ_NAME_INAM|DMUS_OBJ_CATEGORY|DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    if (desc->dwValidData) {
        desc->guidClass = CLSID_DirectMusicScript;
        desc->dwValidData |= DMUS_OBJ_CLASS;
    }

    TRACE("returning descriptor:\n");
    dump_DMUS_OBJECTDESC(desc);
    return S_OK;
}

static const IDirectMusicObjectVtbl dmobject_vtbl = {
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    script_IDirectMusicObject_ParseDescriptor
};

/* IDirectMusicScriptImpl IPersistStream part: */
static inline IDirectMusicScriptImpl *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicScriptImpl, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI IPersistStreamImpl_Load(IPersistStream *iface, IStream *pStm)
{
        IDirectMusicScriptImpl *This = impl_from_IPersistStream(iface);
	DMUS_PRIVATE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[3], ListCount[3];
	LARGE_INTEGER liMove; /* used when skipping chunks */
	LPDIRECTMUSICGETLOADER pGetLoader = NULL;
	LPDIRECTMUSICLOADER pLoader = NULL;

	FIXME("(%p, %p): Loading not implemented yet\n", This, pStm);
	IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
			StreamSize = Chunk.dwSize - sizeof(FOURCC);
			StreamCount = 0;
			switch (Chunk.fccID) {
				case DMUS_FOURCC_SCRIPT_FORM: {
					TRACE_(dmfile)(": script form\n");
					do {
						IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
						StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
						TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
						switch (Chunk.fccID) { 
						        case DMUS_FOURCC_SCRIPT_CHUNK: {
							        TRACE_(dmfile)(": script header chunk\n");
								This->pHeader = calloc(1, Chunk.dwSize);
								IStream_Read (pStm, This->pHeader, Chunk.dwSize, NULL);
								break;
						        }
						        case DMUS_FOURCC_SCRIPTVERSION_CHUNK: {
							        TRACE_(dmfile)(": script version chunk\n");
								This->pVersion = calloc(1, Chunk.dwSize);
								IStream_Read (pStm, This->pVersion, Chunk.dwSize, NULL); 
								TRACE_(dmfile)("version: 0x%08lx.0x%08lx\n", This->pVersion->dwVersionMS, This->pVersion->dwVersionLS);
								break;
						        }
						        case DMUS_FOURCC_SCRIPTLANGUAGE_CHUNK: {
							        TRACE_(dmfile)(": script language chunk\n");
								This->pwzLanguage = calloc(1, Chunk.dwSize);
								IStream_Read (pStm, This->pwzLanguage, Chunk.dwSize, NULL); 
								TRACE_(dmfile)("using language: %s\n", debugstr_w(This->pwzLanguage));
								break;
						        }
						        case DMUS_FOURCC_SCRIPTSOURCE_CHUNK: {
							        TRACE_(dmfile)(": script source chunk\n");
								This->pwzSource = calloc(1, Chunk.dwSize);
								IStream_Read (pStm, This->pwzSource, Chunk.dwSize, NULL); 
								if (TRACE_ON(dmscript)) {
								    int count = WideCharToMultiByte(CP_ACP, 0, This->pwzSource, -1, NULL, 0, NULL, NULL);
								    LPSTR str = malloc(count);
								    WideCharToMultiByte(CP_ACP, 0, This->pwzSource, -1, str, count, NULL, NULL);
								    str[count-1] = '\n';
								    TRACE("source:\n");
								    fwrite( str, 1, count, stderr );
								    free(str);
								}
								break;
						        }
							case DMUS_FOURCC_GUID_CHUNK: {
								TRACE_(dmfile)(": GUID chunk\n");
								This->dmobj.desc.dwValidData |= DMUS_OBJ_OBJECT;
								IStream_Read (pStm, &This->dmobj.desc.guidObject, Chunk.dwSize, NULL);
								break;
							}
							case DMUS_FOURCC_VERSION_CHUNK: {
								TRACE_(dmfile)(": version chunk\n");
								This->dmobj.desc.dwValidData |= DMUS_OBJ_VERSION;
								IStream_Read (pStm, &This->dmobj.desc.vVersion, Chunk.dwSize, NULL);
								break;
							}
							case DMUS_FOURCC_CATEGORY_CHUNK: {
								TRACE_(dmfile)(": category chunk\n");
								This->dmobj.desc.dwValidData |= DMUS_OBJ_CATEGORY;
								IStream_Read (pStm, This->dmobj.desc.wszCategory, Chunk.dwSize, NULL);
								break;
							}
						        case FOURCC_RIFF: {
								IDirectMusicObject* pObject = NULL;
								DMUS_OBJECTDESC desc;

								ZeroMemory (&desc, sizeof(DMUS_OBJECTDESC));
								desc.dwSize = sizeof(DMUS_OBJECTDESC);
								desc.dwValidData = DMUS_OBJ_STREAM | DMUS_OBJ_CLASS;
								desc.guidClass = CLSID_DirectMusicContainer;
								desc.pStream = NULL;
								IStream_Clone (pStm, &desc.pStream);

								liMove.QuadPart = 0;
								liMove.QuadPart -= (sizeof(FOURCC) + sizeof(DWORD));
								IStream_Seek (desc.pStream, liMove, STREAM_SEEK_CUR, NULL);

								IStream_QueryInterface (pStm, &IID_IDirectMusicGetLoader, (LPVOID*)&pGetLoader);
								IDirectMusicGetLoader_GetLoader (pGetLoader, &pLoader);
								IDirectMusicGetLoader_Release (pGetLoader);

								if (SUCCEEDED(IDirectMusicLoader_GetObject (pLoader, &desc, &IID_IDirectMusicObject, (LPVOID*) &pObject))) {
								  IDirectMusicObject_Release (pObject);
								} else {
								  ERR_(dmfile)("Error on GetObject while trying to load Scrip SubContainer\n");
								}

								IDirectMusicLoader_Release (pLoader); pLoader = NULL; /* release loader */
								IStream_Release(desc.pStream); desc.pStream = NULL; /* release cloned stream */

								liMove.QuadPart = Chunk.dwSize;
								IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
								/*
							        IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
								TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
								ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
								ListCount[0] = 0;

								switch (Chunk.fccID) {
								        default: {
										TRACE_(dmfile)(": unknown (skipping)\n");
										liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
										IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
										break;						
									}
								}
								*/
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
											TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
											switch (Chunk.fccID) {
												/* don't ask me why, but M$ puts INFO elements in UNFO list sometimes
                                              (though strings seem to be valid unicode) */
												case mmioFOURCC('I','N','A','M'):
												case DMUS_FOURCC_UNAM_CHUNK: {
													TRACE_(dmfile)(": name chunk\n");
													This->dmobj.desc.dwValidData |= DMUS_OBJ_NAME;
													IStream_Read (pStm, This->dmobj.desc.wszName, Chunk.dwSize, NULL);
													break;
												}
												case mmioFOURCC('I','A','R','T'):
												case DMUS_FOURCC_UART_CHUNK: {
													TRACE_(dmfile)(": artist chunk (ignored)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','O','P'):
												case DMUS_FOURCC_UCOP_CHUNK: {
													TRACE_(dmfile)(": copyright chunk (ignored)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','S','B','J'):
												case DMUS_FOURCC_USBJ_CHUNK: {
													TRACE_(dmfile)(": subject chunk (ignored)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','M','T'):
												case DMUS_FOURCC_UCMT_CHUNK: {
													TRACE_(dmfile)(": comment chunk (ignored)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												default: {
													TRACE_(dmfile)(": unknown sub-chunk (irrelevant & skipping)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;						
												}
											}
											TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
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
						TRACE_(dmfile)(": StreamCount[0] = %ld < StreamSize[0] = %ld\n", StreamCount, StreamSize);
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

	return S_OK;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    unimpl_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    IPersistStreamImpl_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT DMUSIC_CreateDirectMusicScriptImpl(REFIID lpcGUID, void **ppobj, IUnknown *pUnkOuter)
{
  IDirectMusicScriptImpl *obj;
  HRESULT hr;

  *ppobj = NULL;

  if (pUnkOuter)
    return CLASS_E_NOAGGREGATION;

  if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
  obj->IDirectMusicScript_iface.lpVtbl = &dmscript_vtbl;
  obj->ref = 1;
  dmobject_init(&obj->dmobj, &CLSID_DirectMusicScript, (IUnknown*)&obj->IDirectMusicScript_iface);
  obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
  obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

  hr = IDirectMusicScript_QueryInterface(&obj->IDirectMusicScript_iface, lpcGUID, ppobj);
  IDirectMusicScript_Release(&obj->IDirectMusicScript_iface);

  return hr;
}
