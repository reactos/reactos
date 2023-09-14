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
 */

#include "setupapi_private.h"

#include <fcntl.h>
#include <share.h>
#include <fdi.h>

HINSTANCE SETUPAPI_hInstance = NULL;
OSVERSIONINFOEXW OsVersionInfo;

typedef struct {
  PSP_FILE_CALLBACK_A msghandler;
  PVOID context;
  CHAR most_recent_cabinet_name[MAX_PATH];
  CHAR most_recent_target[MAX_PATH];
} SC_HSC_A, *PSC_HSC_A;

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
  SECURITY_ATTRIBUTES sa;

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
  }

  if (oflag & _O_CREAT) {
    if (oflag & _O_EXCL)
      creation = CREATE_NEW;
    else if (oflag & _O_TRUNC)
      creation = CREATE_ALWAYS;
    else
      creation = OPEN_ALWAYS;
  } else {
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
  }

  sa.nLength              = sizeof( SECURITY_ATTRIBUTES );
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle       = !(ioflag & _O_NOINHERIT);

  return (INT_PTR) CreateFileA(pszFile, ioflag, sharing, &sa, creation, FILE_ATTRIBUTE_NORMAL, NULL);
}

static UINT CDECL sc_cb_read(INT_PTR hf, void *pv, UINT cb)
{
    DWORD num_read;

    if (!ReadFile((HANDLE)hf, pv, cb, &num_read, NULL))
        return -1;
    return num_read;
}

static UINT CDECL sc_cb_write(INT_PTR hf, void *pv, UINT cb)
{
    DWORD num_written;

    if (!WriteFile((HANDLE)hf, pv, cb, &num_written, NULL))
        return -1;
    return num_written;
}

static int CDECL sc_cb_close(INT_PTR hf)
{
    if (!CloseHandle((HANDLE)hf))
        return -1;
    return 0;
}

static LONG CDECL sc_cb_lseek(INT_PTR hf, LONG dist, int seektype)
{
    DWORD ret;

    if (seektype < 0 || seektype > 2)
        return -1;

    if (((ret = SetFilePointer((HANDLE)hf, dist, NULL, seektype)) == INVALID_SET_FILE_POINTER) && GetLastError())
        return -1;
    return ret;
}

#define SIZEOF_MYSTERIO (MAX_PATH*3)

static INT_PTR CDECL sc_FNNOTIFY_A(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
  FILE_IN_CABINET_INFO_A fici;
  PSC_HSC_A phsc = pfdin->pv;
  CABINET_INFO_A ci;
  FILEPATHS_A fp;
  UINT err;

  CHAR mysterio[SIZEOF_MYSTERIO]; /* how big? undocumented! probably 256... */

  memset(mysterio, 0, SIZEOF_MYSTERIO);

  switch (fdint) {
  case fdintCABINET_INFO:
    TRACE("New cabinet, path %s, set %u, number %u, next disk %s, next cabinet %s.\n",
            debugstr_a(pfdin->psz3), pfdin->setID, pfdin->iCabinet, debugstr_a(pfdin->psz2), debugstr_a(pfdin->psz1));
    ci.CabinetFile = pfdin->psz1;
    ci.CabinetPath = pfdin->psz3;
    ci.DiskName = pfdin->psz2;
    ci.SetId = pfdin->setID;
    ci.CabinetNumber = pfdin->iCabinet;
    phsc->msghandler(phsc->context, SPFILENOTIFY_CABINETINFO, (UINT_PTR) &ci, 0);
    return 0;
  case fdintPARTIAL_FILE:
    return 0;
  case fdintCOPY_FILE:
    TRACE("Copy file %s, length %d, date %#x, time %#x, attributes %#x.\n",
            debugstr_a(pfdin->psz1), pfdin->cb, pfdin->date, pfdin->time, pfdin->attribs);
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
      TRACE("Callback specified filename: %s\n", debugstr_a(fici.FullTargetName));
      if (!fici.FullTargetName[0]) {
        WARN("Empty return string causing abort.\n");
        SetLastError(ERROR_PATH_NOT_FOUND);
        return -1;
      }
      strcpy( phsc->most_recent_target, fici.FullTargetName );
      return sc_cb_open(fici.FullTargetName, _O_BINARY | _O_CREAT | _O_WRONLY,  _S_IREAD | _S_IWRITE);
    } else {
      TRACE("Callback skipped file.\n");
      return 0;
    }
  case fdintCLOSE_FILE_INFO:
    TRACE("File extracted.\n");
    fp.Source = phsc->most_recent_cabinet_name;
    fp.Target = phsc->most_recent_target;
    fp.Win32Error = 0;
    fp.Flags = 0;
    /* FIXME: set file time and attributes */
    sc_cb_close(pfdin->hf);
    err = phsc->msghandler(phsc->context, SPFILENOTIFY_FILEEXTRACTED, (UINT_PTR)&fp, 0);
    if (err) {
      SetLastError(err);
      return FALSE;
    } else
      return TRUE;
  case fdintNEXT_CABINET:
    TRACE("Need new cabinet, path %s, file %s, disk %s, set %u, number %u.\n",
            debugstr_a(pfdin->psz3), debugstr_a(pfdin->psz1),
            debugstr_a(pfdin->psz2), pfdin->setID, pfdin->iCabinet);
    ci.CabinetFile = pfdin->psz1;
    ci.CabinetPath = pfdin->psz3;
    ci.DiskName = pfdin->psz2;
    ci.SetId = pfdin->setID;
    ci.CabinetNumber = pfdin->iCabinet;
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
  HFDI hfdi;
  BOOL ret;

  TRACE("(CabinetFile == %s, Reserved == %u, MsgHandler == ^%p, Context == ^%p)\n",
        debugstr_a(CabinetFile), Reserved, MsgHandler, Context);

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

  my_hsc.msghandler = MsgHandler;
  my_hsc.context = Context;
  hfdi = FDICreate(sc_cb_alloc, sc_cb_free, sc_cb_open, sc_cb_read,
        sc_cb_write, sc_cb_close, sc_cb_lseek, cpuUNKNOWN, &erf);

  if (!hfdi) return FALSE;

  ret = FDICopy(hfdi, pszCabinet, pszCabPath, 0, sc_FNNOTIFY_A, NULL, &my_hsc);

  FDIDestroy(hfdi);
  return ret;
}

struct iterate_wtoa_ctx
{
    PSP_FILE_CALLBACK_A orig_cb;
    void *orig_ctx;
};

static UINT WINAPI iterate_wtoa_cb(void *pctx, UINT message, UINT_PTR param1, UINT_PTR param2)
{
    struct iterate_wtoa_ctx *ctx = pctx;

    switch (message)
    {
    case SPFILENOTIFY_CABINETINFO:
    case SPFILENOTIFY_NEEDNEWCABINET:
    {
        const CABINET_INFO_A *infoA = (const CABINET_INFO_A *)param1;
        WCHAR pathW[MAX_PATH], fileW[MAX_PATH], diskW[MAX_PATH];
        CABINET_INFO_W infoW =
        {
            .CabinetPath = pathW,
            .CabinetFile = fileW,
            .DiskName = diskW,
            .SetId = infoA->SetId,
            .CabinetNumber = infoA->CabinetNumber,
        };

        MultiByteToWideChar(CP_ACP, 0, infoA->CabinetPath, -1, pathW, ARRAY_SIZE(pathW));
        MultiByteToWideChar(CP_ACP, 0, infoA->CabinetFile, -1, fileW, ARRAY_SIZE(fileW));
        MultiByteToWideChar(CP_ACP, 0, infoA->DiskName, -1, diskW, ARRAY_SIZE(diskW));

        if (message == SPFILENOTIFY_CABINETINFO)
            return ctx->orig_cb(ctx->orig_ctx, message, (UINT_PTR)&infoW, 0);
        else
        {
            char *newpathA = (char *)param2;
            WCHAR newpathW[MAX_PATH] = {0};
            BOOL ret = ctx->orig_cb(ctx->orig_ctx, message, (UINT_PTR)&infoW, (UINT_PTR)newpathW);

            WideCharToMultiByte(CP_ACP, 0, newpathW, -1, newpathA, MAX_PATH, NULL, NULL);
            return ret;
        }
    }
    case SPFILENOTIFY_FILEINCABINET:
    {
        FILE_IN_CABINET_INFO_A *infoA = (FILE_IN_CABINET_INFO_A *)param1;
        const char *cabA = (const char *)param2;
        WCHAR cabW[MAX_PATH], fileW[MAX_PATH];
        FILE_IN_CABINET_INFO_W infoW =
        {
            .NameInCabinet = fileW,
            .FileSize = infoA->FileSize,
            .Win32Error = infoA->Win32Error,
            .DosDate = infoA->DosDate,
            .DosTime = infoA->DosTime,
            .DosAttribs = infoA->DosAttribs,
        };
        BOOL ret;

        MultiByteToWideChar(CP_ACP, 0, infoA->NameInCabinet, -1, fileW, ARRAY_SIZE(fileW));
        MultiByteToWideChar(CP_ACP, 0, cabA, -1, cabW, ARRAY_SIZE(cabW));

        ret = ctx->orig_cb(ctx->orig_ctx, message, (UINT_PTR)&infoW, (UINT_PTR)cabW);

        WideCharToMultiByte(CP_ACP, 0, infoW.FullTargetName, -1, infoA->FullTargetName,
                ARRAY_SIZE(infoA->FullTargetName), NULL, NULL);

        return ret;
    }
    case SPFILENOTIFY_FILEEXTRACTED:
    {
        const FILEPATHS_A *pathsA = (const FILEPATHS_A *)param1;
        WCHAR targetW[MAX_PATH], sourceW[MAX_PATH];
        FILEPATHS_W pathsW =
        {
            .Target = targetW,
            .Source = sourceW,
            .Win32Error = pathsA->Win32Error,
            .Flags = pathsA->Flags,
        };

        MultiByteToWideChar(CP_ACP, 0, pathsA->Target, -1, targetW, ARRAY_SIZE(targetW));
        MultiByteToWideChar(CP_ACP, 0, pathsA->Source, -1, sourceW, ARRAY_SIZE(sourceW));

        return ctx->orig_cb(ctx->orig_ctx, message, (UINT_PTR)&pathsW, 0);
    }
    default:
        FIXME("Unexpected callback %#x.\n", message);
        return FALSE;
    }
}

/***********************************************************************
 *              SetupIterateCabinetW (SETUPAPI.@)
 */
BOOL WINAPI SetupIterateCabinetW(const WCHAR *fileW, DWORD reserved,
        PSP_FILE_CALLBACK_W handler, void *context)
{
    struct iterate_wtoa_ctx ctx = {handler, context};
    char fileA[MAX_PATH];

    if (!WideCharToMultiByte(CP_ACP, 0, fileW, -1, fileA, ARRAY_SIZE(fileA), NULL, NULL))
        return FALSE;

    return SetupIterateCabinetA(fileA, reserved, iterate_wtoa_cb, &ctx);
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
        break;
    }

    return TRUE;
}
