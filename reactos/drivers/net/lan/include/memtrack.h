#ifndef MEMTRACK_H
#define MEMTRACK_H

#ifndef FOURCC
#define FOURCC(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))
#endif

#define AllocatePacketWithBuffer(x,y,z) AllocatePacketWithBufferX(x,y,z,__FILE__,__LINE__)
#define FreeNdisPacket(x) FreeNdisPacketX(x,__FILE__,__LINE__)

#define MTMARK()
#define Track(x,y)
#define TrackingInit()
#define TrackDump()
#define Untrack(x)
#define TrackTag(x)
#define exAllocatePoolWithTag(x,y,z) ExAllocatePoolWithTag(x,y,z)
#define exAllocatePool(x,y) ExAllocatePool(x,y)
#define exFreePool(x) ExFreePool(x)
#define TrackWithTag(w,x,y,z)
#define UntrackFL(x,y,z)

#endif/*MEMMTRAC_H*/
