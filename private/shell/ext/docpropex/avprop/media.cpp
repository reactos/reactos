#include "pch.h"
#include "resource.h"
#include "media.h"

#define FOURCC_INFO mmioFOURCC('I','N','F','O')
#define FOURCC_DISP mmioFOURCC('D','I','S','P')
#define FOURCC_IARL mmioFOURCC('I','A','R','L')
#define FOURCC_IART mmioFOURCC('I','A','R','T')
#define FOURCC_ICMS mmioFOURCC('I','C','M','S')
#define FOURCC_ICMT mmioFOURCC('I','C','M','T')
#define FOURCC_ICOP mmioFOURCC('I','C','O','P')
#define FOURCC_ICRD mmioFOURCC('I','C','R','D')
#define FOURCC_ICRP mmioFOURCC('I','C','R','P')
#define FOURCC_IDIM mmioFOURCC('I','D','I','M')
#define FOURCC_IDPI mmioFOURCC('I','D','P','I')
#define FOURCC_IENG mmioFOURCC('I','E','N','G')
#define FOURCC_IGNR mmioFOURCC('I','G','N','R')
#define FOURCC_IKEY mmioFOURCC('I','K','E','Y')
#define FOURCC_ILGT mmioFOURCC('I','L','G','T')
#define FOURCC_IMED mmioFOURCC('I','M','E','D')
#define FOURCC_INAM mmioFOURCC('I','N','A','M')
#define FOURCC_IPLT mmioFOURCC('I','P','L','T')
#define FOURCC_IPRD mmioFOURCC('I','P','R','D')
#define FOURCC_ISBJ mmioFOURCC('I','S','B','J')
#define FOURCC_ISFT mmioFOURCC('I','S','F','T')
#define FOURCC_ISHP mmioFOURCC('I','S','H','P')
#define FOURCC_ISRC mmioFOURCC('I','S','R','C')
#define FOURCC_ISRF mmioFOURCC('I','S','R','F')
#define FOURCC_ITCH mmioFOURCC('I','T','C','H')
#define FOURCC_VIDC mmioFOURCC('V','I','D','C')
#define mmioWAVE    mmioFOURCC('W','A','V','E')
#define mmioFMT     mmioFOURCC('f','m','t',' ')
#define mmioDATA    mmioFOURCC('d','a','t','a')
#define MAXNUMSTREAMS   50 

static HANDLE STDMETHODCALLTYPE GetRiffInfo( HMMIO hmmio )
{
    MMCKINFO    ck;
    MMCKINFO    ckINFO;
    MMCKINFO    ckRIFF;
    HANDLE      h = NULL;
    LONG        lSize;
    DWORD       dw;
    BOOL        fDoneDISP;
    BOOL        fDoneINFO;
    BOOL        fDoneName;
    LPSTR pInfo;

    if (!hmmio)
        return NULL ;

    mmioSeek(hmmio, 0, SEEK_SET);

    /* descend the input file into the RIFF chunk */
    if( mmioDescend( hmmio, &ckRIFF, NULL, 0 ) != 0 )
        goto error;

    if( ckRIFF.ckid != FOURCC_RIFF )
        goto error;

    fDoneDISP = fDoneINFO = fDoneName = FALSE;
    while (!(fDoneDISP && fDoneINFO) && !mmioDescend(hmmio, &ck, &ckRIFF, 0))
    {
        if (ck.ckid == FOURCC_DISP)
        {
            /* Read dword into dw, break if read unsuccessful */
            if (mmioRead(hmmio, (char*)&dw, sizeof(dw)) != (LONG)sizeof(dw))
                goto error;

            /* Find out how much memory to allocate */
            lSize = ck.cksize - sizeof(dw);
            if ((int)dw == CF_DIB && h == NULL)
            {
                /* get a handle to memory to hold the description and 
                    lock it down */

                if ((h = GlobalAlloc(GHND, lSize+4)) == NULL)
                    goto error;

                if (mmioRead(hmmio, (char*)GlobalLock(h), lSize) != lSize)
                    goto error;

                fDoneDISP = TRUE;
            }
            else if ((int)dw == CF_TEXT)
            {
                pInfo = (LPSTR)LocalAlloc(LPTR, lSize+1);//+1 not required I think
                if (!pInfo)
                    goto error;

                if (!mmioRead(hmmio, pInfo,  lSize))
                    goto error;

                //AddInfoToList(pmmpsh, pInfo, ck.ckid );
                fDoneName = TRUE;

            }

        }
        else if (ck.ckid    == FOURCC_LIST &&
                 ck.fccType == FOURCC_INFO &&
                 !fDoneINFO)
        {
            while (!mmioDescend(hmmio, &ckINFO, &ck, 0))
            {
                switch (ckINFO.ckid)
                {
                    case FOURCC_ISBJ:
                    case FOURCC_INAM:
                    case FOURCC_ICOP:
                    case FOURCC_ICRD:
                    case FOURCC_IART:
                    case FOURCC_ICMS:
                    case FOURCC_ICMT:        
                    case FOURCC_ICRP:
                    case FOURCC_IDIM:
                    case FOURCC_IARL:
                    case FOURCC_IDPI:
                    case FOURCC_IENG:
                    case FOURCC_IGNR:
                    case FOURCC_IKEY:        
                    case FOURCC_ILGT:
                    case FOURCC_IMED:
                    case FOURCC_IPLT:
                    case FOURCC_IPRD:
                    case FOURCC_ISFT:
                    case FOURCC_ISHP:
                    case FOURCC_ISRC:
                    case FOURCC_ISRF:
                    case FOURCC_ITCH:
                        pInfo = (LPSTR)LocalAlloc(LPTR, ck.cksize+1);//+1 not required I think
                        if (!pInfo)
                            goto error;

                        if (!mmioRead(hmmio, pInfo,  ck.cksize))
                            goto error;

                        //AddInfoToList(pmmpsh, pInfo, ckINFO.ckid);
                        if (ckINFO.ckid == FOURCC_INAM)
                            fDoneName = TRUE;
                        break;
                }

                if (mmioAscend(hmmio, &ckINFO, 0))
                    break;
            }
        }


        /* Ascend so that we can descend into next chunk
         */
        if (mmioAscend(hmmio, &ck, 0))
            break;
    }

    goto exit;

error:
    if (h)
    {
        GlobalUnlock(h);
        GlobalFree(h);
    }
    h = NULL;

exit:
    return h;
}


STDMETHODIMP GetMidiInfo( LPCTSTR pszFile, PMIDIDESC pmidi )
{

#ifdef _MIDI_PROPERTY_SUPPORT_
    MCI_OPEN_PARMS      mciOpen;    /* Structure for MCI_OPEN command */
    DWORD               dwFlags;
    DWORD               dw;
    MCIDEVICEID         wDevID;
    MCI_STATUS_PARMS    mciStatus;
    MCI_SET_PARMS       mciSet;        /* Structure for MCI_SET command */
    MCI_INFO_PARMS      mciInfo;  
        /* Open a file with an explicitly specified device */

    mciOpen.lpstrDeviceType = TEXT("sequencer");
    mciOpen.lpstrElementName = pszFile;
    dwFlags = MCI_WAIT | MCI_OPEN_ELEMENT | MCI_OPEN_TYPE;
    dw = mciSendCommand((MCIDEVICEID)0, MCI_OPEN, dwFlags,(DWORD_PTR)(LPVOID)&mciOpen);
    if (dw)
        return E_FAIL ;
    wDevID = mciOpen.wDeviceID;

    mciSet.dwTimeFormat = MCI_FORMAT_MILLISECONDS;

    dw = mciSendCommand(wDevID, MCI_SET, MCI_SET_TIME_FORMAT,
        (DWORD_PTR) (LPVOID) &mciSet);
    if (dw)
    {
        mciSendCommand(wDevID, MCI_CLOSE, 0L, (DWORD)0);
        return E_FAIL ;
    }

    mciStatus.dwItem = MCI_STATUS_LENGTH;
    dw = mciSendCommand(wDevID, MCI_STATUS, MCI_STATUS_ITEM,
        (DWORD_PTR) (LPTSTR) &mciStatus);
    if (dw)
        pmidi->nLength = 0;
    else
        pmidi->nLength = (UINT)mciStatus.dwReturn;

    mciInfo.dwCallback  = 0;

    mciInfo.lpstrReturn = pmidi->szMidiCopyright;
    mciInfo.dwRetSize   = sizeof(pmidi->szMidiCopyright);
    *mciInfo.lpstrReturn = 0 ;
    mciSendCommand(wDevID, MCI_INFO,  MCI_INFO_COPYRIGHT, (DWORD_PTR)(LPVOID)&mciInfo);

    mciInfo.lpstrReturn = pmidi->szMidiCopyright;
    mciInfo.dwRetSize   = sizeof(pmidi->szMidiSequenceName);
    *mciInfo.lpstrReturn = 0 ;
    mciSendCommand(wDevID, MCI_INFO,  MCI_INFO_NAME, (DWORD_PTR)(LPVOID)&mciInfo);

    mciSendCommand(wDevID, MCI_CLOSE, 0L, (DWORD)0);

    return S_OK ;

#else  _MIDI_PROPERTY_SUPPORT_

    return E_FAIL;

#endif _MIDI_PROPERTY_SUPPORT_
}

//-------------------------------------------------------------------------//
STDMETHODIMP  GetMidiProperty( 
    IN REFFMTID reffmtid, 
    IN PROPID pid, 
    IN const MIDIDESC* pMidi, 
    OUT PROPVARIANT* pVar )
{
    HRESULT hr = S_OK ;
    return hr ;        
}

//-------------------------------------------------------------------------//
//  Wave
//-------------------------------------------------------------------------//

static BOOL PASCAL NEAR ReadWaveHeader(HMMIO hmmio,
    PWAVEDESC    pwd)
{
    BOOL        bRet = FALSE ;
    MMCKINFO    mmckRIFF;
    MMCKINFO    mmck;
    WORD        wFormatSize ;
    MMRESULT    wError;

    ZeroMemory( pwd, sizeof(*pwd) ) ;

    mmckRIFF.fccType = mmioWAVE;
    if( (wError = mmioDescend(hmmio, &mmckRIFF, NULL, MMIO_FINDRIFF)) ) 
    {
        return FALSE;
    }
    
    mmck.ckid = mmioFMT;
    if( (wError = mmioDescend(hmmio, &mmck, &mmckRIFF, MMIO_FINDCHUNK)) ) 
    {
        return FALSE;
    }
    if( mmck.cksize < sizeof(WAVEFORMAT) ) 
    {
        return FALSE;
    }
    
    wFormatSize = (WORD)mmck.cksize;
    if( NULL != (pwd->pwfx = (PWAVEFORMATEX)new BYTE[wFormatSize]) )
    {
        if ((DWORD)mmioRead(hmmio, (HPSTR)pwd->pwfx, mmck.cksize) != mmck.cksize) 
        {
            goto retErr;
        }
        if (pwd->pwfx->wFormatTag == WAVE_FORMAT_PCM) 
        {
            if (wFormatSize < sizeof(PCMWAVEFORMAT)) 
            {
                goto retErr;
            }
        } 
        else if( (wFormatSize < sizeof(WAVEFORMATEX)) || 
                 (wFormatSize < sizeof(WAVEFORMATEX) + pwd->pwfx->cbSize) ) 
        {
            goto retErr ;
        }
        if( (wError = mmioAscend(hmmio, &mmck, 0)) ) 
        {
            goto retErr ;
        }
        mmck.ckid = mmioDATA;
        if( (wError = mmioDescend(hmmio, &mmck, &mmckRIFF, MMIO_FINDCHUNK)) ) 
        {
            goto retErr ;
        }
        pwd->dwSize = mmck.cksize;
        return S_OK ;
    }

retErr:
    delete [] (LPBYTE)pwd->pwfx ;
    return E_FAIL ;
}

//-------------------------------------------------------------------------//
// Retrieves text representation of format tag
STDMETHODIMP  GetWaveFormatTag( PWAVEFORMATEX pwfx, LPTSTR pszTag, IN ULONG cchTag )
{
    ASSERT( pwfx ) ;
    ASSERT( pszTag ) ;
    ASSERT( cchTag ) ;
    
    ACMFORMATTAGDETAILS aftd;
    ZeroMemory( &aftd, sizeof(aftd) ) ;
    aftd.cbStruct    = sizeof(ACMFORMATTAGDETAILSW) ;
    aftd.dwFormatTag = pwfx->wFormatTag ;

    if( 0 == acmFormatTagDetails( NULL, &aftd, ACM_FORMATTAGDETAILSF_FORMATTAG) )
    {
        //  copy to output.
        lstrcpyn( pszTag, aftd.szFormatTag, cchTag ) ;
        return S_OK ;
    }

    return E_FAIL ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP GetWaveInfo( IN LPCTSTR pszFile, OUT PWAVEDESC p )
{
    HMMIO   hmmio ;
    
    if( NULL == (hmmio = mmioOpen( (LPTSTR)pszFile, NULL, MMIO_ALLOCBUF | MMIO_READ)) )
        return E_FAIL ;

    HRESULT hr = ReadWaveHeader(hmmio, p) ;

    mmioClose( hmmio, 0 ) ;

    if( SUCCEEDED( hr ) && p->pwfx )
    {
        // Retrieve text representation of format tag
        GetWaveFormatTag( p->pwfx, p->szWaveFormat, ARRAYSIZE(p->szWaveFormat) ) ;
    }
    return hr ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP FreeWaveInfo( IN OUT PWAVEDESC p )
{
    if( !( p && sizeof(*p) == p->dwSize ) && p->pwfx )
    {
        delete [] p->pwfx ;
        p->pwfx = NULL ;
    }
    return S_OK ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP  _getWaveAudioProperty(
    IN REFFMTID reffmtid,
    IN PROPID   pid,
    IN const PWAVEFORMATEX pwfx,
    OUT PROPVARIANT* pVar )
{
    USES_CONVERSION ;
    
    HRESULT hr = E_UNEXPECTED ;
    TCHAR   szBuf[MAX_DESCRIPTOR],
            szFmt[MAX_DESCRIPTOR] ;

    PropVariantInit( pVar ) ;
    *szBuf = *szFmt = 0 ;

    if( pwfx && IsEqualGUID( reffmtid, FMTID_AudioSummaryInformation ) )
    {
        hr = S_OK ;
        switch( pid )
        {
            case PIDASI_AVG_DATA_RATE:
                if( 0 >= pwfx->nAvgBytesPerSec )
                    return E_FAIL ;
                VERIFY( LoadString( _Module.GetResourceInstance(), IDS_PIDASI_AVG_DATA_RATE_FMT,
                        szFmt, ARRAYSIZE(szFmt) ) ) ;
                wnsprintf( szBuf, ARRAYSIZE(szBuf), szFmt, 
                           pwfx->nAvgBytesPerSec/1000, 
                           pwfx->nAvgBytesPerSec%1000 ) ;
                pVar->bstrVal = SysAllocString( T2W( szBuf ) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            case PIDASI_SAMPLE_RATE:
                if( 0 >= pwfx->nSamplesPerSec )
                    return E_FAIL ;
                VERIFY( LoadString( _Module.GetResourceInstance(), IDS_PIDASI_SAMPLE_RATE_FMT,
                        szFmt, ARRAYSIZE(szFmt) ) ) ;
                wnsprintf( szBuf, ARRAYSIZE(szBuf), szFmt, 
                           pwfx->nSamplesPerSec/1000, 
                           pwfx->nSamplesPerSec%1000 ) ;
                pVar->bstrVal = SysAllocString( T2W( szBuf ) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            case PIDASI_SAMPLE_SIZE:
                if( 0 >= pwfx->wBitsPerSample )
                    return E_FAIL ;
                VERIFY( LoadString( _Module.GetResourceInstance(), IDS_PIDASI_SAMPLE_SIZE_FMT,
                        szFmt, ARRAYSIZE(szFmt) ) ) ;
                wnsprintf( szBuf, ARRAYSIZE(szBuf), szFmt, 
                           (int)pwfx->wBitsPerSample ) ;
                pVar->bstrVal = SysAllocString( T2W( szBuf ) ) ;
                pVar->vt      = VT_BSTR ;
                break ;
                
            case PIDASI_CHANNEL_COUNT:
            {
                if( 0 >= pwfx->nChannels )
                    return E_FAIL ;

                UINT nIDS = pwfx->nChannels == 1 ? IDS_MONO : 
                            pwfx->nChannels == 2 ? IDS_STEREO : 0 ;

                if( nIDS )
                {
                    TCHAR szDescr[MAX_DESCRIPTOR] ;
                    if( LoadString( _Module.GetResourceInstance(), nIDS,
                                    szDescr, ARRAYSIZE(szDescr) ) )
                    {
                        wnsprintf( szBuf, ARRAYSIZE(szBuf), TEXT("%d %s"), 
                                   (int)pwfx->nChannels,
                                   szDescr ) ;
                    }
                    else
                        nIDS = 0 ;

                }

                if( 0 == nIDS )
                {
                    wnsprintf( szBuf, ARRAYSIZE(szBuf), TEXT("%d"), 
                               (int)pwfx->nChannels ) ;
                }

                pVar->bstrVal = SysAllocString( T2W( szBuf ) ) ;
                pVar->vt      = VT_BSTR ;
                break ;
            }
            
            default:
                return E_UNEXPECTED ;
        }
    }
    return hr ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP  GetWaveProperty( 
    IN REFFMTID reffmtid, 
    IN PROPID pid, 
    IN const WAVEDESC* pWave, 
    OUT PROPVARIANT* pVar )
{
    USES_CONVERSION ;
    
    HRESULT hr = E_FAIL ;
    TCHAR   szBuf[MAX_DESCRIPTOR],
            szFmt[MAX_DESCRIPTOR] ;

    PropVariantInit( pVar ) ;
    *szBuf = *szFmt = 0 ;

    if( IsEqualGUID( reffmtid, FMTID_AudioSummaryInformation ) )
    {
        hr = S_OK ;
        switch( pid )
        {
            case PIDASI_FORMAT:
                if( 0 == *pWave->szWaveFormat )
                    return E_FAIL ;
                pVar->bstrVal = SysAllocString( T2W((LPTSTR)pWave->szWaveFormat) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            case PIDASI_TIMELENGTH:
                if( 0 >= pWave->nLength )
                    return E_FAIL ;
                VERIFY( LoadString( _Module.GetResourceInstance(), IDS_PIDASI_TIMELENGTH_FMT,
                        szFmt, ARRAYSIZE(szFmt) ) ) ;
                wnsprintf( szBuf, ARRAYSIZE(szBuf), szFmt, 
                           pWave->nLength/1000, pWave->nLength%1000 ) ;
                pVar->bstrVal = SysAllocString( T2W( szBuf ) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            default:
                hr = E_UNEXPECTED ;
                
        }
    }

    if( FAILED( hr ) )
        hr = _getWaveAudioProperty( reffmtid, pid, pWave->pwfx, pVar ) ;
        
    return hr ;
}



//-------------------------------------------------------------------------//
//  AVI 
//-------------------------------------------------------------------------//

STDMETHODIMP ReadAviStreams( LPCTSTR pszFile, DWORD dwFileSize, PAVIDESC pAvi )
{
    HRESULT         hr;
    PAVIFILE        pfile;
    PAVISTREAM      pavi;
    PAVISTREAM      rgpavis[MAXNUMSTREAMS];    // the current streams
    AVISTREAMINFO   avsi;
    LONG            timeStart;            // cached start, end, length
    LONG            timeEnd;
    int             cpavi;
    int             i;

    if( FAILED( (hr = AVIFileOpen(&pfile, pszFile, 0, 0L)) ) )
        return hr ;

    for (i = 0; i <= MAXNUMSTREAMS; i++) 
    {
        if (AVIFileGetStream(pfile, &pavi, 0L, i) != AVIERR_OK)
            break;
        if (i == MAXNUMSTREAMS) 
        {
            AVIStreamRelease(pavi);
            //DPF("Exceeded maximum number of streams");
            break;
        }
        rgpavis[i] = pavi;
    }

    //
    // Couldn't get any streams out of this file
    //
    if (i == 0)
    {
        //DPF("Unable to open any streams in %s", pszFile);
        if (pfile)
            AVIFileRelease(pfile);
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) ;
    }

    cpavi = i;

    //
    // Start with bogus times
    //
    timeStart = 0x7FFFFFFF;
    timeEnd   = 0;

    //
    // Walk through and init all streams loaded
    //
    for (i = 0; i < cpavi; i++) 
    {

        AVIStreamInfo(rgpavis[i], &avsi, sizeof(avsi));

        switch(avsi.fccType) 
        {
            case streamtypeVIDEO:
            {
                LONG cbFormat;
                LPBYTE lpFormat;
                ICINFO icInfo;
                HIC hic;
                DWORD dwTimeLen;
                USES_CONVERSION ;

                AVIStreamFormatSize(rgpavis[i], 0, &cbFormat);
                dwTimeLen           = AVIStreamEndTime(rgpavis[i]) - AVIStreamStartTime(rgpavis[i]);
                pAvi->cFrames       = avsi.dwLength ;
                pAvi->nFrameRate    = MulDiv(avsi.dwLength, 1000000, dwTimeLen);
                pAvi->nDataRate     = MulDiv(dwFileSize, 1000000, dwTimeLen)/1024 ;
                pAvi->nWidth        = RECTWIDTH( &avsi.rcFrame ) ;
                pAvi->nHeight       = RECTHEIGHT( &avsi.rcFrame ) ;

                lstrcpyn( pAvi->szStreamName, avsi.szName, ARRAYSIZE(pAvi->szStreamName) ) ;

                //  Retrieve raster info (compression, bit depth).
                lpFormat = new BYTE[cbFormat] ;
                if( (lpFormat = new BYTE[cbFormat]) != NULL )
                {
                    AVIStreamReadFormat(rgpavis[i], 0, lpFormat, &cbFormat);
                    hic = (HIC)ICLocate( FOURCC_VIDC, avsi.fccHandler, (BITMAPINFOHEADER*)lpFormat, 
                                         NULL, (WORD)ICMODE_DECOMPRESS );
                    
                    if (hic || ((LPBITMAPINFOHEADER)lpFormat)->biCompression == 0)
                    {
                        if (((LPBITMAPINFOHEADER)lpFormat)->biCompression)
                        {
                            ICGetInfo(hic, &icInfo, sizeof(ICINFO));
                            ICClose(hic);
                            lstrcpy(pAvi->szCompression, W2T(icInfo.szName));
                        }
                        else
                        {
                            LoadString( _Module.GetResourceInstance(), IDS_UNCOMPRESSED, 
                                         pAvi->szCompression, ARRAYSIZE(pAvi->szCompression) ) ;
                        }

                        pAvi->nBitDepth = ((LPBITMAPINFOHEADER)lpFormat)->biBitCount ;
                    }
                    delete [] lpFormat ;
                }
                else
                    hr = E_OUTOFMEMORY ;
                break ;
            }
            case streamtypeAUDIO:
            {
                LONG        cbFormat;

                AVIStreamFormatSize(rgpavis[i], 0, &cbFormat);
                if( (pAvi->pwfx = (PWAVEFORMATEX) new BYTE[cbFormat]) != NULL )
                {
                    ZeroMemory( pAvi->pwfx, cbFormat ) ;
                    if( SUCCEEDED( AVIStreamReadFormat(rgpavis[i], 0, pAvi->pwfx, &cbFormat) ) )
                        GetWaveFormatTag( pAvi->pwfx, pAvi->szWaveFormat, ARRAYSIZE(pAvi->szWaveFormat) ) ;
                }
                break;
            }
            default:
                break;
        }

    //
    // We're finding the earliest and latest start and end points for
    // our scrollbar.
    //  
        timeStart = min(timeStart, AVIStreamStartTime(rgpavis[i]));
        timeEnd   = max(timeEnd, AVIStreamEndTime(rgpavis[i]));
    }

    pAvi->nLength = (UINT)(timeEnd - timeStart);

    for (i = 0; i < cpavi; i++) 
    {
        AVIStreamRelease(rgpavis[i]);
    }
    AVIFileRelease(pfile);

    return S_OK ;
}



//-------------------------------------------------------------------------//
STDMETHODIMP GetAviInfo( LPCTSTR pszFile, PAVIDESC pavi )
{
    HRESULT hr = E_UNEXPECTED ;

    //  Retrieve the file size
    HANDLE hFile = CreateFile( pszFile, 
                               GENERIC_READ, 
                               FILE_SHARE_READ,NULL, 
                               OPEN_EXISTING, 
                               FILE_ATTRIBUTE_NORMAL, 
                               NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        DWORD dwRet = GetLastError() ;
        return ERROR_SUCCESS != dwRet ? HRESULT_FROM_WIN32( dwRet ) : E_UNEXPECTED ;
    }

    DWORD dwFileSize = GetFileSize((HANDLE)hFile, NULL);
    CloseHandle( hFile ) ;

    AVIFileInit();
    hr = ReadAviStreams(pszFile, dwFileSize, pavi);
    AVIFileExit();

    return hr ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP FreeAviInfo( IN OUT PAVIDESC p )
{
    if( !( p && sizeof(*p) == p->dwSize ) && p->pwfx )
    {
        delete [] p->pwfx ;
        p->pwfx = NULL ;
    }
    return S_OK ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP  GetAviProperty( 
    IN REFFMTID reffmtid, 
    IN PROPID pid, 
    IN const AVIDESC* pAvi, 
    OUT PROPVARIANT* pVar )
{
    USES_CONVERSION ;
    
    HRESULT hr = E_UNEXPECTED ;
    TCHAR   szBuf[MAX_DESCRIPTOR],
            szFmt[MAX_DESCRIPTOR] ;

    PropVariantInit( pVar ) ;
    *szBuf = *szFmt = 0 ;

    if( IsEqualGUID( reffmtid, FMTID_VideoSummaryInformation ) )
    {
        hr = S_OK ;
        switch( pid )
        {
            case PIDVSI_STREAM_NAME:
                if( 0 == *pAvi->szStreamName )
                    return E_FAIL ;
                pVar->bstrVal = SysAllocString( T2W((LPTSTR)pAvi->szStreamName) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            case PIDVSI_FRAME_COUNT:
                if( 0 >= pAvi->cFrames )
                    return E_FAIL ;
                wnsprintf( szBuf, ARRAYSIZE(szBuf), TEXT("%d"), pAvi->cFrames ) ;
                pVar->bstrVal = SysAllocString( T2W( szBuf ) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            case PIDVSI_TIMELENGTH:
                if( 0 >= pAvi->nLength )
                    return E_FAIL ;
                VERIFY( LoadString( _Module.GetResourceInstance(), IDS_PIDVSI_TIMELENGTH_FMT,
                        szFmt, ARRAYSIZE(szFmt) ) ) ;
                wnsprintf( szBuf, ARRAYSIZE(szBuf), szFmt, 
                           pAvi->nLength/1000, pAvi->nLength%1000 ) ;
                pVar->bstrVal = SysAllocString( T2W( szBuf ) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            case PIDVSI_FRAME_RATE:
                if( 0 >= pAvi->nFrameRate )
                    return E_FAIL ;
                VERIFY( LoadString( _Module.GetResourceInstance(), IDS_PIDVSI_FRAME_RATE_FMT,
                        szFmt, ARRAYSIZE(szFmt) ) ) ;
                wnsprintf( szBuf, ARRAYSIZE(szBuf), szFmt, 
                           pAvi->nFrameRate/1000, pAvi->nFrameRate%1000 ) ;
                pVar->bstrVal = SysAllocString( T2W( szBuf ) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            case PIDVSI_DATA_RATE:
                if( 0 >= pAvi->nDataRate )
                    return E_FAIL ;
                VERIFY( LoadString( _Module.GetResourceInstance(), IDS_PIDVSI_DATA_RATE_FMT,
                        szFmt, ARRAYSIZE(szFmt) ) ) ;
                wnsprintf( szBuf, ARRAYSIZE(szBuf), szFmt, 
                           pAvi->nDataRate/1000, pAvi->nDataRate%1000 ) ;
                pVar->bstrVal = SysAllocString( T2W( szBuf ) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            case PIDVSI_FRAME_WIDTH:
                if( 0 >= pAvi->nWidth )
                    return E_FAIL ;
                VERIFY( LoadString( _Module.GetResourceInstance(), IDS_PIDVSI_FRAME_WIDTH_FMT,
                        szFmt, ARRAYSIZE(szFmt) ) ) ;
                wnsprintf( szBuf, ARRAYSIZE(szBuf), szFmt, pAvi->nWidth ) ;
                pVar->bstrVal = SysAllocString( T2W( szBuf ) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            case PIDVSI_FRAME_HEIGHT:
                if( 0 >= pAvi->nHeight )
                    return E_FAIL ;
                VERIFY( LoadString( _Module.GetResourceInstance(), IDS_PIDVSI_FRAME_HEIGHT_FMT,
                        szFmt, ARRAYSIZE(szFmt) ) ) ;
                wnsprintf( szBuf, ARRAYSIZE(szBuf), szFmt, pAvi->nHeight ) ;
                pVar->bstrVal = SysAllocString( T2W( szBuf ) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            case PIDVSI_SAMPLE_SIZE:
                if( 0 >= pAvi->nBitDepth )
                    return E_FAIL ;
                VERIFY( LoadString( _Module.GetResourceInstance(), IDS_PIDVSI_SAMPLE_SIZE_FMT,
                        szFmt, ARRAYSIZE(szFmt) ) ) ;
                wnsprintf( szBuf, ARRAYSIZE(szBuf), szFmt, pAvi->nBitDepth ) ;
                pVar->bstrVal = SysAllocString( T2W( szBuf ) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            case PIDVSI_COMPRESSION:
                if( 0 == *pAvi->szCompression )
                    return E_FAIL ;
                pVar->bstrVal = SysAllocString( T2W((LPTSTR)pAvi->szCompression) ) ;
                pVar->vt      = VT_BSTR ;
                break ;

            default:
                hr = E_UNEXPECTED ;
        }
    }
    else if( IsEqualGUID( reffmtid, FMTID_AudioSummaryInformation ) )
    {
        if( PIDASI_FORMAT == pid )
        {
            if( 0 == *pAvi->szWaveFormat )
                return E_FAIL ;
            pVar->bstrVal = SysAllocString( T2W((LPTSTR)pAvi->szWaveFormat) ) ;
            pVar->vt      = VT_BSTR ;
            hr = S_OK ;
        }
    }

    if( FAILED( hr ) )
        hr = _getWaveAudioProperty( reffmtid, pid, pAvi->pwfx, pVar ) ;

    return hr ;
}
