#ifndef CONSOLE_H__
#define CONSOLE_H__

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

#endif /* CONSOLE_H__ */
