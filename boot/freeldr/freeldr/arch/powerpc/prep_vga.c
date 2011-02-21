#include <freeldr.h>
#include "prep.h"

struct _vga_desc {
    char *port;
    char *addr;
};

#define VGA_WIDTH 1024
#define VGA_HEIGHT 768
struct _vga_desc vga1_desc = { (char *)0x800003c0 };

void vga_setup( PCONFIGURATION_COMPONENT_DATA pcibus, 
                struct _pci_desc *desc, struct _vga_desc *vga_desc,
		int bus, int dev, int fn ) {
    struct _pci_bar bar_data;
    int i;

    for( i = 0; i < 6; i++ ) {
        pci_read_bar( desc, bus, dev, fn, i, &bar_data );
        print_bar( &bar_data );
        if( (bar_data.data > 0x10000) || ((bar_data.data&1) == 1) ) {
            vga_desc->addr = (char *)(0xc0000000 + (bar_data.data & ~0x7ff));
//	    BootInfo.dispDeviceBase = vga_desc->addr;
            break;
        }
    }
}
