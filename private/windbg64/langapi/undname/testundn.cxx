//
// testundn.cxx
//	Main file for name undecorator test.
//
// Usage:
//	testundn [-v]
//		-v			Verbose: print ALL tests, even if they pass.
//
// The tests come from the include file "testundn.h", which is used to initialize 
// an array of test records.  Each line should have the following format:
//
//	{<flags>, "<decorated name>", "<undecorated name>" },
//
// 		<flags>				The disable-flags for the undecorator (int)
//		<decorated name>	The name to undecorate (char*)
//		<undecorated name>	The expected output (char*)
//
// The name of the test table can be changed by building with 
// -DTESTFILE=<test-file>.
//
// Each line that fails will print the flags (in hex), the decorated name, the 
// undecoration expected, and the undecoration received.  Should an exception
// occur during the process of undecoration, the undecorated name is shown as
// "***** Exception: 0x%x *****", showing the exception code received.
//
// If any tests fail, the messages "*** FAILED ***" is printed at the end, 
// otherwise "*** PASSED ***" is printed.  A status of 0 is returned on
// success, a status of 1 on failure.
//

#include <stdio.h>
#include <malloc.h>
#include <excpt.h>
#include <string.h>
#include "undname.h"

//
// The tests:
//
struct Test {
	int				Flags;
	const char *	DecoratedName;
	const char *	UndecoratedName;
};

#ifndef TESTFILE
#define TESTFILE "testundn.h"
#endif

const Test tests[] = {
#include TESTFILE
{0, 0, 0}
};

//
// A buffer to store the results in:
//
#ifndef UNDNAMEBUFSIZE
#define UNDNAMEBUFSIZE 3000
#endif

char unDnameBuffer[UNDNAMEBUFSIZE];

//
// main()
//
int main( int argc, char **argv )
{
	int verbose = 0;
	int count = 0;
	int failed_tests = 0;
	int exception = 0;				// Code of the exception, if one is taken

	//
	// Check the arguments
	//
	if (argc > 1) {
		if (_stricmp(argv[1], "-v") == 0) {
			verbose = 1;
			argc--;
		}
	}

	if (argc > 1) {
		printf("Usage:\n  %s [-v]\n", argv[0]);
	}

	//
	// Do the tests
	//
	

	for ( const Test *pTest = tests;
		  pTest->DecoratedName != NULL;
		  pTest++, count++
	) {
		char *pResult;
		int failed = 0;

		__try {
			// Undecorate the name
			pResult = unDName( unDnameBuffer,
							   pTest->DecoratedName,
							   UNDNAMEBUFSIZE,
							   &malloc,
							   &free,
							   pTest->Flags );
		}
		__except (exception = _exception_code(), EXCEPTION_EXECUTE_HANDLER) {
			// Intercept any exceptions
			sprintf(unDnameBuffer, "***** Exception: 0x%08x *****", exception);
			pResult = unDnameBuffer;
		}

		// Check for NULL
		if (pResult == NULL) {
			sprintf(unDnameBuffer, "***** <NULL> *****");
			pResult = unDnameBuffer;
		}

		// Check for failure
		if (strncmp(pResult, pTest->UndecoratedName, UNDNAMEBUFSIZE) != 0) {
			failed = 1;
			failed_tests++;
		}

		// Report result of this test
		if (failed) {
			printf("FAILED: {0x%04x, \"%s\", \"%s\"} gave \"%s\"\n",
					pTest->Flags, pTest->DecoratedName, pTest->UndecoratedName, pResult);
		}
		else if (verbose) {
			printf("PASSED: {0x%04x, \"%s\", \"%s\"}\n",
					pTest->Flags, pTest->DecoratedName, pTest->UndecoratedName);
		}
	}

	// Report final results
	if (verbose || (failed_tests > 0)) {
		printf("Passed: %d, Failed: %d\n", count, failed_tests);
	}

	if (failed_tests > 0) {
		printf("*** FAILED ***\n");
	} else {
		printf("*** PASSED ***\n");
	}

	return failed_tests;
}
