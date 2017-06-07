#ifndef DBG_XHCI_H__
#define DBG_XHCI_H__

#if DBG

    #ifndef NDEBUG_XHCI_TRACE
        #define DPRINT_XHCI(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)
    #else
        #if defined(_MSC_VER)
            #define DPRINT_XHCI __noop
        #else
            #define DPRINT_XHCI(...) do {if(0) {DbgPrint(__VA_ARGS__);}} while(0)
        #endif
    #endif

    #ifndef NDEBUG_XHCI_ROOT_HUB
        #define DPRINT_RH(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)
    #else
        #if defined(_MSC_VER)
            #define DPRINT_RH __noop
        #else
            #define DPRINT_RH(...) do {if(0) {DbgPrint(__VA_ARGS__);}} while(0)
        #endif
    #endif

#else /* not DBG */

    #if defined(_MSC_VER)
        #define DPRINT_XHCI __noop
        #define DPRINT_RH __noop
    #else
        #define DPRINT_XHCI(...) do {if(0) {DbgPrint(__VA_ARGS__);}} while(0)
        #define DPRINT_RH(...) do {if(0) {DbgPrint(__VA_ARGS__);}} while(0)
    #endif /* _MSC_VER */

#endif /* not DBG */

#endif /* DBG_XHCI_H__ */
