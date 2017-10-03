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

static void expect_buf_offset_dbg(HMMIO hmmio, LONG off, int line)
{
    MMIOINFO mmio;
    LONG ret;

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok_(__FILE__, line)(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok_(__FILE__, line)(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok_(__FILE__, line)(ret == off, "expected %d, got %d\n", off, ret);
}

#define expect_buf_offset(a1, a2) expect_buf_offset_dbg(a1, a2, __LINE__)

static void test_mmioDescend(char *fname)
{
    MMRESULT ret;
    HMMIO hmmio;
    MMIOINFO mmio;
    MMCKINFO ckRiff, ckList, ck, ckList2;

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = sizeof(RIFF_buf);
    mmio.pchBuffer = (char *)RIFF_buf;
    hmmio = mmioOpenA(fname, &mmio, MMIO_READ);
    if (fname && !hmmio)
    {
        trace("No optional %s file. Skipping the test\n", fname);
        return;
    }
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);

    expect_buf_offset(hmmio, 0);

    /* first normal RIFF AVI parsing */
    ret = mmioDescend(hmmio, &ckRiff, NULL, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ckRiff.ckid == FOURCC_RIFF, "wrong ckid: %04x\n", ckRiff.ckid);
    ok(ckRiff.fccType == formtypeAVI, "wrong fccType: %04x\n", ckRiff.fccType);
    ok(ckRiff.dwDataOffset == 8, "expected 8 got %u\n", ckRiff.dwDataOffset);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ckRiff.ckid, ckRiff.cksize, (LPCSTR)&ckRiff.fccType,
          ckRiff.dwDataOffset, ckRiff.dwFlags);

    expect_buf_offset(hmmio, 12);

    ret = mmioDescend(hmmio, &ckList, &ckRiff, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ckList.ckid == FOURCC_LIST, "wrong ckid: %04x\n", ckList.ckid);
    ok(ckList.fccType == listtypeAVIHEADER, "wrong fccType: %04x\n", ckList.fccType);
    ok(ckList.dwDataOffset == 20, "expected 20 got %u\n", ckList.dwDataOffset);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ckList.ckid, ckList.cksize, (LPCSTR)&ckList.fccType,
          ckList.dwDataOffset, ckList.dwFlags);

    expect_buf_offset(hmmio, 24);

    ret = mmioDescend(hmmio, &ck, &ckList, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == ckidAVIMAINHDR, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == 0, "wrong fccType: %04x\n", ck.fccType);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ck.ckid, ck.cksize, (LPCSTR)&ck.fccType,
          ck.dwDataOffset, ck.dwFlags);

    expect_buf_offset(hmmio, 32);

    /* Skip chunk data */
    ret = mmioSeek(hmmio, ck.cksize, SEEK_CUR);
    ok(ret == 0x58, "expected 0x58, got %#x\n", ret);

    ret = mmioDescend(hmmio, &ckList2, &ckList, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ckList2.ckid == FOURCC_LIST, "wrong ckid: %04x\n", ckList2.ckid);
    ok(ckList2.fccType == listtypeSTREAMHEADER, "wrong fccType: %04x\n", ckList2.fccType);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ckList2.ckid, ckList2.cksize, (LPCSTR)&ckList2.fccType,
          ckList2.dwDataOffset, ckList2.dwFlags);

    expect_buf_offset(hmmio, 100);

    ret = mmioDescend(hmmio, &ck, &ckList2, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioDescend error %u\n", ret);
    ok(ck.ckid == ckidSTREAMHEADER, "wrong ckid: %04x\n", ck.ckid);
    ok(ck.fccType == 0, "wrong fccType: %04x\n", ck.fccType);
    trace("ckid %4.4s cksize %04x fccType %4.4s off %04x flags %04x\n",
          (LPCSTR)&ck.ckid, ck.cksize, (LPCSTR)&ck.fccType,
          ck.dwDataOffset, ck.dwFlags);

    expect_buf_offset(hmmio, 108);

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
    char buf[MMIO_DEFAULTBUFFER];
    MMRESULT ret;
    HMMIO hmmio;
    MMIOINFO mmio;

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = sizeof(buf);
    mmio.pchBuffer = buf;
    hmmio = mmioOpenA(fname, &mmio, MMIO_READ);
    if (fname && !hmmio)
    {
        trace("No optional %s file. Skipping the test\n", fname);
        return;
    }
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == sizeof(buf), "got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer == buf, "expected %p, got %p\n", buf, mmio.pchBuffer);
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    if (mmio.fccIOProc == FOURCC_DOS)
        ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
    else
        ok(mmio.pchEndRead == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndRead);
    ok(mmio.pchEndWrite == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndWrite);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = 0;
    mmio.pchBuffer = buf;
    hmmio = mmioOpenA(fname, &mmio, MMIO_READ);
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == 0, "expected 0, got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer == buf, "expected %p, got %p\n", buf, mmio.pchBuffer);
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
    ok(mmio.pchEndWrite == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndWrite);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = 0;
    mmio.pchBuffer = NULL;
    hmmio = mmioOpenA(fname, &mmio, MMIO_READ);
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == 0, "expected 0, got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer == NULL, "expected NULL\n");
    ok(mmio.pchNext == NULL, "expected NULL\n");
    ok(mmio.pchEndRead == NULL, "expected NULL\n");
    ok(mmio.pchEndWrite == NULL, "expected NULL\n");
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = 256;
    mmio.pchBuffer = NULL;
    hmmio = mmioOpenA(fname, &mmio, MMIO_READ);
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == (MMIO_READ|MMIO_ALLOCBUF), "expected MMIO_READ|MMIO_ALLOCBUF, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == 256, "expected 256, got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer != NULL, "expected not NULL\n");
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    if (mmio.fccIOProc == FOURCC_DOS)
        ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
    else
        ok(mmio.pchEndRead == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndRead);
    ok(mmio.pchEndWrite == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndWrite);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = sizeof(buf);
    mmio.pchBuffer = buf;
    hmmio = mmioOpenA(fname, &mmio, MMIO_READ | MMIO_ALLOCBUF);
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == sizeof(buf), "got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer == buf, "expected %p, got %p\n", buf, mmio.pchBuffer);
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    if (mmio.fccIOProc == FOURCC_DOS)
        ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
    else
        ok(mmio.pchEndRead == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndRead);
    ok(mmio.pchEndWrite == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndWrite);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = 0;
    mmio.pchBuffer = NULL;
    hmmio = mmioOpenA(fname, &mmio, MMIO_READ | MMIO_ALLOCBUF);
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == (MMIO_READ|MMIO_ALLOCBUF), "expected MMIO_READ|MMIO_ALLOCBUF, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == MMIO_DEFAULTBUFFER, "expected MMIO_DEFAULTBUFFER, got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer != NULL, "expected not NULL\n");
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    if (mmio.fccIOProc == FOURCC_DOS)
        ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
    else
        ok(mmio.pchEndRead == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndRead);
    ok(mmio.pchEndWrite == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndWrite);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = 256;
    mmio.pchBuffer = NULL;
    hmmio = mmioOpenA(fname, &mmio, MMIO_READ | MMIO_ALLOCBUF);
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == (MMIO_READ|MMIO_ALLOCBUF), "expected MMIO_READ|MMIO_ALLOCBUF, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == 256, "expected 256, got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer != NULL, "expected not NULL\n");
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    if (mmio.fccIOProc == FOURCC_DOS)
        ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
    else
        ok(mmio.pchEndRead == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndRead);
    ok(mmio.pchEndWrite == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndWrite);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

    mmioClose(hmmio, 0);

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = fname ? FOURCC_DOS : FOURCC_MEM;
    mmio.cchBuffer = 0;
    mmio.pchBuffer = buf;
    hmmio = mmioOpenA(fname, &mmio, MMIO_READ | MMIO_ALLOCBUF);
    if (!hmmio && mmio.wErrorRet == ERROR_BAD_FORMAT)
    {
        /* Seen on Win9x, WinMe but also XP-SP1 */
        skip("Some Windows versions don't like a 0 size and a given buffer\n");
        return;
    }
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == MMIO_DEFAULTBUFFER, "expected MMIO_DEFAULTBUFFER, got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer == buf, "expected %p, got %p\n", buf, mmio.pchBuffer);
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    if (mmio.fccIOProc == FOURCC_DOS)
        ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
    else
        ok(mmio.pchEndRead == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndRead);
    ok(mmio.pchEndWrite == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndWrite);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

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
    hmmio = mmioOpenA(fname, &mmio, MMIO_READ);
    if (fname && !hmmio)
    {
        trace("No optional %s file. Skipping the test\n", fname);
        return;
    }
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);

    memset(&mmio, 0, sizeof(mmio));
    ret = mmioGetInfo(hmmio, &mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo error %u\n", ret);
    ok(mmio.dwFlags == MMIO_READ, "expected MMIO_READ, got %x\n", mmio.dwFlags);
    ok(mmio.wErrorRet == MMSYSERR_NOERROR, "expected MMSYSERR_NOERROR, got %u\n", mmio.wErrorRet);
    ok(mmio.fccIOProc == (fname ? FOURCC_DOS : FOURCC_MEM), "got %4.4s\n", (LPCSTR)&mmio.fccIOProc);
    ok(mmio.cchBuffer == sizeof(buf), "got %u\n", mmio.cchBuffer);
    ok(mmio.pchBuffer == buf, "expected %p, got %p\n", buf, mmio.pchBuffer);
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    if (mmio.fccIOProc == FOURCC_DOS)
        ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
    else
        ok(mmio.pchEndRead == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndRead);
    ok(mmio.pchEndWrite == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndWrite);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

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
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
    ok(mmio.pchEndWrite == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndWrite);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

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
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
    ok(mmio.pchEndWrite == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndWrite);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

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
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
    ok(mmio.pchEndWrite == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndWrite);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

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
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", buf, mmio.pchEndRead);
    ok(mmio.pchEndWrite == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndWrite);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.lDiskOffset == 0, "expected 0, got %d\n", mmio.lDiskOffset);

    ret = mmioSeek(hmmio, 0, SEEK_CUR);
    ok(ret == 0, "expected 0, got %d\n", ret);

    mmioClose(hmmio, 0);
}

#define FOURCC_XYZ mmioFOURCC('X', 'Y', 'Z', ' ')

static LRESULT CALLBACK mmio_test_IOProc(LPSTR lpMMIOInfo, UINT uMessage, LPARAM lParam1, LPARAM lParam2)
{
    LPMMIOINFO lpInfo = (LPMMIOINFO) lpMMIOInfo;
    int i;

    switch (uMessage)
    {
    case MMIOM_OPEN:
        if (lpInfo->fccIOProc == FOURCC_DOS)
            lpInfo->fccIOProc = mmioFOURCC('F', 'A', 'I', 'L');
        for (i = 0; i < sizeof(lpInfo->adwInfo) / sizeof(*lpInfo->adwInfo); i++)
            ok(lpInfo->adwInfo[i] == 0, "[%d] Expected 0, got %u\n", i, lpInfo->adwInfo[i]);
        return MMSYSERR_NOERROR;
    case MMIOM_CLOSE:
        return MMSYSERR_NOERROR;
    case MMIOM_SEEK:
        lpInfo->adwInfo[1]++;
        lpInfo->lDiskOffset = 0xdeadbeef;
        return 0;
    default:
        return 0;
    }
}

static void test_mmioOpen_fourcc(void)
{
    char fname[] = "file+name.xyz+one.two";

    LPMMIOPROC lpProc;
    HMMIO hmmio;
    MMIOINFO mmio;

    lpProc = mmioInstallIOProcA(FOURCC_DOS, mmio_test_IOProc, MMIO_INSTALLPROC);
    ok(lpProc == mmio_test_IOProc, "mmioInstallIOProcA error\n");

    lpProc = mmioInstallIOProcA(FOURCC_XYZ, mmio_test_IOProc, MMIO_INSTALLPROC);
    ok(lpProc == mmio_test_IOProc, "mmioInstallIOProcA error\n");

    memset(&mmio, 0, sizeof(mmio));
    hmmio = mmioOpenA(fname, &mmio, MMIO_READ);
    mmioGetInfo(hmmio, &mmio, 0);
    ok(hmmio && mmio.fccIOProc == FOURCC_XYZ, "mmioOpenA error %u, got %4.4s\n",
            mmio.wErrorRet, (LPCSTR)&mmio.fccIOProc);
    ok(mmio.adwInfo[1] == 0, "mmioOpenA sent MMIOM_SEEK, got %d\n",
       mmio.adwInfo[1]);
    ok(mmio.lDiskOffset == 0, "mmioOpenA updated lDiskOffset, got %d\n",
       mmio.lDiskOffset);
    mmioClose(hmmio, 0);

    /* Same test with NULL info */
    memset(&mmio, 0, sizeof(mmio));
    hmmio = mmioOpenA(fname, NULL, MMIO_READ);
    mmioGetInfo(hmmio, &mmio, 0);
    ok(hmmio && mmio.fccIOProc == FOURCC_XYZ, "mmioOpenA error %u, got %4.4s\n",
            mmio.wErrorRet, (LPCSTR)&mmio.fccIOProc);
    ok(mmio.adwInfo[1] == 0, "mmioOpenA sent MMIOM_SEEK, got %d\n",
       mmio.adwInfo[1]);
    ok(mmio.lDiskOffset == 0, "mmioOpenA updated lDiskOffset, got %d\n",
       mmio.lDiskOffset);
    mmioClose(hmmio, 0);

    mmioInstallIOProcA(FOURCC_XYZ, NULL, MMIO_REMOVEPROC);

    memset(&mmio, 0, sizeof(mmio));
    hmmio = mmioOpenA(fname, &mmio, MMIO_READ);
    mmioGetInfo(hmmio, &mmio, 0);
    ok(!hmmio && mmio.wErrorRet == MMIOERR_FILENOTFOUND, "mmioOpenA error %u, got %4.4s\n",
            mmio.wErrorRet, (LPCSTR)&mmio.fccIOProc);
    mmioClose(hmmio, 0);

    mmioInstallIOProcA(FOURCC_DOS, NULL, MMIO_REMOVEPROC);
}

static BOOL create_test_file(char *temp_file)
{
    char temp_path[MAX_PATH];
    DWORD ret, written;
    HANDLE h;

    ret = GetTempPathA(sizeof(temp_path), temp_path);
    ok(ret, "Failed to get a temp path, err %d\n", GetLastError());
    if (!ret)
        return FALSE;

    ret = GetTempFileNameA(temp_path, "mmio", 0, temp_file);
    ok(ret, "Failed to get a temp name, err %d\n", GetLastError());
    if (!ret)
        return FALSE;

    h = CreateFileA(temp_file, GENERIC_WRITE, 0, NULL,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(h != INVALID_HANDLE_VALUE, "Failed to create a file, err %d\n", GetLastError());
    if (h == INVALID_HANDLE_VALUE) return FALSE;

    ret = WriteFile(h, RIFF_buf, sizeof(RIFF_buf), &written, NULL);
    ok(ret, "Failed to write a file, err %d\n", GetLastError());
    CloseHandle(h);
    if (!ret) DeleteFileA(temp_file);
    return ret;
}

static void test_mmioSeek(void)
{
    HMMIO hmmio;
    MMIOINFO mmio;
    LONG end, pos;
    const LONG size = sizeof(RIFF_buf), offset = 16;
    char test_file[MAX_PATH];
    MMRESULT res;
    HFILE hfile;
    OFSTRUCT ofs;

    /* test memory file */
    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = FOURCC_MEM;
    mmio.pchBuffer = (char*)&RIFF_buf;
    mmio.cchBuffer = sizeof(RIFF_buf);
    hmmio = mmioOpenA(NULL, &mmio, MMIO_READ);
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);
    if (hmmio != NULL) {
        /* seek to the end */
        end = mmioSeek(hmmio, 0, SEEK_END);
        ok(end == size, "expected %d, got %d\n", size, end);

        /* test MMIOINFO values */
        res = mmioGetInfo(hmmio, &mmio, 0);
        ok(res == MMSYSERR_NOERROR, "expected 0, got %d\n", res);
        ok(mmio.pchNext == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchNext);
        ok(mmio.pchEndRead == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndRead);
        ok(mmio.pchEndWrite == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndWrite);
        ok(mmio.lBufOffset == 0, "expected %d, got %d\n", 0, mmio.lBufOffset);
        ok(mmio.lDiskOffset == 0, "expected %d, got %d\n", 0, mmio.lDiskOffset);

        /* seek backward from the end */
        pos = mmioSeek(hmmio, offset, SEEK_END);
        ok(pos == size-offset, "expected %d, got %d\n", size-offset, pos);

        mmioClose(hmmio, 0);
    }

    if (!create_test_file(test_file)) return;

    /* test standard file without buffering */
    hmmio = NULL;
    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = FOURCC_DOS;
    mmio.pchBuffer = 0;
    mmio.cchBuffer = 0;
    hmmio = mmioOpenA(test_file, &mmio, MMIO_READ);
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);
    if (hmmio != NULL) {
        /* seek to the end */
        end = mmioSeek(hmmio, 0, SEEK_END);
        ok(end == size, "expected %d, got %d\n", size, end);

        /* test MMIOINFO values */
        res = mmioGetInfo(hmmio, &mmio, 0);
        ok(res == MMSYSERR_NOERROR, "expected 0, got %d\n", res);
        ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
        ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
        ok(mmio.pchEndWrite == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndWrite);
        ok(mmio.lBufOffset == size, "expected %d, got %d\n", size, mmio.lBufOffset);
        ok(mmio.lDiskOffset == size, "expected %d, got %d\n", size, mmio.lDiskOffset);

        /* seek backward from the end */
        pos = mmioSeek(hmmio, offset, SEEK_END);
        ok(pos == size-offset, "expected %d, got %d\n", size-offset, pos);

        mmioClose(hmmio, 0);
    }

    /* test standard file with buffering */
    hmmio = NULL;
    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = FOURCC_DOS;
    mmio.pchBuffer = 0;
    mmio.cchBuffer = 0;
    hmmio = mmioOpenA(test_file, &mmio, MMIO_READ | MMIO_ALLOCBUF);
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);
    if (hmmio != NULL) {
        /* seek to the end */
        end = mmioSeek(hmmio, 0, SEEK_END);
        ok(end == size, "expected %d, got %d\n", size, end);

        /* test MMIOINFO values */
        res = mmioGetInfo(hmmio, &mmio, 0);
        ok(res == MMSYSERR_NOERROR, "expected 0, got %d\n", res);
        ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
        ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);
        ok(mmio.pchEndWrite == mmio.pchBuffer + mmio.cchBuffer, "expected %p + %d, got %p\n", mmio.pchBuffer, mmio.cchBuffer, mmio.pchEndWrite);
        ok(mmio.lBufOffset == end, "expected %d, got %d\n", end, mmio.lBufOffset);
        ok(mmio.lDiskOffset == size, "expected %d, got %d\n", size, mmio.lDiskOffset);

        /* seek backward from the end */
        pos = mmioSeek(hmmio, offset, SEEK_END);
        ok(pos == size-offset, "expected %d, got %d\n", size-offset, pos);

        mmioClose(hmmio, 0);
    }

    /* test seek position inheritance from standard file handle */
    hfile = OpenFile(test_file, &ofs, OF_READ);
    ok(hfile != HFILE_ERROR, "Failed to open the file, err %d\n", GetLastError());
    if (hfile != HFILE_ERROR) {
        pos = _llseek(hfile, offset, SEEK_SET);
        ok(pos != HFILE_ERROR, "Failed to seek, err %d\n", GetLastError());
        memset(&mmio, 0, sizeof(mmio));
        mmio.fccIOProc = FOURCC_DOS;
        mmio.adwInfo[0] = (DWORD)hfile;
        hmmio = mmioOpenA(NULL, &mmio, MMIO_READ | MMIO_DENYWRITE | MMIO_ALLOCBUF);
        ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);
        if (hmmio != NULL) {
            pos = mmioSeek(hmmio, 0, SEEK_CUR);
            ok(pos == offset, "expected %d, got %d\n", offset, pos);
            mmioClose(hmmio, 0);
        }
    }

    DeleteFileA(test_file);
}

static void test_mmio_end_of_file(void)
{
    char test_file[MAX_PATH], buffer[128], data[16];
    MMIOINFO mmio;
    HMMIO hmmio;
    LONG ret;
    MMRESULT res;

    if (!create_test_file(test_file)) return;

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = FOURCC_DOS;
    mmio.pchBuffer = buffer;
    mmio.cchBuffer = sizeof(buffer);
    hmmio = mmioOpenA(test_file, &mmio, MMIO_READ);
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);
    if (hmmio == NULL) {
        DeleteFileA(test_file);
        return;
    }

    ret = mmioSeek(hmmio, 0, SEEK_END);
    ok(sizeof(RIFF_buf) == ret, "got %d\n", ret);

    ret = mmioRead(hmmio, data, sizeof(data));
    ok(ret == 0, "expected %d, got %d\n", 0, ret);

    res = mmioGetInfo(hmmio, &mmio, 0);
    ok(res == MMSYSERR_NOERROR, "expected 0, got %d\n", res);

    res = mmioAdvance(hmmio, &mmio, MMIO_READ);
    ok(res == MMSYSERR_NOERROR, "expected 0, got %d\n", res);
    ok(mmio.pchNext == mmio.pchEndRead, "expected %p, got %p\n", mmio.pchEndRead, mmio.pchNext);

    mmioClose(hmmio, 0);
    DeleteFileA(test_file);
}

static void test_mmio_buffer_pointer(void)
{
    char test_file[MAX_PATH];
    char buffer[5], data[16];
    MMIOINFO mmio;
    HMMIO hmmio;
    LONG size, pos;
    MMRESULT res;

    if (!create_test_file(test_file)) return;

    memset(&mmio, 0, sizeof(mmio));
    mmio.fccIOProc = FOURCC_DOS;
    mmio.pchBuffer = buffer;
    mmio.cchBuffer = sizeof(buffer);
    hmmio = mmioOpenA(test_file, &mmio, MMIO_READ);
    ok(hmmio != 0, "mmioOpenA error %u\n", mmio.wErrorRet);
    if (hmmio == NULL) {
        DeleteFileA(test_file);
        return;
    }

    /* the buffer is empty */
    size = mmioRead(hmmio, data, 0);
    ok(size == 0, "expected 0, got %d\n", size);
    res = mmioGetInfo(hmmio, &mmio, 0);
    ok(res == MMSYSERR_NOERROR, "expected 0, got %d\n", res);
    ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);

    /* fill the buffer */
    res = mmioAdvance(hmmio, &mmio, MMIO_READ);
    ok(res == MMSYSERR_NOERROR, "mmioAdvance failed %x\n", res);
    ok(mmio.pchEndRead-mmio.pchBuffer == sizeof(buffer), "got %d\n", (int)(mmio.pchEndRead-mmio.pchBuffer));

    /* seeking to the same buffer chunk, the buffer is kept */
    size = sizeof(buffer)/2;
    pos = mmioSeek(hmmio, size, SEEK_SET);
    ok(pos == size, "failed to seek, expected %d, got %d\n", size, pos);
    res = mmioGetInfo(hmmio, &mmio, 0);
    ok(res == MMSYSERR_NOERROR, "expected 0, got %d\n", res);
    ok(mmio.lBufOffset == 0, "expected 0, got %d\n", mmio.lBufOffset);
    ok(mmio.pchNext-mmio.pchBuffer == size, "expected %d, got %d\n", size, (int)(mmio.pchNext-mmio.pchBuffer));
    ok(mmio.pchEndRead-mmio.pchBuffer == sizeof(buffer), "got %d\n", (int)(mmio.pchEndRead-mmio.pchBuffer));

    /* seeking to another buffer chunk, the buffer is empty */
    size = sizeof(buffer) * 3 + sizeof(buffer) / 2;
    pos = mmioSeek(hmmio, size, SEEK_SET);
    ok(pos == size, "failed to seek, got %d\n", pos);
    res = mmioGetInfo(hmmio, &mmio, 0);
    ok(res == MMSYSERR_NOERROR, "expected 0, got %d\n", res);
    ok(mmio.lBufOffset == size, "expected %d, got %d\n", size, mmio.lBufOffset);
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);

    /* reading a lot (as sizeof(data) > mmio.cchBuffer), the buffer is empty */
    size = mmioRead(hmmio, data, sizeof(data));
    ok(size == sizeof(data), "failed to read, got %d\n", size);
    res = mmioGetInfo(hmmio, &mmio, 0);
    ok(res == MMSYSERR_NOERROR, "expected 0, got %d\n", res);
    ok(mmio.lBufOffset == pos+size, "expected %d, got %d\n", pos+size, mmio.lBufOffset);
    ok(mmio.pchNext == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchNext);
    ok(mmio.pchEndRead == mmio.pchBuffer, "expected %p, got %p\n", mmio.pchBuffer, mmio.pchEndRead);

    mmioClose(hmmio, 0);
    DeleteFileA(test_file);
}

static void test_riff_write(void)
{
    static const DWORD test_write_data[] =
    {
        FOURCC_RIFF, 0x28, mmioFOURCC('W','A','V','E'),  mmioFOURCC('d','a','t','a'),
        0x1b, 0xdededede, 0xdededede, 0xefefefef,
        0xefefefef, 0xbabababa, 0xbabababa, 0xefefef
    };

    char name[] = "test_write.wav";
    char buf[256];
    MMCKINFO chunk_info[2];
    MMIOINFO info;
    HMMIO mmio;
    MMRESULT ret;
    LONG written;
    DWORD read;
    HANDLE h;

    memset(chunk_info, 0, sizeof(chunk_info));

    mmio = mmioOpenA(name, NULL, MMIO_ALLOCBUF|MMIO_CREATE|MMIO_READWRITE);
    ok(mmio != NULL, "mmioOpen failed\n");

    chunk_info[0].fccType = mmioFOURCC('W','A','V','E');
    ret = mmioCreateChunk(mmio, chunk_info, MMIO_CREATERIFF);
    ok(ret == MMSYSERR_NOERROR, "mmioCreateChunk failed %x\n", ret);
    ok(chunk_info[0].ckid == FOURCC_RIFF, "chunk_info[0].ckid = %x\n", chunk_info[0].ckid);
    ok(chunk_info[0].cksize == 0, "chunk_info[0].cksize = %d\n", chunk_info[0].cksize);
    ok(chunk_info[0].dwDataOffset == 8, "chunk_info[0].dwDataOffset = %d\n", chunk_info[0].dwDataOffset);
    ok(chunk_info[0].dwFlags == MMIO_DIRTY, "chunk_info[0].dwFlags = %x\n", chunk_info[0].dwFlags);

    chunk_info[1].ckid = mmioFOURCC('d','a','t','a');
    ret = mmioCreateChunk(mmio, chunk_info+1, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioCreateChunk failed %x\n", ret);
    ok(chunk_info[1].ckid == mmioFOURCC('d','a','t','a'), "chunk_info[1].ckid = %x\n", chunk_info[1].ckid);
    ok(chunk_info[1].cksize == 0, "chunk_info[1].cksize = %d\n", chunk_info[1].cksize);
    ok(chunk_info[1].dwDataOffset == 20, "chunk_info[1].dwDataOffset = %d\n", chunk_info[1].dwDataOffset);
    ok(chunk_info[1].dwFlags == MMIO_DIRTY, "chunk_info[1].dwFlags = %x\n", chunk_info[1].dwFlags);

    memset(buf, 0xde, sizeof(buf));
    written = mmioWrite(mmio, buf, 8);
    ok(written == 8, "mmioWrite failed %x\n", ret);

    ret = mmioGetInfo(mmio, &info, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioGetInfo failed %x\n", ret);

    memset(info.pchNext, 0xef, 8);
    info.pchNext += 8;
    ret = mmioAdvance(mmio, &info, 1);
    ok(ret == MMSYSERR_NOERROR, "mmioAdvance failed %x\n", ret);
    ok(info.lBufOffset == 36, "info.lBufOffset = %d\n", info.lBufOffset);

    info.dwFlags |= MMIO_DIRTY;
    memset(info.pchNext, 0xba, 8);
    info.pchNext += 8;
    ret = mmioAdvance(mmio, &info, 1);
    ok(ret == MMSYSERR_NOERROR, "mmioAdvance failed %x\n", ret);
    ok(info.lBufOffset == 44, "info.lBufOffset = %d\n", info.lBufOffset);

    info.dwFlags |= MMIO_DIRTY;
    memset(info.pchNext, 0xef, 3);
    info.pchNext += 3;
    ret = mmioSetInfo(mmio, &info, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioSetInfo failed %x\n", ret);

    ret = mmioAscend(mmio, chunk_info+1, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioAscend failed %x\n", ret);
    ret = mmioAscend(mmio, chunk_info, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioAscend failed %x\n", ret);
    ret = mmioClose(mmio, 0);
    ok(ret == MMSYSERR_NOERROR, "mmioClose failed %x\n", ret);

    h = CreateFileA("test_write.wav", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(h != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    ok(ReadFile(h, buf, sizeof(buf), &read, NULL), "ReadFile failed\n");
    CloseHandle(h);
    ok(!memcmp(buf, test_write_data, sizeof(test_write_data)), "created file is incorrect\n");

    DeleteFileA("test_write.wav");
}

START_TEST(mmio)
{
    /* Make it possible to run the tests against a specific AVI file in
     * addition to the builtin test data. This is mostly meant as a
     * debugging aid and is not part of the standard tests.
     */
    char fname[] = "msrle.avi";

    test_mmioDescend(NULL);
    test_mmioDescend(fname);
    test_mmioOpen(NULL);
    test_mmioOpen(fname);
    test_mmioSetBuffer(NULL);
    test_mmioSetBuffer(fname);
    test_mmioOpen_fourcc();
    test_mmioSeek();
    test_mmio_end_of_file();
    test_mmio_buffer_pointer();
    test_riff_write();
}
