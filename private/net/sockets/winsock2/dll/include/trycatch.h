/*++

****COPYRIGHT NOTICE*****

Module Name:

    trycatch.h

Abstract:

    This  module  provides macros to support a lexical-scope exception-handling
    mechanism.  A brief comparison between this mechanism and the C++ exception
    mechanism is as follows:

    macro exception mechamism:

        extremely low run-time overhead

        catches exceptions only in lexical scope

        no value passed to exception handler

        explicitly thrown exceptions only

        exception regions can be nested and named

        usable by older compilers

    C++ exception mechanism:

        handles  all  types  of  exceptions  including  C  exceptions implicity
        thrown.

        catches exceptions thrown in dynamic scope

        involves some setup and teardown run-time overhead

        requires an up-to-date C++ compiler version

    These  macros  are  written  and  used  in  such a fashion that they can be
    transformed back into the C++ exception mechanism if needed.

Author:

    Paul Drews (drewsxpa@ashland.intel.com) 31-October-1995

Notes:

    $Revision:   1.2  $

    $Modtime:   12 Jan 1996 15:09:02  $

Revision History:

    most-recent-revision-date email-name
        description

    31-October-1995 drewsxpa@ashland.intel.com
        created

--*/

#ifndef _TRYCATCH_
#define _TRYCATCH_


// The TRY_START macro starts a guarded region

#define TRY_START(block_label) \
    /* nothing to do */


// The  TRY_THROW  macro  is  used  inside a guarded region to exit the guarded
// region immediately and enter the exception-recovery region.

#define TRY_THROW(block_label) \
    goto catch_##block_label


// The  TRY_CATCH  macro  marks  the  end  of the guarded region and starts the
// beginning  of  the  exception-recovery  region.   If  the TRY_CATCH macro is
// encountered in normal execution, the exception-recovery region is skipped.

#define TRY_CATCH(block_label) \
    goto end_##block_label; \
    catch_##block_label:


// The TRY_END macro marks the end of the exception-recovery region.  Execution
// resumes  here after completing execution of either the guarded region or the
// exception-recovery region.

#define TRY_END(block_label) \
    end_##block_label:



// A typical usage example of these macros is as follows:
//
// char *  buf1 = NULL;
// char *  buf2 = NULL;
// BOOL    return_value;
//
// TRY_START(mem_guard) {
//     buf1 = (char *) malloc(1000);
//     if (buf1 == NULL) {
//         TRY_THROW(mem_guard);
//     }
//     buf2 = (char *) malloc(1000);
//     if (buf2 == NULL) {
//         TRY_THROW(mem_guard);
//     }
//     return_value = TRUE;
// } TRY_CATCH(mem_guard) {
//     if (buf1 != NULL) {
//         free(buf1);
//         buf1 = NULL;
//     }
//     return_value = FALSE;
// } TRY_END(mem_guard);
//
// return return_value;


#endif // _TRYCATCH_
