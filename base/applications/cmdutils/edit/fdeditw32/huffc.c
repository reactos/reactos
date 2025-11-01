/*  FreeDOS Edit Help Compiler

    Part of the FreeDOS Project
    Originally part of D-Flat, by Dr. Dobbs Journal
    Modified by Joe Cosentino 2002.

*/

/* I N C L U D E S /////////////////////////////////////////////////////// */

#include "dflat.h"
#include "htree.h"

/* G L O B A L S ///////////////////////////////////////////////////////// */

extern struct htree *ht;
extern int root;
extern int treect;
static int lastchar='\n';
static int ct8;
static char out8;

/* P R O T O T Y P E S /////////////////////////////////////////////////// */

static void compress(FILE *, int, int);
static void outbit(FILE *, int);

/* F U N C T I O N S ///////////////////////////////////////////////////// */

static int fgetcx(FILE *fi)
{
    int c;

    /* Bypass comments */
    if ((c=fgetc(fi))==';' && lastchar=='\n')
        do
            {
            while (c != '\n' && c != EOF)
                c=fgetc(fi);

            }
        while (c==';');

    lastchar=c;
    return c;

}

/* Compress a character value into a bit stream */
static void compress(FILE *fo, int h, int child)
{
    if (ht[h].parent != -1)
        compress(fo, ht[h].parent, h);

    if (child)
        {
        if (child == ht[h].right)
            outbit(fo, 0);
        else if (child == ht[h].left)
            outbit(fo, 1);

        }

}

/* Collect and write bits to the compressed output file */
static void outbit(FILE *fo, int bit)
{
    if (ct8==8 || bit==-1)
        {
        while (ct8<8)
            {
            out8 <<= 1;
            ct8++;
            }

        fputc(out8, fo);
        ct8=0;
        }

    out8=(out8 << 1) | bit;
    ct8++;

}

int main(int argc, char *argv[])
{
    FILE *fi, *fo;
    int c;
    BYTECOUNTER bytectr = 0;

    if (argc < 3)
        {
        printf("Syntax: HUFFC infile outfile\n");
        return 1;
        }

    if ((fi = fopen(argv[1], "rb")) == NULL)
        {
        printf("Cannot open %s\n", argv[1]);
        return 1;
        }

    if ((fo = fopen(argv[2], "wb")) == NULL)
        {
        printf("Cannot open %s\n", argv[2]);
        fclose(fi);
        return 1;
        }

    ht = calloc(256, sizeof(struct htree));

    /* Read the input file and count character frequency */
    while ((c = fgetcx(fi)) != EOF)
        {
        c &= 255;
        ht[c].cnt++;
        bytectr++;
        }

    /* Build the huffman tree */
    buildtree();

    /* Write the byte count to the output file */
    fwrite(&bytectr, sizeof bytectr, 1, fo);

    /* Write the tree count to the output file */
    fwrite(&treect, sizeof treect, 1, fo);

    /* Write the root offset to the output file */
    fwrite(&root, sizeof root, 1, fo);

    /* Write the tree to the output file */
    for (c=256;c<treect;c++)
        {
        fwrite(&ht[c].left, sizeof(ht[c].left), 1, fo);
        fwrite(&ht[c].right, sizeof(ht[c].right), 1, fo);
        }

    /* Compress the file */
    fseek(fi, 0L, 0);
    while ((c = fgetcx(fi)) != EOF)
        compress(fo, (c & 255), 0);

    outbit(fo, -1);
    fclose(fi);
    fclose(fo);
    free(ht);

    return 0;
}

