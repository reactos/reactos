#ifndef __INCLUDE_DDK_IOCTRL_H
#define __INCLUDE_DDK_IOCTRL_H


#define CTL_CODE(Dev, Func, Meth, Acc) ( ((Dev)<<16) | ((Acc)<<14) | ((Func)<<2) | (Meth))

//  IOCTL Parameter buffering methods
#define METHOD_BUFFERED    0
#define METHOD_IN_DIRECT   1
#define METHOD_OUT_DIRECT  2
#define METHOD_NEITHER     3

//  IOCTL File access type
#define FILE_ANY_ACCESS    0
#define FILE_READ_ACCESS   1
#define FILE_WRITE_ACCESS  2

#endif /* __INCLUDE_DDK_IOCTRL_H */
