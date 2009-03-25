#ifndef _APITEST_H
#define _APITEST_H

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

#define APISTATUS_NORMAL 0
#define APISTATUS_NOT_FOUND 1
#define APISTATUS_UNIMPLEMENTED 2
#define APISTATUS_ASSERTION_FAILED 3
#define APISTATUS_REGRESSION 4

/* type definitions */

typedef struct tagTESTINFO
{
	INT passed;
	INT failed;
	INT rfailed;
	BOOL bRegress;
	INT nApiStatus;
} TESTINFO, *PTESTINFO;

typedef INT (*TESTPROC)(PTESTINFO);

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
			printf("%s:%d: non-rtest succeeded (%s)\n", __FILE__, __LINE__, #x);\
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
			printf("%s:%d: test failed (%s)\n", __FILE__, __LINE__, #x);\
		} \
	}

#define TESTX(x, format, ...) \
	if (pti->bRegress) \
	{ \
		if (x)\
		{\
			(pti->passed)++;\
			printf("%s:%d: non-rtest succeeded (%s)\n", __FILE__, __LINE__, #x);\
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
			printf("%s:%d: test failed (%s) ", __FILE__, __LINE__, #x);\
			printf(format, __VA_ARGS__); \
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

#undef ASSERT
#define ASSERT(x) \
	if (!(x)) \
	{ \
			printf("Assertion failed in %s:%d (%s)\n", __FILE__, __LINE__, #x);\
			return APISTATUS_ASSERTION_FAILED; \
	}

int TestMain(LPWSTR pszExe, LPWSTR pszModule);
extern TESTENTRY TestList[];
INT NumTests(void);
BOOL IsFunctionPresent(LPWSTR lpszFunction);
VOID DumpMem(PVOID pData, ULONG cbSize, ULONG nWidth);

#endif /* _APITEST_H */
