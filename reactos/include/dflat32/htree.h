/* ------------------- htree.h -------------------- */

#ifndef HTREE_H
#define HTREE_H

typedef unsigned int BYTECOUNTER;

/* ---- Huffman tree structure for building ---- */
struct htree    {
    BYTECOUNTER cnt;        /* character frequency         */
    int parent;             /* offset to parent node       */
    int right;              /* offset to right child node  */
    int left;               /* offset to left child node   */
};

/* ---- Huffman tree structure in compressed file ---- */
struct htr    {
    int right;              /* offset to right child node  */
    int left;               /* offset to left child node   */
};

extern struct htr *HelpTree;

void buildtree(void);
FILE *OpenHelpFile(void);
void HelpFilePosition(long *, int *);
void *GetHelpLine(char *);
void SeekHelpLine(long, int);

#endif

