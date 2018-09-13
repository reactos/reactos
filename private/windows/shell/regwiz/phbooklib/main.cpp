// ############################################################################
#include "pch.hpp"

extern "C" {
HINSTANCE g_hInstDll;	// instance for this DLL  
}


#ifdef WIN16

int CALLBACK LibMain(HINSTANCE hinst, 
						WORD wDataSeg, 
						WORD cbHeap,
						LPSTR lpszCmdLine )
{
	g_hInstDll = hinst;

	return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   PrivateMalloc()
//
//  Synopsis:   Allocate and initialize memory
//
//  Arguments:  [size	- Size of memory block to be allocated]
//
//	Returns:	pointer to memory block if successful
//				NULL otherwise
//
//  History:    7/9/96     VetriV    Created
//
//----------------------------------------------------------------------------
void far *PrivateMalloc(size_t size)
{
	void far * ReturnValue = NULL;
	
	ReturnValue = malloc(size);
	if (NULL != ReturnValue)
		memset(ReturnValue, 0, size);
		
	return ReturnValue;
}

//+---------------------------------------------------------------------------
//
//  Function:   PrivateReAlloc()
//
//  Synopsis:   Reallocate memory
//
//  Arguments:  [lpBlock 	- Block to be reallocated ]
//				[size		- Size of memory block to be allocated]
//
//	Returns:	pointer to memory block if successful
//				NULL otherwise
//
//  History:    7/25/96     ValdonB    Created
//
//----------------------------------------------------------------------------
void far *PrivateReAlloc(void far *lpBlock, size_t size)
{
	void far *lpRetBlock;
	
	lpRetBlock = PrivateMalloc(size);
	if (NULL == lpRetBlock)
		return NULL;
	
	if (NULL != lpBlock)
	{
		size_t OldBlockSize, MoveSize;
		
		OldBlockSize = _msize(lpBlock);
		MoveSize = min(OldBlockSize, size);
		memmove(lpRetBlock, lpBlock, MoveSize);  
		PrivateFree(lpBlock);
	}
	
	return lpRetBlock;
}


//+---------------------------------------------------------------------------
//
//  Function:   PrivateFree
//
//  Synopsis:   Free a block of memory
//
//  Arguments:  [lpBlock - Block to be freed]
//
//	Returns:	Nothing
//
//  History:    7/9/96     VetriV    Created
//
//----------------------------------------------------------------------------
void PrivateFree(void far *lpBlock)
{
	free(lpBlock);
}


//+---------------------------------------------------------------------------
//
//  Function:   SearchPath()
//
//  Synopsis:   Searchs for the specified file in the given path
//
//  Arguments:  [lpPath			- Address of search path]
//				[lpFileName		- Address of filename]
//				[lpExtension	- Address of Extension]
//				[nBufferLength	- size, in characters, of buffer]
//				[lpBuffer		- address of buffer for found filename]
//				[lpFilePart		- address of pointer to file component]
//
//	Returns:	Length of string copied to buffer (not including terminating
//					NULL character) if successful
//				0 otherwise
//
//  History:    7/9/96     VetriV    Created
//
//----------------------------------------------------------------------------
DWORD SearchPath(LPCTSTR lpPath,LPCTSTR lpFileName, LPCTSTR lpExtension,
					DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart)
{ 
	
	BOOL bUseExtension = FALSE, bPathContainsFileName = FALSE;
	DWORD dwRequiredLength;
	LPSTR lpszPath = lpPath;
	char szFileName[MAX_PATH+1];
	OFSTRUCT OpenBuf;
		
	
	
	// Check if extension should be used
	//
	if ((NULL != lpExtension) && !strchr(lpFileName, '.'))
		bUseExtension = TRUE;

	//
	// Form Filename
	//
	lstrcpy(szFileName, lpFileName);
	if (bUseExtension)
		lstrcat(szFileName, lpExtension);
	
	
	//
	// If search path is NULL, then try to OpenFile using OF_SEARCH flag
	// get the full path in OpenBuf struct
	//
	if (NULL == lpszPath)
	{
		
		if (HFILE_ERROR != OpenFile(szFileName, &OpenBuf, OF_EXIST | OF_SEARCH))
		{ 
			//
			// This path contains the file name also
			//
			lpszPath = &OpenBuf.szPathName[0];
			bPathContainsFileName = TRUE;
		}
		else
			return 0;
	}
			
	//
	// Check if output buffer length is sufficient
	//
	dwRequiredLength = lstrlen(lpszPath) + 
						(bPathContainsFileName ? 0 :lstrlen(szFileName)) + 1;
	if (nBufferLength < dwRequiredLength)
		return 0;

	//
	// Copy the full name to buffer
	//
	if (bPathContainsFileName)
		lstrcpy(lpBuffer, lpszPath);
	else
		wsprintf(lpBuffer, "%s\\%s", lpszPath, szFileName);

	
	//
	// Do not include the terminating null character in return length
	//
	return dwRequiredLength - 1;
}


#else // WIN16

#ifdef   _NOT_USING_ACTIVEX 
extern "C" __declspec(dllexport) BOOL WINAPI DllMain(
    HINSTANCE  hinstDLL,	// handle to DLL module 
    DWORD  fdwReason,		// reason for calling function 
    LPVOID  lpvReserved 	// reserved 
   )
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		g_hInstDll = hinstDLL;
    
	return TRUE;
}
#if 0
extern "C" __declspec(dllexport) BOOL WINAPI DllMain(
    HINSTANCE  hinstDLL,	// handle to DLL module 
    DWORD  fdwReason,		// reason for calling function 
    LPVOID  lpvReserved 	// reserved 
   )
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		g_hInstDll = hinstDLL;
    
	return TRUE;
}
#endif

#endif
#endif // WIN16


