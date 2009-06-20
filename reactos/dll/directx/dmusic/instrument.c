/* IDirectMusicInstrument Implementation
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

static const GUID IID_IDirectMusicInstrumentPRIVATE = {0xbcb20080,0xa40c,0x11d1,{0x86,0xbc,0x00,0xc0,0x4f,0xbf,0x8f,0xef}};

static ULONG WINAPI IDirectMusicInstrumentImpl_IUnknown_AddRef (LPUNKNOWN iface);
static ULONG WINAPI IDirectMusicInstrumentImpl_IDirectMusicInstrument_AddRef (LPDIRECTMUSICINSTRUMENT iface);

/* IDirectMusicInstrument IUnknown part: */
static HRESULT WINAPI IDirectMusicInstrumentImpl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicInstrumentImpl, UnknownVtbl, iface);
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);
	
	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = &This->UnknownVtbl;
		IDirectMusicInstrumentImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;	
	} else if (IsEqualIID (riid, &IID_IDirectMusicInstrument)) {
		*ppobj = &This->InstrumentVtbl;
		IDirectMusicInstrumentImpl_IDirectMusicInstrument_AddRef ((LPDIRECTMUSICINSTRUMENT)&This->InstrumentVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicInstrumentPRIVATE)) {	
		/* it seems to me that this interface is only basic IUnknown, without any
			other inherited functions... *sigh* this is the worst scenario, since it means 
			that whoever calls it knows the layout of original implementation table and therefore
			tries to get data by direct access... expect crashes */
		FIXME("*sigh*... requested private/unspecified interface\n");
		*ppobj = &This->UnknownVtbl;
		IDirectMusicInstrumentImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;	
	}
	
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicInstrumentImpl_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicInstrumentImpl, UnknownVtbl, iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

	DMUSIC_LockModule();

	return refCount;
}

static ULONG WINAPI IDirectMusicInstrumentImpl_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicInstrumentImpl, UnknownVtbl, iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

	if (!refCount) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMUSIC_UnlockModule();
	
	return refCount;
}

static const IUnknownVtbl DirectMusicInstrument_Unknown_Vtbl = {
	IDirectMusicInstrumentImpl_IUnknown_QueryInterface,
	IDirectMusicInstrumentImpl_IUnknown_AddRef,
	IDirectMusicInstrumentImpl_IUnknown_Release
};

/* IDirectMusicInstrumentImpl IDirectMusicInstrument part: */
static HRESULT WINAPI IDirectMusicInstrumentImpl_IDirectMusicInstrument_QueryInterface (LPDIRECTMUSICINSTRUMENT iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicInstrumentImpl, InstrumentVtbl, iface);
	return IDirectMusicInstrumentImpl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

static ULONG WINAPI IDirectMusicInstrumentImpl_IDirectMusicInstrument_AddRef (LPDIRECTMUSICINSTRUMENT iface) {
	ICOM_THIS_MULTI(IDirectMusicInstrumentImpl, InstrumentVtbl, iface);
	return IDirectMusicInstrumentImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

static ULONG WINAPI IDirectMusicInstrumentImpl_IDirectMusicInstrument_Release (LPDIRECTMUSICINSTRUMENT iface) {
	ICOM_THIS_MULTI(IDirectMusicInstrumentImpl, InstrumentVtbl, iface);
	return IDirectMusicInstrumentImpl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

static HRESULT WINAPI IDirectMusicInstrumentImpl_IDirectMusicInstrument_GetPatch (LPDIRECTMUSICINSTRUMENT iface, DWORD* pdwPatch) {
	ICOM_THIS_MULTI(IDirectMusicInstrumentImpl, InstrumentVtbl, iface);
	TRACE("(%p, %p)\n", This, pdwPatch);	
	*pdwPatch = MIDILOCALE2Patch(&This->pHeader->Locale);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicInstrumentImpl_IDirectMusicInstrument_SetPatch (LPDIRECTMUSICINSTRUMENT iface, DWORD dwPatch) {
	ICOM_THIS_MULTI(IDirectMusicInstrumentImpl, InstrumentVtbl, iface);
	TRACE("(%p, %d): stub\n", This, dwPatch);
	Patch2MIDILOCALE(dwPatch, &This->pHeader->Locale);
	return S_OK;
}

static const IDirectMusicInstrumentVtbl DirectMusicInstrument_Instrument_Vtbl = {
	IDirectMusicInstrumentImpl_IDirectMusicInstrument_QueryInterface,
	IDirectMusicInstrumentImpl_IDirectMusicInstrument_AddRef,
	IDirectMusicInstrumentImpl_IDirectMusicInstrument_Release,
	IDirectMusicInstrumentImpl_IDirectMusicInstrument_GetPatch,
	IDirectMusicInstrumentImpl_IDirectMusicInstrument_SetPatch
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicInstrumentImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicInstrumentImpl* dminst;
	
	dminst = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicInstrumentImpl));
	if (NULL == dminst) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	dminst->UnknownVtbl = &DirectMusicInstrument_Unknown_Vtbl;
	dminst->InstrumentVtbl = &DirectMusicInstrument_Instrument_Vtbl;
	dminst->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicInstrumentImpl_IUnknown_QueryInterface ((LPUNKNOWN)&dminst->UnknownVtbl, lpcGUID, ppobj);
}

/* aux. function that completely loads instrument; my tests indicate that it's 
   called somewhere around IDirectMusicCollection_GetInstrument */
HRESULT WINAPI IDirectMusicInstrumentImpl_Custom_Load (LPDIRECTMUSICINSTRUMENT iface, LPSTREAM pStm) {
	ICOM_THIS_MULTI(IDirectMusicInstrumentImpl, InstrumentVtbl, iface);
	
	DMUS_PRIVATE_CHUNK Chunk;
	DWORD ListSize[4], ListCount[4];
	LARGE_INTEGER liMove; /* used when skipping chunks */
	
	TRACE("(%p, %p, offset = %s)\n", This, pStm, wine_dbgstr_longlong(This->liInstrumentPosition.QuadPart));

	/* goto the beginning of chunk */
	IStream_Seek (pStm, This->liInstrumentPosition, STREAM_SEEK_SET, NULL);
	
	IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {
		case FOURCC_LIST: {
			IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
			ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
			ListCount[0] = 0;
			switch (Chunk.fccID) {
				case FOURCC_INS: {
					TRACE_(dmfile)(": instrument list\n");
					do {
						IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
						ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
						TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
						switch (Chunk.fccID) {
							case FOURCC_INSH: {
								TRACE_(dmfile)(": instrument header chunk\n");
								/* should be already initialised */
								IStream_Read (pStm, This->pHeader, Chunk.dwSize, NULL);
								break;	
							}
							case FOURCC_DLID: {
								TRACE_(dmfile)(": DLID (GUID) chunk\n");
								/* should be already initialised */
								IStream_Read (pStm, This->pInstrumentID, Chunk.dwSize, NULL);
								break;
							}
							case FOURCC_LIST: {
								IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
								TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
								ListSize[1] = Chunk.dwSize - sizeof(FOURCC);
								ListCount[1] = 0;
								switch (Chunk.fccID) {
									case FOURCC_LRGN: {
										TRACE_(dmfile)(": regions list\n");
										do {
											IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
											ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
											TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
											switch (Chunk.fccID) {
												case FOURCC_LIST: {
													IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
													TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
													ListSize[2] = Chunk.dwSize - sizeof(FOURCC);
													ListCount[2] = 0;
													switch (Chunk.fccID) {
														case FOURCC_RGN: {																
															/* temporary structures */
															RGNHEADER tmpRegionHeader;
															WSMPL tmpWaveSample;
															WLOOP tmpWaveLoop;
															WAVELINK tmpWaveLink;
															
															TRACE_(dmfile)(": region list\n");
															do {
																IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
																ListCount[2] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
																TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
																switch (Chunk.fccID) {
																	case FOURCC_RGNH: {
																		TRACE_(dmfile)(": region header chunk\n");
																		memset (&tmpRegionHeader, 0, sizeof(RGNHEADER)); /* reset */
																		IStream_Read (pStm, &tmpRegionHeader, Chunk.dwSize, NULL);
																		break;
																	}
																	case FOURCC_WSMP: {
																		TRACE_(dmfile)(": wave sample chunk\n");
																		memset (&tmpWaveSample, 0, sizeof(WSMPL)); /* reset */
																		memset (&tmpWaveLoop, 0, sizeof(WLOOP)); /* reset */
																		if (Chunk.dwSize != (sizeof(WSMPL) + sizeof(WLOOP))) ERR(": incorrect chunk size\n");
																		IStream_Read (pStm, &tmpWaveSample, sizeof(WSMPL), NULL);
																		IStream_Read (pStm, &tmpWaveLoop, sizeof(WLOOP), NULL);
																		break;
																	}
																	case FOURCC_WLNK: {
																		TRACE_(dmfile)(": wave link chunk\n");
																		memset (&tmpWaveLink, 0, sizeof(WAVELINK)); /* reset */
																		IStream_Read (pStm, &tmpWaveLink, Chunk.dwSize, NULL);
																		break;
																	}
																	default: {
																		TRACE_(dmfile)(": unknown (skipping)\n");
																		liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
																		IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
																		break;						
																	}
																}
																TRACE_(dmfile)(": ListCount[2] = %d < ListSize[2] = %d\n", ListCount[2], ListSize[2]);
															} while (ListCount[2] < ListSize[2]);
															FIXME(": need to write temporary data to instrument data\n");
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
											TRACE_(dmfile)(": ListCount[1] = %d < ListSize[1] = %d\n", ListCount[1], ListSize[1]);
										} while (ListCount[1] < ListSize[1]);
										break;
									}
									case FOURCC_LART: {
										TRACE_(dmfile)(": articulators list\n");
										do {
											IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
											ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
											TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
											switch (Chunk.fccID) {
												case FOURCC_ART1: {
													/* temporary structures */
													CONNECTIONLIST tmpConnectionList;
													LPCONNECTION tmpConnections;
													
													TRACE_(dmfile)(": level 1 articulator chunk\n");
													memset (&tmpConnectionList, 0, sizeof(CONNECTIONLIST)); /* reset */
													tmpConnections = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(CONNECTION)*tmpConnectionList.cConnections);
													if (Chunk.dwSize != (sizeof(CONNECTIONLIST) + sizeof(CONNECTION)*tmpConnectionList.cConnections)) ERR(": incorrect chunk size\n");
													IStream_Read (pStm, &tmpConnectionList, sizeof(CONNECTIONLIST), NULL);
													IStream_Read (pStm, tmpConnections, sizeof(CONNECTION)*tmpConnectionList.cConnections, NULL);
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
										break;
									}
									case mmioFOURCC('I','N','F','O'): {
										TRACE_(dmfile)(": INFO list\n");
										do {
											IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
											ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
											TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
											switch (Chunk.fccID) {
												case mmioFOURCC('I','N','A','M'): {
													TRACE_(dmfile)(": name chunk (ignored)\n");
													if (even_or_odd(Chunk.dwSize)) {
														ListCount[1] ++;
														Chunk.dwSize++;
													}
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','A','R','T'): {
													TRACE_(dmfile)(": artist chunk (ignored)\n");
													if (even_or_odd(Chunk.dwSize)) {
														ListCount[1] ++;
														Chunk.dwSize++;
													}
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','O','P'): {
													/* temporary structures */
													CHAR tmpCopyright[DMUS_MAX_NAME];
													
													TRACE_(dmfile)(": copyright chunk\n");
													IStream_Read (pStm, tmpCopyright, Chunk.dwSize, NULL);
													if (even_or_odd(Chunk.dwSize)) {
														ListCount[1] ++;
														liMove.QuadPart = 1;
														IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													}
													break;
												}
												case mmioFOURCC('I','S','B','J'): {
													TRACE_(dmfile)(": subject chunk (ignored)\n");
													if (even_or_odd(Chunk.dwSize)) {
														ListCount[1] ++;
														Chunk.dwSize++;
													}
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','M','T'): {
													TRACE_(dmfile)(": comment chunk (ignored)\n");
													if (even_or_odd(Chunk.dwSize)) {
														ListCount[1] ++;
														Chunk.dwSize++;
													}
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												default: {
													TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
													if (even_or_odd(Chunk.dwSize)) {
														ListCount[1] ++;
														Chunk.dwSize++;
													}
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;						
												}
											}
											TRACE_(dmfile)(": ListCount[1] = %d < ListSize[1] = %d\n", ListCount[1], ListSize[1]);
										} while (ListCount[1] < ListSize[1]);
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
						TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
					} while (ListCount[0] < ListSize[0]);
					break;
				}
				default: {
					TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
					liMove.QuadPart = Chunk.dwSize;
					IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
					break;						
				}
			}
			break;
		}
		default: {
			TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
			liMove.QuadPart = Chunk.dwSize;
			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return E_FAIL;
		}
	}
	/* DEBUG: dumps whole instrument object tree: */
/*	if (TRACE_ON(dmusic)) {		
		TRACE("*** IDirectMusicInstrument (%p) ***\n", This);
		if (This->pInstrumentID)
			TRACE(" - GUID = %s\n", debugstr_dmguid(This->pInstrumentID));
		
		TRACE(" - Instrument header:\n");
		TRACE("    - cRegions: %ld\n", This->pHeader->cRegions);
		TRACE("    - Locale:\n");
		TRACE("       - ulBank: %ld\n", This->pHeader->Locale.ulBank);
		TRACE("       - ulInstrument: %ld\n", This->pHeader->Locale.ulInstrument);
		TRACE("       => dwPatch: %ld\n", MIDILOCALE2Patch(&This->pHeader->Locale));		
	}*/

	return S_OK;
}
