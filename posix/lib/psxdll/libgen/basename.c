/* $Id: basename.c,v 1.2 2002/02/20 09:17:57 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/libgen/basename.c
 * PURPOSE:     Return the last component of a pathname
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/02/2002: Created
 */

#include <sys/types.h>
#include <libgen.h>
#include <wchar.h>
#include <psx/path.h>

static const char    *__basename_dot  =  ".";
static const wchar_t *__Wbasename_dot = L".";

char *basename(char *path)
{
 char  *pcTail;
 size_t nStrLen;

 /* null or empty string */
 if(path == 0 && ((nStrLen = strlen(path)) == 0))
  return ((char *)__basename_dot);

 if(nStrLen == 1)
 {
  /* path is "/", return "/" */
  if(IS_CHAR_DELIMITER_A(path[0]))
  {
   path[0] = '/';
   path[1] = 0;
   return (path);
  }
  /* path is a single character, return it */
  else
  {
   return (path);
  }
 }

 /* tail of the string (null terminator excluded) */
 pcTail = &path[nStrLen - 1];

 /* skip trailing slashes */
 while(pcTail > path && IS_CHAR_DELIMITER_A(*pcTail))
  pcTail --;

 pcTail[1] = 0;

 /* go backwards until a delimiter char or the beginning of the string */
 while(pcTail >= path)
  /* delimiter found, return the basename */
  if(IS_CHAR_DELIMITER_A(*pcTail))
   return (&pcTail[1]);
  else
   pcTail --;

 /* return all the path */
 return (path);
}

wchar_t *_Wbasename(wchar_t *path)
{
 wchar_t *pwcTail;
 size_t   nStrLen;

 /* null or empty string */
 if(path == 0 && ((nStrLen = wcslen(path)) == 0))
  return ((wchar_t *)__Wbasename_dot);

 if(nStrLen == 1)
 {
  /* path is "/", return "/" */
  if(IS_CHAR_DELIMITER_U(path[0]))
  {
   path[0] = L'/';
   path[1] = 0;
   return (path);
  }
  /* path is a single character, return it */
  else
  {
   return (path);
  }
 }

 /* tail of the string (null terminator excluded) */
 pwcTail = &path[nStrLen - 1];

 /* skip trailing slashes */
 while(pwcTail > path && IS_CHAR_DELIMITER_U(*pwcTail))
  pwcTail --;

 pwcTail[1] = 0;

 /* go backwards until a delimiter char or the beginning of the string */
 while(pwcTail >= path)
  /* delimiter found, return the basename */
  if(IS_CHAR_DELIMITER_U(*pwcTail))
   return (&pwcTail[1]);
  else
   pwcTail --;

 /* return all the path */
 return (path);
}

/* EOF */

