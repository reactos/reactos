/* encrypt.c
 *
 * A dumb encryption/decryption algorithm
 *
 * input file
 *
 *   header1
 *      name1
 *      name2
 *
 *   header2
 *      nameA
 *      nameB
 *      nameC
 *
 * output (xor'ed with XOR_MASK)
 *
 *    header1\0\0name1\0name2\0\0header2\0\0nameA\0nameB\0nameC\0\0\0
 *
 */

#include <stdio.h>

#define XOR_MASK    (0x95)
#define XOR_DWORD   (0x95959595)

char ach[512];
unsigned long pad = XOR_DWORD;

int main(int argc, char *argv[])
{
        FILE *fpIn;
        FILE *fpOut;
        int wasname = 0;

        if ( argc != 3 ) {
                fprintf(stderr, "usage: encrypt infile outfile\n");
                return(1);
        }

        if ( (fpIn=fopen(argv[1], "r") ) == NULL) {
                fprintf(stderr, "cant open %s\n", argv[1]);
                return(1);
        }

        if ( (fpOut=fopen(argv[2], "wb") ) == NULL) {
                fprintf(stderr, "cant open %s\n", argv[2]);
                return(1);
        }

        while (fgets(ach, sizeof(ach), fpIn))
        {
            char *pch = ach;
            int len = strlen(pch);
            int isname, i;

            //
            // skip empty lines
            //
            if (len <= 1)
                continue;

            //
            //  convert \n to a delimiter
            //
            pch[len-1] = 0;     // important: 'len' now includes this delimiter

            //
            // indented lines are "names"
            // others are "titles"
            //
            isname = ((*pch == ' ') || (*pch == '\t'));
            if (isname)
            {
                //
                // trim off the indent
                //
                while ((*pch == ' ') || (*pch == '\t'))
                {
                    pch++;
                    len--;
                }

                //
                // totally empty line is really nothing at all
                //
                if (!*pch)
                    continue;
            }

            //
            // comment lines begin with semicolons
            //
            if (*pch == ';')
                continue;

            //
            // if we got here we have real text to write out
            // insert an extra (group) delimiter if required
            //
            if (isname != wasname)
                fwrite(&pad, sizeof(char), 1, fpOut);

            //
            // (barely) shroud the data and write it out
            //
            for (i = 0; i < len; i++)
                pch[i] ^= XOR_MASK;
            fwrite(pch, sizeof(char), len, fpOut);

            //
            // remember what this last one was for the next iteration
            //
            wasname = isname;
        }

        //
        // mark the end of the data stream
        //
        fwrite(&pad, sizeof(char), 2, fpOut);
        fclose(fpOut);

        // bye
        fclose(fpIn);
        return(0);
}
