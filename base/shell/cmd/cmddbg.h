#ifdef _DEBUG_MEM

#define cmd_alloc(size) cmd_alloc_dbg(size, __FILE__, __LINE__)
#define cmd_realloc(ptr,size) cmd_realloc_dbg(ptr, size, __FILE__, __LINE__)
#define cmd_free(ptr) cmd_free_dbg(ptr, __FILE__, __LINE__)
#define cmd_checkbuffer(ptr) cmd_checkbuffer_dbg(ptr, __FILE__, __LINE__)
#define cmd_dup(str) cmd_dup_dbg(str, __FILE__, __LINE__)

void *
cmd_alloc_dbg(size_t size, const char *file, int line);

void *
cmd_realloc_dbg(void *ptr, size_t size, const char *file, int line);

void
cmd_free_dbg(void *ptr, const char *file, int line);

TCHAR *
cmd_dup_dbg(const TCHAR *str, const char *file, int line);

void
cmd_checkbuffer_dbg(void *ptr, const char *file, int line);

void
cmd_exit(int code);

#else

#define cmd_alloc(size) malloc(size)
#define cmd_realloc(ptr,size) realloc(ptr, size)
#define cmd_free(ptr) free(ptr)
#define cmd_dup(str) _tcsdup(str)
#define cmd_checkbuffer(ptr)

#endif
