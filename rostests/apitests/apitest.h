#ifndef _APITEST_H
#define _APITEST_H

#define WINVER 0x501

#include <stdlib.h>

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>


/* type definitions */

typedef struct tagTESTINFO
{
	INT passed;
	INT failed;
	INT rfailed;
	BOOL bRegress;
} TESTINFO, *PTESTINFO;

typedef BOOL (*TESTPROC)(PTESTINFO);

typedef struct tagTEST
{
	WCHAR* Test;
	TESTPROC Proc;
} TESTENTRY, *PTESTENTRY;


/* macros */

#define TEST(x) \
	if (pti->bRegress) \
	{ \
		if (x)\
		{\
			(pti->passed)++;\
			printf("non-rtest succeeded in %s:%d (%s)\n", __FILE__, __LINE__, #x);\
		} else {\
			(pti->failed)++;\
		} \
	} \
	else \
	{ \
		if (x)\
		{\
			(pti->passed)++;\
		} else {\
			(pti->failed)++;\
			printf("test failed in %s:%d (%s)\n", __FILE__, __LINE__, #x);\
		} \
	}

#define RTEST(x) \
	if (pti->bRegress) \
	{ \
		if (x)\
		{\
			(pti->passed)++;\
		} else {\
			(pti->failed)++;\
			(pti->rfailed)++;\
			printf("rtest failed in %s:%d (%s)\n", __FILE__, __LINE__, #x);\
		} \
	} \
	else \
	{ \
		if (x)\
		{\
			(pti->passed)++;\
		} else {\
			(pti->failed)++;\
			printf("test failed in %s:%d (%s)\n", __FILE__, __LINE__, #x);\
		} \
	}


int TestMain(LPWSTR pszExe);
extern TESTENTRY TestList[];
INT NumTests(void);
BOOL IsFunctionPresent(LPWSTR lpszFunction);

#endif /* _APITEST_H */
