
#ifndef __IVPConfig__
#define __IVPConfig__

#ifdef __cplusplus
extern "C" {
#endif


DECLARE_INTERFACE_(IVPBaseConfig, IUnknown)
{
  public:
  STDMETHOD (GetConnectInfo)(THIS_ IN OUT LPDWORD pdwNumConnectInfo,
                                   IN OUT LPDDVIDEOPORTCONNECT pddVPConnectInfo) PURE;

  STDMETHOD (SetConnectInfo)(THIS_ IN DWORD dwChosenEntry) PURE;
  STDMETHOD (GetVPDataInfo)(THIS_ IN OUT LPAMVPDATAINFO pamvpDataInfo) PURE;
  STDMETHOD (GetMaxPixelRate)(THIS_ IN OUT LPAMVPSIZE pamvpSize,
                                    OUT LPDWORD pdwMaxPixelsPerSecond) PURE;

  STDMETHOD (InformVPInputFormats)(THIS_ IN DWORD dwNumFormats, 
                                         IN LPDDPIXELFORMAT pDDPixelFormats) PURE;

  STDMETHOD (GetVideoFormats)(THIS_ IN OUT LPDWORD pdwNumFormats,
                                    IN OUT LPDDPIXELFORMAT pddPixelFormats) PURE;

  STDMETHOD (SetVideoFormat)(THIS_ IN DWORD dwChosenEntry) PURE;
  STDMETHOD (SetInvertPolarity)(THIS_ ) PURE;
  STDMETHOD (GetOverlaySurface)(THIS_ OUT LPDIRECTDRAWSURFACE* ppddOverlaySurface) PURE;
  STDMETHOD (SetDirectDrawKernelHandle)(THIS_ IN ULONG_PTR dwDDKernelHandle) PURE;
  STDMETHOD (SetVideoPortID)(THIS_ IN DWORD dwVideoPortID) PURE;

  STDMETHOD (SetDDSurfaceKernelHandles)(THIS_ IN DWORD cHandles, 
                                              IN ULONG_PTR *rgDDKernelHandles) PURE;

  STDMETHOD (SetSurfaceParameters)(THIS_ IN DWORD dwPitch, IN DWORD dwXOrigin,
                                         IN DWORD dwYOrigin) PURE;
};

DECLARE_INTERFACE_(IVPConfig, IVPBaseConfig)
{
  public:
    STDMETHOD (IsVPDecimationAllowed)(THIS_ OUT LPBOOL pbIsDecimationAllowed) PURE;
    STDMETHOD (SetScalingFactors)(THIS_ IN LPAMVPSIZE pamvpSize) PURE;
};

DECLARE_INTERFACE_(IVPVBIConfig, IVPBaseConfig)
{
  public:
};

#ifdef __cplusplus
}
#endif


#endif

