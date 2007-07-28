#ifndef __NCPA_H
#define __NCPA_H

typedef LONG (CALLBACK *CPLAPPLET_PROC)(VOID);

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  CPLAPPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

extern ULONG DbgPrint(PCCH Fmt, ...);

#endif // __NCPA_H

/* EOF */
