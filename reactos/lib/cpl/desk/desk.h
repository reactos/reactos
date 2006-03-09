#ifndef __CPL_DESK_H__
#define __CPL_DESK_H__

typedef struct
{
    int idIcon;
    int idName;
    int idDescription;
    
    APPLET_PROC AppletProc;
    
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

#endif /* __CPL_DESK_H__ */

