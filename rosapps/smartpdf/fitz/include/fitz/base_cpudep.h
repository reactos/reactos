#if defined(ARCH_X86) || defined(ARCH_X86_64)
#  define HAVE_CPUDEP
#  define HAVE_MMX        (1<<0)
#  define HAVE_MMXEXT     (1<<1)
#  define HAVE_SSE        (1<<2)
#  define HAVE_SSE2       (1<<3)
#  define HAVE_SSE3       (1<<4)
#  define HAVE_3DNOW      (1<<5)
#  define HAVE_AMD64      (1<<6)

#elif defined (ARCH_PPC)
#  define HAVE_CPUDEP
#  define HAVE_ALTIVEC    (1<<7)

#elif defined (ARCH_SPARC)
#  define HAVE_CPUDEP
#  define HAVE_VIS        (1<<8)

#endif

/* call this before using fitz */
extern void fz_cpudetect();

/* treat as constant! */
extern unsigned fz_cpuflags;

