/*
 * Unit tests for msacm functions
 *
 * Copyright (c) 2004 Robert Reif
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "mmsystem.h"
#define NOBITMAP
#include "mmreg.h"
#include "msacm.h"
#include "wine/msacmdrv.h"

static BOOL CALLBACK FormatTagEnumProc(HACMDRIVERID hadid,
                                       PACMFORMATTAGDETAILSA paftd,
                                       DWORD_PTR dwInstance,
                                       DWORD fdwSupport)
{
    MMRESULT rc;
    HACMDRIVER had;

    if (winetest_interactive)
        trace("   Format 0x%04x: %s\n", paftd->dwFormatTag, paftd->szFormatTag);

    rc = acmDriverOpen(&had, hadid, 0);
    ok(rc == MMSYSERR_NOERROR || rc == MMSYSERR_NODRIVER,
       "acmDriverOpen(): rc = %08x, should be %08x\n",
       rc, MMSYSERR_NOERROR);

    if (rc == MMSYSERR_NOERROR)
    {
        ACMFORMATDETAILSA fd = {0};
        WAVEFORMATEX *pwfx, dst;
        ACMFORMATTAGDETAILSA aftd_pcm = {0};
        DWORD dwSize, dwSizeMax;
        DWORD i;

        fd.cbStruct = sizeof(fd);
        if (paftd->cbFormatSize < sizeof(WAVEFORMATEX))
            pwfx = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WAVEFORMATEX));
        else
            pwfx = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, paftd->cbFormatSize);
        fd.pwfx = pwfx;
        fd.cbwfx = paftd->cbFormatSize;
        fd.dwFormatTag = paftd->dwFormatTag;

        /* try bad pwfx */
        fd.pwfx = NULL;
        rc = acmFormatDetailsA(had, &fd, ACM_FORMATDETAILSF_FORMAT);
        ok(rc == MMSYSERR_INVALPARAM,
           "acmFormatDetailsA(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALPARAM);
        fd.pwfx = pwfx;

        /* try bad wFormatTag */
        fd.pwfx->wFormatTag = WAVE_FORMAT_UNKNOWN;
        rc = acmFormatDetailsA(had, &fd, ACM_FORMATDETAILSF_FORMAT);
        ok(rc == MMSYSERR_INVALPARAM,
           "acmFormatDetailsA(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALPARAM);
        fd.pwfx->wFormatTag = paftd->dwFormatTag;

        /* try bad fdwSupport */
        fd.fdwSupport = 0xdeadbeef;
        rc = acmFormatDetailsA(had, &fd, ACM_FORMATDETAILSF_FORMAT);
        ok(rc == MMSYSERR_INVALPARAM,
           "acmFormatDetailsA(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALPARAM);
        fd.fdwSupport = 0;

        /* try bad pwfx structure size */
        fd.cbwfx = sizeof(PCMWAVEFORMAT)-1;
        rc = acmFormatDetailsA(had, &fd, ACM_FORMATDETAILSF_FORMAT);
        ok(rc == MMSYSERR_INVALPARAM,
           "acmFormatDetailsA(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALPARAM);
        fd.cbwfx = paftd->cbFormatSize;

        /* test bad parameters (all zero) */
        rc = acmFormatDetailsA(had, &fd, ACM_FORMATDETAILSF_FORMAT);
        ok(rc == ACMERR_NOTPOSSIBLE,
           "acmFormatDetailsA(): rc = %08x, should be %08x\n",
           rc, ACMERR_NOTPOSSIBLE);

        /* test acmFormatSuggest */

        /* if we don't specify a format, we must give at least the driver's maximum size for any format */
        acmMetrics((HACMOBJ)had, ACM_METRIC_MAX_SIZE_FORMAT, &dwSize);
        rc = acmFormatSuggest(had, pwfx, &dst, dwSize-1, 0);
        ok(rc == MMSYSERR_INVALPARAM,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALPARAM);

        rc = acmFormatSuggest(had, pwfx, &dst, dwSize, 0);
        ok(rc == ACMERR_NOTPOSSIBLE,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, ACMERR_NOTPOSSIBLE);

        /* if we do specify a format, we must give at least the driver's maximum size for that format */
        aftd_pcm.cbStruct = sizeof(aftd_pcm);
        aftd_pcm.dwFormatTag = WAVE_FORMAT_PCM;
        rc = acmFormatTagDetailsA(had, &aftd_pcm, ACM_FORMATTAGDETAILSF_LARGESTSIZE);
        ok(rc == MMSYSERR_NOERROR, "returned %08x\n", rc);

        dst.wFormatTag = WAVE_FORMAT_PCM;
        rc = acmFormatSuggest(had, pwfx, &dst, aftd_pcm.cbFormatSize-1, ACM_FORMATSUGGESTF_WFORMATTAG);
        ok(rc == MMSYSERR_INVALPARAM,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALPARAM);

        rc = acmFormatSuggest(had, pwfx, &dst, aftd_pcm.cbFormatSize, ACM_FORMATSUGGESTF_WFORMATTAG);
        ok(rc == ACMERR_NOTPOSSIBLE,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, ACMERR_NOTPOSSIBLE);

        /* test nonexistent format */
        dst.wFormatTag = 0xbeef;
        rc = acmFormatSuggest(had, pwfx, &dst, 0, ACM_FORMATSUGGESTF_WFORMATTAG);
        ok(rc == ACMERR_NOTPOSSIBLE || rc == MMSYSERR_INVALPARAM,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, ACMERR_NOTPOSSIBLE);

        /* if the driver is NULL, we must give at least the maximum size for any driver */
        acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &dwSizeMax);
        rc = acmFormatSuggest(NULL, pwfx, &dst, dwSizeMax-1, 0);
        ok(rc == MMSYSERR_INVALPARAM,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALPARAM);

        if (paftd->dwFormatTag != WAVE_FORMAT_PCM)
        {
            rc = acmFormatSuggest(NULL, pwfx, &dst, dwSizeMax, 0);
            ok(rc == ACMERR_NOTPOSSIBLE,
               "acmFormatSuggest(): rc = %08x, should be %08x\n",
               rc, ACMERR_NOTPOSSIBLE);
        }

        /* if we specify a dst format, we must give the maximum size for that format */
        dst.wFormatTag = WAVE_FORMAT_PCM;
        rc = acmFormatSuggest(NULL, pwfx, &dst, aftd_pcm.cbFormatSize-1, ACM_FORMATSUGGESTF_WFORMATTAG);
        ok(rc == MMSYSERR_INVALPARAM || broken (rc == ACMERR_NOTPOSSIBLE), /* WinXP */
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALPARAM);

        rc = acmFormatSuggest(NULL, pwfx, &dst, aftd_pcm.cbFormatSize, ACM_FORMATSUGGESTF_WFORMATTAG);
        ok(rc == ACMERR_NOTPOSSIBLE,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, ACMERR_NOTPOSSIBLE);

        dst.wFormatTag = paftd->dwFormatTag;
        rc = acmFormatSuggest(NULL, pwfx, &dst, paftd->cbFormatSize-1, ACM_FORMATSUGGESTF_WFORMATTAG);
        ok(rc == MMSYSERR_INVALPARAM || broken (rc == ACMERR_NOTPOSSIBLE), /* WinXP */
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALPARAM);

        rc = acmFormatSuggest(NULL, pwfx, &dst, paftd->cbFormatSize, ACM_FORMATSUGGESTF_WFORMATTAG);
        ok(rc == ACMERR_NOTPOSSIBLE,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, ACMERR_NOTPOSSIBLE);

        /* test nonexistent format */
        dst.wFormatTag = 0xbeef;
        rc = acmFormatSuggest(NULL, pwfx, &dst, 0, ACM_FORMATSUGGESTF_WFORMATTAG);
        ok(rc == ACMERR_NOTPOSSIBLE || rc == MMSYSERR_INVALPARAM,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, ACMERR_NOTPOSSIBLE);

        /* test index */
        for (i = 0; i < paftd->cStandardFormats; i++)
        {
            fd.dwFormatIndex = i;

            fd.fdwSupport = 0;
            fd.cbwfx = paftd->cbFormatSize;
            fd.pwfx->cbSize = 0xbeef;
            rc = acmFormatDetailsA(had, &fd, ACM_FORMATDETAILSF_INDEX);
            ok(rc == MMSYSERR_NOERROR,
               "acmFormatDetailsA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_NOERROR);

            /* Windows will write cbSize (and other data) even if the
             * given cbwfx is not large enough */
            fd.fdwSupport = 0;
            fd.cbwfx = sizeof(PCMWAVEFORMAT);
            fd.pwfx->cbSize = 0xbeef;
            rc = acmFormatDetailsA(had, &fd, ACM_FORMATDETAILSF_INDEX);
            todo_wine_if(rc != MMSYSERR_NOERROR) /* remove when fixed */
            ok(rc == MMSYSERR_NOERROR,
               "acmFormatDetailsA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_NOERROR);
            if (paftd->dwFormatTag != WAVE_FORMAT_PCM)
                todo_wine_if(fd.pwfx->cbSize != paftd->cbFormatSize - sizeof(WAVEFORMATEX)) /* remove when fixed */
                ok(fd.pwfx->cbSize == paftd->cbFormatSize - sizeof(WAVEFORMATEX),
                   "got %d\n", fd.pwfx->cbSize);
        }

        /* one more */
        fd.dwFormatIndex = paftd->cStandardFormats;
        fd.fdwSupport = 0;
        rc = acmFormatDetailsA(had, &fd, ACM_FORMATDETAILSF_INDEX);
        ok(rc == MMSYSERR_INVALPARAM,
           "acmFormatDetailsA(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALPARAM);

        HeapFree(GetProcessHeap(), 0, pwfx);
    }
    return TRUE;
}

static BOOL CALLBACK FormatEnumProc(HACMDRIVERID hadid,
                                    LPACMFORMATDETAILSA pafd,
                                    DWORD_PTR dwInstance,
                                    DWORD fd)
{
    MMRESULT rc;
    HACMDRIVER had;
    WAVEFORMATEX *dst, *dstMax;
    DWORD dwSize, dwSizeMax;
    DWORD fdwSupport;

    acmMetrics((HACMOBJ)hadid, ACM_METRIC_DRIVER_SUPPORT, &fdwSupport);

    if (winetest_interactive)
        trace("   0x%04x, %s\n", pafd->dwFormatTag, pafd->szFormat);

    acmDriverOpen(&had, hadid, 0);
    dwSize = pafd->cbwfx;
    dst = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);

    /* test acmFormatSuggest with valid src format */
    if (pafd->dwFormatTag == WAVE_FORMAT_PCM)
    {
        rc = acmFormatSuggest(had, pafd->pwfx, dst, dwSize, 0);
        /* this fails on some decode-only drivers */
        ok(rc == MMSYSERR_NOERROR || rc == ACMERR_NOTPOSSIBLE,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_NOERROR);
        if (rc == MMSYSERR_NOERROR)
        {
            if (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC) /* supports different conversions */
                ok(dst->wFormatTag != WAVE_FORMAT_PCM, "expected different format\n");
            else
                ok(dst->wFormatTag == WAVE_FORMAT_PCM,
                   "expected %d, got %d\n", WAVE_FORMAT_PCM, dst->wFormatTag);
        }
    }
    else
    {
        rc = acmFormatSuggest(had, pafd->pwfx, dst, dwSize, 0);
        ok(rc == MMSYSERR_NOERROR,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_NOERROR);
        ok(dst->wFormatTag == WAVE_FORMAT_PCM,
           "expected %d, got %d\n", WAVE_FORMAT_PCM, dst->wFormatTag);
        ok(dst->nChannels == pafd->pwfx->nChannels,
           "expected %d, got %d\n", pafd->pwfx->nChannels, dst->nChannels);
        if (pafd->dwFormatTag != 0x42) /* codec 0x0042 returns a different sample rate */
            ok(dst->nSamplesPerSec == pafd->pwfx->nSamplesPerSec,
               "expected %d, got %d\n", pafd->pwfx->nSamplesPerSec, dst->nSamplesPerSec);
        ok(dst->wBitsPerSample == 16,
           "expected %d, got %d\n", 16, dst->wBitsPerSample);
        ok(dst->nBlockAlign == 2*pafd->pwfx->nChannels,
           "expected %d, got %d\n", 2*pafd->pwfx->nChannels, dst->nBlockAlign);

        /* test with NULL driver */
        acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &dwSizeMax);
        dstMax = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSizeMax);
        rc = acmFormatSuggest(NULL, pafd->pwfx, dstMax, dwSizeMax, 0);
        ok(rc == MMSYSERR_NOERROR,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_NOERROR);

        HeapFree(GetProcessHeap(), 0, dstMax);
    }

    ZeroMemory(dst, dwSize);
    dst->wFormatTag = pafd->pwfx->wFormatTag;
    rc = acmFormatSuggest(had, pafd->pwfx, dst, dwSize, ACM_FORMATSUGGESTF_WFORMATTAG);
    if (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CONVERTER) /* supports same conversions */
        ok(rc == MMSYSERR_NOERROR,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_NOERROR);
    else
        todo_wine_if(rc != ACMERR_NOTPOSSIBLE)
        ok(rc == ACMERR_NOTPOSSIBLE,
           "acmFormatSuggest(): rc = %08x, should be %08x\n",
           rc, ACMERR_NOTPOSSIBLE);

    HeapFree(GetProcessHeap(), 0, dst);
    acmDriverClose(had, 0);

    return TRUE;
}

static BOOL CALLBACK DriverEnumProc(HACMDRIVERID hadid,
                                    DWORD_PTR dwInstance,
                                    DWORD fdwSupport)
{
    MMRESULT rc;
    ACMDRIVERDETAILSA dd;
    HACMDRIVER had;
    
    DWORD dwDriverPriority;
    DWORD dwDriverSupport;

    if (winetest_interactive) {
        trace("id: %p\n", hadid);
        trace("  Supports:\n");
        if (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_ASYNC)
            trace("    async conversions\n");
        if (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC)
            trace("    different format conversions\n");
        if (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CONVERTER)
            trace("    same format conversions\n");
        if (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_FILTER)
            trace("    filtering\n");
    }

    /* try an invalid pointer */
    rc = acmDriverDetailsA(hadid, 0, 0);
    ok(rc == MMSYSERR_INVALPARAM,
       "acmDriverDetailsA(): rc = %08x, should be %08x\n",
       rc, MMSYSERR_INVALPARAM);

    /* try an invalid structure size */
    ZeroMemory(&dd, sizeof(dd));
    rc = acmDriverDetailsA(hadid, &dd, 0);
    ok(rc == MMSYSERR_INVALPARAM,
       "acmDriverDetailsA(): rc = %08x, should be %08x\n",
       rc, MMSYSERR_INVALPARAM);

    /* MSDN says this should fail but it doesn't in practice */
    dd.cbStruct = 4;
    rc = acmDriverDetailsA(hadid, &dd, 0);
    ok(rc == MMSYSERR_NOERROR || rc == MMSYSERR_NOTSUPPORTED,
       "acmDriverDetailsA(): rc = %08x, should be %08x\n",
       rc, MMSYSERR_NOERROR);

    /* try an invalid handle */
    dd.cbStruct = sizeof(dd);
    rc = acmDriverDetailsA((HACMDRIVERID)1, &dd, 0);
    ok(rc == MMSYSERR_INVALHANDLE,
       "acmDriverDetailsA(): rc = %08x, should be %08x\n",
       rc, MMSYSERR_INVALHANDLE);

    /* try an invalid handle and pointer */
    rc = acmDriverDetailsA((HACMDRIVERID)1, 0, 0);
    ok(rc == MMSYSERR_INVALPARAM,
       "acmDriverDetailsA(): rc = %08x, should be %08x\n",
       rc, MMSYSERR_INVALPARAM);

    /* try invalid details */
    rc = acmDriverDetailsA(hadid, &dd, -1);
    ok(rc == MMSYSERR_INVALFLAG,
       "acmDriverDetailsA(): rc = %08x, should be %08x\n",
       rc, MMSYSERR_INVALFLAG);

    /* try valid parameters */
    rc = acmDriverDetailsA(hadid, &dd, 0);
    ok(rc == MMSYSERR_NOERROR || rc == MMSYSERR_NOTSUPPORTED,
       "acmDriverDetailsA(): rc = %08x, should be %08x\n",
       rc, MMSYSERR_NOERROR);

    /* cbStruct should contain size of returned data (at most sizeof(dd)) 
       TODO: should it be *exactly* sizeof(dd), as tested here?
     */
    if (rc == MMSYSERR_NOERROR) {
        static const struct {
            const char *shortname;
            WORD mid;
            WORD pid;
            WORD pid_alt;
        } *iter, expected_ids[] = {
            { "Microsoft IMA ADPCM", MM_MICROSOFT, MM_MSFT_ACM_IMAADPCM },
            { "MS-ADPCM", MM_MICROSOFT, MM_MSFT_ACM_MSADPCM },
            { "Microsoft CCITT G.711", MM_MICROSOFT, MM_MSFT_ACM_G711},
            { "MPEG Layer-3 Codec", MM_FRAUNHOFER_IIS, MM_FHGIIS_MPEGLAYER3_DECODE, MM_FHGIIS_MPEGLAYER3_PROFESSIONAL },
            { "MS-PCM", MM_MICROSOFT, MM_MSFT_ACM_PCM },
            { 0 }
        };

        ok(dd.cbStruct == sizeof(dd),
            "acmDriverDetailsA(): cbStruct = %08x\n", dd.cbStruct);

        for (iter = expected_ids; iter->shortname; ++iter) {
            if (!strcmp(iter->shortname, dd.szShortName)) {
                /* try alternative product id on mismatch */
                if (iter->pid_alt && iter->pid != dd.wPid)
                    ok(iter->mid == dd.wMid && iter->pid_alt == dd.wPid,
                        "Got wrong manufacturer (0x%x vs 0x%x) or product (0x%x vs 0x%x)\n",
                        dd.wMid, iter->mid,
                        dd.wPid, iter->pid_alt);
                else
                    ok(iter->mid == dd.wMid && iter->pid == dd.wPid,
                        "Got wrong manufacturer (0x%x vs 0x%x) or product (0x%x vs 0x%x)\n",
                        dd.wMid, iter->mid,
                        dd.wPid, iter->pid);
            }
        }
    }

    if (rc == MMSYSERR_NOERROR && winetest_interactive) {
        trace("  Short name: %s\n", dd.szShortName);
        trace("  Long name: %s\n", dd.szLongName);
        trace("  Copyright: %s\n", dd.szCopyright);
        trace("  Licensing: %s\n", dd.szLicensing);
        trace("  Features: %s\n", dd.szFeatures);
        trace("  Supports %u formats\n", dd.cFormatTags);
        trace("  Supports %u filter formats\n", dd.cFilterTags);
        trace("  Mid: 0x%x\n", dd.wMid);
        trace("  Pid: 0x%x\n", dd.wPid);
    }

    /* try bad pointer */
    rc = acmMetrics((HACMOBJ)hadid, ACM_METRIC_DRIVER_PRIORITY, 0);
    ok(rc == MMSYSERR_INVALPARAM,
        "acmMetrics(): rc = %08x, should be %08x\n",
        rc, MMSYSERR_INVALPARAM);

    /* try bad handle */
    rc = acmMetrics((HACMOBJ)1, ACM_METRIC_DRIVER_PRIORITY, &dwDriverPriority);
    ok(rc == MMSYSERR_INVALHANDLE,
        "acmMetrics(): rc = %08x, should be %08x\n",
        rc, MMSYSERR_INVALHANDLE);

    /* try bad pointer and handle */
    rc = acmMetrics((HACMOBJ)1, ACM_METRIC_DRIVER_PRIORITY, 0);
    ok(rc == MMSYSERR_INVALHANDLE,
        "acmMetrics(): rc = %08x, should be %08x\n",
        rc, MMSYSERR_INVALHANDLE);

    /* try valid parameters */
    rc = acmMetrics((HACMOBJ)hadid, ACM_METRIC_DRIVER_PRIORITY, &dwDriverSupport);
    ok(rc == MMSYSERR_NOERROR,
        "acmMetrics(): rc = %08x, should be %08x\n",
        rc, MMSYSERR_NOERROR);

    /* try bad pointer */
    rc = acmMetrics((HACMOBJ)hadid, ACM_METRIC_DRIVER_SUPPORT, 0);
    ok(rc == MMSYSERR_INVALPARAM,
        "acmMetrics(): rc = %08x, should be %08x\n",
        rc, MMSYSERR_INVALPARAM);

    /* try bad handle */
    rc = acmMetrics((HACMOBJ)1, ACM_METRIC_DRIVER_SUPPORT, &dwDriverSupport);
    ok(rc == MMSYSERR_INVALHANDLE,
        "acmMetrics(): rc = %08x, should be %08x\n",
        rc, MMSYSERR_INVALHANDLE);

    /* try bad pointer and handle */
    rc = acmMetrics((HACMOBJ)1, ACM_METRIC_DRIVER_SUPPORT, 0);
    ok(rc == MMSYSERR_INVALHANDLE,
        "acmMetrics(): rc = %08x, should be %08x\n",
        rc, MMSYSERR_INVALHANDLE);

    /* try valid parameters */
    rc = acmMetrics((HACMOBJ)hadid, ACM_METRIC_DRIVER_SUPPORT, &dwDriverSupport);
    ok(rc == MMSYSERR_NOERROR,
        "acmMetrics(): rc = %08x, should be %08x\n",
        rc, MMSYSERR_NOERROR);

    /* try invalid pointer */
    rc = acmDriverOpen(0, hadid, 0);
    ok(rc == MMSYSERR_INVALPARAM,
       "acmDriverOpen(): rc = %08x, should be %08x\n",
       rc, MMSYSERR_INVALPARAM);

    /* try invalid handle */
    rc = acmDriverOpen(&had, (HACMDRIVERID)1, 0);
    ok(rc == MMSYSERR_INVALHANDLE,
       "acmDriverOpen(): rc = %08x, should be %08x\n",
       rc, MMSYSERR_INVALHANDLE);

    /* try invalid open */
    rc = acmDriverOpen(&had, hadid, -1);
    ok(rc == MMSYSERR_INVALFLAG,
       "acmDriverOpen(): rc = %08x, should be %08x\n",
       rc, MMSYSERR_INVALFLAG);

    /* try valid parameters */
    rc = acmDriverOpen(&had, hadid, 0);
    ok(rc == MMSYSERR_NOERROR || rc == MMSYSERR_NODRIVER,
       "acmDriverOpen(): rc = %08x, should be %08x\n",
       rc, MMSYSERR_NOERROR);

    if (rc == MMSYSERR_NOERROR) {
        DWORD dwSize;
        HACMDRIVERID hid;

        /* try bad pointer */
        rc = acmDriverID((HACMOBJ)had, 0, 0);
        ok(rc == MMSYSERR_INVALPARAM,
           "acmDriverID(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALPARAM);

        /* try bad handle */
        rc = acmDriverID((HACMOBJ)1, &hid, 0);
        ok(rc == MMSYSERR_INVALHANDLE,
           "acmDriverID(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALHANDLE);

        /* try bad handle and pointer */
        rc = acmDriverID((HACMOBJ)1, 0, 0);
        ok(rc == MMSYSERR_INVALHANDLE,
           "acmDriverID(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALHANDLE);

        /* try bad flag */
        rc = acmDriverID((HACMOBJ)had, &hid, 1);
        ok(rc == MMSYSERR_INVALFLAG,
           "acmDriverID(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALFLAG);

        /* try valid parameters */
        rc = acmDriverID((HACMOBJ)had, &hid, 0);
        ok(rc == MMSYSERR_NOERROR,
           "acmDriverID(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_NOERROR);
        ok(hid == hadid,
           "acmDriverID() returned ID %p doesn't equal %p\n",
           hid, hadid);

        /* try bad pointer */
        rc = acmMetrics((HACMOBJ)had, ACM_METRIC_MAX_SIZE_FORMAT, 0);
        ok(rc == MMSYSERR_INVALPARAM,
           "acmMetrics(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALPARAM);

        /* try bad handle */
        rc = acmMetrics((HACMOBJ)1, ACM_METRIC_MAX_SIZE_FORMAT, &dwSize);
        ok(rc == MMSYSERR_INVALHANDLE,
           "acmMetrics(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALHANDLE);

        /* try bad pointer and handle */
        rc = acmMetrics((HACMOBJ)1, ACM_METRIC_MAX_SIZE_FORMAT, 0);
        ok(rc == MMSYSERR_INVALHANDLE,
           "acmMetrics(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_INVALHANDLE);

        /* try valid parameters */
        rc = acmMetrics((HACMOBJ)had, ACM_METRIC_MAX_SIZE_FORMAT, &dwSize);
        ok(rc == MMSYSERR_NOERROR,
           "acmMetrics(): rc = %08x, should be %08x\n",
           rc, MMSYSERR_NOERROR);
        if (rc == MMSYSERR_NOERROR) {
            ACMFORMATDETAILSA fd;
            WAVEFORMATEX * pwfx;
            ACMFORMATTAGDETAILSA aftd;

            /* try bad pointer */
            rc = acmFormatEnumA(had, 0, FormatEnumProc, 0, 0);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatEnumA(): rc = %08x, should be %08x\n",
                rc, MMSYSERR_INVALPARAM);

            /* try bad structure size */
            ZeroMemory(&fd, sizeof(fd));
            rc = acmFormatEnumA(had, &fd, FormatEnumProc, 0, 0);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatEnumA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALPARAM);

            fd.cbStruct = sizeof(fd) - 1;
            rc = acmFormatEnumA(had, &fd, FormatEnumProc, 0, 0);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatEnumA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALPARAM);

            pwfx = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);

            if (dwSize >= sizeof(WAVEFORMATEX))
                pwfx->cbSize = LOWORD(dwSize) - sizeof(WAVEFORMATEX);
            pwfx->wFormatTag = WAVE_FORMAT_UNKNOWN;

            fd.cbStruct = sizeof(fd);
            fd.pwfx = pwfx;
            fd.cbwfx = dwSize;
            fd.dwFormatTag = WAVE_FORMAT_UNKNOWN;

            /* try bad callback */
            rc = acmFormatEnumA(had, &fd, NULL, 0, 0);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatEnumA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALPARAM);

            /* try bad pwfx */
            fd.pwfx = NULL;
            rc = acmFormatEnumA(had, &fd, FormatEnumProc, 0, 0);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatEnumA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALPARAM);
            fd.pwfx = pwfx;

            /* fdwSupport must be zero */
            fd.fdwSupport = 0xdeadbeef;
            rc = acmFormatEnumA(had, &fd, FormatEnumProc, 0, 0);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatEnumA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALPARAM);
            fd.fdwSupport = 0;

            /* try bad pwfx structure size */
            fd.cbwfx = dwSize-1;
            rc = acmFormatEnumA(had, &fd, FormatEnumProc, 0, 0);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatEnumA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALPARAM);
            fd.cbwfx = dwSize;

            /* try valid parameters */
            rc = acmFormatEnumA(had, &fd, FormatEnumProc, 0, 0);
            ok(rc == MMSYSERR_NOERROR,
               "acmFormatEnumA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_NOERROR);

            /* try bad pointer */
            rc = acmFormatTagEnumA(had, 0, FormatTagEnumProc, 0, 0);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatTagEnumA(): rc = %08x, should be %08x\n",
                rc, MMSYSERR_INVALPARAM);

            /* try bad structure size */
            ZeroMemory(&aftd, sizeof(aftd));
            rc = acmFormatTagEnumA(had, &aftd, FormatTagEnumProc, 0, 0);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatTagEnumA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALPARAM);

            aftd.cbStruct = sizeof(aftd) - 1;
            rc = acmFormatTagEnumA(had, &aftd, FormatTagEnumProc, 0, 0);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatTagEnumA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALPARAM);

            aftd.cbStruct = sizeof(aftd);
            aftd.dwFormatTag = WAVE_FORMAT_UNKNOWN;

            /* try bad flag */
            rc = acmFormatTagEnumA(had, &aftd, FormatTagEnumProc, 0, 1);
            ok(rc == MMSYSERR_INVALFLAG,
               "acmFormatTagEnumA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALFLAG);

            /* try valid parameters */
            rc = acmFormatTagEnumA(had, &aftd, FormatTagEnumProc, 0, 0);
            ok(rc == MMSYSERR_NOERROR,
               "acmFormatTagEnumA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_NOERROR);

            /* try bad pointer */
            rc = acmFormatDetailsA(had, NULL, ACM_FORMATDETAILSF_INDEX);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatDetailsA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALPARAM);

            /* try bad structure size */
            ZeroMemory(&fd, sizeof(fd));
            rc = acmFormatDetailsA(had, &fd, ACM_FORMATDETAILSF_INDEX);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatDetailsA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALPARAM);

            fd.cbStruct = sizeof(fd) - 1;
            rc = acmFormatDetailsA(had, &fd, ACM_FORMATDETAILSF_INDEX);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatDetailsA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALPARAM);

            fd.cbStruct = sizeof(fd);
            fd.pwfx = pwfx;
            ZeroMemory(fd.pwfx, dwSize);
            fd.cbwfx = dwSize;
            fd.dwFormatTag = WAVE_FORMAT_UNKNOWN;

            /* try WAVE_FORMAT_UNKNOWN */
            rc = acmFormatDetailsA(had, &fd, ACM_FORMATDETAILSF_INDEX);
            ok(rc == MMSYSERR_INVALPARAM,
               "acmFormatDetailsA(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALPARAM);

            HeapFree(GetProcessHeap(), 0, pwfx);

            /* try invalid handle */
            rc = acmDriverClose((HACMDRIVER)1, 0);
            ok(rc == MMSYSERR_INVALHANDLE,
               "acmDriverClose(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALHANDLE);

            /* try invalid flag */
            rc = acmDriverClose(had, 1);
            ok(rc == MMSYSERR_INVALFLAG,
               "acmDriverClose(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALFLAG);

            /* try valid parameters */
            rc = acmDriverClose(had, 0);
            ok(rc == MMSYSERR_NOERROR,
               "acmDriverClose(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_NOERROR);

            /* try closing again */
            rc = acmDriverClose(had, 0);
            ok(rc == MMSYSERR_INVALHANDLE,
               "acmDriverClose(): rc = %08x, should be %08x\n",
               rc, MMSYSERR_INVALHANDLE);
        }
    }

    return TRUE;
}

static const char * get_metric(UINT uMetric)
{
    switch (uMetric) {
    case ACM_METRIC_COUNT_CODECS:
        return "ACM_METRIC_COUNT_CODECS";
    case ACM_METRIC_COUNT_CONVERTERS:
        return "ACM_METRIC_COUNT_CONVERTERS";
    case ACM_METRIC_COUNT_DISABLED:
        return "ACM_METRIC_COUNT_DISABLED";
    case ACM_METRIC_COUNT_DRIVERS:
        return "ACM_METRIC_COUNT_DRIVERS";
    case ACM_METRIC_COUNT_FILTERS:
        return "ACM_METRIC_COUNT_FILTERS";
    case ACM_METRIC_COUNT_HARDWARE:
        return "ACM_METRIC_COUNT_HARDWARE";
    case ACM_METRIC_COUNT_LOCAL_CODECS:
        return "ACM_METRIC_COUNT_LOCAL_CODECS";
    case ACM_METRIC_COUNT_LOCAL_CONVERTERS:
        return "ACM_METRIC_COUNT_LOCAL_CONVERTERS";
    case ACM_METRIC_COUNT_LOCAL_DISABLED:
        return "ACM_METRIC_COUNT_LOCAL_DISABLED";
    case ACM_METRIC_COUNT_LOCAL_DRIVERS:
        return "ACM_METRIC_COUNT_LOCAL_DRIVERS";
    case ACM_METRIC_COUNT_LOCAL_FILTERS:
        return "ACM_METRIC_COUNT_LOCAL_FILTERS";
    case ACM_METRIC_DRIVER_PRIORITY:
        return "ACM_METRIC_DRIVER_PRIORITY";
    case ACM_METRIC_DRIVER_SUPPORT:
        return "ACM_METRIC_DRIVER_SUPPORT";
    case ACM_METRIC_HARDWARE_WAVE_INPUT:
        return "ACM_METRIC_HARDWARE_WAVE_INPUT";
    case ACM_METRIC_HARDWARE_WAVE_OUTPUT:
        return "ACM_METRIC_HARDWARE_WAVE_OUTPUT";
    case ACM_METRIC_MAX_SIZE_FILTER:
        return "ACM_METRIC_MAX_SIZE_FILTER";
    case ACM_METRIC_MAX_SIZE_FORMAT:
        return "ACM_METRIC_MAX_SIZE_FORMAT";
    }

    return "UNKNOWN";
}

static void check_count(UINT uMetric)
{
    DWORD dwMetric;
    MMRESULT rc;

    /* try invalid result pointer */
    rc = acmMetrics(NULL, uMetric, 0);
    ok(rc == MMSYSERR_INVALPARAM,
       "acmMetrics(NULL, %s, 0): rc = 0x%08x, should be 0x%08x\n",
       get_metric(uMetric), rc, MMSYSERR_INVALPARAM);

    /* try invalid handle */
    rc = acmMetrics((HACMOBJ)1, uMetric, &dwMetric);
    ok(rc == MMSYSERR_INVALHANDLE,
       "acmMetrics(1, %s, %p): rc = 0x%08x, should be 0x%08x\n",
       get_metric(uMetric), &dwMetric, rc, MMSYSERR_INVALHANDLE);

    /* try invalid result pointer and handle */
    rc = acmMetrics((HACMOBJ)1, uMetric, 0);
    ok(rc == MMSYSERR_INVALHANDLE,
       "acmMetrics(1, %s, 0): rc = 0x%08x, should be 0x%08x\n",
       get_metric(uMetric), rc, MMSYSERR_INVALHANDLE);

    /* try valid parameters */
    rc = acmMetrics(NULL, uMetric, &dwMetric);
    ok(rc == MMSYSERR_NOERROR, "acmMetrics() failed: rc = 0x%08x\n", rc);

    if (rc == MMSYSERR_NOERROR && winetest_interactive)
        trace("%s: %u\n", get_metric(uMetric), dwMetric);
}

static void driver_tests(void)
{
    MMRESULT rc;
    DWORD dwACMVersion = acmGetVersion();

    if (winetest_interactive) {
        trace("ACM version = %u.%02u build %u%s\n",
            HIWORD(dwACMVersion) >> 8,
            HIWORD(dwACMVersion) & 0xff,
            LOWORD(dwACMVersion),
            LOWORD(dwACMVersion)  ==  0 ? " (Retail)" : "");
    }

    check_count(ACM_METRIC_COUNT_CODECS);
    check_count(ACM_METRIC_COUNT_CONVERTERS);
    check_count(ACM_METRIC_COUNT_DISABLED);
    check_count(ACM_METRIC_COUNT_DRIVERS);
    check_count(ACM_METRIC_COUNT_FILTERS);
    check_count(ACM_METRIC_COUNT_HARDWARE);
    check_count(ACM_METRIC_COUNT_LOCAL_CODECS);
    check_count(ACM_METRIC_COUNT_LOCAL_CONVERTERS);
    check_count(ACM_METRIC_COUNT_LOCAL_DISABLED);
    check_count(ACM_METRIC_COUNT_LOCAL_DRIVERS);
    check_count(ACM_METRIC_COUNT_LOCAL_FILTERS);

    if (winetest_interactive)
        trace("enabled drivers:\n");

    rc = acmDriverEnum(DriverEnumProc, 0, 0);
    ok(rc == MMSYSERR_NOERROR,
      "acmDriverEnum() failed, rc=%08x, should be 0x%08x\n",
      rc, MMSYSERR_NOERROR);
}

static void test_prepareheader(void)
{
    HACMSTREAM has;
    ADPCMWAVEFORMAT *src;
    WAVEFORMATEX dst;
    MMRESULT mr;
    ACMSTREAMHEADER hdr;
    BYTE buf[sizeof(WAVEFORMATEX) + 32], pcm[2048], input[512];
    ADPCMCOEFSET *coef;

    src = (ADPCMWAVEFORMAT*)buf;
    coef = src->aCoef;
    src->wfx.cbSize = 32;
    src->wfx.wFormatTag = WAVE_FORMAT_ADPCM;
    src->wfx.nSamplesPerSec = 22050;
    src->wfx.wBitsPerSample = 4;
    src->wfx.nChannels = 1;
    src->wfx.nBlockAlign = 512;
    src->wfx.nAvgBytesPerSec = 11025;
    src->wSamplesPerBlock = 0x3f4;
    src->wNumCoef = 7;
    coef[0].iCoef1 = 0x0100;
    coef[0].iCoef2 = 0x0000;
    coef[1].iCoef1 = 0x0200;
    coef[1].iCoef2 = 0xff00;
    coef[2].iCoef1 = 0x0000;
    coef[2].iCoef2 = 0x0000;
    coef[3].iCoef1 = 0x00c0;
    coef[3].iCoef2 = 0x0040;
    coef[4].iCoef1 = 0x00f0;
    coef[4].iCoef2 = 0x0000;
    coef[5].iCoef1 = 0x01cc;
    coef[5].iCoef2 = 0xff30;
    coef[6].iCoef1 = 0x0188;
    coef[6].iCoef2 = 0xff18;

    dst.cbSize = 0;
    dst.wFormatTag = WAVE_FORMAT_PCM;
    dst.nSamplesPerSec = 22050;
    dst.wBitsPerSample = 8;
    dst.nChannels = 1;
    dst.nBlockAlign = dst.wBitsPerSample * dst.nChannels / 8;
    dst.nAvgBytesPerSec = dst.nSamplesPerSec * dst.nBlockAlign;

    mr = acmStreamOpen(&has, NULL, (WAVEFORMATEX*)src, &dst, NULL, 0, 0, 0);
    ok(mr == MMSYSERR_NOERROR, "open failed: 0x%x\n", mr);

    memset(input, 0, sizeof(input));
    memset(&hdr, 0, sizeof(hdr));
    hdr.cbStruct = sizeof(hdr);
    hdr.pbSrc = input;
    hdr.cbSrcLength = sizeof(input);
    hdr.pbDst = pcm;
    hdr.cbDstLength = sizeof(pcm);

    mr = acmStreamPrepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_NOERROR, "prepare failed: 0x%x\n", mr);
    ok(hdr.fdwStatus == ACMSTREAMHEADER_STATUSF_PREPARED, "header wasn't prepared: 0x%x\n", hdr.fdwStatus);

    mr = acmStreamUnprepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_NOERROR, "unprepare failed: 0x%x\n", mr);
    ok(hdr.fdwStatus == 0, "header wasn't unprepared: 0x%x\n", hdr.fdwStatus);

    memset(&hdr, 0, sizeof(hdr));
    hdr.cbStruct = sizeof(hdr);
    hdr.pbSrc = input;
    hdr.cbSrcLength = 0; /* invalid source length */
    hdr.pbDst = pcm;
    hdr.cbDstLength = sizeof(pcm);

    mr = acmStreamPrepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_INVALPARAM, "expected 0x0b, got 0x%x\n", mr);

    hdr.cbSrcLength = src->wfx.nBlockAlign - 1; /* less than block align */
    mr = acmStreamPrepareHeader(has, &hdr, 0);
    ok(mr == ACMERR_NOTPOSSIBLE, "expected 0x200, got 0x%x\n", mr);

    hdr.cbSrcLength = src->wfx.nBlockAlign + 1; /* more than block align */
    mr = acmStreamPrepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_NOERROR, "prepare failed: 0x%x\n", mr);

    mr = acmStreamUnprepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_NOERROR, "unprepare failed: 0x%x\n", mr);

    hdr.cbSrcLength = src->wfx.nBlockAlign;
    mr = acmStreamPrepareHeader(has, &hdr, 1); /* invalid use of reserved parameter */
    ok(mr == MMSYSERR_INVALFLAG, "expected 0x0a, got 0x%x\n", mr);

    mr = acmStreamPrepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_NOERROR, "prepare failed: 0x%x\n", mr);

    mr = acmStreamUnprepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_NOERROR, "unprepare failed: 0x%x\n", mr);

    memset(&hdr, 0, sizeof(hdr));
    hdr.cbStruct = sizeof(hdr);
    hdr.pbSrc = input;
    hdr.cbSrcLength = sizeof(input);
    hdr.pbDst = pcm;
    hdr.cbDstLength = sizeof(pcm);
    hdr.fdwStatus = ACMSTREAMHEADER_STATUSF_DONE;

    mr = acmStreamPrepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_NOERROR, "prepare failed: 0x%x\n", mr);
    ok(hdr.fdwStatus == (ACMSTREAMHEADER_STATUSF_PREPARED | ACMSTREAMHEADER_STATUSF_DONE), "header wasn't prepared: 0x%x\n", hdr.fdwStatus);

    hdr.cbSrcLengthUsed = 12345;
    hdr.cbDstLengthUsed = 12345;
    hdr.fdwStatus &= ~ACMSTREAMHEADER_STATUSF_DONE;
    mr = acmStreamConvert(has, &hdr, ACM_STREAMCONVERTF_BLOCKALIGN);
    ok(mr == MMSYSERR_NOERROR, "convert failed: 0x%x\n", mr);
    ok(hdr.fdwStatus & ACMSTREAMHEADER_STATUSF_DONE, "conversion was not done: 0x%x\n", hdr.fdwStatus);
    ok(hdr.cbSrcLengthUsed == hdr.cbSrcLength, "expected %d, got %d\n", hdr.cbSrcLength, hdr.cbSrcLengthUsed);
todo_wine
    ok(hdr.cbDstLengthUsed == 1010, "expected 1010, got %d\n", hdr.cbDstLengthUsed);

    mr = acmStreamUnprepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_NOERROR, "unprepare failed: 0x%x\n", mr);
    ok(hdr.fdwStatus == ACMSTREAMHEADER_STATUSF_DONE, "header wasn't unprepared: 0x%x\n", hdr.fdwStatus);

    /* The 2 next tests are related to Lost Horizon (bug 24723) */
    memset(&hdr, 0, sizeof(hdr));
    hdr.cbStruct = sizeof(hdr);
    hdr.pbSrc = input;
    hdr.cbSrcLength = sizeof(input);
    hdr.pbDst = pcm;
    hdr.cbDstLength = -4;

    mr = acmStreamPrepareHeader(has, &hdr, 0);
    if (sizeof(void *) == 4) /* 64 bit fails on this test */
    {
        ok(mr == MMSYSERR_NOERROR, "prepare failed: 0x%x\n", mr);
        ok(hdr.fdwStatus == ACMSTREAMHEADER_STATUSF_PREPARED, "header wasn't prepared: 0x%x\n", hdr.fdwStatus);

        hdr.cbSrcLengthUsed = 12345;
        hdr.cbDstLengthUsed = 12345;
        hdr.fdwStatus &= ~ACMSTREAMHEADER_STATUSF_DONE;
        mr = acmStreamConvert(has, &hdr, ACM_STREAMCONVERTF_BLOCKALIGN);
        ok(mr == MMSYSERR_NOERROR, "convert failed: 0x%x\n", mr);
        ok(hdr.fdwStatus & ACMSTREAMHEADER_STATUSF_DONE, "conversion was not done: 0x%x\n", hdr.fdwStatus);
        ok(hdr.cbSrcLengthUsed == hdr.cbSrcLength, "expected %d, got %d\n", hdr.cbSrcLength, hdr.cbSrcLengthUsed);
todo_wine
        ok(hdr.cbDstLengthUsed == 1010, "expected 1010, got %d\n", hdr.cbDstLengthUsed);

        mr = acmStreamUnprepareHeader(has, &hdr, 0);
        ok(mr == MMSYSERR_NOERROR, "unprepare failed: 0x%x\n", mr);
        ok(hdr.fdwStatus == ACMSTREAMHEADER_STATUSF_DONE, "header wasn't unprepared: 0x%x\n", hdr.fdwStatus);
    }
    else
todo_wine
        ok(mr == MMSYSERR_INVALPARAM, "expected 0x0b, got 0x%x\n", mr);

    memset(&hdr, 0, sizeof(hdr));
    hdr.cbStruct = sizeof(hdr);
    hdr.pbSrc = input;
    hdr.cbSrcLength = 24;
    hdr.pbDst = pcm;
    hdr.cbDstLength = -4;
    mr = acmStreamPrepareHeader(has, &hdr, 0);
    ok(mr == ACMERR_NOTPOSSIBLE, "expected 0x200, got 0x%x\n", mr);
    ok(hdr.fdwStatus == 0, "expected 0, got 0x%x\n", hdr.fdwStatus);

    hdr.cbSrcLengthUsed = 12345;
    hdr.cbDstLengthUsed = 12345;
    mr = acmStreamConvert(has, &hdr, ACM_STREAMCONVERTF_BLOCKALIGN);
    ok(mr == ACMERR_UNPREPARED, "expected 0x202, got 0x%x\n", mr);
    ok(hdr.cbSrcLengthUsed == 12345, "expected 12345, got %d\n", hdr.cbSrcLengthUsed);
    ok(hdr.cbDstLengthUsed == 12345, "expected 12345, got %d\n", hdr.cbDstLengthUsed);

    mr = acmStreamUnprepareHeader(has, &hdr, 0);
    ok(mr == ACMERR_UNPREPARED, "expected 0x202, got 0x%x\n", mr);

    /* Less output space than required */
    memset(&hdr, 0, sizeof(hdr));
    hdr.cbStruct = sizeof(hdr);
    hdr.pbSrc = input;
    hdr.cbSrcLength = sizeof(input);
    hdr.pbDst = pcm;
    hdr.cbDstLength = 32;

    mr = acmStreamPrepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_NOERROR, "prepare failed: 0x%x\n", mr);
    ok(hdr.fdwStatus == ACMSTREAMHEADER_STATUSF_PREPARED, "header wasn't prepared: 0x%x\n", hdr.fdwStatus);

    hdr.cbSrcLengthUsed = 12345;
    hdr.cbDstLengthUsed = 12345;
    hdr.fdwStatus &= ~ACMSTREAMHEADER_STATUSF_DONE;
    mr = acmStreamConvert(has, &hdr, ACM_STREAMCONVERTF_BLOCKALIGN);
    ok(mr == MMSYSERR_NOERROR, "convert failed: 0x%x\n", mr);
    ok(hdr.fdwStatus & ACMSTREAMHEADER_STATUSF_DONE, "conversion was not done: 0x%x\n", hdr.fdwStatus);
todo_wine
    ok(hdr.cbSrcLengthUsed == hdr.cbSrcLength, "expected %d, got %d\n", hdr.cbSrcLength, hdr.cbSrcLengthUsed);
todo_wine
    ok(hdr.cbDstLengthUsed == hdr.cbDstLength, "expected %d, got %d\n", hdr.cbDstLength, hdr.cbDstLengthUsed);

    mr = acmStreamUnprepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_NOERROR, "unprepare failed: 0x%x\n", mr);
    ok(hdr.fdwStatus == ACMSTREAMHEADER_STATUSF_DONE, "header wasn't unprepared: 0x%x\n", hdr.fdwStatus);

    mr = acmStreamClose(has, 0);
    ok(mr == MMSYSERR_NOERROR, "close failed: 0x%x\n", mr);
}

static const BYTE input[64] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63};

struct stream_output
{
    WAVEFORMATEX src;
    WAVEFORMATEX dst;
    BYTE output[256];
    DWORD dst_used;
    BOOL todo;
};

static const struct stream_output expected_output[] = {
    /* #0: Identical conversion */
    {{WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8}, {WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8}, {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63}, 64, FALSE},

    /* #1: 1 -> 2 channels */
    {{WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8}, {WAVE_FORMAT_PCM, 2, 8000, 16000, 2, 8}, {0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,22,23,23,24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31,32,32,33,33,34,34,35,35,36,36,37,37,38,38,39,39,40,40,41,41,42,42,43,43,44,44,45,45,46,46,47,47,48,48,49,49,50,50,51,51,52,52,53,53,54,54,55,55,56,56,57,57,58,58,59,59,60,60,61,61,62,62,63,63}, 128, FALSE},

    /* #2: 2 -> 1 channels: all of the audio underflows due to addition */
    {{WAVE_FORMAT_PCM, 2, 8000, 16000, 2, 8}, {WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 32, FALSE},

    /* #3: 2 -> 2 channels */
    {{WAVE_FORMAT_PCM, 2, 8000, 16000, 2, 8}, {WAVE_FORMAT_PCM, 2, 8000, 16000, 2, 8}, {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63}, 64, FALSE},

    /* #4: 8 -> 16 bits per sample */
    {{WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8}, {WAVE_FORMAT_PCM, 1, 8000, 16000, 2, 16}, {0,128,0,129,0,130,0,131,0,132,0,133,0,134,0,135,0,136,0,137,0,138,0,139,0,140,0,141,0,142,0,143,0,144,0,145,0,146,0,147,0,148,0,149,0,150,0,151,0,152,0,153,0,154,0,155,0,156,0,157,0,158,0,159,0,160,0,161,0,162,0,163,0,164,0,165,0,166,0,167,0,168,0,169,0,170,0,171,0,172,0,173,0,174,0,175,0,176,0,177,0,178,0,179,0,180,0,181,0,182,0,183,0,184,0,185,0,186,0,187,0,188,0,189,0,190,0,191}, 128, FALSE},

    /* #5: 16 -> 8 bits per sample */
    {{WAVE_FORMAT_PCM, 1, 8000, 16000, 2, 16}, {WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8}, {129,131,133,135,137,139,141,143,145,147,149,151,153,155,157,159,161,163,165,167,169,171,173,175,177,179,181,183,185,187,189,191}, 32, FALSE},

    /* #6: 16 bits per sample, 2 -> 1 channels */
    {{WAVE_FORMAT_PCM, 2, 8000, 32000, 4, 16}, {WAVE_FORMAT_PCM, 1, 8000, 16000, 2, 16}, {2,4,10,12,18,20,26,28,34,36,42,44,50,52,58,60,66,68,74,76,82,84,90,92,98,100,106,108,114,116,122,124}, 32, FALSE},

    /* #7: 8000 -> 11025 sample rate */
    /* FIXME: upsampling is slightly off on wine - the algorithm is wrong whenever error > (srcrate + dstrate) / 2 */
    {{WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8}, {WAVE_FORMAT_PCM, 1, 11025, 11025, 1, 8}, {0,1,1,2,3,4,4,5,6,7,7,8,9,9,10,11,12,12,13,14,15,15,16,17,17,18,19,20,20,21,22,22,23,24,25,25,26,27,28,28,29,30,30,31,32,33,33,34,35,36,36,37,38,38,39,40,41,41,42,43,44,44,45,46,46,47,48,49,49,50,51,52,52,53,54,54,55,56,57,57,58,59,60,60,61,62,62,63}, 88, TRUE},

    /* #8: 8000 -> 22050 sample rate */
    {{WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8}, {WAVE_FORMAT_PCM, 1, 22050, 22050, 1, 8}, {0,0,1,1,1,2,2,3,3,3,4,4,4,5,5,5,6,6,7,7,7,8,8,8,9,9,9,10,10,11,11,11,12,12,12,13,13,13,14,14,15,15,15,16,16,16,17,17,17,18,18,19,19,19,20,20,20,21,21,21,22,22,22,23,23,24,24,24,25,25,25,26,26,26,27,27,28,28,28,29,29,29,30,30,30,31,31,32,32,32,33,33,33,34,34,34,35,35,36,36,36,37,37,37,38,38,38,39,39,40,40,40,41,41,41,42,42,42,43,43,44,44,44,45,45,45,46,46,46,47,47,48,48,48,49,49,49,50,50,50,51,51,52,52,52,53,53,53,54,54,54,55,55,56,56,56,57,57,57,58,58,58,59,59,60,60,60,61,61,61,62,62,62,63,63,63}, 176, TRUE},

    /* #9: 11025 -> 22050 sample rate */
    {{WAVE_FORMAT_PCM, 1, 11025, 11025, 1, 8}, {WAVE_FORMAT_PCM, 1, 22050, 22050, 1, 8}, {0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,22,23,23,24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31,32,32,33,33,34,34,35,35,36,36,37,37,38,38,39,39,40,40,41,41,42,42,43,43,44,44,45,45,46,46,47,47,48,48,49,49,50,50,51,51,52,52,53,53,54,54,55,55,56,56,57,57,58,58,59,59,60,60,61,61,62,62,63,63}, 128, FALSE},

    /* #10: 22050 -> 11025 sample rate */
    {{WAVE_FORMAT_PCM, 1, 22050, 22050, 1, 8}, {WAVE_FORMAT_PCM, 1, 11025, 11025, 1, 8}, {1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63}, 32, FALSE},

    /* #11: 11025 -> 8000 sample rate */
    {{WAVE_FORMAT_PCM, 1, 11025, 11025, 1, 8}, {WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8}, {0,2,3,4,6,7,8,10,11,13,14,15,17,18,19,21,22,24,25,26,28,29,31,32,33,35,36,37,39,40,42,43,44,46,47,48,50,51,53,54,55,57,58,59,61,62}, 46, FALSE},

    /* #12: 22050 -> 8000 sample rate */
    {{WAVE_FORMAT_PCM, 1, 22050, 22050, 1, 8}, {WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8}, {1,4,6,9,12,15,17,20,23,26,28,31,34,37,39,42,45,48,50,53,56,59,62}, 23, FALSE},

    /* #13: 44100 -> 8000 sample rate */
    {{WAVE_FORMAT_PCM, 1, 44100, 44100, 1, 8}, {WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8}, {2,8,13,19,24,30,35,41,46,52,57,63}, 12, FALSE},
};

static void test_convert(void)
{
    HACMSTREAM has;
    ACMSTREAMHEADER hdr = {0};
    BYTE output[256];
    MMRESULT mmr;
    unsigned i;

    for (i = 0; i < ARRAY_SIZE(expected_output); i++)
    {
        mmr = acmStreamOpen(&has, NULL, (WAVEFORMATEX *)&expected_output[i].src, (WAVEFORMATEX *)&expected_output[i].dst, NULL, 0, 0, 0);
        ok(mmr == MMSYSERR_NOERROR, "#%d: open failed: 0x%x\n", i, mmr);

        memset(&hdr, 0, sizeof(hdr));
        hdr.cbStruct = sizeof(hdr);
        hdr.pbSrc = (BYTE *)input;
        hdr.cbSrcLength = sizeof(input);
        hdr.pbDst = output;
        hdr.cbDstLength = sizeof(output);

        mmr = acmStreamPrepareHeader(has, &hdr, 0);
        ok(mmr == MMSYSERR_NOERROR, "#%d: prepare failed: 0x%x\n", i, mmr);
        ok(hdr.fdwStatus == ACMSTREAMHEADER_STATUSF_PREPARED, "#%d: header wasn't prepared: 0x%x\n", i, hdr.fdwStatus);

        memset(&output, 0, sizeof(output));
        mmr = acmStreamConvert(has, &hdr, ACM_STREAMCONVERTF_BLOCKALIGN);
        ok(mmr == MMSYSERR_NOERROR, "#%d: convert failed: 0x%x\n", i, mmr);
        ok(hdr.fdwStatus & ACMSTREAMHEADER_STATUSF_DONE, "#%d: conversion was not done: 0x%x\n", i, hdr.fdwStatus);
        ok(hdr.cbSrcLengthUsed == hdr.cbSrcLength, "#%d: expected %d, got %d\n", i, hdr.cbSrcLength, hdr.cbSrcLengthUsed);
        ok(hdr.cbDstLengthUsed == expected_output[i].dst_used, "#%d: expected %d, got %d\n", i, expected_output[i].dst_used, hdr.cbDstLengthUsed);
todo_wine_if(expected_output[i].todo)
        ok(!memcmp(expected_output[i].output, output, hdr.cbDstLengthUsed), "#%d: output does not match\n", i);

        mmr = acmStreamUnprepareHeader(has, &hdr, 0);
        ok(mmr == MMSYSERR_NOERROR, "#%d: unprepare failed: 0x%x\n", i, mmr);
        ok(hdr.fdwStatus == ACMSTREAMHEADER_STATUSF_DONE, "#%d: header wasn't unprepared: 0x%x\n", i, hdr.fdwStatus);

        mmr = acmStreamClose(has, 0);
        ok(mmr == MMSYSERR_NOERROR, "#%d: close failed: 0x%x\n", i, mmr);
    }
}

static void test_acmFormatSuggest(void)
{
    WAVEFORMATEX src, dst;
    DWORD suggest;
    MMRESULT rc;
    DWORD sizeMax;

    acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &sizeMax);

    /* Test a valid PCM format */
    src.wFormatTag = WAVE_FORMAT_PCM;
    src.nChannels = 1;
    src.nSamplesPerSec = 8000;
    src.nAvgBytesPerSec = 16000;
    src.nBlockAlign = 2;
    src.wBitsPerSample = 16;
    src.cbSize = 0;
    suggest = 0;
    memset(&dst, 0, sizeof(dst));
    rc = acmFormatSuggest(NULL, &src, &dst, sizeof(PCMWAVEFORMAT), suggest);
    ok(rc == MMSYSERR_NOERROR, "failed with error 0x%x\n", rc);
    ok(src.wFormatTag == dst.wFormatTag, "expected %d, got %d\n", src.wFormatTag, dst.wFormatTag);
    ok(src.nChannels == dst.nChannels, "expected %d, got %d\n", src.nChannels, dst.nChannels);
    ok(src.nSamplesPerSec == dst.nSamplesPerSec, "expected %d, got %d\n", src.nSamplesPerSec, dst.nSamplesPerSec);
    ok(src.nAvgBytesPerSec == dst.nAvgBytesPerSec, "expected %d, got %d\n", src.nAvgBytesPerSec, dst.nAvgBytesPerSec);
    ok(src.nBlockAlign == dst.nBlockAlign, "expected %d, got %d\n", src.nBlockAlign, dst.nBlockAlign);
    ok(src.wBitsPerSample == dst.wBitsPerSample, "expected %d, got %d\n", src.wBitsPerSample, dst.wBitsPerSample);

    /* All parameters from destination are valid */
    suggest = ACM_FORMATSUGGESTF_NCHANNELS
            | ACM_FORMATSUGGESTF_NSAMPLESPERSEC
            | ACM_FORMATSUGGESTF_WBITSPERSAMPLE
            | ACM_FORMATSUGGESTF_WFORMATTAG;
    dst = src;
    rc = acmFormatSuggest(NULL, &src, &dst, sizeof(PCMWAVEFORMAT), suggest);
    ok(rc == MMSYSERR_NOERROR, "failed with error 0x%x\n", rc);
    ok(src.wFormatTag == dst.wFormatTag, "expected %d, got %d\n", src.wFormatTag, dst.wFormatTag);
    ok(src.nChannels == dst.nChannels, "expected %d, got %d\n", src.nChannels, dst.nChannels);
    ok(src.nSamplesPerSec == dst.nSamplesPerSec, "expected %d, got %d\n", src.nSamplesPerSec, dst.nSamplesPerSec);
    ok(src.nAvgBytesPerSec == dst.nAvgBytesPerSec, "expected %d, got %d\n", src.nAvgBytesPerSec, dst.nAvgBytesPerSec);
    ok(src.nBlockAlign == dst.nBlockAlign, "expected %d, got %d\n", src.nBlockAlign, dst.nBlockAlign);
    ok(src.wBitsPerSample == dst.wBitsPerSample, "expected %d, got %d\n", src.wBitsPerSample, dst.wBitsPerSample);

    /* Test an invalid PCM format */
    ZeroMemory(&dst, sizeof(dst));
    src.nSamplesPerSec = 0xdeadbeef;
    suggest = 0;
    rc = acmFormatSuggest(NULL, &src, &dst, sizeMax, suggest);
    todo_wine {
    ok(rc == MMSYSERR_NOERROR, "failed with error 0x%x\n", rc);
    ok(dst.wFormatTag == WAVE_FORMAT_PCM, "expected %d, got %d\n", WAVE_FORMAT_PCM, dst.wFormatTag);
    ok(dst.nSamplesPerSec == 0xdeadbeef, "expected %d, got %d\n", 0xdeadbeef, dst.nSamplesPerSec);
    }
    src.nSamplesPerSec = 8000;

    /* Test a nonexistent format */
    src.wFormatTag = 0xbeef;
    rc = acmFormatSuggest(NULL, &src, &dst, sizeMax-1, suggest);
    ok(rc == MMSYSERR_INVALPARAM, "failed with error 0x%x\n", rc);

    rc = acmFormatSuggest(NULL, &src, &dst, sizeMax, suggest);
    todo_wine
    ok(rc == MMSYSERR_NODRIVER, "failed with error 0x%x\n", rc);

    /* Test converting between two known but incompatible formats */
    src.wFormatTag = WAVE_FORMAT_ALAW;
    src.nChannels = 1;
    src.nSamplesPerSec = 8000;
    src.nAvgBytesPerSec = 8000;
    src.nBlockAlign = 1;
    src.wBitsPerSample = 8;
    src.cbSize = 0;
    suggest = ACM_FORMATSUGGESTF_WFORMATTAG;
    dst.wFormatTag = WAVE_FORMAT_IMA_ADPCM;
    rc = acmFormatSuggest(NULL, &src, &dst, sizeof(IMAADPCMWAVEFORMAT)-1, suggest);
    ok(rc == MMSYSERR_INVALPARAM, "failed with error 0x%x\n", rc);

    rc = acmFormatSuggest(NULL, &src, &dst, sizeof(IMAADPCMWAVEFORMAT), suggest);
    todo_wine
    ok(rc == MMSYSERR_NODRIVER, "failed with error 0x%x\n", rc);

    /* Invalid suggest flags */
    src.wFormatTag = WAVE_FORMAT_PCM;
    suggest = 0xFFFFFFFF;
    rc = acmFormatSuggest(NULL, &src, &dst, sizeof(dst), suggest);
    ok(rc == MMSYSERR_INVALFLAG, "failed with error 0x%x\n", rc);

    /* Invalid source and destination */
    suggest = 0;
    rc = acmFormatSuggest(NULL, NULL, &dst, sizeof(dst), suggest);
    ok(rc == MMSYSERR_INVALPARAM, "failed with error 0x%x\n", rc);
    rc = acmFormatSuggest(NULL, &src, NULL, sizeof(dst), suggest);
    ok(rc == MMSYSERR_INVALPARAM, "failed with error 0x%x\n", rc);
    rc = acmFormatSuggest(NULL, NULL, NULL, sizeof(dst), suggest);
    ok(rc == MMSYSERR_INVALPARAM, "failed with error 0x%x\n", rc);
}

static void test_acmFormatTagDetails(void)
{
    ACMFORMATTAGDETAILSW aftd = {0};
    MMRESULT rc;

    aftd.cbStruct = sizeof(aftd);
    aftd.dwFormatTag = WAVE_FORMAT_MPEGLAYER3;
    rc = acmFormatTagDetailsW(NULL, &aftd, ACM_FORMATTAGDETAILSF_FORMATTAG);
    if (rc == MMSYSERR_NOERROR)
        ok(aftd.cbFormatSize == sizeof(MPEGLAYER3WAVEFORMAT), "got %d\n", aftd.cbFormatSize);
}

static void test_acmFormatChoose(void)
{
    ACMFORMATCHOOSEW afc = {0};
    WAVEFORMATEX *pwfx;
    DWORD sizeMax;
    MMRESULT rc;

    acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &sizeMax);
    pwfx = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeMax);

    afc.cbStruct = sizeof(afc);
    afc.pwfx = pwfx;

    /* test invalid struct size */
    afc.cbStruct = sizeof(afc)-1;
    rc = acmFormatChooseW(&afc);
    ok(rc == MMSYSERR_INVALPARAM, "expected 0xb, got 0x%x\n", rc);
    afc.cbStruct = sizeof(afc);

    afc.pwfx = NULL;
    rc = acmFormatChooseW(&afc);
    ok(rc == MMSYSERR_INVALPARAM, "expected 0xb, got 0x%x\n", rc);
    afc.pwfx = pwfx;

    HeapFree(GetProcessHeap(), 0, pwfx);
}

static void test_mp3(void)
{
    MPEGLAYER3WAVEFORMAT src;
    WAVEFORMATEX dst;
    HACMSTREAM has;
    DWORD output;
    MMRESULT mr;

    src.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
    src.wfx.nSamplesPerSec = 11025;
    src.wfx.wBitsPerSample = 0;
    src.wfx.nChannels = 1;
    src.wfx.nBlockAlign = 576;
    src.wfx.nAvgBytesPerSec = 2000;

    src.wID = MPEGLAYER3_ID_MPEG;
    src.fdwFlags = 0;
    src.nBlockSize = 576;
    src.nFramesPerBlock = 1;
    src.nCodecDelay = 0;

    dst.cbSize = 0;
    dst.wFormatTag = WAVE_FORMAT_PCM;
    dst.nSamplesPerSec = 11025;
    dst.wBitsPerSample = 16;
    dst.nChannels = 1;
    dst.nBlockAlign = dst.wBitsPerSample * dst.nChannels / 8;
    dst.nAvgBytesPerSec = dst.nSamplesPerSec * dst.nBlockAlign;

    src.wfx.cbSize = 0;

    mr = acmStreamOpen(&has, NULL, (WAVEFORMATEX*)&src, &dst, NULL, 0, 0, 0);
    ok(mr == ACMERR_NOTPOSSIBLE, "expected error ACMERR_NOTPOSSIBLE, got 0x%x\n", mr);
    if (mr == MMSYSERR_NOERROR) acmStreamClose(has, 0);

    src.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
    src.wID = 0;

    mr = acmStreamOpen(&has, NULL, (WAVEFORMATEX*)&src, &dst, NULL, 0, 0, 0);
    ok(mr == ACMERR_NOTPOSSIBLE, "expected error ACMERR_NOTPOSSIBLE, got 0x%x\n", mr);
    if (mr == MMSYSERR_NOERROR) acmStreamClose(has, 0);

    src.wID = MPEGLAYER3_ID_MPEG;
    src.nBlockSize = 0;

    mr = acmStreamOpen(&has, NULL, (WAVEFORMATEX*)&src, &dst, NULL, 0, 0, 0);
    ok(mr == MMSYSERR_NOERROR || broken(mr == ACMERR_NOTPOSSIBLE) /* Win 2008 */,
       "failed with error 0x%x\n", mr);
    if (mr == MMSYSERR_NOERROR)
    {
       mr = acmStreamClose(has, 0);
       ok(mr == MMSYSERR_NOERROR, "failed with error 0x%x\n", mr);
    }
    src.nBlockSize = 576;
    src.wfx.nAvgBytesPerSec = 0;

    mr = acmStreamOpen(&has, NULL, (WAVEFORMATEX*)&src, &dst, NULL, 0, 0, 0);
    ok(mr == MMSYSERR_NOERROR || broken(mr == ACMERR_NOTPOSSIBLE) /* Win 2008 */,
       "failed with error 0x%x\n", mr);
    if (mr == MMSYSERR_NOERROR)
    {
       /* causes a division by zero exception in XP, Vista,
          but throws ACMERR_NOTPOSSIBLE on others */
       if (0) acmStreamSize(has, 4000, &output, ACM_STREAMSIZEF_SOURCE);
       mr = acmStreamClose(has, 0);
       ok(mr == MMSYSERR_NOERROR, "failed with error 0x%x\n", mr);
    }

    src.wfx.nAvgBytesPerSec = 2000;

    mr = acmStreamOpen(&has, NULL, (WAVEFORMATEX*)&src, &dst, NULL, 0, 0, 0);
    ok(mr == MMSYSERR_NOERROR || broken(mr == ACMERR_NOTPOSSIBLE) /* Win 2008 */,
       "failed with error 0x%x\n", mr);
    if (mr == MMSYSERR_NOERROR)
    {
       mr = acmStreamSize(has, 4000, &output, ACM_STREAMSIZEF_SOURCE);
       ok(mr == MMSYSERR_NOERROR, "failed with error 0x%x\n", mr);
       mr = acmStreamClose(has, 0);
       ok(mr == MMSYSERR_NOERROR, "failed with error 0x%x\n", mr);
    }
}

static struct
{
    struct
    {
        int load, free, open, close, enable, disable, install,
            remove, details, notify, querycfg, about;
    } driver;
    struct
    {
        int tag_details, details, suggest;
    } format;
    struct
    {
        int open, close, size, convert, prepare, unprepare, reset;
    } stream;
    int other;
} driver_calls;

static LRESULT CALLBACK acm_driver_func(DWORD_PTR id, HDRVR handle, UINT msg, LPARAM param1, LPARAM param2)
{
    switch (msg)
    {
        /* Driver messages */
        case DRV_LOAD:
            driver_calls.driver.load++;
            return 1;
        case DRV_FREE:
            driver_calls.driver.free++;
            return 1;
        case DRV_OPEN:
            driver_calls.driver.open++;
            return 1;
        case DRV_CLOSE:
            driver_calls.driver.close++;
            return 1;
        case DRV_ENABLE:
            driver_calls.driver.enable++;
            return 1;
        case DRV_DISABLE:
            driver_calls.driver.disable++;
            return 1;
        case DRV_QUERYCONFIGURE:
            driver_calls.driver.querycfg++;
            return 1;
        case DRV_INSTALL:
            driver_calls.driver.install++;
            return DRVCNF_RESTART;
        case DRV_REMOVE:
            driver_calls.driver.remove++;
            return DRVCNF_RESTART;
        case ACMDM_DRIVER_ABOUT:
            driver_calls.driver.about++;
            return MMSYSERR_NOTSUPPORTED;
        case ACMDM_DRIVER_DETAILS:
        {
            ACMDRIVERDETAILSA *ptr = (ACMDRIVERDETAILSA *)param1;

            /* copied from pcmconverter.c */
            ptr->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
            ptr->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
            ptr->wMid = MM_MICROSOFT;
            ptr->wPid = MM_MSFT_ACM_PCM;
            ptr->vdwACM = 0x01000000;
            ptr->vdwDriver = 0x01000000;
            ptr->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CONVERTER;
            ptr->cFormatTags = 1;
            ptr->cFilterTags = 0;
            ptr->hicon = NULL;
            strcpy(ptr->szShortName, "TEST-CODEC");
            strcpy(ptr->szLongName, "Wine Test Codec");
            strcpy(ptr->szCopyright, "Brought to you by the Wine team...");
            strcpy(ptr->szLicensing, "Refer to LICENSE file");
            ptr->szFeatures[0] = 0;

            driver_calls.driver.details++;
            break;
        }
        case ACMDM_DRIVER_NOTIFY:
            driver_calls.driver.notify++;
            return MMSYSERR_NOTSUPPORTED;

        /* Format messages */
        case ACMDM_FORMATTAG_DETAILS:
            driver_calls.format.tag_details++;
            break;
        case ACMDM_FORMAT_DETAILS:
            driver_calls.format.details++;
            break;
        case ACMDM_FORMAT_SUGGEST:
            driver_calls.format.suggest++;
            break;

        /* Stream messages */
        case ACMDM_STREAM_OPEN:
            driver_calls.stream.open++;
            break;
        case ACMDM_STREAM_CLOSE:
            driver_calls.stream.close++;
            break;
        case ACMDM_STREAM_SIZE:
            driver_calls.stream.size++;
            break;
        case ACMDM_STREAM_CONVERT:
            driver_calls.stream.convert++;
            break;
        case ACMDM_STREAM_RESET:
            driver_calls.stream.reset++;
            return MMSYSERR_NOTSUPPORTED;
        case ACMDM_STREAM_PREPARE:
            driver_calls.stream.prepare++;
            break;
        case ACMDM_STREAM_UNPREPARE:
            driver_calls.stream.unprepare++;
            break;

        default:
            driver_calls.other++;
            return DefDriverProc(id, handle, msg, param1, param2);
    }
    return MMSYSERR_NOERROR;
}

static void test_acmDriverAdd(void)
{
    MMRESULT res;
    HACMDRIVERID drvid;
    union
    {
      ACMDRIVERDETAILSA drv_details;
    } acm;

    /* Driver load steps:
     * - acmDriverAdd checks the passed parameters
     * - DRV_LOAD message is sent - required
     * - DRV_ENABLE message is sent - required
     * - DRV_OPEN message is sent - required
     * - DRV_DETAILS message is sent - required
     * - ACMDM_FORMATTAG_DETAILS message is sent - optional
     * - DRV_QUERYCONFIGURE message is sent - optional
     * - ACMDM_DRIVER_ABOUT message is sent - optional
     */

    res = acmDriverAddA(&drvid, GetModuleHandleA(NULL), (LPARAM)acm_driver_func, 0, ACM_DRIVERADDF_FUNCTION);
    ok(res == MMSYSERR_NOERROR, "Expected 0, got %d\n", res);
todo_wine
    ok(driver_calls.driver.open == 1, "Expected 1, got %d\n", driver_calls.driver.open);
    ok(driver_calls.driver.details == 1, "Expected 1, got %d\n", driver_calls.driver.details);

    memset(&acm, 0, sizeof(acm));
    res = acmDriverDetailsA(drvid, &acm.drv_details, 0);
    ok(res == MMSYSERR_INVALPARAM, "Expected 11, got %d\n", res);

    acm.drv_details.cbStruct = sizeof(acm.drv_details);
    res = acmDriverDetailsA(drvid, &acm.drv_details, 0);
    ok(res == MMSYSERR_NOERROR, "Expected 0, got %d\n", res);
todo_wine
    ok(driver_calls.driver.open == 1, "Expected 1, got %d\n", driver_calls.driver.open);
    ok(driver_calls.driver.details == 2, "Expected 2, got %d\n", driver_calls.driver.details);
todo_wine
    ok(driver_calls.driver.close == 0, "Expected 0, got %d\n", driver_calls.driver.close);
}

START_TEST(msacm)
{
    driver_tests();
    test_prepareheader();
    test_convert();
    test_acmFormatSuggest();
    test_acmFormatTagDetails();
    test_acmFormatChoose();
    test_mp3();
    /* Test acmDriverAdd in the end as it may conflict
     * with other tests due to codec lookup order */
    test_acmDriverAdd();
}
