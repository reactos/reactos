#ifndef MC_MOUSE_H
#define MC_MOUSE_H

#ifdef HAVE_LIBGPM

/* GPM mouse support include file */
#include <gpm.h>

#else

/* Equivalent definitions for non-GPM mouse support */
/* These lines are modified version from the lines appearing in the */
/* gpm.h include file of the Linux General Purpose Mouse server */

#define GPM_B_LEFT      4
#define GPM_B_MIDDLE    2
#define GPM_B_RIGHT     1
 
/* Xterm mouse support supports only GPM_DOWN and GPM_UP */
/* If you use others make sure your code also works without them */
enum Gpm_Etype {
  GPM_MOVE=1,
  GPM_DRAG=2,   /* exactly one in four is active at a time */
  GPM_DOWN=4,
  GPM_UP=  8,

#define GPM_BARE_EVENTS(ev) ((ev)&0xF)

  GPM_SINGLE=16,            /* at most one in three is set */
  GPM_DOUBLE=32,
  GPM_TRIPLE=64,
      
  GPM_MFLAG=128,            /* motion during click? */
  GPM_HARD=256             /* if set in the defaultMask, force an already
                              used event to pass over to another handler */
};

typedef struct Gpm_Event {
  int buttons, x, y;
  enum Gpm_Etype type;
} Gpm_Event;

extern int gpm_fd;

#endif

/* General mouse support definitions */

typedef int (*mouse_h)(Gpm_Event *, void *);

#define NO_MOUSE 0
#define GPM_MOUSE 1
#define XTERM_MOUSE 2

void init_mouse (void);
void shut_mouse (void);

/* Type of mouse: NO_MOUSE, GPM_MOUSE or XTERM_MOUSE */
extern int use_mouse_p;
/* If use_mouse_p is XTERM_MOUSE: is mouse currently active? */
extern int xmouse_flag;

int mouse_handler (Gpm_Event *gpm_event);
int redo_mouse (Gpm_Event *event);

/* Constants returned from mouse handlers */

#define MOU_NORMAL    0x00
#define MOU_REPEAT    0x01
#define MOU_ENDLOOP   0x02
#define MOU_LOCK      0x04

#ifdef DEBUGMOUSE
#define DEBUGM(data) fprintf data
#else
#define DEBUGM(data) 
#endif

#ifdef HAVE_LIBGPM

/* GPM specific mouse support definitions */
void show_mouse_pointer (int x, int y);

#else

/* Mouse support definitions for non-GPM mouse */
#define show_mouse_pointer(a,b)

#endif

#endif /* MC_MOUSE_H */
