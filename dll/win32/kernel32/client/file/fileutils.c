/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/fileutils.c
 * PURPOSE:         File utility function shared with kernel32_vista
 * PROGRAMMER:      Taken from wine
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

PWCHAR
FilenameA2W(LPCSTR NameA, BOOL alloc)
{
   ANSI_STRING str;
   UNICODE_STRING strW;
   PUNICODE_STRING pstrW;
   NTSTATUS Status;

   //ASSERT(NtCurrentTeb()->StaticUnicodeString.Buffer == NtCurrentTeb()->StaticUnicodeBuffer);
   ASSERT(NtCurrentTeb()->StaticUnicodeString.MaximumLength == sizeof(NtCurrentTeb()->StaticUnicodeBuffer));

   RtlInitAnsiString(&str, NameA);
   pstrW = alloc ? &strW : &NtCurrentTeb()->StaticUnicodeString;

   if (AreFileApisANSI())
        Status= RtlAnsiStringToUnicodeString( pstrW, &str, (BOOLEAN)alloc );
   else
        Status= RtlOemStringToUnicodeString( pstrW, &str, (BOOLEAN)alloc );

    if (NT_SUCCESS(Status))
       return pstrW->Buffer;

    if (Status== STATUS_BUFFER_OVERFLOW)
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
    else
        BaseSetLastNTError(Status);

    return NULL;
}


/*
No copy/conversion is done if the dest. buffer is too small.

Returns:
   Success: number of TCHARS copied into dest. buffer NOT including nullterm
   Fail: size of buffer in TCHARS required to hold the converted filename, including nullterm
*/
DWORD
FilenameU2A_FitOrFail(
   LPSTR  DestA,
   INT destLen, /* buffer size in TCHARS incl. nullchar */
   PUNICODE_STRING SourceU
   )
{
   DWORD ret;

   /* destLen should never exceed MAX_PATH */
   if (destLen > MAX_PATH) destLen = MAX_PATH;

   ret = AreFileApisANSI() ? RtlUnicodeStringToAnsiSize(SourceU) : RtlUnicodeStringToOemSize(SourceU);
   /* ret incl. nullchar */

   if (DestA && (INT)ret <= destLen)
   {
      ANSI_STRING str;

      str.Buffer = DestA;
      str.MaximumLength = (USHORT)destLen;


      if (AreFileApisANSI())
         RtlUnicodeStringToAnsiString(&str, SourceU, FALSE );
      else
         RtlUnicodeStringToOemString(&str, SourceU, FALSE );

      ret = str.Length;  /* SUCCESS: length without terminating 0 */
   }

   return ret;
}


/*
No copy/conversion is done if the dest. buffer is too small.

Returns:
   Success: number of TCHARS copied into dest. buffer NOT including nullterm
   Fail: size of buffer in TCHARS required to hold the converted filename, including nullterm
*/
DWORD
FilenameW2A_FitOrFail(
   LPSTR  DestA,
   INT destLen, /* buffer size in TCHARS incl. nullchar */
   LPCWSTR SourceW,
   INT sourceLen /* buffer size in TCHARS incl. nullchar */
   )
{
   UNICODE_STRING strW;

   if (sourceLen < 0) sourceLen = wcslen(SourceW) + 1;

   strW.Buffer = (PWCHAR)SourceW;
   strW.MaximumLength = sourceLen * sizeof(WCHAR);
   strW.Length = strW.MaximumLength - sizeof(WCHAR);

   return FilenameU2A_FitOrFail(DestA, destLen, &strW);
}


/*
Return: num. TCHARS copied into dest including nullterm
*/
DWORD
FilenameA2W_N(
   LPWSTR dest,
   INT destlen, /* buffer size in TCHARS incl. nullchar */
   LPCSTR src,
   INT srclen /* buffer size in TCHARS incl. nullchar */
   )
{
    DWORD ret;

    if (srclen < 0) srclen = strlen( src ) + 1;

    if (AreFileApisANSI())
        RtlMultiByteToUnicodeN( dest, destlen* sizeof(WCHAR), &ret, (LPSTR)src, srclen  );
    else
        RtlOemToUnicodeN( dest, destlen* sizeof(WCHAR), &ret, (LPSTR)src, srclen );

    if (ret) dest[(ret/sizeof(WCHAR))-1]=0;

    return ret/sizeof(WCHAR);
}

/*
Return: num. TCHARS copied into dest including nullterm
*/
DWORD
FilenameW2A_N(
   LPSTR dest,
   INT destlen, /* buffer size in TCHARS incl. nullchar */
   LPCWSTR src,
   INT srclen /* buffer size in TCHARS incl. nullchar */
   )
{
    DWORD ret;

    if (srclen < 0) srclen = wcslen( src ) + 1;

    if (AreFileApisANSI())
        RtlUnicodeToMultiByteN( dest, destlen, &ret, (LPWSTR) src, srclen * sizeof(WCHAR));
    else
        RtlUnicodeToOemN( dest, destlen, &ret, (LPWSTR) src, srclen * sizeof(WCHAR) );

    if (ret) dest[ret-1]=0;

    return ret;
}

/* EOF */
