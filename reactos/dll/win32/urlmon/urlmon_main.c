/*
 * UrlMon
 *
 * Copyright (c) 2000 Patrik Stridvall
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

#include <stdarg.h>

#include "urlmon_main.h"

#include "winreg.h"

#define NO_SHLWAPI_REG
#include "shlwapi.h"
#include "wine/debug.h"

#include "urlmon.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

LONG URLMON_refCount = 0;

HINSTANCE URLMON_hInstance = 0;
static HMODULE hCabinet = NULL;

static void init_session(BOOL);

/***********************************************************************
 *		DllMain (URLMON.init)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hinstDLL, fdwReason, fImpLoad);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        URLMON_hInstance = hinstDLL;
        init_session(TRUE);
	break;

    case DLL_PROCESS_DETACH:
        if (hCabinet)
            FreeLibrary(hCabinet);
        hCabinet = NULL;
        init_session(FALSE);
        URLMON_hInstance = 0;
	break;
    }
    return TRUE;
}


/***********************************************************************
 *		DllInstall (URLMON.@)
 */
HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
  FIXME("(%s, %s): stub\n", bInstall?"TRUE":"FALSE",
	debugstr_w(cmdline));

  return S_OK;
}

/***********************************************************************
 *		DllCanUnloadNow (URLMON.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return URLMON_refCount != 0 ? S_FALSE : S_OK;
}



/******************************************************************************
 * Urlmon ClassFactory
 */
typedef struct {
    const IClassFactoryVtbl *lpClassFactoryVtbl;

    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
} ClassFactory;

#define CLASSFACTORY(x)  ((IClassFactory*)  &(x)->lpClassFactoryVtbl)

static HRESULT WINAPI CF_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IClassFactory)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
	IUnknown_AddRef((IUnknown*)*ppv);
	return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI CF_AddRef(IClassFactory *iface)
{
    URLMON_LockModule();
    return 2;
}

static ULONG WINAPI CF_Release(IClassFactory *iface)
{
    URLMON_UnlockModule();
    return 1;
}


static HRESULT WINAPI CF_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                        REFIID riid, LPVOID *ppobj)
{
    ClassFactory *This = (ClassFactory*)iface;
    HRESULT hres;
    LPUNKNOWN punk;
    
    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    if(SUCCEEDED(hres = This->pfnCreateInstance(pOuter, (LPVOID *) &punk))) {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
        IUnknown_Release(punk);
    }
    return hres;
}

static HRESULT WINAPI CF_LockServer(LPCLASSFACTORY iface,BOOL dolock)
{
    TRACE("(%d)\n", dolock);

    if (dolock)
	   URLMON_LockModule();
    else
	   URLMON_UnlockModule();

    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl =
{
    CF_QueryInterface,
    CF_AddRef,
    CF_Release,
    CF_CreateInstance,
    CF_LockServer
};

static const ClassFactory FileProtocolCF =
    { &ClassFactoryVtbl, FileProtocol_Construct};
static const ClassFactory FtpProtocolCF =
    { &ClassFactoryVtbl, FtpProtocol_Construct};
static const ClassFactory HttpProtocolCF =
    { &ClassFactoryVtbl, HttpProtocol_Construct};
static const ClassFactory HttpSProtocolCF =
    { &ClassFactoryVtbl, HttpSProtocol_Construct};
static const ClassFactory MkProtocolCF =
    { &ClassFactoryVtbl, MkProtocol_Construct};
static const ClassFactory SecurityManagerCF =
    { &ClassFactoryVtbl, SecManagerImpl_Construct};
static const ClassFactory ZoneManagerCF =
    { &ClassFactoryVtbl, ZoneMgrImpl_Construct};
 
struct object_creation_info
{
    const CLSID *clsid;
    IClassFactory *cf;
    LPCWSTR protocol;
};

static const WCHAR wszFile[] = {'f','i','l','e',0};
static const WCHAR wszFtp[]  = {'f','t','p',0};
static const WCHAR wszHttp[] = {'h','t','t','p',0};
static const WCHAR wszHttps[] = {'h','t','t','p','s',0};
static const WCHAR wszMk[]   = {'m','k',0};

static const struct object_creation_info object_creation[] =
{
    { &CLSID_FileProtocol,            CLASSFACTORY(&FileProtocolCF),    wszFile },
    { &CLSID_FtpProtocol,             CLASSFACTORY(&FtpProtocolCF),     wszFtp  },
    { &CLSID_HttpProtocol,            CLASSFACTORY(&HttpProtocolCF),    wszHttp },
    { &CLSID_HttpSProtocol,           CLASSFACTORY(&HttpSProtocolCF),   wszHttps },
    { &CLSID_MkProtocol,              CLASSFACTORY(&MkProtocolCF),      wszMk },
    { &CLSID_InternetSecurityManager, CLASSFACTORY(&SecurityManagerCF), NULL    },
    { &CLSID_InternetZoneManager,     CLASSFACTORY(&ZoneManagerCF),     NULL    }
};

static void init_session(BOOL init)
{
    unsigned int i;

    for(i=0; i < sizeof(object_creation)/sizeof(object_creation[0]); i++) {

        if(object_creation[i].protocol)
            register_urlmon_namespace(object_creation[i].cf, object_creation[i].clsid,
                                      object_creation[i].protocol, init);
    }
}

/*******************************************************************************
 * DllGetClassObject [URLMON.@]
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
    
    for (i=0; i < sizeof(object_creation)/sizeof(object_creation[0]); i++)
    {
	if (IsEqualGUID(object_creation[i].clsid, rclsid))
	    return IClassFactory_QueryInterface(object_creation[i].cf, riid, ppv);
    }

    FIXME("%s: no class found.\n", debugstr_guid(rclsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}


/***********************************************************************
 *		DllRegisterServerEx (URLMON.@)
 */
HRESULT WINAPI DllRegisterServerEx(void)
{
    FIXME("(void): stub\n");

    return E_FAIL;
}

/**************************************************************************
 *                 UrlMkSetSessionOption (URLMON.@)
 */
HRESULT WINAPI UrlMkSetSessionOption(DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength,
 					DWORD Reserved)
{
    FIXME("(%#x, %p, %#x): stub\n", dwOption, pBuffer, dwBufferLength);

    return S_OK;
}

static const CHAR Agent[] = "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)";

/**************************************************************************
 *                 ObtainUserAgentString (URLMON.@)
 */
HRESULT WINAPI ObtainUserAgentString(DWORD dwOption, LPSTR pcszUAOut, DWORD *cbSize)
{
    FIXME("(%d, %p, %p): stub\n", dwOption, pcszUAOut, cbSize);

    if (pcszUAOut == NULL || cbSize == NULL)
        return E_INVALIDARG;

    if (*cbSize < sizeof(Agent))
    {
        *cbSize = sizeof(Agent);
        return E_OUTOFMEMORY;
    }

    if (sizeof(Agent) < *cbSize)
        *cbSize = sizeof(Agent);
    lstrcpynA(pcszUAOut, Agent, *cbSize);

    return S_OK;
}

/**************************************************************************
 *                 IsValidURL (URLMON.@)
 * 
 * Determines if a specified string is a valid URL.
 *
 * PARAMS
 *  pBC        [I] ignored, must be NULL.
 *  szURL      [I] string that represents the URL in question.
 *  dwReserved [I] reserved and must be zero.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: S_FALSE.
 *  returns E_INVALIDARG if one or more of the args is invalid.
 *
 * TODO:
 *  test functionality against windows to see what a valid URL is.
 */
HRESULT WINAPI IsValidURL(LPBC pBC, LPCWSTR szURL, DWORD dwReserved)
{
    FIXME("(%p, %s, %d): stub\n", pBC, debugstr_w(szURL), dwReserved);
    
    if (pBC != NULL || dwReserved != 0)
        return E_INVALIDARG;
    
    return S_OK;
}

/**************************************************************************
 *                 FaultInIEFeature (URLMON.@)
 *
 *  Undocumented.  Appears to be used by native shdocvw.dll.
 */
HRESULT WINAPI FaultInIEFeature( HWND hwnd, uCLSSPEC * pClassSpec,
                                 QUERYCONTEXT *pQuery, DWORD flags )
{
    FIXME("%p %p %p %08x\n", hwnd, pClassSpec, pQuery, flags);
    return E_NOTIMPL;
}

/**************************************************************************
 *                 CoGetClassObjectFromURL (URLMON.@)
 */
HRESULT WINAPI CoGetClassObjectFromURL( REFCLSID rclsid, LPCWSTR szCodeURL, DWORD dwFileVersionMS,
                                        DWORD dwFileVersionLS, LPCWSTR szContentType,
                                        LPBINDCTX pBindCtx, DWORD dwClsContext, LPVOID pvReserved,
                                        REFIID riid, LPVOID *ppv )
{
    FIXME("(%s %s %d %d %s %p %d %p %s %p) Stub!\n", debugstr_guid(rclsid), debugstr_w(szCodeURL),
	dwFileVersionMS, dwFileVersionLS, debugstr_w(szContentType), pBindCtx, dwClsContext, pvReserved,
	debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

/***********************************************************************
 *           ReleaseBindInfo (URLMON.@)
 *
 * Release the resources used by the specified BINDINFO structure.
 *
 * PARAMS
 *  pbindinfo [I] BINDINFO to release.
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI ReleaseBindInfo(BINDINFO* pbindinfo)
{
    DWORD size;

    TRACE("(%p)\n", pbindinfo);

    if(!pbindinfo || !(size = pbindinfo->cbSize))
        return;

    CoTaskMemFree(pbindinfo->szExtraInfo);
    ReleaseStgMedium(&pbindinfo->stgmedData);

    if(offsetof(BINDINFO, szExtraInfo) < size)
        CoTaskMemFree(pbindinfo->szCustomVerb);

    if(pbindinfo->pUnk && offsetof(BINDINFO, pUnk) < size)
        IUnknown_Release(pbindinfo->pUnk);

    memset(pbindinfo, 0, size);
    pbindinfo->cbSize = size;
}

/***********************************************************************
 *           CopyStgMedium (URLMON.@)
 */
HRESULT WINAPI CopyStgMedium(const STGMEDIUM *src, STGMEDIUM *dst)
{
    TRACE("(%p %p)\n", src, dst);

    if(!src || !dst)
        return E_POINTER;

    *dst = *src;

    switch(dst->tymed) {
    case TYMED_NULL:
        break;
    case TYMED_FILE:
        if(src->u.lpszFileName && !src->pUnkForRelease) {
            DWORD size = (strlenW(src->u.lpszFileName)+1)*sizeof(WCHAR);
            dst->u.lpszFileName = CoTaskMemAlloc(size);
            memcpy(dst->u.lpszFileName, src->u.lpszFileName, size);
        }
        break;
    case TYMED_ISTREAM:
        if(dst->u.pstm)
            IStream_AddRef(dst->u.pstm);
        break;
    case TYMED_ISTORAGE:
        if(dst->u.pstg)
            IStorage_AddRef(dst->u.pstg);
        break;
    default:
        FIXME("Unimplemented tymed %d\n", src->tymed);
    }

    if(dst->pUnkForRelease)
        IUnknown_AddRef(dst->pUnkForRelease);

    return S_OK;
}

static BOOL text_richtext_filter(const BYTE *b, DWORD size)
{
    return size > 5 && !memcmp(b, "{\\rtf", 5);
}

static BOOL text_html_filter(const BYTE *b, DWORD size)
{
    DWORD i;

    if(size < 5)
        return FALSE;

    for(i=0; i < size-5; i++) {
        if(b[i] == '<'
           && (b[i+1] == 'h' || b[i+1] == 'H')
           && (b[i+2] == 't' || b[i+2] == 'T')
           && (b[i+3] == 'm' || b[i+3] == 'M')
           && (b[i+4] == 'l' || b[i+4] == 'L'))
            return TRUE;
    }

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
    return size > 2 && b[0] == 0x4d && b[1] == 0x4d;
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

static BOOL text_plain_filter(const BYTE *b, DWORD size)
{
    const BYTE *ptr;

    for(ptr = b; ptr < b+size-1; ptr++) {
        if(*ptr < 0x20 && *ptr != '\n' && *ptr != '\r' && *ptr != '\t')
            return FALSE;
    }

    return TRUE;
}

static BOOL application_octet_stream_filter(const BYTE *b, DWORD size)
{
    return TRUE;
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

    if(pwzMimeProposed && (!pBuffer || (pBuffer && !cbSize))) {
        DWORD len;

        if(!pwzMimeProposed)
            return E_FAIL;

        len = strlenW(pwzMimeProposed)+1;
        *ppwzMimeOut = CoTaskMemAlloc(len*sizeof(WCHAR));
        memcpy(*ppwzMimeOut, pwzMimeProposed, len*sizeof(WCHAR));
        return S_OK;
    }

    if(pBuffer) {
        const BYTE *buf = pBuffer;
        DWORD len;
        LPCWSTR ret = NULL;
        unsigned int i;

        static const WCHAR wszTextHtml[] = {'t','e','x','t','/','h','t','m','l',0};
        static const WCHAR wszTextRichtext[] = {'t','e','x','t','/','r','i','c','h','t','e','x','t',0};
        static const WCHAR wszAudioBasic[] = {'a','u','d','i','o','/','b','a','s','i','c',0};
        static const WCHAR wszAudioWav[] = {'a','u','d','i','o','/','w','a','v',0};
        static const WCHAR wszImageGif[] = {'i','m','a','g','e','/','g','i','f',0};
        static const WCHAR wszImagePjpeg[] = {'i','m','a','g','e','/','p','j','p','e','g',0};
        static const WCHAR wszImageTiff[] = {'i','m','a','g','e','/','t','i','f','f',0};
        static const WCHAR wszImageXPng[] = {'i','m','a','g','e','/','x','-','p','n','g',0};
        static const WCHAR wszImageBmp[] = {'i','m','a','g','e','/','b','m','p',0};
        static const WCHAR wszVideoAvi[] = {'v','i','d','e','o','/','a','v','i',0};
        static const WCHAR wszVideoMpeg[] = {'v','i','d','e','o','/','m','p','e','g',0};
        static const WCHAR wszAppPostscript[] =
            {'a','p','p','l','i','c','a','t','i','o','n','/','p','o','s','t','s','c','r','i','p','t',0};
        static const WCHAR wszAppPdf[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
            'p','d','f',0};
        static const WCHAR wszAppXZip[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
            'x','-','z','i','p','-','c','o','m','p','r','e','s','s','e','d',0};
        static const WCHAR wszAppXGzip[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
            'x','-','g','z','i','p','-','c','o','m','p','r','e','s','s','e','d',0};
        static const WCHAR wszAppJava[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
            'j','a','v','a',0};
        static const WCHAR wszAppXMSDownload[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
            'x','-','m','s','d','o','w','n','l','o','a','d',0};
        static const WCHAR wszTextPlain[] = {'t','e','x','t','/','p','l','a','i','n','\0'};
        static const WCHAR wszAppOctetStream[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
            'o','c','t','e','t','-','s','t','r','e','a','m','\0'};

        static const struct {
            LPCWSTR mime;
            BOOL (*filter)(const BYTE *,DWORD);
        } mime_filters[] = {
            {wszTextHtml,       text_html_filter},
            {wszTextRichtext,   text_richtext_filter},
         /* {wszAudioXAiff,     audio_xaiff_filter}, */
            {wszAudioBasic,     audio_basic_filter},
            {wszAudioWav,       audio_wav_filter},
            {wszImageGif,       image_gif_filter},
            {wszImagePjpeg,     image_pjpeg_filter},
            {wszImageTiff,      image_tiff_filter},
            {wszImageXPng,      image_xpng_filter},
         /* {wszImageXBitmap,   image_xbitmap_filter}, */
            {wszImageBmp,       image_bmp_filter},
         /* {wszImageXJg,       image_xjg_filter}, */
         /* {wszImageXEmf,      image_xemf_filter}, */
         /* {wszImageXWmf,      image_xwmf_filter}, */
            {wszVideoAvi,       video_avi_filter},
            {wszVideoMpeg,      video_mpeg_filter},
            {wszAppPostscript,  application_postscript_filter},
         /* {wszAppBase64,      application_base64_filter}, */
         /* {wszAppMacbinhex40, application_macbinhex40_filter}, */
            {wszAppPdf,         application_pdf_filter},
         /* {wszAppXCompressed, application_xcompressed_filter}, */
            {wszAppXZip,        application_xzip_filter},
            {wszAppXGzip,       application_xgzip_filter},
            {wszAppJava,        application_java_filter},
            {wszAppXMSDownload, application_xmsdownload},
            {wszTextPlain,      text_plain_filter},
            {wszAppOctetStream, application_octet_stream_filter}
        };

        if(!cbSize)
            return E_FAIL;

        if(pwzMimeProposed && strcmpW(pwzMimeProposed, wszAppOctetStream)) {
            for(i=0; i < sizeof(mime_filters)/sizeof(*mime_filters); i++) {
                if(!strcmpW(pwzMimeProposed, mime_filters[i].mime))
                    break;
            }

            if(i == sizeof(mime_filters)/sizeof(*mime_filters)
                    || mime_filters[i].filter(buf, cbSize)) {
                len = strlenW(pwzMimeProposed)+1;
                *ppwzMimeOut = CoTaskMemAlloc(len*sizeof(WCHAR));
                memcpy(*ppwzMimeOut, pwzMimeProposed, len*sizeof(WCHAR));
                return S_OK;
            }
        }

        i=0;
        while(!ret) {
            if(mime_filters[i].filter(buf, cbSize))
                ret = mime_filters[i].mime;
            i++;
        }

        TRACE("found %s for data\n"
              "%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
              debugstr_w(ret), buf[0],buf[1],buf[2],buf[3], buf[4],buf[5],buf[6],buf[7],
              buf[8],buf[9],buf[10],buf[11], buf[12],buf[13],buf[14],buf[15]);

        if(pwzMimeProposed) {
            if(i == sizeof(mime_filters)/sizeof(*mime_filters))
                ret = pwzMimeProposed;

            /* text/html is a special case */
            if(!strcmpW(pwzMimeProposed, wszTextHtml) && !strcmpW(ret, wszTextPlain))
                ret = wszTextHtml;
        }

        len = strlenW(ret)+1;
        *ppwzMimeOut = CoTaskMemAlloc(len*sizeof(WCHAR));
        memcpy(*ppwzMimeOut, ret, len*sizeof(WCHAR));
        return S_OK;
    }

    if(pwzUrl) {
        HKEY hkey;
        DWORD res, size;
        LPCWSTR ptr;
        WCHAR mime[64];

        static const WCHAR wszContentType[] =
                {'C','o','n','t','e','n','t',' ','T','y','p','e','\0'};

        ptr = strrchrW(pwzUrl, '.');
        if(!ptr)
            return E_FAIL;

        res = RegOpenKeyW(HKEY_CLASSES_ROOT, ptr, &hkey);
        if(res != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(res);

        size = sizeof(mime);
        res = RegQueryValueExW(hkey, wszContentType, NULL, NULL, (LPBYTE)mime, &size);
        RegCloseKey(hkey);
        if(res != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(res);

        *ppwzMimeOut = CoTaskMemAlloc(size);
        memcpy(*ppwzMimeOut, mime, size);
        return S_OK;
    }

    return E_FAIL;
}

/***********************************************************************
 *           GetClassFileOrMime (URLMON.@)
 *
 * Determines the class ID from the bind context, file name or MIME type.
 */
HRESULT WINAPI GetClassFileOrMime(LPBC pBC, LPCWSTR pszFilename,
        LPVOID pBuffer, DWORD cbBuffer, LPCWSTR pszMimeType, DWORD dwReserved,
        CLSID *pclsid)
{
    FIXME("(%p, %s, %p, %d, %p, 0x%08x, %p): stub\n", pBC,
        debugstr_w(pszFilename), pBuffer, cbBuffer, debugstr_w(pszMimeType),
        dwReserved, pclsid);
    return E_NOTIMPL;
}

/***********************************************************************
 * Extract (URLMON.@)
 */
HRESULT WINAPI Extract(void *dest, LPCSTR szCabName)
{
    HRESULT (WINAPI *pExtract)(void *, LPCSTR);

    if (!hCabinet)
        hCabinet = LoadLibraryA("cabinet.dll");

    if (!hCabinet) return HRESULT_FROM_WIN32(GetLastError());
    pExtract = (void *)GetProcAddress(hCabinet, "Extract");
    if (!pExtract) return HRESULT_FROM_WIN32(GetLastError());

    return pExtract(dest, szCabName);
}

/***********************************************************************
 *           IsLoggingEnabledA (URLMON.@)
 */
BOOL WINAPI IsLoggingEnabledA(LPCSTR url)
{
    FIXME("(%s)\n", debugstr_a(url));
    return FALSE;
}

/***********************************************************************
 *           IsLoggingEnabledW (URLMON.@)
 */
BOOL WINAPI IsLoggingEnabledW(LPCWSTR url)
{
    FIXME("(%s)\n", debugstr_w(url));
    return FALSE;
}
