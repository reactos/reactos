/* ------------------- huffc.c -------------------- */

#include "dflat.h"
#include "htree.h"

extern struct htree *ht;
extern int root;
extern int treect;
static int lastchar = '\n';

static void compress(FILE *, int, int);
static void outbit(FILE *fo, int bit);

static int fgetcx(FILE *fi)
{
    int c;

    /* ------- bypass comments ------- */
    if ((c = fgetc(fi)) == ';' && lastchar == '\n')
        do    {
            while (c != '\n' && c != EOF)
                c = fgetc(fi);
        } while (c == ';');
    lastchar = c;
    return c;
}

void main(int argc, char *argv[])
{
    FILE *fi, *fo;
    int c;
    BYTECOUNTER bytectr = 0;

    if (argc < 3)   {
        printf("\nusage: huffc infile outfile");
        exit(1);
    }

    if ((fi = fopen(argv[1], "rb")) == NULL)    {
        printf("\nCannot open %s", argv[1]);
        exit(1);
    }
    if ((fo = fopen(argv[2], "wb")) == NULL)    {
        printf("\nCannot open %s", argv[2]);
        fclose(fi);
        exit(1);
    }

    ht = calloc(256, sizeof(struct htree));

    /* - read the input file and count character frequency - */
    while ((c = fgetcx(fi)) != EOF)   {
        c &= 255;
        ht[c].cnt++;
        bytectr++;
    }

    /* ---- build the huffman tree ---- */
    buildtree();

    /* --- write the byte count to the output file --- */
    fwrite(&bytectr, sizeof bytectr, 1, fo);

    /* --- write the tree count to the output file --- */
    fwrite(&treect, sizeof treect, 1, fo);

    /* --- write the root offset to the output file --- */
    fwrite(&root, sizeof root, 1, fo);

    /* -- write the tree to the output file -- */
    for (c = 256; c < treect; c++)   {
        int lf = ht[c].left;
        int rt = ht[c].right;
        fwrite(&lf, sizeof lf, 1, fo);
        fwrite(&rt, sizeof rt, 1, fo);
    }

    /* ------ compress the file ------ */
    fseek(fi, 0L, 0);
    while ((c = fgetcx(fi)) != EOF)
        compress(fo, (c & 255), 0);
    outbit(fo, -1);
    fclose(fi);
    fclose(fo);
    free(ht);
    exit(0);
}

/* ---- compress a character value into a bit stream ---- */
static void compress(FILE *fo, int h, int child)
{
    if (ht[h].parent != -1)
        compress(fo, ht[h].parent, h);
    if (child)  {
        if (child == ht[h].right)
            outbit(fo, 0);
        else if (child == ht[h].left)
            outbit(fo, 1);
    }
}

static char out8;
static int ct8;

/* -- collect and write bits to the compressed output file -- */
static void outbit(FILE *fo, int bit)
{
    if (ct8 == 8 || bit == -1)  {
        while (ct8 < 8)    {
            out8 <<= 1;
            ct8++;
        }
        fputc(out8, fo);
        ct8 = 0;
    }
    out8 = (out8 << 1) | bit;
    ct8++;
}
