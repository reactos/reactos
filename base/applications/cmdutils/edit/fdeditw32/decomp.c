/*
 * Decompress the application.HLP file
 * or load the application.TXT file if the .HLP file
 * does not exist
 */

#include <assert.h>
#include "dflat.h"
#include "htree.h"

static int in8;
static int ct8 = 8;
static FILE *fi;
static BYTECOUNTER bytectr;
struct htr *HelpTree;
static int root;

/* ------- open the help database file -------- */
FILE *OpenHelpFile(const char *fn, const char *md)
{
    int treect, i;
#if defined(_WIN32) && defined(__REACTOS__)
    char helpname[MAX_PATH];
#else
    char helpname[65];
#endif

    /* Get the name of the help file */
    BuildFileName(helpname, fn, ".hlp");
    if ((fi = fopen(helpname, md)) == NULL)
        return NULL;

    if (HelpTree == NULL)
        {
        fread(&bytectr, sizeof(bytectr), 1, fi);  /* Read the byte count */
        fread(&treect, sizeof(treect), 1, fi);    /* Read the frequency count */
        fread(&root, sizeof(root), 1, fi);        /* Read the root offset */
        HelpTree=calloc(treect-256, sizeof(struct htr));
        if (HelpTree != NULL)
            {
            for (i=0;i<treect-256;i++)  /* Read in the tree */
                {
                fread(&HelpTree[i].left,  sizeof(HelpTree[i].left), 1, fi);
                fread(&HelpTree[i].right, sizeof(HelpTree[i].right), 1, fi);
                }

            }

	}

    return fi;

}

/* ----- read a line of text from the help database ----- */
void *GetHelpLine(char *line)
{
    int h;

    *line = '\0';
    while (TRUE)
        {
        /* Decompress a line from the file */
        h=root;

        /* Walk the Huffman tree */
        while (h > 255)
            {
            /* h is a node pointer */
            if (ct8 == 8)
                {
                /* Read 8 bits of compressed data */
                if ((in8 = fgetc(fi)) == EOF)
                    {
                    *line = '\0';
                    return NULL;
                    }

                ct8 = 0;
                }

            /* Point to left or right node based on msb */
            if (in8 & 0x80)
                h = HelpTree[h-256].left;
            else
                h = HelpTree[h-256].right;

            /* Shift the next bit in */
            in8 <<= 1;
            ct8++;
            }

        /* h < 255 = decompressed character */
        if (h == '\r')                  /* Skip the '\r' character */
            continue;

        *line++ = h;                    /* Put the character in the buffer */
        if (h == '\n')                  /* End of line */
            break;

        }

    *line = '\0';                       /* Null-terminate the line */
    return line;

}

/* --- compute the database file byte and bit position --- */
void HelpFilePosition(long *offset, int *bit)
{
    *offset = ftell(fi);
    if (ct8 < 8)
        --*offset;

    *bit = ct8;

}

/* -- position the database to the specified byte and bit -- */
void SeekHelpLine(long offset, int bit)
{
    int fs = fseek(fi, offset, 0);

    assert(fs == 0);
    ct8 = bit;
    if (ct8 < 8)
        {
        in8 = fgetc(fi);
        in8 <<= bit;
        }

}
