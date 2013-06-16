#ifndef _BASETYPS_H
#define _BASETYPS_H

#ifndef __OBJC__
# ifdef __cplusplus
#  define EXTERN_C extern "C"
# else
#  define EXTERN_C extern
# endif  /* __cplusplus */
# ifndef _MSC_VER
#  ifndef __int64
#   define __int64 long long
#  endif
#  ifndef __int32
#   define __int32 long
#  endif
#  ifndef __int16
#   define __int16 int
#  endif
#  ifndef __int8
#   define __int8 char
#  endif
# endif
# ifndef __small
#  define __small char
# endif
# ifndef __hyper
#  define __hyper __int64
# endif
#endif

#define STDMETHODCALLTYPE  __stdcall
#define STDMETHODVCALLTYPE __cdecl
#define STDAPICALLTYPE     __stdcall
#define STDAPIVCALLTYPE    __cdecl
#define STDAPI             EXTERN_C HRESULT STDAPICALLTYPE
#define STDAPI_(t)         EXTERN_C t STDAPICALLTYPE
#define STDMETHODIMP       HRESULT STDMETHODCALLTYPE
#define STDMETHODIMP_(t)   t STDMETHODCALLTYPE
#define STDAPIV            EXTERN_C HRESULT STDAPIVCALLTYPE
#define STDAPIV_(t)        EXTERN_C t STDAPIVCALLTYPE
#define STDMETHODIMPV      HRESULT STDMETHODVCALLTYPE
#define STDMETHODIMPV_(t)  t STDMETHODVCALLTYPE

#if defined(__cplusplus) && !defined(CINTERFACE)
# define interface struct
# define STDMETHOD(m) virtual HRESULT STDMETHODCALLTYPE m
# define STDMETHOD_(t,m) virtual t STDMETHODCALLTYPE m
# define PURE =0
# define THIS_
# define THIS void
# define DECLARE_INTERFACE(i)    interface i
# define DECLARE_INTERFACE_(i,b) interface i : public b
#else
# define interface struct
# define STDMETHOD(m) HRESULT (STDMETHODCALLTYPE *m)
# define STDMETHOD_(t,m) t (STDMETHODCALLTYPE *m)
# define PURE
# define THIS_ INTERFACE *,
# define THIS INTERFACE *
# ifdef CONST_VTABLE
#  define DECLARE_INTERFACE(i) \
     typedef interface i { const struct i##Vtbl *lpVtbl; } i; \
     typedef struct i##Vtbl i##Vtbl; \
     struct i##Vtbl
# else
#  define DECLARE_INTERFACE(i) \
     typedef interface i { struct i##Vtbl *lpVtbl; } i; \
     typedef struct i##Vtbl i##Vtbl; \
     struct i##Vtbl
# endif
# define DECLARE_INTERFACE_(i,b) DECLARE_INTERFACE(i)
#endif

#include <guiddef.h>

#ifndef _ERROR_STATUS_T_DEFINED
#define _ERROR_STATUS_T_DEFINED
	typedef unsigned long error_status_t;
#endif

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#endif /* _BASETYPS_H_ */
