/*
 * USB support based on Linux kernel source
 *
 * 2003-06-21 Georg Acher (georg@acher.org)
 *
 * Concept:
 * 
 * 1) Forget all device interrupts, scheduling, semaphores, threads etc.
 * 1a) Forget all DMA and PCI helper functions
 * 2) Forget usbdevfs, procfs and ioctls
 * 3) Emulate OHCI interrupts and root hub timer by polling
 * 4) Emulate hub kernel thread by polling
 * 5) Emulate synchronous USB-messages (usb_*_msg) with busy waiting
 *
 * To be done:
 * 6) Remove code bloat  
 *
 */

#include "../usb_wrapper.h"

/* internal state */

static struct pci_dev *pci_probe_dev;
extern int (*thread_handler)(void*);
extern void* thread_parm;

struct my_irqs reg_irqs[MAX_IRQS];
int num_irqs;
int need_wakeup;

int my_jiffies;

struct timer_list *main_timer_list[MAX_TIMERS];
struct dummy_process act_cur={0};
struct dummy_process *my_current;

int (*thread_handler)(void*);
void* thread_parm;

#define MAX_DRVS 8
static struct device_driver *m_drivers[MAX_DRVS];
static int drvs_num;

/*------------------------------------------------------------------------*/ 
/* 
 * Helper functions for top-level system
 */
/*------------------------------------------------------------------------*/ 
void init_wrapper(struct pci_dev *probe_dev)
{
	int n;
	for(n=0;n<MAX_TIMERS;n++)
	{
		main_timer_list[n]=NULL;
	}

	my_jiffies=0;
	num_irqs=0;
	my_current=&act_cur;
	pci_probe_dev=probe_dev;

	for(n=0;n<MAX_IRQS;n++)
	{
		reg_irqs[n].handler=NULL;
		reg_irqs[n].irq=-1;
	}
	drvs_num=0;
	need_wakeup=0;
	for(n=0;n<MAX_DRVS;n++)
		m_drivers[n]=NULL;
}
/*------------------------------------------------------------------------*/ 
void handle_irqs(int irq)
{
	int n;
	//printk("handle irqs\n");
	for(n=0;n<MAX_IRQS;n++)
	{
		if (reg_irqs[n].handler && (irq==reg_irqs[n].irq || irq==-1))
			reg_irqs[n].handler(reg_irqs[n].irq,reg_irqs[n].data,NULL);
	}
}
/*------------------------------------------------------------------------*/ 
void inc_jiffies(int n)
{
	my_jiffies+=n;
}
/*------------------------------------------------------------------------*/ 
void do_all_timers(void)
{
	int n;
	for(n=0;n<MAX_TIMERS;n++)
	{
		if (main_timer_list[n] &&
		    main_timer_list[n]->function && main_timer_list[n]->expires) 
		{
			void (*function)(unsigned long)=main_timer_list[n]->function;
			unsigned long data=main_timer_list[n]->data;
			main_timer_list[n]->expires=0;

			main_timer_list[n]=NULL; // remove timer
			//printk("do timer %i fn %p\n",n,function);

			function(data);
		}
	}
}
/*------------------------------------------------------------------------*/ 
// Purpose: Remember thread procedure and data in global var
// ReactOS Purpose: Create real kernel thread
int my_kernel_thread(int STDCALL (*handler)(void*), void* parm, int flags)
{
	HANDLE hThread;
	//thread_handler=handler;
	//thread_parm=parm;
	//return 42; // PID :-)
	
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	PsCreateSystemThread(&hThread,
			     THREAD_ALL_ACCESS,
			     NULL,
			     NULL,
			     NULL,
			     (PKSTART_ROUTINE)handler,
				 parm);

	DPRINT1("usbcore: Created system thread %d\n", (int)hThread);

    return (int)hThread; // FIXME: Correct?
}

// Kill the process
int my_kill_proc(int pid, int signal, int unk)
{
	HANDLE hThread;

	// TODO: Implement actual process killing

	hThread = (HANDLE)pid;
	ZwClose(hThread);

	return 0;
}

/*------------------------------------------------------------------------*/ 
/* Device management
 * As simple as possible, but as complete as necessary ...
 */
/*------------------------------------------------------------------------*/ 


/* calls probe function for hotplug (which does device matching), this is the
only link between usbcore and the registered device drivers! */
int my_device_add(struct device *dev)
{
	int n,found=0;
	printk("drv_num %i %p %p\n",drvs_num,m_drivers[0]->probe,m_drivers[1]->probe);

	if (dev->driver)
	{
		if (dev->driver->probe)
			return dev->driver->probe(dev);
	}
	else
	{
		for(n=0;n<drvs_num;n++)
		{
			if (m_drivers[n]->probe)
			{
				dev->driver=m_drivers[n];
				printk("probe%i %p\n",n,m_drivers[n]->probe);

				if (m_drivers[n]->probe(dev) == 0)
				{
//					return 0;
					found=1;
				}
			}
		}
		if (found) return 0;
	}
	dev->driver=NULL;
	return -ENODEV;
}
/*------------------------------------------------------------------------*/ 
int my_driver_register(struct device_driver *driver)
{

	if (drvs_num<MAX_DRVS)
	{
		printk("driver_register %i: %p %p",drvs_num,driver,driver->probe);  

		m_drivers[drvs_num++]=driver;
		return 0;
	}
	return -1;
}
/*------------------------------------------------------------------------*/ 
int my_device_unregister(struct device *dev)
{
	if (dev->driver && dev->driver->remove)
		dev->driver->remove(dev);
	return 0;
		
}
/*------------------------------------------------------------------------*/ 
struct device *my_get_device(struct device *dev)
{
	return NULL;
}
/*------------------------------------------------------------------------*/ 
void my_device_initialize(struct device *dev)
{
}
/*------------------------------------------------------------------------*/ 
void my_wake_up(PKEVENT evnt)
{
	need_wakeup=1;

	KeSetEvent(evnt, 0, FALSE); // Signal event
}
/*------------------------------------------------------------------------*/ 
void my_init_waitqueue_head(PKEVENT evnt)
{
	// this is used only in core/message.c, and it isn't needed there
	//KeInitializeEvent(evnt, NotificationEvent, TRUE); // signalled state
}
/*------------------------------------------------------------------------*/ 
/* wait until woken up (only one wait allowed!) */
int my_schedule_timeout(int x)
{
	int wait=1;
	x+=10; // safety
	printk("schedule_timeout %i\n",x);

	while(x>0)
	{
		do_all_timers();
#ifndef HAVE_IRQS
		handle_irqs(-1);

#endif
		if (need_wakeup)
			break;
		wait_ms(wait);
		inc_jiffies(wait);
		x-=wait;
	}
	need_wakeup=0;
	printk("schedule DONE!!!!!!\n");

	return x;
}
/*------------------------------------------------------------------------*/ 
void my_wait_for_completion(struct completion *x)
{
	int n=100;
	printk("wait for completion\n");

	while(!x->done && (n>0))
	{
		do_all_timers();	
#ifndef HAVE_IRQS
		handle_irqs(-1);

#endif
		wait_ms(10);	
		n--;
	}
	printk("wait for completion done %i\n",x->done);

}
/*------------------------------------------------------------------------*/ 
void my_interruptible_sleep_on(PKEVENT evnt)
{
	KeWaitForSingleObject(evnt, Executive, KernelMode, FALSE, NULL);
	KeClearEvent(evnt); // reset to not-signalled
}
/*------------------------------------------------------------------------*/ 
// Helper for pci_module_init
/*------------------------------------------------------------------------*/ 
int my_pci_module_init(struct pci_driver *x)
{
	struct pci_dev *dev=pci_probe_dev;
	const struct pci_device_id *id=NULL;
	if (!pci_probe_dev)
	{
		DPRINT1("PCI device not set!\n");
		return 0;
	}
	x->probe(dev, id);
	return 0;
}
/*------------------------------------------------------------------------*/ 
struct pci_dev *my_pci_find_slot(int a,int b)
{
	return NULL;
}
/*------------------------------------------------------------------------*/ 
int my_pci_write_config_word(struct pci_dev *dev, int where, u16 val)
{
	//dev->bus, dev->devfn, where, val
	OHCI_DEVICE_EXTENSION *dev_ext = (OHCI_DEVICE_EXTENSION *)dev->dev_ext;

	//FIXME: Is returning this value correct?
	//FIXME: Mixing pci_dev and win structs isn't a good thing at all
	return HalSetBusDataByOffset(PCIConfiguration, dev->bus->number, dev_ext->SystemIoSlotNumber, &val, where, sizeof(val));
}
/*------------------------------------------------------------------------*/ 
int my_request_irq(unsigned int irq,
                       int  (*handler)(int,void *, struct pt_regs *),
                       unsigned long mode, const char *desc, void *data)
{
	if (num_irqs<MAX_IRQS)
	{
		reg_irqs[num_irqs].handler=handler;
		reg_irqs[num_irqs].irq=irq;
		reg_irqs[num_irqs].data=data;
		num_irqs++;
		return 0;
	}
	return 1;
}
/*------------------------------------------------------------------------*/ 
int my_free_irq(int irq, void* p)
{
	/* No free... */
	return 0;
}
/*------------------------------------------------------------------------*/ 
// Lookaside funcs
/*------------------------------------------------------------------------*/ 
kmem_cache_t *my_kmem_cache_create(const char *tag, size_t alloc_size,
								   size_t offset, unsigned long flags,
								   void *ctor,
								   void *dtor)
{
	//TODO: Take in account ctor and dtor - callbacks for alloc/free, flags and offset
	//FIXME: We assume this cache is always NPaged
	PNPAGED_LOOKASIDE_LIST Lookaside;
	ULONG Tag=0x11223344; //FIXME: Make this from tag

	Lookaside = ExAllocatePool(NonPagedPool, sizeof(NPAGED_LOOKASIDE_LIST));
	
	ExInitializeNPagedLookasideList(
		Lookaside,
		NULL,
		NULL,
		0,
		alloc_size,
		Tag,
		0);

	return (kmem_cache_t *)Lookaside;
}
/*------------------------------------------------------------------------*/ 
BOOLEAN my_kmem_cache_destroy(kmem_cache_t *co)
{
	ExDeleteNPagedLookasideList((PNPAGED_LOOKASIDE_LIST)co);

	ExFreePool(co);
	return FALSE;
}
/*------------------------------------------------------------------------*/ 
void *my_kmem_cache_alloc(kmem_cache_t *co, int flags)
{
	return ExAllocateFromNPagedLookasideList((PNPAGED_LOOKASIDE_LIST)co);
}
/*------------------------------------------------------------------------*/ 
void my_kmem_cache_free(kmem_cache_t *co, void *ptr)
{
	ExFreeToNPagedLookasideList((PNPAGED_LOOKASIDE_LIST)co, ptr);
}
/*------------------------------------------------------------------------*/ 
// DMA, not used now
/*------------------------------------------------------------------------*/ 
void *my_dma_pool_alloc(struct dma_pool *pool, int gfp_flags, dma_addr_t *dma_handle)
{
	// HalAllocCommonBuffer
	// But ideally IoGetDmaAdapter
	return NULL;
}
