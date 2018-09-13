//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       img.hxx
//
//  Contents:   CImgInfo
//              CImgLoad
//              CImgTask
//              CImgTaskExec
//
//-------------------------------------------------------------------------

#ifndef I_IMG_HXX_
#define I_IMG_HXX_
#pragma INCMSG("--- Beg 'img.hxx'")

#ifndef X_DWN_HXX_
#define X_DWN_HXX_
#include "dwn.hxx"
#endif

#ifndef X_IMGART_HXX_
#define X_IMGART_HXX_
#include "imgart.hxx"
#endif

#ifndef X_PRGSNK_H
#define X_PRGSNK_H
#include "prgsnk.h"
#endif

#ifndef X_IMGBITS_HXX
#define X_IMGBITS_HXX
#include "imgbits.hxx"
#endif

// Forward --------------------------------------------------------------------

class   CImgInfo;
class   CImgTask;
class   CImgTaskExec;
class   CImgLoad;
class   CArtPlayer;
struct  GIFFRAME;
struct  GIFANIMDATA;

MtExtern(CImgInfo)
MtExtern(CImgTaskExec)
MtExtern(CImgLoad)

// Definitions ----------------------------------------------------------------

enum
{
    gifNoneSpecified =  0,              // no disposal method specified
    gifNoDispose =      1,              // do not dispose, leave the bits there
    gifRestoreBkgnd =   2,              // replace the image with the background color
    gifRestorePrev =    3               // replace the image with the previous pixels
};

#define TRANSF_TRANSPARENT  0x01        // Image is marked transparent
#define TRANSF_TRANSMASK    0x02        // Attempted to create an hbmMask

#define dwGIFVerUnknown     ((DWORD)0)  // unknown version of GIF file
#define dwGIFVer87a         ((DWORD)87) // GIF87a file format
#define dwGIFVer89a         ((DWORD)89) // GIF89a file format.

// Globals --------------------------------------------------------------------

extern WORD         g_wIdxBgColor;
extern WORD         g_wIdxFgColor;
extern WORD         g_wIdxTrans;
extern RGBQUAD      g_rgbBgColor;
extern RGBQUAD      g_rgbFgColor;
extern PALETTEENTRY g_peVga[16];
extern BYTE *       g_pInvCMAP;
extern CImgBits *   g_pImgBitsNotLoaded;
extern CImgBits *   g_pImgBitsMissing;

ExternTag(tagImgProg);

// Types ----------------------------------------------------------------------

struct GIFFRAME
{
    GIFFRAME *      pgfNext;
    CImgBitsDIB *   pibd;
    HRGN            hrgnVis;            // region describing currently visible portion of the frame
    BYTE            bDisposalMethod;    // see enum above
    BYTE            bTransFlags;        // see TRANSF_ flags below
    BYTE            bRgnKind;           // region type for hrgnVis
    UINT            uiDelayTime;        // frame duration, in ms
    long            left;               // bounds relative to the GIF logical screen 
    long            top; 
    long            width;
    long            height;
};

struct GIFANIMDATA
{
    BYTE            fAnimated;          // TRUE if cFrames and pgf define a GIF animation
    BYTE            fLooped;            // TRUE if we've seen a Netscape loop block
    BYTE            fHasTransparency;   // TRUE if a frame is transparent, or if a frame does
                                        // not cover the entire logical screen.
    BYTE            bAlign;             // Reserved
    UINT            cLoops;             // A la Netscape, we will treat this as 
                                        // "loop forever" if it is zero.
    DWORD           dwGIFVer;           // GIF Version <see defines above> we need to special case 87a backgrounds
    GIFFRAME *      pgf;                // animation frame entries
};

struct FIBERINFO
{
    void *          pvFiber;
    void *          pvMain;
    CImgTask *      pImgTask;
};

// Functions ------------------------------------------------------------------

void *  pCreateDitherData(int xsize);
int     x_ComputeConstrainMap(int cEntries, PALETTEENTRY *pcolors,
            int transparent, int *pmapconstrained);
void    x_ColorConstrain(unsigned char HUGEP *psrc, unsigned char HUGEP *pdst,
            int *pmapconstrained, long xsize);
void    x_DitherRelative(BYTE * pbSrc, BYTE * pbDst, PALETTEENTRY *pe,
            int xsize, int ysize, int transparent,int *v_rgb_mem,
            int yfirst, int ylast);
HRESULT x_Dither(unsigned char *pdata, PALETTEENTRY *pe, int xsize,
            int ysize, int transparent);

#ifdef OLDIMAGECODE // replaced by CImgBitsDIB

HBITMAP ImgCreateDib(LONG xWid, LONG yHei, BOOL fPal, int cBitsPerPix,
            int cEnt, PALETTEENTRY * ppe, BYTE ** ppbBits, int * pcbRow, BOOL fMono = FALSE);
HBITMAP ImgCreateDibFromInfo(BITMAPINFO * pbmi, UINT wUsage, BYTE ** ppbBits,
            int * pcbRow);
void    ImgDeleteDib(HBITMAP hbmDib);
ULONG   ImgDibSize(HBITMAP hbm);

#endif

void    CalcStretchRect(RECT * prectStretch, LONG xWid, LONG yHei, LONG xDispWidth, LONG yDispHeight, GIFFRAME * pgf);
void    getPassInfo(int logicalRowX, int height, int *pPassX,
            int *pRowX, int *pBandX);
int     Union(int _yTop, int _yBot, BOOL fInvalidateAll, int yBot);
void    ComputeFrameVisibility(IMGANIMSTATE *pImgAnimState, LONG xWidth, LONG yHeight, LONG xDispWidth, LONG yDispHeight);
void    FreeGifAnimData(GIFANIMDATA * pgad, CImgBitsDIB *pImgBits);
HBITMAP ComputeTransMask(HBITMAP hbmDib, BOOL fPal, BYTE bTrans);
HRESULT StartImgTask(CImgTask * pImgTask);
void    KillImgTaskExec();
BOOL    IsPluginImgFormat(BYTE * pb, UINT cb);

// CImgInfo -------------------------------------------------------------------

class CImgInfo : public CDwnInfo
{
    typedef CDwnInfo super;
    friend class CImgCtx;
    friend class CImgLoad;
    friend class CImgTask;
    friend class CImgTaskExec;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgInfo))

    virtual        ~CImgInfo();
    virtual HRESULT Init(DWNLOADINFO * pdli);
    virtual void    Passivate();

    HRESULT         DrawImage(HDC hdc, RECT * prcDst, RECT * prcSrc, DWORD dwROP, DWORD dwFlags);
    void            InitImgAnimState(IMGANIMSTATE * pImgAnimState);
    BOOL            NextFrame(IMGANIMSTATE *pImgAnimState, DWORD dwCurTimeMS, DWORD *pdwFrameTimeMS);
    void            DrawFrame(HDC hdc, IMGANIMSTATE *pImgAnimState, RECT *prcDst, RECT *prcSrc, RECT *prcDestFull, DWORD dwFlags);
    int             GetColorMode() { return((int)GetFlags(DWNF_COLORMODE)); }
    HRESULT         SaveAsBmp(IStream *pStm, BOOL fFileHeader);
    virtual HRESULT NewDwnCtx(CDwnCtx ** ppDwnCtx);
    virtual HRESULT NewDwnLoad(CDwnLoad ** ppDwnLoad);
#ifndef NO_ART
    CArtPlayer *    GetArtPlayer();
#endif

    void            GetImageAndMask(CImgBits **ppImgBits);
    LONG            GetTrans() { return _lTrans; }
    
protected:

    UINT            GetType() { return(DWNCTX_IMG); }
    DWORD           GetProgSinkClass() { return(PROGSINK_CLASS_MULTIMEDIA); }
    void            Signal(WORD wChg, BOOL fInvalAll, int yBot);
    void            OnLoadTask(CImgLoad * pImgLoad, CImgTask * pImgTask);
    virtual void    OnLoadDone(HRESULT hrErr);
    void            OnTaskSize(CImgTask * pImgTask, LONG xWid, LONG yHei,
                        LONG lTrans, MIMEINFO * pmi);
    void            OnTaskTrans(CImgTask *pImgTask, LONG lTrans);
    void            OnTaskProg(CImgTask * pImgTask, ULONG ulState, BOOL fAll,
                        LONG yBot);
    void            OnTaskAnim(CImgTask * pImgTask);
#ifdef _MAC
    BOOL            OnTaskBits(CImgTask * pImgTask, CImgBits *pImgBits, GIFANIMDATA * pgad, CArtPlayer * pArtPlayer,
                        LONG lTrans, LONG ySrcBot, BOOL fNonProgressive, LPPROFILE Profile);
#else
    BOOL            OnTaskBits(CImgTask * pImgTask, CImgBits *pImgBits, GIFANIMDATA * pgad, CArtPlayer * pArtPlayer,
                        LONG lTrans, LONG ySrcBot, BOOL fNonProgressive);
#endif
    void            OnTaskDone(CImgTask * pImgTask);
    virtual void    Abort(HRESULT hrErr, CDwnLoad ** ppDwnLoad);
    virtual void    Reset();

    virtual BOOL    AttachEarly(UINT dt, DWORD dwRefresh, DWORD dwFlags, DWORD dwBindf);
    virtual BOOL    CanAttachLate(CDwnInfo * pDwnInfo);
    virtual void    AttachLate(CDwnInfo * pDwnInfo);
    virtual ULONG   ComputeCacheSize();

    // Data members

    CImgTask *      _pImgTask;
    LONG            _xWid;
    LONG            _yHei;
    LONG            _ySrcBot;
    
    CImgBits *      _pImgBits;
    
    LONG            _lTrans;
    GIFANIMDATA     _gad;

    unsigned        _fNoOptimize;   // not optimize if imginfo created by external component
#ifndef NO_ART
    CArtPlayer *    _pArtPlayer;
#endif // NO_ART
#ifdef _MAC
    LPPROFILE       _Profile;
#endif // MW_TRIDENT
};


// CImgTaskExec ---------------------------------------------------------------

class CImgTaskExec : public CDwnTaskExec
{
    typedef CDwnTaskExec super;
    friend class CImgTask;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgTaskExec))

                        CImgTaskExec(CRITICAL_SECTION * pcs) : super(pcs) {}
    HRESULT             RequestCoInit();

protected:

    virtual HRESULT     ThreadInit();
    virtual void        ThreadTerm();
    virtual void        ThreadExit();
    virtual void        ThreadTimeout();

    FIBERINFO *         GetFiber(CImgTask * pImgTask);
    void                YieldTask(CImgTask * pImgTask, BOOL fBlock);
    void                RunTask(CImgTask * pImgTask);
    void                AssignFiber(FIBERINFO * pfi);
    static void WINAPI  FiberProc(void * pv);

    #if DBG==1
    virtual void        Invariant();
    #endif

    // Data members

    void *              _pvFiberMain;
    FIBERINFO           _afi[5];
    BOOL                _fCoInit;

};

// CImgTask -------------------------------------------------------------------

class CImgTask : public CDwnTask
{
    typedef CDwnTask super;
    friend class CImgTaskExec;

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Dwn))

public:

    void            Init(CImgInfo * pImgInfo, MIMEINFO * pmi,
                        CDwnBindData * pDwnBindData);
    void            Exec();
    BOOL            Read(void * pv, ULONG cb, ULONG * pcbRead = NULL, ULONG cbMinReq = 0);
    virtual void    Terminate();

    void            OnSize(LONG xWid, LONG yHei, LONG lTrans);
    void            OnTrans(LONG lTrans);
    void            OnProg(BOOL fLast, ULONG ulBits, BOOL fAll, LONG yBot);
    void            OnAnim();
    LPCTSTR         GetUrl();
    GIFFRAME *      GetPgf() { return _gad.pgf; }

    virtual void    BltDib(HDC hdc, RECT * prcDst, RECT * prcSrc, DWORD dwRop, DWORD dwFlags) {};

    BOOL            IsFullyAvail()      { return(_pDwnBindData->IsFullyAvail()); }

    void            GetImageAndMask(CImgBits **ppImgBits);

#ifndef NO_ART
    CArtPlayer *    GetArtPlayer() { return _pArtPlayer; }
    BOOL            DoTaskGetReport(CArtPlayer * pArtPlayer);
#endif

protected:

                   ~CImgTask();
    virtual void    Run();
    virtual void    Decode(BOOL *pfNonProgressive) = 0;
    CImgTaskExec *  GetImgTaskExec()    { return((CImgTaskExec *)_pDwnTaskExec); }

    // Data members

    CImgInfo *      _pImgInfo;
    CDwnBindData *  _pDwnBindData;
    FIBERINFO *     _pfi;
    BOOL            _fTerminate;
    BOOL            _fWaitForFiber;
    LONG            _xWid;
    LONG            _yHei;
    LONG            _ySrcBot;
    LONG            _lTrans;

    CImgBits *      _pImgBits;
    
    GIFANIMDATA     _gad;
#ifndef NO_ART
    CArtPlayer *    _pArtPlayer;
#endif // NO_ART
    PALETTEENTRY    _ape[256];
    LONG            _yBot;
    BOOL            _fComplete;
    LONG            _colorMode;
    MIMEINFO *      _pmi;
    DWORD           _dwTickProg;
    LONG            _yTopProg;
    LONG            _yBotProg;
#ifdef _MAC
    LPPROFILE       _Profile;
#endif
};

// CImgLoad ----------------------------------------------------------------

class CImgLoad : public CDwnLoad
{
    typedef CDwnLoad super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgLoad))

    // CDwnLoad methods

    virtual        ~CImgLoad();
    virtual void    Passivate();
    virtual HRESULT Init(DWNLOADINFO * pdli, CDwnInfo * pDwnInfo);
    virtual HRESULT OnBindHeaders();
    virtual HRESULT OnBindMime(MIMEINFO * pmi);
    virtual HRESULT OnBindData();
    virtual void    OnBindDone(HRESULT hrErr);

    // CImgLoad methods

    CImgInfo *      GetImgInfo()    { return((CImgInfo *)_pDwnInfo); }
    int             GetColorMode()  { return(GetImgInfo()->GetColorMode()); }

protected:

    // Data members

    CImgTask *      _pImgTask;

};

// Inlines --------------------------------------------------------------------

inline void CImgTask::OnSize(LONG xWid, LONG yHei, LONG lTrans)
    { _pImgInfo->OnTaskSize(this, xWid, yHei, lTrans, _pmi); }
inline void CImgTask::OnTrans(LONG lTrans)
    { _pImgInfo->OnTaskTrans(this, lTrans); }
inline LPCTSTR CImgTask::GetUrl()
    { return(_pImgInfo->GetUrl()); }

inline void CImgInfo::GetImageAndMask(CImgBits **ppImgBits)
{
    EnterCriticalSection();

    if (_pImgTask)
        _pImgTask->GetImageAndMask(ppImgBits);
    else
    {
        *ppImgBits = _pImgBits;
    }
    
    LeaveCriticalSection();
}

inline void CImgTask::GetImageAndMask(CImgBits **ppImgBits)
{
    EnterCriticalSection();

    *ppImgBits = _pImgBits;

    LeaveCriticalSection();
}
    

// lookup closest entry in g_hpalHalftone to (r,g,b)

inline BYTE RGB2Index(BYTE r, BYTE g, BYTE b)
{ 
    return g_pInvCMAP[((((r >> 3) << 5) + (g >> 3)) << 5)+(b >> 3)];
}

// ----------------------------------------------------------------------------

#pragma INCMSG("--- End 'img.hxx'")
#else
#pragma INCMSG("*** Dup 'img.hxx'")
#endif
