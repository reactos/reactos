//*******************************************************************************************
//
// Filename : CabP.h
//	
// Some useful macros and routines 
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************


#ifndef _CABP_H_
#define _CABP_H_

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//
// Private QueryContextMenuFlag passed from DefView
//
#define CMF_DVFILE       0x00010000     // "File" pulldown

#define ResultFromShort(i)  ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(i)))
#define ShortFromResult(r)  (short)SCODE_CODE(GetScode(r))

//
// OLE string
//
int OleStrToStrN(LPTSTR, int, LPCOLESTR, int);
int StrToOleStrN(LPOLESTR, int, LPCTSTR, int);
int OleStrToStr(LPTSTR, LPCOLESTR);
int StrToOleStr(LPOLESTR, LPCTSTR);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif // _CABP_H_
