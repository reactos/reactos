/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/bootlog.c
 * PURPOSE:         Boot log file support
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, IopInitBootLog)
#pragma alloc_text(INIT, IopStartBootLog)
#endif

/* GLOBALS ******************************************************************/

static BOOLEAN IopBootLogCreate = FALSE;
static BOOLEAN IopBootLogEnabled = FALSE;
static BOOLEAN IopLogFileEnabled = FALSE;
static ULONG IopLogEntryCount = 0;
static ERESOURCE IopBootLogResource;


/* FUNCTIONS ****************************************************************/

VOID INIT_FUNCTION
IopInitBootLog(BOOLEAN StartBootLog)
{
  ExInitializeResourceLite(&IopBootLogResource);
  if (StartBootLog) IopStartBootLog();
}


VOID INIT_FUNCTION
IopStartBootLog(VOID)
{
  IopBootLogCreate = TRUE;
  IopBootLogEnabled = TRUE;
}


VOID
IopStopBootLog(VOID)
{
  IopBootLogEnabled = FALSE;
}


VOID
IopBootLog(PUNICODE_STRING DriverName,
	   BOOLEAN Success)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR Buffer[256];
  WCHAR ValueNameBuffer[8];
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  HANDLE ControlSetKey;
  HANDLE BootLogKey;
  NTSTATUS Status;

  if (IopBootLogEnabled == FALSE)
    return;

  ExAcquireResourceExclusiveLite(&IopBootLogResource, TRUE);

  DPRINT("Boot log: %wS %wZ\n",
	 Success ? L"Loaded driver" : L"Did not load driver",
	 DriverName);

  swprintf(Buffer,
	   L"%ws %wZ",
	   Success ? L"Loaded driver" : L"Did not load driver",
	   DriverName);

  swprintf(ValueNameBuffer,
	   L"%lu",
	   IopLogEntryCount);

  RtlInitUnicodeString(&KeyName,
		       L"\\Registry\\Machine\\System\\CurrentControlSet");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = ZwOpenKey(&ControlSetKey,
		     KEY_ALL_ACCESS,
		     &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwOpenKey() failed (Status %lx)\n", Status);
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }

  RtlInitUnicodeString(&KeyName, L"BootLog");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     ControlSetKey,
			     NULL);
  Status = ZwCreateKey(&BootLogKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwCreateKey() failed (Status %lx)\n", Status);
      ZwClose(ControlSetKey);
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }

  RtlInitUnicodeString(&ValueName, ValueNameBuffer);
  Status = ZwSetValueKey(BootLogKey,
			 &ValueName,
			 0,
			 REG_SZ,
			 (PVOID)Buffer,
			 (wcslen(Buffer) + 1) * sizeof(WCHAR));
  ZwClose(BootLogKey);
  ZwClose(ControlSetKey);

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
    }
  else
    {
      IopLogEntryCount++;
    }

  ExReleaseResourceLite(&IopBootLogResource);
}


static NTSTATUS
IopWriteLogFile(PWSTR LogText)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING FileName;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  PWSTR CrLf = L"\r\n";
  NTSTATUS Status;

  DPRINT("IopWriteLogFile() called\n");

  RtlInitUnicodeString(&FileName,
		       L"\\SystemRoot\\rosboot.log");
  InitializeObjectAttributes(&ObjectAttributes,
			     &FileName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     NULL,
			     NULL);

  Status = ZwCreateFile(&FileHandle,
			FILE_APPEND_DATA,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			0,
			0,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwCreateFile() failed (Status %lx)\n", Status);
      return Status;
    }

  if (LogText != NULL)
    {
      Status = ZwWriteFile(FileHandle,
			   NULL,
			   NULL,
			   NULL,
			   &IoStatusBlock,
			   (PVOID)LogText,
			   wcslen(LogText) * sizeof(WCHAR),
			   NULL,
			   NULL);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("ZwWriteFile() failed (Status %lx)\n", Status);
	  ZwClose(FileHandle);
	  return Status;
	}
    }

  /* L"\r\n" */
  Status = ZwWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       (PVOID)CrLf,
		       2 * sizeof(WCHAR),
		       NULL,
		       NULL);

  ZwClose(FileHandle);

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwWriteFile() failed (Status %lx)\n", Status);
    }

  return Status;
}


static NTSTATUS
IopCreateLogFile(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING FileName;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  LARGE_INTEGER ByteOffset;
  WCHAR Signature;
  NTSTATUS Status;

  DPRINT("IopSaveBootLogToFile() called\n");

  ExAcquireResourceExclusiveLite(&IopBootLogResource, TRUE);

  RtlInitUnicodeString(&FileName,
		       L"\\SystemRoot\\rosboot.log");
  InitializeObjectAttributes(&ObjectAttributes,
			     &FileName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     NULL,
			     NULL);

  Status = ZwCreateFile(&FileHandle,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			0,
			0,
			FILE_SUPERSEDE,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwCreateFile() failed (Status %lx)\n", Status);
      return Status;
    }

  ByteOffset.QuadPart = (LONGLONG)0;

  Signature = 0xFEFF;
  Status = ZwWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       (PVOID)&Signature,
		       sizeof(WCHAR),
		       &ByteOffset,
		       NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwWriteKey() failed (Status %lx)\n", Status);
    }

  ZwClose(FileHandle);

  return Status;
}


VOID
IopSaveBootLogToFile(VOID)
{
  PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;
  WCHAR ValueNameBuffer[8];
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  HANDLE KeyHandle;
  ULONG BufferSize;
  ULONG ResultLength;
  ULONG i;
  NTSTATUS Status;

  if (IopBootLogCreate == FALSE)
    return;

  DPRINT("IopSaveBootLogToFile() called\n");

  ExAcquireResourceExclusiveLite(&IopBootLogResource, TRUE);

  Status = IopCreateLogFile();
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("IopCreateLogFile() failed (Status %lx)\n", Status);
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }

  //Status = IopWriteLogFile(L"ReactOS "KERNEL_VERSION_STR);

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("IopWriteLogFile() failed (Status %lx)\n", Status);
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }

  Status = IopWriteLogFile(NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("IopWriteLogFile() failed (Status %lx)\n", Status);
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }


  BufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256 * sizeof(WCHAR);
  KeyInfo = ExAllocatePool(PagedPool,
			   BufferSize);
  if (KeyInfo == NULL)
    {
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }

  RtlInitUnicodeString(&KeyName,
		       L"\\Registry\\Machine\\System\\CurrentControlSet\\BootLog");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = ZwOpenKey(&KeyHandle,
		     KEY_ALL_ACCESS,
		     &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(KeyInfo);
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }

  for (i = 0; ; i++)
    {
      swprintf(ValueNameBuffer,
	       L"%lu", i);

      RtlInitUnicodeString(&ValueName,
			   ValueNameBuffer);

      Status = ZwQueryValueKey(KeyHandle,
			       &ValueName,
			       KeyValuePartialInformation,
			       KeyInfo,
			       BufferSize,
			       &ResultLength);
      if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
	{
	  break;
	}

      if (!NT_SUCCESS(Status))
	{
	  ZwClose(KeyHandle);
	  ExFreePool(KeyInfo);
	  ExReleaseResourceLite(&IopBootLogResource);
	  return;
	}

      Status = IopWriteLogFile((PWSTR)&KeyInfo->Data);
      if (!NT_SUCCESS(Status))
	{
	  ZwClose(KeyHandle);
	  ExFreePool(KeyInfo);
	  ExReleaseResourceLite(&IopBootLogResource);
	  return;
	}

      /* Delete keys */
      ZwDeleteValueKey(KeyHandle,
		       &ValueName);
    }

  ZwClose(KeyHandle);

  ExFreePool(KeyInfo);

  IopLogFileEnabled = TRUE;
  ExReleaseResourceLite(&IopBootLogResource);

  DPRINT("IopSaveBootLogToFile() done\n");
}

/* EOF */
