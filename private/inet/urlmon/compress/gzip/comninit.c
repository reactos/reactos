//
// comninit.c
//
// Initialisation code common to inflate and deflate
//
#include <stdio.h>
#include <crtdbg.h>
#include "inflate.h"
#include "deflate.h"


// Called by InitCompression() and InitDecompression() (functions to init global DLL data)
//
// Initialises the tree lengths of static type blocks
//
void InitStaticBlock(void)
{
    int i;

    // No real thread synchronisation problems with doing this
    if (g_InitialisedStaticBlock == FALSE)
    {
        g_InitialisedStaticBlock = TRUE;

        for (i = 0; i <= 143; i++)
        	g_StaticLiteralTreeLength[i] = 8;

        for (i = 144; i <= 255; i++)
	        g_StaticLiteralTreeLength[i] = 9;

        for (i = 256; i <= 279; i++)
	        g_StaticLiteralTreeLength[i] = 7;

        for (i = 280; i <= 287; i++)
	        g_StaticLiteralTreeLength[i] = 8;
    }
}
