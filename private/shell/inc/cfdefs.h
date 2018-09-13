#ifndef _STATIC_CLASS_FACTORY_
#define _STATIC_CLASS_FACTORY_

#define VERSION_2 2 // so we don't get confused by too many integers
#define VERSION_1 1
#define VERSION_0 0
#define COCREATEONLY NULL,NULL,VERSION_0,0,0 // piid,piidEvents,lVersion,dwOleMiscFlags,dwClassFactFlags
#define COCREATEONLY_NOFLAGS NULL,NULL,VERSION_0,0 // piid,piidEvents,lVersion,dwOleMiscFlags

/*
 * Class Factory Implementation for C++ without CTRStartup required.
 */

#ifdef __cplusplus

#ifdef UNIX

#define STDMETHODX  STDMETHOD
#define STDMETHODX_ STDMETHOD_

#define DECLARE_CLASS_FACTORY(cf)                                \
   class cf: public IClassFactory                                \
   {                                                             \
     public:                                                     \
       STDMETHODX (QueryInterface)(REFIID, void **);             \
       STDMETHODX_(ULONG, AddRef)();                             \
       STDMETHODX_(ULONG, Release)();                            \
                                                                 \
       STDMETHODX (CreateInstance)(IUnknown *, REFIID, void **); \
       STDMETHODX (LockServer)(BOOL);                            \
   }                                                             \


#else  // UNIX

#define STDMETHODX(fn)      HRESULT STDMETHODCALLTYPE fn
#define STDMETHODX_(ret,fn) ret STDMETHODCALLTYPE fn

#define DECLARE_CLASS_FACTORY(cf)                                \
   class cf                                                      \
   {                                                             \
     public:                                                     \
       IClassFactory *vtable;                                    \
       STDMETHODX (QueryInterface)(REFIID, void **);             \
       STDMETHODX_(ULONG, AddRef)();                             \
       STDMETHODX_(ULONG, Release)();                            \
                                                                 \
       STDMETHODX (CreateInstance)(IUnknown *, REFIID, void **); \
       STDMETHODX (LockServer)(BOOL);                            \
   }                                                             \

#endif // UNIX


DECLARE_CLASS_FACTORY( CClassFactory );


struct IClassFactoryVtbl
{
    // IUnknown
    HRESULT (STDMETHODCALLTYPE CClassFactory::*QueryInterface)(REFIID riid, void ** ppvObj);
    ULONG (STDMETHODCALLTYPE CClassFactory::*AddRef)();
    ULONG (STDMETHODCALLTYPE CClassFactory::*Release)();
    
    // IClassFactory
    HRESULT (STDMETHODCALLTYPE CClassFactory::*CreateInstance)(IUnknown *pUnkOuter, REFIID riid, void ** ppvObject);
    HRESULT (STDMETHODCALLTYPE CClassFactory::*LockServer)(BOOL);
};

typedef struct IClassFactoryVtbl IClassFactoryVtbl;

//
// class CObjectInfo
//

class CObjectInfo;
typedef CObjectInfo* LPOBJECTINFO;
typedef CObjectInfo const* LPCOBJECTINFO;
typedef HRESULT (*LPFNCREATEOBJINSTANCE)(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

class CObjectInfo : public CClassFactory                               
{                                                                      
public:                                                                
    CLSID const* pclsid;
    LPFNCREATEOBJINSTANCE pfnCreateInstance;

    // for OCs and automation objects:
    IID const* piid;
    IID const* piidEvents;
    long lVersion;
    DWORD dwOleMiscFlags;
    DWORD dwClassFactFlags;

    CObjectInfo(CLSID const* pclsidin, LPFNCREATEOBJINSTANCE pfnCreatein,  IID const* piidIn, IID const* piidEventsIn, long lVersionIn,  DWORD dwOleMiscFlagsIn,  DWORD dwClassFactFlagsIn);

};

const IClassFactoryVtbl c_CFVtbl = {
    CClassFactory::QueryInterface,  CClassFactory::AddRef, CClassFactory::Release,
    CClassFactory::CreateInstance,
    CClassFactory::LockServer
};

//
// CLASS FACTORY TABLE STUFF
//

typedef struct tagOBJECTINFO
{
    void *cf;
    CLSID const* pclsid;
    LPFNCREATEOBJINSTANCE pfnCreateInstance;

    // for OCs and automation objects:
    IID const* piid;
    IID const* piidEvents;
    long lVersion;
    DWORD dwOleMiscFlags;
    DWORD dwClassFactFlags;
} OBJECTINFO;


#ifdef UNIX


#define CF_TABLE_BEGIN(cfTable) const CObjectInfo cfTable[] = { 
#define CF_TABLE_ENTRY         CObjectInfo
#define CF_TABLE_ENTRY_NOFLAGS CObjectInfo
#define CF_TABLE_ENTRY_ALL     CObjectInfo
#define CF_TABLE_END(cfTable)  \
    CF_TABLE_ENTRY(NULL, NULL, COCREATEONLY) };
#define GET_ICLASSFACTORY(ptr) ((IClassFactory *)ptr)

#else // UNIX

#define CF_TABLE_BEGIN(cfTable) const OBJECTINFO cfTable##_tble[] = { 
#define CF_TABLE_ENTRY(p1, p2, p3) { (void *)&c_CFVtbl, p1, p2, p3 }
#define CF_TABLE_ENTRY_NOFLAGS(p1, p2, p3, p4) { (void *)&c_CFVtbl, p1, p2, p3, p4 }
#define CF_TABLE_ENTRY_ALL(p1, p2, p3, p4, p5, p6, p7) { (void *)&c_CFVtbl, p1, p2, p3, p4 , p5, p6, p7}
#define CF_TABLE_END(cfTable)                                         \
    CF_TABLE_ENTRY(NULL, NULL, COCREATEONLY) }; \
    const CObjectInfo *cfTable = (const CObjectInfo *)cfTable##_tble;
#define GET_ICLASSFACTORY(ptr) ((IClassFactory *)&ptr->vtable)

#endif // UNIX

#define DECLARE_CF_TABLE(cfTable) extern const CObjectInfo *cfTable;

#endif // __cplusplus


#endif // _STATIC_CLASS_FACTORY_
