/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    halvprnt.c

Author:

    John Vert (jvert) 13-Aug-1991
        based on TomP's video.c

Abstract:

    Video support routines.

    The vprintf function here outputs to the console via InbvDisplayString.
    All the global variables have been made local, so multiple processors
    can execute it simultaneously.  If this is the case, it relies on
    InbvDisplayString to sort the output to avoid interleaving the text from
    the processors.

History:

--*/

#include <ntos.h>
#include <inbv.h>

typedef unsigned char BYTE, *PBYTE;


//
// Internal routines
//

static
int
xatoi(char c);

static
int
fields(
    char *cp,
    int *zerofill,
    int *fieldwidth
    );

static
VOID
putx(
    ULONG x,
    int digits,
    int zerofill,
    int *fieldwidth
    );

static
VOID
puti(
    LONG i,
    int digits,
    int zerofill,
    int *fieldwidth
    );

static
VOID
putu(
    ULONG u,
    int digits,
    int zerofill,
    int *fieldwidth
    );

static
VOID
putc(
    CHAR c
    );


/*++

Name

    vprintf - DbgPrint function on standard video

    Currently handles


     %i, %li - signed short, signed long  (same as d)
     %d, %ld - signed short, signed long
     %u, %lu - unsigned short, unsigned long
     %c, %s, %.*s - character, string
     %Z - PSTRING data type
     %x, %lx - unsigned print in hex, unsigned long print in hex
     %X, %lX, %X, %X, %X, %X - same as %x and %lx
     field widths
     leading 0 fills

    Does not do yet:

     No floating point.


--*/
void
vprintf(PCHAR cp,USHORT a1)
{
   ULONG cb;
   USHORT b,c;
   PBYTE ap;
   PCHAR s;
   PSTRING str;
   ULONG Flags;
   int zerofill, fieldwidth;

   //
   // Cast a pointer to the first word on the stack
   //

   ap = (PBYTE)&a1;


    //  Save flags in automatic variable on stack, turn off ints.

   _asm {
        pushfd
        pop     Flags
        cli
    }

   //
   // Process the argements using the descriptor string
   //

    while (b = *cp++) {
        if (b == '%') {
            cp += fields(cp, &zerofill, &fieldwidth);
            c = *cp++;

            switch (c) {
            case '.':
                if (*cp != '*' || cp[1] != 's') {
                    putc((char)b);
                    putc((char)c);
                    break;
                }
                cp += 2;
                cb = *((ULONG *)ap);
                ap += sizeof( ULONG );
                s = *((PCHAR *)ap);
                ap += sizeof( PCHAR );
                if (s == NULL) {
                   s = "(null)";
                   cb = 6;
                }
                if (cb > 0xFFF) {
                   s = "(overflow)";
                   cb = 10;
                }

                while (cb--) {
                    if (*s) {
                        putc(*s++);
                    } else {
                        putc(' ');
                    }
                }
                break;

            case 'i':
            case 'd':
                puti((long)*((int *)ap), 1, zerofill, &fieldwidth);
                ap += sizeof(int);
                break;

            case 'S':
                str = *((PSTRING *)ap);
                ap += sizeof (STRING *);
                b = str->Length;
                s = str->Buffer;
                if (s == NULL)
                    s = "(null)";
                while (b--)
                    putc(*s++);
                break;

            case 's':
                s = *((PCHAR *)ap);
                ap += sizeof( PCHAR );
                if (s == NULL)
                   s = "(null)";
                while (*s)
                   putc(*s++);
                break;

            case 'c':
                putc(*((char *)ap));
                ap += sizeof(int);
                break;


            //
            //  If we cannot find the status value in the table, print it in
            //  hex.
            //
            case 'C':
            case 'B':
                //
                // Should search bugcodes.h to display bug code
                // symbolically.  For now just show as hex
                //

            case 'X':
            case 'x':
                putx((ULONG)*((USHORT *)ap), 1, zerofill, &fieldwidth);
                ap += sizeof(int);
                break;

            case 'u':
                putu((ULONG)*((USHORT *)ap), 1, zerofill, &fieldwidth);
                ap += sizeof(int);
                break;

            case 'l':
                c = *cp++;

                switch(c) {
                case 'u':
                    putu(*((ULONG *)ap), 1, zerofill, &fieldwidth);
                    ap += sizeof(long);
                    break;

                case 'C':
                case 'B':
                    //
                    // Should search bugcodes.h to display bug code
                    // symbolically.  For now just show as hex
                    //

                case 'X':
                case 'x':
                    putx(*((ULONG *)ap), 1, zerofill, &fieldwidth);
                    ap += sizeof(long);
                    break;

                case 'i':
                case 'd':
                    puti(*((ULONG *)ap), 1, zerofill, &fieldwidth);
                    ap += sizeof(long);
                    break;
                }                               // inner switch
                break;

            default :
                putc((char)b);
                putc((char)c);
            } // outer switch
        }   // if
        else
            putc((char)b);
    } // while

    // Restore flags from automatic variable on stack

    _asm {
        push    Flags
        popfd
    }
    return;
}


//
// Fields computation
//

static int fields(char *cp, int *zerofill, int *fieldwidth)
{
    int incval = 0;

    *zerofill = 0;
    *fieldwidth = 0;

    if (*cp == '0') {
        *zerofill = 1;
        cp++;
        incval++;
    }

    while ((*cp >= '0') && (*cp <= '9')) {
        *fieldwidth = (*fieldwidth * 10) + xatoi(*cp);
        cp++;
        incval++;
    }
    return incval;
}

//
// Write a hex short to display
//

static void putx(ULONG x, int digits, int zerofill, int *fieldwidth)
{
   ULONG j;

   if (x/16)
      putx(x/16, digits+1, zerofill, fieldwidth);

   if (*fieldwidth > digits) {
        while (*fieldwidth > digits) {
            if (zerofill)
                putc('0');
            else
                putc(' ');
            *fieldwidth--;
        }
    }
    *fieldwidth = 0;


   if((j=x%16) > 9)
      putc((char)(j+'A'- 10));
   else
      putc((char)(j+'0'));

}


//
// Write a short integer to display
//

static void puti(long i, int digits, int zerofill, int *fieldwidth)
{
    if (i<0) {
        i = -i;
        putc((char)'-');
    }

    if (i/10)
        puti(i/10, digits+1, zerofill, fieldwidth);

    if (*fieldwidth > digits) {
        while (*fieldwidth > digits) {
            if (zerofill)
                putc('0');
            else
                putc(' ');
            *fieldwidth--;
        }
    }
    *fieldwidth = 0;

    putc((char)((i%10)+'0'));
}


//
// Write an unsigned short to display
//

static void putu(ULONG u, int digits, int zerofill, int *fieldwidth)
{
   if (u/10)
      putu(u/10, digits+1, zerofill, fieldwidth);

   if (*fieldwidth > digits) {
        while (*fieldwidth > digits) {
            if (zerofill)
                putc('0');
            else
                putc(' ');
            *fieldwidth--;
        }
    }
    *fieldwidth = 0;

    putc((char)((u%10)+'0'));
}

//
// Write a character to display
//

VOID putc(
    CHAR c
    )
{
    static UCHAR OneCharacter[2];

    OneCharacter[1] = '\0';
    OneCharacter[0] = c;
    InbvDisplayString(OneCharacter);
}


//
// Return the integer value of numeral represented by ascii char
//

int xatoi(char c)
{
    return c - '0';
}
