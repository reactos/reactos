#ifndef DRIMEMSETIO_H
#define DRIMEMSETIO_H
/*
* memset an area in I/O space
* We need to be careful about this on some archs
*/
static __inline__ void drimemsetio(void* address, int c, int size)
{
#if defined(__powerpc__) || defined(__ia64__)
     int i;
     for(i=0;i<size;i++)
        *((char *)address + i)=c;
#else
     memset(address,c,size);
#endif
}
#endif
