
#include <windows.h>
#include <stdio.h>

#include "testsuite.h"

static PTEST_RUNNER iTestRunner = 0;

void  tsRunTests (PTEST_RUNNER pTestRunner, PTEST_SUITE pTests)
{
  int testIndex;

  iTestRunner = pTestRunner;
  for (testIndex = 0; pTests [testIndex].testFunc != 0; testIndex++)
  {
    pTests [testIndex].testFunc ();
    pTestRunner->tests++;
  }
}

void  tsReportResults (PTEST_RUNNER pTestRunner)
{
  printf ("\nTotal of %d tests.\n"
          "Assertions: %d\n"
          "  Assertions which passed: %d\n"
          "  Assertions which failed: %d\n",
          pTestRunner->tests,
          pTestRunner->assertions,
          pTestRunner->successes,
          pTestRunner->failures);
  if (pTestRunner->failures == 0)
  {
    printf ("\n*** OK ***\n");
  }
  else
  {
    printf ("\n*** FAIL ***\n");
  }
}

void  tsDoAssertion (BOOL  pTest, 
                     PCHAR  pTestText, 
                     PCHAR  pFunction, 
                     int  pLine,
                     ...)
{
  va_list  ap;

  iTestRunner->assertions++;
  if (!pTest) 
  {
    va_start (ap, pLine);
    printf ("%s(%d): ", pFunction, pLine);
    vprintf (pTestText, ap); 
    printf ("\n");
    va_end(ap);
    iTestRunner->failures++;
  }
  else
  {
    iTestRunner->successes++;
  }
}


