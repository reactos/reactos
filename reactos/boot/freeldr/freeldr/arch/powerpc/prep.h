#ifndef FREELDR_ARCH_POWERPC_PREP_H
#define FREELDR_ARCH_POWERPC_PREP_H

extern struct _idectl_desc ide1_desc;

void sync();
void PpcPrepInit();
void ide_seek( void *extension, int low, int high );
void ide_read( void *extension, char *buffer, int bytes );
void ide_setup( void *extension );

#endif//FREELDR_ARCH_POWERPC_PREP_H
