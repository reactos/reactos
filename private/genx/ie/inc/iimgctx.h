//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1996-1998 Microsoft Corporation. All Rights Reserved.
//
//  File: iimgctx.h
//
//--------------------------------------------------------------------------

#ifndef _IImgCtxObjects_H_
#define _IImgCtxObjects_H_

#ifdef __cplusplus
extern "C" {
#endif
typedef void (CALLBACK *PFNIMGCTXCALLBACK)(void *, void *);
#ifdef __cplusplus
}
#endif

#define IMGCHG_SIZE         0x0001
#define IMGCHG_VIEW         0x0002
#define IMGCHG_COMPLETE     0x0004
#define IMGCHG_ANIMATE      0x0008
#define IMGCHG_MASK         0x000F

#define IMGLOAD_NOTLOADED   0x00100000  // Image has not yet been loaded
#define IMGLOAD_LOADING     0x00200000  // Image in the process of being loaded
#define IMGLOAD_STOPPED     0x00400000  // Imaged aborted
#define IMGLOAD_ERROR       0x00800000  // Error loading image
#define IMGLOAD_COMPLETE    0x01000000  // Image loaded
#define IMGLOAD_MASK        0x01F00000

#define IMGBITS_NONE        0x02000000
#define IMGBITS_PARTIAL     0x04000000
#define IMGBITS_TOTAL       0x08000000
#define IMGBITS_MASK        0x0E000000

#define IMGANIM_ANIMATED    0x10000000
#define IMGANIM_MASK        0x10000000

#define IMGTRANS_OPAQUE     0x20000000
#define IMGTRANS_MASK       0x20000000

#define DWN_COLORMODE       0x0000003F  // Explicit color mode requested
#define DWN_DOWNLOADONLY    0x00000040  // Download data only, don't decode
#define DWN_FORCEDITHER     0x00000080  // Override automatic dithering
#define DWN_RAWIMAGE        0x00000100  // Disable dithering
#define DWN_MIRRORIMAGE     0x00000200  // Mirror the image

/* Definition of interface: IImgCtx */
#undef INTERFACE
#define INTERFACE IImgCtx

// {3050f3d7-98b5-11cf-bb82-00aa00bdce0b}
DEFINE_GUID(IID_IImgCtx, 0x3050f3d7, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b);

DECLARE_INTERFACE_(IImgCtx, IUnknown)
{
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IImgCtx methods */

    /* Initialization/Download methods */
    STDMETHOD(Load)(THIS_ LPCWSTR pszUrl, DWORD dwFlags) PURE;
    STDMETHOD(SelectChanges)(THIS_ ULONG ulChgOn, ULONG ulChgOff, BOOL fSignal) PURE;
    STDMETHOD(SetCallback)(THIS_ PFNIMGCTXCALLBACK pfn, void * pvPrivateData) PURE;
    STDMETHOD(Disconnect)(THIS) PURE;

        /* Query methods */
    STDMETHOD(GetUpdateRects)(THIS_ struct tagRECT FAR* prc, struct tagRECT FAR* prcImg, long FAR* pcrc) PURE;
    STDMETHOD(GetStateInfo)(THIS_ ULONG FAR* pulState, struct tagSIZE FAR* psize, BOOL fClearChanges) PURE;
    STDMETHOD(GetPalette)(THIS_ HPALETTE FAR* phpal) PURE;

    /* Rendering methods */
    STDMETHOD(Draw)(THIS_ HDC hdc, struct tagRECT FAR* prcBounds) PURE;
    STDMETHOD(Tile)(THIS_ HDC hdc, struct tagPOINT FAR* pptBackOrg, struct tagRECT FAR* prcClip, struct tagSIZE FAR* psize) PURE;
    STDMETHOD(StretchBlt)(THIS_ HDC hdc, int dstX, int dstY, int dstXE, int dstYE, int srcX, int srcY, int srcXE, int srcYE, DWORD dwROP) PURE;
};

#ifdef COBJMACROS


#define IImgCtx_QueryInterface(This,riid,ppvObject)     \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImgCtx_AddRef(This)    \
    (This)->lpVtbl -> AddRef(This)

#define IImgCtx_Release(This)   \
    (This)->lpVtbl -> Release(This)

#define IImgCtx_Load(This, pszUrl, dwFlags)     \
        (This)->lpVtbl -> Load(This, pszUrl, dwFlags)

#define IImgCtx_Draw(This, hdc, prcBounds)              \
        (This)->lpVtbl -> Draw(This, hdc, prcBounds)

#define IImgCtx_Tile(This, hdc, pptBackOrg, prcClip, psize)             \
        (This)->lpVtbl -> Tile(This, hdc, pptBackOrg, prcClip, psize)

#define IImgCtx_GetUpdateRects(This, prc, prcImg, pcrc) \
        (This)->lpVtbl -> GetUpdateRects(This, prc, prcImg, pcrc)

#define IImgCtx_GetStateInfo(This, pulState, psize, fClearChanges)      \
        (This)->lpVtbl -> GetStateInfo(This, pulState, psize, fClearChanges)

#define IImgCtx_GetPalette(This, phpal) \
        (This)->lpVtbl -> GetPalette(This, phpal)
        
#define IImgCtx_SelectChanges(This, ulChgOn, ulChgOff, fSignal) \
        (This)->lpVtbl -> SelectChanges(This, ulChgOn, ulChgOff, fSignal)

#define IImgCtx_SetCallback(This, pfnCallback, pvPrivateData)   \
        (This)->lpVtbl -> SetCallback(This, pfnCallback, pvPrivateData)

#define IImgCtx_Disconnect(This) \
    (This)->lpVtbl -> Disconnect(This)

#define IImgCtx_StretchBlt(This, hdc, dstX, dstY, dstXE, dstYE, srcX, srcY, srcXE, srcYE, dwROP)   \
    (This)->lpVtbl -> StretchBlt(This, hdc, dstX, dstY, dstXE, dstYE, srcX, srcY, srcXE, srcYE, dwROP)

#endif /* COBJMACROS */


// {3050f3d6-98b5-11cf-bb82-00aa00bdce0b}
DEFINE_GUID(CLSID_IImgCtx, 0x3050f3d6, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b);

#endif
