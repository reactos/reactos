/* File generated automatically; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

char __wine_dbch_accel[] = "\003accel";
char __wine_dbch_ole[] = "\003ole";
char __wine_dbch_storage[] = "\003storage";

static char * const debug_channels[3] =
{
    __wine_dbch_accel,
    __wine_dbch_ole,
    __wine_dbch_storage
};

static void *debug_registration;

#ifdef __GNUC__
static void __wine_dbg_ole32_init(void) __attribute__((constructor));
static void __wine_dbg_ole32_fini(void) __attribute__((destructor));
#else
static void __asm__dummy_dll_init(void) {
asm("\t.section\t\".init\" ,\"ax\"\n"
    "\tcall ___wine_dbg_ole32_init\n"
    "\t.section\t\".fini\" ,\"ax\"\n"
    "\tcall ___wine_dbg_ole32_fini\n"
    "\t.section\t\".text\"\n");
}
#endif /* defined(__GNUC__) */

#ifdef __GNUC__
static
#endif
void __wine_dbg_ole32_init(void)
{
    extern void *__wine_dbg_register( char * const *, int );
    debug_registration = __wine_dbg_register( debug_channels, 3 );
}

#ifdef __GNUC__
static
#endif
void __wine_dbg_ole32_fini(void)
{
    extern void __wine_dbg_unregister( void* );
    __wine_dbg_unregister( debug_registration );
}
