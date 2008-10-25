/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2004, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/ucnv.h"
#include "unicode/ucnv_err.h"

#include "cintltst.h"
#include "ustr_cnv.h"
void TestDefaultConverterError(void); /* keep gcc happy */


/* This makes sure that a converter isn't leaked when an error is passed to 
    u_getDefaultConverter */
void TestDefaultConverterError(void) {
    UErrorCode          err                 =   U_ZERO_ERROR;

    /* Remove the default converter */
    ucnv_close(u_getDefaultConverter(&err));

    if (U_FAILURE(err)) {
        log_err("Didn't expect a failure yet %s\n", myErrorName(err));
        return;
    }

    /* Set to any radom error state */
    err = U_FILE_ACCESS_ERROR;
    if (u_getDefaultConverter(&err) != NULL) {
        log_err("Didn't expect to get a converter on a failure\n");
    }
}



