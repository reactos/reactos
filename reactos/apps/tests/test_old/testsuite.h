
#ifndef H_TESTSUITE_H
#define H_TESTSUITE_H

struct TestSuite
{
  char *name;
  void (*testFunc)(void);
};

typedef struct TestSuite  TEST_SUITE, *PTEST_SUITE;

#define ADD_TEST(x)  {#x, x}
#define END_TESTS  {0, 0}
#define COUNT_TESTS(x)  (sizeof x/sizeof (struct TestSuite))

struct TestRunner
{
  int  tests;
  int  assertions;
  int  failures;
  int  successes;
};

typedef struct TestRunner  TEST_RUNNER, *PTEST_RUNNER;

void  tsDoAssertion (BOOL pTest, 
                     PCHAR pTestText, 
                     PCHAR pFunction, 
                     int pLine,
                     ...);

#ifdef ASSERT
#undef ASSERT
#endif
#define  ASSERT(x) tsDoAssertion (x, "assertion \"" ## #x ## "\" failed", __FUNCTION__, __LINE__)
#define  ASSERT_MSG(x,y,a...) tsDoAssertion (x, y, __FUNCTION__, __LINE__,a)

void  tsRunTests (PTEST_RUNNER pTestRunner, PTEST_SUITE pTests);
void  tsReportResults (PTEST_RUNNER pTestRunner);

#endif


