#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */

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

    // for automatic registration, type library searching, etc
    int nObjectType;        // OI_ flag
    LPTSTR pszName;         // "Shell.Browser" (in registry -- vb name is from type lib)
    LPTSTR pszFriendlyName; // "Microsoft Shell Browser" (can be string resource id)
    IID const* piid;
    IID const* piidEvents;
    long lVersion;
    DWORD dwOleMiscFlags;
    int nidToolbarBitmap;
} OBJECTINFO;
typedef OBJECTINFO const* LPCOBJECTINFO;

#define VERSION_1 1 // so we don't get confused by too many integers
#define VERSION_0 0

#define OI_NONE          0
#define OI_UNKNOWN       1
#define OI_COCREATEABLE  1
#define OI_AUTOMATION    2
#define OI_CONTROL       3


// to save some typing:
#define CLSIDOFOBJECT(p)          (*((p)->_pObjectInfo->pclsid))
#define NAMEOFOBJECT(p)             ((p)->_pObjectInfo->pszName)
#define INTERFACEOFOBJECT(p)      (*((p)->_pObjectInfo->piid))
#define VERSIONOFOBJECT(p)          ((p)->_pObjectInfo->lVersion)
#define EVENTIIDOFCONTROL(p)      (*((p)->_pObjectInfo->piidEvents))
#define OLEMISCFLAGSOFCONTROL(p)    ((p)->_pObjectInfo->dwOleMiscFlags)
#define BITMAPIDOFCONTROL(p)        ((p)->_pObjectInfo->nidToolbarBitmap)

extern OBJECTINFO g_ObjectInfo[]; 
extern char g_szLibName[]; 
extern LCID g_lcidLocale; 

void DllAddRef(void);
void DllRelease(void);

extern CRITICAL_SECTION	g_cs;		// per-instance

#ifdef __cplusplus
}
#endif  /* __cplusplus */
