/*
 * amvideo.h
 *
 * DirectX header
 *
 * Copyright Magnus Olsen (magnus@greatlord.com)
 */

#ifndef __AMVIDEO__
#define __AMVIDEO__

#ifdef __cplusplus
extern "C" {
#endif

#include <ddraw.h>

#define  AMDDS_NONE        0x00
#define  AMDDS_DCIPS       0x01
#define  AMDDS_PS          0x02
#define  AMDDS_RGBOVR      0x04
#define  AMDDS_YUVOVR      0x08
#define  AMDDS_RGBOFF      0x10
#define  AMDDS_YUVOFF      0x20
#define  AMDDS_RGBFLP      0x40
#define  AMDDS_YUVFLP      0x80
#define  AMDDS_            ALL 0xFF
#define  AMDDS_DEFAULT     AMDDS_ALL
#define  AMDDS_YUV         (AMDDS_YUVOFF | AMDDS_YUVOVR | AMDDS_YUVFLP)
#define  AMDDS_RGB         (AMDDS_RGBOFF | AMDDS_RGBOVR | AMDDS_RGBFLP)
#define  AMDDS_PRIMARY     (AMDDS_DCIPS | AMDDS_PS)
#define  iPALETTE_COLORS   256
#define  iEGA_COLORS       16
#define  iMASK_COLORS      3
#define  iTRUECOLOR        16
#define  iRED              0
#define  iGREEN            1
#define  iBLUE             2
#define  iPALETTE          8
#define  iMAXBITS          8

typedef struct tag_TRUECOLORINFO
{
         DWORD             dwBitMasks[iMASK_COLORS];
         RGBQUAD           bmiColors[iPALETTE_COLORS];
} TRUECOLORINFO;


typedef struct tagVIDEOINFOHEADER
{
         RECT              rcSource;
         RECT              rcTarget;
         DWORD             dwBitRate;
         DWORD             dwBitErrorRate;
         REFERENCE_TIME    AvgTimePerFrame;
         BITMAPINFOHEADER  bmiHeader;
} VIDEOINFOHEADER;


typedef struct tagVIDEOINFO
{
         RECT               rcSource;
         RECT               rcTarget;
         DWORD              dwBitRate;
         DWORD              dwBitErrorRate;
         REFERENCE_TIME     AvgTimePerFrame;
         BITMAPINFOHEADER   bmiHeader;
         union
         {
               RGBQUAD       bmiColors[iPALETTE_COLORS];
               DWORD         dwBitMasks[iMASK_COLORS];
               TRUECOLORINFO TrueColorInfo;
         };
} VIDEOINFO;

typedef struct tagMPEG1VIDEOINFO
{
        VIDEOINFOHEADER      hdr;
        DWORD                dwStartTimeCode;
        DWORD                cbSequenceHeader;
        BYTE                 bSequenceHeader[1];
} MPEG1VIDEOINFO;

typedef struct tagAnalogVideoInfo
{
        RECT                 rcSource;
        RECT                 rcTarget;
        DWORD                dwActiveWidth;
        DWORD                dwActiveHeight;
        REFERENCE_TIME       AvgTimePerFrame;
} ANALOGVIDEOINFO;

#define TRUECOLOR(PBMIH)  ((TRUECOLORINFO *)(((LPBYTE)&((PBMIH)->bmiHeader)) + (PBMIH)->bmiHeader.biSize))
#define COLORS(PBMIH)	  ((RGBQUAD *)(((LPBYTE)&((PBMIH)->bmiHeader)) + (PBMIH)->bmiHeader.biSize))
#define BITMASKS(PBMIH)	  ((DWORD *)(((LPBYTE)&((PBMIH)->bmiHeader)) + (PBMIH)->bmiHeader.biSize))

#define SIZE_EGA_PALETTE  (iEGA_COLORS * sizeof(RGBQUAD))
#define SIZE_PALETTE      (iPALETTE_COLORS * sizeof(RGBQUAD))
#define SIZE_MASKS        (iMASK_COLORS * sizeof(DWORD))
#define SIZE_PREHEADER    (FIELD_OFFSET(VIDEOINFOHEADER,bmiHeader))
#define SIZE_VIDEOHEADER  (sizeof(BITMAPINFOHEADER) + SIZE_PREHEADER)

#define WIDTHBYTES(BTIS)  ((DWORD)(((BTIS)+31) & (~31)) / 8)
#define DIBWIDTHBYTES(BI) (DWORD)(BI).biBitCount) * (DWORD)WIDTHBYTES((DWORD)(BI).biWidth
#define _DIBSIZE(BI)      (DIBWIDTHBYTES(BI) * (DWORD)(BI).biHeight)
#define DIBSIZE(BI)       ((BI).biHeight < 0 ? (-1)*(_DIBSIZE(BI)) : _DIBSIZE(BI))

#define BIT_MASKS_MATCH(PBMIH1,PBMIH2)                                \
        ((PBMIH2)->dwBitMasks[iGREEN] == (PBMIH1)->dwBitMasks[iGREEN]) && \
        (((PBMIH2)->dwBitMasks[iRED] == (PBMIH1)->dwBitMasks[iRED]) &&        \
        ((PBMIH2)->dwBitMasks[iBLUE] == (PBMIH1)->dwBitMasks[iBLUE]))

#define    RESET_MASKS(PBMIH)            (ZeroMemory((PVOID)(PBMIH)->dwBitFields,SIZE_MASKS))
#define    RESET_HEADER(PBMIH)           (ZeroMemory((PVOID)(PBMIH),SIZE_VIDEOHEADER))
#define    RESET_PALETTE(PBMIH)          (ZeroMemory((PVOID)(PBMIH)->bmiColors,SIZE_PALETTE));
#define    PALETTISED(PBMIH)             ((PBMIH)->bmiHeader.biBitCount <= iPALETTE)
#define    PALETTE_ENTRIES(PBMIH)        ((DWORD) 1 << (PBMIH)->bmiHeader.biBitCount)
#define    HEADER(pVideoInfo)            (&(((VIDEOINFOHEADER *) (pVideoInfo))->bmiHeader))
#define    MAX_SIZE_MPEG1_SEQUENCE_INFO  140
#define    MPEG1_SEQUENCE_INFO(pv)       ((const BYTE *)(pv)->bSequenceHeader)
#define    SIZE_MPEG1VIDEOINFO(pv)       (FIELD_OFFSET(MPEG1VIDEOINFO, bSequenceHeader[0]) + \
                                         (pv)->cbSequenceHeader)

#undef INTERFACE
#define INTERFACE IDirectDrawVideo

DECLARE_INTERFACE_(IDirectDrawVideo, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(GetSwitches)(THIS_ DWORD *pSwitches) PURE;
  STDMETHOD(SetSwitches)(THIS_ DWORD Switches) PURE;
  STDMETHOD(GetCaps)(THIS_ DDCAPS *pCaps) PURE;
  STDMETHOD(GetEmulatedCaps)(THIS_ DDCAPS *pCaps) PURE;
  STDMETHOD(GetSurfaceDesc)(THIS_ DDSURFACEDESC *pSurfaceDesc) PURE;
  STDMETHOD(GetFourCCCodes)(THIS_ DWORD *pCount,DWORD *pCodes) PURE;
  STDMETHOD(SetDirectDraw)(THIS_ LPDIRECTDRAW pDirectDraw) PURE;
  STDMETHOD(GetDirectDraw)(THIS_ LPDIRECTDRAW *ppDirectDraw) PURE;
  STDMETHOD(GetSurfaceType)(THIS_ DWORD *pSurfaceType) PURE;
  STDMETHOD(SetDefault)(THIS) PURE;
  STDMETHOD(UseScanLine)(THIS_ long UseScanLine) PURE;
  STDMETHOD(CanUseScanLine)(THIS_ long *UseScanLine) PURE;
  STDMETHOD(UseOverlayStretch)(THIS_ long UseOverlayStretch) PURE;
  STDMETHOD(CanUseOverlayStretch)(THIS_ long *UseOverlayStretch) PURE;
  STDMETHOD(UseWhenFullScreen)(THIS_ long UseWhenFullScreen) PURE;
  STDMETHOD(WillUseFullScreen)(THIS_ long *UseWhenFullScreen) PURE;
};


#undef INTERFACE
#define INTERFACE IFullScreenVideo

DECLARE_INTERFACE_(IFullScreenVideo, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(CountModes)(THIS_ long *pModes) PURE;
  STDMETHOD(GetModeInfo)(THIS_ long Mode,long *pWidth,long *pHeight,long *pDepth) PURE;
  STDMETHOD(GetCurrentMode)(THIS_ long *pMode) PURE;
  STDMETHOD(IsModeAvailable)(THIS_ long Mode) PURE;
  STDMETHOD(IsModeEnabled)(THIS_ long Mode) PURE;
  STDMETHOD(SetEnabled)(THIS_ long Mode,long bEnabled) PURE;
  STDMETHOD(GetClipFactor)(THIS_ long *pClipFactor) PURE;
  STDMETHOD(SetClipFactor)(THIS_ long ClipFactor) PURE;
  STDMETHOD(SetMessageDrain)(THIS_ HWND hwnd) PURE;
  STDMETHOD(GetMessageDrain)(THIS_ HWND *hwnd) PURE;
  STDMETHOD(SetMonitor)(THIS_ long Monitor) PURE;
  STDMETHOD(GetMonitor)(THIS_ long *Monitor) PURE;
  STDMETHOD(HideOnDeactivate)(THIS_ long Hide) PURE;
  STDMETHOD(IsHideOnDeactivate)(THIS) PURE;
  STDMETHOD(SetCaption)(THIS_ BSTR strCaption) PURE;
  STDMETHOD(GetCaption)(THIS_ BSTR *pstrCaption) PURE;
  STDMETHOD(SetDefault)(THIS) PURE;
}

#undef INTERFACE
#define INTERFACE IQualProp

DECLARE_INTERFACE_(IQualProp, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(get_FramesDroppedInRenderer)(THIS_ int *pcFrames) PURE;
  STDMETHOD(get_FramesDrawn)(THIS_ int *pcFramesDrawn) PURE;
  STDMETHOD(get_AvgFrameRate)(THIS_ int *piAvgFrameRate) PURE;
  STDMETHOD(get_Jitter)(THIS_ int *iJitter) PURE;
  STDMETHOD(get_AvgSyncOffset)(THIS_ int *piAvg) PURE;
  STDMETHOD(get_DevSyncOffset)(THIS_ int *piDev) PURE;
};


#undef INTERFACE
#define INTERFACE IBaseVideoMixer

DECLARE_INTERFACE_(IBaseVideoMixer, IUnknown)
{
  STDMETHOD(SetLeadPin)(THIS_ int iPin) PURE;
  STDMETHOD(GetLeadPin)(THIS_ int *piPin) PURE;
  STDMETHOD(GetInputPinCount)(THIS_ int *piPinCount) PURE;
  STDMETHOD(IsUsingClock)(THIS_ int *pbValue) PURE;
  STDMETHOD(SetUsingClock)(THIS_ int bValue) PURE;
  STDMETHOD(GetClockPeriod)(THIS_ int *pbValue) PURE;
  STDMETHOD(SetClockPeriod)(THIS_ int bValue) PURE;
};

#undef INTERFACE
#define INTERFACE IFullScreenVideoEx

DECLARE_INTERFACE_(IFullScreenVideoEx, IFullScreenVideo)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(CountModes)(THIS_ long *pModes) PURE;
  STDMETHOD(GetModeInfo)(THIS_ long Mode,long *pWidth,long *pHeight,long *pDepth) PURE;
  STDMETHOD(GetCurrentMode)(THIS_ long *pMode) PURE;
  STDMETHOD(IsModeAvailable)(THIS_ long Mode) PURE;
  STDMETHOD(IsModeEnabled)(THIS_ long Mode) PURE;
  STDMETHOD(SetEnabled)(THIS_ long Mode,long bEnabled) PURE;
  STDMETHOD(GetClipFactor)(THIS_ long *pClipFactor) PURE;
  STDMETHOD(SetClipFactor)(THIS_ long ClipFactor) PURE;
  STDMETHOD(SetMessageDrain)(THIS_ HWND hwnd) PURE;
  STDMETHOD(GetMessageDrain)(THIS_ HWND *hwnd) PURE;
  STDMETHOD(SetMonitor)(THIS_ long Monitor) PURE;
  STDMETHOD(GetMonitor)(THIS_ long *Monitor) PURE;
  STDMETHOD(HideOnDeactivate)(THIS_ long Hide) PURE;
  STDMETHOD(IsHideOnDeactivate)(THIS) PURE;
  STDMETHOD(SetCaption)(THIS_ BSTR strCaption) PURE;
  STDMETHOD(GetCaption)(THIS_ BSTR *pstrCaption) PURE;
  STDMETHOD(SetDefault)(THIS) PURE;
  STDMETHOD(SetAcceleratorTable)(THIS_ HWND hwnd,HACCEL hAccel) PURE;
  STDMETHOD(GetAcceleratorTable)(THIS_ HWND *phwnd,HACCEL *phAccel) PURE;
  STDMETHOD(KeepPixelAspectRatio)(THIS_ long KeepAspect) PURE;
  STDMETHOD(IsKeepPixelAspectRatio)(THIS_ long *pKeepAspect) PURE;
};




typedef enum
{
  AM_PROPERTY_FRAMESTEP_STEP            = 0x01,
  AM_PROPERTY_FRAMESTEP_CANCEL          = 0x02,
  AM_PROPERTY_FRAMESTEP_CANSTEP         = 0x03,
  AM_PROPERTY_FRAMESTEP_CANSTEPMULTIPLE = 0x04
} AM_PROPERTY_FRAMESTEP;

typedef struct _AM_FRAMESTEP_STEP
{
    DWORD dwFramesToStep;
} AM_FRAMESTEP_STEP;

#ifdef __cplusplus
}
#endif
#endif

