/* $Id: Pattern.cpp,v 1.2 2001/01/13 23:54:40 narnaoud Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (C) 2000,2001 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

// Pattern.cpp: implementation of pattern functions

#include "ph.h"

// based on wstrcmpjoki() by Jason Filby (jasonfilby@yahoo.com)
// reactos kernel, services/fs/vfat/string.c
BOOL PatternMatch(const TCHAR *pszPattern, const TCHAR *pszTry)
{
  while ((*pszPattern == _T('?'))||((_totlower(*pszTry)) == (_totlower(*pszPattern))))
  {
    if (((*pszTry) == 0) && ((*pszPattern) == 0))
      return TRUE;

    if (((*pszTry) == 0) || ((*pszPattern) == 0))
      return FALSE;
    
    pszTry++;
    pszPattern++;	
  }
   
  if (*pszPattern == _T('*'))
  {
    pszPattern++;
    while (*pszTry)
    {
      if (PatternMatch(pszPattern,pszTry))
        return TRUE;
      else
        pszTry++;
    }
  }
   
  if (((*pszTry) == 0) && ((*pszPattern) == 0))
    return TRUE;
   
  return FALSE;
}
