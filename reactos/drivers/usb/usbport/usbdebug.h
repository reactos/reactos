#ifndef USBDEBUG_H__
#define USBDEBUG_H__

#if DBG

    #ifndef NDEBUG_USBPORT_MINIPORT

        #define DPRINT_MINIPORT(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

#if defined(_MSC_VER)
        #define DPRINT_MINIPORT   __noop
#else
        #define DPRINT_MINIPORT(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

    #endif

    #ifndef NDEBUG_USBPORT_CORE

        #define DPRINT_CORE(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

#if defined(_MSC_VER)
        #define DPRINT_CORE   __noop
#else
        #define DPRINT_CORE(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

    #endif

    #ifndef NDEBUG_USBPORT_URB

        #define DPRINT_URB(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

#if defined(_MSC_VER)
        #define DPRINT_URB   __noop
#else
        #define DPRINT_URB(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

    #endif

    #ifndef NDEBUG_USBPORT_INTERRUPT

        #define DPRINT_INT(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

#if defined(_MSC_VER)
        #define DPRINT_INT   __noop
#else
        #define DPRINT_INT(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

    #endif

    #ifndef NDEBUG_USBPORT_TIMER

        #define DPRINT_TIMER(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

#if defined(_MSC_VER)
        #define DPRINT_TIMER   __noop
#else
        #define DPRINT_TIMER(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

    #endif

    #ifndef NDEBUG_USBPORT_QUEUE

        #define DPRINT_QUEUE(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

#if defined(_MSC_VER)
        #define DPRINT_QUEUE   __noop
#else
        #define DPRINT_QUEUE(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

    #endif

#else /* not DBG */

#if defined(_MSC_VER)
    #define DPRINT_MINIPORT    __noop
    #define DPRINT_CORE    __noop
    #define DPRINT_URB    __noop
    #define DPRINT_INT    __noop
    #define DPRINT_TIMER    __noop
    #define DPRINT_QUEUE    __noop
#else
    #define DPRINT_MINIPORT(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT_CORE(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT_URB(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT_INT(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT_TIMER(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT_QUEUE(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif /* _MSC_VER */

#endif /* not DBG */

#endif /* USBDEBUG_H__ */
