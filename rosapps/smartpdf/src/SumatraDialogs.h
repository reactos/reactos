/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#ifndef SUMATRA_PDF_DIALOGS_H_
#define SUMATRA_PDF_DIALOGS_H_

#include "SumatraPDF.h"

#define DIALOG_OK_PRESSED 1
#define DIALOG_YES_PRESSED 1
#define DIALOG_CANCEL_PRESSED 2
#define DIALOG_NO_PRESSED 3

/* For passing data to/from GetPassword dialog */
typedef struct {
    const char *  fileName;   /* name of the file for which we need the password */
    char *        pwdOut;     /* password entered by the user */
} Dialog_GetPassword_Data;

/* For passing data to/from GoToPage dialog */
typedef struct {
    int     currPageNo;      /* currently shown page number */
    int     pageCount;       /* total number of pages */
    int     pageEnteredOut;  /* page number entered by user */
} Dialog_GoToPage_Data;

/* For passing data to/from AssociateWithPdf dialog */
typedef struct {
    BOOL    dontAskAgain;
} Dialog_PdfAssociate_Data;

int     Dialog_GoToPage(WindowInfo *win);
char *  Dialog_GetPassword(WindowInfo *win, const char *fileName);
int     Dialog_PdfAssociate(HWND hwnd, BOOL *dontAskAgainOut);

#endif
