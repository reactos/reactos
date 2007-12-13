/* you can change this */
#undef DEBUG

/* but you better shouldn't change this */
#define CONFIG_FILE "linuxboot.cfg"

#define BUFFERSIZE 256 /* we have little stack */
#define CONFIG_BUFFERSIZE (BUFFERSIZE*16)

#ifdef DEBUG
#define dprintf printf
#else
#define dprintf
#endif

#ifdef DEBUG
#define splash
#define splash_init()
#else
#define splash show_splash
#define splash_init do_splash_init
#endif

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT_576 576
#define SCREEN_HEIGHT_480 480




/* i386 constants */

/* CR0 bit to enable paging */
#define CR0_ENABLE_PAGING		0x80000000
/* Size of a page on x86 */
#define PAGE_SIZE			4096

/* Size of the read chunks to use when reading the kernel; bigger = a lot faster */
#define READ_CHUNK_SIZE 128*1024

