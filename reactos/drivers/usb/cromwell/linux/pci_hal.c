// PCI -> HAL interface
// this file is part of linux_wrapper.h

/*
  Initialize device before it's used by a driver. Ask low-level code to enable I/O and memory.
  Wake up the device if it was suspended. Beware, this function can fail. 
 */
static int __inline__ pci_enable_device(struct pci_dev *dev)
{
	return 0;
}

// Get physical address where resource x resides
static unsigned long __inline__ pci_resource_start (struct pci_dev *dev, int x)
{
	// HalGetBusData...
	// HalAssignSlotResources ?
	return dev->base[x];
}

// ???
static unsigned long __inline__ pci_resource_len (struct pci_dev *dev, int x){return 0;}

// ???
static int __inline__ pci_resource_flags(struct pci_dev *dev, int x)
{
	return dev->flags[x];
}

/*
   Enables bus-mastering for device dev
*/
static int __inline__ pci_set_master(struct pci_dev *dev) {return 0;}

// Store pointer to data for this device
static int __inline__ pci_set_drvdata(struct pci_dev *dev, void* d)
{
	dev->data=(void*)d;
	return 0;
}

// Get pointer to previously saved data
static void __inline__ *pci_get_drvdata(struct pci_dev *dev)
{
	return dev->data;
}


/*
   ===========================================================================
   I/O mem related stuff below
*/

/*
Allocate I/O memory region. 

Parameters:
start  begin of region  
n      length of region  
name   name of requester 
*/
static int __inline__ request_region(unsigned long addr, unsigned long len, const char * d){return 0;}

/*
Unmap I/O memory from kernel address space. 

Parameters:
addr  virtual start address 
*/
static int __inline__ iounmap(void* p)
{
	return 0;
}

/*
Release I/O port region. 

Parameters:
start  begin of region  
n  length of region  
*/
static int __inline__ release_region(unsigned long addr, unsigned long len){return 0;}

/*
Allocate I/O memory region. 

Parameters:
start  begin of region  
n      length of region  
name   name of requester 
*/
static int __inline__ request_mem_region(unsigned long addr, unsigned long len, const char * d)
{
	return 1;
}

/*
Remap I/O memory into kernel address space (no cache). 

Parameters:
phys_addr  begin of physical address range  
size       size of physical address range 

Returns:
virtual start address of mapped range
*/
static void __inline__ *ioremap_nocache(unsigned long addr, unsigned long len)
{
	// MmMapIoSpace ?
	return (void*)addr;
}

/*
Release I/O memory region. 

Parameters:
start  begin of region  
n      length of region  
*/
static int __inline__ release_mem_region(unsigned long addr, unsigned long len)
{
	return 0;
}
