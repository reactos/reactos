#ifndef __CROSSNT_MISC__H__
#define __CROSSNT_MISC__H__

/* The definitions look so crappy, because the code doesn't care 
   whether the source is an array or an integer */
#define MOV_DD_SWP(a,b) ((a) = RtlUlongByteSwap(*(PULONG)&(b)))
#define MOV_DW_SWP(a,b) ((a) = RtlUshortByteSwap(*(PUSHORT)&(b)))
#define MOV_SWP_DW2DD(a,b) ((a) = RtlUshortByteSwap(*(PUSHORT)&(b)))

#endif // __CROSSNT_MISC__H__
