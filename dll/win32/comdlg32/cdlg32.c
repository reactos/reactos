/*
 *  Common Dialog Boxes interface (32 bit)
 *  Find/Replace
 *
 * Copyright 1999 Bertho A. Stultiens
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

HINSTANCE	COMDLG32_hInstance = 0;

static DWORD COMDLG32_TlsIndex = TLS_OUT_OF_INDEXES;

HINSTANCE	SHELL32_hInstance = 0;
HINSTANCE	SHFOLDER_hInstance = 0;

/* ITEMIDLIST */
LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILClone) (LPCITEMIDLIST);
LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILCombine)(LPCITEMIDLIST,LPCITEMIDLIST);
LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILGetNext)(LPITEMIDLIST);
BOOL (WINAPI *COMDLG32_PIDL_ILRemoveLastID)(LPCITEMIDLIST);
BOOL (WINAPI *COMDLG32_PIDL_ILIsEqual)(LPCITEMIDLIST, LPCITEMIDLIST);

/* SHELL */
LPVOID (WINAPI *COMDLG32_SHAlloc)(DWORD);
DWORD (WINAPI *COMDLG32_SHFree)(LPVOID);
BOOL (WINAPI *COMDLG32_SHGetFolderPathA)(HWND,int,HANDLE,DWORD,LPSTR);
BOOL (WINAPI *COMDLG32_SHGetFolderPathW)(HWND,int,HANDLE,DWORD,LPWSTR);

/***********************************************************************
 *	DllMain  (COMDLG32.init)
 *
 *    Initialization code for the COMDLG32 DLL
 *
 * RETURNS:
 *	FALSE if sibling could not be loaded or instantiated twice, TRUE
 *	otherwise.
 */
static const char * GPA_string = "Failed to get entry point %s for hinst = 0x%08x\n";
#define GPA(dest, hinst, name) \
	if(!(dest = (void*)GetProcAddress(hinst,name)))\
	{ \
	  ERR((LPSTR)GPA_string, debugstr_a(name), hinst); \
	  return FALSE; \
	}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
{
	TRACE("(%p, %ld, %p)\n", hInstance, Reason, Reserved);

	switch(Reason)
	{
	case DLL_PROCESS_ATTACH:
		COMDLG32_hInstance = hInstance;
		DisableThreadLibraryCalls(hInstance);

		SHELL32_hInstance = GetModuleHandleA("SHELL32.DLL");

		if (!SHELL32_hInstance)
		{
			ERR("loading of shell32 failed\n");
			return FALSE;
		}

		/* ITEMIDLIST */
		GPA(COMDLG32_PIDL_ILIsEqual, SHELL32_hInstance, (LPCSTR)21L);
		GPA(COMDLG32_PIDL_ILCombine, SHELL32_hInstance, (LPCSTR)25L);
		GPA(COMDLG32_PIDL_ILGetNext, SHELL32_hInstance, (LPCSTR)153L);
		GPA(COMDLG32_PIDL_ILClone, SHELL32_hInstance, (LPCSTR)18L);
		GPA(COMDLG32_PIDL_ILRemoveLastID, SHELL32_hInstance, (LPCSTR)17L);

		/* SHELL */

		GPA(COMDLG32_SHAlloc, SHELL32_hInstance, (LPCSTR)196L);
		GPA(COMDLG32_SHFree, SHELL32_hInstance, (LPCSTR)195L);
		/* for the first versions of shell32 SHGetFolderPathA is in SHFOLDER.DLL */
		COMDLG32_SHGetFolderPathA = (void*)GetProcAddress(SHELL32_hInstance,"SHGetFolderPathA");
		if (!COMDLG32_SHGetFolderPathA)
		{
		  SHFOLDER_hInstance = LoadLibraryA("SHFOLDER.DLL");
		  GPA(COMDLG32_SHGetFolderPathA, SHFOLDER_hInstance,"SHGetFolderPathA");
		}

		/* for the first versions of shell32 SHGetFolderPathW is in SHFOLDER.DLL */
		COMDLG32_SHGetFolderPathW = (void*)GetProcAddress(SHELL32_hInstance,"SHGetFolderPathW");
		if (!COMDLG32_SHGetFolderPathW)
		{
		  SHFOLDER_hInstance = LoadLibraryA("SHFOLDER.DLL");
		  GPA(COMDLG32_SHGetFolderPathW, SHFOLDER_hInstance,"SHGetFolderPathW");
		}

		break;

	case DLL_PROCESS_DETACH:
            if (COMDLG32_TlsIndex != TLS_OUT_OF_INDEXES) TlsFree(COMDLG32_TlsIndex);
            if(SHFOLDER_hInstance) FreeLibrary(SHFOLDER_hInstance);
            break;
	}
	return TRUE;
}
#undef GPA

/***********************************************************************
 *	COMDLG32_AllocMem 			(internal)
 * Get memory for internal datastructure plus stringspace etc.
 *	RETURNS
 *		Success: Pointer to a heap block
 *		Failure: null
 */
LPVOID COMDLG32_AllocMem(
	int size	/* [in] Block size to allocate */
) {
        LPVOID ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        if(!ptr)
        {
        	COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
                return NULL;
        }
        return ptr;
}


/***********************************************************************
 *	COMDLG32_SetCommDlgExtendedError	(internal)
 *
 * Used to set the thread's local error value if a comdlg32 function fails.
 */
void COMDLG32_SetCommDlgExtendedError(DWORD err)
{
	TRACE("(%08lx)\n", err);
        if (COMDLG32_TlsIndex == TLS_OUT_OF_INDEXES)
	  COMDLG32_TlsIndex = TlsAlloc();
	if (COMDLG32_TlsIndex != TLS_OUT_OF_INDEXES)
	  TlsSetValue(COMDLG32_TlsIndex, (LPVOID)(DWORD_PTR)err);
	else
	  FIXME("No Tls Space\n");
}


/***********************************************************************
 *	CommDlgExtendedError			(COMDLG32.@)
 *	CommDlgExtendedError			(COMMDLG.26)
 *
 * Get the thread's local error value if a comdlg32 function fails.
 *	RETURNS
 *		Current error value which might not be valid
 *		if a previous call succeeded.
 */
DWORD WINAPI CommDlgExtendedError(void)
{
        if (COMDLG32_TlsIndex != TLS_OUT_OF_INDEXES)
	  return (DWORD_PTR)TlsGetValue(COMDLG32_TlsIndex);
	else
	  return 0; /* we never set an error, so there isn't one */
}
