/*

 ** Mouse driver 0.0.3
 ** Written by Jason Filby (jasonfilby@yahoo.com)
 ** For ReactOS (www.sid-dis.com/reactos)

 ** Note: The serial.o driver must be loaded before loading this driver

 ** Known Limitations:
 ** Only supports mice on COM port 1

 ** Old Driver, We done build it. Just keep for History. (S.E.)

*/

#include <ddk/ntddk.h>
#include <internal/mmhal.h>
#include <internal/halio.h>
/* #include <internal/hal/ddk.h> */
#include <funcs.h>

#define MOUSE_IRQ_COM1  4
#define MOUSE_IRQ_COM2  3

#define COM1_PORT       0x3f8
#define COM2_PORT       0x2f8

#define max_screen_x    79
#define max_screen_y    24

static unsigned int MOUSE_IRQ=MOUSE_IRQ_COM1;
static unsigned int MOUSE_COM=COM1_PORT;

static unsigned int     bytepos=0, coordinate;
static unsigned char    mpacket[3];
static signed int       mouse_x=40, mouse_y=12;
static unsigned char    mouse_button1, mouse_button2;
static signed int       horiz_sensitivity, vert_sensitivity;

BOOLEAN microsoft_mouse_handler(PKINTERRUPT Interrupt, PVOID ServiceContext)
{
        unsigned int mbyte=inb(MOUSE_COM);

        // Synchronize
        if((mbyte&64)==64) { bytepos=0; };

        mpacket[bytepos]=mbyte;
        bytepos++;

        // Process packet
        if(bytepos==3) {
                // Retrieve change in x and y from packet
                int change_x=((mpacket[0] & 3) << 6) + mpacket[1];
                int change_y=((mpacket[0] & 12) << 4) + mpacket[2];

                // Some mice need this
                if(coordinate==1) {
                  change_x-=128;
                  change_y-=128;
                };

                // Change to signed
                if(change_x>=128) { change_x=change_x-256; };
                if(change_y>=128) { change_y=change_y-256; };

                // Adjust mouse position according to sensitivity
                mouse_x+=change_x/horiz_sensitivity;
                mouse_y+=change_y/vert_sensitivity;

                // Check that mouse is still in screen
                if(mouse_x<0) { mouse_x=0; };
                if(mouse_x>max_screen_x) { mouse_x=max_screen_x; };
                if(mouse_y<0) { mouse_y=0; };
                if(mouse_y>max_screen_y) { mouse_y=max_screen_y; };

                // Retrieve mouse button status from packet
                mouse_button1=mpacket[0] & 32;
                mouse_button2=mpacket[0] & 16;

                bytepos=0;
        };

};

void InitializeMouseHardware(unsigned int mtype)
{
        char clear_error_bits;

        outb_p(MOUSE_COM+3, 0x80); // set DLAB on
        outb_p(MOUSE_COM, 0x60); // speed LO byte
        outb_p(MOUSE_COM+1, 0); // speed HI byte
        outb_p(MOUSE_COM+3, mtype); // 2=MS Mouse; 3=Mouse systems mouse
        outb_p(MOUSE_COM+1, 0); // set comm and DLAB to 0
        outb_p(MOUSE_COM+4, 1); // DR int enable
 
        clear_error_bits=inb_p(MOUSE_COM+5); // clear error bits
};

int DetMicrosoft(void)
{
        char tmp, ind;
        int buttons=0, i;

        outb_p(MOUSE_COM+4, 0x0b);
        tmp=inb_p(MOUSE_COM);

        // Check the first for bytes for signs that this is an MS mouse
        for(i=0; i<4; i++) {
                while((inb_p(MOUSE_COM+5) & 1)==0) ;
                ind=inb_p(MOUSE_COM);
                if(ind==0x33) buttons=3;
                if(ind==0x4d) buttons=2;
        };

        return buttons;
};

int CheckMouseType(unsigned int mtype)
{
        unsigned int retval=0;

        InitializeMouseHardware(mtype);
        if(mtype==2) retval=DetMicrosoft();
        if(mtype==3) {
                outb_p(MOUSE_COM+4, 11);
                retval=3;
        };
        outb_p(MOUSE_COM+1, 1);

        return retval;
};

void ClearMouse(void)
{
        // Waits until the mouse calms down but also quits out after a while
        // in case some destructive user wants to keep moving the mouse
        // before we're done

        unsigned int restarts=0, i;
        for (i=0; i<60000; i++)
        {
          unsigned temp=inb(MOUSE_COM);
          if(temp!=0) {
            restarts++;
            if(restarts<300000) {
                    i=0;
            } else
            {
                    i=60000;
            };
          };
        };
};

void InitializeMouse(void)
{
        int mbuttons=0, gotmouse=0;
        ULONG MappedIrq;
        KIRQL Dirql;
        KAFFINITY Affinity;
        PKINTERRUPT IrqObject;

        horiz_sensitivity=2;
        vert_sensitivity=3;

        // Check for Microsoft mouse (2 buttons)
        if(CheckMouseType(2)!=0)
        {
                gotmouse=1;
                DbgPrint("Microsoft Mouse Detected\n");
                ClearMouse();
                coordinate=0;
        };

        // Check for Microsoft Systems mouse (3 buttons)
        if(gotmouse==0) {
          if(CheckMouseType(3)!=0)
          {
                gotmouse=1;
                DbgPrint("Microsoft Mouse Detected\n");
                ClearMouse();
                coordinate=1;
          };
        };

        if(gotmouse==0) {
                DbgPrint("No Mouse Detected!\n");
        } else {
                MappedIrq = HalGetInterruptVector(Internal, 0, 0, MOUSE_IRQ,
                                                  &Dirql, &Affinity);

                IoConnectInterrupt(&IrqObject, microsoft_mouse_handler, NULL,
                                   NULL, MappedIrq, Dirql, Dirql, 0, FALSE,
                                   Affinity, FALSE);
        };
};

// For test purposes only
unsigned char get_text_char(int x, int y)
{
        unsigned char getchar;
        char *vidmem=(char*)physical_to_linear((0xb8000+(y*160)+(x*2)));
        getchar=*vidmem;
        return getchar;
};

// For test purposes only
unsigned char get_text_color(int x, int y)
{
        unsigned char getcolor;
        char *vidmem=(char*)physical_to_linear((0xb8000+(y*160)+(x*2)));
        vidmem++;
        getcolor=*vidmem;
        return getcolor;
};

// For test purposes only
void put_text_char(int x, int y, unsigned char putchar[2])
{
        char *vidmem=(char*)physical_to_linear((0xb8000+(y*160)+(x*2)));
        *vidmem=putchar[0];
        vidmem++;
        *vidmem=putchar[1];
};

// For test purposes only
void test_mouse(void)
{
  static int i=0, forcechange=0;
  static int old_x=40, old_y=12;
  static unsigned char old_cursor[2], new_cursor[2];

  DbgPrint("Testing mouse...");

  old_cursor[0]=' ';
  old_cursor[1]=7;
  new_cursor[0]='Û';
  new_cursor[1]=15;

  old_cursor[0]=get_text_char(mouse_x, mouse_y);
  old_cursor[1]=get_text_color(mouse_x, mouse_y);
  put_text_char(mouse_x, mouse_y, new_cursor);

  while(i!=1)
  {
    if(mouse_button1!=0) { new_cursor[1]=10; mouse_button1=0; forcechange=1; };
    if(mouse_button2!=0) { new_cursor[1]=12; mouse_button2=0; forcechange=1; };

    if((mouse_x!=old_x) || (mouse_y!=old_y) || (forcechange==1)) {
        forcechange=0;

        put_text_char(old_x, old_y, old_cursor);
        old_cursor[0]=get_text_char(mouse_x, mouse_y);
        old_cursor[1]=get_text_color(mouse_x, mouse_y);
        put_text_char(mouse_x, mouse_y, new_cursor);

        old_x=mouse_x;
        old_y=mouse_y;
    };
  };
};

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
        DbgPrint("Mouse Driver 0.0.3\n");
        InitializeMouse();
        test_mouse();

        return(STATUS_SUCCESS);
};

