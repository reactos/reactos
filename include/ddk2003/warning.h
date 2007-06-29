#pragma warning(3:4092)
#pragma warning(4:4096)
#pragma warning(4:4121)
#pragma warning(3:4125)
#pragma warning(3:4130)
#pragma warning(3:4132)

#if _DBG_MEMCPY_INLINE_
    #pragma warning(disable:4163)
#endif

#pragma warning(4:4206)
#pragma warning(4:4101)
#pragma warning(4:4208)
#pragma warning(3:4212)
#pragma warning(3:4242)

#if defined(_M_IA64)
    #pragma warning(disable:4407)
    #pragma warning(disable:4714)
#endif

#pragma warning(4:4267)
#pragma warning(4:4312)
#pragma warning(disable:4324)
#pragma warning(error:4700)
#pragma warning(error:4259)
#pragma warning(disable:4071)
#pragma warning(error:4013)
#pragma warning(error:4551)
#pragma warning(error:4806)
#pragma warning(4:4509)
#pragma warning(4:4177)
#pragma warning(disable:4274)
#pragma warning(disable:4786)
#pragma warning(disable:4503)
#pragma warning(disable:4263)
#pragma warning(disable:4264)
#pragma warning(disable:4710)
#pragma warning(disable:4917)
#pragma warning(error:4552)
#pragma warning(error:4553)
#pragma warning(3:4288)
#pragma warning(3:4532)
#pragma warning(error:4312)
#pragma warning(error:4296)
#pragma warning(3:4546)

#if _MSC_VER > 1300
    #pragma warning(disable:4197)
    #pragma warning(disable:4675)
    #pragma warning(disable:4356)
#endif


#ifndef __GNUC__
    #ifndef __cplusplus
        #undef try
        #undef except
        #undef finally
        #undef leave
            #define try         __try
            #define except      __except
            #define finally     __finally
            #define leave       __leave
    #endif
#else
        /* FIXME when gcc support ms seh remove  #ifndef __REACTOS__ */ 
#endif

#if _MSC_VER <= 1400
    #pragma warning(disable: 4068)
#endif

#if defined(_M_IA64) && _MSC_VER > 1310
    #define __TYPENAME typename
#elif defined(_M_IX86) && _MSC_FULL_VER >= 13102154
    #define __TYPENAME typename
#elif defined(_M_AMD64) && _MSC_VER >= 1400
    #define __TYPENAME typename
#else
    #define __TYPENAME
#endif

