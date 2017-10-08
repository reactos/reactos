/* This test purpose is simply to check Standard header independancy that
 * is to say that the header can be included alone without any previous
 * include.
 * Additionnaly, for C Standard headers that STLport expose, it can also be
 * used to check that files included by those headers are compatible with
 * pure C compilers.
 */

/*
 * Sometimes, if native setjmp.h was included first, the setjmp functions
 * situated in global namespace, not in vendor's std. This may confuse
 * following csetjmp
 */
#include <setjmp.h>
#include <csetjmp>
