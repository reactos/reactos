/* $Id: ppb.c,v 1.3 2000/02/18 00:49:11 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/ppb.c
 * PURPOSE:         Process parameters functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#define WIN32_NO_PEHDR
#include <ddk/ntddk.h>
#include <wchar.h>
#include <string.h>
#include <pe.h>
#include <internal/i386/segment.h>
#include <ntdll/ldr.h>
#include <internal/teb.h>
#include <ntdll/base.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* MACROS ****************************************************************/

#define NORMALIZE(x,addr)   {if(x) x=(VOID*)((ULONG)(x)+(ULONG)(addr));}
#define DENORMALIZE(x,addr) {if(x) x=(VOID*)((ULONG)(x)-(ULONG)(addr));}
#define ALIGN(x,align)      (((ULONG)(x)+(align)-1UL)&(~((align)-1UL)))


/* FUNCTIONS ****************************************************************/

VOID STDCALL RtlAcquirePebLock(VOID)
{

}


VOID STDCALL RtlReleasePebLock(VOID)
{

}

static
inline
VOID
RtlpCopyParameterString (
	PWCHAR		*Ptr,
	PUNICODE_STRING	Destination,
	PUNICODE_STRING	Source,
	ULONG		Size
	)
{
	Destination->Length = Source->Length;
	Destination->MaximumLength = Size ? Size : Source->MaximumLength;
	Destination->Buffer = (PWCHAR)(*Ptr);
	if (Source->Length)
		memmove (Destination->Buffer, Source->Buffer, Source->Length);
	Destination->Buffer[Destination->Length / sizeof(WCHAR)] = 0;
	*Ptr += Destination->MaximumLength;
}


NTSTATUS
STDCALL
RtlCreateProcessParameters (
	PRTL_USER_PROCESS_PARAMETERS	*Ppb,
	PUNICODE_STRING	CommandLine,
	PUNICODE_STRING	LibraryPath,
	PUNICODE_STRING	CurrentDirectory,
	PUNICODE_STRING	ImageName,
	PVOID		Environment,
	PUNICODE_STRING	Title,
	PUNICODE_STRING	Desktop,
	PUNICODE_STRING	Reserved,
	PUNICODE_STRING	Reserved2
	)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PRTL_USER_PROCESS_PARAMETERS Param = NULL;
	ULONG RegionSize = 0;
	ULONG DataSize = 0;
	PWCHAR Dest;
	UNICODE_STRING EmptyString;
	HANDLE CurrentDirectoryHandle;
	ULONG ConsoleFlags;

	DPRINT ("RtlCreateProcessParameters\n");

	RtlAcquirePebLock ();

	EmptyString.Length = 0;
	EmptyString.MaximumLength = sizeof(WCHAR);
	EmptyString.Buffer = L"";

	if (NtCurrentPeb()->ProcessParameters)
	{
		if (LibraryPath == NULL)
			LibraryPath = &NtCurrentPeb()->ProcessParameters->LibraryPath;
		if (Environment == NULL)
			Environment  = NtCurrentPeb()->ProcessParameters->Environment;
		if (CurrentDirectory == NULL)
			CurrentDirectory = &NtCurrentPeb()->ProcessParameters->CurrentDirectory.DosPath;
		CurrentDirectoryHandle = NtCurrentPeb()->ProcessParameters->CurrentDirectory.Handle;
		ConsoleFlags = NtCurrentPeb()->ProcessParameters->ConsoleFlags;
	}
	else
	{
		if (LibraryPath == NULL)
			LibraryPath = &EmptyString;
		if (CurrentDirectory == NULL)
			CurrentDirectory = &EmptyString;
		CurrentDirectoryHandle = NULL;
		ConsoleFlags = 0;
	}

	if (ImageName == NULL)
		ImageName = CommandLine;
	if (Title == NULL)
		Title = &EmptyString;
	if (Desktop == NULL)
		Desktop = &EmptyString;
	if (Reserved == NULL)
		Reserved = &EmptyString;
	if (Reserved2 == NULL)
		Reserved2 = &EmptyString;

	/* size of process parameter block */
	DataSize = sizeof (RTL_USER_PROCESS_PARAMETERS);

	/* size of current directory buffer */
	DataSize += (MAX_PATH * sizeof(WCHAR));

	/* add string lengths */
	DataSize += ALIGN(LibraryPath->MaximumLength, sizeof(ULONG));
	DataSize += ALIGN(CommandLine->Length, sizeof(ULONG));
	DataSize += ALIGN(ImageName->Length, sizeof(ULONG));
	DataSize += ALIGN(Title->MaximumLength, sizeof(ULONG));
	DataSize += ALIGN(Desktop->MaximumLength, sizeof(ULONG));
	DataSize += ALIGN(Reserved->MaximumLength, sizeof(ULONG));
	DataSize += ALIGN(Reserved2->MaximumLength, sizeof(ULONG));

	/* Calculate the required block size */
	RegionSize = ROUNDUP(DataSize, PAGESIZE);

	Status = NtAllocateVirtualMemory (
		NtCurrentProcess (),
		(PVOID*)&Param,
		0,
		&RegionSize,
		MEM_COMMIT,
		PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	{
		RtlReleasePebLock ();
		return Status;
	}

	DPRINT ("Ppb allocated\n");

	Param->TotalSize = RegionSize;
	Param->DataSize = DataSize;
	Param->Flags = PPF_NORMALIZED;
	Param->Environment = Environment;
	Param->CurrentDirectory.Handle = CurrentDirectoryHandle;
	Param->ConsoleFlags = ConsoleFlags;

	Dest = (PWCHAR)(((PBYTE)Param) + sizeof(RTL_USER_PROCESS_PARAMETERS));

	/* copy current directory */
	RtlpCopyParameterString (&Dest,
	                         &Param->CurrentDirectory.DosPath,
	                         CurrentDirectory,
	                         MAX_PATH * sizeof(WCHAR));

	/* make sure the current directory has a trailing backslash */
	if (Param->CurrentDirectory.DosPath.Length > 0)
	{
		ULONG Length;

		Length = Param->CurrentDirectory.DosPath.Length / sizeof(WCHAR);
		if (Param->CurrentDirectory.DosPath.Buffer[Length-1] != L'\\')
		{
			Param->CurrentDirectory.DosPath.Buffer[Length] = L'\\';
			Param->CurrentDirectory.DosPath.Buffer[Length + 1] = 0;
			Param->CurrentDirectory.DosPath.Length += sizeof(WCHAR);
		}
	}

	/* copy library path */
	RtlpCopyParameterString (&Dest,
	                         &Param->LibraryPath,
	                         LibraryPath,
	                         0);

	/* copy command line */
	RtlpCopyParameterString (&Dest,
	                         &Param->CommandLine,
	                         CommandLine,
	                         CommandLine->Length + sizeof(WCHAR));

	/* copy image name */
	RtlpCopyParameterString (&Dest,
	                         &Param->ImageName,
	                         ImageName,
	                         ImageName->Length + sizeof(WCHAR));

	/* copy title */
	RtlpCopyParameterString (&Dest,
	                         &Param->Title,
	                         Title,
	                         0);

	/* copy desktop */
	RtlpCopyParameterString (&Dest,
	                         &Param->Desktop,
	                         Desktop,
	                         0);

	RtlpCopyParameterString (&Dest,
	                         &Param->ShellInfo,
	                         Reserved,
	                         0);

	RtlpCopyParameterString (&Dest,
	                         &Param->RuntimeData,
	                         Reserved2,
	                         0);

	RtlDeNormalizeProcessParams (Param);
	*Ppb = Param;
	RtlReleasePebLock ();

	return STATUS_SUCCESS;
}

VOID
STDCALL
RtlDestroyProcessParameters (
	PRTL_USER_PROCESS_PARAMETERS	Ppb
	)
{
	ULONG RegionSize = 0;

	NtFreeVirtualMemory (NtCurrentProcess (),
	                     (PVOID)Ppb,
	                     &RegionSize,
	                     MEM_RELEASE);
}

/*
 * denormalize process parameters (Pointer-->Offset)
 */
PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlDeNormalizeProcessParams (
	PRTL_USER_PROCESS_PARAMETERS	Params
	)
{
	if (Params && (Params->Flags & PPF_NORMALIZED))
	{
		DENORMALIZE (Params->CurrentDirectory.DosPath.Buffer, Params);
		DENORMALIZE (Params->LibraryPath.Buffer, Params);
		DENORMALIZE (Params->CommandLine.Buffer, Params);
		DENORMALIZE (Params->ImageName.Buffer, Params);
		DENORMALIZE (Params->Title.Buffer, Params);
		DENORMALIZE (Params->Desktop.Buffer, Params);
		DENORMALIZE (Params->ShellInfo.Buffer, Params);
		DENORMALIZE (Params->RuntimeData.Buffer, Params);

		Params->Flags &= ~PPF_NORMALIZED;
	}

	return Params;
}

/*
 * normalize process parameters (Offset-->Pointer)
 */
PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlNormalizeProcessParams (
	PRTL_USER_PROCESS_PARAMETERS Params)
{
	if (Params && !(Params->Flags & PPF_NORMALIZED))
	{
		NORMALIZE (Params->CurrentDirectory.DosPath.Buffer, Params);
		NORMALIZE (Params->LibraryPath.Buffer, Params);
		NORMALIZE (Params->CommandLine.Buffer, Params);
		NORMALIZE (Params->ImageName.Buffer, Params);
		NORMALIZE (Params->Title.Buffer, Params);
		NORMALIZE (Params->Desktop.Buffer, Params);
		NORMALIZE (Params->ShellInfo.Buffer, Params);
		NORMALIZE (Params->RuntimeData.Buffer, Params);

		Params->Flags |= PPF_NORMALIZED;
	}

	return Params;
}

/* EOF */
