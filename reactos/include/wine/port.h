/* Missing Defines and structures 
 * These are either missing from the w32api package
 * the ReactOS build system is broken and needs to
 * fixed.
 */
#ifndef _ROS_WINE_PORT
#define _ROS_WINE_PORT

#define HFILE_ERROR ((HFILE)-1) /* Already in winbase.h - ros is fubar */


/* Wine debugging porting */

#define TRACE        DbgPrint
#define TRACE_(ch)   DbgPrint
#define TRACE_ON(ch) DbgPrint

#define WARN         DbgPrint
#define WARN_(ch)    DbgPrint
#define WARN_ON(ch)  DbgPrint

#define FIXME        DbgPrint
#define FIXME_(ch)   DbgPrint
#define FIXME_ON(ch) DbgPrint

#undef ERR  /* Solaris got an 'ERR' define in <sys/reg.h> */
#define ERR          DbgPrint
#define ERR_(ch)     DbgPrint
#define ERR_ON(ch)   DbgPrint


/* some useful string manipulation routines */

static inline unsigned int strlenW( const WCHAR *str )
{
#if defined(__i386__) && defined(__GNUC__)
    int dummy, res;
    __asm__ __volatile__( "cld\n\t"
                          "repnz\n\t"
                          "scasw\n\t"
                          "notl %0"
                          : "=c" (res), "=&D" (dummy)
                          : "0" (0xffffffff), "1" (str), "a" (0) );
    return res - 1;
#else
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
#endif
}

#endif /* _ROS_WINE_PORT */
