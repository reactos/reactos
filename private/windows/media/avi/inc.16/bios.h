/***
*bios.h - declarations for bios interface functions and supporting definitions
*
*   Copyright (c) 1987-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   This file declares the constants, structures, and functions
*   used for accessing and using various BIOS interfaces.
*
****/

#ifndef _INC_BIOS

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

#ifndef _MT

/* manifest constants for BIOS serial communications (RS-232) support */

/* serial port services */

#define _COM_INIT   0   /* init serial port */
#define _COM_SEND   1   /* send character */
#define _COM_RECEIVE    2   /* receive character */
#define _COM_STATUS 3   /* get serial port status */

/* serial port initializers.  One and only one constant from each of the
 * following four groups - character size, stop bit, parity, and baud rate -
 * must be specified in the initialization byte.
 */

/* character size initializers */

#define _COM_CHR7   2   /* 7 bits characters */
#define _COM_CHR8   3   /* 8 bits characters */

/* stop bit values - on or off */

#define _COM_STOP1  0   /* 1 stop bit */
#define _COM_STOP2  4   /* 2 stop bits */

/*  parity initializers */

#define _COM_NOPARITY   0   /* no parity */
#define _COM_ODDPARITY  8   /* odd parity */
#define _COM_EVENPARITY 24  /* even parity */

/*  baud rate initializers */

#define _COM_110    0   /* 110 baud */
#define _COM_150    32  /* 150 baud */
#define _COM_300    64  /* 300 baud */
#define _COM_600    96  /* 600 baud */
#define _COM_1200   128 /* 1200 baud */
#define _COM_2400   160 /* 2400 baud */
#define _COM_4800   192 /* 4800 baud */
#define _COM_9600   224 /* 9600 baud */


/* manifest constants for BIOS disk support */

/* disk services */

#define _DISK_RESET 0   /* reset disk controller */
#define _DISK_STATUS    1   /* get disk status */
#define _DISK_READ  2   /* read disk sectors */
#define _DISK_WRITE 3   /* write disk sectors */
#define _DISK_VERIFY    4   /* verify disk sectors */
#define _DISK_FORMAT    5   /* format disk track */

/* struct used to send/receive information to/from the BIOS disk services */

#ifndef _DISKINFO_T_DEFINED
#pragma pack(2)

struct _diskinfo_t {
    unsigned drive;
    unsigned head;
    unsigned track;
    unsigned sector;
    unsigned nsectors;
    void __far *buffer;
    };

#ifndef __STDC__
/* Non-ANSI name for compatibility */
#define diskinfo_t _diskinfo_t
#endif 

#pragma pack()
#define _DISKINFO_T_DEFINED
#endif 


/* manifest constants for BIOS keyboard support */

/* keyboard services */

#define _KEYBRD_READ        0   /* read next character from keyboard */
#define _KEYBRD_READY       1   /* check for keystroke */
#define _KEYBRD_SHIFTSTATUS 2   /* get current shift key status */

/* services for enhanced keyboards */

#define _NKEYBRD_READ       0x10    /* read next character from keyboard */
#define _NKEYBRD_READY      0x11    /* check for keystroke */
#define _NKEYBRD_SHIFTSTATUS    0x12    /* get current shift key status */


/* manifest constants for BIOS printer support */

/* printer services */

#define _PRINTER_WRITE  0   /* write character to printer */
#define _PRINTER_INIT   1   /* intialize printer */
#define _PRINTER_STATUS 2   /* get printer status */


/* manifest constants for BIOS time of day support */

/* time of day services */

#define _TIME_GETCLOCK  0   /* get current clock count */
#define _TIME_SETCLOCK  1   /* set current clock count */


#ifndef _REGS_DEFINED

/* word registers */

struct _WORDREGS {
    unsigned int ax;
    unsigned int bx;
    unsigned int cx;
    unsigned int dx;
    unsigned int si;
    unsigned int di;
    unsigned int cflag;
    };

/* byte registers */

struct _BYTEREGS {
    unsigned char al, ah;
    unsigned char bl, bh;
    unsigned char cl, ch;
    unsigned char dl, dh;
    };

/* general purpose registers union -
 *  overlays the corresponding word and byte registers.
 */

union _REGS {
    struct _WORDREGS x;
    struct _BYTEREGS h;
    };

/* segment registers */

struct _SREGS {
    unsigned int es;
    unsigned int cs;
    unsigned int ss;
    unsigned int ds;
    };

#ifndef __STDC__
/* Non-ANSI names for compatibility */

struct WORDREGS {
    unsigned int ax;
    unsigned int bx;
    unsigned int cx;
    unsigned int dx;
    unsigned int si;
    unsigned int di;
    unsigned int cflag;
    };

struct BYTEREGS {
    unsigned char al, ah;
    unsigned char bl, bh;
    unsigned char cl, ch;
    unsigned char dl, dh;
    };

union REGS {
    struct WORDREGS x;
    struct BYTEREGS h;
    };

struct SREGS {
    unsigned int es;
    unsigned int cs;
    unsigned int ss;
    unsigned int ds;
    };

#endif 

#define _REGS_DEFINED
#endif 


/* function prototypes */

#ifndef _WINDOWS
unsigned __cdecl _bios_disk(unsigned, struct _diskinfo_t *);
#endif 
unsigned __cdecl _bios_equiplist(void);
#ifndef _WINDOWS
unsigned __cdecl _bios_keybrd(unsigned);
#endif 
unsigned __cdecl _bios_memsize(void);
#ifndef _WINDOWS
unsigned __cdecl _bios_printer(unsigned, unsigned, unsigned);
unsigned __cdecl _bios_serialcom(unsigned, unsigned, unsigned);
#endif 
unsigned __cdecl _bios_timeofday(unsigned, long *);
int __cdecl _int86(int, union _REGS *, union _REGS *);
int __cdecl _int86x(int, union _REGS *, union _REGS *, struct _SREGS *);

#ifndef __STDC__
/* Non-ANSI names for compatibility */
int __cdecl int86(int, union REGS *, union REGS *);
int __cdecl int86x(int, union REGS *, union REGS *, struct SREGS *);
#endif 

#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_BIOS
#endif 
