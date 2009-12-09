/* FILE:            System Call Stubs for Native API
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         obj-amd64\lib\win32ksys\win32k.S
 * PROGRAMMER:      Computer Generated File. See tools/nci/ncitool.c
 * REMARK:          DO NOT EDIT OR COMMIT MODIFICATIONS TO THIS FILE
 */


#include <ndk/asm.h>

.global _NtGdiAbortDoc
_NtGdiAbortDoc:
    movl $0x1000, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiAbortPath
_NtGdiAbortPath:
    movl $0x1001, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiAddFontResourceW
_NtGdiAddFontResourceW:
    movl $0x1002, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiAddRemoteFontToDC
_NtGdiAddRemoteFontToDC:
    movl $0x1003, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiAddFontMemResourceEx
_NtGdiAddFontMemResourceEx:
    movl $0x1004, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiRemoveMergeFont
_NtGdiRemoveMergeFont:
    movl $0x1005, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiAddRemoteMMInstanceToDC
_NtGdiAddRemoteMMInstanceToDC:
    movl $0x1006, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiAlphaBlend
_NtGdiAlphaBlend:
    movl $0x1007, %eax
    movq %rcx, %r10
    syscall
    ret $0x30

.global _NtGdiAngleArc
_NtGdiAngleArc:
    movl $0x1008, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiAnyLinkedFonts
_NtGdiAnyLinkedFonts:
    movl $0x1009, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtGdiFontIsLinked
_NtGdiFontIsLinked:
    movl $0x100a, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiArcInternal
_NtGdiArcInternal:
    movl $0x100b, %eax
    movq %rcx, %r10
    syscall
    ret $0x28

.global _NtGdiBeginPath
_NtGdiBeginPath:
    movl $0x100c, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiBitBlt
_NtGdiBitBlt:
    movl $0x100d, %eax
    movq %rcx, %r10
    syscall
    ret $0x2c

.global _NtGdiCancelDC
_NtGdiCancelDC:
    movl $0x100e, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiCheckBitmapBits
_NtGdiCheckBitmapBits:
    movl $0x100f, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtGdiCloseFigure
_NtGdiCloseFigure:
    movl $0x1010, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiClearBitmapAttributes
_NtGdiClearBitmapAttributes:
    movl $0x1011, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiClearBrushAttributes
_NtGdiClearBrushAttributes:
    movl $0x1012, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiColorCorrectPalette
_NtGdiColorCorrectPalette:
    movl $0x1013, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiCombineRgn
_NtGdiCombineRgn:
    movl $0x1014, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiCombineTransform
_NtGdiCombineTransform:
    movl $0x1015, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiComputeXformCoefficients
_NtGdiComputeXformCoefficients:
    movl $0x1016, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiConsoleTextOut
_NtGdiConsoleTextOut:
    movl $0x1017, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiConvertMetafileRect
_NtGdiConvertMetafileRect:
    movl $0x1018, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiCreateBitmap
_NtGdiCreateBitmap:
    movl $0x1019, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiCreateClientObj
_NtGdiCreateClientObj:
    movl $0x101a, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiCreateColorSpace
_NtGdiCreateColorSpace:
    movl $0x101b, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiCreateColorTransform
_NtGdiCreateColorTransform:
    movl $0x101c, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtGdiCreateCompatibleBitmap
_NtGdiCreateCompatibleBitmap:
    movl $0x101d, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiCreateCompatibleDC
_NtGdiCreateCompatibleDC:
    movl $0x101e, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiCreateDIBBrush
_NtGdiCreateDIBBrush:
    movl $0x101f, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiCreateDIBitmapInternal
_NtGdiCreateDIBitmapInternal:
    movl $0x1020, %eax
    movq %rcx, %r10
    syscall
    ret $0x2c

.global _NtGdiCreateDIBSection
_NtGdiCreateDIBSection:
    movl $0x1021, %eax
    movq %rcx, %r10
    syscall
    ret $0x24

.global _NtGdiCreateEllipticRgn
_NtGdiCreateEllipticRgn:
    movl $0x1022, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiCreateHalftonePalette
_NtGdiCreateHalftonePalette:
    movl $0x1023, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiCreateHatchBrushInternal
_NtGdiCreateHatchBrushInternal:
    movl $0x1024, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiCreateMetafileDC
_NtGdiCreateMetafileDC:
    movl $0x1025, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiCreatePaletteInternal
_NtGdiCreatePaletteInternal:
    movl $0x1026, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiCreatePatternBrushInternal
_NtGdiCreatePatternBrushInternal:
    movl $0x1027, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiCreatePen
_NtGdiCreatePen:
    movl $0x1028, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiCreateRectRgn
_NtGdiCreateRectRgn:
    movl $0x1029, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiCreateRoundRectRgn
_NtGdiCreateRoundRectRgn:
    movl $0x102a, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiCreateServerMetaFile
_NtGdiCreateServerMetaFile:
    movl $0x102b, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiCreateSolidBrush
_NtGdiCreateSolidBrush:
    movl $0x102c, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiD3dContextCreate
_NtGdiD3dContextCreate:
    movl $0x102d, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiD3dContextDestroy
_NtGdiD3dContextDestroy:
    movl $0x102e, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiD3dContextDestroyAll
_NtGdiD3dContextDestroyAll:
    movl $0x102f, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiD3dValidateTextureStageState
_NtGdiD3dValidateTextureStageState:
    movl $0x1030, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiD3dDrawPrimitives2
_NtGdiD3dDrawPrimitives2:
    movl $0x1031, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtGdiDdGetDriverState
_NtGdiDdGetDriverState:
    movl $0x1032, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiDdAddAttachedSurface
_NtGdiDdAddAttachedSurface:
    movl $0x1033, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiDdAlphaBlt
_NtGdiDdAlphaBlt:
    movl $0x1034, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiDdAttachSurface
_NtGdiDdAttachSurface:
    movl $0x1035, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdBeginMoCompFrame
_NtGdiDdBeginMoCompFrame:
    movl $0x1036, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdBlt
_NtGdiDdBlt:
    movl $0x1037, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiDdCanCreateSurface
_NtGdiDdCanCreateSurface:
    movl $0x1038, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdCanCreateD3DBuffer
_NtGdiDdCanCreateD3DBuffer:
    movl $0x1039, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdColorControl
_NtGdiDdColorControl:
    movl $0x103a, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdCreateDirectDrawObject
_NtGdiDdCreateDirectDrawObject:
    movl $0x103b, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiDdCreateSurface
_NtGdiDdCreateSurface:
    movl $0x103c, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtGdiDdCreateD3DBuffer
_NtGdiDdCreateD3DBuffer:
    movl $0x103d, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtGdiDdCreateMoComp
_NtGdiDdCreateMoComp:
    movl $0x103e, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdCreateSurfaceObject
_NtGdiDdCreateSurfaceObject:
    movl $0x103f, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiDdDeleteDirectDrawObject
_NtGdiDdDeleteDirectDrawObject:
    movl $0x1040, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiDdDeleteSurfaceObject
_NtGdiDdDeleteSurfaceObject:
    movl $0x1041, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiDdDestroyMoComp
_NtGdiDdDestroyMoComp:
    movl $0x1042, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdDestroySurface
_NtGdiDdDestroySurface:
    movl $0x1043, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdDestroyD3DBuffer
_NtGdiDdDestroyD3DBuffer:
    movl $0x1044, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiDdEndMoCompFrame
_NtGdiDdEndMoCompFrame:
    movl $0x1045, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdFlip
_NtGdiDdFlip:
    movl $0x1046, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiDdFlipToGDISurface
_NtGdiDdFlipToGDISurface:
    movl $0x1047, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdGetAvailDriverMemory
_NtGdiDdGetAvailDriverMemory:
    movl $0x1048, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdGetBltStatus
_NtGdiDdGetBltStatus:
    movl $0x1049, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdGetDC
_NtGdiDdGetDC:
    movl $0x104a, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdGetDriverInfo
_NtGdiDdGetDriverInfo:
    movl $0x104b, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdGetDxHandle
_NtGdiDdGetDxHandle:
    movl $0x104c, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiDdGetFlipStatus
_NtGdiDdGetFlipStatus:
    movl $0x104d, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdGetInternalMoCompInfo
_NtGdiDdGetInternalMoCompInfo:
    movl $0x104e, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdGetMoCompBuffInfo
_NtGdiDdGetMoCompBuffInfo:
    movl $0x104f, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdGetMoCompGuids
_NtGdiDdGetMoCompGuids:
    movl $0x1050, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdGetMoCompFormats
_NtGdiDdGetMoCompFormats:
    movl $0x1051, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdGetScanLine
_NtGdiDdGetScanLine:
    movl $0x1052, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdLock
_NtGdiDdLock:
    movl $0x1053, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiDdLockD3D
_NtGdiDdLockD3D:
    movl $0x1054, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdQueryDirectDrawObject
_NtGdiDdQueryDirectDrawObject:
    movl $0x1055, %eax
    movq %rcx, %r10
    syscall
    ret $0x2c

.global _NtGdiDdQueryMoCompStatus
_NtGdiDdQueryMoCompStatus:
    movl $0x1056, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdReenableDirectDrawObject
_NtGdiDdReenableDirectDrawObject:
    movl $0x1057, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdReleaseDC
_NtGdiDdReleaseDC:
    movl $0x1058, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiDdRenderMoComp
_NtGdiDdRenderMoComp:
    movl $0x1059, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdResetVisrgn
_NtGdiDdResetVisrgn:
    movl $0x105a, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdSetColorKey
_NtGdiDdSetColorKey:
    movl $0x105b, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdSetExclusiveMode
_NtGdiDdSetExclusiveMode:
    movl $0x105c, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdSetGammaRamp
_NtGdiDdSetGammaRamp:
    movl $0x105d, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiDdCreateSurfaceEx
_NtGdiDdCreateSurfaceEx:
    movl $0x105e, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiDdSetOverlayPosition
_NtGdiDdSetOverlayPosition:
    movl $0x105f, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiDdUnattachSurface
_NtGdiDdUnattachSurface:
    movl $0x1060, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdUnlock
_NtGdiDdUnlock:
    movl $0x1061, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdUnlockD3D
_NtGdiDdUnlockD3D:
    movl $0x1062, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDdUpdateOverlay
_NtGdiDdUpdateOverlay:
    movl $0x1063, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiDdWaitForVerticalBlank
_NtGdiDdWaitForVerticalBlank:
    movl $0x1064, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpCanCreateVideoPort
_NtGdiDvpCanCreateVideoPort:
    movl $0x1065, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpColorControl
_NtGdiDvpColorControl:
    movl $0x1066, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpCreateVideoPort
_NtGdiDvpCreateVideoPort:
    movl $0x1067, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpDestroyVideoPort
_NtGdiDvpDestroyVideoPort:
    movl $0x1068, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpFlipVideoPort
_NtGdiDvpFlipVideoPort:
    movl $0x1069, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiDvpGetVideoPortBandwidth
_NtGdiDvpGetVideoPortBandwidth:
    movl $0x106a, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpGetVideoPortField
_NtGdiDvpGetVideoPortField:
    movl $0x106b, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpGetVideoPortFlipStatus
_NtGdiDvpGetVideoPortFlipStatus:
    movl $0x106c, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpGetVideoPortInputFormats
_NtGdiDvpGetVideoPortInputFormats:
    movl $0x106d, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpGetVideoPortLine
_NtGdiDvpGetVideoPortLine:
    movl $0x106e, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpGetVideoPortOutputFormats
_NtGdiDvpGetVideoPortOutputFormats:
    movl $0x106f, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpGetVideoPortConnectInfo
_NtGdiDvpGetVideoPortConnectInfo:
    movl $0x1070, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpGetVideoSignalStatus
_NtGdiDvpGetVideoSignalStatus:
    movl $0x1071, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpUpdateVideoPort
_NtGdiDvpUpdateVideoPort:
    movl $0x1072, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiDvpWaitForVideoPortSync
_NtGdiDvpWaitForVideoPortSync:
    movl $0x1073, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDvpAcquireNotification
_NtGdiDvpAcquireNotification:
    movl $0x1074, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiDvpReleaseNotification
_NtGdiDvpReleaseNotification:
    movl $0x1075, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDxgGenericThunk
_NtGdiDxgGenericThunk:
    movl $0x1076, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiDeleteClientObj
_NtGdiDeleteClientObj:
    movl $0x1077, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiDeleteColorSpace
_NtGdiDeleteColorSpace:
    movl $0x1078, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiDeleteColorTransform
_NtGdiDeleteColorTransform:
    movl $0x1079, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDeleteObjectApp
_NtGdiDeleteObjectApp:
    movl $0x107a, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiDescribePixelFormat
_NtGdiDescribePixelFormat:
    movl $0x107b, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiGetPerBandInfo
_NtGdiGetPerBandInfo:
    movl $0x107c, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiDoBanding
_NtGdiDoBanding:
    movl $0x107d, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiDoPalette
_NtGdiDoPalette:
    movl $0x107e, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiDrawEscape
_NtGdiDrawEscape:
    movl $0x107f, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiEllipse
_NtGdiEllipse:
    movl $0x1080, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiEnableEudc
_NtGdiEnableEudc:
    movl $0x1081, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEndDoc
_NtGdiEndDoc:
    movl $0x1082, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEndPage
_NtGdiEndPage:
    movl $0x1083, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEndPath
_NtGdiEndPath:
    movl $0x1084, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEnumFontChunk
_NtGdiEnumFontChunk:
    movl $0x1085, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiEnumFontClose
_NtGdiEnumFontClose:
    movl $0x1086, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEnumFontOpen
_NtGdiEnumFontOpen:
    movl $0x1087, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtGdiEnumObjects
_NtGdiEnumObjects:
    movl $0x1088, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiEqualRgn
_NtGdiEqualRgn:
    movl $0x1089, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiEudcLoadUnloadLink
_NtGdiEudcLoadUnloadLink:
    movl $0x108a, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtGdiExcludeClipRect
_NtGdiExcludeClipRect:
    movl $0x108b, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiExtCreatePen
_NtGdiExtCreatePen:
    movl $0x108c, %eax
    movq %rcx, %r10
    syscall
    ret $0x2c

.global _NtGdiExtCreateRegion
_NtGdiExtCreateRegion:
    movl $0x108d, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiExtEscape
_NtGdiExtEscape:
    movl $0x108e, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtGdiExtFloodFill
_NtGdiExtFloodFill:
    movl $0x108f, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiExtGetObjectW
_NtGdiExtGetObjectW:
    movl $0x1090, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiExtSelectClipRgn
_NtGdiExtSelectClipRgn:
    movl $0x1091, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiExtTextOutW
_NtGdiExtTextOutW:
    movl $0x1092, %eax
    movq %rcx, %r10
    syscall
    ret $0x24

.global _NtGdiFillPath
_NtGdiFillPath:
    movl $0x1093, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiFillRgn
_NtGdiFillRgn:
    movl $0x1094, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiFlattenPath
_NtGdiFlattenPath:
    movl $0x1095, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiFlushUserBatch
_NtGdiFlushUserBatch:
    movl $0x1096, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtGdiFlush
_NtGdiFlush:
    movl $0x1097, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtGdiForceUFIMapping
_NtGdiForceUFIMapping:
    movl $0x1098, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiFrameRgn
_NtGdiFrameRgn:
    movl $0x1099, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiFullscreenControl
_NtGdiFullscreenControl:
    movl $0x109a, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiGetAndSetDCDword
_NtGdiGetAndSetDCDword:
    movl $0x109b, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiGetAppClipBox
_NtGdiGetAppClipBox:
    movl $0x109c, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetBitmapBits
_NtGdiGetBitmapBits:
    movl $0x109d, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetBitmapDimension
_NtGdiGetBitmapDimension:
    movl $0x109e, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetBoundsRect
_NtGdiGetBoundsRect:
    movl $0x109f, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetCharABCWidthsW
_NtGdiGetCharABCWidthsW:
    movl $0x10a0, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiGetCharacterPlacementW
_NtGdiGetCharacterPlacementW:
    movl $0x10a1, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiGetCharSet
_NtGdiGetCharSet:
    movl $0x10a2, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiGetCharWidthW
_NtGdiGetCharWidthW:
    movl $0x10a3, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiGetCharWidthInfo
_NtGdiGetCharWidthInfo:
    movl $0x10a4, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetColorAdjustment
_NtGdiGetColorAdjustment:
    movl $0x10a5, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetColorSpaceforBitmap
_NtGdiGetColorSpaceforBitmap:
    movl $0x10a6, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiGetDCDword
_NtGdiGetDCDword:
    movl $0x10a7, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetDCforBitmap
_NtGdiGetDCforBitmap:
    movl $0x10a8, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiGetDCObject
_NtGdiGetDCObject:
    movl $0x10a9, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetDCPoint
_NtGdiGetDCPoint:
    movl $0x10aa, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetDeviceCaps
_NtGdiGetDeviceCaps:
    movl $0x10ab, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetDeviceGammaRamp
_NtGdiGetDeviceGammaRamp:
    movl $0x10ac, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetDeviceCapsAll
_NtGdiGetDeviceCapsAll:
    movl $0x10ad, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetDIBitsInternal
_NtGdiGetDIBitsInternal:
    movl $0x10ae, %eax
    movq %rcx, %r10
    syscall
    ret $0x24

.global _NtGdiGetETM
_NtGdiGetETM:
    movl $0x10af, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetEudcTimeStampEx
_NtGdiGetEudcTimeStampEx:
    movl $0x10b0, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetFontData
_NtGdiGetFontData:
    movl $0x10b1, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiGetFontResourceInfoInternalW
_NtGdiGetFontResourceInfoInternalW:
    movl $0x10b2, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtGdiGetGlyphIndicesW
_NtGdiGetGlyphIndicesW:
    movl $0x10b3, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiGetGlyphIndicesWInternal
_NtGdiGetGlyphIndicesWInternal:
    movl $0x10b4, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiGetGlyphOutline
_NtGdiGetGlyphOutline:
    movl $0x10b5, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtGdiGetKerningPairs
_NtGdiGetKerningPairs:
    movl $0x10b6, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetLinkedUFIs
_NtGdiGetLinkedUFIs:
    movl $0x10b7, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetMiterLimit
_NtGdiGetMiterLimit:
    movl $0x10b8, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetMonitorID
_NtGdiGetMonitorID:
    movl $0x10b9, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetNearestColor
_NtGdiGetNearestColor:
    movl $0x10ba, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetNearestPaletteIndex
_NtGdiGetNearestPaletteIndex:
    movl $0x10bb, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetObjectBitmapHandle
_NtGdiGetObjectBitmapHandle:
    movl $0x10bc, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetOutlineTextMetricsInternalW
_NtGdiGetOutlineTextMetricsInternalW:
    movl $0x10bd, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiGetPath
_NtGdiGetPath:
    movl $0x10be, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiGetPixel
_NtGdiGetPixel:
    movl $0x10bf, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetRandomRgn
_NtGdiGetRandomRgn:
    movl $0x10c0, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetRasterizerCaps
_NtGdiGetRasterizerCaps:
    movl $0x10c1, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetRealizationInfo
_NtGdiGetRealizationInfo:
    movl $0x10c2, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetRegionData
_NtGdiGetRegionData:
    movl $0x10c3, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetRgnBox
_NtGdiGetRgnBox:
    movl $0x10c4, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetServerMetaFileBits
_NtGdiGetServerMetaFileBits:
    movl $0x10c5, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtGdiGetSpoolMessage
_NtGdiGetSpoolMessage:
    movl $0x10c6, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiGetStats
_NtGdiGetStats:
    movl $0x10c7, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiGetStockObject
_NtGdiGetStockObject:
    movl $0x10c8, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiGetStringBitmapW
_NtGdiGetStringBitmapW:
    movl $0x10c9, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiGetSystemPaletteUse
_NtGdiGetSystemPaletteUse:
    movl $0x10ca, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiGetTextCharsetInfo
_NtGdiGetTextCharsetInfo:
    movl $0x10cb, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetTextExtent
_NtGdiGetTextExtent:
    movl $0x10cc, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiGetTextExtentExW
_NtGdiGetTextExtentExW:
    movl $0x10cd, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtGdiGetTextFaceW
_NtGdiGetTextFaceW:
    movl $0x10ce, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiGetTextMetricsW
_NtGdiGetTextMetricsW:
    movl $0x10cf, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetTransform
_NtGdiGetTransform:
    movl $0x10d0, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetUFI
_NtGdiGetUFI:
    movl $0x10d1, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiGetEmbUFI
_NtGdiGetEmbUFI:
    movl $0x10d2, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtGdiGetUFIPathname
_NtGdiGetUFIPathname:
    movl $0x10d3, %eax
    movq %rcx, %r10
    syscall
    ret $0x28

.global _NtGdiGetEmbedFonts
_NtGdiGetEmbedFonts:
    movl $0x10d4, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtGdiChangeGhostFont
_NtGdiChangeGhostFont:
    movl $0x10d5, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiAddEmbFontToDC
_NtGdiAddEmbFontToDC:
    movl $0x10d6, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetFontUnicodeRanges
_NtGdiGetFontUnicodeRanges:
    movl $0x10d7, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiGetWidthTable
_NtGdiGetWidthTable:
    movl $0x10d8, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtGdiGradientFill
_NtGdiGradientFill:
    movl $0x10d9, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiHfontCreate
_NtGdiHfontCreate:
    movl $0x10da, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiIcmBrushInfo
_NtGdiIcmBrushInfo:
    movl $0x10db, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtGdiInit
_NtGdiInit:
    movl $0x10dc, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtGdiInitSpool
_NtGdiInitSpool:
    movl $0x10dd, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtGdiIntersectClipRect
_NtGdiIntersectClipRect:
    movl $0x10de, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiInvertRgn
_NtGdiInvertRgn:
    movl $0x10df, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiLineTo
_NtGdiLineTo:
    movl $0x10e0, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiMakeFontDir
_NtGdiMakeFontDir:
    movl $0x10e1, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiMakeInfoDC
_NtGdiMakeInfoDC:
    movl $0x10e2, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiMaskBlt
_NtGdiMaskBlt:
    movl $0x10e3, %eax
    movq %rcx, %r10
    syscall
    ret $0x34

.global _NtGdiModifyWorldTransform
_NtGdiModifyWorldTransform:
    movl $0x10e4, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiMonoBitmap
_NtGdiMonoBitmap:
    movl $0x10e5, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiMoveTo
_NtGdiMoveTo:
    movl $0x10e6, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiOffsetClipRgn
_NtGdiOffsetClipRgn:
    movl $0x10e7, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiOffsetRgn
_NtGdiOffsetRgn:
    movl $0x10e8, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiOpenDCW
_NtGdiOpenDCW:
    movl $0x10e9, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtGdiPatBlt
_NtGdiPatBlt:
    movl $0x10ea, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiPolyPatBlt
_NtGdiPolyPatBlt:
    movl $0x10eb, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiPathToRegion
_NtGdiPathToRegion:
    movl $0x10ec, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiPlgBlt
_NtGdiPlgBlt:
    movl $0x10ed, %eax
    movq %rcx, %r10
    syscall
    ret $0x2c

.global _NtGdiPolyDraw
_NtGdiPolyDraw:
    movl $0x10ee, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiPolyPolyDraw
_NtGdiPolyPolyDraw:
    movl $0x10ef, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiPolyTextOutW
_NtGdiPolyTextOutW:
    movl $0x10f0, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiPtInRegion
_NtGdiPtInRegion:
    movl $0x10f1, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiPtVisible
_NtGdiPtVisible:
    movl $0x10f2, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiQueryFonts
_NtGdiQueryFonts:
    movl $0x10f3, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiQueryFontAssocInfo
_NtGdiQueryFontAssocInfo:
    movl $0x10f4, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiRectangle
_NtGdiRectangle:
    movl $0x10f5, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiRectInRegion
_NtGdiRectInRegion:
    movl $0x10f6, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiRectVisible
_NtGdiRectVisible:
    movl $0x10f7, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiRemoveFontResourceW
_NtGdiRemoveFontResourceW:
    movl $0x10f8, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiRemoveFontMemResourceEx
_NtGdiRemoveFontMemResourceEx:
    movl $0x10f9, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiResetDC
_NtGdiResetDC:
    movl $0x10fa, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiResizePalette
_NtGdiResizePalette:
    movl $0x10fb, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiRestoreDC
_NtGdiRestoreDC:
    movl $0x10fc, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiRoundRect
_NtGdiRoundRect:
    movl $0x10fd, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtGdiSaveDC
_NtGdiSaveDC:
    movl $0x10fe, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiScaleViewportExtEx
_NtGdiScaleViewportExtEx:
    movl $0x10ff, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiScaleWindowExtEx
_NtGdiScaleWindowExtEx:
    movl $0x1100, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiSelectBitmap
_NtGdiSelectBitmap:
    movl $0x1101, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSelectBrush
_NtGdiSelectBrush:
    movl $0x1102, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSelectClipPath
_NtGdiSelectClipPath:
    movl $0x1103, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSelectFont
_NtGdiSelectFont:
    movl $0x1104, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSelectPen
_NtGdiSelectPen:
    movl $0x1105, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSetBitmapAttributes
_NtGdiSetBitmapAttributes:
    movl $0x1106, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSetBitmapBits
_NtGdiSetBitmapBits:
    movl $0x1107, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiSetBitmapDimension
_NtGdiSetBitmapDimension:
    movl $0x1108, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiSetBoundsRect
_NtGdiSetBoundsRect:
    movl $0x1109, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiSetBrushAttributes
_NtGdiSetBrushAttributes:
    movl $0x110a, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSetBrushOrg
_NtGdiSetBrushOrg:
    movl $0x110b, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiSetColorAdjustment
_NtGdiSetColorAdjustment:
    movl $0x110c, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSetColorSpace
_NtGdiSetColorSpace:
    movl $0x110d, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSetDeviceGammaRamp
_NtGdiSetDeviceGammaRamp:
    movl $0x110e, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSetDIBitsToDeviceInternal
_NtGdiSetDIBitsToDeviceInternal:
    movl $0x110f, %eax
    movq %rcx, %r10
    syscall
    ret $0x40

.global _NtGdiSetFontEnumeration
_NtGdiSetFontEnumeration:
    movl $0x1110, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiSetFontXform
_NtGdiSetFontXform:
    movl $0x1111, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiSetIcmMode
_NtGdiSetIcmMode:
    movl $0x1112, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiSetLinkedUFIs
_NtGdiSetLinkedUFIs:
    movl $0x1113, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiSetMagicColors
_NtGdiSetMagicColors:
    movl $0x1114, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiSetMetaRgn
_NtGdiSetMetaRgn:
    movl $0x1115, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiSetMiterLimit
_NtGdiSetMiterLimit:
    movl $0x1116, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetDeviceWidth
_NtGdiGetDeviceWidth:
    movl $0x1117, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiMirrorWindowOrg
_NtGdiMirrorWindowOrg:
    movl $0x1118, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiSetLayout
_NtGdiSetLayout:
    movl $0x1119, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiSetPixel
_NtGdiSetPixel:
    movl $0x111a, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiSetPixelFormat
_NtGdiSetPixelFormat:
    movl $0x111b, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSetRectRgn
_NtGdiSetRectRgn:
    movl $0x111c, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiSetSystemPaletteUse
_NtGdiSetSystemPaletteUse:
    movl $0x111d, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSetTextJustification
_NtGdiSetTextJustification:
    movl $0x111e, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiSetupPublicCFONT
_NtGdiSetupPublicCFONT:
    movl $0x111f, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiSetVirtualResolution
_NtGdiSetVirtualResolution:
    movl $0x1120, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiSetSizeDevice
_NtGdiSetSizeDevice:
    movl $0x1121, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiStartDoc
_NtGdiStartDoc:
    movl $0x1122, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiStartPage
_NtGdiStartPage:
    movl $0x1123, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiStretchBlt
_NtGdiStretchBlt:
    movl $0x1124, %eax
    movq %rcx, %r10
    syscall
    ret $0x30

.global _NtGdiStretchDIBitsInternal
_NtGdiStretchDIBitsInternal:
    movl $0x1125, %eax
    movq %rcx, %r10
    syscall
    ret $0x40

.global _NtGdiStrokeAndFillPath
_NtGdiStrokeAndFillPath:
    movl $0x1126, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiStrokePath
_NtGdiStrokePath:
    movl $0x1127, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiSwapBuffers
_NtGdiSwapBuffers:
    movl $0x1128, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiTransformPoints
_NtGdiTransformPoints:
    movl $0x1129, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiTransparentBlt
_NtGdiTransparentBlt:
    movl $0x112a, %eax
    movq %rcx, %r10
    syscall
    ret $0x2c

.global _NtGdiUnloadPrinterDriver
_NtGdiUnloadPrinterDriver:
    movl $0x112b, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiUnmapMemFont
_NtGdiUnmapMemFont:
    movl $0x112c, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiUnrealizeObject
_NtGdiUnrealizeObject:
    movl $0x112d, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiUpdateColors
_NtGdiUpdateColors:
    movl $0x112e, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiWidenPath
_NtGdiWidenPath:
    movl $0x112f, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserActivateKeyboardLayout
_NtUserActivateKeyboardLayout:
    movl $0x1130, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserAlterWindowStyle
_NtUserAlterWindowStyle:
    movl $0x1131, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserAssociateInputContext
_NtUserAssociateInputContext:
    movl $0x1132, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserAttachThreadInput
_NtUserAttachThreadInput:
    movl $0x1133, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserBeginPaint
_NtUserBeginPaint:
    movl $0x1134, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserBitBltSysBmp
_NtUserBitBltSysBmp:
    movl $0x1135, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtUserBlockInput
_NtUserBlockInput:
    movl $0x1136, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserBuildHimcList
_NtUserBuildHimcList:
    movl $0x1137, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserBuildHwndList
_NtUserBuildHwndList:
    movl $0x1138, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtUserBuildNameList
_NtUserBuildNameList:
    movl $0x1139, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserBuildPropList
_NtUserBuildPropList:
    movl $0x113a, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserCallHwnd
_NtUserCallHwnd:
    movl $0x113b, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserCallHwndLock
_NtUserCallHwndLock:
    movl $0x113c, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserCallHwndOpt
_NtUserCallHwndOpt:
    movl $0x113d, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserCallHwndParam
_NtUserCallHwndParam:
    movl $0x113e, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserCallHwndParamLock
_NtUserCallHwndParamLock:
    movl $0x113f, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserCallMsgFilter
_NtUserCallMsgFilter:
    movl $0x1140, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserCallNextHookEx
_NtUserCallNextHookEx:
    movl $0x1141, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserCallNoParam
_NtUserCallNoParam:
    movl $0x1142, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserCallOneParam
_NtUserCallOneParam:
    movl $0x1143, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserCallTwoParam
_NtUserCallTwoParam:
    movl $0x1144, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserChangeClipboardChain
_NtUserChangeClipboardChain:
    movl $0x1145, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserChangeDisplaySettings
_NtUserChangeDisplaySettings:
    movl $0x1146, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserCheckImeHotKey
_NtUserCheckImeHotKey:
    movl $0x1147, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserCheckMenuItem
_NtUserCheckMenuItem:
    movl $0x1148, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserChildWindowFromPointEx
_NtUserChildWindowFromPointEx:
    movl $0x1149, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserClipCursor
_NtUserClipCursor:
    movl $0x114a, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserCloseClipboard
_NtUserCloseClipboard:
    movl $0x114b, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserCloseDesktop
_NtUserCloseDesktop:
    movl $0x114c, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserCloseWindowStation
_NtUserCloseWindowStation:
    movl $0x114d, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserConsoleControl
_NtUserConsoleControl:
    movl $0x114e, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserConvertMemHandle
_NtUserConvertMemHandle:
    movl $0x114f, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserCopyAcceleratorTable
_NtUserCopyAcceleratorTable:
    movl $0x1150, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserCountClipboardFormats
_NtUserCountClipboardFormats:
    movl $0x1151, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserCreateAcceleratorTable
_NtUserCreateAcceleratorTable:
    movl $0x1152, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserCreateCaret
_NtUserCreateCaret:
    movl $0x1153, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserCreateDesktop
_NtUserCreateDesktop:
    movl $0x1154, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserCreateInputContext
_NtUserCreateInputContext:
    movl $0x1155, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserCreateLocalMemHandle
_NtUserCreateLocalMemHandle:
    movl $0x1156, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserCreateWindowEx
_NtUserCreateWindowEx:
    movl $0x1157, %eax
    movq %rcx, %r10
    syscall
    ret $0x3c

.global _NtUserCreateWindowStation
_NtUserCreateWindowStation:
    movl $0x1158, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtUserDdeGetQualityOfService
_NtUserDdeGetQualityOfService:
    movl $0x1159, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserDdeInitialize
_NtUserDdeInitialize:
    movl $0x115a, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserDdeSetQualityOfService
_NtUserDdeSetQualityOfService:
    movl $0x115b, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserDeferWindowPos
_NtUserDeferWindowPos:
    movl $0x115c, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtUserDefSetText
_NtUserDefSetText:
    movl $0x115d, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserDeleteMenu
_NtUserDeleteMenu:
    movl $0x115e, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserDestroyAcceleratorTable
_NtUserDestroyAcceleratorTable:
    movl $0x115f, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserDestroyCursor
_NtUserDestroyCursor:
    movl $0x1160, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserDestroyInputContext
_NtUserDestroyInputContext:
    movl $0x1161, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserDestroyMenu
_NtUserDestroyMenu:
    movl $0x1162, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserDestroyWindow
_NtUserDestroyWindow:
    movl $0x1163, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserDisableThreadIme
_NtUserDisableThreadIme:
    movl $0x1164, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserDispatchMessage
_NtUserDispatchMessage:
    movl $0x1165, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserDragDetect
_NtUserDragDetect:
    movl $0x1166, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserDragObject
_NtUserDragObject:
    movl $0x1167, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserDrawAnimatedRects
_NtUserDrawAnimatedRects:
    movl $0x1168, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserDrawCaption
_NtUserDrawCaption:
    movl $0x1169, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserDrawCaptionTemp
_NtUserDrawCaptionTemp:
    movl $0x116a, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtUserDrawIconEx
_NtUserDrawIconEx:
    movl $0x116b, %eax
    movq %rcx, %r10
    syscall
    ret $0x2c

.global _NtUserDrawMenuBarTemp
_NtUserDrawMenuBarTemp:
    movl $0x116c, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserEmptyClipboard
_NtUserEmptyClipboard:
    movl $0x116d, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserEnableMenuItem
_NtUserEnableMenuItem:
    movl $0x116e, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserEnableScrollBar
_NtUserEnableScrollBar:
    movl $0x116f, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserEndDeferWindowPosEx
_NtUserEndDeferWindowPosEx:
    movl $0x1170, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserEndMenu
_NtUserEndMenu:
    movl $0x1171, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserEndPaint
_NtUserEndPaint:
    movl $0x1172, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserEnumDisplayDevices
_NtUserEnumDisplayDevices:
    movl $0x1173, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserEnumDisplayMonitors
_NtUserEnumDisplayMonitors:
    movl $0x1174, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserEnumDisplaySettings
_NtUserEnumDisplaySettings:
    movl $0x1175, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserEvent
_NtUserEvent:
    movl $0x1176, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserExcludeUpdateRgn
_NtUserExcludeUpdateRgn:
    movl $0x1177, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserFillWindow
_NtUserFillWindow:
    movl $0x1178, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserFindExistingCursorIcon
_NtUserFindExistingCursorIcon:
    movl $0x1179, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserFindWindowEx
_NtUserFindWindowEx:
    movl $0x117a, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserFlashWindowEx
_NtUserFlashWindowEx:
    movl $0x117b, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetAltTabInfo
_NtUserGetAltTabInfo:
    movl $0x117c, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtUserGetAncestor
_NtUserGetAncestor:
    movl $0x117d, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetAppImeLevel
_NtUserGetAppImeLevel:
    movl $0x117e, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetAsyncKeyState
_NtUserGetAsyncKeyState:
    movl $0x117f, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetAtomName
_NtUserGetAtomName:
    movl $0x1180, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetCaretBlinkTime
_NtUserGetCaretBlinkTime:
    movl $0x1181, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserGetCaretPos
_NtUserGetCaretPos:
    movl $0x1182, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetClassInfo
_NtUserGetClassInfo:
    movl $0x1183, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserGetClassName
_NtUserGetClassName:
    movl $0x1184, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetClipboardData
_NtUserGetClipboardData:
    movl $0x1185, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetClipboardFormatName
_NtUserGetClipboardFormatName:
    movl $0x1186, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetClipboardOwner
_NtUserGetClipboardOwner:
    movl $0x1187, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserGetClipboardSequenceNumber
_NtUserGetClipboardSequenceNumber:
    movl $0x1188, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserGetClipboardViewer
_NtUserGetClipboardViewer:
    movl $0x1189, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserGetClipCursor
_NtUserGetClipCursor:
    movl $0x118a, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetComboBoxInfo
_NtUserGetComboBoxInfo:
    movl $0x118b, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetControlBrush
_NtUserGetControlBrush:
    movl $0x118c, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetControlColor
_NtUserGetControlColor:
    movl $0x118d, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserGetCPD
_NtUserGetCPD:
    movl $0x118e, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetCursorFrameInfo
_NtUserGetCursorFrameInfo:
    movl $0x118f, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserGetCursorInfo
_NtUserGetCursorInfo:
    movl $0x1190, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetDC
_NtUserGetDC:
    movl $0x1191, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetDCEx
_NtUserGetDCEx:
    movl $0x1192, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetDoubleClickTime
_NtUserGetDoubleClickTime:
    movl $0x1193, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserGetForegroundWindow
_NtUserGetForegroundWindow:
    movl $0x1194, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserGetGuiResources
_NtUserGetGuiResources:
    movl $0x1195, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetGUIThreadInfo
_NtUserGetGUIThreadInfo:
    movl $0x1196, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetIconInfo
_NtUserGetIconInfo:
    movl $0x1197, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtUserGetIconSize
_NtUserGetIconSize:
    movl $0x1198, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserGetImeHotKey
_NtUserGetImeHotKey:
    movl $0x1199, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserGetImeInfoEx
_NtUserGetImeInfoEx:
    movl $0x119a, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetInternalWindowPos
_NtUserGetInternalWindowPos:
    movl $0x119b, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetKeyboardLayoutList
_NtUserGetKeyboardLayoutList:
    movl $0x119c, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetKeyboardLayoutName
_NtUserGetKeyboardLayoutName:
    movl $0x119d, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetKeyboardState
_NtUserGetKeyboardState:
    movl $0x119e, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetKeyNameText
_NtUserGetKeyNameText:
    movl $0x119f, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetKeyState
_NtUserGetKeyState:
    movl $0x11a0, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetListBoxInfo
_NtUserGetListBoxInfo:
    movl $0x11a1, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetMenuBarInfo
_NtUserGetMenuBarInfo:
    movl $0x11a2, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserGetMenuIndex
_NtUserGetMenuIndex:
    movl $0x11a3, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetMenuItemRect
_NtUserGetMenuItemRect:
    movl $0x11a4, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserGetMessage
_NtUserGetMessage:
    movl $0x11a5, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserGetMouseMovePointsEx
_NtUserGetMouseMovePointsEx:
    movl $0x11a6, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserGetObjectInformation
_NtUserGetObjectInformation:
    movl $0x11a7, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserGetOpenClipboardWindow
_NtUserGetOpenClipboardWindow:
    movl $0x11a8, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserGetPriorityClipboardFormat
_NtUserGetPriorityClipboardFormat:
    movl $0x11a9, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetProcessWindowStation
_NtUserGetProcessWindowStation:
    movl $0x11aa, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserGetRawInputBuffer
_NtUserGetRawInputBuffer:
    movl $0x11ab, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetRawInputData
_NtUserGetRawInputData:
    movl $0x11ac, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserGetRawInputDeviceInfo
_NtUserGetRawInputDeviceInfo:
    movl $0x11ad, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserGetRawInputDeviceList
_NtUserGetRawInputDeviceList:
    movl $0x11ae, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetRegisteredRawInputDevices
_NtUserGetRegisteredRawInputDevices:
    movl $0x11af, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetScrollBarInfo
_NtUserGetScrollBarInfo:
    movl $0x11b0, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetSystemMenu
_NtUserGetSystemMenu:
    movl $0x11b1, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetThreadDesktop
_NtUserGetThreadDesktop:
    movl $0x11b2, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetThreadState
_NtUserGetThreadState:
    movl $0x11b3, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetTitleBarInfo
_NtUserGetTitleBarInfo:
    movl $0x11b4, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetUpdateRect
_NtUserGetUpdateRect:
    movl $0x11b5, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetUpdateRgn
_NtUserGetUpdateRgn:
    movl $0x11b6, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetWindowDC
_NtUserGetWindowDC:
    movl $0x11b7, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetWindowPlacement
_NtUserGetWindowPlacement:
    movl $0x11b8, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetWOWClass
_NtUserGetWOWClass:
    movl $0x11b9, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserHardErrorControl
_NtUserHardErrorControl:
    movl $0x11ba, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserHideCaret
_NtUserHideCaret:
    movl $0x11bb, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserHiliteMenuItem
_NtUserHiliteMenuItem:
    movl $0x11bc, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserImpersonateDdeClientWindow
_NtUserImpersonateDdeClientWindow:
    movl $0x11bd, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserInitialize
_NtUserInitialize:
    movl $0x11be, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserInitializeClientPfnArrays
_NtUserInitializeClientPfnArrays:
    movl $0x11bf, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserInitTask
_NtUserInitTask:
    movl $0x11c0, %eax
    movq %rcx, %r10
    syscall
    ret $0x30

.global _NtUserInternalGetWindowText
_NtUserInternalGetWindowText:
    movl $0x11c1, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserInvalidateRect
_NtUserInvalidateRect:
    movl $0x11c2, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserInvalidateRgn
_NtUserInvalidateRgn:
    movl $0x11c3, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserIsClipboardFormatAvailable
_NtUserIsClipboardFormatAvailable:
    movl $0x11c4, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserKillTimer
_NtUserKillTimer:
    movl $0x11c5, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserLoadKeyboardLayoutEx
_NtUserLoadKeyboardLayoutEx:
    movl $0x11c6, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtUserLockWindowStation
_NtUserLockWindowStation:
    movl $0x11c7, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserLockWindowUpdate
_NtUserLockWindowUpdate:
    movl $0x11c8, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserLockWorkStation
_NtUserLockWorkStation:
    movl $0x11c9, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserMapVirtualKeyEx
_NtUserMapVirtualKeyEx:
    movl $0x11ca, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserMenuItemFromPoint
_NtUserMenuItemFromPoint:
    movl $0x11cb, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserMessageCall
_NtUserMessageCall:
    movl $0x11cc, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtUserMinMaximize
_NtUserMinMaximize:
    movl $0x11cd, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserMNDragLeave
_NtUserMNDragLeave:
    movl $0x11ce, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserMNDragOver
_NtUserMNDragOver:
    movl $0x11cf, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserModifyUserStartupInfoFlags
_NtUserModifyUserStartupInfoFlags:
    movl $0x11d0, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserMoveWindow
_NtUserMoveWindow:
    movl $0x11d1, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtUserNotifyIMEStatus
_NtUserNotifyIMEStatus:
    movl $0x11d2, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserNotifyProcessCreate
_NtUserNotifyProcessCreate:
    movl $0x11d3, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserNotifyWinEvent
_NtUserNotifyWinEvent:
    movl $0x11d4, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserOpenClipboard
_NtUserOpenClipboard:
    movl $0x11d5, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserOpenDesktop
_NtUserOpenDesktop:
    movl $0x11d6, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserOpenInputDesktop
_NtUserOpenInputDesktop:
    movl $0x11d7, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserOpenWindowStation
_NtUserOpenWindowStation:
    movl $0x11d8, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserPaintDesktop
_NtUserPaintDesktop:
    movl $0x11d9, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserPeekMessage
_NtUserPeekMessage:
    movl $0x11da, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserPostMessage
_NtUserPostMessage:
    movl $0x11db, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserPostThreadMessage
_NtUserPostThreadMessage:
    movl $0x11dc, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserPrintWindow
_NtUserPrintWindow:
    movl $0x11dd, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserProcessConnect
_NtUserProcessConnect:
    movl $0x11de, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserQueryInformationThread
_NtUserQueryInformationThread:
    movl $0x11df, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserQueryInputContext
_NtUserQueryInputContext:
    movl $0x11e0, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserQuerySendMessage
_NtUserQuerySendMessage:
    movl $0x11e1, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserQueryUserCounters
_NtUserQueryUserCounters:
    movl $0x11e2, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserQueryWindow
_NtUserQueryWindow:
    movl $0x11e3, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserRealChildWindowFromPoint
_NtUserRealChildWindowFromPoint:
    movl $0x11e4, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserRealInternalGetMessage
_NtUserRealInternalGetMessage:
    movl $0x11e5, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtUserRealWaitMessageEx
_NtUserRealWaitMessageEx:
    movl $0x11e6, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserRedrawWindow
_NtUserRedrawWindow:
    movl $0x11e7, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserRegisterClassExWOW
_NtUserRegisterClassExWOW:
    movl $0x11e8, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtUserRegisterUserApiHook
_NtUserRegisterUserApiHook:
    movl $0x11e9, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserRegisterHotKey
_NtUserRegisterHotKey:
    movl $0x11ea, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserRegisterRawInputDevices
_NtUserRegisterRawInputDevices:
    movl $0x11eb, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserRegisterTasklist
_NtUserRegisterTasklist:
    movl $0x11ec, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserRegisterWindowMessage
_NtUserRegisterWindowMessage:
    movl $0x11ed, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserRemoveMenu
_NtUserRemoveMenu:
    movl $0x11ee, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserRemoveProp
_NtUserRemoveProp:
    movl $0x11ef, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserResolveDesktop
_NtUserResolveDesktop:
    movl $0x11f0, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserResolveDesktopForWOW
_NtUserResolveDesktopForWOW:
    movl $0x11f1, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSBGetParms
_NtUserSBGetParms:
    movl $0x11f2, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserScrollDC
_NtUserScrollDC:
    movl $0x11f3, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtUserScrollWindowEx
_NtUserScrollWindowEx:
    movl $0x11f4, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtUserSelectPalette
_NtUserSelectPalette:
    movl $0x11f5, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserSendInput
_NtUserSendInput:
    movl $0x11f6, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserSetActiveWindow
_NtUserSetActiveWindow:
    movl $0x11f7, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSetAppImeLevel
_NtUserSetAppImeLevel:
    movl $0x11f8, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetCapture
_NtUserSetCapture:
    movl $0x11f9, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSetClassLong
_NtUserSetClassLong:
    movl $0x11fa, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserSetClassWord
_NtUserSetClassWord:
    movl $0x11fb, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserSetClipboardData
_NtUserSetClipboardData:
    movl $0x11fc, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserSetClipboardViewer
_NtUserSetClipboardViewer:
    movl $0x11fd, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSetConsoleReserveKeys
_NtUserSetConsoleReserveKeys:
    movl $0x11fe, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetCursor
_NtUserSetCursor:
    movl $0x11ff, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSetCursorContents
_NtUserSetCursorContents:
    movl $0x1200, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetCursorIconData
_NtUserSetCursorIconData:
    movl $0x1201, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtUserSetDbgTag
_NtUserSetDbgTag:
    movl $0x1202, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetFocus
_NtUserSetFocus:
    movl $0x1203, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSetImeHotKey
_NtUserSetImeHotKey:
    movl $0x1204, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserSetImeInfoEx
_NtUserSetImeInfoEx:
    movl $0x1205, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSetImeOwnerWindow
_NtUserSetImeOwnerWindow:
    movl $0x1206, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetInformationProcess
_NtUserSetInformationProcess:
    movl $0x1207, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserSetInformationThread
_NtUserSetInformationThread:
    movl $0x1208, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserSetInternalWindowPos
_NtUserSetInternalWindowPos:
    movl $0x1209, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserSetKeyboardState
_NtUserSetKeyboardState:
    movl $0x120a, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSetLogonNotifyWindow
_NtUserSetLogonNotifyWindow:
    movl $0x120b, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSetMenu
_NtUserSetMenu:
    movl $0x120c, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserSetMenuContextHelpId
_NtUserSetMenuContextHelpId:
    movl $0x120d, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetMenuDefaultItem
_NtUserSetMenuDefaultItem:
    movl $0x120e, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserSetMenuFlagRtoL
_NtUserSetMenuFlagRtoL:
    movl $0x120f, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSetObjectInformation
_NtUserSetObjectInformation:
    movl $0x1210, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserSetParent
_NtUserSetParent:
    movl $0x1211, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetProcessWindowStation
_NtUserSetProcessWindowStation:
    movl $0x1212, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSetProp
_NtUserSetProp:
    movl $0x1213, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserSetRipFlags
_NtUserSetRipFlags:
    movl $0x1214, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetScrollInfo
_NtUserSetScrollInfo:
    movl $0x1215, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserSetShellWindowEx
_NtUserSetShellWindowEx:
    movl $0x1216, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetSysColors
_NtUserSetSysColors:
    movl $0x1217, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserSetSystemCursor
_NtUserSetSystemCursor:
    movl $0x1218, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetSystemMenu
_NtUserSetSystemMenu:
    movl $0x1219, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetSystemTimer
_NtUserSetSystemTimer:
    movl $0x121a, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserSetThreadDesktop
_NtUserSetThreadDesktop:
    movl $0x121b, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSetThreadLayoutHandles
_NtUserSetThreadLayoutHandles:
    movl $0x121c, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetThreadState
_NtUserSetThreadState:
    movl $0x121d, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetTimer
_NtUserSetTimer:
    movl $0x121e, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserSetWindowFNID
_NtUserSetWindowFNID:
    movl $0x121f, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetWindowLong
_NtUserSetWindowLong:
    movl $0x1220, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserSetWindowPlacement
_NtUserSetWindowPlacement:
    movl $0x1221, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSetWindowPos
_NtUserSetWindowPos:
    movl $0x1222, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtUserSetWindowRgn
_NtUserSetWindowRgn:
    movl $0x1223, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserSetWindowsHookAW
_NtUserSetWindowsHookAW:
    movl $0x1224, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserSetWindowsHookEx
_NtUserSetWindowsHookEx:
    movl $0x1225, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtUserSetWindowStationUser
_NtUserSetWindowStationUser:
    movl $0x1226, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserSetWindowWord
_NtUserSetWindowWord:
    movl $0x1227, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserSetWinEventHook
_NtUserSetWinEventHook:
    movl $0x1228, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtUserShowCaret
_NtUserShowCaret:
    movl $0x1229, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserShowScrollBar
_NtUserShowScrollBar:
    movl $0x122a, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserShowWindow
_NtUserShowWindow:
    movl $0x122b, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserShowWindowAsync
_NtUserShowWindowAsync:
    movl $0x122c, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSoundSentry
_NtUserSoundSentry:
    movl $0x122d, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserSwitchDesktop
_NtUserSwitchDesktop:
    movl $0x122e, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserSystemParametersInfo
_NtUserSystemParametersInfo:
    movl $0x122f, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserTestForInteractiveUser
_NtUserTestForInteractiveUser:
    movl $0x1230, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserThunkedMenuInfo
_NtUserThunkedMenuInfo:
    movl $0x1231, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserThunkedMenuItemInfo
_NtUserThunkedMenuItemInfo:
    movl $0x1232, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtUserToUnicodeEx
_NtUserToUnicodeEx:
    movl $0x1233, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtUserTrackMouseEvent
_NtUserTrackMouseEvent:
    movl $0x1234, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserTrackPopupMenuEx
_NtUserTrackPopupMenuEx:
    movl $0x1235, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtUserCalcMenuBar
_NtUserCalcMenuBar:
    movl $0x1236, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserPaintMenuBar
_NtUserPaintMenuBar:
    movl $0x1237, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtUserTranslateAccelerator
_NtUserTranslateAccelerator:
    movl $0x1238, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserTranslateMessage
_NtUserTranslateMessage:
    movl $0x1239, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserUnhookWindowsHookEx
_NtUserUnhookWindowsHookEx:
    movl $0x123a, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserUnhookWinEvent
_NtUserUnhookWinEvent:
    movl $0x123b, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserUnloadKeyboardLayout
_NtUserUnloadKeyboardLayout:
    movl $0x123c, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserUnlockWindowStation
_NtUserUnlockWindowStation:
    movl $0x123d, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserUnregisterClass
_NtUserUnregisterClass:
    movl $0x123e, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserUnregisterUserApiHook
_NtUserUnregisterUserApiHook:
    movl $0x123f, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserUnregisterHotKey
_NtUserUnregisterHotKey:
    movl $0x1240, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserUpdateInputContext
_NtUserUpdateInputContext:
    movl $0x1241, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserUpdateInstance
_NtUserUpdateInstance:
    movl $0x1242, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserUpdateLayeredWindow
_NtUserUpdateLayeredWindow:
    movl $0x1243, %eax
    movq %rcx, %r10
    syscall
    ret $0x28

.global _NtUserGetLayeredWindowAttributes
_NtUserGetLayeredWindowAttributes:
    movl $0x1244, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserSetLayeredWindowAttributes
_NtUserSetLayeredWindowAttributes:
    movl $0x1245, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserUpdatePerUserSystemParameters
_NtUserUpdatePerUserSystemParameters:
    movl $0x1246, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserUserHandleGrantAccess
_NtUserUserHandleGrantAccess:
    movl $0x1247, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserValidateHandleSecure
_NtUserValidateHandleSecure:
    movl $0x1248, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserValidateRect
_NtUserValidateRect:
    movl $0x1249, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserValidateTimerCallback
_NtUserValidateTimerCallback:
    movl $0x124a, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserVkKeyScanEx
_NtUserVkKeyScanEx:
    movl $0x124b, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserWaitForInputIdle
_NtUserWaitForInputIdle:
    movl $0x124c, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserWaitForMsgAndEvent
_NtUserWaitForMsgAndEvent:
    movl $0x124d, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserWaitMessage
_NtUserWaitMessage:
    movl $0x124e, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserWin32PoolAllocationStats
_NtUserWin32PoolAllocationStats:
    movl $0x124f, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtUserWindowFromPhysicalPoint
_NtUserWindowFromPhysicalPoint:
    movl $0x1250, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserWindowFromPoint
_NtUserWindowFromPoint:
    movl $0x1251, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserYieldTask
_NtUserYieldTask:
    movl $0x1252, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserRemoteConnect
_NtUserRemoteConnect:
    movl $0x1253, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserRemoteRedrawRectangle
_NtUserRemoteRedrawRectangle:
    movl $0x1254, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserRemoteRedrawScreen
_NtUserRemoteRedrawScreen:
    movl $0x1255, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserRemoteStopScreenUpdates
_NtUserRemoteStopScreenUpdates:
    movl $0x1256, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtUserCtxDisplayIOCtl
_NtUserCtxDisplayIOCtl:
    movl $0x1257, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiEngAssociateSurface
_NtGdiEngAssociateSurface:
    movl $0x1258, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiEngCreateBitmap
_NtGdiEngCreateBitmap:
    movl $0x1259, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiEngCreateDeviceSurface
_NtGdiEngCreateDeviceSurface:
    movl $0x125a, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiEngCreateDeviceBitmap
_NtGdiEngCreateDeviceBitmap:
    movl $0x125b, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiEngCreatePalette
_NtGdiEngCreatePalette:
    movl $0x125c, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiEngComputeGlyphSet
_NtGdiEngComputeGlyphSet:
    movl $0x125d, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiEngCopyBits
_NtGdiEngCopyBits:
    movl $0x125e, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiEngDeletePalette
_NtGdiEngDeletePalette:
    movl $0x125f, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEngDeleteSurface
_NtGdiEngDeleteSurface:
    movl $0x1260, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEngEraseSurface
_NtGdiEngEraseSurface:
    movl $0x1261, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiEngUnlockSurface
_NtGdiEngUnlockSurface:
    movl $0x1262, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEngLockSurface
_NtGdiEngLockSurface:
    movl $0x1263, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEngBitBlt
_NtGdiEngBitBlt:
    movl $0x1264, %eax
    movq %rcx, %r10
    syscall
    ret $0x2c

.global _NtGdiEngStretchBlt
_NtGdiEngStretchBlt:
    movl $0x1265, %eax
    movq %rcx, %r10
    syscall
    ret $0x2c

.global _NtGdiEngPlgBlt
_NtGdiEngPlgBlt:
    movl $0x1266, %eax
    movq %rcx, %r10
    syscall
    ret $0x2c

.global _NtGdiEngMarkBandingSurface
_NtGdiEngMarkBandingSurface:
    movl $0x1267, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEngStrokePath
_NtGdiEngStrokePath:
    movl $0x1268, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtGdiEngFillPath
_NtGdiEngFillPath:
    movl $0x1269, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtGdiEngStrokeAndFillPath
_NtGdiEngStrokeAndFillPath:
    movl $0x126a, %eax
    movq %rcx, %r10
    syscall
    ret $0x28

.global _NtGdiEngPaint
_NtGdiEngPaint:
    movl $0x126b, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiEngLineTo
_NtGdiEngLineTo:
    movl $0x126c, %eax
    movq %rcx, %r10
    syscall
    ret $0x24

.global _NtGdiEngAlphaBlend
_NtGdiEngAlphaBlend:
    movl $0x126d, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtGdiEngGradientFill
_NtGdiEngGradientFill:
    movl $0x126e, %eax
    movq %rcx, %r10
    syscall
    ret $0x28

.global _NtGdiEngTransparentBlt
_NtGdiEngTransparentBlt:
    movl $0x126f, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtGdiEngTextOut
_NtGdiEngTextOut:
    movl $0x1270, %eax
    movq %rcx, %r10
    syscall
    ret $0x28

.global _NtGdiEngStretchBltROP
_NtGdiEngStretchBltROP:
    movl $0x1271, %eax
    movq %rcx, %r10
    syscall
    ret $0x34

.global _NtGdiXLATEOBJ_cGetPalette
_NtGdiXLATEOBJ_cGetPalette:
    movl $0x1272, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiXLATEOBJ_iXlate
_NtGdiXLATEOBJ_iXlate:
    movl $0x1273, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiXLATEOBJ_hGetColorTransform
_NtGdiXLATEOBJ_hGetColorTransform:
    movl $0x1274, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiCLIPOBJ_bEnum
_NtGdiCLIPOBJ_bEnum:
    movl $0x1275, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiCLIPOBJ_cEnumStart
_NtGdiCLIPOBJ_cEnumStart:
    movl $0x1276, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiCLIPOBJ_ppoGetPath
_NtGdiCLIPOBJ_ppoGetPath:
    movl $0x1277, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEngDeletePath
_NtGdiEngDeletePath:
    movl $0x1278, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEngCreateClip
_NtGdiEngCreateClip:
    movl $0x1279, %eax
    movq %rcx, %r10
    syscall
    ret $0x0

.global _NtGdiEngDeleteClip
_NtGdiEngDeleteClip:
    movl $0x127a, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiBRUSHOBJ_ulGetBrushColor
_NtGdiBRUSHOBJ_ulGetBrushColor:
    movl $0x127b, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiBRUSHOBJ_pvAllocRbrush
_NtGdiBRUSHOBJ_pvAllocRbrush:
    movl $0x127c, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiBRUSHOBJ_pvGetRbrush
_NtGdiBRUSHOBJ_pvGetRbrush:
    movl $0x127d, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiBRUSHOBJ_hGetColorTransform
_NtGdiBRUSHOBJ_hGetColorTransform:
    movl $0x127e, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiXFORMOBJ_bApplyXform
_NtGdiXFORMOBJ_bApplyXform:
    movl $0x127f, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiXFORMOBJ_iGetXform
_NtGdiXFORMOBJ_iGetXform:
    movl $0x1280, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiFONTOBJ_vGetInfo
_NtGdiFONTOBJ_vGetInfo:
    movl $0x1281, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiFONTOBJ_pxoGetXform
_NtGdiFONTOBJ_pxoGetXform:
    movl $0x1282, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiFONTOBJ_cGetGlyphs
_NtGdiFONTOBJ_cGetGlyphs:
    movl $0x1283, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtGdiFONTOBJ_pifi
_NtGdiFONTOBJ_pifi:
    movl $0x1284, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiFONTOBJ_pfdg
_NtGdiFONTOBJ_pfdg:
    movl $0x1285, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiFONTOBJ_pQueryGlyphAttrs
_NtGdiFONTOBJ_pQueryGlyphAttrs:
    movl $0x1286, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiFONTOBJ_pvTrueTypeFontFile
_NtGdiFONTOBJ_pvTrueTypeFontFile:
    movl $0x1287, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiFONTOBJ_cGetAllGlyphHandles
_NtGdiFONTOBJ_cGetAllGlyphHandles:
    movl $0x1288, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiSTROBJ_bEnum
_NtGdiSTROBJ_bEnum:
    movl $0x1289, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiSTROBJ_bEnumPositionsOnly
_NtGdiSTROBJ_bEnumPositionsOnly:
    movl $0x128a, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiSTROBJ_bGetAdvanceWidths
_NtGdiSTROBJ_bGetAdvanceWidths:
    movl $0x128b, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiSTROBJ_vEnumStart
_NtGdiSTROBJ_vEnumStart:
    movl $0x128c, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiSTROBJ_dwGetCodePage
_NtGdiSTROBJ_dwGetCodePage:
    movl $0x128d, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiPATHOBJ_vGetBounds
_NtGdiPATHOBJ_vGetBounds:
    movl $0x128e, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiPATHOBJ_bEnum
_NtGdiPATHOBJ_bEnum:
    movl $0x128f, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiPATHOBJ_vEnumStart
_NtGdiPATHOBJ_vEnumStart:
    movl $0x1290, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiPATHOBJ_vEnumStartClipLines
_NtGdiPATHOBJ_vEnumStartClipLines:
    movl $0x1291, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiPATHOBJ_bEnumClipLines
_NtGdiPATHOBJ_bEnumClipLines:
    movl $0x1292, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiGetDhpdev
_NtGdiGetDhpdev:
    movl $0x1293, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiEngCheckAbort
_NtGdiEngCheckAbort:
    movl $0x1294, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiHT_Get8BPPFormatPalette
_NtGdiHT_Get8BPPFormatPalette:
    movl $0x1295, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiHT_Get8BPPMaskPalette
_NtGdiHT_Get8BPPMaskPalette:
    movl $0x1296, %eax
    movq %rcx, %r10
    syscall
    ret $0x18

.global _NtGdiUpdateTransform
_NtGdiUpdateTransform:
    movl $0x1297, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiSetPUMPDOBJ
_NtGdiSetPUMPDOBJ:
    movl $0x1298, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiBRUSHOBJ_DeleteRbrush
_NtGdiBRUSHOBJ_DeleteRbrush:
    movl $0x1299, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtGdiUMPDEngFreeUserMem
_NtGdiUMPDEngFreeUserMem:
    movl $0x129a, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtGdiDrawStream
_NtGdiDrawStream:
    movl $0x129b, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtGdiSetDIBits
_NtGdiSetDIBits:
    movl $0x129c, %eax
    movq %rcx, %r10
    syscall
    ret $0x1c

.global _NtGdiSetViewportOrgEx
_NtGdiSetViewportOrgEx:
    movl $0x129d, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiSetWindowOrgEx
_NtGdiSetWindowOrgEx:
    movl $0x129e, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiGetFontFamilyInfo
_NtGdiGetFontFamilyInfo:
    movl $0x129f, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiOffsetViewportOrgEx
_NtGdiOffsetViewportOrgEx:
    movl $0x12a0, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtGdiOffsetWindowOrgEx
_NtGdiOffsetWindowOrgEx:
    movl $0x12a1, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserBuildMenuItemList
_NtUserBuildMenuItemList:
    movl $0x12a2, %eax
    movq %rcx, %r10
    syscall
    ret $0x10

.global _NtUserCreateCursorIconHandle
_NtUserCreateCursorIconHandle:
    movl $0x12a3, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetClassLong
_NtUserGetClassLong:
    movl $0x12a4, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetMenuDefaultItem
_NtUserGetMenuDefaultItem:
    movl $0x12a5, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetLastInputInfo
_NtUserGetLastInputInfo:
    movl $0x12a6, %eax
    movq %rcx, %r10
    syscall
    ret $0x4

.global _NtUserGetMinMaxInfo
_NtUserGetMinMaxInfo:
    movl $0x12a7, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetMonitorInfo
_NtUserGetMonitorInfo:
    movl $0x12a8, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetScrollInfo
_NtUserGetScrollInfo:
    movl $0x12a9, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserGetWindow
_NtUserGetWindow:
    movl $0x12aa, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserGetWindowLong
_NtUserGetWindowLong:
    movl $0x12ab, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserMenuInfo
_NtUserMenuInfo:
    movl $0x12ac, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserMenuItemInfo
_NtUserMenuItemInfo:
    movl $0x12ad, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserMonitorFromPoint
_NtUserMonitorFromPoint:
    movl $0x12ae, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

.global _NtUserMonitorFromRect
_NtUserMonitorFromRect:
    movl $0x12af, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserMonitorFromWindow
_NtUserMonitorFromWindow:
    movl $0x12b0, %eax
    movq %rcx, %r10
    syscall
    ret $0x8

.global _NtUserSendMessage
_NtUserSendMessage:
    movl $0x12b1, %eax
    movq %rcx, %r10
    syscall
    ret $0x14

.global _NtUserSendMessageTimeout
_NtUserSendMessageTimeout:
    movl $0x12b2, %eax
    movq %rcx, %r10
    syscall
    ret $0x20

.global _NtUserSetScrollBarInfo
_NtUserSetScrollBarInfo:
    movl $0x12b3, %eax
    movq %rcx, %r10
    syscall
    ret $0xc

