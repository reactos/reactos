#include "iphlpapi_private.h"

#include "debug.h"

int GetLongestChildKeyName( HANDLE RegHandle ) {
  LONG Status;
  DWORD MaxAdapterName;

  Status = RegQueryInfoKeyA(RegHandle, 
			    NULL, 
			    NULL, 
			    NULL, 
			    NULL, 
			    &MaxAdapterName, 
			    NULL, 
			    NULL, 
			    NULL, 
			    NULL, 
			    NULL, 
			    NULL);
  if (Status == ERROR_SUCCESS) 
    return MaxAdapterName + 1;
  else
    return -1;
}

LONG OpenChildKeyRead( HANDLE RegHandle, 
		       PCHAR ChildKeyName, 
		       PHKEY ReturnHandle ) {
  return RegOpenKeyExA( RegHandle, 
			ChildKeyName, 
			0,
			KEY_READ,
			ReturnHandle );
}

/*
 * Yields a malloced value that must be freed.
 */

PCHAR GetNthChildKeyName( HANDLE RegHandle, DWORD n ) {
  LONG Status;
  int MaxAdapterName = GetLongestChildKeyName( RegHandle );
  PCHAR Value;
  DWORD ValueLen;

  if (MaxAdapterName == -1) {
    RegCloseKey( RegHandle );
    return 0;
  }

  ValueLen = MaxAdapterName;
  Value = (PCHAR)HeapAlloc( GetProcessHeap(), 0, MaxAdapterName );
  Status = RegEnumKeyExA( RegHandle, n, Value, &ValueLen, 
			  NULL, NULL, NULL, NULL );
  if (Status != ERROR_SUCCESS)
    return 0;
  else {
    Value[ValueLen] = 0;
    return Value;
  }
}

void ConsumeChildKeyName( PCHAR Name ) {
  if (Name) HeapFree( GetProcessHeap(), 0, Name );
}

PCHAR QueryRegistryValueString( HANDLE RegHandle, PCHAR ValueName ) {
  PCHAR Name;
  DWORD ReturnedSize = 0;
  
  if (RegQueryValueExA( RegHandle, ValueName, NULL, NULL, NULL, 
			&ReturnedSize ) != 0) 
    return 0;
  else {
    Name = malloc( (ReturnedSize + 1) * sizeof(WCHAR) );
    RegQueryValueExA( RegHandle, ValueName, NULL, NULL, (PVOID)Name, 
		      &ReturnedSize );
    Name[ReturnedSize] = 0;
    return Name;
  }
}

void ConsumeRegValueString( PCHAR Value ) {
  if (Value) free(Value);
}

PWCHAR *QueryRegistryValueStringMulti( HANDLE RegHandle, PWCHAR ValueName ) {
  return 0; /* FIXME if needed */
}

void ConsumeRegValueStringMulti( PCHAR *Value ) {
  PCHAR *Orig = Value;
  if (Value) {
    while (*Value) {
      free(*Value);
    }
    free(Orig);
  }
}
