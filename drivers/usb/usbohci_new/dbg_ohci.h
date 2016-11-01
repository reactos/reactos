#ifndef DBG_OHCI_H__
#define DBG_OHCI_H__

#if DBG

    #ifndef NDEBUG_OHCI_TRACE
        #define DPRINT_OHCI(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)
    #else
        #if defined(_MSC_VER)
            #define DPRINT_OHCI __noop
        #else
            #define DPRINT_OHCI(...) do {if(0) {DbgPrint(__VA_ARGS__);}} while(0)
        #endif
    #endif

#else /* not DBG */

    #if defined(_MSC_VER)
        #define DPRINT_OHCI __noop
    #else
        #define DPRINT_OHCI(...) do {if(0) {DbgPrint(__VA_ARGS__);}} while(0)
    #endif /* _MSC_VER */

#endif /* not DBG */

#endif /* DBG_OHCI_H__ */
