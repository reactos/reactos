/*
 * COPYRIGHT:   See COPYING in the top level directory
 *      Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details 
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/mktemp.c
 * PURPOSE:     Makes a temp file based on a template
 * PROGRAMER:   DJ Delorie
                Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Appropriated for the Reactos Kernel
 */

/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <msvcrt/stdio.h>
#include <msvcrt/string.h>
#include <msvcrt/io.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


/*
 * @implemented
 */
wchar_t* _wmktemp (wchar_t *_template)
{
  static int count = 0;
  wchar_t *cp, *dp;
  int i, len, xcount, loopcnt;

  DPRINT("_wmktemp('%S')\n", _template);
  len = wcslen (_template);
  cp = _template + len;

  xcount = 0;
  while (xcount < 6 && cp > _template && cp[-1] == L'X')
    xcount++, cp--;

  if (xcount) {
    dp = cp;
    while (dp > _template && dp[-1] != L'/' && dp[-1] != L'\\' && dp[-1] != L':')
      dp--;

    /* Keep the first characters of the template, but turn the rest into
       Xs.  */
    while (cp > dp + 8 - xcount) {
      *--cp = L'X';
      xcount = (xcount >= 6) ? 6 : 1 + xcount;
    }

    /* If dots occur too early -- squash them.  */
    while (dp < cp) {
      if (*dp == L'.') *dp = L'a';
      dp++;
    }

    /* Try to add ".tmp" to the filename.  Truncate unused Xs.  */
    if (cp + xcount + 3 < _template + len)
      wcscpy (cp + xcount, L".tmp");
    else
      cp[xcount] = 0;

    /* This loop can run up to 2<<(5*6) times, or about 10^9 times.  */
    for (loopcnt = 0; loopcnt < (1 << (5 * xcount)); loopcnt++) {
      int c = count++;
      for (i = 0; i < xcount; i++, c >>= 5)
    cp[i] = L"abcdefghijklmnopqrstuvwxyz012345"[c & 0x1f];
      if (_waccess(_template,0) == -1)
    return _template;
    }
  }

  /* Failure:  truncate the template and return NULL.  */
  *_template = 0;
  return 0;
}
