/* $Id:
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/psxdll/sys/utsname/uname.c
 * PURPOSE:     Get name of current system
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              19/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <string.h>
#include <sys/utsname.h>
#include <psx/stdlib.h>
#include <psx/debug.h>
#include <psx/errno.h>

int uname(struct utsname *name)
{
 NTSTATUS          nErrCode;
 OBJECT_ATTRIBUTES oaKeyAttribs;
 UNICODE_STRING    wstrKeyPath;
 UNICODE_STRING    wstrValueName;
 UNICODE_STRING    wstrValueData;
 ANSI_STRING       strValueData;
 PKEY_VALUE_PARTIAL_INFORMATION pkvpiKeyValue;
 ULONG             nKeyValueSize;
 HANDLE            hKey;

 /* system name and version info are fixed strings, at the moment */ /* FIXME? */
 strncpy(name->sysname, "ReactOS"  , 255);
 strncpy(name->release, "0.0"      , 255);
 strncpy(name->version, "pre-alpha", 255);

 /* hardware identifier */
 /* FIXME: this should definitely be determined programmatically */
 strncpy(name->machine, "i386"     , 255);

 /* we use the active computer's name as the node name */
 /* TODO: POSIX-style registry functions */

 /* initialize the registry key path */
 RtlInitUnicodeString(
  &wstrKeyPath,
  L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"
 );

 /* initialize object attributes */
 oaKeyAttribs.Length = sizeof(OBJECT_ATTRIBUTES);
 oaKeyAttribs.RootDirectory = NULL;
 oaKeyAttribs.ObjectName = &wstrKeyPath;
 oaKeyAttribs.Attributes = OBJ_CASE_INSENSITIVE /* | OBJ_OPENLINK | OBJ_OPENIF */ /* FIXME? */;
 oaKeyAttribs.SecurityDescriptor = NULL;
 oaKeyAttribs.SecurityQualityOfService = NULL;

 /* open the key object */
 nErrCode = NtOpenKey
 (
  &hKey,
  KEY_QUERY_VALUE,
  &oaKeyAttribs
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {

  ERR("NtOpenKey() failed with status 0x%08X", nErrCode);
  errno = __status_to_errno(nErrCode);
  return (-1);
 }

 /* initialize the registry value name */
 RtlInitUnicodeString(&wstrValueName, L"ComputerName");

 /* fake query - null buffer and zero length to pre-fetch the appropriate buffer size */
 nErrCode = NtQueryValueKey
 (
  hKey,
  &wstrValueName,
  KeyValuePartialInformation,
  NULL,
  0,
  &nKeyValueSize
 );

 /* success */
 if(nErrCode == (NTSTATUS)STATUS_BUFFER_TOO_SMALL)
 {
 
  /* allocate the appropriate buffer size */
  if(nKeyValueSize < sizeof(KEY_VALUE_PARTIAL_INFORMATION)) /* just to be sure */
   nKeyValueSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION);

  pkvpiKeyValue = __malloc(nKeyValueSize);

 }
 /* failure */
 else
 {
 
  ERR("NtQueryValueKey() failed with status 0x%08X", nErrCode);
  NtClose(hKey);
  errno = __status_to_errno(nErrCode);
  return (-1);
 }

 /* query the value */
 nErrCode = NtQueryValueKey
 (
  hKey,
  &wstrValueName,
  KeyValuePartialInformation,
  pkvpiKeyValue,
  nKeyValueSize,
  &nKeyValueSize
 );

 /* close the key handle (not needed anymore) */
 NtClose(hKey);

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  ERR("NtQueryValueKey() failed with status 0x%08X", nErrCode);
  __free(pkvpiKeyValue);
  errno = __status_to_errno(nErrCode);
  return (-1);
 }

 /* make wstrValueData refer to the Data field of the key value information */
 wstrValueData.Length = pkvpiKeyValue->DataLength;
 wstrValueData.MaximumLength = wstrValueData.Length;
 wstrValueData.Buffer = (PWCHAR)&(pkvpiKeyValue->Data[0]);

 /* make strValueData refer to the nodename buffer */
 strValueData.Length = 0;
 strValueData.MaximumLength = 254;
 strValueData.Buffer = name->nodename;

 RtlUnicodeStringToAnsiString
 (
  &strValueData,
  &wstrValueData,
  FALSE
 );

 /* free the key value buffer */
 __free(pkvpiKeyValue);

 /* null-terminate the returned string */
 name->nodename[strValueData.Length] = '0';

 INFO
 (
   " \
name->sysname = \"%s\"\n\
tname->nodename = \"%s\"\n\
tname->release = \"%s\"\n\
tname->version = \"%s\"\n\
tname->machine = \"%s\"",
   name->sysname,
   name->nodename,
   name->release,
   name->version,
   name->machine
 );

 /* success */
 return (0);

}

/* EOF */

