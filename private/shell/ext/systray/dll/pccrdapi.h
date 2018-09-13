#ifndef _INC_PCCRDAPI
#define _INC_PCCRDAPI


#ifndef PCCARD_DEVICE_ID
#define PCCARD_DEVICE_ID    0x097C
#endif

//
//  32-bit IOCTL APIs
//
#define PCCARD_IOCTL_GET_VERSION	0
//
//  Output buffer contains array of devnodes for all sockets
//
#define PCCARD_IOCTL_GET_DEVNODES	1
//
//  Input:  Devnode handle
//  Output: String description of device
//
#define PCCARD_IOCTL_GET_DESCRIPTION	2


//
//  16-Bit APIs
//
//  ENTRY:
//	  AX = 0000h - Get Version Information
//  EXIT:
//	  AX = Version number
//	  Carry clear
#define PCCARD_API_GET_VERSION		0x0000


//  ENTRY:
//	  AX = 100h - Get Devnodes for Sockets
//	  CX = user buffer length in bytes
//	  ES:BX = pointer to user buffer
//  EXIT:
//	  CX = number of logical sockets in system
//	  Carry set if error
//	  Carry clear, user buffer is filled in with devnodes for sockets
//
#define PCCARD_API_GET_DEVNODES 	0x0100

#endif // _INC_PCCRDAPI
