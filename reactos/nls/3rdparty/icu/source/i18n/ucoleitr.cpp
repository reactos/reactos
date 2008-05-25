/*
******************************************************************************
*   Copyright (C) 2001-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
******************************************************************************
*
* File ucoleitr.cpp
*
* Modification History:
*
* Date        Name        Description
* 02/15/2001  synwee      Modified all methods to process its own function 
*                         instead of calling the equivalent c++ api (coleitr.h)
******************************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucoleitr.h"
#include "unicode/ustring.h"
#include "unicode/sortkey.h"
#include "ucol_imp.h"
#include "cmemory.h"

U_NAMESPACE_USE

#define BUFFER_LENGTH             100

typedef struct collIterate collIterator;

/* public methods ---------------------------------------------------- */

U_CAPI UCollationElements* U_EXPORT2
ucol_openElements(const UCollator  *coll,
                  const UChar      *text,
                        int32_t    textLength,
                        UErrorCode *status)
{
  UCollationElements *result;

  if (U_FAILURE(*status)) {
    return NULL;
  }

  result = (UCollationElements *)uprv_malloc(sizeof(UCollationElements));
  /* test for NULL */
  if (result == NULL) {
      *status = U_MEMORY_ALLOCATION_ERROR;
      return NULL;
  }

  result->reset_   = TRUE;
  result->isWritable = FALSE;

  if (text == NULL) {
      textLength = 0;
  }
  uprv_init_collIterate(coll, text, textLength, &result->iteratordata_);

  return result;
}

U_CAPI void U_EXPORT2
ucol_closeElements(UCollationElements *elems)
{
  collIterate *ci = &elems->iteratordata_;
  if (ci->writableBuffer != ci->stackWritableBuffer) {
    uprv_free(ci->writableBuffer);
  }
  if (elems->isWritable && elems->iteratordata_.string != NULL)
  {
    uprv_free(elems->iteratordata_.string);
  }
  uprv_free(elems);
}

U_CAPI void U_EXPORT2
ucol_reset(UCollationElements *elems)
{
  collIterate *ci = &(elems->iteratordata_);
  elems->reset_   = TRUE;
  ci->pos         = ci->string;
  if ((ci->flags & UCOL_ITER_HASLEN) == 0 || ci->endp == NULL) {
    ci->endp      = ci->string + u_strlen(ci->string);
  }
  ci->CEpos       = ci->toReturn = ci->CEs;
  ci->flags       = UCOL_ITER_HASLEN;
  if (ci->coll->normalizationMode == UCOL_ON) {
    ci->flags |= UCOL_ITER_NORM;
  }
  
  if (ci->stackWritableBuffer != ci->writableBuffer) {
    uprv_free(ci->writableBuffer);
    ci->writableBuffer = ci->stackWritableBuffer;
    ci->writableBufSize = UCOL_WRITABLE_BUFFER_SIZE;
  }
  ci->fcdPosition = NULL;
}

U_CAPI int32_t U_EXPORT2
ucol_next(UCollationElements *elems, 
          UErrorCode         *status)
{
  int32_t result;
  if (U_FAILURE(*status)) {
    return UCOL_NULLORDER;
  }

  elems->reset_ = FALSE;

  result = (int32_t)ucol_getNextCE(elems->iteratordata_.coll,
                                   &elems->iteratordata_, 
                                   status);
  
  if (result == UCOL_NO_MORE_CES) {
    result = UCOL_NULLORDER;
  }
  return result;
}

U_CAPI int32_t U_EXPORT2
ucol_previous(UCollationElements *elems,
              UErrorCode         *status)
{
  if(U_FAILURE(*status)) {
    return UCOL_NULLORDER;
  }
  else
  {
    int32_t result;

    if (elems->reset_ && 
        (elems->iteratordata_.pos == elems->iteratordata_.string)) {
        if (elems->iteratordata_.endp == NULL) {
            elems->iteratordata_.endp = elems->iteratordata_.string + 
                                        u_strlen(elems->iteratordata_.string);
            elems->iteratordata_.flags |= UCOL_ITER_HASLEN;
        }
        elems->iteratordata_.pos = elems->iteratordata_.endp;
        elems->iteratordata_.fcdPosition = elems->iteratordata_.endp;
    }

    elems->reset_ = FALSE;

    result = (int32_t)ucol_getPrevCE(elems->iteratordata_.coll,
                                     &(elems->iteratordata_), 
                                     status);

    if (result == UCOL_NO_MORE_CES) {
      result = UCOL_NULLORDER;
    }

    return result;
  }
}

U_CAPI int32_t U_EXPORT2
ucol_getMaxExpansion(const UCollationElements *elems,
                           int32_t            order)
{
  uint8_t result;
  UCOL_GETMAXEXPANSION(elems->iteratordata_.coll, (uint32_t)order, result);
  return result;
}
 
U_CAPI void U_EXPORT2
ucol_setText(      UCollationElements *elems,
             const UChar              *text,
                   int32_t            textLength,
                   UErrorCode         *status)
{
  if (U_FAILURE(*status)) {
    return;
  }

  if (elems->isWritable && elems->iteratordata_.string != NULL)
  {
    uprv_free(elems->iteratordata_.string);
  }
 
  if (text == NULL) {
      textLength = 0;
  }

  elems->isWritable = FALSE;
  uprv_init_collIterate(elems->iteratordata_.coll, text, textLength, 
                   &elems->iteratordata_);

  elems->reset_   = TRUE;
}

U_CAPI int32_t U_EXPORT2
ucol_getOffset(const UCollationElements *elems)
{
  const collIterate *ci = &(elems->iteratordata_);
  // while processing characters in normalization buffer getOffset will 
  // return the next non-normalized character. 
  // should be inline with the old implementation since the old codes uses
  // nextDecomp in normalizer which also decomposes the string till the 
  // first base character is found.
  if (ci->flags & UCOL_ITER_INNORMBUF) {
      if (ci->fcdPosition == NULL) {
        return 0;
      }
      return (int32_t)(ci->fcdPosition - ci->string);
  }
  else {
      return (int32_t)(ci->pos - ci->string);
  }
}

U_CAPI void U_EXPORT2
ucol_setOffset(UCollationElements    *elems,
               int32_t           offset,
               UErrorCode            *status)
{
  if (U_FAILURE(*status)) {
    return;
  }

  // this methods will clean up any use of the writable buffer and points to 
  // the original string
  collIterate *ci = &(elems->iteratordata_);
  ci->pos         = ci->string + offset;
  ci->CEpos       = ci->toReturn = ci->CEs;
  if (ci->flags & UCOL_ITER_INNORMBUF) {
    ci->flags = ci->origFlags;
  }
  if ((ci->flags & UCOL_ITER_HASLEN) == 0) {
      ci->endp  = ci->string + u_strlen(ci->string);
      ci->flags |= UCOL_ITER_HASLEN;
  }
  ci->fcdPosition = NULL;
  elems->reset_ = FALSE;
}

U_CAPI int32_t U_EXPORT2
ucol_primaryOrder (int32_t order) 
{
  order &= UCOL_PRIMARYMASK;
  return (order >> UCOL_PRIMARYORDERSHIFT);
}

U_CAPI int32_t U_EXPORT2
ucol_secondaryOrder (int32_t order) 
{
  order &= UCOL_SECONDARYMASK;
  return (order >> UCOL_SECONDARYORDERSHIFT);
}

U_CAPI int32_t U_EXPORT2
ucol_tertiaryOrder (int32_t order) 
{
  return (order & UCOL_TERTIARYMASK);
}

#endif /* #if !UCONFIG_NO_COLLATION */
