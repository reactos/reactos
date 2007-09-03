#include "freeldr.h"
#include "machine.h"
#include "ppcmmu/mmu.h"
#include "prep.h"

#define SWAP_W(x) ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))

typedef struct _idectl_desc {
    int port;
    long long seekto;
    int seek_cylinder, seek_head, seek_sector;
    int cylinders, heads, sectors, bytespersec;
} idectl_desc;

idectl_desc ide1_desc = { 0x800001f0 };

void ide_seek( void *extension, int low, int high ) {
    idectl_desc *desc = (idectl_desc *)extension;
    long long seekto = ((((long long)high) << 32) | (low & 0xffffffff));
    /* order = sector, head, cylinder */
    desc->seek_sector = seekto % desc->sectors;
    seekto /= desc->sectors;
    desc->seek_head = seekto % desc->heads;
    seekto /= desc->heads;
    desc->seek_cylinder = seekto;
    desc->seekto = seekto;
}

/* Thanks chuck moore.  This is based on the color forth ide code */
/* Wait for ready */
void ide_rdy( void *extension ) {
    idectl_desc *desc = (idectl_desc *)extension;
    while( !(GetPhysByte(desc->port+7) & 0x40) ) sync(); 
}

void ide_drq( void *extension ) {
    idectl_desc *desc = (idectl_desc *)extension;
    while( !(GetPhysByte(desc->port+7) & 0x08) ) sync(); 
}

void ide_bsy( void *extension ) {
    idectl_desc *desc = (idectl_desc *)extension;
    while( GetPhysByte(desc->port+7) & 0x80 ) 
    {
	printf("Waiting for not busy\n");
	sync(); 
    }
}

int ide_read( void *extension, char *buffer, int bytes ) {
    idectl_desc *desc = (idectl_desc *)extension;
    short *databuf = (short *)buffer;
    int inwords;

    ide_bsy( extension );
    SetPhysByte(desc->port+2, bytes / desc->bytespersec);
    SetPhysByte(desc->port+3, desc->seek_sector + 1);
    SetPhysByte(desc->port+4, desc->seek_cylinder);
    SetPhysByte(desc->port+5, desc->seek_cylinder >> 8);
    SetPhysByte(desc->port+6, desc->seek_head | 0xa0);
    SetPhysByte(desc->port+7, 0x20);
    
    for( inwords = 0; inwords < desc->bytespersec / sizeof(short); inwords++ ) {
	databuf[inwords] = GetPhysHalf(desc->port);
    }

    desc->seekto += desc->bytespersec;
    ide_seek( extension, desc->seekto, desc->seekto >> 32 );

    return bytes - (bytes % desc->bytespersec);
}

void ide_setup( void *extension ) {
    idectl_desc *desc = (idectl_desc *)extension;
    short identbuffer[256];
    char namebuf[41];
    short *databuf = (short *)identbuffer, in;
    int inwords;

    ide_rdy( extension );
    ide_bsy( extension );
    desc->bytespersec = 512;
    SetPhysByte(desc->port+2, 1);
    SetPhysByte(desc->port+3, 0);
    SetPhysByte(desc->port+4, 0);
    SetPhysByte(desc->port+5, 0);
    SetPhysByte(desc->port+6, 0);
    SetPhysByte(desc->port+7, 0xec);
    ide_drq( extension );

    for( inwords = 0; inwords < desc->bytespersec / sizeof(short); inwords++ ) {
	in = GetPhysHalf(desc->port);
	databuf[inwords] = SWAP_W(in);
	sync();
    }

    desc->cylinders = identbuffer[1];
    desc->heads = identbuffer[3];
    desc->sectors = identbuffer[6];

    /* Debug: Write out hard disc model */

    strncpy(namebuf, (char *)(identbuffer+0x1b), 41);
    printf("HARD DISC MODEL: %s c,h,s %d,%d,%d\n", 
	   namebuf, desc->cylinders, desc->heads, desc->sectors);
}
