#ifdef DEBUG
#ifdef K32
const char UnimpMessage[] = "Unimplemented API %s";
const char AttentionMessage[] = "ATTENTION !!! API stub might corrupt the stack";

enum {DEB_FATAL, DEB_ERR, DEB_WARN, DEB_TRACE };
int apiLevel = DEB_WARN;
VOID    KERNENTRY   vDebugOut(int level, const char *pfmt, ...);

#define DebugOut(args) vDebugOut args
#define UNIMP_MESSAGE DebugOut((apiLevel, UnimpMessage, api_string));
#define DEBUG_OOPS    DebugOut((DEB_ERR, UnimpMessage, api_string)); \

#else

const char UnimpMessage[] = "Unimplemented API %s\r\n";
const char AttentionMessage[] = "ATTENTION !!! API stub might corrupt the stack\r\n";
#define UNIMP_MESSAGE DebugPrintf(UnimpMessage, api_string);
#define DEBUG_OOPS    DebugPrintf(UnimpMessage, api_string); _asm { int 3 };
#endif
#endif
/*
 * Macros for unimplemented APIs stubs.
 *
 */



#define API_name(api_name) api_name

#define API_decl(api_name) long APIENTRY API_name(api_name)

#ifdef DEBUG
#define API_body(api_name, return_value) \
    const char api_string[] = #api_name; \
    UNIMP_MESSAGE \
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED); \
    return(return_value);
#else
#define API_body(api_name, return_value) \
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED); \
    return(return_value);
#endif

#ifdef DEBUG
#define APISTUB_(api_name, return_value) \
API_decl(api_name) (VOID) \
{ \
    const char api_string[] = #api_name; \
    DebugPrintf(AttentionMessage); \
    DEBUG_OOPS \
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED); \
    return(return_value); \
}
#else
#define APISTUB_(api_name, return_value) \
API_decl(api_name) (VOID) \
{ \
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED); \
    return(return_value); \
}
#endif

#define APISTUB_0(api_name, return_value) \
API_decl(api_name) (VOID) \
{ \
API_body(api_name, return_value) \
}

#define APISTUB_1(api_name, return_value) \
API_decl(api_name) ( \
    int p1 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
}

#define APISTUB_2(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
}

#define APISTUB_3(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2, \
    int p3 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
    p3; \
}

#define APISTUB_4(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2, \
    int p3, \
    int p4 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
    p3; \
    p4; \
}

#define APISTUB_5(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2, \
    int p3, \
    int p4, \
    int p5 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
    p3; \
    p4; \
    p5; \
}

#define APISTUB_6(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2, \
    int p3, \
    int p4, \
    int p5, \
    int p6 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
    p3; \
    p4; \
    p5; \
    p6; \
}

#define APISTUB_7(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2, \
    int p3, \
    int p4, \
    int p5, \
    int p6, \
    int p7 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
    p3; \
    p4; \
    p5; \
    p6; \
    p7; \
}

#define APISTUB_8(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2, \
    int p3, \
    int p4, \
    int p5, \
    int p6, \
    int p7, \
    int p8 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
    p3; \
    p4; \
    p5; \
    p6; \
    p7; \
    p8; \
}

#define APISTUB_9(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2, \
    int p3, \
    int p4, \
    int p5, \
    int p6, \
    int p7, \
    int p8, \
    int p9 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
    p3; \
    p4; \
    p5; \
    p6; \
    p7; \
    p8; \
    p9; \
}

#define APISTUB_10(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2, \
    int p3, \
    int p4, \
    int p5, \
    int p6, \
    int p7, \
    int p8, \
    int p9, \
    int p10 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
    p3; \
    p4; \
    p5; \
    p6; \
    p7; \
    p8; \
    p9; \
    p10; \
}

#define APISTUB_11(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2, \
    int p3, \
    int p4, \
    int p5, \
    int p6, \
    int p7, \
    int p8, \
    int p9, \
    int p10, \
    int p11 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
    p3; \
    p4; \
    p5; \
    p6; \
    p7; \
    p8; \
    p9; \
    p10; \
    p11; \
}

#define APISTUB_12(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2, \
    int p3, \
    int p4, \
    int p5, \
    int p6, \
    int p7, \
    int p8, \
    int p9, \
    int p10, \
    int p11, \
    int p12 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
    p3; \
    p4; \
    p5; \
    p6; \
    p7; \
    p8; \
    p9; \
    p10; \
    p11; \
    p12; \
}

#define APISTUB_13(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2, \
    int p3, \
    int p4, \
    int p5, \
    int p6, \
    int p7, \
    int p8, \
    int p9, \
    int p10, \
    int p11, \
    int p12, \
    int p13 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
    p3; \
    p4; \
    p5; \
    p6; \
    p7; \
    p8; \
    p9; \
    p10; \
    p11; \
    p12; \
    p13; \
}

#define APISTUB_14(api_name, return_value) \
API_decl(api_name) ( \
    int p1, \
    int p2, \
    int p3, \
    int p4, \
    int p5, \
    int p6, \
    int p7, \
    int p8, \
    int p9, \
    int p10, \
    int p11, \
    int p12, \
    int p13, \
    int p14 \
    ) \
{ \
API_body(api_name, return_value) \
    p1; \
    p2; \
    p3; \
    p4; \
    p5; \
    p6; \
    p7; \
    p8; \
    p9; \
    p10; \
    p11; \
    p12; \
    p13; \
    p14; \
}

/*
 * Miscelaneous declarations needed for the APIs
 */
extern void _cdecl DebugPrintf();
