/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        include/WinError.h
 * PURPOSE:     Miscellaneous error codes.
 * PROGRAMER:   Art Yerkes
 * UPDATE HISTORY:
 *              12/15/03 -- Created
 */

#ifndef WINERROR_H
#define WINERROR_H

#include <errors.h> /* Current locations of reactos errors */

/* Used generally */
#define DNS_ERROR_NO_MEMORY 14 /* ERROR_OUTOFMEMORY */

/* Used by DnsValidateName */
/* Returned by DnsValidateName to indicate that the name is too long or
   doesn't fit some other obvious constraint. */
#define DNS_ERROR_INVALID_NAME 123 /* ERROR_INVALID_NAME */
/* Returned by DnsValidateName to indicate a name that contains a non-special
 * and non-ascii character. */
#define DNS_ERROR_NON_RFC_NAME 9556
/* Returned by DnsValidateName to indicate that a 'special' char was used
 * in the name.  These are the punctuation " {|}~[\]^':;<=>?@!"#$%^`()+/," */
#define DNS_ERROR_INVALID_NAME_CHAR 9560
/* Returned by DnsValidateName to indicate that the name consisted entirely
 * of digits, and so collides with the network numbers. */
#define DNS_ERROR_NUMERIC_NAME 9561

#endif//WINERROR_H
