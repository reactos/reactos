//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       perhist.cxx
//
//  Contents:   IPersistHistory implementation
//
//  Classes:    CDoc (partial)
//
//-------------------------------------------------------------------------

#ifndef I_PERHIST_HXX_
#define I_PERHIST_HXX_
#pragma INCMSG("--- Beg 'perhist.hxx'")

#ifndef X_DOWNBASE_HXX_
#define X_DOWNBASE_HXX_
#include "downbase.hxx"
#endif

#ifndef X_DATASTM_HXX_
#define X_DATASTM_HXX_
#include "datastm.hxx"
#endif

#ifndef X_OPTARY_H_
#define X_OPTARY_H_
#pragma INCMSG("--- Beg <optary.h>")
#include <optary.h>
#pragma INCMSG("--- End <optary.h>")
#endif

MtExtern(CHistoryLoadCtx)
MtExtern(CHistoryLoadCtx_arySubstreams_pv)
MtExtern(COptionArray)
MtExtern(COptionArray_aryOption_pv)

#define SAVEHIST_INPUT       0x0001 // Save user input
#define SAVEHIST_ECHOHEADERS 0x0002 // Save echo headers

// codes used in top-level history stream

enum HISTORY_CODE {
    HISTORY_END,
    HISTORY_STMDIRTY,
    HISTORY_STMREFRESH,
    HISTORY_PCHFILENAME,
    HISTORY_PCHURL,
    HISTORY_PMKNAME,
    HISTORY_POSTDATA,
    HISTORY_CURRENTSITE,
    HISTORY_SCROLLPOS,
    HISTORY_BOOKMARKNAME,
    HISTORY_STMHISTORY,
    HISTORY_PCHDOCREFERER,
    HISTORY_PCHSUBREFERER,
    HISTORY_FTLASTMOD,
    HISTORY_HREFCODEPAGE,
    HISTORY_CODEPAGE,
    HISTORY_USERDATA,
    HISTORY_REQUESTHEADERS,
    HISTORY_BINDONAPT,
    HISTORY_NAVIGATED,
    HISTORY_FONTVERSION,
    HISTORY_DOCDIRECTION,
};

//+------------------------------------------------------------------------
//
//  Class:      CHistoryLoadCtx
//
//              Breaks down a history stream into multiple substreams
//              (identified by index), and holds on to them until somebody
//              requests them.
//
//              Also hands out history indices in order so that they can
//              be passed back to a CHistorySaveCtx.
//              
//              Created and held by the document when any substream is
//              requested (e.g., by an input element during a load), or
//              when doing an IPersistHistory::Load
//
//-------------------------------------------------------------------------

class CHistoryLoadCtx
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHistoryLoadCtx))

    CHistoryLoadCtx() : _arySubstreams(Mt(CHistoryLoadCtx_arySubstreams_pv)) {}
    ~CHistoryLoadCtx();

    HRESULT Init(IStream *pStream, IBindCtx *pbc);
    void    Clear();
    ULONG GetLoadIndex();
    HRESULT GetLoadStream(ULONG index, DWORD dwCheck, IStream **ppStream);
    HRESULT GetBindCtx(IBindCtx **ppbc);

    IBindCtx *_pbc;

    struct SubstreamEntry
    {
        IStream *pStream;
        DWORD    dwCheck;
        ULONG    uCookieIndex;
    };

    // Member Variables
    //-----------------------------------------
    ULONG _cCountZeroed;
    ULONG _iLastFound;
    
    CDataAry<SubstreamEntry> _arySubstreams;
};


//+------------------------------------------------------------------------
//
//  Class:      CHistorySaveCtx
//
//              Saves out a stream which can be read back in by a
//              CHistoryLoadCtx.
//
//              Allows a caller to save into a substream by specifying
//              an index (previously handed out by CHistoryLoadCtx)
//
//              Created when processing an IPersistHistory::Save
//
//-------------------------------------------------------------------------
class CHistorySaveCtx
{
public:
    CHistorySaveCtx() { memset(this, 0, sizeof(*this)); }
    HRESULT Init(IStream *pStream);
    HRESULT BeginSaveStream(DWORD index, DWORD dwCheck, IStream **ppStream);
    HRESULT EndSaveStream();
    HRESULT Finish();

    CDataStream _ds;
    ULONG _cSubstreams;
    
    DWORD _dwPoscSubstreams;
};


//+------------------------------------------------------------------------
//
//  Interface:  IHistoryBindInfo
//
//              The interface for setting/grabbing data to control
//              a history bind.
//
//-------------------------------------------------------------------------

enum
{
    HISTORYOPTION_BINDF     = 0,
    HISTORYOPTION_REFRESH   = 1,
    HISTORYOPTION_Last_Enum
};

interface IHistoryBindInfo : public IOptionArray
{
};


//+------------------------------------------------------------------------
//
//  Class:      CHistoryBindInfo
//
//              Implements IOptionArray
//
//-------------------------------------------------------------------------

class COptionArray :
        public CBaseFT,
        public IOptionArray
{
    typedef CBaseFT super;

public:
    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(COptionArray))

    COptionArray(REFIID iidToImplement) { _iid = iidToImplement; }
    ~COptionArray();
    
    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID * ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IHistoryBindInfo methods

    STDMETHOD(QueryOption)(DWORD dwOption, LPVOID pBuffer, ULONG *pcbBuf);
    STDMETHOD(SetOption)(DWORD dwOption, LPVOID pBuffer, ULONG cbBuf);

private:
    BOOL IndexFromOption(ULONG *pindex, DWORD dwOption);
    
    struct Option
    {
        DWORD  dwOption;
        ULONG  cb;
        void * pv;
    };

    DECLARE_CDataAry(CAryOption, Option, Mt(Mem), Mt(COptionArray_aryOption_pv))
    CAryOption _aryOption;
    IID _iid;
};


HRESULT CreateOptionArray(IOptionArray **ppHistoryBindInfo, REFIID iid);
HRESULT CreateHistoryBindInfo(IHistoryBindInfo **ppHistoryBindInfo);

HRESULT CreateIHtmlLoadOptions(IUnknown * pUnkOuter, IUnknown **ppUnk);

#define WSZ_HISTORYBINDINFO _T("HistoryBindInfo")

#pragma INCMSG("--- End 'perhist.hxx'")
#else
#pragma INCMSG("*** Dup 'perhist.hxx'")
#endif
