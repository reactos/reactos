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

/* mem_trim() must trim the buffer without relocating it.
 * Since we can't do that, we just return the buffer passed in unchanged */
#define mem_trim(_m_, _s_) (_m_)

/* Unsigned int types */
typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned long u32_t;

/* Signed int types */
typedef signed char s8_t;
typedef signed short s16_t;
typedef signed long s32_t;

/* Memory pointer */
typedef ULONG_PTR mem_ptr_t;

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

