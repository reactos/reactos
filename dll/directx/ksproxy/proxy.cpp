/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/proxy.cpp
 * PURPOSE:         IKsProxy interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

/*
    Needs IKsClock, IKsNotifyEvent
*/

class CKsProxy : public IBaseFilter,
                 public IAMovieSetup,
                 public IPersistStream,
                 public ISpecifyPropertyPages,
                 public IPersistPropertyBag,
                 public IReferenceClock,
                 public IMediaSeeking,
                 public IKsObject,
                 public IKsPropertySet,
                 public IKsClockPropertySet,
                 public IAMFilterMiscFlags,
                 public IKsControl,
                 public IKsTopology,
                 public IKsAggregateControl,
                 public IAMDeviceRemoval
{


};



HRESULT
WINAPI
CKsProxy_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    OutputDebugString("CKsProxy_Constructor UNIMPLEMENTED\n");
    return E_NOTIMPL;
}
