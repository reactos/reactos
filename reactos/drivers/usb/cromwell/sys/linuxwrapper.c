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
static int drvs_num=0;
unsigned int LAST_USB_EVENT_TICK;

NTSTATUS init_dma(POHCI_DEVICE_EXTENSION pDevExt);

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
		
	init_dma(probe_dev->dev_ext);
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
		if (main_timer_list[n] && main_timer_list[n]->function) 
		{
			void (*function)(unsigned long)=main_timer_list[n]->function;
			unsigned long data=main_timer_list[n]->data;
			
			if (main_timer_list[n]->expires>1) {
				main_timer_list[n]->expires--;
			} else {
				
				main_timer_list[n]->expires=0;
				main_timer_list[n]=0; // remove timer
				// Call Timer Function Data
				function(data);
			}
		}
	}
}
/*------------------------------------------------------------------------*/ 
// Purpose: Remember thread procedure and data in global var
// ReactOS Purpose: Create real kernel thread
int my_kernel_thread(int STDCALL (*handler)(void*), void* parm, int flags)
{
	HANDLE hThread = NULL;
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
extern unsigned int LAST_USB_IRQ;

int my_schedule_timeout(int x)
{
	LONGLONG HH;
	//LONGLONG temp;
	LARGE_INTEGER delay;
	//PULONG tmp_debug=NULL;
	//extern unsigned int LAST_USB_EVENT_TICK;

	//*tmp_debug = 0xFFAAFFAA;

	printk("schedule_timeout: %d ms\n", x);

		//delay.QuadPart = -x*10000; // convert to 100ns units
		//KeDelayExecutionThread(KernelMode, FALSE, &delay); //wait_us(1);

	/*
	x+=5; // safety
	x = x*1000;	// to us format
	*/
	x = 300; // it's enough for most purposes

	while(x>0)
	{
		KeQueryTickCount((LARGE_INTEGER *)&HH);//IoInputDword(0x8008);
    	//temp = HH - LAST_USB_EVENT_TICK;
    	
		//if (temp>(3579)) { //3579 = 1ms!
		//if (temp>1000) {
			do_all_timers();
		//	LAST_USB_EVENT_TICK = HH;
		//}

		handle_irqs(-1);
		
		if (need_wakeup)
			break;

		delay.QuadPart = -10;
		KeDelayExecutionThread(KernelMode, FALSE, &delay); //wait_us(1);
		x-=1;
		//DPRINT("schedule_timeout(): time left: %d\n", x);
	}
	need_wakeup=0;

	printk("schedule DONE!!!!!!\n");

	return 0;//x;
}
/*------------------------------------------------------------------------*/ 
void my_wait_for_completion(struct completion *x)
{
	LONGLONG HH;
	LONGLONG temp;
	LARGE_INTEGER delay;

	extern unsigned int LAST_USB_EVENT_TICK;

	printk("wait for completion11, x=0x%08x\n", (DWORD)x);

	int n=10;
	n = n*1000;	// to us format

	while(!x->done && (n>0))
	{
		KeQueryTickCount((LARGE_INTEGER *)&HH);//IoInputDword(0x8008);
		temp = HH - LAST_USB_EVENT_TICK;

		//if (temp>(3579)) {
		if (temp>(1000)) {
		//	do_all_timers();
			LAST_USB_EVENT_TICK = HH;
		}

		//	handle_irqs(-1);

		delay.QuadPart = -10;
		KeDelayExecutionThread(KernelMode, FALSE, &delay); //wait_us(1);
		n--;
	}
	printk("wait for completion done %i\n",x->done);

}
/*------------------------------------------------------------------------*/ 
void my_init_completion(struct completion *x)
{
	x->done=0;
	KeInitializeEvent(&x->wait, NotificationEvent, FALSE);
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
// DMA support routines
/*------------------------------------------------------------------------*/ 
#ifdef USB_DMA_SINGLE_SUPPORT
static IO_ALLOCATION_ACTION NTAPI MapRegisterCallback(PDEVICE_OBJECT DeviceObject,
                                                      PIRP Irp,
                                                      PVOID MapRegisterBase,
                                                      PVOID Context);
#endif

NTSTATUS
init_dma(POHCI_DEVICE_EXTENSION pDevExt)
{
	// Prepare device descriptor structure
	DEVICE_DESCRIPTION dd;
#ifdef USB_DMA_SINGLE_SUPPORT
	KEVENT DMAEvent;
	KIRQL OldIrql;
	NTSTATUS Status;
#endif

	RtlZeroMemory( &dd, sizeof(dd) );
	dd.Version = DEVICE_DESCRIPTION_VERSION;
	dd.Master = TRUE;
	dd.ScatterGather = TRUE;
	dd.DemandMode = FALSE;
	dd.AutoInitialize = FALSE;
	dd.Dma32BitAddresses = TRUE;
	dd.InterfaceType = PCIBus;
	dd.DmaChannel = 0;//pDevExt->dmaChannel;
	dd.MaximumLength = 128;//MAX_DMA_LENGTH;
	dd.DmaWidth = Width32Bits;
	dd.DmaSpeed = MaximumDmaSpeed;

	// The following taken from Win2k DDB:
	// "Compute the maximum number of mapping regs
	// this device could possibly need. Since the
	// transfer may not be paged aligned, add one
	// to allow the max xfer size to span a page."
	//pDevExt->mapRegisterCount = (MAX_DMA_LENGTH / PAGE_SIZE) + 1;

    // TODO: Free it somewhere (PutDmaAdapter)
	pDevExt->pDmaAdapter =
		IoGetDmaAdapter( pDevExt->PhysicalDeviceObject,
		&dd,
		&pDevExt->mapRegisterCount);
		
	DPRINT1("IoGetDmaAdapter done 0x%X, mapRegisterCount=%d\n", pDevExt->pDmaAdapter, pDevExt->mapRegisterCount);

	// Fail if failed
	if (pDevExt->pDmaAdapter == NULL)
		return STATUS_INSUFFICIENT_RESOURCES;

#ifdef USB_DMA_SINGLE_SUPPORT
	/* Allocate buffer now */
    pDevExt->BufferSize = pDevExt->mapRegisterCount * PAGE_SIZE;
    DPRINT1("Bufsize = %u\n", pDevExt->BufferSize);
    pDevExt->VirtualBuffer = pDevExt->pDmaAdapter->DmaOperations->AllocateCommonBuffer(
									pDevExt->pDmaAdapter, pDevExt->BufferSize, &pDevExt->Buffer, FALSE);
    DPRINT1("Bufsize = %u, Buffer = 0x%x", pDevExt->BufferSize, pDevExt->Buffer.LowPart);

	if (!pDevExt->VirtualBuffer)
    {
        DPRINT1("Could not allocate buffer\n");
        // should try again with smaller buffer...
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT1("Calling IoAllocateMdl()\n");
    pDevExt->Mdl = IoAllocateMdl(pDevExt->VirtualBuffer, pDevExt->BufferSize, FALSE, FALSE, NULL);
    DPRINT1("Bufsize == %u\n", pDevExt->BufferSize);

    if (!pDevExt->Mdl)
    {
        DPRINT1("IoAllocateMdl() FAILED\n");
        //TODO: Free the HAL buffer
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT1("VBuffer == 0x%x Mdl == %u Bufsize == %u\n", pDevExt->VirtualBuffer, pDevExt->Mdl, pDevExt->BufferSize);

    DPRINT1("Calling MmBuildMdlForNonPagedPool\n");
    MmBuildMdlForNonPagedPool(pDevExt->Mdl);


	/* Get map registers for DMA */
	KeInitializeEvent(&DMAEvent, SynchronizationEvent, FALSE);

	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
	// TODO: Free adapter channel somewhere
	Status = pDevExt->pDmaAdapter->DmaOperations->AllocateAdapterChannel(pDevExt->pDmaAdapter,
				pDevExt->PhysicalDeviceObject, pDevExt->mapRegisterCount, MapRegisterCallback, &DMAEvent);
	KeLowerIrql(OldIrql);

    DPRINT1("VBuffer == 0x%x Bufsize == %u\n", pDevExt->VirtualBuffer, pDevExt->BufferSize);
    KeWaitForSingleObject(&DMAEvent, Executive, KernelMode, FALSE, NULL);

	if(Status != STATUS_SUCCESS)
	{
		DPRINT("init_dma(): unable to allocate adapter channels\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
#endif
	return STATUS_SUCCESS;
}

/*
 * FUNCTION: Acquire map registers in prep for DMA
 * ARGUMENTS:
 *     DeviceObject: unused
 *     Irp: unused
 *     MapRegisterBase: returned to blocked thread via a member var
 *     Context: contains a pointer to the right ControllerInfo
 *     struct
 * RETURNS:
 *     KeepObject, because that's what the DDK says to do
 */
#ifdef USB_DMA_SINGLE_SUPPORT
static IO_ALLOCATION_ACTION NTAPI MapRegisterCallback(PDEVICE_OBJECT DeviceObject,
                                                      PIRP Irp,
                                                      PVOID MapRegisterBase,
                                                      PVOID Context)
{
	POHCI_DEVICE_EXTENSION pDevExt = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	UNREFERENCED_PARAMETER(Irp);

	DPRINT("usb_linuxwrapper: MapRegisterCallback Called, base=0x%08x\n", MapRegisterBase);

	pDevExt->MapRegisterBase = MapRegisterBase;

	// signal that we are finished
    KeSetEvent(Context, 0, FALSE);

	return KeepObject;//DeallocateObjectKeepRegisters;
}
#endif

void *my_dma_pool_alloc(struct dma_pool *pool, int gfp_flags, dma_addr_t *dma_handle)
{
	// HalAllocCommonBuffer
	// But ideally IoGetDmaAdapter

	DPRINT1("dma_pool_alloc() called\n");
	return NULL;
}

/*
pci_pool_create --  Creates a pool of pci consistent memory blocks, for dma. 

struct pci_pool * pci_pool_create (const char * name, struct pci_dev * pdev, size_t size, size_t align, size_t allocation, int flags);

Arguments:
name - name of pool, for diagnostics 
pdev - pci device that will be doing the DMA 
size - size of the blocks in this pool. 
align - alignment requirement for blocks; must be a power of two 
allocation - returned blocks won't cross this boundary (or zero) 
flags - SLAB_* flags (not all are supported). 

Description:
Returns a pci allocation pool with the requested characteristics, or null if one can't be created.
Given one of these pools, pci_pool_alloc may be used to allocate memory. Such memory will all have
"consistent" DMA mappings, accessible by the device and its driver without using cache flushing
primitives. The actual size of blocks allocated may be larger than requested because of alignment. 
If allocation is nonzero, objects returned from pci_pool_alloc won't cross that size boundary.
This is useful for devices which have addressing restrictions on individual DMA transfers, such
as not crossing boundaries of 4KBytes. 
*/
struct pci_pool *my_pci_pool_create(const char * name, struct pci_dev * pdev, size_t size, size_t align, size_t allocation)
{
	struct pci_pool		*retval;

	if (align == 0)
		align = 1;
	if (size == 0)
		return 0;
	else if (size < align)
		size = align;
	else if ((size % align) != 0) {
		size += align + 1;
		size &= ~(align - 1);
	}

	if (allocation == 0) {
		if (PAGE_SIZE < size)
			allocation = size;
		else
			allocation = PAGE_SIZE;
		// FIXME: round up for less fragmentation
	} else if (allocation < size)
		return 0;
		
	retval = ExAllocatePool(NonPagedPool, sizeof(struct pci_pool)); // Non-paged because could be
																	// accesses at IRQL < PASSIVE

	// fill retval structure
	strncpy (retval->name, name, sizeof retval->name);
	retval->name[sizeof retval->name - 1] = 0;
	
	retval->allocation = allocation;
	retval->size = size;
	retval->blocks_per_page = allocation / size;
	retval->pdev = pdev;

	retval->pages_allocated = 0;
	retval->blocks_allocated = 0;
	
	DPRINT("pci_pool_create(): %s/%s size %d, %d/page (%d alloc)\n",
		pdev ? pdev->slot_name : NULL, retval->name, size,
		retval->blocks_per_page, allocation);

	return retval;
}

/*
Name:
pci_pool_alloc --  get a block of consistent memory 

Synopsis:
void * pci_pool_alloc (struct pci_pool * pool, int mem_flags, dma_addr_t * handle);

Arguments:
pool - pci pool that will produce the block 

mem_flags - SLAB_KERNEL or SLAB_ATOMIC 

handle - pointer to dma address of block 

Description:
This returns the kernel virtual address of a currently unused block, and reports its dma
address through the handle. If such a memory block can't be allocated, null is returned. 
*/
void * my_pci_pool_alloc(struct pci_pool * pool, int mem_flags, dma_addr_t *dma_handle)
{
	PVOID result;
	POHCI_DEVICE_EXTENSION devExt = (POHCI_DEVICE_EXTENSION)pool->pdev->dev_ext;
	int page=0, offset;
	int map, i, block;

	//DPRINT1("pci_pool_alloc() called, blocks already allocated=%d, dma_handle=%p\n", pool->blocks_allocated, dma_handle);
	//ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	if (pool->pages_allocated == 0)
	{
		// we need to allocate at least one page
		pool->pages[pool->pages_allocated].virtualAddress =
			devExt->pDmaAdapter->DmaOperations->AllocateCommonBuffer(devExt->pDmaAdapter,
				PAGE_SIZE, &pool->pages[pool->pages_allocated].dmaAddress, FALSE); //FIXME: Cache-enabled?

		// mark all blocks as free (bit=set)
		memset(pool->pages[pool->pages_allocated].bitmap, 0xFF, 128*sizeof(unsigned long));

		/* FIXME: the next line replaces physical address by virtual address:
		* this change is needed to boot VMWare, but I'm really not sure this
		* change is correct!
		*/
		//pool->pages[pool->pages_allocated].dmaAddress.QuadPart = (ULONG_PTR)pool->pages[pool->pages_allocated].virtualAddress;
		pool->pages_allocated++;
	}

	// search for a free block in all pages
	for (page=0; page<pool->pages_allocated; page++)
	{
		for (map=0,i=0; i < pool->blocks_per_page; i+= BITS_PER_LONG, map++)
		{
			if (pool->pages[page].bitmap[map] == 0)
				continue;
            
			block = ffz(~ pool->pages[page].bitmap[map]);

			if ((i + block) < pool->blocks_per_page)
			{
				DPRINT("pci_pool_alloc(): Allocating block %p:%d:%d:%d\n", pool, page, map, block);
				clear_bit(block, &pool->pages[page].bitmap[map]);
				offset = (BITS_PER_LONG * map) + block;
				offset *= pool->size;
				goto ready;
			}
		}
	}

	//TODO: alloc page here then
	DPRINT1("Panic!! We need one more page to be allocated, and Fireball doesn't want to alloc it!\n");
	offset = 0;
	return 0;

ready:
	*dma_handle = pool->pages[page].dmaAddress.QuadPart + offset;
	result = (char *)pool->pages[page].virtualAddress + offset;
	pool->blocks_allocated++;

	return result;
}

/*
Name
pci_pool_free --  put block back into pci pool 
Synopsis

void pci_pool_free (struct pci_pool * pool, void * vaddr, dma_addr_t dma);

Arguments

pool - the pci pool holding the block 

vaddr - virtual address of block 

dma - dma address of block 

Description:
Caller promises neither device nor driver will again touch this block unless it is first re-allocated.
*/
void my_pci_pool_free (struct pci_pool * pool, void * vaddr, dma_addr_t dma)
{
	int page, block, map;

	// Find page
	for (page=0; page<pool->pages_allocated; page++)
	{
		if (dma < pool->pages[page].dmaAddress.QuadPart)
			continue;
		if (dma < (pool->pages[page].dmaAddress.QuadPart + pool->allocation))
			break;
	}

	block = dma - pool->pages[page].dmaAddress.QuadPart;
	block /= pool->size;
	map = block / BITS_PER_LONG;
	block %= BITS_PER_LONG;

	// mark as free
	set_bit (block, &pool->pages[page].bitmap[map]);

	pool->blocks_allocated--;
	DPRINT("pci_pool_free(): alloc'd: %d\n", pool->blocks_allocated);
}

/*
pci_pool_destroy --  destroys a pool of pci memory blocks. 
Synopsis

void pci_pool_destroy (struct pci_pool * pool);


Arguments:
pool - pci pool that will be destroyed 

Description
Caller guarantees that no more memory from the pool is in use, and that nothing will try to
use the pool after this call. 
*/
void __inline__ my_pci_pool_destroy (struct pci_pool * pool)
{
	DPRINT1("pci_pool_destroy(): alloc'd: %d, UNIMPLEMENTED\n", pool->blocks_allocated);

	ExFreePool(pool);
}

void  *my_pci_alloc_consistent(struct pci_dev *hwdev, size_t size, dma_addr_t *dma_handle)
{
    POHCI_DEVICE_EXTENSION devExt = (POHCI_DEVICE_EXTENSION)hwdev->dev_ext;
	DPRINT1("pci_alloc_consistent() size=%d\n", size);

    return devExt->pDmaAdapter->DmaOperations->AllocateCommonBuffer(devExt->pDmaAdapter, size, (PPHYSICAL_ADDRESS)dma_handle, FALSE); //FIXME: Cache-enabled?
}

dma_addr_t my_dma_map_single(struct device *hwdev, void *ptr, size_t size, enum dma_data_direction direction)
{
    //PHYSICAL_ADDRESS BaseAddress;
    //POHCI_DEVICE_EXTENSION pDevExt = (POHCI_DEVICE_EXTENSION)hwdev->dev_ext;
    //PUCHAR VirtualAddress = (PUCHAR) MmGetMdlVirtualAddress(pDevExt->Mdl);
	//ULONG transferSize = size;
	//BOOLEAN WriteToDevice;

	//DPRINT1("dma_map_single() ptr=0x%lx, size=0x%x, dir=%d\n", ptr, size, direction);
	/*ASSERT(pDevExt->BufferSize > size);

	// FIXME: It must be an error if DMA_BIDIRECTIONAL trasnfer happens, since MSDN says
	//        the buffer is locked
	if (direction == DMA_BIDIRECTIONAL || direction == DMA_TO_DEVICE)
        WriteToDevice = TRUE;
	else
		WriteToDevice = FALSE;

    DPRINT1("IoMapTransfer\n");
    BaseAddress = pDevExt->pDmaAdapter->DmaOperations->MapTransfer(pDevExt->pDmaAdapter,
                    pDevExt->Mdl,
                    pDevExt->MapRegisterBase,
                    (PUCHAR) MmGetMdlVirtualAddress(pDevExt->Mdl),
                    &transferSize,
                    WriteToDevice);

	if (WriteToDevice)
	{
		DPRINT1("Writing to the device...\n");
		memcpy(VirtualAddress, ptr, size);
	}
	else
	{
		DPRINT1("Reading from the device...\n");
		memcpy(ptr, VirtualAddress, size);
	}*/

	//DPRINT1("VBuffer == 0x%x (really 0x%x?) transf_size == %u\n", pDevExt->VirtualBuffer, MmGetPhysicalAddress(pDevExt->VirtualBuffer).LowPart, transferSize);
	//DPRINT1("VBuffer == 0x%x (really 0x%x?) transf_size == %u\n", ptr, MmGetPhysicalAddress(ptr).LowPart, transferSize);
	
	return MmGetPhysicalAddress(ptr).QuadPart;//BaseAddress.QuadPart; /* BIG HACK */
}

// 2.6 version of pci_unmap_single
//void my_dma_unmap_single(struct device *dev, dma_addr_t dma_addr, size_t size, enum dma_data_direction direction)
void my_dma_unmap_single(struct device *dev, dma_addr_t dma_addr, size_t size, enum dma_data_direction direction)
{
	//DPRINT1("dma_unmap_single() called, nothing to do\n");
	/* nothing yet */
}

void my_dma_sync_single(struct device *hwdev,
				       dma_addr_t dma_handle,
				       size_t size, int direction)
{
	DPRINT1("dma_sync_single() called, UNIMPLEMENTED\n");
	/* nothing yet */
}

void my_dma_sync_sg(struct device *hwdev,
				   struct scatterlist *sg,
				   int nelems, int direction)
{
	DPRINT1("dma_sync_sg() called, UNIMPLEMENTED\n");
	/* nothing yet */
}


int my_dma_map_sg(struct device *hwdev, struct scatterlist *sg, int nents, enum dma_data_direction direction)
{
	DPRINT1("dma_map_sg() called, UNIMPLEMENTED\n");
	return 0;
}

void my_dma_unmap_sg(struct device *hwdev, struct scatterlist *sg, int nents, enum dma_data_direction direction)
{
	DPRINT1("dma_unmap_sg() called, UNIMPLEMENTED\n");
	/* nothing yet */
}

/* forwarder ro dma_ equivalent */
void my_pci_unmap_single(struct pci_dev *hwdev, dma_addr_t dma_addr, size_t size, int direction)
{
	my_dma_unmap_single(&hwdev->dev, dma_addr, size, direction);
}


/*------------------------------------------------------------------------*/ 
/* SPINLOCK routines                                                      */
/*------------------------------------------------------------------------*/ 
void my_spin_lock_init(spinlock_t *sl)
{
	KeInitializeSpinLock(&sl->SpinLock);
}

void my_spin_lock(spinlock_t *sl)
{
	//KeAcquireSpinLock(&sl->SpinLock, &sl->OldIrql);
}

void my_spin_unlock(spinlock_t *sl)
{
	//KeReleaseSpinLock(&sl->SpinLock, sl->OldIrql);
}

void my_spin_lock_irqsave(spinlock_t *sl, int flags)
{
	my_spin_lock(sl);
}
