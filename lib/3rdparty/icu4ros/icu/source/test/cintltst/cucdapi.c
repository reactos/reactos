/********************************************************************
 * Copyright (c) 1997-2007, International Business Machines
 * Corporation and others. All Rights Reserved.
 ********************************************************************/

#include <string.h>
#include "unicode/utypes.h"
#include "unicode/uscript.h"
#include "unicode/uchar.h"
#include "cintltst.h"
#include "cucdapi.h"

#define ARRAY_SIZE(array) (int32_t)(sizeof array  / sizeof array[0])

void TestUScriptCodeAPI(){
    int i =0;
    int numErrors =0;
    {
        const char* testNames[]={
        /* test locale */
        "en", "en_US", "sr", "ta" , "te_IN",
        "hi", "he", "ar",
        /* test abbr */
        "Hani", "Hang","Hebr","Hira",
        "Knda","Kana","Khmr","Lao",
        "Latn",/*"Latf","Latg",*/ 
        "Mlym", "Mong",
    
        /* test names */
        "CYRILLIC","DESERET","DEVANAGARI","ETHIOPIC","GEORGIAN", 
        "GOTHIC",  "GREEK",  "GUJARATI", "COMMON", "INHERITED", 
        /* test lower case names */
        "malayalam", "mongolian", "myanmar", "ogham", "old-italic",
        "oriya",     "runic",     "sinhala", "syriac","tamil",     
        "telugu",    "thaana",    "thai",    "tibetan", 
        /* test the bounds*/
        "tagb", "arabic",
        /* test bogus */
        "asfdasd", "5464", "12235",
        /* test the last index */
        "zyyy", "YI",
        '\0'  
        };
        UScriptCode expected[] ={
            /* locales should return */
            USCRIPT_LATIN, USCRIPT_LATIN, USCRIPT_CYRILLIC, USCRIPT_TAMIL, USCRIPT_TELUGU, 
            USCRIPT_DEVANAGARI, USCRIPT_HEBREW, USCRIPT_ARABIC,
            /* abbr should return */
            USCRIPT_HAN, USCRIPT_HANGUL, USCRIPT_HEBREW, USCRIPT_HIRAGANA,
            USCRIPT_KANNADA, USCRIPT_KATAKANA, USCRIPT_KHMER, USCRIPT_LAO,
            USCRIPT_LATIN,/* USCRIPT_LATIN, USCRIPT_LATIN,*/ 
            USCRIPT_MALAYALAM, USCRIPT_MONGOLIAN,
            /* names should return */
            USCRIPT_CYRILLIC, USCRIPT_DESERET, USCRIPT_DEVANAGARI, USCRIPT_ETHIOPIC, USCRIPT_GEORGIAN,
            USCRIPT_GOTHIC, USCRIPT_GREEK, USCRIPT_GUJARATI, USCRIPT_COMMON, USCRIPT_INHERITED,
            /* lower case names should return */    
            USCRIPT_MALAYALAM, USCRIPT_MONGOLIAN, USCRIPT_MYANMAR, USCRIPT_OGHAM, USCRIPT_OLD_ITALIC,
            USCRIPT_ORIYA, USCRIPT_RUNIC, USCRIPT_SINHALA, USCRIPT_SYRIAC, USCRIPT_TAMIL,
            USCRIPT_TELUGU, USCRIPT_THAANA, USCRIPT_THAI, USCRIPT_TIBETAN,
            /* bounds */
            USCRIPT_TAGBANWA, USCRIPT_ARABIC,
            /* bogus names should return invalid code */
            USCRIPT_INVALID_CODE, USCRIPT_INVALID_CODE, USCRIPT_INVALID_CODE,
            USCRIPT_COMMON, USCRIPT_YI,
        };

        UErrorCode err = U_ZERO_ERROR;

        const int32_t capacity = 10;

        for( ; testNames[i]!='\0'; i++){
            UScriptCode script[10]={USCRIPT_INVALID_CODE};
            uscript_getCode(testNames[i],script,capacity, &err);
            if( script[0] != expected[i]){
                   log_err("Error getting script code Got: %i  Expected: %i for name %s\n",
                       script[0],expected[i],testNames[i]);
                   numErrors++;
            }
        }
        if(numErrors >0 ){
            log_data_err("Errors uchar_getScriptCode() : %i \n",numErrors);
        }
    }

    {
        UErrorCode err = U_ZERO_ERROR;
        int32_t capacity=0;
        int32_t j;
        UScriptCode jaCode[]={USCRIPT_KATAKANA, USCRIPT_HIRAGANA, USCRIPT_HAN };
        UScriptCode script[10]={USCRIPT_INVALID_CODE};
        int32_t num = uscript_getCode("ja",script,capacity, &err);
        /* preflight */
        if(err==U_BUFFER_OVERFLOW_ERROR){
            err = U_ZERO_ERROR;
            capacity = 10;
            num = uscript_getCode("ja",script,capacity, &err);
            if(num!=(sizeof(jaCode)/sizeof(UScriptCode))){
                log_err("Errors uscript_getScriptCode() for Japanese locale: num=%d, expected %d \n",
                        num, (sizeof(jaCode)/sizeof(UScriptCode)));
            }
            for(j=0;j<sizeof(jaCode)/sizeof(UScriptCode);j++) {
                if(script[j]!=jaCode[j]) {
                    log_err("Japanese locale: code #%d was %d (%s) but expected %d (%s)\n", j,
                            script[j], uscript_getName(script[j]),
                            jaCode[j], uscript_getName(jaCode[j]));
                    
                }
            }
        }else{
            log_data_err("Errors in uscript_getScriptCode() expected error : %s got: %s \n", 
                "U_BUFFER_OVERFLOW_ERROR",
                 u_errorName(err));
        }

    }

    {
        UScriptCode testAbbr[]={
            /* names should return */
            USCRIPT_CYRILLIC, USCRIPT_DESERET, USCRIPT_DEVANAGARI, USCRIPT_ETHIOPIC, USCRIPT_GEORGIAN,
            USCRIPT_GOTHIC, USCRIPT_GREEK, USCRIPT_GUJARATI,
        };

        const char* expectedNames[]={
              
            /* test names */
            "Cyrillic","Deseret","Devanagari","Ethiopic","Georgian", 
            "Gothic",  "Greek",  "Gujarati", 
             '\0'
        };
        i=0;
        while(i<sizeof(testAbbr)/sizeof(UScriptCode)){
            const char* name = uscript_getName(testAbbr[i]);
             if(name == NULL) {
               log_data_err("Couldn't get script name\n");
               return;
             }
            numErrors=0;
            if(strcmp(expectedNames[i],name)!=0){
                log_err("Error getting abbreviations Got: %s Expected: %s\n",name,expectedNames[i]);
                numErrors++;
            }
            if(numErrors > 0){
                if(numErrors >0 ){
                    log_err("Errors uchar_getScriptAbbr() : %i \n",numErrors);
                }
            }
            i++;
        }

    }

    {
        UScriptCode testAbbr[]={
            /* abbr should return */
            USCRIPT_HAN, USCRIPT_HANGUL, USCRIPT_HEBREW, USCRIPT_HIRAGANA,
            USCRIPT_KANNADA, USCRIPT_KATAKANA, USCRIPT_KHMER, USCRIPT_LAO,
            USCRIPT_LATIN, 
            USCRIPT_MALAYALAM, USCRIPT_MONGOLIAN,
        };

        const char* expectedAbbr[]={
              /* test abbr */
            "Hani", "Hang","Hebr","Hira",
            "Knda","Kana","Khmr","Laoo",
            "Latn",
            "Mlym", "Mong",
             '\0'
        };
        i=0;
        while(i<sizeof(testAbbr)/sizeof(UScriptCode)){
            const char* name = uscript_getShortName(testAbbr[i]);
            numErrors=0;
            if(strcmp(expectedAbbr[i],name)!=0){
                log_err("Error getting abbreviations Got: %s Expected: %s\n",name,expectedAbbr[i]);
                numErrors++;
            }
            if(numErrors > 0){
                if(numErrors >0 ){
                    log_err("Errors uchar_getScriptAbbr() : %i \n",numErrors);
                }
            }
            i++;
        }

    }
    /* now test uscript_getScript() API */
    {
#define MAX_ARRAY_SIZE 23
        uint32_t codepoints[] = {
                0x0000FF9D, /* USCRIPT_KATAKANA*/
                0x0000FFBE, /* USCRIPT_HANGUL*/
                0x0000FFC7, /* USCRIPT_HANGUL*/
                0x0000FFCF, /* USCRIPT_HANGUL*/
                0x0000FFD7, /* USCRIPT_HANGUL*/
                0x0000FFDC, /* USCRIPT_HANGUL*/
                0x00010300, /* USCRIPT_OLD_ITALIC*/
                0x00010330, /* USCRIPT_GOTHIC*/
                0x0001034A, /* USCRIPT_GOTHIC*/
                0x00010400, /* USCRIPT_DESERET*/
                0x00010428, /* USCRIPT_DESERET*/
                0x0001D167, /* USCRIPT_INHERITED*/
                0x0001D17B, /* USCRIPT_INHERITED*/
                0x0001D185, /* USCRIPT_INHERITED*/
                0x0001D1AA, /* USCRIPT_INHERITED*/
                0x00020000, /* USCRIPT_HAN*/
                0x00000D02, /* USCRIPT_MALAYALAM*/
                0x00000D00, /* USCRIPT_UNKNOWN (new Zzzz value in Unicode 5.0) */
                0x00000000, /* USCRIPT_COMMON*/
                0x0001D169, /* USCRIPT_INHERITED*/
                0x0001D182, /* USCRIPT_INHERITED*/
                0x0001D18B, /* USCRIPT_INHERITED*/
                0x0001D1AD, /* USCRIPT_INHERITED*/
                0x00110000, /* USCRIPT_INVALID_CODE */
        };

        UScriptCode expected[] = {
                USCRIPT_KATAKANA ,
                USCRIPT_HANGUL ,
                USCRIPT_HANGUL ,
                USCRIPT_HANGUL ,
                USCRIPT_HANGUL ,
                USCRIPT_HANGUL ,
                USCRIPT_OLD_ITALIC, 
                USCRIPT_GOTHIC ,
                USCRIPT_GOTHIC ,
                USCRIPT_DESERET ,
                USCRIPT_DESERET ,
                USCRIPT_INHERITED,
                USCRIPT_INHERITED,
                USCRIPT_INHERITED,
                USCRIPT_INHERITED,
                USCRIPT_HAN ,
                USCRIPT_MALAYALAM,
                USCRIPT_UNKNOWN,
                USCRIPT_COMMON,
                USCRIPT_INHERITED ,
                USCRIPT_INHERITED ,
                USCRIPT_INHERITED ,
                USCRIPT_INHERITED ,
                USCRIPT_INVALID_CODE,
        };
        UScriptCode code = USCRIPT_INVALID_CODE;
        UErrorCode status = U_ZERO_ERROR;
        UBool passed = TRUE;

        i =0;
        while(i< MAX_ARRAY_SIZE){
            code = uscript_getScript(codepoints[i],&status);
            if(U_SUCCESS(status)){
                if( code != expected[i] ||
                    code != (UScriptCode)u_getIntPropertyValue(codepoints[i], UCHAR_SCRIPT)
                ) {
                    log_err("uscript_getScript for codepoint \\U%08X failed\n",codepoints[i]);
                    passed = FALSE;
                }
            }else{
                log_err("uscript_getScript for codepoint \\U%08X failed. Error: %s\n", 
                         codepoints[i],u_errorName(status));
                break;
            }
            i++;
        }
        
        if(passed==FALSE){
           log_err("uscript_getScript failed.\n");
        }      
    }
    {
        UScriptCode code= USCRIPT_INVALID_CODE;
        UErrorCode  status = U_ZERO_ERROR;
        code = uscript_getScript(0x001D169,&status);
        if(code != USCRIPT_INHERITED){
            log_err("\\U001D169 is not contained in USCRIPT_INHERITED");
        }
    }
    {
        UScriptCode code= USCRIPT_INVALID_CODE;
        UErrorCode  status = U_ZERO_ERROR;
        int32_t err = 0;

        for(i = 0; i<=0x10ffff; i++){
            code =  uscript_getScript(i,&status);
            if(code == USCRIPT_INVALID_CODE){
                err++;
                log_err("uscript_getScript for codepoint \\U%08X failed.\n", i);
            }
        }
        if(err>0){
            log_err("uscript_getScript failed for %d codepoints\n", err);
        }
    }
    {
        for(i=0; (UScriptCode)i< USCRIPT_CODE_LIMIT; i++){
            const char* name = uscript_getName((UScriptCode)i);
            if(name==NULL || strcmp(name,"")==0){
                log_err("uscript_getName failed for code %i: name is NULL or \"\"\n",i);
            }
        }
    }
                
    {
        /*
         * These script codes were originally added to ICU pre-3.6, so that ICU would
         * have all ISO 15924 script codes. ICU was then based on Unicode 4.1.
         * These script codes were added with only short names because we don't
         * want to invent long names ourselves.
         * Unicode 5 and later encode some of these scripts and give them long names.
         * Whenever this happens, the long script names here need to be updated.
         */
        static const char* expectedLong[] = {
            "Balinese", "Batk", "Blis", "Brah", "Cham", "Cirt", "Cyrs", "Egyd", "Egyh", "Egyp", 
            "Geok", "Hans", "Hant", "Hmng", "Hung", "Inds", "Java", "Kali", "Latf", "Latg", 
            "Lepc", "Lina", "Mand", "Maya", "Mero", "Nko", "Orkh", "Perm", "Phags_Pa", "Phoenician", 
            "Plrd", "Roro", "Sara", "Syre", "Syrj", "Syrn", "Teng", "Vaii", "Visp", "Cuneiform", 
            "Zxxx", "Unknown",
            "Cari", "Jpan", "Lana", "Lyci", "Lydi", "Olck", "Rjng", "Saur", "Sgnw", "Sund",
            "Moon", "Mtei",
        };
        static const char* expectedShort[] = {
            "Bali", "Batk", "Blis", "Brah", "Cham", "Cirt", "Cyrs", "Egyd", "Egyh", "Egyp", 
            "Geok", "Hans", "Hant", "Hmng", "Hung", "Inds", "Java", "Kali", "Latf", "Latg", 
            "Lepc", "Lina", "Mand", "Maya", "Mero", "Nkoo", "Orkh", "Perm", "Phag", "Phnx", 
            "Plrd", "Roro", "Sara", "Syre", "Syrj", "Syrn", "Teng", "Vaii", "Visp", "Xsux", 
            "Zxxx", "Zzzz",
            "Cari", "Jpan", "Lana", "Lyci", "Lydi", "Olck", "Rjng", "Saur", "Sgnw", "Sund",
            "Moon", "Mtei",
        };
        int32_t j = 0;
        for(i=USCRIPT_BALINESE; (UScriptCode)i<USCRIPT_CODE_LIMIT; i++, j++){
            const char* name = uscript_getName((UScriptCode)i);
            if(name==NULL || strcmp(name,expectedLong[j])!=0){
                log_err("uscript_getName failed for code %i: %s!=%s\n", i, name, expectedLong[j]);
            }
            name = uscript_getShortName((UScriptCode)i);
            if(name==NULL || strcmp(name,expectedShort[j])!=0){
                log_err("uscript_getShortName failed for code %i: %s!=%s\n", i, name, expectedShort[j]);
            }
        }
        for(i=0; i<ARRAY_SIZE(expectedLong); i++){
            UScriptCode fillIn[5] = {USCRIPT_INVALID_CODE};
            UErrorCode status = U_ZERO_ERROR;
            int32_t len = 0;
            len = uscript_getCode(expectedShort[i], fillIn, ARRAY_SIZE(fillIn), &status);
            if(U_FAILURE(status)){
                log_err("uscript_getCode failed for script name %s. Error: %s\n",expectedShort[i], u_errorName(status));
            }
            if(len>1){
                log_err("uscript_getCode did not return expected number of codes for script %s. EXPECTED: 1 GOT: %i\n", expectedShort[i], len);
            }
            if(fillIn[0]!= (UScriptCode)(USCRIPT_BALINESE+i)){
                log_err("uscript_getCode did not return expected code for script %s. EXPECTED: %i GOT: %i\n", expectedShort[i], (USCRIPT_BALINESE+i), fillIn[0] );
            }
        }
    }
 }
