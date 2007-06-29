
#if ! (defined(lint) || defined(RC_INVOKED))
    #if ( _MSC_VER >= 800 && !defined(_M_I86)) || defined(_PUSHPOP_SUPPORTED)
        #pragma warning(disable:4103)
        #if !(defined( MIDL_PASS )) || defined( __midl )
            #pragma pack(push,8)
        #else
            #pragma pack(8)
        #endif
    #else
        #pragma pack(8)
    #endif
#endif

