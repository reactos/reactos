/*
 * Stack protector and fortify stubs for MSYS2/ucrt64 toolchains.
 *
 * ucrt64's libsupc++ and libwinpthread are compiled with -fstack-protector,
 * which references __stack_chk_fail/__stack_chk_guard/__chk_fail and
 * __mingw_snprintf. These symbols normally come from libmingwex, but that
 * library depends on ucrt-specific imports not available in ReactOS.
 *
 * This file provides minimal implementations so ReactOS C++ targets can link.
 * Not needed with the Linux cross-compiler (its runtime libs lack these refs).
 */

/* Provide a non-zero canary so stack checks don't false-positive */
void *__stack_chk_guard = (void *)0xDEADBEEF;

__attribute__((noreturn))
void __stack_chk_fail(void)
{
    /* In ReactOS user-mode, just crash. */
    __builtin_trap();
}

__attribute__((noreturn))
void __chk_fail(void)
{
    __builtin_trap();
}

/* Minimal snprintf used by libsupc++'s demangler */
int __mingw_snprintf(char *buf, unsigned long long size, const char *fmt, ...);

/* Forward to the CRT's _snprintf */
int _snprintf(char *buf, unsigned long long size, const char *fmt, ...);

int __mingw_snprintf(char *buf, unsigned long long size, const char *fmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    /* Use _vsnprintf from msvcrt */
    extern int _vsnprintf(char *, unsigned long long, const char *, __builtin_va_list);
    int ret = _vsnprintf(buf, size, fmt, ap);
    __builtin_va_end(ap);
    return ret;
}
