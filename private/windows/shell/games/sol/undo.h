
/* UnDo Record  */
typedef struct _udr
{
	BOOL fAvail;
    BOOL fEndDeck;
	INT sco;
	INT icol1, icol2;
    INT irep;
	COL *rgpcol[2];
} UDR;


BOOL FInitUndo(UDR *pudr);
VOID FreeUndo(UDR *pudr);



#define icrdUndoMax 52
