/*
 * msvcrt.dll mbcs functions
 *
 * Copyright 1999 Alexandre Julliard
 * Copyright 2000 Jon Griffths
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
 * FIXME
 * Not currently binary compatible with win32. MSVCRT_mbctype must be
 * populated correctly and the ismb* functions should reference it.
 */

#include <precomp.h>

#include <mbctype.h>

/* It seems that the data about valid trail bytes is not available from kernel32
 * so we have to store is here. The format is the same as for lead bytes in CPINFO */
struct cp_extra_info_t
{
    int cp;
    BYTE TrailBytes[MAX_LEADBYTES];
};

static struct cp_extra_info_t g_cpextrainfo[] =
{
    {932, {0x40, 0x7e, 0x80, 0xfc, 0, 0}},
    {936, {0x40, 0xfe, 0, 0}},
    {949, {0x41, 0xfe, 0, 0}},
    {950, {0x40, 0x7e, 0xa1, 0xfe, 0, 0}},
    {1361, {0x31, 0x7e, 0x81, 0xfe, 0, 0}},
    {20932, {1, 255, 0, 0}},  /* seems to give different results on different systems */
    {0, {1, 255, 0, 0}}       /* match all with FIXME */
};

/*********************************************************************
 * INTERNAL: _setmbcp_l
 */
int _setmbcp_l(int cp, LCID lcid, MSVCRT_pthreadmbcinfo mbcinfo)
{
  const char format[] = ".%d";

  int newcp;
  CPINFO cpi;
  BYTE *bytes;
  WORD chartypes[256];
  char bufA[256];
  WCHAR bufW[256];
  int charcount;
  int ret;
  int i;

  if(!mbcinfo)
      mbcinfo = get_mbcinfo();

  switch (cp)
  {
    case _MB_CP_ANSI:
      newcp = GetACP();
      break;
    case _MB_CP_OEM:
      newcp = GetOEMCP();
      break;
    case _MB_CP_LOCALE:
      newcp = get_locinfo()->lc_codepage;
      if(newcp)
          break;
      /* fall through (C locale) */
    case _MB_CP_SBCS:
      newcp = 20127;   /* ASCII */
      break;
    default:
      newcp = cp;
      break;
  }

  if(lcid == -1) {
    sprintf(bufA, format, newcp);
    mbcinfo->mblcid = MSVCRT_locale_to_LCID(bufA, NULL);
  } else {
    mbcinfo->mblcid = lcid;
  }

  if(mbcinfo->mblcid == -1)
  {
    WARN("Can't assign LCID to codepage (%d)\n", mbcinfo->mblcid);
    mbcinfo->mblcid = 0;
  }

  if (!GetCPInfo(newcp, &cpi))
  {
    WARN("Codepage %d not found\n", newcp);
    *_errno() = EINVAL;
    return -1;
  }

  /* setup the _mbctype */
  memset(mbcinfo->mbctype, 0, sizeof(unsigned char[257]));
  memset(mbcinfo->mbcasemap, 0, sizeof(unsigned char[256]));

  bytes = cpi.LeadByte;
  while (bytes[0] || bytes[1])
  {
    for (i = bytes[0]; i <= bytes[1]; i++)
      mbcinfo->mbctype[i + 1] |= _M1;
    bytes += 2;
  }

  if (cpi.MaxCharSize > 1)
  {
    /* trail bytes not available through kernel32 but stored in a structure in msvcrt */
    struct cp_extra_info_t *cpextra = g_cpextrainfo;

    mbcinfo->ismbcodepage = 1;
    while (TRUE)
    {
      if (cpextra->cp == 0 || cpextra->cp == newcp)
      {
        if (cpextra->cp == 0)
          FIXME("trail bytes data not available for DBCS codepage %d - assuming all bytes\n", newcp);

        bytes = cpextra->TrailBytes;
        while (bytes[0] || bytes[1])
        {
          for (i = bytes[0]; i <= bytes[1]; i++)
            mbcinfo->mbctype[i + 1] |= _M2;
          bytes += 2;
        }
        break;
      }
      cpextra++;
    }
  }
  else
    mbcinfo->ismbcodepage = 0;

  /* we can't use GetStringTypeA directly because we don't have a locale - only a code page
   */
  charcount = 0;
  for (i = 0; i < 256; i++)
    if (!(mbcinfo->mbctype[i + 1] & _M1))
      bufA[charcount++] = i;

  ret = MultiByteToWideChar(newcp, 0, bufA, charcount, bufW, charcount);
  if (ret != charcount)
    ERR("MultiByteToWideChar of chars failed for cp %d, ret=%d (exp %d), error=%d\n", newcp, ret, charcount, GetLastError());

  GetStringTypeW(CT_CTYPE1, bufW, charcount, chartypes);

  charcount = 0;
  for (i = 0; i < 256; i++)
    if (!(mbcinfo->mbctype[i + 1] & _M1))
    {
      if (chartypes[charcount] & C1_UPPER)
      {
        mbcinfo->mbctype[i + 1] |= _SBUP;
        bufW[charcount] = tolowerW(bufW[charcount]);
      }
      else if (chartypes[charcount] & C1_LOWER)
      {
	mbcinfo->mbctype[i + 1] |= _SBLOW;
        bufW[charcount] = toupperW(bufW[charcount]);
      }
      charcount++;
    }

  ret = WideCharToMultiByte(newcp, 0, bufW, charcount, bufA, charcount, NULL, NULL);
  if (ret != charcount)
    ERR("WideCharToMultiByte failed for cp %d, ret=%d (exp %d), error=%d\n", newcp, ret, charcount, GetLastError());

  charcount = 0;
  for (i = 0; i < 256; i++)
  {
    if(!(mbcinfo->mbctype[i + 1] & _M1))
    {
      if(mbcinfo->mbctype[i] & (C1_UPPER|C1_LOWER))
        mbcinfo->mbcasemap[i] = bufA[charcount];
      charcount++;
    }
  }

  if (newcp == 932)   /* CP932 only - set _MP and _MS */
  {
    /* On Windows it's possible to calculate the _MP and _MS from CT_CTYPE1
     * and CT_CTYPE3. But as of Wine 0.9.43 we return wrong values what makes
     * it hard. As this is set only for codepage 932 we hardcode it what gives
     * also faster execution.
     */
    for (i = 161; i <= 165; i++)
      mbcinfo->mbctype[i + 1] |= _MP;
    for (i = 166; i <= 223; i++)
      mbcinfo->mbctype[i + 1] |= _MS;
  }

  mbcinfo->mbcodepage = newcp;
  if(global_locale && mbcinfo == MSVCRT_locale->mbcinfo)
    memcpy(_mbctype, MSVCRT_locale->mbcinfo->mbctype, sizeof(_mbctype));

  return 0;
}

/*********************************************************************
 *              _setmbcp (MSVCRT.@)
 */
int CDECL _setmbcp(int cp)
{
    return _setmbcp_l(cp, -1, NULL);
}

