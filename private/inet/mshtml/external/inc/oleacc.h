/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0158 */
/* at Wed May 13 11:31:10 1998
 */
/* Compiler settings for oleacc.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data , no_format_optimization
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef UNIX
#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#else
#define MIDL_INTERFACE(x) struct
#endif

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __oleacc_h__
#define __oleacc_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IAccessible_FWD_DEFINED__
#define __IAccessible_FWD_DEFINED__
typedef interface IAccessible IAccessible;
#endif 	/* __IAccessible_FWD_DEFINED__ */


#ifndef __IAccessibleHandler_FWD_DEFINED__
#define __IAccessibleHandler_FWD_DEFINED__
typedef interface IAccessibleHandler IAccessibleHandler;
#endif 	/* __IAccessibleHandler_FWD_DEFINED__ */


#ifndef __IAccessible_FWD_DEFINED__
#define __IAccessible_FWD_DEFINED__
typedef interface IAccessible IAccessible;
#endif 	/* __IAccessible_FWD_DEFINED__ */


#ifndef __IAccessibleHandler_FWD_DEFINED__
#define __IAccessibleHandler_FWD_DEFINED__
typedef interface IAccessibleHandler IAccessibleHandler;
#endif 	/* __IAccessibleHandler_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_oleacc_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// OLEACC.H
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// Typedefs
//=--------------------------------------------------------------------------=

typedef LRESULT (STDAPICALLTYPE *LPFNLRESULTFROMOBJECT)(REFIID riid, WPARAM wParam, LPUNKNOWN punk);
typedef HRESULT (STDAPICALLTYPE *LPFNOBJECTFROMLRESULT)(LRESULT lResult, REFIID riid, WPARAM wParam, void** ppvObject);
typedef HRESULT (STDAPICALLTYPE *LPFNACCESSIBLEOBJECTFROMWINDOW)(HWND hwnd, DWORD dwId, REFIID riid, void** ppvObject);
typedef HRESULT (STDAPICALLTYPE *LPFNACCESSIBLEOBJECTFROMPOINT)(POINT ptScreen, IAccessible** ppacc, VARIANT* pvarChild);
typedef HRESULT (STDAPICALLTYPE *LPFNCREATESTDACCESSIBLEOBJECT)(HWND hwnd, LONG idObject, REFIID riid, void** ppvObject);
typedef HRESULT (STDAPICALLTYPE *LPFNACCESSIBLECHILDREN)(IAccessible* paccContainer, LONG iChildStart,LONG cChildren,VARIANT* rgvarChildren,LONG* pcObtained);

//=--------------------------------------------------------------------------=
// GUIDs
//=--------------------------------------------------------------------------=

DEFINE_GUID(LIBID_Accessibility,	0x1ea4dbf0, 0x3c3b, 0x11cf, 0x81, 0x0c, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(IID_IAccessible,		0x618736e0, 0x3c3d, 0x11cf, 0x81, 0x0c, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(IID_IAccessibleHandler, 0x03022430, 0xABC4, 0x11d0, 0xBD, 0xE2, 0x00, 0xAA, 0x00, 0x1A, 0x19, 0x53);

//=--------------------------------------------------------------------------=
// MSAA API Prototypes
//=--------------------------------------------------------------------------=

STDAPI_(LRESULT) LresultFromObject(REFIID riid, WPARAM wParam, LPUNKNOWN punk);
STDAPI          ObjectFromLresult(LRESULT lResult, REFIID riid, WPARAM wParam, void** ppvObject);
STDAPI          WindowFromAccessibleObject(IAccessible*, HWND* phwnd);
STDAPI          AccessibleObjectFromWindow(HWND hwnd, DWORD dwId, REFIID riid, void **ppvObject);
STDAPI          AccessibleObjectFromEvent(HWND hwnd, DWORD dwId, DWORD dwChildId, IAccessible** ppacc, VARIANT* pvarChild);
STDAPI          AccessibleObjectFromPoint(POINT ptScreen, IAccessible ** ppacc, VARIANT* pvarChild);
STDAPI          CreateStdAccessibleObject(HWND hwnd, LONG idObject, REFIID riid, void** ppvObject);
STDAPI          AccessibleChildren (IAccessible* paccContainer, LONG iChildStart,LONG cChildren, VARIANT* rgvarChildren,LONG* pcObtained);

STDAPI_(UINT)   GetRoleTextA(DWORD lRole, LPSTR lpszRole, UINT cchRoleMax);
STDAPI_(UINT)   GetRoleTextW(DWORD lRole, LPWSTR lpszRole, UINT cchRoleMax);
#ifdef UNICODE
#define GetRoleText     GetRoleTextW
#else
#define GetRoleText     GetRoleTextA
#endif // UNICODE

STDAPI_(UINT)   GetStateTextA(DWORD lStateBit, LPSTR lpszState, UINT cchState);
STDAPI_(UINT)   GetStateTextW(DWORD lStateBit, LPWSTR lpszState, UINT cchState);
#ifdef UNICODE
#define GetStateText    GetStateTextW
#else
#define GetStateText    GetStateTextA
#endif // UNICODE

//=--------------------------------------------------------------------------=
// Interface Definitions
//=--------------------------------------------------------------------------=



extern RPC_IF_HANDLE __MIDL_itf_oleacc_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_oleacc_0000_v0_0_s_ifspec;

#ifndef __IAccessible_INTERFACE_DEFINED__
#define __IAccessible_INTERFACE_DEFINED__

/* interface IAccessible */
/* [unique][dual][hidden][uuid][object] */ 

#define	DISPID_ACC_PARENT	( -5000 )

#define	DISPID_ACC_CHILDCOUNT	( -5001 )

#define	DISPID_ACC_CHILD	( -5002 )

#define	DISPID_ACC_NAME	( -5003 )

#define	DISPID_ACC_VALUE	( -5004 )

#define	DISPID_ACC_DESCRIPTION	( -5005 )

#define	DISPID_ACC_ROLE	( -5006 )

#define	DISPID_ACC_STATE	( -5007 )

#define	DISPID_ACC_HELP	( -5008 )

#define	DISPID_ACC_HELPTOPIC	( -5009 )

#define	DISPID_ACC_KEYBOARDSHORTCUT	( -5010 )

#define	DISPID_ACC_FOCUS	( -5011 )

#define	DISPID_ACC_SELECTION	( -5012 )

#define	DISPID_ACC_DEFAULTACTION	( -5013 )

#define	DISPID_ACC_SELECT	( -5014 )

#define	DISPID_ACC_LOCATION	( -5015 )

#define	DISPID_ACC_NAVIGATE	( -5016 )

#define	DISPID_ACC_HITTEST	( -5017 )

#define	DISPID_ACC_DODEFAULTACTION	( -5018 )

typedef /* [unique] */ IAccessible __RPC_FAR *LPACCESSIBLE;

#define	NAVDIR_MIN	( 0 )

#define	NAVDIR_UP	( 0x1 )

#define	NAVDIR_DOWN	( 0x2 )

#define	NAVDIR_LEFT	( 0x3 )

#define	NAVDIR_RIGHT	( 0x4 )

#define	NAVDIR_NEXT	( 0x5 )

#define	NAVDIR_PREVIOUS	( 0x6 )

#define	NAVDIR_FIRSTCHILD	( 0x7 )

#define	NAVDIR_LASTCHILD	( 0x8 )

#define	NAVDIR_MAX	( 0x9 )

#define	SELFLAG_NONE	( 0 )

#define	SELFLAG_TAKEFOCUS	( 0x1 )

#define	SELFLAG_TAKESELECTION	( 0x2 )

#define	SELFLAG_EXTENDSELECTION	( 0x4 )

#define	SELFLAG_ADDSELECTION	( 0x8 )

#define	SELFLAG_REMOVESELECTION	( 0x10 )

#define	SELFLAG_VALID	( 0x1f )

#define	STATE_SYSTEM_UNAVAILABLE	( 0x1 )

#define	STATE_SYSTEM_SELECTED	( 0x2 )

#define	STATE_SYSTEM_FOCUSED	( 0x4 )

#define	STATE_SYSTEM_PRESSED	( 0x8 )

#define	STATE_SYSTEM_CHECKED	( 0x10 )

#define	STATE_SYSTEM_MIXED	( 0x20 )

#define	STATE_SYSTEM_INDETERMINATE	( STATE_SYSTEM_MIXED )

#define	STATE_SYSTEM_READONLY	( 0x40 )

#define	STATE_SYSTEM_HOTTRACKED	( 0x80 )

#define	STATE_SYSTEM_DEFAULT	( 0x100 )

#define	STATE_SYSTEM_EXPANDED	( 0x200 )

#define	STATE_SYSTEM_COLLAPSED	( 0x400 )

#define	STATE_SYSTEM_BUSY	( 0x800 )

#define	STATE_SYSTEM_FLOATING	( 0x1000 )

#define	STATE_SYSTEM_MARQUEED	( 0x2000 )

#define	STATE_SYSTEM_ANIMATED	( 0x4000 )

#define	STATE_SYSTEM_INVISIBLE	( 0x8000 )

#define	STATE_SYSTEM_OFFSCREEN	( 0x10000 )

#define	STATE_SYSTEM_SIZEABLE	( 0x20000 )

#define	STATE_SYSTEM_MOVEABLE	( 0x40000 )

#define	STATE_SYSTEM_SELFVOICING	( 0x80000 )

#define	STATE_SYSTEM_FOCUSABLE	( 0x100000 )

#define	STATE_SYSTEM_SELECTABLE	( 0x200000 )

#define	STATE_SYSTEM_LINKED	( 0x400000 )

#define	STATE_SYSTEM_TRAVERSED	( 0x800000 )

#define	STATE_SYSTEM_MULTISELECTABLE	( 0x1000000 )

#define	STATE_SYSTEM_EXTSELECTABLE	( 0x2000000 )

#define	STATE_SYSTEM_ALERT_LOW	( 0x4000000 )

#define	STATE_SYSTEM_ALERT_MEDIUM	( 0x8000000 )

#define	STATE_SYSTEM_ALERT_HIGH	( 0x10000000 )

#define	STATE_SYSTEM_PROTECTED	( 0x20000000 )

#define	STATE_SYSTEM_ONLY_REDUNDANT	( 0x40000000 )

#define	STATE_SYSTEM_VALID	( 0x7fffffff )

#define	ROLE_SYSTEM_TITLEBAR	( 0x1 )

#define	ROLE_SYSTEM_MENUBAR	( 0x2 )

#define	ROLE_SYSTEM_SCROLLBAR	( 0x3 )

#define	ROLE_SYSTEM_GRIP	( 0x4 )

#define	ROLE_SYSTEM_SOUND	( 0x5 )

#define	ROLE_SYSTEM_CURSOR	( 0x6 )

#define	ROLE_SYSTEM_CARET	( 0x7 )

#define	ROLE_SYSTEM_ALERT	( 0x8 )

#define	ROLE_SYSTEM_WINDOW	( 0x9 )

#define	ROLE_SYSTEM_CLIENT	( 0xa )

#define	ROLE_SYSTEM_MENUPOPUP	( 0xb )

#define	ROLE_SYSTEM_MENUITEM	( 0xc )

#define	ROLE_SYSTEM_TOOLTIP	( 0xd )

#define	ROLE_SYSTEM_APPLICATION	( 0xe )

#define	ROLE_SYSTEM_DOCUMENT	( 0xf )

#define	ROLE_SYSTEM_PANE	( 0x10 )

#define	ROLE_SYSTEM_CHART	( 0x11 )

#define	ROLE_SYSTEM_DIALOG	( 0x12 )

#define	ROLE_SYSTEM_BORDER	( 0x13 )

#define	ROLE_SYSTEM_GROUPING	( 0x14 )

#define	ROLE_SYSTEM_SEPARATOR	( 0x15 )

#define	ROLE_SYSTEM_TOOLBAR	( 0x16 )

#define	ROLE_SYSTEM_STATUSBAR	( 0x17 )

#define	ROLE_SYSTEM_TABLE	( 0x18 )

#define	ROLE_SYSTEM_COLUMNHEADER	( 0x19 )

#define	ROLE_SYSTEM_ROWHEADER	( 0x1a )

#define	ROLE_SYSTEM_COLUMN	( 0x1b )

#define	ROLE_SYSTEM_ROW	( 0x1c )

#define	ROLE_SYSTEM_CELL	( 0x1d )

#define	ROLE_SYSTEM_LINK	( 0x1e )

#define	ROLE_SYSTEM_HELPBALLOON	( 0x1f )

#define	ROLE_SYSTEM_CHARACTER	( 0x20 )

#define	ROLE_SYSTEM_LIST	( 0x21 )

#define	ROLE_SYSTEM_LISTITEM	( 0x22 )

#define	ROLE_SYSTEM_OUTLINE	( 0x23 )

#define	ROLE_SYSTEM_OUTLINEITEM	( 0x24 )

#define	ROLE_SYSTEM_PAGETAB	( 0x25 )

#define	ROLE_SYSTEM_PROPERTYPAGE	( 0x26 )

#define	ROLE_SYSTEM_INDICATOR	( 0x27 )

#define	ROLE_SYSTEM_GRAPHIC	( 0x28 )

#define	ROLE_SYSTEM_STATICTEXT	( 0x29 )

#define	ROLE_SYSTEM_TEXT	( 0x2a )

#define	ROLE_SYSTEM_PUSHBUTTON	( 0x2b )

#define	ROLE_SYSTEM_CHECKBUTTON	( 0x2c )

#define	ROLE_SYSTEM_RADIOBUTTON	( 0x2d )

#define	ROLE_SYSTEM_COMBOBOX	( 0x2e )

#define	ROLE_SYSTEM_DROPLIST	( 0x2f )

#define	ROLE_SYSTEM_PROGRESSBAR	( 0x30 )

#define	ROLE_SYSTEM_DIAL	( 0x31 )

#define	ROLE_SYSTEM_HOTKEYFIELD	( 0x32 )

#define	ROLE_SYSTEM_SLIDER	( 0x33 )

#define	ROLE_SYSTEM_SPINBUTTON	( 0x34 )

#define	ROLE_SYSTEM_DIAGRAM	( 0x35 )

#define	ROLE_SYSTEM_ANIMATION	( 0x36 )

#define	ROLE_SYSTEM_EQUATION	( 0x37 )

#define	ROLE_SYSTEM_BUTTONDROPDOWN	( 0x38 )

#define	ROLE_SYSTEM_BUTTONMENU	( 0x39 )

#define	ROLE_SYSTEM_BUTTONDROPDOWNGRID	( 0x3a )

#define	ROLE_SYSTEM_WHITESPACE	( 0x3b )

#define	ROLE_SYSTEM_PAGETABLIST	( 0x3c )

#define	ROLE_SYSTEM_CLOCK	( 0x3d )


EXTERN_C const IID IID_IAccessible;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("618736e0-3c3d-11cf-810c-00aa00389b71")
    IAccessible : public IDispatch
    {
    public:
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accParent( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispParent) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accChildCount( 
            /* [retval][out] */ long __RPC_FAR *pcountChildren) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accChild( 
            /* [in] */ VARIANT varChild,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispChild) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accName( 
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR __RPC_FAR *pszName) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accValue( 
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR __RPC_FAR *pszValue) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accDescription( 
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR __RPC_FAR *pszDescription) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accRole( 
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarRole) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accState( 
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarState) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accHelp( 
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR __RPC_FAR *pszHelp) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accHelpTopic( 
            /* [out] */ BSTR __RPC_FAR *pszHelpFile,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ long __RPC_FAR *pidTopic) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut( 
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR __RPC_FAR *pszKeyboardShortcut) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accFocus( 
            /* [retval][out] */ VARIANT __RPC_FAR *pvarChild) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accSelection( 
            /* [retval][out] */ VARIANT __RPC_FAR *pvarChildren) = 0;
        
        virtual /* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE get_accDefaultAction( 
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR __RPC_FAR *pszDefaultAction) = 0;
        
        virtual /* [id][hidden] */ HRESULT STDMETHODCALLTYPE accSelect( 
            /* [in] */ long flagsSelect,
            /* [optional][in] */ VARIANT varChild) = 0;
        
        virtual /* [id][hidden] */ HRESULT STDMETHODCALLTYPE accLocation( 
            /* [out] */ long __RPC_FAR *pxLeft,
            /* [out] */ long __RPC_FAR *pyTop,
            /* [out] */ long __RPC_FAR *pcxWidth,
            /* [out] */ long __RPC_FAR *pcyHeight,
            /* [optional][in] */ VARIANT varChild) = 0;
        
        virtual /* [id][hidden] */ HRESULT STDMETHODCALLTYPE accNavigate( 
            /* [in] */ long navDir,
            /* [optional][in] */ VARIANT varStart,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarEndUpAt) = 0;
        
        virtual /* [id][hidden] */ HRESULT STDMETHODCALLTYPE accHitTest( 
            /* [in] */ long xLeft,
            /* [in] */ long yTop,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarChild) = 0;
        
        virtual /* [id][hidden] */ HRESULT STDMETHODCALLTYPE accDoDefaultAction( 
            /* [optional][in] */ VARIANT varChild) = 0;
        
        virtual /* [id][propput][hidden] */ HRESULT STDMETHODCALLTYPE put_accName( 
            /* [optional][in] */ VARIANT varChild,
            /* [in] */ BSTR szName) = 0;
        
        virtual /* [id][propput][hidden] */ HRESULT STDMETHODCALLTYPE put_accValue( 
            /* [optional][in] */ VARIANT varChild,
            /* [in] */ BSTR szValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessibleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAccessible __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAccessible __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAccessible __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IAccessible __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IAccessible __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IAccessible __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IAccessible __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accParent )( 
            IAccessible __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispParent);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accChildCount )( 
            IAccessible __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pcountChildren);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accChild )( 
            IAccessible __RPC_FAR * This,
            /* [in] */ VARIANT varChild,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispChild);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accName )( 
            IAccessible __RPC_FAR * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR __RPC_FAR *pszName);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accValue )( 
            IAccessible __RPC_FAR * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR __RPC_FAR *pszValue);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accDescription )( 
            IAccessible __RPC_FAR * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR __RPC_FAR *pszDescription);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accRole )( 
            IAccessible __RPC_FAR * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarRole);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accState )( 
            IAccessible __RPC_FAR * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarState);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accHelp )( 
            IAccessible __RPC_FAR * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR __RPC_FAR *pszHelp);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accHelpTopic )( 
            IAccessible __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pszHelpFile,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ long __RPC_FAR *pidTopic);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accKeyboardShortcut )( 
            IAccessible __RPC_FAR * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR __RPC_FAR *pszKeyboardShortcut);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accFocus )( 
            IAccessible __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarChild);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accSelection )( 
            IAccessible __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarChildren);
        
        /* [id][propget][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_accDefaultAction )( 
            IAccessible __RPC_FAR * This,
            /* [optional][in] */ VARIANT varChild,
            /* [retval][out] */ BSTR __RPC_FAR *pszDefaultAction);
        
        /* [id][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *accSelect )( 
            IAccessible __RPC_FAR * This,
            /* [in] */ long flagsSelect,
            /* [optional][in] */ VARIANT varChild);
        
        /* [id][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *accLocation )( 
            IAccessible __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pxLeft,
            /* [out] */ long __RPC_FAR *pyTop,
            /* [out] */ long __RPC_FAR *pcxWidth,
            /* [out] */ long __RPC_FAR *pcyHeight,
            /* [optional][in] */ VARIANT varChild);
        
        /* [id][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *accNavigate )( 
            IAccessible __RPC_FAR * This,
            /* [in] */ long navDir,
            /* [optional][in] */ VARIANT varStart,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarEndUpAt);
        
        /* [id][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *accHitTest )( 
            IAccessible __RPC_FAR * This,
            /* [in] */ long xLeft,
            /* [in] */ long yTop,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarChild);
        
        /* [id][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *accDoDefaultAction )( 
            IAccessible __RPC_FAR * This,
            /* [optional][in] */ VARIANT varChild);
        
        /* [id][propput][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_accName )( 
            IAccessible __RPC_FAR * This,
            /* [optional][in] */ VARIANT varChild,
            /* [in] */ BSTR szName);
        
        /* [id][propput][hidden] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_accValue )( 
            IAccessible __RPC_FAR * This,
            /* [optional][in] */ VARIANT varChild,
            /* [in] */ BSTR szValue);
        
        END_INTERFACE
    } IAccessibleVtbl;

    interface IAccessible
    {
        CONST_VTBL struct IAccessibleVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessible_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAccessible_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAccessible_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAccessible_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAccessible_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAccessible_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAccessible_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAccessible_get_accParent(This,ppdispParent)	\
    (This)->lpVtbl -> get_accParent(This,ppdispParent)

#define IAccessible_get_accChildCount(This,pcountChildren)	\
    (This)->lpVtbl -> get_accChildCount(This,pcountChildren)

#define IAccessible_get_accChild(This,varChild,ppdispChild)	\
    (This)->lpVtbl -> get_accChild(This,varChild,ppdispChild)

#define IAccessible_get_accName(This,varChild,pszName)	\
    (This)->lpVtbl -> get_accName(This,varChild,pszName)

#define IAccessible_get_accValue(This,varChild,pszValue)	\
    (This)->lpVtbl -> get_accValue(This,varChild,pszValue)

#define IAccessible_get_accDescription(This,varChild,pszDescription)	\
    (This)->lpVtbl -> get_accDescription(This,varChild,pszDescription)

#define IAccessible_get_accRole(This,varChild,pvarRole)	\
    (This)->lpVtbl -> get_accRole(This,varChild,pvarRole)

#define IAccessible_get_accState(This,varChild,pvarState)	\
    (This)->lpVtbl -> get_accState(This,varChild,pvarState)

#define IAccessible_get_accHelp(This,varChild,pszHelp)	\
    (This)->lpVtbl -> get_accHelp(This,varChild,pszHelp)

#define IAccessible_get_accHelpTopic(This,pszHelpFile,varChild,pidTopic)	\
    (This)->lpVtbl -> get_accHelpTopic(This,pszHelpFile,varChild,pidTopic)

#define IAccessible_get_accKeyboardShortcut(This,varChild,pszKeyboardShortcut)	\
    (This)->lpVtbl -> get_accKeyboardShortcut(This,varChild,pszKeyboardShortcut)

#define IAccessible_get_accFocus(This,pvarChild)	\
    (This)->lpVtbl -> get_accFocus(This,pvarChild)

#define IAccessible_get_accSelection(This,pvarChildren)	\
    (This)->lpVtbl -> get_accSelection(This,pvarChildren)

#define IAccessible_get_accDefaultAction(This,varChild,pszDefaultAction)	\
    (This)->lpVtbl -> get_accDefaultAction(This,varChild,pszDefaultAction)

#define IAccessible_accSelect(This,flagsSelect,varChild)	\
    (This)->lpVtbl -> accSelect(This,flagsSelect,varChild)

#define IAccessible_accLocation(This,pxLeft,pyTop,pcxWidth,pcyHeight,varChild)	\
    (This)->lpVtbl -> accLocation(This,pxLeft,pyTop,pcxWidth,pcyHeight,varChild)

#define IAccessible_accNavigate(This,navDir,varStart,pvarEndUpAt)	\
    (This)->lpVtbl -> accNavigate(This,navDir,varStart,pvarEndUpAt)

#define IAccessible_accHitTest(This,xLeft,yTop,pvarChild)	\
    (This)->lpVtbl -> accHitTest(This,xLeft,yTop,pvarChild)

#define IAccessible_accDoDefaultAction(This,varChild)	\
    (This)->lpVtbl -> accDoDefaultAction(This,varChild)

#define IAccessible_put_accName(This,varChild,szName)	\
    (This)->lpVtbl -> put_accName(This,varChild,szName)

#define IAccessible_put_accValue(This,varChild,szValue)	\
    (This)->lpVtbl -> put_accValue(This,varChild,szValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accParent_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispParent);


void __RPC_STUB IAccessible_get_accParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accChildCount_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pcountChildren);


void __RPC_STUB IAccessible_get_accChildCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accChild_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [in] */ VARIANT varChild,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdispChild);


void __RPC_STUB IAccessible_get_accChild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accName_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ BSTR __RPC_FAR *pszName);


void __RPC_STUB IAccessible_get_accName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accValue_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ BSTR __RPC_FAR *pszValue);


void __RPC_STUB IAccessible_get_accValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accDescription_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ BSTR __RPC_FAR *pszDescription);


void __RPC_STUB IAccessible_get_accDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accRole_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarRole);


void __RPC_STUB IAccessible_get_accRole_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accState_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarState);


void __RPC_STUB IAccessible_get_accState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accHelp_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ BSTR __RPC_FAR *pszHelp);


void __RPC_STUB IAccessible_get_accHelp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accHelpTopic_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pszHelpFile,
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ long __RPC_FAR *pidTopic);


void __RPC_STUB IAccessible_get_accHelpTopic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accKeyboardShortcut_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ BSTR __RPC_FAR *pszKeyboardShortcut);


void __RPC_STUB IAccessible_get_accKeyboardShortcut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accFocus_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarChild);


void __RPC_STUB IAccessible_get_accFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accSelection_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarChildren);


void __RPC_STUB IAccessible_get_accSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_get_accDefaultAction_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ BSTR __RPC_FAR *pszDefaultAction);


void __RPC_STUB IAccessible_get_accDefaultAction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_accSelect_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [in] */ long flagsSelect,
    /* [optional][in] */ VARIANT varChild);


void __RPC_STUB IAccessible_accSelect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_accLocation_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pxLeft,
    /* [out] */ long __RPC_FAR *pyTop,
    /* [out] */ long __RPC_FAR *pcxWidth,
    /* [out] */ long __RPC_FAR *pcyHeight,
    /* [optional][in] */ VARIANT varChild);


void __RPC_STUB IAccessible_accLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_accNavigate_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [in] */ long navDir,
    /* [optional][in] */ VARIANT varStart,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarEndUpAt);


void __RPC_STUB IAccessible_accNavigate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_accHitTest_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [in] */ long xLeft,
    /* [in] */ long yTop,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarChild);


void __RPC_STUB IAccessible_accHitTest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_accDoDefaultAction_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [optional][in] */ VARIANT varChild);


void __RPC_STUB IAccessible_accDoDefaultAction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_put_accName_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [optional][in] */ VARIANT varChild,
    /* [in] */ BSTR szName);


void __RPC_STUB IAccessible_put_accName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput][hidden] */ HRESULT STDMETHODCALLTYPE IAccessible_put_accValue_Proxy( 
    IAccessible __RPC_FAR * This,
    /* [optional][in] */ VARIANT varChild,
    /* [in] */ BSTR szValue);


void __RPC_STUB IAccessible_put_accValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAccessible_INTERFACE_DEFINED__ */


#ifndef __IAccessibleHandler_INTERFACE_DEFINED__
#define __IAccessibleHandler_INTERFACE_DEFINED__

/* interface IAccessibleHandler */
/* [unique][oleautomation][hidden][uuid][object] */ 

typedef /* [unique] */ IAccessibleHandler __RPC_FAR *LPACCESSIBLEHANDLER;


EXTERN_C const IID IID_IAccessibleHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("03022430-ABC4-11d0-BDE2-00AA001A1953")
    IAccessibleHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AccessibleObjectFromID( 
            /* [in] */ long hwnd,
            /* [in] */ long lObjectID,
            /* [out] */ LPACCESSIBLE __RPC_FAR *pIAccessible) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessibleHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAccessibleHandler __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAccessibleHandler __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAccessibleHandler __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AccessibleObjectFromID )( 
            IAccessibleHandler __RPC_FAR * This,
            /* [in] */ long hwnd,
            /* [in] */ long lObjectID,
            /* [out] */ LPACCESSIBLE __RPC_FAR *pIAccessible);
        
        END_INTERFACE
    } IAccessibleHandlerVtbl;

    interface IAccessibleHandler
    {
        CONST_VTBL struct IAccessibleHandlerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessibleHandler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAccessibleHandler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAccessibleHandler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAccessibleHandler_AccessibleObjectFromID(This,hwnd,lObjectID,pIAccessible)	\
    (This)->lpVtbl -> AccessibleObjectFromID(This,hwnd,lObjectID,pIAccessible)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAccessibleHandler_AccessibleObjectFromID_Proxy( 
    IAccessibleHandler __RPC_FAR * This,
    /* [in] */ long hwnd,
    /* [in] */ long lObjectID,
    /* [out] */ LPACCESSIBLE __RPC_FAR *pIAccessible);


void __RPC_STUB IAccessibleHandler_AccessibleObjectFromID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAccessibleHandler_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_oleacc_0105 */
/* [local] */ 


//=--------------------------------------------------------------------------=
// Type Library Definitions
//=--------------------------------------------------------------------------=



extern RPC_IF_HANDLE __MIDL_itf_oleacc_0105_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_oleacc_0105_v0_0_s_ifspec;


#ifndef __Accessibility_LIBRARY_DEFINED__
#define __Accessibility_LIBRARY_DEFINED__

/* library Accessibility */
/* [hidden][version][lcid][uuid] */ 




EXTERN_C const IID LIBID_Accessibility;
#endif /* __Accessibility_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
