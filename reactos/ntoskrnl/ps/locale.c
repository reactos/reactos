/* $Id: locale.c,v 1.2 2002/08/20 20:37:14 hyperion Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/locale.c
 * PURPOSE:         Locale support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

/*
 * Default setting: LANG_NEUTRAL, SUBLANG_NEUTRAL, SORT_DEFAULT
 */
LCID PsDefaultThreadLocaleId = 0;
LCID PsDefaultSystemLocaleId = 0;


#define VALUE_BUFFER_SIZE 256

/* FUNCTIONS *****************************************************************/

VOID
PiInitDefaultLocale(VOID)
/*
 * FUNCTION:
 *    Initializes the default locale.
 *    Reads default locale from registry, if available
 * ARGUMENTS:
 *    None.
 * Returns:
 *    None.
 */
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING KeyName;
   UNICODE_STRING ValueName;
   HANDLE KeyHandle;
   ULONG ValueLength;
   UCHAR ValueBuffer[VALUE_BUFFER_SIZE];
   PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
   UNICODE_STRING ValueString;
   ULONG LocaleValue;
   NTSTATUS Status;

   ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;

   /* read system locale */
   RtlInitUnicodeStringFromLiteral(&KeyName,
			L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\Language");
   RtlInitUnicodeStringFromLiteral(&ValueName,
			L"Default");

   InitializeObjectAttributes(&ObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
   Status = NtOpenKey(&KeyHandle,
		      KEY_QUERY_VALUE,
		      &ObjectAttributes);
   if (NT_SUCCESS(Status))
     {
	Status = NtQueryValueKey(KeyHandle,
				 &ValueName,
				 KeyValuePartialInformation,
				 ValueBuffer,
				 VALUE_BUFFER_SIZE,
				 &ValueLength);
	if ((NT_SUCCESS(Status)) && (ValueInfo->Type == REG_SZ))
	  {
	     ValueString.Length = ValueInfo->DataLength;
	     ValueString.MaximumLength = ValueInfo->DataLength;
	     ValueString.Buffer = (PWSTR)ValueInfo->Data;

	     Status = RtlUnicodeStringToInteger(&ValueString,
						10,
						&LocaleValue);
	     if (NT_SUCCESS(Status))
	       {
		  DPRINT("System locale: %08lu\n", LocaleValue);
		  PsDefaultSystemLocaleId = (LCID)LocaleValue;
	       }
	  }
	NtClose(KeyHandle);
     }

   /* read default thread locale */
   RtlInitUnicodeStringFromLiteral(&KeyName,
			L"\\Registry\\User\\.Default\\Control Panel\\International");
   RtlInitUnicodeStringFromLiteral(&ValueName,
			L"Locale");

   InitializeObjectAttributes(&ObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
   Status = NtOpenKey(&KeyHandle,
		      KEY_QUERY_VALUE,
		      &ObjectAttributes);
   if (NT_SUCCESS(Status))
     {
	Status = NtQueryValueKey(KeyHandle,
				 &ValueName,
				 KeyValuePartialInformation,
				 ValueBuffer,
				 VALUE_BUFFER_SIZE,
				 &ValueLength);
	if ((NT_SUCCESS(Status)) && (ValueInfo->Type == REG_SZ))
	  {
	     ValueString.Length = ValueInfo->DataLength;
	     ValueString.MaximumLength = ValueInfo->DataLength;
	     ValueString.Buffer = (PWSTR)ValueInfo->Data;

	     Status = RtlUnicodeStringToInteger(&ValueString,
						10,
						&LocaleValue);
	     if (NT_SUCCESS(Status))
	       {
		  DPRINT("Thread locale: %08lu\n", LocaleValue);
		  PsDefaultThreadLocaleId = (LCID)LocaleValue;
	       }
	  }
	NtClose(KeyHandle);
     }
}


NTSTATUS STDCALL
NtQueryDefaultLocale(IN BOOLEAN ThreadOrSystem,
		     OUT PLCID DefaultLocaleId)
/*
 * FUNCTION:
 *    Returns the default locale.
 * ARGUMENTS:
 *    ThreadOrSystem = If TRUE then the locale for this thread is returned,
 *                     otherwise the locale for the system is returned.
 *    DefaultLocaleId = Points to a variable that receives the locale id.
 * Returns:
 *    Status.
 */
{
   if (DefaultLocaleId == NULL)
     return STATUS_UNSUCCESSFUL;

   if (ThreadOrSystem == TRUE)
     {
	/* set thread locale */
	*DefaultLocaleId = PsDefaultThreadLocaleId;
     }
   else
     {
	/* set system locale */
	*DefaultLocaleId = PsDefaultSystemLocaleId;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtSetDefaultLocale(IN BOOLEAN ThreadOrSystem,
		   IN LCID DefaultLocaleId)
/*
 * FUNCTION:
 *    Sets the default locale.
 * ARGUMENTS:
 *    ThreadOrSystem = If TRUE then the locale for this thread is set,
 *                     otherwise the locale for the system is set.
 *    DefaultLocaleId = The locale id to be set.
 * Returns:
 *    Status.
 */
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING KeyName;
   UNICODE_STRING ValueName;
   HANDLE KeyHandle;
   ULONG ValueLength;
   WCHAR ValueBuffer[20];
   HANDLE UserKey = NULL;
   NTSTATUS Status;

   if (ThreadOrSystem == TRUE)
     {
	/* thread locale */
	Status = RtlOpenCurrentUser(KEY_WRITE,
				    &UserKey);
	if (!NT_SUCCESS(Status))
	  return(Status);
	RtlInitUnicodeStringFromLiteral(&KeyName,
			     L"Control Panel\\International");
	RtlInitUnicodeStringFromLiteral(&ValueName,
			     L"Locale");
     }
   else
     {
	/* system locale */
	RtlInitUnicodeStringFromLiteral(&KeyName,
			     L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\Language");
	RtlInitUnicodeStringFromLiteral(&ValueName,
			     L"Default");
     }

   InitializeObjectAttributes(&ObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      UserKey,
			      NULL);
   Status = NtOpenKey(&KeyHandle,
		      KEY_SET_VALUE,
		      &ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	if (UserKey != NULL)
	  {
	     NtClose(UserKey);
	  }
	return(Status);
     }

   ValueLength = swprintf(ValueBuffer,
			  L"%08lu",
			  (ULONG)DefaultLocaleId);
   ValueLength = (ValueLength + 1) * sizeof(WCHAR);

   Status = NtSetValueKey(KeyHandle,
			  &ValueName,
			  0,
			  REG_SZ,
			  ValueBuffer,
			  ValueLength);

   NtClose(KeyHandle);
   if (UserKey != NULL)
     {
	NtClose(UserKey);
     }

   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   if (ThreadOrSystem == TRUE)
     {
	/* set thread locale */
	DPRINT("Thread locale: %08lu\n", DefaultLocaleId);
	PsDefaultThreadLocaleId = DefaultLocaleId;
     }
   else
     {
	/* set system locale */
	DPRINT("System locale: %08lu\n", DefaultLocaleId);
	PsDefaultSystemLocaleId = DefaultLocaleId;
     }

   return(STATUS_SUCCESS);
}

/* EOF */
