

#include <stdio.h>
#include "m68k/m68k.h"
#include "misc.h"

int main(int argc, char * argv[])
{
    printf("Usage :\n");
    printf("       -cpu m68000      : convert motorala 68000/68008 to intel asm \n");
    printf("       -cpu m68010      : convert motorala 68010 to intel asm \n");
    printf("       -cpu m68020      : convert motorala 68020 to intel asm \n");
    printf("       -cpu m68030      : convert motorala 68030 to intel asm \n");
    printf("       -cpu m68040      : convert motorala 68040 to intel asm \n");
    printf("--------------------------------------------------------------\n");
    printf("       -inBin filename  : the bin file you whant convert\n");
    printf("       -OutAsm filename  : the Asm file you whant create\n");
    printf("--------------------------------------------------------------\n");
    printf("More cpu will be added with the time or options, this is      \n");
    printf("version 0.0.1 of the cpu to intel converter writen by         \n");
    printf("Magnus Olsen (magnus@greatlord.com), it does not do anything  \n");
    printf("yet, more that basic desgin how it should be writen.          \n");
    printf("Copyright 2006 by Magnus Olsen, licen under GPL 2.0 for now.  \n");

    return 0;
}

