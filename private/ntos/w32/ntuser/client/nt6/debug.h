//
//
//

 // #define _DEBUG

#ifdef _DEBUG
void FAR _cdecl 
TRACE(
	LPTSTR lpszFormat, 
	...);

#else
#define TRACE 0?0:
#endif
