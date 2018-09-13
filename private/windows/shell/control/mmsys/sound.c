/*    -    -    -    -    -    -    -    -    */
//
//    sound.c
//
//    Copyright (C) 1994 Microsoft Corporation.  All Rights Reserved.
//
/*    -    -    -    -    -    -    -    -    */

#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "sound.h"
#include <tchar.h>

/*    -    -    -    -    -    -    -    -    */
typedef struct tagSOUND FAR *PSOUND;
typedef struct tagSOUND 
{
    WAVEHDR        header;
    LPBYTE        pbData;
    DWORD        cbLength;
    LPWAVEFORMATEX    lpwfx;
    HGLOBAL        hgData;
    HWAVEOUT    hwave;
    HWND        hwndNotify;
} SOUND;

typedef struct 
{
    FOURCC    fccRiff;
    DWORD    cbSize;
    FOURCC    fccWave;
} FILEHEADER,  *LPFILEHEADER;


typedef struct 
{
    DWORD    dwCKID;
    DWORD    dwSize;
} CHUNKHEADER,  *LPCHUNKHEADER;

#define    RIFF_FILE    MAKEFOURCC('R','I','F','F')
#define    RIFF_WAVE    MAKEFOURCC('W','A','V','E')
#define    RIFF_FORMAT    MAKEFOURCC('f','m','t',' ')
#define    RIFF_DATA    MAKEFOURCC('d','a','t','a')

/*    -    -    -    -    -    -    -    -    */
#ifdef DEBUG
#define    STATIC
#else
#define    STATIC    static
#endif

typedef CHUNKHEADER UNALIGNED FAR *ULPCHUNKHEADER;
typedef WAVEFORMATEX UNALIGNED FAR *ULPWAVEFORMATEX;

/*    -    -    -    -    -    -    -    -    */
STATIC MMRESULT NEAR PASCAL soundInitWaveHeader(
    PSOUND    ps)
{
    size_t cbWFX;
    ULPWAVEFORMATEX    pwfx;
    ULPCHUNKHEADER    pckhdr;
    LPFILEHEADER    pfhdr;
    LPBYTE        pbData;
    DWORD        dwFileSize;
    DWORD        dwCurPos;

    if (ps->cbLength < sizeof(FILEHEADER))
        return MMSYSERR_INVALPARAM;
    pfhdr = (LPFILEHEADER)ps->pbData;
    if ((pfhdr->fccRiff != RIFF_FILE) || (pfhdr->fccWave != RIFF_WAVE))
        return MMSYSERR_NOTSUPPORTED;
    dwFileSize = pfhdr->cbSize;
    dwCurPos = sizeof(FILEHEADER);
    pbData = (LPBYTE)ps->pbData + sizeof(FILEHEADER);
    if (ps->cbLength < dwFileSize)
        return MMSYSERR_INVALPARAM;
    for (;;) 
    {
        pckhdr = (ULPCHUNKHEADER)pbData;
        if (pckhdr->dwCKID == RIFF_FORMAT)
            break;
        dwCurPos += pckhdr->dwSize + sizeof(CHUNKHEADER);
        if (dwCurPos >= dwFileSize)
            return MMSYSERR_INVALPARAM;
        pbData += pckhdr->dwSize + sizeof(CHUNKHEADER);
    }
    pwfx = (ULPWAVEFORMATEX)(pbData + sizeof(CHUNKHEADER));
    
    cbWFX = sizeof(WAVEFORMATEX);
    if (pwfx->wFormatTag!=WAVE_FORMAT_PCM)
    {
        cbWFX += pwfx->cbSize;
    }

    if ((ps->lpwfx = (LPWAVEFORMATEX)GlobalAlloc (GMEM_FIXED, cbWFX)) == 0)
        return MMSYSERR_NOMEM;

    memcpy ((char *)ps->lpwfx, pwfx, cbWFX);

    pbData = pbData + ((ULPCHUNKHEADER)pbData)->dwSize + sizeof(CHUNKHEADER);
    for (;;) 
    {
        pckhdr = (ULPCHUNKHEADER)pbData;
        if (pckhdr->dwCKID == RIFF_DATA)
            break;
        dwCurPos += pckhdr->dwSize + sizeof(CHUNKHEADER);
        if (dwCurPos >= dwFileSize) 
        {
            GlobalFree ((HGLOBAL)ps->lpwfx);
            ps->lpwfx = NULL;
            return MMSYSERR_INVALPARAM;
        }
        pbData += pckhdr->dwSize + sizeof(CHUNKHEADER);
    }
    ps->header.lpData = pbData + sizeof(CHUNKHEADER);
    ps->header.dwBufferLength = pckhdr->dwSize;
    ps->header.dwBytesRecorded = 0;
    ps->header.dwUser = (DWORD_PTR)ps;
    ps->header.dwFlags = 0;
    ps->header.dwLoops = 0;
    ps->header.lpNext = NULL;
    ps->header.reserved = 0;
    return MMSYSERR_NOERROR;
}

/*    -    -    -    -    -    -    -    -    */
STATIC MMRESULT NEAR PASCAL soundLoadFile(
    LPCTSTR    pszSound,
    HWND    hwndNotify,
    PSOUND    ps)
{
    HFILE        hf;
    MMRESULT    mmr;

    if ((hf = HandleToUlong(CreateFile(pszSound,GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))) != HFILE_ERROR) 
    {
        ps->cbLength = _llseek(hf, 0L, SEEK_END);
        _llseek(hf, 0L, SEEK_SET);
        if (ps->hgData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, ps->cbLength)) 
        {
            if (ps->pbData = GlobalLock(ps->hgData)) 
            {
                if (_hread(hf, ps->pbData, ps->cbLength) == (LONG)ps->cbLength) 
                {
                    if (!(mmr = soundInitWaveHeader(ps))) 
                    {
                        _lclose(hf);
                        ps->hwndNotify = hwndNotify;
                        return MMSYSERR_NOERROR;
                    }
                } 
                else
                    mmr = MMSYSERR_ERROR;
                GlobalUnlock(ps->hgData);
            } 
            else
                mmr = MMSYSERR_ERROR;
            GlobalFree(ps->hgData);
        } 
        else
            mmr = MMSYSERR_NOMEM;
        _lclose(hf);
    } 
    else
        mmr = MMSYSERR_ERROR;

    return mmr;
}

/*    -    -    -    -    -    -    -    -    */
void FAR PASCAL soundOnDone(
    HSOUND    hs)
{
    PSOUND        ps;

    ps = (PSOUND)hs;
    waveOutUnprepareHeader(ps->hwave, &ps->header, sizeof(WAVEHDR));
    waveOutClose(ps->hwave);
    ps->hwave = NULL;
}

/*    -    -    -    -    -    -    -    -    */
MMRESULT FAR PASCAL soundOpen(
    LPCTSTR    pszSound,
    HWND    hwndNotify,
    PHSOUND    phs)
{
    MMRESULT    mmr;

    if (!(*phs = (HSOUND)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(SOUND))))
        return MMSYSERR_NOMEM;
    if (mmr = soundLoadFile(pszSound, hwndNotify, (PSOUND)*phs))
        LocalFree((HLOCAL)*phs);
    return mmr;
}

/*    -    -    -    -    -    -    -    -    */
MMRESULT FAR PASCAL soundClose(
    HSOUND    hs)
{
    PSOUND        ps;
    MMRESULT    mmr;

    if (mmr = soundStop(hs))
        return mmr;
    ps = (PSOUND)hs;
    if (ps->lpwfx != NULL) {
        GlobalFree ((HGLOBAL)ps->lpwfx);
        ps->lpwfx = NULL;
    }
    GlobalUnlock(ps->hgData);
    GlobalFree(ps->hgData);
    LocalFree((HLOCAL)hs);
    return MMSYSERR_NOERROR;
}

/*    -    -    -    -    -    -    -    -    */
MMRESULT FAR PASCAL soundPlay(
    HSOUND    hs)
{
    PSOUND        ps;
    MMRESULT    mmr;

    if (mmr = soundStop(hs))
        return mmr;
    ps = (PSOUND)hs;
    if (!(mmr = waveOutOpen(&ps->hwave, WAVE_MAPPER, ps->lpwfx, (DWORD_PTR)ps->hwndNotify, (DWORD_PTR)ps, CALLBACK_WINDOW | WAVE_ALLOWSYNC))) 
    {
        ps->header.dwFlags &= ~WHDR_DONE;
        if (!(mmr = waveOutPrepareHeader(ps->hwave, &ps->header, sizeof(WAVEHDR)))) 
        {
            if (!(mmr = waveOutWrite(ps->hwave, &ps->header, sizeof(WAVEHDR))))
                return MMSYSERR_NOERROR;
            waveOutUnprepareHeader(ps->hwave, &ps->header, sizeof(WAVEHDR));
        }
        waveOutClose(ps->hwave);
        ps->hwave = NULL;
    }
    return mmr;
}

/*    -    -    -    -    -    -    -    -    */
MMRESULT FAR PASCAL soundStop(
    HSOUND    hs)
{
    PSOUND        ps;
    MSG        msg;

    ps = (PSOUND)hs;
    if (ps->hwave)
        waveOutReset(ps->hwave);
    if (IsWindow(ps->hwndNotify))
    while (PeekMessage(&msg, ps->hwndNotify, MM_WOM_OPEN, MM_WOM_DONE, PM_REMOVE))
    {
        DispatchMessage(&msg);
    }
    return MMSYSERR_NOERROR;
}

/*    -    -    -    -    -    -    -    -    */
