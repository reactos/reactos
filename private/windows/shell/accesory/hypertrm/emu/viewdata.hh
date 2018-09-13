/* viewdata.hh -- Common definitions for HyperACCESS/5 Viewdata
 *					 terminal emualation routines
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:28p $
 */

/* maxcol definitions */

#define VIEWDATA_COLS_40MODE	40

/* attribute definitions */

#define ALPHA_ATTR		0x01
#define MOSAIC_ATTR 	0x02
#define CONTIGUOUS_ATTR 0x03
#define SEPARATED_ATTR	0x04
#define NORMALSIZE_ATTR 0x05
#define FLASH_ATTR		0x06
#define STEADY_ATTR 	0x07
#define NEW_BACKGROUND_ATTR 0x08
#define DOUBLESIZE_ATTR 0x09
#define CONCEAL_ATTR    0x0A

/* --- view datat attribute structure --- */

typedef struct _viewdata
	{
	unsigned int attr	 : 4;	   // attribute type
	unsigned int clr	 : 4;	   // color if attribute type is color
	unsigned int smosaic : 1;	   // separated mosaics
	unsigned int cmosaic : 1;	   // contigous mosaics
	//unsigned int dblsiz  : 1;	   // double size
	} STVIEWDATA;

typedef STVIEWDATA *PSTVIEWDATA;

// Private emulator data for Viewdata.
//
typedef struct stPrivateViewdata
	{
	PSTVIEWDATA *apstVD;

	int fMosaicMode,
		fSeperatedMosaic,
		fSetAttr;

	unsigned aMapColors[7];

	} VIEWDATAPRIVATE;

typedef VIEWDATAPRIVATE *PSTVIEWDATAPRIVATE;

/* --- viewdini.c --- */

void	  EmuViewdataInit(const HHEMU hhEmu);
void	  EmuViewdataDeinstall(const HHEMU hhEmu);

/* --- viewdata.c --- */
void	  EmuViewdataAnswerback(const HHEMU hhEmu);
void	  EmuViewdataCursorLeft(const HHEMU hhEmu);
void	  EmuViewdataCursorRight(const HHEMU hhEmu);
void	  EmuViewdataCursorDown(const HHEMU hhEmu);
void	  EmuViewdataCursorUp(const HHEMU hhEmu);
void	  EmuViewdataCursorHome(const HHEMU hhEmu);
void	  EmuViewdataCursorSet(const HHEMU hhEmu);
void	  EmuViewdataSetAttr(const HHEMU hhEmu);
void	  EmuViewdataMosaicSet(const HHEMU hhEmu);
void	  EmuViewdataMosaicHold(const HHEMU hhEmu);
void	  EmuViewdataMosaicRelease(const HHEMU hhEmu);
void	  EmuViewdataCharDisplay(const HHEMU hhEmu);
int 	  EmuViewdataReset(const HHEMU hhEmu, int const fHost);

int 	  EmuViewdataKbd(const HHEMU hhEmu, int kcode, const BOOL fTest);
int 	  emuViewdataPadAttrStr(const HHEMU hhEmu, int iRow, const int iCol, const TCHAR tchar);
void	  EmuViewdataClearScreen(const HHEMU hhEmu);

/**************************** end of viewdata.hh ***************************/
