/* mem.h */


  #define       DSSMalloc(a)       LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, a)
  #define       DSSFree(a)         if (a) LocalFree(a)


