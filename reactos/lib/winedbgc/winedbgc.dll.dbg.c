/* File generated automatically; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

char __wine_dbch_winedbgc[] = "\003winedbgc";

static char * const debug_channels[4] =
{
    __wine_dbch_winedbgc
};

static void *debug_registration;

#ifdef __GNUC__
static void __wine_dbg_winedbgc32_init(void) __attribute__((constructor));
static void __wine_dbg_winedbgc32_fini(void) __attribute__((destructor));
#else
static void __asm__dummy_dll_init(void) {
asm("\t.section\t\".init\" ,\"ax\"\n"
    "\tcall ___wine_dbg_winedbgc32_init\n"
    "\t.section\t\".fini\" ,\"ax\"\n"
    "\tcall ___wine_dbg_winedbgc32_fini\n"
    "\t.section\t\".text\"\n");
}
#endif /* defined(__GNUC__) */

#ifdef __GNUC__
static
#endif
void __wine_dbg_winedbgc32_init(void)
{
//    extern void *__wine_dbg_register( char * const *, int );
//    debug_registration = __wine_dbg_register( debug_channels, 4 );
}

#ifdef __GNUC__
static
#endif
void __wine_dbg_winedbgc32_fini(void)
{
//    extern void __wine_dbg_unregister( void* );
//    __wine_dbg_unregister( debug_registration );
}
