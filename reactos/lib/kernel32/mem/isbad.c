/* $Id: isbad.c,v 1.6 2003/04/29 22:39:57 gvg Exp $
 *
 * lib/kernel32/mem/isbad.c
 *
 * ReactOS Operating System
 *
 */
#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* FIXME: Stubs. What is it for? */
UINT
wcsnlen (
	LPCWSTR	lpsz,
	UINT	ucchMax
	)
{
	DPRINT1("wcsnlen stub called\n");

	return 0;
}


/* FIXME: Stubs. What is it for? */
UINT
strnlen (
	LPCSTR	lpsz,
	UINT	uiMax
	)
{
	DPRINT1("strnlen stub called\n");

	return 0;
}

/* --- --- --- */

WINBOOL 
STDCALL
IsBadReadPtr (
	CONST VOID	* lp,
	UINT		ucb
	)
{	
	MEMORY_BASIC_INFORMATION MemoryInformation;

	if ( ucb == 0 )
	{
		return TRUE;
	}

	VirtualQuery (
		lp,
		& MemoryInformation,
		sizeof (MEMORY_BASIC_INFORMATION)
		);
	
	if ( MemoryInformation.State != MEM_COMMIT )
	{
		return TRUE;
	}
		
	if ( MemoryInformation.RegionSize < ucb )
	{
		return TRUE;
	}
		
	if ( MemoryInformation.Protect == PAGE_EXECUTE )
	{
		return TRUE;
	}
		
	if ( MemoryInformation.Protect == PAGE_NOACCESS )
	{
		return TRUE;
	}
		
	return FALSE;
			
}


WINBOOL 
STDCALL
IsBadHugeReadPtr (
	CONST VOID	* lp,
	UINT		ucb
	)
{
	return IsBadReadPtr (lp, ucb);
}


WINBOOL 
STDCALL
IsBadCodePtr (
	FARPROC	lpfn
	)
{
	MEMORY_BASIC_INFORMATION MemoryInformation;


	VirtualQuery (
		lpfn,
		& MemoryInformation,
		sizeof (MEMORY_BASIC_INFORMATION)
		);
	
	if ( MemoryInformation.State != MEM_COMMIT )
	{
		return TRUE;
	}	
			
	if (	(MemoryInformation.Protect == PAGE_EXECUTE)
		|| (MemoryInformation.Protect == PAGE_EXECUTE_READ)
		)
	{
		return FALSE;
	}
		
	return TRUE;
}


WINBOOL
STDCALL
IsBadWritePtr (
	LPVOID	lp,
	UINT	ucb
	)
{
	MEMORY_BASIC_INFORMATION MemoryInformation;

	if ( ucb == 0 )
	{
		return TRUE;
	}

	VirtualQuery (
		lp,
		& MemoryInformation,
		sizeof (MEMORY_BASIC_INFORMATION)
		);
	
	if ( MemoryInformation.State != MEM_COMMIT )
	{
		return TRUE;
	}
		
	if ( MemoryInformation.RegionSize < ucb )
	{
		return TRUE;
	}
		
		
	if ( MemoryInformation.Protect == PAGE_READONLY)
	{
		return TRUE;
	}
		
	if (	(MemoryInformation.Protect == PAGE_EXECUTE)
		|| (MemoryInformation.Protect == PAGE_EXECUTE_READ)
		)
	{
		return TRUE;
	}
		
	if ( MemoryInformation.Protect == PAGE_NOACCESS )
	{
		return TRUE;	
	}
		
	return FALSE;
}


WINBOOL
STDCALL
IsBadHugeWritePtr (
	LPVOID	lp,
	UINT	ucb
	)
{
	return IsBadWritePtr (lp, ucb);
}


WINBOOL
STDCALL
IsBadStringPtrW (
	LPCWSTR	lpsz,
	UINT	ucchMax
	)
{
	UINT Len = wcsnlen (
			lpsz + 1,
			ucchMax >> 1
			);
	return IsBadReadPtr (
			lpsz,
			Len << 1
			);
}


WINBOOL 
STDCALL
IsBadStringPtrA (
	LPCSTR	lpsz,
	UINT	ucchMax
	)
{
	UINT Len = strnlen (
			lpsz + 1,
			ucchMax
			);
	return IsBadReadPtr (
			lpsz,
			Len
			);
}


/* EOF */
