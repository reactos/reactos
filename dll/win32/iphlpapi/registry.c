#include "iphlpapi_private.h"

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

  if (MaxAdapterName == -1)
    return 0;

  ValueLen = MaxAdapterName;
  Value = (PWCHAR)HeapAlloc( GetProcessHeap(), 0, MaxAdapterName * sizeof(WCHAR) );
  if (!Value) return 0;

  Status = RegEnumKeyExW( RegHandle, n, Value, &ValueLen,
			  NULL, NULL, NULL, NULL );
  if (Status != ERROR_SUCCESS) {
    HeapFree(GetProcessHeap(), 0, Value);
    return 0;
  } else {
    Value[ValueLen] = 0;
    return Value;
  }
}

void ConsumeChildKeyName( PWCHAR Name ) {
  if (Name) HeapFree( GetProcessHeap(), 0, Name );
}

PVOID QueryRegistryValue(HANDLE RegHandle, PWCHAR ValueName, LPDWORD RegistryType, LPDWORD Length)
{
    PVOID ReadValue = NULL;
    DWORD Error;

    *Length = 0;
    *RegistryType = REG_NONE;

    while (TRUE)
    {
        Error = RegQueryValueExW(RegHandle, ValueName, NULL, RegistryType, ReadValue, Length);
        if (Error == ERROR_SUCCESS)
        {
            if (ReadValue) break;
        }
        else if (Error == ERROR_MORE_DATA)
        {
            HeapFree(GetProcessHeap(), 0, ReadValue);
        }
        else break;

        ReadValue = HeapAlloc(GetProcessHeap(), 0, *Length);
        if (!ReadValue) return NULL;
    }

    if (Error != ERROR_SUCCESS)
    {
        if (ReadValue) HeapFree(GetProcessHeap(), 0, ReadValue);

        *Length = 0;
        *RegistryType = REG_NONE;
        ReadValue = NULL;
    }

    return ReadValue;
}

PWCHAR TerminateReadString(PWCHAR String, DWORD Length)
{
    PWCHAR TerminatedString;

    TerminatedString = HeapAlloc(GetProcessHeap(), 0, Length + sizeof(WCHAR));
    if (TerminatedString == NULL)
        return NULL;

    memcpy(TerminatedString, String, Length);

    TerminatedString[Length / sizeof(WCHAR)] = UNICODE_NULL;

    return TerminatedString;
}

PWCHAR QueryRegistryValueString( HANDLE RegHandle, PWCHAR ValueName )
{
    PWCHAR String, TerminatedString;
    DWORD Type, Length;

    String = QueryRegistryValue(RegHandle, ValueName, &Type, &Length);
    if (!String) return NULL;
    if (Type != REG_SZ)
    {
        DbgPrint("Type mismatch for %S (%d != %d)\n", ValueName, Type, REG_SZ);
        //HeapFree(GetProcessHeap(), 0, String);
        //return NULL;
    }

    TerminatedString = TerminateReadString(String, Length);
    HeapFree(GetProcessHeap(), 0, String);
    if (!TerminatedString) return NULL;

    return TerminatedString;
}

void ConsumeRegValueString( PWCHAR Value ) {
  if (Value) HeapFree(GetProcessHeap(), 0, Value);
}

PWCHAR *QueryRegistryValueStringMulti( HANDLE RegHandle, PWCHAR ValueName ) {
    PWCHAR String, TerminatedString, Tmp;
    PWCHAR *Table;
    DWORD Type, Length, i, j;

    String = QueryRegistryValue(RegHandle, ValueName, &Type, &Length);
    if (!String) return NULL;
    if (Type != REG_MULTI_SZ)
    {
        DbgPrint("Type mismatch for %S (%d != %d)\n", ValueName, Type, REG_MULTI_SZ);
        //HeapFree(GetProcessHeap(), 0, String);
        //return NULL;
    }

    TerminatedString = TerminateReadString(String, Length);
    HeapFree(GetProcessHeap(), 0, String);
    if (!TerminatedString) return NULL;

    for (Tmp = TerminatedString, i = 0; *Tmp; Tmp++, i++) while (*Tmp) Tmp++;

    Table = HeapAlloc(GetProcessHeap(), 0, (i + 1) * sizeof(PWCHAR));
    if (!Table)
    {
        HeapFree(GetProcessHeap(), 0, TerminatedString);
        return NULL;
    }

    for (Tmp = TerminatedString, j = 0; *Tmp; Tmp++, j++)
    {
        PWCHAR Orig = Tmp;

        for (i = 0; *Tmp; i++, Tmp++);

        Table[j] = HeapAlloc(GetProcessHeap(), 0, i * sizeof(WCHAR));
        memcpy(Table[j], Orig, i * sizeof(WCHAR));
    }

    Table[j] = NULL;

    HeapFree(GetProcessHeap(), 0, TerminatedString);

    return Table;
}
