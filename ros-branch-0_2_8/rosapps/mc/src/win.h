#ifndef __WIN_H
#define __WIN_H

/* Window utilities */
#include "dlg.h"

void print_bytesize (int size, int scale);
void sprint_bytesize (char *buffer, int size, int scale);

/* Labels at the screen bottom */

/* Keys managing */
int check_fkeys (int c);
int check_movement_keys (int c, int additional, int page_size, void *,
    movefn backfn, movefn forfn, movefn topfn, movefn bottomfn);
int lookup_key (char *keyname);

/* Terminal managing */
extern int xterm_flag;
void do_enter_ca_mode (void);
void do_exit_ca_mode (void);
#define wclr(w) wclrn(w, 0)

void mc_raw_mode (void);
void mc_noraw_mode (void);
void mc_init_cbreak (void);
#endif	/* __WIN_H */
