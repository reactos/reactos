#include "fileshar.h"

typedef struct {
    LPVOID	lp;
    LONG	cb;
} EXTRA, FAR * LPEXTRA;

HRESULT ReadExtra(LPEXTRA extra,
		DWORD ckid,
	       LPVOID lpData,
	       LONG FAR *lpcbData);
HRESULT WriteExtra(LPEXTRA extra,
		DWORD ckid,
		LPVOID lpData,
		LONG cbData);

HRESULT ReadIntoExtra(LPEXTRA extra,
		   HSHFILE hshfile,
		   MMCKINFO FAR *lpck);

LONG FindChunkAndKeepExtras(LPEXTRA extra, HSHFILE hshfile,
			MMCKINFO FAR* lpck, MMCKINFO FAR* lpckParent,
			UINT uFlags);
