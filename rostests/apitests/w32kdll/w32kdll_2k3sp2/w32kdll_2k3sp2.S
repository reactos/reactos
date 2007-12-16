/* FILE:            System Call Stubs for Native API
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         obj-i386\lib\win32ksys\win32k.S
 * PROGRAMMER:      Computer Generated File. See tools/nci/ncitool.c
 * REMARK:          DO NOT EDIT OR COMMIT MODIFICATIONS TO THIS FILE
 */


#include <ndk/asm.h>

.global _NtGdiAbortDoc@4
_NtGdiAbortDoc@4:
    movl $0x1000, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiAbortPath@4
_NtGdiAbortPath@4:
    movl $0x1001, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiAddFontResourceW@24
_NtGdiAddFontResourceW@24:
    movl $0x1002, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiAddRemoteFontToDC@16
_NtGdiAddRemoteFontToDC@16:
    movl $0x1003, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiAddFontMemResourceEx@20
_NtGdiAddFontMemResourceEx@20:
    movl $0x1004, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiRemoveMergeFont@8
_NtGdiRemoveMergeFont@8:
    movl $0x1005, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiAddRemoteMMInstanceToDC@12
_NtGdiAddRemoteMMInstanceToDC@12:
    movl $0x1006, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiAlphaBlend@48
_NtGdiAlphaBlend@48:
    movl $0x1007, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x30

.global _NtGdiAngleArc@24
_NtGdiAngleArc@24:
    movl $0x1008, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiAnyLinkedFonts@0
_NtGdiAnyLinkedFonts@0:
    movl $0x1009, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtGdiFontIsLinked@4
_NtGdiFontIsLinked@4:
    movl $0x100a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiArcInternal@40
_NtGdiArcInternal@40:
    movl $0x100b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x28

.global _NtGdiBeginPath@4
_NtGdiBeginPath@4:
    movl $0x100c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiBitBlt@44
_NtGdiBitBlt@44:
    movl $0x100d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x2c

.global _NtGdiCancelDC@4
_NtGdiCancelDC@4:
    movl $0x100e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiCheckBitmapBits@32
_NtGdiCheckBitmapBits@32:
    movl $0x100f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtGdiCloseFigure@4
_NtGdiCloseFigure@4:
    movl $0x1010, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiClearBitmapAttributes@8
_NtGdiClearBitmapAttributes@8:
    movl $0x1011, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiClearBrushAttributes@8
_NtGdiClearBrushAttributes@8:
    movl $0x1012, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiColorCorrectPalette@24
_NtGdiColorCorrectPalette@24:
    movl $0x1013, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiCombineRgn@16
_NtGdiCombineRgn@16:
    movl $0x1014, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiCombineTransform@12
_NtGdiCombineTransform@12:
    movl $0x1015, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiComputeXformCoefficients@4
_NtGdiComputeXformCoefficients@4:
    movl $0x1016, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiConsoleTextOut@16
_NtGdiConsoleTextOut@16:
    movl $0x1017, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiConvertMetafileRect@8
_NtGdiConvertMetafileRect@8:
    movl $0x1018, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiCreateBitmap@20
_NtGdiCreateBitmap@20:
    movl $0x1019, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiCreateClientObj@4
_NtGdiCreateClientObj@4:
    movl $0x101a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiCreateColorSpace@4
_NtGdiCreateColorSpace@4:
    movl $0x101b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiCreateColorTransform@32
_NtGdiCreateColorTransform@32:
    movl $0x101c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtGdiCreateCompatibleBitmap@12
_NtGdiCreateCompatibleBitmap@12:
    movl $0x101d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiCreateCompatibleDC@4
_NtGdiCreateCompatibleDC@4:
    movl $0x101e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiCreateDIBBrush@24
_NtGdiCreateDIBBrush@24:
    movl $0x101f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiCreateDIBitmapInternal@44
_NtGdiCreateDIBitmapInternal@44:
    movl $0x1020, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x2c

.global _NtGdiCreateDIBSection@36
_NtGdiCreateDIBSection@36:
    movl $0x1021, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x24

.global _NtGdiCreateEllipticRgn@16
_NtGdiCreateEllipticRgn@16:
    movl $0x1022, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiCreateHalftonePalette@4
_NtGdiCreateHalftonePalette@4:
    movl $0x1023, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiCreateHatchBrushInternal@12
_NtGdiCreateHatchBrushInternal@12:
    movl $0x1024, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiCreateMetafileDC@4
_NtGdiCreateMetafileDC@4:
    movl $0x1025, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiCreatePaletteInternal@8
_NtGdiCreatePaletteInternal@8:
    movl $0x1026, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiCreatePatternBrushInternal@12
_NtGdiCreatePatternBrushInternal@12:
    movl $0x1027, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiCreatePen@16
_NtGdiCreatePen@16:
    movl $0x1028, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiCreateRectRgn@16
_NtGdiCreateRectRgn@16:
    movl $0x1029, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiCreateRoundRectRgn@24
_NtGdiCreateRoundRectRgn@24:
    movl $0x102a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiCreateServerMetaFile@24
_NtGdiCreateServerMetaFile@24:
    movl $0x102b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiCreateSolidBrush@8
_NtGdiCreateSolidBrush@8:
    movl $0x102c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiD3dContextCreate@16
_NtGdiD3dContextCreate@16:
    movl $0x102d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiD3dContextDestroy@4
_NtGdiD3dContextDestroy@4:
    movl $0x102e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiD3dContextDestroyAll@4
_NtGdiD3dContextDestroyAll@4:
    movl $0x102f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiD3dValidateTextureStageState@4
_NtGdiD3dValidateTextureStageState@4:
    movl $0x1030, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiD3dDrawPrimitives2@28
_NtGdiD3dDrawPrimitives2@28:
    movl $0x1031, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtGdiDdGetDriverState@4
_NtGdiDdGetDriverState@4:
    movl $0x1032, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiDdAddAttachedSurface@12
_NtGdiDdAddAttachedSurface@12:
    movl $0x1033, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiDdAlphaBlt@12
_NtGdiDdAlphaBlt@12:
    movl $0x1034, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiDdAttachSurface@8
_NtGdiDdAttachSurface@8:
    movl $0x1035, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdBeginMoCompFrame@8
_NtGdiDdBeginMoCompFrame@8:
    movl $0x1036, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdBlt@12
_NtGdiDdBlt@12:
    movl $0x1037, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiDdCanCreateSurface@8
_NtGdiDdCanCreateSurface@8:
    movl $0x1038, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdCanCreateD3DBuffer@8
_NtGdiDdCanCreateD3DBuffer@8:
    movl $0x1039, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdColorControl@8
_NtGdiDdColorControl@8:
    movl $0x103a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdCreateDirectDrawObject@4
_NtGdiDdCreateDirectDrawObject@4:
    movl $0x103b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiDdCreateSurface@32
_NtGdiDdCreateSurface@32:
    movl $0x103c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtGdiDdCreateD3DBuffer@32
_NtGdiDdCreateD3DBuffer@32:
    movl $0x103d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtGdiDdCreateMoComp@8
_NtGdiDdCreateMoComp@8:
    movl $0x103e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdCreateSurfaceObject@24
_NtGdiDdCreateSurfaceObject@24:
    movl $0x103f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiDdDeleteDirectDrawObject@4
_NtGdiDdDeleteDirectDrawObject@4:
    movl $0x1040, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiDdDeleteSurfaceObject@4
_NtGdiDdDeleteSurfaceObject@4:
    movl $0x1041, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiDdDestroyMoComp@8
_NtGdiDdDestroyMoComp@8:
    movl $0x1042, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdDestroySurface@8
_NtGdiDdDestroySurface@8:
    movl $0x1043, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdDestroyD3DBuffer@4
_NtGdiDdDestroyD3DBuffer@4:
    movl $0x1044, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiDdEndMoCompFrame@8
_NtGdiDdEndMoCompFrame@8:
    movl $0x1045, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdFlip@20
_NtGdiDdFlip@20:
    movl $0x1046, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiDdFlipToGDISurface@8
_NtGdiDdFlipToGDISurface@8:
    movl $0x1047, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdGetAvailDriverMemory@8
_NtGdiDdGetAvailDriverMemory@8:
    movl $0x1048, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdGetBltStatus@8
_NtGdiDdGetBltStatus@8:
    movl $0x1049, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdGetDC@8
_NtGdiDdGetDC@8:
    movl $0x104a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdGetDriverInfo@8
_NtGdiDdGetDriverInfo@8:
    movl $0x104b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdGetDxHandle@12
_NtGdiDdGetDxHandle@12:
    movl $0x104c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiDdGetFlipStatus@8
_NtGdiDdGetFlipStatus@8:
    movl $0x104d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdGetInternalMoCompInfo@8
_NtGdiDdGetInternalMoCompInfo@8:
    movl $0x104e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdGetMoCompBuffInfo@8
_NtGdiDdGetMoCompBuffInfo@8:
    movl $0x104f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdGetMoCompGuids@8
_NtGdiDdGetMoCompGuids@8:
    movl $0x1050, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdGetMoCompFormats@8
_NtGdiDdGetMoCompFormats@8:
    movl $0x1051, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdGetScanLine@8
_NtGdiDdGetScanLine@8:
    movl $0x1052, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdLock@12
_NtGdiDdLock@12:
    movl $0x1053, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiDdLockD3D@8
_NtGdiDdLockD3D@8:
    movl $0x1054, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdQueryDirectDrawObject@44
_NtGdiDdQueryDirectDrawObject@44:
    movl $0x1055, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x2c

.global _NtGdiDdQueryMoCompStatus@8
_NtGdiDdQueryMoCompStatus@8:
    movl $0x1056, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdReenableDirectDrawObject@8
_NtGdiDdReenableDirectDrawObject@8:
    movl $0x1057, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdReleaseDC@4
_NtGdiDdReleaseDC@4:
    movl $0x1058, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiDdRenderMoComp@8
_NtGdiDdRenderMoComp@8:
    movl $0x1059, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdResetVisrgn@8
_NtGdiDdResetVisrgn@8:
    movl $0x105a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdSetColorKey@8
_NtGdiDdSetColorKey@8:
    movl $0x105b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdSetExclusiveMode@8
_NtGdiDdSetExclusiveMode@8:
    movl $0x105c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdSetGammaRamp@12
_NtGdiDdSetGammaRamp@12:
    movl $0x105d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiDdCreateSurfaceEx@12
_NtGdiDdCreateSurfaceEx@12:
    movl $0x105e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiDdSetOverlayPosition@12
_NtGdiDdSetOverlayPosition@12:
    movl $0x105f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiDdUnattachSurface@8
_NtGdiDdUnattachSurface@8:
    movl $0x1060, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdUnlock@8
_NtGdiDdUnlock@8:
    movl $0x1061, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdUnlockD3D@8
_NtGdiDdUnlockD3D@8:
    movl $0x1062, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDdUpdateOverlay@12
_NtGdiDdUpdateOverlay@12:
    movl $0x1063, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiDdWaitForVerticalBlank@8
_NtGdiDdWaitForVerticalBlank@8:
    movl $0x1064, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpCanCreateVideoPort@8
_NtGdiDvpCanCreateVideoPort@8:
    movl $0x1065, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpColorControl@8
_NtGdiDvpColorControl@8:
    movl $0x1066, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpCreateVideoPort@8
_NtGdiDvpCreateVideoPort@8:
    movl $0x1067, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpDestroyVideoPort@8
_NtGdiDvpDestroyVideoPort@8:
    movl $0x1068, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpFlipVideoPort@16
_NtGdiDvpFlipVideoPort@16:
    movl $0x1069, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiDvpGetVideoPortBandwidth@8
_NtGdiDvpGetVideoPortBandwidth@8:
    movl $0x106a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpGetVideoPortField@8
_NtGdiDvpGetVideoPortField@8:
    movl $0x106b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpGetVideoPortFlipStatus@8
_NtGdiDvpGetVideoPortFlipStatus@8:
    movl $0x106c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpGetVideoPortInputFormats@8
_NtGdiDvpGetVideoPortInputFormats@8:
    movl $0x106d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpGetVideoPortLine@8
_NtGdiDvpGetVideoPortLine@8:
    movl $0x106e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpGetVideoPortOutputFormats@8
_NtGdiDvpGetVideoPortOutputFormats@8:
    movl $0x106f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpGetVideoPortConnectInfo@8
_NtGdiDvpGetVideoPortConnectInfo@8:
    movl $0x1070, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpGetVideoSignalStatus@8
_NtGdiDvpGetVideoSignalStatus@8:
    movl $0x1071, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpUpdateVideoPort@16
_NtGdiDvpUpdateVideoPort@16:
    movl $0x1072, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiDvpWaitForVideoPortSync@8
_NtGdiDvpWaitForVideoPortSync@8:
    movl $0x1073, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDvpAcquireNotification@12
_NtGdiDvpAcquireNotification@12:
    movl $0x1074, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiDvpReleaseNotification@8
_NtGdiDvpReleaseNotification@8:
    movl $0x1075, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDxgGenericThunk@24
_NtGdiDxgGenericThunk@24:
    movl $0x1076, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiDeleteClientObj@4
_NtGdiDeleteClientObj@4:
    movl $0x1077, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiDeleteColorSpace@4
_NtGdiDeleteColorSpace@4:
    movl $0x1078, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiDeleteColorTransform@8
_NtGdiDeleteColorTransform@8:
    movl $0x1079, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDeleteObjectApp@4
_NtGdiDeleteObjectApp@4:
    movl $0x107a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiDescribePixelFormat@16
_NtGdiDescribePixelFormat@16:
    movl $0x107b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiGetPerBandInfo@8
_NtGdiGetPerBandInfo@8:
    movl $0x107c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiDoBanding@16
_NtGdiDoBanding@16:
    movl $0x107d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiDoPalette@24
_NtGdiDoPalette@24:
    movl $0x107e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiDrawEscape@16
_NtGdiDrawEscape@16:
    movl $0x107f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiEllipse@20
_NtGdiEllipse@20:
    movl $0x1080, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiEnableEudc@4
_NtGdiEnableEudc@4:
    movl $0x1081, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEndDoc@4
_NtGdiEndDoc@4:
    movl $0x1082, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEndPage@4
_NtGdiEndPage@4:
    movl $0x1083, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEndPath@4
_NtGdiEndPath@4:
    movl $0x1084, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEnumFontChunk@20
_NtGdiEnumFontChunk@20:
    movl $0x1085, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiEnumFontClose@4
_NtGdiEnumFontClose@4:
    movl $0x1086, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEnumFontOpen@28
_NtGdiEnumFontOpen@28:
    movl $0x1087, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtGdiEnumObjects@16
_NtGdiEnumObjects@16:
    movl $0x1088, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiEqualRgn@8
_NtGdiEqualRgn@8:
    movl $0x1089, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiEudcLoadUnloadLink@28
_NtGdiEudcLoadUnloadLink@28:
    movl $0x108a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtGdiExcludeClipRect@20
_NtGdiExcludeClipRect@20:
    movl $0x108b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiExtCreatePen@44
_NtGdiExtCreatePen@44:
    movl $0x108c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x2c

.global _NtGdiExtCreateRegion@12
_NtGdiExtCreateRegion@12:
    movl $0x108d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiExtEscape@32
_NtGdiExtEscape@32:
    movl $0x108e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtGdiExtFloodFill@20
_NtGdiExtFloodFill@20:
    movl $0x108f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiExtGetObjectW@12
_NtGdiExtGetObjectW@12:
    movl $0x1090, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiExtSelectClipRgn@12
_NtGdiExtSelectClipRgn@12:
    movl $0x1091, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiExtTextOutW@36
_NtGdiExtTextOutW@36:
    movl $0x1092, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x24

.global _NtGdiFillPath@4
_NtGdiFillPath@4:
    movl $0x1093, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiFillRgn@12
_NtGdiFillRgn@12:
    movl $0x1094, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiFlattenPath@4
_NtGdiFlattenPath@4:
    movl $0x1095, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiFlush@0
_NtGdiFlush@0:
    movl $0x1096, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtGdiForceUFIMapping@8
_NtGdiForceUFIMapping@8:
    movl $0x1097, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiFrameRgn@20
_NtGdiFrameRgn@20:
    movl $0x1098, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiFullscreenControl@20
_NtGdiFullscreenControl@20:
    movl $0x1099, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiGetAndSetDCDword@16
_NtGdiGetAndSetDCDword@16:
    movl $0x109a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiGetAppClipBox@8
_NtGdiGetAppClipBox@8:
    movl $0x109b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetBitmapBits@12
_NtGdiGetBitmapBits@12:
    movl $0x109c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetBitmapDimension@8
_NtGdiGetBitmapDimension@8:
    movl $0x109d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetBoundsRect@12
_NtGdiGetBoundsRect@12:
    movl $0x109e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetCharABCWidthsW@24
_NtGdiGetCharABCWidthsW@24:
    movl $0x109f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiGetCharacterPlacementW@24
_NtGdiGetCharacterPlacementW@24:
    movl $0x10a0, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiGetCharSet@4
_NtGdiGetCharSet@4:
    movl $0x10a1, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiGetCharWidthW@24
_NtGdiGetCharWidthW@24:
    movl $0x10a2, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiGetCharWidthInfo@8
_NtGdiGetCharWidthInfo@8:
    movl $0x10a3, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetColorAdjustment@8
_NtGdiGetColorAdjustment@8:
    movl $0x10a4, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetColorSpaceforBitmap@4
_NtGdiGetColorSpaceforBitmap@4:
    movl $0x10a5, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiGetDCDword@12
_NtGdiGetDCDword@12:
    movl $0x10a6, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetDCforBitmap@4
_NtGdiGetDCforBitmap@4:
    movl $0x10a7, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiGetDCObject@8
_NtGdiGetDCObject@8:
    movl $0x10a8, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetDCPoint@12
_NtGdiGetDCPoint@12:
    movl $0x10a9, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetDeviceCaps@8
_NtGdiGetDeviceCaps@8:
    movl $0x10aa, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetDeviceGammaRamp@8
_NtGdiGetDeviceGammaRamp@8:
    movl $0x10ab, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetDeviceCapsAll@8
_NtGdiGetDeviceCapsAll@8:
    movl $0x10ac, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetDIBitsInternal@36
_NtGdiGetDIBitsInternal@36:
    movl $0x10ad, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x24

.global _NtGdiGetETM@8
_NtGdiGetETM@8:
    movl $0x10ae, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetEudcTimeStampEx@12
_NtGdiGetEudcTimeStampEx@12:
    movl $0x10af, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetFontData@20
_NtGdiGetFontData@20:
    movl $0x10b0, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiGetFontResourceInfoInternalW@28
_NtGdiGetFontResourceInfoInternalW@28:
    movl $0x10b1, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtGdiGetGlyphIndicesW@20
_NtGdiGetGlyphIndicesW@20:
    movl $0x10b2, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiGetGlyphIndicesWInternal@24
_NtGdiGetGlyphIndicesWInternal@24:
    movl $0x10b3, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiGetGlyphOutline@32
_NtGdiGetGlyphOutline@32:
    movl $0x10b4, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtGdiGetKerningPairs@12
_NtGdiGetKerningPairs@12:
    movl $0x10b5, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetLinkedUFIs@12
_NtGdiGetLinkedUFIs@12:
    movl $0x10b6, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetMiterLimit@8
_NtGdiGetMiterLimit@8:
    movl $0x10b7, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetMonitorID@12
_NtGdiGetMonitorID@12:
    movl $0x10b8, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetNearestColor@8
_NtGdiGetNearestColor@8:
    movl $0x10b9, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetNearestPaletteIndex@8
_NtGdiGetNearestPaletteIndex@8:
    movl $0x10ba, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetObjectBitmapHandle@8
_NtGdiGetObjectBitmapHandle@8:
    movl $0x10bb, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetOutlineTextMetricsInternalW@16
_NtGdiGetOutlineTextMetricsInternalW@16:
    movl $0x10bc, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiGetPath@16
_NtGdiGetPath@16:
    movl $0x10bd, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiGetPixel@12
_NtGdiGetPixel@12:
    movl $0x10be, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetRandomRgn@12
_NtGdiGetRandomRgn@12:
    movl $0x10bf, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetRasterizerCaps@8
_NtGdiGetRasterizerCaps@8:
    movl $0x10c0, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetRealizationInfo@12
_NtGdiGetRealizationInfo@12:
    movl $0x10c1, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetRegionData@12
_NtGdiGetRegionData@12:
    movl $0x10c2, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetRgnBox@8
_NtGdiGetRgnBox@8:
    movl $0x10c3, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetServerMetaFileBits@28
_NtGdiGetServerMetaFileBits@28:
    movl $0x10c4, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtGdiGetSpoolMessage@16
_NtGdiGetSpoolMessage@16:
    movl $0x10c5, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiGetStats@20
_NtGdiGetStats@20:
    movl $0x10c6, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiGetStockObject@4
_NtGdiGetStockObject@4:
    movl $0x10c7, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiGetStringBitmapW@20
_NtGdiGetStringBitmapW@20:
    movl $0x10c8, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiGetSystemPaletteUse@4
_NtGdiGetSystemPaletteUse@4:
    movl $0x10c9, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiGetTextCharsetInfo@12
_NtGdiGetTextCharsetInfo@12:
    movl $0x10ca, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetTextExtent@20
_NtGdiGetTextExtent@20:
    movl $0x10cb, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiGetTextExtentExW@32
_NtGdiGetTextExtentExW@32:
    movl $0x10cc, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtGdiGetTextFaceW@16
_NtGdiGetTextFaceW@16:
    movl $0x10cd, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiGetTextMetricsW@12
_NtGdiGetTextMetricsW@12:
    movl $0x10ce, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetTransform@12
_NtGdiGetTransform@12:
    movl $0x10cf, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetUFI@24
_NtGdiGetUFI@24:
    movl $0x10d0, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiGetEmbUFI@28
_NtGdiGetEmbUFI@28:
    movl $0x10d1, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtGdiGetUFIPathname@40
_NtGdiGetUFIPathname@40:
    movl $0x10d2, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x28

.global _NtGdiGetEmbedFonts@0
_NtGdiGetEmbedFonts@0:
    movl $0x10d3, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtGdiChangeGhostFont@8
_NtGdiChangeGhostFont@8:
    movl $0x10d4, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiAddEmbFontToDC@8
_NtGdiAddEmbFontToDC@8:
    movl $0x10d5, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetFontUnicodeRanges@8
_NtGdiGetFontUnicodeRanges@8:
    movl $0x10d6, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiGetWidthTable@28
_NtGdiGetWidthTable@28:
    movl $0x10d7, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtGdiGradientFill@24
_NtGdiGradientFill@24:
    movl $0x10d8, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiHfontCreate@20
_NtGdiHfontCreate@20:
    movl $0x10d9, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiIcmBrushInfo@32
_NtGdiIcmBrushInfo@32:
    movl $0x10da, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtGdiInit@0
_NtGdiInit@0:
    movl $0x10db, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtGdiInitSpool@0
_NtGdiInitSpool@0:
    movl $0x10dc, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtGdiIntersectClipRect@20
_NtGdiIntersectClipRect@20:
    movl $0x10dd, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiInvertRgn@8
_NtGdiInvertRgn@8:
    movl $0x10de, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiLineTo@12
_NtGdiLineTo@12:
    movl $0x10df, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiMakeFontDir@20
_NtGdiMakeFontDir@20:
    movl $0x10e0, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiMakeInfoDC@8
_NtGdiMakeInfoDC@8:
    movl $0x10e1, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiMaskBlt@52
_NtGdiMaskBlt@52:
    movl $0x10e2, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x34

.global _NtGdiModifyWorldTransform@12
_NtGdiModifyWorldTransform@12:
    movl $0x10e3, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiMonoBitmap@4
_NtGdiMonoBitmap@4:
    movl $0x10e4, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiMoveTo@16
_NtGdiMoveTo@16:
    movl $0x10e5, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiOffsetClipRgn@12
_NtGdiOffsetClipRgn@12:
    movl $0x10e6, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiOffsetRgn@12
_NtGdiOffsetRgn@12:
    movl $0x10e7, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiOpenDCW@28
_NtGdiOpenDCW@28:
    movl $0x10e8, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtGdiPatBlt@24
_NtGdiPatBlt@24:
    movl $0x10e9, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiPolyPatBlt@20
_NtGdiPolyPatBlt@20:
    movl $0x10ea, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiPathToRegion@4
_NtGdiPathToRegion@4:
    movl $0x10eb, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiPlgBlt@44
_NtGdiPlgBlt@44:
    movl $0x10ec, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x2c

.global _NtGdiPolyDraw@16
_NtGdiPolyDraw@16:
    movl $0x10ed, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiPolyPolyDraw@20
_NtGdiPolyPolyDraw@20:
    movl $0x10ee, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiPolyTextOutW@16
_NtGdiPolyTextOutW@16:
    movl $0x10ef, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiPtInRegion@12
_NtGdiPtInRegion@12:
    movl $0x10f0, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiPtVisible@12
_NtGdiPtVisible@12:
    movl $0x10f1, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiQueryFonts@12
_NtGdiQueryFonts@12:
    movl $0x10f2, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiQueryFontAssocInfo@4
_NtGdiQueryFontAssocInfo@4:
    movl $0x10f3, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiRectangle@20
_NtGdiRectangle@20:
    movl $0x10f4, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiRectInRegion@8
_NtGdiRectInRegion@8:
    movl $0x10f5, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiRectVisible@8
_NtGdiRectVisible@8:
    movl $0x10f6, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiRemoveFontResourceW@24
_NtGdiRemoveFontResourceW@24:
    movl $0x10f7, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiRemoveFontMemResourceEx@4
_NtGdiRemoveFontMemResourceEx@4:
    movl $0x10f8, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiResetDC@20
_NtGdiResetDC@20:
    movl $0x10f9, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiResizePalette@8
_NtGdiResizePalette@8:
    movl $0x10fa, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiRestoreDC@8
_NtGdiRestoreDC@8:
    movl $0x10fb, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiRoundRect@28
_NtGdiRoundRect@28:
    movl $0x10fc, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtGdiSaveDC@4
_NtGdiSaveDC@4:
    movl $0x10fd, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiScaleViewportExtEx@24
_NtGdiScaleViewportExtEx@24:
    movl $0x10fe, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiScaleWindowExtEx@24
_NtGdiScaleWindowExtEx@24:
    movl $0x10ff, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _GreSelectBitmap@8
_GreSelectBitmap@8:
    movl $0x1100, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSelectBrush@8
_NtGdiSelectBrush@8:
    movl $0x1101, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSelectClipPath@8
_NtGdiSelectClipPath@8:
    movl $0x1102, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSelectFont@8
_NtGdiSelectFont@8:
    movl $0x1103, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSelectPen@8
_NtGdiSelectPen@8:
    movl $0x1104, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSetBitmapAttributes@8
_NtGdiSetBitmapAttributes@8:
    movl $0x1105, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSetBitmapBits@12
_NtGdiSetBitmapBits@12:
    movl $0x1106, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiSetBitmapDimension@16
_NtGdiSetBitmapDimension@16:
    movl $0x1107, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiSetBoundsRect@12
_NtGdiSetBoundsRect@12:
    movl $0x1108, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiSetBrushAttributes@8
_NtGdiSetBrushAttributes@8:
    movl $0x1109, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSetBrushOrg@16
_NtGdiSetBrushOrg@16:
    movl $0x110a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiSetColorAdjustment@8
_NtGdiSetColorAdjustment@8:
    movl $0x110b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSetColorSpace@8
_NtGdiSetColorSpace@8:
    movl $0x110c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSetDeviceGammaRamp@8
_NtGdiSetDeviceGammaRamp@8:
    movl $0x110d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSetDIBitsToDeviceInternal@64
_NtGdiSetDIBitsToDeviceInternal@64:
    movl $0x110e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x40

.global _NtGdiSetFontEnumeration@4
_NtGdiSetFontEnumeration@4:
    movl $0x110f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiSetFontXform@12
_NtGdiSetFontXform@12:
    movl $0x1110, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiSetIcmMode@12
_NtGdiSetIcmMode@12:
    movl $0x1111, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiSetLinkedUFIs@12
_NtGdiSetLinkedUFIs@12:
    movl $0x1112, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiSetMagicColors@12
_NtGdiSetMagicColors@12:
    movl $0x1113, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiSetMetaRgn@4
_NtGdiSetMetaRgn@4:
    movl $0x1114, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiSetMiterLimit@12
_NtGdiSetMiterLimit@12:
    movl $0x1115, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetDeviceWidth@4
_NtGdiGetDeviceWidth@4:
    movl $0x1116, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiMirrorWindowOrg@4
_NtGdiMirrorWindowOrg@4:
    movl $0x1117, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiSetLayout@12
_NtGdiSetLayout@12:
    movl $0x1118, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiSetPixel@16
_NtGdiSetPixel@16:
    movl $0x1119, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiSetPixelFormat@8
_NtGdiSetPixelFormat@8:
    movl $0x111a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSetRectRgn@20
_NtGdiSetRectRgn@20:
    movl $0x111b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiSetSystemPaletteUse@8
_NtGdiSetSystemPaletteUse@8:
    movl $0x111c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSetTextJustification@12
_NtGdiSetTextJustification@12:
    movl $0x111d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiSetupPublicCFONT@12
_NtGdiSetupPublicCFONT@12:
    movl $0x111e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiSetVirtualResolution@20
_NtGdiSetVirtualResolution@20:
    movl $0x111f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiSetSizeDevice@12
_NtGdiSetSizeDevice@12:
    movl $0x1120, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiStartDoc@16
_NtGdiStartDoc@16:
    movl $0x1121, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiStartPage@4
_NtGdiStartPage@4:
    movl $0x1122, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiStretchBlt@48
_NtGdiStretchBlt@48:
    movl $0x1123, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x30

.global _NtGdiStretchDIBitsInternal@64
_NtGdiStretchDIBitsInternal@64:
    movl $0x1124, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x40

.global _NtGdiStrokeAndFillPath@4
_NtGdiStrokeAndFillPath@4:
    movl $0x1125, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiStrokePath@4
_NtGdiStrokePath@4:
    movl $0x1126, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiSwapBuffers@4
_NtGdiSwapBuffers@4:
    movl $0x1127, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiTransformPoints@20
_NtGdiTransformPoints@20:
    movl $0x1128, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiTransparentBlt@44
_NtGdiTransparentBlt@44:
    movl $0x1129, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x2c

.global _NtGdiUnloadPrinterDriver@8
_NtGdiUnloadPrinterDriver@8:
    movl $0x112a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiUnmapMemFont@4
_NtGdiUnmapMemFont@4:
    movl $0x112b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiUnrealizeObject@4
_NtGdiUnrealizeObject@4:
    movl $0x112c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiUpdateColors@4
_NtGdiUpdateColors@4:
    movl $0x112d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiWidenPath@4
_NtGdiWidenPath@4:
    movl $0x112e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserActivateKeyboardLayout@8
_NtUserActivateKeyboardLayout@8:
    movl $0x112f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserAlterWindowStyle@12
_NtUserAlterWindowStyle@12:
    movl $0x1130, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserAssociateInputContext@12
_NtUserAssociateInputContext@12:
    movl $0x1131, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserAttachThreadInput@12
_NtUserAttachThreadInput@12:
    movl $0x1132, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserBeginPaint@8
_NtUserBeginPaint@8:
    movl $0x1133, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserBitBltSysBmp@32
_NtUserBitBltSysBmp@32:
    movl $0x1134, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtUserBlockInput@4
_NtUserBlockInput@4:
    movl $0x1135, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserBuildHimcList@16
_NtUserBuildHimcList@16:
    movl $0x1136, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserBuildHwndList@28
_NtUserBuildHwndList@28:
    movl $0x1137, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtUserBuildNameList@16
_NtUserBuildNameList@16:
    movl $0x1138, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserBuildPropList@16
_NtUserBuildPropList@16:
    movl $0x1139, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserCallHwnd@8
_NtUserCallHwnd@8:
    movl $0x113a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserCallHwndLock@8
_NtUserCallHwndLock@8:
    movl $0x113b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserCallHwndOpt@8
_NtUserCallHwndOpt@8:
    movl $0x113c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserCallHwndParam@12
_NtUserCallHwndParam@12:
    movl $0x113d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserCallHwndParamLock@12
_NtUserCallHwndParamLock@12:
    movl $0x113e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserCallMsgFilter@8
_NtUserCallMsgFilter@8:
    movl $0x113f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserCallNextHookEx@16
_NtUserCallNextHookEx@16:
    movl $0x1140, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserCallNoParam@4
_NtUserCallNoParam@4:
    movl $0x1141, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserCallOneParam@8
_NtUserCallOneParam@8:
    movl $0x1142, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserCallTwoParam@12
_NtUserCallTwoParam@12:
    movl $0x1143, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserChangeClipboardChain@8
_NtUserChangeClipboardChain@8:
    movl $0x1144, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserChangeDisplaySettings@16
_NtUserChangeDisplaySettings@16:
    movl $0x1145, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserCheckImeHotKey@8
_NtUserCheckImeHotKey@8:
    movl $0x1146, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserCheckMenuItem@12
_NtUserCheckMenuItem@12:
    movl $0x1147, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserChildWindowFromPointEx@16
_NtUserChildWindowFromPointEx@16:
    movl $0x1148, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserClipCursor@4
_NtUserClipCursor@4:
    movl $0x1149, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserCloseClipboard@0
_NtUserCloseClipboard@0:
    movl $0x114a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserCloseDesktop@4
_NtUserCloseDesktop@4:
    movl $0x114b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserCloseWindowStation@4
_NtUserCloseWindowStation@4:
    movl $0x114c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserConsoleControl@12
_NtUserConsoleControl@12:
    movl $0x114d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserConvertMemHandle@8
_NtUserConvertMemHandle@8:
    movl $0x114e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserCopyAcceleratorTable@12
_NtUserCopyAcceleratorTable@12:
    movl $0x114f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserCountClipboardFormats@0
_NtUserCountClipboardFormats@0:
    movl $0x1150, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserCreateAcceleratorTable@8
_NtUserCreateAcceleratorTable@8:
    movl $0x1151, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserCreateCaret@16
_NtUserCreateCaret@16:
    movl $0x1152, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserCreateDesktop@20
_NtUserCreateDesktop@20:
    movl $0x1153, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtUserCreateInputContext@4
_NtUserCreateInputContext@4:
    movl $0x1154, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserCreateLocalMemHandle@16
_NtUserCreateLocalMemHandle@16:
    movl $0x1155, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserCreateWindowEx@60
_NtUserCreateWindowEx@60:
    movl $0x1156, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x3c

.global _NtUserCreateWindowStation@28
_NtUserCreateWindowStation@28:
    movl $0x1157, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtUserDdeGetQualityOfService@12
_NtUserDdeGetQualityOfService@12:
    movl $0x1158, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserDdeInitialize@20
_NtUserDdeInitialize@20:
    movl $0x1159, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtUserDdeSetQualityOfService@12
_NtUserDdeSetQualityOfService@12:
    movl $0x115a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserDeferWindowPos@32
_NtUserDeferWindowPos@32:
    movl $0x115b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtUserDefSetText@8
_NtUserDefSetText@8:
    movl $0x115c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserDeleteMenu@12
_NtUserDeleteMenu@12:
    movl $0x115d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserDestroyAcceleratorTable@4
_NtUserDestroyAcceleratorTable@4:
    movl $0x115e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserDestroyCursor@8
_NtUserDestroyCursor@8:
    movl $0x115f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserDestroyInputContext@4
_NtUserDestroyInputContext@4:
    movl $0x1160, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserDestroyMenu@4
_NtUserDestroyMenu@4:
    movl $0x1161, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserDestroyWindow@4
_NtUserDestroyWindow@4:
    movl $0x1162, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserDisableThreadIme@4
_NtUserDisableThreadIme@4:
    movl $0x1163, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserDispatchMessage@4
_NtUserDispatchMessage@4:
    movl $0x1164, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserDragDetect@12
_NtUserDragDetect@12:
    movl $0x1165, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserDragObject@20
_NtUserDragObject@20:
    movl $0x1166, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtUserDrawAnimatedRects@16
_NtUserDrawAnimatedRects@16:
    movl $0x1167, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserDrawCaption@16
_NtUserDrawCaption@16:
    movl $0x1168, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserDrawCaptionTemp@28
_NtUserDrawCaptionTemp@28:
    movl $0x1169, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtUserDrawIconEx@44
_NtUserDrawIconEx@44:
    movl $0x116a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x2c

.global _NtUserDrawMenuBarTemp@20
_NtUserDrawMenuBarTemp@20:
    movl $0x116b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtUserEmptyClipboard@0
_NtUserEmptyClipboard@0:
    movl $0x116c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserEnableMenuItem@12
_NtUserEnableMenuItem@12:
    movl $0x116d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserEnableScrollBar@12
_NtUserEnableScrollBar@12:
    movl $0x116e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserEndDeferWindowPosEx@8
_NtUserEndDeferWindowPosEx@8:
    movl $0x116f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserEndMenu@0
_NtUserEndMenu@0:
    movl $0x1170, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserEndPaint@8
_NtUserEndPaint@8:
    movl $0x1171, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserEnumDisplayDevices@16
_NtUserEnumDisplayDevices@16:
    movl $0x1172, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserEnumDisplayMonitors@16
_NtUserEnumDisplayMonitors@16:
    movl $0x1173, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserEnumDisplaySettings@16
_NtUserEnumDisplaySettings@16:
    movl $0x1174, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserEvent@4
_NtUserEvent@4:
    movl $0x1175, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserExcludeUpdateRgn@8
_NtUserExcludeUpdateRgn@8:
    movl $0x1176, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserFillWindow@16
_NtUserFillWindow@16:
    movl $0x1177, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserFindExistingCursorIcon@12
_NtUserFindExistingCursorIcon@12:
    movl $0x1178, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserFindWindowEx@20
_NtUserFindWindowEx@20:
    movl $0x1179, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtUserFlashWindowEx@4
_NtUserFlashWindowEx@4:
    movl $0x117a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetAltTabInfo@24
_NtUserGetAltTabInfo@24:
    movl $0x117b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtUserGetAncestor@8
_NtUserGetAncestor@8:
    movl $0x117c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetAppImeLevel@4
_NtUserGetAppImeLevel@4:
    movl $0x117d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetAsyncKeyState@4
_NtUserGetAsyncKeyState@4:
    movl $0x117e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetAtomName@8
_NtUserGetAtomName@8:
    movl $0x117f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetCaretBlinkTime@0
_NtUserGetCaretBlinkTime@0:
    movl $0x1180, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserGetCaretPos@4
_NtUserGetCaretPos@4:
    movl $0x1181, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetClassInfoEx@20
_NtUserGetClassInfoEx@20:
    movl $0x1182, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtUserGetClassName@12
_NtUserGetClassName@12:
    movl $0x1183, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetClipboardData@8
_NtUserGetClipboardData@8:
    movl $0x1184, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetClipboardFormatName@12
_NtUserGetClipboardFormatName@12:
    movl $0x1185, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetClipboardOwner@0
_NtUserGetClipboardOwner@0:
    movl $0x1186, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserGetClipboardSequenceNumber@0
_NtUserGetClipboardSequenceNumber@0:
    movl $0x1187, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserGetClipboardViewer@0
_NtUserGetClipboardViewer@0:
    movl $0x1188, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserGetClipCursor@4
_NtUserGetClipCursor@4:
    movl $0x1189, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetComboBoxInfo@8
_NtUserGetComboBoxInfo@8:
    movl $0x118a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetControlBrush@12
_NtUserGetControlBrush@12:
    movl $0x118b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetControlColor@16
_NtUserGetControlColor@16:
    movl $0x118c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserGetCPD@12
_NtUserGetCPD@12:
    movl $0x118d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetCursorFrameInfo@16
_NtUserGetCursorFrameInfo@16:
    movl $0x118e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserGetCursorInfo@4
_NtUserGetCursorInfo@4:
    movl $0x118f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetDC@4
_NtUserGetDC@4:
    movl $0x1190, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetDCEx@12
_NtUserGetDCEx@12:
    movl $0x1191, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetDoubleClickTime@0
_NtUserGetDoubleClickTime@0:
    movl $0x1192, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserGetForegroundWindow@0
_NtUserGetForegroundWindow@0:
    movl $0x1193, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserGetGuiResources@8
_NtUserGetGuiResources@8:
    movl $0x1194, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetGUIThreadInfo@8
_NtUserGetGUIThreadInfo@8:
    movl $0x1195, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetIconInfo@24
_NtUserGetIconInfo@24:
    movl $0x1196, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtUserGetIconSize@16
_NtUserGetIconSize@16:
    movl $0x1197, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserGetImeHotKey@16
_NtUserGetImeHotKey@16:
    movl $0x1198, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserGetImeInfoEx@8
_NtUserGetImeInfoEx@8:
    movl $0x1199, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetInternalWindowPos@12
_NtUserGetInternalWindowPos@12:
    movl $0x119a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetKeyboardLayoutList@8
_NtUserGetKeyboardLayoutList@8:
    movl $0x119b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetKeyboardLayoutName@4
_NtUserGetKeyboardLayoutName@4:
    movl $0x119c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetKeyboardState@4
_NtUserGetKeyboardState@4:
    movl $0x119d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetKeyNameText@12
_NtUserGetKeyNameText@12:
    movl $0x119e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetKeyState@4
_NtUserGetKeyState@4:
    movl $0x119f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetListBoxInfo@4
_NtUserGetListBoxInfo@4:
    movl $0x11a0, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetMenuBarInfo@16
_NtUserGetMenuBarInfo@16:
    movl $0x11a1, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserGetMenuIndex@8
_NtUserGetMenuIndex@8:
    movl $0x11a2, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetMenuItemRect@16
_NtUserGetMenuItemRect@16:
    movl $0x11a3, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserGetMessage@16
_NtUserGetMessage@16:
    movl $0x11a4, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserGetMouseMovePointsEx@20
_NtUserGetMouseMovePointsEx@20:
    movl $0x11a5, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtUserGetObjectInformation@20
_NtUserGetObjectInformation@20:
    movl $0x11a6, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtUserGetOpenClipboardWindow@0
_NtUserGetOpenClipboardWindow@0:
    movl $0x11a7, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserGetPriorityClipboardFormat@8
_NtUserGetPriorityClipboardFormat@8:
    movl $0x11a8, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetProcessWindowStation@0
_NtUserGetProcessWindowStation@0:
    movl $0x11a9, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserGetRawInputBuffer@12
_NtUserGetRawInputBuffer@12:
    movl $0x11aa, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetRawInputData@20
_NtUserGetRawInputData@20:
    movl $0x11ab, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtUserGetRawInputDeviceInfo@16
_NtUserGetRawInputDeviceInfo@16:
    movl $0x11ac, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserGetRawInputDeviceList@12
_NtUserGetRawInputDeviceList@12:
    movl $0x11ad, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetRegisteredRawInputDevices@12
_NtUserGetRegisteredRawInputDevices@12:
    movl $0x11ae, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetScrollBarInfo@12
_NtUserGetScrollBarInfo@12:
    movl $0x11af, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetSystemMenu@8
_NtUserGetSystemMenu@8:
    movl $0x11b0, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetThreadDesktop@8
_NtUserGetThreadDesktop@8:
    movl $0x11b1, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetThreadState@4
_NtUserGetThreadState@4:
    movl $0x11b2, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetTitleBarInfo@8
_NtUserGetTitleBarInfo@8:
    movl $0x11b3, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetUpdateRect@12
_NtUserGetUpdateRect@12:
    movl $0x11b4, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetUpdateRgn@12
_NtUserGetUpdateRgn@12:
    movl $0x11b5, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserGetWindowDC@4
_NtUserGetWindowDC@4:
    movl $0x11b6, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserGetWindowPlacement@8
_NtUserGetWindowPlacement@8:
    movl $0x11b7, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserGetWOWClass@8
_NtUserGetWOWClass@8:
    movl $0x11b8, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserHardErrorControl@12
_NtUserHardErrorControl@12:
    movl $0x11b9, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserHideCaret@4
_NtUserHideCaret@4:
    movl $0x11ba, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserHiliteMenuItem@16
_NtUserHiliteMenuItem@16:
    movl $0x11bb, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserImpersonateDdeClientWindow@8
_NtUserImpersonateDdeClientWindow@8:
    movl $0x11bc, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserInitialize@12
_NtUserInitialize@12:
    movl $0x11bd, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserInitializeClientPfnArrays@16
_NtUserInitializeClientPfnArrays@16:
    movl $0x11be, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserInitTask@48
_NtUserInitTask@48:
    movl $0x11bf, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x30

.global _NtUserInternalGetWindowText@12
_NtUserInternalGetWindowText@12:
    movl $0x11c0, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserInvalidateRect@12
_NtUserInvalidateRect@12:
    movl $0x11c1, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserInvalidateRgn@12
_NtUserInvalidateRgn@12:
    movl $0x11c2, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserIsClipboardFormatAvailable@4
_NtUserIsClipboardFormatAvailable@4:
    movl $0x11c3, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserKillTimer@8
_NtUserKillTimer@8:
    movl $0x11c4, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserLoadKeyboardLayoutEx@28
_NtUserLoadKeyboardLayoutEx@28:
    movl $0x11c5, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtUserLockWindowStation@4
_NtUserLockWindowStation@4:
    movl $0x11c6, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserLockWindowUpdate@4
_NtUserLockWindowUpdate@4:
    movl $0x11c7, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserLockWorkStation@0
_NtUserLockWorkStation@0:
    movl $0x11c8, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserMapVirtualKeyEx@16
_NtUserMapVirtualKeyEx@16:
    movl $0x11c9, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserMenuItemFromPoint@16
_NtUserMenuItemFromPoint@16:
    movl $0x11ca, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserMessageCall@28
_NtUserMessageCall@28:
    movl $0x11cb, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtUserMinMaximize@12
_NtUserMinMaximize@12:
    movl $0x11cc, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserMNDragLeave@0
_NtUserMNDragLeave@0:
    movl $0x11cd, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserMNDragOver@8
_NtUserMNDragOver@8:
    movl $0x11ce, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserModifyUserStartupInfoFlags@8
_NtUserModifyUserStartupInfoFlags@8:
    movl $0x11cf, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserMoveWindow@24
_NtUserMoveWindow@24:
    movl $0x11d0, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtUserNotifyIMEStatus@12
_NtUserNotifyIMEStatus@12:
    movl $0x11d1, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserNotifyProcessCreate@16
_NtUserNotifyProcessCreate@16:
    movl $0x11d2, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserNotifyWinEvent@16
_NtUserNotifyWinEvent@16:
    movl $0x11d3, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserOpenClipboard@8
_NtUserOpenClipboard@8:
    movl $0x11d4, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserOpenDesktop@12
_NtUserOpenDesktop@12:
    movl $0x11d5, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserOpenInputDesktop@12
_NtUserOpenInputDesktop@12:
    movl $0x11d6, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserOpenWindowStation@8
_NtUserOpenWindowStation@8:
    movl $0x11d7, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserPaintDesktop@4
_NtUserPaintDesktop@4:
    movl $0x11d8, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserPeekMessage@20
_NtUserPeekMessage@20:
    movl $0x11d9, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtUserPostMessage@16
_NtUserPostMessage@16:
    movl $0x11da, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserPostThreadMessage@16
_NtUserPostThreadMessage@16:
    movl $0x11db, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserPrintWindow@12
_NtUserPrintWindow@12:
    movl $0x11dc, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserProcessConnect@12
_NtUserProcessConnect@12:
    movl $0x11dd, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserQueryInformationThread@16
_NtUserQueryInformationThread@16:
    movl $0x11de, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserQueryInputContext@8
_NtUserQueryInputContext@8:
    movl $0x11df, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserQuerySendMessage@4
_NtUserQuerySendMessage@4:
    movl $0x11e0, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserQueryWindow@8
_NtUserQueryWindow@8:
    movl $0x11e1, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserRealChildWindowFromPoint@12
_NtUserRealChildWindowFromPoint@12:
    movl $0x11e2, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserRealInternalGetMessage@24
_NtUserRealInternalGetMessage@24:
    movl $0x11e3, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtUserRealWaitMessageEx@8
_NtUserRealWaitMessageEx@8:
    movl $0x11e4, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserRedrawWindow@16
_NtUserRedrawWindow@16:
    movl $0x11e5, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserRegisterClassExWOW@28
_NtUserRegisterClassExWOW@28:
    movl $0x11e6, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtUserRegisterUserApiHook@16
_NtUserRegisterUserApiHook@16:
    movl $0x11e7, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserRegisterHotKey@16
_NtUserRegisterHotKey@16:
    movl $0x11e8, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserRegisterRawInputDevices@12
_NtUserRegisterRawInputDevices@12:
    movl $0x11e9, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserRegisterTasklist@4
_NtUserRegisterTasklist@4:
    movl $0x11ea, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserRegisterWindowMessage@4
_NtUserRegisterWindowMessage@4:
    movl $0x11eb, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserRemoveMenu@12
_NtUserRemoveMenu@12:
    movl $0x11ec, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserRemoveProp@8
_NtUserRemoveProp@8:
    movl $0x11ed, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserResolveDesktop@16
_NtUserResolveDesktop@16:
    movl $0x11ee, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserResolveDesktopForWOW@4
_NtUserResolveDesktopForWOW@4:
    movl $0x11ef, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSBGetParms@16
_NtUserSBGetParms@16:
    movl $0x11f0, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserScrollDC@28
_NtUserScrollDC@28:
    movl $0x11f1, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtUserScrollWindowEx@32
_NtUserScrollWindowEx@32:
    movl $0x11f2, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtUserSelectPalette@12
_NtUserSelectPalette@12:
    movl $0x11f3, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserSendInput@12
_NtUserSendInput@12:
    movl $0x11f4, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserSetActiveWindow@4
_NtUserSetActiveWindow@4:
    movl $0x11f5, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSetAppImeLevel@8
_NtUserSetAppImeLevel@8:
    movl $0x11f6, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetCapture@4
_NtUserSetCapture@4:
    movl $0x11f7, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSetClassLong@16
_NtUserSetClassLong@16:
    movl $0x11f8, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetClassWord@12
_NtUserSetClassWord@12:
    movl $0x11f9, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserSetClipboardData@12
_NtUserSetClipboardData@12:
    movl $0x11fa, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserSetClipboardViewer@4
_NtUserSetClipboardViewer@4:
    movl $0x11fb, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSetConsoleReserveKeys@8
_NtUserSetConsoleReserveKeys@8:
    movl $0x11fc, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetCursor@4
_NtUserSetCursor@4:
    movl $0x11fd, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSetCursorContents@8
_NtUserSetCursorContents@8:
    movl $0x11fe, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetCursorIconData@16
_NtUserSetCursorIconData@16:
    movl $0x11ff, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetFocus@4
_NtUserSetFocus@4:
    movl $0x1200, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSetImeHotKey@20
_NtUserSetImeHotKey@20:
    movl $0x1201, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtUserSetImeInfoEx@4
_NtUserSetImeInfoEx@4:
    movl $0x1202, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSetImeOwnerWindow@8
_NtUserSetImeOwnerWindow@8:
    movl $0x1203, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetInformationProcess@16
_NtUserSetInformationProcess@16:
    movl $0x1204, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetInformationThread@16
_NtUserSetInformationThread@16:
    movl $0x1205, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetInternalWindowPos@16
_NtUserSetInternalWindowPos@16:
    movl $0x1206, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetKeyboardState@4
_NtUserSetKeyboardState@4:
    movl $0x1207, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSetLogonNotifyWindow@4
_NtUserSetLogonNotifyWindow@4:
    movl $0x1208, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSetMenu@12
_NtUserSetMenu@12:
    movl $0x1209, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserSetMenuContextHelpId@8
_NtUserSetMenuContextHelpId@8:
    movl $0x120a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetMenuDefaultItem@12
_NtUserSetMenuDefaultItem@12:
    movl $0x120b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserSetMenuFlagRtoL@4
_NtUserSetMenuFlagRtoL@4:
    movl $0x120c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSetObjectInformation@16
_NtUserSetObjectInformation@16:
    movl $0x120d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetParent@8
_NtUserSetParent@8:
    movl $0x120e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetProcessWindowStation@4
_NtUserSetProcessWindowStation@4:
    movl $0x120f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSetProp@12
_NtUserSetProp@12:
    movl $0x1210, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserSetScrollInfo@16
_NtUserSetScrollInfo@16:
    movl $0x1211, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetShellWindowEx@8
_NtUserSetShellWindowEx@8:
    movl $0x1212, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetSysColors@16
_NtUserSetSysColors@16:
    movl $0x1213, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetSystemCursor@8
_NtUserSetSystemCursor@8:
    movl $0x1214, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetSystemMenu@8
_NtUserSetSystemMenu@8:
    movl $0x1215, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetSystemTimer@16
_NtUserSetSystemTimer@16:
    movl $0x1216, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetThreadDesktop@4
_NtUserSetThreadDesktop@4:
    movl $0x1217, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSetThreadLayoutHandles@8
_NtUserSetThreadLayoutHandles@8:
    movl $0x1218, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetThreadState@8
_NtUserSetThreadState@8:
    movl $0x1219, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetTimer@16
_NtUserSetTimer@16:
    movl $0x121a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetWindowFNID@8
_NtUserSetWindowFNID@8:
    movl $0x121b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetWindowLong@16
_NtUserSetWindowLong@16:
    movl $0x121c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetWindowPlacement@8
_NtUserSetWindowPlacement@8:
    movl $0x121d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSetWindowPos@28
_NtUserSetWindowPos@28:
    movl $0x121e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtUserSetWindowRgn@12
_NtUserSetWindowRgn@12:
    movl $0x121f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserSetWindowsHookAW@12
_NtUserSetWindowsHookAW@12:
    movl $0x1220, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserSetWindowsHookEx@24
_NtUserSetWindowsHookEx@24:
    movl $0x1221, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtUserSetWindowStationUser@16
_NtUserSetWindowStationUser@16:
    movl $0x1222, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetWindowWord@12
_NtUserSetWindowWord@12:
    movl $0x1223, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserSetWinEventHook@32
_NtUserSetWinEventHook@32:
    movl $0x1224, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtUserShowCaret@4
_NtUserShowCaret@4:
    movl $0x1225, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserShowScrollBar@12
_NtUserShowScrollBar@12:
    movl $0x1226, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserShowWindow@8
_NtUserShowWindow@8:
    movl $0x1227, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserShowWindowAsync@8
_NtUserShowWindowAsync@8:
    movl $0x1228, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserSoundSentry@0
_NtUserSoundSentry@0:
    movl $0x1229, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserSwitchDesktop@4
_NtUserSwitchDesktop@4:
    movl $0x122a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserSystemParametersInfo@16
_NtUserSystemParametersInfo@16:
    movl $0x122b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserTestForInteractiveUser@4
_NtUserTestForInteractiveUser@4:
    movl $0x122c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserThunkedMenuInfo@8
_NtUserThunkedMenuInfo@8:
    movl $0x122d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserThunkedMenuItemInfo@24
_NtUserThunkedMenuItemInfo@24:
    movl $0x122e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtUserToUnicodeEx@28
_NtUserToUnicodeEx@28:
    movl $0x122f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtUserTrackMouseEvent@4
_NtUserTrackMouseEvent@4:
    movl $0x1230, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserTrackPopupMenuEx@24
_NtUserTrackPopupMenuEx@24:
    movl $0x1231, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtUserCalcMenuBar@20
_NtUserCalcMenuBar@20:
    movl $0x1232, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtUserPaintMenuBar@24
_NtUserPaintMenuBar@24:
    movl $0x1233, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtUserTranslateAccelerator@12
_NtUserTranslateAccelerator@12:
    movl $0x1234, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserTranslateMessage@8
_NtUserTranslateMessage@8:
    movl $0x1235, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserUnhookWindowsHookEx@4
_NtUserUnhookWindowsHookEx@4:
    movl $0x1236, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserUnhookWinEvent@4
_NtUserUnhookWinEvent@4:
    movl $0x1237, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserUnloadKeyboardLayout@4
_NtUserUnloadKeyboardLayout@4:
    movl $0x1238, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserUnlockWindowStation@4
_NtUserUnlockWindowStation@4:
    movl $0x1239, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserUnregisterClass@12
_NtUserUnregisterClass@12:
    movl $0x123a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserUnregisterUserApiHook@0
_NtUserUnregisterUserApiHook@0:
    movl $0x123b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserUnregisterHotKey@8
_NtUserUnregisterHotKey@8:
    movl $0x123c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserUpdateInputContext@12
_NtUserUpdateInputContext@12:
    movl $0x123d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserUpdateInstance@12
_NtUserUpdateInstance@12:
    movl $0x123e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserUpdateLayeredWindow@40
_NtUserUpdateLayeredWindow@40:
    movl $0x123f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x28

.global _NtUserGetLayeredWindowAttributes@16
_NtUserGetLayeredWindowAttributes@16:
    movl $0x1240, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserSetLayeredWindowAttributes@16
_NtUserSetLayeredWindowAttributes@16:
    movl $0x1241, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserUpdatePerUserSystemParameters@8
_NtUserUpdatePerUserSystemParameters@8:
    movl $0x1242, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserUserHandleGrantAccess@12
_NtUserUserHandleGrantAccess@12:
    movl $0x1243, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserValidateHandleSecure@4
_NtUserValidateHandleSecure@4:
    movl $0x1244, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserValidateRect@8
_NtUserValidateRect@8:
    movl $0x1245, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserValidateTimerCallback@4
_NtUserValidateTimerCallback@4:
    movl $0x1246, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserVkKeyScanEx@12
_NtUserVkKeyScanEx@12:
    movl $0x1247, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserWaitForInputIdle@12
_NtUserWaitForInputIdle@12:
    movl $0x1248, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserWaitForMsgAndEvent@4
_NtUserWaitForMsgAndEvent@4:
    movl $0x1249, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtUserWaitMessage@0
_NtUserWaitMessage@0:
    movl $0x124a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserWin32PoolAllocationStats@24
_NtUserWin32PoolAllocationStats@24:
    movl $0x124b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtUserWindowFromPoint@8
_NtUserWindowFromPoint@8:
    movl $0x124c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtUserYieldTask@0
_NtUserYieldTask@0:
    movl $0x124d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserRemoteConnect@12
_NtUserRemoteConnect@12:
    movl $0x124e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtUserRemoteRedrawRectangle@16
_NtUserRemoteRedrawRectangle@16:
    movl $0x124f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtUserRemoteRedrawScreen@0
_NtUserRemoteRedrawScreen@0:
    movl $0x1250, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserRemoteStopScreenUpdates@0
_NtUserRemoteStopScreenUpdates@0:
    movl $0x1251, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtUserCtxDisplayIOCtl@12
_NtUserCtxDisplayIOCtl@12:
    movl $0x1252, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiEngAssociateSurface@12
_NtGdiEngAssociateSurface@12:
    movl $0x1253, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiEngCreateBitmap@24
_NtGdiEngCreateBitmap@24:
    movl $0x1254, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiEngCreateDeviceSurface@16
_NtGdiEngCreateDeviceSurface@16:
    movl $0x1255, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiEngCreateDeviceBitmap@16
_NtGdiEngCreateDeviceBitmap@16:
    movl $0x1256, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiEngCreatePalette@24
_NtGdiEngCreatePalette@24:
    movl $0x1257, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiEngComputeGlyphSet@12
_NtGdiEngComputeGlyphSet@12:
    movl $0x1258, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiEngCopyBits@24
_NtGdiEngCopyBits@24:
    movl $0x1259, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiEngDeletePalette@4
_NtGdiEngDeletePalette@4:
    movl $0x125a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEngDeleteSurface@4
_NtGdiEngDeleteSurface@4:
    movl $0x125b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEngEraseSurface@12
_NtGdiEngEraseSurface@12:
    movl $0x125c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiEngUnlockSurface@4
_NtGdiEngUnlockSurface@4:
    movl $0x125d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEngLockSurface@4
_NtGdiEngLockSurface@4:
    movl $0x125e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEngBitBlt@44
_NtGdiEngBitBlt@44:
    movl $0x125f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x2c

.global _NtGdiEngStretchBlt@44
_NtGdiEngStretchBlt@44:
    movl $0x1260, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x2c

.global _NtGdiEngPlgBlt@44
_NtGdiEngPlgBlt@44:
    movl $0x1261, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x2c

.global _NtGdiEngMarkBandingSurface@4
_NtGdiEngMarkBandingSurface@4:
    movl $0x1262, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEngStrokePath@32
_NtGdiEngStrokePath@32:
    movl $0x1263, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtGdiEngFillPath@28
_NtGdiEngFillPath@28:
    movl $0x1264, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtGdiEngStrokeAndFillPath@40
_NtGdiEngStrokeAndFillPath@40:
    movl $0x1265, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x28

.global _NtGdiEngPaint@20
_NtGdiEngPaint@20:
    movl $0x1266, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiEngLineTo@36
_NtGdiEngLineTo@36:
    movl $0x1267, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x24

.global _NtGdiEngAlphaBlend@28
_NtGdiEngAlphaBlend@28:
    movl $0x1268, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x1c

.global _NtGdiEngGradientFill@40
_NtGdiEngGradientFill@40:
    movl $0x1269, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x28

.global _NtGdiEngTransparentBlt@32
_NtGdiEngTransparentBlt@32:
    movl $0x126a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x20

.global _NtGdiEngTextOut@40
_NtGdiEngTextOut@40:
    movl $0x126b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x28

.global _NtGdiEngStretchBltROP@52
_NtGdiEngStretchBltROP@52:
    movl $0x126c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x34

.global _NtGdiXLATEOBJ_cGetPalette@16
_NtGdiXLATEOBJ_cGetPalette@16:
    movl $0x126d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiXLATEOBJ_iXlate@8
_NtGdiXLATEOBJ_iXlate@8:
    movl $0x126e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiXLATEOBJ_hGetColorTransform@4
_NtGdiXLATEOBJ_hGetColorTransform@4:
    movl $0x126f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiCLIPOBJ_bEnum@12
_NtGdiCLIPOBJ_bEnum@12:
    movl $0x1270, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiCLIPOBJ_cEnumStart@20
_NtGdiCLIPOBJ_cEnumStart@20:
    movl $0x1271, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiCLIPOBJ_ppoGetPath@4
_NtGdiCLIPOBJ_ppoGetPath@4:
    movl $0x1272, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEngDeletePath@4
_NtGdiEngDeletePath@4:
    movl $0x1273, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEngCreateClip@0
_NtGdiEngCreateClip@0:
    movl $0x1274, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x0

.global _NtGdiEngDeleteClip@4
_NtGdiEngDeleteClip@4:
    movl $0x1275, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiBRUSHOBJ_ulGetBrushColor@4
_NtGdiBRUSHOBJ_ulGetBrushColor@4:
    movl $0x1276, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiBRUSHOBJ_pvAllocRbrush@8
_NtGdiBRUSHOBJ_pvAllocRbrush@8:
    movl $0x1277, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiBRUSHOBJ_pvGetRbrush@4
_NtGdiBRUSHOBJ_pvGetRbrush@4:
    movl $0x1278, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiBRUSHOBJ_hGetColorTransform@4
_NtGdiBRUSHOBJ_hGetColorTransform@4:
    movl $0x1279, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiXFORMOBJ_bApplyXform@20
_NtGdiXFORMOBJ_bApplyXform@20:
    movl $0x127a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiXFORMOBJ_iGetXform@8
_NtGdiXFORMOBJ_iGetXform@8:
    movl $0x127b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiFONTOBJ_vGetInfo@12
_NtGdiFONTOBJ_vGetInfo@12:
    movl $0x127c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiFONTOBJ_pxoGetXform@4
_NtGdiFONTOBJ_pxoGetXform@4:
    movl $0x127d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiFONTOBJ_cGetGlyphs@20
_NtGdiFONTOBJ_cGetGlyphs@20:
    movl $0x127e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x14

.global _NtGdiFONTOBJ_pifi@4
_NtGdiFONTOBJ_pifi@4:
    movl $0x127f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiFONTOBJ_pfdg@4
_NtGdiFONTOBJ_pfdg@4:
    movl $0x1280, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiFONTOBJ_pQueryGlyphAttrs@8
_NtGdiFONTOBJ_pQueryGlyphAttrs@8:
    movl $0x1281, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiFONTOBJ_pvTrueTypeFontFile@8
_NtGdiFONTOBJ_pvTrueTypeFontFile@8:
    movl $0x1282, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiFONTOBJ_cGetAllGlyphHandles@8
_NtGdiFONTOBJ_cGetAllGlyphHandles@8:
    movl $0x1283, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiSTROBJ_bEnum@12
_NtGdiSTROBJ_bEnum@12:
    movl $0x1284, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiSTROBJ_bEnumPositionsOnly@12
_NtGdiSTROBJ_bEnumPositionsOnly@12:
    movl $0x1285, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiSTROBJ_bGetAdvanceWidths@16
_NtGdiSTROBJ_bGetAdvanceWidths@16:
    movl $0x1286, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiSTROBJ_vEnumStart@4
_NtGdiSTROBJ_vEnumStart@4:
    movl $0x1287, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiSTROBJ_dwGetCodePage@4
_NtGdiSTROBJ_dwGetCodePage@4:
    movl $0x1288, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiPATHOBJ_vGetBounds@8
_NtGdiPATHOBJ_vGetBounds@8:
    movl $0x1289, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiPATHOBJ_bEnum@8
_NtGdiPATHOBJ_bEnum@8:
    movl $0x128a, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiPATHOBJ_vEnumStart@4
_NtGdiPATHOBJ_vEnumStart@4:
    movl $0x128b, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiPATHOBJ_vEnumStartClipLines@16
_NtGdiPATHOBJ_vEnumStartClipLines@16:
    movl $0x128c, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiPATHOBJ_bEnumClipLines@12
_NtGdiPATHOBJ_bEnumClipLines@12:
    movl $0x128d, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _NtGdiGetDhpdev@4
_NtGdiGetDhpdev@4:
    movl $0x128e, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiEngCheckAbort@4
_NtGdiEngCheckAbort@4:
    movl $0x128f, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiHT_Get8BPPFormatPalette@16
_NtGdiHT_Get8BPPFormatPalette@16:
    movl $0x1290, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiHT_Get8BPPMaskPalette@24
_NtGdiHT_Get8BPPMaskPalette@24:
    movl $0x1291, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x18

.global _NtGdiUpdateTransform@4
_NtGdiUpdateTransform@4:
    movl $0x1292, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiSetPUMPDOBJ@16
_NtGdiSetPUMPDOBJ@16:
    movl $0x1293, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x10

.global _NtGdiBRUSHOBJ_DeleteRbrush@8
_NtGdiBRUSHOBJ_DeleteRbrush@8:
    movl $0x1294, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _NtGdiUMPDEngFreeUserMem@4
_NtGdiUMPDEngFreeUserMem@4:
    movl $0x1295, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

.global _NtGdiDrawStream@12
_NtGdiDrawStream@12:
    movl $0x1296, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0xc

.global _UMPDDrvQuerySpoolType@8
_UMPDDrvQuerySpoolType@8:
    movl $0x1297, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x8

.global _DefaultHTCallBack@4
_DefaultHTCallBack@4:
    movl $0x1298, %eax
    movl $KUSER_SHARED_SYSCALL, %ecx
    call *(%ecx)
    ret $0x4

