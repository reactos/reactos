#ifndef __IMPConfig__
#define __IMPConfig__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _AM_ASPECT_RATIO_MODE
{
  AM_ARMODE_STRETCHED,
  AM_ARMODE_LETTER_BOX,
  AM_ARMODE_CROP,
  AM_ARMODE_STRETCHED_AS_PRIMARY
} AM_ASPECT_RATIO_MODE;

DECLARE_INTERFACE_(IMixerPinConfig, IUnknown)
{
  STDMETHOD (SetRelativePosition)(THIS_ IN DWORD dwLeft, IN DWORD dwTop,
                                        IN DWORD dwRight, IN DWORD dwBottom) PURE;
  STDMETHOD (GetRelativePosition)(THIS_ OUT DWORD *pdwLeft,OUT DWORD *pdwTop,
                                        OUT DWORD *pdwRight, OUT DWORD *pdwBottom) PURE;

  STDMETHOD (SetZOrder)(THIS_ IN DWORD dwZOrder) PURE;
  STDMETHOD (GetZOrder)(THIS_ OUT DWORD *pdwZOrder) PURE;
  STDMETHOD (SetColorKey)(THIS_ IN COLORKEY *pColorKey) PURE;
  STDMETHOD (GetColorKey)(THIS_ OUT COLORKEY *pColorKey, OUT DWORD *pColor) PURE;
  STDMETHOD (SetBlendingParameter)(THIS_ IN DWORD dwBlendingParameter) PURE;
  STDMETHOD (GetBlendingParameter)(THIS_ OUT DWORD *pdwBlendingParameter) PURE;
  STDMETHOD (SetAspectRatioMode)(THIS_ IN AM_ASPECT_RATIO_MODE amAspectRatioMode) PURE;
  STDMETHOD (GetAspectRatioMode)(THIS_ OUT AM_ASPECT_RATIO_MODE* pamAspectRatioMode) PURE;
  STDMETHOD (SetStreamTransparent)(THIS_ IN BOOL bStreamTransparent) PURE;
  STDMETHOD (GetStreamTransparent)(THIS_ OUT BOOL *pbStreamTransparent) PURE;
};

DECLARE_INTERFACE_(IMixerPinConfig2, IMixerPinConfig)
{
  STDMETHOD (SetOverlaySurfaceColorControls)(THIS_ IN LPDDCOLORCONTROL pColorControl) PURE;
  STDMETHOD (GetOverlaySurfaceColorControls)(THIS_ OUT LPDDCOLORCONTROL pColorControl) PURE;
};



#ifdef __cplusplus
}
#endif

#endif
