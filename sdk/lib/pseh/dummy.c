 /* intentionally empty file */

int _SEH2_Volatile0 = 0;
int _SEH2_VolatileExceptionCode = 0xC0000005;

/*
 * _SEH2_OpaqueBarrier: a no-op function called at __try/__except boundaries
 * to force Clang to emit a CALL inside the SEH scope. Declared as a volatile
 * function pointer so LLVM cannot see through it and remove the call.
 */
static void _SEH2_OpaqueBarrier_Impl(void) {}
void (*volatile _SEH2_OpaqueBarrier_FnPtr)(void) = _SEH2_OpaqueBarrier_Impl;
void _SEH2_OpaqueBarrier(void) { _SEH2_OpaqueBarrier_FnPtr(); }
