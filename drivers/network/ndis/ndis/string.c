/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/string.c
 * PURPOSE:     String management routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   Vizzini 08-Oct-2003  Error checking, documentation, and formatting
 */

#include "ndissys.h"


/*
 * @implemented
 */
#undef NdisAnsiStringToUnicodeString
NDIS_STATUS
EXPORT
NdisAnsiStringToUnicodeString(
    IN OUT  PNDIS_STRING   DestinationString,
    IN      PANSI_STRING   SourceString)
/*
 * FUNCTION: Converts an ANSI string to an NDIS (unicode) string
 * ARGUMENTS:
 *     DestinationString = Address of buffer to place converted string in
 *     SourceString      = Pointer to ANSI string to be converted
 * NOTES:
 *     - caller must be running at IRQL = PASSIVE_LEVEL
 */
{
  PAGED_CODE();
  ASSERT(DestinationString);
  ASSERT(SourceString);

  return (NDIS_STATUS)RtlAnsiStringToUnicodeString(
      (PUNICODE_STRING)DestinationString,
      (PANSI_STRING)SourceString, FALSE);
}



/*
 * @implemented
 */
#undef NdisEqualString
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
 * NOTES:
 *     - caller must be at IRQL = PASSIVE_LEVEL
 */
{
  PAGED_CODE();
  ASSERT(String1);
  ASSERT(String2);

  return RtlEqualUnicodeString((PUNICODE_STRING)String1,
      (PUNICODE_STRING)String2,
      CaseInsensitive);
}


/*
 * @implemented
 */
#undef NdisInitAnsiString
VOID
EXPORT
NdisInitAnsiString(
    IN OUT  PANSI_STRING   DestinationString,
    IN      PCSTR          SourceString)
/*
 * FUNCTION: Initializes an ANSI string
 * ARGUMENTS:
 *     DestinationString = Address of buffer to place string in
 *     SourceString      = Pointer to null terminated ANSI string
 * NOTES:
 *     - Caller must be at IRQL <= DISPATCH_LEVEL
 */
{
  ASSERT(DestinationString);
  ASSERT(SourceString);

  RtlInitString((PANSI_STRING)DestinationString, (PCSZ)SourceString);
}


/*
 * @implemented
 */
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
 * NOTES:
 *     - Must be called at IRQL = PASSIVE_LEVEL
 */
{
  ANSI_STRING AnsiString;

  PAGED_CODE();
  ASSERT(DestinationString);
  ASSERT(SourceString);

  RtlInitAnsiString(&AnsiString, (PCSZ)SourceString);

  RtlAnsiStringToUnicodeString((PUNICODE_STRING)DestinationString, &AnsiString, TRUE);
}


/*
 * @implemented
 */
#undef NdisInitUnicodeString
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
 * NOTES:
 *     - call with IRQL <= DISPATCH_LEVEL
 */
{
  ASSERT(DestinationString);
  ASSERT(SourceString);

  RtlInitUnicodeString((PUNICODE_STRING)DestinationString, SourceString);
}


/*
 * @implemented
 */
#undef NdisUnicodeStringToAnsiString
NDIS_STATUS
EXPORT
NdisUnicodeStringToAnsiString(
    IN OUT  PANSI_STRING   DestinationString,
    IN      PNDIS_STRING   SourceString)
/*
 * FUNCTION: Converts an NDIS (unicode) string to an ANSI string
 * ARGUMENTS:
 *     DestinationString = Address of buffer to place converted string in
 *     SourceString      = Pointer to unicode string to be converted
 * NOTES:
 *     - must be called at IRQL = PASSIVE_LEVEL
 */
{
  PAGED_CODE();
  ASSERT(DestinationString);
  ASSERT(SourceString);

  return (NDIS_STATUS)RtlUnicodeStringToAnsiString(
      (PANSI_STRING)DestinationString,
      (PUNICODE_STRING)SourceString,
      FALSE);
}


/*
 * @implemented
 */
#undef NdisUpcaseUnicodeString
NTSTATUS
EXPORT
NdisUpcaseUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN  PUNICODE_STRING SourceString)
/*
 * FUNCTION: Uppercase a UNICODE string
 * ARGUMENTS:
 *     DestinationString: caller-allocated space for the uppercased string
 *     SourceString: string to be uppercased
 * NOTES:
 *     - Currently requires caller to allocate destination string - XXX is this right?
 *     - callers must be running at IRQL = PASSIVE_LEVEL
 */
{
  PAGED_CODE();
  ASSERT(SourceString);
  ASSERT(DestinationString);

  return RtlUpcaseUnicodeString (DestinationString, SourceString, FALSE );
}

/* EOF */

