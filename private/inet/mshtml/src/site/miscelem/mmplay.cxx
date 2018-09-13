#include "headers.hxx"

#ifndef X_MMPLAY_HXX_
#define X_MMPLAY_HXX_
#include "mmplay.hxx"
#endif

#ifndef X_STRMIF_H_
#define X_STRMIF_H_
#include <strmif.h>
#endif

#ifndef X_CONTROL_H_
#define X_CONTROL_H_
#include <control.h>
#endif

#ifndef X_EVCODE_H_
#define X_EVCODE_H_
#include "evcode.h"
#endif

#ifndef X_UUIDS_H_
#define X_UUIDS_H_
#include <uuids.h>
#endif

#ifdef WIN16
#ifndef X_MMSYSTEM_H_
#define X_MMSYSTEM_H_
#include <mmsystem.h>
#endif

WORD MapFileToDeviceType(LPCTSTR);
#endif

#define DEFAULT_VIDEOWIDTH 32   // bugbug default size?
#define DEFAULT_VIDEOHEIGHT 32

MtDefine(CIEMediaPlayer, Dwn, "CIEMediaPlayer")
MtDefine(CIEMediaPlayerUrl, CIEMediaPlayer, "CIEMediaPlayer::_pchUrl")

// ======================================================================
//
// ======================================================================
CIEMediaPlayer::CIEMediaPlayer()
{
    _ulRefs = 1;        // born with 1
    _fState = IEMM_Uninitialized;
#ifdef WIN16
    _wMCIDeviceID = -1;
#else
    _pGraph = NULL;
#endif
    _pchURL = NULL;
    _hwndOwner = NULL;
    _fHasAudio = FALSE;
    _fHasVideo = FALSE;

    _fDataDownloaded = FALSE;
    _fRestoreVolume = FALSE;
    _lLoopCount = 1;
    _lPlaysDone = 0;
    _lOriginalVol = 1000;       // init it out of range 
    _lOriginalBal = -100000;    // ditto

    _xWidth = DEFAULT_VIDEOWIDTH;   // bugbug default size?
    _yHeight = DEFAULT_VIDEOHEIGHT;
}


// ======================================================================
//
// ======================================================================
CIEMediaPlayer::~CIEMediaPlayer()
{
    if(_fRestoreVolume)
    {
        SetVolume(_lOriginalVol);
        SetBalance(_lOriginalBal);
        Stop();
    }

    DeleteContents();
}
#if !defined(WINCE) && !defined(NO_MEDIA_PLAYER)
// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::QueryInterface (REFIID riid, LPVOID * ppv)
{ 
    if (riid == IID_IUnknown)
    {
        *ppv = (IUnknown *) this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


// ======================================================================
//
// ======================================================================
void CIEMediaPlayer::DeleteContents( void )
{
#ifdef WIN16
    mciSendCommand(_wMCIDeviceID, MCI_CLOSE, 0, 0);
#else
    if (_pGraph) 
    {
        if (_hwndOwner)
        {
            HRESULT hr;
            IVideoWindow * pVW = NULL;
            hr = _pGraph->QueryInterface(IID_IVideoWindow, (void **) &pVW);
            if (OK(hr))
            {
                pVW->put_MessageDrain((OAHWND) NULL);
                pVW->Release();
                _hwndOwner = NULL;
            }
        }
        _pGraph->Release();
        _pGraph = NULL;
    }
#endif // ndef WIN16

    if(_pchURL)
    {
        MemFreeString(_pchURL);
        _pchURL = NULL;
    }

    _fState = IEMM_Uninitialized;
}

    
// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::Initialize(void) 
{
    HRESULT hr; // return code

#ifndef WIN16
    if(_pGraph)         // already initialized
    {
        _pGraph->Release(); // go away
        _pGraph = NULL;
    }


    hr = CoCreateInstance(CLSID_FilterGraph,    // get this documents graph object
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IGraphBuilder,
                          (void **) &_pGraph);

    if (FAILED(hr)) 
    {
        DeleteContents();
        return hr;
    }
#endif // ndef WIN16

    _fState = IEMM_Initialized;
    return S_OK;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetURL(const TCHAR  *pchURL)
{
    HRESULT hr = S_OK, hr2 = S_OK;
    IVideoWindow * pVW;

    if(!pchURL)
        return ERROR_INVALID_PARAMETER;

    MemReplaceString(Mt(CIEMediaPlayerUrl), pchURL, &_pchURL);
    
#ifdef WIN16
    mciSendCommand(_wMCIDeviceID, MCI_CLOSE, 0, 0);
#else
    if(_pGraph)
        Stop();     // We already have a graph built so call stop in case it's running
#endif // ndef WIN16

#if 1   // BUGBUG: Davidna:     Do this until AMovie implements ReleaseAllFilters()
    hr = Initialize();       // This will release the Graph and CoCreateInstance() a new one.
#else
    hr = _pGraph->ReleaseAllFilters();       // Someday...
#endif

    if (FAILED(hr))
        goto Failed;

    _fDataDownloaded = TRUE;    // we we're passed in a valid URL

#ifdef WIN16
    MCI_OPEN_PARMS mciOpenParms;

    /*
     * Open the device by specifying the
     * device name and device element.
     * MCI will attempt to choose the
     * MIDI Mapper as the output port.
     */
    //mciOpenParms.lpstrDeviceType = (LPSTR)(LONG)MapFileToDeviceType(pchURL);
    mciOpenParms.lpstrElementName = _pchURL;
    if (hr = (HRESULT) mciSendCommand(NULL, MCI_OPEN,
            /*MCI_OPEN_TYPE |  */ MCI_OPEN_ELEMENT,
            (DWORD)(LPVOID) &mciOpenParms)) {
        
         /*
         * Failed to open device;
         * don't close it, just return error.
         */
        _fState = IEMM_Aborted;
        goto Failed;
    }
    else
    {
        _fState = IEMM_Stopped;
    }

    _wMCIDeviceID = mciOpenParms.wDeviceID;
#else
    // Build the graph.
    //
    // This won't return until the the file type is sniffed and the appropriate
    // graph is built
    //
    hr = _pGraph->RenderFile(_pchURL, NULL);
    if(SUCCEEDED(hr))
    {
        _fState = IEMM_Stopped;
    }
    else
    {
        _fState = IEMM_Aborted;
        goto Failed;
    }

    // Need to check to see if there's a Video renderer interface and
    // shut it off if it's there. Our default state is to show no window until
    // someone sets our window position
    //
    // For BGSound this allows video files to be used without having a video window
    //  pop up on us.
    // For DYNSRC a video window size will be set at which point we'll 
    //  turn the thing on again.
    
    hr = _pGraph->QueryInterface(IID_IVideoWindow, (void **) &pVW);
    if(OK(hr)) 
    {
        long lVisible;

        // if this fails then we have an audio only stream
        hr2 = pVW->get_Visible(&lVisible);  
        if(hr2 == S_OK)
        {
            SIZE size;
            GetSize(&size);             // this will get the size of the video source
                                        // and cache the results for later

            hr2 = pVW->put_AutoShow(0);  // turn off the auto show of the video window
            _fHasVideo = TRUE;
        }
        else
        {
            _xWidth = 0;
            _yHeight = 0;
        }
        pVW->Release();
    }

    _fUseSegments = FALSE;

    IMediaSeeking *pIMediaSeeking;
    hr = _pGraph->QueryInterface(IID_IMediaSeeking, (void **) &pIMediaSeeking);
    if( SUCCEEDED(hr))
    {
        // See if Segment seeking is supported (for Seamless looping)
        if (pIMediaSeeking)
        {
            DWORD dwCaps = AM_SEEKING_CanDoSegments;
            _fUseSegments =
                (S_OK == pIMediaSeeking->CheckCapabilities(&dwCaps));
        } 
        pIMediaSeeking->Release();
    }

    IBasicAudio *pIBa;
    long lOriginalVolume, lOriginalBalance;

    lOriginalVolume = lOriginalBalance = 0;

    hr = _pGraph->QueryInterface(IID_IBasicAudio, (void **) &pIBa);
    if( SUCCEEDED(hr))
    {
        hr2 = pIBa->get_Volume(&lOriginalVolume);
        if(hr2 == S_OK)
        {
            _fHasAudio = TRUE;
            pIBa->get_Balance(&lOriginalBalance);
        }
        pIBa->Release();
    }

    // save away the original volume so that we can restore it on our way out
    if(_fHasAudio && _lOriginalVol > 0)
    {
        _lOriginalBal = lOriginalBalance;
        _lOriginalVol = lOriginalVolume;
    }

#endif // ndef WIN16

Failed:
    return hr;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetVideoWindow(HWND hwnd)
{
    IVideoWindow * pVW = NULL;
    HRESULT        hr  = S_OK;

    if(!_fHasVideo)
        return S_FALSE;

#ifndef WIN16
    if(!_pGraph)
        return E_FAIL;

    if (hwnd)
    {
        _hwndOwner = hwnd;

        hr = _pGraph->QueryInterface(IID_IVideoWindow, (void **) &pVW);
        if (FAILED(hr))
            goto Cleanup;

        hr = pVW->put_Owner((OAHWND) hwnd);
        if (FAILED(hr))
            goto Cleanup;

        hr = pVW->put_MessageDrain((OAHWND) hwnd);
        if (FAILED(hr))
            goto Cleanup;

        hr = pVW->put_WindowStyle(WS_CHILDWINDOW);
        if (FAILED(hr))
            goto Cleanup;

        hr = pVW->put_BackgroundPalette(-1); // OATRUE
        if (FAILED(hr))
            goto Cleanup;
    }
#endif // ndef WIN16

Cleanup:
    if (pVW)
        pVW->Release();

    RRETURN(hr);
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetWindowPosition(RECT *prc)
{
    HRESULT hr;
    IVideoWindow * pVW = NULL;

    Assert(prc);

    if (!_fHasVideo)
        return S_FALSE;

#ifndef WIN16
    if (!_pGraph)
        return E_FAIL;

    hr = _pGraph->QueryInterface(IID_IVideoWindow, (void **) &pVW);
    if (FAILED(hr))
        goto Cleanup;

    hr = pVW->SetWindowPosition(prc->left,
                                prc->top,
                                prc->right - prc->left,
                                prc->bottom - prc->top);
#endif // ndef WIN16

Cleanup:
    if (pVW)
        pVW->Release();

    RRETURN(hr);

}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetVisible(BOOL fVisible)
{
    HRESULT hr;
    IVideoWindow * pVW = NULL;

    if(!_fHasVideo)
        return S_FALSE;

#ifndef WIN16
    if(!_pGraph)
        return E_FAIL;

    hr = _pGraph->QueryInterface(IID_IVideoWindow, (void **) &pVW);
    if (OK(hr))
    {
        if(fVisible)
            hr = pVW->put_Visible(-1);  // OATRUE
        else 
            hr = pVW->put_Visible(0);   // OAFALSE
    }

    if (pVW)
        pVW->Release();
#endif // ndef WIN16

    return hr;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::GetSize(SIZE *psize)
{
#ifdef WIN16
    psize->cx = psize->cy = 0;
    return S_OK;
#else
    long    lWidth = 0;
    long    lHeight = 0;
    HRESULT hr = S_OK;
    IBasicVideo * pBV = NULL;

    if(!_pGraph)
        return E_FAIL;

    if(_xWidth == DEFAULT_VIDEOWIDTH && _yHeight == DEFAULT_VIDEOHEIGHT)
    {
        hr = _pGraph->QueryInterface(IID_IBasicVideo, (void **) &pBV);

        if (OK(hr))
            hr = pBV->get_SourceWidth(&lWidth);

        if (OK(hr))
            hr = pBV->get_SourceHeight(&lHeight);
        
        if (OK(hr))
        {
            _xWidth = lWidth;
            _yHeight = lHeight;
        }
    }

    psize->cx = _xWidth;
    psize->cy = _yHeight;

    if (pBV)
        pBV->Release();

    return hr;
#endif
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetNotifyWindow(HWND hwnd, long lmsg, long lParam)
{
#ifdef WIN16
    return E_FAIL;
#else
    HRESULT hr = S_OK;
    IMediaEventEx *pMvEx = NULL;

    if(!_pGraph)
        return E_FAIL;

    hr = _pGraph->QueryInterface(IID_IMediaEventEx, (void **) &pMvEx);
    if (FAILED(hr))
        goto Failed;

    hr = pMvEx->SetNotifyWindow((OAHWND) hwnd, lmsg, lParam);
    
    pMvEx->Release();

Failed:
    return hr;
#endif
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetLoopCount(long uLoopCount)
{
    _lLoopCount = uLoopCount;
    _lPlaysDone = 0;
    return S_OK;
}

// ======================================================================
//
// ======================================================================
long CIEMediaPlayer::GetVolume(void)
{
#ifdef WIN16
    return E_FAIL;
#else
    HRESULT hr;
    IBasicAudio *pIBa;
    long lTheVolume=E_FAIL; // BUGBUG Arye: shouldn't this be zero?

    if(_pGraph)
    {
        hr = _pGraph->QueryInterface(IID_IBasicAudio, (void **) &pIBa);
        if( SUCCEEDED(hr) && pIBa)
        {
            hr = pIBa->get_Volume(&lTheVolume);
            pIBa->Release();
        }
    }
    return lTheVolume;
#endif
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetVolume(long lVol)
{
#ifdef WIN16
    return E_FAIL;
#else
    HRESULT hr = S_OK;

    IBasicAudio *pIBa;

    if(lVol < -10000 && lVol > 0)
        return ERROR_INVALID_PARAMETER;

    if(!_pGraph)
        return E_FAIL;

    hr = _pGraph->QueryInterface(IID_IBasicAudio, (void **) &pIBa);
    if( SUCCEEDED(hr) && pIBa)
    {
        hr = pIBa->put_Volume(lVol);
        pIBa->Release();
    }
    _fRestoreVolume = TRUE;

    return hr;
#endif // ndef WIN16
}

// ======================================================================
//
// ======================================================================
long CIEMediaPlayer::GetBalance(void)
{
#ifdef WIN16
    return E_FAIL;
#else
    HRESULT hr;

    IBasicAudio *pIBa;
    long lTheBal=E_FAIL;

    if(_pGraph)
    {
        hr = _pGraph->QueryInterface(IID_IBasicAudio, (void **) &pIBa);
        if( SUCCEEDED(hr) && pIBa)
        {
            hr = pIBa->get_Balance(&lTheBal);
            pIBa->Release();
        }
    }

    return lTheBal;
#endif // ndef WIN16
}   
// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetBalance(long lBal)
{
#ifdef WIN16
    return E_FAIL;
#else
    HRESULT hr = S_OK;

    IBasicAudio *pIBa;

    if(lBal < -10000 && lBal > 10000)
        return ERROR_INVALID_PARAMETER;

    if(!_pGraph)
        return E_FAIL;

    hr = _pGraph->QueryInterface(IID_IBasicAudio, (void **) &pIBa);
    if( SUCCEEDED(hr) && pIBa)
    {
        hr = pIBa->put_Balance(lBal);
        pIBa->Release();
    }

    _fRestoreVolume = TRUE;

    return hr;
#endif // ndef WIN16
}

// ======================================================================
//
// ======================================================================
int CIEMediaPlayer::GetStatus(void)
{
    return _fState;
}


// ======================================================================
//
// CIEMediaPlayer commands
// ======================================================================
//
HRESULT CIEMediaPlayer::Play()
{
    HRESULT hr = S_OK;

    if (CanPlay())
    {
#ifdef WIN16
        MCI_PLAY_PARMS mciPlayParms = { 0,0,0};
        /*
        * Begin playback. The window procedure function
        * for the parent window is notified with an
        * MM_MCINOTIFY message when playback is complete.
        * The window procedure then closes the device.
        */
        if (hr = mciSendCommand(_wMCIDeviceID, MCI_PLAY,
            0, (DWORD)(LPVOID) &mciPlayParms)) {
            mciSendCommand(_wMCIDeviceID, MCI_CLOSE, 0, 0);
            return hr;
        }
#else
        IMediaControl *pMC = NULL;

        // Obtain the interface to our filter graph
        //
        hr = _pGraph->QueryInterface(IID_IMediaControl, (void **) &pMC);
        if (SUCCEEDED(hr) && pMC)
        {
            if (_fUseSegments)
            {
                // If we're using seamless looping, we need to 1st set seeking flags
                Seek(0);
            }

            // Ask the filter graph to play 
            hr = pMC->Run();

            if (SUCCEEDED(hr))
            {
                _fState = IEMM_Playing;
                if (_lLoopCount > 0)
                    _lPlaysDone++;
            }
            else
            {
                pMC->Stop();    // some filters in the graph may have started
                                // so we better stop them
            }
            pMC->Release();
        }
        else
            hr = S_FALSE;
#endif
    }

    RRETURN1(hr, S_FALSE);
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::Pause()
{
    HRESULT hr = S_OK;

    if( CanPause() )
    {
#ifdef WIN16
		// we are not too sophisticated, we just stop & start it.
		mciSendCommand(_wMCIDeviceID, MCI_CLOSE, 0, 0);
#else
        IMediaControl *pMC;
        // Obtain the interface to our filter graph
        hr = _pGraph->QueryInterface(IID_IMediaControl, (void **) &pMC);

        // Ask the filter graph to pause
        if( SUCCEEDED(hr ) )
            pMC->Pause();
        
        pMC->Release();

        _fState = IEMM_Paused;
#endif
    }
    else
        hr = S_FALSE;

    RRETURN1(hr, S_FALSE);
}

// ======================================================================
// 
//  Stop playback if it's active
//
// ======================================================================
HRESULT CIEMediaPlayer::Abort()
{
    // Must stop play first.
    //
    Stop();

    _fState = IEMM_Aborted;

    return S_OK;   // must not fail
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::Seek(ULONG uPosition)
{
#ifdef WIN16
    Assert(0);
    return S_FALSE;
#else
    HRESULT hr = S_OK;

    IMediaSeeking *pIMediaSeeking=NULL;
    if(_pGraph)
        hr = _pGraph->QueryInterface(IID_IMediaSeeking, (void **) &pIMediaSeeking);

    if( SUCCEEDED(hr) && pIMediaSeeking)
    {
        LONGLONG llStop;

        hr = pIMediaSeeking->GetPositions(NULL, &llStop);
        if (SUCCEEDED(hr) && (llStop > uPosition))
        {
            long lSegmentSeek = 0L;
            LONGLONG llPosition = (LONGLONG) uPosition;

            SetSegmentSeekFlags(&lSegmentSeek); // in case we're seamless looping
            hr = pIMediaSeeking->SetPositions( &llPosition
                                              , AM_SEEKING_AbsolutePositioning | lSegmentSeek
                                              , &llStop
                                              , AM_SEEKING_NoPositioning );
        }
        pIMediaSeeking->Release();
    }

    RRETURN1(hr, S_FALSE);
#endif // ndef WIN16
}

// ======================================================================
//
// ======================================================================
void CIEMediaPlayer::SetSegmentSeekFlags(LONG *plSegmentSeek)
{
#ifdef WIN16
    Assert(0);
#else
    if (plSegmentSeek)
    {
        *plSegmentSeek = 
            _fUseSegments && _lLoopCount != 1 ?
    	        ((_lLoopCount == -1) ||
	             (_lLoopCount > _lPlaysDone + 1) ? AM_SEEKING_NoFlush |
		        	  AM_SEEKING_Segment 
	            	: AM_SEEKING_NoFlush
                ) :  0L;
    }
#endif // ndef WIN16
}


// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::Stop()
{
    HRESULT hr = S_OK;

    if( CanStop() )
    {
#ifdef WIN16
        mciSendCommand(_wMCIDeviceID, MCI_CLOSE, 0, 0);
        Assert(0);
#else
        IMediaControl *pMC;

        // Obtain the interface to our filter graph
        hr = _pGraph->QueryInterface(IID_IMediaControl, (void **) &pMC);
        if( SUCCEEDED(hr) )
        {
            // Stop the filter graph
            hr = pMC->Stop();
            // Release the interface
            pMC->Release();

            // set the flags
            _fState = IEMM_Stopped;
        }
#endif
    }
    else
        hr = S_FALSE;

    RRETURN1(hr, S_FALSE);
}

//
// ======================================================================
//
// If the event handle is valid, ask the graph
// if anything has happened. eg the graph has stopped...
// ======================================================================
HRESULT CIEMediaPlayer::NotifyEvent(void) 
{
#ifdef WIN16
	Assert(0);
    return E_FAIL;
#else
    HRESULT hr = S_OK;
    long lEventCode;
    LPARAM lParam1, lParam2;
    IMediaEvent *pME = NULL;

    if(!_pGraph)
        return E_FAIL;

    hr = _pGraph->QueryInterface(IID_IMediaEvent, (void **) &pME); 
    if( FAILED(hr) )
        goto GN_Failed;

    hr = pME->GetEvent(&lEventCode, &lParam1, &lParam2, 0);
    if( FAILED(hr) )
        goto GN_Failed;

    if (lEventCode == EC_COMPLETE || lEventCode == EC_END_OF_SEGMENT)
    {
        // Do we need to loop?
        //
        if(_lLoopCount == -1 || _lLoopCount > _lPlaysDone )
        {
            Seek(0);        // we're still playing so seek back to the begining
                            // and we'll keep going
            if(_lLoopCount >0)
                _lPlaysDone++;
        }
        else    
        {
            // we're done stop the graph
            Stop();
            _fState = IEMM_Completed;
        }

    } else if ((lEventCode == EC_ERRORABORT) || (lEventCode == EC_USERABORT)      ) 
    {
        Stop();
    }

GN_Failed:
    if(pME) pME->Release();

    RRETURN1(hr, S_FALSE);
#endif
}

#else // !WINCE && !NO_MEDIA_PLAYER

// Just stub these methods out for GALAHAD / UNIX

// ======================================================================
//
// ======================================================================
void CIEMediaPlayer::DeleteContents( void )
{
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::QueryInterface (REFIID riid, LPVOID * ppv)
{ 
    *ppv = NULL;
    return E_NOINTERFACE;
}
    
// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::Initialize(void) 
{
    return E_FAIL;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetURL(const TCHAR  *pchURL)
{
	return E_FAIL;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetVideoWindow(HWND hwnd)
{
	return E_FAIL;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetWindowPosition(RECT *prc)
{
	return E_FAIL;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetVisible(BOOL fVisible)
{
	return E_FAIL;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::GetSize(SIZE *psize)
{
	return E_FAIL;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetNotifyWindow(HWND hwnd, long lmsg, long lParam)
{
	return E_FAIL;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetLoopCount(long uLoopCount)
{
    return E_FAIL;
}

// ======================================================================
//
// ======================================================================
long CIEMediaPlayer::GetVolume(void)
{
	return E_FAIL;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetVolume(long lVol)
{
	return E_FAIL;
}

// ======================================================================
//
// ======================================================================
long CIEMediaPlayer::GetBalance(void)
{
	return E_FAIL;
}
// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::SetBalance(long lBal)
{
	return E_FAIL;
}

// ======================================================================
//
// ======================================================================
int CIEMediaPlayer::GetStatus(void)
{
    return _fState;
}


// ======================================================================
//
// CIEMediaPlayer commands
// ======================================================================
//
HRESULT CIEMediaPlayer::Play()
{
	return E_FAIL;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::Pause()
{
	return E_FAIL;
}

// ======================================================================
// 
//  Stop playback if it's active
//
// ======================================================================
HRESULT CIEMediaPlayer::Abort()
{
	return E_FAIL;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::Seek(ULONG uPosition)
{
	return E_FAIL;
}

// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::Stop()
{
	return E_FAIL;
}

//
// ======================================================================
//
// ======================================================================
HRESULT CIEMediaPlayer::NotifyEvent(void) 
{
	return E_FAIL;
}

#endif // WINCE && NO_MEDIA_PLAYER

