
/* COLumn CLaSs struct */
typedef struct _colcls
{
    INT tcls;                   /* class type  */
    INT (FAR *lpfnColProc)();
    INT ccolDep;                /* # of dependent columns */
    DX  dxUp;                   /* up card offsets */
    DY  dyUp;
    DX  dxDn;                   /* down card offsets */
    DY  dyDn;
    INT dcrdUp;                 /* up # of cards between ofsetting */
    INT dcrdDn;                 /* down # of cards between ofsetting */
} COLCLS;


/* MOVE struct, only set up while dragging */
typedef struct _move
{
    INT     icrdSel;          /* current card sel */
    INT     ccrdSel;          /* # of cards selected */
    DEL     delHit;           /* negative of offset from card where mouse hit */
    BOOL    fHdc;             /* TRUE if hdc's are allocated */
    DY      dyCol;            /* height of col (BUG: can only drag vert cols) */
    HDC     hdcScreen;

    HDC     hdcCol;           /*  the column */
    HBITMAP hbmColOld;        /* original hbm in hdcCol */

    HDC     hdcScreenSave;    /* save buffer for screen */
    HBITMAP hbmScreenSaveOld; /* original hbm in hdcScreenSave */
    
    HDC     hdcT;
    HBITMAP hbmT;
    INT     izip;
} MOVE;




/* COL struct, this is what a column o' cards is */
typedef struct _col
{
    COLCLS *pcolcls;          /* class of this instance */
    INT (FAR *lpfnColProc)(); /* duplicate of fn in colcls struct (for efficiency) */
    RC rc;                    /* bounding rectangle of this col */
    MOVE *pmove;              /* move info, only valid while draggin */
    INT icrdMac;                
    INT icrdMax;
    CRD rgcrd[1];
} COL;

/* SCOre */
typedef INT SCO;

// Constants - earlier they were generated in the col.msg file.

#define icolNil             -1
#define msgcNil             0		  
#define msgcInit            1		  
#define msgcEnd             2		  
#define msgcClearCol        3	   
#define msgcNumCards        4     
#define msgcHit             5		  
#define msgcSel             6		  
#define msgcEndSel          7	  
#define msgcFlip            8		  
#define msgcInvert          9	  
#define msgcMouseUp         10	  
#define msgcDblClk          11	  
#define msgcRemove          12	  
#define msgcInsert          13	  
#define msgcMove            14		  
#define msgcCopy            15		  
#define msgcValidMove       16  
#define msgcValidMovePt     17  
#define msgcRender          18	  
#define msgcPaint           19		  
#define msgcDrawOutline     20  
#define msgcComputeCrdPos   21
#define msgcDragInvert      22  
#define msgcGetPtInCrd      23  
#define msgcValidKbdColSel  24
#define msgcValidKbdCrdSel  25
#define msgcShuffle         26
#define msgcAnimate         27
#define msgcZip             28



#ifdef DEBUG
INT SendColMsg(COL *pcol, INT msgc, WPARAM wp1, LPARAM wp2);
#else
#define SendColMsg(pcol, msgc, wp1, wp2) \
    (*((pcol)->lpfnColProc))((pcol), (msgc), (wp1), (wp2))    
#endif
INT DefColProc(COL *pcol, INT msgc, WPARAM wp1, LPARAM wp2);
VOID OOM( VOID );
VOID DrawOutline( PT *, INT, DX, DY );

/*---------------------------------------------------------------------------
     --------------------> Message explanations <---------------------

    // WARNING: probably totally out-o-date

msgcNil:
    Nil Message, not used 
    wp1:         N/A
    wp2:        N/A
    returns:    TRUE

msgcInit:
    Sent when a column in created.  (currently not used)
    wp1:        N/A
    wp2:       N/A
    returns    TRUE

msgcEnd:
    Sent when a column is destroyed.  Frees pcol
    wp1:        N/A
    wp2:       N/A
    returns     TRUE
    
msgcClearCol:
    Sent to clear a column of it's cards
    wp1:        N/A
    wp2:       N/A
    returns     TRUE

msgcHit:
    Checks if a card is hit by the mouse.  pcol->delHit is set to the
    point where the mouse hit the card relative to the upper right
    corner of the card.  Sets pcol->icrdSel and pcol->ccrdSel
    wp1:        ppt
    wp2:        N/A
    returns: icrd if hit, icrdNil if no hit.  May return icrdEmpty if hit
                an empty column

msgcSel:
    Selects cards in the column for future moves.  Sets pcol->icrdSel and
    pcol->ccrdSel;
    wp1:        icrdFirst, first card to select; icrdEnd selects last card
    wp2:        ccrdSel, # of cards to select; ccrdToEnd selects to end of col
    returns:    TRUE/FALSE

msgcFlip:
    Flips cards to fUp
    wp1:        fUp, TRUE if to flip cards up, FALSE to flip down
    wp2:        N/A
    returns:    TRUE/FALSE

msgcInvert:
    Inverts order of cards selected in pcol
    Often used for dealing to discard.
    wp1:        N/A
    wp2:        N/A
    returns: TRUE/FALSE

msgcRemove:
    Removes cards from pcol and places them in pcolTemp (wp1)
    pcolTemp must be larger than the number of cards selected
    wp1:        pcolTemp, must be of class tcls == tclsTemp
    wp2:        N/A
    returns:    TRUE/FALSE

msgcInsert:
    Inserts all cards from pcolSrc into pcol at icrdInsAfter (wp2)  
    pcol must be large enough to accomodate the cards.
    wp1:        pcolTemp, must be of class tcls == tclsTemp
    wp2:        icrdInsAfter, card to insert after.  icrdToEnd if append to end of col
    returns:    TRUE/FALSE

msgcMove:
    Combines msgcRemove, msgcInsert, and rendering.
    Moves cards from pcolSrc into pcolDest at icrdInsAfter.  Computes new
    card positions and sends render messages to both columns
    wp1:        pcolSrc
    wp2:        icrdInsAfter, card to insert after. may be '|'ed with flags:
                    icrdInvert : Inverts order of cards
    returns:    TRUE/FALSE

msgcCopy:
    Copies pcolSrc into pcol.  pcol must be large enough
    wp1:        pcolSrc
    wp2:        fAll,  If true then entire structure is copied, else just the cards
    returns:    TRUE/FALSE

msgcValidMove:  ***** MUST BE SUPPLIED BY GAME, NO DEFAULT ACTION ******
    Determines if a move is valid.
    wp1:        pcolSrc
    wp2:        N/A
    returns:    TRUE/FALSE


msgcValidMovePt:
    Determines if a card being dragged over a column is a valid move.
    Sends msgcValidMove if card overlaps
    wp1:        pcolSrc
    wp2:        pptMouse
    returns:    icrdHit/icrdNil

msgcRender
    Renders the column starting at icrdFirst
    wp1:        icrdFirst
    wp2:        N/A
    return:    TRUE/FALSE

msgcPaint:
    Renders column if it intersects the paint update rect
    wp1:        ppaint, if NULL then renders entire column
    wp2:        N/A
    returns: TRUE/FALSE


msgcDrawOutline:
    Draws outline of cards when being dragged by mouse
    wp1:        pptMouse
    wp2:        N/A
    returns: TRUE/FALSE

msgcComputeCrdPos:
    Computes position of cards based on colcls.{dxcrdUp|dxcrdDn|dycrdUp|dycrdDn}
    wp1:        icrdFirst, first card to compute
    wp2:        N/A
    returns:    TRUE

msgcDragInvert:
    Inverts the topmost card in the pile
    wp1:        N/A
    wp2:        N/A
    returns: TRUE/FALSE

-----------------------------------------------------------------------------*/




#define icrdNil       0x1fff
#define icrdEmpty     0x1ffe

#define icrdToEnd     0x1ffd
#define ccrdToEnd     -2

#define icrdEnd       0x1ffc


/* special flags |'d with icrd for msgcMove */
#define bitFZip       0x2000
#define icrdMask      0x1fff

