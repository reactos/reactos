/* $Id: env.c,v 1.9 2000/02/27 15:45:57 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/env.c
 * PURPOSE:         Environment functions
 * PROGRAMMER:      Eric Kohl
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <internal/teb.h>
#include <string.h>

#define NDEBUG
#include <ntdll/ntdll.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
RtlCreateEnvironment (
	BOOLEAN	Initialize,
	PVOID	*Environment
	)
{
	MEMORY_BASIC_INFORMATION MemInfo;
	PVOID EnvPtr = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG RegionSize = PAGESIZE;

	if (Initialize == FALSE)
	{
		RtlAcquirePebLock ();
#if 0
		if (NtCurrentPeb()->ProcessParameters->Environment != NULL)
		{
			Status = NtQueryVirtualMemory (NtCurrentProcess (),
			                               NtCurrentPeb ()->ProcessParameters->Environment,
			                               MemoryBasicInformation,
			                               &MemInfo,
			                               sizeof(MEMORY_BASIC_INFORMATION),
			                               NULL);
			if (!NT_SUCCESS(Status))
			{
				RtlReleasePebLock ();
				*Environment = NULL;
				return Status;
			}

			RegionSize = MemInfo.RegionSize;
			Status = NtAllocateVirtualMemory (NtCurrentProcess (),
			                                  &EnvPtr,
			                                  0,
			                                  &RegionSize,
			                                  MEM_COMMIT,
			                                  PAGE_READWRITE);
			if (!NT_SUCCESS(Status))
			{
				RtlReleasePebLock ();
				*Environment = NULL;
				return Status;
			}

			memmove (EnvPtr,
			         NtCurrentPeb ()->ProcessParameters->Environment,
			         MemInfo.RegionSize);

			*Environment = EnvPtr;
		}
#endif
		RtlReleasePebLock ();
	}
	else
	{
		Status = NtAllocateVirtualMemory (NtCurrentProcess (),
		                                  &EnvPtr,
		                                  0,
		                                  &RegionSize,
		                                  MEM_COMMIT,
		                                  PAGE_READWRITE);
		if (NT_SUCCESS(Status))
		{
			memset (EnvPtr, 0, RegionSize);
			*Environment = EnvPtr;
		}
	}

	return Status;
}


VOID
STDCALL
RtlDestroyEnvironment (
	PVOID	Environment
	)
{
	ULONG Size = 0;

	NtFreeVirtualMemory (NtCurrentProcess (),
	                     &Environment,
	                     &Size,
	                     MEM_RELEASE);
}



NTSTATUS
STDCALL
RtlExpandEnvironmentStrings_U (
	PVOID		Environment,
	PUNICODE_STRING	Source,
	PUNICODE_STRING	Destination,
	PULONG		Length
	)
{
	UNICODE_STRING var;
	UNICODE_STRING val;
	NTSTATUS Status = STATUS_SUCCESS;
	BOOLEAN flag = FALSE;
	PWSTR s;
	PWSTR d;
	PWSTR w;
	int src_len;
	int dst_max;
	int tail;

	DPRINT ("RtlExpandEnvironmentStrings_U %p %wZ %p %p\n",
	        Environment, Source, Destination, Length);

	src_len = Source->Length / sizeof(WCHAR);
	s = Source->Buffer;
	dst_max = Destination->MaximumLength / sizeof(WCHAR);
	d = Destination->Buffer;

	while (src_len)
	{
		if (*s == L'%')
		{
			if (flag)
			{
				flag = FALSE;
				goto copy;
			}
			w = s + 1;
			tail = src_len - 1;
			while (*w != L'%' && tail)
			{
				w++;
				tail--;
			}
			if (!tail)
				goto copy;

			var.Length = (w - ( s + 1)) * sizeof(WCHAR);
			var.MaximumLength = var.Length;
			var.Buffer = s + 1;

			val.Length = 0;
			val.MaximumLength = dst_max * sizeof(WCHAR);
			val.Buffer = d;
			Status = RtlQueryEnvironmentVariable_U (Environment, &var, &val);
			if (NT_SUCCESS(Status))
			{
				d += val.Length / sizeof(WCHAR);
				dst_max -= val.Length / sizeof(WCHAR);
				s = w + 1;
				src_len = tail - 1;
				continue;
			}
			/* variable not found or buffer too small, just copy %var% */
			flag = TRUE;
		}
copy:
		if (!dst_max)
		{
			Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		*d++ = *s++;
		dst_max--;
		src_len--;
	}
	Destination->Length = (d - Destination->Buffer) * sizeof(WCHAR);
	if (Length != NULL)
		*Length = Destination->Length;
	if (dst_max)
		Destination->Buffer[Destination->Length / sizeof(WCHAR)] = 0;

	DPRINT ("Destination %wZ\n", Destination);
	return Status;
}


VOID
STDCALL
RtlSetCurrentEnvironment (
	PVOID	NewEnvironment,
	PVOID	*OldEnvironment
	)
{
	PVOID EnvPtr;

	DPRINT ("NewEnvironment %x OldEnvironment %x\n",
	        NewEnvironment, OldEnvironment);

	RtlAcquirePebLock ();

	EnvPtr = NtCurrentPeb()->ProcessParameters->Environment;
	NtCurrentPeb()->ProcessParameters->Environment = NewEnvironment;

	if (OldEnvironment != NULL)
		*OldEnvironment = EnvPtr;

	RtlReleasePebLock ();
}


NTSTATUS
STDCALL
RtlSetEnvironmentVariable (
	PVOID		*Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value)
{
	MEMORY_BASIC_INFORMATION mbi;
	UNICODE_STRING var;
	int     hole_len, new_len, env_len = 0;
	WCHAR   *new_env = 0, *env_end = 0, *wcs, *env, *val = 0, *tail = 0, *hole = 0;
	PWSTR head = NULL;
	ULONG size = 0, new_size;
	LONG f = 1;
	NTSTATUS Status = STATUS_SUCCESS;

	DPRINT ("RtlSetEnvironmentVariable Environment %p Name %wZ Value %wZ\n",
	        Environment, Name, Value);

	if (Environment)
	{
		env = *Environment;
	}
	else
	{
		RtlAcquirePebLock();
		env = NtCurrentPeb()->ProcessParameters->Environment;
	}

	if (env)
	{
		/* get environment length */
		wcs = env_end = env;
		while (*env_end)
			while (*env_end++)
				;
		env_end++;
		env_len = env_end - env;
		DPRINT ("environment length %ld characters\n", env_len);

		/* find where to insert */
		while (*wcs)
		{
			for (var.Buffer = wcs++; *wcs && *wcs != L'='; wcs++);
			if (*wcs)
			{
				var.Length        = (wcs - var.Buffer) * sizeof(WCHAR);
				var.MaximumLength = var.Length;
				for ( val = ++wcs; *wcs; wcs++);
				f = RtlCompareUnicodeString (&var, Name, TRUE);
				if (f >= 0)
				{
					if (f) /* Insert before found */
					{
						hole = tail = var.Buffer;
					}
					else /* Exact match */
					{
						head = var.Buffer;
						tail = ++wcs;
						hole = val;
					}
					goto found;
				}
			}
			wcs++;
		}
		hole = tail = wcs; /* Append to environment */
	}

found:
	if (Value->Length > 0)
	{
		hole_len = tail - hole;
		/* calculate new environment size */
		new_size = Value->Length + sizeof(WCHAR);
		/* adding new variable */
		if (f)
			new_size += Name->Length + sizeof(WCHAR);
		new_len = new_size / sizeof(WCHAR);
		if (hole_len < new_len)
		{
			/* we must enlarge environment size */
			/* let's check the size of available memory */
			new_size += (env_len - hole_len) * sizeof(WCHAR);
			new_size = ROUNDUP(new_size, PAGESIZE);
			mbi.RegionSize = 0;
			DPRINT("new_size %lu\n", new_size);

			if (env)
			{
				Status = NtQueryVirtualMemory (NtCurrentProcess (),
				                               env,
				                               0,
				                               &mbi,
				                               sizeof(mbi),
				                               NULL);
				if (!NT_SUCCESS(Status))
				{
					RtlReleasePebLock ();
					return Status;
				}
			}

			if (new_size > mbi.RegionSize)
			{
				/* reallocate memory area */
				Status = NtAllocateVirtualMemory (NtCurrentProcess (),
				                                  (VOID**)&new_env,
				                                  0,
				                                  &new_size,
				                                  MEM_COMMIT,
				                                  PAGE_READWRITE);
				if (!NT_SUCCESS(Status))
				{
					RtlReleasePebLock ();
					return Status;
				}

				if (env)
				{
					memmove (new_env,
					         env,
					         (hole - env) * sizeof(WCHAR));
					hole = new_env + (hole - env);
				}
				else
				{
					/* absolutely new environment */
					tail = hole = new_env;
					*hole = 0;
					env_end = hole + 1;
				}
			}
		}

		/* move tail */
		memmove (hole + new_len, tail, (env_end - tail) * sizeof(WCHAR));

		if (new_env)
		{
			/* we reallocated environment, let's free the old one */
			if (Environment)
				*Environment = new_env;
			else
				NtCurrentPeb()->ProcessParameters->Environment = new_env;

			if (env)
			{
				size = 0;
				NtFreeVirtualMemory (NtCurrentProcess (),
				                     (VOID**)&env,
				                     &size,
				                     MEM_RELEASE);
			}
		}

		/* and now copy given stuff */
		if (f)
		{
			/* copy variable name and '=' character */
			memmove (hole, Name->Buffer, Name->Length);
			hole += Name->Length / sizeof(WCHAR);
			*hole++ = L'=';
		}

		/* copy value */
		memmove (hole, Value->Buffer, Value->Length);
		hole += Value->Length / sizeof(WCHAR);
		*hole = 0;
	}
	else
	{
		/* remove the environment variable */
		if (f == 0)
			memmove (head,
			         tail,
			         (env_end - tail) * sizeof(WCHAR));
		else
			Status = STATUS_VARIABLE_NOT_FOUND;
	}

	RtlReleasePebLock ();
	return Status;
}


NTSTATUS
STDCALL
RtlQueryEnvironmentVariable_U (
	PVOID		Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	)
{
	NTSTATUS Status;
	PWSTR wcs;
	PWSTR var;
	PWSTR val;
	int varlen;
	int len;
	BOOLEAN SysEnvUsed = FALSE;

	DPRINT("RtlQueryEnvironmentVariable_U Environment %p Variable %wZ Value %p\n",
	       Environment, Name, Value);

	if (Environment == NULL)
	{
		Environment = NtCurrentPeb()->ProcessParameters->Environment;
		SysEnvUsed = TRUE;
	}

	if (Environment == NULL)
		return STATUS_VARIABLE_NOT_FOUND;

	Value->Length = 0;
	if (SysEnvUsed == TRUE)
		RtlAcquirePebLock();

	wcs = Environment;
	len = Name->Length / sizeof(WCHAR);
	while (*wcs)
	{
		for (var = wcs++; *wcs && *wcs != L'='; wcs++)
			;

		if (*wcs)
		{
			varlen = wcs - var;
			for (val = ++wcs; *wcs; wcs++)
				;

			if (varlen == len &&
			    !_wcsnicmp (var, Name->Buffer, len))
			{
				Value->Length = (wcs - val) * sizeof(WCHAR);
				if (Value->Length < Value->MaximumLength)
				{
					wcscpy (Value->Buffer, val);
					DPRINT("Value %S\n", val);
					DPRINT("Return STATUS_SUCCESS\n");
					Status = STATUS_SUCCESS;
				}
				else
				{
					DPRINT("Return STATUS_BUFFER_TOO_SMALL\n");
					Status = STATUS_BUFFER_TOO_SMALL;
				}

				if (SysEnvUsed == TRUE)
					RtlReleasePebLock ();

				return Status;
			}
		}
		wcs++;
	}

	if (SysEnvUsed == TRUE)
		RtlReleasePebLock ();

	DPRINT("Return STATUS_VARIABLE_NOT_FOUND\n");
	return STATUS_VARIABLE_NOT_FOUND;
}

/* EOF */
