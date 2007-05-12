#include "ddrawtest.h"

PCHAR DDErrorString (HRESULT hResult);

BOOL Test_CreateDDraw (INT* passed, INT* failed)
{
	LPDIRECTDRAW7 DirectDraw;
	IDirectDraw* DirectDraw2;

	// FIXME: Test first parameter 

	TEST (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, (IUnknown*)0xdeadbeef) == CLASS_E_NOAGGREGATION);
	TEST (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw4, NULL) == DDERR_INVALIDPARAMS);
	TEST (DirectDrawCreateEx(NULL, NULL, IID_IDirectDraw7, NULL) == DDERR_INVALIDPARAMS); 
	TEST (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL) == DD_OK);
	TEST (DirectDrawCreate(NULL ,&DirectDraw2, NULL) == DD_OK);

	return TRUE;
}
