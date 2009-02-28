/* $Id: stubs.c 28533 2007-08-24 22:44:36Z greatlrd $
 *
 * reactos/lib/gdi32/misc/eng.c
 *
 * GDI32.DLL eng part
 *
 *
 */

#include "precomp.h"

/*
 * @implemented
 */
VOID
WINAPI
EngAcquireSemaphore ( IN HSEMAPHORE hsem )
{
    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)hsem);
}


/*
 * @unimplemented
 */
BOOL
copy_my_glyphset( FD_GLYPHSET *dst_glyphset , FD_GLYPHSET * src_glyphset, ULONG Size)
{
    BOOL retValue = FALSE;

    memcpy(src_glyphset, dst_glyphset, Size);
    if (src_glyphset->cRuns == 0)
    {
        retValue = TRUE;
    }

    /* FIXME copy wrun */
    return retValue;
}

FD_GLYPHSET* WINAPI
EngComputeGlyphSet(INT nCodePage,INT nFirstChar,INT cChars)
{
    FD_GLYPHSET * ntfd_glyphset;
    FD_GLYPHSET * myfd_glyphset = NULL;

    ntfd_glyphset = NtGdiEngComputeGlyphSet(nCodePage,nFirstChar,cChars);

    if (!ntfd_glyphset)
    {
        if (ntfd_glyphset->cjThis)
        {
            myfd_glyphset = GlobalAlloc(0,ntfd_glyphset->cjThis);

            if (!myfd_glyphset)
            {
                if (copy_my_glyphset(myfd_glyphset,ntfd_glyphset,ntfd_glyphset->cjThis) == FALSE)
                {
                    GlobalFree(myfd_glyphset);
                    myfd_glyphset = NULL;
                }
            }
        }
    }
    return myfd_glyphset;
}

/*
 * @implemented
 */
HSEMAPHORE
WINAPI
EngCreateSemaphore ( VOID )
{
    PRTL_CRITICAL_SECTION CritSect = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(RTL_CRITICAL_SECTION));
    if (!CritSect)
    {
        return NULL;
    }

    RtlInitializeCriticalSection( CritSect );
    return (HSEMAPHORE)CritSect;
}

/*
 * @implemented
 */
VOID
WINAPI
EngDeleteSemaphore ( IN HSEMAPHORE hsem )
{
    if (hsem)
    {
        RtlDeleteCriticalSection( (PRTL_CRITICAL_SECTION) hsem );
        RtlFreeHeap( GetProcessHeap(), 0, hsem );
    }
}

/*
 * @implemented
 */
PVOID WINAPI
EngFindResource(HANDLE h,
                int iName,
                int iType,
                PULONG pulSize)
{
    HRSRC HRSrc;
    DWORD Size = 0;
    HGLOBAL Hg;
    LPVOID Lock = NULL;

    if ((HRSrc = FindResourceW( (HMODULE) h, MAKEINTRESOURCEW(iName), MAKEINTRESOURCEW(iType))))
    {
        if ((Size = SizeofResource( (HMODULE) h, HRSrc )))
        {
            if ((Hg = LoadResource( (HMODULE) h, HRSrc )))
            {
                Lock = LockResource( Hg );
            }
        }
    }

    *pulSize = Size;
    return (PVOID) Lock;
}

/*
 * @implemented
 */
VOID WINAPI
EngFreeModule(HANDLE h)
{
    FreeLibrary(h);
}

/*
 * @implemented
 */

VOID WINAPI
EngGetCurrentCodePage( OUT PUSHORT OemCodePage,
                       OUT PUSHORT AnsiCodePage)
{
    *OemCodePage  = GetOEMCP();
    *AnsiCodePage = GetACP();
}


/*
 * @implemented
 */
LPWSTR WINAPI
EngGetDriverName(HDEV hdev)
{
  // DHPDEV from NtGdiGetDhpdev must be from print driver.
  PUMPDEV pPDev = (PUMPDEV)NtGdiGetDhpdev(hdev);

  if (!pPDev) return NULL;
  
  if (pPDev->Sig != PDEV_UMPD_ID)
  {
     pPDev = (PUMPDEV)pPDev->Sig;
  }
  return pPDev->pdi5Info->pDriverPath;
}

/*
 * @implemented
 */
LPWSTR WINAPI
EngGetPrinterDataFileName(HDEV hdev)
{
  PUMPDEV pPDev = (PUMPDEV)NtGdiGetDhpdev(hdev);

  if (!pPDev) return NULL;

  if (pPDev->Sig != PDEV_UMPD_ID)
  {
     pPDev = (PUMPDEV)pPDev->Sig;
  }
  return pPDev->pdi5Info->pDataFile;
}

/*
 * @implemented
 */
HANDLE WINAPI
EngLoadModule(LPWSTR pwsz)
{
   return LoadLibraryExW ( pwsz, NULL, LOAD_LIBRARY_AS_DATAFILE);
}

/*
 * @implemented
 */
INT WINAPI
EngMultiByteToWideChar(UINT CodePage,
                       LPWSTR WideCharString,
                       INT BytesInWideCharString,
                       LPSTR MultiByteString,
                       INT BytesInMultiByteString)
{
  return MultiByteToWideChar(CodePage,0,MultiByteString,BytesInMultiByteString,WideCharString,BytesInWideCharString / sizeof(WCHAR));
}

/*
 * @implemented
 */
VOID WINAPI
EngQueryLocalTime(PENG_TIME_FIELDS etf)
{
  SYSTEMTIME SystemTime;
  GetLocalTime( &SystemTime );
  etf->usYear    = SystemTime.wYear;
  etf->usMonth   = SystemTime.wMonth;
  etf->usWeekday = SystemTime.wDayOfWeek;
  etf->usDay     = SystemTime.wDay;
  etf->usHour    = SystemTime.wHour;
  etf->usMinute  = SystemTime.wMinute;
  etf->usSecond  = SystemTime.wSecond;
  etf->usMilliseconds = SystemTime.wMilliseconds;
}

/*
 * @implemented
 */
VOID
WINAPI
EngReleaseSemaphore ( IN HSEMAPHORE hsem )
{
  RtlLeaveCriticalSection( (PRTL_CRITICAL_SECTION) hsem);
}




/*
 * @implemented
 */
INT
WINAPI
EngWideCharToMultiByte( UINT CodePage,
                        LPWSTR WideCharString,
                        INT BytesInWideCharString,
                        LPSTR MultiByteString,
                        INT BytesInMultiByteString)
{
  return WideCharToMultiByte(CodePage, 0, WideCharString, (BytesInWideCharString/sizeof(WCHAR)),
                             MultiByteString, BytesInMultiByteString, NULL, NULL);
}
