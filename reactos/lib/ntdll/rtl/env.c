/* $Id: env.c,v 1.6 2000/02/13 16:05:16 dwelch Exp $
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

//#define NDEBUG
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
	ULONG RegionSize = 1;

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
		RegionSize = 1;
		Status = NtAllocateVirtualMemory (NtCurrentProcess (),
		                                  &EnvPtr,
		                                  0,
		                                  &RegionSize,
		                                  MEM_COMMIT,
		                                  PAGE_READWRITE);
		if (NT_SUCCESS(Status))
			*Environment = EnvPtr;
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
	PUNICODE_STRING	src,
	PUNICODE_STRING	dst,
	PULONG		Length
	)
{
	UNICODE_STRING var,val;
	int            src_len, dst_max, tail;
	WCHAR          *s,*d,*w;
	BOOLEAN        flag = FALSE;
	NTSTATUS Status = STATUS_SUCCESS;

	src_len = src->Length / 2;
	s       = src->Buffer;
	dst_max = dst->MaximumLength / 2;
	d       = dst->Buffer;

	while( src_len )
	{
		if( *s == L'%' )
		{
			if( flag )
			{
				flag = FALSE;
				goto copy;
			}
			w = s + 1; tail = src_len - 1;
			while( *w != L'%' && tail )
			{
				w++;
				tail--;
			}
			if( !tail )
				goto copy;

			var.Length        = ( w - ( s + 1 ) ) * 2;
			var.MaximumLength = var.Length;
			var.Buffer        = s + 1;

			val.Length        = 0;
			val.MaximumLength = dst_max * 2;
			val.Buffer        = d;
			Status = RtlQueryEnvironmentVariable_U (Environment, &var, &val );
			if( Status >= 0 )
			{
				d       += val.Length / 2;
				dst_max -= val.Length / 2;
				s       = w + 1;
				src_len = tail - 1;
				continue;
			}
			/* variable not found or buffer too small, just copy %var% */
			flag = TRUE;
		}
copy:;
		if( !dst_max )
		{
			Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		*d++ = *s++;
		dst_max--;
		src_len--;
	}
	dst->Length = ( d - dst->Buffer ) * 2;
	if (Length)
		*Length = dst->Length;
	if( dst_max )
		dst->Buffer[ dst->Length / 2 ] = 0;

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

#if 0
NTSTATUS
STDCALL
RtlSetEnvironmentVariable (
	PVOID		*Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	)
{
	NTSTATUS Status;
	PWSTR EnvPtr;
	PWSTR EndPtr;
	ULONG EnvLength;

	Status = STATUS_VARIABLE_NOT_FOUND;

	if (Environment != NULL)
	{
		EnvPtr = *Environment
	}
	else
	{
		RtlAcquirePebLock ();
		EnvPtr = NtCurrentPeb()->ProcessParameters->Environment;
	}

	if (EnvPtr != NULL)
	{
		/* get environment length */
		EndPtr = EnvPtr;
		while (*EndPtr)
		{
			while (*EndPtr++)
				;
			EndPtr++;
		}
		EnvLen = EndPtr - EnvPtr;

		/* FIXME: add missing stuff */

	}

	if (EnvPtr != *Environment)
		RtlReleasePebLock ();

	return Status;
}
#endif


NTSTATUS
WINAPI
RtlSetEnvironmentVariable (
	PVOID		*Environment,
	UNICODE_STRING	*varname,
	UNICODE_STRING	*value)
{
	UNICODE_STRING           var;
	MEMORY_BASIC_INFORMATION mbi;
	int     hole_len, new_len, env_len = 0;
	WCHAR   *new_env = 0, *env_end = 0, *wcs, *env, *val = 0, *tail = 0, *hole = 0;
	ULONG   size = 0, new_size;
	LONG    f = 1;
	NTSTATUS Status = STATUS_SUCCESS;

	if (Environment)
	{
		env = *Environment;
	}
	else
	{
		RtlAcquirePebLock();
		env = NtCurrentPeb()->ProcessParameters->Environment;
	}

	if( env )
	{
		/* get environment length */
		wcs = env_end = env;
		while( *env_end ) while( *env_end++ ); env_end++;
		env_len = env_end - env;

		/* find where to insert */
		while( *wcs )
		{
			for (var.Buffer = wcs++; *wcs && *wcs != L'='; wcs++);
			if( *wcs )
			{
				var.Length        = ( wcs - var.Buffer ) * 2;
				var.MaximumLength = var.Length;
				for ( val = ++wcs; *wcs; wcs++);
				f = RtlCompareUnicodeString( &var, varname, TRUE );
				if( f >= 0 )
				{
					if( f ) /* Insert before found */
					{
						hole = tail = var.Buffer;
					}
					else /* Exact match */
					{
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

found:;
	if( value )
	{
		hole_len = tail - hole;
		/* calculate new environment size */
		new_size = value->Length + 2;
		if( f )
			new_size += varname->Length + 2; /* adding new variable */
		new_len = new_size / 2;
		if (hole_len < new_len)
		{
			/* we must enlarge environment size, let's check the size of available
			 * memory */
			new_size += ( env_len - hole_len ) * 2;
			mbi.RegionSize = 0;

			if (env)
			{
				Status = NtQueryVirtualMemory( (HANDLE)-1, env, 0, &mbi, sizeof(mbi), NULL );
				if (!NT_SUCCESS(Status))
				{
					RtlReleasePebLock ();
					return Status;
				}
			}

			if (new_size > mbi.RegionSize)
			{
				/* reallocate memory area */
				Status = NtAllocateVirtualMemory( (HANDLE)-1, (VOID**)&new_env, 0, &new_size, MEM_COMMIT, PAGE_READWRITE );
				if (!NT_SUCCESS(Status))
				{
					RtlReleasePebLock ();
					return Status;
				}

				if (env)
				{
					memmove( new_env, env, ( hole - env ) * 2 );
					hole = new_env + ( hole - env );
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
		memmove ( hole + new_len, tail, ( env_end - tail ) * 2 );
		if( new_env )
		{
			/* we reallocated environment, let's free the old one */
			if (Environment)
				*Environment = new_env;
			else
				NtCurrentPeb()->ProcessParameters->Environment = new_env;

			if (env)
			{
				NtFreeVirtualMemory (NtCurrentProcess(),
				                     (VOID**)&env,
				                     &size,
				                     MEM_RELEASE);
			}
		}

		/* and now copy given stuff */
		if( f )
		{
			/* copy variable name and '=' sign */
			memmove (hole, varname->Buffer, varname->Length);
			hole += varname->Length / 2; *hole++ = L'=';
		}

		/* copy value */
		memmove( hole, value->Buffer, value->Length );
		hole += value->Length / 2; *hole = 0;
	}
	else
	{
		if (!f)
			memmove (hole, tail, ( env_end - tail ) * 2 ); /* remove it */
		else
			Status = STATUS_VARIABLE_NOT_FOUND; /* notingh to remove*/
	}

	RtlReleasePebLock ();
	return Status;
}
//#endif


NTSTATUS
STDCALL
RtlQueryEnvironmentVariable_U (
	PVOID		env,
	UNICODE_STRING	*varname,
	UNICODE_STRING	*value
	)
{
	NTSTATUS Status = STATUS_VARIABLE_NOT_FOUND;
	WCHAR    *wcs,*var,*val;
	int      varlen, len;

	if (!env)
		env = NtCurrentPeb()->ProcessParameters->Environment;

	if (!env)
		return Status;

	value->Length = 0;
	if (env == NtCurrentPeb()->ProcessParameters->Environment)
		RtlAcquirePebLock();

	wcs = env;
	len = varname->Length / 2;
	while( *wcs )
	{
		for (var = wcs++; *wcs && *wcs != L'='; wcs++)
			;

		if (*wcs)
		{
			varlen = wcs - var;
			for (val = ++wcs; *wcs; wcs++)
				;
			if (varlen == len &&
			    !_wcsnicmp (var, varname->Buffer, len))
			{
				value->Length = (wcs - val) * sizeof(WCHAR);
				if (value->Length < value->MaximumLength)
				{
					wcscpy (value->Buffer, val);
					Status = STATUS_SUCCESS;
				}
				else
				{
					Status = STATUS_BUFFER_TOO_SMALL;
				}

				if (env == NtCurrentPeb()->ProcessParameters->Environment)
					RtlReleasePebLock ();

				return Status;
			}
		}
		wcs++;
	}

	if (env == NtCurrentPeb()->ProcessParameters->Environment)
		RtlReleasePebLock ();

	return Status;
}

/* EOF */
