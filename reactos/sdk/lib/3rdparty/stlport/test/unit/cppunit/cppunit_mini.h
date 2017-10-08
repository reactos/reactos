/*
 * Copyright (c) 2003, 2004
 * Zdenek Nemec
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

/* $Id$ */

#ifndef _CPPUNITMPFR_H_
#define _CPPUNITMPFR_H_

#if 0
#  define CPPUNIT_NS CppUnitMini
#else
#  define CPPUNIT_NS
#endif

#include <string.h>

#if 0
namespace CPPUNIT_NS
{
#endif
  class Reporter {
  public:
    virtual ~Reporter() {}
    virtual void error(const char * /*macroName*/, const char * /*in_macro*/, const char * /*in_file*/, int /*in_line*/) {}
    virtual void message( const char * /*msg*/ ) {}
    virtual void progress( const char * /*in_className*/, const char * /*in_testName*/, bool /*ignored*/, bool /* explicit */) {}
    virtual void end() {}
    virtual void printSummary() {}
  };

  class TestFixture {
  public:
    virtual ~TestFixture() {}

    //! \brief Set up context before running a test.
    virtual void setUp() {}

    //! Clean up after the test run.
    virtual void tearDown() {}
  };

  class TestCase : public TestFixture {
  public:
    TestCase() { registerTestCase(this); }

    void setUp() { m_failed = false; }
    static int run(Reporter *in_reporter = 0, const char *in_testName = "", bool invert = false);
    int numErrors() { return m_numErrors; }
    static void registerTestCase(TestCase *in_testCase);

    virtual void myRun(const char * /*in_name*/, bool /*invert*/ = false) {}

    virtual void error(const char *in_macroName, const char *in_macro, const char *in_file, int in_line) {
      m_failed = true;
      if (m_reporter) {
        m_reporter->error(in_macroName, in_macro, in_file, in_line);
      }
    }

    static void message(const char *msg) {
      if (m_reporter) {
        m_reporter->message(msg);
      }
    }

    bool equalDoubles(double in_expected, double in_real, double in_maxErr) {
      double diff = in_expected - in_real;
      if (diff < 0.) {
        diff = -diff;
      }
      return diff < in_maxErr;
    }

    virtual void progress(const char *in_className, const char *in_functionName, bool ignored, bool explicitTest) {
      ++m_numTests;
      if (m_reporter) {
        m_reporter->progress(in_className, in_functionName, ignored, explicitTest);
      }
    }

    bool shouldRunThis(const char *in_desiredTest, const char *in_className, const char *in_functionName,
                       bool invert, bool explicit_test, bool &do_progress) {
      if ((in_desiredTest) && (in_desiredTest[0] != '\0')) {
        do_progress = false;
        const char *ptr = strstr(in_desiredTest, "::");
        if (ptr) {
          bool match = (strncmp(in_desiredTest, in_className, strlen(in_className)) == 0) &&
                       (strncmp(ptr + 2, in_functionName, strlen(in_functionName)) == 0);
          // Invert shall not make explicit test run:
          return invert ? (match ? !match : !explicit_test)
                        : match;
        }
        bool match = (strcmp(in_desiredTest, in_className) == 0);
        do_progress = match;
        return !explicit_test && (match == !invert);
      }
      do_progress = true;
      return !explicit_test;
    }

    void tearDown() {
      if (m_failed)
        ++m_numErrors;
      m_reporter->end();
    }

  protected:
    static int m_numErrors;
    static int m_numTests;

  private:
    static TestCase *m_root;
    TestCase *m_next;
    bool m_failed;

    static Reporter *m_reporter;
  };
#if 0
}
#endif

#if !defined (CPPUNIT_MINI_HIDE_UNUSED_VARIABLE)
#  if defined (_MSC_VER)
#    define CPPUNIT_MINI_HIDE_UNUSED_VARIABLE(v) (v);
#  else
#    define CPPUNIT_MINI_HIDE_UNUSED_VARIABLE(v)
#  endif
#endif

#define CPPUNIT_TEST_SUITE(X) \
  typedef CPPUNIT_NS::TestCase Base; \
  virtual void myRun(const char *in_name, bool invert = false) { \
    const char *className = #X; CPPUNIT_MINI_HIDE_UNUSED_VARIABLE(className) \
    bool ignoring = false; CPPUNIT_MINI_HIDE_UNUSED_VARIABLE(ignoring)

#if defined CPPUNIT_MINI_USE_EXCEPTIONS
#  define CPPUNIT_TEST_BASE(X, Y) \
  { \
    bool do_progress; \
    bool shouldRun = shouldRunThis(in_name, className, #X, invert, Y, do_progress); \
    if (shouldRun || do_progress) { \
      setUp(); \
      progress(className, #X, ignoring || !shouldRun, !ignoring && Y); \
      if (shouldRun && !ignoring) { \
        try { \
          X(); \
        } \
        catch(...) { \
          Base::error("Test Failed: An Exception was thrown.", #X, __FILE__, __LINE__); \
        } \
      } \
      tearDown(); \
    } \
  }
#else
#  define CPPUNIT_TEST_BASE(X, Y) \
  { \
    bool do_progress; \
    bool shouldRun = shouldRunThis(in_name, className, #X, invert, Y, do_progress); \
    if (shouldRun || do_progress) { \
      setUp(); \
      progress(className, #X, ignoring || !shouldRun, !ignoring && Y); \
      if (shouldRun && !ignoring) \
        X(); \
      tearDown(); \
    } \
  }
#endif

#define CPPUNIT_TEST(X) CPPUNIT_TEST_BASE(X, false)
#define CPPUNIT_EXPLICIT_TEST(X) CPPUNIT_TEST_BASE(X, true)

#define CPPUNIT_IGNORE \
  ignoring = true

#define CPPUNIT_STOP_IGNORE \
  ignoring = false

#define CPPUNIT_TEST_SUITE_END() }

#define CPPUNIT_TEST_SUITE_REGISTRATION(X) static X local

#define CPPUNIT_CHECK(X) \
  if (!(X)) { \
    Base::error("CPPUNIT_CHECK", #X, __FILE__, __LINE__); \
  }

#define CPPUNIT_ASSERT(X) \
  if (!(X)) { \
    Base::error("CPPUNIT_ASSERT", #X, __FILE__, __LINE__); \
    return; \
  }

#define CPPUNIT_FAIL { \
    Base::error("CPPUNIT_FAIL", "", __FILE__, __LINE__); \
    return; \
  }

#define CPPUNIT_ASSERT_EQUAL(X, Y) \
  if ((X) != (Y)) { \
    Base::error("CPPUNIT_ASSERT_EQUAL", #X","#Y, __FILE__, __LINE__); \
    return; \
  }

#define CPPUNIT_ASSERT_DOUBLES_EQUAL(X, Y, Z) \
  if (!equalDoubles((X), (Y), (Z))) { \
    Base::error("CPPUNIT_ASSERT_DOUBLES_EQUAL", #X","#Y","#Z, __FILE__, __LINE__); \
    return; \
  }

#define CPPUNIT_MESSAGE(m) CPPUNIT_NS::TestCase::message(m)

#endif
