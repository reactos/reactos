#ifndef FREELDR_ARCH_POWERPC_PREP_H
#define FREELDR_ARCH_POWERPC_PREP_H

extern struct _pci_desc pci1_desc;
extern struct _idectl_desc ide1_desc;
extern struct _vga_desc vga1_desc;
struct _pci_bar {
    unsigned long data;
};

void sync();
void PpcPrepInit();
void ide_seek( void *extension, int low, int high );
int  ide_read( void *extension, char *buffer, int bytes );
void ide_setup( void *extension );

void print_bar( struct _pci_bar *bar );
void pci_setup( struct _pci_desc *pci_desc );
void pci_read_bar( struct _pci_desc *pci_desc, int bus, int dev, int fn, int bar, struct _pci_bar *bar_data );

void vga_setup( struct _pci_desc *pci_desc, struct _vga_desc *vga_desc,
		int bus, int dev, int fn );

#endif//FREELDR_ARCH_POWERPC_PREP_H
