/*
 * Copyright 2014 Michael Stefaniuc
 *
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

#define COBJMACROS

#include <stdarg.h>
#include <windef.h>
#include <initguid.h>
#include <wine/test.h>
#include <ole2.h>
#include <dmusici.h>
#include <dmusicf.h>

DEFINE_GUID(GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_GUID(IID_IDirectMusicWavePRIVATE, 0x69e934e4, 0x97f1, 0x4f1d, 0x88, 0xe8, 0xf2, 0xac, 0x88,
        0x67, 0x13, 0x27);

static BOOL missing_dswave(void)
{
    IDirectMusicObject *dmo;
    HRESULT hr = CoCreateInstance(&CLSID_DirectSoundWave, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmo);

    if (hr == S_OK && dmo)
    {
        IDirectMusicObject_Release(dmo);
        return FALSE;
    }
    return TRUE;
}

static void test_COM(void)
{
    IDirectMusicObject *dmo = (IDirectMusicObject*)0xdeadbeef;
    IPersistStream *ps;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectSoundWave, (IUnknown*)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dmo);
    ok(hr == CLASS_E_NOAGGREGATION,
            "DirectSoundWave create failed: %#lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dmo, "dmo = %p\n", dmo);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectSoundWave, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicSegment8, (void**)&dmo);
    ok(hr == E_NOINTERFACE, "DirectSoundWave create failed: %#lx, expected E_NOINTERFACE\n", hr);

    /* Same refcount for all DirectSoundWave interfaces */
    hr = CoCreateInstance(&CLSID_DirectSoundWave, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "DirectSoundWave create failed: %#lx, expected S_OK\n", hr);
    refcount = IDirectMusicObject_AddRef(dmo);
    ok(refcount == 2, "refcount == %lu, expected 2\n", refcount);

    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    refcount = IPersistStream_AddRef(ps);
    ok(refcount == 4, "refcount == %lu, expected 4\n", refcount);
    IPersistStream_Release(ps);

    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %#lx\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 5, "refcount == %lu, expected 5\n", refcount);
    refcount = IUnknown_Release(unk);

    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IDirectMusicWavePRIVATE, (void**)&unk);
    todo_wine ok(hr == S_OK, "QueryInterface for IID_IDirectMusicWavePRIVATE failed: %#lx\n", hr);
    if (hr == S_OK) {
        refcount = IUnknown_AddRef(unk);
        ok(refcount == 6, "refcount == %lu, expected 6\n", refcount);
        refcount = IUnknown_Release(unk);
    }

    /* Interfaces that native does not support */
    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IDirectMusicSegment, (void**)&unk);
    ok(hr == E_NOINTERFACE, "QueryInterface for IID_IDirectMusicSegment failed: %#lx\n", hr);
    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IDirectMusicSegment8, (void**)&unk);
    ok(hr == E_NOINTERFACE, "QueryInterface for IID_IDirectMusicSegment8 failed: %#lx\n", hr);

    while (IDirectMusicObject_Release(dmo));
}

struct chunk {
    FOURCC id;
    DWORD size;
    FOURCC type;
};

#define CHUNK_HDR_SIZE (sizeof(FOURCC) + sizeof(DWORD))

/* Generate a RIFF file format stream from an array of FOURCC ids.
   RIFF and LIST need to be followed by the form type respectively list type,
   followed by the chunks of the list and terminated with 0. */
static IStream *gen_riff_stream(const FOURCC *ids)
{
    static const LARGE_INTEGER zero;
    int level = -1;
    DWORD *sizes[4];    /* Stack for the sizes of RIFF and LIST chunks */
    char riff[1024];
    char *p = riff;
    struct chunk *ck;
    IStream *stream;

    do {
        ck = (struct chunk *)p;
        ck->id = *ids++;
        switch (ck->id) {
            case 0:
                *sizes[level] = p - (char *)sizes[level] - sizeof(DWORD);
                level--;
                break;
            case FOURCC_LIST:
            case FOURCC_RIFF:
                level++;
                sizes[level] = &ck->size;
                ck->type = *ids++;
                p += sizeof(*ck);
                break;
            case DMUS_FOURCC_GUID_CHUNK:
                ck->size = sizeof(GUID_NULL);
                p += CHUNK_HDR_SIZE;
                memcpy(p, &GUID_NULL, sizeof(GUID_NULL));
                p += ck->size;
                break;
            case DMUS_FOURCC_VERSION_CHUNK:
            {
                DMUS_VERSION ver = {5, 8};

                ck->size = sizeof(ver);
                p += CHUNK_HDR_SIZE;
                memcpy(p, &ver, sizeof(ver));
                p += ck->size;
                break;
            }
            case mmioFOURCC('I','N','A','M'):
                ck->size = 5;
                p += CHUNK_HDR_SIZE;
                strcpy(p, "INAM");
                p += ck->size + 1; /* WORD aligned */
                break;
            default:
            {
                /* Just convert the FOURCC id to a WCHAR string */
                WCHAR *s;

                ck->size = 5 * sizeof(WCHAR);
                p += CHUNK_HDR_SIZE;
                s = (WCHAR *)p;
                s[0] = (char)(ck->id);
                s[1] = (char)(ck->id >> 8);
                s[2] = (char)(ck->id >> 16);
                s[3] = (char)(ck->id >> 24);
                s[4] = 0;
                p += ck->size;
            }
        }
    } while (level >= 0);

    ck = (struct chunk *)riff;
    CreateStreamOnHGlobal(NULL, TRUE, &stream);
    IStream_Write(stream, riff, ck->size + CHUNK_HDR_SIZE, NULL);
    IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);

    return stream;
}

static void test_parsedescriptor(void)
{
    IDirectMusicObject *dmo;
    IStream *stream;
    DMUS_OBJECTDESC desc = {0};
    HRESULT hr;
    const FOURCC alldesc[] =
    {
        FOURCC_RIFF, mmioFOURCC('W','A','V','E'), DMUS_FOURCC_CATEGORY_CHUNK, FOURCC_LIST,
        DMUS_FOURCC_UNFO_LIST, DMUS_FOURCC_UNAM_CHUNK, DMUS_FOURCC_UCOP_CHUNK,
        DMUS_FOURCC_UCMT_CHUNK, DMUS_FOURCC_USBJ_CHUNK, 0, DMUS_FOURCC_VERSION_CHUNK,
        DMUS_FOURCC_GUID_CHUNK, 0
    };
    const FOURCC dupes[] =
    {
        FOURCC_RIFF, mmioFOURCC('W','A','V','E'), DMUS_FOURCC_CATEGORY_CHUNK,
        DMUS_FOURCC_CATEGORY_CHUNK, DMUS_FOURCC_VERSION_CHUNK, DMUS_FOURCC_VERSION_CHUNK,
        DMUS_FOURCC_GUID_CHUNK, DMUS_FOURCC_GUID_CHUNK, FOURCC_LIST, DMUS_FOURCC_INFO_LIST,
        mmioFOURCC('I','N','A','M'), 0, FOURCC_LIST, DMUS_FOURCC_INFO_LIST,
        mmioFOURCC('I','N','A','M'), 0, 0
    };
    FOURCC empty[] = {FOURCC_RIFF, mmioFOURCC('W','A','V','E'), 0};
    FOURCC inam[] =
    {
        FOURCC_RIFF, mmioFOURCC('W','A','V','E'), FOURCC_LIST, DMUS_FOURCC_UNFO_LIST,
        mmioFOURCC('I','N','A','M'), 0, 0
    };

    hr = CoCreateInstance(&CLSID_DirectSoundWave, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void **)&dmo);
    ok(hr == S_OK, "DirectSoundWave create failed: %#lx, expected S_OK\n", hr);

    /* Nothing loaded */
    hr = IDirectMusicObject_GetDescriptor(dmo, &desc);
    todo_wine ok(hr == E_INVALIDARG, "GetDescriptor failed: %#lx, expected E_INVALIDARG\n", hr);
    todo_wine ok(!desc.dwValidData, "Got valid data %#lx, expected 0\n", desc.dwValidData);

    /* Empty RIFF stream */
    stream = gen_riff_stream(empty);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(!desc.dwValidData, "Got valid data %#lx, expected 0\n", desc.dwValidData);
    IStream_Release(stream);

    /* NULL pointers */
    if (0) {
        /* Crashes on Windows */
        memset(&desc, 0, sizeof(desc));
        hr = IDirectMusicObject_ParseDescriptor(dmo, NULL, &desc);
        ok(hr == E_POINTER, "ParseDescriptor failed: %#lx, expected E_POINTER\n", hr);
    }
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, NULL);
    ok(hr == E_POINTER, "ParseDescriptor failed: %#lx, expected E_POINTER\n", hr);

    /* Wrong form */
    empty[1] = DMUS_FOURCC_CONTAINER_FORM;
    stream = gen_riff_stream(empty);
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == DMUS_E_CHUNKNOTFOUND, "ParseDescriptor failed: %#lx, expected DMUS_E_CHUNKNOTFOUND\n", hr);
    IStream_Release(stream);

    /* All desc chunks, only DMUS_OBJ_OBJECT and DMUS_OBJ_VERSION supported */
    stream = gen_riff_stream(alldesc);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == (DMUS_OBJ_OBJECT | DMUS_OBJ_VERSION),
            "Got valid data %#lx, expected DMUS_OBJ_OBJECT | DMUS_OBJ_VERSION\n", desc.dwValidData);
    ok(IsEqualGUID(&desc.guidObject, &GUID_NULL), "Got object guid %s, expected GUID_NULL\n",
            wine_dbgstr_guid(&desc.guidClass));
    ok(desc.vVersion.dwVersionMS == 5 && desc.vVersion.dwVersionLS == 8,
            "Got version %lu.%lu, expected 5.8\n", desc.vVersion.dwVersionMS,
            desc.vVersion.dwVersionLS);
    IStream_Release(stream);

    /* UNFO list with INAM */
    inam[3] = DMUS_FOURCC_UNFO_LIST;
    stream = gen_riff_stream(inam);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(!desc.dwValidData, "Got valid data %#lx, expected 0\n", desc.dwValidData);
    IStream_Release(stream);

    /* INFO list with INAM */
    inam[3] = DMUS_FOURCC_INFO_LIST;
    stream = gen_riff_stream(inam);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == DMUS_OBJ_NAME, "Got valid data %#lx, expected DMUS_OBJ_NAME\n",
            desc.dwValidData);
    ok(!lstrcmpW(desc.wszName, L"INAM"), "Got name '%s', expected 'INAM'\n",
            wine_dbgstr_w(desc.wszName));
    IStream_Release(stream);

    /* Duplicated chunks */
    stream = gen_riff_stream(dupes);
    memset(&desc, 0, sizeof(desc));
    hr = IDirectMusicObject_ParseDescriptor(dmo, stream, &desc);
    ok(hr == S_OK, "ParseDescriptor failed: %#lx, expected S_OK\n", hr);
    ok(desc.dwValidData == (DMUS_OBJ_OBJECT | DMUS_OBJ_NAME | DMUS_OBJ_VERSION),
            "Got valid data %#lx, expected DMUS_OBJ_OBJECT | DMUS_OBJ_NAME | DMUS_OBJ_VERSION\n",
            desc.dwValidData);
    ok(!lstrcmpW(desc.wszName, L"INAM"), "Got name '%s', expected 'INAM'\n",
            wine_dbgstr_w(desc.wszName));
    IStream_Release(stream);

    IDirectMusicObject_Release(dmo);
}

static void test_dswave(void)
{
    IDirectMusicObject *dmo;
    IPersistStream *ps;
    CLSID class = { 0 };
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectSoundWave, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicObject, (void**)&dmo);
    ok(hr == S_OK, "DirectSoundWave create failed: %#lx, expected S_OK\n", hr);

    /* IPersistStream */
    hr = IDirectMusicObject_QueryInterface(dmo, &IID_IPersistStream, (void**)&ps);
    ok(hr == S_OK, "QueryInterface for IID_IPersistStream failed: %#lx\n", hr);
    hr = IPersistStream_GetClassID(ps, &class);
    ok(hr == S_OK, "IPersistStream_GetClassID failed: %#lx\n", hr);
    ok(IsEqualGUID(&class, &CLSID_DirectSoundWave),
            "Expected class CLSID_DirectSoundWave got %s\n", wine_dbgstr_guid(&class));

    /* Unimplemented IPersistStream methods */
    hr = IPersistStream_IsDirty(ps);
    ok(hr == S_FALSE, "IPersistStream_IsDirty failed: %#lx\n", hr);
    hr = IPersistStream_GetSizeMax(ps, &size);
    ok(hr == E_NOTIMPL, "IPersistStream_GetSizeMax failed: %#lx\n", hr);
    hr = IPersistStream_Save(ps, NULL, TRUE);
    ok(hr == E_NOTIMPL, "IPersistStream_Save failed: %#lx\n", hr);

    while (IDirectMusicObject_Release(dmo));
}

START_TEST(dswave)
{
    CoInitialize(NULL);

    if (missing_dswave())
    {
        skip("dswave not available\n");
        CoUninitialize();
        return;
    }
    test_COM();
    test_dswave();
    test_parsedescriptor();

    CoUninitialize();
}
