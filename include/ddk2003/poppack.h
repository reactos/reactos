
#if ! (defined(lint) || defined(RC_INVOKED))
    #if ( _MSC_VER >= 800 && !defined(_M_I86)) || defined(_PUSHPOP_SUPPORTED)
        #pragma warning(disable:4103)
        #if !(defined( MIDL_PASS )) || defined( __midl )
            #pragma pack(pop)
        #else
            #pragma pack()
        #endif
    #else
        #pragma pack()
    #endif
#endif

