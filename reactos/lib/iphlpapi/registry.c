#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include "ipregprivate.h"

#include "debug.h"

int GetLongestChildKeyName( HANDLE RegHandle ) {
  LONG Status;
  DWORD MaxAdapterName;

  Status = RegQueryInfoKeyW(RegHandle, 
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
		       PWCHAR ChildKeyName, 
		       PHKEY ReturnHandle ) {
  return RegOpenKeyExW( RegHandle, 
			ChildKeyName, 
			0,
			KEY_READ,
			ReturnHandle );
}

/*
 * Yields a malloced value that must be freed.
 */

PWCHAR GetNthChildKeyName( HANDLE RegHandle, DWORD n ) {
  LONG Status;
  int MaxAdapterName = GetLongestChildKeyName( RegHandle );
  PWCHAR Value;
  DWORD ValueLen;

  if (MaxAdapterName == -1) {
    RegCloseKey( RegHandle );
    return 0;
  }

  ValueLen = MaxAdapterName;
  Value = (PWCHAR)malloc( MaxAdapterName * sizeof(WCHAR) );
  Status = RegEnumKeyExW( RegHandle, n, Value, &ValueLen, 
			  NULL, NULL, NULL, NULL );
  if (Status != ERROR_SUCCESS)
    return 0;
  else {
    Value[ValueLen] = 0;
    return Value;
  }
}

void ConsumeChildKeyName( PWCHAR Name ) {
  if (Name) free( Name );
}

PWCHAR QueryRegistryValueString( HANDLE RegHandle, PWCHAR ValueName ) {
  PWCHAR Name;
  DWORD ReturnedSize = 0;
  
  if (RegQueryValueExW( RegHandle, ValueName, NULL, NULL, NULL, 
			&ReturnedSize ) != 0) 
    return 0;
  else {
    Name = malloc( (ReturnedSize + 1) * sizeof(WCHAR) );
    RegQueryValueExW( RegHandle, ValueName, NULL, NULL, (PVOID)Name, 
		      &ReturnedSize );
    Name[ReturnedSize] = 0;
    return Name;
  }
}

void ConsumeRegValueString( PWCHAR Value ) {
  if (Value) free(Value);
}

PWCHAR *QueryRegistryValueStringMulti( HANDLE RegHandle, PWCHAR ValueName ) {
  return 0; /* FIXME if needed */
}

void ConsumeRegValueStringMulti( PWCHAR *Value ) {
  PWCHAR *Orig = Value;
  if (Value) {
    while (*Value) {
      free(*Value);
    }
    free(Orig);
  }
}
