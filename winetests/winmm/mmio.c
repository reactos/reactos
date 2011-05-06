/*
 * Unit tests for mmio APIs
 *
 * Copyright 2005 Dmitry Timoshkov
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

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "mmsystem.h"
#include "vfw.h"
#include "wine/test.h"

static DWORD RIFF_buf[] =
{
    FOURCC_RIFF, 32*sizeof(DWORD), mmioFOURCC('A','V','I',' '),
    FOURCC_LIST, 29*sizeof(DWORD), listtypeAVIHEADER, ckidAVIMAINHDR,
    sizeof(MainAVIHeader), 0xdeadbeef, 0xdeadbeef, 0xdeadbeef,
    0xdeadbeef, 0xdeadbeef, 0xdeadbeef,0xdeadbeef,
    0xdeadbeef, 0xdeadbeef, 0xdeadbeef,0xdeadbeef,
    0xdeadbeef, 0xdeadbeef, 0xdeadbeef,
    FOURCC_LIST, 10*sizeof(DWORD),listtypeSTREAMHEADER, ckidSTREAMHEADER,
    7*sizeof(DWORD), streamtypeVIDEO, 0xdeadbeef, 0xdeadbeef,
    0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void test_mmioDescend(char *fname)
{
    MMRESULT ret;
    HMMIO hmmio;
    MMIOINFO mmio;
    MMCKINFO ckRiff, ckList, ck;

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = sizeof(RIFF_buf);
    mmio.pchBuffer = (char *)RIFF_buf;
    hmmio = mmioOpen(fname, &mmio, MMIO_READ);
    if (fname && !hmmio)
    {
        skip("%s file is missing, skipping the test\n", fname);
        return;
    }
    ok(hmmio != 0, "mmioOpen error %u\n", mmio.wErrorRet);

    /* first normal RIFF AVI parsing */
    ret = mmioDescend(hmmio, &ckRiff, NULL, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ckRiff.ckid == FOURCC_RIFF, "wrong ckid: %04x\n", ckRiff.ckid);
    ok(ckRiff.fccType == formtypeAVI, "wrong fccType: %04x\n", ckRiff.fccType);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ckRiff.ckid, ckRiff.cksize, (LPCSTR)&ckRiff.fccType,
          ckRiff.dwDataOffset, ckRiff.dwFlags);

    ret = mmioDescend(hmmio, &ckList, &ckRiff, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ckList.ckid == FOURCC_LIST, "wrong ckid: %04x\n", ckList.ckid);
    ok(ckList.fccType == listtypeAVIHEADER, "wrong fccType: %04x\n", ckList.fccType);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ckList.ckid, ckList.cksize, (LPCSTR)&ckList.fccType,
          ckList.dwDataOffset, ckList.dwFlags);

    ret = mmioDescend(hmmio, &ck, &ckList, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == ckidAVIMAINHDR, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == 0, "wrong fccType: %04x\n", ck.fccType);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ck.ckid, ck.cksize, (LPCSTR)&ck.fccType,
          ck.dwDataOffset, ck.dwFlags);

    /* Skip chunk data */
    mmioSeek(hmmio, ck.cksize, SEEK_CUR);

    ret = mmioDescend(hmmio, &ckList, &ckList, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ckList.ckid == FOURCC_LIST, "wrong ckid: %04x\n", ckList.ckid);
    ok(ckList.fccType == listtypeSTREAMHEADER, "wrong fccType: %04x\n", ckList.fccType);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ckList.ckid, ckList.cksize, (LPCSTR)&ckList.fccType,
          ckList.dwDataOffset, ckList.dwFlags);

    ret = mmioDescend(hmmio, &ck, &ckList, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == ckidSTREAMHEADER, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == 0, "wrong fccType: %04x\n", ck.fccType);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ck.ckid, ck.cksize, (LPCSTR)&ck.fccType,
          ck.dwDataOffset, ck.dwFlags);

    /* test various mmioDescend flags */

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDRIFF);
    ok(ret == MMIOERR_CHUNKNOTFOUND ||
       ret == MMIOERR_INVALIDFILE, "mmioDescend returned %u\n", ret);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ck.ckid = 0;
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDRIFF);
    ok(ret == MMIOERR_CHUNKNOTFOUND ||
       ret == MMIOERR_INVALIDFILE, "mmioDescend returned %u\n", ret);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ck.fccType = 0;
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDRIFF);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == FOURCC_RIFF, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == formtypeAVI, "wrong fccType: %04x\n", ck.fccType);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ret = mmioDescend(hmmio, &ck, NULL, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == FOURCC_RIFF, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == formtypeAVI, "wrong fccType: %04x\n", ck.fccType);

    /* do NOT seek, use current file position */
    memset(&ck, 0x66, sizeof(ck));
    ck.fccType = 0;
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDLIST);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == FOURCC_LIST, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == listtypeAVIHEADER, "wrong fccType: %04x\n", ck.fccType);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ck.ckid = 0;
    ck.fccType = listtypeAVIHEADER;
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDCHUNK);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == FOURCC_RIFF, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == formtypeAVI, "wrong fccType: %04x\n", ck.fccType);

    /* do NOT seek, use current file position */
    memset(&ck, 0x66, sizeof(ck));
    ck.ckid = FOURCC_LIST;
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDCHUNK);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == FOURCC_LIST, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == listtypeAVIHEADER, "wrong fccType: %04x\n", ck.fccType);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ck.ckid = FOURCC_RIFF;
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDCHUNK);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == FOURCC_RIFF, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == formtypeAVI, "wrong fccType: %04x\n", ck.fccType);

    /* do NOT seek, use current file position */
    memset(&ckList, 0x66, sizeof(ckList));
    ckList.ckid = 0;
    ret = mmioDescend(hmmio, &ckList, &ck, MMIO_FINDCHUNK);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ckList.ckid == FOURCC_LIST, "wrong ckid: %04x\n", ckList.ckid);
    ok(ckList.fccType == listtypeAVIHEADER, "wrong fccType: %04x\n", ckList.fccType);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDCHUNK);
    ok(ret == MMIOERR_CHUNKNOTFOUND ||
       ret == MMIOERR_INVALIDFILE, "mmioDescend returned %u\n", ret);
    ok(ck.ckid != 0x66666666, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType != 0x66666666, "wrong fccType: %04x\n", ck.fccType);
    ok(ck.dwDataOffset != 0x66666666, "wrong dwDataOffset: %04x\n", ck.dwDataOffset);

    mmioSeek(hmmio, 0, SEEK_SET);
    memset(&ck, 0x66, sizeof(ck));
    ret = mmioDescend(hmmio, &ck, NULL, MMIO_FINDRIFF);
    ok(ret == MMIOERR_CHUNKNOTFOUND ||
       ret == MMIOERR_INVALIDFILE, "mmioDescend returned %u\n", ret);

    mmioClose(hmmio, 0);
}

static void test_mmioOpen(char *fname)
{
    char buf[256];
    MMRESULT ret;
    HMMIO hmmio;
    MMIOINFO mmio;

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = sizeof(buf);
    mmio.pchBuffer = buf;
    hmmio = mmioOpen(fname, &mmio, MMIO_READ);
    if (fname && !hmmio)
    {
        skip("%s file is missing, skipping the test\n", fname);
        return;
    }
    ok(hmmio != 0, "mmioOpen error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == sizeof(buf), "got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer == buf, "expected %p, got %p\n", buf, mmio.pchBuffer);

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = 0;
    mmio.pchBuffer = buf;
    hmmio = mmioOpen(fname, &mmio, MMIO_READ);
    ok(hmmio != 0, "mmioOpen error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == 0, "expected 0, got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer == buf, "expected %p, got %p\n", buf, mmio.pchBuffer);

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = 0;
    mmio.pchBuffer = NULL;
    hmmio = mmioOpen(fname, &mmio, MMIO_READ);
    ok(hmmio != 0, "mmioOpen error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == 0, "expected 0, got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer == NULL, "expected NULL\n");

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = 256;
    mmio.pchBuffer = NULL;
    hmmio = mmioOpen(fname, &mmio, MMIO_READ);
    ok(hmmio != 0, "mmioOpen error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == (MMIO_READ|MMIO_ALLOCBUF), "expected MMIO_READ|MMIO_ALLOCBUF, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == 256, "expected 256, got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer != NULL, "expected not NULL\n");

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = sizeof(buf);
    mmio.pchBuffer = buf;
    hmmio = mmioOpen(fname, &mmio, MMIO_READ | MMIO_ALLOCBUF);
    ok(hmmio != 0, "mmioOpen error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == sizeof(buf), "got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer == buf, "expected %p, got %p\n", buf, mmio.pchBuffer);

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = 0;
    mmio.pchBuffer = NULL;
    hmmio = mmioOpen(fname, &mmio, MMIO_READ | MMIO_ALLOCBUF);
    ok(hmmio != 0, "mmioOpen error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == (MMIO_READ|MMIO_ALLOCBUF), "expected MMIO_READ|MMIO_ALLOCBUF, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == MMIO_DEFAULTBUFFER, "expected MMIO_DEFAULTBUFFER, got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer != NULL, "expected not NULL\n");

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = 256;
    mmio.pchBuffer = NULL;
    hmmio = mmioOpen(fname, &mmio, MMIO_READ | MMIO_ALLOCBUF);
    ok(hmmio != 0, "mmioOpen error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == (MMIO_READ|MMIO_ALLOCBUF), "expected MMIO_READ|MMIO_ALLOCBUF, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == 256, "expected 256, got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer != NULL, "expected not NULL\n");

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = 0;
    mmio.pchBuffer = buf;
    hmmio = mmioOpen(fname, &mmio, MMIO_READ | MMIO_ALLOCBUF);
    if (!hmmio && mmio.wErrorRet == ERROR_BAD_FORMAT)
    {
        /* Seen on Win9x, WinMe but also XP-SP1 */
        skip("Some Windows versions don't like a 0 size and a given buffer\n");
        return;
    }
    ok(hmmio != 0, "mmioOpen error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == MMIO_DEFAULTBUFFER, "expected MMIO_DEFAULTBUFFER, got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer == buf, "expected %p, got %p\n", buf, mmio.pchBuffer);

    mmioClose(hmmio, 0);
}

static void test_mmioSetBuffer(char *fname)
{
    char buf[256];
    MMRESULT ret;
    HMMIO hmmio;
    MMIOINFO mmio;

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = sizeof(buf);
    mmio.pchBuffer = buf;
    hmmio = mmioOpen(fname, &mmio, MMIO_READ);
    if (fname && !hmmio)
    {
        skip("%s file is missing, skipping the test\n", fname);
        return;
    }
    ok(hmmio != 0, "mmioOpen error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == sizeof(buf), "got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer == buf, "expected %p, got %p\n", buf, mmio.pchBuffer);

    ret = mmioSetBuffer(hmmio, NULL, 0, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioSetBuffer error %u\n", ret);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == 0, "got not 0\n");
    ok(mmio.pchBuffer == NULL, "got not NULL buf\n");

    ret = mmioSetBuffer(hmmio, NULL, 0, MMIO_ALLOCBUF);
    ok(ret == MMSYSERR_NOERROR, "mmioSetBuffer error %u\n", ret);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == 0, "got not 0\n");
    ok(mmio.pchBuffer == NULL, "got not NULL buf\n");

    ret = mmioSetBuffer(hmmio, buf, 0, MMIO_ALLOCBUF);
    ok(ret == MMSYSERR_NOERROR, "mmioSetBuffer error %u\n", ret);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == 0, "got not 0\n");
    ok(mmio.pchBuffer == buf, "expected %p, got %p\n", buf, mmio.pchBuffer);

    ret = mmioSetBuffer(hmmio, NULL, 256, MMIO_WRITE|MMIO_ALLOCBUF);
    ok(ret == MMSYSERR_NOERROR, "mmioSetBuffer error %u\n", ret);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == (MMIO_READ|MMIO_ALLOCBUF), "expected MMIO_READ|MMIO_ALLOCBUF, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == 256, "got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer != NULL, "expected not NULL\n");
    ok(mmio.pchBuffer != buf, "expected != buf\n");

    mmioClose(hmmio, 0);
}

START_TEST(mmio)
{
    char fname[] = "msrle.avi";

    test_mmioDescend(NULL);
    test_mmioDescend(fname);
    test_mmioOpen(NULL);
    test_mmioOpen(fname);
    test_mmioSetBuffer(NULL);
    test_mmioSetBuffer(fname);
}
