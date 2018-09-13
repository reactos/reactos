/*
 * PLEASE NOTE: WINDISK is the file "WINDISK.CPL", this means
 *		you cannot implicit link to these APIs by simply
 *		linking with WINDISK.LIB. This is because KERNEL
 *		will only implicit link to module file names with
 *		the .DLL or .EXE extensions.
 *
 *		To use these APIs you need to LoadLibrary("WINDISK.CPL")
 *		and then use GetProcAddress.
 *
 *		  WDFMTDRVPROC lpfnFmtDrv;
 *		  HINSTANCE wdInst;
 *
 *		  wdInst = LoadLibrary("WINDISK.CPL");
 *		  if (wdInst) {
 *		    lpfnFmtDrv = (WDFMTDRVPROC)GetProcAddress(wdInst,"WinDisk_FormatDrive");
 *		    if (HIWORD(lpfnFmtDrv)) {
 *		      switch ((*lpfnFmtDrv)(hwnd, drive, WD_FMTID_DEFAULT, WD_FMT_OPT_FULL)) {
 *
 *			  case WD_FMT_ERROR:
 *
 *			  ...
 *
 *		      }
 *		      FreeLibrary(wdInst);
 *		    } else {
 *		      FreeLibrary(wdInst);
 *		      goto NoLib;
 *		    }
 *		  } else {
 *		NoLib:
 *
 *		WINDISK.LIB is provided for completeness only (and also
 *		allows you to figure out the ordinal, note that
 *		GetProcAddress "by name" is recommended however).
 */

/*
 * The WinDisk_FormatDrive API provides access to the WINDISK
 *   format dialog. This allows apps which want to format disks
 *   to bring up the same dialog that WINDISK does to do it.
 *
 *   This dialog is not sub-classable. You cannot put custom
 *   controls in it. If you want this ability, you will have
 *   to write your own front end for the DMaint_FormatDrive
 *   engine.
 *
 *   NOTE that the user can format as many diskettes in the specified
 *   drive, or as many times, as he/she wishes to. There is no way to
 *   force any specififc number of disks to format. If you want this
 *   ability, you will have to write your own front end for the
 *   DMaint_FormatDrive engine.
 *
 *   NOTE also that the format will not start till the user pushes the
 *   start button in the dialog. There is no way to do auto start. If
 *   you want this ability, you will have to write your own front end
 *   for the DMaint_FormatDrive engine.
 *
 *   PARAMETERS
 *
 *     hwnd    = The window handle of the window which will own the dialog
 *     drive   = The 0 based (A: == 0) drive number of the drive to format
 *     fmtID   = The ID of the physical format to format the disk with
 *		 NOTE: The special value WD_FMTID_DEFAULT means "use the
 *		       default format specified by the DMaint_FormatDrive
 *		       engine". If you want to FORCE a particular format
 *		       ID "up front" you will have to call
 *		       DMaint_GetFormatOptions yourself before calling
 *		       this to obtain the valid list of phys format IDs
 *		       (contents of the PhysFmtIDList array in the
 *		       FMTINFOSTRUCT).
 *     options = There is currently only one option bit defined
 *
 *		  WD_FMT_OPT_FULL
 *
 *		 The normal defualt in the WINDISK format dialog is
 *		 "Quick Format", setting this option bit indicates that
 *		 the caller wants to start with FULL format selected
 *		 (this is useful for folks detecting "unformatted" disks
 *		 and wanting to bring up the format dialog).
 *
 *		 All other bits are reserved for future expansion and
 *		 must be 0.
 *
 *		 Please note that this is a bit field and not a value
 *		 and treat it accordingly.
 *
 *   RETURN
 *	The return is either one of the WD_FMT_* values, or if the
 *	returned DWORD value is not == to one of these values, then
 *	the return is the physical format ID of the last succesful
 *	format. The LOWORD of this value can be passed on subsequent
 *	calls as the fmtID parameter to "format the same type you did
 *	last time".
 *
 */
DWORD WINAPI WinDisk_FormatDrive(HWND hwnd, WORD drive, WORD fmtID,
				 WORD options);

typedef DWORD (CALLBACK* WDFMTDRVPROC)(HWND,WORD,WORD,WORD);

//
// Special value of fmtID which means "use the default format"
//
#define WD_FMTID_DEFAULT    0xFFFF

//
// Option bits for options parameter
//
#define WD_FMT_OPT_FULL     0x0001

//
// Special return values. PLEASE NOTE that these are DWORD values.
//
#define WD_FMT_ERROR	0xFFFFFFFFL	// Error on last format, drive may be formatable
#define WD_FMT_CANCEL	0xFFFFFFFEL	// Last format was canceled
#define WD_FMT_NOFORMAT 0xFFFFFFFDL	// Drive is not formatable


/*
 * The WinDisk_CheckDrive API provides access to the WINDISK
 *   Check Disk dialog. This allows apps which want to check disks
 *   to bring up the same dialog that WINDISK does to do it.
 *
 *   This dialog is not sub-classable. You cannot put custom
 *   controls in it. If you want this ability, you will have
 *   to write your own front end for the DMaint_FixDrive
 *   engine.
 *
 *   NOTE that the check will not start till the user pushes the
 *   start button in the dialog unless the CHKOPT_AUTO option is set.
 *
 *   PARAMETERS
 *
 *     hwnd    = The window handle of the window which will own the dialog
 *     options = These options basically coorespond to the check boxes
 *		 in the Advanced Options dialog. See CHKOPT_ defines
 *		 below.
 *     DrvList = This is a DWORD bit field which indicates the 0 based
 *		 drive numbers to check. Bit 0 = A, Bit 1 = B, ...
 *		 For use on this API at least one bit must be set (if
 *		 this argument is 0, the call will return WD_CHK_NOCHK).
 *
 *   RETURN
 *	The return is either one of the WD_CHK_* values.
 *
 */
DWORD WINAPI WinDisk_CheckDrive(HWND hwnd, WORD options, DWORD DrvList);


typedef DWORD (CALLBACK* WDCHKDRVPROC)(HWND,WORD,DWORD);

//
// Special return values. PLEASE NOTE that these are DWORD values.
//
#define WD_CHK_ERROR	0xFFFFFFFFL	// Fatal Error on check
#define WD_CHK_CANCEL	0xFFFFFFFEL	// Check was canceled
#define WD_CHK_NOCHK	0xFFFFFFFDL	// At least one Drive is not "checkable"
#define WD_CHK_SMNOTFIX 0xFFFFFFFCL	// Some errors were not fixed

//
// Option bits
//
// IMPORTANT NOTE: These are set up so that the default setting is 0
//		   for all bits WITH ONE EXCEPTION. Currently the default
//		   setting has the CHKOPT_XLCPY bit set......
//
// Also note that specification of invalid combonations of bits (for example
// setting both CHKOPT_XLCPY and CHKOPT_XLDEL) will result in very random
// behavior.
//
#define CHKOPT_REP	       0x0001	// Generate detail report
#define CHKOPT_RO	       0x0002	// Run in preview mode
#define CHKOPT_NOSYS	       0x0004	// Surf Anal don't check system area
#define CHKOPT_NODATA	       0x0008	// Surf Anal don't check data area
#define CHKOPT_NOBAD	       0x0010	// Disable Surface Analysis
#define CHKOPT_LSTMF	       0x0020	// Convert lost clusters to files
#define CHKOPT_NOCHKNM	       0x0040	// Don't check file names
#define CHKOPT_NOCHKDT	       0x0080	// DOn't check date/time fields
#define CHKOPT_INTER	       0x0100	// Interactive mode
#define CHKOPT_XLCPY	       0x0200	// Def cross link resolution is COPY
#define CHKOPT_XLDEL	       0x0400	// Def cross link resolution is DELETE
#define CHKOPT_ALLHIDSYS       0x0800	// All HID SYS files are unmovable
#define CHKOPT_NOWRTTST        0x1000	// Surf Anal no write testing.
#define CHKOPT_DRVLISTONLY     0x4000	// Normaly all drives in the system
					// are shown in the drive list box
					// and those on the DrvList are selected
					// This option says put only the drives
					// in DrvList in the list box and
					// disable the control
#define CHKOPT_AUTO	       0x8000	// Auto push start button

/*
 * The WinDisk_GetBigDriveBM API provides access to the drive bitmaps
 *   WINDISK uses in its main drive dialogs.
 *
 *   These bitmaps are intended for dialogs, and are intended
 *   to have a "chart" drawn on top of them. WINDISK draws
 *   the Used/Free chart. This chart is intended to look like
 *   the disk inside the drive.
 *
 *   These bitmaps are placed on a background of COLOR_DIALOG. There
 *   is no way to change this mapping color.
 *
 *   The returned HBITMAP belongs to the calling app, it is up to the
 *   caller to call DeleteObject on it to free it.
 *
 *   PARAMETERS
 *
 *     drive   = The 0 based (A: == 0) drive number of the drive to get
 *		 the drive bitmap of.
 *     lpChrt  -> an array of 9 words whose format and meaning
 *		  depends on the returned "chart style" type
 *     options = There are currently no options defined this param should
 *		 be zero. This field is reserved for future expansion.
 *
 *   RETURN
 *	The return is 0 if the bitmap could not be loaded (memory or
 *	invalid drive).
 *
 *	If the return is non-zero, the LOWORD is an HBITMAP,
 *	and the HIWORD is a "chart style" ID which defines the
 *	format of the data placed at lpChrt and the style for
 *	the chart placed on top of the bitmap by WINDISK.
 *
 *	 USETYPE_NONE	is for never-writable drives (like CD-ROM).
 *			lpChrt data is not used.
 *	 USETYPE_ELLIPS is for circular type drives (Fixed,Floppy).
 *	 USETYPE_BARH	is a horizontal parallelogram (RAMDrive).
 *	 USETYPE_BARV	is a verticle parallelogram.
 *
 * lpChrt[0] word is the "3-D effect" height/width for the parallelogram or
 *   ellips. NOTE that the 3-D effect is disabled if the height/width
 *   is 0. Also note that with 3-D effect disabled, the parallelogram
 *   can be turned into a rectangle.
 *
 * For USETYPE_ELLIPS, the next four words (lpChrt[1],lpChrt[2],lpChrt[3]
 *   and lpChrt[4]) form a RECT structure which defines the bounding
 *   rect for the ellips (including the 3-D effect). This RECT is in
 *   coordinates of the returned bitmap (0,0 corresponds to the top
 *   leftmost pixel of the bitmap).
 *
 * For USETYPE_BARH or USETYPE_BARV the 8 words starting at lpChrt[1]
 *   are four POINT structures which define a parallelogram for the
 *   chart (NOT including the 3-D effect).
 *
 *    POINT 0 is at lpChrt[1]
 *    POINT 1 is at lpChrt[3]
 *    POINT 2 is at lpChrt[5]
 *    POINT 3 is at lpChrt[7]
 *
 *    the "3-D->" in the following indicate the sides that the 3-D effect
 *    is drawn on. And the numbers are the point array
 *    indexes.
 *
 *	 USETYPE_BARH (0.y)==(1.y) and (3.y)==(2.y)
 *
 *	       0 _____________ 1
 *		 \ 	      \
 *	    3-D-> \ 	       \
 *		   \ 		\
 *		  3 ------------- 2
 *			  ^
 *			 3-D
 *
 *	 USETYPE_BARV (0.x)==(3.x) and (1.x)==(2.x)
 *
 *		     1
 *		  /|
 *		 / |
 *		/  |
 *	     0 |   |
 *	       |   |
 *	       |   | <- 3-D
 *	       |   |
 *	       |   |
 *	       |   |
 *	       |   |
 *	       |   | 2
 *	       |  /
 *	       | / <- 3-D
 *	       |/
 *
 *	       3
 *
 */
DWORD WINAPI WinDisk_GetBigDriveBM(WORD drive, LPWORD lpChrt, WORD options);

typedef DWORD (CALLBACK* WDGETBDBMPROC)(WORD,LPWORD,WORD);

//
// HIWORD return "chart type" values
//
#define USETYPE_ELLIPS	0
#define USETYPE_BARV	1
#define USETYPE_BARH	2
#define USETYPE_NONE	3

/*
 * The WinDisk_PropSheet API provides access to the main drive
 *   dialog.
 *
 *   This API is intended for the SHELL so it can bring up a
 *   drive property sheet.
 *
 *   PARAMETERS
 *
 *     drive   = The 0 based (A: == 0) drive number of the drive to bring
 *		 up the property dialog for.
 *     hwnd    = The HWND which will own the dialog
 *     options = There are currently no options defined this param should
 *		 be zero. This field is reserved for future expansion.
 *
 *   RETURN
 *	The return is -2 if the drive number is invalid.
 *	Else the return is the return from a DialogBox call to bring
 *	up the dialog. NOTE that -1 means that the DialogBox failed.
 *
 */
int WINAPI WinDisk_PropSheet(int drive, HWND hwndpar, WORD options);

typedef int (CALLBACK* WDPROPSHEET)(int,HWND,WORD);
