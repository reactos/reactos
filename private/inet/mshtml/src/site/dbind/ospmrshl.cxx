// Copyright (c) 1996-1997  Microsoft Corporation.  All rights reserved.
#include "headers.hxx"

#define JAVAVMAPI                   // avoid dll linkage errors

#ifndef X_WINDOWS_H_
#define X_WINDOWS_H_
#include <windows.h>
#endif

#ifndef X_OLEAUTO_H_
#define X_OLEAUTO_H_
#include <oleauto.h>
#endif

#ifndef X_NATIVE_H_
#define X_NATIVE_H_
#include "native.h" // Raw Native Interface declarations.
#endif

#ifndef X_NATIVCOM_H_
#define X_NATIVCOM_H_
#include "nativcom.h"
#endif

// helpers for date conversions
const static double daysBetween1900And1970 = 25567.0;
const static __int64 iSecondsPerDay = 86400;

typedef enum {
    IntegerClass,
    StringClass,
    LongClass,
    FloatClass,
    DoubleClass,
    BooleanClass,
    DateClass
} ClassIDs;

typedef VARIANT *PETYPE, **PPETYPE;

typedef OBJECT* JTYPE;

typedef void (*pf)(JTYPE javaval, PETYPE petype);

typedef struct {
    ClassClass *type;
    pf func;
    char *className;
} TypeFuncPair;

#define EXTERNC extern "C"

static void fInteger(JTYPE javaval, PETYPE petype)
{
    long iTemp = execute_java_dynamic_method(NULL,
                                             javaval,
                                             "intValue",
                                             "()I");
    V_VT(petype) = VT_I4;
    V_I4(petype) = iTemp;
}

static void fLong(JTYPE javaval, PETYPE petype)
{
#if 0
    // do we have a long long type???
    long long iTemp = execute_java_dynamic_method64(NULL,
        javaval,
        "longValue",
        "()J");
    V_VT(petype) = VT_I8;
    V_I4(petype) = iTemp;
#else
    fInteger(javaval, petype);
#endif
}

static void fFloat(JTYPE javaval, PETYPE petype)
{
    long rTemp = execute_java_dynamic_method(NULL,
                                             javaval,
                                             "floatValue",
                                             "()F");
    V_VT(petype) = VT_R4;
    V_I4(petype) = rTemp;
}

static void fDouble(JTYPE javaval, PETYPE petype)
{
    double iTemp = execute_java_dynamic_method64(NULL,
                                                 javaval,
                                                 "doubleValue",
                                                 "()D");
    V_VT(petype) = VT_R8;
    V_R8(petype) = iTemp;
}

static void fBoolean(JTYPE javaval, PETYPE petype)
{
    int iTemp = execute_java_dynamic_method64(NULL,
                                              javaval,
                                              "booleanValue",
                                              "()Z");
    V_VT(petype) = VT_BOOL;
    V_BOOL(petype) = !!iTemp;
}

static void fDate(JTYPE javaval, PETYPE petype)
{
    // java to variant
	
    __int64 iDate = execute_java_dynamic_method64(NULL,
                                                  javaval,
                                                  "getTime",
                                                  "()J");

    __int64 iDays = (iDate / 1000) / iSecondsPerDay;
    __int64 iRemainingMS = iDate - (iDays * iSecondsPerDay * 1000);

    double variantDate = daysBetween1900And1970    +
                         (double)iDays             +
                         ((double)iRemainingMS / ((double)iSecondsPerDay * 1000.0));
    
    V_VT(petype) = VT_DATE;
    V_DATE(petype) = variantDate;
}

static void fString(JTYPE javaval, PETYPE petype)
{
    Hjava_lang_String *stringValue = (Hjava_lang_String *) javaval;
    unicode *uc = javaStringStart (stringValue);
    int ucLen = javaStringLength(stringValue);

    V_VT(petype) = VT_BSTR;
    V_BSTR(petype) = SysAllocStringLen(uc, ucLen);
}

static void fStringViaToString(JTYPE javaval, PETYPE petype)
{
    LONG_PTR stringValue = execute_java_dynamic_method(NULL,                       // ExecEnv *ee,
        javaval,                    // HObject *obj,
        "toString",	                // method name
        "()Ljava/lang/String;");    // signature
        fString((JTYPE)stringValue, petype);
}

static void fIUnknown(JTYPE javaval, PETYPE petype)
{
    // BUGBUG what about the iid param
    IUnknown *punk = convert_Java_Object_to_IUnknown(javaval, &IID_IUnknown);
    if (punk != NULL)
    {
        V_VT(petype) = VT_UNKNOWN;
        V_UNKNOWN(petype) = punk;
    }
    else
        fStringViaToString(javaval, petype);
}

static void fVariant(JTYPE javaval, PETYPE petype)
{
    VARIANT *v = (VARIANT*) jcdwGetData(javaval);
    VariantCopy(petype, v);
}

static TypeFuncPair dispatchTable[] = {
    NULL,		fInteger,			"java/lang/Integer",
    NULL,		fString,            "java/lang/String",
    NULL,		fLong,              "java/lang/Long",
    NULL,		fFloat,				"java/lang/Float",
    NULL,		fDouble,			"java/lang/Double",
    NULL,	    fBoolean,			"java/lang/Boolean",
    NULL,       fDate,				"java/util/Date",
    NULL,       fVariant,           "com/ms/com/Variant"
};



static int initialized = 0;

#define EXTERNC     extern "C"

#define JAVAMETHOD(typ, name) \
                              EXTERNC \
                              typ __cdecl com_ms_osp_ospmrshl_##name

DWORD __cdecl RNIGetCompatibleVersion()
{
    return RNIVER;
}

JAVAMETHOD(void, classInit) (OBJECT*x)
{
    int i;
    for (i = 0; i < sizeof(dispatchTable)/sizeof(TypeFuncPair); i++)
    {
        TypeFuncPair *pair = &dispatchTable[i];
        pair->type = FindClass(NULL, pair->className, TRUE);
        if (pair->type == NULL)
        {
            //			char buf[512];
            //			sprintf(buf, "Could not preload %s class!", pair->className);
            //			SignalErrorPrintf("java/lang/ClassNotFoundException", buf);
        }
    }
    initialized = 1;
}

//==========================================================================
// toJava
//==========================================================================
JAVAMETHOD(JTYPE, toJava) (OBJECT*x, PPETYPE ppVariant, int flags)
{
    VARIANT *pVariant = *ppVariant;

    VARTYPE type = V_VT(pVariant) & ~VT_BYREF;
    int isByRef = V_ISBYREF(pVariant);

    if ( !initialized )
        com_ms_osp_ospmrshl_classInit(0);

    // question: can we safely ignore the VT_BYREF and assume that
    // the accessor macros will dereference for us?

    if (V_ISARRAY(pVariant) || V_ISVECTOR(pVariant))
        return NULL;

    switch (type)
    {
        case VT_UI1:
        case VT_I1:
        case VT_UI2:
        case VT_I2:
            //		case VT_UI4:
        case VT_I4:
        {
            int intVal = 0;
            switch(V_VT(pVariant))
            {
                case VT_UI1: intVal = (isByRef ? *V_UI1REF(pVariant) : V_UI1(pVariant)); break;
                case VT_I1 : intVal = (isByRef ? *V_I1REF (pVariant) : V_I1 (pVariant)); break;

                case VT_UI2: intVal = (isByRef ? *V_UI2REF(pVariant) : V_UI2(pVariant)); break;
                case VT_I2 : intVal = (isByRef ? *V_I2REF (pVariant) : V_I2 (pVariant)); break;

                             //				case VT_UI4: intVal = (isByRef ? *V_UI4REF(pVariant) : V_UI4(pVariant)); break;
                case VT_I4 : intVal = (isByRef ? *V_I4REF (pVariant) : V_I4 (pVariant)); break;
            }
            return execute_java_constructor(NULL, NULL,
                                            dispatchTable[IntegerClass].type,
                                            "(I)", intVal);
        }

        case VT_UI4:
        {
            __int64 intVal = (isByRef ? *V_UI4REF(pVariant) : V_UI4(pVariant));
            return execute_java_constructor(NULL, NULL,
                                            dispatchTable[LongClass].type,
                                            "(J)", intVal);
        }	

        case VT_R4:
            return execute_java_constructor(NULL, NULL,
                                            dispatchTable[FloatClass].type, "(F)",
                                            (isByRef ? *V_R4REF(pVariant) : V_R4(pVariant)));

        case VT_R8:
            return execute_java_constructor(NULL, NULL,
                                            dispatchTable[DoubleClass].type, "(D)",
                                            (isByRef ? *V_R8REF(pVariant) : V_R8(pVariant)));

        case VT_CY:
            return NULL;  // BUGBUG (mwagner) what to do about VT_CY (currency)
            break;

        case VT_BSTR:
        {
            BSTR string = (isByRef ? *V_BSTRREF(pVariant) : V_BSTR(pVariant));
            return (HObject*)makeJavaStringW(string, SysStringLen(string));
        }

        case VT_BOOL:
            return execute_java_constructor(NULL, NULL,
                                            dispatchTable[BooleanClass].type, "(Z)",
                                            (isByRef ? *V_BOOLREF(pVariant) : V_BOOL(pVariant)));

        case VT_DATE:
        {
            double variantDate = (isByRef ? *V_DATEREF(pVariant) : V_DATE(pVariant));
            __int64 iDays = (__int64)variantDate;  // truncate, giving integral number of days
            __int64 iMS = (__int64)(variantDate - (double)iDays) * iSecondsPerDay * 1000;
            __int64 iJavaDate = (iDays * 1000) + iMS;  // millisecond units
            
            return execute_java_constructor(NULL, NULL,
                                            dispatchTable[DateClass].type, "(J)",  // bugbug
                                            iJavaDate);       
        }	

        case VT_VARIANT:
            return com_ms_osp_ospmrshl_toJava(x, &V_VARIANTREF(pVariant), flags);
#if 1
            //commented out for now (until we're in the trident environment)
        case VT_DISPATCH:
        {
            Hjava_lang_Object *retVal = NULL;
            IDispatch *pdisp = (isByRef ? *V_DISPATCHREF(pVariant) : V_DISPATCH(pVariant));
            IUnknown *punk = NULL;
            if (0 == pdisp->QueryInterface(IID_IUnknown, (void**)&punk))
            {
                retVal = convert_IUnknown_to_Java_Object(punk, NULL, 1);
                punk->Release();
            }
            return retVal;
            break;
        }

        case VT_UNKNOWN:
        {
            IUnknown *punk = (isByRef ? *V_UNKNOWNREF(pVariant) : V_UNKNOWN(pVariant));
            return convert_IUnknown_to_Java_Object(punk, NULL, 1);
        }
#endif
        case VT_ARRAY:
        case VT_BYREF:
            
        case VT_ERROR:
        case VT_NULL:
        case VT_EMPTY:

        default:
        {
            return NULL;
        }	
    }
}


//==========================================================================
// copyToExternal
//==========================================================================

JAVAMETHOD(void, copyToExternal) (OBJECT*x, JTYPE javaval, PPETYPE ppetype, int flags)
{
    PETYPE petype = *ppetype;
    int i;
    JTYPE preservedValues[2] = { NULL, (JTYPE)-1 };
    GCFrame frame;
    GCFramePush(&frame, &preservedValues, sizeof(javaval));
    preservedValues[0] = javaval;

    BOOL fDone = FALSE;

    if ( !initialized )
        com_ms_osp_ospmrshl_classInit(0);

    if (preservedValues[0] == NULL)
    {
        V_VT(petype) = VT_NULL;
    }
    else {
        for (i = 0; i < sizeof(dispatchTable)/sizeof(TypeFuncPair); i++)
        {
            const TypeFuncPair *pair = &dispatchTable[i];
            if ( pair->type && is_instance_of(preservedValues[0], pair->type, NULL) )
            {
                (*pair->func)(preservedValues[0], petype);
                fDone = TRUE;
            }
        }
        if (!fDone)
            fIUnknown(preservedValues[0], petype); // could also be fStringViaToString(javaval, petype);
    }
    GCFramePop(&frame);
}

//==========================================================================
// releaseByValExternal
//==========================================================================
JAVAMETHOD(void, releaseByValExternal) (OBJECT*x, PPETYPE ppetype, int flags)
{
    PETYPE petype = *ppetype;
    VariantClear(petype);
}


