/* $Id: serial.c,v 1.9 2002/08/25 06:59:34 robd Exp $
 *
 * Serial driver
 * Written by Jason Filby (jasonfilby@yahoo.com)
 * For ReactOS (www.reactos.com)
 *
 */

#include <ddk/ntddk.h>
//#include <internal/mmhal.h>
//#include "../../../ntoskrnl/include/internal/i386/io.h"
//#include "../../../ntoskrnl/include/internal/io.h"

#define outb_p(a,p) WRITE_PORT_UCHAR((PUCHAR)a,p)
#define outw_p(a,p) WRITE_PORT_USHORT((PUSHORT)a,p)
#define inb_p(p)    READ_PORT_UCHAR((PUCHAR)p)

#define NDEBUG
#include <debug.h>


#define COM1    0x3F8
#define COM2    0x2F8
#define COM3    0x3E8
#define COM4    0x2E8

#define UART_BAUDRATE   96      // 1200 BPS
#define UART_LCRVAL     0x1b    // 0x1b for 8e1
#define UARY_FCRVAL     0x7

int uart_detect(unsigned base)
{
        // Returns 0 if no UART detected

        int olddata=inb_p(base+4);
        outb_p(base+4, 0x10);
        if ((inb_p(base+6) & 0xf0)) return 0;
        return 1;
};

int irq_setup(unsigned base)
{
        // Returns -1 if not found -- otherwise returns interrupt level

        char ier, mcr, imrm, imrs, maskm, masks, irqm, irqs;

        __asm("cli");          // disable all CPU interrupts
        ier = inb_p(base+1);   // read IER
        outb_p(base+1,0);      // disable all UART ints
        while (!(inb_p(base+5)&0x20));  // wait for the THR to be empty
        mcr = inb_p(base+4);   // read MCR
        outb_p(base+4,0x0F);   // connect UART to irq line
        imrm = inb_p(0x21);    // read contents of master ICU mask register
        imrs = inb_p(0xA1);    // read contents of slave ICU mask register
        outb_p(0xA0,0x0A);     // next read access to 0xA0 reads out IRR
        outb_p(0x20,0x0A);     // next read access to 0x20 reads out IRR
        outb_p(base+1,2);      // let's generate interrupts...
        maskm = inb_p(0x20);   // this clears all bits except for the one
        masks = inb_p(0xA0);   // that corresponds to the int
        outb_p(base+1,0);      // drop the int line
        maskm &= ~inb_p(0x20); // this clears all bits except for the one
        masks &= ~inb_p(0xA0); // that corresponds to the int
        outb_p(base+1,2);      // and raise it again just to be sure...
        maskm &= inb_p(0x20);  // this clears all bits except for the one
        masks &= inb_p(0xA0);  // that corresponds to the int
        outb_p(0xA1,~masks);   // now let us unmask this interrupt only
        outb_p(0x21,~maskm);
        outb_p(0xA0,0x0C);     // enter polled mode
        outb_p(0x20,0x0C);     // that order is important with Pentium/PCI systems
        irqs = inb_p(0xA0);    // and accept the interrupt
        irqm = inb_p(0x20);
        inb_p(base+2);         // reset transmitter interrupt in UART
        outb_p(base+4,mcr);    // restore old value of MCR
        outb_p(base+1,ier);    // restore old value of IER
        if (masks) outb_p(0xA0,0x20);  // send an EOI to slave
        if (maskm) outb_p(0x20,0x20);  // send an EOI to master
        outb_p(0x21,imrm);     // restore old mask register contents
        outb_p(0xA1,imrs);
        __asm("sti");
        if (irqs&0x80)       // slave interrupt occured
          return (irqs&0x07)+8;
        if (irqm&0x80)       // master interrupt occured
          return irqm&0x07;
        return -1;
};

void uart_init(unsigned uart_base)
{
        // Initialize the UART
        outb_p(uart_base+3, 0x80);
        outw_p(uart_base, UART_BAUDRATE);
        outb_p(uart_base+3, UART_LCRVAL);
        outb_p(uart_base+4, 0);
};

unsigned uart_getchar(unsigned uart_base)
{
        unsigned x;

        x=(inb_p(uart_base+5) & 0x9f) << 8;
        if(x & 0x100) x|=((unsigned)inb_p(uart_base)) & 0xff;
        return x;
};

void InitializeSerial(void)
{
        unsigned comports[4] = { COM1, COM2, COM3, COM4 };
        char *comname[4] = { "COM1", "COM2", "COM3", "COM4" };
        int i, irq_level;

        for (i=0; i<4; i++)
        {
                if(uart_detect(comports[i])==0)
                {
                          DbgPrint("%s not detected\n", comname[i]);
                } else {
                          uart_init(comports[i]);
                          irq_level=irq_setup(comports[i]);
                          if(irq_level==-1)
                          {
                                    DbgPrint("Warning: IRQ not detected!\n");
                          } else {
                                    DbgPrint("%s hooked to interrupt level %d\n", comname[i], irq_level);
                          };
                };
        };
};

// For testing purposes
void testserial(void)
{
        int i=0;
        char testc;

        union {
          unsigned val;
          char character;
        } x;

        DbgPrint("Testing serial input...\n");

        while(i==0) {
          x.val=uart_getchar(COM1);
//    if(!x.val) continue;
//    if(x.val & 0x100)

                testc=inb_p(COM1);

//    DbgPrint("(%x-%c)  %c\n", x.val, x.character, testc);
        };
};

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
        DbgPrint("Serial Driver 0.0.2\n");
        InitializeSerial();
//        testserial();
        return(STATUS_SUCCESS);
};

