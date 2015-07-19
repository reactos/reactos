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

static BOOL CALLBACK FormatTagEnumProc(HACMDRIVERID hadid,
                                       PACMFORMATTAGDETAILSA paftd,
                                       DWORD_PTR dwInstance,
                                       DWORD fdwSupport)
{
    if (winetest_interactive)
        trace("   Format 0x%04x: %s\n", paftd->dwFormatTag, paftd->szFormatTag);

    return TRUE;
}

static BOOL CALLBACK FormatEnumProc(HACMDRIVERID hadid,
                                    LPACMFORMATDETAILSA pafd,
                                    DWORD_PTR dwInstance,
                                    DWORD fd)
{
    if (winetest_interactive)
        trace("   0x%04x, %s\n", pafd->dwFormatTag, pafd->szFormat);

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
        struct {
            const char *shortname;
            const WORD mid;
            const WORD pid;
        } *iter, expected_ids[] = {
            { "Microsoft IMA ADPCM", MM_MICROSOFT, MM_MSFT_ACM_IMAADPCM },
            { "MS-ADPCM", MM_MICROSOFT, MM_MSFT_ACM_MSADPCM },
            { "Microsoft CCITT G.711", MM_MICROSOFT, MM_MSFT_ACM_G711},
            { "MPEG Layer-3 Codec", MM_FRAUNHOFER_IIS, MM_FHGIIS_MPEGLAYER3_DECODE },
            { "MS-PCM", MM_MICROSOFT, MM_MSFT_ACM_PCM },
            { 0 }
        };

        ok(dd.cbStruct == sizeof(dd),
            "acmDriverDetailsA(): cbStruct = %08x\n", dd.cbStruct);

        for (iter = expected_ids; iter->shortname; ++iter) {
            if (dd.szShortName && !strcmp(iter->shortname, dd.szShortName)) {
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

            if (dwSize < sizeof(WAVEFORMATEX))
                dwSize = sizeof(WAVEFORMATEX);

            pwfx = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);

            pwfx->cbSize = LOWORD(dwSize) - sizeof(WAVEFORMATEX);
            pwfx->wFormatTag = WAVE_FORMAT_UNKNOWN;

            fd.cbStruct = sizeof(fd);
            fd.pwfx = pwfx;
            fd.cbwfx = dwSize;
            fd.dwFormatTag = WAVE_FORMAT_UNKNOWN;

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
    BYTE buf[sizeof(WAVEFORMATEX) + 32], pcm[512], input[512];
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
    hdr.cbSrcLength = sizeof(input);
    hdr.pbDst = pcm;
    hdr.cbDstLength = sizeof(pcm);
    hdr.fdwStatus = ACMSTREAMHEADER_STATUSF_DONE;

    mr = acmStreamPrepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_NOERROR, "prepare failed: 0x%x\n", mr);
    ok(hdr.fdwStatus == (ACMSTREAMHEADER_STATUSF_PREPARED | ACMSTREAMHEADER_STATUSF_DONE), "header wasn't prepared: 0x%x\n", hdr.fdwStatus);

    mr = acmStreamUnprepareHeader(has, &hdr, 0);
    ok(mr == MMSYSERR_NOERROR, "unprepare failed: 0x%x\n", mr);
    ok(hdr.fdwStatus == ACMSTREAMHEADER_STATUSF_DONE, "header wasn't unprepared: 0x%x\n", hdr.fdwStatus);

    mr = acmStreamClose(has, 0);
    ok(mr == MMSYSERR_NOERROR, "close failed: 0x%x\n", mr);
}

START_TEST(msacm)
{
    driver_tests();
    test_prepareheader();
}
