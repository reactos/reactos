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
void init_wrapper(void)
{
	int n;
	for(n=0;n<MAX_TIMERS;n++)
	{
		main_timer_list[n]=NULL;
	}

	my_jiffies=0;
	num_irqs=0;
	my_current=&act_cur;
	pci_probe_dev=NULL;

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
//	printk("handle irqs\n");
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
//			printk("do timer %i fn %p\n",n,function);

			function(data);
		}
	}
}
/*------------------------------------------------------------------------*/ 
// Purpose: Remember thread procedure and data in global var
int my_kernel_thread(int (*handler)(void*), void* parm, int flags)
{
	thread_handler=handler;
	thread_parm=parm;
	return 42; // PID :-)
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
//	printk("drv_num %i %p %p\n",drvs_num,m_drivers[0]->probe,m_drivers[1]->probe);
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
//				printk("probe%i %p ",n,m_drivers[n]->probe);
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
//		printk("driver_register %i: %p %p",drvs_num,driver,driver->probe);
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
void my_wake_up(void* p)
{
	need_wakeup=1;
}
/*------------------------------------------------------------------------*/ 
/* wait until woken up (only one wait allowed!) */
int my_schedule_timeout(int x)
{
	int wait=1;
	x+=10; // safety
//	printk("schedule_timeout %i\n",x);
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
//	printk("schedule DONE!!!!!!\n");
	return x;
}
/*------------------------------------------------------------------------*/ 
void my_wait_for_completion(struct completion *x)
{
	int n=1000;
//	printk("wait for completion\n");
	while(!x->done && (n>0))
	{
		do_all_timers();	
#ifndef HAVE_IRQS
		handle_irqs(-1);

#endif
		wait_ms(10);	
		n--;
	}
//	printk("wait for completion done %i\n",x->done);
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
		printk(KERN_ERR "PCI device not set!\n");
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
