#include <freeldr.h>
#include "prep.h"

typedef struct _pci_cfg {
    unsigned long addr;
    unsigned long data;
} pci_cfg;

typedef struct _pci_desc {
    pci_cfg *cfg;
} pci_desc;

pci_desc pci1_desc = { (void *)0x80000cf8 };
#define rev16(x) ((((x)>>8)&0xff)|(((x)&0xff)<<8))
#define rev32(x) ((((x)>>24)&0xff)|(((x)>>8)&0xff00)|(((x)&0xff00)<<8)|(((x)&0xff)<<24))
#define pci_addr(bus,dev,fn,reg) \
         (0x80000000 | \
          ((bus & 0xff) << 16) | \
          ((dev & 0x1f) << 11) | \
          ((fn & 7) << 8) | \
          (reg & 0xfc))
#if 0
#define pci_cfg_addr(bus,dev,fn,reg) \
        ((bus == 0) ? \
         ((1 << (dev + 16)) | \
          (dev << 11) | \
          (fn << 8) | \
          ((reg & 0xfc) | 1)) : pci_addr(bus,dev,fn,reg))
#else
#define pci_cfg_addr(bus,dev,fn,reg) pci_addr(bus,dev,fn,reg)
#endif
    unsigned long pci_read( pci_desc *desc, int bus, int dev, int fn, int reg, int len ) {
	sync();
	unsigned long save_state = desc->cfg->addr, ret = 0;
	unsigned long addr = pci_cfg_addr(bus,dev,fn,reg);
	unsigned long offset = reg & 3;
	desc->cfg->addr = rev32(addr);
	sync();
	switch( len ) {
	case 4:
	    ret = desc->cfg->data;
	    break;
	case 2:
	    ret = desc->cfg->data;
	    ret = (ret >> (offset << 3)) & 0xffff;
	    break;
	case 1:
	    ret = desc->cfg->data;
	    ret = (ret >> (offset << 3)) & 0xff;
	    break;
	}
	desc->cfg->addr = save_state;
	sync();
	return ret;
    }

void pci_read_bar( pci_desc *desc, int bus, int dev, int fn, int bar, 
		   struct _pci_bar *bar_data ) {
    bar_data->data = pci_read( desc, bus, dev, fn, 0x10 + (bar * 4), 4 );
}

/* 
 * Imagine: offset 3, len 1
 * let oldval = 0x12345678 and val = 0xabcd1234;
 * mask = ((1 << 8) - 1) << 24; // 0xff000000
 * oldval = (0x12345678 & 0x00ffffff) | (0xabcd1234 & 0xff000000) = 0xab345678;
 */
void pci_write( pci_desc *desc, int bus, int dev, int fn, int reg, int len, int val ) {
    unsigned long save_state = desc->cfg->addr;
    unsigned long addr = pci_cfg_addr(bus,dev,fn,reg);
    unsigned long offset = reg & 3;
    unsigned long oldval = pci_read( desc, bus, dev, fn, reg & ~3, 4 );
    unsigned long mask = ((1 << (len * 8)) - 1) << (offset << 3);
    oldval = (oldval & ~mask) | ((val << (offset << 3)) & mask);
    desc->cfg->addr = rev32(addr);
    sync();
    desc->cfg->data = rev32(oldval);
    sync();
    desc->cfg->addr = save_state;
    sync();
}

void pci_write_bar( pci_desc *desc, int bus, int dev, int fn, int bar, struct _pci_bar *bar_data ) {
    pci_write( desc, bus, dev, fn, 0x10 + (bar * 4), 4, bar_data->data );
}

void print_bar( struct _pci_bar *bar ) {
    printf("BAR: %x\n", bar->data);
}

#define PCI_VENDORID 0
#define PCI_DEVICEID 2
#define PCI_HEADER_TYPE 0xe
#define PCI_BASECLASS   0xb

void pci_setup( pci_desc *desc ) {
    unsigned char type;
    unsigned short vendor, device, devclass;
    int funcs, bus, dev, fn;

    pci1_desc.cfg = (pci_cfg *)0x80000cf8;

    printf("PCI Bus:\n");
    for( bus = 0; bus < 1; bus++ ) {
        for( dev = 0; dev < 32; dev++ ) {
            type = pci_read(desc,bus,dev,0,PCI_HEADER_TYPE,1);
            vendor = pci_read(desc,bus,dev,0,PCI_VENDORID,2);
            device = pci_read(desc,bus,dev,0,PCI_DEVICEID,2);
            
            if(vendor == 0 || vendor == 0xffff) continue;
            if(type & 0x80) funcs = 8; else funcs = 1;

            for( fn = 0; fn < funcs; fn++ ) {
                devclass = pci_read(desc,bus,dev,fn,PCI_BASECLASS,1);
		printf(" %d:%d -> vendor:device:class %x:%x:%x\n",
		       bus, dev, vendor, device, devclass);
                
                if( devclass == 3 ) {
		    printf("Setting up vga...\n");
                    vga_setup(desc,&vga1_desc,bus,dev,fn);
		    printf("Done with vga\n");
                }
            }
        }
    }
    printf("^-- end PCI\n");
}
