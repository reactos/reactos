/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/string.c
 * PURPOSE:     String management routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>


NDIS_STATUS
EXPORT
NdisAnsiStringToUnicodeString(
    IN OUT  PNDIS_STRING        DestinationString,
    IN      PNDIS_ANSI_STRING   SourceString)
/*
 * FUNCTION: Converts an ANSI string to an NDIS (unicode) string
 * ARGUMENTS:
 *     DestinationString = Address of buffer to place converted string in
 *     SourceString      = Pointer to ANSI string to be converted
 */
{
	return (NDIS_STATUS)RtlAnsiStringToUnicodeString(
        (PUNICODE_STRING)DestinationString,
        (PANSI_STRING)SourceString, FALSE);
}


BOOLEAN
EXPORT
NdisEqualString(
    IN  PNDIS_STRING    String1,
    IN  PNDIS_STRING    String2,
    IN  BOOLEAN         CaseInsensitive)
/*
 * FUNCTION: Tests two strings for equality
 * ARGUMENTS:
 *     String1         = Pointer to first string
 *     String2         = Pointer to second string
 *     CaseInsensitive = TRUE if the compare should be case insensitive
 */
{
    return RtlEqualUnicodeString(
        (PUNICODE_STRING)String1,
        (PUNICODE_STRING)String2,
        CaseInsensitive);
}


VOID
EXPORT
NdisInitAnsiString(
    IN OUT  PNDIS_ANSI_STRING   DestinationString,
    IN      PCSTR               SourceString)
/*
 * FUNCTION: Initializes an ANSI string
 * ARGUMENTS:
 *     DestinationString = Address of buffer to place string in
 *     SourceString      = Pointer to null terminated ANSI string
 */
{
    RtlInitString(
        (PANSI_STRING)DestinationString,
        (PCSZ)SourceString);
}


VOID
EXPORT
NdisInitializeString(
    IN OUT  PNDIS_STRING    DestinationString,
    IN      PUCHAR          SourceString)
/*
 * FUNCTION: Initializes an NDIS (unicode) string
 * ARGUMENTS:
 *     DestinationString = Address of buffer to place string in
 *     SourceString      = Pointer to null terminated ANSI string
 */
{
    ANSI_STRING AnsiString;

    RtlInitAnsiString(
        &AnsiString,
        (PCSZ)SourceString);

    RtlAnsiStringToUnicodeString(
        (PUNICODE_STRING)DestinationString,
        &AnsiString,
        TRUE);
}


VOID
EXPORT
NdisInitUnicodeString(
    IN OUT  PNDIS_STRING    DestinationString,
    IN      PCWSTR          SourceString)
/*
 * FUNCTION: Initializes an unicode string
 * ARGUMENTS:
 *     DestinationString = Address of buffer to place string in
 *     SourceString      = Pointer to null terminated unicode string
 */
{
    RtlInitUnicodeString(
        (PUNICODE_STRING)DestinationString,
        SourceString);
}


NDIS_STATUS
EXPORT
NdisUnicodeStringToAnsiString(
    IN OUT  PNDIS_ANSI_STRING   DestinationString,
    IN      PNDIS_STRING        SourceString)
/*
 * FUNCTION: Converts an NDIS (unicode) string to an ANSI string
 * ARGUMENTS:
 *     DestinationString = Address of buffer to place converted string in
 *     SourceString      = Pointer to unicode string to be converted
 */
{
	return (NDIS_STATUS)RtlUnicodeStringToAnsiString(
        (PANSI_STRING)DestinationString,
        (PUNICODE_STRING)SourceString,
        FALSE);
}


NTSTATUS
EXPORT
NdisUpcaseUnicodeString(
    OUT PUNICODE_STRING DestinationString,  
    IN  PUNICODE_STRING SourceString)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
