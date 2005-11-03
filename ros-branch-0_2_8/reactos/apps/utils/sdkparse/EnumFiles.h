//
// EnumFiles.h
//

#ifndef ENUMFILES_H
#define ENUMFILES_H

typedef BOOL (*MYENUMFILESPROC) ( PWIN32_FIND_DATA, const TCHAR*, long );
BOOL EnumFilesInDirectory ( const TCHAR* szDirectory, const TCHAR* szFileSpec, MYENUMFILESPROC pProc, long lParam, BOOL bSubsToo, BOOL bSubsMustMatchFileSpec = FALSE );

#endif//ENUMFILES_H
