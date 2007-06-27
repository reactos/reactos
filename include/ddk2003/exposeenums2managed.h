
#ifdef MANAGED_ENUMS

    #ifndef _MANAGED
        #error "Sorry u can only generate managed enums when compiling managed code"
    #endif

    #define ENUM typedef public __value enum
    #define ENUM16 ENUM
    #define FLAGS [System::Flags] ENUM
    #define TAG(x) x

#else

    #ifdef __midl
        #define V1_ENUM [v1_enum]
    #else
        #define V1_ENUM
    #endif

    #define ENUM typedef V1_ENUM enum
    #define ENUM16 typedef enum
    #define FLAGS ENUM
    #define TAG(x) tag##x
#endif

