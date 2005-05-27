// PCI -> HAL interface
// this file is part of linux_wrapper.h

//FIXME: Move this file, make its definitions more general
#include "../host/ohci_main.h"

/*
  Initialize device before it's used by a driver. Ask low-level code to enable I/O and memory.
  Wake up the device if it was suspended. Beware, this function can fail. 
 */
static int __inline__ pci_enable_device(struct pci_dev *dev)
{
	DPRINT1("pci_enable_device() called...\n");
	return 0;
}

// Get physical address where resource x resides
static PHYSICAL_ADDRESS __inline__ pci_resource_start (struct pci_dev *dev, int x)
{
	POHCI_DEVICE_EXTENSION dev_ext = (POHCI_DEVICE_EXTENSION)dev->dev_ext;
	DPRINT1("pci_resource_start() called, x=0x%x\n", x);
	
	//FIXME: Take x into account
    return dev_ext->BaseAddress;
	//return dev->base[x];
}

// ???
static unsigned long __inline__ pci_resource_len (struct pci_dev *dev, int x)
{
	POHCI_DEVICE_EXTENSION ext = (POHCI_DEVICE_EXTENSION)dev->dev_ext;

	DPRINT1("pci_resource_len() called, x=0x%x\n", x);

	//FIXME: Take x into account
    return ext->BaseAddrLength;
}

// ???
static int __inline__ pci_resource_flags(struct pci_dev *dev, int x)
{
	POHCI_DEVICE_EXTENSION ext = (POHCI_DEVICE_EXTENSION)dev->dev_ext;
	
	DPRINT1("pci_resource_flags() called, x=0x%x\n", x);
	
		//FIXME: Take x into account
    return ext->Flags;
}

/*
   Enables bus-mastering for device dev
*/
static int __inline__ pci_set_master(struct pci_dev *dev) {return 0;}

// Store pointer to data for this device
static int __inline__ pci_set_drvdata(struct pci_dev *dev, void* d)
{
	DPRINT1("pci_set_drvdata() called...\n");
	dev->data=(void*)d;
	return 0;
}

// Get pointer to previously saved data
static void __inline__ *pci_get_drvdata(struct pci_dev *dev)
{
	DPRINT1("pci_get_drvdata() called...\n");
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
static int __inline__ request_region(PHYSICAL_ADDRESS addr, unsigned long len, const char * d)
{
	DPRINT1("request_region(): addr=0x%lx, len=0x%lx\n", addr.u.LowPart, len);
	return ~0;
}

/*
Unmap I/O memory from kernel address space. 

Parameters:
addr  virtual start address 

*/
static int __inline__ iounmap(void* p)
{
	DPRINT1("iounmap(): p=0x%x. FIXME - how to obtain len of mapped region?\n", p);
	
	//MmUnnapIoSpace(p);
	
	return 0;
}

/*
Release I/O port region. 

Parameters:
start  begin of region  
n  length of region  
*/
static int __inline__ release_region(PHYSICAL_ADDRESS addr, unsigned long len)
{
	DPRINT1("release_region(): addr=0x%lx, len=0x%lx\n", addr.u.LowPart, len);
	return 0;
}

/*
Allocate I/O memory region. 

Parameters:
start  begin of region  
n      length of region  
name   name of requester 
*/
static int __inline__ request_mem_region(PHYSICAL_ADDRESS addr, unsigned long len, const char * d)
{
	DPRINT1("request_mem_region(): addr=0x%lx, len=0x%lx\n", addr.u.LowPart, len);
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
static void __inline__ *ioremap_nocache(PHYSICAL_ADDRESS addr, unsigned long len)
{
	// MmMapIoSpace with NoCache param
	DPRINT1("ioremap_nocache(): addr=0x%lx, len=0x%lx\n", addr.u.LowPart, len);

	return MmMapIoSpace(addr, len, MmNonCached);
}

/*
Release I/O memory region. 

Parameters:
start  begin of region  
n      length of region  
*/
static int __inline__ release_mem_region(PHYSICAL_ADDRESS addr, unsigned long len)
{
	DPRINT1("release_mem_region(): addr=0x%lx, len=0x%lx\n", addr.u.LowPart, len);
	return 0;
}
