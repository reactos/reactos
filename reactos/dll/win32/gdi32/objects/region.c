#include "precomp.h"


/*
 * @implemented
 */
int
STDCALL
GetClipRgn(
        HDC     hdc,
        HRGN    hrgn
        )
{
	HRGN rgn = NtGdiGetClipRgn(hdc);
	if(rgn)
	{
		if(NtGdiCombineRgn(hrgn, rgn, 0, RGN_COPY) != ERROR) return 1;
		else
			return -1;
	}
	else	return 0;
}

