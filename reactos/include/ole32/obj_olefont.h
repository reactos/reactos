/*
 * Defines the COM interfaces and APIs related to OLE font support.
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_OLEFONT_H
#define __WINE_WINE_OBJ_OLEFONT_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IFont, 0xBEF6E002, 0xA874, 0x101A, 0x8B, 0xBA, 0x00, 0xAA, 0x00, 0x30, 0x0C, 0xAB);
typedef struct IFont IFont,*LPFONT;

DEFINE_GUID(IID_IFontDisp, 0xBEF6E003, 0xA874, 0x101A, 0x8B, 0xBA, 0x00, 0xAA, 0x00, 0x30, 0x0C, 0xAB);
typedef struct IFontDisp IFontDisp,*LPFONTDISP;

typedef TEXTMETRICW TEXTMETRICOLE;

/*****************************************************************************
 * IFont interface
 */
#define ICOM_INTERFACE IFont
#define IFont_METHODS \
  ICOM_METHOD1(HRESULT, get_Name, BSTR*, pname) \
  ICOM_METHOD1(HRESULT, put_Name, BSTR, name) \
  ICOM_METHOD1(HRESULT, get_Size, CY*, psize) \
  ICOM_METHOD1(HRESULT, put_Size, CY, size) \
  ICOM_METHOD1(HRESULT, get_Bold, BOOL*, pbold) \
  ICOM_METHOD1(HRESULT, put_Bold, BOOL, bold) \
  ICOM_METHOD1(HRESULT, get_Italic, BOOL*, pitalic) \
  ICOM_METHOD1(HRESULT, put_Italic, BOOL, italic) \
  ICOM_METHOD1(HRESULT, get_Underline, BOOL*, punderline) \
  ICOM_METHOD1(HRESULT, put_Underline, BOOL, underline) \
  ICOM_METHOD1(HRESULT, get_Strikethrough, BOOL*, pstrikethrough) \
  ICOM_METHOD1(HRESULT, put_Strikethrough, BOOL, strikethrough) \
  ICOM_METHOD1(HRESULT, get_Weight, short*, pweight) \
  ICOM_METHOD1(HRESULT, put_Weight, short, weight) \
  ICOM_METHOD1(HRESULT, get_Charset, short*, pcharset) \
  ICOM_METHOD1(HRESULT, put_Charset, short, charset) \
  ICOM_METHOD1(HRESULT, get_hFont, HFONT*, phfont) \
  ICOM_METHOD1(HRESULT, Clone, IFont**, ppfont) \
  ICOM_METHOD1(HRESULT, IsEqual, IFont*, pFontOther) \
  ICOM_METHOD2(HRESULT, SetRatio, long, cyLogical, long, cyHimetric) \
  ICOM_METHOD1(HRESULT, QueryTextMetrics, TEXTMETRICOLE*, ptm) \
  ICOM_METHOD1(HRESULT, AddRefHfont, HFONT, hfont) \
  ICOM_METHOD1(HRESULT, ReleaseHfont, HFONT, hfont) \
  ICOM_METHOD1(HRESULT, SetHdc, HDC, hdc)     
#define IFont_IMETHODS \
	IUnknown_IMEHTODS \
	IFont_METHODS
ICOM_DEFINE(IFont,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IFont_QueryInterface(p,a,b)  ICOM_CALL2(QueryInterface,p,a,b)
#define IFont_AddRef(p)              ICOM_CALL (AddRef,p)
#define IFont_Release(p)             ICOM_CALL (Release,p)
/*** IFont methods ***/
#define IFont_getName(p,a)           ICOM_CALL1(get_Name,p,a)
#define IFont_putName(p,a)           ICOM_CALL1(put_Name,p,a)
#define IFont_get_Size(p,a)          ICOM_CALL1(get_Size,p,a)
#define IFont_put_Size(p,a)          ICOM_CALL1(put_Size,p,a)
#define IFont_get_Bold(p,a)          ICOM_CALL1(get_Bold,a)
#define IFont_put_Bold(p,a)          ICOM_CALL1(put_Bold,a)
#define IFont_get_Italic(p,a)        ICOM_CALL1(get_Italic,a)
#define IFont_put_Italic(p,a)        ICOM_CALL1(put_Italic,a)
#define IFont_get_Underline(p,a)     ICOM_CALL1(get_Underline,a)
#define IFont_put_Underline(p,a)     ICOM_CALL1(put_Underline,a)
#define IFont_get_Strikethrough(p,a) ICOM_CALL1(get_Strikethrough,a)
#define IFont_put_Strikethrough(p,a) ICOM_CALL1(put_Strikethrough,a)
#define IFont_get_Weight(p,a)        ICOM_CALL1(get_Weight,a)
#define IFont_put_Weight(p,a)        ICOM_CALL1(put_Weight,a)
#define IFont_get_Charset(p,a)       ICOM_CALL1(get_Charset,a)
#define IFont_put_Charset(p,a)       ICOM_CALL1(put_Charset,a)
#define IFont_get_hFont(p,a)         ICOM_CALL1(get_hFont,a)
#define IFont_put_hFont(p,a)         ICOM_CALL1(put_hFont,a)
#define IFont_Clone(p,a)             ICOM_CALL1(Clone,a)
#define IFont_IsEqual(p,a)           ICOM_CALL1(IsEqual,a)
#define IFont_SetRatio(p,a,b)        ICOM_CALL2(SetRatio,a,b)
#define IFont_QueryTextMetrics(p,a)  ICOM_CALL1(QueryTextMetrics,a)
#define IFont_AddRefHfont(p,a)       ICOM_CALL1(AddRefHfont,a)
#define IFont_ReleaseHfont(p,a)      ICOM_CALL1(ReleaseHfont,a)
#define IFont_SetHdc(p,a)            ICOM_CALL1(SetHdc,a)

/*****************************************************************************
 * IFont interface
 */
#define ICOM_INTERFACE IFontDisp
#define IFontDisp_METHODS 
#define IFontDisp_IMETHODS \
  IUnknown_IMETHODS \
	IFontDisp_METHODS
ICOM_DEFINE(IFontDisp,IDispatch)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IFontDisp_QueryInterface(p,a,b)  ICOM_CALL2(QueryInterface,p,a,b)
#define IFontDisp_AddRef(p)              ICOM_CALL (AddRef,p)
#define IFontDisp_Release(p)             ICOM_CALL (Release,p)
/*** IDispatch methods ***/
#define IFontDisp_GetTypeInfoCount(p,a)      ICOM_CALL1 (GetTypeInfoCount,p,a)
#define IFontDisp_GetTypeInfo(p,a,b,c)       ICOM_CALL3 (GetTypeInfo,p,b,c)
#define IFontDisp_GetIDsOfNames(p,a,b,c,d,e) ICOM_CALL5 (GetIDsOfNames,p,a,b,c,d,e)
#define IFontDisp_Invoke(p,a,b,c,d,e,f,g,h)  ICOM_CALL8 (Invoke,p,a,b,c,d,e,f,g,h)
/*** IFontDisp methods ***/

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_OLEFONT_H */


