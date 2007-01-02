

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ARM/ARM.h"
#include "m68k/m68k.h"
#include "PPC/PPC.h"
#include "misc.h"

int main(int argc, char * argv[])
{
    CPU_UNINT BaseAddress=0;
    int t=0;
    char *infile=NULL;
    char *outfile=NULL;
    char *cpuid=NULL;
    CPU_INT type=0;


    printf("Usage :\n");
    printf(" need for -inbin and autodetect  if it does not found a PE header \n");
    printf("       -cpu m68000      : convert motorala 68000/68008 to intel asm \n");
    printf("       -cpu m68010      : convert motorala 68010 to intel asm \n");
    printf("       -cpu m68020      : convert motorala 68020 to intel asm \n");
    printf("       -cpu m68030      : convert motorala 68030 to intel asm \n");
    printf("       -cpu m68040      : convert motorala 68040 to intel asm \n");
    printf("       -cpu ppc         : convert PowerPC to intel asm \n");
    printf("       -cpu ARM4        : convert ARM4 to intel asm \n");
    printf("------------------------------------------------------------------\n");
    printf(" for -inbin and autodetect  if it does not found a PE header or do\n");
    printf(" not set at all, this options are free to use     \n");
    printf(".......-BaseAddress adr : the start base address only accpect \n");
    printf(".......                   dec value");
    printf("------------------------------------------------------------------\n");
    printf("       -in filename     : try autodetect file type for you");
    printf("                          whant convert\n");
    printf("       -inBin filename  : the bin file you whant convert\n");
    printf("       -inExe filename  : the PE file you whant convert\n");
    printf("       -OutAsm filename : the Asm file you whant create\n");
    printf("------------------------------------------------------------------\n");
    printf("More cpu will be added with the time or options, this is      \n");
    printf("version 0.0.1 of the cpu to intel converter writen by         \n");
    printf("Magnus Olsen (magnus@greatlord.com), it does not do anything  \n");
    printf("yet, more that basic desgin how it should be writen.          \n");
    printf("Copyright 2006 by Magnus Olsen, licen under GPL 2.0 for now.  \n");

    if (argc < 7)
        return .110;

    for (t=1; t<7;t+=2)
    {
        if (stricmp(argv[t],"-in"))
        {
            infile = argv[t+1];
            type=0;
        }

        if (stricmp(argv[t],"-inBin"))
        {
            infile = argv[t+1];
            type=1;
        }

        if (stricmp(argv[t],"-inExe"))
        {
            infile = argv[t+1];
            type=1;
        }

        if (stricmp(argv[t],"-OutAsm"))
        {
            outfile = argv[t+1];
        }
        if (stricmp(argv[t],"-BaseAddress"))
        {
            BaseAddress = atol(argv[t+1]);
        }
        if (stricmp(argv[t],"-cpu"))
        {
            cpuid = argv[t+1];
        }

    }

    return LoadPFileImage(infile,outfile,BaseAddress,cpuid,type);
}








