/*
 * Structure and equates of PIF files
 */

#define PIFEDITMAXPIF		1024	 /* maximum PIF file size we support */
#define PIFEDITMAXPIFL		1024L

#define PIFNAMESIZE		30
#define PIFSTARTLOCSIZE 	63
#define PIFDEFPATHSIZE		64
#define PIFPARAMSSIZE		64
#define PIFSHPROGSIZE		64
#define PIFSHDATASIZE		64

#define PIFEXTSIGSIZE		16

#define PIFSIZE 		367 /* two bytes less, this is used for checksum */

#ifndef RC_INVOKED
#pragma pack(1)
#endif
typedef struct {
    char unknown;
    char id;
    char name[PIFNAMESIZE];
    short maxmem;
    short minmem;
    char startfile[PIFSTARTLOCSIZE];
    char MSflags;
    char reserved;
    char defpath[PIFDEFPATHSIZE];
    char params[PIFPARAMSSIZE];
    char screen;
    char cPages;
    unsigned char lowVector;
    unsigned char highVector;
    char rows;
    char cols;
    char rowoff;
    char coloff;
    unsigned short sysmem;
    char shprog[PIFSHPROGSIZE];
    char shdata[PIFSHDATASIZE];
    unsigned char behavior;
    unsigned char sysflags;
    } PIFOLD286STR;
#ifndef RC_INVOKED
#pragma pack()
#endif

#ifndef RC_INVOKED
#pragma pack(1)
#endif
typedef struct {
    char extsig[PIFEXTSIGSIZE];
    unsigned short extnxthdrfloff;
    unsigned short extfileoffset;
    unsigned short extsizebytes;
    } PIFEXTHEADER;
#ifndef RC_INVOKED
#pragma pack()
#endif

#define LASTHEADERPTR		0xFFFF
#define STDHDRSIG		"MICROSOFT PIFEX"

#define W386HDRSIG		"WINDOWS 386 3.0"
#define W286HDRSIG30		"WINDOWS 286 3.0"
#define WNTHDRSIG31		"WINDOWS NT  3.1"
/*
 *#define W286HDRSIG31		  "WINDOWS 286 3.1"
 */

#ifndef RC_INVOKED
#pragma pack(1)
#endif
typedef struct {
    char unknown;
    char id;
    char name[PIFNAMESIZE];
    short maxmem;
    short minmem;
    char startfile[PIFSTARTLOCSIZE];
    char MSflags;
    char reserved;
    char defpath[PIFDEFPATHSIZE];
    char params[PIFPARAMSSIZE];
    char screen;
    char cPages;
    unsigned char lowVector;
    unsigned char highVector;
    char rows;
    char cols;
    char rowoff;
    char coloff;
    unsigned short sysmem;
    char shprog[PIFSHPROGSIZE];
    char shdata[PIFSHDATASIZE];
    unsigned char behavior;
    unsigned char sysflags;
    PIFEXTHEADER stdpifext;
    } PIFNEWSTRUCT;
#ifndef RC_INVOKED
#pragma pack()
#endif

/*
 * Windows/386 PIF file extension
 *
 */
#ifndef RC_INVOKED
#pragma pack(1)
#endif
typedef struct {
    short maxmem;
    short minmem;
    unsigned short PfFPriority;
    unsigned short PfBPriority;
    short PfMaxEMMK;
    unsigned short PfMinEMMK;
    short PfMaxXmsK;
    unsigned short PfMinXmsK;
    unsigned long PfW386Flags;
    unsigned long PfW386Flags2;
    unsigned short PfHotKeyScan;
    unsigned short PfHotKeyShVal;
    unsigned short PfHotKeyShMsk;
    unsigned char PfHotKeyVal;
    unsigned char PfHotKeyPad[9];
    char params[PIFPARAMSSIZE];
    } PIF386EXT;
#ifndef RC_INVOKED
#pragma pack()
#endif

/* Bits of PfW386Flags */
#define fEnableClose		0x00000001L
#define fBackground		0x00000002L
#define fExclusive		0x00000004L
#define fFullScreen		0x00000008L
#define fALTTABdis		0x00000020L
#define fALTESCdis		0x00000040L
#define fALTSPACEdis		0x00000080L
#define fALTENTERdis		0x00000100L
#define fALTPRTSCdis		0x00000200L
#define fPRTSCdis		0x00000400L
#define fCTRLESCdis		0x00000800L
#define fPollingDetect		0x00001000L
#define fNoHMA			0x00002000L
#define fHasHotKey		0x00004000L
#define fEMSLocked		0x00008000L
#define fXMSLocked		0x00010000L
#define fINT16Paste		0x00020000L
#define fVMLocked		0x00040000L

/* Bits of PfW386Flags2 */
/*
 *  NOTE THAT THE LOW 16 BITS OF THIS DWORD ARE VDD RELATED!!!!!!!!
 *
 *	You cannot monkey with these bits locations without breaking
 *	all VDDs as well as all old PIF files. SO DON'T MESS WITH THEM.
 *
 */
#define fVidTxtEmulate		0x00000001L
#define fVidNoTrpTxt		0x00000002L
#define fVidNoTrpLRGrfx 	0x00000004L
#define fVidNoTrpHRGrfx 	0x00000008L
#define fVidTextMd		0x00000010L
#define fVidLowRsGrfxMd 	0x00000020L
#define fVidHghRsGrfxMd 	0x00000040L
#define fVidRetainAllo		0x00000080L
/* NOTE THAT ALL OF THE LOW 16 BITS ARE RESERVED FOR VIDEO BITS */


/*
 * Windows/286 PIF file extension
 *
 *
 * Windows 3.00 extension format
 *
 */
#ifndef RC_INVOKED
#pragma pack(1)
#endif
typedef struct {
    short PfMaxXmsK;
    unsigned short PfMinXmsK;
    unsigned short PfW286Flags;
    } PIF286EXT30;
#ifndef RC_INVOKED
#pragma pack()
#endif

/* Bits of PfW286Flags */
#define fALTTABdis286		0x0001
#define fALTESCdis286		0x0002
#define fALTPRTSCdis286 	0x0004
#define fPRTSCdis286		0x0008
#define fCTRLESCdis286		0x0010
/*
 * Following bit is version >= 3.10 specific
 */
#define fNoSaveVid286		0x0020

#define fCOM3_286		0x4000
#define fCOM4_286		0x8000

/*
 *
 * NEW Windows 3.10 extension format
 *
 *#ifndef RC_INVOKED
 *#pragma pack(1)
 *#endif
 *typedef struct {
 *    short PfMaxEmsK;
 *    unsigned short PfMinEmsK;
 *    } PIF286EXT31;
 *#ifndef RC_INVOKED
 *#pragma pack()
 *#endif
 */

/* Windows NT extension format */
#ifndef RC_INVOKED
#pragma pack (1)                          
#endif
typedef struct                            
   {                                      
   DWORD dwWNTFlags;                      
   DWORD dwRes1;                          
   DWORD dwRes2;                          
   char  achConfigFile[PIFDEFPATHSIZE];   
   char  achAutoexecFile[PIFDEFPATHSIZE]; 
   } PIFWNTEXT;                           
#ifndef RC_INVOKED
#pragma pack()                            
#endif
#define PIFWNTEXTSIZE sizeof(PIFWNTEXT)


// equates for dwWNTFlags
#define NTPIF_SUBSYSMASK        0x0000000F      // sub system type mask
#define SUBSYS_DEFAULT          0
#define SUBSYS_DOS              1
#define SUBSYS_WOW              2
#define SUBSYS_OS2              3
#define COMPAT_TIMERTIC         0x10




/* behavior and sysflags */
#define SWAPS			0x20
#define SWAPMASK		0x20
#define NOTSWAPMASK		0xdf

#define PARMS			0x40
#define PARMMASK		0x40
#define NOTPARMMASK		0xbf

#define SCR			0xC0
#define SCRMASK 		0xC0
#define NOTSCRMASK		0x3f

#define MASK8087		0x20
#define NOTMASK8087		0xdf
#define KEYMASK 		0x10
#define NOTKEYMASK		0xef

/* Microsoft PIF flags */
#define MEMMASK 		0x01
#define NOTMEMMASK		0xfe

#define GRAPHMASK		0x02
#define TEXTMASK		0xfd

#define PSMASK			0x04
#define NOTPSMASK		0xfb

#define SGMASK			0x08
#define NOTSGMASK		0xf7

#define EXITMASK		0x10
#define NOTEXITMASK		0xef

#define DONTUSE 		0x20

#define COM2MASK		0x40
#define NOTCOM2MASK		0xbf

#define COM1MASK		0x80
#define NOTCOM1MASK		0x7f

#if defined(NEC_98)
#ifndef _PIFNT_NEC98_
#define _PIFNT_NEC98_
/*****************************************************************************/
/*   Windows 3.1 PIF file extension for NEC PC-9800                          */
/*****************************************************************************/
/* For Header signature. */

#define W30NECHDRSIG "WINDOWS NEC 3.0"

/* Real Extended Structire for NEC PC-9800 */

#ifndef RC_INVOKED
#pragma pack (1) 
#endif
typedef struct {
    BYTE cPlaneFlags;     
    BYTE cNecFlags;       // +1
    BYTE cVCDFlags;       // +2
    BYTE EnhExtBit;       // +3
    BYTE Extcont;         // +4 byte
    BYTE cReserved[27];   // reserved
    } PIFNECEXT;          // all = 32bytes
#ifndef RC_INVOKED
#pragma pack() 
#endif
#define PIFNECEXTSIZE sizeof(PIFNECEXT)
/*-----------------------------------------------------------------------------
  cPlaneFlags (8 bit)

         0 0 0 0 X X X X
         | | | | | | | +-- Plane 0{On/Off}
         | | | | | | +---- Plane 1{On/Off}
         | | | | | +------ Plane 2{On/Off}
         | | | | +-------- Plane 3{On/Off}
         +-+-+-+---------- Reserved for 256 color

-----------------------------------------------------------------------------*/

#define P0MASK 0x01                   /* plane 1 <ON>   */
#define NOTP0MASK 0xfe                /* plane 1 <OFF>  */

#define P1MASK 0x02                   /* plane 2 <ON>   */
#define NOTP1MASK 0xfd                /* plane 2 <OFF>  */

#define P2MASK 0x04                   /* plane 3 <ON>   */
#define NOTP2MASK 0xfb                /* plane 3 <OFF>  */

#define P3MASK 0x08                   /* plane 4 <ON>   */
#define NOTP3MASK 0xf7                /* plane 4 <OFF>  */

/*-----------------------------------------------------------------------------
    cNECFLAGS (8 bit)
 
         X 0 0 X X X X X
         | | | | | | | +-- CRTC tracer
         | | | | | | +---- screen info trans.Åo0:text /1:graph or textÅp
         | | | | | +------ N/H Dynamic1 (N?H:0 H/N:1)
         | | | | +-------- N/H Dynamic2 (H:0 N:1)
         | | | +---------- graph on window
         | +-+------------ Reserved
         +---------------- EMM large page frame
-----------------------------------------------------------------------------*/

#define CRTCMASK 0x01           /* CRTC <ON>        */
#define NOTCRTCMASK 0xfe        /* CRTC <OFF>        */

#define EXCHGMASK 0x02          /* Screen Exchange <GRPH ON>  */
#define NOTEXCHGMASK 0xfd       /* Screen Exchange <GRPH OFF> */

#define EMMLGPGMASK 0x80        /* EMM Large Page Frame <ON>  */
#define NOTEMMLGPGMASK 0x7f     /* EMM Large Page Frame <OFF> */

#define NH1MASK 0x04            /* N/H Dynamic1  <N/H> (UpdateNECScreen)*/
#define NOTNH1MASK 0xfb         /* N/H Dynamic1  <N?H> (UpdateNECScreen)*/

#define NH2MASK 0x08            /* N/H Dynamic2  < N > (UpdateNECScreen)*/
#define NOTNH2MASK 0xf7         /* N/H Dynamic2  < H > (UpdateNECScreen)*/

#define WINGRPMASK 0x10         /* door mado 1992 9 14 */
#define NOTWINGRPMASK 0xef      /*                     */

/*-----------------------------------------------------------------------------
  cVCDFlags (8 bit)

         0 0 0 0 X X X X
         | | | | | | | +-- 0/1 RS / CS
         | | | | | | +---- 0/1 Xon / Xoff
         | | | | | +------ 0/1 ER/DR
         | | | | +-------- Port(Reserved)
         | | | +---------- Port(Reserved)
         +-+-+-+---------- Reserved

------------------------------------------------------------------------------*/
#define VCDRSCSMASK 0x001                /* 0/1 RS/CS   handshake */
#define NOTVCDRSCSMASK 0xfe

#define VCDXONOFFMASK 0x02               /* 0/1 Xon/off handshake */
#define NOTVCDXONOFFMASK 0xfd

#define VCDERDRMASK 0x04                 /* 0/1 ER/DR   handshake */
#define NOTVCDERDRMASK 0xfb

/* Now Only Reserved */
                                         /* port assign */
#define VCDPORTASMASK 0x18               /* 00:none                */
#define NOTVCDPORTASMASK 0xe7            /* 01:port1Å®port2        */
                                         /* 10:port1Å®port3        */
                                         /* 11:unused              */

/*-----------------------------------------------------------------------------
  EnhExtBit (8 bit)

         X 0 0 X X X X X
         | | | | | | | +-- Mode F/F (Yes:0 No:1)
         | | | | | | +---- Display/Draw (Yes:0 No:1)
         | | | | | +------ ColorPallett (Yes:0 No:1)
         | | | | +-------- GDC (Yes:0 No:1)
         | | | +---------- Font (Yes:0 No:1)
         | +-+-+---------- Reserved
         +---------------- All is set/not(Set:1 No:0)

------------------------------------------------------------------------------*/
#define MODEFFMASK  0x01
#define NOTMODEFFMASK 0xfe

#define DISPLAYDRAWMASK 0x02            /* 0/1 Xon/off handhshake */
#define NOTDISPLAYDRAWMASK 0xfd

#define COLORPALLETTMASK 0x04           /* 0/1 ER/DR   handshake */
#define NOTCOLORPALLETTMASK 0xfb

#define GDCMASK 0x08
#define NOTGDCMASK 0xf7

#define FONTMASK 0x10
#define NOTFONTMASK 0xef

#define VDDMASK  0x80
#define NOTVDDMASK  0x7f

/*-----------------------------------------------------------------------------
  Extcont (8 bit)

        0 0 0 0 X X X X
        | | | | | | | +-- Mode F/F (8Color:0 16Color:1)
        | | | | | | +---- Reserved
        | | | | | +------ GDC TEXT (ON:1 OFF:0)
        | | | | +-------- GDC GRPH (ON:1 OFF:0)
        +-+-+-+---------- Reserved

------------------------------------------------------------------------------*/
#define  MODEFF16   0x01
#define  MODEFF8    0xfe

#define  GDCTEXTMASK    0x04
#define  NOTGDCTEXTMASK  0xfb

#define GDCGRPHMASK    0x08
#define NOTGDCGRPHMASK  0xf7

/*-----------------------------------------------------------------------------
    Reserved(8 bit)
 
         0 0 0 0 0 0 0 0
         | | | | | | | |
         +-+-+-+-+-+-+-+-- Reserved

-----------------------------------------------------------------------------*/
/*        unused        */

/*****************************************************************************/
/*  Windows NT 3.1 PIF file extension for NEC PC-9800                        */
/*****************************************************************************/
/*  For Header signature.  */

#define WNTNECHDRSIG   "WINDOWS NT31NEC"

/* Real Extended Structire for NEC PC-9800 */

#ifndef RC_INVOKED
#pragma pack (1)   
#endif
typedef struct {
 BYTE   cFontFlags;
 BYTE   cReserved[31];   // reserved
 } PIFNTNECEXT;   // all = 32bytes
#ifndef RC_INVOKED
#pragma pack()        
#endif
#define PIFNTNECEXTSIZE sizeof(PIFNTNECEXT)
/*-----------------------------------------------------------------------------
    cFontFlags (8 bit)
 
         0 0 0 0 0 0 0 X
         | | | | | | | +-- N mode compatible font(default:FALSE)
         +-+-+-+-+-+-+---- Reserved
-----------------------------------------------------------------------------*/

#define NECFONTMASK  0x01  /* NEC Font <ON>  */
#define NONECFONTMASK  0xfe  /* NEC Font <OFF>  */

/*-----------------------------------------------------------------------------
    Reserved(8 bit)[31]
 
         0 0 0 0 0 0 0 0
         | | | | | | | |
         +-+-+-+-+-+-+-+-- Reserved

-----------------------------------------------------------------------------*/
/*        unused        */

#endif // !_PIFNT_NEC98_
#endif // NEC_98
