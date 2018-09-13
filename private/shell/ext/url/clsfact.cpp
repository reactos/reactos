/*
 * clsfact.cpp - IClassFactory implementation.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include "clsfact.h"
#include "ftps.hpp"
#include "inetcpl.h"
#include "inetps.hpp"


/* Types
 ********/

// callback function used by ClassFactory::ClassFactory()

typedef PIUnknown (*NEWOBJECTPROC)(OBJECTDESTROYEDPROC);
DECLARE_STANDARD_TYPES(NEWOBJECTPROC);

// description of class supported by DllGetClassObject()

typedef struct classconstructor
{
   PCCLSID pcclsid;

   NEWOBJECTPROC NewObject;
}
CLASSCONSTRUCTOR;
DECLARE_STANDARD_TYPES(CLASSCONSTRUCTOR);


/* Classes
 **********/

// object class factory

class ClassFactory : public RefCount,
                     public IClassFactory
{
private:
   NEWOBJECTPROC m_NewObject;

public:
   ClassFactory(NEWOBJECTPROC NewObject, OBJECTDESTROYEDPROC ObjectDestroyed);
   ~ClassFactory(void);

   // IClassFactory methods

   HRESULT STDMETHODCALLTYPE CreateInstance(PIUnknown piunkOuter, REFIID riid, PVOID *ppvObject);
   HRESULT STDMETHODCALLTYPE LockServer(BOOL bLock);

   // IUnknown methods

   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);
   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);

   // friends

#ifdef DEBUG

   friend BOOL IsValidPCClassFactory(const ClassFactory *pcurlcf);

#endif

};
DECLARE_STANDARD_TYPES(ClassFactory);


/* Module Prototypes
 ********************/

PRIVATE_CODE PIUnknown NewInternetShortcut(OBJECTDESTROYEDPROC ObjectDestroyed);
PRIVATE_CODE PIUnknown NewMIMEHook(OBJECTDESTROYEDPROC ObjectDestroyed);
PRIVATE_CODE PIUnknown NewInternet(OBJECTDESTROYEDPROC ObjectDestroyed);


/* Module Constants
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

PRIVATE_DATA CCLASSCONSTRUCTOR s_cclscnstr[] =
{
   { &CLSID_InternetShortcut,             &NewInternetShortcut },
   { &CLSID_MIMEFileTypesPropSheetHook,   &NewMIMEHook },
   { &CLSID_Internet,                     &NewInternet },
};

#pragma data_seg()


/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_PER_INSTANCE)

// DLL reference count == number of class factories +
//                        number of URLs +
//                        LockServer() count

PRIVATE_DATA ULONG s_ulcDLLRef   = 0;

#pragma data_seg()


/***************************** Private Functions *****************************/


PRIVATE_CODE HRESULT GetClassConstructor(REFCLSID rclsid,
                                         PNEWOBJECTPROC pNewObject)
{
   HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
   UINT u;

   ASSERT(IsValidREFCLSID(rclsid));
   ASSERT(IS_VALID_WRITE_PTR(pNewObject, NEWOBJECTPROC));

   *pNewObject = NULL;

   for (u = 0; u < ARRAY_ELEMENTS(s_cclscnstr); u++)
   {
      if (rclsid == *(s_cclscnstr[u].pcclsid))
      {
         *pNewObject = s_cclscnstr[u].NewObject;
         hr = S_OK;
      }
   }

   ASSERT((hr == S_OK &&
           IS_VALID_CODE_PTR(*pNewObject, NEWOBJECTPROC)) ||
          (hr == CLASS_E_CLASSNOTAVAILABLE &&
           ! *pNewObject));

   return(hr);
}


PRIVATE_CODE void STDMETHODCALLTYPE DLLObjectDestroyed(void)
{
   TRACE_OUT(("DLLObjectDestroyed(): Object destroyed."));

   DLLRelease();
}


PRIVATE_CODE PIUnknown NewInternetShortcut(OBJECTDESTROYEDPROC ObjectDestroyed)
{
   ASSERT(! ObjectDestroyed ||
          IS_VALID_CODE_PTR(ObjectDestroyed, OBJECTDESTROYEDPROC));

   TRACE_OUT(("NewInternetShortcut(): Creating a new InternetShortcut."));

   return((PIUnknown)(PIUniformResourceLocator)new(InternetShortcut(ObjectDestroyed)));
}


PRIVATE_CODE PIUnknown NewMIMEHook(OBJECTDESTROYEDPROC ObjectDestroyed)
{
   ASSERT(! ObjectDestroyed ||
          IS_VALID_CODE_PTR(ObjectDestroyed, OBJECTDESTROYEDPROC));

   TRACE_OUT(("NewMIMEHook(): Creating a new MIMEHook."));

   return((PIUnknown)(PIShellPropSheetExt)new(MIMEHook(ObjectDestroyed)));
}


PRIVATE_CODE PIUnknown NewInternet(OBJECTDESTROYEDPROC ObjectDestroyed)
{
   ASSERT(! ObjectDestroyed ||
          IS_VALID_CODE_PTR(ObjectDestroyed, OBJECTDESTROYEDPROC));

   TRACE_OUT(("NewInternet(): Creating a new Internet."));

   return((PIUnknown)(PIShellPropSheetExt)new(Internet(ObjectDestroyed)));
}


#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCClassFactory(PCClassFactory pccf)
{
   return(IS_VALID_READ_PTR(pccf, CClassFactory) &&
          IS_VALID_CODE_PTR(pccf->m_NewObject, NEWOBJECTPROC) &&
          IS_VALID_STRUCT_PTR((PCRefCount)pccf, CRefCount) &&
          IS_VALID_INTERFACE_PTR((PCIClassFactory)pccf, IClassFactory));
}

#endif


/****************************** Public Functions *****************************/


PUBLIC_CODE ULONG DLLAddRef(void)
{
   ULONG ulcRef;

   ASSERT(s_ulcDLLRef < ULONG_MAX);

   ulcRef = ++s_ulcDLLRef;

   TRACE_OUT(("DLLAddRef(): DLL reference count is now %lu.",
              ulcRef));

   return(ulcRef);
}


PUBLIC_CODE ULONG DLLRelease(void)
{
   ULONG ulcRef;

   if (EVAL(s_ulcDLLRef > 0))
      s_ulcDLLRef--;

   ulcRef = s_ulcDLLRef;

   TRACE_OUT(("DLLRelease(): DLL reference count is now %lu.",
              ulcRef));

   return(ulcRef);
}


PUBLIC_CODE PULONG GetDLLRefCountPtr(void)
{
   return(&s_ulcDLLRef);
}


/********************************** Methods **********************************/


ClassFactory::ClassFactory(NEWOBJECTPROC NewObject,
                           OBJECTDESTROYEDPROC ObjectDestroyed) :
   RefCount(ObjectDestroyed)
{
   DebugEntry(ClassFactory::ClassFactory);

   // Don't validate this until after construction.

   ASSERT(IS_VALID_CODE_PTR(NewObject, NEWOBJECTPROC));

   m_NewObject = NewObject;

   ASSERT(IS_VALID_STRUCT_PTR(this, CClassFactory));

   DebugExitVOID(ClassFactory::ClassFactory);

   return;
}


ClassFactory::~ClassFactory(void)
{
   DebugEntry(ClassFactory::~ClassFactory);

   ASSERT(IS_VALID_STRUCT_PTR(this, CClassFactory));

   m_NewObject = NULL;

   // Don't validate this after destruction.

   DebugExitVOID(ClassFactory::~ClassFactory);

   return;
}


ULONG STDMETHODCALLTYPE ClassFactory::AddRef(void)
{
   ULONG ulcRef;

   DebugEntry(ClassFactory::AddRef);

   ASSERT(IS_VALID_STRUCT_PTR(this, CClassFactory));

   ulcRef = RefCount::AddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CClassFactory));

   DebugExitULONG(ClassFactory::AddRef, ulcRef);

   return(ulcRef);
}


ULONG STDMETHODCALLTYPE ClassFactory::Release(void)
{
   ULONG ulcRef;

   DebugEntry(ClassFactory::Release);

   ASSERT(IS_VALID_STRUCT_PTR(this, CClassFactory));

   ulcRef = RefCount::Release();

   DebugExitULONG(ClassFactory::Release, ulcRef);

   return(ulcRef);
}


HRESULT STDMETHODCALLTYPE ClassFactory::QueryInterface(REFIID riid,
                                                       PVOID *ppvObject)
{
   HRESULT hr = S_OK;

   DebugEntry(ClassFactory::QueryInterface);

   ASSERT(IS_VALID_STRUCT_PTR(this, CClassFactory));
   ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   if (riid == IID_IClassFactory)
   {
      *ppvObject = (PIClassFactory)this;
      ASSERT(IS_VALID_INTERFACE_PTR((PIClassFactory)*ppvObject, IClassFactory));
      TRACE_OUT(("ClassFactory::QueryInterface(): Returning IClassFactory."));
   }
   else if (riid == IID_IUnknown)
   {
      *ppvObject = (PIUnknown)this;
      ASSERT(IS_VALID_INTERFACE_PTR((PIUnknown)*ppvObject, IUnknown));
      TRACE_OUT(("ClassFactory::QueryInterface(): Returning IUnknown."));
   }
   else
   {
      *ppvObject = NULL;
      hr = E_NOINTERFACE;
      TRACE_OUT(("ClassFactory::QueryInterface(): Called on unknown interface."));
   }

   if (hr == S_OK)
      AddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CClassFactory));
   ASSERT(FAILED(hr) ||
          IS_VALID_INTERFACE_PTR(*ppvObject, INTERFACE));

   DebugExitHRESULT(ClassFactory::QueryInterface, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE ClassFactory::CreateInstance(PIUnknown piunkOuter,
                                                       REFIID riid,
                                                       PVOID *ppvObject)
{
   HRESULT hr;

   DebugEntry(ClassFactory::CreateInstance);

   ASSERT(IS_VALID_STRUCT_PTR(this, CClassFactory));
   ASSERT(! piunkOuter ||
          IS_VALID_INTERFACE_PTR(piunkOuter, IUnknown));
   ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   *ppvObject = NULL;

   if (! piunkOuter)
   {
      PIUnknown piunk;

      piunk = (*m_NewObject)(&DLLObjectDestroyed);

      if (piunk)
      {
         DLLAddRef();

         hr = piunk->QueryInterface(riid, ppvObject);

         // N.b., the Release() method will destroy the object if the
         // QueryInterface() method failed.

         piunk->Release();
      }
      else
         hr = E_OUTOFMEMORY;
   }
   else
   {
      hr = CLASS_E_NOAGGREGATION;
      WARNING_OUT(("ClassFactory::CreateInstance(): Aggregation not supported."));
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CClassFactory));
   ASSERT(FAILED(hr) ||
          IS_VALID_INTERFACE_PTR(*ppvObject, INTERFACE));

   DebugExitHRESULT(ClassFactory::CreateInstance, hr);

   return(hr);
}


HRESULT STDMETHODCALLTYPE ClassFactory::LockServer(BOOL bLock)
{
   HRESULT hr;

   DebugEntry(ClassFactory::LockServer);

   ASSERT(IS_VALID_STRUCT_PTR(this, CClassFactory));

   // bLock may be any value.

   if (bLock)
      DLLAddRef();
   else
      DLLRelease();

   hr = S_OK;

   ASSERT(IS_VALID_STRUCT_PTR(this, CClassFactory));

   DebugExitHRESULT(ClassFactory::LockServer, hr);

   return(hr);
}


/***************************** Exported Functions ****************************/


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PVOID *ppvObject)
{
   HRESULT hr = S_OK;
   NEWOBJECTPROC NewObject;

   DebugEntry(DllGetClassObject);

   ASSERT(IsValidREFCLSID(rclsid));
   ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   *ppvObject = NULL;

   hr = GetClassConstructor(rclsid, &NewObject);

   if (hr == S_OK)
   {
      if (riid == IID_IUnknown ||
          riid == IID_IClassFactory)
      {
         PClassFactory pcf;

         pcf = new(ClassFactory(NewObject, &DLLObjectDestroyed));

         if (pcf)
         {
            if (riid == IID_IClassFactory)
            {
               *ppvObject = (PIClassFactory)pcf;
               ASSERT(IS_VALID_INTERFACE_PTR((PIClassFactory)*ppvObject, IClassFactory));
               TRACE_OUT(("DllGetClassObject(): Returning IClassFactory."));
            }
            else
            {
               ASSERT(riid == IID_IUnknown);
               *ppvObject = (PIUnknown)pcf;
               ASSERT(IS_VALID_INTERFACE_PTR((PIUnknown)*ppvObject, IUnknown));
               TRACE_OUT(("DllGetClassObject(): Returning IUnknown."));
            }

            DLLAddRef();
            hr = S_OK;

            TRACE_OUT(("DllGetClassObject(): Created a new class factory."));
         }
         else
            hr = E_OUTOFMEMORY;
      }
      else
      {
         WARNING_OUT(("DllGetClassObject(): Called on unknown interface."));
         hr = E_NOINTERFACE;
      }
   }
   else
      WARNING_OUT(("DllGetClassObject(): Called on unknown class."));

   ASSERT(FAILED(hr) ||
          IS_VALID_INTERFACE_PTR(*ppvObject, INTERFACE));

   DebugExitHRESULT(DllGetClassObject, hr);

   return(hr);
}


STDAPI DllCanUnloadNow(void)
{
   HRESULT hr;

   DebugEntry(DllCanUnloadNow);

   hr = (s_ulcDLLRef > 0) ? S_FALSE : S_OK;
    
    if (hr == S_OK) 
        hr = InternetCPLCanUnloadNow();

   TRACE_OUT(("DllCanUnloadNow(): DLL reference count is %lu.",
              s_ulcDLLRef));

   DebugExitHRESULT(DllCanUnloadNow, hr);

   return(hr);
}

