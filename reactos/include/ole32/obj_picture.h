/*
 * Defines the COM interfaces and APIs related to OLE picture support.
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_PICTURE_H
#define __WINE_WINE_OBJ_PICTURE_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the structures
 */
typedef UINT OLE_HANDLE;
typedef LONG OLE_XPOS_HIMETRIC;
typedef LONG OLE_YPOS_HIMETRIC;
typedef LONG OLE_XSIZE_HIMETRIC;
typedef LONG OLE_YSIZE_HIMETRIC;



/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IPicture, 0x7bf80980, 0xbf32, 0x101a, 0x8b, 0xbb, 0x00, 0xAA, 0x00, 0x30, 0x0C, 0xAB);
typedef struct IPicture IPicture, *LPPICTURE;

DEFINE_GUID(IID_IPictureDisp, 0x7bf80981, 0xbf32, 0x101a, 0x8b, 0xbb, 0x00, 0xAA, 0x00, 0x30, 0x0C, 0xAB);
typedef struct IPictureDisp IPictureDisp, *LPPICTUREDISP;

/*****************************************************************************
 * IPicture interface
 */
#define ICOM_INTERFACE IPicture
#define IPicture_METHODS \
  ICOM_METHOD1(HRESULT,get_Handle, OLE_HANDLE*,pHandle) \
  ICOM_METHOD1(HRESULT,get_hPal, OLE_HANDLE*,phPal) \
  ICOM_METHOD1(HRESULT,get_Type, SHORT*,pType) \
  ICOM_METHOD1(HRESULT,get_Width, OLE_XSIZE_HIMETRIC*,pWidth) \
  ICOM_METHOD1(HRESULT,get_Height, OLE_YSIZE_HIMETRIC*,pHeight) \
  ICOM_METHOD10(HRESULT,Render, HDC,hdc, LONG,x, LONG,y, LONG,cx, LONG,cy, OLE_XPOS_HIMETRIC,xSrc, OLE_YPOS_HIMETRIC,ySrc, OLE_XSIZE_HIMETRIC,cxSrc, OLE_YSIZE_HIMETRIC,cySrc, LPCRECT,pRcWBounds) \
  ICOM_METHOD1(HRESULT,set_hPal, OLE_HANDLE,hPal) \
  ICOM_METHOD1(HRESULT,get_CurDC, HDC*,phDC) \
  ICOM_METHOD3(HRESULT,SelectPicture, HDC,hDCIn, HDC*,phDCOut, OLE_HANDLE*,phBmpOut) \
  ICOM_METHOD1(HRESULT,get_KeepOriginalFormat, BOOL*,pKeep) \
  ICOM_METHOD1(HRESULT,put_KeepOriginalFormat, BOOL,Keep) \
  ICOM_METHOD (HRESULT,PictureChanged) \
  ICOM_METHOD3(HRESULT,SaveAsFile, LPSTREAM,pStream, BOOL,fSaveMemCopy, LONG*,pCbSize) \
  ICOM_METHOD1(HRESULT,get_Attributes, DWORD*,pDwAttr) 
#define IPicture_IMETHODS \
	IUnknown_IMETHODS \
	IPicture_METHODS
ICOM_DEFINE(IPicture,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPicture_QueryInterface(p,a,b)         ICOM_CALL2(QueryInterface,p,a,b)
#define IPicture_AddRef(p)                     ICOM_CALL (AddRef,p)
#define IPicture_Release(p)                    ICOM_CALL (Release,p)
/*** IPicture methods ***/
#define IPicture_get_Handle(p,a)               ICOM_CALL1(get_Handle,p,a)
#define IPicture_get_hPal(p,a)                 ICOM_CALL1(get_hPal,p,a)
#define IPicture_get_Type(p,a)                 ICOM_CALL1(get_Type,p,a)
#define IPicture_get_Width(p,a)                ICOM_CALL1(get_Width,p,a)
#define IPicture_get_Height(p,a)               ICOM_CALL1(get_Height,p,a)
#define IPicture_Render(p,a,b,c,d,e,f,g,h,i,j) ICOM_CALL10(Render,p,a,b,c,d,e,f,g,h,i,j)
#define IPicture_set_hPal(p,a)                 ICOM_CALL1(set_hPal,p,a)
#define IPicture_get_CurDC(p,a)                ICOM_CALL1(get_CurDC,p,a)
#define IPicture_SelectPicture(p,a,b,c)        ICOM_CALL3(SelectPicture,p,a,b,c)
#define IPicture_get_KeepOriginalFormat(p,a)   ICOM_CALL1(get_KeepOriginalFormat,p,a)
#define IPicture_put_KeepOriginalFormat(p,a)   ICOM_CALL1(put_KeepOriginalFormat,p,a)
#define IPicture_PictureChanged(p)             ICOM_CALL (PictureChanged,p)
#define IPicture_SaveAsFile(p,a,b,c)           ICOM_CALL3(SaveAsFile,p,a,b,c)
#define IPicture_get_Attributes(p,a)           ICOM_CALL1(get_Attributes,p,a)


/*****************************************************************************
 * IPictureDisp interface
 */
#define ICOM_INTERFACE IPictureDisp
#define IPictureDisp_METHODS 
#define IPictureDisp_IMETHODS \
				IDispatch_IMETHODS \
				IPictureDisp_METHODS
ICOM_DEFINE(IPictureDisp,IDispatch)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPictureDisp_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IPictureDisp_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IPictureDisp_Release(p)                 ICOM_CALL (Release,p)
/*** IDispatch methods ***/
#define IPictureDisp_GetTypeInfoCount(p,a)      ICOM_CALL1 (GetTypeInfoCount,p,a)
#define IPictureDisp_GetTypeInfo(p,a,b,c)       ICOM_CALL3 (GetTypeInfo,p,b,c)
#define IPictureDisp_GetIDsOfNames(p,a,b,c,d,e) ICOM_CALL5 (GetIDsOfNames,p,a,b,c,d,e)
#define IPictureDisp_Invoke(p,a,b,c,d,e,f,g,h)  ICOM_CALL8 (Invoke,p,a,b,c,d,e,f,g,h)
/*** IPictureDisp methods ***/

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_PICTURE_H */


