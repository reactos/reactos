//-------------------------------------------------------------------------//
//
//  PTdebug.cpp
//
//  Common debugging helpers for docprop2 modules
//
//-------------------------------------------------------------------------//
#include "pch.h"
#include "PTdebug.h"

#if (defined(_DEBUG) || defined(_ENABLE_TRACING))
#include <stdio.h>
//-------------------------------------------------------------------------//
void _cdecl DebugTrace(LPCTSTR lpszFormat, ...)
{
	va_list args;
	va_start(args, lpszFormat);

	int nBuf;
	TCHAR szBuffer[512];

	nBuf = _vstprintf(szBuffer, lpszFormat, args);
	_ASSERTE(nBuf < sizeof(szBuffer));

	OutputDebugString(szBuffer);
	va_end(args);
}
#endif