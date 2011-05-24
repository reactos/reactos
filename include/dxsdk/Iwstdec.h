

#ifndef __IWSTDEC__
#define __IWSTDEC__

typedef enum _AM_WST_DRAWBGMODE
{
  AM_WST_DRAWBGMODE_Opaque,
  AM_WST_DRAWBGMODE_Transparent
} AM_WST_DRAWBGMODE, *PAM_WST_DRAWBGMODE;

typedef struct _AM_WST_PAGE
{
  DWORD dwPageNr ;
  DWORD dwSubPageNr ;
  BYTE *pucPageData;
} AM_WST_PAGE, *PAM_WST_PAGE;

typedef enum _AM_WST_STATE
{
  AM_WST_STATE_Off = 0,
  AM_WST_STATE_On
} AM_WST_STATE, *PAM_WST_STATE;

typedef enum _AM_WST_SERVICE
{
  AM_WST_SERVICE_None = 0,
  AM_WST_SERVICE_Text,
  AM_WST_SERVICE_IDS,
  AM_WST_SERVICE_Invalid
} AM_WST_SERVICE, *PAM_WST_SERVICE;

typedef enum _AM_WST_STYLE
{
  AM_WST_STYLE_None = 0,
  AM_WST_STYLE_Invers
} AM_WST_STYLE, *PAM_WST_STYLE;

typedef enum _AM_WST_LEVEL
{
  AM_WST_LEVEL_1_5 = 0
} AM_WST_LEVEL, *PAM_WST_LEVEL;

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_INTERFACE_(IAMWstDecoder, IUnknown)
{
  public:
  STDMETHOD(GetDecoderLevel)(THIS_ AM_WST_LEVEL *lpLevel) PURE;
  STDMETHOD(GetCurrentService)(THIS_ AM_WST_SERVICE *lpService) PURE;
  STDMETHOD(GetServiceState)(THIS_ AM_WST_STATE *lpState) PURE;
  STDMETHOD(SetServiceState)(THIS_ AM_WST_STATE State) PURE ;
  STDMETHOD(GetOutputFormat)(THIS_ LPBITMAPINFOHEADER lpbmih) PURE;
  STDMETHOD(SetOutputFormat)(THIS_ LPBITMAPINFO lpbmi) PURE;
  STDMETHOD(GetBackgroundColor)(THIS_ DWORD *pdwPhysColor) PURE;
  STDMETHOD(SetBackgroundColor)(THIS_ DWORD dwPhysColor) PURE;
  STDMETHOD(GetRedrawAlways)(THIS_ LPBOOL lpbOption) PURE;
  STDMETHOD(SetRedrawAlways)(THIS_ BOOL bOption) PURE;
  STDMETHOD(GetDrawBackgroundMode)(THIS_ AM_WST_DRAWBGMODE *lpMode) PURE;
  STDMETHOD(SetDrawBackgroundMode)(THIS_ AM_WST_DRAWBGMODE Mode) PURE;
  STDMETHOD(SetAnswerMode)(THIS_ BOOL bAnswer) PURE;
  STDMETHOD(GetAnswerMode)(THIS_ BOOL* pbAnswer) PURE;
  STDMETHOD(SetHoldPage)(THIS_ BOOL bHoldPage) PURE;
  STDMETHOD(GetHoldPage)(THIS_ BOOL* pbHoldPage) PURE;
  STDMETHOD(GetCurrentPage)(THIS_ PAM_WST_PAGE pWstPage) PURE;
  STDMETHOD(SetCurrentPage)(THIS_ AM_WST_PAGE WstPage) PURE;
} ;

#ifdef __cplusplus
}
#endif
#endif

