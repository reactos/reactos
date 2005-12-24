/* $Id$
 *
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>

static CMDLINEINFO CmdLineInfo;

static char *
SkipWhitespace(char *s)
{
  while ('\0' != *s && isspace(*s))
    {
      s++;
    }

  return s;
}

void
CmdLineParse(char *CmdLine)
{
  char *s;
  char *Name;
  char *Value;
  char *End;

  CmdLineInfo.DefaultOperatingSystem = NULL;
  CmdLineInfo.TimeOut = -1;

  if (NULL == CmdLine)
    {
      return;
    }

  /* Skip over "kernel name" */
  s = CmdLine;
  while ('\0' != *s && ! isspace(*s))
    {
      s++;
    }
  s = SkipWhitespace(s);

  while ('\0' != *s)
    {
      Name = s;
      while (! isspace(*s) && '=' != *s && '\0' != *s)
        {
          s++;
        }
      End = s;
      s = SkipWhitespace(s);
      if ('=' == *s)
        {
          s++;
          *End = '\0';
          s = SkipWhitespace(s);
          if ('"' == *s)
            {
              s++;
              Value = s;
              while ('"' != *s && '\0' != *s)
                {
                  s++;
                }
            }
          else
            {
              Value = s;
              while (! isspace(*s) && '\0' != *s)
                {
                  s++;
                }
            }
          if ('\0' != *s)
            {
              *s++ = '\0';
            }
          if (0 == _stricmp(Name, "defaultos"))
            {
              CmdLineInfo.DefaultOperatingSystem = Value;
            }
          else if (0 == _stricmp(Name, "timeout"))
            {
              CmdLineInfo.TimeOut = atoi(Value);
            }
        }
    }
}

const char *
CmdLineGetDefaultOS(void)
{
  return CmdLineInfo.DefaultOperatingSystem;
}

LONG
CmdLineGetTimeOut(void)
{
  return CmdLineInfo.TimeOut;
}

/* EOF */

