
/*
 * Win32 File Api functions
 * Author: Boudewijn Dekker
 * to do: many more to add ..
 */


#include <windows.h>

// AnsiOrOemtoUnicode
// pupose: internal procedure used in file api 

NTSTATUS AnsiOrOemtoUnicode(PUNICODE_STRING DestinationString,PANSI_STRING SourceString, BOOLEAN AllocateDestinationString);


BOOLEAN  bIsFileApiAnsi; // set the file api to ansi or oem


NTSTATUS AnsiOrOemtoUnicode(PUNICODE_STRING DestinationString,PANSI_STRING SourceString, BOOLEAN AllocateDestinationString)
{
	if ( bIsFileApiAnsi ) {
		return __AnsiStringToUnicodeString(DestinationString, SourceString, AllocateDestinationString);
	else
		return __OemStringToUnicodeString(DestinationString, SourceString, AllocateDestinationString);

}


WINBASEAPI
VOID
WINAPI
SetFileApisToOEM(VOID)
{
	bIsFileApiAnsi = FALSE;
	return;	
}



WINBASEAPI
VOID
WINAPI
SetFileApisToANSI(VOID)
{
	bIsFileApiAnsi = TRUE;
	return;	
}


WINBASEAPI
BOOLEAN
WINAPI
AreFileApisANSI(VOID)
{
	return  bIsFileApiAnsi;
	
}
