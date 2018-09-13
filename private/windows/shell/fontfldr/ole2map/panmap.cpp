/***************************************************************************
 * PANMAP.CPP - Implementaion for ElseWare PANOSE(tm) 1.0 Font Mapper.
 *             OLE 2.0 Implementation
 *
 *
 * Copyright (C) 1991-94 ElseWare Corporation.  All rights reserved.
 ***************************************************************************/
#define _INC_OLE
#define CONST_VTABLE
#include <windows.h>
#include <ole2.h>

// #pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <cguid.h>
#include "elsepan.h"
#include "panmap.h"
#undef INITGUID
// #pragma data_seg()


ULONG g_ulRefThisDll = 0;
ULONG g_ulLock = 0;         //
ULONG g_ulObjs = 0;         // Number of objects

typedef void (FAR PASCAL *LPFNDESTROYED)(void);

////////////////////////////////////////////////////////////////////////////
//
//  Overloaded allocation operators.
//
//  Needed by NT since we don't yet link to real C++ runtimes
//
////////////////////////////////////////////////////////////////////////////

static inline void * __cdecl operator new(
    size_t size)
{
    return ((void *)LocalAlloc(LPTR, size));
}

static inline void __cdecl operator delete(
    void *ptr)
{
    LocalFree(ptr);
}

extern "C" inline __cdecl _purecall(void)
{
    return (0);
}




/**********************************************************************
 */
class CPanoseMapper : public IPANOSEMapper {
public:
   CPanoseMapper(LPUNKNOWN pUnk);
   ~CPanoseMapper(void);


   BOOL  bInit(void);

   /* IUnknown
    */
   STDMETHODIMP         QueryInterface ( REFIID riid, LPVOID FAR* ppvObj);
   STDMETHODIMP_(ULONG) AddRef( void );
   STDMETHODIMP_(ULONG) Release( void ) ;

   /* The PANOSE Mapper interface.
    */
   STDMETHODIMP_(USHORT) unPANMatchFonts(
         LPBYTE lpPanWant,
         ULONG ulSizeWant, EW_LPBYTE lpPanThis, ULONG ulSizeThis,
         BYTE jMapToFamily);


   STDMETHODIMP_(EW_VOID) vPANMakeDummy(
         LPBYTE lpPanThis, USHORT unSize ) ;

   STDMETHODIMP_(SHORT) nPANGetMapDefault(
         LPBYTE lpPanDef,
         USHORT unSizePanDef ) ;

   STDMETHODIMP_(SHORT) nPANSetMapDefault(
         LPBYTE lpPanDef,
         USHORT unSizePanDef ) ;

   STDMETHODIMP_(BOOL) bPANEnableMapDefault (
         BOOL bEnable )  ;

   STDMETHODIMP_(BOOL) bPANIsDefaultEnabled(  )  ;

   STDMETHODIMP_(USHORT) unPANPickFonts (
         USHORT FAR *lpIndsBest,
         USHORT FAR *lpMatchValues, LPBYTE lpPanWant,
         USHORT unNumInds, LPBYTE lpPanFirst, USHORT unNumAvail,
         SHORT nRecSize, BYTE jMapToFamily )  ;

   STDMETHODIMP_(USHORT) unPANGetMapThreshold(  ) ;

   STDMETHODIMP_(BOOL) bPANSetMapThreshold (
         USHORT unThreshold ) ;

   STDMETHODIMP_(BOOL) bPANIsThresholdRelaxed(  ) ;

   STDMETHODIMP_(VOID) vPANRelaxThreshold(  ) ;

   STDMETHODIMP_(BOOL) bPANRestoreThreshold(  ) ;

   STDMETHODIMP_(BOOL) bPANGetMapWeights (
         BYTE jFamilyA,
         BYTE jFamilyB, LPBYTE lpjWts, LPBOOL lpbIsCustom ) ;

   STDMETHODIMP_(BOOL) bPANSetMapWeights (
         BYTE jFamilyA,
         BYTE jFamilyB, LPBYTE lpjWts ) ;

   STDMETHODIMP_(BOOL) bPANClearMapWeights (
         BYTE jFamilyA,
         BYTE jFamilyB ) ;


private:
   LPUNKNOWN      m_pUnkOuter;   //Controlling unknown
   EW_MAPSTATE    m_MapState;    // Used for Core Mapper.

};

/**********************************************************************
 */
class CWrapper : public IUnknown {
public:
   CWrapper(LPUNKNOWN pUnk, LPFNDESTROYED pfn);
   ~CWrapper(void);

   BOOL bInit(void);

   /* IUnknown
    */
   STDMETHODIMP         QueryInterface ( REFIID riid, LPVOID FAR* ppvObj);
   STDMETHODIMP_(ULONG) AddRef( void );
   STDMETHODIMP_(ULONG) Release( void ) ;

private:
   ULONG          m_ulRef;       //Object reference count
   LPUNKNOWN      m_pUnkOuter;   //Controlling unknown
   LPFNDESTROYED  m_pfnDestroy;  // Function to call when destroyed.

   CPanoseMapper * m_pPM;        // the actual PAN mapper.
};

/**********************************************************************
 */
class CImpIClassFactory : public IClassFactory
{
public:
   CImpIClassFactory()
      { m_ulRef = 0; g_ulRefThisDll++;}
   ~CImpIClassFactory() { g_ulRefThisDll--; }

   // *** IUnknown methods ***
   STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   // *** IClassFactory methods ***
   STDMETHODIMP CreateInstance(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              LPVOID FAR* ppvObject);
   STDMETHODIMP LockServer(BOOL fLock);

private:
   int m_ulRef;
};


// ******************************************************************
// LibMain
// extern "C" BOOL APIENTRY DllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
STDAPI_(BOOL) DllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
{
   switch(dwReason)
   {
      case DLL_PROCESS_ATTACH:
         break;

      case DLL_PROCESS_DETACH:
         break;

      case DLL_THREAD_ATTACH:
         break;

      case DLL_THREAD_DETACH:
         break;

      default:
         break;

   } // switch

   return(TRUE);
}

// ******************************************************************
// DllCanUnloadNow
STDAPI DllCanUnloadNow()
{
   HRESULT retval;

   retval = ResultFromScode((g_ulRefThisDll == 0) &&( g_ulLock == 0 )
            &&( g_ulObjs = 0 )
            ? S_OK : S_FALSE);
   return(retval);
}

//*******************************************************************
// Object destruction notification proc.
void FAR PASCAL vObjectDestroyed(void)
{
   g_ulObjs--;
}

// ******************************************************************
// DllGetClassObject
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid,
        LPVOID FAR* ppvObj)
{
   if(!(rclsid == CLSID_PANOSEMapper))
      return(ResultFromScode(E_FAIL));

   if(!(riid == IID_IUnknown) && !(riid == IID_IClassFactory))
      return(ResultFromScode(E_NOINTERFACE));

   *ppvObj = (LPVOID) new CImpIClassFactory;

   if(!*ppvObj)
      return(ResultFromScode(E_OUTOFMEMORY));


  ( (LPUNKNOWN )*ppvObj)->AddRef();

   return NOERROR;
}


// ***********************************************************************
// ***********************************************************************
// CImpIClassFactory member functions

// *** IUnknown methods ***
STDMETHODIMP CImpIClassFactory::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
   *ppvObj = NULL;

   // Any interface on this object is the object pointer
   if((riid == IID_IUnknown) ||( riid == IID_IClassFactory ))
      *ppvObj =( LPVOID ) this;

   if(*ppvObj) {
     ( (LPUNKNOWN )*ppvObj)->AddRef();
      return NOERROR;
   }

   return(ResultFromScode(E_NOINTERFACE));
}


STDMETHODIMP_(ULONG) CImpIClassFactory::AddRef(void)
{
   return(++m_ulRef);
}


STDMETHODIMP_(ULONG) CImpIClassFactory::Release(void)
{
   ULONG retval;
   retval = --m_ulRef;
   if(!retval) delete this;
   return(retval);
}


// *** IClassFactory methods ***
STDMETHODIMP CImpIClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
        REFIID riid, LPVOID FAR* ppvObj)
{
   HRESULT retval;
   CWrapper * pPM;

   *ppvObj=NULL;
   retval = ResultFromScode(E_OUTOFMEMORY);

   // Verify that a controlling unknown asks for IUnknown
   //
   if( NULL != pUnkOuter && !(riid == IID_IUnknown))
      return( E_NOINTERFACE );

   pPM = new CWrapper(pUnkOuter, vObjectDestroyed);

   // Increment the object count if we were successful.
   //
   if(!pPM)
      return(retval);
   g_ulObjs++;

   // Initialize the Panose Mapper. It will destroy itself if initialization
   // fails.
   //
   if(pPM->bInit())
      retval = pPM->QueryInterface(riid, ppvObj);

   return(retval);
}


STDMETHODIMP CImpIClassFactory::LockServer(BOOL fLock)
{
   if(fLock)
      g_ulLock++;
   else
      g_ulLock--;
   return(NOERROR);
}

// ***********************************************************************
// ***********************************************************************
// CWrapper member functions
CWrapper::CWrapper( LPUNKNOWN pUnk, LPFNDESTROYED pfn )
   :  m_pUnkOuter(pUnk),
      m_pfnDestroy(pfn),
      m_ulRef(0) ,
      m_pPM(0)
{

}

CWrapper::~CWrapper(void)
{
   // Clean up the mapper.
   //
   if( m_pPM )
      delete m_pPM;

   // Notify of destruction.
   //
   if( NULL != m_pfnDestroy )
     ( *m_pfnDestroy )();
}


BOOL  CWrapper::bInit(void)
{
   BOOL bRet = FALSE;
   LPUNKNOWN   pUnk = this;

   if( m_pUnkOuter )
      pUnk = m_pUnkOuter;

   m_pPM = new CPanoseMapper(pUnk);

   if(m_pPM) {
      bRet = m_pPM->bInit();
      if(  !bRet  )
         m_pPM = NULL;
   }

   if( !bRet )
      delete this;

   return bRet;
}

STDMETHODIMP CWrapper::QueryInterface(  REFIID riid, LPVOID FAR* ppvObj )
{
   *ppvObj=NULL;

   /*
    * The only calls for IUnknown are either in a nonaggregated
    * case or when created in an aggregation, so in either case
    * always return our IUnknown for IID_IUnknown.
    */
   if( riid == IID_IUnknown )
      *ppvObj=(LPVOID)this;

   /*
    * Return
    */
   if( riid == IID_IPANOSEMapper )
      *ppvObj=(LPVOID)m_pPM;

   //AddRef any interface we'll return.
   if( NULL!=*ppvObj )  {
     ( (LPUNKNOWN )*ppvObj)->AddRef();
      return NOERROR;
   }

   return ResultFromScode(E_NOINTERFACE);

}

STDMETHODIMP_(ULONG) CWrapper::AddRef( void )
{
   m_ulRef++;

    return m_ulRef;
}

STDMETHODIMP_(ULONG) CWrapper::Release( void )
{
   ULONG ulRet = --m_ulRef;
   if(( ULONG )0 == ulRet )
      delete this;

   return ulRet;
}

// ***********************************************************************
// ***********************************************************************
// CPanoseMapper member functions

CPanoseMapper::CPanoseMapper( LPUNKNOWN pUnk )
   :  m_pUnkOuter(pUnk)
{

}

CPanoseMapper::~CPanoseMapper(void)
{
   // Clean up the mapper.
   //
   ::bPANMapClose( &m_MapState );

}


BOOL  CPanoseMapper::bInit(void)
{
   // Initialize the MapState struct
   //
   BOOL bOK =( ::nPANMapInit(&m_MapState, sizeof(m_MapState )) > 0);

   // If init fails, then this is not valid. Delete it.
   //
   if( ! bOK )
      delete this;

   return bOK;
}


/* IUnknown
   */
STDMETHODIMP CPanoseMapper::QueryInterface(  REFIID riid, LPVOID FAR* ppvObj )
{
   return m_pUnkOuter->QueryInterface(riid, ppvObj);

}

STDMETHODIMP_(ULONG) CPanoseMapper::AddRef( void )
{
   return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CPanoseMapper::Release( void )
{
   return m_pUnkOuter->Release();
}

/* The PANOSE Mapper interface.
   */
STDMETHODIMP_(USHORT) CPanoseMapper::unPANMatchFonts(
      LPBYTE lpPanWant,
      ULONG ulSizeWant, LPBYTE lpPanThis, ULONG ulSizeThis,
      BYTE jMapToFamily)
{
   return ::unPANMatchFonts(
         &m_MapState,
         lpPanWant,
         ulSizeWant,
         lpPanThis,
         ulSizeThis,
         jMapToFamily);
}


STDMETHODIMP_(VOID) CPanoseMapper::vPANMakeDummy(
      LPBYTE lpPanThis, USHORT unSize )
{
   ::vPANMakeDummy( lpPanThis, unSize );
}

STDMETHODIMP_(SHORT) CPanoseMapper::nPANGetMapDefault(
      LPBYTE lpPanDef,
      USHORT unSizePanDef )
{
   return ::nPANGetMapDefault( &m_MapState, lpPanDef, unSizePanDef );
}


STDMETHODIMP_(SHORT) CPanoseMapper::nPANSetMapDefault(
      LPBYTE lpPanDef,
      USHORT unSizePanDef )
{
   return ::nPANSetMapDefault( &m_MapState, lpPanDef, unSizePanDef );
}


STDMETHODIMP_(BOOL) CPanoseMapper::bPANEnableMapDefault (
      BOOL bEnable )
{
   return ::bPANEnableMapDefault( &m_MapState, (EW_BOOL)bEnable  );
}


STDMETHODIMP_(BOOL) CPanoseMapper::bPANIsDefaultEnabled(  )
{
   return ::bPANIsDefaultEnabled(  &m_MapState  );
}


STDMETHODIMP_(USHORT) CPanoseMapper::unPANPickFonts (
      USHORT FAR * lpIndsBest,
      USHORT FAR * lpMatchValues, LPBYTE lpPanWant,
      USHORT unNumInds, LPBYTE lpPanFirst, USHORT unNumAvail,
      SHORT nRecSize, BYTE jMapToFamily )
{
   return ::unPANPickFonts (
         &m_MapState,
         lpIndsBest,
         lpMatchValues,
         lpPanWant,
         unNumInds,
         lpPanFirst,
         unNumAvail,
         nRecSize,
         jMapToFamily );
}


STDMETHODIMP_(USHORT) CPanoseMapper::unPANGetMapThreshold(  )
{
   return ::unPANGetMapThreshold( &m_MapState  );
}


STDMETHODIMP_(BOOL) CPanoseMapper::bPANSetMapThreshold (
      USHORT unThreshold )
{
   return ::bPANSetMapThreshold( &m_MapState, unThreshold  );
}


STDMETHODIMP_(BOOL) CPanoseMapper::bPANIsThresholdRelaxed(  )
{
   return ::bPANIsThresholdRelaxed( &m_MapState  );
}


STDMETHODIMP_(VOID) CPanoseMapper::vPANRelaxThreshold(  )
{
   ::vPANRelaxThreshold( &m_MapState  );
}


STDMETHODIMP_(BOOL) CPanoseMapper::bPANRestoreThreshold(  )
{
   return ::bPANRestoreThreshold( &m_MapState  );
}


STDMETHODIMP_(BOOL) CPanoseMapper::bPANGetMapWeights (
      BYTE jFamilyA,
      BYTE jFamilyB, LPBYTE lpjWts, LPBOOL lpbIsCustom )
{
   *lpbIsCustom =( BOOL )0;
   return ::bPANGetMapWeights (&m_MapState, jFamilyA,
               jFamilyB, lpjWts,( EW_LPBOOL )lpbIsCustom );
}


STDMETHODIMP_(BOOL) CPanoseMapper::bPANSetMapWeights (
      BYTE jFamilyA,
      BYTE jFamilyB, LPBYTE lpjWts )
{
   return ::bPANSetMapWeights (
      &m_MapState, jFamilyA,
      jFamilyB, lpjWts );
}


STDMETHODIMP_(BOOL) CPanoseMapper::bPANClearMapWeights (
      BYTE jFamilyA,
      BYTE jFamilyB )
{
   return ::bPANClearMapWeights( &m_MapState, jFamilyA, jFamilyB  );
}
