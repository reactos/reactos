////////////////////////////////////////////////////////////////////////////////
//
// debug.c
//
////////////////////////////////////////////////////////////////////////////////
#include "priv.h"
#pragma hdrstop

#ifdef DEBUG

#ifndef WINNT
#include <windows.h>
//#include <stdlib.h>
//#include <stdio.h>
#include "debug.h"
#endif

void
_Assert
  (DWORD dw, LPSTR lpszExp, LPSTR lpszFile, DWORD dwLine)
{
  DWORD dwT;
  TCHAR lpszT[256];
  wsprintf (lpszT, TEXT("Assertion %hs Failed.\n\n%hs, line# %ld\n\nYes to continue, No to debug, Cancel to exit"), lpszExp, lpszFile, dwLine);
  dwT = MessageBox (GetFocus(), lpszT, TEXT("Assertion Failed!"), MB_YESNOCANCEL);
  switch (dwT)
  {
    case IDCANCEL :
      //exit (1);
        FatalExit(1);
    case IDNO :
      DebugTrap;
  }
}

void
_AssertSz
  (DWORD dw, LPSTR lpszExp, LPTSTR lpsz, LPSTR lpszFile, DWORD dwLine)
{
  DWORD dwT;
  TCHAR lpszT[512];
  wsprintf (lpszT, TEXT("Assertion %hs Failed.\n\n%s\n%hs, line# %ld\n\nYes to continue, No to debug, Cancel to exit"), lpszExp, lpsz, lpszFile, dwLine);
  dwT = MessageBox (GetFocus(), lpszT, TEXT("Assertion Failed!"), MB_YESNOCANCEL);
  switch (dwT)
  {
    case IDCANCEL:
      //exit (1);
                FatalExit(1);
    case IDNO :
      DebugTrap;
  }
}

#ifdef LOTS_O_DEBUG
#include <windows.h>
#include <winerror.h>
#include <oleauto.h>
#include "debug.h"

void
_DebugHr
  (HRESULT hr, LPTSTR lpszFile, DWORD dwLine)
{
  TCHAR lpstzT[512];

  switch (hr) {
    case S_OK :
      return;
    case STG_E_INVALIDNAME:
      wsprintf (lpstzT, TEXT("\tBogus filename\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case STG_E_INVALIDFUNCTION :
      wsprintf (lpstzT, TEXT("\tInvalid Function\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case STG_E_FILENOTFOUND:
      wsprintf (lpstzT, TEXT("\tFile not found\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case STG_E_INVALIDFLAG:
      wsprintf (lpstzT, TEXT("\tBogus flag\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case STG_E_INVALIDPOINTER:
      wsprintf (lpstzT, TEXT("\tBogus pointer\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case STG_E_ACCESSDENIED:
      wsprintf (lpstzT, TEXT("\tAccess Denied\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case STG_E_INSUFFICIENTMEMORY :
    case E_OUTOFMEMORY            :
      wsprintf (lpstzT, TEXT("\tInsufficient Memory\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case E_INVALIDARG :
      wsprintf (lpstzT, TEXT("\tInvalid argument\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case TYPE_E_UNKNOWNLCID:
      wsprintf (lpstzT, TEXT("\tUnknown LCID\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case TYPE_E_CANTLOADLIBRARY:
      wsprintf (lpstzT, TEXT("\tCan't load typelib or dll\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case TYPE_E_INVDATAREAD:
      wsprintf (lpstzT, TEXT("\tCan't read file\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case TYPE_E_INVALIDSTATE:
      wsprintf (lpstzT, TEXT("\tTypelib couldn't be opened\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    case TYPE_E_IOERROR:
      wsprintf (lpstzT, TEXT("\tI/O error\n\n%s, line# %ld\n"),lpszFile, dwLine);
      break;
    default:
      wsprintf (lpstzT, TEXT("\tUnknown HRESULT %lx (%ld) \n\n%s, line# %ld\n"),hr, hr, lpszFile, dwLine);
  }

  MessageBox (GetFocus(), lpstzT, NULL, MB_OK);
  return;
}


void
_DebugGUID (GUID g)
{
  TCHAR lpsz[200];
  wsprintf (lpsz, TEXT("GUID is: %lx-%hx-%hx-%hx%hx-%hx%hx%hx%hx%hx%hx"),
           g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
           g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
  DebugSz (lpsz);
}
#endif // LOTS_O_DEBUG

#endif // DEBUG
