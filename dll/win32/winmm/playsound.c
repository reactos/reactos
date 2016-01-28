/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYSTEM functions
 *
 * Copyright 1993      Martin Ayotte
 *           1998-2002 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "winemm.h"

#include <winternl.h>

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

typedef struct tagWINE_PLAYSOUND
{
    unsigned                    bLoop : 1,
                                bAlloc : 1;
    LPCWSTR                     pszSound;
    HMODULE                     hMod;
    DWORD                       fdwSound;
    HANDLE                      hThread;
    HWAVEOUT                    hWave;
    struct tagWINE_PLAYSOUND*   lpNext;
} WINE_PLAYSOUND;

static WINE_PLAYSOUND *PlaySoundList;

static HMMIO	get_mmioFromFile(LPCWSTR lpszName)
{
    HMMIO       ret;
    WCHAR       buf[256];
    LPWSTR      dummy;

    ret = mmioOpenW((LPWSTR)lpszName, NULL,
                    MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
    if (ret != 0) return ret;
    if (SearchPathW(NULL, lpszName, NULL, sizeof(buf)/sizeof(buf[0]), buf, &dummy))
    {
        return mmioOpenW(buf, NULL,
                         MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
    }
    return 0;
}

static HMMIO	get_mmioFromProfile(UINT uFlags, LPCWSTR lpszName)
{
    WCHAR	str[128];
    LPWSTR	ptr;
    HMMIO  	hmmio;
    HKEY        hRegSnd, hRegApp, hScheme, hSnd;
    DWORD       err, type, count;

    static const WCHAR  wszSounds[] = {'S','o','u','n','d','s',0};
    static const WCHAR  wszDefault[] = {'D','e','f','a','u','l','t',0};
    static const WCHAR  wszKey[] = {'A','p','p','E','v','e','n','t','s','\\',
                                    'S','c','h','e','m','e','s','\\',
                                    'A','p','p','s',0};
    static const WCHAR  wszDotDefault[] = {'.','D','e','f','a','u','l','t',0};
    static const WCHAR  wszDotCurrent[] = {'.','C','u','r','r','e','n','t',0};
    static const WCHAR  wszNull[] = {0};

    TRACE("searching in SystemSound list for %s\n", debugstr_w(lpszName));
    GetProfileStringW(wszSounds, lpszName, wszNull, str, sizeof(str)/sizeof(str[0]));
    if (lstrlenW(str) == 0)
    {
	if (uFlags & SND_NODEFAULT) goto next;
	GetProfileStringW(wszSounds, wszDefault, wszNull, str, sizeof(str)/sizeof(str[0]));
	if (lstrlenW(str) == 0) goto next;
    }
    for (ptr = str; *ptr && *ptr != ','; ptr++);
    if (*ptr) *ptr = 0;
    hmmio = mmioOpenW(str, NULL, MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
    if (hmmio != 0) return hmmio;
 next:
    /* we look up the registry under
     *      HKCU\AppEvents\Schemes\Apps\.Default
     *      HKCU\AppEvents\Schemes\Apps\<AppName>
     */
    if (RegOpenKeyW(HKEY_CURRENT_USER, wszKey, &hRegSnd) != 0) goto none;
    if (uFlags & SND_APPLICATION)
    {
        DWORD len;

        err = 1; /* error */
        len = GetModuleFileNameW(0, str, sizeof(str)/sizeof(str[0]));
        if (len > 0 && len < sizeof(str)/sizeof(str[0]))
        {
            for (ptr = str + lstrlenW(str) - 1; ptr >= str; ptr--)
            {
                if (*ptr == '.') *ptr = 0;
                if (*ptr == '\\')
                {
                    err = RegOpenKeyW(hRegSnd, ptr+1, &hRegApp);
                    break;
                }
            }
        }
    }
    else
    {
        err = RegOpenKeyW(hRegSnd, wszDotDefault, &hRegApp);
    }
    RegCloseKey(hRegSnd);
    if (err != 0) goto none;
    err = RegOpenKeyW(hRegApp, lpszName, &hScheme);
    RegCloseKey(hRegApp);
    if (err != 0) goto none;
    /* what's the difference between .Current and .Default ? */
    err = RegOpenKeyW(hScheme, wszDotDefault, &hSnd);
    if (err != 0)
    {
        err = RegOpenKeyW(hScheme, wszDotCurrent, &hSnd);
        RegCloseKey(hScheme);
        if (err != 0)
            goto none;
    }
    count = sizeof(str)/sizeof(str[0]);
    err = RegQueryValueExW(hSnd, NULL, 0, &type, (LPBYTE)str, &count);
    RegCloseKey(hSnd);
    if (err != 0 || !*str) goto none;
    hmmio = mmioOpenW(str, NULL, MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
    if (hmmio) return hmmio;
 none:
    WARN("can't find SystemSound=%s !\n", debugstr_w(lpszName));
    return 0;
}

struct playsound_data
{
    HANDLE	hEvent;
    LONG	dwEventCount;
};

static void CALLBACK PlaySound_Callback(HWAVEOUT hwo, UINT uMsg,
					DWORD_PTR dwInstance,
					DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    struct playsound_data*	s = (struct playsound_data*)dwInstance;

    switch (uMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
	break;
    case WOM_DONE:
	InterlockedIncrement(&s->dwEventCount);
	TRACE("Returning waveHdr=%lx\n", dwParam1);
	SetEvent(s->hEvent);
	break;
    default:
	ERR("Unknown uMsg=%d\n", uMsg);
    }
}

static void PlaySound_WaitDone(struct playsound_data* s)
{
    for (;;) {
	ResetEvent(s->hEvent);
	if (InterlockedDecrement(&s->dwEventCount) >= 0) break;
	InterlockedIncrement(&s->dwEventCount);

	WaitForSingleObject(s->hEvent, INFINITE);
    }
}

static BOOL PlaySound_IsString(DWORD fdwSound, const void* psz)
{
    /* SND_RESOURCE is 0x40004 while
     * SND_MEMORY is 0x00004
     */
    switch (fdwSound & (SND_RESOURCE|SND_ALIAS_ID|SND_FILENAME))
    {
    case SND_RESOURCE:  return HIWORD(psz) != 0; /* by name or by ID ? */
    case SND_ALIAS_ID:
    case SND_MEMORY:    return FALSE;
    case SND_ALIAS:
    case SND_FILENAME:
    case 0:             return TRUE;
    default:            FIXME("WTF\n"); return FALSE;
    }
}

static void     PlaySound_Free(WINE_PLAYSOUND* wps)
{
    WINE_PLAYSOUND**    p;

    EnterCriticalSection(&WINMM_cs);
    for (p = &PlaySoundList; *p && *p != wps; p = &((*p)->lpNext));
    if (*p) *p = (*p)->lpNext;
    if (PlaySoundList == NULL) SetEvent(psLastEvent);
    LeaveCriticalSection(&WINMM_cs);
    if (wps->bAlloc) HeapFree(GetProcessHeap(), 0, (void*)wps->pszSound);
    if (wps->hThread) CloseHandle(wps->hThread);
    HeapFree(GetProcessHeap(), 0, wps);
}

static WINE_PLAYSOUND*  PlaySound_Alloc(const void* pszSound, HMODULE hmod,
                                        DWORD fdwSound, BOOL bUnicode)
{
    WINE_PLAYSOUND* wps;

    wps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wps));
    if (!wps) return NULL;

    wps->hMod = hmod;
    wps->fdwSound = fdwSound;
    if (PlaySound_IsString(fdwSound, pszSound))
    {
        if (bUnicode)
        {
            if (fdwSound & SND_ASYNC)
            {
                LPWSTR sound = HeapAlloc(GetProcessHeap(), 0,
                                         (lstrlenW(pszSound)+1) * sizeof(WCHAR));
                if (!sound) goto oom_error;
                wps->pszSound = lstrcpyW(sound, pszSound);
                wps->bAlloc = TRUE;
            }
            else
                wps->pszSound = pszSound;
        }
        else
        {
            UNICODE_STRING usBuffer;
            RtlCreateUnicodeStringFromAsciiz(&usBuffer, pszSound);
            wps->pszSound = usBuffer.Buffer;
            if (!wps->pszSound) goto oom_error;
            wps->bAlloc = TRUE;
        }
    }
    else
        wps->pszSound = pszSound;

    return wps;
 oom_error:
    PlaySound_Free(wps);
    return NULL;
}

static DWORD WINAPI proc_PlaySound(LPVOID arg)
{
    WINE_PLAYSOUND*     wps = arg;
    BOOL		bRet = FALSE;
    HMMIO		hmmio = 0;
    MMCKINFO		ckMainRIFF;
    MMCKINFO        	mmckInfo;
    LPWAVEFORMATEX      lpWaveFormat = NULL;
    LPWAVEHDR		waveHdr = NULL;
    INT			count, bufsize, left, index;
    struct playsound_data	s;
    void*               data;

    s.hEvent = 0;

    TRACE("SoundName=%s !\n", debugstr_w(wps->pszSound));

    /* if resource, grab it */
    if ((wps->fdwSound & SND_RESOURCE) == SND_RESOURCE) {
        static const WCHAR wszWave[] = {'W','A','V','E',0};
        HRSRC	hRes;
        HGLOBAL	hGlob;

        if ((hRes = FindResourceW(wps->hMod, wps->pszSound, wszWave)) == 0 ||
            (hGlob = LoadResource(wps->hMod, hRes)) == 0)
            goto errCleanUp;
        if ((data = LockResource(hGlob)) == NULL) {
            FreeResource(hGlob);
            goto errCleanUp;
        }
        FreeResource(hGlob);
    } else
        data = (void*)wps->pszSound;

    /* construct an MMIO stream (either in memory, or from a file */
    if (wps->fdwSound & SND_MEMORY)
    { /* NOTE: SND_RESOURCE has the SND_MEMORY bit set */
	MMIOINFO	mminfo;

	memset(&mminfo, 0, sizeof(mminfo));
	mminfo.fccIOProc = FOURCC_MEM;
	mminfo.pchBuffer = data;
	mminfo.cchBuffer = -1; /* FIXME: when a resource, could grab real size */
	TRACE("Memory sound %p\n", data);
	hmmio = mmioOpenW(NULL, &mminfo, MMIO_READ);
    }
    else if (wps->fdwSound & SND_ALIAS)
    {
        if ((wps->fdwSound & SND_ALIAS_ID) == SND_ALIAS_ID)
        {
            static const WCHAR  wszSystemAsterisk[] = {'S','y','s','t','e','m','A','s','t','e','r','i','s','k',0};
            static const WCHAR  wszSystemDefault[] = {'S','y','s','t','e','m','D','e','f','a','u','l','t',0};
            static const WCHAR  wszSystemExclamation[] = {'S','y','s','t','e','m','E','x','c','l','a','m','a','t','i','o','n',0};
            static const WCHAR  wszSystemExit[] = {'S','y','s','t','e','m','E','x','i','t',0};
            static const WCHAR  wszSystemHand[] = {'S','y','s','t','e','m','H','a','n','d',0};
            static const WCHAR  wszSystemQuestion[] = {'S','y','s','t','e','m','Q','u','e','s','t','i','o','n',0};
            static const WCHAR  wszSystemStart[] = {'S','y','s','t','e','m','S','t','a','r','t',0};
            static const WCHAR  wszSystemWelcome[] = {'S','y','s','t','e','m','W','e','l','c','o','m','e',0};

            wps->fdwSound &= ~(SND_ALIAS_ID ^ SND_ALIAS);
            if (wps->pszSound == (LPCWSTR)SND_ALIAS_SYSTEMASTERISK)
                wps->pszSound = wszSystemAsterisk;
            else if (wps->pszSound == (LPCWSTR)SND_ALIAS_SYSTEMDEFAULT)
                wps->pszSound = wszSystemDefault;
            else if (wps->pszSound == (LPCWSTR)SND_ALIAS_SYSTEMEXCLAMATION)
                wps->pszSound = wszSystemExclamation;
            else if (wps->pszSound == (LPCWSTR)SND_ALIAS_SYSTEMEXIT)
                wps->pszSound = wszSystemExit;
            else if (wps->pszSound == (LPCWSTR)SND_ALIAS_SYSTEMHAND)
                wps->pszSound = wszSystemHand;
            else if (wps->pszSound == (LPCWSTR)SND_ALIAS_SYSTEMQUESTION)
                wps->pszSound = wszSystemQuestion;
            else if (wps->pszSound == (LPCWSTR)SND_ALIAS_SYSTEMSTART)
                wps->pszSound = wszSystemStart;
            else if (wps->pszSound == (LPCWSTR)SND_ALIAS_SYSTEMWELCOME)
                wps->pszSound = wszSystemWelcome;
            else goto errCleanUp;
        }
        hmmio = get_mmioFromProfile(wps->fdwSound, wps->pszSound);
    }
    else if (wps->fdwSound & SND_FILENAME)
    {
        hmmio = get_mmioFromFile(wps->pszSound);
    }
    else
    {
        if ((hmmio = get_mmioFromProfile(wps->fdwSound | SND_NODEFAULT, wps->pszSound)) == 0)
        {
            if ((hmmio = get_mmioFromFile(wps->pszSound)) == 0)
            {
                hmmio = get_mmioFromProfile(wps->fdwSound, wps->pszSound);
            }
        }
    }
    if (hmmio == 0) goto errCleanUp;

    if (mmioDescend(hmmio, &ckMainRIFF, NULL, 0))
	goto errCleanUp;

    TRACE("ParentChunk ckid=%.4s fccType=%.4s cksize=%08X\n",
	  (LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType, ckMainRIFF.cksize);

    if ((ckMainRIFF.ckid != FOURCC_RIFF) ||
	(ckMainRIFF.fccType != mmioFOURCC('W', 'A', 'V', 'E')))
	goto errCleanUp;

    mmckInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK))
	goto errCleanUp;

    TRACE("Chunk Found ckid=%.4s fccType=%08x cksize=%08X\n",
	  (LPSTR)&mmckInfo.ckid, mmckInfo.fccType, mmckInfo.cksize);

    lpWaveFormat = HeapAlloc(GetProcessHeap(), 0, mmckInfo.cksize);
    if (mmioRead(hmmio, (HPSTR)lpWaveFormat, mmckInfo.cksize) < sizeof(PCMWAVEFORMAT))
	goto errCleanUp;

    TRACE("wFormatTag=%04X !\n", 	lpWaveFormat->wFormatTag);
    TRACE("nChannels=%d\n", 		lpWaveFormat->nChannels);
    TRACE("nSamplesPerSec=%d\n", 	lpWaveFormat->nSamplesPerSec);
    TRACE("nAvgBytesPerSec=%d\n", 	lpWaveFormat->nAvgBytesPerSec);
    TRACE("nBlockAlign=%d\n", 		lpWaveFormat->nBlockAlign);
    TRACE("wBitsPerSample=%u !\n", 	lpWaveFormat->wBitsPerSample);

    /* move to end of 'fmt ' chunk */
    mmioAscend(hmmio, &mmckInfo, 0);

    mmckInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK))
	goto errCleanUp;

    TRACE("Chunk Found ckid=%.4s fccType=%08x cksize=%08X\n",
	  (LPSTR)&mmckInfo.ckid, mmckInfo.fccType, mmckInfo.cksize);

    s.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    if (waveOutOpen(&wps->hWave, WAVE_MAPPER, lpWaveFormat, (DWORD_PTR)PlaySound_Callback,
		    (DWORD_PTR)&s, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
	goto errCleanUp;

    /* make it so that 3 buffers per second are needed */
    bufsize = (((lpWaveFormat->nAvgBytesPerSec / 3) - 1) / lpWaveFormat->nBlockAlign + 1) *
	lpWaveFormat->nBlockAlign;
    waveHdr = HeapAlloc(GetProcessHeap(), 0, 2 * sizeof(WAVEHDR) + 2 * bufsize);
    waveHdr[0].lpData = (char*)waveHdr + 2 * sizeof(WAVEHDR);
    waveHdr[1].lpData = (char*)waveHdr + 2 * sizeof(WAVEHDR) + bufsize;
    waveHdr[0].dwUser = waveHdr[1].dwUser = 0L;
    waveHdr[0].dwLoops = waveHdr[1].dwLoops = 0L;
    waveHdr[0].dwFlags = waveHdr[1].dwFlags = 0L;
    waveHdr[0].dwBufferLength = waveHdr[1].dwBufferLength = bufsize;
    if (waveOutPrepareHeader(wps->hWave, &waveHdr[0], sizeof(WAVEHDR)) ||
	waveOutPrepareHeader(wps->hWave, &waveHdr[1], sizeof(WAVEHDR))) {
	goto errCleanUp;
    }

    s.dwEventCount = 1L; /* for first buffer */
    index = 0;

    do {
	left = mmckInfo.cksize;

	mmioSeek(hmmio, mmckInfo.dwDataOffset, SEEK_SET);
	while (left)
        {
	    if (WaitForSingleObject(psStopEvent, 0) == WAIT_OBJECT_0)
            {
		wps->bLoop = FALSE;
		break;
	    }
	    count = mmioRead(hmmio, waveHdr[index].lpData, min(bufsize, left));
	    if (count < 1) break;
	    left -= count;
	    waveHdr[index].dwBufferLength = count;
	    waveHdr[index].dwFlags &= ~WHDR_DONE;
	    if (waveOutWrite(wps->hWave, &waveHdr[index], sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
                index ^= 1;
                PlaySound_WaitDone(&s);
            }
            else FIXME("Couldn't play header\n");
	}
	bRet = TRUE;
    } while (wps->bLoop);

    PlaySound_WaitDone(&s); /* for last buffer */
    waveOutReset(wps->hWave);

    waveOutUnprepareHeader(wps->hWave, &waveHdr[0], sizeof(WAVEHDR));
    waveOutUnprepareHeader(wps->hWave, &waveHdr[1], sizeof(WAVEHDR));

errCleanUp:
    TRACE("Done playing=%s => %s!\n", debugstr_w(wps->pszSound), bRet ? "ok" : "ko");
    CloseHandle(s.hEvent);
    HeapFree(GetProcessHeap(), 0, waveHdr);
    HeapFree(GetProcessHeap(), 0, lpWaveFormat);
    if (wps->hWave)	while (waveOutClose(wps->hWave) == WAVERR_STILLPLAYING) Sleep(100);
    if (hmmio) 		mmioClose(hmmio, 0);

    PlaySound_Free(wps);

    return bRet;
}

static BOOL MULTIMEDIA_PlaySound(const void* pszSound, HMODULE hmod, DWORD fdwSound, BOOL bUnicode)
{
    WINE_PLAYSOUND*     wps = NULL;

    TRACE("pszSound='%p' hmod=%p fdwSound=%08X\n",
	  pszSound, hmod, fdwSound);

    /* FIXME? I see no difference between SND_NOWAIT and SND_NOSTOP !
     * there could be one if several sounds can be played at once...
     */
    if ((fdwSound & (SND_NOWAIT | SND_NOSTOP)) && PlaySoundList != NULL)
	return FALSE;

    /* alloc internal structure, if we need to play something */
    if (pszSound && !(fdwSound & SND_PURGE))
    {
        if (!(wps = PlaySound_Alloc(pszSound, hmod, fdwSound, bUnicode)))
            return FALSE;
    }

    EnterCriticalSection(&WINMM_cs);
    /* since several threads can enter PlaySound in parallel, we're not
     * sure, at this point, that another thread didn't start a new playsound
     */
    while (PlaySoundList != NULL)
    {
        ResetEvent(psLastEvent);
        /* FIXME: doc says we have to stop all instances of pszSound if it's non
         * NULL... as of today, we stop all playing instances */
        SetEvent(psStopEvent);

        waveOutReset(PlaySoundList->hWave);
        LeaveCriticalSection(&WINMM_cs);
        WaitForSingleObject(psLastEvent, INFINITE);
        EnterCriticalSection(&WINMM_cs);

        ResetEvent(psStopEvent);
    }

    if (wps) wps->lpNext = PlaySoundList;
    PlaySoundList = wps;
    LeaveCriticalSection(&WINMM_cs);

    if (!pszSound || (fdwSound & SND_PURGE)) return TRUE;

    if (fdwSound & SND_ASYNC)
    {
        DWORD       id;
        HANDLE      handle;
        wps->bLoop = (fdwSound & SND_LOOP) ? TRUE : FALSE;
        if ((handle = CreateThread(NULL, 0, proc_PlaySound, wps, 0, &id)) != 0) {
            wps->hThread = handle;
            SetThreadPriority(handle, THREAD_PRIORITY_TIME_CRITICAL);
            return TRUE;
        }
    }
    else return proc_PlaySound(wps);

    /* error cases */
    PlaySound_Free(wps);
    return FALSE;
}

/**************************************************************************
 * 				PlaySoundA		[WINMM.@]
 */
BOOL WINAPI PlaySoundA(LPCSTR pszSoundA, HMODULE hmod, DWORD fdwSound)
{
    return MULTIMEDIA_PlaySound(pszSoundA, hmod, fdwSound, FALSE);
}

/**************************************************************************
 * 				PlaySoundW		[WINMM.@]
 */
BOOL WINAPI PlaySoundW(LPCWSTR pszSoundW, HMODULE hmod, DWORD fdwSound)
{
    return MULTIMEDIA_PlaySound(pszSoundW, hmod, fdwSound, TRUE);
}

/**************************************************************************
 * 				sndPlaySoundA		[WINMM.@]
 */
BOOL WINAPI sndPlaySoundA(LPCSTR pszSoundA, UINT uFlags)
{
    uFlags &= SND_ALIAS_ID|SND_FILENAME|SND_ASYNC|SND_LOOP|SND_MEMORY|SND_NODEFAULT|SND_NOSTOP|SND_SYNC;
    return MULTIMEDIA_PlaySound(pszSoundA, 0, uFlags, FALSE);
}

/**************************************************************************
 * 				sndPlaySoundW		[WINMM.@]
 */
BOOL WINAPI sndPlaySoundW(LPCWSTR pszSound, UINT uFlags)
{
    uFlags &= SND_ALIAS_ID|SND_FILENAME|SND_ASYNC|SND_LOOP|SND_MEMORY|SND_NODEFAULT|SND_NOSTOP|SND_SYNC;
    return MULTIMEDIA_PlaySound(pszSound, 0, uFlags, TRUE);
}

/**************************************************************************
 * 				mmsystemGetVersion	[WINMM.@]
 */
UINT WINAPI mmsystemGetVersion(void)
{
    TRACE("3.10 (Win95?)\n");
    return 0x030a;
}
