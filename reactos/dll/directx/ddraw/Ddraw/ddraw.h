/* public interfaces  we doing pur type cast here instead in the code, it will make more cleaner code */

HRESULT WINAPI
Main_DirectDraw_QueryInterface (
                                LPDDRAWI_DIRECTDRAW_INT This,
                                REFIID id,
                                LPVOID *obj);

ULONG WINAPI
Main_DirectDraw_AddRef (LPDDRAWI_DIRECTDRAW_INT This);

ULONG WINAPI
Main_DirectDraw_Release (LPDDRAWI_DIRECTDRAW_INT This);

HRESULT WINAPI
Main_DirectDraw_Compact(LPDDRAWI_DIRECTDRAW_INT This);

HRESULT WINAPI
Main_DirectDraw_CreateClipper(
                              LPDDRAWI_DIRECTDRAW_INT This,
                              DWORD dwFlags,
                              LPDIRECTDRAWCLIPPER *ppClipper,
                              IUnknown *pUnkOuter);

HRESULT WINAPI
Main_DirectDraw_CreatePalette(
                              LPDDRAWI_DIRECTDRAW_INT This,
                              DWORD dwFlags,
                              LPPALETTEENTRY palent,
                              LPDIRECTDRAWPALETTE* ppPalette,
                              LPUNKNOWN pUnkOuter);

HRESULT WINAPI
Main_DirectDraw_CreateSurface4(
                               LPDDRAWI_DIRECTDRAW_INT This,
                               LPDDSURFACEDESC2 pDDSD,
                               LPDIRECTDRAWSURFACE7 *ppSurf,
                               IUnknown *pUnkOuter);

HRESULT WINAPI
Main_DirectDraw_DuplicateSurface(
                                 LPDDRAWI_DIRECTDRAW_INT This,
                                 LPDIRECTDRAWSURFACE7 src,
                                 LPDIRECTDRAWSURFACE7* dst);

HRESULT WINAPI
Main_DirectDraw_EnumDisplayModes(
                                 LPDDRAWI_DIRECTDRAW_INT This,
                                 DWORD dwFlags,
                                 LPDDSURFACEDESC2 pDDSD,
                                 LPVOID pContext,
                                 LPDDENUMMODESCALLBACK2 pCallback);

HRESULT WINAPI
Main_DirectDraw_EnumSurfaces(
                             LPDDRAWI_DIRECTDRAW_INT This,
                             DWORD dwFlags,
                             LPDDSURFACEDESC2 lpDDSD2,
                             LPVOID context,
                             LPDDENUMSURFACESCALLBACK7 callback);

HRESULT WINAPI
Main_DirectDraw_FlipToGDISurface(LPDDRAWI_DIRECTDRAW_INT This);

HRESULT WINAPI
Main_DirectDraw_GetCaps(
                        LPDDRAWI_DIRECTDRAW_INT This,
                        LPDDCAPS pDriverCaps,
                        LPDDCAPS pHELCaps);

HRESULT WINAPI
Main_DirectDraw_GetDisplayMode (
                                LPDDRAWI_DIRECTDRAW_INT This,
                                LPDDSURFACEDESC2 pDDSD);

HRESULT WINAPI
Main_DirectDraw_GetFourCCCodes(
                               LPDDRAWI_DIRECTDRAW_INT This,
                               LPDWORD lpNumCodes,
                               LPDWORD lpCodes);

HRESULT WINAPI
Main_DirectDraw_GetGDISurface(
                              LPDDRAWI_DIRECTDRAW_INT This,
                              LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface);

HRESULT WINAPI
Main_DirectDraw_GetMonitorFrequency (
                                     LPDDRAWI_DIRECTDRAW_INT This,
                                     LPDWORD lpFreq);


HRESULT WINAPI
Main_DirectDraw_GetScanLine(LPDDRAWI_DIRECTDRAW_INT This,
                            LPDWORD lpdwScanLine);

HRESULT WINAPI
Main_DirectDraw_GetVerticalBlankStatus(
                                       LPDDRAWI_DIRECTDRAW_INT This,
                                       LPBOOL lpbIsInVB);

HRESULT WINAPI
Main_DirectDraw_Initialize (
                            LPDDRAWI_DIRECTDRAW_INT This,
                            LPGUID lpGUID);


HRESULT WINAPI
Main_DirectDraw_RestoreDisplayMode (LPDDRAWI_DIRECTDRAW_INT This);

HRESULT WINAPI
Main_DirectDraw_SetCooperativeLevel (
                                     LPDDRAWI_DIRECTDRAW_INT This,
                                     HWND hwnd,
                                     DWORD cooplevel);

HRESULT WINAPI
Main_DirectDraw_SetDisplayMode(
                               LPDDRAWI_DIRECTDRAW_INT This,
                               DWORD dwWidth,
                               DWORD dwHeight,
                               DWORD dwBPP);

HRESULT WINAPI
Main_DirectDraw_SetDisplayMode2(
                               LPDDRAWI_DIRECTDRAW_INT This,
                               DWORD dwWidth,
                               DWORD dwHeight,
                               DWORD dwBPP,
                               DWORD dwRefreshRate,
                               DWORD dwFlags);

HRESULT WINAPI
Main_DirectDraw_WaitForVerticalBlank(
                                     LPDDRAWI_DIRECTDRAW_INT This,
                                     DWORD dwFlags,
                                     HANDLE h);


HRESULT WINAPI
Main_DirectDraw_GetAvailableVidMem4(
                                    LPDDRAWI_DIRECTDRAW_INT This,
                                    LPDDSCAPS2 ddscaps,
                                    LPDWORD dwTotal,
                                    LPDWORD dwFree);

HRESULT WINAPI
Main_DirectDraw_GetSurfaceFromDC(
                                 LPDDRAWI_DIRECTDRAW_INT This,
                                 HDC hdc,
                                 LPDIRECTDRAWSURFACE7 *lpDDS);

HRESULT WINAPI
Main_DirectDraw_RestoreAllSurfaces(LPDDRAWI_DIRECTDRAW_INT This);

HRESULT WINAPI
Main_DirectDraw_TestCooperativeLevel(LPDDRAWI_DIRECTDRAW_INT This);

HRESULT WINAPI
Main_DirectDraw_GetDeviceIdentifier7(
                                     LPDDRAWI_DIRECTDRAW_INT This,
                                     LPDDDEVICEIDENTIFIER2 pDDDI,
                                     DWORD dwFlags);

HRESULT WINAPI
Main_DirectDraw_StartModeTest(
                              LPDDRAWI_DIRECTDRAW_INT This,
                              LPSIZE pModes,
                              DWORD dwNumModes,
                              DWORD dwFlags);

HRESULT WINAPI
Main_DirectDraw_EvaluateMode(
                             LPDDRAWI_DIRECTDRAW_INT This,
                             DWORD a,
                             DWORD* b);




// hel callbacks

DWORD CALLBACK HelDdSurfAddAttachedSurface(LPDDHAL_ADDATTACHEDSURFACEDATA lpDestroySurface);
DWORD CALLBACK HelDdSurfBlt(LPDDHAL_BLTDATA lpBltData);
DWORD CALLBACK HelDdSurfDestroySurface(LPDDHAL_DESTROYSURFACEDATA lpDestroySurfaceData);
DWORD CALLBACK HelDdSurfFlip(LPDDHAL_FLIPDATA lpFlipData);
DWORD CALLBACK HelDdSurfGetBltStatus(LPDDHAL_GETBLTSTATUSDATA lpGetBltStatusData);
DWORD CALLBACK HelDdSurfGetFlipStatus(LPDDHAL_GETFLIPSTATUSDATA lpGetFlipStatusData);
DWORD CALLBACK HelDdSurfLock(LPDDHAL_LOCKDATA lpLockData);
DWORD CALLBACK HelDdSurfreserved4(DWORD *lpPtr);
DWORD CALLBACK HelDdSurfSetClipList(LPDDHAL_SETCLIPLISTDATA lpSetClipListData);
DWORD CALLBACK HelDdSurfSetColorKey(LPDDHAL_SETCOLORKEYDATA lpSetColorKeyData);
DWORD CALLBACK HelDdSurfSetOverlayPosition(LPDDHAL_SETOVERLAYPOSITIONDATA lpSetOverlayPositionData);
DWORD CALLBACK HelDdSurfSetPalette(LPDDHAL_SETPALETTEDATA lpSetPaletteData);
DWORD CALLBACK HelDdSurfUnlock(LPDDHAL_UNLOCKDATA lpUnLockData);
DWORD CALLBACK HelDdSurfUpdateOverlay(LPDDHAL_UPDATEOVERLAYDATA lpUpDateOveryLayData);

