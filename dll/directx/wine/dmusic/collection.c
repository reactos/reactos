/*
 * IDirectMusicCollection Implementation
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
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicCollectionImpl implementation
 */
typedef struct IDirectMusicCollectionImpl {
    IDirectMusicCollection IDirectMusicCollection_iface;
    struct dmobject dmobj;
    LONG ref;
    /* IDirectMusicCollectionImpl fields */
    IStream *pStm; /* stream from which we load collection and later instruments */
    LARGE_INTEGER liCollectionPosition; /* offset in a stream where collection was loaded from */
    LARGE_INTEGER liWavePoolTablePosition; /* offset in a stream where wave pool table can be found */
    CHAR *szCopyright; /* FIXME: should probably be placed somewhere else */
    DLSHEADER *pHeader;
    /* pool table */
    POOLTABLE *pPoolTable;
    POOLCUE *pPoolCues;
    /* instruments */
    struct list Instruments;
} IDirectMusicCollectionImpl;

static inline IDirectMusicCollectionImpl *impl_from_IDirectMusicCollection(IDirectMusicCollection *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicCollectionImpl, IDirectMusicCollection_iface);
}

static inline struct dmobject *impl_from_IDirectMusicObject(IDirectMusicObject *iface)
{
    return CONTAINING_RECORD(iface, struct dmobject, IDirectMusicObject_iface);
}

static inline IDirectMusicCollectionImpl *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicCollectionImpl, dmobj.IPersistStream_iface);
}

/* IDirectMusicCollectionImpl IUnknown part: */
static HRESULT WINAPI IDirectMusicCollectionImpl_QueryInterface(IDirectMusicCollection *iface,
        REFIID riid, void **ret_iface)
{
    IDirectMusicCollectionImpl *This = impl_from_IDirectMusicCollection(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicCollection))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else
    {
        WARN("(%p/%p)->(%s, %p): not found\n", iface, This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI IDirectMusicCollectionImpl_AddRef(IDirectMusicCollection *iface)
{
    IDirectMusicCollectionImpl *This = impl_from_IDirectMusicCollection(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %u\n", iface, This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicCollectionImpl_Release(IDirectMusicCollection *iface)
{
    IDirectMusicCollectionImpl *This = impl_from_IDirectMusicCollection(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %u\n", iface, This, ref);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
        DMUSIC_UnlockModule();
    }

    return ref;
}

/* IDirectMusicCollection Interface follows: */
static HRESULT WINAPI IDirectMusicCollectionImpl_GetInstrument(IDirectMusicCollection *iface,
        DWORD patch, IDirectMusicInstrument **instrument)
{
    IDirectMusicCollectionImpl *This = impl_from_IDirectMusicCollection(iface);
    DMUS_PRIVATE_INSTRUMENTENTRY *inst_entry;
    struct list *list_entry;
    DWORD inst_patch;

    TRACE("(%p/%p)->(%u, %p)\n", iface, This, patch, instrument);

    LIST_FOR_EACH(list_entry, &This->Instruments) {
        inst_entry = LIST_ENTRY(list_entry, DMUS_PRIVATE_INSTRUMENTENTRY, entry);
        IDirectMusicInstrument_GetPatch(inst_entry->pInstrument, &inst_patch);
        if (patch == inst_patch) {
            *instrument = inst_entry->pInstrument;
            IDirectMusicInstrument_AddRef(inst_entry->pInstrument);
            IDirectMusicInstrumentImpl_CustomLoad(inst_entry->pInstrument, This->pStm);
            TRACE(": returning instrument %p\n", *instrument);
            return S_OK;
        }
    }

    TRACE(": instrument not found\n");

    return DMUS_E_INVALIDPATCH;
}

static HRESULT WINAPI IDirectMusicCollectionImpl_EnumInstrument(IDirectMusicCollection *iface,
        DWORD index, DWORD *patch, LPWSTR name, DWORD name_length)
{
    IDirectMusicCollectionImpl *This = impl_from_IDirectMusicCollection(iface);
    DWORD i = 0;
    DMUS_PRIVATE_INSTRUMENTENTRY *inst_entry;
    struct list *list_entry;
    DWORD length;

    TRACE("(%p/%p)->(%d, %p, %p, %d)\n", iface, This, index, patch, name, name_length);

    LIST_FOR_EACH(list_entry, &This->Instruments) {
        inst_entry = LIST_ENTRY(list_entry, DMUS_PRIVATE_INSTRUMENTENTRY, entry);
        if (i == index) {
            IDirectMusicInstrumentImpl *instrument = impl_from_IDirectMusicInstrument(inst_entry->pInstrument);
            IDirectMusicInstrument_GetPatch(inst_entry->pInstrument, patch);
            if (name) {
                length = min(strlenW(instrument->wszName), name_length - 1);
                memcpy(name, instrument->wszName, length * sizeof(WCHAR));
                name[length] = '\0';
            }
            return S_OK;
        }
        i++;
    }

    return S_FALSE;
}

static const IDirectMusicCollectionVtbl DirectMusicCollection_Collection_Vtbl = {
    IDirectMusicCollectionImpl_QueryInterface,
    IDirectMusicCollectionImpl_AddRef,
    IDirectMusicCollectionImpl_Release,
    IDirectMusicCollectionImpl_GetInstrument,
    IDirectMusicCollectionImpl_EnumInstrument
};

/* IDirectMusicCollectionImpl IDirectMusicObject part: */
static HRESULT read_from_stream(IStream *stream, void *data, ULONG size)
{
    ULONG read;
    HRESULT hr;

    hr = IStream_Read(stream, data, size, &read);
    if (FAILED(hr)) {
        TRACE("IStream_Read failed: %08x\n", hr);
        return hr;
    }
    if (read < size) {
        TRACE("Didn't read full chunk: %u < %u\n", read, size);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI IDirectMusicObjectImpl_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct dmobject *This = impl_from_IDirectMusicObject(iface);
    DMUS_PRIVATE_CHUNK chunk;
    DWORD StreamSize, StreamCount, ListSize[1], ListCount[1];
    LARGE_INTEGER liMove; /* used when skipping chunks */
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", This, stream, desc);

    /* FIXME: should this be determined from stream? */
    desc->dwValidData |= DMUS_OBJ_CLASS;
    desc->guidClass = This->desc.guidClass;

    hr = read_from_stream(stream, &chunk, sizeof(FOURCC) + sizeof(DWORD));
    if (FAILED(hr))
        return hr;
    TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc(chunk.fccID), chunk.dwSize);

    if (chunk.fccID != FOURCC_RIFF) {
        TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
        liMove.QuadPart = chunk.dwSize;
        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
        return DMUS_E_INVALIDFILE;
    }

    hr = read_from_stream(stream, &chunk.fccID, sizeof(FOURCC));
    if (FAILED(hr))
        return hr;
    TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(chunk.fccID));
    StreamSize = chunk.dwSize - sizeof(FOURCC);

    if (chunk.fccID != FOURCC_DLS) {
        TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
        liMove.QuadPart = StreamSize;
        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
        return E_FAIL;
    }

    StreamCount = 0;
    TRACE_(dmfile)(": collection form\n");

    do {
        hr = read_from_stream(stream, &chunk, sizeof(FOURCC) + sizeof(DWORD));
        if (FAILED(hr))
            return hr;
        StreamCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.dwSize;
        TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc(chunk.fccID), chunk.dwSize);
        switch (chunk.fccID) {
            case FOURCC_DLID:
                TRACE_(dmfile)(": GUID chunk\n");
                desc->dwValidData |= DMUS_OBJ_OBJECT;
                hr = read_from_stream(stream, &desc->guidObject, chunk.dwSize);
                if (FAILED(hr))
                    return hr;
                break;

            case DMUS_FOURCC_VERSION_CHUNK:
                TRACE_(dmfile)(": version chunk\n");
                desc->dwValidData |= DMUS_OBJ_VERSION;
                hr = read_from_stream(stream, &desc->vVersion, chunk.dwSize);
                if (FAILED(hr))
                    return hr;
                break;

            case DMUS_FOURCC_CATEGORY_CHUNK:
                TRACE_(dmfile)(": category chunk\n");
                desc->dwValidData |= DMUS_OBJ_CATEGORY;
                hr = read_from_stream(stream, desc->wszCategory, chunk.dwSize);
                if (FAILED(hr))
                    return hr;
                break;

            case FOURCC_LIST:
                hr = read_from_stream(stream, &chunk.fccID, sizeof(FOURCC));
                if (FAILED(hr))
                    return hr;
                TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunk.fccID));
                ListSize[0] = chunk.dwSize - sizeof(FOURCC);
                ListCount[0] = 0;
                switch (chunk.fccID) {
                    /* pure INFO list, such can be found in dls collections */
                    case DMUS_FOURCC_INFO_LIST:
                        TRACE_(dmfile)(": INFO list\n");
                        do {
                            hr = read_from_stream(stream, &chunk, sizeof(FOURCC) + sizeof(DWORD));
                            if (FAILED(hr))
                                return hr;
                            ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + chunk.dwSize;
                            TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc(chunk.fccID), chunk.dwSize);
                            switch (chunk.fccID) {
                                case mmioFOURCC('I','N','A','M'): {
                                    CHAR szName[DMUS_MAX_NAME];
                                    TRACE_(dmfile)(": name chunk\n");
                                    desc->dwValidData |= DMUS_OBJ_NAME;
                                    hr = read_from_stream(stream, szName, chunk.dwSize);
                                    if (FAILED(hr))
                                        return hr;
                                    MultiByteToWideChar (CP_ACP, 0, szName, -1, desc->wszName, DMUS_MAX_NAME);
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        liMove.QuadPart = 1;
                                        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    }
                                    break;
                                }

                                case mmioFOURCC('I','A','R','T'):
                                    TRACE_(dmfile)(": artist chunk (ignored)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        chunk.dwSize++;
                                    }
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;

                                case mmioFOURCC('I','C','O','P'):
                                    TRACE_(dmfile)(": copyright chunk (ignored)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        chunk.dwSize++;
                                    }
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;

                                case mmioFOURCC('I','S','B','J'):
                                    TRACE_(dmfile)(": subject chunk (ignored)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        chunk.dwSize++;
                                    }
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;

                                case mmioFOURCC('I','C','M','T'):
                                    TRACE_(dmfile)(": comment chunk (ignored)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        chunk.dwSize++;
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;
                                }

                                default:
                                    TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0] ++;
                                        chunk.dwSize++;
                                    }
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;
                            }
                            TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
                         } while (ListCount[0] < ListSize[0]);
                         break;

                     default:
                         TRACE_(dmfile)(": unknown (skipping)\n");
                         liMove.QuadPart = chunk.dwSize - sizeof(FOURCC);
                         IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                         break;
                 }
                 break;

            default:
                TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
                liMove.QuadPart = chunk.dwSize;
                IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                break;
        }
        TRACE_(dmfile)(": StreamCount[0] = %d < StreamSize[0] = %d\n", StreamCount, StreamSize);
    } while (StreamCount < StreamSize);

    TRACE_(dmfile)(": reading finished\n");

    if (TRACE_ON(dmusic)) {
        TRACE("Returning descriptor:\n");
        dump_DMUS_OBJECTDESC(desc);
    }

    return S_OK;
}

static const IDirectMusicObjectVtbl dmobject_vtbl = {
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    IDirectMusicObjectImpl_ParseDescriptor
};

/* IDirectMusicCollectionImpl IPersistStream part: */
static HRESULT WINAPI IPersistStreamImpl_Load(IPersistStream *iface,
        IStream *stream)
{
    IDirectMusicCollectionImpl *This = impl_from_IPersistStream(iface);
    DMUS_PRIVATE_CHUNK chunk;
    DWORD StreamSize, StreamCount, ListSize[2], ListCount[2];
    LARGE_INTEGER liMove; /* used when skipping chunks */
    ULARGE_INTEGER dlibCollectionPosition, dlibInstrumentPosition, dlibWavePoolPosition;

    IStream_AddRef(stream); /* add count for later references */
    liMove.QuadPart = 0;
    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, &dlibCollectionPosition); /* store offset, in case it'll be needed later */
    This->liCollectionPosition.QuadPart = dlibCollectionPosition.QuadPart;
    This->pStm = stream;

    IStream_Read(stream, &chunk, sizeof(FOURCC) + sizeof(DWORD), NULL);
    TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc(chunk.fccID), chunk.dwSize);

    if (chunk.fccID != FOURCC_RIFF) {
        TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
        liMove.QuadPart = chunk.dwSize;
        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
        return E_FAIL;
    }

    IStream_Read(stream, &chunk.fccID, sizeof(FOURCC), NULL);
    TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(chunk.fccID));
    StreamSize = chunk.dwSize - sizeof(FOURCC);
    StreamCount = 0;

    if (chunk.fccID != FOURCC_DLS) {
        TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
        liMove.QuadPart = StreamSize;
        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
        return E_FAIL;
    }

    TRACE_(dmfile)(": collection form\n");
    do {
        IStream_Read(stream, &chunk, sizeof(FOURCC) + sizeof(DWORD), NULL);
        StreamCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.dwSize;
        TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc(chunk.fccID), chunk.dwSize);
        switch (chunk.fccID) {
            case FOURCC_COLH: {
                TRACE_(dmfile)(": collection header chunk\n");
                This->pHeader = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, chunk.dwSize);
                IStream_Read(stream, This->pHeader, chunk.dwSize, NULL);
                break;
            }
            case FOURCC_DLID: {
                TRACE_(dmfile)(": DLID (GUID) chunk\n");
                This->dmobj.desc.dwValidData |= DMUS_OBJ_OBJECT;
                IStream_Read(stream, &This->dmobj.desc.guidObject, chunk.dwSize, NULL);
                break;
            }
            case FOURCC_VERS: {
                TRACE_(dmfile)(": version chunk\n");
                This->dmobj.desc.dwValidData |= DMUS_OBJ_VERSION;
                IStream_Read(stream, &This->dmobj.desc.vVersion, chunk.dwSize, NULL);
                break;
            }
            case FOURCC_PTBL: {
                TRACE_(dmfile)(": pool table chunk\n");
                This->pPoolTable = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(POOLTABLE));
                IStream_Read(stream, This->pPoolTable, sizeof(POOLTABLE), NULL);
                chunk.dwSize -= sizeof(POOLTABLE);
                This->pPoolCues = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->pPoolTable->cCues * sizeof(POOLCUE));
                IStream_Read(stream, This->pPoolCues, chunk.dwSize, NULL);
                break;
            }
            case FOURCC_LIST: {
                IStream_Read(stream, &chunk.fccID, sizeof(FOURCC), NULL);
                TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunk.fccID));
                ListSize[0] = chunk.dwSize - sizeof(FOURCC);
                ListCount[0] = 0;
                switch (chunk.fccID) {
                    case DMUS_FOURCC_INFO_LIST: {
                        TRACE_(dmfile)(": INFO list\n");
                        do {
                            IStream_Read(stream, &chunk, sizeof(FOURCC) + sizeof(DWORD), NULL);
                            ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + chunk.dwSize;
                            TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc(chunk.fccID), chunk.dwSize);
                            switch (chunk.fccID) {
                                case mmioFOURCC('I','N','A','M'): {
                                    CHAR szName[DMUS_MAX_NAME];
                                    TRACE_(dmfile)(": name chunk\n");
                                    This->dmobj.desc.dwValidData |= DMUS_OBJ_NAME;
                                    IStream_Read(stream, szName, chunk.dwSize, NULL);
                                    MultiByteToWideChar(CP_ACP, 0, szName, -1, This->dmobj.desc.wszName, DMUS_MAX_NAME);
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        liMove.QuadPart = 1;
                                        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    }
                                    break;
                                }
                                case mmioFOURCC('I','A','R','T'): {
                                    TRACE_(dmfile)(": artist chunk (ignored)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        chunk.dwSize++;
                                    }
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;
                                }
                                case mmioFOURCC('I','C','O','P'): {
                                    TRACE_(dmfile)(": copyright chunk\n");
                                    This->szCopyright = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, chunk.dwSize);
                                    IStream_Read(stream, This->szCopyright, chunk.dwSize, NULL);
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        liMove.QuadPart = 1;
                                        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    }
                                    break;
                                }
                                case mmioFOURCC('I','S','B','J'): {
                                    TRACE_(dmfile)(": subject chunk (ignored)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        chunk.dwSize++;
                                    }
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;
                                }
                                case mmioFOURCC('I','C','M','T'): {
                                    TRACE_(dmfile)(": comment chunk (ignored)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        chunk.dwSize++;
                                    }
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;
                                }
                                default: {
                                    TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        chunk.dwSize++;
                                    }
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
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
                        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, &dlibWavePoolPosition); /* store position */
                        This->liWavePoolTablePosition.QuadPart = dlibWavePoolPosition.QuadPart;
                        liMove.QuadPart = chunk.dwSize - sizeof(FOURCC);
                        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                        break;
                    }
                    case FOURCC_LINS: {
                        TRACE_(dmfile)(": instruments list\n");
                        do {
                            IStream_Read(stream, &chunk, sizeof(FOURCC) + sizeof(DWORD), NULL);
                            ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + chunk.dwSize;
                            TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc(chunk.fccID), chunk.dwSize);
                            switch (chunk.fccID) {
                                case FOURCC_LIST: {
                                    IStream_Read(stream, &chunk.fccID, sizeof(FOURCC), NULL);
                                    TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunk.fccID));
                                    ListSize[1] = chunk.dwSize - sizeof(FOURCC);
                                    ListCount[1] = 0;
                                    switch (chunk.fccID) {
                                        case FOURCC_INS: {
                                            LPDMUS_PRIVATE_INSTRUMENTENTRY new_instrument = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_INSTRUMENTENTRY));
                                            TRACE_(dmfile)(": instrument list\n");
                                            /* Only way to create this one... even M$ does it discretely */
                                            DMUSIC_CreateDirectMusicInstrumentImpl(&IID_IDirectMusicInstrument, (void**)&new_instrument->pInstrument, NULL);
                                            {
                                                IDirectMusicInstrumentImpl *instrument = impl_from_IDirectMusicInstrument(new_instrument->pInstrument);
                                                /* Store offset and length, they will be needed when loading the instrument */
                                                liMove.QuadPart = 0;
                                                IStream_Seek(stream, liMove, STREAM_SEEK_CUR, &dlibInstrumentPosition);
                                                instrument->liInstrumentPosition.QuadPart = dlibInstrumentPosition.QuadPart;
                                                instrument->length = ListSize[1];
                                                do {
                                                    IStream_Read(stream, &chunk, sizeof(FOURCC) + sizeof(DWORD), NULL);
                                                    ListCount[1] += sizeof(FOURCC) + sizeof(DWORD) + chunk.dwSize;
                                                    TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc(chunk.fccID), chunk.dwSize);
                                                    switch (chunk.fccID) {
                                                        case FOURCC_INSH: {
                                                            TRACE_(dmfile)(": instrument header chunk\n");
                                                            IStream_Read(stream, &instrument->header, chunk.dwSize, NULL);
                                                            break;
                                                        }
                                                        case FOURCC_DLID: {
                                                            TRACE_(dmfile)(": DLID (GUID) chunk\n");
                                                            IStream_Read(stream, &instrument->id, chunk.dwSize, NULL);
                                                            break;
                                                        }
                                                        case FOURCC_LIST: {
                                                            IStream_Read(stream, &chunk.fccID, sizeof(FOURCC), NULL);
                                                            TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunk.fccID));
                                                            switch (chunk.fccID) {
                                                                default: {
                                                                    TRACE_(dmfile)(": unknown (skipping)\n");
                                                                    liMove.QuadPart = chunk.dwSize - sizeof(FOURCC);
                                                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                                                    break;
                                                                }
                                                            }
                                                            break;
                                                        }
                                                        default: {
                                                            TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
                                                            liMove.QuadPart = chunk.dwSize;
                                                            IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                                            break;
                                                        }
                                                    }
                                                    TRACE_(dmfile)(": ListCount[1] = %d < ListSize[1] = %d\n", ListCount[1], ListSize[1]);
                                                } while (ListCount[1] < ListSize[1]);
                                                /* DEBUG: dumps whole instrument object tree: */
                                                if (TRACE_ON(dmusic)) {
                                                    TRACE("*** IDirectMusicInstrument (%p) ***\n", instrument);
                                                    if (!IsEqualGUID(&instrument->id, &GUID_NULL))
                                                        TRACE(" - GUID = %s\n", debugstr_dmguid(&instrument->id));
                                                    TRACE(" - Instrument header:\n");
                                                    TRACE("    - cRegions: %d\n", instrument->header.cRegions);
                                                    TRACE("    - Locale:\n");
                                                    TRACE("       - ulBank: %d\n", instrument->header.Locale.ulBank);
                                                    TRACE("       - ulInstrument: %d\n", instrument->header.Locale.ulInstrument);
                                                    TRACE("       => dwPatch: %d\n", MIDILOCALE2Patch(&instrument->header.Locale));
                                                }
                                                list_add_tail(&This->Instruments, &new_instrument->entry);
                                            }
                                            break;
                                        }
                                    }
                                    break;
                                }
                                default: {
                                    TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;
                                }
                            }
                            TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
                        } while (ListCount[0] < ListSize[0]);
                        break;
                    }
                    default: {
                        TRACE_(dmfile)(": unknown (skipping)\n");
                        liMove.QuadPart = chunk.dwSize - sizeof(FOURCC);
                        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                        break;
                    }
                }
                break;
            }
            default: {
                TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
                liMove.QuadPart = chunk.dwSize;
                IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                break;
            }
        }
        TRACE_(dmfile)(": StreamCount = %d < StreamSize = %d\n", StreamCount, StreamSize);
    } while (StreamCount < StreamSize);

    TRACE_(dmfile)(": reading finished\n");


    /* DEBUG: dumps whole collection object tree: */
    if (TRACE_ON(dmusic)) {
        int r = 0;
        DMUS_PRIVATE_INSTRUMENTENTRY *tmpEntry;
        struct list *listEntry;

        TRACE("*** IDirectMusicCollection (%p) ***\n", &This->IDirectMusicCollection_iface);
        if (This->dmobj.desc.dwValidData & DMUS_OBJ_OBJECT)
            TRACE(" - GUID = %s\n", debugstr_dmguid(&This->dmobj.desc.guidObject));
        if (This->dmobj.desc.dwValidData & DMUS_OBJ_VERSION)
            TRACE(" - Version = %i,%i,%i,%i\n", (This->dmobj.desc.vVersion.dwVersionMS >> 8) & 0x0000FFFF, This->dmobj.desc.vVersion.dwVersionMS & 0x0000FFFF,
                  (This->dmobj.desc.vVersion.dwVersionLS >> 8) & 0x0000FFFF, This->dmobj.desc.vVersion.dwVersionLS & 0x0000FFFF);
        if (This->dmobj.desc.dwValidData & DMUS_OBJ_NAME)
            TRACE(" - Name = %s\n", debugstr_w(This->dmobj.desc.wszName));

        TRACE(" - Collection header:\n");
        TRACE("    - cInstruments: %d\n", This->pHeader->cInstruments);
        TRACE(" - Instruments:\n");

        LIST_FOR_EACH(listEntry, &This->Instruments) {
            tmpEntry = LIST_ENTRY( listEntry, DMUS_PRIVATE_INSTRUMENTENTRY, entry );
            TRACE("    - Instrument[%i]: %p\n", r, tmpEntry->pInstrument);
            r++;
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


HRESULT WINAPI DMUSIC_CreateDirectMusicCollectionImpl(LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter)
{
	IDirectMusicCollectionImpl* obj;
        HRESULT hr;

        *ppobj = NULL;
        if (pUnkOuter)
                return CLASS_E_NOAGGREGATION;

	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicCollectionImpl));
        if (!obj)
                return E_OUTOFMEMORY;

	obj->IDirectMusicCollection_iface.lpVtbl = &DirectMusicCollection_Collection_Vtbl;
        obj->ref = 1;
        dmobject_init(&obj->dmobj, &CLSID_DirectMusicCollection,
                (IUnknown*)&obj->IDirectMusicCollection_iface);
        obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
        obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

	list_init (&obj->Instruments);

        DMUSIC_LockModule();
        hr = IDirectMusicCollection_QueryInterface(&obj->IDirectMusicCollection_iface, lpcGUID, ppobj);
        IDirectMusicCollection_Release(&obj->IDirectMusicCollection_iface);

        return hr;
}
