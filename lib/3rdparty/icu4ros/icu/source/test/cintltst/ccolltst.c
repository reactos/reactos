/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2006, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CCOLLTST.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda               Creation
*********************************************************************************
*/
#include <stdio.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "cintltst.h"
#include "ccolltst.h"
#include "unicode/ucol.h"
#include "unicode/ustring.h"
#include "cmemory.h"

void addCollTest(TestNode** root);

void addCollTest(TestNode** root)
{
    addCollAPITest(root);
    addCurrencyCollTest(root);
    addNormTest(root);
    addGermanCollTest(root);
    addSpanishCollTest(root);
    addFrenchCollTest(root);
    addKannaCollTest(root);
    addTurkishCollTest(root);
    addEnglishCollTest(root);
    addFinnishCollTest(root);
    
    /* WEIVTODO: return tests here */
    addRuleBasedCollTest(root);
    addCollIterTest(root);
    addAllCollTest(root);
    addMiscCollTest(root);

    addSearchTest(root);
}



/*Internal functions used*/
static char* dumpSk(uint8_t *sourceKey, char *sk) {
    uint32_t kLen = (uint32_t)strlen((const char *)sourceKey);
    uint32_t i = 0;
    
    *sk = 0;
    
    for(i = 0; i<kLen; i++) {
        sprintf(sk+2*i, "%02X", sourceKey[i]);
    }
    return sk;
}

void reportCResult( const UChar source[], const UChar target[], 
                         uint8_t *sourceKey, uint8_t *targetKey,
                         UCollationResult compareResult,
                         UCollationResult keyResult,
                         UCollationResult incResult,
                         UCollationResult expectedResult )
{
    UChar *sResult, *sExpect;
    sResult=(UChar*)malloc(sizeof(UChar) * 10);
    sExpect=(UChar*)malloc(sizeof(UChar) * 10);
    if (expectedResult < -1 || expectedResult > 1)
    {
        log_err("***** invalid call to reportCResult ****\n");
        return;
    }

    if (compareResult != expectedResult)
    {
        
        appendCompareResult(compareResult, sResult);
        appendCompareResult(expectedResult, sExpect);
        log_err("Compare(%s , %s) returned: %s expected: %s\n", aescstrdup(source,-1), aescstrdup(target,-1),
            austrdup(sResult), austrdup(sExpect) );
    }

    if (incResult != expectedResult)
    {
        
        appendCompareResult(incResult, sResult);
        appendCompareResult(expectedResult, sExpect);
        log_err("incCompare(%s , %s) returned: %s expected: %s\n", aescstrdup(source,-1), aescstrdup(target,-1),
            austrdup(sResult), austrdup(sExpect) );
    }

    if (keyResult != expectedResult)
    {
    
        appendCompareResult(keyResult, sResult);
        appendCompareResult(expectedResult, sExpect);

        log_err("KeyCompare(%s , %s) returned: %s expected: %s\n", aescstrdup(source,-1), aescstrdup(target,-1), 
            austrdup(sResult), austrdup(sExpect) );

    
    }

    if (keyResult != compareResult)
    {
    
        appendCompareResult(keyResult, sResult);
        appendCompareResult(compareResult, sExpect);

        log_err("difference between sortkey and compare result for (%s , %s) Keys: %s compare %s\n", aescstrdup(source,-1), aescstrdup(target,-1), 
            austrdup(sResult), austrdup(sExpect) );

    
    }

    if(keyResult != expectedResult || keyResult != compareResult)
    {
      char sk[10000];
      log_verbose("SortKey1: %s\n", dumpSk(sourceKey, sk));
      log_verbose("SortKey2: %s\n", dumpSk(targetKey, sk));
    }

    free(sExpect);
    free(sResult);
}

UChar* appendCompareResult(UCollationResult result, UChar* target)
{
    if (result == UCOL_LESS)
    {
        u_uastrcpy(target, "LESS");
    }
    else if (result == UCOL_EQUAL)
    {
        u_uastrcpy(target, "EQUAL");
    }
    else if (result == UCOL_GREATER)
    {
        u_uastrcpy(target, "GREATER");
    }
    else
    {
        u_uastrcpy(target, "huh???");
    }

    return target;
}

#endif /* #if !UCONFIG_NO_COLLATION */
