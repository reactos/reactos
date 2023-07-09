#ifndef _DEBUG_H
#define _DEBUG_H

void _coreASSERT(const char *filename, int line, const char *errMst, void (*cleanup)());

void dumpOnBuild();

#ifdef _ASSERT
#undef _ASSERT
#endif

// DESCRIPTION:
//		macro useful for run-time debugging
//		it tels exactly which source and which line an error occured.
//		it also calls a user-supplied clean-up function, if available
//		and exits the program.
// PARAMETERS:
//		(in)b - boolean condition that has to be checked
//		(in)s - error message string to be displayed if assertion fails
//		(in)f - user-defined function to be called for cleanup before exiting
#define _ASSERT(b,s,f)											\
	{															\
		if (!(b))												\
		{														\
			_coreASSERT(__FILE__, __LINE__, (s), (f));			\
			exit(-1);											\
		}														\
	}

// DESCRIPTION
//		macro useful for parameter checking.
// PARAMETERS
//		(in)b - boolean condition that has to be checked
//		(in)v - value to be returned if condition fails
#define _VERIFY(b,v)													\
	{																	\
		if (!(b))														\
		{																\
			_coreASSERT(__FILE__, __LINE__, " _VERIFY failure ", NULL);	\
			return v;													\
		}																\
	}
															
#endif
