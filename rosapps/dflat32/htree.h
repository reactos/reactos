/* ------------------- htree.h -------------------- */

#ifndef HTREE_H
#define HTREE_H

typedef unsigned int DF_BYTECOUNTER;

/* ---- Huffman tree structure for building ---- */
struct DfHTree    {
    DF_BYTECOUNTER cnt;        /* character frequency         */
    int parent;             /* offset to parent node       */
    int right;              /* offset to right child node  */
    int left;               /* offset to left child node   */
};

/* ---- Huffman tree structure in compressed file ---- */
struct DfHTr    {
    int right;              /* offset to right child node  */
    int left;               /* offset to left child node   */
};

extern struct DfHTr *DfHelpTree;

void DfBuildTree(void);
FILE *DfOpenHelpFile(void);
void DfHelpFilePosition(long *, int *);
void *DfGetHelpLine(char *);
void DfSeekHelpLine(long, int);

#endif

