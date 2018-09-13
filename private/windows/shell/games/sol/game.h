



/* game stuff */

/* GaMe struct  */
typedef struct _gm
{
	LRESULT (FAR *lpfnGmProc)(GM, INT, WPARAM, LPARAM); /* our illustrious gameproc  */
	UDR  udr;          /* undo record  */
	BOOL fDealt;       /* TRUE if cards have been dealt  */
	BOOL fInput;       /* TRUE if input has been recieved after dealing */
	BOOL fWon;         /* TRUE if game is won (and in win sequence)  */
	INT  sco;          /* da sco  */
	INT  iqsecScore;   /* # of quarter seconds since first input  */
	INT  dqsecScore;   /* # of quarter seconds betweeen decrementing score  */
	INT  ccrdDeal;     /* # of cards to deal from deck  */
	INT  irep;         /* # of times thru the deck */
	PT   ptMousePrev;  /* cache of previous mouse position */
	BOOL fButtonDown;  /* TRUE if mouse button down or kbd sel */
	INT  icolKbd;      /* Current cursor position via kbd */
	INT  icrdKbd;					
	INT  icolSel;      /* Current selection  */
	INT  icolHilight;  /* Column currently hilighted (while draggin)  */
	DY	 dyDragMax;    /* maximum height of column (for dragging)  */
	INT  icolMac;
	INT  icolMax;
	COL  *rgpcol[1];
} GM;


// Constants - earlier they were generated in the game.msg file.

#define icolNil             -1
#define msggInit            0		
#define msggEnd             1		
#define msggKeyHit          2	
#define msggMouseDown       3
#define msggMouseUp         4	
#define msggMouseMove       5
#define msggMouseDblClk     6
#define msggPaint           7		
#define msggDeal            8		
#define msggUndo            9		
#define msggSaveUndo        10	
#define msggKillUndo        11	
#define msggIsWinner        12	
#define msggWinner          13	
#define msggScore           14		
#define msggChangeScore     15
#define msggDrawStatus      16
#define msggTimer           17
#define msggForceWin        18	
#define msggMouseRightClk   19


#define ID_ICON_MAIN        500

HICON   hIconMain;              // the main freecell icon.
HICON   hImageMain;             // the main freecell image.

/* Score MoDe  */
typedef INT SMD;
#define smdStandard   ideScoreStandard
#define smdVegas      ideScoreVegas
#define smdNone       ideScoreNone


#define FSelOfGm(pgm)      ((pgm)->icolSel != icolNil)
#define FHilightOfGm(pgm)  ((pgm)->icolHilight != icolNil)



BOOL FInitKlondGm( VOID );
VOID FreeGm(GM *pgm);

#ifdef DEBUG
LRESULT SendGmMsg(GM *pgm, INT msgg, WPARAM wp1, LPARAM wp2);
#else
#define SendGmMsg(pgm, msgg, wp1, wp2) \
	(*((pgm)->lpfnGmProc))((pgm), (msgg), (wp1), (wp2))
#endif	
INT DefGmProc(GM *pgm, INT msgg, WPARAM wp1, LPARAM wp2);


/* standard change score notification codes */
/* instance specific codes should be positive  */
#define csNil     -1  /* no score change  */
#define csAbs     -2  /* change score to an absolute #  */
#define csDel     -3  /* change score by an absolute #  */
#define csDelPos  -4  /* change score by an absolute #, but don't let it get negative */


// define the virtual key constant for key a
#define  VK_A     (INT) 'A'

