/* DirectMusicScript Main
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


#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "initguid.h"
#include "dmusici.h"

#include "dmscript_private.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmscript);

typedef struct {
        IClassFactory IClassFactory_iface;
        HRESULT (*fnCreateInstance)(REFIID riid, void **ppv, IUnknown *pUnkOuter);
} IClassFactoryImpl;

static HRESULT create_unimpl_instance(REFIID riid, void **ppv, IUnknown *pUnkOuter)
{
        FIXME("(%p, %s, %p) stub\n", pUnkOuter, debugstr_dmguid(riid), ppv);

        return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************
 *      IClassFactory implementation
 */
static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
        return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
        if (ppv == NULL)
                return E_POINTER;

        if (IsEqualGUID(&IID_IUnknown, riid))
                TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        else if (IsEqualGUID(&IID_IClassFactory, riid))
                TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        else {
                FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
                *ppv = NULL;
                return E_NOINTERFACE;
        }

        *ppv = iface;
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
        return 2; /* non-heap based object */
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
        return 1; /* non-heap based object */
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
        REFIID riid, void **ppv)
{
        IClassFactoryImpl *This = impl_from_IClassFactory(iface);

        TRACE ("(%p, %s, %p)\n", pUnkOuter, debugstr_dmguid(riid), ppv);

        return This->fnCreateInstance(riid, ppv, pUnkOuter);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
        TRACE("(%d)\n", dolock);
        return S_OK;
}

static const IClassFactoryVtbl classfactory_vtbl = {
        ClassFactory_QueryInterface,
        ClassFactory_AddRef,
        ClassFactory_Release,
        ClassFactory_CreateInstance,
        ClassFactory_LockServer
};

static IClassFactoryImpl ScriptAutoImplSegment_CF = {{&classfactory_vtbl}, create_unimpl_instance};
static IClassFactoryImpl ScriptTrack_CF = {{&classfactory_vtbl},
                                           DMUSIC_CreateDirectMusicScriptTrack};
static IClassFactoryImpl AudioVBScript_CF = {{&classfactory_vtbl}, create_unimpl_instance};
static IClassFactoryImpl Script_CF = {{&classfactory_vtbl}, DMUSIC_CreateDirectMusicScriptImpl};
static IClassFactoryImpl ScriptAutoImplPerformance_CF = {{&classfactory_vtbl},
                                                         create_unimpl_instance};
static IClassFactoryImpl ScriptSourceCodeLoader_CF = {{&classfactory_vtbl}, create_unimpl_instance};
static IClassFactoryImpl ScriptAutoImplSegmentState_CF = {{&classfactory_vtbl},
                                                          create_unimpl_instance};
static IClassFactoryImpl ScriptAutoImplAudioPathConfig_CF = {{&classfactory_vtbl},
                                                             create_unimpl_instance};
static IClassFactoryImpl ScriptAutoImplAudioPath_CF = {{&classfactory_vtbl},
                                                       create_unimpl_instance};
static IClassFactoryImpl ScriptAutoImplSong_CF = {{&classfactory_vtbl}, create_unimpl_instance};


/******************************************************************
 *		DllGetClassObject (DMSCRIPT.@)
 *
 *
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("(%s, %s, %p)\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
    if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpSegment) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptAutoImplSegment_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptTrack) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptTrack_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_AudioVBScript) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &AudioVBScript_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScript) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &Script_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpPerformance) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptAutoImplPerformance_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptSourceCodeLoader) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptSourceCodeLoader_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpSegmentState) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptAutoImplSegmentState_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpAudioPathConfig) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptAutoImplAudioPathConfig_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpAudioPath) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptAutoImplAudioPath_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	} else if (IsEqualCLSID (rclsid, &CLSID_DirectMusicScriptAutoImpSong) && IsEqualIID (riid, &IID_IClassFactory)) {
      *ppv = &ScriptAutoImplSong_CF;
      IClassFactory_AddRef((IClassFactory*)*ppv);
      return S_OK;
	}		

    WARN("(%s, %s, %p): no interface found.\n", debugstr_dmguid(rclsid), debugstr_dmguid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
