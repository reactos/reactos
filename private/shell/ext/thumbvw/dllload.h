#ifndef _DLLLOAD_H_
#define _DLLLOAD_H_

//
// Temporarily delay load any post-w95 shell32 APIs
//

// (These alternative names are used to avoid the inconsistent DLL linkage
// errors we get in dllload.c if we use the real function name.)


//
//  WARNING!  These functions are available only on NT5 shell.  If you
//  call them with an IE4 shell or earlier, they will try to emulate
//  or possibly just fail outright.  Be prepared.
//
STDAPI_(BOOL)    _DAD_DragEnterEx2(HWND hwndTarget, const POINT ptStart, IDataObject *pdtObject);

#define DAD_DragEnterEx2            _DAD_DragEnterEx2

#endif // _DLLLOAD_H_

