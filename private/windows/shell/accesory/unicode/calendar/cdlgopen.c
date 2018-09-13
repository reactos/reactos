/* ** This file contains routines required to display a standard open
      dialog box.  Apps can directly link to object file or modify this
      source for slightly different dialog box.

      Note - in order to use these routines, the application must
      export DlgfnOpen().  Also, an app that uses these routines must
      be running ss=ds, since they use near pointers into stack.
*/

#include "cal.h"

#define ATTRDIRLIST 0xC010  /* include directories and drives in listbox */
#define ATTRFILELIST    0x0000
#define ID_LISTBOX  10
#define ID_EDIT     11
#define CBEXTMAX    6  /* Number of bytes in "\*.txt" */

TCHAR szLastDir[120];   /* Dir where the last open occurred */
                        /* useful if file is picked up from other than current dir e.g path */
INT idEditSave;
INT idListboxSave;
INT idPathSave;
TCHAR *  szExtSave;
TCHAR *  szFileNameSave;
INT *pfpSave;
INT cbRootNameMax;

#define CCHNG       15

TCHAR   rgchNg[CCHNG] =  {TEXT('"'), TEXT('\\'), TEXT('/'), TEXT('['), TEXT(']'), TEXT(':'), TEXT('|'),
                          TEXT('<'), TEXT('>'), TEXT('+'), TEXT('='), TEXT(';'), TEXT(','), TEXT(' '), TEXT('\0')};


/*
 *  Function prototypes
 */

VOID cDlgAddCorrectExtension(TCHAR *szEdit, WORD fSearching);


/*
 *  Functions
 */

