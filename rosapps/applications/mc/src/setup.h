#ifndef __SETUP_H
#define __SETUP_H

void save_layout (void);
void save_configure (void);
void load_setup (void);
void save_setup (void);
void done_setup (void);
#ifdef WANT_WIDGETS
void panel_save_setup (WPanel *panel, char *section);
void panel_load_setup (WPanel *panel, char *section);
#else
void panel_save_setup ();
void panel_load_setup ();
#endif
void load_key_defs (void);
void save_panel_types (void);
char *load_anon_passwd ();

extern char *profile_name;

extern char setup_color_string [];
extern char term_color_string [];
extern char color_terminal_string [];

extern int startup_left_mode;
extern int startup_right_mode;

#ifdef OS2_NT
#    define PROFILE_NAME     "mc.ini"
#    define HOTLIST_FILENAME "mc.hot"
#else
#    define PROFILE_NAME     ".mc/ini"
#    define HOTLIST_FILENAME ".mc/hotlist"
#endif
#endif
