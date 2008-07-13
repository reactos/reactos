/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/udbgutil.h"
#include "unicode/dbgutil.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/ustring.h"
#include "util.h"
#include "ucln.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static UnicodeString **strs = NULL;

static const UnicodeString&  _fieldString(UDebugEnumType type, int32_t field, UnicodeString& fillin) {
    const char *str = udbg_enumName(type, field);
    if(str == NULL) {
        return fillin.remove();
    } else {
        return fillin = UnicodeString(str, ""); // optimize?
    }
}

U_CDECL_BEGIN
static void udbg_cleanup(void) {
    if(strs != NULL) {
        for(int t=0;t<=UDBG_ENUM_COUNT;t++) {
            delete [] strs[t];
        }
        delete[] strs;
        strs = NULL;
    }
}

static UBool ctestfw_cleanup(void)
{
    udbg_cleanup();
    return TRUE;
}

static void udbg_register_cleanup(void) {
   ucln_registerCleanup(UCLN_CTESTFW, ctestfw_cleanup);
}
U_CDECL_END

static void udbg_setup(void) {
    if(strs == NULL) {
        udbg_register_cleanup();
        //fprintf(stderr,"Initializing string cache..\n");
        //fflush(stderr);
        UnicodeString **newStrs = new UnicodeString*[UDBG_ENUM_COUNT+1];
        for(int t=0;t<UDBG_ENUM_COUNT;t++) {
            int32_t c = udbg_enumCount((UDebugEnumType)t);
            newStrs[t] = new UnicodeString[c+1];
            for(int f=0;f<=c;f++) {
                _fieldString((UDebugEnumType)t, f, newStrs[t][f]);
            }
        }
        newStrs[UDBG_ENUM_COUNT] = new UnicodeString[1]; // empty string

        strs = newStrs;
    }
}



T_CTEST_API const UnicodeString& T_CTEST_EXPORT2 udbg_enumString(UDebugEnumType type, int32_t field) {
    if(strs == NULL ) {
        udbg_setup();
    }
    if(type<0||type>=UDBG_ENUM_COUNT) {
        // use UDBG_ENUM_COUNT,0  to mean an empty string
        //fprintf(stderr, "** returning out of range on %d\n",type);
        //fflush(stderr);
        return strs[UDBG_ENUM_COUNT][0];
    }
    int32_t count = udbg_enumCount(type);
    //fprintf(stderr, "enumString [%d,%d]: typecount %d, fieldcount %d\n", type,field,UDBG_ENUM_COUNT,count);
    //fflush(stderr);
    if(field<0 || field > count) {
        return strs[type][count];
    } else {        return strs[type][field];
    }
}

T_CTEST_API int32_t  T_CTEST_EXPORT2 udbg_enumByString(UDebugEnumType type, const UnicodeString& string) {
    if(type<0||type>=UDBG_ENUM_COUNT) {
        return -1;
    }
    // initialize array
    udbg_enumString(type,0);
    // search
    for(int i=0;i<udbg_enumCount(type);i++) {
        if(string == (strs[type][i])) {
            return i;
        }
    }
    return -1;
}

// from DataMap::utoi
T_CTEST_API int32_t 
udbg_stoi(const UnicodeString &s)
{
    char ch[256];
    const UChar *u = s.getBuffer();
    int32_t len = s.length();
    u_UCharsToChars(u, ch, len);
    ch[len] = 0; /* include terminating \0 */
    return atoi(ch);
}

T_CTEST_API UnicodeString *
udbg_escape(const UnicodeString &src, UnicodeString *dst)
{
    dst->remove();
    for (int32_t i = 0; i < src.length(); ++i) {
        UChar c = src[i];
        if(ICU_Utility::isUnprintable(c)) {
            *dst += UnicodeString("[");
            ICU_Utility::escapeUnprintable(*dst, c);
            *dst += UnicodeString("]");
        }
        else {
            *dst += c;
        }
    }

    return dst;
}



#endif
