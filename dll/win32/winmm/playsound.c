/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYSTEM functions
 *
 * Copyright 1993      Martin Ayotte
 *           1998-2002 Eric Pouech
 *
 * Modified for use with ReactOS by Thamatip Chitpong, 2023
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
#include <userenv.h>

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

typedef struct tagWINE_PLAYSOUND
{
    unsigned                    bLoop : 1;
    HMMIO                       hmmio;
    HWAVEOUT                    hWave;
} WINE_PLAYSOUND;

static WINE_PLAYSOUND *PlaySoundCurrent;
static BOOL bPlaySoundStop;

/* An impersonation-aware equivalent of ExpandEnvironmentStringsW */
static DWORD PlaySound_ExpandEnvironmentStrings(LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize)
{
    HANDLE hToken;
    DWORD dwError;
    DWORD dwLength = 0;

    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                         TRUE,
                         &hToken))
    {
        dwError = GetLastError();

        if (dwError == ERROR_NO_TOKEN)
        {
            /* We are not impersonating, forward this to ExpandEnvironmentStrings */
            return ExpandEnvironmentStringsW(lpSrc, lpDst, nSize);
        }

        ERR("OpenThreadToken failed (0x%x)\n", dwError);
        return 0;
    }

    if (!ExpandEnvironmentStringsForUserW(hToken, lpSrc, lpDst, nSize))
    {
        dwError = GetLastError();

        if (dwError == ERROR_INSUFFICIENT_BUFFER || nSize == 0)
        {
            /* The buffer is too small, find the required buffer size.
             * NOTE: ExpandEnvironmentStringsForUser doesn't support retrieving buffer size. */
            WCHAR szExpanded[1024];

            if (ExpandEnvironmentStringsForUserW(hToken, lpSrc, szExpanded, ARRAY_SIZE(szExpanded)))
            {
                /* We success, return the required buffer size */
                dwLength = lstrlenW(szExpanded) + 1;
                goto Cleanup;
            }
        }

        ERR("ExpandEnvironmentStringsForUser failed (0x%x)\n", dwError);
    }
    else
    {
        /* We success, return the size of the string */
        dwLength = lstrlenW(lpDst) + 1;
    }

Cleanup:
    CloseHandle(hToken);
    return dwLength;
}

static HMMIO    get_mmioFromFile(LPCWSTR lpszName)
{
    HMMIO       ret;
    WCHAR       buf[256];
    LPWSTR      dummy;

    ret = mmioOpenW((LPWSTR)lpszName, NULL,
                    MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
    if (ret != 0) return ret;
    if (SearchPathW(NULL, lpszName, L".wav", ARRAY_SIZE(buf), buf, &dummy))
    {
        return mmioOpenW(buf, NULL,
                         MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
    }
    return 0;
}

static HMMIO get_mmioFromProfile(UINT uFlags, LPCWSTR lpszName)
{
    WCHAR str[128];
    LPWSTR ptr, pszSnd;
    HMMIO hmmio;
    HKEY hUserKey, hRegSnd, hRegApp, hScheme, hSnd;
    DWORD err, type, count;
    BOOL bIsDefault;

    TRACE("searching in SystemSound list for %s\n", debugstr_w(lpszName));

    bIsDefault = (_wcsicmp(lpszName, L"SystemDefault") == 0);

    GetProfileStringW(L"Sounds",
                      bIsDefault ? L"Default" : lpszName,
                      L"",
                      str,
                      ARRAY_SIZE(str));
    if (!*str)
        goto Next;

    for (ptr = str; *ptr && *ptr != L','; ptr++);

    if (*ptr)
        *ptr = UNICODE_NULL;

    hmmio = mmioOpenW(str, NULL, MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
    if (hmmio)
        return hmmio;

Next:
    /* we look up the registry under
     *      HKCU\AppEvents\Schemes\Apps\.Default
     *      HKCU\AppEvents\Schemes\Apps\<AppName>
     */
    err = RegOpenCurrentUser(KEY_READ, &hUserKey);
    if (err == ERROR_SUCCESS)
    {
        err = RegOpenKeyW(hUserKey, L"AppEvents\\Schemes\\Apps", &hRegSnd);
        RegCloseKey(hUserKey);
    }

    if (err != ERROR_SUCCESS)
        goto None;

    if (uFlags & SND_APPLICATION)
    {
        DWORD len;

        err = ERROR_FILE_NOT_FOUND; /* error */
        len = GetModuleFileNameW(NULL, str, ARRAY_SIZE(str));
        if (len > 0 && len < ARRAY_SIZE(str))
        {
            for (ptr = str + lstrlenW(str) - 1; ptr >= str; ptr--)
            {
                if (*ptr == L'.')
                    *ptr = UNICODE_NULL;

                if (*ptr == L'\\')
                {
                    err = RegOpenKeyW(hRegSnd, ptr + 1, &hRegApp);
                    break;
                }
            }
        }
    }
    else
    {
        err = RegOpenKeyW(hRegSnd, L".Default", &hRegApp);
    }

    RegCloseKey(hRegSnd);

    if (err != ERROR_SUCCESS)
        goto None;

    err = RegOpenKeyW(hRegApp,
                      bIsDefault ? L".Default" : lpszName,
                      &hScheme);

    RegCloseKey(hRegApp);

    if (err != ERROR_SUCCESS)
        goto None;
    
    err = RegOpenKeyW(hScheme, L".Current", &hSnd);

    RegCloseKey(hScheme);

    if (err != ERROR_SUCCESS)
        goto None;

    count = sizeof(str);
    err = RegQueryValueExW(hSnd, NULL, 0, &type, (LPBYTE)str, &count);

    RegCloseKey(hSnd);

    if (err != ERROR_SUCCESS || !*str)
        goto None;

    if (type == REG_EXPAND_SZ)
    {
        count = PlaySound_ExpandEnvironmentStrings(str, NULL, 0);
        if (count == 0)
            goto None;

        pszSnd = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR));
        if (!pszSnd)
            goto None;

        if (PlaySound_ExpandEnvironmentStrings(str, pszSnd, count) == 0)
        {
            HeapFree(GetProcessHeap(), 0, pszSnd);
            goto None;
        }
    }
    else if (type == REG_SZ)
    {
        /* The type is REG_SZ, no need to expand */
        pszSnd = str;
    }
    else
    {
        /* Invalid type */
        goto None;
    }

    hmmio = mmioOpenW(pszSnd, NULL, MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);

    if (type == REG_EXPAND_SZ)
        HeapFree(GetProcessHeap(), 0, pszSnd);

    if (hmmio)
        return hmmio;

None:
    WARN("can't find SystemSound=%s !\n", debugstr_w(lpszName));
    return NULL;
}

static HMMIO PlaySound_GetMMIO(LPCWSTR pszSound, HMODULE hMod, DWORD fdwSound)
{
    BOOL bIsDefault = FALSE;
    HMMIO hmmio = NULL;

    TRACE("SoundName=%s !\n", debugstr_w(pszSound));

    if (fdwSound & SND_MEMORY)
    {
        PVOID data;
        MMIOINFO mminfo;

        /* NOTE: SND_RESOURCE has the SND_MEMORY bit set */
        if ((fdwSound & SND_RESOURCE) == SND_RESOURCE)
        {
            HRSRC hRes;
            HGLOBAL hGlob;

            hRes = FindResourceW(hMod, pszSound, L"WAVE");
            hGlob = LoadResource(hMod, hRes);
            if (!hRes || !hGlob)
                goto Quit;

            data = LockResource(hGlob);
            FreeResource(hGlob);
            if (!data)
                goto Quit;
        }
        else
        {
            data = (PVOID)pszSound;
        }
        
        ZeroMemory(&mminfo, sizeof(mminfo));
        mminfo.fccIOProc = FOURCC_MEM;
        mminfo.pchBuffer = data;
        mminfo.cchBuffer = -1; /* FIXME: when a resource, could grab real size */

        TRACE("Memory sound %p\n", data);

        hmmio = mmioOpenW(NULL, &mminfo, MMIO_READ);
    }
    else if (fdwSound & SND_ALIAS)
    {
        LPCWSTR pszName;

        /* NOTE: SND_ALIAS_ID has the SND_ALIAS bit set */
        if ((fdwSound & SND_ALIAS_ID) == SND_ALIAS_ID)
        {
            if (pszSound == (LPCWSTR)SND_ALIAS_SYSTEMASTERISK)
                pszName = L"SystemAsterisk";
            else if (pszSound == (LPCWSTR)SND_ALIAS_SYSTEMDEFAULT)
                pszName = L"SystemDefault";
            else if (pszSound == (LPCWSTR)SND_ALIAS_SYSTEMEXCLAMATION)
                pszName = L"SystemExclamation";
            else if (pszSound == (LPCWSTR)SND_ALIAS_SYSTEMEXIT)
                pszName = L"SystemExit";
            else if (pszSound == (LPCWSTR)SND_ALIAS_SYSTEMHAND)
                pszName = L"SystemHand";
            else if (pszSound == (LPCWSTR)SND_ALIAS_SYSTEMQUESTION)
                pszName = L"SystemQuestion";
            else if (pszSound == (LPCWSTR)SND_ALIAS_SYSTEMSTART)
                pszName = L"SystemStart";
            else if (pszSound == (LPCWSTR)SND_ALIAS_SYSTEMWELCOME)
                pszName = L"SystemWelcome";
            else
                goto Quit;
        }
        else
        {
            pszName = pszSound;
        }

        bIsDefault = (_wcsicmp(pszName, L"SystemDefault") == 0);
        hmmio = get_mmioFromProfile(fdwSound, pszName);
    }
    else if (fdwSound & SND_FILENAME)
    {
        hmmio = get_mmioFromFile(pszSound);
    }
    else
    {
        hmmio = get_mmioFromProfile(fdwSound, pszSound);
        if (!hmmio)
            hmmio = get_mmioFromFile(pszSound);
    }

Quit:
    if (!hmmio && !(fdwSound & SND_NODEFAULT))
    {
        if (fdwSound & SND_APPLICATION)
        {
            if (!bIsDefault)
            {
                /* Find application-defined default sound */
                hmmio = get_mmioFromProfile(fdwSound, L"SystemDefault");
                if (hmmio)
                    return hmmio;
            }

            /* Find system default sound */
            hmmio = get_mmioFromProfile(fdwSound & ~SND_APPLICATION, L"SystemDefault");
        }
        else if (!bIsDefault)
        {
            hmmio = get_mmioFromProfile(fdwSound, L"SystemDefault");
        }
    }

    return hmmio;
}

struct playsound_data
{
    HANDLE  hEvent;
    LONG    dwEventCount;
};

static void CALLBACK PlaySound_Callback(HWAVEOUT hwo, UINT uMsg,
                                        DWORD_PTR dwInstance,
                                        DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    struct playsound_data*  s = (struct playsound_data*)dwInstance;

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
    switch (fdwSound & (SND_RESOURCE | SND_ALIAS_ID | SND_FILENAME))
    {
        case SND_RESOURCE:
            return HIWORD(psz) != 0; /* by name or by ID ? */

        case SND_ALIAS_ID:
        case SND_MEMORY:
            return FALSE;

        case SND_ALIAS:
        case SND_FILENAME:
        case 0:
            return TRUE;

        default:
            FIXME("WTF\n");
            return FALSE;
    }
}

static void     PlaySound_Free(WINE_PLAYSOUND* wps)
{
    EnterCriticalSection(&WINMM_cs);
    PlaySoundCurrent = NULL;
    SetEvent(psLastEvent);
    LeaveCriticalSection(&WINMM_cs);
    if (wps->hmmio) mmioClose(wps->hmmio, 0);
    HeapFree(GetProcessHeap(), 0, wps);
}

static WINE_PLAYSOUND* PlaySound_AllocAndGetMMIO(const void* pszSound, HMODULE hmod,
                                                 DWORD fdwSound, BOOL bUnicode)
{
    BOOL bIsString;
    LPCWSTR pszSoundW;
    UNICODE_STRING usBuffer;
    WINE_PLAYSOUND* wps;

    bIsString = PlaySound_IsString(fdwSound, pszSound);

    if (bIsString && !bUnicode)
    {
        RtlCreateUnicodeStringFromAsciiz(&usBuffer, pszSound);
        if (!usBuffer.Buffer)
            return NULL;

        pszSoundW = usBuffer.Buffer;
    }
    else
    {
        pszSoundW = pszSound;
    }

    wps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wps));
    if (wps)
    {
        /* construct an MMIO stream (either in memory, or from a file) */
        wps->hmmio = PlaySound_GetMMIO(pszSoundW, hmod, fdwSound);
        if (!wps->hmmio)
        {
            PlaySound_Free(wps);
            wps = NULL;
        }
    }

    if (bIsString && !bUnicode)
        RtlFreeUnicodeString(&usBuffer);

    return wps;
}

static BOOL proc_PlaySound(WINE_PLAYSOUND *wps)
{
    BOOL                bRet = FALSE;
    MMCKINFO            ckMainRIFF;
    MMCKINFO            mmckInfo;
    LPWAVEFORMATEX      lpWaveFormat = NULL;
    HWAVEOUT            hWave = 0;
    LPWAVEHDR           waveHdr = NULL;
    INT                 count, bufsize, left, index;
    struct playsound_data       s;
    LONG                r;

    s.hEvent = 0;

    if (mmioDescend(wps->hmmio, &ckMainRIFF, NULL, 0))
        goto errCleanUp;

    TRACE("ParentChunk ckid=%.4s fccType=%.4s cksize=%08X\n",
          (LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType, ckMainRIFF.cksize);

    if ((ckMainRIFF.ckid != FOURCC_RIFF) ||
        (ckMainRIFF.fccType != mmioFOURCC('W', 'A', 'V', 'E')))
        goto errCleanUp;

    mmckInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(wps->hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK))
        goto errCleanUp;

    TRACE("Chunk Found ckid=%.4s fccType=%08x cksize=%08X\n",
          (LPSTR)&mmckInfo.ckid, mmckInfo.fccType, mmckInfo.cksize);

    lpWaveFormat = HeapAlloc(GetProcessHeap(), 0, mmckInfo.cksize);
    if (!lpWaveFormat)
        goto errCleanUp;
    r = mmioRead(wps->hmmio, (HPSTR)lpWaveFormat, mmckInfo.cksize);
    if (r < 0 || r < sizeof(PCMWAVEFORMAT))
        goto errCleanUp;

    TRACE("wFormatTag=%04X !\n",    lpWaveFormat->wFormatTag);
    TRACE("nChannels=%d\n",         lpWaveFormat->nChannels);
    TRACE("nSamplesPerSec=%d\n",    lpWaveFormat->nSamplesPerSec);
    TRACE("nAvgBytesPerSec=%d\n",   lpWaveFormat->nAvgBytesPerSec);
    TRACE("nBlockAlign=%d\n",       lpWaveFormat->nBlockAlign);
    TRACE("wBitsPerSample=%u !\n",  lpWaveFormat->wBitsPerSample);

    /* move to end of 'fmt ' chunk */
    mmioAscend(wps->hmmio, &mmckInfo, 0);

    mmckInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(wps->hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK))
        goto errCleanUp;

    TRACE("Chunk Found ckid=%.4s fccType=%08x cksize=%08X\n",
          (LPSTR)&mmckInfo.ckid, mmckInfo.fccType, mmckInfo.cksize);

    s.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!s.hEvent || bPlaySoundStop)
        goto errCleanUp;

    if (waveOutOpen(&hWave, WAVE_MAPPER, lpWaveFormat, (DWORD_PTR)PlaySound_Callback,
                    (DWORD_PTR)&s, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
        goto errCleanUp;

    /* make it so that 3 buffers per second are needed */
    bufsize = (((lpWaveFormat->nAvgBytesPerSec / 3) - 1) / lpWaveFormat->nBlockAlign + 1) *
        lpWaveFormat->nBlockAlign;
    waveHdr = HeapAlloc(GetProcessHeap(), 0, 2 * sizeof(WAVEHDR) + 2 * bufsize);
    if (!waveHdr)
        goto errCleanUp;
    waveHdr[0].lpData = (char*)waveHdr + 2 * sizeof(WAVEHDR);
    waveHdr[1].lpData = (char*)waveHdr + 2 * sizeof(WAVEHDR) + bufsize;
    waveHdr[0].dwUser = waveHdr[1].dwUser = 0L;
    waveHdr[0].dwLoops = waveHdr[1].dwLoops = 0L;
    waveHdr[0].dwFlags = waveHdr[1].dwFlags = 0L;
    waveHdr[0].dwBufferLength = waveHdr[1].dwBufferLength = bufsize;
    if (waveOutPrepareHeader(hWave, &waveHdr[0], sizeof(WAVEHDR)) ||
        waveOutPrepareHeader(hWave, &waveHdr[1], sizeof(WAVEHDR))) {
        goto errCleanUp;
    }

    wps->hWave = hWave;
    s.dwEventCount = 1L; /* for first buffer */
    index = 0;

    do {
        left = mmckInfo.cksize;

        mmioSeek(wps->hmmio, mmckInfo.dwDataOffset, SEEK_SET);
        while (left)
        {
            if (bPlaySoundStop)
            {
                wps->bLoop = FALSE;
                break;
            }
            count = mmioRead(wps->hmmio, waveHdr[index].lpData, min(bufsize, left));
            if (count < 1) break;
            left -= count;
            waveHdr[index].dwBufferLength = count;
            if (waveOutWrite(hWave, &waveHdr[index], sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
                index ^= 1;
                PlaySound_WaitDone(&s);
            }
            else {
                ERR("Aborting play loop, waveOutWrite error\n");
                wps->bLoop = FALSE;
                break;
            }
        }
        bRet = TRUE;
    } while (wps->bLoop);

    PlaySound_WaitDone(&s); /* to balance first buffer */
    waveOutReset(hWave);

    waveOutUnprepareHeader(hWave, &waveHdr[0], sizeof(WAVEHDR));
    waveOutUnprepareHeader(hWave, &waveHdr[1], sizeof(WAVEHDR));

errCleanUp:
    TRACE("Done playing sound => %s!\n", bRet ? "ok" : "ko");
    HeapFree(GetProcessHeap(), 0, lpWaveFormat);
    if (hWave)
    {
        EnterCriticalSection(&WINMM_cs);
        /* the CS prevents a concurrent waveOutReset */
        wps->hWave = 0;
        LeaveCriticalSection(&WINMM_cs);
        while (waveOutClose(hWave) == WAVERR_STILLPLAYING)
            Sleep(100);
    }
    CloseHandle(s.hEvent);
    HeapFree(GetProcessHeap(), 0, waveHdr);

    PlaySound_Free(wps);

    return bRet;
}

static DWORD WINAPI PlaySoundAsyncThreadProc(LPVOID lpParameter)
{
    WINE_PLAYSOUND *wps = (WINE_PLAYSOUND*)lpParameter;

    /* Play the sound */
    proc_PlaySound(wps);

    return 0;
}

static BOOL proc_PlaySoundAsync(WINE_PLAYSOUND *wps)
{
    HANDLE hThread;

    /* Create a thread to play the sound asynchronously */
    hThread = CreateThread(NULL, 0, PlaySoundAsyncThreadProc, wps, 0, NULL);
    if (hThread)
    {
        SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
        CloseHandle(hThread);
        return TRUE;
    }

    /* Error cases */
    PlaySound_Free(wps);
    return FALSE;
}

static BOOL MULTIMEDIA_PlaySound(const void* pszSound, HMODULE hmod, DWORD fdwSound, BOOL bUnicode)
{
    WINE_PLAYSOUND*     wps = NULL;

    TRACE("pszSound='%p' hmod=%p fdwSound=%08X\n",
          pszSound, hmod, fdwSound);

    /* SND_NOWAIT is ignored in w95/2k/xp. */
    if ((fdwSound & SND_NOSTOP) && PlaySoundCurrent != NULL)
        return FALSE;

    /* alloc internal structure, if we need to play something */
    if (pszSound && !(fdwSound & SND_PURGE))
    {
        if (!(wps = PlaySound_AllocAndGetMMIO(pszSound, hmod, fdwSound, bUnicode)))
            return FALSE;
    }

    EnterCriticalSection(&WINMM_cs);
    /* since several threads can enter PlaySound in parallel, we're not
     * sure, at this point, that another thread didn't start a new playsound
     */
    while (PlaySoundCurrent != NULL)
    {
        ResetEvent(psLastEvent);
        /* FIXME: doc says we have to stop all instances of pszSound if it's non
         * NULL... as of today, we stop all playing instances */
        bPlaySoundStop = TRUE;
        if (PlaySoundCurrent->hWave)
            waveOutReset(PlaySoundCurrent->hWave);

        LeaveCriticalSection(&WINMM_cs);
        WaitForSingleObject(psLastEvent, INFINITE);
        EnterCriticalSection(&WINMM_cs);

        bPlaySoundStop = FALSE;
    }

    PlaySoundCurrent = wps;
    LeaveCriticalSection(&WINMM_cs);

    if (!wps) return TRUE;

    if (fdwSound & SND_ASYNC)
    {
        wps->bLoop = (fdwSound & SND_LOOP) ? TRUE : FALSE;
        
        return proc_PlaySoundAsync(wps);
    }
    
    return proc_PlaySound(wps);
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
    uFlags &= SND_RESOURCE|SND_ALIAS_ID|SND_FILENAME|SND_ASYNC|SND_LOOP|SND_MEMORY|SND_NODEFAULT|SND_NOSTOP|SND_SYNC;
    return MULTIMEDIA_PlaySound(pszSoundA, 0, uFlags, FALSE);
}

/**************************************************************************
 * 				sndPlaySoundW		[WINMM.@]
 */
BOOL WINAPI sndPlaySoundW(LPCWSTR pszSound, UINT uFlags)
{
    uFlags &= SND_RESOURCE|SND_ALIAS_ID|SND_FILENAME|SND_ASYNC|SND_LOOP|SND_MEMORY|SND_NODEFAULT|SND_NOSTOP|SND_SYNC;
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
