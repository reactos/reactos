/*
 * Copyright 2009 Jacek Caban for CodeWeavers
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

#include "urlmon_main.h"

typedef struct {
    IInternetProtocol     IInternetProtocol_iface;
    IInternetProtocolSink IInternetProtocolSink_iface;

    LONG ref;
} MimeFilter;

static inline MimeFilter *impl_from_IInternetProtocol(IInternetProtocol *iface)
{
    return CONTAINING_RECORD(iface, MimeFilter, IInternetProtocol_iface);
}

static HRESULT WINAPI MimeFilterProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", This, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", This, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolSink %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolSink_iface;
    }

    if(*ppv) {
        IInternetProtocol_AddRef(iface);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MimeFilterProtocol_AddRef(IInternetProtocol *iface)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI MimeFilterProtocol_Release(IInternetProtocol *iface)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI MimeFilterProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%s %p %p %08x %lx)\n", This, debugstr_w(szUrl), pOIProtSink,
          pOIBindInfo, grfPI, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%08x)\n", This, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Suspend(IInternetProtocol *iface)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Resume(IInternetProtocol *iface)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%08x)\n", This, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_UnlockRequest(IInternetProtocol *iface)
{
    MimeFilter *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IInternetProtocolVtbl MimeFilterProtocolVtbl = {
    MimeFilterProtocol_QueryInterface,
    MimeFilterProtocol_AddRef,
    MimeFilterProtocol_Release,
    MimeFilterProtocol_Start,
    MimeFilterProtocol_Continue,
    MimeFilterProtocol_Abort,
    MimeFilterProtocol_Terminate,
    MimeFilterProtocol_Suspend,
    MimeFilterProtocol_Resume,
    MimeFilterProtocol_Read,
    MimeFilterProtocol_Seek,
    MimeFilterProtocol_LockRequest,
    MimeFilterProtocol_UnlockRequest
};

static inline MimeFilter *impl_from_IInternetProtocolSink(IInternetProtocolSink *iface)
{
    return CONTAINING_RECORD(iface, MimeFilter, IInternetProtocolSink_iface);
}

static HRESULT WINAPI MimeFilterSink_QueryInterface(IInternetProtocolSink *iface,
        REFIID riid, void **ppv)
{
    MimeFilter *This = impl_from_IInternetProtocolSink(iface);
    return IInternetProtocol_QueryInterface(&This->IInternetProtocol_iface, riid, ppv);
}

static ULONG WINAPI MimeFilterSink_AddRef(IInternetProtocolSink *iface)
{
    MimeFilter *This = impl_from_IInternetProtocolSink(iface);
    return IInternetProtocol_AddRef(&This->IInternetProtocol_iface);
}

static ULONG WINAPI MimeFilterSink_Release(IInternetProtocolSink *iface)
{
    MimeFilter *This = impl_from_IInternetProtocolSink(iface);
    return IInternetProtocol_Release(&This->IInternetProtocol_iface);
}

static HRESULT WINAPI MimeFilterSink_Switch(IInternetProtocolSink *iface,
        PROTOCOLDATA *pProtocolData)
{
    MimeFilter *This = impl_from_IInternetProtocolSink(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterSink_ReportProgress(IInternetProtocolSink *iface,
        ULONG ulStatusCode, LPCWSTR szStatusText)
{
    MimeFilter *This = impl_from_IInternetProtocolSink(iface);
    FIXME("(%p)->(%u %s)\n", This, ulStatusCode, debugstr_w(szStatusText));
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterSink_ReportData(IInternetProtocolSink *iface,
        DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    MimeFilter *This = impl_from_IInternetProtocolSink(iface);
    FIXME("(%p)->(%d %u %u)\n", This, grfBSCF, ulProgress, ulProgressMax);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterSink_ReportResult(IInternetProtocolSink *iface,
        HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    MimeFilter *This = impl_from_IInternetProtocolSink(iface);
    FIXME("(%p)->(%08x %d %s)\n", This, hrResult, dwError, debugstr_w(szResult));
    return E_NOTIMPL;
}

static const IInternetProtocolSinkVtbl InternetProtocolSinkVtbl = {
    MimeFilterSink_QueryInterface,
    MimeFilterSink_AddRef,
    MimeFilterSink_Release,
    MimeFilterSink_Switch,
    MimeFilterSink_ReportProgress,
    MimeFilterSink_ReportData,
    MimeFilterSink_ReportResult
};

HRESULT MimeFilter_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    MimeFilter *ret;

    TRACE("(%p %p)\n", pUnkOuter, ppobj);

    URLMON_LockModule();

    ret = heap_alloc_zero(sizeof(MimeFilter));

    ret->IInternetProtocol_iface.lpVtbl = &MimeFilterProtocolVtbl;
    ret->IInternetProtocolSink_iface.lpVtbl = &InternetProtocolSinkVtbl;
    ret->ref = 1;

    *ppobj = &ret->IInternetProtocol_iface;
    return S_OK;
}

static BOOL text_richtext_filter(const BYTE *b, DWORD size)
{
    return size > 5 && !memcmp(b, "{\\rtf", 5);
}

static BOOL text_html_filter(const BYTE *b, DWORD size)
{
    if(size < 6)
        return FALSE;

    if((b[0] == '<'
                && (b[1] == 'h' || b[1] == 'H')
                && (b[2] == 't' || b[2] == 'T')
                && (b[3] == 'm' || b[3] == 'M')
                && (b[4] == 'l' || b[4] == 'L'))
            || (b[0] == '<'
                && (b[1] == 'h' || b[1] == 'H')
                && (b[2] == 'e' || b[2] == 'E')
                && (b[3] == 'a' || b[3] == 'A')
                && (b[4] == 'd' || b[4] == 'D'))
            || (b[0] == '<'
                && (b[1] == 'b' || b[1] == 'B')
                && (b[2] == 'o' || b[2] == 'O')
                && (b[3] == 'd' || b[3] == 'D')
                && (b[4] == 'y' || b[4] == 'Y'))) return TRUE;

    return FALSE;
}

static BOOL text_xml_filter(const BYTE *b, DWORD size)
{
    if(size < 7)
        return FALSE;

    if(b[0] == '<' && b[1] == '?'
            && (b[2] == 'x' || b[2] == 'X')
            && (b[3] == 'm' || b[3] == 'M')
            && (b[4] == 'l' || b[4] == 'L')
            && b[5] == ' ') return TRUE;

    return FALSE;
}

static BOOL audio_basic_filter(const BYTE *b, DWORD size)
{
    return size > 4
        && b[0] == '.' && b[1] == 's' && b[2] == 'n' && b[3] == 'd';
}

static BOOL audio_wav_filter(const BYTE *b, DWORD size)
{
    return size > 12
        && b[0] == 'R' && b[1] == 'I' && b[2] == 'F' && b[3] == 'F'
        && b[8] == 'W' && b[9] == 'A' && b[10] == 'V' && b[11] == 'E';
}

static BOOL image_gif_filter(const BYTE *b, DWORD size)
{
    return size >= 6
        && (b[0] == 'G' || b[0] == 'g')
        && (b[1] == 'I' || b[1] == 'i')
        && (b[2] == 'F' || b[2] == 'f')
        &&  b[3] == '8'
        && (b[4] == '7' || b[4] == '9')
        && (b[5] == 'A' || b[5] == 'a');
}

static BOOL image_pjpeg_filter(const BYTE *b, DWORD size)
{
    return size > 2 && b[0] == 0xff && b[1] == 0xd8;
}

static BOOL image_tiff_filter(const BYTE *b, DWORD size)
{
    static const BYTE magic1[] = {0x4d,0x4d,0x00,0x2a};
    static const BYTE magic2[] = {0x49,0x49,0x2a,0xff};

    return size >= 4 && (!memcmp(b, magic1, 4) || !memcmp(b, magic2, 4));
}

static BOOL image_xpng_filter(const BYTE *b, DWORD size)
{
    static const BYTE xpng_header[] = {0x89,'P','N','G',0x0d,0x0a,0x1a,0x0a};
    return size > sizeof(xpng_header) && !memcmp(b, xpng_header, sizeof(xpng_header));
}

static BOOL image_bmp_filter(const BYTE *b, DWORD size)
{
    return size >= 14
        && b[0] == 0x42 && b[1] == 0x4d
        && *(const DWORD *)(b+6) == 0;
}

static BOOL video_avi_filter(const BYTE *b, DWORD size)
{
    return size > 12
        && b[0] == 'R' && b[1] == 'I' && b[2] == 'F' && b[3] == 'F'
        && b[8] == 'A' && b[9] == 'V' && b[10] == 'I' && b[11] == 0x20;
}

static BOOL video_mpeg_filter(const BYTE *b, DWORD size)
{
    return size > 4
        && !b[0] && !b[1] && b[2] == 0x01
        && (b[3] == 0xb3 || b[3] == 0xba);
}

static BOOL application_postscript_filter(const BYTE *b, DWORD size)
{
    return size > 2 && b[0] == '%' && b[1] == '!';
}

static BOOL application_pdf_filter(const BYTE *b, DWORD size)
{
    return size > 4 && b[0] == 0x25 && b[1] == 0x50 && b[2] == 0x44 && b[3] == 0x46;
}

static BOOL application_xzip_filter(const BYTE *b, DWORD size)
{
    return size > 2 && b[0] == 0x50 && b[1] == 0x4b;
}

static BOOL application_xgzip_filter(const BYTE *b, DWORD size)
{
    return size > 2 && b[0] == 0x1f && b[1] == 0x8b;
}

static BOOL application_java_filter(const BYTE *b, DWORD size)
{
    return size > 4 && b[0] == 0xca && b[1] == 0xfe && b[2] == 0xba && b[3] == 0xbe;
}

static BOOL application_xmsdownload(const BYTE *b, DWORD size)
{
    return size > 2 && b[0] == 'M' && b[1] == 'Z';
}

static inline BOOL is_text_plain_char(BYTE b)
{
    if(b < 0x20 && b != '\n' && b != '\r' && b != '\t')
        return FALSE;
    return TRUE;
}

static BOOL text_plain_filter(const BYTE *b, DWORD size)
{
    const BYTE *ptr;

    for(ptr = b; ptr < b+size-1; ptr++) {
        if(!is_text_plain_char(*ptr))
            return FALSE;
    }

    return TRUE;
}

static BOOL application_octet_stream_filter(const BYTE *b, DWORD size)
{
    return TRUE;
}

HRESULT find_mime_from_ext(const WCHAR *ext, WCHAR **ret)
{
    DWORD res, size;
    WCHAR mime[64];
    HKEY hkey;

    static const WCHAR content_typeW[] = {'C','o','n','t','e','n','t',' ','T','y','p','e','\0'};

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, ext, &hkey);
    if(res != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(res);

    size = sizeof(mime);
    res = RegQueryValueExW(hkey, content_typeW, NULL, NULL, (LPBYTE)mime, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(res);

    TRACE("found MIME %s\n", debugstr_w(mime));

    *ret = CoTaskMemAlloc(size);
    memcpy(*ret, mime, size);
    return S_OK;
}

static HRESULT find_mime_from_url(const WCHAR *url, WCHAR **ret)
{
    const WCHAR *ptr, *end_ptr;
    WCHAR *ext = NULL;
    HRESULT hres;

    for(end_ptr = url; *end_ptr; end_ptr++) {
        if(*end_ptr == '?' || *end_ptr == '#')
            break;
    }

    for(ptr = end_ptr; ptr >= url; ptr--) {
        if(*ptr == '.')
            break;
    }

    if(ptr < url)
        return E_FAIL;

    if(*end_ptr) {
        unsigned len = end_ptr-ptr;

        ext = heap_alloc((len+1)*sizeof(WCHAR));
        if(!ext)
            return E_OUTOFMEMORY;

        memcpy(ext, ptr, len*sizeof(WCHAR));
        ext[len] = 0;
    }

    hres = find_mime_from_ext(ext ? ext : ptr, ret);
    heap_free(ext);
    return hres;
}

static const WCHAR text_htmlW[] = {'t','e','x','t','/','h','t','m','l',0};
static const WCHAR text_richtextW[] = {'t','e','x','t','/','r','i','c','h','t','e','x','t',0};
static const WCHAR text_xmlW[] = {'t','e','x','t','/','x','m','l',0};
static const WCHAR audio_basicW[] = {'a','u','d','i','o','/','b','a','s','i','c',0};
static const WCHAR audio_wavW[] = {'a','u','d','i','o','/','w','a','v',0};
static const WCHAR image_gifW[] = {'i','m','a','g','e','/','g','i','f',0};
static const WCHAR image_pjpegW[] = {'i','m','a','g','e','/','p','j','p','e','g',0};
static const WCHAR image_tiffW[] = {'i','m','a','g','e','/','t','i','f','f',0};
static const WCHAR image_xpngW[] = {'i','m','a','g','e','/','x','-','p','n','g',0};
static const WCHAR image_bmpW[] = {'i','m','a','g','e','/','b','m','p',0};
static const WCHAR video_aviW[] = {'v','i','d','e','o','/','a','v','i',0};
static const WCHAR video_mpegW[] = {'v','i','d','e','o','/','m','p','e','g',0};
static const WCHAR app_postscriptW[] =
        {'a','p','p','l','i','c','a','t','i','o','n','/','p','o','s','t','s','c','r','i','p','t',0};
static const WCHAR app_pdfW[] = {'a','p','p','l','i','c','a','t','i','o','n','/','p','d','f',0};
static const WCHAR app_xzipW[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
        'x','-','z','i','p','-','c','o','m','p','r','e','s','s','e','d',0};
static const WCHAR app_xgzipW[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
        'x','-','g','z','i','p','-','c','o','m','p','r','e','s','s','e','d',0};
static const WCHAR app_javaW[] = {'a','p','p','l','i','c','a','t','i','o','n','/','j','a','v','a',0};
static const WCHAR app_xmsdownloadW[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
        'x','-','m','s','d','o','w','n','l','o','a','d',0};
static const WCHAR text_plainW[] = {'t','e','x','t','/','p','l','a','i','n','\0'};
static const WCHAR app_octetstreamW[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
        'o','c','t','e','t','-','s','t','r','e','a','m','\0'};

static const struct {
    const WCHAR *mime;
    BOOL (*filter)(const BYTE *,DWORD);
} mime_filters_any_pos[] = {
    {text_htmlW,       text_html_filter},
    {text_xmlW,        text_xml_filter}
}, mime_filters[] = {
    {text_richtextW,   text_richtext_filter},
 /* {audio_xaiffW,     audio_xaiff_filter}, */
    {audio_basicW,     audio_basic_filter},
    {audio_wavW,       audio_wav_filter},
    {image_gifW,       image_gif_filter},
    {image_pjpegW,     image_pjpeg_filter},
    {image_tiffW,      image_tiff_filter},
    {image_xpngW,      image_xpng_filter},
 /* {image_xbitmapW,   image_xbitmap_filter}, */
    {image_bmpW,       image_bmp_filter},
 /* {image_xjgW,       image_xjg_filter}, */
 /* {image_xemfW,      image_xemf_filter}, */
 /* {image_xwmfW,      image_xwmf_filter}, */
    {video_aviW,       video_avi_filter},
    {video_mpegW,      video_mpeg_filter},
    {app_postscriptW,  application_postscript_filter},
 /* {app_base64W,      application_base64_filter}, */
 /* {app_macbinhex40W, application_macbinhex40_filter}, */
    {app_pdfW,         application_pdf_filter},
 /* {app_zcompressedW, application_xcompressed_filter}, */
    {app_xzipW,        application_xzip_filter},
    {app_xgzipW,       application_xgzip_filter},
    {app_javaW,        application_java_filter},
    {app_xmsdownloadW, application_xmsdownload},
    {text_plainW,      text_plain_filter},
    {app_octetstreamW, application_octet_stream_filter}
};

static BOOL is_known_mime_type(const WCHAR *mime)
{
    unsigned i;

    for(i=0; i < sizeof(mime_filters_any_pos)/sizeof(*mime_filters_any_pos); i++) {
        if(!strcmpW(mime, mime_filters_any_pos[i].mime))
            return TRUE;
    }

    for(i=0; i < sizeof(mime_filters)/sizeof(*mime_filters); i++) {
        if(!strcmpW(mime, mime_filters[i].mime))
            return TRUE;
    }

    return FALSE;
}

static HRESULT find_mime_from_buffer(const BYTE *buf, DWORD size, const WCHAR *proposed_mime, const WCHAR *url, WCHAR **ret_mime)
{
    int len, i, any_pos_mime = -1;
    const WCHAR *ret = NULL;

    if(!buf || !size) {
        if(!proposed_mime)
            return E_FAIL;

        len = strlenW(proposed_mime)+1;
        *ret_mime = CoTaskMemAlloc(len*sizeof(WCHAR));
        if(!*ret_mime)
            return E_OUTOFMEMORY;

        memcpy(*ret_mime, proposed_mime, len*sizeof(WCHAR));
        return S_OK;
    }

    if(proposed_mime && (!strcmpW(proposed_mime, app_octetstreamW)
                || !strcmpW(proposed_mime, text_plainW)))
        proposed_mime = NULL;

    if(proposed_mime) {
        ret = proposed_mime;

        for(i=0; i < sizeof(mime_filters_any_pos)/sizeof(*mime_filters_any_pos); i++) {
            if(!strcmpW(proposed_mime, mime_filters_any_pos[i].mime)) {
                any_pos_mime = i;
                for(len=size; len>0; len--) {
                    if(mime_filters_any_pos[i].filter(buf+size-len, len))
                        break;
                }
                if(!len)
                    ret = NULL;
                break;
            }
        }

        if(i == sizeof(mime_filters_any_pos)/sizeof(*mime_filters_any_pos)) {
            for(i=0; i < sizeof(mime_filters)/sizeof(*mime_filters); i++) {
                if(!strcmpW(proposed_mime, mime_filters[i].mime)) {
                    if(!mime_filters[i].filter(buf, size))
                        ret = NULL;
                    break;
                }
            }
        }
    }

    /* Looks like a bug in native implementation, html and xml mimes
     * are not looked for if none of them was proposed */
    if(!proposed_mime || any_pos_mime!=-1) {
        for(len=size; !ret && len>0; len--) {
            for(i=0; i<sizeof(mime_filters_any_pos)/sizeof(*mime_filters_any_pos); i++) {
                if(mime_filters_any_pos[i].filter(buf+size-len, len)) {
                    ret = mime_filters_any_pos[i].mime;
                    break;
                }
            }
        }
    }

    i=0;
    while(!ret) {
        if(mime_filters[i].filter(buf, size))
            ret = mime_filters[i].mime;
        i++;
    }

    if(any_pos_mime!=-1 && ret==text_plainW)
        ret = mime_filters_any_pos[any_pos_mime].mime;
    else if(proposed_mime && ret==app_octetstreamW) {
        for(len=size; ret==app_octetstreamW && len>0; len--) {
            if(!is_text_plain_char(buf[size-len]))
                break;
            for(i=0; i<sizeof(mime_filters_any_pos)/sizeof(*mime_filters_any_pos); i++) {
                if(mime_filters_any_pos[i].filter(buf+size-len, len)) {
                    ret = text_plainW;
                    break;
                }
            }
        }

        if(ret == app_octetstreamW)
            ret = proposed_mime;
    }

    if(url && (ret == app_octetstreamW || ret == text_plainW)) {
        WCHAR *url_mime;
        HRESULT hres;

        hres = find_mime_from_url(url, &url_mime);
        if(SUCCEEDED(hres)) {
            if(!is_known_mime_type(url_mime)) {
                *ret_mime = url_mime;
                return hres;
            }
            CoTaskMemFree(url_mime);
        }
    }

    TRACE("found %s for %s\n", debugstr_w(ret), debugstr_an((const char*)buf, min(32, size)));

    len = strlenW(ret)+1;
    *ret_mime = CoTaskMemAlloc(len*sizeof(WCHAR));
    if(!*ret_mime)
        return E_OUTOFMEMORY;

    memcpy(*ret_mime, ret, len*sizeof(WCHAR));
    return S_OK;
}

/***********************************************************************
 *           FindMimeFromData (URLMON.@)
 *
 * Determines the Multipurpose Internet Mail Extensions (MIME) type from the data provided.
 */
HRESULT WINAPI FindMimeFromData(LPBC pBC, LPCWSTR pwzUrl, LPVOID pBuffer,
        DWORD cbSize, LPCWSTR pwzMimeProposed, DWORD dwMimeFlags,
        LPWSTR* ppwzMimeOut, DWORD dwReserved)
{
    TRACE("(%p,%s,%p,%d,%s,0x%x,%p,0x%x)\n", pBC, debugstr_w(pwzUrl), pBuffer, cbSize,
            debugstr_w(pwzMimeProposed), dwMimeFlags, ppwzMimeOut, dwReserved);

    if(dwMimeFlags)
        WARN("dwMimeFlags=%08x\n", dwMimeFlags);
    if(dwReserved)
        WARN("dwReserved=%d\n", dwReserved);

    /* pBC seams to not be used */

    if(!ppwzMimeOut || (!pwzUrl && !pBuffer))
        return E_INVALIDARG;

    if(pwzMimeProposed || pBuffer)
        return find_mime_from_buffer(pBuffer, cbSize, pwzMimeProposed, pwzUrl, ppwzMimeOut);

    return find_mime_from_url(pwzUrl, ppwzMimeOut);
}
