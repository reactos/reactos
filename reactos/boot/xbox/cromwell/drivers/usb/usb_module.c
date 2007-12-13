/* Tiny module to test USB encapsulation 
 * (c) 2003, Georg Acher, georg@acher.org
 */
#include <linux/module.h>
#include <linux/socket.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/hardirq.h>
#include <linux/pci.h>
/*------------------------------------------------------------------------*/ 
 void my_wait_ms(unsigned int ms)
{
//        if(!in_interrupt()) {

                current->state = TASK_UNINTERRUPTIBLE;
                schedule_timeout(1 + ms * HZ / 1000);
//        }
//        else
//                mdelay(ms);
}
/*------------------------------------------------------------------------*/ 
void my_mdelay(int x)
{
	mdelay(x);
}
/*------------------------------------------------------------------------*/ 
void my_udelay(int x)
{
	udelay(x);
}
/*------------------------------------------------------------------------*/ 

/*------------------------------------------------------------------------*/ 
void* zxmalloc(size_t  s)
{
	return kmalloc(s,GFP_DMA);
}
/*------------------------------------------------------------------------*/ 
void zxfree(void* x)
{
	kfree(x);
}
/*------------------------------------------------------------------------*/ 
void zxprintf(char* fmt, ...)
{
	va_list ap;
	char buffer[1024];
	va_start(ap, fmt);
	vsnprintf(buffer,1024,fmt,ap);
	usbprintk(buffer);
	va_end(ap);
}
/*------------------------------------------------------------------------*/ 
int zxsnprintf(char *buffer, size_t s, char* fmt, ...)
{
	va_list ap;
	int x;
	va_start(ap, fmt);
	x=vsnprintf(buffer,s,fmt,ap);
	va_end(ap);
	return x;
}
/*------------------------------------------------------------------------*/ 
int zxsprintf(char *buffer, char* fmt, ...)
{
	va_list ap;
	int x;
	va_start(ap, fmt);
	x=vsprintf(buffer,fmt,ap);
	va_end(ap);
	return x;
}

static DECLARE_COMPLETION(testd_exited);
static pid_t testd_pid=0;
void test_usb_init(void);
void test_usb_events(void);
static int test_thread(void *x)
{
	reparent_to_init();

        /* Setup a nice name */
        strcpy(current->comm, "testd_usb");

	usbprintk("Thread started\n");

	test_usb_init();

	do {		
		test_usb_events();
		my_wait_ms(50);
	} while (!signal_pending(current));

	usbprintk("hub_thread exiting\n");
	complete_and_exit(&testd_exited, 0);	
}
/*------------------------------------------------------------------------*/ 

void			*ohci_base;
static int __init test_init (void)
{
	struct pci_dev *dev = NULL;
	unsigned long		resource, len;

	usbprintk("module init\n");
	dev =pci_find_device (0x1045,0xc861, dev);
	pci_enable_device (dev);
	resource = pci_resource_start (dev, 0);
	len = pci_resource_len (dev, 0);
	ohci_base = ioremap_nocache (resource, len);
	usbprintk("base %p, len %i\n",ohci_base,len);
	pci_set_master (dev);

	testd_pid = kernel_thread(test_thread, NULL,
		CLONE_FS | CLONE_FILES | CLONE_SIGHAND);



        return 0;
}
/*------------------------------------------------------------------------*/ 
static void __exit test_cleanup (void)
{
        usbprintk("test_cleanup start\n");
	if (testd_pid)
	{
		int ret;
		ret = kill_proc(testd_pid, SIGKILL, 1);

		my_wait_ms(1000);
//		wait_for_completion(&testd_exited);
	}
        usbprintk("test_cleanup\n");

}
/*------------------------------------------------------------------------*/ 

MODULE_LICENSE("GPL");

module_init (test_init);
module_exit (test_cleanup);
