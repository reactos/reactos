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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#include "ole2.h"
#include "olectl.h"
#include "oleauto.h"

#include "tmarshal.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* The OLE Automation ProxyStub Interface Class (aka Typelib Marshaler) */
extern const GUID CLSID_PSOAInterface;

/* IDispatch marshaler */
extern const GUID CLSID_PSDispatch;

static BOOL BSTR_bCache = TRUE; /* Cache allocations to minimise alloc calls? */

HMODULE OLEAUT32_hModule = NULL;


#ifndef COLOR_GRADIENTINACTIVECAPTION
/*FIXME There are missing some constants in MinGW: */
#if(WINVER >= 0x0500)
#define COLOR_HOTLIGHT                  26
#define COLOR_GRADIENTACTIVECAPTION     27
#define COLOR_GRADIENTINACTIVECAPTION   28
#endif /* WINVER >= 0x0500 */
#endif


/******************************************************************************
 * BSTR  {OLEAUT32}
 *
 * NOTES
 *  BSTR is a simple typedef for a wide-character string used as the principle
 *  string type in ole automation. When encapsulated in a Variant type they are
 *  automatically copied and destroyed as the variant is processed.
 *
 *  The low level BSTR Api allows manipulation of these strings and is used by
 *  higher level Api calls to manage the strings transparently to the caller.
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
unsigned/*FIXME: wrong return type of SysStringLen() in MinGW header */
int WINAPI SysStringLen(BSTR str)
{
    DWORD* bufferPointer;

     if (!str) return 0;
    /*
     * The length of the string (in bytes) is contained in a DWORD placed
     * just before the BSTR pointer
     */
    bufferPointer = (DWORD*)str;

    bufferPointer--;

    return (int)(*bufferPointer/sizeof(WCHAR));
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
unsigned/*FIXME: wrong return type of SysStringLen() in MinGW header */
int WINAPI SysStringByteLen(BSTR str)
{
    DWORD* bufferPointer;

     if (!str) return 0;
    /*
     * The length of the string (in bytes) is contained in a DWORD placed
     * just before the BSTR pointer
     */
    bufferPointer = (DWORD*)str;

    bufferPointer--;

    return (int)(*bufferPointer);
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
    DWORD* bufferPointer;

    /* NULL is a valid parameter */
    if(!str) return;

    /*
     * We have to be careful when we free a BSTR pointer, it points to
     * the beginning of the string but it skips the byte count contained
     * before the string.
     */
    bufferPointer = (DWORD*)str;

    bufferPointer--;

    /*
     * Free the memory from its "real" origin.
     */
    HeapFree(GetProcessHeap(), 0, bufferPointer);
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
    DWORD  bufferSize;
    DWORD* newBuffer;
    WCHAR* stringBuffer;

    /*
     * Find the length of the buffer passed-in in bytes.
     */
    bufferSize = len * sizeof (WCHAR);

    /*
     * Allocate a new buffer to hold the string.
     * don't forget to keep an empty spot at the beginning of the
     * buffer for the character count and an extra character at the
     * end for the NULL.
     */
    newBuffer = (DWORD*)HeapAlloc(GetProcessHeap(),
                                 0,
                                 bufferSize + sizeof(WCHAR) + sizeof(DWORD));

    /*
     * If the memory allocation failed, return a null pointer.
     */
    if (newBuffer==0)
      return 0;

    /*
     * Copy the length of the string in the placeholder.
     */
    *newBuffer = bufferSize;

    /*
     * Skip the byte count.
     */
    newBuffer++;

    /*
     * Copy the information in the buffer.
     * Since it is valid to pass a NULL pointer here, we'll initialize the
     * buffer to nul if it is the case.
     */
    if (str != 0)
      memcpy(newBuffer, str, bufferSize);
    else
      memset(newBuffer, 0, bufferSize);

    /*
     * Make sure that there is a nul character at the end of the
     * string.
     */
    stringBuffer = (WCHAR*)newBuffer;
    stringBuffer[len] = L'\0';

    return (LPWSTR)stringBuffer;
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
 *  *pbstr may be changed by this function.
 */
int WINAPI SysReAllocStringLen(BSTR* old, const OLECHAR* str, unsigned int len)
{
    if (*old!=NULL) {
      DWORD newbytelen = len*sizeof(WCHAR);
      DWORD *ptr = HeapReAlloc(GetProcessHeap(),0,((DWORD*)*old)-1,newbytelen+sizeof(WCHAR)+sizeof(DWORD));
      *old = (BSTR)(ptr+1);
      *ptr = newbytelen;
      if (str) {
        memcpy(*old, str, newbytelen);
        (*old)[len] = 0;
      } else {
	/* Subtle hidden feature: The old string data is still there
	 * when 'in' is NULL!
	 * Some Microsoft program needs it.
	 */
      }
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
    DWORD* newBuffer;
    char* stringBuffer;

    /*
     * Allocate a new buffer to hold the string.
     * don't forget to keep an empty spot at the beginning of the
     * buffer for the character count and an extra character at the
     * end for the NULL.
     */
    newBuffer = (DWORD*)HeapAlloc(GetProcessHeap(),
                                 0,
                                 len + sizeof(WCHAR) + sizeof(DWORD));

    /*
     * If the memory allocation failed, return a null pointer.
     */
    if (newBuffer==0)
      return 0;

    /*
     * Copy the length of the string in the placeholder.
     */
    *newBuffer = len;

    /*
     * Skip the byte count.
     */
    newBuffer++;

    /*
     * Copy the information in the buffer.
     * Since it is valid to pass a NULL pointer here, we'll initialize the
     * buffer to nul if it is the case.
     */
    if (str != 0)
      memcpy(newBuffer, str, len);

    /*
     * Make sure that there is a nul character at the end of the
     * string.
     */
    stringBuffer = (char *)newBuffer;
    stringBuffer[len] = 0;
    stringBuffer[len+1] = 0;

    return (LPWSTR)stringBuffer;
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
    if (*old!=NULL)
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
 *  See BSTR.
 */
void WINAPI SetOaNoCache(void)
{
  BSTR_bCache = FALSE;
}

static WCHAR	_delimiter[2] = {'!',0}; /* default delimiter apparently */
static WCHAR	*pdelimiter = &_delimiter[0];

/***********************************************************************
 *		RegisterActiveObject (OLEAUT32.33)
 */
HRESULT WINAPI RegisterActiveObject(
	LPUNKNOWN punk,REFCLSID rcid,DWORD dwFlags,LPDWORD pdwRegister
) {
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
	ret = IRunningObjectTable_Register(runobtable,dwFlags,punk,moniker,pdwRegister);
	IRunningObjectTable_Release(runobtable);
	IMoniker_Release(moniker);
	return ret;
}

/***********************************************************************
 *		RevokeActiveObject (OLEAUT32.34)
 */
HRESULT WINAPI RevokeActiveObject(DWORD xregister,LPVOID reserved)
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
 */
HRESULT WINAPI GetActiveObject(REFCLSID rcid,LPVOID preserved,LPUNKNOWN *ppunk)
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
ULONG WINAPI OaBuildVersion()
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
    default:
		ERR("Version value not known yet. Please investigate it !\n");
		return 0x0;
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

  TRACE("(%08lx, %p, %p):stub\n", clr, hpal, pColorRef);

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
      if ((index < COLOR_SCROLLBAR) || (index > COLOR_GRADIENTINACTIVECAPTION))
        return E_INVALIDARG;

      *pColorRef =  GetSysColor(index);

      break;
    }

    default:
      return E_INVALIDARG;
  }

  return S_OK;
}

extern HRESULT OLEAUTPS_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

extern void _get_STDFONT_CF(LPVOID);
extern void _get_STDPIC_CF(LPVOID);

/***********************************************************************
 *		DllGetClassObject (OLEAUT32.1)
 */
#ifndef __REACTOS__ /*FIXME: no marshalling yet*/
HRESULT WINAPI OLEAUT32_DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
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
    if (IsEqualGUID(rclsid,&CLSID_PSDispatch)) {
	return OLEAUTPS_DllGetClassObject(rclsid,iid,ppv);
    }
    if (IsEqualGUID(rclsid,&CLSID_PSOAInterface)) {
	if (S_OK==TypeLibFac_DllGetClassObject(rclsid,iid,ppv))
	    return S_OK;
	/*FALLTHROUGH*/
    }
    FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
#endif

/***********************************************************************
 *		DllCanUnloadNow (OLEAUT32.410)
 *
 * Determine if this dll can be unloaded from the callers address space.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Always returns S_FALSE. This dll cannot be unloaded.
 */
HRESULT WINAPI OLEAUT32_DllCanUnloadNow()
{
    return S_FALSE;
}

/*****************************************************************************
 *              DllMain         [OLEAUT32.@]
 */
BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
  TRACE("(%p,%lu,%p)\n", hInstDll, fdwReason, lpvReserved);

  switch (fdwReason) {
  case DLL_PROCESS_ATTACH:
    DisableThreadLibraryCalls(hInstDll);
    OLEAUT32_hModule = (HMODULE)hInstDll;
    break;
  case DLL_PROCESS_DETACH:
    break;
  };

  return TRUE;
}
