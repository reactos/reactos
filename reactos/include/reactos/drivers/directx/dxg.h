

/* win32k driver functions table  it have created for the engine the DxEng* api */


/* DXG.SYS FUNCTIONS INDEX */
/***************************************************************************/
/* This driver functions are exported raw from NtGdiDd* / NtDvp* / NtD3d*  */
/***************************************************************************/
#define DXG_INDEX_DXDXGGENERICTHUNK               0X00
#define DXG_INDEX_DXD3DCONTEXTCREATE              0X01
#define DXG_INDEX_DXD3DCONTEXTDESTROY             0X02
#define DXG_INDEX_DXD3DCONTEXTDESTROYALL          0X03
#define DXG_INDEX_DXD3DVALIDATETEXTURESTAGESTATE  0X04
#define DXG_INDEX_DXD3DDRAWPRIMITIVES2            0X05
#define DXG_INDEX_DXDDGETDRIVERSTATE              0X06
#define DXG_INDEX_DXDDADDATTACHEDSURFACE          0X07
#define DXG_INDEX_DXDDALPHABLT                    0X08
#define DXG_INDEX_DXDDATTACHSURFACE               0X09
#define DXG_INDEX_DXDDBEGINMOCOMPFRAME            0X0A
#define DXG_INDEX_DXDDBLT                         0X0B
#define DXG_INDEX_DXDDCANCREATESURFACE            0X0C
#define DXG_INDEX_DXDDCANCREATED3DBUFFER          0X0D
#define DXG_INDEX_DXDDCOLORCONTROL                0X0E
#define DXG_INDEX_DXDDCREATEDIRECTDRAWOBJECT      0X0F
/* DXG_INDEX_DxDdCreateSurface and  DXG_INDEX_DxDdCreateD3DBuffer2 are same */
#define DXG_INDEX_DXDDCREATESURFACE               0X10
#define DXG_INDEX_DXDDCREATED3DBUFFER             0X11
#define DXG_INDEX_DXDDCREATEMOCOMP                0X12
#define DXG_INDEX_DXDDCREATESURFACEOBJECT         0X13
#define DXG_INDEX_DXDDDELETEDIRECTDRAWOBJECT      0X14
#define DXG_INDEX_DXDDDELETESURFACEOBJECT         0X15
#define DXG_INDEX_DXDDDESTROYMOCOMP               0X16
#define DXG_INDEX_DXDDDESTROYSURFACE              0X17
#define DXG_INDEX_DXDDDESTROYD3DBUFFER            0X18
#define DXG_INDEX_DXDDENDMOCOMPFRAME              0X19
#define DXG_INDEX_DXDDFLIP                        0X1A
#define DXG_INDEX_DXDDFLIPTOGDISURFACE            0X1B
#define DXG_INDEX_DXDDGETAVAILDRIVERMEMORY        0X1C
#define DXG_INDEX_DXDDGETBLTSTATUS                0X1D
#define DXG_INDEX_DXDDGETDC                       0X1E
#define DXG_INDEX_DXDDGETDRIVERINFO               0X1F
#define DXG_INDEX_DXDDGETDXHANDLE                 0X20
#define DXG_INDEX_DXDDGETFLIPSTATUS               0X21
#define DXG_INDEX_DXDDGETINTERNALMOCOMPINFO       0X22
#define DXG_INDEX_DXDDGETMOCOMPBUFFINFO           0X23
#define DXG_INDEX_DXDDGETMOCOMPGUIDS              0X24
#define DXG_INDEX_DXDDGETMOCOMPFORMATS            0X25
#define DXG_INDEX_DXDDGETSCANLINE                 0X26
#define DXG_INDEX_DXDDLOCK                        0X27
#define DXG_INDEX_DXDDLOCKD3D                     0X28
#define DXG_INDEX_DXDDQUERYDIRECTDRAWOBJECT       0X29
#define DXG_INDEX_DXDDQUERYMOCOMPSTATUS           0X2A
#define DXG_INDEX_DXDDREENABLEDIRECTDRAWOBJECT    0X2B
#define DXG_INDEX_DXDDRELEASEDC                   0X2C
#define DXG_INDEX_DXDDRENDERMOCOMP                0X2D
#define DXG_INDEX_DXDDRESETVISRGN                 0X2E
#define DXG_INDEX_DXDDSETCOLORKEY                 0X2F
#define DXG_INDEX_DXDDSETEXCLUSIVEMODE            0X30
#define DXG_INDEX_DXDDSETGAMMARAMP                0X31
#define DXG_INDEX_DXDDCREATESURFACEEX             0X32
#define DXG_INDEX_DXDDSETOVERLAYPOSITION          0X33
#define DXG_INDEX_DXDDUNATTACHSURFACE             0X34
/* DXG_INDEX_DxDdUnlock and  DXG_INDEX_DxDdUnlockD3D are same */
#define DXG_INDEX_DXDDUNLOCK                      0X35
#define DXG_INDEX_DXDDUNLOCKD3D                   0X36
#define DXG_INDEX_DXDDUPDATEOVERLAY               0X37
#define DXG_INDEX_DXDDWAITFORVERTICALBLANK        0X38
#define DXG_INDEX_DXDVPCANCREATEVIDEOPORT         0X39
#define DXG_INDEX_DXDVPCOLORCONTROL               0X3A
#define DXG_INDEX_DXDVPCREATEVIDEOPORT            0X3B
#define DXG_INDEX_DXDVPDESTROYVIDEOPORT           0X3C
#define DXG_INDEX_DXDVPFLIPVIDEOPORT              0X3D
#define DXG_INDEX_DXDVPGETVIDEOPORTBANDWIDTH      0X3E
#define DXG_INDEX_DXDVPGETVIDEOPORTFIELD          0X3F
#define DXG_INDEX_DXDVPGETVIDEOPORTFLIPSTATUS     0X40
#define DXG_INDEX_DXDVPGETVIDEOPORTINPUTFORMATS   0X41
#define DXG_INDEX_DXDVPGETVIDEOPORTLINE           0X42
#define DXG_INDEX_DXDVPGETVIDEOPORTOUTPUTFORMATS  0X43
#define DXG_INDEX_DXDVPGETVIDEOPORTCONNECTINFO    0X44
#define DXG_INDEX_DXDVPGETVIDEOSIGNALSTATUS       0X45
#define DXG_INDEX_DXDVPUPDATEVIDEOPORT            0X46
#define DXG_INDEX_DXDVPWAITFORVIDEOPORTSYNC       0X47
#define DXG_INDEX_DXDVPACQUIRENOTIFICATION        0X48
#define DXG_INDEX_DXDVPRELEASENOTIFICATION        0X49

/***********************************************************************************/
/* This driver functions are exported raw from Eng* it only exists in the def file */
/* you can not do syscallback to thuse but you can import them from win32k.sys     */
/* for them are in the export list                                                 */
/***********************************************************************************/
/* not addedd yet */
#define DXG_INDEX_DXDDHEAPVIDMEMALLOCALIGNED      0X4A
#define DXG_INDEX_DXDDHEAPVIDMEMFREE              0X4B
#define DXG_INDEX_DXDDALLOCPRIVATEUSERMEM         0X54
#define DXG_INDEX_DXDDFREEPRIVATEUSERMEM          0X55
#define DXG_INDEX_DXDDLOCKDIRECTDRAWSURFACE       0X56
#define DXG_INDEX_DXDDUNLOCKDIRECTDRAWSURFACE     0X57
#define DXG_INDEX_DXDDIOCTL                       0X5B


/***********************************************************************************/
/* Internal use in diffent part in Windows and ReactOS                             */
/***********************************************************************************/
/* not inuse yet */
#define DXG_INDEX_DXDDENABLEDIRECTDRAW            0X4C
#define DXG_INDEX_DXDDDISABLEDIRECTDRAW           0X4D
#define DXG_INDEX_DXDDSUSPENDDIRECTDRAW           0X4E
#define DXG_INDEX_DXDDRESUMEDIRECTDRAW            0X4F
#define DXG_INDEX_DXDDDYNAMICMODECHANGE           0X50
#define DXG_INDEX_DXDDCLOSEPROCESS                0X51
#define DXG_INDEX_DXDDGETDIRECTDRAWBOUND          0X52
#define DXG_INDEX_DXDDENABLEDIRECTDRAWREDIRECTION 0X53
#define DXG_INDEX_DXDDSETACCELLEVEL               0X58
#define DXG_INDEX_DXDDGETSURFACELOCK              0X59
#define DXG_INDEX_DXDDENUMLOCKEDSURFACERECT       0X5A
#define DXG_INDEX_DXDDIOCTL                       0X5B
#define DXG_INDEX_DXDDSWITCHTOGDI                 0X5C
#define DXG_INDEX_LAST                            DXG_INDEX_DXDDSWITCHTOGDI + 1


/***********************************************************************************/
/* Driver Functions Protypes                                                       */
/***********************************************************************************/
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
//typedef x (NTAPI *PGD_DxDdDisableDirectDraw)(
//typedef x (NTAPI *PGD_DxDdSuspendDirectDraw)(
//typedef x (NTAPI *PGD_DxDdResumeDirectDraw)(
//typedef x (NTAPI *PGD_DxDdDynamicModeChange)(
//typedef x (NTAPI *PGD_DxDdCloseProcess)(
//typedef x (NTAPI *PGD_DxDdGetDirectDrawBound)(
//typedef x (NTAPI *PGD_DxDdEnableDirectDrawRedirection)(
//typedef x (NTAPI *PGD_DxDdAllocPrivateUserMem)(
//typedef x (NTAPI *PGD_DxDdFreePrivateUserMem)(
//typedef x (NTAPI *PGD_DxDdSetAccelLevel)(
//typedef x (NTAPI *PGD_DxDdGetSurfaceLock)(
//typedef x (NTAPI *PGD_DxDdEnumLockedSurfaceRect)(

typedef DWORD (NTAPI *PGD_DXGENERICTRUNK)(ULONG_PTR, ULONG_PTR, SIZE_T*, PVOID, SIZE_T*, PVOID);
typedef BOOLEAN (NTAPI *PGD_DXDDENABLEDIRECTDRAW)(PVOID, BOOLEAN);
typedef PDD_SURFACE_LOCAL (NTAPI *PGD_DXDDLOCKDIRECTDRAWSURFACE)(HANDLE hDdSurface);
typedef BOOL (NTAPI *PGD_DXDDUNLOCKDIRECTDRAWSURFACE)(HANDLE hDdSurface);
typedef DWORD (NTAPI *PGD_ENGDXIOCTL)(ULONG, PVOID, ULONG);



