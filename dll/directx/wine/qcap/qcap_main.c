/*
 * Qcap implementation, dllentry points
 *
 * Copyright (C) 2003 Dominik Strasser
 * Copyright (C) 2005 Rolf Kalbermatter
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
#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "objbase.h"
#include "uuids.h"
#include "strmif.h"

#include "qcap_main.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

static LONG objects_ref = 0;

static const WCHAR wAudioCaptureFilter[] =
{'A','u','d','i','o',' ','C','a','p','t','u','r','e',' ','F','i','l','t','e','r',0};
static const WCHAR wAVICompressor[] =
{'A','V','I',' ','C','o','m','p','r','e','s','s','o','r',0};
static const WCHAR wVFWCaptFilter[] =
{'V','F','W',' ','C','a','p','t','u','r','e',' ','F','i','l','t','e','r',0};
static const WCHAR wVFWCaptFilterProp[] =
{'V','F','W',' ','C','a','p','t','u','r','e',' ','F','i','l','t','e','r',' ',
 'P','r','o','p','e','r','t','y',' ','P','a','g','e',0};
static const WCHAR wAVIMux[] =
{'A','V','I',' ','m','u','x',0};
static const WCHAR wAVIMuxPropPage[] =
{'A','V','I',' ','m','u','x',' ','P','r','o','p','e','r','t','y',' ','P','a','g','e',0};
static const WCHAR wAVIMuxPropPage1[] =
{'A','V','I',' ','m','u','x',' ','P','r','o','p','e','r','t','y',' ','P','a','g','e','1',0};
static const WCHAR wFileWriter[] =
{'F','i','l','e',' ','W','r','i','t','e','r',0};
static const WCHAR wCaptGraphBuilder[] =
{'C','a','p','t','u','r','e',' ','G','r','a','p','h',' ','B','u','i','l','d','e','r',0};
static const WCHAR wCaptGraphBuilder2[] =
{'C','a','p','t','u','r','e',' ','G','r','a','p','h',' ','B','u','i','l','d','e','r','2',0};
static const WCHAR wInfPinTeeFilter[] =
{'I','n','f','i','n','i','t','e',' ','P','i','n',' ','T','e','e',' ','F','i',
 'l','t','e','r',0};
static const WCHAR wSmartTeeFilter[] =
{'S','m','a','r','t',' ','T','e','e',' ','F','i','l','t','e','r',0};
static const WCHAR wAudioInMixerProp[] =
{'A','u','d','i','o','I','n','p','u','t','M','i','x','e','r',' ','P','r','o',
 'p','e','r','t','y',' ','P','a','g','e',0};
 
FactoryTemplate const g_Templates[] = {
    {
        wAudioCaptureFilter,
        &CLSID_AudioRecord,
        QCAP_createAudioCaptureFilter,
        NULL
    },{
        wAVICompressor,
        &CLSID_AVICo,
        QCAP_createAVICompressor,
        NULL
    },{
        wVFWCaptFilter,
        &CLSID_VfwCapture,
        QCAP_createVFWCaptureFilter,
        NULL
    },{
        wVFWCaptFilterProp,
        &CLSID_CaptureProperties,
        NULL, /* FIXME: Implement QCAP_createVFWCaptureFilterPropertyPage */
        NULL
    },{
        wAVIMux,
        &CLSID_AviDest,
        QCAP_createAVIMux,
        NULL
    },{
        wAVIMuxPropPage,
        &CLSID_AviMuxProptyPage,
        NULL, /* FIXME: Implement QCAP_createAVIMuxPropertyPage */
        NULL
    },{
        wAVIMuxPropPage1,
        &CLSID_AviMuxProptyPage1,
        NULL, /* FIXME: Implement QCAP_createAVIMuxPropertyPage1 */
        NULL
    },{
        wFileWriter,
        &CLSID_FileWriter,
        NULL, /* FIXME: Implement QCAP_createFileWriter */
        NULL
    },{
        wCaptGraphBuilder,
        &CLSID_CaptureGraphBuilder,
        QCAP_createCaptureGraphBuilder2,
        NULL
    },{
        wCaptGraphBuilder2,
        &CLSID_CaptureGraphBuilder2,
        QCAP_createCaptureGraphBuilder2,
        NULL
    },{
        wInfPinTeeFilter, 
        &CLSID_InfTee,
        NULL, /* FIXME: Implement QCAP_createInfinitePinTeeFilter */
        NULL
    },{
        wSmartTeeFilter,
        &CLSID_SmartTee,
        QCAP_createSmartTeeFilter,
        NULL
    },{
        wAudioInMixerProp,
        &CLSID_AudioInputMixerProperties,
        NULL, /* FIXME: Implement QCAP_createAudioInputMixerPropertyPage */
        NULL
    }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

/***********************************************************************
 *    Dll EntryPoint (QCAP.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    return STRMBASE_DllMain(hInstDLL,fdwReason,lpv);
}

/***********************************************************************
 *    DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return STRMBASE_DllGetClassObject( rclsid, riid, ppv );
}

/***********************************************************************
 *    DllRegisterServer (QCAP.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    TRACE("()\n");
    return AMovieDllRegisterServer2(TRUE);
}

/***********************************************************************
 *    DllUnregisterServer (QCAP.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("\n");
    return AMovieDllRegisterServer2(FALSE);
}

/***********************************************************************
 *    DllCanUnloadNow (QCAP.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("\n");

    if (STRMBASE_DllCanUnloadNow() == S_OK && objects_ref == 0)
        return S_OK;
    return S_FALSE;
}

DWORD ObjectRefCount(BOOL increment)
{
    if (increment)
        return InterlockedIncrement(&objects_ref);
    return InterlockedDecrement(&objects_ref);
}
