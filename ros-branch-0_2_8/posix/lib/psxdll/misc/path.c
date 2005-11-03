/* $Id: path.c,v 1.4 2002/10/29 04:45:33 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/misc/path.c
 * PURPOSE:     POSIX subsystem path utilities
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              31/01/2002: Created
 */

#include <ddk/ntddk.h>
#include <errno.h>
#include <string.h>
#include <psx/stdlib.h>
#include <psx/pdata.h>
#include <psx/path.h>

BOOLEAN
__PdxPosixPathGetNextComponent_U
(
 IN UNICODE_STRING PathName,
 IN OUT PUNICODE_STRING PathComponent,
 OUT PBOOLEAN TrailingDelimiter OPTIONAL
)
{
 int i, j;
 USHORT l = PathName.Length / sizeof(WCHAR);

 if(PathComponent->Buffer == 0)
  i = 0;
 else
  i = ((ULONG)PathComponent->Buffer - (ULONG)PathName.Buffer + PathComponent->Length) / sizeof(WCHAR);

 /* skip leading empty components */
 while(1)
  if(i >= l)
  {
   PathComponent->Length = PathComponent->MaximumLength = 0;
   return (FALSE);
  }
  else if(IS_CHAR_DELIMITER_U(PathName.Buffer[i]))
   i ++;
  else
   break;

 if(i > l)
 {
  PathComponent->Length = PathComponent->MaximumLength = 0;
  return (FALSE);
 }

 PathComponent->Buffer = &PathName.Buffer[i];

 j = i + 1;

 /* advance until the end of the string, or the next delimiter */
 while(1)
 {
  if(j >= l)
  {

   if(TrailingDelimiter != 0)
    *TrailingDelimiter = FALSE;

   break;
  }
  else if (IS_CHAR_DELIMITER_U(PathName.Buffer[j]))
  {

   if(TrailingDelimiter != 0)
    *TrailingDelimiter = TRUE;

   break;
  }
  else
   j ++;
 }

 PathComponent->Length = PathComponent->MaximumLength = (j - i) * sizeof(WCHAR);

 return (TRUE);

}

BOOLEAN
__PdxPosixPathResolve_U
(
 IN UNICODE_STRING PathName,
 OUT PUNICODE_STRING ResolvedPathName,
 IN WCHAR PathDelimiter OPTIONAL
)
{
 UNICODE_STRING wstrThisComponent = {0, 0, NULL};
 PWCHAR         pwcCurPos;
 PWCHAR         pwcStartPos;
 BOOLEAN        bIsDirectory;

 if(PathDelimiter == 0)
  PathDelimiter = L'/';

 /* start from the beginning of the return buffer */
 pwcCurPos = ResolvedPathName->Buffer;

 /* path begins with a delimiter (absolute path) */
 if(IS_CHAR_DELIMITER_U(PathName.Buffer[0]))
 {
  /* put a delimiter in front of the return buffer */
  *pwcCurPos = PathDelimiter;
  /* move to next character */
  pwcCurPos ++;
 }

 pwcStartPos = pwcCurPos;

 /* repeat until the end of the path string */
 while(__PdxPosixPathGetNextComponent_U(PathName, &wstrThisComponent, &bIsDirectory))
 {
  /* ".": skip */
  if(IS_COMPONENT_DOT_U(wstrThisComponent))
   continue;
  /* "..": go back to the last component */
  else if(IS_COMPONENT_DOTDOT_U(wstrThisComponent))
  {
   if(pwcCurPos == pwcStartPos)
    continue;

   /* skip the last (undefined) character */
   pwcCurPos --;
   /* down to the previous path delimiter */
   do{ pwcCurPos --; }while(!IS_CHAR_DELIMITER_U(*pwcCurPos));
   /* include the delimiter */
   pwcCurPos ++;
  }
  else
  {
   /* copy this component into the return string */
   memcpy
   (
    pwcCurPos,
    wstrThisComponent.Buffer, 
    wstrThisComponent.Length
   );

   /* move the current position to the end of the string */
   pwcCurPos = (PWCHAR)((PBYTE)pwcCurPos + wstrThisComponent.Length);

   /* component had a trailing delimiter */
   if(bIsDirectory)
   {
    /* append a delimiter */
    *pwcCurPos = PathDelimiter;
    /* on to next character */
    pwcCurPos ++;
   }
  }
 }

 /* set the return string's length as the byte offset between the initial buffer
    position and the current position */
 ResolvedPathName->Length = ((ULONG)pwcCurPos - (ULONG)ResolvedPathName->Buffer);

 return (TRUE);

}

BOOLEAN
__PdxPosixPathGetNextComponent_A
(
 IN ANSI_STRING PathName,
 IN OUT PANSI_STRING PathComponent,
 OUT PBOOLEAN TrailingDelimiter OPTIONAL
)
{
 int i, j;

 if(PathComponent->Buffer == 0)
  i = 0;
 else
  i = ((ULONG)PathComponent->Buffer - (ULONG)PathName.Buffer + PathComponent->Length);

 /* skip leading empty components */
 while(1)
  if(i >= PathName.Length)
  {
   PathComponent->Length = PathComponent->MaximumLength = 0;
   return (FALSE);
  }
  else if(IS_CHAR_DELIMITER_A(PathName.Buffer[i]))
   i ++;
  else
   break;

 if(i > PathName.Length)
 {
  PathComponent->Length = PathComponent->MaximumLength = 0;
  return (FALSE);
 }

 PathComponent->Buffer = &PathName.Buffer[i];

 j = i + 1;

 /* advance until the end of the string, or the next delimiter */
 while(1)
 {
  if(j >= PathName.Length)
  {

   if(TrailingDelimiter != 0)
    *TrailingDelimiter = FALSE;

   break;
  }
  else if (IS_CHAR_DELIMITER_A(PathName.Buffer[j]))
  {

   if(TrailingDelimiter != 0)
    *TrailingDelimiter = TRUE;

   break;
  }
  else
   j ++;
 }

 PathComponent->Length = PathComponent->MaximumLength = j - i;

 return (TRUE);

}

BOOLEAN
__PdxPosixPathResolve_A
(
 IN ANSI_STRING PathName,
 OUT PANSI_STRING ResolvedPathName,
 IN CHAR PathDelimiter OPTIONAL
)
{
 ANSI_STRING strThisComponent = {0, 0, NULL};
 PCHAR       pcCurPos;
 PCHAR       pcStartPos;
 BOOLEAN     bIsDirectory;

 if(PathDelimiter == 0)
  PathDelimiter = '/';

 /* start from the beginning of the return buffer */
 pcCurPos = ResolvedPathName->Buffer;

 /* path begins with a delimiter (absolute path) */
 if(IS_CHAR_DELIMITER_A(PathName.Buffer[0]))
 {
  /* put a delimiter in front of the return buffer */
  *pcCurPos = PathDelimiter;
  /* move to next character */
  pcCurPos ++;
 }

 pcStartPos = pcCurPos;

 /* repeat until the end of the path string */
 while(__PdxPosixPathGetNextComponent_A(PathName, &strThisComponent, &bIsDirectory))
 {
  /* ".": skip */
  if(IS_COMPONENT_DOT_A(strThisComponent))
   continue;
  /* "..": go back to the last component */
  else if(IS_COMPONENT_DOTDOT_A(strThisComponent))
  {
   if(pcCurPos == pcStartPos)
    continue;

   /* skip the last (undefined) character */
   pcCurPos --;
   /* down to the previous path delimiter */
   do{ pcCurPos --; }while(!IS_CHAR_DELIMITER_A(*pcCurPos));
   /* include the delimiter */
   pcCurPos ++;
  }
  else
  {
   /* copy this component into the return string */
   strncpy
   (
    pcCurPos,
    strThisComponent.Buffer, 
    strThisComponent.Length
   );

   /* move the current position to the end of the string */
   pcCurPos = (PCHAR)((PBYTE)pcCurPos + strThisComponent.Length);

   /* component had a trailing delimiter */
   if(bIsDirectory)
   {
    /* append a delimiter */
    *pcCurPos = PathDelimiter;
    /* on to next character */
    pcCurPos ++;
   }
  }
 }

 /* set the return string's length as the byte offset between the initial buffer
    position and the current position */
 ResolvedPathName->Length = ((ULONG)pcCurPos - (ULONG)ResolvedPathName->Buffer);

 return (TRUE);

}

BOOLEAN
__PdxPosixPathNameToNtPathName
(
 IN PWCHAR PosixPath,
	OUT PUNICODE_STRING NativePath,
	IN PUNICODE_STRING CurDir OPTIONAL,
 IN PUNICODE_STRING RootDir OPTIONAL
)
{
 UNICODE_STRING wstrPosixPath;
 UNICODE_STRING wstrTempString;

 /* parameter validation */
 if
 (
  PosixPath == 0 ||
  NativePath == 0 ||
  NativePath->Buffer == 0 ||
  NativePath->MaximumLength == 0 ||
  (RootDir != 0 && RootDir->Buffer == 0)
 )
 {
  errno = EINVAL;
  return (FALSE);
 }

 RtlInitUnicodeString(&wstrPosixPath, PosixPath);

 /* path is null */
 if(0 == wstrPosixPath.Length)
 {
  errno = EINVAL;
  return (FALSE);
 }

 /* first, copy the root path into the return buffer */
 /* if no root dir passed by the caller... */
 if(RootDir == 0)
  /* return buffer too small */
  if(NativePath->MaximumLength < sizeof(WCHAR))
  {
   errno = ENOBUFS;
   return (FALSE);
  }
  /* set the first character to a backslash, and set length accordingly */
  else
  {
   NativePath->Buffer[0] = L'\\';
   NativePath->Length = sizeof(WCHAR);
  }
 /* ... else copy the root dir into the return buffer */
 else
  /* return buffer too small */
  if(NativePath->MaximumLength < RootDir->Length)
  {
   errno = ENOBUFS;
   return (FALSE);
  }
  /* copy the root directory into the return buffer, and set length */
  else
  {
   memcpy(NativePath->Buffer, RootDir->Buffer, RootDir->Length);
   NativePath->Length = RootDir->Length;
  }

 /* path is "/" - our work is done */
 if(sizeof(WCHAR) == wstrPosixPath.Length && IS_CHAR_DELIMITER_U(wstrPosixPath.Buffer[0]))
  return (TRUE);

 /* temp string pointing to the tail of the return buffer */
 wstrTempString.Length = 0;
 wstrTempString.MaximumLength = NativePath->MaximumLength - NativePath->Length;
 wstrTempString.Buffer = (PWCHAR)(((PBYTE)(NativePath->Buffer)) + NativePath->Length);

 /* path begins with '/': absolute path. Append the resolved path to the return buffer */
 if(IS_CHAR_DELIMITER_U(wstrPosixPath.Buffer[0]))
 {
  /* copy the resolved path in the return buffer */
  __PdxPosixPathResolve_U(wstrPosixPath, &wstrTempString, L'\\');

  return (TRUE);
 }
 else
 {
  UNICODE_STRING wstrAbsolutePath;

  if(CurDir == 0)
   CurDir = __PdxGetCurDir();

  /* initialize the buffer for the absolute path */
  wstrAbsolutePath.Length = 0;
  wstrAbsolutePath.MaximumLength = 0xFFFF;
  wstrAbsolutePath.Buffer = __malloc(0xFFFF);

  /* if the current directory is not null... */
  if(!(CurDir->Buffer == 0 || CurDir->Length == 0))
  {
   /* copy it into the absolute path buffer */
   memcpy(wstrAbsolutePath.Buffer, CurDir->Buffer, CurDir->Length);
   wstrAbsolutePath.Length += CurDir->Length;
  }

  /* not enough space to append an extra slash */
  if((wstrAbsolutePath.MaximumLength - wstrAbsolutePath.Length) < (USHORT)sizeof(WCHAR))
  {
   __free(wstrAbsolutePath.Buffer);
   NativePath->Length = 0;
   errno = ENOBUFS;
   return (FALSE);
  }

  /* append an extra slash */
  wstrAbsolutePath.Buffer[wstrAbsolutePath.Length / sizeof(WCHAR)] = L'/';
  wstrAbsolutePath.Length += sizeof(WCHAR);

  /* not enough space to copy the relative path */
  if((wstrAbsolutePath.MaximumLength - wstrAbsolutePath.Length) < wstrPosixPath.Length)
  {
   __free(wstrAbsolutePath.Buffer);
   NativePath->Length = 0;
   errno = ENOBUFS;
   return (FALSE);
  }

  /* append the relative path to the absolute path */
  memcpy(
   (PWCHAR)(((PBYTE)wstrAbsolutePath.Buffer) + wstrAbsolutePath.Length),
   wstrPosixPath.Buffer,
   wstrPosixPath.Length
  );
  wstrAbsolutePath.Length += wstrPosixPath.Length;

  /* resolve the path */
  __PdxPosixPathResolve_U(wstrAbsolutePath, &wstrTempString, L'\\');

  __free(wstrAbsolutePath.Buffer);

  return (TRUE);
 }

 return (FALSE);

}

/* EOF */

