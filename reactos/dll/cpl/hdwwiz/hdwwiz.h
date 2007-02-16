#ifndef __CPL_HDWWIZ_H
#define __CPL_HDWWIZ_H

typedef struct
{
	int idIcon;
	int idName;
	int idDescription;
	APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

#endif /* __CPL_HDWWIZ_H */

/* EOF */
