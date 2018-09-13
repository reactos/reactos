//+----------------------------------------------------------------------------
//
// File:        qnotifywrap.cxx
//
// Contents:    Wrapper for notification calls for QuillLayout
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
// @doc INTERNAL
//-----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X__LINE_H_
#define X__LINE_H_
#include "_line.h"
#endif

#ifndef X_QUILGLUE_HXX_
#define X_QUILGLUE_HXX_
#include "quilglue.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

class CNotificationWrapper : public ITextChangeNotification
{
	//
    // IUnknown declarations
    //
    DECLARE_FORMS_STANDARD_IUNKNOWN(CNotificationWrapper)

	//// ITextChangeNotification
    STDMETHODIMP_(long) Type();
    STDMETHODIMP_(BOOL)        IsType(long ntype);
    STDMETHODIMP_(long) AntiType();
    STDMETHODIMP_(BOOL)        IsTextChange();
    STDMETHODIMP_(BOOL)        IsTreeChange();
    STDMETHODIMP_(BOOL)        IsLayoutChange();
    STDMETHODIMP_(BOOL)        IsForOleSites();
    STDMETHODIMP_(BOOL)        IsForAllElements();
    STDMETHODIMP_(BOOL)        IsFlagSet(long f);
    STDMETHODIMP_(DWORD)       LayoutFlags();

    STDMETHODIMP_(void)        Data(long * pl);

    STDMETHODIMP_(void *)      Node();
    STDMETHODIMP_(void *)      Element();
    STDMETHODIMP_(void *)      Handler();
    STDMETHODIMP_(long)        SI();
    STDMETHODIMP_(long)        CElements();
    STDMETHODIMP_(long)        ElementsChanged();
    STDMETHODIMP_(long)        Cp();
    STDMETHODIMP_(long)        Cch();
    STDMETHODIMP_(long)        CchChanged();
    STDMETHODIMP_(long)        Run();
    STDMETHODIMP_(long)        NRuns();
    STDMETHODIMP_(long)        Ich();

    STDMETHODIMP_(DWORD)       SerialNumber();
    STDMETHODIMP_(BOOL)        IsReceived(DWORD sn);

public:
    // initialization
    void SetNf(CNotification *pnf)
    {
        _nf = *pnf;
    }

    void SetTextRange(
        long        cp,
        long        cch,
        long        iRun,
        long        nRuns,
        long        ich)
    {
        _nf.SetTextRange(cp, cch);
    }

protected:
	CNotification	_nf;	// wrapped notification
};

//+------------------------------------------------------------------------
//
//  Member:     CQuillGlue::QueryInterface, IUnknown
//
//  Synopsis:   QI.
//
//-------------------------------------------------------------------------
HRESULT CNotificationWrapper::QueryInterface(REFIID riid, LPVOID * ppv)
{
	// NO ONE NEEDS TO QI THIS. IMPLEMENT IF NEEDED.
    *ppv = NULL;
    return E_NOINTERFACE;
}

long        CNotificationWrapper::Type()
{
	return _nf.Type();
}

BOOL        CNotificationWrapper::IsType(long ntype)
{
	return _nf.IsType((NOTIFYTYPE)ntype);
}

long        CNotificationWrapper::AntiType()
{
	return _nf.AntiType();
}

BOOL        CNotificationWrapper::IsTextChange()
{
	return _nf.IsTextChange();
}

BOOL        CNotificationWrapper::IsTreeChange()
{
	return _nf.IsTreeChange();
}

BOOL        CNotificationWrapper::IsLayoutChange()
{
	return _nf.IsLayoutChange();
}

BOOL        CNotificationWrapper::IsForOleSites()
{
#ifdef REVIEW   // sidda: fun tree
	return _nf.IsForOleSites();
#endif  // REVIEW
    return 0;
}

BOOL        CNotificationWrapper::IsForAllElements()
{
#ifdef REVIEW   // sidda: fun tree
	return _nf.IsForAllElements();
#endif  // REVIEW
    return 0;
}

BOOL        CNotificationWrapper::IsFlagSet(long f)
{
    return _nf.IsFlagSet((NOTIFYFLAGS) f);
}

DWORD       CNotificationWrapper::LayoutFlags()
{
    return _nf.LayoutFlags();
}

void        CNotificationWrapper::Data(long * pl)
{
	_nf.Data(pl);
}

void *      CNotificationWrapper::Node()
{
	return _nf.Node();
}

void *      CNotificationWrapper::Element()
{
	return _nf.Element();
}

void *      CNotificationWrapper::Handler()
{
	return _nf.Handler();
}

long        CNotificationWrapper::SI()
{
	return _nf.SI();
}

long        CNotificationWrapper::CElements()
{
	return _nf.CElements();
}

long        CNotificationWrapper::ElementsChanged()
{
	return _nf.ElementsChanged();
}

long        CNotificationWrapper::Cp()
{
    // NOTE: CQuillNotificationWrapper overloads this to translate to relative CP
	return _nf.Cp();
}

long        CNotificationWrapper::Cch()
{
	return _nf.Cch();
}

long        CNotificationWrapper::CchChanged()
{
	return _nf.CchChanged();
}

long        CNotificationWrapper::Run()
{
#ifdef REVIEW   // sidda: fun tree
	return _nf.Run();
#endif  // REVIEW
    return 0;
}

long        CNotificationWrapper::NRuns()
{
#ifdef REVIEW   // sidda: fun tree
	return _nf.NRuns();
#endif  // REVIEW
    return 0;
}

long        CNotificationWrapper::Ich()
{
#ifdef REVIEW   // sidda: fun tree
	return _nf.Ich();
#endif  // REVIEW
    return 0;
}

DWORD       CNotificationWrapper::SerialNumber()
{
	return _nf.SerialNumber();
}

BOOL        CNotificationWrapper::IsReceived(DWORD sn)
{
	return _nf.IsReceived(sn);
}

//
// CQuillNotificationWrapper overrides Cp() to translate
// absolute cp to relative cp when needed
//

class CQuillNotificationWrapper : public CNotificationWrapper
{
    typedef CQuillNotificationWrapper super;

public:
    // constructor: stores the notification and pointer to CQuillGlue
    CQuillNotificationWrapper (CQuillGlue *pqg, CNotification *pnf);

    // method overload, converts CP to site-relative
    STDMETHODIMP_(long) Cp();
   
private:
    CQuillGlue *_pqg;
};

// constructor: stores the notification and pointer to CQuillGlue
CQuillNotificationWrapper::CQuillNotificationWrapper(CQuillGlue *pqg, CNotification *pnf)
{
    // Assert that major flags have not changed 
    // (must update QuillSite.idl if flags change)

    Assert(QNFLAGS_ANCESTORS         == NFLAGS_ANCESTORS       );
    Assert(QNFLAGS_SELF              == NFLAGS_SELF            );
    Assert(QNFLAGS_DESCENDENTS       == NFLAGS_DESCENDENTS     );
    Assert(QNFLAGS_TREELEVEL         == NFLAGS_TREELEVEL       );
    Assert(QNFLAGS_TEXTCHANGE        == NFLAGS_TEXTCHANGE      );
    Assert(QNFLAGS_TREECHANGE        == NFLAGS_TREECHANGE      );
    Assert(QNFLAGS_LAYOUTCHANGE      == NFLAGS_LAYOUTCHANGE    );
    Assert(QNFLAGS_FOR_ACTIVEX       == NFLAGS_FOR_ACTIVEX     );
    Assert(QNFLAGS_FOR_LAYOUTS       == NFLAGS_FOR_LAYOUTS     );
    Assert(QNFLAGS_FOR_POSITIONED    == NFLAGS_FOR_POSITIONED  );
    Assert(QNFLAGS_FOR_ALLELEMENTS   == NFLAGS_FOR_ALLELEMENTS );
    Assert(QNFLAGS_CATEGORYMASK      == NFLAGS_CATEGORYMASK    );
    Assert(QNFLAGS_SENDUNTILHANDLED  == NFLAGS_SENDUNTILHANDLED);
    Assert(QNFLAGS_LAZYRANGE         == NFLAGS_LAZYRANGE       );
    Assert(QNFLAGS_CLEANCHANGE       == NFLAGS_CLEANCHANGE     );
    Assert(QNFLAGS_CALLSUPERLAST     == NFLAGS_CALLSUPERLAST   );
    Assert(QNFLAGS_DATAISVALID       == NFLAGS_DATAISVALID     );
    Assert(QNFLAGS_SYNCHRONOUSONLY   == NFLAGS_SYNCHRONOUSONLY );
    Assert(QNFLAGS_DONOTBLOCK        == NFLAGS_DONOTBLOCK      );
    Assert(QNFLAGS_PROPERTYMASK      == NFLAGS_PROPERTYMASK    );
    Assert(QNFLAGS_FORCE             == NFLAGS_FORCE           );
    Assert(QNFLAGS_SENDENDED         == NFLAGS_SENDENDED       );
    Assert(QNFLAGS_DONOTLAYOUT       == NFLAGS_DONOTLAYOUT     );
    Assert(QNFLAGS_CONTROLMASK       == NFLAGS_CONTROLMASK     );

    _pqg = pqg;
    SetNf(pnf);
}

// method overload, converts CP to site-relative
long CQuillNotificationWrapper::Cp()
{
    // $PERF alexmog: relative CP can be cached if
    //       CpRelFromCpAbs is found to be expensive.
	return _pqg->CpRelFromCpAbs(_nf.Cp());
}


//
// Translate notifications and send to the view
//
HRESULT CQuillGlue::Notify(CNotification *pnf)
{
    if (m_ptle)
    {
        // create wrapper
        CQuillNotificationWrapper nfw(this, pnf);

        // call layout engine
        BOOL fRequestLayout;
        m_ptle->Notify((ITextChangeNotification *)&nfw, &fRequestLayout);

        if (fRequestLayout && !pnf->IsFlagSet(NFLAGS_DONOTLAYOUT))
        {
            RequestLayout(pnf->LayoutFlags());
        }

        return S_OK;
    }

    return S_FALSE;
}

