

/* win32k driver functions table  it have created for the engine the DxEng* api */


/* DXG.SYS FUNCTIONS INDEX */
/***************************************************************************/
/* This driver functions are exported raw from NtGdiDd* / NtDvp* / NtD3d*  */
/***************************************************************************/
#define DXG_INDEX_DxDxgGenericThunk               0x00
#define DXG_INDEX_DxD3dContextCreate              0x01
#define DXG_INDEX_DxD3dContextDestroy             0x02
#define DXG_INDEX_DxD3dContextDestroyAll          0x03
#define DXG_INDEX_DxD3dValidateTextureStageState  0x04
#define DXG_INDEX_DxD3dDrawPrimitives2            0x05
#define DXG_INDEX_DxDdGetDriverState              0x06
#define DXG_INDEX_DxDdAddAttachedSurface          0x07
#define DXG_INDEX_DxDdAlphaBlt                    0x08
#define DXG_INDEX_DxDdAttachSurface               0x09
#define DXG_INDEX_DxDdBeginMoCompFrame            0x0A
#define DXG_INDEX_DxDdBlt                         0x0B
#define DXG_INDEX_DxDdCanCreateSurface            0x0C
#define DXG_INDEX_DxDdCanCreateD3DBuffer          0x0D
#define DXG_INDEX_DxDdColorControl                0x0E
#define DXG_INDEX_DxDdCreateDirectDrawObject      0x0F
/* DXG_INDEX_DxDdCreateSurface and  DXG_INDEX_DxDdCreateD3DBuffer2 are same */
#define DXG_INDEX_DxDdCreateSurface               0x10
#define DXG_INDEX_DxDdCreateD3DBuffer             0x11
#define DXG_INDEX_DxDdCreateMoComp                0x12
#define DXG_INDEX_DxDdCreateSurfaceObject         0x13
#define DXG_INDEX_DxDdDeleteDirectDrawObject      0x14
#define DXG_INDEX_DxDdDeleteSurfaceObject         0x15
#define DXG_INDEX_DxDdDestroyMoComp               0x16
#define DXG_INDEX_DxDdDestroySurface              0x17
#define DXG_INDEX_DxDdDestroyD3DBuffer            0x18
#define DXG_INDEX_DxDdEndMoCompFrame              0x19
#define DXG_INDEX_DxDdFlip                        0x1A
#define DXG_INDEX_DxDdFlipToGDISurface            0x1B
#define DXG_INDEX_DxDdGetAvailDriverMemory        0x1C
#define DXG_INDEX_DxDdGetBltStatus                0x1D
#define DXG_INDEX_DxDdGetDC                       0x1E
#define DXG_INDEX_DxDdGetDriverInfo               0x1F
#define DXG_INDEX_DxDdGetDxHandle                 0x20
#define DXG_INDEX_DxDdGetFlipStatus               0x21
#define DXG_INDEX_DxDdGetInternalMoCompInfo       0x22
#define DXG_INDEX_DxDdGetMoCompBuffInfo           0x23
#define DXG_INDEX_DxDdGetMoCompGuids              0x24
#define DXG_INDEX_DxDdGetMoCompFormats            0x25
#define DXG_INDEX_DxDdGetScanLine                 0x26
/* DXG_INDEX_DxDdLock and  DXG_INDEX_DxDdLockD3D are same */
#define DXG_INDEX_DxDdLock                        0x27
#define DXG_INDEX_DxDdLockD3D                     0x28
#define DXG_INDEX_DxDdQueryDirectDrawObject       0x29
#define DXG_INDEX_DxDdQueryMoCompStatus           0x2A
#define DXG_INDEX_DxDdReenableDirectDrawObject    0x2B
#define DXG_INDEX_DxDdReleaseDC                   0x2C
#define DXG_INDEX_DxDdRenderMoComp                0x2D
#define DXG_INDEX_DxDdResetVisrgn                 0x2E
#define DXG_INDEX_DxDdSetColorKey                 0x2F
#define DXG_INDEX_DxDdSetExclusiveMode            0x30
#define DXG_INDEX_DxDdSetGammaRamp                0x31
#define DXG_INDEX_DxDdCreateSurfaceEx             0x32
#define DXG_INDEX_DxDdSetOverlayPosition          0x33
#define DXG_INDEX_DxDdUnattachSurface             0x34
/* DXG_INDEX_DxDdUnlock and  DXG_INDEX_DxDdUnlockD3D are same */
#define DXG_INDEX_DxDdUnlock                      0x35
#define DXG_INDEX_DxDdUnlockD3D                   0x36
#define DXG_INDEX_DxDdUpdateOverlay               0x37
#define DXG_INDEX_DxDdWaitForVerticalBlank        0x38
#define DXG_INDEX_DxDvpCanCreateVideoPort         0x39
#define DXG_INDEX_DxDvpColorControl               0x3A
#define DXG_INDEX_DxDvpCreateVideoPort            0x3B
#define DXG_INDEX_DxDvpDestroyVideoPort           0x3C
#define DXG_INDEX_DxDvpFlipVideoPort              0x3D
#define DXG_INDEX_DxDvpGetVideoPortBandwidth      0x3E
#define DXG_INDEX_DxDvpGetVideoPortField          0x3F
#define DXG_INDEX_DxDvpGetVideoPortFlipStatus     0x40
#define DXG_INDEX_DxDvpGetVideoPortInputFormats   0x41
#define DXG_INDEX_DxDvpGetVideoPortLine           0x42
#define DXG_INDEX_DxDvpGetVideoPortOutputFormats  0x43
#define DXG_INDEX_DxDvpGetVideoPortConnectInfo    0x44
#define DXG_INDEX_DxDvpGetVideoSignalStatus       0x45
#define DXG_INDEX_DxDvpUpdateVideoPort            0x46
#define DXG_INDEX_DxDvpWaitForVideoPortSync       0x47
#define DXG_INDEX_DxDvpAcquireNotification        0x48
#define DXG_INDEX_DxDvpReleaseNotification        0x49

/***********************************************************************************/
/* This driver functions are exported raw from Eng* it only exists in the def file */
/* you can not do syscallback to thuse but you can import them from win32k.sys     */
/* for them are in the export list                                                 */
/***********************************************************************************/
/* not addedd yet */
#define DXG_INDEX_DxDdHeapVidMemAllocAligned      0x4A
#define DXG_INDEX_DxDdHeapVidMemFree              0x4B
#define DXG_INDEX_DxDdAllocPrivateUserMem         0x54
#define DXG_INDEX_DxDdFreePrivateUserMem          0x55
#define DXG_INDEX_DxDdLockDirectDrawSurface       0x56
#define DXG_INDEX_DxDdUnlockDirectDrawSurface     0x57
#define DXG_INDEX_DxDdIoctl                       0x5B


/***********************************************************************************/
/* Internal use in diffent part in Windows and ReactOS                             */
/***********************************************************************************/
/* not inuse yet */
#define DXG_INDEX_DxDdEnableDirectDraw            0x4C
#define DXG_INDEX_DxDdDisableDirectDraw           0x4D
#define DXG_INDEX_DxDdSuspendDirectDraw           0x4E
#define DXG_INDEX_DxDdResumeDirectDraw            0x4F
#define DXG_INDEX_DxDdDynamicModeChange           0x50
#define DXG_INDEX_DxDdCloseProcess                0x51
#define DXG_INDEX_DxDdGetDirectDrawBound          0x52
#define DXG_INDEX_DxDdEnableDirectDrawRedirection 0x53
#define DXG_INDEX_DxDdSetAccelLevel               0x58
#define DXG_INDEX_DxDdGetSurfaceLock              0x59
#define DXG_INDEX_DxDdEnumLockedSurfaceRect       0x5A


/***********************************************************************************/
/* Driver Functions Protypes                                                       */
/***********************************************************************************/
typedef DWORD (NTAPI *PGD_DXDXGGENERICTHUNK)(ULONG_PTR, ULONG_PTR, SIZE_T *, PVOID, SIZE_T *, PVOID);
//typedef x (NTAPI *PGD_DxD3dContextCreate)(
//typedef x (NTAPI *PGD_DxD3dContextDestroy)(
//typedef x (NTAPI *PGD_DxD3dContextDestroyAll)(
//typedef x (NTAPI *PGD_DxD3dValidateTextureStageState)(
//typedef x (NTAPI *PGD_DxD3dDrawPrimitives2)(
//typedef x (NTAPI *PGD_DxDdGetDriverState)(
//typedef x (NTAPI *PGD_DxDdAddAttachedSurface)(
//typedef x (NTAPI *PGD_DxDdAlphaBlt)(
//typedef x (NTAPI *PGD_DxDdAttachSurface)(
//typedef x (NTAPI *PGD_DxDdBeginMoCompFrame)(
//typedef x (NTAPI *PGD_DxDdBlt)(
//typedef x (NTAPI *PGD_DxDdCanCreateSurface)(
//typedef x (NTAPI *PGD_DxDdCanCreateD3DBuffer)(
//typedef x (NTAPI *PGD_DxDdColorControl)(
//typedef x (NTAPI *PGD_DxDdCreateDirectDrawObject)(
//typedef x (NTAPI *PGD_DxDdCreateSurface)(
//typedef x (NTAPI *PGD_DxDdCreateD3DBuffer)(
//typedef x (NTAPI *PGD_DxDdCreateMoComp)(
//typedef x (NTAPI *PGD_DxDdCreateSurfaceObject)(
//typedef x (NTAPI *PGD_DxDdDeleteDirectDrawObject)(
//typedef x (NTAPI *PGD_DxDdDeleteSurfaceObject)(
//typedef x (NTAPI *PGD_DxDdDestroyMoComp)(
//typedef x (NTAPI *PGD_DxDdDestroySurface)(
//typedef x (NTAPI *PGD_DxDdDestroyD3DBuffer)(
//typedef x (NTAPI *PGD_DxDdEndMoCompFrame)(
//typedef x (NTAPI *PGD_DxDdFlip)(
//typedef x (NTAPI *PGD_DxDdFlipToGDISurface)(
//typedef x (NTAPI *PGD_DxDdGetAvailDriverMemory)(
//typedef x (NTAPI *PGD_DxDdGetBltStatus)(
//typedef x (NTAPI *PGD_DxDdGetDC)(
//typedef x (NTAPI *PGD_DxDdGetDriverInfo)(
//typedef x (NTAPI *PGD_DxDdGetDxHandle)(
//typedef x (NTAPI *PGD_DxDdGetFlipStatus)(
//typedef x (NTAPI *PGD_DxDdGetInternalMoCompInfo)(
//typedef x (NTAPI *PGD_DxDdGetMoCompBuffInfo)(
//typedef x (NTAPI *PGD_DxDdGetMoCompGuids)(
//typedef x (NTAPI *PGD_DxDdGetMoCompFormats)(
//typedef x (NTAPI *PGD_DxDdGetScanLine)(
//typedef x (NTAPI *PGD_DxDdLock)(
//typedef x (NTAPI *PGD_DxDdLockD3D)(
//typedef x (NTAPI *PGD_DxDdQueryDirectDrawObject)(
//typedef x (NTAPI *PGD_DxDdQueryMoCompStatus)(
//typedef x (NTAPI *PGD_DxDdReenableDirectDrawObject)(
//typedef x (NTAPI *PGD_DxDdReleaseDC)(
//typedef x (NTAPI *PGD_DxDdRenderMoComp)(
//typedef x (NTAPI *PGD_DxDdResetVisrgn)(
//typedef x (NTAPI *PGD_DxDdSetColorKey)(
//typedef x (NTAPI *PGD_DxDdSetExclusiveMode)(
//typedef x (NTAPI *PGD_DxDdSetGammaRamp)(
//typedef x (NTAPI *PGD_DxDdCreateSurfaceEx)(
//typedef x (NTAPI *PGD_DxDdSetOverlayPosition)(
//typedef x (NTAPI *PGD_DxDdUnattachSurface)(
//typedef x (NTAPI *PGD_DxDdUnlock)(
//typedef x (NTAPI *PGD_DxDdUnlockD3D)(
//typedef x (NTAPI *PGD_DxDdUpdateOverlay)(
//typedef x (NTAPI *PGD_DxDdWaitForVerticalBlank)(
//typedef x (NTAPI *PGD_DxDvpCanCreateVideoPort)(
//typedef x (NTAPI *PGD_DxDvpColorControl)(
//typedef x (NTAPI *PGD_DxDvpCreateVideoPort)(
//typedef x (NTAPI *PGD_DxDvpDestroyVideoPort)(
//typedef x (NTAPI *PGD_DxDvpFlipVideoPort)(
//typedef x (NTAPI *PGD_DxDvpGetVideoPortBandwidth)(
//typedef x (NTAPI *PGD_DxDvpGetVideoPortField)(
//typedef x (NTAPI *PGD_DxDvpGetVideoPortFlipStatus)(
//typedef x (NTAPI *PGD_DxDvpGetVideoPortInputFormats)(
//typedef x (NTAPI *PGD_DxDvpGetVideoPortLine)(
//typedef x (NTAPI *PGD_DxDvpGetVideoPortOutputFormats)(
//typedef x (NTAPI *PGD_DxDvpGetVideoPortConnectInfo)(
//typedef x (NTAPI *PGD_DxDvpGetVideoSignalStatus)(
//typedef x (NTAPI *PGD_DxDvpUpdateVideoPort)(
//typedef x (NTAPI *PGD_DxDvpWaitForVideoPortSync)(
//typedef x (NTAPI *PGD_DxDvpAcquireNotification)(
//typedef x (NTAPI *PGD_DxDvpReleaseNotification)(
//typedef x (NTAPI *PGD_DxDdHeapVidMemAllocAligned)(
//typedef x (NTAPI *PGD_DxDdHeapVidMemFree)(
//typedef x (NTAPI *PGD_DxDdEnableDirectDraw)(
//typedef x (NTAPI *PGD_DxDdDisableDirectDraw)(
//typedef x (NTAPI *PGD_DxDdSuspendDirectDraw)(
//typedef x (NTAPI *PGD_DxDdResumeDirectDraw)(
//typedef x (NTAPI *PGD_DxDdDynamicModeChange)(
//typedef x (NTAPI *PGD_DxDdCloseProcess)(
//typedef x (NTAPI *PGD_DxDdGetDirectDrawBound)(
//typedef x (NTAPI *PGD_DxDdEnableDirectDrawRedirection)(
//typedef x (NTAPI *PGD_DxDdAllocPrivateUserMem)(
//typedef x (NTAPI *PGD_DxDdFreePrivateUserMem)(
//typedef x (NTAPI *PGD_DxDdLockDirectDrawSurface)(
//typedef x (NTAPI *PGD_DxDdUnlockDirectDrawSurface)(
//typedef x (NTAPI *PGD_DxDdSetAccelLevel)(
//typedef x (NTAPI *PGD_DxDdGetSurfaceLock)(
//typedef x (NTAPI *PGD_DxDdEnumLockedSurfaceRect)(
typedef DWORD (NTAPI *PGD_ENGDXIOCTL)(ULONG, PVOID, ULONG);



