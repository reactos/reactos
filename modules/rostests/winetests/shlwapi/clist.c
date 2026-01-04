/* Unit test suite for SHLWAPI Compact List and IStream ordinal functions
 *
 * Copyright 2002 Jon Griffiths
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

#define COBJMACROS
#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "shlobj.h"

/* Items to add */
static const DATABLOCK_HEADER clist_items[] =
{
  {4, 1},
  {8, 3},
  {12, 2},
  {16, 8},
  {20, 9},
  {3, 11},
  {9, 82},
  {33, 16},
  {32, 55},
  {24, 100},
  {39, 116},
  { 0, 0}
};

/* Dummy IStream object for testing calls */
struct dummystream
{
  IStream IStream_iface;
  LONG  ref;
  int   readcalls;
  BOOL  failreadcall;
  BOOL  failreadsize;
  BOOL  readbeyondend;
  BOOL  readreturnlarge;
  int   writecalls;
  BOOL  failwritecall;
  BOOL  failwritesize;
  int   seekcalls;
  int   statcalls;
  BOOL  failstatcall;
  const DATABLOCK_HEADER *item;
  ULARGE_INTEGER   pos;
};

static inline struct dummystream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, struct dummystream, IStream_iface);
}

static HRESULT WINAPI QueryInterface(IStream *iface, REFIID riid, void **ret_iface)
{
    if (IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IStream, riid)) {
        *ret_iface = iface;
        IStream_AddRef(iface);
        return S_OK;
    }
    trace("Unexpected REFIID %s\n", wine_dbgstr_guid(riid));
    *ret_iface = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI AddRef(IStream *iface)
{
    struct dummystream *This = impl_from_IStream(iface);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI Release(IStream *iface)
{
    struct dummystream *This = impl_from_IStream(iface);

    return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI Read(IStream *iface, void *lpMem, ULONG ulSize, ULONG *lpRead)
{
  struct dummystream *This = impl_from_IStream(iface);
  HRESULT hRet = S_OK;

  ++This->readcalls;
  if (This->failreadcall)
  {
    return STG_E_ACCESSDENIED;
  }
  else if (This->failreadsize)
  {
    *lpRead = ulSize + 8;
    return S_OK;
  }
  else if (This->readreturnlarge)
  {
    *((ULONG*)lpMem) = 0xffff01;
    *lpRead = ulSize;
    This->readreturnlarge = FALSE;
    return S_OK;
  }
  if (ulSize == sizeof(ULONG))
  {
    /* Read size of item */
    *((ULONG*)lpMem) = This->item->cbSize ? This->item->cbSize + sizeof(DATABLOCK_HEADER) : 0;
    *lpRead = ulSize;
  }
  else
  {
    unsigned int i;
    char* buff = lpMem;

    /* Read item data */
    if (!This->item->cbSize)
    {
      This->readbeyondend = TRUE;
      *lpRead = 0;
      return E_FAIL; /* Should never happen */
    }
    *((ULONG *)lpMem) = This->item->dwSignature;
    *lpRead = ulSize;

    for (i = 0; i < This->item->cbSize; i++)
      buff[4+i] = i*2;

    This->item++;
  }
  return hRet;
}

static HRESULT WINAPI Write(IStream *iface, const void *lpMem, ULONG ulSize, ULONG *lpWritten)
{
  struct dummystream *This = impl_from_IStream(iface);
  HRESULT hRet = S_OK;

  ++This->writecalls;
  if (This->failwritecall)
  {
    return STG_E_ACCESSDENIED;
  }
  else if (This->failwritesize)
  {
    *lpWritten = 0;
  }
  else
    *lpWritten = ulSize;
  return hRet;
}

static HRESULT WINAPI Seek(IStream *iface, LARGE_INTEGER dlibMove, DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition)
{
  struct dummystream *This = impl_from_IStream(iface);

  ++This->seekcalls;
  This->pos.QuadPart = dlibMove.QuadPart;
  if (plibNewPosition)
    plibNewPosition->QuadPart = dlibMove.QuadPart;
  return S_OK;
}

static HRESULT WINAPI Stat(IStream *iface, STATSTG *pstatstg, DWORD grfStatFlag)
{
  struct dummystream *This = impl_from_IStream(iface);

  ++This->statcalls;
  if (This->failstatcall)
    return E_FAIL;
  if (pstatstg)
    pstatstg->cbSize.QuadPart = This->pos.QuadPart;
  return S_OK;
}

/* VTable */
static IStreamVtbl iclvt =
{
  QueryInterface,
  AddRef,
  Release,
  Read,
  Write,
  Seek,
  NULL, /* SetSize */
  NULL, /* CopyTo */
  NULL, /* Commit */
  NULL, /* Revert */
  NULL, /* LockRegion */
  NULL, /* UnlockRegion */
  Stat,
  NULL  /* Clone */
};

/* Function ptrs for ordinal calls */
static HMODULE SHLWAPI_hshlwapi = 0;

static void (WINAPI *pSHFreeDataBlockList)(DATABLOCK_HEADER *);
static HRESULT (WINAPI *pSHAddDataBlock)(DATABLOCK_HEADER **, DATABLOCK_HEADER *);
static BOOL (WINAPI *pSHRemoveDataBlock)(DATABLOCK_HEADER **,ULONG);
static DATABLOCK_HEADER *(WINAPI *pSHFindDataBlock)(DATABLOCK_HEADER *,ULONG);
static HRESULT (WINAPI *pSHWriteDataBlockList)(IStream *, DATABLOCK_HEADER *);
static HRESULT (WINAPI *pSHReadDataBlockList)(IStream *, DATABLOCK_HEADER **);

static BOOL (WINAPI *pSHIsEmptyStream)(IStream *);
static HRESULT (WINAPI *pIStream_Read)(IStream *, void *, ULONG);
static HRESULT (WINAPI *pIStream_Write)(IStream *, const void *, ULONG);
static HRESULT (WINAPI *pIStream_Reset)(IStream *);
static HRESULT (WINAPI *pIStream_Size)(IStream *, ULARGE_INTEGER *);


static BOOL InitFunctionPtrs(void)
{
  SHLWAPI_hshlwapi = GetModuleHandleA("shlwapi.dll");

  /* SHCreateStreamOnFileEx was introduced in shlwapi v6.0 */
  if(!GetProcAddress(SHLWAPI_hshlwapi, "SHCreateStreamOnFileEx")){
      win_skip("Too old shlwapi version\n");
      return FALSE;
  }

  pSHWriteDataBlockList = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)17);
  ok(pSHWriteDataBlockList != 0, "No Ordinal 17\n");
  pSHReadDataBlockList = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)18);
  ok(pSHReadDataBlockList != 0, "No Ordinal 18\n");
  pSHFreeDataBlockList = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)19);
  ok(pSHFreeDataBlockList != 0, "No Ordinal 19\n");
  pSHAddDataBlock = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)20);
  ok(pSHAddDataBlock != 0, "No Ordinal 20\n");
  pSHRemoveDataBlock = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)21);
  ok(pSHRemoveDataBlock != 0, "No Ordinal 21\n");
  pSHFindDataBlock = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)22);
  ok(pSHFindDataBlock != 0, "No Ordinal 22\n");
  pSHIsEmptyStream = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)166);
  ok(pSHIsEmptyStream != 0, "No Ordinal 166\n");
  pIStream_Read = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)184);
  ok(pIStream_Read != 0, "No Ordinal 184\n");
  pIStream_Write = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)212);
  ok(pIStream_Write != 0, "No Ordinal 212\n");
  pIStream_Reset = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)213);
  ok(pIStream_Reset != 0, "No Ordinal 213\n");
  pIStream_Size = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)214);
  ok(pIStream_Size != 0, "No Ordinal 214\n");

  return TRUE;
}

static void InitDummyStream(struct dummystream *obj)
{
    obj->IStream_iface.lpVtbl = &iclvt;
    obj->ref = 1;
    obj->readcalls = 0;
    obj->failreadcall = FALSE;
    obj->failreadsize = FALSE;
    obj->readbeyondend = FALSE;
    obj->readreturnlarge = FALSE;
    obj->writecalls = 0;
    obj->failwritecall = FALSE;
    obj->failwritesize = FALSE;
    obj->seekcalls = 0;
    obj->statcalls = 0;
    obj->failstatcall = FALSE;
    obj->item = clist_items;
    obj->pos.QuadPart = 0;
}


static void test_CList(void)
{
  struct dummystream streamobj;
  DATABLOCK_HEADER *list = NULL;
  const DATABLOCK_HEADER *item = clist_items;
  HRESULT hRet;
  DATABLOCK_HEADER *inserted;
  BYTE buff[64];
  unsigned int i;
  BOOL ret;

  if (!pSHWriteDataBlockList || !pSHReadDataBlockList || !pSHFreeDataBlockList || !pSHAddDataBlock ||
      !pSHRemoveDataBlock || !pSHFindDataBlock)
    return;

  /* Populate a list and test the items are added correctly */
  while (item->cbSize)
  {
    /* Create item and fill with data */
    inserted = (DATABLOCK_HEADER *)buff;
    inserted->cbSize = item->cbSize + sizeof(DATABLOCK_HEADER);
    inserted->dwSignature = item->dwSignature;
    for (i = 0; i < item->cbSize; i++)
      buff[sizeof(DATABLOCK_HEADER) + i] = i * 2;

    ret = pSHAddDataBlock(&list, inserted);
    ok(ret == TRUE, "got %d\n", ret);

    if (ret == TRUE)
    {
      ok(list && list->cbSize, "item not added\n");

      inserted = pSHFindDataBlock(list, item->dwSignature);
      ok(inserted != NULL, "lost after adding\n");

      ok(!inserted || inserted->dwSignature != ~0U, "find returned a container\n");

      /* Check size */
      if (inserted && (inserted->cbSize & 0x3))
      {
        /* Contained */
        ok(inserted[-1].dwSignature == ~0U, "invalid size is not countained\n");
        ok(inserted[-1].cbSize > inserted->cbSize + sizeof(DATABLOCK_HEADER),
           "container too small\n");
      }
      else if (inserted)
      {
        ok(inserted->cbSize == item->cbSize + sizeof(DATABLOCK_HEADER),
           "id %ld wrong size %ld\n", inserted->dwSignature, inserted->cbSize);
      }
      if (inserted)
      {
        BOOL bDataOK = TRUE;
        LPBYTE bufftest = (LPBYTE)inserted;

        for (i = 0; i < inserted->cbSize - sizeof(DATABLOCK_HEADER); i++)
          if (bufftest[sizeof(DATABLOCK_HEADER) + i] != i * 2)
            bDataOK = FALSE;

        ok(bDataOK == TRUE, "data corrupted on insert\n");
      }
      ok(!inserted || inserted->dwSignature == item->dwSignature, "find got wrong item\n");
    }
    item++;
  }

  /* Write the list */
  InitDummyStream(&streamobj);

  hRet = pSHWriteDataBlockList(&streamobj.IStream_iface, list);
  ok(hRet == S_OK, "write failed\n");
  if (hRet == S_OK)
  {
    /* 1 call for each element, + 1 for OK (use our null element for this) */
    ok(streamobj.writecalls == ARRAY_SIZE(clist_items), "wrong call count\n");
    ok(streamobj.readcalls == 0,"called Read() in write\n");
    ok(streamobj.seekcalls == 0,"called Seek() in write\n");
  }

  /* Failure cases for writing */
  InitDummyStream(&streamobj);
  streamobj.failwritecall = TRUE;
  hRet = pSHWriteDataBlockList(&streamobj.IStream_iface, list);
  ok(hRet == STG_E_ACCESSDENIED, "changed object failure return\n");
  ok(streamobj.writecalls == 1, "called object after failure\n");
  ok(streamobj.readcalls == 0,"called Read() after failure\n");
  ok(streamobj.seekcalls == 0,"called Seek() after failure\n");

  InitDummyStream(&streamobj);
  streamobj.failwritesize = TRUE;
  hRet = pSHWriteDataBlockList(&streamobj.IStream_iface, list);
  ok(hRet == STG_E_MEDIUMFULL || broken(hRet == E_FAIL) /* Win7 */,
     "changed size failure return\n");
  ok(streamobj.writecalls == 1, "called object after size failure\n");
  ok(streamobj.readcalls == 0,"called Read() after failure\n");
  ok(streamobj.seekcalls == 0,"called Seek() after failure\n");

  /* Invalid inputs for adding */
  inserted = (DATABLOCK_HEADER *)buff;
  inserted->cbSize = sizeof(DATABLOCK_HEADER) - 1;
  inserted->dwSignature = 33;

  ret = pSHAddDataBlock(NULL, inserted);
  ok(!ret, "got %d\n", ret);

  ret = pSHAddDataBlock(&list, inserted);
  ok(!ret, "got %d\n", ret);

  inserted = pSHFindDataBlock(list, 33);
  ok(inserted == NULL, "inserted bad element size\n");

  inserted = (DATABLOCK_HEADER *)buff;
  inserted->cbSize = 44;
  inserted->dwSignature = ~0U;

  ret = pSHAddDataBlock(&list, inserted);
  ok(!ret, "got %d\n", ret);

  item = clist_items;

  /* Look for nonexistent item in populated list */
  inserted = pSHFindDataBlock(list, 99999999);
  ok(inserted == NULL, "found a nonexistent item\n");

  while (item->cbSize)
  {
    BOOL bRet = pSHRemoveDataBlock(&list, item->dwSignature);
    ok(bRet == TRUE, "couldn't find item to delete\n");
    item++;
  }

  /* Look for nonexistent item in empty list */
  inserted = pSHFindDataBlock(list, 99999999);
  ok(inserted == NULL, "found an item in empty list\n");

  /* Create a list by reading in data */
  InitDummyStream(&streamobj);

  hRet = pSHReadDataBlockList(&streamobj.IStream_iface, &list);
  ok(hRet == S_OK, "failed create from Read()\n");
  if (hRet == S_OK)
  {
    ok(streamobj.readbeyondend == FALSE, "read beyond end\n");
    /* 2 calls per item, but only 1 for the terminator */
    ok(streamobj.readcalls == ARRAY_SIZE(clist_items) * 2 - 1, "wrong call count\n");
    ok(streamobj.writecalls == 0, "called Write() from create\n");
    ok(streamobj.seekcalls == 0,"called Seek() from create\n");

    item = clist_items;

    /* Check the items were added correctly */
    while (item->cbSize)
    {
      inserted = pSHFindDataBlock(list, item->dwSignature);
      ok(inserted != NULL, "lost after adding\n");

      ok(!inserted || inserted->dwSignature != ~0U, "find returned a container\n");

      /* Check size */
      if (inserted && inserted->cbSize & 0x3)
      {
        /* Contained */
        ok(inserted[-1].dwSignature == ~0U, "invalid size is not countained\n");
        ok(inserted[-1].cbSize > inserted->cbSize + sizeof(DATABLOCK_HEADER),
           "container too small\n");
      }
      else if (inserted)
      {
        ok(inserted->cbSize == item->cbSize + sizeof(DATABLOCK_HEADER),
           "id %ld wrong size %ld\n", inserted->dwSignature, inserted->cbSize);
      }
      ok(!inserted || inserted->dwSignature == item->dwSignature, "find got wrong item\n");
      if (inserted)
      {
        BOOL bDataOK = TRUE;
        LPBYTE bufftest = (LPBYTE)inserted;

        for (i = 0; i < inserted->cbSize - sizeof(DATABLOCK_HEADER); i++)
          if (bufftest[sizeof(DATABLOCK_HEADER) + i] != i * 2)
            bDataOK = FALSE;

        ok(bDataOK == TRUE, "data corrupted on insert\n");
      }
      item++;
    }
  }

  /* Failure cases for reading */
  InitDummyStream(&streamobj);
  streamobj.failreadcall = TRUE;
  hRet = pSHReadDataBlockList(&streamobj.IStream_iface, &list);
  ok(hRet == STG_E_ACCESSDENIED, "changed object failure return\n");
  ok(streamobj.readbeyondend == FALSE, "read beyond end\n");
  ok(streamobj.readcalls == 1, "called object after read failure\n");
  ok(streamobj.writecalls == 0,"called Write() after read failure\n");
  ok(streamobj.seekcalls == 0,"called Seek() after read failure\n");

  /* Read returns large object */
  InitDummyStream(&streamobj);
  streamobj.readreturnlarge = TRUE;
  hRet = pSHReadDataBlockList(&streamobj.IStream_iface, &list);
  ok(hRet == S_OK, "failed create from Read() with large item\n");
  ok(streamobj.readbeyondend == FALSE, "read beyond end\n");
  ok(streamobj.readcalls == 1,"wrong call count\n");
  ok(streamobj.writecalls == 0,"called Write() after read failure\n");
  ok(streamobj.seekcalls == 2,"wrong Seek() call count (%d)\n", streamobj.seekcalls);

  pSHFreeDataBlockList(list);
}

static BOOL test_SHIsEmptyStream(void)
{
  struct dummystream streamobj;
  BOOL bRet;

  if (!pSHIsEmptyStream)
    return FALSE;

  InitDummyStream(&streamobj);
  bRet = pSHIsEmptyStream(&streamobj.IStream_iface);

  if (bRet != TRUE)
    return FALSE; /* This version doesn't support stream ops on clists */

  ok(streamobj.readcalls == 0, "called Read()\n");
  ok(streamobj.writecalls == 0, "called Write()\n");
  ok(streamobj.seekcalls == 0, "called Seek()\n");
  ok(streamobj.statcalls == 1, "wrong call count\n");

  streamobj.statcalls = 0;
  streamobj.pos.QuadPart = 50001;

  bRet = pSHIsEmptyStream(&streamobj.IStream_iface);

  ok(bRet == FALSE, "failed after seek adjusted\n");
  ok(streamobj.readcalls == 0, "called Read()\n");
  ok(streamobj.writecalls == 0, "called Write()\n");
  ok(streamobj.seekcalls == 0, "called Seek()\n");
  ok(streamobj.statcalls == 1, "wrong call count\n");

  /* Failure cases */
  InitDummyStream(&streamobj);
  streamobj.pos.QuadPart = 50001;
  streamobj.failstatcall = TRUE; /* 1: Stat() Bad, Read() OK */
  bRet = pSHIsEmptyStream(&streamobj.IStream_iface);
  ok(bRet == FALSE, "should be FALSE after read is OK\n");
  ok(streamobj.readcalls == 1, "wrong call count\n");
  ok(streamobj.writecalls == 0, "called Write()\n");
  ok(streamobj.seekcalls == 1, "wrong call count\n");
  ok(streamobj.statcalls == 1, "wrong call count\n");
  ok(streamobj.pos.QuadPart == 0, "Didn't seek to start\n");

  InitDummyStream(&streamobj);
  streamobj.pos.QuadPart = 50001;
  streamobj.failstatcall = TRUE;
  streamobj.failreadcall = TRUE; /* 2: Stat() Bad, Read() Bad Also */
  bRet = pSHIsEmptyStream(&streamobj.IStream_iface);
  ok(bRet == TRUE, "Should be true after read fails\n");
  ok(streamobj.readcalls == 1, "wrong call count\n");
  ok(streamobj.writecalls == 0, "called Write()\n");
  ok(streamobj.seekcalls == 0, "Called Seek()\n");
  ok(streamobj.statcalls == 1, "wrong call count\n");
  ok(streamobj.pos.QuadPart == 50001, "called Seek() after read failed\n");
  return TRUE;
}

static void test_IStream_Read(void)
{
  struct dummystream streamobj;
  char buff[256];
  HRESULT hRet;

  if (!pIStream_Read)
    return;

  InitDummyStream(&streamobj);
  hRet = pIStream_Read(&streamobj.IStream_iface, buff, sizeof(buff));

  ok(hRet == S_OK, "failed Read()\n");
  ok(streamobj.readcalls == 1, "wrong call count\n");
  ok(streamobj.writecalls == 0, "called Write()\n");
  ok(streamobj.seekcalls == 0, "called Seek()\n");
}

static void test_IStream_Write(void)
{
  struct dummystream streamobj;
  char buff[256] = {0};
  HRESULT hRet;

  if (!pIStream_Write)
    return;

  InitDummyStream(&streamobj);
  hRet = pIStream_Write(&streamobj.IStream_iface, buff, sizeof(buff));

  ok(hRet == S_OK, "failed Write()\n");
  ok(streamobj.readcalls == 0, "called Read()\n");
  ok(streamobj.writecalls == 1, "wrong call count\n");
  ok(streamobj.seekcalls == 0, "called Seek()\n");
}

static void test_IStream_Reset(void)
{
  struct dummystream streamobj;
  ULARGE_INTEGER ul;
  LARGE_INTEGER ll;
  HRESULT hRet;

  if (!pIStream_Reset || !pIStream_Size)
    return;

  InitDummyStream(&streamobj);
  ll.QuadPart = 5000l;
  Seek(&streamobj.IStream_iface, ll, 0, NULL); /* Seek to 5000l */

  streamobj.seekcalls = 0;
  pIStream_Reset(&streamobj.IStream_iface);
  ok(streamobj.statcalls == 0, "called Stat()\n");
  ok(streamobj.readcalls == 0, "called Read()\n");
  ok(streamobj.writecalls == 0, "called Write()\n");
  ok(streamobj.seekcalls == 1, "wrong call count\n");

  ul.QuadPart = 50001;
  hRet = pIStream_Size(&streamobj.IStream_iface, &ul);
  ok(hRet == S_OK, "failed Stat()\n");
  ok(ul.QuadPart == 0, "213 didn't rewind stream\n");
}

static void test_IStream_Size(void)
{
  struct dummystream streamobj;
  ULARGE_INTEGER ul;
  LARGE_INTEGER ll;
  HRESULT hRet;

  if (!pIStream_Size)
    return;

  InitDummyStream(&streamobj);
  ll.QuadPart = 5000l;
  Seek(&streamobj.IStream_iface, ll, 0, NULL);
  ul.QuadPart = 0;
  streamobj.seekcalls = 0;
  hRet = pIStream_Size(&streamobj.IStream_iface, &ul);

  ok(hRet == S_OK, "failed Stat()\n");
  ok(streamobj.statcalls == 1, "wrong call count\n");
  ok(streamobj.readcalls == 0, "called Read()\n");
  ok(streamobj.writecalls == 0, "called Write()\n");
  ok(streamobj.seekcalls == 0, "called Seek()\n");
  ok(ul.QuadPart == 5000l, "Stat gave wrong size\n");
}

START_TEST(clist)
{
  if(!InitFunctionPtrs())
    return;

  test_CList();

  /* Test streaming if this version supports it */
  if (test_SHIsEmptyStream())
  {
    test_IStream_Read();
    test_IStream_Write();
    test_IStream_Reset();
    test_IStream_Size();
  }
}
