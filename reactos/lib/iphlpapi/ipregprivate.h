#ifndef IPREGPRIVATE_H
#define IPREGPRIVATE_H

int GetLongestChildKeyName( HANDLE RegHandle );
LONG OpenChildKeyRead( HANDLE RegHandle,
		       PWCHAR ChildKeyName,
		       PHKEY ReturnHandle );
PWCHAR GetNthChildKeyName( HANDLE RegHandle, DWORD n );
void ConsumeChildKeyName( PWCHAR Name );
PWCHAR QueryRegistryValueString( HANDLE RegHandle, PWCHAR ValueName );
void ConsumeRegValueString( PWCHAR NameServer );

#endif/*IPREGPRIVATE_H*/
