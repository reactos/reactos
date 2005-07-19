/*
 * linux-wrapper.h
 *
 * Hard coded Linux kernel replacements for x86
 *
 * (c) 2003 Georg Acher (georg@acher.org)
 *
 * Emulation of:
 * typedefs
 * structs
 * macros
 *
 * All structs and prototypes are based on kernel source 2.5.72
 *
 * Modified by Aleksey Bragin (aleksey@reactos.com) for ReactOS needs
 *
 *
 * #include <standard-GPL-header.h>
 */

/*------------------------------------------------------------------------*/
/* Typedefs */
/*------------------------------------------------------------------------*/ 
#include "cromwell_types.h"

typedef unsigned int __u32;
//typedef __u32 u32;
typedef unsigned short __u16;
//typedef __u16 u16;
typedef unsigned char __u8;
//typedef __u8 u8;

typedef short s16;

typedef u32 dma_addr_t;

typedef int spinlock_t;
typedef int atomic_t;
#ifndef STANDALONE
#ifndef _MODE_T_
#define _MODE_T_
typedef int mode_t;
#endif
#ifndef _PID_T_
#define _PID_T_
typedef int pid_t;
#endif
#ifndef _SSIZE_T_
#define _SSIZE_T_
typedef int ssize_t;
#endif

#endif
typedef int irqreturn_t;
typedef unsigned long kernel_ulong_t;

typedef int wait_queue_head_t;
/*------------------------------------------------------------------------*/ 
/* Stuff from xbox/linux environment */
/*------------------------------------------------------------------------*/ 

#include "list.h"

#ifndef STANDALONE
#ifdef MODULE
typedef int size_t;
#define NULL ((void*)0)
extern void * memset(void *,int,unsigned int);
extern void * memcpy(void *,const void *,unsigned int);
#if 0
extern char * strcpy(char *,const char *);
#else
static inline char * strcpy(char * dest,const char *src)
{
int d0, d1, d2;
__asm__ __volatile__(
        "1:\tlodsb\n\t"
        "stosb\n\t"
        "testb %%al,%%al\n\t"
        "jne 1b"
        : "=&S" (d0), "=&D" (d1), "=&a" (d2)
        :"0" (src),"1" (dest) : "memory");
return dest;
}
#endif
extern size_t strlen(const char *);

extern int memcmp(const void *,const void *,unsigned int);

#else
#include "boot.h"
#include "config.h"
#endif
#else
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "consts.h"
#include <string.h>
#endif

/*------------------------------------------------------------------------*/ 
/* General structs */
/*------------------------------------------------------------------------*/ 

struct timer_list { 
	void (*function)(unsigned long);
	unsigned long data;
	int expires;
	struct list_head timer_list;
};

struct work_struct {
	void (*func)(void *);
};
struct device {
	char name[128];
	struct bus_type *bus;
	int dma_mask;
	char    bus_id[16];
	struct device_driver* driver;
	void            *driver_data;
	struct device *parent;
	struct list_head driver_list;
	void    (*release)(struct device * dev);

	void *dev_ext; // ReactOS-specific: pointer to windows device extension
};
struct class_device{int a;};
struct semaphore{int a;};

struct device_driver{
	char *name;
	struct bus_type *bus;
	int     (*probe)        (struct device * dev);
        int     (*remove)       (struct device * dev);
	struct list_head        devices;
};

struct bus_type {
        char                    * name;       
        int             (*match)(struct device * dev, struct device_driver * drv);
        struct device * (*add)  (struct device * parent, char * bus_id);
        int             (*hotplug) (struct device *dev, char **envp, 
                                    int num_envp, char *buffer, int buffer_size);
};

struct dummy_process
{
	int flags;
};

struct pt_regs
{
	int a;
};
struct completion {
        unsigned int done;
        wait_queue_head_t wait;
};

// windows lookaside list head
typedef void* kmem_cache_t;

struct dma_pool
{
	int dummy;
};

/* These definitions mirror those in pci.h, so they can be used
 * interchangeably with their PCI_ counterparts */
enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};

/* compatibility */
#define PCI_DMA_TODEVICE DMA_TO_DEVICE
#define PCI_DMA_FROMDEVICE DMA_FROM_DEVICE

/* from mod_devicetable.h */
struct usb_device_id {
        /* which fields to match against? */
        __u16           match_flags;

        /* Used for product specific matches; range is inclusive */
        __u16           idVendor;
        __u16           idProduct;
        __u16           bcdDevice_lo;
        __u16           bcdDevice_hi;

        /* Used for device class matches */
        __u8            bDeviceClass;
        __u8            bDeviceSubClass;
        __u8            bDeviceProtocol;

        /* Used for interface class matches */
        __u8            bInterfaceClass;
        __u8            bInterfaceSubClass;
        __u8            bInterfaceProtocol;

        /* not matched against */
        kernel_ulong_t  driver_info;
};

/* Some useful macros to use to create struct usb_device_id */
#define USB_DEVICE_ID_MATCH_VENDOR              0x0001
#define USB_DEVICE_ID_MATCH_PRODUCT             0x0002
#define USB_DEVICE_ID_MATCH_DEV_LO              0x0004
#define USB_DEVICE_ID_MATCH_DEV_HI              0x0008
#define USB_DEVICE_ID_MATCH_DEV_CLASS           0x0010
#define USB_DEVICE_ID_MATCH_DEV_SUBCLASS        0x0020
#define USB_DEVICE_ID_MATCH_DEV_PROTOCOL        0x0040
#define USB_DEVICE_ID_MATCH_INT_CLASS           0x0080
#define USB_DEVICE_ID_MATCH_INT_SUBCLASS        0x0100
#define USB_DEVICE_ID_MATCH_INT_PROTOCOL        0x0200

/*------------------------------------------------------------------------*/ 
/* imported functions from top-level */
/*------------------------------------------------------------------------*/ 

//void zxprintf(char* fmt, ...);
//void zxsprintf(char *buffer, char* fmt, ...);
//int zxsnprintf(char *buffer, size_t s, char* fmt, ...);

/*------------------------------------------------------------------------*/ 
/* PCI structs (taken from linux/pci.h et al., but slightly modified) */
/*------------------------------------------------------------------------*/ 

struct pci_dev {
	int vendor;
	int device;
	struct pci_bus  *bus;
	int irq;
	char *slot_name;
	struct device dev;
	int base[4];
	int flags[4];
	void * data;
	void * dev_ext; // link to Windows DeviceExtension
};

struct pci_bus {
	unsigned char   number;
};

struct pci_device_id {
        __u32 vendor, device;           /* Vendor and device ID or PCI_ANY_ID*/
        __u32 subvendor, subdevice;     /* Subsystem ID's or PCI_ANY_ID */
        __u32 class, class_mask;        /* (class,subclass,prog-if) triplet */
        kernel_ulong_t driver_data;     /* Data private to the driver */
};

struct pci_driver {
        struct list_head node;
        char *name;
        const struct pci_device_id *id_table;   /* must be non-NULL for probe to be called */
        int STDCALL (*probe)  (struct pci_dev *dev, const struct pci_device_id *id);   /* New device inserted */
        void STDCALL (*remove) (struct pci_dev *dev);   /* Device removed (NULL if not a hot-plug capable driver) */
        int  (*save_state) (struct pci_dev *dev, u32 state);    /* Save Device Context */
        int  (*suspend) (struct pci_dev *dev, u32 state);       /* Device suspended */
        int  (*resume) (struct pci_dev *dev);                   /* Device woken up */
        int  (*enable_wake) (struct pci_dev *dev, u32 state, int enable);   /* Enable wake event */
};

struct scatterlist
{
	int page;
	int offset;
	int length;
};

struct usbdevfs_hub_portinfo
{
	int nports;
	int port[8];
};

/*------------------------------------------------------------------------*/ 
/* constant defines */
/*------------------------------------------------------------------------*/ 

#define TASK_UNINTERRUPTIBLE 0
#define HZ 100    /* Don't rely on that... */
#define KERN_DEBUG "DBG: "
#define KERN_ERR "ERR: "
#define KERN_WARNING "WRN: "
#define KERN_INFO "INF: "
#define GFP_KERNEL 0
#define GFP_ATOMIC 0x20
#define GFP_NOIO 0
#define SLAB_ATOMIC 0
#define PCI_ANY_ID (~0)
#define SIGKILL 9
#define THIS_MODULE 0
//#define PAGE_SIZE 4096


#define CLONE_FS 0
#define CLONE_FILES 0
#define CLONE_SIGHAND 0
#define PF_FREEZE 0
#define PF_IOTHREAD 0


#define USBDEVFS_HUB_PORTINFO 1234
#define SA_SHIRQ 0

#undef PCI_COMMAND
#define PCI_COMMAND 0
#undef PCI_COMMAND_MASTER
#define PCI_COMMAND_MASTER 0
/*------------------------------------------------------------------------*/ 
/* Module/export macros */
/*------------------------------------------------------------------------*/ 

#define MODULE_AUTHOR(a)
#define MODULE_DESCRIPTION(a)
#define MODULE_LICENSE(a)
#define MODULE_DEVICE_TABLE(type,name) void* module_table_##name=&name
#define MODULE_PARM(a,b)
#define MODULE_PARM_DESC(a,b)

#define __devinit
#define __exit
#define __init
#define __devinitdata
#define module_init(x) static void module_init_##x(void){ x();}
#define module_exit(x) void module_exit_##x(void){ x();}
#define EXPORT_SYMBOL_GPL(x)
#define EXPORT_SYMBOL(x)

#define __setup(x,y) int setup_##y=(int)y
#define subsys_initcall(x) void subsys_##x(void){x();}

/*------------------------------------------------------------------------*/ 
/* Access macros */
/*------------------------------------------------------------------------*/ 

#define dev_get_drvdata(a) (a)->driver_data
#define dev_set_drvdata(a,b) (a)->driver_data=(b)

#define __io_virt(x) ((void *)(x))
#define readl(addr) (*(volatile unsigned int *) __io_virt(addr))
#define writel(b,addr) (*(volatile unsigned int *) __io_virt(addr) = (b))
#define likely(x) (x)
#define unlikely(x) (x)
#define prefetch(x) 1

#define inw(x) READ_PORT_USHORT((PUSHORT)(x))
#define outw(x,p) WRITE_PORT_USHORT((PUSHORT)(p),(x))
#define outl(x,p) WRITE_PORT_ULONG((PULONG)(p),(x))

/* The kernel macro for list_for_each_entry makes nonsense (have no clue
 * why, this is just the same definition...) */

#undef list_for_each_entry
#define list_for_each_entry(pos, head, member)                          \
        for (pos = list_entry((head)->next, typeof(*pos), member),      \
                     prefetch(pos->member.next);                        \
             &pos->member != (head);                                    \
             pos = list_entry(pos->member.next, typeof(*pos), member),  \
                     prefetch(pos->member.next))

/*------------------------------------------------------------------------*/ 
/* function wrapper macros */
/*------------------------------------------------------------------------*/ 
#define kmalloc(x,y) ExAllocatePool(PagedPool,x)
#define kfree(x) ExFreePool(x)

//#define sprintf(a,b,format, arg...) zxsprintf((a),(b),format, ## arg)
//#define snprintf(a,b,format, arg...) zxsnprintf((a),(b),format, ##arg)
//#define printk(format, arg...) zxprintf(format, ## arg)
#define snprintf(a,b,format, arg...) _snprintf((a),(b),format, ##arg)
#define printk(format, arg...) DPRINT1(format, ## arg)
#define BUG(...) do {} while(0)

/* Locks & friends */

#define DECLARE_MUTEX(x) struct semaphore x
#define init_MUTEX(x)

#define SPIN_LOCK_UNLOCKED 0
#define spin_lock_init(a)  do {} while(0)
#define spin_lock(a) *(int*)a=1
#define spin_unlock(a) do {} while(0)

#define spin_lock_irqsave(a,b) b=0
#define spin_unlock_irqrestore(a,b)

#if 0
#define local_irq_save(x) __asm__ __volatile__("pushfl ; popl %0 ; cli":"=g" (x): /* no input */ :"memory")
#define local_irq_restore(x) __asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory", "cc")
#else
#define local_irq_save(x) do {} while(0) 
#define local_irq_restore(x) do {} while(0) 
#endif

#define atomic_inc(x) *(x)+=1
#define atomic_dec(x) *(x)-=1
#define atomic_dec_and_test(x) (*(x)-=1,(*(x))==0)
#define atomic_set(x,a) *(x)=a
#define atomic_read(x) *(x)
#define ATOMIC_INIT(x) (x)

#define down(x) do {} while(0) 
#define up(x) do {} while(0)
#define down_trylock(a) 0

#define down_read(a) do {} while(0)
#define up_read(a) do {} while(0)

#define DECLARE_WAIT_QUEUE_HEAD(x) KEVENT x

#define DECLARE_COMPLETION(x) struct completion x

/* driver */

#define driver_unregister(a)    do {} while(0)
#define put_device(a)           do {} while(0)


/* PCI */
#define MAX_POOL_PAGES 2
#define BITS_PER_LONG 32

struct pci_page
{
	PHYSICAL_ADDRESS dmaAddress;
	PVOID	virtualAddress;

	unsigned long bitmap[128]; // 128 == 32bits*4096 blocks
};

struct pci_pool
{
	char name[32];
	size_t size;
	size_t allocation;
	size_t blocks_per_page;
	struct pci_dev *pdev;

	// internal stuff
	int pages_allocated;
	int blocks_allocated;

	struct pci_page pages[MAX_POOL_PAGES];
};

#define	to_pci_dev(n) container_of(n, struct pci_dev, dev)

#define pci_pool_create(a,b,c,d,e) my_pci_pool_create(a,b,c,d,e)
struct pci_pool *my_pci_pool_create(const char * name, struct pci_dev * pdev, size_t size, size_t align, size_t allocation);

#define pci_pool_alloc(a,b,c)  my_pci_pool_alloc(a,b,c)
void *my_pci_pool_alloc(struct pci_pool * pool, int mem_flags, dma_addr_t *dma_handle);

#define pci_pool_free(a,b,c)    my_pci_pool_free(a,b,c)
void my_pci_pool_free(struct pci_pool * pool, void * vaddr, dma_addr_t dma);

#define pci_alloc_consistent(a,b,c) my_pci_alloc_consistent(a,b,c)
void  *my_pci_alloc_consistent(struct pci_dev *hwdev, size_t size, dma_addr_t *dma_handle);

#define pci_free_consistent(a,b,c,d)  kfree(c)
#define pci_pool_destroy(a)           my_pci_pool_destroy(a)
void my_pci_pool_destroy (struct pci_pool * pool);

#define pci_module_init(x) my_pci_module_init(x)
int my_pci_module_init(struct pci_driver *x);

#define pci_unregister_driver(a)      do {} while(0)  

#define pci_write_config_word(a,b,c) my_pci_write_config_word(a,b,c)

#define bus_register(a) do {} while(0)
#define bus_unregister(a) do {} while(0)

/* DMA */
//#define dma_pool_alloc(a,b,c) my_dma_pool_alloc((a),(b),(c))
#define dma_pool_alloc(a,b,c) pci_pool_alloc(a,b,c)
#define dma_pool_create(a,b,c,d,e) pci_pool_create(a,b,c,d,e)
#define dma_pool_free(a,b,c) pci_pool_free(a,b,c)
#define dma_pool_destroy(a) pci_pool_destroy(a)

//#define dma_alloc_coherent(a,b,c,d) NULL
//#define dma_free_coherent(a,b,c,d) do {} while(0)
#define dma_map_single(a,b,c,d)		my_dma_map_single(a,b,c,d)
dma_addr_t my_dma_map_single(struct device *hwdev, void *ptr, size_t size, enum dma_data_direction direction);

#define dma_unmap_single(a,b,c,d)	my_dma_unmap_single(a,b,c,d)
void my_dma_unmap_single(struct device *dev, dma_addr_t dma_addr, size_t size, enum dma_data_direction direction);

#define pci_unmap_single(a,b,c,d)	my_pci_unmap_single(a,b,c,d)
void my_pci_unmap_single(struct pci_dev *hwdev, dma_addr_t dma_addr, size_t size, int direction);

#define dma_sync_single(a,b,c,d)	my_dma_sync_single(a,b,c,d)
void my_dma_sync_single(struct device *hwdev, dma_addr_t dma_handle, size_t size, int direction);

#define dma_sync_sg(a,b,c,d)		my_dma_sync_sg(a,b,c,d)
void my_dma_sync_sg(struct device *hwdev,  struct scatterlist *sg, int nelems, int direction);

#define dma_map_sg(a,b,c,d)			my_dma_map_sg(a,b,c,d)
int my_dma_map_sg(struct device *hwdev, struct scatterlist *sg, int nents, enum dma_data_direction direction);

#define dma_unmap_sg(a,b,c,d)		my_dma_unmap_sg(a,b,c,d)
void my_dma_unmap_sg(struct device *hwdev, struct scatterlist *sg, int nents, enum dma_data_direction direction);

#define usb_create_driverfs_dev_files(a) do {} while(0)
#define usb_create_driverfs_intf_files(a) do {} while(0)
#define sg_dma_address(x) ((u32)((x)->page*4096 + (x)->offset))
#define sg_dma_len(x) ((x)->length) 

#define page_address(x) ((void*)(x/4096))

#define PCI_ROM_RESOURCE 1
#define IORESOURCE_IO CM_RESOURCE_PORT_IO

#define DECLARE_WAITQUEUE(a,b) KEVENT a=0
#define init_waitqueue_head(a) my_init_waitqueue_head(a)
#define add_wait_queue(a,b) do {} while(0)
#define remove_wait_queue(a,b) do {} while(0)
void my_init_waitqueue_head(PKEVENT a);

VOID KeMemoryBarrier(VOID);

#define mb() KeMemoryBarrier()
#define wmb() __asm__ __volatile__ ("": : :"memory")
#define rmb() __asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory")

#define in_interrupt() 0

#define init_completion(x) (x)->done=0
#define wait_for_completion(x) my_wait_for_completion(x)
void my_wait_for_completion(struct completion*);

#define IRQ_NONE 0
#define IRQ_HANDLED 1

#define INIT_WORK(a,b,c) (a)->func=b

#define set_current_state(a) do {} while(0)

#define might_sleep()        do {} while(0)
#define daemonize(a)         do {} while(0)
#define allow_signal(a)      do {} while(0)
#define wait_event_interruptible(x,y) do {} while(0)

#define interruptible_sleep_on(a) my_interruptible_sleep_on(a)
void my_interruptible_sleep_on(PKEVENT evnt);

#define flush_scheduled_work() do {} while(0)
#define refrigerator(x)        do {} while(0)
#define signal_pending(x)      0  // Don't fall through threads! ReactOS implements this correctly
#define complete_and_exit(a,b) return 0

//#define kill_proc(a,b,c)     0
#define kill_proc(a,b,c) my_kill_proc(a, b, c);
int my_kill_proc(int pid, int signal, int unk);

#define yield() do {} while(0)
#define cpu_relax() do {} while(0)

#define WARN_ON(a) do {} while(0)

/*------------------------------------------------------------------------*/ 
/* Lookaside lists funcs */
/*------------------------------------------------------------------------*/ 
#define kmem_cache_create(a,b,c,d,e,f) my_kmem_cache_create((a),(b),(c),(d),(e),(f))
#define kmem_cache_destroy(a) my_kmem_cache_destroy((a))
#define kmem_cache_alloc(co, flags) my_kmem_cache_alloc((co), (flags))
#define kmem_cache_free(co, ptr) my_kmem_cache_free((co), (ptr))

kmem_cache_t *my_kmem_cache_create(const char *tag, size_t alloc_size,
								   size_t offset, unsigned long flags,
								   void *ctor,
								   void *dtor);

BOOLEAN my_kmem_cache_destroy(kmem_cache_t *co);
void *my_kmem_cache_alloc(kmem_cache_t *co, int flags);
void my_kmem_cache_free(kmem_cache_t *co, void *ptr);

/*------------------------------------------------------------------------*/ 
/* Kernel macros */
/*------------------------------------------------------------------------*/ 

#define LINUX_VERSION_CODE 0x020572
#define UTS_SYSNAME "XBOX"
#define UTS_RELEASE "----"

/* from linux/kernel.h */
#define max_t(type,x,y) \
        ({ type __x = (x); type __y = (y); __x > __y ? __x: __y; })

#define min_t(type,x,y) \
        ({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

/* from linux/stddef.h */

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/*------------------------------------------------------------------------*/ 
/* Conversion macros */
/*------------------------------------------------------------------------*/ 

#define __constant_cpu_to_le32(x) (x)
#define cpu_to_le16(x) (x)
#define le16_to_cpu(x) (x)
#define cpu_to_le32(x) (x)
#define cpu_to_le32p(x) (*(__u32*)(x))
#define le32_to_cpup(x) (*(__u32*)(x))
#define le32_to_cpu(x) ((u32)x)
#define le16_to_cpus(x) do {} while (0)
#define le16_to_cpup(x) (*(__u16*)(x))
#define cpu_to_le16p(x) (*(__u16*)(x))

/*------------------------------------------------------------------------*/ 
/* Debug output */
/*------------------------------------------------------------------------*/ 
#ifdef DEBUG_MODE
#define dev_printk(lvl,x,f,arg...) printk(f, ## arg)
#define dev_dbg(x,f,arg...) printk(f, ## arg)
#define dev_info(x,f,arg...) printk(f,## arg)
#define dev_warn(x,f,arg...) printk(f,## arg)
#define dev_err(x,f,arg...) printk(f,## arg)
#define pr_debug(x,f,arg...) printk(f,## arg)
#define usbprintk printk
#endif

#ifndef DEBUG_MODE
#define dev_printk(lvl,x,f,arg...) do {} while (0)
#define dev_dbg(x,f,arg...) do {} while (0) //printk(f, ## arg)
#define dev_info(x,f,arg...) do {} while (0)
#define dev_warn(x,f,arg...) do {} while (0)
#define dev_err(x,f,arg...) do {} while (0)
#define pr_debug(x,f,arg...) do {} while (0)
#define usbprintk
#endif



#define PCI_DEVFN(a,b) 0
#define PCI_SLOT(a) 0

/**
 * PCI_DEVICE_CLASS - macro used to describe a specific pci device class
 * @dev_class: the class, subclass, prog-if triple for this device
 * @dev_class_mask: the class mask for this device
 *
 * This macro is used to create a struct pci_device_id that matches a
 * specific PCI class.  The vendor, device, subvendor, and subdevice 
 * fields will be set to PCI_ANY_ID.
 */
#define PCI_DEVICE_CLASS(dev_class,dev_class_mask) \
	.class = (dev_class), .class_mask = (dev_class_mask), \
	.vendor = PCI_ANY_ID, .device = PCI_ANY_ID, \
	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID


/*------------------------------------------------------------------------*/ 
/* Stuff from kernel */
/*------------------------------------------------------------------------*/ 

#include "errno.h"
#include "bitops.h"
//#include "linux/pci_ids.h"

/*------------------------------------------------------------------------*/ 
/* global variables */
/*------------------------------------------------------------------------*/ 

#define jiffies my_jiffies
extern int my_jiffies;
#define current my_current
extern struct dummy_process *my_current;

extern struct list_head interrupt_list;

/*------------------------------------------------------------------------*/ 
/* Function prototypes */
/*------------------------------------------------------------------------*/ 
void STDCALL usb_hcd_pci_remove (struct pci_dev *dev);

#define my_wait_ms(x) wait_ms(x)

#define my_udelay(x) wait_ms(x)
#define udelay(x) my_udelay(x)

#define my_mdelay(x) wait_ms(1+x/1000);
#define mdelay(x) my_mdelay(x);

#define pci_find_slot(a,b) my_pci_find_slot(a,b)
struct pci_dev *my_pci_find_slot(int a,int b);

/*------------------------------------------------------------------------*/ 
/* Timer management */
/*------------------------------------------------------------------------*/ 

#define MAX_TIMERS 20
extern struct timer_list *main_timer_list[MAX_TIMERS];

static void __inline__ init_timer(struct timer_list* t)
{
	INIT_LIST_HEAD(&t->timer_list);
	t->function=NULL;
	t->expires=0;
}

static void __inline__ add_timer(struct timer_list* t)
{
	int n;
	for(n=0;n<MAX_TIMERS;n++)
		if (main_timer_list[n]==0)
		{
			main_timer_list[n]=t;
			break;
		}
}

static void __inline__ del_timer(struct timer_list* t)
{
	int n;
	for(n=0;n<MAX_TIMERS;n++)
		if (main_timer_list[n]==t)
		{
			main_timer_list[n]=0;
			break;
		}
}
static void __inline__ del_timer_sync(struct timer_list* t)
{
	int n;
	for(n=0;n<MAX_TIMERS;n++)
		if (main_timer_list[n]==t)
		{
			main_timer_list[n]=0;
			break;
		}

}
static void __inline__ mod_timer(struct timer_list* t, int ex)
{
	del_timer(t);
	t->expires=ex;
	add_timer(t);
}

#define time_after_eq(a,b)	\
	(((long)(a) - (long)(b) >= 0))

/*------------------------------------------------------------------------*/ 
/* Device driver and process related stuff */
/*------------------------------------------------------------------------*/ 

static int __inline__ usb_major_init(void){return 0;}
static void __inline__ usb_major_cleanup(void){}
static void __inline__ schedule_work(void* p){}

#define device_initialize(x) my_device_initialize(x)
void my_device_initialize(struct device *dev);

#define get_device(x) my_get_device(x)
struct device *my_get_device(struct device *dev);

#define device_add(x) my_device_add(x)
int my_device_add(struct device *dev);

#define driver_register(x) my_driver_register(x)
int my_driver_register(struct device_driver *driver);

#define device_unregister(a)    my_device_unregister(a)
int my_device_unregister(struct device *dev);

#define DEVICE_ATTR(a,b,c,d) int xxx_##a
#define device_create_file(a,b)  do {} while(0)
#define device_remove_file(a,b)  do {} while(0)

#define schedule_timeout(x) my_schedule_timeout(x)
int my_schedule_timeout(int x);

#define wake_up(x) my_wake_up(x)
void my_wake_up(PKEVENT);

// cannot be mapped via macro due to collision with urb->complete
static void __inline__ complete(struct completion *p)
{
	/* Wake up x->wait */
	p->done++;
	wake_up((PKEVENT)&p->wait);
}

#define kernel_thread(a,b,c) my_kernel_thread(a,b,c)
int my_kernel_thread(int STDCALL (*handler)(void*), void* parm, int flags);

/*------------------------------------------------------------------------*/ 
/* PCI, simple and inlined... */
/*------------------------------------------------------------------------*/ 
#include "pci_hal.c"

/*------------------------------------------------------------------------*/ 
/* IRQ handling */
/*------------------------------------------------------------------------*/ 

#define request_irq(a,b,c,d,e) my_request_irq(a,b,c,d,e)
int my_request_irq(unsigned int irq,
                       int  (*handler)(int, void *, struct pt_regs *),
		unsigned long mode, const char *desc, void *data);

#define free_irq(a,b) my_free_irq(a,b)
int free_irq(int irq, void* p);



struct my_irqs {
	int  (*handler)(int, void *, struct pt_regs *);
	int irq;
	void* data;
};

#define MAX_IRQS 8

// Exported to top level

void handle_irqs(int irq);
void inc_jiffies(int);
void init_wrapper(struct pci_dev *pci_dev);
void do_all_timers(void);

#define __KERNEL_DS   0x18

int my_pci_write_config_word(struct pci_dev *, int, u16);

