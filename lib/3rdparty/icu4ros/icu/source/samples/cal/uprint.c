/*
**********************************************************************
* Copyright (C) 1998-2001, International Business Machines Corporation
* and others.  All Rights Reserved.
**********************************************************************
*
* File date.c
*
* Modification History:
*
*   Date        Name        Description
*   06/14/99    stephen     Creation.
*******************************************************************************
*/

#include "uprint.h"
#include "unicode/ucnv.h"
#include "unicode/ustring.h"

#define BUF_SIZE 128

/* Print a ustring to the specified FILE* in the default codepage */
void
uprint(const UChar *s,
       FILE *f,
       UErrorCode *status)
{
  /* converter */
  UConverter *converter;
  char buf [BUF_SIZE];
  int32_t sourceLen;
  const UChar *mySource;
  const UChar *mySourceEnd;
  char *myTarget;
  int32_t arraySize;

  if(s == 0) return;

  /* set up the conversion parameters */
  sourceLen    = u_strlen(s);
  mySource     = s;
  mySourceEnd  = mySource + sourceLen;
  myTarget     = buf;
  arraySize    = BUF_SIZE;

  /* open a default converter */
  converter = ucnv_open(0, status);
  
  /* if we failed, clean up and exit */
  if(U_FAILURE(*status)) goto finish;
  
  /* perform the conversion */
  do {
    /* reset the error code */
    *status = U_ZERO_ERROR;

    /* perform the conversion */
    ucnv_fromUnicode(converter, &myTarget, myTarget + arraySize,
             &mySource, mySourceEnd, NULL,
             TRUE, status);

    /* Write the converted data to the FILE* */
    fwrite(buf, sizeof(char), myTarget - buf, f);

    /* update the conversion parameters*/
    myTarget     = buf;
    arraySize    = BUF_SIZE;
  }
  while(*status == U_BUFFER_OVERFLOW_ERROR); 

 finish:
  
  /* close the converter */
  ucnv_close(converter);
}
