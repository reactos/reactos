
#include <windows.h>
#include <ddk/ntddk.h>
#include <kernel32/error.h>

/***********************************************************************
 *           SYSRES_GetResourcePtr
 *
 * Return a pointer to a system resource.
 */
LPCVOID SYSRES_GetResPtr( int id )
{
//    return SYSRES_Resources[Options.language][id]->data;


	return NULL;
}

int
STDCALL
LoadStringA( HINSTANCE hInstance,
	     UINT uID,
	     LPSTR lpBuffer,
	     int nBufferMax)
{
  HRSRC rsc;
  PBYTE ptr;
  int len;
  int count, dest = uID % 16;
  PWSTR pwstr;
  UNICODE_STRING UString;
  ANSI_STRING AString;
  NTSTATUS Status;
  
  rsc = FindResource( (HMODULE)hInstance,
		      MAKEINTRESOURCE( (uID / 16) + 1 ),
		      RT_STRING );
  if( rsc == NULL )
    return 0;
  // get pointer to string table
  ptr = (PBYTE)LoadResource( (HMODULE)hInstance, rsc );
  if( ptr == NULL )
    return 0;
  for( count = 0; count <= dest; count++ )
    {
      // walk each of the 16 string slots in the string table
      len = (*(USHORT *)ptr) * 2;  // length is in unicode chars, convert to bytes
      ptr += 2;    // first 2 bytes are length, string follows
      pwstr = (PWSTR)ptr;
      ptr += len;
    }
  if( !len )
    return 0;   // zero means no string is there
  // convert unitocde to ansi, and copy string to caller buffer
  UString.Length = UString.MaximumLength = len;
  UString.Buffer = pwstr;
  memset( &AString, 0, sizeof AString );
  Status = RtlUnicodeStringToAnsiString( &AString, &UString, TRUE );
  if( !NT_SUCCESS( Status ) )
    {
      SetLastErrorByStatus( Status );
      return 0;
    }
  nBufferMax--;  // save room for the null
  if( nBufferMax > AString.Length )
    nBufferMax = AString.Length;
  memcpy( lpBuffer, AString.Buffer, nBufferMax );
  lpBuffer[nBufferMax] = 0;
  RtlFreeAnsiString( &AString );
  return nBufferMax;
}
 
