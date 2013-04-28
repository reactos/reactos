//
// assert.h
//

#ifndef __ASSERT_H
#define __ASSERT_H

#ifndef _MFC_VER
#ifdef _WIN32_WCE

#else//_WIN32_WCE
	#include <assert.h>
#endif//_WIN32_WCE
#endif

#ifndef ASSERT
	#ifdef _DEBUG
		#include <crtdbg.h>
		#include <stdio.h> // _snprintf
		//#ifndef WINVER
		#ifdef _CONSOLE
			#define ASSERT(x) if(!(x)){printf("ASSERT FAILURE: (%s) at %s:%i\n", #x, __FILE__, __LINE__); _CrtDbgBreak(); }
		#else//_CONSOLE/WINVER
			#define ASSERT(x) if(!(x)){char stmp_assert[1024+1]; _snprintf(stmp_assert,1024,"ASSERT FAILURE: (%s) at %s:%i\n",#x,__FILE__,__LINE__); ::MessageBox(NULL,stmp_assert,"Assertion Failure",MB_OK|MB_ICONSTOP); _CrtDbgBreak(); }
		#endif//_CONSOLE/WINVER
	#else//_DEBUG
		#define ASSERT(x)
	#endif//_DEBUG
#endif//ASSERT

#undef VERIFY
#ifdef _DEBUG
	#define VERIFY(x) ASSERT(x)
#else//_DEBUG
	#define VERIFY(x) x
#endif//_DEBUG

// code for ASSERTing in Release mode...
#ifdef RELEASE_ASSERT
	#undef ASSERT
	#include <stdio.h>
	#define ASSERT(x) if ( !(x) ) { char s[1024+1]; _snprintf(s,1024,"ASSERTION FAILURE:\n%s\n\n%s: line %i", #x, __FILE__, __LINE__ ); ::MessageBox(NULL,s,"Assertion Failure",MB_OK|MB_ICONERROR); }
	#undef VERIFY
	#define VERIFY ASSERT
#endif//RELEASE_ASSERT

#endif//__ASSERT_H
