#ifndef _Boot_H_
#define _Boot_H_

#include "config.h"

/***************************************************************************
      Includes used by XBox boot code
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/////////////////////////////////
// configuration

#include "consts.h"
#include "stdint.h"
#include "cromwell_types.h"


unsigned int cromwell_config;
unsigned int cromwell_retryload;
unsigned int cromwell_loadbank;
unsigned int cromwell_Biostype;

unsigned int xbox_ram;

#define XROMWELL	0
#define CROMWELL	1

#define ICON_WIDTH 64
#define ICON_HEIGHT 64

static inline double min (double a, double b)
{
	if (a < b) return a; else return b;
}

static inline double max (double a, double b)
{
	if (a > b) return a; else return b;
}

#include "iso_fs.h"
#include "BootVideo.h"

#define ASSERT(exp) { if(!(exp)) { bprintf("Assert failed file " __FILE__ " line %d\n", __LINE__); } }

extern volatile CURRENT_VIDEO_MODE_DETAILS vmode;
unsigned int video_encoder;

volatile DWORD VIDEO_CURSOR_POSX;
volatile DWORD VIDEO_CURSOR_POSY;
volatile DWORD VIDEO_ATTR;
volatile DWORD VIDEO_LUMASCALING;
volatile DWORD VIDEO_RSCALING;
volatile DWORD VIDEO_BSCALING;
volatile DWORD BIOS_TICK_COUNT;
volatile DWORD VIDEO_VSYNC_POSITION;
volatile DWORD VIDEO_VSYNC_DIR;
volatile DWORD DVD_TRAY_STATE;

BYTE VIDEO_AV_MODE ;

#define DVD_CLOSED 		0
#define DVD_CLOSING 		1
#define DVD_OPEN   		2
#define DVD_OPENING   		3

/////////////////////////////////
// Superfunky i386 internal structures

typedef struct gdt_t {
        unsigned short       m_wSize __attribute__ ((packed));
        unsigned long m_dwBase32 __attribute__ ((packed));
        unsigned short       m_wDummy __attribute__ ((packed));
} ts_descriptor_pointer;

typedef struct {  // inside an 8-byte protected mode interrupt vector
	WORD m_wHandlerHighAddressLow16;
	WORD m_wSelector;
	WORD m_wType;
	WORD m_wHandlerLinearAddressHigh16;
} ts_pm_interrupt;

typedef enum {
	EDT_UNKNOWN= 0,
	EDT_XBOXFS
} enumDriveType;

typedef struct tsHarddiskInfo {  // this is the retained knowledge about an IDE device after init
    unsigned short m_fwPortBase;
    unsigned short m_wCountHeads;
    unsigned short m_wCountCylinders;
    unsigned short m_wCountSectorsPerTrack;
    unsigned long m_dwCountSectorsTotal; /* total */
    unsigned char m_bLbaMode;	/* am i lba (0x40) or chs (0x00) */
    unsigned char m_szIdentityModelNumber[40];
    unsigned char term_space_1[2];
    unsigned char m_szSerial[20];
    unsigned char term_space_2[2];
    char m_szFirmware[8];
    unsigned char term_space_3[2];
    unsigned char m_fDriveExists;
    unsigned char m_fAtapi;  // true if a CDROM, etc
    enumDriveType m_enumDriveType;
    unsigned char m_bCableConductors;  // valid for device 0 if present
    unsigned short m_wAtaRevisionSupported;
    unsigned char s_length;
    unsigned char m_length;
    unsigned char m_fHasMbr;
} tsHarddiskInfo;

/////////////////////////////////
// LED-flashing codes
// or these together as argument to I2cSetFrontpanelLed

enum {
	I2C_LED_RED0 = 0x80,
	I2C_LED_RED1 = 0x40,
	I2C_LED_RED2 = 0x20,
	I2C_LED_RED3 = 0x10,
	I2C_LED_GREEN0 = 0x08,
	I2C_LED_GREEN1 = 0x04,
	I2C_LED_GREEN2 = 0x02,
	I2C_LED_GREEN3 = 0x01
};

///////////////////////////////
/* BIOS-wide error codes		all have b31 set  */

enum {
	ERR_SUCCESS = 0,  // completed without error

	ERR_I2C_ERROR_TIMEOUT = 0x80000001,  // I2C action failed because it did not complete in a reasonable time
	ERR_I2C_ERROR_BUS = 0x80000002, // I2C action failed due to non retryable bus error

	ERR_BOOT_PIC_ALG_BROKEN = 0x80000101 // PIC algorithm did not pass its self-test
};

/////////////////////////////////
// some Boot API prototypes

//////// BootPerformPicChallengeResponseAction.c

/* ----------------------------  IO primitives -----------------------------------------------------------
*/

static __inline void IoOutputByte(WORD wAds, BYTE bValue) {
//	__asm__  ("			     out	%%al,%%dx" : : "edx" (dwAds), "al" (bValue)  );
    __asm__ __volatile__ ("outb %b0,%w1": :"a" (bValue), "Nd" (wAds));
}

static __inline void IoOutputWord(WORD wAds, WORD wValue) {
//	__asm__  ("	 out	%%ax,%%dx	" : : "edx" (dwAds), "ax" (wValue)  );
    __asm__ __volatile__ ("outw %0,%w1": :"a" (wValue), "Nd" (wAds));
	}

static __inline void IoOutputDword(WORD wAds, DWORD dwValue) {
//	__asm__  ("	 out	%%eax,%%dx	" : : "edx" (dwAds), "ax" (wValue)  );
    __asm__ __volatile__ ("outl %0,%w1": :"a" (dwValue), "Nd" (wAds));
}


static __inline BYTE IoInputByte(WORD wAds) {
  unsigned char _v;

  __asm__ __volatile__ ("inb %w1,%0":"=a" (_v):"Nd" (wAds));
  return _v;
}

static __inline WORD IoInputWord(WORD wAds) {
  WORD _v;

  __asm__ __volatile__ ("inw %w1,%0":"=a" (_v):"Nd" (wAds));
  return _v;
}

static __inline DWORD IoInputDword(WORD wAds) {
  DWORD _v;

  __asm__ __volatile__ ("inl %w1,%0":"=a" (_v):"Nd" (wAds));
  return _v;
}

#define rdmsr(msr,val1,val2) \
       __asm__ __volatile__("rdmsr" \
			    : "=a" (val1), "=d" (val2) \
			    : "c" (msr))

#define wrmsr(msr,val1,val2) \
     __asm__ __volatile__("wrmsr" \
			  : /* no outputs */ \
			  : "c" (msr), "a" (val1), "d" (val2))


void BootPciInterruptEnable(void);

	// boot process
int BootPerformPicChallengeResponseAction(void);
	// LED control (see associated enum above)
int I2cSetFrontpanelLed(BYTE b);

#define bprintf(...)

#if PRINT_TRACE
#define TRACE bprintf(__FILE__ " :%d\n\r",__LINE__);
#else
#define TRACE
#endif

#define SPAMI2C() 				__asm__ __volatile__ (\
	"retry: ; "\
	"movb $0x55, %al ;"\
	"movl $0xc004, %edx ;"\
	"shl	$1, %al ;"\
	"out %al, %dx ;"\
\
	"movb $0xaa, %al ;"\
	"add $4, %edx ;"\
	"out %al, %dx ;"\
\
	"movb $0xbb, %al ;"\
	"sub $0x2, %edx ;"\
	"out %al, %dx ;"\
\
	"sub $6, %edx ;"\
	"in %dx, %ax ;"\
	"out %ax, %dx ;"\
	"add $2, %edx ;"\
	"movb $0xa, %al ;"\
	"out %al, %dx ;"\
	"sub $0x2, %dx ;"\
"spin: ;"\
	"in %dx, %al ;"\
	"test $8, %al ;"\
	"jnz spin ;"\
	"test $8, %al ;"\
	"jnz retry ;"\
	"test $0x24, %al ;"\
\
	"jmp retry"\
);


typedef struct _LIST_ENTRY {
	struct _LIST_ENTRY *m_plistentryNext;
	struct _LIST_ENTRY *m_plistentryPrevious;
} LIST_ENTRY;

void ListEntryInsertAfterCurrent(LIST_ENTRY *plistentryCurrent, LIST_ENTRY *plistentryNew);
void ListEntryRemove(LIST_ENTRY *plistentryCurrent);

////////// BootPerformXCodeActions.c

int BootPerformXCodeActions(void);

#include "BootEEPROM.h"
#include "BootParser.h"

////////// BootStartBios.c

void StartBios(CONFIGENTRY *config,int nActivePartition, int nFATXPresent,int bootfrom);
int BootMenu(CONFIGENTRY *config,int nDrive,int nActivePartition, int nFATXPresent);

////////// BootResetActions.c
void ClearIDT (void);
void BootResetAction(void);
void BootCpuCache(bool fEnable) ;
int printk(const char *szFormat, ...);
void BiosCmosWrite(BYTE bAds, BYTE bData);
BYTE BiosCmosRead(BYTE bAds);


///////// BootPciPeripheralInitialization.c
void BootPciPeripheralInitialization(void);
void BootAGPBUSInitialization(void);
void BootDetectMemorySize(void);
extern void	ReadPCIByte(unsigned int bus, unsigned int dev, unsigned intfunc, 	unsigned int reg_off, unsigned char *pbyteval);
extern void	WritePCIByte(unsigned int bus, unsigned int dev, unsigned int func,	unsigned int reg_off, unsigned char byteval);
extern void	ReadPCIDword(unsigned int bus, unsigned int dev, unsigned int func,	unsigned int reg_off, unsigned int *pdwordval);
extern void	WritePCIDword(unsigned int bus, unsigned int dev, unsigned int func,		unsigned int reg_off, unsigned int dwordval);
extern void	ReadPCIBlock(unsigned int bus, unsigned int dev, unsigned int func,		unsigned int reg_off, unsigned char *buf,	unsigned int nbytes);
extern void	WritePCIBlock(unsigned int bus, unsigned int dev, unsigned int func, 	unsigned int reg_off, unsigned char *buf, unsigned int nbytes);

void PciWriteByte (unsigned int bus, unsigned int dev, unsigned int func,
		unsigned int reg_off, unsigned char byteval);
BYTE PciReadByte(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);
DWORD PciWriteDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, DWORD dw);
DWORD PciReadDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);

///////// BootPerformPicChallengeResponseAction.c

int I2CTransmitWord(BYTE bPicAddressI2cFormat, WORD wDataToWrite);
int I2CTransmitByteGetReturn(BYTE bPicAddressI2cFormat, BYTE bDataToWrite);
bool I2CGetTemperature(int *, int *);
void I2CModifyBits(BYTE bAds, BYTE bReg, BYTE bData, BYTE bMask);

///////// BootIde.c

extern tsHarddiskInfo tsaHarddiskInfo[];  // static struct stores data about attached drives
int BootIdeInit(void);
int BootIdeReadSector(int nDriveIndex, void * pbBuffer, unsigned int block, int byte_offset, int n_bytes);
int BootIdeBootSectorHddOrElTorito(int nDriveIndex, BYTE * pbaResult);
int BootIdeAtapiAdditionalSenseCode(int nDrive, BYTE * pba, int nLengthMaxReturn);
int BootIdeSetTransferMode(int nIndexDrive, int nMode);
int BootIdeWaitNotBusy(unsigned uIoBase);
bool BootIdeAtapiReportFriendlyError(int nDriveIndex, char * szErrorReturn, int nMaxLengthError);
void BootIdeAtapiPrintkFriendlyError(int nDriveIndex);

///////// BootUSB.c

void BootStopUSB(void);
void BootStartUSB(void);
void USBGetEvents(void);

#include "xpad.h"

extern struct xpad_data XPAD_current[4];
extern struct xpad_data XPAD_last[4];

extern void wait_ms(DWORD ticks);
extern void wait_us(DWORD ticks);
extern void wait_smalldelay(void);


void * memcpy(void *dest, const void *src,  size_t size);
void * memset(void *dest, int data,  size_t size);
int memcmp(const void *buffer1, const void *buffer2, size_t num);
int _strncmp(const char *sz1, const char *sz2, int nMax);
char * strcpy(char *sz, const char *szc);
char * _strncpy (char * dest, const char * src, size_t n);
void chrreplace(char *string, char search, char ch);

#define printf printk
#define sleep wait_ms
int tolower(int ch);
int isspace (int c);

void MemoryManagementInitialization(void * pvStartAddress, DWORD dwTotalMemoryAllocLength);
void * malloc(size_t size);
void free(void *);

extern volatile int nCountI2cinterrupts, nCountUnusedInterrupts, nCountUnusedInterruptsPic2, nCountInterruptsSmc, nCountInterruptsIde;
extern volatile bool fSeenPowerdown;
typedef enum {
	ETS_OPEN_OR_OPENING=0,
	ETS_CLOSING,
	ETS_CLOSED
} TRAY_STATE;
extern volatile TRAY_STATE traystate;


extern void BootInterruptsWriteIdt(void);

int copy_swap_trim(unsigned char *dst, unsigned char *src, int len);
void HMAC_SHA1( unsigned char *result,
                unsigned char *key, int key_length,
                unsigned char *text1, int text1_length,
                unsigned char *text2, int text2_length );

char *HelpGetLine(char *ptr);
void HelpGetParm(char *szBuffer, char *szOrig);
char *strrchr0(char *string, char ch);

#endif // _Boot_H_
