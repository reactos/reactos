// Create Instance functions

#ifndef _SCCLS_H_
#define _SCCLS_H_

//
// global object array - used for class factory, auto registration, type libraries, oc information
//
typedef struct tagOBJECTINFO
{
#ifdef __cplusplus
    void *cf;
#else
    const IClassFactoryVtbl *cf;
#endif
    CLSID const* pclsid;
    HRESULT (*pfnCreateInstance)(IUnknown* pUnkOuter, IUnknown** ppunk, const struct tagOBJECTINFO *);

    // for OCs and automation objects:
    IID const* piid;
    IID const* piidEvents;
    long lVersion;
    DWORD dwOleMiscFlags;
    DWORD dwClassFactFlags;
} OBJECTINFO;

typedef OBJECTINFO const * LPCOBJECTINFO;

#define OIF_ALLOWAGGREGATION  0x0001



#define VERSION_2 2 // so we don't get confused by too many integers
#define VERSION_1 1
#define VERSION_0 0
#define COCREATEONLY NULL,NULL,VERSION_0,0,0 // piid,piidEvents,lVersion,dwOleMiscFlags,dwClassFactFlags
#define COCREATEONLY_NOFLAGS NULL,NULL,VERSION_0,0 // piid,piidEvents,lVersion,dwOleMiscFlags



STDAPI  CShellAppManager_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CEnumInstalledApps_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
#ifndef DOWNLEVEL_PLATFORM
STDAPI  CDarwinAppPublisher_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
#endif //DOWNLEVEL_PLATFORM

// to save some typing:
#define CLSIDOFOBJECT(p)          (*((p)->_pObjectInfo->pclsid))
#define VERSIONOFOBJECT(p)          ((p)->_pObjectInfo->lVersion)
#define EVENTIIDOFCONTROL(p)      (*((p)->_pObjectInfo->piidEvents))
#define OLEMISCFLAGSOFCONTROL(p)    ((p)->_pObjectInfo->dwOleMiscFlags)

extern const OBJECTINFO g_ObjectInfo[]; // sccls.c

STDAPI GetClassObject(REFCLSID rclsid, REFIID riid, void **ppv);

#endif // _SCCLS_H_

