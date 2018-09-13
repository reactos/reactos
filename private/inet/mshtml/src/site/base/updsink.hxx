#ifndef I_UPDSINK_HXX_
#define I_UPDSINK_HXX_
#pragma INCMSG("--- Beg 'updsink.hxx'")

#ifndef X_OCMM_H_
#define X_OCMM_H_
#pragma INCMSG("--- Beg <ocmm.h>")
#include <ocmm.h>
#pragma INCMSG("--- End <ocmm.h>")
#endif

MtExtern(CDocUpdateIntSink)

//+---------------------------------------------------------------------------
//
//  Flag values for state of UpdateInterval accumulated hrgn
//
//----------------------------------------------------------------------------

enum UPDATEINTERVAL_STATE
{
    UPDATEINTERVAL_EMPTY,       // No HRGN, _hrgn is NULL, don't RedrawWindow
    UPDATEINTERVAL_REGION,      // HRGN present, RedrawWindow
    UPDATEINTERVAL_ALL,         // No HRGN, _hrgn is NULL, Redraw entire client area, stop accumulating
    UPDATEINTERVAL_STATE_Last_Enum
};

//+---------------------------------------------------------------
//
//  Class:      CDocUpdateIntSink
//
//  Purpose:    Timer Sink object for CDoc's updateInterval
//              property, which throttles invalidates for dueling
//              controls and other things. All related info
//              held here for centralization, and because prop is
//              not oft used.
//---------------------------------------------------------------
class CDocUpdateIntSink : public ITimerSink
{
    friend class CDoc;

public :

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDocUpdateIntSink))
    CDocUpdateIntSink( CDoc *pDoc );
    ~CDocUpdateIntSink(void);

    // IUnknown methods
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface (REFIID iid, void **ppvObj);

    // ITimerSink methods
    STDMETHODIMP OnTimer        ( VARIANT vtimeAdvise );

private :
    ULONG                   _ulRefs;
    CDoc                   *_pDoc;
    ITimer                 *_pTimer;    // Timer object managed by CDoc, just held by us
    DWORD                   _cookie;
    LONG                    _interval;  // time between paint updates for controls.
    HRGN                    _hrgn;      // accumulated region between update interval;
    DWORD                   _dwFlags;   // flags to send along to Invalidate
    UPDATEINTERVAL_STATE    _state;     // see comments for enum

};

#pragma INCMSG("--- End 'updsink.hxx'")
#else
#pragma INCMSG("*** Dup 'updsink.hxx'")
#endif
