/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/printk.c
 * PURPOSE:         Writing to the console 
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *             ??/??/??: Created
 *             05/06/98: Implemented BSOD
 */

/* INCLUDES *****************************************************************/

#include <internal/ntoskrnl.h>
#include <internal/string.h>
#include <internal/hal/page.h>
#include <internal/hal/io.h>

#include <internal/debug.h>

/* GLOBALS ******************************************************************/

/*
 * PURPOSE: Current cursor position
 */
static unsigned int cursorx=0, cursory=0;

static unsigned int lines_seen = 0;

#define NR_ROWS     25
#define NR_COLUMNS  80
#define VIDMEM_BASE 0xb8000

/*
 * PURPOSE: Points to the base of text mode video memory
 */
static char* vidmem = (char *)(VIDMEM_BASE + IDMAP_BASE);

#define CRTC_COMMAND 0x3d4
#define CRTC_DATA 0x3d5
#define CRTC_CURLO 0x0f
#define CRTC_CURHI 0x0e

/*
 * PURPOSE: This flag is set to true if the system is in HAL managed 
 * console mode. This is initially true then once the graphics drivers
 * initialize, it is turned off, HAL console mode is reentered if a fatal
 * error occurs or on system shutdown
 */
static unsigned int in_hal_console = 1;

/*
 * PURPOSE: Defines the hal console video mode
 */
static unsigned char mode03[] = {0x67,0x00,0x03,0x00,0x03,0x00,0x02,
                                 0x5f,0x4f,0x50,0x82,0x55,0x81,0xbf,
                                 0x1f,0x00,0x4f,0x0e,0x0f,0x00,0x00,
                                 0x00,0x00,0x9c,0x0e,0x8f,0x28,0x01,
                                 0x96,0xb9,0xa3,0xff,0x00,0x00,0x00,
                                 0x00,0x00,0x10,0x0e,0x00,0xff,0x00,
                                 0x00,0x01,0x02,0x03,0x04,0x05,0x06,
                                 0x07,0x10,0x11,0x12,0x13,0x14,0x15,
                                 0x16,0x17,0x0c,0x00,0x0f,0x08,0x00};

/* FUNCTIONS ***************************************************************/

void HalSwitchToBlueScreen(void)
/*
 * FUNCTION: Switches the monitor to text mode and writes a blue background
 * NOTE: This function is entirely self contained and can be used from any
 * graphics mode. 
 */
{
   /*
    * Sanity check
    */
   if (in_hal_console)
     {
	return;
     }

   /*
    * Reset the cursor position
    */
   cursorx=0;
   cursory=0;

   /*
    * This code section is taken from the sample routines by
    * Jeff Morgan (kinfira@hotmail.com)
    */
   
}

void HalDisplayString(char* string)
/*
 * FUNCTION: Switches the screen to HAL console mode (BSOD) if not there
 * already and displays a string
 * ARGUMENT:
 *        string = ASCII string to display
 * NOTE: Use with care because there is no support for returning from BSOD
 * mode
 */
{
   if (!in_hal_console)
     {
	HalSwitchToBlueScreen();
     }
   printk("%s",string);
}

static void putchar(char c)
/*
 * FUNCTION: Writes a character to the console and updates the cursor position
 * ARGUMENTS: 
 *         c = the character to write
 * NOTE: This function handles newlines as well
 */
{
   char* address;
   int offset;
   int i;

   switch(c)
     {
      case '\n':
	cursory++;
	cursorx=0;
        lines_seen++;
	break;
	
      default:
	vidmem[(cursorx*2) + (cursory*80*2)]=c;
	vidmem[(cursorx*2) + (cursory*80*2)+1]=0x7;
	cursorx++;
	if (cursorx>=NR_COLUMNS)
	  {
	     cursory++;
             lines_seen++;
	     cursorx=0;
	  }
     }
   
   #if 0
   if (lines_seen == 24)
   {
        char str[] = "--- press escape to continue";

        lines_seen = 0;
        for (i=0;str[i]!=0;i++)
        {
                vidmem[NR_COLUMNS*(NR_ROWS-1)*2+i*2]=str[i];
                vidmem[NR_COLUMNS*(NR_ROWS-1)*2+i*2+1]=0x7;
        }

        while (inb_p(0x60)!=0x81);
        memset(&vidmem[NR_COLUMNS*(NR_ROWS-1)*2],0,NR_COLUMNS*2);
   }
   #endif
   
   if (cursory>=NR_ROWS)
     {
	memcpy(vidmem,&vidmem[NR_COLUMNS*2],
	       NR_COLUMNS*(NR_ROWS-1)*2);
	memset(&vidmem[NR_COLUMNS*(NR_ROWS-1)*2],0,NR_COLUMNS*2);
	cursory=24;
     }
   
   /*
    * Set the cursor position
    */
   
   offset=cursory*NR_COLUMNS;
   offset=offset+cursorx;
   
   outb_p(CRTC_COMMAND,CRTC_CURLO);
   outb_p(CRTC_DATA,offset);
   outb_p(CRTC_COMMAND,CRTC_CURHI);
   offset>>=8;
   outb_p(CRTC_DATA,offset);

}

asmlinkage void printk(const char* fmt, ...)
/*
 * FUNCTION: Print a formatted string to the hal console
 * ARGUMENTS: As for printf
 * NOTE: So it can used from irq handlers this function disables interrupts
 * during its execution, they are restored to the previous state on return
 */
{
   char buffer[256];
   char* str=buffer;
   va_list ap;
   unsigned int eflags;

   /*
    * Because this is used by alomost every subsystem including irqs it
    * must be atomic. The following code sequence disables interrupts after
    * saving the previous state of the interrupt flag
    */
   __asm__("pushf\n\tpop %0\n\tcli\n\t"
	   : "=m" (eflags)
	   : /* */);

   /*
    * Process the format string into a fixed length buffer using the
    * standard C RTL function
    */
   va_start(ap,fmt);
   vsprintf(buffer,fmt,ap);
   va_end(ap);
   
   while ((*str)!=0)
     {
	putchar(*str);
	str++;
     }
   
   /*
    * Restore the interrupt flag
    */
   __asm__("push %0\n\tpopf\n\t"
	   :
	   : "m" (eflags));
}

int bad_user_access_length(void)
{
        printk("Bad user access length\n");
}

ULONG DbgPrint(PCH Format, ...)
{
   char buffer[256];
   char* str=buffer;
   va_list ap;
   unsigned int eflags;

   /*
    * Because this is used by alomost every subsystem including irqs it
    * must be atomic. The following code sequence disables interrupts after
    * saving the previous state of the interrupt flag
    */
   __asm__("pushf\n\tpop %0\n\tcli\n\t"
	   : "=m" (eflags)
	   : /* */);

   /*
    * Process the format string into a fixed length buffer using the
    * standard C RTL function
    */
   va_start(ap,Format);
   vsprintf(buffer,Format,ap);
   va_end(ap);
   
   while ((*str)!=0)
     {
	putchar(*str);
	str++;
     }
   
   /*
    * Restore the interrupt flag
    */
   __asm__("push %0\n\tpopf\n\t"
	   :
	   : "m" (eflags));
}

void InitConsole(boot_param* bp)
/*
 * FUNCTION: Initalize the console
 * ARGUMENTS:
 *         bp = Parameters setup by the boot loader
 */
{
        cursorx=bp->cursorx;
        cursory=bp->cursory;
}
