#include <stdio.h>
#include "htuu.h"

int main (int argc, char **argv)
{
    char outbuf[500];
    char *pOut = outbuf;
    int cbOut = (strlen(argv[1]) * 3) / 4;

    if (argc != 2)
    {
        fprintf (stderr, "usage: uudec <base64-string>\n");
        exit (1);
    }
    
    HTUU_decode (argv[1], pOut, sizeof(outbuf));

    while (cbOut--)
    {
        printf ("%02x %c\n", (unsigned char) *pOut, *pOut);
        pOut++;
    }
        
}
