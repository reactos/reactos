#include "syshdrs.h"

#if defined(WIN32) || defined(_WINDOWS)

extern "C" void
GetSpecialDir(char *dst, size_t size, int whichDir)
{
	LPITEMIDLIST idl;
	LPMALLOC shl;
	char path[MAX_PATH + 1];
	HRESULT hResult;
	
	memset(dst, 0, size);
	hResult = SHGetMalloc(&shl);
	if (SUCCEEDED(hResult)) {
		hResult = SHGetSpecialFolderLocation(
					NULL,
					CSIDL_PERSONAL,
					&idl
					);

		if (SUCCEEDED(hResult)) {
			if(SHGetPathFromIDList(idl, path)) {
				(void) strncpy(dst, path, size - 1);
				dst[size - 1] = '\0';
			}
			shl->Free(idl);
		}
		shl->Release();
	}
}	// GetSpecialDir


#endif
