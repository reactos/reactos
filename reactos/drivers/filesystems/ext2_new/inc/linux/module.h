/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             Modules.h
 * PURPOSE:          Header file: nls structures & linux kernel ...
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY:
 */

#ifndef _EXT2_MODULE_HEADER_
#define _EXT2_MODULE_HEADER_

/* INCLUDES *************************************************************/

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/log2.h>
#include <linux/rbtree.h>

#if _WIN32_WINNT <= 0x500
#define _WIN2K_TARGET_ 1
#endif

/* STRUCTS ******************************************************/

#ifndef offsetof
# define offsetof(type, member) ((ULONG_PTR)&(((type *)0)->member))
#endif

#ifndef container_of
#define container_of(ptr, type, member)                  \
                ((type *)((char *)ptr - (char *)offsetof(type, member)))
#endif

//
// Byte order swapping routines
//

/* use the runtime routine or compiler's implementation */
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || \
    ((defined(_M_AMD64) || defined(_M_IA64)) &&         \
     (_MSC_FULL_VER > 13009175))
#ifdef __cplusplus
extern "C" {
#endif
    unsigned short __cdecl _byteswap_ushort(unsigned short);
    unsigned long  __cdecl _byteswap_ulong (unsigned long);
    unsigned __int64 __cdecl _byteswap_uint64(unsigned __int64);
#ifdef __cplusplus
}
#endif
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)

#define RtlUshortByteSwap(_x)    _byteswap_ushort((USHORT)(_x))
#define RtlUlongByteSwap(_x)     _byteswap_ulong((_x))
#define RtlUlonglongByteSwap(_x) _byteswap_uint64((_x))

#elif !defined(__REACTOS__)

USHORT
FASTCALL
RtlUshortByteSwap(
    IN USHORT Source
);

ULONG
FASTCALL
RtlUlongByteSwap(
    IN ULONG Source
);

ULONGLONG
FASTCALL
RtlUlonglongByteSwap(
    IN ULONGLONG Source
);
#endif

#define __swab16(x) RtlUshortByteSwap(x)
#define __swab32(x) RtlUlongByteSwap(x)
#define __swab64(x) RtlUlonglongByteSwap(x)

#define __constant_swab32  __swab32
#define __constant_swab64  __swab64

#define __constant_htonl(x) __constant_swab32((x))
#define __constant_ntohl(x) __constant_swab32((x))
#define __constant_htons(x) __constant_swab16((x))
#define __constant_ntohs(x) __constant_swab16((x))
#define __constant_cpu_to_le64(x) ((__u64)(x))
#define __constant_le64_to_cpu(x) ((__u64)(x))
#define __constant_cpu_to_le32(x) ((__u32)(x))
#define __constant_le32_to_cpu(x) ((__u32)(x))
#define __constant_cpu_to_le16(x) ((__u16)(x))
#define __constant_le16_to_cpu(x) ((__u16)(x))
#define __constant_cpu_to_be64(x) __constant_swab64((x))
#define __constant_be64_to_cpu(x) __constant_swab64((x))
#define __constant_cpu_to_be32(x) __constant_swab32((x))
#define __constant_be32_to_cpu(x) __constant_swab32((x))
#define __constant_cpu_to_be16(x) __constant_swab16((x))
#define __constant_be16_to_cpu(x) __constant_swab16((x))
#define __cpu_to_le64(x) ((__u64)(x))
#define __le64_to_cpu(x) ((__u64)(x))
#define __cpu_to_le32(x) ((__u32)(x))
#define __le32_to_cpu(x) ((__u32)(x))
#define __cpu_to_le16(x) ((__u16)(x))
#define __le16_to_cpu(x) ((__u16)(x))
#define __cpu_to_be64(x) __swab64((x))
#define __be64_to_cpu(x) __swab64((x))
#define __cpu_to_be32(x) __swab32((x))
#define __be32_to_cpu(x) __swab32((x))
#define __cpu_to_be16(x) __swab16((x))
#define __be16_to_cpu(x) __swab16((x))
#define __cpu_to_le64p(x) (*(__u64*)(x))
#define __le64_to_cpup(x) (*(__u64*)(x))
#define __cpu_to_le32p(x) (*(__u32*)(x))
#define __le32_to_cpup(x) (*(__u32*)(x))
#define __cpu_to_le16p(x) (*(__u16*)(x))
#define __le16_to_cpup(x) (*(__u16*)(x))
#define __cpu_to_be64p(x) __swab64p((x))
#define __be64_to_cpup(x) __swab64p((x))
#define __cpu_to_be32p(x) __swab32p((x))
#define __be32_to_cpup(x) __swab32p((x))
#define __cpu_to_be16p(x) __swab16p((x))
#define __be16_to_cpup(x) __swab16p((x))
#define __cpu_to_le64s(x) ((__s64)(x))
#define __le64_to_cpus(x) ((__s64)(x))
#define __cpu_to_le32s(x) ((__s32)(x))
#define __le32_to_cpus(x) ((__s32)(x))
#define __cpu_to_le16s(x) ((__s16)(x))
#define __le16_to_cpus(x) ((__s16)(x))
#define __cpu_to_be64s(x) __swab64s((x))
#define __be64_to_cpus(x) __swab64s((x))
#define __cpu_to_be32s(x) __swab32s((x))
#define __be32_to_cpus(x) __swab32s((x))
#define __cpu_to_be16s(x) __swab16s((x))
#define __be16_to_cpus(x) __swab16s((x))

#ifndef cpu_to_le64
#define cpu_to_le64 __cpu_to_le64
#define le64_to_cpu __le64_to_cpu
#define cpu_to_le32 __cpu_to_le32
#define le32_to_cpu __le32_to_cpu
#define cpu_to_le16 __cpu_to_le16
#define le16_to_cpu __le16_to_cpu
#endif

#define cpu_to_be64 __cpu_to_be64
#define be64_to_cpu __be64_to_cpu
#define cpu_to_be32 __cpu_to_be32
#define be32_to_cpu __be32_to_cpu
#define cpu_to_be16 __cpu_to_be16
#define be16_to_cpu __be16_to_cpu
#define cpu_to_le64p __cpu_to_le64p
#define le64_to_cpup __le64_to_cpup
#define cpu_to_le32p __cpu_to_le32p
#define le32_to_cpup __le32_to_cpup
#define cpu_to_le16p __cpu_to_le16p
#define le16_to_cpup __le16_to_cpup
#define cpu_to_be64p __cpu_to_be64p
#define be64_to_cpup __be64_to_cpup
#define cpu_to_be32p __cpu_to_be32p
#define be32_to_cpup __be32_to_cpup
#define cpu_to_be16p __cpu_to_be16p
#define be16_to_cpup __be16_to_cpup
#define cpu_to_le64s __cpu_to_le64s
#define le64_to_cpus __le64_to_cpus
#define cpu_to_le32s __cpu_to_le32s
#define le32_to_cpus __le32_to_cpus
#define cpu_to_le16s __cpu_to_le16s
#define le16_to_cpus __le16_to_cpus
#define cpu_to_be64s __cpu_to_be64s
#define be64_to_cpus __be64_to_cpus
#define cpu_to_be32s __cpu_to_be32s
#define be32_to_cpus __be32_to_cpus
#define cpu_to_be16s __cpu_to_be16s
#define be16_to_cpus __be16_to_cpus


static inline void le16_add_cpu(__le16 *var, u16 val)
{
	*var = cpu_to_le16(le16_to_cpu(*var) + val);
}

static inline void le32_add_cpu(__le32 *var, u32 val)
{
	*var = cpu_to_le32(le32_to_cpu(*var) + val);
}

static inline void le64_add_cpu(__le64 *var, u64 val)
{
	*var = cpu_to_le64(le64_to_cpu(*var) + val);
}

//
// Network to host byte swap functions
//

#define ntohl(x)           ( ( ( ( x ) & 0x000000ff ) << 24 ) | \
                             ( ( ( x ) & 0x0000ff00 ) << 8 ) | \
                             ( ( ( x ) & 0x00ff0000 ) >> 8 ) | \
                             ( ( ( x ) & 0xff000000 ) >> 24 )   )

#define ntohs(x)           ( ( ( ( x ) & 0xff00 ) >> 8 ) | \
                             ( ( ( x ) & 0x00ff ) << 8 ) )


#define htonl(x)           ntohl(x)
#define htons(x)           ntohs(x)


//
// kernel printk flags
//

#define KERN_EMERG      "<0>"   /* system is unusable                   */
#define KERN_ALERT      "<1>"   /* action must be taken immediately     */
#define KERN_CRIT       "<2>"   /* critical conditions                  */
#define KERN_ERR        "<3>"   /* error conditions                     */
#define KERN_WARNING    "<4>"   /* warning conditions                   */
#define KERN_NOTICE     "<5>"   /* normal but significant condition     */
#define KERN_INFO       "<6>"   /* informational                        */
#define KERN_DEBUG      "<7>"   /* debug-level messages                 */

#define printk  DbgPrint

/*
 * error pointer
 */
#define MAX_ERRNO	4095
#define IS_ERR_VALUE(x) ((x) >= (unsigned long)-MAX_ERRNO)

static inline void *ERR_PTR(long error)
{
	return (void *)(long_ptr_t) error;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long)(long_ptr_t) ptr;
}

static inline long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)(long_ptr_t)ptr);
}


#define BUG_ON(c) assert(!(c))

#define WARN_ON(c) BUG_ON(c)

//
// Linux module definitions
//

#define likely
#define unlikely

#define __init
#define __exit

#define THIS_MODULE NULL
#define MODULE_LICENSE(x)
#define MODULE_ALIAS_NLS(x)
#define EXPORT_SYMBOL(x)


#define try_module_get(x) (TRUE)
#define module_put(x)

#define module_init(X) int  __init module_##X() {return X();}
#define module_exit(X) void __exit module_##X() {X();}

#define DECLARE_INIT(X) int  __init  module_##X(void)
#define DECLARE_EXIT(X) void __exit  module_##X(void)

#define LOAD_MODULE(X) do {                             \
                            rc = module_##X();          \
                       } while(0)

#define UNLOAD_MODULE(X) do {                           \
                            module_##X();               \
                         } while(0)

#define LOAD_NLS    LOAD_MODULE
#define UNLOAD_NLS  UNLOAD_MODULE

//
// spinlocks .....
//

typedef struct _spinlock_t {

    KSPIN_LOCK  lock;
    KIRQL       irql;
} spinlock_t;

#define spin_lock_init(sl)    KeInitializeSpinLock(&((sl)->lock))
#define spin_lock(sl)         KeAcquireSpinLock(&((sl)->lock), &((sl)->irql))
#define spin_unlock(sl)       KeReleaseSpinLock(&((sl)->lock), (sl)->irql)
#define spin_lock_irqsave(sl, flags) do {spin_lock(sl); flags=(sl)->irql;} while(0)
#define spin_unlock_irqrestore(sl, flags) do {ASSERT((KIRQL)(flags)==(sl)->irql); spin_unlock(sl);} while(0)

#define assert_spin_locked(x)   do {} while(0)

/*
 * Does a critical section need to be broken due to another
 * task waiting?: (technically does not depend on CONFIG_PREEMPT,
 * but a general need for low latency)
 */
static inline int spin_needbreak(spinlock_t *lock)
{
#ifdef CONFIG_PREEMPT
    return spin_is_contended(lock);
#else
    return 0;
#endif
}

//
// bit operations
//

/**
 * __set_bit - Set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * Unlike set_bit(), this function is non-atomic and may be reordered.
 * If it's called on the same region of memory simultaneously, the effect
 * may be that only one operation succeeds.
 */
static inline int set_bit(int nr, volatile unsigned long *addr)
{
    addr += (nr >> ORDER_PER_LONG);
    nr &= (BITS_PER_LONG - 1);

    return !!(InterlockedOr(addr, (1 << nr)) & (1 << nr));
}


/**
 * clear_bit - Clears a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * clear_bit() is atomic and may not be reordered.  However, it does
 * not contain a memory barrier, so if it is used for locking purposes,
 * you should call smp_mb__before_clear_bit() and/or smp_mb__after_clear_bit()
 * in order to ensure changes are visible on other processors.
 */
static inline int clear_bit(int nr, volatile unsigned long *addr)
{
    addr += (nr >> ORDER_PER_LONG);
    nr &= (BITS_PER_LONG - 1);

    return !!(InterlockedAnd(addr, ~(1 << nr)) & (1 << nr));
}

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static inline int test_and_clear_bit(int nr, volatile unsigned long *addr)
{
    return clear_bit(nr, addr);
}

/*
 *  test
 */
static int test_bit(int nr, volatile const unsigned long *addr)
{
    return !!((1 << (nr & (BITS_PER_LONG - 1))) &
              (addr[nr >> ORDER_PER_LONG]));
}

/**
 * test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static inline int test_and_set_bit(int nr, volatile unsigned long *addr)
{
    return set_bit(nr, addr);
}

//
// list definition ...
//

#include <linux/list.h>


/*********************************************
 *  linux scheduler related structures      *
*********************************************/

//
// task structure
//

#define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2

struct task_struct {
    pid_t pid;
    pid_t tid;
    char comm[32];
    void * journal_info;
};

extern struct task_struct *current;

//
// scheduler routines
//


static inline int cond_resched() {
    return FALSE;
}
static inline int need_resched() {
    return FALSE;
}

#define yield()        do {} while(0)
#define might_sleep()  do {} while(0)

//
// mutex
//

typedef struct mutex {
    FAST_MUTEX  lock;
} mutex_t;

#define mutex_init(x)   ExInitializeFastMutex(&((x)->lock))
#define mutex_lock(x)   ExAcquireFastMutex(&((x)->lock))
#define mutex_unlock(x) ExReleaseFastMutex(&((x)->lock))


//
// wait_queue
//


typedef PVOID wait_queue_t;

#define WQ_FLAG_EXCLUSIVE	    0x01
#define WQ_FLAG_AUTO_REMOVAL	0x02

struct __wait_queue {
    unsigned int    flags;
    void *          private;
    KEVENT          event;
    struct list_head task_list;
};


#define DEFINE_WAIT(name) \
	wait_queue_t name = (PVOID)wait_queue_create();

/*
struct wait_bit_key {
	void *flags;
	int bit_nr;
};

struct wait_bit_queue {
	struct wait_bit_key key;
	wait_queue_t wait;
};
*/

struct __wait_queue_head {
    spinlock_t lock;
    struct list_head task_list;
};
typedef struct __wait_queue_head wait_queue_head_t;

#define is_sync_wait(wait)  (TRUE)
#define set_current_state(state) do {} while(0)
#define __set_current_state(state)  do {} while(0)

void init_waitqueue_head(wait_queue_head_t *q);
int wake_up(wait_queue_head_t *queue);


/*
 * Waitqueues which are removed from the waitqueue_head at wakeup time
 */
struct __wait_queue * wait_queue_create();
void wait_queue_destroy(struct __wait_queue *);

void prepare_to_wait(wait_queue_head_t *q, wait_queue_t *wait, int state);
void prepare_to_wait_exclusive(wait_queue_head_t *q, wait_queue_t *wait, int state);
void finish_wait(wait_queue_head_t *q, wait_queue_t *wait);
int autoremove_wake_function(wait_queue_t *wait, unsigned mode, int sync, void *key);
int wake_bit_function(wait_queue_t *wait, unsigned mode, int sync, void *key);


//
// timer structure
//

struct timer_list {
    struct list_head entry;
    unsigned long expires;

    void (*function)(unsigned long);
    unsigned long data;

#ifdef CONFIG_TIMER_STATS
    void *start_site;
    char start_comm[16];
    int start_pid;
#endif
};


typedef struct kmem_cache kmem_cache_t;

struct block_device {

    unsigned long           bd_flags;   /* flags */
    atomic_t		        bd_count;   /* reference count */
    PDEVICE_OBJECT          bd_dev;     /* device object */
    ANSI_STRING             bd_name;    /* name in ansi string */
    DISK_GEOMETRY           bd_geo;     /* disk geometry */
    PARTITION_INFORMATION   bd_part;    /* partition information */
    void *                  bd_priv;    /* pointers to EXT2_VCB
                                           NULL if it's a journal dev */
    PFILE_OBJECT            bd_volume;  /* streaming object file */
    LARGE_MCB               bd_extents; /* dirty extents */

    spinlock_t              bd_bh_lock;    /**/
    kmem_cache_t *          bd_bh_cache;   /* memory cache for buffer_head */
    struct rb_root          bd_bh_root;    /* buffer_head red-black tree root */
};

//
// page information
//

// vom trata paginile in felul urmator:
// alocam la sfarsitul structurii inca PAGE_SIZE octeti cand alocam o structura
// de tip pagina - acolo vor veni toate buffer-headurile
// deci -> page_address(page) = page + sizeof(page)
#define page_address(_page) ((char*)_page + sizeof(struct page))

typedef struct page {
    void           *addr;
    void           *mapping;
    void           *private;
    atomic_t        count;
    __u32           index;
    __u32           flags;
} mem_map_t;

#define get_page(p) atomic_inc(&(p)->count)

#define PG_locked		 0	/* Page is locked. Don't touch. */
#define PG_error		 1
#define PG_referenced		 2
#define PG_uptodate		 3
#define PG_dirty		 4
#define PG_unused		 5
#define PG_lru			 6
#define PG_active		 7
#define PG_slab			 8
#define PG_skip			10
#define PG_highmem		11
#define PG_checked		12	/* kill me in 2.5.<early>. */
#define PG_arch_1		13
#define PG_reserved		14
#define PG_launder		15	/* written out by VM pressure.. */
#define PG_fs_1			16	/* Filesystem specific */

#ifndef arch_set_page_uptodate
#define arch_set_page_uptodate(page)
#endif

/* Make it prettier to test the above... */
#define UnlockPage(page)        unlock_page(page)
#define Page_Uptodate(page)     test_bit(PG_uptodate, &(page)->flags)
#define SetPageUptodate(page) \
	do {								\
		arch_set_page_uptodate(page);				\
		set_bit(PG_uptodate, &(page)->flags);			\
	} while (0)
#define ClearPageUptodate(page) clear_bit(PG_uptodate, &(page)->flags)
#define PageDirty(page)         test_bit(PG_dirty, &(page)->flags)
#define SetPageDirty(page)      set_bit(PG_dirty, &(page)->flags)
#define ClearPageDirty(page)    clear_bit(PG_dirty, &(page)->flags)
#define PageLocked(page)        test_bit(PG_locked, &(page)->flags)
#define LockPage(page)          set_bit(PG_locked, &(page)->flags)
#define TryLockPage(page)       test_and_set_bit(PG_locked, &(page)->flags)
#define PageChecked(page)       test_bit(PG_checked, &(page)->flags)
#define SetPageChecked(page)    set_bit(PG_checked, &(page)->flags)
#define ClearPageChecked(page)  clear_bit(PG_checked, &(page)->flags)
#define PageLaunder(page)       test_bit(PG_launder, &(page)->flags)
#define SetPageLaunder(page)    set_bit(PG_launder, &(page)->flags)
#define ClearPageLaunder(page)  clear_bit(PG_launder, &(page)->flags)
#define ClearPageArch1(page)    clear_bit(PG_arch_1, &(page)->flags)

#define PageError(page)		test_bit(PG_error, &(page)->flags)
#define SetPageError(page)	set_bit(PG_error, &(page)->flags)
#define ClearPageError(page)	clear_bit(PG_error, &(page)->flags)
#define PageReferenced(page)    test_bit(PG_referenced, &(page)->flags)
#define SetPageReferenced(page) set_bit(PG_referenced, &(page)->flags)
#define ClearPageReferenced(page)       clear_bit(PG_referenced, &(page)->flags)

#define PageActive(page)        test_bit(PG_active, &(page)->flags)
#define SetPageActive(page)     set_bit(PG_active, &(page)->flags)
#define ClearPageActive(page)   clear_bit(PG_active, &(page)->flags)


extern unsigned long __get_free_pages(unsigned int gfp_mask, unsigned int order);
#define __get_free_page(gfp_mask) \
		__get_free_pages((gfp_mask),0)

extern void __free_pages(struct page *page, unsigned int order);
extern void free_pages(unsigned long addr, unsigned int order);

#define __free_page(page) __free_pages((page), 0)
#define free_page(addr) free_pages((addr),0)

#ifndef __REACTOS__
extern void truncate_inode_pages(struct address_space *, loff_t);
#endif

#define __GFP_HIGHMEM   0x02

#define __GFP_WAIT	0x10	/* Can wait and reschedule? */
#define __GFP_HIGH	0x20	/* Should access emergency pools? */
#define __GFP_IO	0x40	/* Can start low memory physical IO? */
#define __GFP_HIGHIO	0x80	/* Can start high mem physical IO? */
#define __GFP_FS	0x100	/* Can call down to low-level FS? */

#define GFP_ATOMIC	(__GFP_HIGH)
#define GFP_USER	(             __GFP_WAIT | __GFP_IO | __GFP_HIGHIO | __GFP_FS)
#define GFP_HIGHUSER    (             __GFP_WAIT | __GFP_IO | __GFP_HIGHIO | __GFP_FS | __GFP_HIGHMEM)
#define GFP_KERNEL	(__GFP_HIGH | __GFP_WAIT | __GFP_IO | __GFP_HIGHIO | __GFP_FS)
#define GFP_NOFS    0
#define __GFP_NOFAIL 0


#define KM_USER0 0

//
// buffer head definitions
//

enum bh_state_bits {
    BH_Uptodate,	        /* Contains valid data */
    BH_Dirty,	            /* Is dirty */
    BH_Verified,	 /* Is verified */
    BH_Lock,	            /* Is locked */
    BH_Req,		            /* Has been submitted for I/O */
    BH_Uptodate_Lock,       /* Used by the first bh in a page, to serialise
			                 * IO completion of other buffers in the page
			                 */

    BH_Mapped,	            /* Has a disk mapping */
    BH_New,		            /* Disk mapping was newly created by get_block */
    BH_Async_Read,	        /* Is under end_buffer_async_read I/O */
    BH_Async_Write,	        /* Is under end_buffer_async_write I/O */
    BH_Delay,	            /* Buffer is not yet allocated on disk */
    BH_Boundary,	        /* Block is followed by a discontiguity */
    BH_Write_EIO,	        /* I/O error on write */
    BH_Ordered,	            /* ordered write */
    BH_Eopnotsupp,	        /* operation not supported (barrier) */
    BH_Unwritten,	        /* Buffer is allocated on disk but not written */

    BH_PrivateStart,        /* not a state bit, but the first bit available
			                 * for private allocation by other entities
			                 */
};

#define PAGE_CACHE_SIZE  (PAGE_SIZE)
#define PAGE_CACHE_SHIFT (12)
#define MAX_BUF_PER_PAGE (PAGE_CACHE_SIZE / 512)

#ifdef __REACTOS__
struct buffer_head;
#endif
typedef void (bh_end_io_t)(struct buffer_head *bh, int uptodate);

/*
 * Historically, a buffer_head was used to map a single block
 * within a page, and of course as the unit of I/O through the
 * filesystem and block layers.  Nowadays the basic I/O unit
 * is the bio, and buffer_heads are used for extracting block
 * mappings (via a get_block_t call), for tracking state within
 * a page (via a page_mapping) and for wrapping bio submission
 * for backward compatibility reasons (e.g. submit_bh).
 */
struct buffer_head {
    unsigned long b_state;		            /* buffer state bitmap (see above) */
    struct page *b_page;                    /* the page this bh is mapped to */
    PMDL         b_mdl;                     /* MDL of the locked buffer */
    void	*b_bcb;			    /* BCB of the buffer */

    // kdev_t b_dev;                        /* device (B_FREE = free) */
    struct block_device *b_bdev;            /* block device object */

    blkcnt_t b_blocknr;		        /* start block number */
    size_t        b_size;			        /* size of mapping */
    char *        b_data;			        /* pointer to data within the page */
    bh_end_io_t *b_end_io;		        /* I/O completion */
    void *b_private;		                /* reserved for b_end_io */
    // struct list_head b_assoc_buffers;    /* associated with another mapping */
    // struct address_space *b_assoc_map;   /* mapping this buffer is associated with */
    atomic_t b_count;		                /* users using this buffer_head */
    struct rb_node b_rb_node;                /* Red-black tree node entry */
};


/*
 * macro tricks to expand the set_buffer_foo(), clear_buffer_foo()
 * and buffer_foo() functions.
 */
#define BUFFER_FNS(bit, name)						\
static inline void set_buffer_##name(struct buffer_head *bh)		\
{									\
	set_bit(BH_##bit, &(bh)->b_state);				\
}									\
static inline void clear_buffer_##name(struct buffer_head *bh)		\
{									\
	clear_bit(BH_##bit, &(bh)->b_state);				\
}									\
static inline int buffer_##name(const struct buffer_head *bh)		\
{									\
	return test_bit(BH_##bit, &(bh)->b_state);			\
}

/*
 * test_set_buffer_foo() and test_clear_buffer_foo()
 */
#define TAS_BUFFER_FNS(bit, name)					\
static inline int test_set_buffer_##name(struct buffer_head *bh)	\
{									\
	return test_and_set_bit(BH_##bit, &(bh)->b_state);		\
}									\
static inline int test_clear_buffer_##name(struct buffer_head *bh)	\
{									\
	return test_and_clear_bit(BH_##bit, &(bh)->b_state);		\
}									\
 
/*
 * Emit the buffer bitops functions.   Note that there are also functions
 * of the form "mark_buffer_foo()".  These are higher-level functions which
 * do something in addition to setting a b_state bit.
 */
BUFFER_FNS(Uptodate, uptodate)
BUFFER_FNS(Dirty, dirty)
TAS_BUFFER_FNS(Dirty, dirty)
BUFFER_FNS(Verified, verified)
BUFFER_FNS(Lock, locked)
TAS_BUFFER_FNS(Lock, locked)
BUFFER_FNS(Req, req)
TAS_BUFFER_FNS(Req, req)
BUFFER_FNS(Mapped, mapped)
BUFFER_FNS(New, new)
BUFFER_FNS(Async_Read, async_read)
BUFFER_FNS(Async_Write, async_write)
BUFFER_FNS(Delay, delay)
BUFFER_FNS(Boundary, boundary)
BUFFER_FNS(Write_EIO, write_io_error)
BUFFER_FNS(Ordered, ordered)
BUFFER_FNS(Eopnotsupp, eopnotsupp)
BUFFER_FNS(Unwritten, unwritten)

#define bh_offset(bh)		((unsigned long)(bh)->b_data & ~PAGE_MASK)
#define touch_buffer(bh)	mark_page_accessed(bh->b_page)

/* If we *know* page->private refers to buffer_heads */

#define page_buffers(page)					\
	(                                       \
		BUG_ON(!PagePrivate(page)),			\
		((struct buffer_head *)page_private(page))	\
	)
#define page_has_buffers(page)	PagePrivate(page)


/*
 * Declarations
 */

void mark_buffer_dirty(struct buffer_head *bh);
void init_buffer(struct buffer_head *, bh_end_io_t *, void *);
void set_bh_page(struct buffer_head *bh,
                 struct page *page, unsigned long offset);
int try_to_free_buffers(struct page *);
struct buffer_head *alloc_page_buffers(struct page *page, unsigned long size,
                                                   int retry);
void create_empty_buffers(struct page *, unsigned long,
                          unsigned long b_state);

/* Things to do with buffers at mapping->private_list */
void mark_buffer_dirty_inode(struct buffer_head *bh, struct inode *inode);
int inode_has_buffers(struct inode *);
void invalidate_inode_buffers(struct inode *);
int remove_inode_buffers(struct inode *inode);
#ifndef __REACTOS__
int sync_mapping_buffers(struct address_space *mapping);
#endif
void unmap_underlying_metadata(struct block_device *bdev, sector_t block);

void mark_buffer_async_write(struct buffer_head *bh);
void invalidate_bdev(struct block_device *);
int sync_blockdev(struct block_device *bdev);
void __wait_on_buffer(struct buffer_head *);
wait_queue_head_t *bh_waitq_head(struct buffer_head *bh);
int fsync_bdev(struct block_device *);
struct super_block *freeze_bdev(struct block_device *);
void thaw_bdev(struct block_device *, struct super_block *);
int fsync_super(struct super_block *);
int fsync_no_super(struct block_device *);
struct buffer_head *__find_get_block(struct block_device *bdev, sector_t block,
                                                 unsigned long size);
struct buffer_head *get_block_bh(struct block_device *bdev, sector_t block,
                                 unsigned long size, int zero);
struct buffer_head *__getblk(struct block_device *bdev, sector_t block,
                                         unsigned long size);
void __brelse(struct buffer_head *);
void __bforget(struct buffer_head *);
void __breadahead(struct block_device *, sector_t block, unsigned int size);
struct buffer_head *__bread(struct block_device *, sector_t block, unsigned size);
void invalidate_bh_lrus(void);
struct buffer_head *alloc_buffer_head(gfp_t gfp_flags);
void free_buffer_head(struct buffer_head * bh);
void unlock_buffer(struct buffer_head *bh);
void __lock_buffer(struct buffer_head *bh);
void ll_rw_block(int, int, struct buffer_head * bh[]);
int sync_dirty_buffer(struct buffer_head *bh);
int submit_bh(int, struct buffer_head *);
void write_boundary_block(struct block_device *bdev,
                          sector_t bblock, unsigned blocksize);
int bh_uptodate_or_lock(struct buffer_head *bh);
int bh_submit_read(struct buffer_head *bh);
/* They are separately managed  */
struct buffer_head *extents_bread(struct super_block *sb, sector_t block);
struct buffer_head *extents_bwrite(struct super_block *sb, sector_t block);
void extents_mark_buffer_dirty(struct buffer_head *bh);
void extents_brelse(struct buffer_head *bh);

extern int buffer_heads_over_limit;

/*
 * Generic address_space_operations implementations for buffer_head-backed
 * address_spaces.
 */

#if 0

int block_write_full_page(struct page *page, get_block_t *get_block,
                          struct writeback_control *wbc);
int block_read_full_page(struct page*, get_block_t*);
int block_write_begin(struct file *, struct address_space *,
                      loff_t, unsigned, unsigned,
                      struct page **, void **, get_block_t*);
int block_write_end(struct file *, struct address_space *,
                    loff_t, unsigned, unsigned,
                    struct page *, void *);
int generic_write_end(struct file *, struct address_space *,
                      loff_t, unsigned, unsigned,
                      struct page *, void *);

int block_prepare_write(struct page*, unsigned, unsigned, get_block_t*);
int cont_write_begin(struct file *, struct address_space *, loff_t,
                     unsigned, unsigned, struct page **, void **,
                     get_block_t *, loff_t *);
int block_page_mkwrite(struct vm_area_struct *vma, struct page *page,
                       get_block_t get_block);
sector_t generic_block_bmap(struct address_space *, sector_t, get_block_t *);
int generic_commit_write(struct file *, struct page *, unsigned, unsigned);
int block_truncate_page(struct address_space *, loff_t, get_block_t *);
int file_fsync(struct file *, struct dentry *, int);
int nobh_write_begin(struct file *, struct address_space *,
                     loff_t, unsigned, unsigned,
                     struct page **, void **, get_block_t*);
int nobh_write_end(struct file *, struct address_space *,
                   loff_t, unsigned, unsigned,
                   struct page *, void *);
int nobh_truncate_page(struct address_space *, loff_t, get_block_t *);
int nobh_writepage(struct page *page, get_block_t *get_block,
                   struct writeback_control *wbc);
int generic_cont_expand_simple(struct inode *inode, loff_t size);
#endif

void block_invalidatepage(struct page *page, unsigned long offset);
void page_zero_new_buffers(struct page *page, unsigned from, unsigned to);
int  block_commit_write(struct page *page, unsigned from, unsigned to);
void block_sync_page(struct page *);

void buffer_init(void);

/*
 * inline definitions
 */
#if 0
static inline void attach_page_buffers(struct page *page,
                                       struct buffer_head *head)
{
    page_cache_get(page);
    SetPagePrivate(page);
    set_page_private(page, (unsigned long)head);
}
#endif

static inline void get_bh(struct buffer_head *bh)
{
    atomic_inc(&bh->b_count);
}

static inline void put_bh(struct buffer_head *bh)
{
    if (bh)
        __brelse(bh);
}

static inline void brelse(struct buffer_head *bh)
{
    if (bh)
        __brelse(bh);
}

static inline void bforget(struct buffer_head *bh)
{
    if (bh)
        __bforget(bh);
}

static inline struct buffer_head *
            sb_getblk(struct super_block *sb, sector_t block)
{
    return get_block_bh(sb->s_bdev, block, sb->s_blocksize, 0);
}

static inline struct buffer_head *
            sb_getblk_zero(struct super_block *sb, sector_t block)
{
    return get_block_bh(sb->s_bdev, block, sb->s_blocksize, 1);
}

static inline struct buffer_head *
            sb_bread(struct super_block *sb, sector_t block)
{
    struct buffer_head *bh = __getblk(sb->s_bdev, block, sb->s_blocksize);
    if (!bh)
	    return NULL;
    if (!buffer_uptodate(bh) && (bh_submit_read(bh) < 0)) {
        brelse(bh);
	return NULL;
    }
    return bh;
}

static inline struct buffer_head *
            sb_find_get_block(struct super_block *sb, sector_t block)
{
    return __find_get_block(sb->s_bdev, block, sb->s_blocksize);
}

static inline void
map_bh(struct buffer_head *bh, struct super_block *sb, sector_t block)
{
    set_buffer_mapped(bh);
    bh->b_bdev = sb->s_bdev;
    bh->b_blocknr = block;
    bh->b_size = sb->s_blocksize;
}

/*
 * Calling wait_on_buffer() for a zero-ref buffer is illegal, so we call into
 * __wait_on_buffer() just to trip a debug check.  Because debug code in inline
 * functions is bloaty.
 */

static inline void wait_on_buffer(struct buffer_head *bh)
{
    might_sleep();
    if (buffer_locked(bh) || atomic_read(&bh->b_count) == 0)
        __wait_on_buffer(bh);
}

static inline void lock_buffer(struct buffer_head *bh)
{
    might_sleep();
    if (test_set_buffer_locked(bh))
        __lock_buffer(bh);
}

extern int __set_page_dirty_buffers(struct page *page);

//
// unicode character
//

struct nls_table {
    char *charset;
    char *alias;
    int (*uni2char) (wchar_t uni, unsigned char *out, int boundlen);
    int (*char2uni) (const unsigned char *rawstring, int boundlen,
                     wchar_t *uni);
    unsigned char *charset2lower;
    unsigned char *charset2upper;
    struct module *owner;
    struct nls_table *next;
};

/* this value hold the maximum octet of charset */
#define NLS_MAX_CHARSET_SIZE 6 /* for UTF-8 */

/* nls.c */
extern int register_nls(struct nls_table *);
extern int unregister_nls(struct nls_table *);
extern struct nls_table *load_nls(char *);
extern void unload_nls(struct nls_table *);
extern struct nls_table *load_nls_default(void);

extern int utf8_mbtowc(wchar_t *, const __u8 *, int);
extern int utf8_mbstowcs(wchar_t *, const __u8 *, int);
extern int utf8_wctomb(__u8 *, wchar_t, int);
extern int utf8_wcstombs(__u8 *, const wchar_t *, int);

//
//  kernel jiffies
//

#define HZ  (100)

static inline __u32 JIFFIES()
{
    LARGE_INTEGER Tick;

    KeQueryTickCount(&Tick);
    Tick.QuadPart *= KeQueryTimeIncrement();
    Tick.QuadPart /= (10000000 / HZ);

    return Tick.LowPart;
}

#define jiffies JIFFIES()

//
// memory routines
//

#ifdef _WIN2K_TARGET_

typedef GUID UUID;
NTKERNELAPI
NTSTATUS
ExUuidCreate(
    OUT UUID *Uuid
);

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
);

#define  ExFreePoolWithTag(_P, _T) ExFreePool(_P)
#endif

PVOID Ext2AllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
);

VOID
Ext2FreePool(
    IN PVOID P,
    IN ULONG Tag
);

void *kzalloc(int size, int flags);
#define kmalloc(size, gfp) Ext2AllocatePool(NonPagedPool, size, 'JBDM')
#define kfree(p) Ext2FreePool(p, 'JBDM')


/* memory slab */

#define	SLAB_HWCACHE_ALIGN	0x00002000U	/* align objs on a h/w cache lines */
#define SLAB_KERNEL         0x00000001U
#define SLAB_TEMPORARY      0x00000002U

typedef void (*kmem_cache_cb_t)(void*, kmem_cache_t *, unsigned long);

struct kmem_cache {
    CHAR                    name[32];
    ULONG                   flags;
    ULONG                   size;
    atomic_t                count;
    atomic_t                acount;
    NPAGED_LOOKASIDE_LIST   la;
    kmem_cache_cb_t         constructor;
};


kmem_cache_t *
kmem_cache_create(
    const char *name,
    size_t size,
    size_t offset,
    unsigned long flags,
    kmem_cache_cb_t ctor
);

void* kmem_cache_alloc(kmem_cache_t *kc, int flags);
void  kmem_cache_free(kmem_cache_t *kc, void *p);
int   kmem_cache_destroy(kmem_cache_t *kc);


//
// block device
//

#define BDEVNAME_SIZE      32      /* Largest string for a blockdev identifier */

//
// ll_rw_block ....
//


#define RW_MASK         1
#define RWA_MASK        2
#define READ 0
#define WRITE 1
#define READA 2         /* read-ahead  - don't block if no resources */
#define SWRITE 3        /* for ll_rw_block() - wait for buffer lock */
#define READ_SYNC       (READ | (1 << BIO_RW_SYNC))
#define READ_META       (READ | (1 << BIO_RW_META))
#define WRITE_SYNC      (WRITE | (1 << BIO_RW_SYNC))
#define WRITE_BARRIER   ((1 << BIO_RW) | (1 << BIO_RW_BARRIER))

//
// timer routines
//

/*
 *      These inlines deal with timer wrapping correctly. You are
 *      strongly encouraged to use them
 *      1. Because people otherwise forget
 *      2. Because if the timer wrap changes in future you won't have to
 *         alter your driver code.
 *
 * time_after(a,b) returns true if the time a is after time b.
 *
 * Do this with "<0" and ">=0" to only test the sign of the result. A
 * good compiler would generate better code (and a really good compiler
 * wouldn't care). Gcc is currently neither.
 */
#define typecheck(x, y) (TRUE)

#define time_after(a,b)         \
        (typecheck(unsigned long, a) && \
         typecheck(unsigned long, b) && \
         ((long)(b) - (long)(a) < 0))
#define time_before(a,b)        time_after(b,a)

#define time_after_eq(a,b)      \
        (typecheck(unsigned long, a) && \
         typecheck(unsigned long, b) && \
         ((long)(a) - (long)(b) >= 0))
#define time_before_eq(a,b)     time_after_eq(b,a)

#define time_in_range(a,b,c) \
        (time_after_eq(a,b) && \
         time_before_eq(a,c))

#define smp_rmb()  do {}while(0)


static inline __u32 do_div64 (__u64 * n, __u64 b)
{
    __u64 mod;

    mod = *n % b;
    *n = *n / b;
    return (__u32) mod;
}
#define do_div(n, b) do_div64(&(n), (__u64)b)

#endif // _EXT2_MODULE_HEADER_
