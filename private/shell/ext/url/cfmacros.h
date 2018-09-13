#ifndef _STATIC_CLASS_FACTORY_
#define _STATIC_CLASS_FACTORY_

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

#define STDMETHODX(fn)      HRESULT __stdcall fn
#define STDMETHODX_(ret,fn) ret __stdcall fn

#define DECLARE_CLASS_FACTORY(cf)                                \
   class cf                                                      \
   {                                                             \
     public:                                                     \
       void *vtable;                                             \
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
      // *** IUnknown methods ***
      HRESULT (STDMETHODCALLTYPE CClassFactory::* QueryInterface) (
                                REFIID riid,
                                LPVOID FAR* ppvObj) ;
      ULONG (STDMETHODCALLTYPE CClassFactory::*AddRef) () ;
      ULONG (STDMETHODCALLTYPE CClassFactory::*Release) () ;

      // *** IClassFactory methods ***
      HRESULT (STDMETHODCALLTYPE CClassFactory::*CreateInstance) (
                                LPUNKNOWN pUnkOuter,
                                REFIID riid,
                                LPVOID FAR* ppvObject) ;
      HRESULT (STDMETHODCALLTYPE CClassFactory::*LockServer)(BOOL);
};

typedef struct IClassFactoryVtbl IClassFactoryVtbl;

//
// class CObjectInfo
//
typedef HRESULT (*LPFNCREATEINSTANCE)(IUnknown *punkOuter, REFIID riid, void **ppvOut);

class CObjectInfo : public CClassFactory                               
{                                                                      
public:                                                                
    CObjectInfo(CLSID const* pclsidin, LPFNCREATEINSTANCE pfnCreatein) 
    { pclsid = pclsidin; pfnCreate = pfnCreatein; }                    
    CLSID const* pclsid;                                               
    LPFNCREATEINSTANCE pfnCreate;                                      
};

const IClassFactoryVtbl c_CFVtbl = {
    CClassFactory::QueryInterface, 
    CClassFactory::AddRef,
    CClassFactory::Release,
    CClassFactory::CreateInstance,
    CClassFactory::LockServer
};

//
// CLASS FACTORY TABLE STUFF
//

typedef struct {
    const IClassFactoryVtbl *cf;
    const CLSID *rclsid;
    HRESULT (*pfnCreate)(IUnknown *, REFIID, void **);
} OBJ_ENTRY;


#ifdef UNIX

#define CF_TABLE_BEGIN(cfTable) const CObjectInfo cfTable[] = { 
#define CF_TABLE_ENTRY(pclsid, pfnCreate)  CObjectInfo( pclsid, pfnCreate),
#define CF_TABLE_END(cfTable)  \
    CF_TABLE_ENTRY(NULL, NULL) };

#define GET_ICLASSFACTORY(ptr) SAFECAST( ptr, IClassFactory *)

#else

#define CF_TABLE_BEGIN(cfTable) const OBJ_ENTRY cfTable##_tble[] = { 
#define CF_TABLE_ENTRY(pClsid, pfnCreate )   { &c_CFVtbl, pClsid, pfnCreate },
#define CF_TABLE_END(cfTable)  \
    CF_TABLE_ENTRY(NULL, NULL) }; \
    const CObjectInfo *cfTable = (CObjectInfo *)cfTable##_tble;

#define GET_ICLASSFACTORY(ptr) (&(ptr->vtable))

#endif


#endif // __cplusplus


#endif // _STATIC_CLASS_FACTORY_
