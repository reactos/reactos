#ifndef __BYTESEX_H_
#define __BYTESEX_H_

#ifdef _PPC_
#define SWAPD(x) ((((x)&0xff)<<24)|(((x)&0xff00)<<8)|(((x)>>8)&0xff00)|(((x)>>24)&0xff))
#define SWAPW(x) ((((x)&0xff)<<8)|(((x)>>8)&0xff))
#else
#define SWAPD(x) x
#define SWAPW(x) x
#endif
#define SD(Object,Field) Object->Field = SWAPD(Object->Field)
#define SW(Object,Field) Object->Field = SWAPW(Object->Field)

#endif/*__BYTESEX_H_*/
