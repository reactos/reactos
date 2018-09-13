#include "precomp.hxx"

#define JAVAVMAPI                  // avoid dll linkage errors

#ifndef X_NATIVE_H_
#define X_NATIVE_H_
#include "native.h"
#endif

#ifndef X_NATIVCOM_H_
#define X_NATIVCOM_H_
#include "nativcom.h"
#endif



DYNLIB g_dynlibMSJAVA = { NULL, NULL, "MSJAVA.DLL" };
extern DYNLIB g_dynlibSHDOCVW;

#define EXTERNC extern "C"

EXTERNC long __cdecl execute_java_dynamic_method(ExecEnv *ee, HObject *obj, const char
        *method_name, const char *signature, ...)
{
    va_list arg;
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "execute_java_dynamic_methodV" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return 0;

    va_start(arg, signature);

    int64_t retVal = (*(int64_t (__cdecl *)(ExecEnv*, HObject*, const char *,
        const char*, va_list))s_dynproc.pfn)
            ( ee, obj, method_name, signature, arg);
    va_end(arg);
    return (long)retVal;
}

EXTERNC int64_t __cdecl execute_java_dynamic_method64(ExecEnv *ee, HObject *obj, const char
        *method_name, const char *signature, ...)
{
    va_list arg;
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "execute_java_dynamic_methodV" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return 0;

    va_start(arg, signature);

    int64_t retVal = (*(int64_t (__cdecl *)(ExecEnv*, HObject*, const char *,
        const char*, va_list))s_dynproc.pfn)
            ( ee, obj, method_name, signature, arg);
    va_end(arg);
    return retVal;
}

EXTERNC HObject* __cdecl execute_java_constructor(ExecEnv *ee, const char *classname,       \
        ClassClass *cb, const char *signature, ...)
{
    va_list arg;
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "execute_java_constructorV" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return NULL;

    va_start(arg, signature);

    HObject* retVal = (*(HObject* (__cdecl *)(ExecEnv*, const char *,
        ClassClass*, const char*, va_list))s_dynproc.pfn)
            ( ee, classname, cb, signature, arg );
    va_end(arg);
    return retVal;
}

EXTERNC BOOL __cdecl is_instance_of(JHandle *phobj,ClassClass *dcb,ExecEnv *ee)
{
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "is_instance_of" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return FALSE;

    return (*(BOOL (__cdecl *)(JHandle*,ClassClass*,ExecEnv*))s_dynproc.pfn)
            ( phobj, dcb, ee );
}

EXTERNC Hjava_lang_String* __cdecl makeJavaStringW(const unicode *pszwSrc, int cch)
{
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "makeJavaStringW" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return 0;

    return (*(Hjava_lang_String* (__cdecl *)(const unicode*,int))s_dynproc.pfn)
            ( pszwSrc, cch );
}

EXTERNC ClassClass* __cdecl FindClass(ExecEnv *ee, char *classname, bool_t resolve)
{
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "FindClass" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return 0;

    return (*(ClassClass* (__cdecl *)(ExecEnv*, char*, bool_t))s_dynproc.pfn)
            ( ee, classname, resolve );
}

EXTERNC unicode * __cdecl javaStringStart (HString *string)
{
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "javaStringStart" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return 0;

    return (*(unicode* (__cdecl *)(HString*))s_dynproc.pfn)
            ( string );
}

EXTERNC int __cdecl javaStringLength(Hjava_lang_String *string)
{
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "javaStringLength" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return 0;

    return (*(int (__cdecl *)(HString*))s_dynproc.pfn)
            ( string );
}

Hjava_lang_Object * __cdecl convert_IUnknown_to_Java_Object(IUnknown *punk,
                                                            Hjava_lang_Object *phJavaClass,
                                                            int       fAssumeThreadSafe)
{
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "convert_IUnknown_to_Java_Object" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return NULL;

    return (*(Hjava_lang_Object* (__cdecl *)(IUnknown*,
                                              Hjava_lang_Object*,
                                              int))s_dynproc.pfn)
            ( punk, phJavaClass, fAssumeThreadSafe );
}

IUnknown * __cdecl convert_Java_Object_to_IUnknown(Hjava_lang_Object *phJavaObject, const IID *pIID)
{
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "convert_Java_Object_to_IUnknown" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return NULL;

    return (*(IUnknown* (__cdecl *)(Hjava_lang_Object*,
                                     const IID*))s_dynproc.pfn)
            ( phJavaObject, pIID );
}

void __cdecl GCFramePush(PVOID pGCFrame, PVOID pObjects, DWORD cbObjectStructSize)
{
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "GCFramePush" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return;

    (*(void (__cdecl *)(PVOID, PVOID, DWORD))s_dynproc.pfn)
            ( pGCFrame, pObjects, cbObjectStructSize );
}

void __cdecl GCFramePop(PVOID pGCFrame)
{
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "GCFramePop" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return;

    (*(void (__cdecl *)(PVOID))s_dynproc.pfn) ( pGCFrame );
}

void* __cdecl jcdwGetData(Hjava_lang_Object * phJCDW)
{
    static DYNPROC s_dynproc = { NULL, &g_dynlibMSJAVA, "jcdwGetData" };
    HRESULT hr = LoadProcedure(&s_dynproc);
    if (hr)
        return NULL;

    return (*(void* (__cdecl *)(Hjava_lang_Object*))s_dynproc.pfn)
            ( phJCDW );
}

