/* $Id: ppb.c,v 1.1 2000/02/13 16:05:16 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/ppb.c
 * PURPOSE:         Process functions
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

/* FUNCTIONS ****************************************************************/

VOID STDCALL RtlAcquirePebLock(VOID)
{

}


VOID STDCALL RtlReleasePebLock(VOID)
{

}

NTSTATUS
STDCALL
RtlCreateProcessParameters (
	PRTL_USER_PROCESS_PARAMETERS		*Ppb,
	PUNICODE_STRING	CommandLine,
	PUNICODE_STRING	LibraryPath,
	PUNICODE_STRING	CurrentDirectory,
	PUNICODE_STRING	ImageName,
	PVOID		Environment,
	PUNICODE_STRING	Title,
	PUNICODE_STRING	Desktop,
	PUNICODE_STRING	Reserved,
	PVOID		Reserved2
	)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PRTL_USER_PROCESS_PARAMETERS Param = NULL;
	ULONG RegionSize = 0;
	ULONG DataSize = 0;
	PWCHAR Dest;

	DPRINT ("RtlCreateProcessParameters\n");

	RtlAcquirePebLock ();

	/* size of process parameter block */
	DataSize = sizeof (RTL_USER_PROCESS_PARAMETERS);

	/* size of (reserved) buffer */
	DataSize += (256 * sizeof(WCHAR));

	/* size of current directory buffer */
	DataSize += (MAX_PATH * sizeof(WCHAR));

	/* add string lengths */
	if (LibraryPath != NULL)
		DataSize += (LibraryPath->Length + sizeof(WCHAR));

	if (CommandLine != NULL)
		DataSize += (CommandLine->Length + sizeof(WCHAR));

	if (ImageName != NULL)
		DataSize += (ImageName->Length + sizeof(WCHAR));

	if (Title != NULL)
		DataSize += (Title->Length + sizeof(WCHAR));

	if (Desktop != NULL)
		DataSize += (Desktop->Length + sizeof(WCHAR));

	if (Reserved != NULL)
		DataSize += (Reserved->Length + sizeof(WCHAR));

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
	Param->Flags = TRUE;
	Param->Environment = Environment;
//	Param->Unknown1 =
//	Param->Unknown2 =
//	Param->Unknown3 =
//	Param->Unknown4 =

	/* copy current directory */
   Dest = (PWCHAR)(((PBYTE)Param) + 
		   sizeof(RTL_USER_PROCESS_PARAMETERS) + 
		   (256 * sizeof(WCHAR)));

   Param->CurrentDirectory.DosPath.Buffer = Dest;
   if (CurrentDirectory != NULL)
     {
	Param->CurrentDirectory.DosPath.Length = CurrentDirectory->Length;
	Param->CurrentDirectory.DosPath.MaximumLength = 
	  CurrentDirectory->Length + sizeof(WCHAR);
	memcpy(Dest,
	       CurrentDirectory->Buffer,
	       CurrentDirectory->Length);
	Dest = (PWCHAR)(((PBYTE)Dest) + CurrentDirectory->Length);
     }
   *Dest = 0;

   Dest = (PWCHAR)(((PBYTE)Param) + sizeof(RTL_USER_PROCESS_PARAMETERS) +
		   (256 * sizeof(WCHAR)) + (MAX_PATH * sizeof(WCHAR)));
   
   /* copy library path */
   Param->LibraryPath.Buffer = Dest;
   if (LibraryPath != NULL)
     {
	Param->LibraryPath.Length = LibraryPath->Length;
	memcpy (Dest,
		LibraryPath->Buffer,
		LibraryPath->Length);
	Dest = (PWCHAR)(((PBYTE)Dest) + LibraryPath->Length);
     }
   Param->LibraryPath.MaximumLength = Param->LibraryPath.Length + 
     sizeof(WCHAR);
   *Dest = 0;
   Dest++;
   
   /* copy command line */
   Param->CommandLine.Buffer = Dest;
   if (CommandLine != NULL)
     {
	Param->CommandLine.Length = CommandLine->Length;
	memcpy (Dest,
		CommandLine->Buffer,
		CommandLine->Length);
	Dest = (PWCHAR)(((PBYTE)Dest) + CommandLine->Length);
     }
   Param->CommandLine.MaximumLength = Param->CommandLine.Length + sizeof(WCHAR);
   *Dest = 0;
   Dest++;
   
   /* copy image name */
   Param->ImageName.Buffer = Dest;
   if (ImageName != NULL)
     {
	Param->ImageName.Length = ImageName->Length;
		memcpy (Dest,
		        ImageName->Buffer,
		        ImageName->Length);
	Dest = (PWCHAR)(((PBYTE)Dest) + ImageName->Length);
     }
   Param->ImageName.MaximumLength = Param->ImageName.Length + sizeof(WCHAR);
   *Dest = 0;
   Dest++;

	/* copy title */
	Param->Title.Buffer = Dest;
	if (Title != NULL)
	{
		Param->Title.Length = Title->Length;
		memcpy (Dest,
		        Title->Buffer,
		        Title->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + Title->Length);
	}
	Param->Title.MaximumLength = Param->Title.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* copy desktop */
   Param->Desktop.Buffer = Dest;
   if (Desktop != NULL)
     {
	Param->Desktop.Length = Desktop->Length;
	memcpy (Dest,
		Desktop->Buffer,
		Desktop->Length);
	Dest = (PWCHAR)(((PBYTE)Dest) + Desktop->Length);
     }
   Param->Desktop.MaximumLength = Param->Desktop.Length + sizeof(WCHAR);
   *Dest = 0;
   Dest++;
   
   RtlDeNormalizeProcessParams (Param);
   *Ppb = Param;
   RtlReleasePebLock ();
   
   return(Status);
}

VOID STDCALL RtlDestroyProcessParameters (PRTL_USER_PROCESS_PARAMETERS	Ppb)
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
VOID
STDCALL
RtlDeNormalizeProcessParams (
	PRTL_USER_PROCESS_PARAMETERS	Ppb
	)
{
	if (Ppb == NULL)
		return;

	if (Ppb->Flags == FALSE)
		return;

	if (Ppb->CurrentDirectory.DosPath.Buffer != NULL)
	{
		Ppb->CurrentDirectory.DosPath.Buffer =
			(PWSTR)((ULONG)Ppb->CurrentDirectory.DosPath.Buffer -
				(ULONG)Ppb);
	}

	if (Ppb->LibraryPath.Buffer != NULL)
	{
		Ppb->LibraryPath.Buffer =
			(PWSTR)((ULONG)Ppb->LibraryPath.Buffer -
				(ULONG)Ppb);
	}

	if (Ppb->CommandLine.Buffer != NULL)
	{
		Ppb->CommandLine.Buffer =
			(PWSTR)((ULONG)Ppb->CommandLine.Buffer -
				(ULONG)Ppb);
	}

	if (Ppb->ImageName.Buffer != NULL)
	{
		Ppb->ImageName.Buffer =
			(PWSTR)((ULONG)Ppb->ImageName.Buffer -
				(ULONG)Ppb);
	}

	if (Ppb->Title.Buffer != NULL)
	{
		Ppb->Title.Buffer =
			(PWSTR)((ULONG)Ppb->Title.Buffer -
				(ULONG)Ppb);
	}

	if (Ppb->Desktop.Buffer != NULL)
	{
		Ppb->Desktop.Buffer =
			(PWSTR)((ULONG)Ppb->Desktop.Buffer -
				(ULONG)Ppb);
	}

   Ppb->Flags = FALSE;
}

/*
 * normalize process parameters (Offset-->Pointer)
 */
VOID STDCALL RtlNormalizeProcessParams (PRTL_USER_PROCESS_PARAMETERS Ppb)
{
   if (Ppb == NULL)
     return;
   
   if (Ppb->Flags == TRUE)
     return;
   
   if (Ppb->CurrentDirectory.DosPath.Buffer != NULL)
     {
	Ppb->CurrentDirectory.DosPath.Buffer =
	  (PWSTR)((ULONG)Ppb->CurrentDirectory.DosPath.Buffer +
		  (ULONG)Ppb);
     }
   
   if (Ppb->LibraryPath.Buffer != NULL)
     {
	Ppb->LibraryPath.Buffer =
	  (PWSTR)((ULONG)Ppb->LibraryPath.Buffer +
		  (ULONG)Ppb);
     }
   
   if (Ppb->CommandLine.Buffer != NULL)
     {
	Ppb->CommandLine.Buffer =
	  (PWSTR)((ULONG)Ppb->CommandLine.Buffer +
		  (ULONG)Ppb);
     }
   
   if (Ppb->ImageName.Buffer != NULL)
     {
	Ppb->ImageName.Buffer =
			(PWSTR)((ULONG)Ppb->ImageName.Buffer +
				(ULONG)Ppb);
     }
   
   if (Ppb->Title.Buffer != NULL)
     {
	Ppb->Title.Buffer =
	  (PWSTR)((ULONG)Ppb->Title.Buffer +
		  (ULONG)Ppb);
     }
   
   if (Ppb->Desktop.Buffer != NULL)
     {
	Ppb->Desktop.Buffer =
	  (PWSTR)((ULONG)Ppb->Desktop.Buffer +
		  (ULONG)Ppb);
     }

   Ppb->Flags = TRUE;
}
