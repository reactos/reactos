/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu Apr 03 11:59:43 1997
 */
/* Compiler settings for ADMover.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ADMover_h__
#define __ADMover_h__

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __ADMOVERLib_LIBRARY_DEFINED__
#define __ADMOVERLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: ADMOVERLib
 * at Thu Apr 03 11:59:43 1997
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 

#ifdef __cplusplus
class DECLSPEC_UUID("72267F6A-A6F9-11D0-BC94-00C04FB67863")
DeskMovr;
#endif


#endif /* __ADMOVERLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif

#ifndef __ADMOVR2__
#define __ADMOVR2__

#include "mshtml.h"
#include "mshtmdid.h"

//=--------------------------------------------------------------------------=
// Useful macros
//=--------------------------------------------------------------------------=
//
// handy error macros, randing from cleaning up, to returning to clearing
// rich error information as well.
//
#ifdef __cplusplus

#define RETURN_ON_FAILURE(hr) if (FAILED(hr)) return hr
#define RETURN_ON_NULLALLOC(ptr) if (!(ptr)) return E_OUTOFMEMORY
#define CLEANUP_ON_FAILURE(hr) if (FAILED(hr)) goto CleanUp

// Reference counting help.
//
#define RELEASE_OBJECT(ptr)    ATOMICRELEASE(ptr)
#define QUICK_RELEASE(ptr)     ATOMICRELEASE(ptr)
#define ADDREF_OBJECT(ptr)     if (ptr) (ptr)->AddRef()

#define NEW_HIT_TEST

    
    interface DECLSPEC_UUID("72267F69-A6F9-11D0-BC94-00C04FB67863")
    IDeskMovr : public IUnknown
    {
    public:        
        virtual HRESULT STDMETHODCALLTYPE Duck( 
            BOOL fDuck) = 0;
 
       virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Engaged( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
    };


    interface DECLSPEC_UUID("72267F6C-A6F9-11D0-BC94-00C04FB67863")
    IDeskSizr : public IUnknown
    {
    public:
         
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Target( 
            /* [in] */ LPDISPATCH newVal) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Engaged( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Duck( 
            BOOL fDuck) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TrackTarget( void) = 0;
        
    };

#endif

#define OLEMISMOVR (OLEMISC_ALWAYSRUN|OLEMISC_NOUIACTIVATE|OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_RECOMPOSEONRESIZE|OLEMISC_CANTLINKINSIDE|OLEMISC_INSIDEOUT)
EXTERN_C void PersistTargetPosition( IHTMLElement *pielem,
                            int left,
                            int top,
                            int width,
                            int height,
                            int zIndex,
                            BOOL fSaveRestore,
                            DWORD dwNewState);

EXTERN_C BOOL WINAPI DeskMovr_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/);

#endif // __ADMOVR2__
