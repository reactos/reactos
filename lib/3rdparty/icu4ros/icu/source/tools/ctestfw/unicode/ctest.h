/*
********************************************************************************
*
*   Copyright (C) 1996-2004, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
********************************************************************************
*/

#ifndef CTEST_H
#define CTEST_H

#include "unicode/testtype.h"
#include "unicode/utrace.h"


/* prototypes *********************************/

U_CDECL_BEGIN
typedef void (U_CALLCONV *TestFunctionPtr)(void);
typedef struct TestNode TestNode;
U_CDECL_END

/**
 * Set this to zero to disable log_verbose() messages.
 * Otherwise nonzero to see log_verbose() messages.
 *
 * @internal Internal APIs for testing purpose only
 */
extern T_CTEST_EXPORT_API int REPEAT_TESTS;

/**
 * Set this to zero to disable log_verbose() messages.
 * Otherwise nonzero to see log_verbose() messages.
 *
 * @internal Internal APIs for testing purpose only
 */
extern T_CTEST_EXPORT_API int VERBOSITY;

/**
 * Set this to zero to disable log_verbose() messages.
 * Otherwise nonzero to see log_verbose() messages.
 *
 * @internal Internal APIs for testing purpose only
 */
extern T_CTEST_EXPORT_API int ERR_MSG;

/**
 * Set this to zero to disable some of the slower tests.
 * Otherwise nonzero to run the slower tests.
 *
 * @internal Internal APIs for testing purpose only
 */
extern T_CTEST_EXPORT_API int QUICK;

/**
 * Set this to nonzero to warn (not error) on missing data. 
 * Otherwise, zero will cause an error to be propagated when data is not available.
 * Affects the behavior of log_dataerr.
 *
 * @see log_data_err
 * @internal Internal APIs for testing purpose only
 */
extern T_CTEST_EXPORT_API int WARN_ON_MISSING_DATA;

/**
 * ICU tracing level, is set by command line option
 *
 * @internal
 */
extern T_CTEST_EXPORT_API UTraceLevel ICU_TRACE;

/**
 * Show the names of all nodes.
 *
 * @param root Subtree of tests.
 * @internal Internal APIs for testing purpose only
 */
T_CTEST_API void T_CTEST_EXPORT2
showTests ( const TestNode *root);

/**
 * Run a subtree of tests.
 *
 * @param root Subtree of tests.
 * @internal Internal APIs for testing purpose only
 */
T_CTEST_API void T_CTEST_EXPORT2
runTests ( const TestNode* root);

/**
 * Add a test to the subtree.
 * Example usage:
 * <PRE>
 *     TestNode* root=NULL;
 *     addTest(&root, &mytest, "/a/b/mytest" );
 * </PRE>
 * @param root Pointer to the root pointer.
 * @param test Pointer to 'void function(void)' for actual test.
 * @param path Path from root under which test will be placed. Ex. '/a/b/mytest'
 * @internal Internal APIs for testing purpose only
 */
T_CTEST_API void T_CTEST_EXPORT2
addTest(TestNode** root,
        TestFunctionPtr test,
        const char *path);

/**
 * Clean up any allocated memory.
 * Conditions for calling this function are the same as u_cleanup().
 * @see u_cleanup
 * @internal Internal APIs for testing purpose only
 */
T_CTEST_API void T_CTEST_EXPORT2
cleanUpTestTree(TestNode *tn);

/**
 * Retreive a specific subtest. (subtree).
 *
 * @param root Pointer to the root.
 * @param path Path relative to the root, Ex. '/a/b'
 * @return The subtest, or NULL on failure.
 * @internal Internal APIs for testing purpose only
 */
T_CTEST_API const TestNode* T_CTEST_EXPORT2
getTest(const TestNode* root,
        const char *path);


/**
 * Log an error message. (printf style)
 * @param pattern printf-style format string
 * @internal Internal APIs for testing purpose only
 */
T_CTEST_API void T_CTEST_EXPORT2
log_err(const char* pattern, ...);

/**
 * Log an informational message. (printf style)
 * @param pattern printf-style format string
 * @internal Internal APIs for testing purpose only
 */
T_CTEST_API void T_CTEST_EXPORT2
log_info(const char* pattern, ...);

/**
 * Log an informational message. (vprintf style)
 * @param prefix a string that is output before the pattern and without formatting
 * @param pattern printf-style format string
 * @param ap variable-arguments list
 * @internal Internal APIs for testing purpose only
 */
T_CTEST_API void T_CTEST_EXPORT2
vlog_info(const char *prefix, const char *pattern, va_list ap);

/**
 * Log a verbose informational message. (printf style)
 * This message will only appear if the global VERBOSITY is nonzero
 * @param pattern printf-style format string
 * @internal Internal APIs for testing purpose only
 */
T_CTEST_API void T_CTEST_EXPORT2
log_verbose(const char* pattern, ...);

/**
 * Log an error message concerning missing data. (printf style)
 * If WARN_ON_MISSING_DATA is nonzero, this will case a log_info (warning) to be
 * printed, but if it is zero this will produce an error (log_err).
 * @param pattern printf-style format string
 * @internal Internal APIs for testing purpose only
 */
T_CTEST_API void T_CTEST_EXPORT2
log_data_err(const char *pattern, ...);

/**
 * Processes the command line arguments.
 * This is a sample implementation
 * <PRE>Usage: %s [ -l ] [ -v ] [ -? ] [ /path/to/test ]
 *        -l List only, do not run\
 *        -v turn OFF verbosity
 *        -? print this message</PRE>
 * @param root Testnode root with tests already attached to it
 * @param argv argument list from main (stdio.h)
 * @param argc argument list count from main (stdio.h)
 * @return positive for error count, 0 for success, negative for illegal argument
 * @internal Internal APIs for testing purpose only
 */
T_CTEST_API int T_CTEST_EXPORT2 
processArgs(const TestNode* root,
            int argc,
            const char* const argv[]);


T_CTEST_API const char* T_CTEST_EXPORT2
getTestName(void);



#endif
