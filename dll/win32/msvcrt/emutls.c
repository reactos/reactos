/* Emulated TLS support for GCC 15 */
#include <precomp.h>
#include <setjmp.h>

/* Stub for __emutls_get_address - This is usually provided by libgcc */
void* __emutls_get_address(void* control)
{
    /* This is a simplified stub - proper implementation would need thread-local storage */
    static void* dummy_tls[256] = {0};
    size_t index = (size_t)control & 0xFF;
    return &dummy_tls[index];
}

/* Stub for __intrinsic_setjmpex */
int __intrinsic_setjmpex(void* jmpbuf, void* frame)
{
    /* Forward to regular setjmp with context */
    jmp_buf* buf = (jmp_buf*)jmpbuf;
    return _setjmp(*buf, frame);
}