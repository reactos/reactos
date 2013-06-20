#define _STLP_PLATFORM "Open BSD"

#define _STLP_USE_UNIX_IO

#if defined (_POSIX_THREADS) && !defined (_STLP_THREADS)
#  define _STLP_THREADS
#endif

#if defined (_POSIX_THREADS) && !defined (_STLP_DONT_USE_PTHREAD_SPINLOCK)
#  define _STLP_USE_PTHREAD_SPINLOCK
#  define _STLP_STATIC_MUTEX _STLP_mutex
#endif
