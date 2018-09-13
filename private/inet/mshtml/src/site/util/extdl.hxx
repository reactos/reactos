#ifndef I_EXTDL_HXX_
#define I_EXTDL_HXX_
#pragma INCMSG("--- Beg 'extdl.hxx'")

MtExtern(CExternalDownload)

//+------------------------------------------------------------------------
//
//  Class:     CExternalDownload
//
//-------------------------------------------------------------------------

class CExternalDownload : public CVoid
{
public:

    DECLARE_CLASS_TYPES(CExternalDownload, CVoid)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CExternalDownload))

    //
    // methods
    //

    CExternalDownload();
    ~CExternalDownload();

    static HRESULT Download(
        LPTSTR      pchUrl,
        IDispatch * pdispCallbackFunction,
        CDoc *      pDoc,
        CElement *  pElement = NULL);

    HRESULT Init(
        LPTSTR      pchUrl,
        IDispatch * pdispCallbackFunction,
        CDoc *      pDoc,
        CElement *  pElement);

    HRESULT Done ();

    void SetBitsCtx(CBitsCtx * pBitsCtx);
    void OnDwnChan(CDwnChan * pDwnChan);
    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg)
        { ((CExternalDownload *)pvArg)->OnDwnChan((CDwnChan *)pvObj); }

    //
    // data
    //

    CDoc *          _pDoc;
    CBitsCtx *      _pBitsCtx;
    IDispatch *     _pdispCallbackFunction;
};

#pragma INCMSG("--- End 'extdl.hxx'")
#else
#pragma INCMSG("*** Dup 'extdl.hxx'")
#endif
 