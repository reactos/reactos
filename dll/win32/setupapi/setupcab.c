/* 
 * Setupapi cabinet routines
 *
 * Copyright 2003 Gregory M. Turner
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
 *
 *
 * Many useful traces are commented in code, uncomment them if you have
 * trouble and run with WINEDEBUG=+setupapi
 * 
 */

#include "setupapi_private.h"

#include <fcntl.h>
#include <share.h>
#include <fdi.h>

HINSTANCE SETUPAPI_hInstance = NULL;
OSVERSIONINFOEXW OsVersionInfo;

static HINSTANCE CABINET_hInstance = NULL;

static HFDI (__cdecl *sc_FDICreate)(PFNALLOC, PFNFREE, PFNOPEN,
                PFNREAD, PFNWRITE, PFNCLOSE, PFNSEEK, int, PERF);

static BOOL (__cdecl *sc_FDICopy)(HFDI, char *, char *, int,
                PFNFDINOTIFY, PFNFDIDECRYPT, void *);

static BOOL (__cdecl *sc_FDIDestroy)(HFDI);

#define SC_HSC_A_MAGIC 0xACABFEED
typedef struct {
  UINT magic;
  HFDI hfdi;
  PSP_FILE_CALLBACK_A msghandler;
  PVOID context;
  CHAR most_recent_cabinet_name[MAX_PATH];
  CHAR most_recent_target[MAX_PATH];
} SC_HSC_A, *PSC_HSC_A;

#define SC_HSC_W_MAGIC 0x0CABFEED
typedef struct {
  UINT magic;
  HFDI hfdi;
  PSP_FILE_CALLBACK_W msghandler;
  PVOID context;
  WCHAR most_recent_cabinet_name[MAX_PATH];
  WCHAR most_recent_target[MAX_PATH];
} SC_HSC_W, *PSC_HSC_W;

static BOOL LoadCABINETDll(void)
{
  if (!CABINET_hInstance) {
    CABINET_hInstance = LoadLibraryA("cabinet.dll");
    if (CABINET_hInstance)  {
      sc_FDICreate = (void *)GetProcAddress(CABINET_hInstance, "FDICreate");
      sc_FDICopy = (void *)GetProcAddress(CABINET_hInstance, "FDICopy");
      sc_FDIDestroy = (void *)GetProcAddress(CABINET_hInstance, "FDIDestroy");
      return TRUE;
    } else {
      ERR("load cabinet dll failed.\n");
      return FALSE;
    }
  } else
    return TRUE;
}

/* FDICreate callbacks */

static void * CDECL sc_cb_alloc(ULONG cb)
{
  return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void CDECL sc_cb_free(void *pv)
{
  HeapFree(GetProcessHeap(), 0, pv);
}

static INT_PTR CDECL sc_cb_open(char *pszFile, int oflag, int pmode)
{
  DWORD creation = 0, sharing = 0;
  int ioflag = 0;
  INT_PTR ret = 0;
  SECURITY_ATTRIBUTES sa;

  /* TRACE("(pszFile == %s, oflag == %d, pmode == %d)\n", debugstr_a(pszFile), oflag, pmode); */

  switch(oflag & (_O_RDONLY | _O_WRONLY | _O_RDWR)) {
  case _O_RDONLY:
    ioflag |= GENERIC_READ;
    break;
  case _O_WRONLY:
    ioflag |= GENERIC_WRITE;
    break;
  case _O_RDWR:
    ioflag |= GENERIC_READ | GENERIC_WRITE;
    break;
  case _O_WRONLY | _O_RDWR: /* hmmm.. */
    ERR("_O_WRONLY & _O_RDWR in oflag?\n");
    return -1;
  }

  if (oflag & _O_CREAT) {
    if (oflag & _O_EXCL)
      creation = CREATE_NEW;
    else if (oflag & _O_TRUNC)
      creation = CREATE_ALWAYS;
    else
      creation = OPEN_ALWAYS;
  } else  /* no _O_CREAT */ {
    if (oflag & _O_TRUNC)
      creation = TRUNCATE_EXISTING;
    else
      creation = OPEN_EXISTING;
  }

  switch( pmode & 0x70 ) {
    case _SH_DENYRW:
      sharing = 0L;
      break;
    case _SH_DENYWR:
      sharing = FILE_SHARE_READ;
      break;
    case _SH_DENYRD:
      sharing = FILE_SHARE_WRITE;
      break;
    case _SH_COMPAT:
    case _SH_DENYNO:
      sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;
    default:
      ERR("<-- -1 (Unhandled pmode 0x%x)\n", pmode);
      return -1;
  }

  if (oflag & ~(_O_BINARY | _O_TRUNC | _O_EXCL | _O_CREAT | _O_RDWR | _O_WRONLY | _O_NOINHERIT))
    WARN("unsupported oflag 0x%04x\n",oflag);

  sa.nLength              = sizeof( SECURITY_ATTRIBUTES );
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle       = !(ioflag & _O_NOINHERIT);

  ret = (INT_PTR) CreateFileA(pszFile, ioflag, sharing, &sa, creation, FILE_ATTRIBUTE_NORMAL, NULL);

  /* TRACE("<-- %d\n", ret); */

  return ret;
}

static UINT CDECL sc_cb_read(INT_PTR hf, void *pv, UINT cb)
{
  DWORD num_read;
  BOOL rslt;

  /* TRACE("(hf == %d, pv == ^%p, cb == %u)\n", hf, pv, cb); */

  rslt = ReadFile((HANDLE) hf, pv, cb, &num_read, NULL);


  /* eof and failure both give "-1" return */
  if ((! rslt) || ((cb > 0) && (num_read == 0))) {
    /* TRACE("<-- -1\n"); */
    return -1;
  }

  /* TRACE("<-- %lu\n", num_read); */
  return num_read;
}

static UINT CDECL sc_cb_write(INT_PTR hf, void *pv, UINT cb)
{
  DWORD num_written;
  /* BOOL rv; */

  /* TRACE("(hf == %d, pv == ^%p, cb == %u)\n", hf, pv, cb); */

  if ( /* (rv = */ WriteFile((HANDLE) hf, pv, cb, &num_written, NULL) /* ) */
       && (num_written == cb)) {
    /* TRACE("<-- %lu\n", num_written); */
    return num_written;
  } else {
    /* TRACE("rv == %d, num_written == %lu, cb == %u\n", rv, num_written,cb); */
    /* TRACE("<-- -1\n"); */
    return -1;
  }
}

static int CDECL sc_cb_close(INT_PTR hf)
{
  /* TRACE("(hf == %d)\n", hf); */

  if (CloseHandle((HANDLE) hf))
    return 0;
  else
    return -1;
}

static LONG CDECL sc_cb_lseek(INT_PTR hf, LONG dist, int seektype)
{
  DWORD ret;

  /* TRACE("(hf == %d, dist == %ld, seektype == %d)\n", hf, dist, seektype); */

  if (seektype < 0 || seektype > 2)
    return -1;

  if (((ret = SetFilePointer((HANDLE) hf, dist, NULL, seektype)) != INVALID_SET_FILE_POINTER) || !GetLastError()) {
    /* TRACE("<-- %lu\n", ret); */
    return ret;
  } else {
    /* TRACE("<-- -1\n"); */
    return -1;
  }
}

#define SIZEOF_MYSTERIO (MAX_PATH*3)

/* FDICopy callbacks */

static INT_PTR CDECL sc_FNNOTIFY_A(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
  FILE_IN_CABINET_INFO_A fici;
  PSC_HSC_A phsc;
  CABINET_INFO_A ci;
  FILEPATHS_A fp;
  UINT err;

  CHAR mysterio[SIZEOF_MYSTERIO]; /* how big? undocumented! probably 256... */

  memset(mysterio, 0, SIZEOF_MYSTERIO);

  TRACE("(fdint == %d, pfdin == ^%p)\n", fdint, pfdin);

  if (pfdin && pfdin->pv && (((PSC_HSC_A) pfdin->pv)->magic == SC_HSC_A_MAGIC))
    phsc = pfdin->pv;
  else {
    ERR("pv %p is not an SC_HSC_A.\n", (pfdin) ? pfdin->pv : NULL);
    return -1;
  }

  switch (fdint) {
  case fdintCABINET_INFO:
    TRACE("Cabinet info notification\n");
    /* TRACE("  Cabinet name: %s\n", debugstr_a(pfdin->psz1));
    TRACE("  Cabinet disk: %s\n", debugstr_a(pfdin->psz2));
    TRACE("  Cabinet path: %s\n", debugstr_a(pfdin->psz3));
    TRACE("  Cabinet Set#: %d\n", pfdin->setID);
    TRACE("  Cabinet Cab#: %d\n", pfdin->iCabinet); */
    WARN("SPFILENOTIFY_CABINETINFO undocumented: guess implementation.\n");
    ci.CabinetFile = phsc->most_recent_cabinet_name;
    ci.CabinetPath = pfdin->psz3;
    ci.DiskName = pfdin->psz2;
    ci.SetId = pfdin->setID;
    ci.CabinetNumber = pfdin->iCabinet;
    phsc->msghandler(phsc->context, SPFILENOTIFY_CABINETINFO, (UINT_PTR) &ci, 0);
    return 0;
  case fdintPARTIAL_FILE:
    TRACE("Partial file notification\n");
    /* TRACE("  Partial file name: %s\n", debugstr_a(pfdin->psz1)); */
    return 0;
  case fdintCOPY_FILE:
    TRACE("Copy file notification\n");
    TRACE("  File name: %s\n", debugstr_a(pfdin->psz1));
    /* TRACE("  File size: %ld\n", pfdin->cb);
    TRACE("  File date: %u\n", pfdin->date);
    TRACE("  File time: %u\n", pfdin->time);
    TRACE("  File attr: %u\n", pfdin->attribs); */
    fici.NameInCabinet = pfdin->psz1;
    fici.FileSize = pfdin->cb;
    fici.Win32Error = 0;
    fici.DosDate = pfdin->date;
    fici.DosTime = pfdin->time;
    fici.DosAttribs = pfdin->attribs;
    memset(fici.FullTargetName, 0, MAX_PATH);
    err = phsc->msghandler(phsc->context, SPFILENOTIFY_FILEINCABINET,
                           (UINT_PTR)&fici, (UINT_PTR)pfdin->psz1);
    if (err == FILEOP_DOIT) {
      TRACE("  Callback specified filename: %s\n", debugstr_a(fici.FullTargetName));
      if (!fici.FullTargetName[0]) {
        WARN("  Empty return string causing abort.\n");
        SetLastError(ERROR_PATH_NOT_FOUND);
        return -1;
      }
      strcpy( phsc->most_recent_target, fici.FullTargetName );
      return sc_cb_open(fici.FullTargetName, _O_BINARY | _O_CREAT | _O_WRONLY,  _S_IREAD | _S_IWRITE);
    } else {
      TRACE("  Callback skipped file.\n");
      return 0;
    }
  case fdintCLOSE_FILE_INFO:
    TRACE("Close file notification\n");
    /* TRACE("  File name: %s\n", debugstr_a(pfdin->psz1));
    TRACE("  Exec file? %s\n", (pfdin->cb) ? "Yes" : "No");
    TRACE("  File hndl: %d\n", pfdin->hf); */
    fp.Source = phsc->most_recent_cabinet_name;
    fp.Target = phsc->most_recent_target;
    fp.Win32Error = 0;
    fp.Flags = 0;
    /* the following should be a fixme -- but it occurs too many times */
    WARN("Should set file date/time/attribs (and execute files?)\n");
    if (sc_cb_close(pfdin->hf))
      WARN("_close failed.\n");
    err = phsc->msghandler(phsc->context, SPFILENOTIFY_FILEEXTRACTED, (UINT_PTR)&fp, 0);
    if (err) {
      SetLastError(err);
      return FALSE;
    } else
      return TRUE;
  case fdintNEXT_CABINET:
    TRACE("Next cabinet notification\n");
    /* TRACE("  Cabinet name: %s\n", debugstr_a(pfdin->psz1));
    TRACE("  Cabinet disk: %s\n", debugstr_a(pfdin->psz2));
    TRACE("  Cabinet path: %s\n", debugstr_a(pfdin->psz3));
    TRACE("  Cabinet Set#: %d\n", pfdin->setID);
    TRACE("  Cabinet Cab#: %d\n", pfdin->iCabinet); */
    ci.CabinetFile = pfdin->psz1;
    ci.CabinetPath = pfdin->psz3;
    ci.DiskName = pfdin->psz2;
    ci.SetId = pfdin->setID;
    ci.CabinetNumber = pfdin->iCabinet;
    /* remember the new cabinet name */
    strcpy(phsc->most_recent_cabinet_name, pfdin->psz1);
    err = phsc->msghandler(phsc->context, SPFILENOTIFY_NEEDNEWCABINET, (UINT_PTR)&ci, (UINT_PTR)mysterio);
    if (err) {
      SetLastError(err);
      return -1;
    } else {
      if (mysterio[0]) {
        /* some easy paranoia.  no such carefulness exists on the wide API IIRC */
        lstrcpynA(pfdin->psz3, mysterio, SIZEOF_MYSTERIO);
      }
      return 0;
    }
  default:
    FIXME("Unknown notification type %d.\n", fdint);
    return 0;
  }
}

static INT_PTR CDECL sc_FNNOTIFY_W(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
  FILE_IN_CABINET_INFO_W fici;
  PSC_HSC_W phsc;
  CABINET_INFO_W ci;
  FILEPATHS_W fp;
  UINT err;
  int len;

  WCHAR mysterio[SIZEOF_MYSTERIO]; /* how big? undocumented! */
  WCHAR buf[MAX_PATH], buf2[MAX_PATH];
  CHAR charbuf[MAX_PATH];

  memset(mysterio, 0, SIZEOF_MYSTERIO * sizeof(WCHAR));
  memset(buf, 0, MAX_PATH * sizeof(WCHAR));
  memset(buf2, 0, MAX_PATH * sizeof(WCHAR));
  memset(charbuf, 0, MAX_PATH);

  TRACE("(fdint == %d, pfdin == ^%p)\n", fdint, pfdin);

  if (pfdin && pfdin->pv && (((PSC_HSC_W) pfdin->pv)->magic == SC_HSC_W_MAGIC))
    phsc = pfdin->pv;
  else {
    ERR("pv %p is not an SC_HSC_W.\n", (pfdin) ? pfdin->pv : NULL);
    return -1;
  }

  switch (fdint) {
  case fdintCABINET_INFO:
    TRACE("Cabinet info notification\n");
    /* TRACE("  Cabinet name: %s\n", debugstr_a(pfdin->psz1));
    TRACE("  Cabinet disk: %s\n", debugstr_a(pfdin->psz2));
    TRACE("  Cabinet path: %s\n", debugstr_a(pfdin->psz3));
    TRACE("  Cabinet Set#: %d\n", pfdin->setID);
    TRACE("  Cabinet Cab#: %d\n", pfdin->iCabinet); */
    WARN("SPFILENOTIFY_CABINETINFO undocumented: guess implementation.\n");
    ci.CabinetFile = phsc->most_recent_cabinet_name;
    len = 1 + MultiByteToWideChar(CP_ACP, 0, pfdin->psz3, -1, buf, MAX_PATH);
    if ((len > MAX_PATH) || (len <= 1))
      buf[0] = '\0';
    ci.CabinetPath = buf;
    len = 1 + MultiByteToWideChar(CP_ACP, 0, pfdin->psz2, -1, buf2, MAX_PATH);
    if ((len > MAX_PATH) || (len <= 1))
      buf2[0] = '\0';
    ci.DiskName = buf2;
    ci.SetId = pfdin->setID;
    ci.CabinetNumber = pfdin->iCabinet;
    phsc->msghandler(phsc->context, SPFILENOTIFY_CABINETINFO, (UINT_PTR)&ci, 0);
    return 0;
  case fdintPARTIAL_FILE:
    TRACE("Partial file notification\n");
    /* TRACE("  Partial file name: %s\n", debugstr_a(pfdin->psz1)); */
    return 0;
  case fdintCOPY_FILE:
    TRACE("Copy file notification\n");
    TRACE("  File name: %s\n", debugstr_a(pfdin->psz1));
    /* TRACE("  File size: %ld\n", pfdin->cb);
    TRACE("  File date: %u\n", pfdin->date);
    TRACE("  File time: %u\n", pfdin->time);
    TRACE("  File attr: %u\n", pfdin->attribs); */
    len = 1 + MultiByteToWideChar(CP_ACP, 0, pfdin->psz1, -1, buf2, MAX_PATH);
    if ((len > MAX_PATH) || (len <= 1))
      buf2[0] = '\0';
    fici.NameInCabinet = buf2;
    fici.FileSize = pfdin->cb;
    fici.Win32Error = 0;
    fici.DosDate = pfdin->date;
    fici.DosTime = pfdin->time;
    fici.DosAttribs = pfdin->attribs;
    memset(fici.FullTargetName, 0, MAX_PATH * sizeof(WCHAR));
    err = phsc->msghandler(phsc->context, SPFILENOTIFY_FILEINCABINET,
                           (UINT_PTR)&fici, (UINT_PTR)pfdin->psz1);
    if (err == FILEOP_DOIT) {
      TRACE("  Callback specified filename: %s\n", debugstr_w(fici.FullTargetName));
      if (fici.FullTargetName[0]) {
        len = strlenW(fici.FullTargetName) + 1;
        if ((len > MAX_PATH ) || (len <= 1))
          return 0;
        if (!WideCharToMultiByte(CP_ACP, 0, fici.FullTargetName, len, charbuf, MAX_PATH, 0, 0))
          return 0;
      } else {
        WARN("Empty buffer string caused abort.\n");
        SetLastError(ERROR_PATH_NOT_FOUND);
        return -1;
      }
      strcpyW( phsc->most_recent_target, fici.FullTargetName );
      return sc_cb_open(charbuf, _O_BINARY | _O_CREAT | _O_WRONLY,  _S_IREAD | _S_IWRITE);
    } else {
      TRACE("  Callback skipped file.\n");
      return 0;
    }
  case fdintCLOSE_FILE_INFO:
    TRACE("Close file notification\n");
    /* TRACE("  File name: %s\n", debugstr_a(pfdin->psz1));
    TRACE("  Exec file? %s\n", (pfdin->cb) ? "Yes" : "No");
    TRACE("  File hndl: %d\n", pfdin->hf); */
    fp.Source = phsc->most_recent_cabinet_name;
    fp.Target = phsc->most_recent_target;
    fp.Win32Error = 0;
    fp.Flags = 0;
    /* a valid fixme -- but occurs too many times */
    /* FIXME("Should set file date/time/attribs (and execute files?)\n"); */
    if (sc_cb_close(pfdin->hf))
      WARN("_close failed.\n");
    err = phsc->msghandler(phsc->context, SPFILENOTIFY_FILEEXTRACTED, (UINT_PTR)&fp, 0);
    if (err) {
      SetLastError(err);
      return FALSE;
    } else
      return TRUE;
  case fdintNEXT_CABINET:
    TRACE("Next cabinet notification\n");
    /* TRACE("  Cabinet name: %s\n", debugstr_a(pfdin->psz1));
    TRACE("  Cabinet disk: %s\n", debugstr_a(pfdin->psz2));
    TRACE("  Cabinet path: %s\n", debugstr_a(pfdin->psz3));
    TRACE("  Cabinet Set#: %d\n", pfdin->setID);
    TRACE("  Cabinet Cab#: %d\n", pfdin->iCabinet); */
    /* remember the new cabinet name */
    len = 1 + MultiByteToWideChar(CP_ACP, 0, pfdin->psz1, -1, phsc->most_recent_cabinet_name, MAX_PATH);
    if ((len > MAX_PATH) || (len <= 1))
      phsc->most_recent_cabinet_name[0] = '\0';
    ci.CabinetFile = phsc->most_recent_cabinet_name;
    len = 1 + MultiByteToWideChar(CP_ACP, 0, pfdin->psz3, -1, buf, MAX_PATH);
    if ((len > MAX_PATH) || (len <= 1))
      buf[0] = '\0';
    ci.CabinetPath = buf;
    len = 1 + MultiByteToWideChar(CP_ACP, 0, pfdin->psz2, -1, buf2, MAX_PATH);
    if ((len > MAX_PATH) || (len <= 1))
      buf2[0] = '\0';
    ci.DiskName = buf2;
    ci.SetId = pfdin->setID;
    ci.CabinetNumber = pfdin->iCabinet;
    err = phsc->msghandler(phsc->context, SPFILENOTIFY_NEEDNEWCABINET, (UINT_PTR)&ci, (UINT_PTR)mysterio);
    if (err) {
      SetLastError(err);
      return -1;
    } else {
      if (mysterio[0]) {
        len = strlenW(mysterio) + 1;
        if ((len > 255) || (len <= 1))
          return 0;
        if (!WideCharToMultiByte(CP_ACP, 0, mysterio, len, pfdin->psz3, 255, 0, 0))
          return 0;
      }
      return 0;
    }
  default:
    FIXME("Unknown notification type %d.\n", fdint);
    return 0;
  }
}

/***********************************************************************
 *		SetupIterateCabinetA (SETUPAPI.@)
 */
BOOL WINAPI SetupIterateCabinetA(PCSTR CabinetFile, DWORD Reserved,
                                 PSP_FILE_CALLBACK_A MsgHandler, PVOID Context)
{

  SC_HSC_A my_hsc;
  ERF erf;
  CHAR pszCabinet[MAX_PATH], pszCabPath[MAX_PATH], *p = NULL;
  DWORD fpnsize;
  BOOL ret;

  TRACE("(CabinetFile == %s, Reserved == %u, MsgHandler == ^%p, Context == ^%p)\n",
        debugstr_a(CabinetFile), Reserved, MsgHandler, Context);

  if (!LoadCABINETDll())
    return FALSE;

  if (!CabinetFile)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  fpnsize = strlen(CabinetFile);
  if (fpnsize >= MAX_PATH) {
    SetLastError(ERROR_BAD_PATHNAME);
    return FALSE;
  }

  fpnsize = GetFullPathNameA(CabinetFile, MAX_PATH, pszCabPath, &p);
  if (fpnsize > MAX_PATH) {
    SetLastError(ERROR_BAD_PATHNAME);
    return FALSE;
  }

  if (p) {
    strcpy(pszCabinet, p);
    *p = '\0';
  } else {
    strcpy(pszCabinet, CabinetFile);
    pszCabPath[0] = '\0';
  }

  TRACE("path: %s, cabfile: %s\n", debugstr_a(pszCabPath), debugstr_a(pszCabinet));

  /* remember the cabinet name */
  strcpy(my_hsc.most_recent_cabinet_name, pszCabinet);

  my_hsc.magic = SC_HSC_A_MAGIC;
  my_hsc.msghandler = MsgHandler;
  my_hsc.context = Context;
  my_hsc.hfdi = sc_FDICreate( sc_cb_alloc, sc_cb_free, sc_cb_open, sc_cb_read,
                           sc_cb_write, sc_cb_close, sc_cb_lseek, cpuUNKNOWN, &erf );

  if (!my_hsc.hfdi) return FALSE;

  ret = sc_FDICopy(my_hsc.hfdi, pszCabinet, pszCabPath, 0, sc_FNNOTIFY_A, NULL, &my_hsc);

  sc_FDIDestroy(my_hsc.hfdi);
  return ret;
}


/***********************************************************************
 *		SetupIterateCabinetW (SETUPAPI.@)
 */
BOOL WINAPI SetupIterateCabinetW(PCWSTR CabinetFile, DWORD Reserved,
                                 PSP_FILE_CALLBACK_W MsgHandler, PVOID Context)
{
  CHAR pszCabinet[MAX_PATH], pszCabPath[MAX_PATH];
  UINT len;
  SC_HSC_W my_hsc;
  ERF erf;
  WCHAR pszCabPathW[MAX_PATH], *p = NULL;
  DWORD fpnsize;
  BOOL ret;

  TRACE("(CabinetFile == %s, Reserved == %u, MsgHandler == ^%p, Context == ^%p)\n",
        debugstr_w(CabinetFile), Reserved, MsgHandler, Context);

  if (!LoadCABINETDll())
    return FALSE;

  if (!CabinetFile)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  fpnsize = GetFullPathNameW(CabinetFile, MAX_PATH, pszCabPathW, &p);
  if (fpnsize > MAX_PATH) {
    SetLastError(ERROR_BAD_PATHNAME);
    return FALSE;
  }

  if (p) {
    strcpyW(my_hsc.most_recent_cabinet_name, p);
    *p = 0;
    len = WideCharToMultiByte(CP_ACP, 0, pszCabPathW, -1, pszCabPath,
				MAX_PATH, 0, 0);
    if (!len) return FALSE;
  } else {
    strcpyW(my_hsc.most_recent_cabinet_name, CabinetFile);
    pszCabPath[0] = '\0';
  }

  len = WideCharToMultiByte(CP_ACP, 0, my_hsc.most_recent_cabinet_name, -1,
				pszCabinet, MAX_PATH, 0, 0);
  if (!len) return FALSE;

  TRACE("path: %s, cabfile: %s\n",
	debugstr_a(pszCabPath), debugstr_a(pszCabinet));

  my_hsc.magic = SC_HSC_W_MAGIC;
  my_hsc.msghandler = MsgHandler;
  my_hsc.context = Context;
  my_hsc.hfdi = sc_FDICreate( sc_cb_alloc, sc_cb_free, sc_cb_open, sc_cb_read,
                              sc_cb_write, sc_cb_close, sc_cb_lseek, cpuUNKNOWN, &erf );

  if (!my_hsc.hfdi) return FALSE;

  ret = sc_FDICopy(my_hsc.hfdi, pszCabinet, pszCabPath, 0, sc_FNNOTIFY_W, NULL, &my_hsc);

  sc_FDIDestroy(my_hsc.hfdi);
  return ret;
}


/***********************************************************************
 * DllMain
 *
 * PARAMS
 *     hinstDLL    [I] handle to the DLL's instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserved, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
        if (!GetVersionExW((POSVERSIONINFOW)&OsVersionInfo))
            return FALSE;
        SETUPAPI_hInstance = hinstDLL;
        break;
    case DLL_PROCESS_DETACH:
        if (lpvReserved) break;
        SetupCloseLog();
        if (CABINET_hInstance) FreeLibrary(CABINET_hInstance);
        break;
    }

    return TRUE;
}
