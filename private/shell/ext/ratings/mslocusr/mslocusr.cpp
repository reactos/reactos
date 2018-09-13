#include "mslocusr.h"
#include <netspi.h>

/* the following defs will make msluglob.h actually define globals */
#define EXTERN
#define ASSIGN(value) = value
#include "msluglob.h"

HANDLE g_hmtxShell = 0;              // Note: Handle is per-instance.

#ifdef DEBUG
BOOL g_fCritical=FALSE;
#endif

HINSTANCE hInstance = NULL;

const char szMutexName[] = "MSLocUsrMutex";
UINT g_cRefThisDll = 0;		// Reference count of this DLL.
UINT g_cLocks = 0;			// Number of locks on this server.


void LockThisDLL(BOOL fLock)
{
	ENTERCRITICAL
	{
		if (fLock)
			g_cLocks++;
		else
			g_cLocks--;
	}
	LEAVECRITICAL
}


void RefThisDLL(BOOL fRef)
{
	ENTERCRITICAL
	{
		if (fRef)
			g_cRefThisDll++;
		else
			g_cRefThisDll--;
	}
	LEAVECRITICAL
}


void Netlib_EnterCriticalSection(void)
{
    WaitForSingleObject(g_hmtxShell, INFINITE);
#ifdef DEBUG
    g_fCritical=TRUE;
#endif
}

void Netlib_LeaveCriticalSection(void)
{
#ifdef DEBUG
    g_fCritical=FALSE;
#endif
    ReleaseMutex(g_hmtxShell);
}

void _ProcessAttach()
{
    //
    // All the per-instance initialization code should come here.
    //
    // We should not pass TRUE as fInitialOwner, read the CreateMutex
    // section of Win32 API help file for detail.
    //
	::DisableThreadLibraryCalls(::hInstance);
    
    g_hmtxShell = CreateMutex(NULL, FALSE, ::szMutexName);  // per-instance

    ::InitStringLibrary();
}

void _ProcessDetach()
{
    UnloadShellEntrypoint();
    CloseHandle(g_hmtxShell);
}

STDAPI_(BOOL) DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID reserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hInstDll;
	_ProcessAttach();
    }
    else if (fdwReason == DLL_PROCESS_DETACH) 
    {
	_ProcessDetach();
    }


    return TRUE;
}


UINT
NPSCopyNLS ( 
    NLS_STR FAR *   pnlsSourceString, 
    LPVOID          lpDestBuffer, 
    LPDWORD         lpBufferSize )
{
    if ((!lpBufferSize) || (!lpDestBuffer && (*lpBufferSize != 0))) {
        return ERROR_INVALID_PARAMETER;
    }
    if (pnlsSourceString != NULL) {

        DWORD   dwDestLen = 0;  // bytes copied to dest buffer, including NULL
        DWORD   dwSourceLen = pnlsSourceString->strlen() + 1; // bytes in source buffer, including NULL

        if ((lpDestBuffer) && (*lpBufferSize != 0)) {
            NLS_STR nlsDestination( STR_OWNERALLOC_CLEAR, (LPSTR)lpDestBuffer, (UINT) *lpBufferSize );        
            nlsDestination = *pnlsSourceString;      /* copy source string to caller's buffer */
            dwDestLen = nlsDestination.strlen() + 1;
        }
        if (dwSourceLen != dwDestLen) {
            // Only update buffersize parameter if there is more data,
            // and store source string size, counting NULL.
            *lpBufferSize = dwSourceLen;
            return ERROR_MORE_DATA;
        }
        else {
            return NOERROR;
        }
    }
    else {
        if (*lpBufferSize == 0) {
            *lpBufferSize = 1;
            return ERROR_MORE_DATA;
        }
        else {
            *(LPSTR)lpDestBuffer = NULL; // validated to not be NULL above
            return NOERROR;
        }
    }            
}

DWORD
NPSCopyString (
    LPCTSTR lpSourceString,
    LPVOID  lpDestBuffer,
    LPDWORD lpBufferSize )
{
    if (lpSourceString != NULL) {    
        NLS_STR nlsSource( STR_OWNERALLOC, (LPTSTR)lpSourceString );
        return NPSCopyNLS ( &nlsSource,
                            lpDestBuffer,
                            lpBufferSize );
    }
    else {
        return NPSCopyNLS ( NULL,
                            lpDestBuffer,
                            lpBufferSize );    
    }                               
}

