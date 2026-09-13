/*
 *	OLEAUT32
 *
 * Copyright 1999, 2000 Marcus Meissner
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
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#include "ole2.h"
#include "olectl.h"
#include "oleauto.h"
#include "rpcproxy.h"
#include "initguid.h"
#include "typelib.h"
#include "oleaut32_oaidl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);
WINE_DECLARE_DEBUG_CHANNEL(heap);

/******************************************************************************
 * BSTR  {OLEAUT32}
 *
 * NOTES
 *  BSTR is a simple typedef for a wide-character string used as the principle
 *  string type in ole automation. When encapsulated in a Variant type they are
 *  automatically copied and destroyed as the variant is processed.
 *
 *  The low level BSTR API allows manipulation of these strings and is used by
 *  higher level API calls to manage the strings transparently to the caller.
 *
 *  Internally the BSTR type is allocated with space for a DWORD byte count before
 *  the string data begins. This is undocumented and non-system code should not
 *  access the count directly. Use SysStringLen() or SysStringByteLen()
 *  instead. Note that the byte count does not include the terminating NUL.
 *
 *  To create a new BSTR, use SysAllocString(), SysAllocStringLen() or
 *  SysAllocStringByteLen(). To change the size of an existing BSTR, use SysReAllocString()
 *  or SysReAllocStringLen(). Finally to destroy a string use SysFreeString().
 *
 *  BSTR's are cached by Ole Automation by default. To override this behaviour
 *  either set the environment variable 'OANOCACHE', or call SetOaNoCache().
 *
 * SEE ALSO
 *  'Inside OLE, second edition' by Kraig Brockshmidt.
 */

static BOOL bstr_cache_enabled;

static CRITICAL_SECTION cs_bstr_cache;
static CRITICAL_SECTION_DEBUG cs_bstr_cache_dbg =
{
    0, 0, &cs_bstr_cache,
    { &cs_bstr_cache_dbg.ProcessLocksList, &cs_bstr_cache_dbg.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": bstr_cache") }
};
static CRITICAL_SECTION cs_bstr_cache = { &cs_bstr_cache_dbg, -1, 0, 0, 0, 0 };

typedef struct {
#ifdef _WIN64
    DWORD pad;
#endif
    DWORD size;
    union {
        char ptr[1];
        WCHAR str[1];
        DWORD dwptr[1];
    } u;
} bstr_t;

#define BUCKET_SIZE 16
#define BUCKET_BUFFER_SIZE 6

typedef struct {
    unsigned short head;
    unsigned short cnt;
    bstr_t *buf[BUCKET_BUFFER_SIZE];
} bstr_cache_entry_t;

#define ARENA_INUSE_FILLER     0x55
#define ARENA_TAIL_FILLER      0xab
#define ARENA_FREE_FILLER      0xfeeefeee

static bstr_cache_entry_t bstr_cache[0x10000/BUCKET_SIZE];

static inline size_t bstr_alloc_size(size_t size)
{
    return (FIELD_OFFSET(bstr_t, u.ptr[size]) + sizeof(WCHAR) + BUCKET_SIZE-1) & ~(BUCKET_SIZE-1);
}

static inline bstr_t *bstr_from_str(BSTR str)
{
    return CONTAINING_RECORD(str, bstr_t, u.str);
}

static inline bstr_cache_entry_t *get_cache_entry_from_idx(unsigned cache_idx)
{
    return bstr_cache_enabled && cache_idx < ARRAY_SIZE(bstr_cache) ? bstr_cache + cache_idx : NULL;
}

static inline bstr_cache_entry_t *get_cache_entry(size_t size)
{
    unsigned cache_idx = FIELD_OFFSET(bstr_t, u.ptr[size+sizeof(WCHAR)-1])/BUCKET_SIZE;
    return get_cache_entry_from_idx(cache_idx);
}

static inline bstr_cache_entry_t *get_cache_entry_from_alloc_size(SIZE_T alloc_size)
{
    unsigned cache_idx;
    if (alloc_size < BUCKET_SIZE) return NULL;
    cache_idx = (alloc_size - BUCKET_SIZE) / BUCKET_SIZE;
    return get_cache_entry_from_idx(cache_idx);
}

static bstr_t *alloc_bstr(size_t size)
{
    bstr_cache_entry_t *cache_entry = get_cache_entry(size);
    bstr_t *ret;

    if(cache_entry) {
        EnterCriticalSection(&cs_bstr_cache);

        if(!cache_entry->cnt) {
            cache_entry = get_cache_entry(size+BUCKET_SIZE);
            if(cache_entry && !cache_entry->cnt)
                cache_entry = NULL;
        }

        if(cache_entry) {
            ret = cache_entry->buf[cache_entry->head++];
            cache_entry->head %= BUCKET_BUFFER_SIZE;
            cache_entry->cnt--;
        }

        LeaveCriticalSection(&cs_bstr_cache);

        if(cache_entry) {
            if(WARN_ON(heap)) {
                size_t fill_size = (FIELD_OFFSET(bstr_t, u.ptr[size])+2*sizeof(WCHAR)-1) & ~(sizeof(WCHAR)-1);
                memset(ret, ARENA_INUSE_FILLER, fill_size);
                memset((char *)ret+fill_size, ARENA_TAIL_FILLER, bstr_alloc_size(size)-fill_size);
            }
            ret->size = size;
            return ret;
        }
    }

    ret = CoTaskMemAlloc(bstr_alloc_size(size));
    if(ret)
        ret->size = size;
    return ret;
}

/******************************************************************************
 *             SysStringLen  [OLEAUT32.7]
 *
 * Get the allocated length of a BSTR in wide characters.
 *
 * PARAMS
 *  str [I] BSTR to find the length of
 *
 * RETURNS
 *  The allocated length of str, or 0 if str is NULL.
 *
 * NOTES
 *  See BSTR.
 *  The returned length may be different from the length of the string as
 *  calculated by lstrlenW(), since it returns the length that was used to
 *  allocate the string by SysAllocStringLen().
 */
UINT WINAPI SysStringLen(BSTR str)
{
    return str ? bstr_from_str(str)->size/sizeof(WCHAR) : 0;
}

/******************************************************************************
 *             SysStringByteLen  [OLEAUT32.149]
 *
 * Get the allocated length of a BSTR in bytes.
 *
 * PARAMS
 *  str [I] BSTR to find the length of
 *
 * RETURNS
 *  The allocated length of str, or 0 if str is NULL.
 *
 * NOTES
 *  See SysStringLen(), BSTR().
 */
UINT WINAPI SysStringByteLen(BSTR str)
{
    return str ? bstr_from_str(str)->size : 0;
}

/******************************************************************************
 *		SysAllocString	[OLEAUT32.2]
 *
 * Create a BSTR from an OLESTR.
 *
 * PARAMS
 *  str [I] Source to create BSTR from
 *
 * RETURNS
 *  Success: A BSTR allocated with SysAllocStringLen().
 *  Failure: NULL, if oleStr is NULL.
 *
 * NOTES
 *  See BSTR.
 *  MSDN (October 2001) incorrectly states that NULL is returned if oleStr has
 *  a length of 0. Native Win32 and this implementation both return a valid
 *  empty BSTR in this case.
 */
BSTR WINAPI SysAllocString(LPCOLESTR str)
{
    if (!str) return 0;

    /* Delegate this to the SysAllocStringLen32 method. */
    return SysAllocStringLen(str, lstrlenW(str));
}

static inline IMalloc *get_malloc(void)
{
    static IMalloc *malloc;

    if (!malloc)
        CoGetMalloc(1, &malloc);

    return malloc;
}

/******************************************************************************
 *		SysFreeString	[OLEAUT32.6]
 *
 * Free a BSTR.
 *
 * PARAMS
 *  str [I] BSTR to free.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  See BSTR.
 *  str may be NULL, in which case this function does nothing.
 */
void WINAPI DECLSPEC_HOTPATCH SysFreeString(BSTR str)
{
    bstr_cache_entry_t *cache_entry;
    bstr_t *bstr;
    IMalloc *malloc = get_malloc();
    SIZE_T alloc_size;

    if(!str)
        return;

    bstr = bstr_from_str(str);

    alloc_size = IMalloc_GetSize(malloc, bstr);
    if (alloc_size == ~0UL)
        return;

    cache_entry = get_cache_entry_from_alloc_size(alloc_size);
    if(cache_entry) {
        unsigned i;

        EnterCriticalSection(&cs_bstr_cache);

        /* According to tests, freeing a string that's already in cache doesn't corrupt anything.
         * For that to work we need to search the cache. */
        for(i=0; i < cache_entry->cnt; i++) {
            if(cache_entry->buf[(cache_entry->head+i) % BUCKET_BUFFER_SIZE] == bstr) {
                WARN_(heap)("String already is in cache!\n");
                LeaveCriticalSection(&cs_bstr_cache);
                return;
            }
        }

        if(cache_entry->cnt < ARRAY_SIZE(cache_entry->buf)) {
            cache_entry->buf[(cache_entry->head+cache_entry->cnt) % BUCKET_BUFFER_SIZE] = bstr;
            cache_entry->cnt++;

            if(WARN_ON(heap)) {
                unsigned n = (alloc_size-FIELD_OFFSET(bstr_t, u.ptr))/sizeof(DWORD);
                for(i=0; i<n; i++)
                    bstr->u.dwptr[i] = ARENA_FREE_FILLER;
            }

            LeaveCriticalSection(&cs_bstr_cache);
            return;
        }

        LeaveCriticalSection(&cs_bstr_cache);
    }

    CoTaskMemFree(bstr);
}

/******************************************************************************
 *             SysAllocStringLen     [OLEAUT32.4]
 *
 * Create a BSTR from an OLESTR of a given wide character length.
 *
 * PARAMS
 *  str [I] Source to create BSTR from
 *  len [I] Length of oleStr in wide characters
 *
 * RETURNS
 *  Success: A newly allocated BSTR from SysAllocStringByteLen()
 *  Failure: NULL, if len is >= 0x80000000, or memory allocation fails.
 *
 * NOTES
 *  See BSTR(), SysAllocStringByteLen().
 */
BSTR WINAPI SysAllocStringLen(const OLECHAR *str, unsigned int len)
{
    bstr_t *bstr;
    DWORD size;

    /* Detect integer overflow. */
    if (len >= ((UINT_MAX-sizeof(WCHAR)-sizeof(DWORD))/sizeof(WCHAR)))
	return NULL;

    TRACE("%s\n", debugstr_wn(str, len));

    size = len*sizeof(WCHAR);
    bstr = alloc_bstr(size);
    if(!bstr)
        return NULL;

    if(str) {
        memcpy(bstr->u.str, str, size);
        bstr->u.str[len] = 0;
    }else {
        memset(bstr->u.str, 0, size+sizeof(WCHAR));
    }

    return bstr->u.str;
}

/******************************************************************************
 *             SysReAllocStringLen   [OLEAUT32.5]
 *
 * Change the length of a previously created BSTR.
 *
 * PARAMS
 *  old [O] BSTR to change the length of
 *  str [I] New source for pbstr
 *  len [I] Length of oleStr in wide characters
 *
 * RETURNS
 *  Success: 1. The size of pbstr is updated.
 *  Failure: 0, if len >= 0x80000000 or memory allocation fails.
 *
 * NOTES
 *  See BSTR(), SysAllocStringByteLen().
 *  *old may be changed by this function.
 */
int WINAPI SysReAllocStringLen(BSTR* old, const OLECHAR* str, unsigned int len)
{
    /* Detect integer overflow. */
    if (len >= ((UINT_MAX-sizeof(WCHAR)-sizeof(DWORD))/sizeof(WCHAR)))
	return FALSE;

    if (*old!=NULL) {
      DWORD newbytelen = len*sizeof(WCHAR);
      bstr_t *old_bstr = bstr_from_str(*old);
      bstr_t *bstr = CoTaskMemRealloc(old_bstr, bstr_alloc_size(newbytelen));

      if (!bstr) return FALSE;

      *old = bstr->u.str;
      bstr->size = newbytelen;
      /* The old string data is still there when str is NULL */
      if (str && old_bstr->u.str != str) memmove(bstr->u.str, str, newbytelen);
      bstr->u.str[len] = 0;
    } else {
      *old = SysAllocStringLen(str, len);
    }

    return TRUE;
}

/******************************************************************************
 *             SysAllocStringByteLen     [OLEAUT32.150]
 *
 * Create a BSTR from an OLESTR of a given byte length.
 *
 * PARAMS
 *  str [I] Source to create BSTR from
 *  len [I] Length of oleStr in bytes
 *
 * RETURNS
 *  Success: A newly allocated BSTR
 *  Failure: NULL, if len is >= 0x80000000, or memory allocation fails.
 *
 * NOTES
 *  -If len is 0 or oleStr is NULL the resulting string is empty ("").
 *  -This function always NUL terminates the resulting BSTR.
 *  -oleStr may be either an LPCSTR or LPCOLESTR, since it is copied
 *  without checking for a terminating NUL.
 *  See BSTR.
 */
BSTR WINAPI DECLSPEC_HOTPATCH SysAllocStringByteLen(LPCSTR str, UINT len)
{
    bstr_t *bstr;

    /* Detect integer overflow. */
    if (len >= (UINT_MAX-sizeof(WCHAR)-sizeof(DWORD)))
	return NULL;

    bstr = alloc_bstr(len);
    if(!bstr)
        return NULL;

    if(str) {
        memcpy(bstr->u.ptr, str, len);
        bstr->u.ptr[len] = 0;
    }else {
        memset(bstr->u.ptr, 0, len+1);
    }
    bstr->u.str[(len+sizeof(WCHAR)-1)/sizeof(WCHAR)] = 0;

    return bstr->u.str;
}

/******************************************************************************
 *		SysReAllocString	[OLEAUT32.3]
 *
 * Change the length of a previously created BSTR.
 *
 * PARAMS
 *  old [I/O] BSTR to change the length of
 *  str [I]   New source for pbstr
 *
 * RETURNS
 *  Success: 1
 *  Failure: 0.
 *
 * NOTES
 *  See BSTR(), SysAllocStringStringLen().
 */
INT WINAPI SysReAllocString(LPBSTR old,LPCOLESTR str)
{
    /*
     * Sanity check
     */
    if (old==NULL)
      return 0;

    /*
     * Make sure we free the old string.
     */
    SysFreeString(*old);

    /*
     * Allocate the new string
     */
    *old = SysAllocString(str);

     return 1;
}

/******************************************************************************
 *		SetOaNoCache (OLEAUT32.327)
 *
 * Instruct Ole Automation not to cache BSTR allocations.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  SetOaNoCache does not release cached strings, so it leaks by design.
 */
void WINAPI SetOaNoCache(void)
{
    TRACE("\n");
    bstr_cache_enabled = FALSE;
}

/***********************************************************************
 *		RegisterActiveObject (OLEAUT32.33)
 *
 * Registers an object in the global item table.
 *
 * PARAMS
 *  punk        [I] Object to register.
 *  rcid        [I] CLSID of the object.
 *  dwFlags     [I] Flags.
 *  pdwRegister [O] Address to store cookie of object registration in.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI DECLSPEC_HOTPATCH RegisterActiveObject(
	LPUNKNOWN punk,REFCLSID rcid,DWORD dwFlags,LPDWORD pdwRegister
) {
	WCHAR 			guidbuf[80];
	HRESULT			ret;
	LPRUNNINGOBJECTTABLE	runobtable;
	LPMONIKER		moniker;
        DWORD                   rot_flags = ROTFLAGS_REGISTRATIONKEEPSALIVE; /* default registration is strong */

	StringFromGUID2(rcid,guidbuf,39);
	ret = CreateItemMoniker(L"!", guidbuf, &moniker);
	if (FAILED(ret))
		return ret;
	ret = GetRunningObjectTable(0,&runobtable);
	if (FAILED(ret)) {
		IMoniker_Release(moniker);
		return ret;
	}
        if(dwFlags == ACTIVEOBJECT_WEAK)
          rot_flags = 0;
	ret = IRunningObjectTable_Register(runobtable,rot_flags,punk,moniker,pdwRegister);
	IRunningObjectTable_Release(runobtable);
	IMoniker_Release(moniker);
	return ret;
}

/***********************************************************************
 *		RevokeActiveObject (OLEAUT32.34)
 *
 * Revokes an object from the global item table.
 *
 * PARAMS
 *  xregister [I] Registration cookie.
 *  reserved  [I] Reserved. Set to NULL.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI DECLSPEC_HOTPATCH RevokeActiveObject(DWORD xregister,LPVOID reserved)
{
	LPRUNNINGOBJECTTABLE	runobtable;
	HRESULT			ret;

	ret = GetRunningObjectTable(0,&runobtable);
	if (FAILED(ret)) return ret;
	ret = IRunningObjectTable_Revoke(runobtable,xregister);
	if (SUCCEEDED(ret)) ret = S_OK;
	IRunningObjectTable_Release(runobtable);
	return ret;
}

/***********************************************************************
 *		GetActiveObject (OLEAUT32.35)
 *
 * Gets an object from the global item table.
 *
 * PARAMS
 *  rcid        [I] CLSID of the object.
 *  preserved   [I] Reserved. Set to NULL.
 *  ppunk       [O] Address to store object into.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI DECLSPEC_HOTPATCH GetActiveObject(REFCLSID rcid,LPVOID preserved,LPUNKNOWN *ppunk)
{
	WCHAR 			guidbuf[80];
	HRESULT			ret;
	LPRUNNINGOBJECTTABLE	runobtable;
	LPMONIKER		moniker;

	StringFromGUID2(rcid,guidbuf,39);
	ret = CreateItemMoniker(L"!", guidbuf, &moniker);
	if (FAILED(ret))
		return ret;
	ret = GetRunningObjectTable(0,&runobtable);
	if (FAILED(ret)) {
		IMoniker_Release(moniker);
		return ret;
	}
	ret = IRunningObjectTable_GetObject(runobtable,moniker,ppunk);
	IRunningObjectTable_Release(runobtable);
	IMoniker_Release(moniker);
	return ret;
}


/***********************************************************************
 *           OaBuildVersion           [OLEAUT32.170]
 *
 * Get the Ole Automation build version.
 *
 * PARAMS
 *  None
 *
 * RETURNS
 *  The build version.
 *
 * NOTES
 *  Known oleaut32.dll versions:
 *| OLE Ver.  Comments                   Date     Build Ver.
 *| --------  -------------------------  ----     ---------
 *| OLE 2.1   NT                         1993-95  10 3023
 *| OLE 2.1                                       10 3027
 *| Win32s    Ver 1.1e                            20 4049
 *| OLE 2.20  W95/NT                     1993-96  20 4112
 *| OLE 2.20  W95/NT                     1993-96  20 4118
 *| OLE 2.20  W95/NT                     1993-96  20 4122
 *| OLE 2.30  W95/NT                     1993-98  30 4265
 *| OLE 2.40  NT??                       1993-98  40 4267
 *| OLE 2.40  W98 SE orig. file          1993-98  40 4275
 *| OLE 2.40  W2K orig. file             1993-XX  40 4514
 *
 * Currently the versions returned are 2.20 for Win3.1, 2.30 for Win95 & NT 3.51,
 * and 2.40 for all later versions. The build number is maximum, i.e. 0xffff.
 */
ULONG WINAPI OaBuildVersion(void)
{
    switch(GetVersion() & 0x8000ffff)  /* mask off build number */
    {
    case 0x80000a03:  /* WIN31 */
		return MAKELONG(0xffff, 20);
    case 0x00003303:  /* NT351 */
		return MAKELONG(0xffff, 30);
    case 0x80000004:  /* WIN95; I'd like to use the "standard" w95 minor
		         version here (30), but as we still use w95
		         as default winver (which is good IMHO), I better
		         play safe and use the latest value for w95 for now.
		         Change this as soon as default winver gets changed
		         to something more recent */
    case 0x80000a04:  /* WIN98 */
    case 0x00000004:  /* NT40 */
    case 0x00000005:  /* W2K */
		return MAKELONG(0xffff, 40);
    case 0x00000105:  /* WinXP */
    case 0x00000006:  /* Vista */
    case 0x00000106:  /* Win7 */
    case 0x00000205:  /* W2K3 */
    case 0x00000206:  /* Win8, Win10, Win11 */
		return MAKELONG(0xffff, 50);
    default:
		FIXME("Version value not known yet. Please investigate it !\n");
		return MAKELONG(0xffff, 50);  /* for now return the same value as for Win10 */
    }
}

/******************************************************************************
 *		OleTranslateColor	[OLEAUT32.421]
 *
 * Convert an OLE_COLOR to a COLORREF.
 *
 * PARAMS
 *  clr       [I] Color to convert
 *  hpal      [I] Handle to a palette for the conversion
 *  pColorRef [O] Destination for converted color, or NULL to test if the conversion is ok
 *
 * RETURNS
 *  Success: S_OK. The conversion is ok, and pColorRef contains the converted color if non-NULL.
 *  Failure: E_INVALIDARG, if any argument is invalid.
 *
 * FIXME
 *  Document the conversion rules.
 */
HRESULT WINAPI OleTranslateColor(
  OLE_COLOR clr,
  HPALETTE  hpal,
  COLORREF* pColorRef)
{
  COLORREF colorref;
  BYTE b = HIBYTE(HIWORD(clr));

  TRACE("%#lx, %p, %p.\n", clr, hpal, pColorRef);

  /*
   * In case pColorRef is NULL, provide our own to simplify the code.
   */
  if (pColorRef == NULL)
    pColorRef = &colorref;

  switch (b)
  {
    case 0x00:
    {
      if (hpal != 0)
        *pColorRef =  PALETTERGB(GetRValue(clr),
                                 GetGValue(clr),
                                 GetBValue(clr));
      else
        *pColorRef = clr;

      break;
    }

    case 0x01:
    {
      if (hpal != 0)
      {
        PALETTEENTRY pe;
        /*
         * Validate the palette index.
         */
        if (GetPaletteEntries(hpal, LOWORD(clr), 1, &pe) == 0)
          return E_INVALIDARG;
      }

      *pColorRef = clr;

      break;
    }

    case 0x02:
      *pColorRef = clr;
      break;

    case 0x80:
    {
      int index = LOBYTE(LOWORD(clr));

      /*
       * Validate GetSysColor index.
       */
      if ((index < COLOR_SCROLLBAR) || (index > COLOR_MENUBAR))
        return E_INVALIDARG;

      *pColorRef =  GetSysColor(index);

      break;
    }

    default:
      return E_INVALIDARG;
  }

  return S_OK;
}

extern HRESULT WINAPI OLEAUTPS_DllGetClassObject(REFCLSID, REFIID, LPVOID *);
extern BOOL WINAPI OLEAUTPS_DllMain(HINSTANCE, DWORD, LPVOID);
extern HRESULT WINAPI OLEAUTPS_DllRegisterServer(void);
extern HRESULT WINAPI OLEAUTPS_DllUnregisterServer(void);

extern HRESULT WINAPI CreateProxyFromTypeInfo(ITypeInfo *typeinfo,
        IUnknown *outer, REFIID iid, IRpcProxyBuffer **proxy, void **obj);
extern HRESULT WINAPI CreateStubFromTypeInfo(ITypeInfo *typeinfo, REFIID iid,
        IUnknown *server, IRpcStubBuffer **stub);

struct ifacepsredirect_data
{
    ULONG size;
    DWORD mask;
    GUID  iid;
    ULONG nummethods;
    GUID  tlbid;
    GUID  base;
    ULONG name_len;
    ULONG name_offset;
};

struct tlibredirect_data
{
    ULONG  size;
    DWORD  res;
    ULONG  name_len;
    ULONG  name_offset;
    LANGID langid;
    WORD   flags;
    ULONG  help_len;
    ULONG  help_offset;
    WORD   major_version;
    WORD   minor_version;
};

static BOOL actctx_get_typelib_module(REFIID iid, WCHAR *module, DWORD len)
{
    struct ifacepsredirect_data *iface;
    struct tlibredirect_data *tlib;
    ACTCTX_SECTION_KEYED_DATA data;
    WCHAR *ptrW;

    data.cbSize = sizeof(data);
    if (!FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION,
            iid, &data))
        return FALSE;

    iface = (struct ifacepsredirect_data *)data.lpData;
    if (!FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION,
            &iface->tlbid, &data))
        return FALSE;

    tlib = (struct tlibredirect_data *)data.lpData;
    ptrW = (WCHAR *)((BYTE *)data.lpSectionBase + tlib->name_offset);

    if (tlib->name_len/sizeof(WCHAR) >= len)
    {
        ERR("need larger module buffer, %lu.\n", tlib->name_len);
        return FALSE;
    }

    memcpy(module, ptrW, tlib->name_len);
    module[tlib->name_len/sizeof(WCHAR)] = 0;
    return TRUE;
}

static HRESULT reg_get_typelib_module(REFIID iid, WCHAR *module, DWORD len)
{
    REGSAM opposite = (sizeof(void*) == 8) ? KEY_WOW64_32KEY : KEY_WOW64_64KEY;
    char tlguid[200], typelibkey[316], interfacekey[300], ver[100], tlfn[260];
    DWORD tlguidlen, verlen, type;
    LONG tlfnlen, err;
    BOOL is_wow64;
    HKEY ikey;

    sprintf( interfacekey, "Interface\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\Typelib",
        iid->Data1, iid->Data2, iid->Data3,
        iid->Data4[0], iid->Data4[1], iid->Data4[2], iid->Data4[3],
        iid->Data4[4], iid->Data4[5], iid->Data4[6], iid->Data4[7]
    );

    err = RegOpenKeyExA(HKEY_CLASSES_ROOT,interfacekey,0,KEY_READ,&ikey);
    if (err && (opposite == KEY_WOW64_32KEY || (IsWow64Process(GetCurrentProcess(), &is_wow64)
                                                && is_wow64)))
        err = RegOpenKeyExA(HKEY_CLASSES_ROOT,interfacekey,0,KEY_READ|opposite,&ikey);

    if (err)
    {
        ERR("No %s key found.\n", interfacekey);
        return E_FAIL;
    }

    tlguidlen = sizeof(tlguid);
    if (RegQueryValueExA(ikey, NULL, NULL, &type, (BYTE *)tlguid, &tlguidlen))
    {
        ERR("Getting typelib guid failed.\n");
        RegCloseKey(ikey);
        return E_FAIL;
    }

    verlen = sizeof(ver);
    if (RegQueryValueExA(ikey, "Version", NULL, &type, (BYTE *)ver, &verlen))
    {
        ERR("Could not get version value?\n");
        RegCloseKey(ikey);
        return E_FAIL;
    }

    RegCloseKey(ikey);

#ifndef __REACTOS__
    sprintf(typelibkey, "Typelib\\%s\\%s\\0\\win%u", tlguid, ver, sizeof(void *) == 8 ? 64 : 32);
#else
    snprintf(typelibkey, sizeof(typelibkey), "Typelib\\%s\\%s\\0\\win%u", tlguid, ver, sizeof(void *) == 8 ? 64 : 32);
#endif /* __REACTOS__ */
    tlfnlen = sizeof(tlfn);
    if (RegQueryValueA(HKEY_CLASSES_ROOT, typelibkey, tlfn, &tlfnlen))
    {
#ifdef _WIN64
        sprintf(typelibkey, "Typelib\\%s\\%s\\0\\win32", tlguid, ver);
        tlfnlen = sizeof(tlfn);
        if (RegQueryValueA(HKEY_CLASSES_ROOT, typelibkey, tlfn, &tlfnlen))
        {
#endif
            ERR("Could not get typelib fn?\n");
            return E_FAIL;
#ifdef _WIN64
        }
#endif
    }
    MultiByteToWideChar(CP_ACP, 0, tlfn, -1, module, len);
    return S_OK;
}

static HRESULT get_typeinfo_for_iid(REFIID iid, ITypeInfo **typeinfo)
{
    WCHAR module[MAX_PATH];
    ITypeLib *typelib;
    HRESULT hr;

    *typeinfo = NULL;

    module[0] = 0;
    if (!actctx_get_typelib_module(iid, module, ARRAY_SIZE(module)))
    {
        hr = reg_get_typelib_module(iid, module, ARRAY_SIZE(module));
        if (FAILED(hr))
            return hr;
    }

    hr = LoadTypeLib(module, &typelib);
    if (hr != S_OK) {
        ERR("Failed to load typelib for %s, but it should be there.\n", debugstr_guid(iid));
        return hr;
    }

    hr = ITypeLib_GetTypeInfoOfGuid(typelib, iid, typeinfo);
    ITypeLib_Release(typelib);
    if (hr != S_OK)
        ERR("typelib does not contain info for %s\n", debugstr_guid(iid));

    return hr;
}

static HRESULT WINAPI dispatch_typelib_ps_QueryInterface(IPSFactoryBuffer *iface, REFIID iid, void **out)
{
    if (IsEqualIID(iid, &IID_IPSFactoryBuffer) || IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
        return S_OK;
    }

    FIXME("No interface for %s.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dispatch_typelib_ps_AddRef(IPSFactoryBuffer *iface)
{
    return 2;
}

static ULONG WINAPI dispatch_typelib_ps_Release(IPSFactoryBuffer *iface)
{
    return 1;
}

static HRESULT dispatch_create_proxy(IUnknown *outer, IRpcProxyBuffer **proxy, void **out)
{
    IPSFactoryBuffer *factory;
    HRESULT hr;

    hr = OLEAUTPS_DllGetClassObject(&CLSID_PSFactoryBuffer, &IID_IPSFactoryBuffer, (void **)&factory);
    if (FAILED(hr)) return hr;

    hr = IPSFactoryBuffer_CreateProxy(factory, outer, &IID_IDispatch, proxy, out);
    IPSFactoryBuffer_Release(factory);
    return hr;
}

static HRESULT WINAPI dispatch_typelib_ps_CreateProxy(IPSFactoryBuffer *iface,
    IUnknown *outer, REFIID iid, IRpcProxyBuffer **proxy, void **out)
{
    ITypeInfo *typeinfo;
    TYPEATTR *attr;
    HRESULT hr;

    if (IsEqualGUID(iid, &IID_IDispatch))
        return dispatch_create_proxy(outer, proxy, out);

    hr = get_typeinfo_for_iid(iid, &typeinfo);
    if (FAILED(hr)) return hr;

    hr = ITypeInfo_GetTypeAttr(typeinfo, &attr);
    if (FAILED(hr))
    {
        ITypeInfo_Release(typeinfo);
        return hr;
    }

    if (attr->typekind == TKIND_INTERFACE || (attr->wTypeFlags & TYPEFLAG_FDUAL))
        hr = CreateProxyFromTypeInfo(typeinfo, outer, iid, proxy, out);
    else
        hr = dispatch_create_proxy(outer, proxy, out);

    if (FAILED(hr))
        ERR("Failed to create proxy, hr %#lx.\n", hr);

    ITypeInfo_ReleaseTypeAttr(typeinfo, attr);
    ITypeInfo_Release(typeinfo);
    return hr;
}

static HRESULT dispatch_create_stub(IUnknown *server, IRpcStubBuffer **stub)
{
    IPSFactoryBuffer *factory;
    HRESULT hr;

    hr = OLEAUTPS_DllGetClassObject(&CLSID_PSFactoryBuffer, &IID_IPSFactoryBuffer, (void **)&factory);
    if (FAILED(hr)) return hr;

    hr = IPSFactoryBuffer_CreateStub(factory, &IID_IDispatch, server, stub);
    IPSFactoryBuffer_Release(factory);
    return hr;
}

struct dispinterface_stub
{
    CInterfaceStubVtbl stub_vtbl;
    CStdStubBuffer stub_buffer;
};

static struct dispinterface_stub *impl_from_IRpcStubBuffer(IRpcStubBuffer *iface)
{
    return CONTAINING_RECORD(&iface->lpVtbl, struct dispinterface_stub, stub_buffer.lpVtbl);
}

static ULONG WINAPI dispinterface_stub_Release(IRpcStubBuffer *iface)
{
    struct dispinterface_stub *stub = impl_from_IRpcStubBuffer(iface);
    unsigned int refcount = InterlockedDecrement(&stub->stub_buffer.RefCount);

    TRACE("%p decreasing refcount to %u.\n", stub, refcount);

    if (!refcount)
    {
        /* Copied from NdrCStdStubBuffer_Release(), but supposedly incorrect
         * according to the comment there. */
        IRpcStubBuffer_Disconnect(iface);

        free(stub);
    }
    return refcount;
}

extern const ExtendedProxyFileInfo oleaut32_oaidl_ProxyFileInfo;

static const CInterfaceStubVtbl *find_idispatch_stub_vtbl(void)
{
    CInterfaceStubVtbl *const *vtbl;

    for (vtbl = oleaut32_oaidl_ProxyFileInfo.pStubVtblList; *vtbl; ++vtbl)
    {
        if (IsEqualGUID((*vtbl)->header.piid, &IID_IDispatch))
            return *vtbl;
    }

    assert(0);
    return NULL;
}

/* Normal dispinterfaces have an IID specified by the IDL compiler as DIID_*,
 * but are otherwise identical to IDispatch. Unfortunately, such interfaces may
 * not actually support IDispatch in QueryInterface.
 *
 * This becomes a problem, since CreateStub() was designed such that, for some
 * reason, the caller need not actually pass the interface matching "iid". As
 * such the standard rpcrt4 implementation will query the server for the
 * relevant IID.
 *
 * This means that we cannot just pass IID_IDispatch with the object, even
 * though it is in theory an IDispatch. However, while the standard stub
 * constructor is not exported from rpcrt4, all of the vtbl methods are, and
 * the type is public, so we *can* manually create it ourselves, bypassing the
 * QueryInterface check.
 *
 * This relies on some rpcrt4 implementation details.
 */
static HRESULT dispinterface_create_stub(IUnknown *server, const GUID *iid, IRpcStubBuffer **stub)
{
    const CInterfaceStubVtbl *stub_vtbl = find_idispatch_stub_vtbl();
    struct dispinterface_stub *object;
    void *dispatch;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    /* It's possible we can just assume that "server" is already the
     * dispinterface type—we don't have tests for this—but since rpcrt4 queries
     * (which we do have tests for) it makes sense for us to match that
     * behaviour. */
    if (FAILED(hr = IUnknown_QueryInterface(server, iid, &dispatch)))
    {
        ERR("Object does not support interface %s.\n", debugstr_guid(iid));
        free(object);
        return hr;
    }

    object->stub_vtbl.header = stub_vtbl->header;
    object->stub_vtbl.Vtbl.QueryInterface             = CStdStubBuffer_QueryInterface;
    object->stub_vtbl.Vtbl.AddRef                     = CStdStubBuffer_AddRef;
    object->stub_vtbl.Vtbl.Release                    = dispinterface_stub_Release;
    object->stub_vtbl.Vtbl.Connect                    = CStdStubBuffer_Connect;
    object->stub_vtbl.Vtbl.Disconnect                 = CStdStubBuffer_Disconnect;
    object->stub_vtbl.Vtbl.Invoke                     = CStdStubBuffer_Invoke;
    object->stub_vtbl.Vtbl.IsIIDSupported             = CStdStubBuffer_IsIIDSupported;
    object->stub_vtbl.Vtbl.CountRefs                  = CStdStubBuffer_CountRefs;
    object->stub_vtbl.Vtbl.DebugServerQueryInterface  = CStdStubBuffer_DebugServerQueryInterface;
    object->stub_vtbl.Vtbl.DebugServerRelease         = CStdStubBuffer_DebugServerRelease;
    object->stub_buffer.lpVtbl = &object->stub_vtbl.Vtbl;
    object->stub_buffer.RefCount = 1;
    object->stub_buffer.pvServerObject = dispatch;
    /* rpcrt4 will also fill pPSFactory, but it never uses it except in the
     * Release method (which we reimplement). It's only to keep a reference to
     * the module to implement NdrDllCanUnloadNow(). We use the default
     * DllCanUnloadNow() from winecrt0, which always returns S_FALSE, so don't
     * bother filling pPSFactory. */

    TRACE("Created stub %p.\n", object);
    *stub = (IRpcStubBuffer *)&object->stub_buffer.lpVtbl;
    return S_OK;
}

static HRESULT WINAPI dispatch_typelib_ps_CreateStub(IPSFactoryBuffer *iface,
    REFIID iid, IUnknown *server, IRpcStubBuffer **stub)
{
    ITypeInfo *typeinfo;
    TYPEATTR *attr;
    HRESULT hr;

    if (IsEqualGUID(iid, &IID_IDispatch))
        return dispatch_create_stub(server, stub);

    hr = get_typeinfo_for_iid(iid, &typeinfo);
    if (FAILED(hr)) return hr;

    hr = ITypeInfo_GetTypeAttr(typeinfo, &attr);
    if (FAILED(hr))
    {
        ITypeInfo_Release(typeinfo);
        return hr;
    }

    if (attr->typekind == TKIND_INTERFACE || (attr->wTypeFlags & TYPEFLAG_FDUAL))
        hr = CreateStubFromTypeInfo(typeinfo, iid, server, stub);
    else
        hr = dispinterface_create_stub(server, iid, stub);

    if (FAILED(hr))
        ERR("Failed to create stub, hr %#lx.\n", hr);

    ITypeInfo_ReleaseTypeAttr(typeinfo, attr);
    ITypeInfo_Release(typeinfo);
    return hr;
}

static const IPSFactoryBufferVtbl dispatch_typelib_ps_vtbl =
{
    dispatch_typelib_ps_QueryInterface,
    dispatch_typelib_ps_AddRef,
    dispatch_typelib_ps_Release,
    dispatch_typelib_ps_CreateProxy,
    dispatch_typelib_ps_CreateStub,
};

static IPSFactoryBuffer dispatch_typelib_ps = { &dispatch_typelib_ps_vtbl };

extern void _get_STDFONT_CF(LPVOID *);
extern void _get_STDPIC_CF(LPVOID *);

/***********************************************************************
 *		DllGetClassObject (OLEAUT32.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualGUID(rclsid,&CLSID_StdFont)) {
	if (IsEqualGUID(iid,&IID_IClassFactory)) {
	    _get_STDFONT_CF(ppv);
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    }
    if (IsEqualGUID(rclsid,&CLSID_StdPicture)) {
	if (IsEqualGUID(iid,&IID_IClassFactory)) {
	    _get_STDPIC_CF(ppv);
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    }

    if (IsEqualGUID(rclsid, &CLSID_PSDispatch) || IsEqualGUID(rclsid, &CLSID_PSOAInterface))
        return IPSFactoryBuffer_QueryInterface(&dispatch_typelib_ps, iid, ppv);

    if (IsEqualCLSID(rclsid, &CLSID_PSTypeComp) ||
        IsEqualCLSID(rclsid, &CLSID_PSTypeInfo) ||
        IsEqualCLSID(rclsid, &CLSID_PSTypeLib) ||
        IsEqualCLSID(rclsid, &CLSID_PSDispatch) ||
        IsEqualCLSID(rclsid, &CLSID_PSEnumVariant))
        return OLEAUTPS_DllGetClassObject(&CLSID_PSFactoryBuffer, iid, ppv);

    return OLEAUTPS_DllGetClassObject(rclsid, iid, ppv);
}

/*****************************************************************************
 *              DllMain         [OLEAUT32.@]
 */
BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason == DLL_PROCESS_ATTACH)
        bstr_cache_enabled = !GetEnvironmentVariableW(L"oanocache", NULL, 0);

    return OLEAUTPS_DllMain( hInstDll, fdwReason, lpvReserved );
}

/***********************************************************************
 *		DllRegisterServer (OLEAUT32.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return OLEAUTPS_DllRegisterServer();
}

/***********************************************************************
 *		DllUnregisterServer (OLEAUT32.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return OLEAUTPS_DllUnregisterServer();
}

/***********************************************************************
 *              OleIconToCursor (OLEAUT32.415)
 */
HCURSOR WINAPI OleIconToCursor( HINSTANCE hinstExe, HICON hIcon)
{
    FIXME("(%p,%p), partially implemented.\n",hinstExe,hIcon);
    /* FIXME: make an extended conversation from HICON to HCURSOR */
    return CopyCursor(hIcon);
}

/***********************************************************************
 *              GetAltMonthNames (OLEAUT32.@)
 */
HRESULT WINAPI GetAltMonthNames(LCID lcid, LPOLESTR **str)
{
    static const WCHAR *arabic_hijri[] =
    {
        L"\x0645\x062d\x0631\x0645",
        L"\x0635\x0641\x0631",
        L"\x0631\x0628\x064a\x0639 \x0627\x0644\x0627\x0648\x0644",
        L"\x0631\x0628\x064a\x0639 \x0627\x0644\x062b\x0627\x0646\x064a",
        L"\x062c\x0645\x0627\x062f\x0649 \x0627\x0644\x0627\x0648\x0644\x0649",
        L"\x062c\x0645\x0627\x062f\x0649 \x0627\x0644\x062b\x0627\x0646\x064a\x0629",
        L"\x0631\x062c\x0628",
        L"\x0634\x0639\x0628\x0627\x0646",
        L"\x0631\x0645\x0636\x0627\x0646",
        L"\x0634\x0648\x0627\x0643",
        L"\x0630\x0648 \x0627\x0644\x0642\x0639\x062f\x0629",
        L"\x0630\x0648 \x0627\x0644\x062d\x062c\x0629",
        NULL
    };

    static const WCHAR *polish_genitive_names[] =
    {
        L"stycznia",
        L"lutego",
        L"marca",
        L"kwietnia",
        L"maja",
        L"czerwca",
        L"lipca",
        L"sierpnia",
        L"wrze\x015bnia",
        L"pa\x017a" "dziernika",
        L"listopada",
        L"grudnia",
        NULL
    };

    static const WCHAR *russian_genitive_names[] =
    {
        L"\x044f\x043d\x0432\x0430\x0440\x044f",
        L"\x0444\x0435\x0432\x0440\x0430\x043b\x044f",
        L"\x043c\x0430\x0440\x0442\x0430",
        L"\x0430\x043f\x0440\x0435\x043b\x044f",
        L"\x043c\x0430\x044f",
        L"\x0438\x044e\x043d\x044f",
        L"\x0438\x044e\x043b\x044f",
        L"\x0430\x0432\x0433\x0443\x0441\x0442\x0430",
        L"\x0441\x0435\x043d\x0442\x044f\x0431\x0440\x044f",
        L"\x043e\x043a\x0442\x044f\x0431\x0440\x044f",
        L"\x043d\x043e\x044f\x0431\x0440\x044f",
        L"\x0434\x0435\x043a\x0430\x0431\x0440\x044f",
        NULL
    };

    TRACE("%#lx, %p.\n", lcid, str);

    if (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_ARABIC)
        *str = (LPOLESTR *)arabic_hijri;
    else if (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_POLISH)
        *str = (LPOLESTR *)polish_genitive_names;
    else if (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_RUSSIAN)
        *str = (LPOLESTR *)russian_genitive_names;
    else
        *str = NULL;

    return S_OK;
}
