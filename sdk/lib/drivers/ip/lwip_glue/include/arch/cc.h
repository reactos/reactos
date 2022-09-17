/* ReactOS-Specific lwIP binding header - by Cameron Gutman */

#include <wdm.h>

/* We provide our own ntohs, etc. functions for now */
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS

/* ROS-specific mem defs */
void *
ros_malloc(size_t size);

void *
ros_calloc(size_t count, size_t size);

void
ros_free(void *mem);

#define mem_clib_malloc ros_malloc
#define mem_clib_calloc ros_calloc
#define mem_clib_free   ros_free


/* Unsigned int types */
typedef unsigned char u8_t;

/* Signed int types */
typedef signed char s8_t;
typedef signed short s16_t;

/* Printf/DPRINT formatters */
#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "lu"
#define S32_F "ld"
#define X32_F "lx"

/* Endianness */
#define BYTE_ORDER LITTLE_ENDIAN

/* Checksum calculation algorithm choice */
#define LWIP_CHKSUM_ALGORITHM 3

/* Diagnostics */
#define LWIP_PLATFORM_DIAG(x) (DbgPrint x)
#define LWIP_PLATFORM_ASSERT(x) ASSERTMSG(x, FALSE)

/* Synchronization */
#define SYS_ARCH_DECL_PROTECT(lev) sys_prot_t (lev)
#define SYS_ARCH_PROTECT(lev) sys_arch_protect(&(lev))
#define SYS_ARCH_UNPROTECT(lev) sys_arch_unprotect(lev)

/* Compiler hints for packing structures */
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_USE_INCLUDES

