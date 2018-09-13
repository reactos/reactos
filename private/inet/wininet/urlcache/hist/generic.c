#include <windows.h>

#define ASSERT(x) if (!(x)) DebugBreak();


LPBYTE
MemFind (LPBYTE lpB, DWORD cbB, LPBYTE lpP, DWORD cbP)

{
	DWORD i, j;
	LPBYTE lpF = NULL;
//	LPBYTE lpB = (LPBYTE) lp1, lpP = (LPBYTE) lp2;
	
	if ( (!lpB) || (!cbB) || (!lpP) || (!cbP) )
		return NULL;
	
	for (i = 0; i < cbB ; i++)
	{
		for (j = 0; i < cbB, j < cbP ; i++, j++)
		{
			if (lpB[i] != lpP[j])
			{	
				lpF = NULL;
				break;
			}
			if (!j)  //the first letter
				lpF = &(lpB[i]);
		}
		if (lpF)
			return lpF;
	}
	return NULL;
}


BOOL
ParseArgsDyn(
    LPTSTR InBuffer,
    LPTSTR **pArgv,
    LPDWORD pArgc
    )

#define DEFAULT_ARGV_SIZE 16

{
    LPTSTR CurrentPtr = InBuffer;
    DWORD cArgv = DEFAULT_ARGV_SIZE;
	LPTSTR *temp = NULL;

	*pArgv = (LPTSTR *) LocalAlloc (LPTR, cArgv * sizeof(LPTSTR));
	if (!*pArgv)
		return FALSE;

    for ( ;; ) {

        //
        // skip blanks.
        //

        while( *CurrentPtr == ' ' ) {
            CurrentPtr++;
        }

        if( *CurrentPtr == '\0' ) {
            break;
        }

        (*pArgv)[*pArgc] = CurrentPtr;
		(*pArgc)++;

        //
        // go to next space.
        //

        while( (*CurrentPtr != ' ') &&
                (*CurrentPtr != '\0') &&
                (*CurrentPtr != '\n') ) {
            CurrentPtr++;
        }

        if( *CurrentPtr == '\0' ) {
            break;
        }

        *CurrentPtr++ = '\0';
		
		ASSERT(*pArgc <= cArgv);

		if (*pArgc == cArgv)  //grow the array
		{
			temp = *pArgv;

			*pArgv = (LPTSTR *) LocalAlloc (LPTR, (cArgv + DEFAULT_ARGV_SIZE) * sizeof (LPTSTR));
			if (!*pArgv)
			{
				LocalFree (temp);
				return FALSE;
			}

			memcpy (*pArgv, temp, cArgv * sizeof(LPTSTR));

			LocalFree (temp);
			
			cArgv += DEFAULT_ARGV_SIZE;
		}
    }
    return TRUE;
}


DWORD 
AddArgvDyn (LPTSTR **pArgv, DWORD *pArgc, LPTSTR szNew)
{
	DWORD cArgv = (*pArgc / DEFAULT_ARGV_SIZE + ( *pArgc % DEFAULT_ARGV_SIZE ? 1 : 0 )) * DEFAULT_ARGV_SIZE;
	LPTSTR *temp = NULL;

	if (cArgv <= *pArgc)
	{
		temp = *pArgv;
		*pArgv = (LPTSTR *) LocalAlloc (LPTR, (cArgv + DEFAULT_ARGV_SIZE) * sizeof (LPTSTR));
		if (!*pArgv)
		{
			*pArgv = temp;
			return ERROR_NOT_ENOUGH_MEMORY;
		}
		

		memcpy (*pArgv, temp, cArgv * sizeof(LPTSTR));
		if (temp)
			LocalFree (temp);

		cArgv += DEFAULT_ARGV_SIZE;

		ASSERT (*pArgc < cArgv);
	}


	//this means there is room for another LPTSTR
	(*pArgv)[*pArgc] = szNew;	//NOTE this is volatile memory not alloc by us
	(*pArgc)++;
	return ERROR_SUCCESS;
}




