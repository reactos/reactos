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

#include "precomp.h"

#include <initguid.h>
#include <oleaut32_oaidl.h>

#include "typelib.h"

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

static inline bstr_cache_entry_t *get_cache_entry(size_t size)
{
    unsigned cache_idx = FIELD_OFFSET(bstr_t, u.ptr[size-1])/BUCKET_SIZE;
    return bstr_cache_enabled && cache_idx < sizeof(bstr_cache)/sizeof(*bstr_cache)
        ? bstr_cache + cache_idx
        : NULL;
}

static bstr_t *alloc_bstr(size_t size)
{
    bstr_cache_entry_t *cache_entry = get_cache_entry(size+sizeof(WCHAR));
    bstr_t *ret;

    if(cache_entry) {
        EnterCriticalSection(&cs_bstr_cache);

        if(!cache_entry->cnt) {
            cache_entry = get_cache_entry(size+sizeof(WCHAR)+BUCKET_SIZE);
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
                size_t tail;

                memset(ret, ARENA_INUSE_FILLER, FIELD_OFFSET(bstr_t, u.ptr[size+sizeof(WCHAR)]));
                tail = bstr_alloc_size(size) - FIELD_OFFSET(bstr_t, u.ptr[size+sizeof(WCHAR)]);
                if(tail)
                    memset(ret->u.ptr+size+sizeof(WCHAR), ARENA_TAIL_FILLER, tail);
            }
            ret->size = size;
            return ret;
        }
    }

    ret = HeapAlloc(GetProcessHeap(), 0, bstr_alloc_size(size));
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
void WINAPI SysFreeString(BSTR str)
{
    bstr_cache_entry_t *cache_entry;
    bstr_t *bstr;

    if(!str)
        return;

    bstr = bstr_from_str(str);
    cache_entry = get_cache_entry(bstr->size+sizeof(WCHAR));
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

        if(cache_entry->cnt < sizeof(cache_entry->buf)/sizeof(*cache_entry->buf)) {
            cache_entry->buf[(cache_entry->head+cache_entry->cnt) % BUCKET_BUFFER_SIZE] = bstr;
            cache_entry->cnt++;

            if(WARN_ON(heap)) {
                unsigned n = bstr_alloc_size(bstr->size) / sizeof(DWORD) - 1;
                bstr->size = ARENA_FREE_FILLER;
                for(i=0; i<n; i++)
                    bstr->u.dwptr[i] = ARENA_FREE_FILLER;
            }

            LeaveCriticalSection(&cs_bstr_cache);
            return;
        }

        LeaveCriticalSection(&cs_bstr_cache);
    }

    HeapFree(GetProcessHeap(), 0, bstr);
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
	return 0;

    if (*old!=NULL) {
      BSTR old_copy = *old;
      DWORD newbytelen = len*sizeof(WCHAR);
      bstr_t *bstr = HeapReAlloc(GetProcessHeap(),0,((DWORD*)*old)-1,bstr_alloc_size(newbytelen));
      *old = bstr->u.str;
      bstr->size = newbytelen;
      /* Subtle hidden feature: The old string data is still there
       * when 'in' is NULL!
       * Some Microsoft program needs it.
       * FIXME: Is it a sideeffect of BSTR caching?
       */
      if (str && old_copy!=str) memmove(*old, str, newbytelen);
      (*old)[len] = 0;
    } else {
      /*
       * Allocate the new string
       */
      *old = SysAllocStringLen(str, len);
    }

    return 1;
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
BSTR WINAPI SysAllocStringByteLen(LPCSTR str, UINT len)
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
        bstr->u.ptr[len] = bstr->u.ptr[len+1] = 0;
    }else {
        memset(bstr->u.ptr, 0, len+sizeof(WCHAR));
    }

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

static const WCHAR	_delimiter[] = {'!',0}; /* default delimiter apparently */
static const WCHAR	*pdelimiter = &_delimiter[0];

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
	ret = CreateItemMoniker(pdelimiter,guidbuf,&moniker);
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
	ret = CreateItemMoniker(pdelimiter,guidbuf,&moniker);
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
		return MAKELONG(0xffff, 50);
    default:
		FIXME("Version value not known yet. Please investigate it !\n");
		return MAKELONG(0xffff, 40);  /* for now return the same value as for w2k */
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

  TRACE("(%08x, %p, %p)\n", clr, hpal, pColorRef);

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

extern HRESULT WINAPI OLEAUTPS_DllGetClassObject(REFCLSID, REFIID, LPVOID *) DECLSPEC_HIDDEN;
extern BOOL WINAPI OLEAUTPS_DllMain(HINSTANCE, DWORD, LPVOID) DECLSPEC_HIDDEN;
extern HRESULT WINAPI OLEAUTPS_DllRegisterServer(void) DECLSPEC_HIDDEN;
extern HRESULT WINAPI OLEAUTPS_DllUnregisterServer(void) DECLSPEC_HIDDEN;

extern void _get_STDFONT_CF(LPVOID *);
extern void _get_STDPIC_CF(LPVOID *);

static HRESULT WINAPI PSDispatchFacBuf_QueryInterface(IPSFactoryBuffer *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPSFactoryBuffer))
    {
        IPSFactoryBuffer_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI PSDispatchFacBuf_AddRef(IPSFactoryBuffer *iface)
{
    return 2;
}

static ULONG WINAPI PSDispatchFacBuf_Release(IPSFactoryBuffer *iface)
{
    return 1;
}

static HRESULT WINAPI PSDispatchFacBuf_CreateProxy(IPSFactoryBuffer *iface, IUnknown *pUnkOuter, REFIID riid, IRpcProxyBuffer **ppProxy, void **ppv)
{
    IPSFactoryBuffer *pPSFB;
    HRESULT hr;

    if (IsEqualIID(riid, &IID_IDispatch))
        hr = OLEAUTPS_DllGetClassObject(&CLSID_PSFactoryBuffer, &IID_IPSFactoryBuffer, (void **)&pPSFB);
    else
        hr = TMARSHAL_DllGetClassObject(&CLSID_PSOAInterface, &IID_IPSFactoryBuffer, (void **)&pPSFB);

    if (FAILED(hr)) return hr;

    hr = IPSFactoryBuffer_CreateProxy(pPSFB, pUnkOuter, riid, ppProxy, ppv);

    IPSFactoryBuffer_Release(pPSFB);
    return hr;
}

static HRESULT WINAPI PSDispatchFacBuf_CreateStub(IPSFactoryBuffer *iface, REFIID riid, IUnknown *pUnkOuter, IRpcStubBuffer **ppStub)
{
    IPSFactoryBuffer *pPSFB;
    HRESULT hr;

    if (IsEqualIID(riid, &IID_IDispatch))
        hr = OLEAUTPS_DllGetClassObject(&CLSID_PSFactoryBuffer, &IID_IPSFactoryBuffer, (void **)&pPSFB);
    else
        hr = TMARSHAL_DllGetClassObject(&CLSID_PSOAInterface, &IID_IPSFactoryBuffer, (void **)&pPSFB);

    if (FAILED(hr)) return hr;

    hr = IPSFactoryBuffer_CreateStub(pPSFB, riid, pUnkOuter, ppStub);

    IPSFactoryBuffer_Release(pPSFB);
    return hr;
}

static const IPSFactoryBufferVtbl PSDispatchFacBuf_Vtbl =
{
    PSDispatchFacBuf_QueryInterface,
    PSDispatchFacBuf_AddRef,
    PSDispatchFacBuf_Release,
    PSDispatchFacBuf_CreateProxy,
    PSDispatchFacBuf_CreateStub
};

/* This is the whole PSFactoryBuffer object, just the vtableptr */
static const IPSFactoryBufferVtbl *pPSDispatchFacBuf = &PSDispatchFacBuf_Vtbl;

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
    if (IsEqualCLSID(rclsid, &CLSID_PSDispatch) && IsEqualIID(iid, &IID_IPSFactoryBuffer)) {
        *ppv = &pPSDispatchFacBuf;
        IPSFactoryBuffer_AddRef((IPSFactoryBuffer *)*ppv);
        return S_OK;
    }
    if (IsEqualGUID(rclsid,&CLSID_PSOAInterface)) {
	if (S_OK==TMARSHAL_DllGetClassObject(rclsid,iid,ppv))
	    return S_OK;
	/*FALLTHROUGH*/
    }
    if (IsEqualCLSID(rclsid, &CLSID_PSTypeInfo) ||
        IsEqualCLSID(rclsid, &CLSID_PSTypeLib) ||
        IsEqualCLSID(rclsid, &CLSID_PSDispatch) ||
        IsEqualCLSID(rclsid, &CLSID_PSEnumVariant))
        return OLEAUTPS_DllGetClassObject(&CLSID_PSFactoryBuffer, iid, ppv);

    return OLEAUTPS_DllGetClassObject(rclsid, iid, ppv);
}

/***********************************************************************
 *		DllCanUnloadNow (OLEAUT32.@)
 *
 * Determine if this dll can be unloaded from the callers address space.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Always returns S_FALSE. This dll cannot be unloaded.
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/*****************************************************************************
 *              DllMain         [OLEAUT32.@]
 */
BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
    static const WCHAR oanocacheW[] = {'o','a','n','o','c','a','c','h','e',0};

    if(fdwReason == DLL_PROCESS_ATTACH)
        bstr_cache_enabled = !GetEnvironmentVariableW(oanocacheW, NULL, 0);

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
