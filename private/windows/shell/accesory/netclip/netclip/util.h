// util.h

#ifndef _UTIL_H_
#define _UTIL_H_

/// Utilities   

void ErrorMessageID( const UINT id, HRESULT hr);
void ErrorMessage( const CString& str, HRESULT hr ) ;

LPTSTR HRtoString( HRESULT hr ) ;
LPTSTR VTtoString( VARTYPE vt ) ;
   
LONG WINAPI ParseOffNumber( LPTSTR FAR *lplp, LPINT lpConv );

#endif 

