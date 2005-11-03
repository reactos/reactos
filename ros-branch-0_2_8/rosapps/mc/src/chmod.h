#ifndef __CHMOD_H
#define __CHMOD_H
void chmod_cmd (void);
int stat_file (char *, struct stat *);
void ch1_cmd (int id);
void ch2_cmd (int id);

extern Dlg_head *ch_dlg;

extern umode_t c_stat;
extern char *c_fname, *c_fown, *c_fgrp, *c_fperm;
extern int c_fsize;

#endif
