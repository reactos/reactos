#ifndef IPREGPRIVATE_H
#define IPREGPRIVATE_H

int GetLongestChildKeyName( HANDLE RegHandle );
LONG OpenChildKeyRead( HANDLE RegHandle,
		       PCHAR ChildKeyName,
		       PHKEY ReturnHandle );
PCHAR GetNthChildKeyName( HANDLE RegHandle, DWORD n );
void ConsumeChildKeyName( PCHAR Name );
PCHAR QueryRegistryValueString( HANDLE RegHandle, PCHAR ValueName );
void ConsumeRegValueString( PCHAR NameServer );

#endif/*IPREGPRIVATE_H*/
