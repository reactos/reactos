/* ------------------- decomp.c -------------------- */

/*
 * Decompress the application.HLP file
 * or load the application.TXT file if the .HLP file
 * does not exist
 */

#include "dflat.h"
#include "htree.h"

static int in8;
static int ct8 = 8;
static FILE *fi;
static DF_BYTECOUNTER bytectr;
static int LoadingASCII;
struct DfHTr *DfHelpTree;
static int root;

/* ------- open the help database file -------- */
FILE *DfOpenHelpFile(void)
{
    char *cp;
    int treect, i;
    char helpname[65];

    /* -------- get the name of the help file ---------- */
    DfBuildFileName(helpname, ".hlp");
    LoadingASCII = FALSE;
    if ((fi = fopen(helpname, "rb")) == NULL)    {
        /* ---- no .hlp file, look for .txt file ---- */
        if ((cp = strrchr(helpname, '.')) != NULL)    {
            strcpy(cp, ".TXT");
            fi = fopen(helpname, "rt");
        }
        if (fi == NULL)
            return NULL;
        LoadingASCII = TRUE;
    }

    if (!LoadingASCII && DfHelpTree == NULL)    {
           /* ----- read the byte count ------ */
           fread(&bytectr, sizeof bytectr, 1, fi);
           /* ----- read the frequency count ------ */
           fread(&treect, sizeof treect, 1, fi);
           /* ----- read the root offset ------ */
           fread(&root, sizeof root, 1, fi);
        DfHelpTree = DfCalloc(treect-256, sizeof(struct DfHTr));
        /* ---- read in the tree --- */
        for (i = 0; i < treect-256; i++)    {
               fread(&DfHelpTree[i].left,  sizeof(int), 1, fi);
            fread(&DfHelpTree[i].right, sizeof(int), 1, fi);
        }
    }
    return fi;
}

/* ----- read a line of text from the help database ----- */
void *DfGetHelpLine(char *line)
{
    int h;
    if (LoadingASCII)	{
		void *hp;
		do
			hp = fgets(line, 160, fi);
		while (*line == ';');
		return hp;
	}
    *line = '\0';
    while (TRUE)    {
        /* ----- decompress a line from the file ------ */
        h = root;
        /* ----- walk the Huffman tree ----- */
        while (h > 255)    {
            /* --- h is a node pointer --- */
            if (ct8 == 8)   {
                /* --- read 8 bits of compressed data --- */
                if ((in8 = fgetc(fi)) == EOF)    {
                    *line = '\0';
                    return NULL;
                }
                ct8 = 0;
            }
            /* -- point to left or right node based on msb -- */
            if (in8 & 0x80)
                h = DfHelpTree[h-256].left;
            else
                h = DfHelpTree[h-256].right;
            /* --- shift the next bit in --- */
            in8 <<= 1;
            ct8++;
        }
        /* --- h < 255 = decompressed character --- */
        if (h == '\r')
            continue;    /* skip the '\r' character */
        /* --- put the character in the buffer --- */
        *line++ = h;
        /* --- if '\n', end of line --- */
        if (h == '\n')
            break;
    }
    *line = '\0';    /* null-terminate the line */
    return line;
}

/* --- compute the database file byte and bit position --- */
void DfHelpFilePosition(long *offset, int *bit)
{
    *offset = ftell(fi);
    if (LoadingASCII)
        *bit = 0;
    else    {
        if (ct8 < 8)
            --*offset;
        *bit = ct8;
    }
}

/* -- position the database to the specified byte and bit -- */
void DfSeekHelpLine(long offset, int bit)
{
    int i;
    fseek(fi, offset, 0);
    if (!LoadingASCII)    {
        ct8 = bit;
        if (ct8 < 8)    {
            in8 = fgetc(fi);
            for (i = 0; i < bit; i++)
                in8 <<= 1;
        }
    }
}


