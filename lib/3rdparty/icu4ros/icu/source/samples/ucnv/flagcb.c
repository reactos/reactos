/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1999-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"
#include "unicode/ucnv.h"
#include "flagcb.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG_TMI 0  /* set to 1 for Too Much Information (TMI) */

U_CAPI FromUFLAGContext* U_EXPORT2  flagCB_fromU_openContext()
{
    FromUFLAGContext *ctx;

    ctx = (FromUFLAGContext*) malloc(sizeof(FromUFLAGContext));

    ctx->subCallback = NULL;
    ctx->subContext  = NULL;
    ctx->flag        = FALSE;

    return ctx;
}

U_CAPI void U_EXPORT2 flagCB_fromU(
                  const void *context,
                  UConverterFromUnicodeArgs *fromUArgs,
                  const UChar* codeUnits,
                  int32_t length,
                  UChar32 codePoint,
                  UConverterCallbackReason reason,
				  UErrorCode * err)
{
  /* First step - based on the reason code, take action */

  if(reason == UCNV_UNASSIGNED) { /* whatever set should be trapped here */
    ((FromUFLAGContext*)context)->flag = TRUE;
  }

  if(reason == UCNV_CLONE) {
      /* The following is the recommended way to implement UCNV_CLONE
         in a callback. */
      UConverterFromUCallback   saveCallback;
      const void *saveContext;
      FromUFLAGContext *old, *cloned;
      UErrorCode subErr = U_ZERO_ERROR;

#if DEBUG_TMI
      printf("*** FLAGCB: cloning %p ***\n", context);
#endif
      old = (FromUFLAGContext*)context;
      cloned = flagCB_fromU_openContext();
      
      memcpy(cloned, old, sizeof(FromUFLAGContext));

#if DEBUG_TMI
      printf("%p: my subcb=%p:%p\n", old, old->subCallback, 
             old->subContext);
      printf("%p: cloned subcb=%p:%p\n", cloned, cloned->subCallback, 
             cloned->subContext);
#endif

      /* We need to get the sub CB to handle cloning, 
       * so we have to set up the following, temporarily:
       *
       *   - Set the callback+context to the sub of this (flag) cb
       *   - preserve the current cb+context, it could be anything
       *
       *   Before:
       *      CNV  ->   FLAG ->  subcb -> ...
       *
       *   After:
       *      CNV  ->   subcb -> ...
       *
       *    The chain from 'something' on is saved, and will be restored
       *   at the end of this block.
       *
       */
      
      ucnv_setFromUCallBack(fromUArgs->converter,
                            cloned->subCallback,
                            cloned->subContext,
                            &saveCallback,
                            &saveContext,
                            &subErr);
      
      if( cloned->subCallback != NULL ) {
          /* Now, call the sub callback if present */
          cloned->subCallback(cloned->subContext, fromUArgs, codeUnits,
                              length, codePoint, reason, err);
      }
      
      ucnv_setFromUCallBack(fromUArgs->converter,
                            saveCallback,  /* Us */
                            cloned,        /* new context */
                            &cloned->subCallback,  /* IMPORTANT! Accept any change in CB or context */
                            &cloned->subContext,
                            &subErr);

      if(U_FAILURE(subErr)) {
          *err = subErr;
      }
  }

  /* process other reasons here if need be */

  /* Always call the subCallback if present */
  if(((FromUFLAGContext*)context)->subCallback != NULL &&
      reason != UCNV_CLONE) {
      ((FromUFLAGContext*)context)->subCallback(  ((FromUFLAGContext*)context)->subContext,
                                                  fromUArgs,
                                                  codeUnits,
                                                  length,
                                                  codePoint,
                                                  reason,
                                                  err);
  }

  /* cleanup - free the memory AFTER calling the sub CB */
  if(reason == UCNV_CLOSE) {
      free((void*)context);
  }
}

/* Debugging callback, just outputs what happens */

/* Test safe clone callback */

static uint32_t    debugCB_nextSerial()
{
    static uint32_t n = 1;
    
    return (n++);
}

static void debugCB_print_log(debugCBContext *q, const char *name)
{
    if(q==NULL) {
        printf("debugCBontext: %s is NULL!!\n", name);
    } else {
        if(q->magic != 0xC0FFEE) {
            fprintf(stderr, "debugCBContext: %p:%d's magic is %x, supposed to be 0xC0FFEE\n",
                    q,q->serial, q->magic);
        }
        printf("debugCBContext %p:%d=%s - magic %x\n",
                    q, q->serial, name, q->magic);
    }
}

static debugCBContext *debugCB_clone(debugCBContext *ctx)
{
    debugCBContext *newCtx;
    newCtx = malloc(sizeof(debugCBContext));
    
    newCtx->serial = debugCB_nextSerial();
    newCtx->magic = 0xC0FFEE;

    newCtx->subCallback = ctx->subCallback;
    newCtx->subContext = ctx->subContext;
    
#if DEBUG_TMI
    printf("debugCB_clone: %p:%d -> new context %p:%d\n", ctx, ctx->serial, newCtx, newCtx->serial);
#endif
    
    return newCtx;
}

void debugCB_fromU(const void *context,
                   UConverterFromUnicodeArgs *fromUArgs,
                   const UChar* codeUnits,
                   int32_t length,
                   UChar32 codePoint,
                   UConverterCallbackReason reason,
                   UErrorCode * err)
{
    debugCBContext *ctx = (debugCBContext*)context;
    /*UConverterFromUCallback junkFrom;*/
    
#if DEBUG_TMI
    printf("debugCB_fromU: Context %p:%d called, reason %d on cnv %p [err=%s]\n", ctx, ctx->serial, reason, fromUArgs->converter, u_errorName(*err));
#endif

    if(ctx->magic != 0xC0FFEE) {
        fprintf(stderr, "debugCB_fromU: Context %p:%d magic is 0x%x should be 0xC0FFEE.\n", ctx,ctx->serial, ctx->magic);
        return;
    }

    if(reason == UCNV_CLONE) {
        /* see comments in above flagCB clone code */

        UConverterFromUCallback   saveCallback;
        const void *saveContext;
        debugCBContext *cloned;
        UErrorCode subErr = U_ZERO_ERROR;

        /* "recreate" it */
#if DEBUG_TMI
        printf("debugCB_fromU: cloning..\n");
#endif
        cloned = debugCB_clone(ctx);

        if(cloned == NULL) {
            fprintf(stderr, "debugCB_fromU: internal clone failed on %p\n", ctx);
            *err = U_MEMORY_ALLOCATION_ERROR;
            return;
        }

        ucnv_setFromUCallBack(fromUArgs->converter,
                              cloned->subCallback,
                              cloned->subContext,
                              &saveCallback,
                              &saveContext,
                              &subErr);
        
        if( cloned->subCallback != NULL) {
#if DEBUG_TMI
            printf("debugCB_fromU:%p calling subCB %p\n", ctx, cloned->subCallback);
#endif
            /* call subCB if present */
            cloned->subCallback(cloned->subContext, fromUArgs, codeUnits,
                                length, codePoint, reason, err);
        } else {
            printf("debugCB_fromU:%p, NOT calling subCB, it's NULL\n", ctx);
        }

        /* set back callback */
        ucnv_setFromUCallBack(fromUArgs->converter,
                              saveCallback,  /* Us */
                              cloned,        /* new context */
                              &cloned->subCallback,  /* IMPORTANT! Accept any change in CB or context */
                              &cloned->subContext,
                              &subErr);
        
        if(U_FAILURE(subErr)) {
            *err = subErr;
        }
    }
    
    /* process other reasons here */

    /* always call subcb if present */
    if(ctx->subCallback != NULL && reason != UCNV_CLONE) {
        ctx->subCallback(ctx->subContext,
                         fromUArgs,
                         codeUnits,
                         length,
                         codePoint,
                         reason,
                         err);
    }

    if(reason == UCNV_CLOSE) {
#if DEBUG_TMI
        printf("debugCB_fromU: Context %p:%d closing\n", ctx, ctx->serial);
#endif
        free(ctx);
    }

#if DEBUG_TMI
    printf("debugCB_fromU: leaving cnv %p, ctx %p: err %s\n", fromUArgs->converter, ctx, u_errorName(*err));
#endif
}

debugCBContext *debugCB_openContext()
{
    debugCBContext *ctx;

    ctx = malloc(sizeof(debugCBContext));

    if(ctx != NULL) {
        ctx->magic = 0xC0FFEE;
        ctx->serial = debugCB_nextSerial();
        ctx->subCallback = NULL;
        ctx->subContext  = NULL;

#if DEBUG_TMI
        fprintf(stderr, "debugCB:openContext opened[%p] = serial #%d\n", ctx, ctx->serial);
#endif

    }


    return ctx;
}
