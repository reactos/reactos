/*  
**********************************************************************
*   Copyright (C) 2000-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  ucnvhz.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000oct16
*   created by: Ram Viswanadha
*   10/31/2000  Ram     Implemented offsets logic function
*   
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION && !UCONFIG_NO_LEGACY_CONVERSION

#include "cmemory.h"
#include "unicode/ucnv.h"
#include "unicode/ucnv_cb.h"
#include "unicode/uset.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"

#define UCNV_TILDE 0x7E          /* ~ */
#define UCNV_OPEN_BRACE 0x7B     /* { */
#define UCNV_CLOSE_BRACE 0x7D   /* } */
#define SB_ESCAPE    "\x7E\x7D"
#define DB_ESCAPE    "\x7E\x7B"
#define TILDE_ESCAPE "\x7E\x7E"
#define ESC_LEN       2


#define CONCAT_ESCAPE_MACRO( args, targetIndex,targetLength,strToAppend, err, len,sourceIndex){                             \
    while(len-->0){                                                                                                         \
        if(targetIndex < targetLength){                                                                                     \
            args->target[targetIndex] = (unsigned char) *strToAppend;                                                       \
            if(args->offsets!=NULL){                                                                                        \
                *(offsets++) = sourceIndex-1;                                                                               \
            }                                                                                                               \
            targetIndex++;                                                                                                  \
        }                                                                                                                   \
        else{                                                                                                               \
            args->converter->charErrorBuffer[(int)args->converter->charErrorBufferLength++] = (unsigned char) *strToAppend; \
            *err =U_BUFFER_OVERFLOW_ERROR;                                                                                  \
        }                                                                                                                   \
        strToAppend++;                                                                                                      \
    }                                                                                                                       \
}


typedef struct{
    UConverter* gbConverter;
    int32_t targetIndex;
    int32_t sourceIndex;
    UBool isEscapeAppended;
    UBool isStateDBCS;
    UBool isTargetUCharDBCS;
}UConverterDataHZ;



static void 
_HZOpen(UConverter *cnv, const char *name,const char *locale,uint32_t options, UErrorCode *errorCode){
    cnv->toUnicodeStatus = 0;
    cnv->fromUnicodeStatus= 0;
    cnv->mode=0;
    cnv->fromUChar32=0x0000;
    cnv->extraInfo = uprv_malloc(sizeof(UConverterDataHZ));
    if(cnv->extraInfo != NULL){
        uprv_memset(cnv->extraInfo, 0, sizeof(UConverterDataHZ));
        ((UConverterDataHZ*)cnv->extraInfo)->gbConverter = ucnv_open("ibm-1386",errorCode);
    }
    else {
        *errorCode = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
}

static void 
_HZClose(UConverter *cnv){
    if(cnv->extraInfo != NULL) {
        ucnv_close (((UConverterDataHZ *) (cnv->extraInfo))->gbConverter);
        if(!cnv->isExtraLocal) {
            uprv_free(cnv->extraInfo);
        }
        cnv->extraInfo = NULL;
    }
}

static void 
_HZReset(UConverter *cnv, UConverterResetChoice choice){
    if(choice<=UCNV_RESET_TO_UNICODE) {
        cnv->toUnicodeStatus = 0;
        cnv->mode=0;
        if(cnv->extraInfo != NULL){
            ((UConverterDataHZ*)cnv->extraInfo)->isStateDBCS = FALSE;
        }
    }
    if(choice!=UCNV_RESET_TO_UNICODE) {
        cnv->fromUnicodeStatus= 0;
        cnv->fromUChar32=0x0000; 
        if(cnv->extraInfo != NULL){
            ((UConverterDataHZ*)cnv->extraInfo)->isEscapeAppended = FALSE;
            ((UConverterDataHZ*)cnv->extraInfo)->targetIndex = 0;
            ((UConverterDataHZ*)cnv->extraInfo)->sourceIndex = 0;
            ((UConverterDataHZ*)cnv->extraInfo)->isTargetUCharDBCS = FALSE;
        }
    }
}

/**************************************HZ Encoding*************************************************
* Rules for HZ encoding
* 
*   In ASCII mode, a byte is interpreted as an ASCII character, unless a
*   '~' is encountered. The character '~' is an escape character. By
*   convention, it must be immediately followed ONLY by '~', '{' or '\n'
*   (<LF>), with the following special meaning.

*   1. The escape sequence '~~' is interpreted as a '~'.
*   2. The escape-to-GB sequence '~{' switches the mode from ASCII to GB.
*   3. The escape sequence '~\n' is a line-continuation marker to be
*     consumed with no output produced.
*   In GB mode, characters are interpreted two bytes at a time as (pure)
*   GB codes until the escape-from-GB code '~}' is read. This code
*   switches the mode from GB back to ASCII.  (Note that the escape-
*   from-GB code '~}' ($7E7D) is outside the defined GB range.)
*
*   Source: RFC 1842
*/


static void 
UConverter_toUnicode_HZ_OFFSETS_LOGIC(UConverterToUnicodeArgs *args,
                                                            UErrorCode* err){
    char tempBuf[2];
    const char *mySource = ( char *) args->source;
    UChar *myTarget = args->target;
    const char *mySourceLimit = args->sourceLimit;
    UChar32 targetUniChar = 0x0000;
    UChar mySourceChar = 0x0000;
    UConverterDataHZ* myData=(UConverterDataHZ*)(args->converter->extraInfo);
    tempBuf[0]=0; 
    tempBuf[1]=0;
    if ((args->converter == NULL) || (args->targetLimit < args->target) || (mySourceLimit < args->source)){
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    while(mySource< mySourceLimit){
        
        if(myTarget < args->targetLimit){
            
            mySourceChar= (unsigned char) *mySource++;

            switch(mySourceChar){
                case 0x0A:
                    if(args->converter->mode ==UCNV_TILDE){
                        args->converter->mode=0;
                        
                    }
                    *(myTarget++)=(UChar)mySourceChar;
                    continue;
            
                case UCNV_TILDE:
                    if(args->converter->mode ==UCNV_TILDE){
                        *(myTarget++)=(UChar)mySourceChar;
                        args->converter->mode=0;
                        continue;
                        
                    }
                    else if(args->converter->toUnicodeStatus !=0){
                        args->converter->mode=0;
                        break;
                    }
                    else{
                        args->converter->mode = UCNV_TILDE;
                        continue;
                    }
                
                
                case UCNV_OPEN_BRACE:
                    if(args->converter->mode == UCNV_TILDE){
                        args->converter->mode=0;
                        myData->isStateDBCS = TRUE;
                        continue;
                    }
                    else{
                        break;
                    }
               
                
                case UCNV_CLOSE_BRACE:
                    if(args->converter->mode == UCNV_TILDE){
                        args->converter->mode=0;
                         myData->isStateDBCS = FALSE;
                        continue;
                    }
                    else{
                        break;
                    }
                
                default:
                     /* if the first byte is equal to TILDE and the trail byte
                     * is not a valid byte then it is an error condition
                     */
                    if(args->converter->mode == UCNV_TILDE){
                        args->converter->mode=0;
                        mySourceChar= (UChar)(((UCNV_TILDE+0x80) << 8) | ((mySourceChar & 0x00ff)+0x80));
                        goto SAVE_STATE;
                    }
                    
                    break;

            }
             
            if(myData->isStateDBCS){
                if(args->converter->toUnicodeStatus == 0x00){
                    args->converter->toUnicodeStatus = (UChar) mySourceChar;
                    continue;
                }
                else{
                    tempBuf[0] = (char) (args->converter->toUnicodeStatus+0x80) ;
                    tempBuf[1] = (char) (mySourceChar+0x80);
                    mySourceChar= (UChar)(((args->converter->toUnicodeStatus+0x80) << 8) | ((mySourceChar & 0x00ff)+0x80));
                    args->converter->toUnicodeStatus =0x00;
                    targetUniChar = ucnv_MBCSSimpleGetNextUChar(myData->gbConverter->sharedData,
                        tempBuf, 2, args->converter->useFallback);
                }
            }
            else{
                if(args->converter->fromUnicodeStatus == 0x00){
                    targetUniChar = ucnv_MBCSSimpleGetNextUChar(myData->gbConverter->sharedData,
                        mySource - 1, 1, args->converter->useFallback);
                }
                else{
                    goto SAVE_STATE;
                }

            }
            if(targetUniChar < 0xfffe){
                if(args->offsets) {
                    args->offsets[myTarget - args->target]=(int32_t)(mySource - args->source - 1-(myData->isStateDBCS));
                }

                *(myTarget++)=(UChar)targetUniChar;
            }
            else if(targetUniChar>=0xfffe){
SAVE_STATE:
                if(targetUniChar == 0xfffe){
                    *err = U_INVALID_CHAR_FOUND;
                }
                else{
                    *err = U_ILLEGAL_CHAR_FOUND;
                }
                if(myData->isStateDBCS){
                    /* this should never occur since isStateDBCS is set to true 
                     * only after tempBuf[0] and tempBuf[1]
                     * are set to the input ..  just to please BEAM 
                     */
                    if(tempBuf[0]==0 || tempBuf[1]==0){
                        *err = U_INTERNAL_PROGRAM_ERROR;
                    }else{
                        args->converter->toUBytes[0] = (uint8_t)(tempBuf[0]-0x80);
                        args->converter->toUBytes[1] = (uint8_t)(tempBuf[1]-0x80);
                        args->converter->toULength=2;
                    }
                }
                else{
                    args->converter->toUBytes[0] = (uint8_t)mySourceChar;
                    args->converter->toULength=1;
                }
                break;
            }
        }
        else{
            *err =U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }

    args->target = myTarget;
    args->source = mySource;
}


static void 
UConverter_fromUnicode_HZ_OFFSETS_LOGIC (UConverterFromUnicodeArgs * args,
                                                      UErrorCode * err){
    const UChar *mySource = args->source;
    char *myTarget = args->target;
    int32_t* offsets = args->offsets;
    int32_t mySourceIndex = 0;
    int32_t myTargetIndex = 0;
    int32_t targetLength = (int32_t)(args->targetLimit - myTarget);
    int32_t mySourceLength = (int32_t)(args->sourceLimit - args->source);
    int32_t length=0;
    uint32_t targetUniChar = 0x0000;
    UChar32 mySourceChar = 0x0000;
    UConverterDataHZ *myConverterData=(UConverterDataHZ*)args->converter->extraInfo;
    UBool isTargetUCharDBCS = (UBool) myConverterData->isTargetUCharDBCS;
    UBool oldIsTargetUCharDBCS = isTargetUCharDBCS;
    int len =0;
    const char* escSeq=NULL;
    
    if ((args->converter == NULL) || (args->targetLimit < myTarget) || (args->sourceLimit < args->source)){
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if(args->converter->fromUChar32!=0 && myTargetIndex < targetLength) {
        goto getTrail;
    }
    /*writing the char to the output stream */
    while (mySourceIndex < mySourceLength){
        targetUniChar = missingCharMarker;
        if (myTargetIndex < targetLength){
            
            mySourceChar = (UChar) mySource[mySourceIndex++];
            

            oldIsTargetUCharDBCS = isTargetUCharDBCS;
            if(mySourceChar ==UCNV_TILDE){
                /*concatEscape(args, &myTargetIndex, &targetLength,"\x7E\x7E",err,2,&mySourceIndex);*/
                len = ESC_LEN;
                escSeq = TILDE_ESCAPE;
                CONCAT_ESCAPE_MACRO(args, myTargetIndex, targetLength, escSeq,err,len,mySourceIndex);
                continue;
            }
            else{
                length= ucnv_MBCSFromUChar32(myConverterData->gbConverter->sharedData,
                    mySourceChar,&targetUniChar,args->converter->useFallback);

            }
            /* only DBCS or SBCS characters are expected*/
            /* DB haracters with high bit set to 1 are expected */
            if(length > 2 || length==0 ||(((targetUniChar & 0x8080) != 0x8080)&& length==2)){
                targetUniChar= missingCharMarker;
            }
            if (targetUniChar != missingCharMarker){
               myConverterData->isTargetUCharDBCS = isTargetUCharDBCS = (UBool)(targetUniChar>0x00FF);     
                 if(oldIsTargetUCharDBCS != isTargetUCharDBCS || !myConverterData->isEscapeAppended ){
                    /*Shifting from a double byte to single byte mode*/
                    if(!isTargetUCharDBCS){
                        len =ESC_LEN;
                        escSeq = SB_ESCAPE;
                        CONCAT_ESCAPE_MACRO(args, myTargetIndex, targetLength, escSeq,err,len,mySourceIndex);
                        myConverterData->isEscapeAppended = TRUE;
                    }
                    else{ /* Shifting from a single byte to double byte mode*/
                        len =ESC_LEN;
                        escSeq = DB_ESCAPE;
                        CONCAT_ESCAPE_MACRO(args, myTargetIndex, targetLength, escSeq,err,len,mySourceIndex);
                        myConverterData->isEscapeAppended = TRUE;
                        
                    }
                }
            
                if(isTargetUCharDBCS){
                    if( myTargetIndex <targetLength){
                        myTarget[myTargetIndex++] =(char) ((targetUniChar >> 8) -0x80);
                        if(offsets){
                            *(offsets++) = mySourceIndex-1;
                        }
                        if(myTargetIndex < targetLength){
                            myTarget[myTargetIndex++] =(char) ((targetUniChar & 0x00FF) -0x80);
                            if(offsets){
                                *(offsets++) = mySourceIndex-1;
                            }
                        }else{
                            args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = (char) ((targetUniChar & 0x00FF) -0x80);
                            *err = U_BUFFER_OVERFLOW_ERROR;
                        } 
                    }else{
                        args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] =(char) ((targetUniChar >> 8) -0x80);
                        args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = (char) ((targetUniChar & 0x00FF) -0x80);
                        *err = U_BUFFER_OVERFLOW_ERROR;
                    }

                }else{
                    if( myTargetIndex <targetLength){
                        myTarget[myTargetIndex++] = (char) (targetUniChar );
                        if(offsets){
                            *(offsets++) = mySourceIndex-1;
                        }
                        
                    }else{
                        args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = (char) targetUniChar;
                        *err = U_BUFFER_OVERFLOW_ERROR;
                    }
                }

            }
            else{
                /* oops.. the code point is unassigned */
                /*Handle surrogates */
                /*check if the char is a First surrogate*/
                if(UTF_IS_SURROGATE(mySourceChar)) {
                    if(UTF_IS_SURROGATE_FIRST(mySourceChar)) {
                        args->converter->fromUChar32=mySourceChar;
getTrail:
                        /*look ahead to find the trail surrogate*/
                        if(mySourceIndex <  mySourceLength) {
                            /* test the following code unit */
                            UChar trail=(UChar) args->source[mySourceIndex];
                            if(UTF_IS_SECOND_SURROGATE(trail)) {
                                ++mySourceIndex;
                                mySourceChar=UTF16_GET_PAIR_VALUE(args->converter->fromUChar32, trail);
                                args->converter->fromUChar32=0x00;
                                /* there are no surrogates in GB2312*/
                                *err = U_INVALID_CHAR_FOUND;
                                /* exit this condition tree */
                            } else {
                                /* this is an unmatched lead code unit (1st surrogate) */
                                /* callback(illegal) */
                                *err=U_ILLEGAL_CHAR_FOUND;
                            }
                        } else {
                            /* no more input */
                            *err = U_ZERO_ERROR;
                        }
                    } else {
                        /* this is an unmatched trail code unit (2nd surrogate) */
                        /* callback(illegal) */
                        *err=U_ILLEGAL_CHAR_FOUND;
                    }
                } else {
                    /* callback(unassigned) for a BMP code point */
                    *err = U_INVALID_CHAR_FOUND;
                }

                args->converter->fromUChar32=mySourceChar;
                break;
            }
        }
        else{
            *err = U_BUFFER_OVERFLOW_ERROR;
            break;
        }
        targetUniChar=missingCharMarker;
    }

    args->target += myTargetIndex;
    args->source += mySourceIndex;
    myConverterData->isTargetUCharDBCS = isTargetUCharDBCS;
}

static void
_HZ_WriteSub(UConverterFromUnicodeArgs *args, int32_t offsetIndex, UErrorCode *err) {
    UConverter *cnv = args->converter;
    UConverterDataHZ *convData=(UConverterDataHZ *) cnv->extraInfo;
    char *p;
    char buffer[4];
    p = buffer;
    
    if( convData->isTargetUCharDBCS){
        *p++= UCNV_TILDE;
        *p++= UCNV_CLOSE_BRACE;
        convData->isTargetUCharDBCS=FALSE;
    }
    *p++= (char)cnv->subChars[0];

    ucnv_cbFromUWriteBytes(args,
                           buffer, (int32_t)(p - buffer),
                           offsetIndex, err);
}

/*
 * Structure for cloning an HZ converter into a single memory block.
 * ucnv_safeClone() of the HZ converter will align the entire cloneHZStruct,
 * and then ucnv_safeClone() of the sub-converter may additionally align
 * subCnv inside the cloneHZStruct, for which we need the deadSpace after
 * subCnv. This is because UAlignedMemory may be larger than the actually
 * necessary alignment size for the platform.
 * The other cloneHZStruct fields will not be moved around,
 * and are aligned properly with cloneHZStruct's alignment.
 */
struct cloneHZStruct
{
    UConverter cnv;
    UConverter subCnv;
    UAlignedMemory deadSpace;
    UConverterDataHZ mydata;
};


static UConverter * 
_HZ_SafeClone(const UConverter *cnv, 
              void *stackBuffer, 
              int32_t *pBufferSize, 
              UErrorCode *status)
{
    struct cloneHZStruct * localClone;
    int32_t size, bufferSizeNeeded = sizeof(struct cloneHZStruct);

    if (U_FAILURE(*status)){
        return 0;
    }

    if (*pBufferSize == 0){ /* 'preflighting' request - set needed size into *pBufferSize */
        *pBufferSize = bufferSizeNeeded;
        return 0;
    }

    localClone = (struct cloneHZStruct *)stackBuffer;
    /* ucnv.c/ucnv_safeClone() copied the main UConverter already */

    uprv_memcpy(&localClone->mydata, cnv->extraInfo, sizeof(UConverterDataHZ));
    localClone->cnv.extraInfo = &localClone->mydata;
    localClone->cnv.isExtraLocal = TRUE;

    /* deep-clone the sub-converter */
    size = (int32_t)(sizeof(UConverter) + sizeof(UAlignedMemory)); /* include size of padding */
    ((UConverterDataHZ*)localClone->cnv.extraInfo)->gbConverter =
        ucnv_safeClone(((UConverterDataHZ*)cnv->extraInfo)->gbConverter, &localClone->subCnv, &size, status);

    return &localClone->cnv;
}

static void
_HZ_GetUnicodeSet(const UConverter *cnv,
                  const USetAdder *sa,
                  UConverterUnicodeSet which,
                  UErrorCode *pErrorCode) {
    /* the tilde '~' is hardcoded in the converter */
    sa->add(sa->set, 0x7e);

    /* add all of the code points that the sub-converter handles */
    ((UConverterDataHZ*)cnv->extraInfo)->
        gbConverter->sharedData->impl->
            getUnicodeSet(((UConverterDataHZ*)cnv->extraInfo)->gbConverter,
                          sa, which, pErrorCode);
}

static const UConverterImpl _HZImpl={

    UCNV_HZ,
    
    NULL,
    NULL,
    
    _HZOpen,
    _HZClose,
    _HZReset,
    
    UConverter_toUnicode_HZ_OFFSETS_LOGIC,
    UConverter_toUnicode_HZ_OFFSETS_LOGIC,
    UConverter_fromUnicode_HZ_OFFSETS_LOGIC,
    UConverter_fromUnicode_HZ_OFFSETS_LOGIC,
    NULL,
    
    NULL,
    NULL,
    _HZ_WriteSub,
    _HZ_SafeClone,
    _HZ_GetUnicodeSet
};

static const UConverterStaticData _HZStaticData={
    sizeof(UConverterStaticData),
        "HZ",
         0, 
         UCNV_IBM, 
         UCNV_HZ, 
         1, 
         4,
        { 0x1a, 0, 0, 0 },
        1,
        FALSE, 
        FALSE,
        0,
        0,
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, /* reserved */

};
            
            
const UConverterSharedData _HZData={
    sizeof(UConverterSharedData),
        ~((uint32_t) 0),
        NULL, 
        NULL, 
        &_HZStaticData, 
        FALSE, 
        &_HZImpl, 
        0
};

#endif /* #if !UCONFIG_NO_LEGACY_CONVERSION */
