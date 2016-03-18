#pragma once

extern struct _pci_desc pci1_desc;
extern struct _idectl_desc ide1_desc;
extern struct _vga_desc vga1_desc;
struct _pci_bar {
    unsigned long data;
};

void sync( void );
void PpcPrepInit( void );
void ide_seek( void *extension, int low, int high );
int  ide_read( void *extension, char *buffer, int bytes );
void ide_setup( void *extension );

void print_bar( struct _pci_bar *bar );
void pci_setup
( PCONFIGURATION_COMPONENT_DATA pci_bus,
  struct _pci_desc *pci_desc );
void pci_read_bar
( struct _pci_desc *pci_desc,
  int bus, int dev, int fn, int bar,
  struct _pci_bar *bar_data );

void vga_setup
( PCONFIGURATION_COMPONENT_DATA pci_bus,
  struct _pci_desc *pci_desc, struct _vga_desc *vga_desc,
  int bus, int dev, int fn );
