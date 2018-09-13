
#if USE_HEAP_CHECKING

void
ValidateTheHeap(
    char *fName,
    unsigned long dwLine
    );

#define ValidateHeap()   ValidateTheHeap( __FILE__, __LINE__ )

#else

#define ValidateHeap()

#endif
