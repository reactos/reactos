/*
 * Defines the COM interfaces and APIs related to the IE Web browser control
 *
 * 2001 John R. Sheets (for CodeWeavers)
 */

#ifndef __WINE_WINE_OBJ_WEBBROWSER_H
#define __WINE_WINE_OBJ_WEBBROWSER_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */


/*****************************************************************************
 * Predeclare the interfaces and class IDs
 */
DEFINE_GUID(IID_IWebBrowser, 0xeab22ac1, 0x30c1, 0x11cf, 0xa7, 0xeb, 0x00, 0x00, 0xc0, 0x5b, 0xae, 0x0b);
typedef struct IWebBrowser IWebBrowser, *LPWEBBROWSER;

DEFINE_GUID(CLSID_WebBrowser, 0x8856f961, 0x340a, 0x11d0, 0xa9, 0x6b, 0x00, 0xc0, 0x4f, 0xd7, 0x05, 0xa2);

/*****************************************************************************
 * IWebBrowser interface
 */
#define ICOM_INTERFACE IWebBrowser
#define IWebBrowser_METHODS \
	ICOM_METHOD(HRESULT,GoBack) \
	ICOM_METHOD(HRESULT,GoForward) \
	ICOM_METHOD(HRESULT,GoHome) \
	ICOM_METHOD(HRESULT,GoSearch) \
	ICOM_METHOD5(HRESULT,Navigate, BSTR*,URL, VARIANT*,Flags, VARIANT*,TargetFrameName, \
                                       VARIANT*,PostData, VARIANT*,Headers) \
	ICOM_METHOD(HRESULT,Refresh) \
	ICOM_METHOD1(HRESULT,Refresh2, VARIANT*,Level) \
	ICOM_METHOD(HRESULT,Stop) \
	ICOM_METHOD1(HRESULT,get_Application, void**,ppDisp) \
	ICOM_METHOD1(HRESULT,get_Parent, void**,ppDisp) \
	ICOM_METHOD1(HRESULT,get_Container, void**,ppDisp) \
	ICOM_METHOD1(HRESULT,get_Document, void**,ppDisp) \
	ICOM_METHOD1(HRESULT,get_TopLevelContainer, VARIANT*,pBool) \
	ICOM_METHOD1(HRESULT,get_Type, BSTR*,Type) \
	ICOM_METHOD1(HRESULT,get_Left, long*,pl) \
	ICOM_METHOD1(HRESULT,put_Left, long,Left) \
	ICOM_METHOD1(HRESULT,get_Top, long*,pl) \
	ICOM_METHOD1(HRESULT,put_Top, long,Top) \
	ICOM_METHOD1(HRESULT,get_Width, long*,pl) \
	ICOM_METHOD1(HRESULT,put_Width, long,Width) \
	ICOM_METHOD1(HRESULT,get_Height, long*,pl) \
	ICOM_METHOD1(HRESULT,put_Height, long,Height) \
	ICOM_METHOD1(HRESULT,get_LocationName, BSTR*,LocationName) \
	ICOM_METHOD1(HRESULT,get_LocationURL, BSTR*,LocationURL) \
	ICOM_METHOD1(HRESULT,get_Busy, VARIANT*,pBool)
#define IWebBrowser_IMETHODS \
	IDispatch_METHODS \
	IWebBrowser_METHODS
ICOM_DEFINE(IWebBrowser,IDispatch)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IWebBrowser_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IWebBrowser_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IWebBrowser_Release(p)                 ICOM_CALL (Release,p)
/*** IDispatch methods ***/
#define IWebBrowser_GetTypeInfoCount(p,a)      ICOM_CALL1 (GetTypeInfoCount,p,a)
#define IWebBrowser_GetTypeInfo(p,a,b,c)       ICOM_CALL3 (GetTypeInfo,p,a,b,c)
#define IWebBrowser_GetIDsOfNames(p,a,b,c,d,e) ICOM_CALL5 (GetIDsOfNames,p,a,b,c,d,e)
#define IWebBrowser_Invoke(p,a,b,c,d,e,f,g,h)  ICOM_CALL8 (Invoke,p,a,b,c,d,e,f,g,h)
/*** IWebBrowserContainer methods ***/
#define IWebBrowser_GoBack(p,a)      ICOM_CALL1(GoBack,p,a)
#define IWebBrowser_GoForward(p,a)      ICOM_CALL1(GoForward,p,a)
#define IWebBrowser_GoHome(p,a)      ICOM_CALL1(GoHome,p,a)
#define IWebBrowser_GoSearch(p,a)      ICOM_CALL1(GoSearch,p,a)
#define IWebBrowser_Navigate(p,a)      ICOM_CALL1(Navigate,p,a)
#define IWebBrowser_Refresh(p,a)      ICOM_CALL1(Refresh,p,a)
#define IWebBrowser_Refresh2(p,a)      ICOM_CALL1(Refresh2,p,a)
#define IWebBrowser_Stop(p,a)      ICOM_CALL1(Stop,p,a)
#define IWebBrowser_get_Application(p,a)      ICOM_CALL1(get_Application,p,a)
#define IWebBrowser_get_Parent(p,a)      ICOM_CALL1(get_Parent,p,a)
#define IWebBrowser_get_Container(p,a)      ICOM_CALL1(get_Container,p,a)
#define IWebBrowser_get_Document(p,a)      ICOM_CALL1(get_Document,p,a)
#define IWebBrowser_get_TopLevelContainer(p,a)      ICOM_CALL1(get_TopLevelContainer,p,a)
#define IWebBrowser_get_Type(p,a)      ICOM_CALL1(get_Type,p,a)
#define IWebBrowser_get_Left(p,a)      ICOM_CALL1(get_Left,p,a)
#define IWebBrowser_put_Left(p,a)      ICOM_CALL1(put_Left,p,a)
#define IWebBrowser_get_Top(p,a)      ICOM_CALL1(get_Top,p,a)
#define IWebBrowser_put_Top(p,a)      ICOM_CALL1(put_Top,p,a)
#define IWebBrowser_get_Width(p,a)      ICOM_CALL1(get_Width,p,a)
#define IWebBrowser_put_Width(p,a)      ICOM_CALL1(put_Width,p,a)
#define IWebBrowser_get_Height(p,a)      ICOM_CALL1(get_Height,p,a)
#define IWebBrowser_put_Height(p,a)      ICOM_CALL1(put_Height,p,a)
#define IWebBrowser_get_LocationName(p,a)      ICOM_CALL1(get_LocationName,p,a)
#define IWebBrowser_get_LocationURL(p,a)      ICOM_CALL1(get_LocationURL,p,a)
#define IWebBrowser_get_Busy(p,a)      ICOM_CALL1(get_Busy,p,a)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_WEBBROWSER_H */
