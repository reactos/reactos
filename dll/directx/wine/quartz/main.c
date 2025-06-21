/*              DirectShow Base Functions (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
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

#include "wine/debug.h"

#include "quartz_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

extern HRESULT WINAPI QUARTZ_DllGetClassObject(REFCLSID, REFIID, LPVOID *);
extern BOOL WINAPI QUARTZ_DllMain(HINSTANCE, DWORD, LPVOID);
extern HRESULT WINAPI QUARTZ_DllRegisterServer(void);
extern HRESULT WINAPI QUARTZ_DllUnregisterServer(void);

bool array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    unsigned int new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return true;

    max_capacity = ~(size_t)0 / size;
    if (count > max_capacity)
        return false;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return false;

    *elements = new_elements;
    *capacity = new_capacity;

    return true;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_DETACH && !reserved)
    {
        video_window_unregister_class();
        strmbase_release_typelibs();
    }
    return QUARTZ_DllMain(instance, reason, reserved);
}

/******************************************************************************
 * DirectShow ClassFactory
 */
typedef struct {
    IClassFactory IClassFactory_iface;
    LONG ref;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

struct object_creation_info
{
    const CLSID *clsid;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
};

static const struct object_creation_info object_creation[] =
{
    { &CLSID_ACMWrapper, acm_wrapper_create },
    { &CLSID_AllocPresenter, vmr7_presenter_create },
    { &CLSID_AsyncReader, async_reader_create },
    { &CLSID_AudioRender, dsound_render_create },
    { &CLSID_AVIDec, avi_dec_create },
    { &CLSID_AviSplitter, avi_splitter_create },
    { &CLSID_CMpegAudioCodec, mpeg_audio_codec_create },
    { &CLSID_CMpegVideoCodec, mpeg_video_codec_create },
    { &CLSID_DSoundRender, dsound_render_create },
    { &CLSID_FilterGraph, filter_graph_create },
    { &CLSID_FilterGraphNoThread, filter_graph_no_thread_create },
    { &CLSID_FilterMapper, filter_mapper_create },
    { &CLSID_FilterMapper2, filter_mapper_create },
    { &CLSID_MemoryAllocator, mem_allocator_create },
    { &CLSID_MPEG1Splitter, mpeg1_splitter_create },
    { &CLSID_SeekingPassThru, seeking_passthrough_create },
    { &CLSID_SystemClock, system_clock_create },
    { &CLSID_VideoRenderer, video_renderer_create },
    { &CLSID_VideoMixingRenderer, vmr7_create },
    { &CLSID_VideoMixingRenderer9, vmr9_create },
    { &CLSID_VideoRendererDefault, video_renderer_default_create },
    { &CLSID_WAVEParser, wave_parser_create },
};

static HRESULT WINAPI DSCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppobj)
{
    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IClassFactory))
    {
	IClassFactory_AddRef(iface);
        *ppobj = iface;
	return S_OK;
    }

    *ppobj = NULL;
    WARN("(%p)->(%s,%p), not found\n", iface, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI DSCF_AddRef(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DSCF_Release(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
	CoTaskMemFree(This);

    return ref;
}


static HRESULT WINAPI DSCF_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                          REFIID riid, void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    HRESULT hres;
    LPUNKNOWN punk;

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    if(pOuter && !IsEqualGUID(&IID_IUnknown, riid))
        return E_NOINTERFACE;

    if (SUCCEEDED(hres = This->create_instance(pOuter, &punk)))
    {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
        IUnknown_Release(punk);
    }
    return hres;
}

static HRESULT WINAPI DSCF_LockServer(IClassFactory *iface, BOOL lock)
{
    FIXME("iface %p, lock %d, stub!\n", iface, lock);
    return S_OK;
}

static const IClassFactoryVtbl DSCF_Vtbl =
{
    DSCF_QueryInterface,
    DSCF_AddRef,
    DSCF_Release,
    DSCF_CreateInstance,
    DSCF_LockServer
};

/*******************************************************************************
 * DllGetClassObject [QUARTZ.@]
 * Retrieves class object from a DLL object
 *
 * NOTES
 *    Docs say returns STDAPI
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    unsigned int i;

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (IsEqualGUID( &IID_IClassFactory, riid ) || IsEqualGUID( &IID_IUnknown, riid))
    {
        for (i = 0; i < ARRAY_SIZE(object_creation); i++)
        {
            if (IsEqualGUID(object_creation[i].clsid, rclsid))
            {
                IClassFactoryImpl *factory = CoTaskMemAlloc(sizeof(*factory));
                if (factory == NULL) return E_OUTOFMEMORY;

                factory->IClassFactory_iface.lpVtbl = &DSCF_Vtbl;
                factory->ref = 1;

                factory->create_instance = object_creation[i].create_instance;

                *ppv = &factory->IClassFactory_iface;
                return S_OK;
            }
        }
    }
    return QUARTZ_DllGetClassObject( rclsid, riid, ppv );
}

/***********************************************************************
 *      DllRegisterServer (QUARTZ.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    static const REGPINTYPES video_renderer_inputs[] =
    {
        {&MEDIATYPE_Video, &GUID_NULL},
    };
    static const REGFILTERPINS2 video_renderer_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(video_renderer_inputs),
            .lpMediaType = video_renderer_inputs,
            .dwFlags = REG_PINFLAG_B_RENDERER,
        },
    };
    static const REGFILTER2 video_renderer_default_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_PREFERRED + 1,
        .cPins2 = ARRAY_SIZE(video_renderer_pins),
        .rgPins2 = video_renderer_pins,
    };
    static const REGFILTER2 video_renderer_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_PREFERRED,
        .cPins2 = ARRAY_SIZE(video_renderer_pins),
        .rgPins2 = video_renderer_pins,
    };

    static const REGPINTYPES vmr9_filter_inputs[] =
    {
        {&MEDIATYPE_Video, &GUID_NULL},
    };
    static const REGFILTERPINS2 vmr9_filter_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(vmr9_filter_inputs),
            .lpMediaType = vmr9_filter_inputs,
            .dwFlags = REG_PINFLAG_B_RENDERER,
        },
    };
    static const REGFILTER2 vmr9_filter_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_DO_NOT_USE,
        .cPins2 = ARRAY_SIZE(vmr9_filter_pins),
        .rgPins2 = vmr9_filter_pins,
    };

    static const REGPINTYPES avi_decompressor_inputs[] =
    {
        {&MEDIATYPE_Video, &GUID_NULL},
    };
    static const REGPINTYPES avi_decompressor_outputs[] =
    {
        {&MEDIATYPE_Video, &GUID_NULL},
    };
    static const REGFILTERPINS2 avi_decompressor_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(avi_decompressor_inputs),
            .lpMediaType = avi_decompressor_inputs,
        },
        {
            .nMediaTypes = ARRAY_SIZE(avi_decompressor_outputs),
            .lpMediaType = avi_decompressor_outputs,
            .dwFlags = REG_PINFLAG_B_OUTPUT,
        },
    };
    static const REGFILTER2 avi_decompressor_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_NORMAL - 16,
        .cPins2 = ARRAY_SIZE(avi_decompressor_pins),
        .rgPins2 = avi_decompressor_pins,
    };

    static const REGPINTYPES async_reader_outputs[] =
    {
        {&MEDIATYPE_Stream, &GUID_NULL},
    };
    static const REGFILTERPINS2 async_reader_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(async_reader_outputs),
            .lpMediaType = async_reader_outputs,
            .dwFlags = REG_PINFLAG_B_OUTPUT,
        },
    };
    static const REGFILTER2 async_reader_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_UNLIKELY,
        .cPins2 = ARRAY_SIZE(async_reader_pins),
        .rgPins2 = async_reader_pins,
    };

    static const REGPINTYPES acm_wrapper_inputs[] =
    {
        {&MEDIATYPE_Audio, &GUID_NULL},
    };
    static const REGPINTYPES acm_wrapper_outputs[] =
    {
        {&MEDIATYPE_Audio, &GUID_NULL},
    };
    static const REGFILTERPINS2 acm_wrapper_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(acm_wrapper_inputs),
            .lpMediaType = acm_wrapper_inputs,
        },
        {
            .nMediaTypes = ARRAY_SIZE(acm_wrapper_outputs),
            .lpMediaType = acm_wrapper_outputs,
            .dwFlags = REG_PINFLAG_B_OUTPUT,
        },
    };
    static const REGFILTER2 acm_wrapper_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_NORMAL - 16,
        .cPins2 = ARRAY_SIZE(acm_wrapper_pins),
        .rgPins2 = acm_wrapper_pins,
    };

    static const REGPINTYPES mpeg_splitter_inputs[] =
    {
        {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Audio},
        {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Video},
        {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1System},
        {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1VideoCD},
    };
    static const REGPINTYPES mpeg_splitter_audio_outputs[] =
    {
        {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Packet},
        {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload},
    };
    static const REGPINTYPES mpeg_splitter_video_outputs[] =
    {
        {&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet},
        {&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload},
    };
    static const REGFILTERPINS2 mpeg_splitter_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(mpeg_splitter_inputs),
            .lpMediaType = mpeg_splitter_inputs,
        },
        {
            .dwFlags = REG_PINFLAG_B_ZERO | REG_PINFLAG_B_OUTPUT,
            .nMediaTypes = ARRAY_SIZE(mpeg_splitter_audio_outputs),
            .lpMediaType = mpeg_splitter_audio_outputs,
        },
        {
            .dwFlags = REG_PINFLAG_B_ZERO | REG_PINFLAG_B_OUTPUT,
            .nMediaTypes = ARRAY_SIZE(mpeg_splitter_video_outputs),
            .lpMediaType = mpeg_splitter_video_outputs,
        },
    };
    static const REGFILTER2 mpeg_splitter_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_NORMAL,
        .cPins2 = ARRAY_SIZE(mpeg_splitter_pins),
        .rgPins2 = mpeg_splitter_pins,
    };

    static const REGPINTYPES avi_splitter_inputs[] =
    {
        {&MEDIATYPE_Stream, &MEDIASUBTYPE_Avi},
    };
    static const REGPINTYPES avi_splitter_outputs[] =
    {
        {&MEDIATYPE_Video, &GUID_NULL},
    };
    static const REGFILTERPINS2 avi_splitter_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(avi_splitter_inputs),
            .lpMediaType = avi_splitter_inputs,
        },
        {
            .dwFlags = REG_PINFLAG_B_OUTPUT,
            .nMediaTypes = ARRAY_SIZE(avi_splitter_outputs),
            .lpMediaType = avi_splitter_outputs,
        },
    };
    static const REGFILTER2 avi_splitter_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_NORMAL,
        .cPins2 = ARRAY_SIZE(avi_splitter_pins),
        .rgPins2 = avi_splitter_pins,
    };

    static const REGPINTYPES wave_parser_inputs[] =
    {
        {&MEDIATYPE_Stream, &MEDIASUBTYPE_WAVE},
        {&MEDIATYPE_Stream, &MEDIASUBTYPE_AU},
        {&MEDIATYPE_Stream, &MEDIASUBTYPE_AIFF},
    };
    static const REGPINTYPES wave_parser_outputs[] =
    {
        {&MEDIATYPE_Audio, &GUID_NULL},
    };
    static const REGFILTERPINS2 wave_parser_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(wave_parser_inputs),
            .lpMediaType = wave_parser_inputs,
        },
        {
            .dwFlags = REG_PINFLAG_B_OUTPUT,
            .nMediaTypes = ARRAY_SIZE(wave_parser_outputs),
            .lpMediaType = wave_parser_outputs,
        },
    };
    static const REGFILTER2 wave_parser_reg =
    {
        .dwVersion = 2,
        .dwMerit = MERIT_UNLIKELY,
        .cPins2 = ARRAY_SIZE(wave_parser_pins),
        .rgPins2 = wave_parser_pins,
    };

    static const REGPINTYPES mpeg_audio_codec_inputs[] =
    {
        {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Packet},
        {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Payload},
        {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload},
    };
    static const REGPINTYPES mpeg_audio_codec_outputs[] =
    {
        {&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
    };
    static const REGFILTERPINS2 mpeg_audio_codec_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(mpeg_audio_codec_inputs),
            .lpMediaType = mpeg_audio_codec_inputs,
        },
        {
            .dwFlags = REG_PINFLAG_B_OUTPUT,
            .nMediaTypes = ARRAY_SIZE(mpeg_audio_codec_outputs),
            .lpMediaType = mpeg_audio_codec_outputs,
        },
    };
    static const REGFILTER2 mpeg_audio_codec_reg =
    {
        .dwVersion = 2,
        .dwMerit = 0x03680001,
        .cPins2 = ARRAY_SIZE(mpeg_audio_codec_pins),
        .rgPins2 = mpeg_audio_codec_pins,
    };

    static const REGPINTYPES mpeg_video_codec_inputs[] =
    {
        {&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet},
        {&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload},
    };
    static const REGPINTYPES mpeg_video_codec_outputs[] =
    {
        {&MEDIATYPE_Video, &GUID_NULL},
    };
    static const REGFILTERPINS2 mpeg_video_codec_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(mpeg_video_codec_inputs),
            .lpMediaType = mpeg_video_codec_inputs,
        },
        {
            .dwFlags = REG_PINFLAG_B_OUTPUT,
            .nMediaTypes = ARRAY_SIZE(mpeg_video_codec_outputs),
            .lpMediaType = mpeg_video_codec_outputs,
        },
    };
    static const REGFILTER2 mpeg_video_codec_reg =
    {
        .dwVersion = 2,
        .dwMerit = 0x40000001,
        .cPins2 = ARRAY_SIZE(mpeg_video_codec_pins),
        .rgPins2 = mpeg_video_codec_pins,
    };

    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = QUARTZ_DllRegisterServer()))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_VideoRenderer, L"Video Renderer", NULL,
            &CLSID_LegacyAmFilterCategory, NULL, &video_renderer_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_VideoRendererDefault, L"Video Renderer", NULL,
            &CLSID_LegacyAmFilterCategory, NULL, &video_renderer_default_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_VideoMixingRenderer9, L"Video Mixing Renderer 9", NULL,
            &CLSID_LegacyAmFilterCategory, NULL, &vmr9_filter_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_AVIDec, L"AVI Decompressor", NULL,
            &CLSID_LegacyAmFilterCategory, NULL, &avi_decompressor_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_AsyncReader, L"File Source (Async.)", NULL,
            &CLSID_LegacyAmFilterCategory, NULL, &async_reader_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_ACMWrapper, L"ACM Wrapper", NULL,
            &CLSID_LegacyAmFilterCategory, NULL, &acm_wrapper_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_AviSplitter, L"AVI Splitter", NULL,
            NULL, NULL, &avi_splitter_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_MPEG1Splitter, L"MPEG-I Stream Splitter", NULL,
            NULL, NULL, &mpeg_splitter_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_WAVEParser, L"Wave Parser", NULL,
            NULL, NULL, &wave_parser_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_CMpegAudioCodec, L"MPEG Audio Decoder", NULL,
            NULL, NULL, &mpeg_audio_codec_reg)))
        goto done;
    if (FAILED(hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_CMpegVideoCodec, L"MPEG Video Decoder", NULL,
            NULL, NULL, &mpeg_video_codec_reg)))
        goto done;

done:
    IFilterMapper2_Release(mapper);
    return hr;
}

/***********************************************************************
 *      DllUnregisterServer (QUARTZ.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, &CLSID_LegacyAmFilterCategory, NULL, &CLSID_VideoRenderer)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, &CLSID_LegacyAmFilterCategory, NULL, &CLSID_VideoRendererDefault)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, &CLSID_LegacyAmFilterCategory, NULL, &CLSID_VideoMixingRenderer9)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, &CLSID_LegacyAmFilterCategory, NULL, &CLSID_AVIDec)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, &CLSID_LegacyAmFilterCategory, NULL, &CLSID_AsyncReader)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, &CLSID_LegacyAmFilterCategory, NULL, &CLSID_ACMWrapper)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_AviSplitter)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_MPEG1Splitter)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_WAVEParser)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_CMpegAudioCodec)))
        goto done;
    if (FAILED(hr = IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_CMpegVideoCodec)))
        goto done;

done:
    IFilterMapper2_Release(mapper);
    if (SUCCEEDED(hr))
        hr = QUARTZ_DllUnregisterServer();

    return hr;
}

#define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } , #name },

static const struct {
	const GUID	riid;
	const char 	*name;
} InterfaceDesc[] =
{
#include "uuids.h"
    { { 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} }, NULL }
};

/***********************************************************************
 *              proxies
 */
HRESULT CALLBACK ICaptureGraphBuilder_FindInterface_Proxy( ICaptureGraphBuilder *This,
                                                           const GUID *pCategory,
                                                           IBaseFilter *pf,
                                                           REFIID riid,
                                                           void **ppint )
{
    return ICaptureGraphBuilder_RemoteFindInterface_Proxy( This, pCategory, pf,
                                                           riid, (IUnknown **)ppint );
}

HRESULT __RPC_STUB ICaptureGraphBuilder_FindInterface_Stub( ICaptureGraphBuilder *This,
                                                            const GUID *pCategory,
                                                            IBaseFilter *pf,
                                                            REFIID riid,
                                                            IUnknown **ppint )
{
    return ICaptureGraphBuilder_FindInterface( This, pCategory, pf, riid, (void **)ppint );
}

HRESULT CALLBACK ICaptureGraphBuilder2_FindInterface_Proxy( ICaptureGraphBuilder2 *This,
                                                            const GUID *pCategory,
                                                            const GUID *pType,
                                                            IBaseFilter *pf,
                                                            REFIID riid,
                                                            void **ppint )
{
    return ICaptureGraphBuilder2_RemoteFindInterface_Proxy( This, pCategory, pType,
                                                            pf, riid, (IUnknown **)ppint );
}

HRESULT __RPC_STUB ICaptureGraphBuilder2_FindInterface_Stub( ICaptureGraphBuilder2 *This,
                                                             const GUID *pCategory,
                                                             const GUID *pType,
                                                             IBaseFilter *pf,
                                                             REFIID riid,
                                                             IUnknown **ppint )
{
    return ICaptureGraphBuilder2_FindInterface( This, pCategory, pType, pf, riid, (void **)ppint );
}

/***********************************************************************
 *              qzdebugstr_guid (internal)
 *
 * Gives a text version of DirectShow GUIDs
 */
const char * qzdebugstr_guid( const GUID * id )
{
    int i;

    for (i=0; InterfaceDesc[i].name; i++)
        if (IsEqualGUID(&InterfaceDesc[i].riid, id)) return InterfaceDesc[i].name;

    return debugstr_guid(id);
}

int WINAPI AmpFactorToDB(int ampfactor)
{
    FIXME("(%d) Stub!\n", ampfactor);
    return 0;
}

int WINAPI DBToAmpFactor(int db)
{
    FIXME("(%d) Stub!\n", db);
    /* Avoid divide by zero (probably during range computation) in Windows Media Player 6.4 */
    if (db < -1000)
	return 0;
    return 100;
}

/***********************************************************************
 *              AMGetErrorTextA (QUARTZ.@)
 */
DWORD WINAPI AMGetErrorTextA(HRESULT hr, LPSTR buffer, DWORD maxlen)
{
    DWORD res;
    WCHAR errorW[MAX_ERROR_TEXT_LEN];

    TRACE("hr %#lx, buffer %p, maxlen %lu.\n", hr, buffer, maxlen);

    if (!buffer)
        return 0;

    res = AMGetErrorTextW(hr, errorW, ARRAY_SIZE(errorW));
    if (!res)
        return 0;

    res = WideCharToMultiByte(CP_ACP, 0, errorW, -1, NULL, 0, 0, 0);
    if (res > maxlen || !res)
        return 0;
    return WideCharToMultiByte(CP_ACP, 0, errorW, -1, buffer, maxlen, 0, 0) - 1;
}

/***********************************************************************
 *              AMGetErrorTextW (QUARTZ.@)
 */
DWORD WINAPI AMGetErrorTextW(HRESULT hr, LPWSTR buffer, DWORD maxlen)
{
    unsigned int len;
    WCHAR error[MAX_ERROR_TEXT_LEN];

    TRACE("hr %#lx, buffer %p, maxlen %lu.\n", hr, buffer, maxlen);

    if (!buffer) return 0;
    swprintf(error, ARRAY_SIZE(error), L"Error: 0x%lx", hr);
    if ((len = wcslen(error)) >= maxlen)
        return 0;
    wcscpy(buffer, error);
    return len;
}
