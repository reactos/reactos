/* $Id: caret.c,v 1.1 2003/10/15 20:48:19 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Caret functions
 * FILE:             subsys/win32k/ntuser/caret.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY:
 *       10/15/2003  Created
 */

#include <win32k/win32k.h>
#include <internal/safe.h>
#include <include/error.h>
#include <include/window.h>

#define NDEBUG
#include <debug.h>

BOOL
STDCALL
NtUserCreateCaret(
  HWND hWnd,
  HBITMAP hBitmap,
  int nWidth,
  int nHeight)
{
  UNIMPLEMENTED

  return 0;
}

UINT
STDCALL
NtUserGetCaretBlinkTime(VOID)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserGetCaretPos(
  LPPOINT lpPoint)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserHideCaret(
  HWND hWnd)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserShowCaret(
  HWND hWnd)
{
  UNIMPLEMENTED

  return 0;
}

