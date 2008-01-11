#include "precomp.h"

NTSTATUS TiGetProtocolNumber(
  PUNICODE_STRING FileName,
  PULONG Protocol)
/*
 * FUNCTION: Returns the protocol number from a file name
 * ARGUMENTS:
 *     FileName = Pointer to string with file name
 *     Protocol = Pointer to buffer to put protocol number in
 * RETURNS:
 *     Status of operation
 */
{
  UNICODE_STRING us;
  NTSTATUS Status;
  ULONG Value;
  PWSTR Name;

  TI_DbgPrint(MAX_TRACE, ("Called. FileName (%wZ).\n", FileName));

  Name = FileName->Buffer;

  if (*Name++ != (WCHAR)L'\\')
    return STATUS_UNSUCCESSFUL;

  if (*Name == L'\0')
    return STATUS_UNSUCCESSFUL;

  RtlInitUnicodeString(&us, Name);

  Status = RtlUnicodeStringToInteger(&us, 10, &Value);
  if (!NT_SUCCESS(Status) || ((Value > 255)))
    return STATUS_UNSUCCESSFUL;

  *Protocol = Value;

  return STATUS_SUCCESS;
}

