/* ReactOS-Specific lwIP binding header - by Cameron Gutman */

#include <wdm.h>

/* ROS-specific mem defs */
void *
malloc(size_t size);

void *
calloc(size_t count, size_t size);

void
free(void *mem);

void *
realloc(void *mem, size_t size);

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

